
// base send control to drone
void SendControlMessage(Ptr<Socket> socket, Ptr<Node> drone, uint32_t communicationId) {

    Ipv4Address droneIp = drone->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    InetSocketAddress destination(droneIp, controlMsgPort);  // Send to drone port 8081

    std::string msg = "NAV_UPDATE: AltitudeAdjust";
    Ptr<Packet> packet = Create<Packet>((uint8_t*)msg.c_str(), msg.size());
    // Attach the communication ID tag
    CommunicationIdTag commIdTag(communicationId);
    packet->AddPacketTag(commIdTag);
     NodeIdTag tag;
        tag.SetNodeId(socket->GetNode()->GetId());
        packet->AddPacketTag(tag);

    socket->SendTo(packet, 0, destination);
    //NS_LOG_INFO("Base Station sent navigation update to Drone " << drone->GetId());
     // Log the packet event
    uint32_t packetId = packet->GetUid();
         LogPacketEvent(
        socket,
        socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Source: Base station
        droneIp,                                                           // Destination: Drone
        packetId,
        0.0,                                                               // Placeholder for delay
        0.0,                                                               // Placeholder for throughput
        packet->GetSize(),
        controlMsgPort,                                                 // Packet size
        "Control Message Sent",                                                 // Packet type
       communicationId,"TX"
    );

Simulator::Schedule(Seconds(1.0), &SendControlMessage, socket, drone,communicationId);

}

//drone recieves small base station packet
void ReceiveControlMessage(Ptr<Socket> socket) {
    Ptr<Packet> packet;
    Address senderAddress;

    while ((packet = socket->RecvFrom(senderAddress))) {
        // Extract sender's IP address
        Ipv4Address senderIp = InetSocketAddress::ConvertFrom(senderAddress).GetIpv4();
        uint32_t receiverId = socket->GetNode()->GetId();

        // Get sender's node ID from IP map
        uint32_t senderId = ipToNodeIdMap[senderIp];

        // Retrieve sender and receiver nodes
        Ptr<Node> senderNode = NodeList::GetNode(senderId);
        Ptr<Node> receiverNode = NodeList::GetNode(receiverId);

        // Retrieve sender and receiver positions using MobilityModel
        Ptr<MobilityModel> senderMob = senderNode->GetObject<MobilityModel>();
        Ptr<MobilityModel> receiverMob = receiverNode->GetObject<MobilityModel>();
        Vector senderPos = senderMob->GetPosition();
        Vector receiverPos = receiverMob->GetPosition();

            // Extract the communication ID tag
      CommunicationIdTag commIdTag;
    uint32_t communicationId = 0;
    if (packet->PeekPacketTag(commIdTag)) {
        communicationId = commIdTag.GetCommunicationId();
       // std::cout << "Retrieved Communication ID Tag: " << commIdTag.GetCommunicationId() << std::endl;

    } else {
      //  std::cerr << "Warning: Communication ID tag missing from packet." << std::endl;
        return;  // Exit early if the tag is missing
    }
        // Calculate distance
        double dist = CalculateDistance(senderPos, receiverPos,true);

        // Update maximum reception distance
        if (dist > maxReceptionDistance[receiverId]) {
            maxReceptionDistance[receiverId] = dist;
        }

        // Extract and display the packet data
        uint32_t packetId = packet->GetUid();
        std::vector<uint8_t> buffer(packet->GetSize());
        packet->CopyData(buffer.data(), packet->GetSize());
        std::string msg(reinterpret_cast<char*>(buffer.data()), packet->GetSize());

        // Calculate delay and throughput
        double receiveTime = Simulator::Now().GetSeconds();
        double delay = (sendTimestamps.count(packetId) > 0) ? (receiveTime - sendTimestamps[packetId]) : 0.0;
        double throughput = (delay > 0) ? (packet->GetSize() * 8) / delay : 0.0; // Throughput in bits per second

             // Log the received packet event
        LogPacketEvent(
            socket,
            socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Destination: Drone
            senderIp,                                // Source: Base station
            packetId,
            delay,                                   // Calculated delay
            throughput,                              // Calculated throughput
            packet->GetSize(),                       // Packet size
            controlMsgPort,
            "Control Message Received",             // Packet type
            communicationId,"RX"
        );
        //NS_LOG_INFO("Node " << receiverId << " received control message from Node " 
                 //   << senderId << ": " << msg);
        //NS_LOG_INFO("Distance: " << dist << ", Max Reception Distance: " 
                //    << maxReceptionDistance[receiverId]);
    }
}
void SetupBaseToDroneControl(NodeContainer droneNodes, Ptr<Node> baseNode, uint16_t controlMsgPort)
{
    // Base Station: Create a socket for sending control messages.
    Ptr<Socket> baseSocketControlmsgs = Socket::CreateSocket(baseNode, UdpSocketFactory::GetTypeId());
    baseSocketControlmsgs->SetIpTos(0x60);

    InetSocketAddress baseAddressdrone(Ipv4Address::GetAny(), controlMsgPort);
    baseSocketControlmsgs->Bind(baseAddressdrone);

    // Use a base communication ID (e.g., 10000) and increment for each drone.
    uint32_t baseControlCommId = 10000;

    // Loop through all drone nodes.
    for (uint32_t i = 0; i < droneNodes.GetN(); ++i)
    {
        Ptr<Node> drone = droneNodes.Get(i);
        // Retrieve the drone's IP address.
        Ipv4Address droneIp = drone->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        // Generate a unique communication ID for this drone.
        uint32_t controlCommId = baseControlCommId + i;

        // Log the communication session for base-to-drone control.
        StartCommunicationSession(
            controlCommId,
            baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
            droneIp,
            "Base-to-Drone Control",
            Simulator::Now().GetSeconds(),
            "UDP",
            controlMsgPort
        );

        // Drone: Set up the receiving socket for control messages.
        Ptr<Socket> droneSocketControlmsgs = Socket::CreateSocket(drone, UdpSocketFactory::GetTypeId());
        InetSocketAddress droneAddress(Ipv4Address::GetAny(), controlMsgPort);
        droneSocketControlmsgs->Bind(droneAddress);
        droneSocketControlmsgs->SetRecvCallback(MakeCallback(&ReceiveControlMessage));

        // Schedule control messages from the base station to the drone.
        // The start time is staggered (e.g., 10 seconds plus an offset per drone) to avoid collisions.
        Simulator::Schedule(Seconds(10.0 + i * 0.5),
                            &SendControlMessage,
                            baseSocketControlmsgs,
                            drone,
                            controlCommId);
    }
}
