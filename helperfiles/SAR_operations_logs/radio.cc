// Function to log voice data received at a node with detailed logging
void LogVoiceData(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);

   uint8_t tos = socket->GetIpTos();


    // Receiver node's IP address
    Ipv4Address receiverAddress = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    uint32_t packetId = packet->GetUid();
    double receiveTime = Simulator::Now().GetSeconds();

      // Print the packet size and content for debugging
    std::ostringstream oss;
    packet->Print(oss);
  //  std::cout << "Received VOICE Packet (UID: " << packetId << ") Content: " << oss.str() << std::endl;

    // Log in voice_communication_logfinalv3.csv
    voiceLogFile << receiveTime << ",Received,Voice," << address.GetIpv4() << ","
                 << receiverAddress << "," << packet->GetSize() << " bytes" <<","<<(int)tos << std::endl;

    // Extract the communication ID tag
      CommunicationIdTag commIdTag;
    uint32_t communicationId = 0;
    if (packet->PeekPacketTag(commIdTag)) {
        communicationId = commIdTag.GetCommunicationId();
      //  std::cout << "Retrieved Communication ID Tag: " << commIdTag.GetCommunicationId() << std::endl;

    } else {
       // std::cerr << "Warning: Communication ID tag missing from packet." << std::endl;
        return;  // Exit early if the tag is missing
    }

    // Calculate delay (assuming we track send times in sendTimestamps)
    double delay = (sendTimestamps.count(packetId) > 0) ? (receiveTime - sendTimestamps[packetId]) : 0.0;

    // Retrieve the sender's IP address
    Ipv4Address senderIp = address.GetIpv4();

    // Calculate throughput
    double throughput = (delay > 0) ? (packet->GetSize() * 8) / delay : 0.0;

    // Calculate distance between sender and receiver
    uint32_t receiverId = socket->GetNode()->GetId();
    uint32_t senderId = ipToNodeIdMap[senderIp];
    Ptr<Node> senderNode = NodeList::GetNode(senderId);
    Ptr<Node> receiverNode = NodeList::GetNode(receiverId);
    Ptr<MobilityModel> senderMob = senderNode->GetObject<MobilityModel>();
    Ptr<MobilityModel> receiverMob = receiverNode->GetObject<MobilityModel>();
    Vector senderPos = senderMob->GetPosition();
    Vector receiverPos = receiverMob->GetPosition();
    double dist = CalculateDistance(senderPos, receiverPos, true);

    if (dist > maxReceptionDistance[receiverId]) {
        maxReceptionDistance[receiverId] = dist;
    }

    // Log in detailed-packet-logfinalv3.csv
    LogPacketEvent(socket,  receiverAddress,senderIp, packetId, delay, throughput, packet->GetSize(),voicePort ,"Voice Received", communicationId,"RX");

    // Clean up after processing
    sendTimestamps.erase(packetId);
}


// Function to send voice data (from one ground team node to another) with detailed logging
void SendVoiceData(Ptr<Socket> socket, Ipv4Address receiverAddress, uint16_t port, std::string senderNode, double interval) {


    uint8_t tos = 0xA0;
    



        Ipv4Address senderAddress = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        uint32_t senderNodeId = socket->GetNode()->GetId();
        Ptr<Packet> packet = Create<Packet>(200);  // Voice packet (200 bytes)
        uint32_t packetId = packet->GetUid();
        // Generate a unique key for the sender-receiver pair
    uint32_t receiverNodeId = receiverAddress.Get(); // Extract integer representation of the receiver
    std::pair<uint32_t, uint32_t> sessionKey = std::make_pair(senderNodeId, receiverNodeId);

    // Check if this session already has a communication ID
    uint32_t voiceCommId;
    if (sessionTable.find(sessionKey) != sessionTable.end()) {
        // Reuse existing communication ID
        voiceCommId = sessionTable[sessionKey];
    } else {
        // Create a new communication ID and store it in the session table
        voiceCommId = voiceCommunicationId++;
        sessionTable[sessionKey] = voiceCommId;
    }

     StartCommunicationSession(voiceCommId, senderAddress, receiverAddress, " Voice Communication", Simulator::Now().GetSeconds(), "UDP",voicePort);

     socket->SetIpTos(tos);  //assign ToS to socket

    InetSocketAddress remote = InetSocketAddress(receiverAddress, port);

    // Attach the communication ID tag
    CommunicationIdTag commIdTag(voiceCommId);
    packet->AddPacketTag(commIdTag);
    NodeIdTag tag;
    tag.SetNodeId(senderNodeId); 
    packet->AddPacketTag(tag);

       // Debug: Check if the tag is attached correctly
    CommunicationIdTag debugTag;
    if (packet->PeekPacketTag(debugTag)) {
    //    std::cout << "CommunicationIdTag attached successfully: " << debugTag.GetCommunicationId() << std::endl;
    } else {
        std::cerr << "Failed to attach CommunicationIdTag." << std::endl;
    }

        socket->SendTo(packet, 0, remote);

    double sendTime = Simulator::Now().GetSeconds();
    sendTimestamps[packetId] = sendTime;  // Store send time for delay calculation

    // Log in detailed-packet-logfinalv3.csv
    LogPacketEvent(socket, senderAddress, receiverAddress, packetId, 0.0, 0.0, packet->GetSize(),voicePort, "Voice Sent", voiceCommId,"TX");

    // Reschedule the next voice packet
    Simulator::Schedule(Seconds(interval), &SendVoiceData, socket, receiverAddress, voicePort, senderNode, interval);
}

// Function to set up voice communication between ground nodes and the base station with logging
void SetupVoiceCommunication(NodeContainer vehicleNode,NodeContainer Helicopter, NodeContainer footNodes, Ptr<Node> baseNode) {
   
    // Setup voice communication between all ground nodes and with the base station
    for (uint32_t i = 0; i < groundNodes.GetN(); ++i) {
        Ptr<Node> senderNode = groundNodes.Get(i);

        // Create sender and receiver sockets for each ground node
        Ptr<Socket> sendSocket = Socket::CreateSocket(senderNode, UdpSocketFactory::GetTypeId());
        Ptr<Socket> recvSocket = Socket::CreateSocket(senderNode, UdpSocketFactory::GetTypeId());

        // Bind receiving socket to the voice port
        recvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), voicePort));
        recvSocket->SetRecvCallback(MakeCallback(&LogVoiceData));

           // Schedule communication to other ground nodes
        for (uint32_t j = 0; j < groundNodes.GetN(); ++j) {
            if (i != j) { // Avoid sending to self
                Ptr<Node> receiverNode = groundNodes.Get(j);
                Ipv4Address receiverAddress = receiverNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

                // Schedule regular voice messages between ground nodes
                Simulator::Schedule(Seconds(30.0 + (i * 0.5)), &SendVoiceData, sendSocket, receiverAddress, voicePort, "Voice From SAR", 2.0);
            }
        }

        // Schedule communication from each ground node to the base station
        Ipv4Address baseAddress = baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Fixed: Access the IP address directly from baseNode
        Simulator::Schedule(Seconds(10.0 + (i * 0.5)), &SendVoiceData, sendSocket, baseAddress, voicePort, "TeamToBase", 2.0);
    }

    // Setup base station to communicate with each ground node
    Ptr<Socket> baseSendSocket = Socket::CreateSocket(baseNode, UdpSocketFactory::GetTypeId()); // Fixed: Use baseNode directly
    Ptr<Socket> baseRecvSocket = Socket::CreateSocket(baseNode, UdpSocketFactory::GetTypeId());

    // Bind receiving socket at the base station to the voice port
    baseRecvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), voicePort));
    baseRecvSocket->SetRecvCallback(MakeCallback(&LogVoiceData));

    // Schedule periodic voice messages from the base station to each ground node
    for (uint32_t i = 0; i < groundNodes.GetN(); ++i) {
        Ptr<Node> receiverNode = groundNodes.Get(i);
        Ipv4Address receiverAddress = receiverNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        Simulator::Schedule(Seconds(10.5), &SendVoiceData, baseSendSocket, receiverAddress, voicePort, "BaseToGroundTeam", 2.0);
    }
}
