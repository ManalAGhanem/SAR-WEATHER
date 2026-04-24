
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

    // ✅ DRAIN SOCKET
    while ((receivedPacket = socket->RecvFrom(senderAddress)))
    {
        g_activeFound = true;
        g_lastActiveTime = Simulator::Now();

        InetSocketAddress senderInetAddr = InetSocketAddress::ConvertFrom(senderAddress);
        Ipv4Address senderIp = senderInetAddr.GetIpv4();
       // double receiveTime = Simulator::Now().GetSeconds();

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

// void OnSarQueryReceived(Ptr<Socket> socket) {
//     uint8_t tos=0xE0 ;
//     uint32_t civilianNodeId = socket->GetNode()->GetId();

//     if (
//         // 1) Civilian is marked safe
//         (civilianSafetyStatus.find(civilianNodeId) != civilianSafetyStatus.end() &&
//          civilianSafetyStatus[civilianNodeId] == true)
    
        
//     )
//     {
//    // NS_LOG_INFO("Civilian " << civilianNodeId << " is already safe. Skipping QUERYRESPONSE.");
//     return; // No further discovery needed.
// }


// else if (    // 2) Civilian is in g_civilianAttachedState AND is attached (== true)
//         (g_civilianAttachedState.find(civilianNodeId) != g_civilianAttachedState.end() &&
//          g_civilianAttachedState[civilianNodeId]))
         
//          {
//         //    NS_LOG_INFO("Civilian " << civilianNodeId << " is already ATTACHED. Skipping QUERYRESPONSE.");
//             return; // No further discovery needed.
//         }

// else{            NS_LOG_INFO("Civilian " << civilianNodeId << " is sending  QUERYRESPONSE.");
// }
//     Address senderAddress;
//     Ptr<Packet> receivedPacket = socket->RecvFrom(senderAddress);

//     if (!receivedPacket) {
//         std::cerr << "Error: No packet received on Node-" << socket->GetNode()->GetId() << std::endl;
//         return;
//     }

//     // Mark that we found something via active discovery
//     g_activeFound = true;
//     g_lastActiveTime = Simulator::Now();

//     // Extract sender IP address
//     InetSocketAddress senderInetAddr = InetSocketAddress::ConvertFrom(senderAddress);
//     Ipv4Address senderIp = senderInetAddr.GetIpv4();
//     double receiveTime = Simulator::Now().GetSeconds();

//     // Extract and decode the payload
//     uint8_t buffer[256] = {0};
//     receivedPacket->CopyData(buffer, receivedPacket->GetSize());
//     std::string payload = std::string((char*)buffer, receivedPacket->GetSize());

//     // Print the packet for debugging purposes
//     std::ostringstream oss;
//     receivedPacket->Print(oss);
//    // std::cout << "Received Packet (UID: " << receivedPacket->GetUid() << ") Content: " << oss.str() << std::endl;

//     // Extract the communication ID tag
//     CommunicationIdTag commIdTag;
//     uint32_t communicationId = 0;
//     if (receivedPacket->PeekPacketTag(commIdTag)) {
//         communicationId = commIdTag.GetCommunicationId();
//        // std::cout << "Retrieved Communication ID Tag: " << commIdTag.GetCommunicationId() << std::endl;
//     } else {
//        // std::cerr << "Warning: Communication ID tag missing from packet." << std::endl;
//         return; // Exit early if the tag is missing
//     }


//     uint32_t receiverId = civilianNodeId;
//     uint32_t senderId = ipToNodeIdMap[senderIp];
//     Ptr<Node> senderNode = NodeList::GetNode(senderId);
//     Ptr<Node> receiverNode = NodeList::GetNode(receiverId);
//     Ptr<MobilityModel> senderMob = senderNode->GetObject<MobilityModel>();
//     Ptr<MobilityModel> receiverMob = receiverNode->GetObject<MobilityModel>();
//     Vector senderPos = senderMob->GetPosition();
//     Vector receiverPos = receiverMob->GetPosition();
//     double dist = CalculateDistance(senderPos, receiverPos, true);

//     if (dist > maxReceptionDistance[receiverId]) {
//         maxReceptionDistance[receiverId] = dist;
//     }
//     // Log the received query
//     uint32_t packetId = receivedPacket->GetUid();
//     double delay = (sendTimestamps.count(packetId) > 0) ? (receiveTime - sendTimestamps[packetId]) : 0.0;
//     double throughput = (delay > 0) ? (receivedPacket->GetSize() * 8) / delay : 0.0;
//     LogPacketEvent(
//         socket,
//         socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Destination IP
//         senderIp,                                                         // Source IP
//         packetId,
//         delay,                                                            // Delay
//         throughput,                                                       // Throughput
//         receivedPacket->GetSize(),                                        // Packet size
//         SARcivilianqueryport,
//         "SARQuery Received",                                              // Packet type
//         communicationId
//     );

//     // Extract SAR node ID and prepare response
//   //  uint32_t receiverId = socket->GetNode()->GetId();
//     //uint32_t senderId = ipToNodeIdMap[senderIp];

//     // Ensure sender and receiver IDs are valid
//     if (receiverId == senderId) {
//         std::cerr << "Error: Sender and receiver are the same node." << std::endl;
//         return;
//     }

//     // Generate a unique communication ID for the new session
//     uint32_t newCommunicationId = 70000 + receiverId  + senderId;

//     // Start a new communication session
//     StartCommunicationSession(
//         newCommunicationId,
//         socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Source IP (local node)
//         senderIp,                                                         // Destination IP (SAR node)
//         "Civilian QueryResponse Session",
//         Simulator::Now().GetSeconds(),                                    // Start time
//         "UDP",                                                            // Protocol
//         QueryResponsePort                                                 // Port
//     );

//     // Prepare response payload
//     std::ostringstream responsePayloadStream;
//     responsePayloadStream << "QueryResponse;Node-" << receiverId;
//     std::string responsePayload = responsePayloadStream.str();
//     Ptr<Packet> responsePacket = Create<Packet>((uint8_t*)responsePayload.c_str(), responsePayload.size());

//     // Attach communication ID to response packet
//     CommunicationIdTag responseCommIdTag(newCommunicationId);
//     responsePacket->AddPacketTag(responseCommIdTag);

//     // Create a new socket for sending the response
//     //uint8_t tos = 0xE0;
//         Ptr<Socket> responseSocket = GetCachedUdpSocket(socket->GetNode(), tos);
//         if (!responseSocket) { return; }
//             // Ptr<Socket> responseSocket = Socket::CreateSocket(socket->GetNode(), UdpSocketFactory::GetTypeId());
//     // responseSocket->SetIpTos(tos);
//     InetSocketAddress responseLocal = InetSocketAddress(Ipv4Address::GetAny(), 0);
//     if (responseSocket->Bind(responseLocal) == -1) {
//         std::cerr << "Error: Failed to bind response socket for Node-" << receiverId << std::endl;
//         return;
//     }

//     // Send the response packet to the original SAR node
//     uint32_t responsePacketId = responsePacket->GetUid();
//     // std::cout << "Sending QueryResponse from Node-" << receiverId
//     //           << " to SAR Node-" << senderIp
//     //           << " on port " << QueryResponsePort << std::endl;
//     NodeIdTag tag;
//     tag.SetNodeId(civilianNodeId);
//     responsePacket->AddPacketTag(tag);
//     int status = responseSocket->SendTo(responsePacket, 0, InetSocketAddress(senderIp, QueryResponsePort));
//     if (status == -1) {
//         std::cerr << "Error: Failed to send QueryResponse from Node-" << receiverId
//                   << " to SAR Node-" << senderIp << std::endl;
//         return;
//     }

//     std::cout << "QueryResponse successfully sent from Node-" << receiverId
//               << " to SAR Node-" << senderIp
//               << " with Packet ID: " << responsePacketId
//               << " and Communication ID: " << newCommunicationId << std::endl;

//     // Log the response in the detailed packet log
//     LogPacketEvent(
//         responseSocket,
//         socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Source IP
//         senderIp,                                                         // Destination IP
//         responsePacketId,
//         0.0,                                                              // Placeholder for delay
//         0.0,                                                              // Placeholder for throughput
//         responsePacket->GetSize(),                                        // Packet size
//         QueryResponsePort,
//         "Civilian QueryResponse Sent",                                    // Packet type
//         newCommunicationId
//     );
// }


//SAR RECIEVES QUERY RESPONSE AND TRIGGERS DISCOVERY
void OnQueryResponseReceived(Ptr<Socket> socket)
{
    Address senderAddress;
    Ptr<Packet> receivedPacket;

    // ✅ DRAIN SOCKET
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

// void OnQueryResponseReceived(Ptr<Socket> socket) {
//     Address senderAddress;
//     Ptr<Packet> receivedPacket = socket->RecvFrom(senderAddress);

//     if (!receivedPacket) {
//         std::cerr << "Error: No packet received." << std::endl;
//         return;
//     }

//     // Convert packet to string
//     uint8_t buffer[256] = {0};
//     receivedPacket->CopyData(buffer, receivedPacket->GetSize());
//     std::string payload = std::string((char*)buffer, receivedPacket->GetSize());

//     // Parse the payload
//     std::string signalType = "Unknown", senderNode = "Unknown";
//     std::istringstream payloadStream(payload);
//     std::getline(payloadStream, signalType, ';');
//     std::getline(payloadStream, senderNode, ';');

//     if (signalType != "QueryResponse") {
//         std::cout << "Ignoring non-QueryResponse packet with SignalType: " << signalType << std::endl;
//         return;
//     }

//     InetSocketAddress senderInetAddr = InetSocketAddress::ConvertFrom(senderAddress);
//     Ipv4Address senderIp = senderInetAddr.GetIpv4();
//     uint32_t receiverId = socket->GetNode()->GetId();
//     uint32_t senderId = ipToNodeIdMap[senderIp];

//     // Extract the communication ID tag
//     CommunicationIdTag commIdTag;
//     uint32_t communicationId = 0;
//     if (receivedPacket->PeekPacketTag(commIdTag)) {
//         communicationId = commIdTag.GetCommunicationId();
//        // std::cout << "sssRetrieved Communication ID Tag: " << commIdTag.GetCommunicationId() << std::endl;
//     } else {
//       //  std::cerr << "Warning: Communication ID tag missing from packet." << std::endl;
//         return;
//     }

//     // Ignore packets sent by SAR nodes
//     if (receiverId == senderId) {
//        // std::cout << "Ignoring packet from self or another SAR node." << std::endl;
//         return;
//     }

//     double receiveTime = Simulator::Now().GetSeconds();
//     // uint32_t packetId = receivedPacket->GetUid();
//     // double delay = (sendTimestamps.count(packetId) > 0) ? (receiveTime - sendTimestamps[packetId]) : 0.0;
//     // double throughput = (delay > 0) ? (receivedPacket->GetSize() * 8) / delay : 0.0;
// uint32_t packetId = receivedPacket->GetUid();

// // SAFE lookup (no insertion)
// double delay = 0.0;
// double throughput = 0.0;

// auto itTs = sendTimestamps.find(packetId);
// if (itTs != sendTimestamps.end())
// {
//     delay = receiveTime - itTs->second;
//     if (delay > 1e-9)
//     {
//         throughput = (receivedPacket->GetSize() * 8.0) / delay;
//     }
//     sendTimestamps.erase(itTs); // ✅ critical: free memory
// }

//     // Update session metrics
//     if (activeSessions.find(communicationId) != activeSessions.end()) {
//         CommunicationSession &session = activeSessions[communicationId];
//         session.destination = senderIp;
//     }

//     // Log packet event
//     LogPacketEvent(
//         socket,
//          socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Destination IP
//         senderIp,                                                         // Source IP
//         packetId,
//         delay,                                                           // Delay
//         throughput,                                                      // Throughput
//         receivedPacket->GetSize(),                                       // Packet size
//         QueryResponsePort,
//         "Civilian Query Response Received",                              // Packet type
//         communicationId
//     );

//     // Calculate distance between sender and receiver
//     Ptr<Node> senderNodePtr = NodeList::GetNode(senderId);
//     Ptr<Node> receiverNodePtr = NodeList::GetNode(receiverId);
//     Ptr<MobilityModel> senderMob = senderNodePtr->GetObject<MobilityModel>();
//     Ptr<MobilityModel> receiverMob = receiverNodePtr->GetObject<MobilityModel>();
//     Vector senderPos = senderMob->GetPosition();
//     Vector receiverPos = receiverMob->GetPosition();
//     double distance = CalculateDistance(senderPos, receiverPos, true);

//     // Update maximum reception distance
//     if (distance > maxReceptionDistance[receiverId]) {
//         maxReceptionDistance[receiverId] = distance;
//     }

//     // Log the response in the signal log
//     signalLogFile << receiveTime << ",Received,CivilianQueryResponse,"
//                   << socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal() << ","
//                   << senderIp << "," << receivedPacket->GetSize() << " bytes,"
//                   << "SenderNode: Node-" << socket->GetNode()->GetId() << ","
//                   << distance << ","
//                   << senderPos.x << ", " << senderPos.y << ", " << senderPos.z << ","
//                   << "Port: " << QueryResponsePort << std::endl;

//     // std::cout << receiveTime << " - Civilian Query Response received by Node-" << socket->GetNode()->GetId()
//     //           << " from Node-" << senderIp << " with Location: ("
//     //           << senderPos.x << ", " << senderPos.y << ", " << senderPos.z << ")" << std::endl;

//     // Process discovery event
//     if (signalType == "QueryResponse") {
//         ProcessDiscoveryEvent(
//             receiverId,    // Discoverer (SAR node)
//             senderId,      // Discovered (Civilian node)
//             distance,
//             "Query Response",
//             senderPos,     // Civilian location
//             socket,
//             g_baseNode,
//             discoveryPort
//         );

//         discoveryState[receiverId][senderId] = true;

        
//     }
// }

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


