
//Civilian Receives a query and sends back a response packet
void OnSarQueryReceived(Ptr<Socket> socket)
{
    uint8_t tos = 0xE0;
    uint32_t civilianNodeId = socket->GetNode()->GetId();

    // ---- Safety / attached checks (unchanged logic) ----
    if ((civilianSafetyStatus.count(civilianNodeId) &&
         civilianSafetyStatus[civilianNodeId]) ||
        (g_civilianAttachedState.count(civilianNodeId) &&
         g_civilianAttachedState[civilianNodeId]))
    {
        return;
    }

    Address senderAddress;
    Ptr<Packet> receivedPacket;

    //  DRAIN SOCKET
    while ((receivedPacket = socket->RecvFrom(senderAddress)))
    {
        g_activeFound = true;
        g_lastActiveTime = Simulator::Now();

        InetSocketAddress senderInetAddr = InetSocketAddress::ConvertFrom(senderAddress);
        Ipv4Address senderIp = senderInetAddr.GetIpv4();

        // ---- Safe payload extraction ----
        uint8_t buffer[256] = {0};
        uint32_t n = std::min<uint32_t>(receivedPacket->GetSize(), sizeof(buffer));
        receivedPacket->CopyData(buffer, n);
        std::string payload((char*)buffer, n);

        // ---- Extract Communication ID ----
        CommunicationIdTag commIdTag;
        uint32_t communicationId = 0;
        if (!receivedPacket->PeekPacketTag(commIdTag))
        {
            continue; // keep draining
        }
        communicationId = commIdTag.GetCommunicationId();

        uint32_t receiverId = civilianNodeId;
        uint32_t senderId = ipToNodeIdMap[senderIp];

        if (receiverId == senderId)
        {
            continue;
        }

        Ptr<Node> senderNode = NodeList::GetNode(senderId);
        Ptr<Node> receiverNode = NodeList::GetNode(receiverId);

        Vector senderPos = senderNode->GetObject<MobilityModel>()->GetPosition();
        Vector receiverPos = receiverNode->GetObject<MobilityModel>()->GetPosition();
        double dist = CalculateDistance(senderPos, receiverPos, true);

        if (dist > maxReceptionDistance[receiverId])
        {
            maxReceptionDistance[receiverId] = dist;
        }

        // ---- Log received query ----
        uint32_t packetId = receivedPacket->GetUid();
        LogPacketEvent(
            socket,
            socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
            senderIp,
            packetId,
            0.0,
            0.0,
            receivedPacket->GetSize(),
            SARcivilianqueryport,
            "SARQuery Received",
            communicationId,"RX"
        );

        // ---- Prepare QueryResponse ----
        uint32_t newCommunicationId = 70000 + receiverId + senderId;

        StartCommunicationSession(
            newCommunicationId,
            socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
            senderIp,
            "Civilian QueryResponse Session",
            Simulator::Now().GetSeconds(),
            "UDP",
            QueryResponsePort
        );

        std::ostringstream responsePayloadStream;
        responsePayloadStream << "QueryResponse;Node-" << receiverId;
        std::string responsePayload = responsePayloadStream.str();

        Ptr<Packet> responsePacket =
            Create<Packet>((uint8_t*)responsePayload.c_str(), responsePayload.size());

        CommunicationIdTag responseCommIdTag(newCommunicationId);
        responsePacket->AddPacketTag(responseCommIdTag);

        NodeIdTag tag;
        tag.SetNodeId(civilianNodeId);
        responsePacket->AddPacketTag(tag);

        // ---- SEND RESPONSE (cached send socket, NO Bind) ----
        Ptr<Socket> responseSocket = GetCachedUdpSocket(socket->GetNode(), tos);
        if (!responseSocket)
        {
            continue;
        }

        responseSocket->SetIpTos(tos);
        responseSocket->SendTo(responsePacket, 0,
                               InetSocketAddress(senderIp, QueryResponsePort));

        LogPacketEvent(
            responseSocket,
            socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
            senderIp,
            responsePacket->GetUid(),
            0.0,
            0.0,
            responsePacket->GetSize(),
            QueryResponsePort,
            "Civilian QueryResponse Sent",
            newCommunicationId,"TX"
        );
    }
}


//SAR RECIEVES QUERY RESPONSE AND TRIGGERS DISCOVERY
void OnQueryResponseReceived(Ptr<Socket> socket)
{
    Address senderAddress;
    Ptr<Packet> receivedPacket;

    //  DRAIN SOCKET
    while ((receivedPacket = socket->RecvFrom(senderAddress)))
    {
        InetSocketAddress senderInetAddr = InetSocketAddress::ConvertFrom(senderAddress);
        Ipv4Address senderIp = senderInetAddr.GetIpv4();

        // ---- Safe payload extraction ----
        uint8_t buffer[256] = {0};
        uint32_t n = std::min<uint32_t>(receivedPacket->GetSize(), sizeof(buffer));
        receivedPacket->CopyData(buffer, n);
        std::string payload((char*)buffer, n);

        std::istringstream payloadStream(payload);
        std::string signalType, senderNode;
        std::getline(payloadStream, signalType, ';');
        std::getline(payloadStream, senderNode, ';');

        if (signalType != "QueryResponse")
        {
            continue;
        }

        uint32_t receiverId = socket->GetNode()->GetId();
        uint32_t senderId = ipToNodeIdMap[senderIp];

        CommunicationIdTag commIdTag;
        uint32_t communicationId = 0;
        if (!receivedPacket->PeekPacketTag(commIdTag))
        {
            continue;
        }
        communicationId = commIdTag.GetCommunicationId();

        if (receiverId == senderId)
        {
            continue;
        }

        double receiveTime = Simulator::Now().GetSeconds();
        uint32_t packetId = receivedPacket->GetUid();

        double delay = 0.0;
        double throughput = 0.0;

        auto itTs = sendTimestamps.find(packetId);
        if (itTs != sendTimestamps.end())
        {
            delay = receiveTime - itTs->second;
            if (delay > 1e-9)
            {
                throughput = (receivedPacket->GetSize() * 8.0) / delay;
            }
            sendTimestamps.erase(itTs);
        }

        if (activeSessions.count(communicationId))
        {
            activeSessions[communicationId].destination = senderIp;
        }

        LogPacketEvent(
            socket,
            socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
            senderIp,
            packetId,
            delay,
            throughput,
            receivedPacket->GetSize(),
            QueryResponsePort,
            "Civilian Query Response Received",
            communicationId,"RX"
        );

        Ptr<Node> senderNodePtr = NodeList::GetNode(senderId);
        Ptr<Node> receiverNodePtr = NodeList::GetNode(receiverId);

        Vector senderPos = senderNodePtr->GetObject<MobilityModel>()->GetPosition();
        Vector receiverPos = receiverNodePtr->GetObject<MobilityModel>()->GetPosition();
        double distance = CalculateDistance(senderPos, receiverPos, true);

        if (distance > maxReceptionDistance[receiverId])
        {
            maxReceptionDistance[receiverId] = distance;
        }

        signalLogFile << receiveTime << ",Received,CivilianQueryResponse,"
                      << socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal() << ","
                      << senderIp << "," << receivedPacket->GetSize() << ","
                      << "SenderNode: Node-" << socket->GetNode()->GetId() << ","
                      << distance << ","
                      << senderPos.x << "," << senderPos.y << "," << senderPos.z << ","
                      << "Port:" << QueryResponsePort << "\n";

        ProcessDiscoveryEvent(
            receiverId,
            senderId,
            distance,
            "Query Response",
            senderPos,
            socket,
            g_baseNode,
            discoveryPort
        );

        discoveryState[receiverId][senderId] = true;
    }
}

// Set up Hybrid nodes to listen and respond to SAR queries
// Binds sockets to the specified port and assigns a callback
void SetupHybridModeForCivilians(NodeContainer& civilianNodesquery , uint16_t SARcivilianqueryport) {
    for (uint32_t i = 0; i < civilianNodesquery .GetN(); ++i) {
        Ptr<Node> civilianNode = civilianNodesquery .Get(i);
        Ptr<Socket> socket = Socket::CreateSocket(civilianNode, TypeId::LookupByName("ns3::UdpSocketFactory"));
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), SARcivilianqueryport);

        if (socket->Bind(local) == -1) {
            std::cerr << "Failed to bind socket for civilian node " << civilianNode->GetId()
                      << " on port " << SARcivilianqueryport << std::endl;
        } else {
            std::cout << "Socket bound successfully for civilian node " << civilianNode->GetId()
                      << " on port " << SARcivilianqueryport << std::endl;
        }

        socket->SetRecvCallback(MakeCallback(&OnSarQueryReceived));  // Callback for SAR queries

      
    }
}


struct SarBroadcastQuery {
    uint32_t sarNodeId;       // Node ID of the SAR node
    Ipv4Address originalSarIp; // IP address of the original SAR node
};
// Broadcast queries from SAR nodes
// Periodically sends discovery queries to discover Hybrid nodes
void BroadcastQuery(Ptr<Socket> socket, ns3::Ipv4Address broadcastAddress, uint16_t queryPort, double interval, uint32_t communicationId) {

    Ptr<Node> node = socket->GetNode();
uint32_t senderNodeId = node->GetId();

    for (uint32_t i = 0; i < civilianNodesquery.GetN(); ++i)
    {
        Ptr<Node> node = civilianNodesquery.Get(i);
        uint32_t civilianNodeId = node->GetId();  // Extract node ID
    
        // Check if the civilian is already marked safe
        if (civilianSafetyStatus.find(civilianNodeId) != civilianSafetyStatus.end() &&
            civilianSafetyStatus[civilianNodeId] == true)
        {
            // NS_LOG_INFO("Civilian " << civilianNodeId << " is already safe. Skipping QUERY.");
            continue;
        }
    
        // Check if the civilian is already attached
        if (g_civilianAttachedState.find(civilianNodeId) != g_civilianAttachedState.end() &&
            g_civilianAttachedState[civilianNodeId])
        {
            // NS_LOG_INFO("Civilian " << civilianNodeId << " is already attached. Skipping QUERY.");
            continue;
        }
    
        // Civilian is not safe or attached, proceed with sending query
        NS_LOG_INFO("SAR is SENDING QUERY to Civilian " << civilianNodeId);
    }
    
    uint8_t tos= 0xE0 ;
    // Step 1: Get the SAR node's IP address
    Ipv4Address sarIp = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // SAR node's IP
    
    // Convert Ipv4Address to string
    std::ostringstream sarIpStream;
    sarIpStream << sarIp;
    std::string sarIpString = sarIpStream.str();

    // Build the query payload
    std::string payload = "Query;SARNode-" + std::to_string(socket->GetNode()->GetId()) + ";SARIP-" + sarIpString;
    Ptr<ns3::Packet> queryPacket = Create<Packet>((uint8_t*)payload.c_str(), payload.size());
    socket->SetIpTos(tos);
    ns3::InetSocketAddress remote = ns3::InetSocketAddress(broadcastAddress, queryPort);

    // Step 2: Attach the communication ID tag
    CommunicationIdTag commIdTag(communicationId);
    queryPacket->AddPacketTag(commIdTag);
    NodeIdTag tag;
    tag.SetNodeId(senderNodeId);
    queryPacket->AddPacketTag(tag);
    // Step 3: Track the packet for debugging and logging
    uint32_t packetId = queryPacket->GetUid();
    double sendTime = Simulator::Now().GetSeconds();
    sendTimestamps[packetId] = sendTime;

    // Step 4: Send the query packet
    socket->SendTo(queryPacket, 0, remote);

    // Step 5: Debugging and logging
    // std::cout << "Query broadcast sent from Node-" << socket->GetNode()->GetId()
    //           << " on port " << queryPort << " to broadcast address " << broadcastAddress
    //           << " with SAR IP: " << sarIpString << std::endl;

    signalLogFile << Simulator::Now().GetSeconds() << ",Sent,QueryBroadcast,"
                  << sarIpString << "," << broadcastAddress << "," << queryPacket->GetSize() << " bytes,"
                  << "SenderNode: Node-" << socket->GetNode()->GetId() << ","
                  << "not applicable,"
                  << "LOCATION placeholder" << "," << "LOCATION placeholder" << "," << "LOCATION placeholder"
                  << ",Port: " << queryPort << std::endl;

    LogPacketEvent(
        socket,
        sarIp,                                // Source IP
        broadcastAddress,                     // Destination IP
        packetId,
        0.0,                                  // Placeholder for delay
        0.0,                                  // Placeholder for throughput
        queryPacket->GetSize(),
        SARcivilianqueryport,                 // Packet size
        "SARQUERY Sent",                      // Packet type
        communicationId,"TX"
    );

    // Step 6: Reschedule the next broadcast
    Simulator::Schedule(Seconds(interval), &BroadcastQuery, socket, broadcastAddress, queryPort, interval, communicationId);
      signalLogFile << Simulator::Now().GetSeconds() << ",Rescheduled,QueryBroadcast,"
                  << sarIpString << "," << broadcastAddress << "," << queryPacket->GetSize() << " bytes,"
                  << "SenderNode: Node-" << socket->GetNode()->GetId() << ","
                  << "not applicable,"
                  << "LOCATION placeholder" << "," << "LOCATION placeholder" << "," << "LOCATION placeholder"
                  << ",Port: " << queryPort << std::endl;

}


