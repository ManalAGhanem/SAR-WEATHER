// Send proactive signals from PLB and WiFi nodes
// Periodically sends a signal packet to the specified receiver
void SendSignalData(Ptr<Socket> socket, Ipv4Address receiverAddress, uint16_t port, 
                    std::string senderNode, std::string signalType, double x, double y, double z, double interval, uint32_t communicationId ) {
    Ptr<Node> node = socket->GetNode();
    uint32_t senderNodeId = node->GetId();
   // uint32_t nodeId = socket->GetNode()->GetId(); // Get the ID of the sender node

    // Check if the node is allowed to send
    // if (g_civilianAttachedState.find(nodeId) != g_civilianAttachedState.end() && 
    //     !g_civilianAttachedState[nodeId]) 
    // {
    //     //NS_LOG_INFO("Civilian Node " << nodeId << " is attached. Signal sending stopped.");
    //     return; // Do not send if the node is attached
    // }
    // Format payload as "SignalType;SenderNode;X;Y;Z"

    uint8_t tos= 0xE0 ;
    uint32_t civilianNodeId = socket->GetNode()->GetId();

    if (
        // 1) Civilian is marked safe
        (civilianSafetyStatus.find(civilianNodeId) != civilianSafetyStatus.end() &&
         civilianSafetyStatus[civilianNodeId] == true)
    
        
    )
    {
   // NS_LOG_INFO("Civilian " << civilianNodeId << " is already safe. Skipping SENDING SIGNAL.");
    return; // No further discovery needed.
}
else if (    // 2) Civilian is in g_civilianAttachedState AND is attached (== true)
        (g_civilianAttachedState.find(civilianNodeId) != g_civilianAttachedState.end() &&
         g_civilianAttachedState[civilianNodeId]))
         
         {
        //    NS_LOG_INFO("Civilian " << civilianNodeId << " is already ATTACHED. Skipping SENDING SIGNAL.");
            return; // No further discovery needed.
        }
else{            NS_LOG_INFO("Civilian " << civilianNodeId << " is  SENDING SIGNAL.");
}
    std::ostringstream payloadStream;
    payloadStream << signalType << ";" << senderNode << ";" 
                  << std::fixed << x << ";" << y << ";" << z;  // Ensure fixed-point formatting for consistency
    std::string payload = payloadStream.str();

    // Debug log for payload
  //  std::cout << "Payload constructed: " << payload << std::endl;

    // Create the packet with the payload
    Ptr<Packet> packet = Create<Packet>((uint8_t*)payload.c_str(), payload.size());
    socket->SetIpTos(tos);
    InetSocketAddress remote = InetSocketAddress(receiverAddress, port);
    // Attach the communication ID tag
    CommunicationIdTag commIdTag(communicationId);
    packet->AddPacketTag(commIdTag);
    NodeIdTag tag;
    tag.SetNodeId(senderNodeId); 
    packet->AddPacketTag(tag);
      uint32_t packetId = packet->GetUid();
    double sendTime = Simulator::Now().GetSeconds();
    sendTimestamps[packetId] = sendTime;

    socket->SendTo(packet, 0, remote);
    
    uint32_t signalport = (signalType == "PLB" ? 9100 : 9200);

    LogPacketEvent(
        socket,
        socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Source IP
        receiverAddress,                                                  // Destination IP
        packetId,
        0.0,                                                              // Placeholder for delay
        0.0,                                                              // Placeholder for throughput
        packet->GetSize(),                                                // Packet size
        signalport,
        signalType + " Sent",                                                       // Packet type
        communicationId,"TX"    );

    // Log the sent signal
    signalLogFile << sendTime << ",Sent," << signalType << ","
                  << socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal() << ","
                  << receiverAddress << "," << packet->GetSize() << " bytes,"
                  << "SenderNode: " << senderNode << ","
                  << "NOT APPLICABLE,"
                  << x << ", " << y << ", " << z << ","
                  << "Port: " << port << std::endl;

    // Console output for debugging
    // std::cout << sendTime << " - " << signalType << " signal sent from Node " 
    //           << senderNode << " to " << receiverAddress << " with Location: (" 
    //           << x << ", " << y << ", " << z << ")" << std::endl;


    // std::cout << "Payload constructed in SendSignalData: " << payload << std::endl;

    //  schedule the next signal
    Simulator::Schedule(Seconds(interval), &SendSignalData, socket, receiverAddress, port, 
                        senderNode, signalType, x, y, z, interval,communicationId);
}
// Logs the reception of signals

void OnSignalReceived(Ptr<Socket> socket) {
    Address senderAddress;
    Ptr<Packet> receivedPacket = socket->RecvFrom(senderAddress);

    if (receivedPacket) {
         // Mark that we found something via active discovery
        g_activeFound = true;
        g_lastActiveTime = Simulator::Now();
        // Extract sender information
        InetSocketAddress senderInetAddr = InetSocketAddress::ConvertFrom(senderAddress);
        Ipv4Address senderIp = senderInetAddr.GetIpv4();
        double receiveTime = Simulator::Now().GetSeconds(); // Add receive time for logging
        uint32_t packetId = receivedPacket->GetUid();

            // Extract the communication ID tag
      CommunicationIdTag commIdTag;
    uint32_t communicationId = 0;
    if (receivedPacket->PeekPacketTag(commIdTag)) {
        communicationId = commIdTag.GetCommunicationId();
       // std::cout << "Retrieved Communication ID Tag: " << commIdTag.GetCommunicationId() << std::endl;

    } else {
       // std::cerr << "Warning: Communication ID tag missing from packet." << std::endl;
        return;  // Exit early if the tag is missing
    }
      // Extract and decode the payload
    uint8_t buffer[256] = {0};
    receivedPacket->CopyData(buffer, receivedPacket->GetSize());
    std::string payload = std::string((char*)buffer, receivedPacket->GetSize());

    // Debug: Log raw payload
   // std::cout << "plb Raw payload received: " << payload << std::endl;

    // Parse the payload to extract the SignalType and other fields
    std::string signalType = "Unknown";
    std::string senderNode = "Unknown";
    double x = 0.0, y = 0.0, z = 0.0; // Coordinates (if included in payload)

    // Use std::istringstream to parse the payload
    std::istringstream payloadStream(payload);
        std::getline(payloadStream, signalType, ';');
        if (signalType != "PLB" ) {
            std::cerr << "Unexpected SignalType: " << signalType << std::endl;
        }
            std::getline(payloadStream, senderNode, ';'); // Extract SenderNode (if applicable)




double delay = -1.0;
double throughputBps = 0.0;

auto itTs = sendTimestamps.find(packetId);
if (itTs != sendTimestamps.end())
{
    const double sendTime = itTs->second;
    delay = Simulator::Now().GetSeconds() - sendTime;

    const double dt = receiveTime - sendTime;
    if (dt > 1e-9) // avoid division by 0
    {
        throughputBps = (receivedPacket->GetSize() * 8.0) / dt;
    }

    //  CRITICAL FIX: free memory as soon as we’ve used it
    sendTimestamps.erase(itTs);
}


     // Update session metrics
     if (activeSessions.find(communicationId) != activeSessions.end()) {
         CommunicationSession &session = activeSessions[communicationId];
    
         session.destination = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();


     }            uint32_t signalport = (signalType == "PLB" ? 9100 : 9200);

            LogPacketEvent(
                socket,
                socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Destination IP
                senderIp,                                                         // Source IP
                packetId,
                 delay,                          // Delay
               throughputBps, // Throughput
                receivedPacket->GetSize(),                                        // Packet size
                signalport,
                signalType +" Recievd",                                                // Packet type
                communicationId,
                "RX"
            );
           
        // Parse x, y, and z coordinates
            std::string coordinateStr;
            std::getline(payloadStream, coordinateStr, ';');
            x = std::stod(coordinateStr);  // Convert string to double
            std::getline(payloadStream, coordinateStr, ';');
            y = std::stod(coordinateStr);  // Convert string to double
            std::getline(payloadStream, coordinateStr, ';');
            z = std::stod(coordinateStr);  // Convert string to double


     
        // Calculate distance between sender and receiver
        uint32_t receiverId = socket->GetNode()->GetId();
        uint32_t senderId = ipToNodeIdMap[senderIp];

        Ptr<Node> senderNodePtr = NodeList::GetNode(senderId);
        Ptr<Node> receiverNodePtr = NodeList::GetNode(receiverId);
        Ptr<MobilityModel> senderMob = senderNodePtr->GetObject<MobilityModel>();
        Ptr<MobilityModel> receiverMob = receiverNodePtr->GetObject<MobilityModel>();
        Vector senderPos = senderMob->GetPosition();
        Vector receiverPos = receiverMob->GetPosition();
        double distance = CalculateDistance(senderPos, receiverPos,true);

        // Debug: Log distance calculation
        // std::cout << "Distance between sender (Node-" << senderId << ") and receiver (Node-" 
        //           << receiverId << "): " << distance << " meters" << std::endl;

        // Update maximum reception distance for the receiver node
        if (distance > maxReceptionDistance[receiverId]) {
            maxReceptionDistance[receiverId] = distance;
        }

        // Log the received signal
        signalLogFile << receiveTime << ",Received," << signalType << ","
                      << senderIp << ","
                      << socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal() << ","
                      << receivedPacket->GetSize() << " bytes,"
                      << "SenderNode: " << senderNode << ","
                      << distance << ","
                      << x << ", " << y << ", " << z << std::endl;

        // Trigger the discovery process
        uint32_t sarNodeId = receiverId;
        uint32_t civilianNodeId = senderId;
        Vector position(x, y, z); // Use coordinates from the payload

        ProcessDiscoveryEvent(
            sarNodeId, civilianNodeId, distance, signalType,
            position, socket, g_baseNode, discoveryPort
        );

        // Mark this civilian as discovered
        discoveryState[sarNodeId][civilianNodeId] = true;

        // Debug: Confirm discovery
        // std::cout << "Discovery processed for SAR Node-" << sarNodeId 
        //           << " and Civilian Node-" << civilianNodeId << std::endl;
    } else {
        // std::cerr << Simulator::Now().GetSeconds() << " - Error: No packet received on Node-"
        //           << socket->GetNode()->GetId() << std::endl;
    }
}



// Set up sockets for proactive communication by nodes
// Nodes send periodic signals based on their assigned communication type
void SetupSocketCommunication(NodeContainer& nodes,
                              uint16_t port,
                              Ipv4Address receiverAddress,
                              std::string signalType,
                              double interval)
{ 

    static bool cleanupScheduled = false;
if (!cleanupScheduled)
{
    cleanupScheduled = true;
    Simulator::Schedule(Seconds(10.0), &CleanupSendTimestamps, 120.0); // keep 2 min of timestamps max
}
    // Base for all nodes of this signal type
    uint32_t baseCommId = (signalType == "PLB" ? 3000 : 4000);

    // Offset to ensure each node gets a unique ID
    uint32_t iComm = 0;

    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<Node> node = nodes.Get(i);

        // Create a socket for each node
           uint8_t tos = 0xE0; 
        Ptr<Socket> socket = GetCachedUdpSocket(node, tos);
        if (!socket) { continue; }
        //Ptr<Socket> socket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);

        if (socket->Bind(local) == -1) {
            std::cerr << "Failed to bind socket for Node-" << node->GetId()
                      << " on port " << port << std::endl;
            continue; // skip this node if binding fails
        }

        socket->SetAllowBroadcast(true);

        // Mobility (optional for logging)
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        Vector position = mobility->GetPosition();

        // **Unique** communication ID per node
        uint32_t commIdThisNode = baseCommId + iComm;
        iComm++;
        uint32_t signalport = (signalType == "PLB" ? 9100 : 9200);

        // Start a comm session
        Ipv4Address senderAddress = node->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        StartCommunicationSession(
            commIdThisNode,
            senderAddress,
            receiverAddress,
            signalType + " Signal Communication",
            Simulator::Now().GetSeconds(),
            "UDP",
            signalport
        );

        // Schedule the first signal
        Simulator::Schedule(
            Seconds(5.0),
            &SendSignalData,
            socket,
            receiverAddress,
            port,
            "Node-" + std::to_string(node->GetId()),
            signalType,
            position.x,
            position.y,
            position.z,
            interval,
            commIdThisNode
        );
    }
}




// Set up SAR nodes to listen for signals(plb&query response)
// Binds sockets to the specified port and assigns a callback for received signals
void SetupReceiversForSARNodes(NodeContainer& sarNodes, uint16_t port, std::string signalType) {
    for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
        Ptr<Node> sarNode = sarNodes.Get(i);

        Ptr<Socket> socket = Socket::CreateSocket(sarNode, TypeId::LookupByName("ns3::UdpSocketFactory"));
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);

        if (socket->Bind(local) == -1) {
            std::cerr << "Failed to bind socket for SAR node " << sarNode->GetId() << " on port " << port << std::endl;
        } else {
            std::cout << " mixed Socket bound successfully for SAR node " << sarNode->GetId()
                      << " on port " << port << " for signal type " << signalType << std::endl;
        }

        socket->SetAllowBroadcast(true);

        // Assign appropriate callback
        if (signalType == "PLB" ) {
         socket->SetRecvCallback(MakeCallback(&OnSignalReceived));

        } else if (signalType == "QueryResponse") {
            socket->SetRecvCallback(MakeCallback(&OnQueryResponseReceived));
        } else {
            // std::cerr << "Error: Unsupported signal type '" << signalType 
            //         << "' for Node-" << sarNode->GetId() << std::endl;
        }
     
    }
}


