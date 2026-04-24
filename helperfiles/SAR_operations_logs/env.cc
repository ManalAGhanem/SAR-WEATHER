// Function to log received environmental data with detailed logging
void LogEnvironmentalData(Ptr<Socket> socket)
{
    Address from;

    while (Ptr<Packet> packet = socket->RecvFrom(from))
    {
        InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
        Ipv4Address receiverAddress =
            socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        uint32_t packetId = packet->GetUid();
        double receiveTime = Simulator::Now().GetSeconds();

        // Extract communication ID
        CommunicationIdTag commIdTag;
        uint32_t communicationId = 0;
        if (packet->PeekPacketTag(commIdTag))
        {
            communicationId = commIdTag.GetCommunicationId();
        }
        else
        {
            NS_LOG_WARN("CommunicationIdTag missing on env packet uid=" << packetId);
            continue; // IMPORTANT: keep draining
        }

        // Delay and throughput
        double delay = (sendTimestamps.count(packetId) > 0)
                           ? (receiveTime - sendTimestamps[packetId])
                           : 0.0;
        double throughput = (delay > 0) ? (packet->GetSize() * 8) / delay : 0.0;

        // Distance
        Ipv4Address senderIp = address.GetIpv4();

        auto it = ipToNodeIdMap.find(senderIp);
        if (it == ipToNodeIdMap.end())
        {
            NS_LOG_WARN("Sender IP not found in ipToNodeIdMap: " << senderIp);
            continue;
        }

        uint32_t receiverId = socket->GetNode()->GetId();
        uint32_t senderId = it->second;

        Ptr<Node> senderNode = NodeList::GetNode(senderId);
        Ptr<Node> receiverNode = NodeList::GetNode(receiverId);

        Ptr<MobilityModel> senderMob = senderNode->GetObject<MobilityModel>();
        Ptr<MobilityModel> receiverMob = receiverNode->GetObject<MobilityModel>();

        if (!senderMob || !receiverMob)
        {
            NS_LOG_WARN("MobilityModel missing sender=" << senderId << " receiver=" << receiverId);
            continue;
        }

        Vector senderPos = senderMob->GetPosition();
        Vector receiverPos = receiverMob->GetPosition();
        double dist = CalculateDistance(senderPos, receiverPos, true);

        if (dist > maxReceptionDistance[receiverId])
        {
            maxReceptionDistance[receiverId] = dist;
        }

        // Dataset log (FIXED src/dst)
        LogPacketEvent(
            socket,
            senderIp,            // Source IP
            receiverAddress,     // Destination IP
            packetId,
            delay,
            throughput,
            packet->GetSize(),
            envPort,
            "Environmental Received",
            communicationId,"RX"
        );

        // Env log file (same as yours)
        envLogFile << receiveTime << ",Received,Environmental," << senderIp << ","
                   << receiverAddress << ",Packet ID:" << packetId
                   << ",Delay:" << delay << ",Throughput:" << throughput << " bps"
                   << ",Distance:" << dist << " meters" << std::endl;

        sendTimestamps.erase(packetId);
    }
}


// Function to send environmental data with detailed logging
void SendEnvironmentalData(Ptr<Socket> socket, Ipv4Address receiverAddress, uint16_t port, 
                           double temperature, double humidity, double windSpeed, double interval,uint32_t communicationId) {
    struct EnvironmentalData {
        double temperature;
        double humidity;
        double windSpeed;
    } data = {temperature, humidity, windSpeed};
    Ptr<Node> node = socket->GetNode();
    uint32_t senderNodeId = node->GetId();

    
    uint8_t tos = 0x20;

    Ptr<Packet> packet = Create<Packet>((uint8_t *)&data, sizeof(data));
    socket->SetIpTos(tos);  //assign ToS to socket

    InetSocketAddress remote = InetSocketAddress(receiverAddress, port);
    // Attach the communication ID tag
    CommunicationIdTag commIdTag(communicationId);
    packet->AddPacketTag(commIdTag);
     NodeIdTag tag;
        tag.SetNodeId(senderNodeId);
        packet->AddPacketTag(tag);
    socket->SendTo(packet, 0, remote);

    uint32_t packetId = packet->GetUid();
    double sendTime = Simulator::Now().GetSeconds();
    sendTimestamps[packetId] = sendTime;

    // Log environmental data to the log file
    envLogFile << sendTime << ",Sent,Environmental," 
               << socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal() << ","
               << receiverAddress << ",Temperature:" << temperature 
               << ",Humidity:" << humidity << ",Wind:" << windSpeed << std::endl;
    
 

    // Log packet event to the communication dataset
    LogPacketEvent(
        socket,
        socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Source IP
        receiverAddress,                                                  // Destination IP
        packetId,                                                         // Packet ID
        0.0,                                                              // Placeholder for delay
        0.0,                                                              // Placeholder for throughput
        packet->GetSize(),                                                // Packet size
        envPort,
        "Environmental Sent",                                                  // Packet type
        communicationId,"TX"                                                   // Communication session ID
    );

    // Schedule the next environmental data send
    Simulator::Schedule(Seconds(interval), &SendEnvironmentalData, socket, receiverAddress, port, temperature, humidity, windSpeed, interval,communicationId);
}
void SetupEnvironmentalCommunication(NodeContainer droneNodes, NodeContainer helicopterNodes,
    Ptr<Node> baseNode, uint16_t envPort, double temperature, double humidity, double windSpeed)
{
    // 1. Base Station: Set up the receiving socket for environmental data.
    Ptr<Socket> envRecvSocket = Socket::CreateSocket(baseNode, UdpSocketFactory::GetTypeId());
    envRecvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), envPort));
    envRecvSocket->SetRecvCallback(MakeCallback(&LogEnvironmentalData));

    // 2. Drones: Set up sending sockets and schedule environmental data transmissions.
    // Use a base communication ID for drones (e.g., 8001) and increment for each additional drone.
    uint32_t droneBaseCommId = 8001;
    double droneStartDelay = 3.0;      // Starting delay for the first drone
    double droneSendInterval = 15.0;    // Drone transmission interval
    for (uint32_t i = 0; i < droneNodes.GetN(); ++i) {
    Ptr<Node> drone = droneNodes.Get(i);
    Ptr<Socket> sendDroneSocket = Socket::CreateSocket(drone, UdpSocketFactory::GetTypeId());
    uint32_t droneCommId = droneBaseCommId + i;  // Unique Communication ID for each drone
    Ipv4Address droneSenderAddress = drone->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    // Start the communication session for this drone.
    StartCommunicationSession(droneCommId,
    droneSenderAddress,
    baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
    "Environmental Drone",
    Simulator::Now().GetSeconds(), "UDP", envPort);

    // Schedule the environmental data transmission.
    // Each drone is scheduled with a small incremental delay to avoid simultaneous transmissions.
    Simulator::Schedule(Seconds(droneStartDelay + i * 0.1),
    &SendEnvironmentalData,
    sendDroneSocket,
    baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
    envPort,
    temperature,
    humidity,
    windSpeed,
    droneSendInterval,
    droneCommId);
    }

    // 3. Helicopters: Set up sending sockets and schedule environmental data transmissions.
    // Use a separate base Communication ID for helicopters (e.g., 9001) and increment for each.
    uint32_t helicopterBaseCommId = 9001;
    double helicopterStartDelay = 4.0;      // Starting delay for the first helicopter
    double helicopterSendInterval = 15.0;   // Helicopter transmission interval
    for (uint32_t i = 0; i < helicopterNodes.GetN(); ++i) {
    Ptr<Node> helicopter = helicopterNodes.Get(i);
    Ptr<Socket> sendHelicopterSocket = Socket::CreateSocket(helicopter, UdpSocketFactory::GetTypeId());
    uint32_t helicopterCommId = helicopterBaseCommId + i;  // Unique Communication ID for each helicopter
    Ipv4Address helicopterSenderAddress = helicopter->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    // Start the communication session for this helicopter.
    StartCommunicationSession(helicopterCommId,
    helicopterSenderAddress,
    baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
    "Environmental Helicopter",
    Simulator::Now().GetSeconds(), "UDP", envPort);

    // Schedule the environmental data transmission.
    Simulator::Schedule(Seconds(helicopterStartDelay + i * 0.1),
    &SendEnvironmentalData,
    sendHelicopterSocket,
    baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
    envPort,
    temperature,
    humidity,
    windSpeed,
    helicopterSendInterval,
    helicopterCommId);
    }
}
