// Rescue assignment data structure
struct RescueAssignmentData {
    uint32_t civilianId;
    uint32_t assignedNodeId;
    double civilianX, civilianY, civilianZ;
};
void BroadcastAttachmentStatus(uint32_t sarNodeId,
                               uint32_t civilianId,
                               const NodeContainer& allSarNodes,
                               uint16_t broadcastPort)
{
    uint8_t tos=0x60;

    // The sender node (SAR node that became attached)
    Ptr<Node> senderNode = allSarNodes.Get(sarNodeId);
    if (!senderNode) {
        NS_LOG_ERROR("SAR node with ID=" << sarNodeId << " not found in the NodeContainer.");
        return;
    }

    // Sender's IP address
    Ptr<Ipv4> senderIpv4 = senderNode->GetObject<Ipv4>();
    if (!senderIpv4) {
        NS_LOG_ERROR("No Ipv4 object for SAR node " << sarNodeId);
        return;
    }
    Ipv4Address senderIp = senderIpv4->GetAddress(1, 0).GetLocal();

    // Create socket on the sender node
    Ptr<Socket> socket =GetCachedUdpSocket(senderNode, tos);
    // Prepare the payload for the broadcast
    struct AttachmentBroadcastData {
        uint32_t sarNodeId;
        uint32_t civilianId;
    } data = { sarNodeId, civilianId };

    // Loop through all SAR nodes and send a unicast message to each (except the sender)
    for (uint32_t i = 0; i < allSarNodes.GetN(); ++i) {
        Ptr<Node> receiverNode = allSarNodes.Get(i);

        // Skip the sender node
        if (receiverNode->GetId() == sarNodeId)
            continue;

        // Get the receiver's IP address
        Ptr<Ipv4> recvIpv4 = receiverNode->GetObject<Ipv4>();
        if (!recvIpv4) {
            NS_LOG_WARN("No Ipv4 object for Node " << receiverNode->GetId());
            continue;
        }
        Ipv4Address recvIp = recvIpv4->GetAddress(1, 0).GetLocal();

        // Generate a unique communication ID using the session table
        uint32_t sendattachCommId;
        std::pair<uint32_t, uint32_t> sessionKey = std::make_pair(senderNode->GetId(), receiverNode->GetId());

        if (sessionTable.find(sessionKey) != sessionTable.end()) {
            // Reuse existing communication ID
            sendattachCommId = sessionTable[sessionKey];
        } else {
            // Create a new communication ID and store it in the session table
            sendattachCommId = nextBroadcastCommId++;
            sessionTable[sessionKey] = sendattachCommId;
        }

        // Create the packet
        Ptr<Packet> packet = Create<Packet>((uint8_t*)&data, sizeof(data));

        // Tag the packet with the communication ID
        CommunicationIdTag commIdTag(sendattachCommId);
        packet->AddPacketTag(commIdTag);

        // Log each communication in the log file
        uint32_t packetId  = packet->GetUid();
        double   sendTime  = Simulator::Now().GetSeconds();
        sendTimestamps[packetId] = sendTime;

        StartCommunicationSession(
            sendattachCommId,      // Unique Comm ID for each session
            senderIp,              // Source IP
            recvIp,                // Destination IP
            "Attachment Announcement Session",
            sendTime,
            "UDP",
            broadcastPort
        );
        // Send to the receiver node
        InetSocketAddress remote(recvIp, broadcastPort);
        NodeIdTag tag;
        tag.SetNodeId(sarNodeId);
        packet->AddPacketTag(tag);
        socket->SendTo(packet, 0, remote);

        // Log the event
        LogPacketEvent(
            socket,
            senderIp,
            recvIp,
            packetId,
            0.0,               // Delay placeholder
            0.0,               // Throughput placeholder
            packet->GetSize(),
            broadcastPort,
            "Attachment Broadcast Sent",
            sendattachCommId,"TX"
        );

        // Log the broadcast to the attachment log file
        attachmentLogFile << sendTime << ",Broadcast," << sarNodeId << ","
                          << civilianId << ","
                          << "Unicast to Node " << receiverNode->GetId() << "\n";
        attachmentLogFile.flush();
    }

   
}

void LogAttachment(uint32_t sarNodeId, uint32_t civilianId, Vector sarPosition, Vector civilianPosition, double timestamp) {
    attachmentLogFile << timestamp << ",Attachment," << sarNodeId << ","
                  << sarPosition.x << "," << sarPosition.y << "," << sarPosition.z << ","
                  << civilianId << "," << civilianPosition.x << "," << civilianPosition.y << "," << civilianPosition.z
                  << ",\n"; 
attachmentLogFile.flush();


    std::cout << "Attachment Log: SAR Node " << sarNodeId
              << " attached to Civilian " << civilianId
              << " at time " << timestamp << "\n";
}

void MoveTowardBaseStation(
        Ptr<Node> sarNode, 
        Ptr<Node> civilianNode, 
        Ptr<Node> baseNode, 
        Ptr<Socket> broadcastSocket, 
        uint16_t rescuemovmentport );

        
void ConvergeOnCivilian(Ptr<Node> sarNode, Ptr<Node> civilianNode, Ptr<Socket> baseSocket, uint16_t port) {
    // Calculate the distance between SAR node and civilian node
    Vector civilianPosition = civilianNode->GetObject<MobilityModel>()->GetPosition();
    Vector sarPosition = sarNode->GetObject<MobilityModel>()->GetPosition();
    Vector basePosition = g_baseNode->GetObject<MobilityModel>()->GetPosition();


    double distance = CalculateDistance(sarPosition, civilianPosition, true);
    double distanceToBase  = CalculateDistance(sarPosition, basePosition, true);


    // Retrieve the event ID for this civilian
    uint32_t civilianNodeId = civilianNode->GetId();
    if (g_civilianToEventIdMap.find(civilianNodeId) == g_civilianToEventIdMap.end()) {
        NS_LOG_ERROR("Event ID for Civilian Node " << civilianNodeId << " not found!");
        return;
    }
    uint32_t eventId = g_civilianToEventIdMap[civilianNodeId];

    // Log the start of convergence if not already logged
    static std::set<uint32_t> loggedConvergenceStart; // To avoid multiple starts being logged
    if (loggedConvergenceStart.find(eventId) == loggedConvergenceStart.end()) {
        ns3::Ipv4Address sarNodeIp = sarNode->GetObject<ns3::Ipv4>()->GetAddress(1, 0).GetLocal();

        // Call RecordConvergenceStart
        RecordConvergenceStart(eventId, {sarNode->GetId()}, {sarNodeIp}, Simulator::Now().GetSeconds(), distance);
        loggedConvergenceStart.insert(eventId);
    }

    uint32_t convergenceCommId = 30000 + sarNode->GetId(); // Unique communication ID for convergence

    // Start the communication session for convergence
    Ipv4Address sarNodeIp = sarNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    Ipv4Address baseStationIp = Ipv4Address("10.1.1.1"); // Ensure this IP is correct

    StartCommunicationSession(
        convergenceCommId,
        sarNodeIp,         // Source IP (SAR node)
        baseStationIp,     // Destination IP (Base station)
        "Convergence Session", // Communication type
        Simulator::Now().GetSeconds(),
        "UDP", // Protocol
        port
    );

    // Determine status based on distance
    std::string status = (distance <= convergenceThresholdDistance) ? "Attached" : "Converging";
            //NS_LOG_INFO("SAR Node " << sarNode->GetId() << " has attached to Civilian " << civilianNode->GetId());

    // Log convergence progress
    double timestamp = Simulator::Now().GetSeconds();
    convergenceLogFile << timestamp << "," <<sarNodeIp<<","<< sarNode->GetId() << "," << civilianNode->GetId() << ","
                       << sarPosition.x << "," << sarPosition.y << "," << sarPosition.z << ","
                       << civilianPosition.x << "," << civilianPosition.y << "," << civilianPosition.z << ","
                       << basePosition.x << "," << basePosition.y << "," << basePosition.z << ","
                       << distance << ","<< distanceToBase  << "," << status << "\n";
    convergenceLogFile.flush();

    std::cout << " SAR NODE " <<sarNode->GetId()<<" CONVERGING ON "<<civilianNode->GetId()<< " DISTANCE TILL ATTACHMENT "<< distance<< std::endl;


    // Append the current position to the tracked positions
    if (trackedConvergencePositions.find(sarNode->GetId()) == trackedConvergencePositions.end()) {
        trackedConvergencePositions[sarNode->GetId()] = {};
    }

   auto &hist = trackedConvergencePositions[sarNode->GetId()];
    hist.push_back(sarPosition);

    static const size_t MAX_TRACKED_POS = 300; // tune
    if (hist.size() > MAX_TRACKED_POS)
    {
        hist.erase(hist.begin(), hist.begin() + (hist.size() - MAX_TRACKED_POS));
    }


    // If SAR node is attached to the civilian, stop further movement and proceed to rescue
    if (status == "Attached") {
       // NS_LOG_INFO("SAR Node " << sarNode->GetId() << " has attached to Civilian " << civilianNode->GetId());
        g_civilianAttachedState[civilianNode->GetId()] = true; // Mark as attached
        g_civilianToSarNodeMap[sarNode->GetId()] = civilianNode->GetId();


        // Log the attachment
        LogAttachment(sarNode->GetId(), civilianNode->GetId(), sarPosition, civilianPosition, timestamp);

        // Broadcast attachment status to other SAR nodes
        BroadcastAttachmentStatus(sarNode->GetId(), civilianNode->GetId(), allSarNodes, broadcastPort);

        // Record convergence completion
        RecordConvergenceComplete(
            eventId,                         // Event ID
            timestamp,                       // Completion time
            sarNode,                         // Assigned SAR node
            civilianNode,                    // Civilian node
            trackedConvergencePositions[sarNode->GetId()] // Tracked positions
        );

        // Clear tracked positions for this SAR node after completion
        trackedConvergencePositions[sarNode->GetId()].clear();

        // Log the start of rescue
        static std::set<uint32_t> loggedRescueStart; // To avoid multiple starts being logged
        if (loggedRescueStart.find(eventId) == loggedRescueStart.end()) {
            RecordRescueStart(eventId, {sarNode->GetId()}, timestamp);
            loggedRescueStart.insert(eventId);
        }

        // Create a socket for SAR node's movement toward the base station
           uint8_t tos=0x60;

        Ptr<Socket> movSarSocket =GetCachedUdpSocket(sarNode, tos);

        //Ptr<Socket> movSarSocket = Socket::CreateSocket(sarNode, UdpSocketFactory::GetTypeId());
        if (!movSarSocket) {
            NS_FATAL_ERROR("Failed to create socket for SAR Node " << sarNode->GetId());
        }

        // Start moving the SAR node and the civilian toward the base station
        MoveTowardBaseStation(sarNode, civilianNode, g_baseNode, movSarSocket, rescuemovmentport);
            
        return; // Stop further convergence
    }

    // If not attached, move the SAR node towards the civilian
    Ptr<MobilityModel> sarMobility = sarNode->GetObject<MobilityModel>();
    Vector direction = {
        civilianPosition.x - sarPosition.x,
        civilianPosition.y - sarPosition.y,
        civilianPosition.z - sarPosition.z
    };

    // Normalize the direction vector
    double magnitude = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    direction.x /= magnitude;
    direction.y /= magnitude;
    direction.z /= magnitude;

    // Update the SAR node's position
    sarMobility->SetPosition({
        sarPosition.x + direction.x * 10.0, // Step size
        sarPosition.y + direction.y * 10.0,
        sarPosition.z + direction.z * 10.0
    });

    // Schedule the next movement step toward the civilian
    Simulator::Schedule(Seconds(1.0), &ConvergeOnCivilian, sarNode, civilianNode, baseSocket, port);

    // Send status update to the base station
   
   // Send the update to the base station
    struct ConvergenceUpdate {
        uint32_t sarNodeId;
        uint32_t civilianId;
        double timestamp;
        char status[16];
        double sarX;
        double sarY;
        double sarZ;
        double civilianX;
        double civilianY;
        double civilianZ;
        double distanceToBase;
        double distance;
    } update = {
        sarNode->GetId(),
        civilianNode->GetId(),
        timestamp,
        "",
        sarPosition.x, sarPosition.y, sarPosition.z,
        civilianPosition.x, civilianPosition.y, civilianPosition.z,
        distanceToBase,
        distance
    };
    strncpy(update.status, status.c_str(), sizeof(update.status) - 1);
    update.status[sizeof(update.status) - 1] = '\0';

    Ptr<Packet> packet = Create<Packet>((uint8_t *)&update, sizeof(update));
    InetSocketAddress remote = InetSocketAddress(baseStationIp, port);
    CommunicationIdTag commIdTag(convergenceCommId);
    packet->AddPacketTag(commIdTag);
    baseSocket->SendTo(packet, 0, remote);

    // Log the packet event
    uint32_t packetId = packet->GetUid();
    sendTimestamps[packetId] = timestamp;
    LogPacketEvent(
        baseSocket,
        sarNodeIp, 
        baseStationIp,
        packetId,
        0.0, 
        0.0,
        packet->GetSize(),
        port,
        "Convergence Update Sent",
        convergenceCommId,"TX"
    );
}


void AssignRescuer(uint32_t civilianId, Ptr<Node> civilianNode, const NodeContainer& sarNodes, 
    Ptr<Socket> baseBroadcastSocket, uint16_t assignmentPort) {

        std::cout << "[AssignRescuer] called for civilian=" << civilianId
          << " time=" << Simulator::Now().GetSeconds()
          << " stream_open=" << rescueAssignmentLogFile.is_open()
          << " fail=" << rescueAssignmentLogFile.fail()
          << "\n";

// 1) If civilian is already assigned, exit.
if (rescueAssignments.find(civilianId) != rescueAssignments.end()) {
return;
}
  
// 2) Validate the civilian node
if (!civilianNode) {
NS_LOG_ERROR("Civilian Node is null for ID " << civilianId);
return;
}

uint8_t tos = 0x60 ;

// 3) Retrieve the mobility model for the civilian
Ptr<MobilityModel> civilianMobility = civilianNode->GetObject<MobilityModel>();
if (!civilianMobility) {
NS_LOG_ERROR("No MobilityModel found for Civilian Node " << civilianId);
return;
}

// 4) Find the closest SAR node to the civilian
Vector civilianPosition = civilianMobility->GetPosition();
double minDistance = std::numeric_limits<double>::max();
uint32_t assignedNodeId = 0;
Ptr<Node> assignedNode = nullptr;

// Base-assigned: Select the closest SAR node

    for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
        Ptr<Node> sarNode = sarNodes.Get(i);
        if (!sarNode) continue;

        // Skip nodes already assigned
        if (assignedSarNodes.find(sarNode->GetId()) != assignedSarNodes.end()) {
            continue;
        }

        // Skip drones and helicopters
        if (IsDroneOrHelicopter(sarNode)) {
            continue;
        }

        // Check the mobility model of the SAR node
        Ptr<MobilityModel> sarMobility = sarNode->GetObject<MobilityModel>();
        if (!sarMobility) continue;

        // Calculate the distance to the civilian
        double distance = CalculateDistance(civilianNode, sarNode, true);
        if (distance < minDistance) {
            minDistance = distance;
            assignedNodeId = sarNode->GetId();
            assignedNode = sarNode;
        }
    }

// 5) Ensure a suitable rescuer was found
if (!assignedNode) {
    NS_LOG_WARN("All SARs busy. Civilian " << civilianId << " queued for later assignment.");
    pendingCivilians.insert(civilianId);
    return;
}


// 6) Record the assignment
rescueAssignments[civilianId] = assignedNodeId;
assignedSarNodes.insert(assignedNodeId);

// 7) Log the assignment
double timestamp = Simulator::Now().GetSeconds();
Ipv4Address rescuerIp = assignedNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
// std::cout << "[AssignRescuer] &rescueAssignmentLogFile=" << (void*)&rescueAssignmentLogFile
//           << " is_open=" << rescueAssignmentLogFile.is_open()
//           << " fail=" << rescueAssignmentLogFile.fail()
//           << "\n";

        //   std::cout << "[OPEN] rescue addr=" << (void*)&rescueAssignmentLogFile
        //   << " recv addr=" << (void*)&receivedAssignmentsLogFile << "\n";



rescueAssignmentLogFile << timestamp << "," << civilianId << "," << assignedNodeId << "( BASE ASSIGNMENET),"
             << rescuerIp << "," // Include rescuer's IP address
             << civilianPosition.x << "," << civilianPosition.y << "," << civilianPosition.z << "\n";
rescueAssignmentLogFile.flush();



// If this assignment was triggered by the base, broadcast the assignment.

for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
Ptr<Node> sarNode = sarNodes.Get(i);
if (!sarNode) continue;

// Create a new assignment packet for each SAR node
RescueAssignmentData assignmentData = {civilianId, assignedNodeId, 
                                    civilianPosition.x, civilianPosition.y, civilianPosition.z};
Ptr<Packet> packet = Create<Packet>((uint8_t*)&assignmentData, sizeof(assignmentData));

// Generate a unique communication ID for each SAR node
uint32_t communicationId = 20000 + sarNode->GetId();
CommunicationIdTag commIdTag(communicationId);
packet->AddPacketTag(commIdTag);

// Send the packet to the current SAR node
Ipv4Address receiverAddress = sarNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
baseBroadcastSocket->SetIpTos(tos);
InetSocketAddress remote = InetSocketAddress(receiverAddress, assignmentPort);
NodeIdTag tag;
tag.SetNodeId(baseBroadcastSocket->GetNode()->GetId());
packet->AddPacketTag(tag);
baseBroadcastSocket->SendTo(packet, 0, remote);

// Start a communication session for the current SAR node
StartCommunicationSession(
 communicationId,
 baseBroadcastSocket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
 receiverAddress,
 "Rescue Assignment Broadcast",
 Simulator::Now().GetSeconds(),
 "UDP",
 assignmentPort
);

// Log packet event for the current SAR node
uint32_t packetId = packet->GetUid();
double sendTime = Simulator::Now().GetSeconds();
sendTimestamps[packetId] = sendTime;

LogPacketEvent(
 baseBroadcastSocket,
 baseBroadcastSocket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
 receiverAddress,
 packetId,
 0.0,
 0.0,
 packet->GetSize(),
 assignmentPort,
 "Rescue Assignment Packet Broadcast SENT",
 communicationId,"TX"
);
}


// 8) Create a sender socket for the assigned node
    Ptr<Socket> senderSocket =GetCachedUdpSocket(assignedNode, tos);

//Ptr<Socket> senderSocket = Socket::CreateSocket(assignedNode, UdpSocketFactory::GetTypeId());
if (!senderSocket) {
NS_LOG_ERROR("Failed to create sender socket for SAR Node " << assignedNodeId);
return;
}

// 9) Start the convergence process
ConvergeOnCivilian(assignedNode, civilianNode, senderSocket, convergencePort);
}
void TryAssignPendingCivilians()
{
    for (auto it = pendingCivilians.begin(); it != pendingCivilians.end(); )
    {
        uint32_t civId = *it;

        Ptr<Node> civilianNode = NodeList::GetNode(civId);
        if (!civilianNode)
        {
            it = pendingCivilians.erase(it);
            continue;
        }

        // Log that this civilian is currently pending
        pendingAssignmentCsv << Simulator::Now().GetSeconds() << ","
                             << civId << ",PENDING\n";
        pendingAssignmentCsv.flush();

        size_t before = rescueAssignments.size();

        AssignRescuer(
            civId,
            civilianNode,
            sarNodes,
            baseBroadcastSocket,
            assignmentPort
        );

        // If assignment succeeded
        if (rescueAssignments.size() > before)
        {
            pendingAssignmentCsv << Simulator::Now().GetSeconds() << ","
                                 << civId << ",ASSIGNED\n";
            pendingAssignmentCsv.flush();

            it = pendingCivilians.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
void AssignRescuerBySelf(uint32_t civilianId, uint32_t sarNodeId, Ptr<Node> baseNode) {
    // 1) If the civilian is already assigned, exit
    if (rescueAssignments.find(civilianId) != rescueAssignments.end()) return;

    // 2) If the SAR is already assigned, exit
    if (assignedSarNodes.find(sarNodeId) != assignedSarNodes.end()) return;

    sarToCivilianOffset.erase(sarNodeId);
    sarPreviousPositions[sarNodeId].clear();
    trackedConvergencePositions[sarNodeId].clear();
    g_civilianToSarNodeMap.erase(sarNodeId);
    Ptr<Node> sarNode = NodeList::GetNode(sarNodeId);
    Ptr<Node> civilianNode = NodeList::GetNode(civilianId);

    if (!sarNode || !civilianNode || !baseNode) {
        NS_LOG_ERROR("SAR, Civilian, or Base node not found for self-assignment.");
        return;
    }

    // 3) Ensure the SAR is not a drone or helicopter
    if (IsDroneOrHelicopter(sarNode)) return;

    // 4) Retrieve the civilian position for logging
    Ptr<MobilityModel> civilianMobility = civilianNode->GetObject<MobilityModel>();
    if (!civilianMobility) {
        NS_LOG_ERROR("No MobilityModel found for Civilian Node " << civilianId);
        return;
    }
    Vector civilianPosition = civilianMobility->GetPosition();

    // 5) Record the assignment
    rescueAssignments[civilianId] = sarNodeId;
    assignedSarNodes.insert(sarNodeId);

    // 6) Log the assignment
    double timestamp = Simulator::Now().GetSeconds();
    Ipv4Address rescuerIp = sarNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
//   std::cout << "[OPEN] rescue addr=" << (void*)&rescueAssignmentLogFile
//           << " recv addr=" << (void*)&receivedAssignmentsLogFile << "\n";

    rescueAssignmentLogFile << timestamp << "," << civilianId << "," << sarNodeId << "( SELF ASSIGNMENT),"
                            << rescuerIp << "," 
                            << civilianPosition.x << "," << civilianPosition.y << "," << civilianPosition.z << "\n";
   rescueAssignmentLogFile.flush();

    // 7) Create a sender socket
        uint8_t tos=0x60;

        Ptr<Socket> senderSocket =GetCachedUdpSocket(sarNode, tos);

    if (!senderSocket) {
        NS_LOG_ERROR("Failed to create sender socket for SAR Node " << sarNodeId);
        return;
    }

    // ✅ 8) Set the global base node if needed (optional safety)
    g_baseNode = baseNode;

    // ✅ 9) Start the convergence process and pass correct base socket
    ConvergeOnCivilian(sarNode, civilianNode, senderSocket, convergencePort);
}
void CheckAssignmentStatusAndPossiblySelfAssign(
    uint32_t sarNodeId,
    uint32_t civilianNodeId,
    Ptr<Node> baseNode
) {
    if (rescueAssignments.find(civilianNodeId) != rescueAssignments.end())
        return;

    Ptr<Node> sarNode = NodeList::GetNode(sarNodeId);

         // Skip drones and helicopters
     if (IsDroneOrHelicopter(sarNode)) {
            return;
        }

    if (assignedSarNodes.find(sarNodeId) != assignedSarNodes.end()) {
        // SAR still busy — try again later
        Simulator::Schedule(
            Seconds(2.0),
            &CheckAssignmentStatusAndPossiblySelfAssign,
            sarNodeId,
            civilianNodeId,
            baseNode
        );
        return;
    }

    Ptr<Node> civilianNode = NodeList::GetNode(civilianNodeId);
    if (!sarNode || !civilianNode) return;

    std::cout << "[SELF-ASSIGNMENT] SAR " << sarNodeId
              << " self-assigning to Civilian " << civilianNodeId << std::endl;

    AssignRescuerBySelf(civilianNodeId, sarNodeId, baseNode);
}
void LogConvergenceUpdate(Ptr<Socket> socket)
{
    Vector basePosition = g_baseNode->GetObject<MobilityModel>()->GetPosition();

    Address from;
    Ptr<Packet> packet;

    while ((packet = socket->RecvFrom(from)))
    {
        uint32_t packetId = packet->GetUid();
        double receiveTime = Simulator::Now().GetSeconds();

        CommunicationIdTag commIdTag;
        uint32_t commId = 0;
        if (packet->PeekPacketTag(commIdTag))
        {
            commId = commIdTag.GetCommunicationId();
        }

        struct ConvergenceUpdate {
            uint32_t sarNodeId;
            uint32_t civilianId;
            double timestamp;
            char status[16];
            double sarX;
            double sarY;
            double sarZ;
            double civilianX;
            double civilianY;
            double civilianZ;
            double distanceToBase;
            double distance;
        } receivedUpdate;

        if (packet->GetSize() < sizeof(receivedUpdate))
        {
            // malformed/short packet, skip safely
            continue;
        }

        packet->CopyData(reinterpret_cast<uint8_t*>(&receivedUpdate), sizeof(receivedUpdate));

        InetSocketAddress senderAddress = InetSocketAddress::ConvertFrom(from);
        Ipv4Address senderIp = senderAddress.GetIpv4();

        double currentTime = Simulator::Now().GetSeconds();
        convergenceLogFile << currentTime << "," << senderIp << ","
                           << receivedUpdate.sarNodeId << "," << receivedUpdate.civilianId << ","
                           << receivedUpdate.sarX << "," << receivedUpdate.sarY << "," << receivedUpdate.sarZ << ","
                           << receivedUpdate.civilianX << "," << receivedUpdate.civilianY << "," << receivedUpdate.civilianZ << ","
                           << basePosition.x << "," << basePosition.y << "," << basePosition.z << ","
                           << receivedUpdate.distance << "," << receivedUpdate.distanceToBase << ","
                           << "Convergence update sent to base"
                           << "\n";
        convergenceLogFile.flush();

        // delay lookup + erase to prevent RAM growth
        double delay = 0.0;
        auto itTs = sendTimestamps.find(packetId);
        if (itTs != sendTimestamps.end())
        {
            delay = receiveTime - itTs->second;
            sendTimestamps.erase(itTs); // ✅ critical
        }

        double throughput = 0.0;
        if (delay > 1e-9)
        {
            throughput = (packet->GetSize() * 8.0) / delay;
        }

        LogPacketEvent(
            socket,
            socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Destination IP
            senderIp,                                                         // Source IP
            packetId,
            delay,
            throughput,
            packet->GetSize(),
            convergencePort,
            "Convergence Update Received",
            commId,"RX"
        );
    }
}


void ReceiveBroadcastAttachment(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);

     // ... you call packet = socket->RecvFrom(...)
    uint32_t packetId = packet->GetUid();
    double receiveTime = Simulator::Now().GetSeconds();

    // Retrieve the CommunicationIdTag:
    CommunicationIdTag commIdTag;
    uint32_t commId = 0;
    if (packet->PeekPacketTag(commIdTag)) {
        commId = commIdTag.GetCommunicationId();
    }

    struct AttachmentStatus {
        uint32_t sarNodeId;
        uint32_t civilianId;
        double timestamp;
    } status;

    packet->CopyData((uint8_t *)&status, sizeof(status));
    double delay = 0.0;
    auto itTs = sendTimestamps.find(packetId);
    if (itTs != sendTimestamps.end())
    {
        delay = receiveTime - itTs->second;
        sendTimestamps.erase(itTs);   // ✅ critical: frees memory
    }

double throughput = 0.0;
if (delay > 1e-9)
{
    throughput = (packet->GetSize() * 8.0) / delay;
}

    //  LogPacketEvent again, but now filling in the actual delay & throughput
    LogPacketEvent(
        socket,
        socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
        InetSocketAddress::ConvertFrom(from).GetIpv4(),   
        packetId,
        delay,
        throughput,
        packet->GetSize(),
        broadcastPort,
        "Attachmenet braodcast recieved",
        commId,"RX"
    );

    //NS_LOG_INFO("Node " << socket->GetNode()->GetId() << " received broadcast: SAR Node " 
               //  << status.sarNodeId << " attached to Civilian " << status.civilianId 
               //  << " at " << status.timestamp << " seconds.");
}
void ReceiveAssignment(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);


     // Retrieve the source address of the packet
    InetSocketAddress source = InetSocketAddress::ConvertFrom(from);
    Ipv4Address receiverAddress = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    Ipv4Address sourceAddress = source.GetIpv4();
    uint32_t packetId = packet->GetUid();
    double receiveTime = Simulator::Now().GetSeconds();

    // Print the packet content for debugging
    std::ostringstream oss;
    packet->Print(oss);
    //std::cout << "Received Packet (UID: " << packetId << ") Content: " << oss.str() << std::endl;

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
     // Deserialize the rescue assignment data
    RescueAssignmentData data;
    packet->CopyData((uint8_t*)&data, sizeof(data));

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

    // Log to the console
    //NS_LOG_INFO("Node " << socket->GetNode()->GetId() 
                //  << " received rescue assignment: Civilian " << data.civilianId 
                //  << " assigned to SAR Node " << data.assignedNodeId 
                //  << " at Position (" << data.civilianX << ", " << data.civilianY 
                //  << ", " << data.civilianZ << ") from Base Station at " << sourceAddress);

    // Log to file
   if (receivedAssignmentsLogFile.is_open()) {
    // Get the IP address of the assigned node
    Ptr<Node> assignedNode = NodeList::GetNode(data.assignedNodeId);
    Ipv4Address assignedNodeIp = assignedNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    receivedAssignmentsLogFile << Simulator::Now().GetSeconds() << ","  // Simulation time
                               << socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal() << "," // Logs IP address of receiving node
                               << sourceAddress << ","                 // Source/Base Station Address
                               << data.civilianId << ","               // Civilian ID
                               << data.assignedNodeId << ","           // Assigned SAR Node ID (Node Number)
                               << assignedNodeIp << ","                // Assigned SAR Node's IP address
                               << data.civilianX << ","                // Civilian X position
                               << data.civilianY << ","                // Civilian Y position
                               << data.civilianZ << "\n";              // Civilian Z position
    receivedAssignmentsLogFile.flush();  // Force write to disk


    // Log the packet event
    LogPacketEvent(socket,  receiverAddress, senderIp,packetId, delay, throughput, packet->GetSize(),assignmentPort, "Rescue Assignment Received", communicationId,"RX");

    // Clean up after processing
    sendTimestamps.erase(packetId);
}

}
