// Function to continuously stream video data from a node to the base station or other nodes
void StreamVideoFeed(Ptr<Socket> socket, Ipv4Address receiverAddress, uint16_t port, std::string senderNode, double interval, uint32_t communicationId) {
    
    


    Ptr<Packet> packet = Create<Packet>(1500);  // Streaming video packet (1500 bytes)

    InetSocketAddress remote = InetSocketAddress(receiverAddress, port);
    // Attach the communication ID tag
    CommunicationIdTag commIdTag(communicationId);
    packet->AddPacketTag(commIdTag);
    socket->SendTo(packet, 0, remote);

    uint32_t packetId = packet->GetUid();
    double sendTime = Simulator::Now().GetSeconds();
    sendTimestamps[packetId] = sendTime;  // Store send time for delay calculation

    // Declare senderAddress only once
    Ipv4Address senderAddress = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    
      // Log the video packet event to the expanded communication dataset
    LogPacketEvent(
        socket,                      // Socket
        senderAddress,               // Source address
        receiverAddress,             // Destination address
        packetId,                    // Packet ID
        0.0,                         // Placeholder for delay (will be updated on reception)
        0.0,                         // Placeholder for throughput
        packet->GetSize(),           // Packet size
         videoPort,  // Port for video feed
        "STREAMING Video Packet",              // Packet type
        communicationId,              // Communication session ID
        "TX"
    );

    // Log video data to the video log file
    videoLogFile << sendTime << ",Stream,Video," << senderAddress << ","
                 << receiverAddress << "," << packet->GetSize() << " bytes" << std::endl;

    // Reschedule for continuous streaming
    Simulator::Schedule(Seconds(interval), &StreamVideoFeed, socket, receiverAddress, port, senderNode, interval, communicationId);
}


// Function to log video data received with real-time delay and throughput calculation
void LogVideoData(Ptr<Socket> socket)
{
    Address from;

    while (Ptr<Packet> packet = socket->RecvFrom(from))
    {
        InetSocketAddress address = InetSocketAddress::ConvertFrom(from);

        // Receiver (this node)
        Ipv4Address receiverAddress =
            socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        uint32_t packetId = packet->GetUid();
        double receiveTime = Simulator::Now().GetSeconds();

        // Delay and throughput (same logic)
        double delay = 0.0;
        if (sendTimestamps.count(packetId) > 0)
        {
            delay = receiveTime - sendTimestamps[packetId];
        }
        else
        {
            NS_LOG_WARN("Packet ID not found in sendTimestamps: " << packetId);
        }

        double throughput = (delay > 0) ? (packet->GetSize() * 8) / delay : 0.0;

        // Sender IP
        Ipv4Address senderIp = address.GetIpv4();
        uint32_t receiverId = socket->GetNode()->GetId();

        // SAFE lookup (prevents accidental insertion / wrong senderId)
        auto it = ipToNodeIdMap.find(senderIp);
        if (it == ipToNodeIdMap.end())
        {
            NS_LOG_WARN("Sender IP not found in ipToNodeIdMap: " << senderIp);
            // keep draining other queued packets
            continue;
        }
        uint32_t senderId = it->second;

        // Distance (same logic, but with null checks)
        Ptr<Node> senderNode = NodeList::GetNode(senderId);
        Ptr<Node> receiverNode = NodeList::GetNode(receiverId);

        Ptr<MobilityModel> senderMob = senderNode->GetObject<MobilityModel>();
        Ptr<MobilityModel> receiverMob = receiverNode->GetObject<MobilityModel>();

        if (!senderMob || !receiverMob)
        {
            NS_LOG_WARN("MobilityModel missing for sender=" << senderId
                                                           << " or receiver=" << receiverId);
            continue;
        }

        Vector senderPos = senderMob->GetPosition();
        Vector receiverPos = receiverMob->GetPosition();
        double dist = CalculateDistance(senderPos, receiverPos, true);

        if (dist > maxReceptionDistance[receiverId])
        {
            maxReceptionDistance[receiverId] = dist;
        }

        // Communication ID tag
        CommunicationIdTag commIdTag;
        uint32_t communicationId = 0;
        if (packet->PeekPacketTag(commIdTag))
        {
            communicationId = commIdTag.GetCommunicationId();
        }
        else
        {
            // IMPORTANT: do not return; just skip this packet
            NS_LOG_WARN("CommunicationIdTag missing on video packet uid=" << packetId);
            continue;
        }

        // Dataset log (FIXED src/dst order)
        LogPacketEvent(
            socket,
            senderIp,              // Source IP (sender)
            receiverAddress,       // Destination IP (receiver)
            packetId,
            delay,
            throughput,
            packet->GetSize(),
            videoPort,
            "Video Packet Received",
            communicationId,"RX"
        );

        // Human-readable log (same idea as yours)
        videoLogFile << receiveTime << ",Received, Video,"
                     << senderIp << "," << receiverAddress << ","
                     << packet->GetSize() << " bytes"
                     << std::endl;

        // Cleanup
        sendTimestamps.erase(packetId);
    }
}


// Function to send video feed data with real-time logging of send time
void SendVideoFeed(Ptr<Socket> socket, Ipv4Address receiverAddress, uint16_t port, const std::string& senderNode, double interval,uint32_t communicationId) {

    Ptr<Node> node = socket->GetNode();
uint32_t senderNodeId = node->GetId();
  //  uint8_t tos = 0x20;

   // socket->SetIpTos(tos);  //assign ToS to socket

    Ptr<Packet> packet = Create<Packet>(1500);  // Video packet (1500 bytes)
    InetSocketAddress remote = InetSocketAddress(receiverAddress, port);
    // Attach the communication ID tag
    CommunicationIdTag commIdTag(communicationId);
    packet->AddPacketTag(commIdTag);
     NodeIdTag tag;
    tag.SetNodeId(senderNodeId); 
    packet->AddPacketTag(tag);
    socket->SendTo(packet, 0, remote);

    uint32_t packetId = packet->GetUid();
    sendTimestamps[packetId] = Simulator::Now().GetSeconds();  // Store send time

    Ipv4Address senderAddress = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

  
      // Log the video packet event to the expanded communication dataset
    LogPacketEvent(
        socket,                      // Socket
        senderAddress,               // Source address
        receiverAddress,             // Destination address
        packetId,                    // Packet ID
        0.0,                         // Placeholder for delay (will be updated on reception)
        0.0,                         // Placeholder for throughput
        packet->GetSize(),           // Packet size
        videoPort,
        "Video Packet Sent",              // Packet type
        communicationId,              // Communication session ID
        "TX"
    );

    videoLogFile << Simulator::Now().GetSeconds() << ",Sent, Video," << senderAddress << ","
                 << receiverAddress << "," << packet->GetSize() << " bytes" << std::endl;


    Simulator::Schedule(Seconds(interval), &SendVideoFeed, socket, receiverAddress, port, std::cref(senderNode), interval,communicationId);
}
void SetupVideoCommunication(NodeContainer vehicleNodes, NodeContainer droneNodes, Ptr<Node> baseNode, uint16_t videoPort)
{
// 1. Base Station: Set up the receiving socket for video feed.

Ptr<Socket> recvBaseStationSocket = Socket::CreateSocket(baseNode, UdpSocketFactory::GetTypeId());
recvBaseStationSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), videoPort));
recvBaseStationSocket->SetRecvCallback(MakeCallback(&LogVideoData));
//NS_LOG_INFO("Base Station set up to receive video feed on port " << videoPort);

// 2. Vehicles: Create sending sockets and schedule video feed transmissions.
for (uint32_t i = 0; i < vehicleNodes.GetN(); ++i) {
Ptr<Node> vehicle = vehicleNodes.Get(i);
Ptr<Socket> sendVideoSocket = Socket::CreateSocket(vehicle, UdpSocketFactory::GetTypeId());
sendVideoSocket->SetIpTos(0x20);

// Generate a unique communication ID for each vehicle (starting from 1010)
uint32_t communicationVehicleId = 1010 + i;

// Retrieve the sender's IP address from the vehicle node.
Ipv4Address senderAddress = vehicle->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

// Destination is the base station address from baseInterfaces.
Ipv4Address destinationAddress = baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

// Start the communication session for this vehicle.
StartCommunicationSession(communicationVehicleId, senderAddress, destinationAddress,
          "VehicleToBase Video Feed", Simulator::Now().GetSeconds(),
          "UDP", videoPort);

// Schedule video feed transmission for this vehicle.
// Here all vehicles are scheduled to start at 1.0 seconds.
Simulator::Schedule(Seconds(40.0),
    &SendVideoFeed,
    sendVideoSocket,
    destinationAddress,
    videoPort,
    "Vehicle " + std::to_string(i+1),
    1.0,  // Send interval (every 1 second)
    communicationVehicleId);
}

// 3. Drone: Optionally set up the drone video feed if any drone node is provided.
if (droneNodes.GetN() > 0) {
Ptr<Node> drone = droneNodes.Get(0); // Use the first drone (or modify if handling multiple drones)
uint32_t droneCommunicationId = 1020;  // Unique communication ID for the drone

Ptr<Socket> streamVideoSocket = Socket::CreateSocket(drone, UdpSocketFactory::GetTypeId());
streamVideoSocket->SetIpTos(0x20);

Ipv4Address droneSenderAddress = drone->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

// Start the communication session for the drone.
StartCommunicationSession(droneCommunicationId, droneSenderAddress,  baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
          "Drone Video Feed", Simulator::Now().GetSeconds(), "UDP", videoPort);

// Schedule continuous video streaming from the drone.
Simulator::Schedule(Seconds(50.0),
    &StreamVideoFeed,
    streamVideoSocket,
    baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
        videoPort,
    "Drone",
    0.1,  // Send interval for the drone stream
    droneCommunicationId);
}
}
