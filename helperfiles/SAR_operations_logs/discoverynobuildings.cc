struct DiscoveryBroadcastData {
    uint32_t discoveringNodeId; // The SAR node that discovered the civilian
    uint32_t civilianNodeId;    // ID of the discovered civilian
    double distance;            // Distance between the discovering SAR node and the civilian
    double civilianX;           // X-coordinate of the civilian's position
    double civilianY;           // Y-coordinate of the civilian's position
    double civilianZ;           // Z-coordinate of the civilian's position
};
struct DiscoveryData {
    uint32_t sarNodeId;
    uint32_t civilianNodeId;
    double distance;
    uint8_t status; // 1 for Discovered, 0 for Out of Range
    double civilianX = 0.0; // Default initialization
    double civilianY = 0.0; // Default initialization
    double civilianZ = 0.0; // Default initialization
    char signalType[64]; // Fixed-size array to hold the signal type (e.g., WiFi, PLB)

    
};

void CheckAssignmentStatusAndPossiblySelfAssign(uint32_t sarNodeId, 
    uint32_t civilianNodeId,
    Ptr<Node> baseNode
    );
void BroadcastDiscovery(Ptr<Node> senderNode, DiscoveryBroadcastData data, const NodeContainer& sarNodes, uint16_t sarDiscoveryPort) ;

void sendDiscoveryToBase(uint32_t sarNodeId, uint32_t civilianNodeId, double distance, bool inRange,
                        Ptr<Socket> sendSocket, Ipv4Address baseAddress, uint16_t discoverybasePort,
                        double civilianX, double civilianY, double civilianZ, const std::string& signalType,uint32_t communicationId);
//This helper function ensures all discovery types
// (LOS, WiFi, PLB, Query-Response) are logged and forwarded to the base station uniformly.
void ProcessDiscoveryEvent(
    uint32_t sarNodeId,
    uint32_t civilianNodeId,
    double distance,
    const std::string& signalType,
    Vector position,
    Ptr<Socket> discoverySocket,
    Ptr<Node> baseStation,
    uint16_t discoveryPort
    
) {
    if (
        // 1) Civilian is marked safe
        (civilianSafetyStatus.find(civilianNodeId) != civilianSafetyStatus.end() &&
         civilianSafetyStatus[civilianNodeId] == true)
    
        
    )
    {
    NS_LOG_INFO("Civilian " << civilianNodeId << " is already safe. Skipping discovery.");
    return; // No further discovery needed.
}


if (    // 2) Civilian is in g_civilianAttachedState AND is attached (== true)
        (g_civilianAttachedState.find(civilianNodeId) != g_civilianAttachedState.end() &&
         g_civilianAttachedState[civilianNodeId]))
         
         {
          //  NS_LOG_INFO("Civilian " << civilianNodeId << " is already ATTACHED. Skipping discovery.");
            return; // No further discovery needed.
        }
  if (signalType.empty()) {
    centralDiscoveryLogFile << "Error: Signal type is empty for SAR Node " << sarNodeId 
                            << " and Civilian Node " << civilianNodeId << "\n";
    return;  // Exit early to avoid logging incomplete data.
}
     // Check how far the new position is from the last logged position
    Vector lastPos(-1, -1, -1);
    auto it = lastLoggedPosition.find({sarNodeId, civilianNodeId});
    if (it != lastLoggedPosition.end()) {
        lastPos = it->second;
    }

    
    // Log the discovery locally
    discoveryLogFile << Simulator::Now().GetSeconds() << ",SAR Node " << sarNodeId
                     << ",discovered Civilian Node " << civilianNodeId
                     << ", via " << signalType
                     << "," << distance << " meters,"
                     << "DISCOVERED"
                     << "," << position.x << ", " << position.y << ", " << position.z << "\n";
    discoveryLogFile.flush();
    
       
    // Mark discovered
    discoveryState[sarNodeId][civilianNodeId] = true;
    Ptr<Node> sarNode = NodeList::GetNode(sarNodeId);
    if (assignedSarNodes.find(sarNodeId) != assignedSarNodes.end()) {
    //NS_LOG_INFO("SAR Node " << sarNodeId << " is already assigned. Skipping.");
    return;
}
    // Activate dataset for discovery
    ns3::Ipv4Address sarIp = NodeList::GetNode(sarNodeId)->GetObject<ns3::Ipv4>()->GetAddress(1, 0).GetLocal();
    ns3::Ipv4Address civIp = GetNodePublicIp(NodeList::GetNode(civilianNodeId));


    // Check if this SAR node has already discovered this civilian
    if (discoveryState[civilianNodeId].find(sarNodeId) == discoveryState[civilianNodeId].end()) {
        // This SAR node has not discovered this civilian before
        discoveryState[civilianNodeId][sarNodeId] = true; // Mark as discovered
        g_civilianToDiscoveringNodesCount[civilianNodeId]++; // Increment the count of discovering nodes
        g_civilianToDiscoveringNodeIps[civilianNodeId].insert(sarIp); // Add the SAR IP
    }

    // Prepare a list of all discovering nodes for this civilian
    std::vector<uint32_t> allDiscoveringNodeIds;
    for (const auto& entry : discoveryState[civilianNodeId]) {
        allDiscoveringNodeIds.push_back(entry.first);
    }

    // Use a set to ensure unique IPs
    std::set<ns3::Ipv4Address> uniqueDiscoveringNodeIps;

    // Populate unique IPs based on all discovering node IDs
    for (uint32_t nodeId : allDiscoveringNodeIds) {
        Ptr<Node> node = NodeList::GetNode(nodeId);
        if (node) {
            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
            if (ipv4) {
                ns3::Ipv4Address ip = ipv4->GetAddress(1, 0).GetLocal();
                uniqueDiscoveringNodeIps.insert(ip); // Add IP to the set
            } else {
                NS_LOG_WARN("Node " << nodeId << " does not have an Ipv4 object!");
            }
        } else {
            NS_LOG_WARN("Node ID " << nodeId << " is invalid!");
        }
    }

    // Convert the set to a vector for further processing
    std::vector<ns3::Ipv4Address> allDiscoveringNodeIps(
        uniqueDiscoveringNodeIps.begin(), uniqueDiscoveringNodeIps.end());

    // Record the discovery event, including the distance
       uint32_t eventId = RecordDiscoveryEvent(
        civilianNodeId,                     // Civilian ID
        civIp,                              // Civilian IP
        allDiscoveringNodeIds,              // List of all discovering node IDs
        allDiscoveringNodeIps,              // List of all discovering node IPs
        Simulator::Now().GetSeconds() - scenarioStartTime, // Time since scenario started
        signalType                      // Discovery signal type
    );


    // Use eventId to log additional details
    centralDiscoveryLogFile << "Event ID: " << eventId
                            << " - SAR Node " << sarNodeId
                            << " discovered Civilian Node " << civilianNodeId 
                            << " by " << signalType 
                            << "\n"; 

     centralDiscoveryLogFile.flush();

     // -------------------- Integration with BroadcastDiscovery -------------------- //
    // Prepare the discovery data for broadcasting
    DiscoveryBroadcastData broadcastData;
    broadcastData.discoveringNodeId = sarNodeId;
    broadcastData.civilianNodeId = civilianNodeId;
    broadcastData.distance = distance;
    broadcastData.civilianX = position.x;
    broadcastData.civilianY = position.y;
    broadcastData.civilianZ = position.z;

    // Retrieve the SAR node pointer
    // -------------------- Send Discovery Data to Base Station -------------------- //
     // Generate a unique communication ID for the broadcast
    uint32_t communicationIddiscovery = 11000 + sarNodeId;
    
      sendDiscoveryToBase(
        sarNodeId, civilianNodeId, distance, true, discoverySocket,
        baseStation->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), discoverybasePort,
        position.x, position.y, position.z,signalType,communicationIddiscovery  );

    // Broadcast the discovery to all SAR nodes
    BroadcastDiscovery(sarNode, broadcastData, sarNodes, sarDiscoveryPort);


}
void ProcessDiscoveryAck(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);

    if (!packet || packet->GetSize() != sizeof(uint32_t)*2) return;

    struct DiscoveryAck {
        uint32_t sarNodeId;
        uint32_t civilianNodeId;
    } ack;

    packet->CopyData((uint8_t*)&ack, sizeof(ack));

    std::pair<uint32_t,uint32_t> key = {
        ack.sarNodeId,
        ack.civilianNodeId
    };

    g_discoveryAcked[key] = true;

    std::cout << "[ACK RECEIVED] SAR "
              << ack.sarNodeId
              << " Civilian "
              << ack.civilianNodeId << std::endl;
}


double GetThermalImagingRange(Ptr<Node> sarNode, 
    const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes,
    const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes) {
double thermalRange = 10.0;  // Default range

if (IsNodeInContainer(sarNode, droneNodes)) {
thermalRange = 50.0;  // Drones have higher thermal imaging range
} 
else if (IsNodeInContainer(sarNode, helicopterNodes)) {
thermalRange = 70.0;  // Helicopters can scan larger areas
} 
else if (IsNodeInContainer(sarNode, vehicleNodes)) {
thermalRange = 30.0;  // Vehicles have mid-range thermal imaging
} 
else if (IsNodeInContainer(sarNode, footNodes)) {
thermalRange = 15.0;  // Foot teams have lower thermal imaging range
}

//NS_LOG_INFO(" Thermal Imaging Range for SAR Node " << sarNode->GetId() << ": " << thermalRange << " m");
return thermalRange;
}


bool IsNodeInsideStructure(Ptr<Node> node, const BuildingContainer& structures) {
    Vector nodePos = node->GetObject<MobilityModel>()->GetPosition();

    for (uint32_t i = 0; i < structures.GetN(); i++) {
        Ptr<Building> structure = structures.Get(i);
        Box bounds = structure->GetBoundaries();

        if (nodePos.x >= bounds.xMin && nodePos.x <= bounds.xMax &&
            nodePos.y >= bounds.yMin && nodePos.y <= bounds.yMax &&
            nodePos.z >= bounds.zMin && nodePos.z <= bounds.zMax) {
            return true; //  Inside a building or tree
        }
    }
    return false;
}

static inline double ComputeNominalDiscoveryRadius(
    Ptr<Node> node,
    const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes,
    const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes)
{
    if (IsNodeInContainer(node, droneNodes))      return 100.0;
    if (IsNodeInContainer(node, helicopterNodes)) return 120.0;
    if (IsNodeInContainer(node, vehicleNodes))    return 50.0;
    if (IsNodeInContainer(node, footNodes))       return 20.0;
    return 0.0;
}

static inline const char* NodeTypeOf(
    Ptr<Node> n,
    const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes,
    const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes)
{
    for (uint32_t i=0;i<droneNodes.GetN();++i)      if (droneNodes.Get(i)==n)      return "drone";
    for (uint32_t i=0;i<helicopterNodes.GetN();++i) if (helicopterNodes.Get(i)==n) return "helicopter";
    for (uint32_t i=0;i<vehicleNodes.GetN();++i)    if (vehicleNodes.Get(i)==n)    return "vehicle";
    for (uint32_t i=0;i<footNodes.GetN();++i)       if (footNodes.Get(i)==n)       return "foot";
    return "unknown";
}

#include "discoverybuildingnoocclusion.cc"
//include "discoverybuildingocclusion.cc"



void CheckThermalImagingDiscoveryForUndiscovered(
    const NodeContainer& sarNodes,
    const NodeContainer& civilianNodes,
    const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes,
    const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes,
    Ptr<Node> baseStation,
    uint16_t discoveryPort
) {
    for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
        Ptr<Node> sarNode = sarNodes.Get(i);
        uint32_t sarId = sarNode->GetId();
        if (sarNode == baseStation) continue;

    double thermalRange = GetThermalImagingRange(sarNode, droneNodes, vehicleNodes, footNodes, helicopterNodes);
      uint8_t tos = 0xE0; 
        Ptr<Socket> discoverySocket = GetCachedUdpSocket(sarNode, tos);
        if (!discoverySocket) { continue; }

        for (uint32_t j = 0; j < civilianNodes.GetN(); ++j) {
            Ptr<Node> civNode = civilianNodes.Get(j);
            uint32_t civId = civNode->GetId();

            // skip if discovered
            if (IsDiscoveredNoInsert(sarId, civId)) continue;

            double distance = CalculateDistance(sarNode, civNode);
            if (distance <= thermalRange) {
                Vector civPos = civNode->GetObject<MobilityModel>()->GetPosition();
                ProcessDiscoveryEvent(sarId, civId, distance, "Thermal",
                                      civPos, discoverySocket, baseStation, discoveryPort);
            }
        }
    }
}

// ==================================================================
// PASSIVE DISCOVERY = LOS + Thermal
// ==================================================================
void PerformPassiveDiscoveryForUndiscovered(
    const NodeContainer& sarNodes,
    const NodeContainer& civilianNodes,
    Ptr<Node> baseStation,
    uint16_t discoveryPort,
    const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes,
    const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes

) {  CheckLosBasedDiscoveryForUndiscovered(
    sarNodes, civilianNodes, baseStation, discoveryPort,
    droneNodes, vehicleNodes, footNodes, helicopterNodes

);
    CheckThermalImagingDiscoveryForUndiscovered(
        sarNodes, civilianNodes, droneNodes, vehicleNodes, footNodes, helicopterNodes,baseStation, discoveryPort
    );
  

   
}


void DiscoverCivilianNodes(
    const NodeContainer& sarNodes,
    const NodeContainer& civilianNodes,
    Ptr<Node> baseStation,
    uint16_t discoveryPort,
    const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes,
    const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes
    

) {
    Time now = Simulator::Now();

            // Still do partial fallback for undiscovered civilians
        PerformPassiveDiscoveryForUndiscovered(
            sarNodes,
            civilianNodes,
            baseStation,
            discoveryPort,
            droneNodes,
            vehicleNodes,
            footNodes,
            helicopterNodes
            
            

        );
    //  }

    // Reschedule after 3 second
    Simulator::Schedule(Seconds(3.0), &DiscoverCivilianNodes,
                        sarNodes, civilianNodes, baseStation, 
                        discoveryPort, droneNodes, vehicleNodes, 
                        footNodes, helicopterNodes);
}



void BroadcastDiscovery(Ptr<Node> senderNode,
                       DiscoveryBroadcastData data,
                       const NodeContainer& sarNodes,
                       uint16_t sarDiscoveryPort
                       ) // rename param
{
    // (Optional) If the sender is the base station, skip
    if (senderNode->GetId() == 1) {
        //NS_LOG_INFO("Skipping broadcast from Base Station.");
        return;
    }

    uint8_t tos=0x60;

   // uint32_t sartosarCommunicationId= 150000; 

    Ipv4Address senderAddress = senderNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    //NS_LOG_INFO("Broadcasting discovery from Node " << senderNode->GetId());
   
    Ptr<Socket> sendSocket = GetCachedUdpSocket(senderNode, tos);
    if (!sendSocket) { return; }

        for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
        Ptr<Node> receiverNode = sarNodes.Get(i);
        if (senderNode == receiverNode) 
            continue;

        Ipv4Address receiverAddress = receiverNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        InetSocketAddress remote = InetSocketAddress(receiverAddress, sarDiscoveryPort);

        Ptr<Packet> packet = Create<Packet>((uint8_t*)&data, sizeof(data));
        
     


               // Generate a unique key for the sender-receiver pair
    uint32_t receiverNodeId = receiverAddress.Get() ; // Extract integer representation of the receiver
    uint32_t senderNodeId = senderAddress.Get();

    std::pair<uint32_t, uint32_t> sessionKey = std::make_pair(senderNodeId, receiverNodeId);

    // Check if this session already has a communication ID
    uint32_t commIdThisReceiver;
    if (sessionTable.find(sessionKey) != sessionTable.end()) {
        // Reuse existing communication ID
        commIdThisReceiver = sessionTable[sessionKey];
    } else {
        // Create a new communication ID and store it in the session table
        commIdThisReceiver = sartosarCommunicationId++;
        sessionTable[sessionKey] = commIdThisReceiver;
    }

        // Tag the packet
        CommunicationIdTag commIdTag(commIdThisReceiver);
        packet->AddPacketTag(commIdTag);

        // Store send timestamp
        uint32_t packetId = packet->GetUid();
        double sendTime = Simulator::Now().GetSeconds();
        sendTimestamps[packetId] = sendTime;

        // Start a fresh session
        StartCommunicationSession(
            commIdThisReceiver,
            senderAddress,
            receiverAddress,
            "SAR to SAR Discovery Broadcast",
            Simulator::Now().GetSeconds(),
            "UDP",
            sarDiscoveryPort
        );
        NodeIdTag tag;
        tag.SetNodeId(senderNode->GetId());
        packet->AddPacketTag(tag);
        // Send packet
        sendSocket->SendTo(packet, 0, remote);

        // Log
        LogPacketEvent(
            sendSocket,
            senderAddress,
            receiverAddress,
            packetId,
            0.0,
            0.0,
            packet->GetSize(),
            sarDiscoveryPort,
            "SAR TO SAR Discovery Broadcast Sent",
            commIdThisReceiver,"TX"            
        );

        sarDiscoveryBroadcastLogFile << Simulator::Now().GetSeconds() << ","
                                     << receiverNode->GetId() << ","
                                     << senderNode->GetId() << ","
                                     << data.discoveringNodeId << ","
                                     << data.civilianNodeId << ","
                                     << data.distance << ","
                                     << data.civilianX << ","
                                     << data.civilianY << ","
                                     << data.civilianZ << ","
                                     << "Discovered\n";
        sarDiscoveryBroadcastLogFile.flush();

        // std::cout << "Broadcasting discovery from Node " 
        //           << senderNode->GetId() 
        //           << " to Node " << receiverNode->GetId()
        //           << " with Communication ID " << commIdThisReceiver << std::endl;
    }

    // done
    sendSocket->Close();
}

// Initialize the centralized discovery log at the base station
void SetupCentralDiscoveryLogging() {
    centralDiscoveryLogFile.open("central_discovery_logfinalv3.csv");
    centralDiscoveryLogFile << "Time (s),SAR Node ID,Civilian Node ID,Distance To civilian (m), SAR Distance To BASE (m),Status, METHOD OF DISCOVERY,Civilian X,Civilian Y,Civilian Z\n";

   
}

// Initialize the local log file for civilian discovery events
void SetupLocalDiscoveryLogging() {
    discoveryLogFile.open("civilian_discovery_logfinalv3.csv");
    discoveryLogFile << "Time (s),SAR Node ID,Civilian Node ID,METHOD OF DISCOVERY,Distance (m),Status,Civilian X,Civilian Y,Civilian Z\n"; // Headers for local log
}

void OnDiscoveryAckTimeout(
    uint32_t sarNodeId,
    uint32_t civilianNodeId,
    Ipv4Address baseAddress,
    uint16_t discoverybasePort,
    Ptr<Socket> sendSocket,
    double distance,
    double civilianX,
    double civilianY,
    double civilianZ,
    std::string signalType
) {
    std::pair<uint32_t,uint32_t> key = {sarNodeId, civilianNodeId};



    // If ACK received, stop
    if (g_discoveryAcked[key]) return;

    // If already assigned by base, stop
    if (rescueAssignments.count(civilianNodeId)) return;

    uint32_t& retries = g_discoveryRetries[key];

    if (retries < MAX_DISCOVERY_RETRIES) {
        retries++;

        std::cout << "[DISCOVERY RETRY] SAR "
                  << sarNodeId << " -> Civilian "
                  << civilianNodeId
                  << " retry=" << retries << std::endl;

        sendDiscoveryToBase(
            sarNodeId,
            civilianNodeId,
            distance,
            true,
            sendSocket,
            baseAddress,
            discoverybasePort,
            civilianX,
            civilianY,
            civilianZ,
            signalType,
            11000 + sarNodeId + retries
        );
        return;
    }

    // ---- FINAL FALLBACK: SELF ASSIGN ----
    std::cout << "[DISCOVERY FAILOVER] SAR "
              << sarNodeId
              << " Did not recieve ack from base for "
              << civilianNodeId << std::endl;


    double fallbackTimeout = 10.0; // Time limit before checking for self-assignment
    Simulator::Schedule(Seconds(fallbackTimeout), &CheckAssignmentStatusAndPossiblySelfAssign, sarNodeId, civilianNodeId, g_baseNode);

   // AssignRescuerBySelf(civilianNodeId, sarNodeId, g_baseNode);
}

// Function to send log discovery events to the base station’s centralized log
void  sendDiscoveryToBase(uint32_t sarNodeId, uint32_t civilianNodeId, double distance, bool inRange,
                        Ptr<Socket> sendSocket, Ipv4Address baseAddress, uint16_t discoverybasePort,
                        double civilianX, double civilianY, double civilianZ, const std::string& signalType,  uint32_t communicationId) {
    DiscoveryData data;
    data.sarNodeId = sarNodeId;
    data.civilianNodeId = civilianNodeId;
    data.distance = distance;
    data.status = inRange ? 1 : 0;
    data.civilianX = civilianX;
    data.civilianY = civilianY;
    data.civilianZ = civilianZ;
    //data.signalType= signalType;
    uint8_t tos=0xE0 ;
    // Copy the signalType into the char array
    strncpy(data.signalType, signalType.c_str(), sizeof(data.signalType));
    data.signalType[sizeof(data.signalType) - 1] = '\0'; // Ensure null termination


    // Send the data packet
    Ptr<Packet> packet = Create<Packet>((uint8_t *)&data, sizeof(data));
      // Attach the communication ID to the packet
    CommunicationIdTag commIdTag(communicationId);
    packet->AddPacketTag(commIdTag);
    sendSocket->SetIpTos(tos);
    InetSocketAddress remote = InetSocketAddress(baseAddress, discoverybasePort);
    NodeIdTag tag;
    tag.SetNodeId(sarNodeId);
    packet->AddPacketTag(tag);
    sendSocket->SendTo(packet, 0, remote);
    std::cout << "SAR node "<<sarNodeId<<" DISCOVERED CIVILLIAN  " <<civilianNodeId<<" BY "<< signalType << ".\n";                       
           StartCommunicationSession(
        communicationId,                        // Communication ID
        sendSocket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Source IP
        baseAddress,                           // Destination IP (Base Station)
        "Discovery Sent to Base",            // Communication Type
        Simulator::Now().GetSeconds(),         // Start Time
        "UDP",                                  // Protocol
        discoverybasePort
    );

     // Log the packet event
    uint32_t packetId = packet->GetUid();
    double sendTime = Simulator::Now().GetSeconds();
    sendTimestamps[packetId] = sendTime; // Track the send timestamp

    LogPacketEvent(
        sendSocket,                            // Socket
        sendSocket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Source IP
        baseAddress,                           // Destination IP
        packetId,                              // Packet ID
        0.0,                                   // Placeholder for delay
        0.0,                                   // Placeholder for throughput
        packet->GetSize(),  // Packet size
        discoverybasePort,                   
        "SAR Discovery PACKET Sent BASE",               // Packet Type
        communicationId ,"TX"                       // Communication ID
    );

    Vector basePosition = g_baseNode->GetObject<MobilityModel>()->GetPosition();
    Ptr<Node> sarNode = allNodes.Get(sarNodeId);  //  Convert ID to Ptr<Node>
    Vector sarPosition = sarNode->GetObject<MobilityModel>()->GetPosition();


    double distancetoBase=ns3::CalculateDistance(sarPosition, basePosition);
    // Log to the centralized log
     std::string status = inRange ? "Discovered" : "Out of Range";
     centralDiscoveryLogFile << Simulator::Now().GetSeconds() << ","    // Timestamp
                             << data.sarNodeId << ","                  // SAR Node ID
                             << data.civilianNodeId << ","             // Civilian Node ID
                             << data.distance << ","                  // Distance
                             << distancetoBase << ","                  // Distance

                             << status << ","                         // Status (Discovered/Out of Range)
                             << signalType +" sent" << ","                     // Signal Type (WiFi/PLB/LOS/Query Response)
                             << data.civilianX << ","                 // Civilian X Coordinate
                             << data.civilianY << ","                 // Civilian Y Coordinate
                             << data.civilianZ << "\n";               // Civilian Z Coordinate
     centralDiscoveryLogFile.flush();



     // ---- Schedule ACK timeout ----
    std::pair<uint32_t,uint32_t> key = {sarNodeId, civilianNodeId};

    g_discoveryAcked[key] = false;

    Simulator::Schedule(
        Seconds(DISCOVERY_ACK_TIMEOUT),
        &OnDiscoveryAckTimeout,
        sarNodeId,
        civilianNodeId,
        baseAddress,
        discoverybasePort,
        sendSocket,
        distance,
        civilianX,
        civilianY,
        civilianZ,
        signalType
    );

}

// SAR RECIEVEN DISCOVERY BROADCAST
void ReceiveBroadcast(Ptr<Socket> socket) {
    //NS_LOG_INFO("ReceiveBroadcast called");

    // Check which node received the broadcast
    //NS_LOG_INFO("Node ID: " << socket->GetNode()->GetId() << " is processing the received broadcast.");

    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);

    // Log the sender's address (IP and Port)
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
    Ipv4Address senderIp = address.GetIpv4();
    //uint16_t senderPort = address.GetPort();

    //NS_LOG_INFO("Received packet from sender IP: " << senderIp << ", Port: " << senderPort);

    // Log the packet ID and size
    uint32_t packetId = packet->GetUid();
    double receiveTime = Simulator::Now().GetSeconds();
    //NS_LOG_INFO("Packet ID: " << packetId << ", Packet Size: " << packet->GetSize() 
              //  << ", Received at Time: " << receiveTime);

    // Check packet size
    if (packet->GetSize() != sizeof(DiscoveryBroadcastData)) {
        NS_LOG_WARN("Received packet size does not match DiscoveryBroadcastData structure!");
        return;
    }

    // Extract the communication ID tag
    CommunicationIdTag commIdTag;
    uint32_t communicationId = 0;
    if (packet->PeekPacketTag(commIdTag)) {
        communicationId = commIdTag.GetCommunicationId();
        //NS_LOG_INFO("Retrieved Communication ID Tag: " << communicationId);
    } else {
      //  NS_LOG_WARN("Warning: Communication ID tag missing from packet.");
        return;  // Exit early if the tag is missing
    }

    // Calculate delay
    double delay = -1.0;  // Default value indicating no delay calculated
    if (sendTimestamps.find(packetId) != sendTimestamps.end()) {
       
            auto itTs = sendTimestamps.find(packetId);
            if (itTs != sendTimestamps.end())
            {
                delay = receiveTime - itTs->second;
                sendTimestamps.erase(itTs);
            }
            else
            {
                NS_LOG_WARN("Send timestamp not found for Packet ID: " << packetId
                            << ". Delay will not be calculated.");
            }



        //NS_LOG_INFO("Calculated Delay: " << delay << " seconds for Packet ID: " << packetId);
    } else {
        NS_LOG_WARN("Send timestamp not found for Packet ID: " << packetId
                    << ". Delay will not be calculated.");
    }

    // Parse the packet
    DiscoveryBroadcastData data;
    packet->CopyData((uint8_t *)&data, sizeof(data));

    //NS_LOG_INFO("Parsed DiscoveryBroadcastData:");
    //NS_LOG_INFO("  Discovering Node ID: " << data.discoveringNodeId);
    //NS_LOG_INFO("  Civilian Node ID: " << data.civilianNodeId);
    //NS_LOG_INFO("  Distance: " << data.distance);
    //NS_LOG_INFO("  Civilian Position: (" << data.civilianX << ", " 
              //  << data.civilianY << ", " << data.civilianZ << ")");

    // Log the packet event
    LogPacketEvent(
        socket,
        socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Destination IP
        senderIp,                                       // Source IP
        packetId,
        delay,
        (delay > 0 ? (packet->GetSize() * 8) / delay : 0.0), // Throughput (in bits/second)
        packet->GetSize(),                                  // Packet size
        sarDiscoveryPort,                            
        "SAR TO SAR Discovery Broadcast Received",      // Packet type
        communicationId,"RX"                                 // Communication ID
    );

    // Final confirmation of successful reception
    //NS_LOG_INFO("SAR Node " << socket->GetNode()->GetId() 
             //   << " successfully processed broadcast discovery of Civilian Node " 
               // << data.civilianNodeId << ".");
}

void AssignRescuer(uint32_t civilianId, Ptr<Node> civilianNode, const NodeContainer& sarNodes, 
    Ptr<Socket> baseBroadcastSocket, uint16_t assignmentPort);

// Function to process discovery packets at the base station
void ProcessDiscoveryPacket(Ptr<Socket> socket, const NodeContainer& sarNodes, Ptr<Socket> baseBroadcastSocket, uint16_t discoverybasePort) {
     Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);

    if (!packet || packet->GetSize() != sizeof(DiscoveryData)) {
        NS_LOG_ERROR("Received invalid packet or incorrect size!");
        return;
    }

    // Extract the communication ID tag
    CommunicationIdTag commIdTag;
    uint32_t communicationId = 0;
    if (packet->PeekPacketTag(commIdTag)) {
        communicationId = commIdTag.GetCommunicationId();
        //NS_LOG_INFO("Retrieved Communication ID Tag: " << communicationId);
    } else {
     //   NS_LOG_WARN("Warning: Communication ID tag missing from packet.");
        return; // Exit early if the tag is missing
    }

    // Parse the packet
    DiscoveryData data;
    packet->CopyData((uint8_t*)&data, sizeof(data));
    // Access the signalType directly
    std::string signalType(data.signalType);

    if (data.status == 0) {
        //NS_LOG_INFO("Received out-of-range discovery for Civilian " << data.civilianNodeId);
        return;
    }

    // Log the packet-level metadata
    uint32_t packetId = packet->GetUid();
    double receiveTime = Simulator::Now().GetSeconds();
        double delay = 0.0;
        auto itTs = sendTimestamps.find(packetId);
        if (itTs != sendTimestamps.end())
        {
            delay = receiveTime - itTs->second;
            sendTimestamps.erase(itTs); // ✅ free memory
        }
    LogPacketEvent(
        socket,
        socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), // Destination IP
        InetSocketAddress::ConvertFrom(from).GetIpv4(), // Source IP
        packetId,
        delay,
        (packet->GetSize() * 8) / (delay > 0 ? delay : 1), // Throughput (prevent divide-by-zero)
        packet->GetSize(),
        discoverybasePort,
        "SAR Discovery Packet Received at Base",
        communicationId,"RX"
    );
    
    // Find the SAR node using `data.sarNodeId`
    Ptr<Node> sarNode = nullptr;
    for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
        if (sarNodes.Get(i)->GetId() == data.sarNodeId) {
            sarNode = sarNodes.Get(i);
            break;
        }
    }

    if (!sarNode) {
        NS_LOG_ERROR("SAR Node " << data.sarNodeId << " not found!");
        return;
    }

   
    //  Get Positions
    Vector sarPosition = sarNode->GetObject<MobilityModel>()->GetPosition();
    Vector basePosition = g_baseNode->GetObject<MobilityModel>()->GetPosition();

    //  Calculate Distance
    double distanceToBase = ns3::CalculateDistance(sarPosition, basePosition);


    //Log the received data to a centralized log
    centralDiscoveryLogFile << Simulator::Now().GetSeconds() << "," // Timestamp
                            << data.sarNodeId << ","              // SAR Node ID
                            << data.civilianNodeId << ","         // Civilian Node ID
                            << data.distance << ","               // Distance
                            <<distanceToBase<<","
                            << (data.status == 1 ? "Discovered" : "Out of Range") << "," // Status
                            << signalType + " recieved"<< ","
                            << data.civilianX << ","              // Civilian X Coordinate
                            << data.civilianY << ","              // Civilian Y Coordinate
                            << data.civilianZ << "\n";            // Civilian Z Coordinate
    centralDiscoveryLogFile.flush();

    //NS_LOG_INFO("Base Station received discovery packet: SAR Node " << data.sarNodeId
              //   << " discovered Civilian " << data.civilianNodeId);
    // ---- SEND ACK BACK TO SAR ----
        struct DiscoveryAck {
            uint32_t sarNodeId;
            uint32_t civilianNodeId;
        };

        DiscoveryAck ack;
        ack.sarNodeId = data.sarNodeId;
        ack.civilianNodeId = data.civilianNodeId;

        Ptr<Packet> ackPacket = Create<Packet>((uint8_t*)&ack, sizeof(ack));

        InetSocketAddress sarAddr = InetSocketAddress(
            InetSocketAddress::ConvertFrom(from).GetIpv4(),
            discoveryAckPort   
        );

        socket->SendTo(ackPacket, 0, sarAddr);

    // Ensure the civilian node exists
    Ptr<Node> civilianNode = NodeList::GetNode(data.civilianNodeId);
    if (!civilianNode) {
        NS_LOG_ERROR("Civilian Node " << data.civilianNodeId << " does not exist!");
        return;
    }




    // Assign a rescuer using the AssignRescuer function
    AssignRescuer(data.civilianNodeId, civilianNode, sarNodes, baseBroadcastSocket, assignmentPort);

    

}
// Wrapper function for callback
void ProcessDiscoveryPacketWrapper(Ptr<Socket> socket) {
    extern  NodeContainer sarNodes;          // SAR node container
    extern Ptr<Socket> baseBroadcastSocket; // Broadcast socket for assignments
    extern uint16_t discoverybasePort;          // Broadcast port

    if (!baseBroadcastSocket) {
        NS_LOG_ERROR("Base broadcast socket is null!");
        return;
    }

    if (sarNodes.GetN() == 0) {
        NS_LOG_ERROR("SAR nodes container is empty!");
        return;
    }

    ProcessDiscoveryPacket(socket, sarNodes, baseBroadcastSocket, discoverybasePort);
}
