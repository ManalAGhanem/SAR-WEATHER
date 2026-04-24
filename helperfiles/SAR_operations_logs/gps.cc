// Function to log GPS data received at the base station with real-time delay and throughput calculation
void LogGPSData(Ptr<Socket> socket)
{
    Address from;

    while (Ptr<Packet> packet = socket->RecvFrom(from))
    {
        InetSocketAddress address = InetSocketAddress::ConvertFrom(from);

        // Receiver (this node)
        Ipv4Address receiverIp =
            socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        uint32_t packetId = packet->GetUid();
        double receiveTime = Simulator::Now().GetSeconds();

        // Deserialize position data
        Vector receivedPosition;
        packet->CopyData(reinterpret_cast<uint8_t*>(&receivedPosition), sizeof(receivedPosition));

        // Extract the communication ID tag
        CommunicationIdTag commIdTag;
        uint32_t communicationId = 0;
        if (packet->PeekPacketTag(commIdTag))
        {
            communicationId = commIdTag.GetCommunicationId();
        }
        
        // Map sender IP -> sender node id (your existing logic)
        Ipv4Address senderIp = address.GetIpv4();
        if (ipToNodeIdMap.find(senderIp) == ipToNodeIdMap.end())
        {
            NS_LOG_WARN("Sender IP not found in ipToNodeIdMap: " << senderIp);
            continue; // IMPORTANT: continue, not return
        }

        uint32_t senderId   = ipToNodeIdMap[senderIp];
        uint32_t receiverId = socket->GetNode()->GetId();

        Ptr<Node> senderNode   = NodeList::GetNode(senderId);
        Ptr<Node> receiverNode = NodeList::GetNode(receiverId);

        Ptr<MobilityModel> senderMob   = senderNode->GetObject<MobilityModel>();
        Ptr<MobilityModel> receiverMob = receiverNode->GetObject<MobilityModel>();

        if (!senderMob || !receiverMob)
        {
            NS_LOG_WARN("MobilityModel not found for sender or receiver nodes.");
            continue;
        }

        Vector senderPos   = senderMob->GetPosition();
        Vector receiverPos = receiverMob->GetPosition();
        double dist        = CalculateDistance(senderPos, receiverPos, true);

        if (dist > maxReceptionDistance[receiverId])
        {
            maxReceptionDistance[receiverId] = dist;
        }

        // Log details to GPS log file (unchanged logic)
        gpsLogFile << receiveTime << ",Received,GPS,"
                   << senderIp << ","                // Source Address
                   << receiverIp << ","              // Destination Address
                   << receivedPosition.x << ","
                   << receivedPosition.y << ","
                   << receivedPosition.z << ","
                   << packet->GetSize() << " bytes (GPS)"
                   << std::endl;

        // Delay and throughput
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

        LogPacketEvent(
            socket,
            senderIp,              
            receiverIp,             
            packetId,
            delay,
            throughput,
            packet->GetSize(),
            gpsPort,
            "GPS Update Received",
            communicationId,"RX"
        );
        

        sendTimestamps.erase(packetId); // Clean up after processing
    }
}

// Function to send GPS updates with real-time logging of send time
void SendGpsUpdate(Ptr<Socket> socket, Ipv4Address baseStationAddress, uint16_t port, 
                   Ptr<Node> node, std::string nodeName, double threshold, double fallbackInterval,uint32_t communicationId) {
   
  //  uint8_t tos = 0xA0 ;


    // Get current position
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector currentPosition = mobility->GetPosition();

    // Get last-known position
    uint32_t nodeId = node->GetId();
    Vector lastPosition = lastKnownPositions[nodeId];

    // Calculate distance moved
    double distanceMoved = std::sqrt(
        std::pow(currentPosition.x - lastPosition.x, 2) +
        std::pow(currentPosition.y - lastPosition.y, 2) +
        std::pow(currentPosition.z - lastPosition.z, 2)
    );

    double currentTime = Simulator::Now().GetSeconds();
    static std::unordered_map<uint32_t, double> lastUpdateTimes; // To track last update times

    // Check if update is needed
    if (distanceMoved > threshold || currentTime - lastUpdateTimes[nodeId] > fallbackInterval) {
        // Send GPS update
        Ptr<Packet> packet = Create<Packet>((uint8_t*)&currentPosition, sizeof(currentPosition));
      //  socket->SetIpTos(tos);  //assign ToS to soc               

        InetSocketAddress remote = InetSocketAddress(baseStationAddress, port);
         // Attach the communication ID tag
        CommunicationIdTag commIdTag(communicationId);
        packet->AddPacketTag(commIdTag);
        NodeIdTag tag;
        tag.SetNodeId(nodeId);
        packet->AddPacketTag(tag);
        // Debug: Check if the tag is attached correctly
        CommunicationIdTag debugTag;
        if (packet->PeekPacketTag(debugTag)) {
           // std::cout << "CommunicationIdTag attached successfully: " << debugTag.GetCommunicationId() << std::endl;
        } else {
            std::cerr << "Failed to attach CommunicationIdTag." << std::endl;
        }
        socket->SendTo(packet, 0, remote);

        // Store send timestamp
        uint32_t packetId = packet->GetUid();
        sendTimestamps[packetId] = currentTime;

       
        LogPacketEvent(
            socket,
            node->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Source IP
            baseStationAddress,                                  // Destination IP
            packetId,                                            // Packet ID
            0.0,                                                 // Placeholder for delay
            0.0,                                                 // Placeholder for throughput
            packet->GetSize(),                                   // Packet size
            gpsPort,
            "GPS Update Sent",                                   // Packet type
           communicationId, // Communication ID
           "TX"
        );
        // Log update
        gpsLogFile << currentTime << ",Sent,GPS,"
                   << nodeName << ","                        // Node Name
                   << node->GetObject<Ipv4>()->GetAddress(1, 0) << ","  // Node Address
                   << lastPosition.x << ","                  // Old X position
                   << lastPosition.y << ","                  // Old Y position
                   << lastPosition.z << ","                  // Old Z position
                   << currentPosition.x << ","               // Current X position
                   << currentPosition.y << ","               // Current Y position
                   << currentPosition.z << ","               // Current Z position
                   << "Moved " << distanceMoved << " meters\n";

        // Update last-known position and time
        lastKnownPositions[nodeId] = currentPosition;
        lastUpdateTimes[nodeId] = currentTime;
    }

    // Schedule the next check
    Simulator::Schedule(Seconds(20.0), &SendGpsUpdate, socket, baseStationAddress, port, node, nodeName, threshold, fallbackInterval,communicationId);
}

void SetupGpsUpdates(NodeContainer sarNodes,
    Ptr<Node> baseNode,
    uint16_t gpsPort,
    double positionChangeThreshold,
    double fallbackInterval,
    double baseInterval)
{



// 1) Create a receiving socket on the base station
Ptr<Socket> baseRecvSocket = Socket::CreateSocket(baseNode, UdpSocketFactory::GetTypeId());
InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), gpsPort);
baseRecvSocket->Bind(local);
baseRecvSocket->SetRecvCallback(MakeCallback(&LogGPSData));

// 2) Create a random variable for scheduling jitter
Ptr<UniformRandomVariable> jitter = CreateObject<UniformRandomVariable>();
jitter->SetAttribute("Min", DoubleValue(0.0));
jitter->SetAttribute("Max", DoubleValue(1.0));

// 3) For each node in sarNodes, create a send socket & schedule the first GPS
for (uint32_t i = 0; i < sarNodes.GetN(); ++i)
{
Ptr<Node> senderNode = sarNodes.Get(i);

Ptr<Socket> sendSocket = Socket::CreateSocket(senderNode, UdpSocketFactory::GetTypeId());
sendSocket->SetIpTos(0x20);

// Unique ID for each node's GPS session
uint32_t gpsCommId = 7101 + i;

// Name for logs
std::string nodeName = "SARNode_" + std::to_string(senderNode->GetId());

// The base station IP
Ipv4Address baseAddress = baseNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

// Jittered start time
double startTime = baseInterval + jitter->GetValue();
Ptr<Node> node = sarNodes.Get(i);
Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
Ipv4Address senderaddress = ipv4->GetAddress(1, 0).GetLocal();

StartCommunicationSession(gpsCommId, senderaddress, baseAddress, 
" GPS Communication", Simulator::Now().GetSeconds(), "UDP",gpsPort);
// 4) Schedule the first call to SendGpsUpdate(...) 
//    The function re-schedules itself every 1 second
Simulator::Schedule(Seconds(startTime),
                &SendGpsUpdate,
                sendSocket,
                baseAddress,
                gpsPort,
                senderNode,
                nodeName,
                positionChangeThreshold,
                fallbackInterval,
                gpsCommId);
}
}
