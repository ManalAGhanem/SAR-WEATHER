
//Initialize civilian safety status
void InitializeCivilianSafetyStatus(const NodeContainer& civilianNodes) {
    for (uint32_t i = 0; i < civilianNodes.GetN(); ++i) {
        uint32_t civilianId = civilianNodes.Get(i)->GetId();
        civilianSafetyStatus[civilianId] = false; // Initially, no civilian is safe
    }
}
void LogRescueStatus(Ptr<Node> sarNode, Ptr<Node> civilianNode, Ptr<Node> baseNode, const std::string& status) {
    double timestamp = Simulator::Now().GetSeconds();

    // Get positions of SAR, civilian, and base nodes
    Vector sarPosition = sarNode->GetObject<MobilityModel>()->GetPosition();
    Vector civilianPosition = civilianNode->GetObject<MobilityModel>()->GetPosition();
    Vector basePosition = baseNode->GetObject<MobilityModel>()->GetPosition();

    // Calculate distance to the base station
    double sarDistanceToBase = std::sqrt(
        std::pow(sarPosition.x - basePosition.x, 2) +
        std::pow(sarPosition.y - basePosition.y, 2) +
        std::pow(sarPosition.z - basePosition.z, 2)
    );

     double CivilianDistanceToBase = std::sqrt(
        std::pow(civilianPosition.x - basePosition.x, 2) +
        std::pow(civilianPosition.y - basePosition.y, 2) +
        std::pow(civilianPosition.z - basePosition.z, 2)
    );
    // Unified log entry for rescue status
    rescueCompletionLogFile << timestamp << "," 
                            << sarNode->GetId() << "," 
                            << civilianNode->GetId() << ","
                            << sarPosition.x << "," << sarPosition.y << "," << sarPosition.z << ","
                            << civilianPosition.x << "," << civilianPosition.y << "," << civilianPosition.z << ","
                            << sarDistanceToBase << ","  // Log distance to base station
                             << CivilianDistanceToBase << ","  // Log distance to base station
                            << basePosition.x << "," << basePosition.y << "," << basePosition.z << ","
                            << status << "\n";
    rescueCompletionLogFile.flush();

    // Log info with distance to base
    //NS_LOG_INFO("SAR Node " << sarNode->GetId() 
                //  << " updated Civilian " << civilianNode->GetId() 
                //  << " status to " << status << ". Positions: SAR(" 
                //  << sarPosition.x << "," << sarPosition.y << "," << sarPosition.z 
                //  << "), Civilian(" << civilianPosition.x << "," << civilianPosition.y << "," << civilianPosition.z 
                //  << "). Distance to Base: " << sarDistanceToBase << " meters.");
}

void MarkCivilianAsSafe(Ptr<Node> sarNode, Ptr<Node> civilianNode, Ptr<Node> baseNode)
{
    uint8_t tos = 0x60;

    const uint32_t civilianId = civilianNode->GetId();

    //  1. Fast early-out before any allocations or logging
    auto itSafe = civilianSafetyStatus.find(civilianId);
    if (itSafe != civilianSafetyStatus.end() && itSafe->second) {
        return;
    }
    civilianSafetyStatus[civilianId] = true;

    std::cout << " Civilian " << civilianId << " is SAFE" << std::endl;

    const uint32_t safetyCommIdBase = 50000;

    //  2. Log status once
    LogRescueStatus(sarNode, civilianNode, baseNode, "Safe");

    //  3. Get base station IP once
    Ptr<Ipv4> baseIpv4 = baseNode->GetObject<Ipv4>();
    if (!baseIpv4) {
        NS_LOG_ERROR("Base node has no Ipv4 object!");
        return;
    }
    Ipv4Address baseStationIp = baseIpv4->GetAddress(1, 0).GetLocal();

    //  4. Prepare payload once
    const double timestamp = Simulator::Now().GetSeconds();
    Ptr<MobilityModel> sarMob = sarNode->GetObject<MobilityModel>();
    Ptr<MobilityModel> civMob = civilianNode->GetObject<MobilityModel>();

    if (!sarMob || !civMob) {
        NS_LOG_ERROR("Mobility model missing for SAR or civilian in MarkCivilianAsSafe");
        return;
    }

    Vector sarPosition      = sarMob->GetPosition();
    Vector civilianPosition = civMob->GetPosition();

    struct SafeStatus {
        uint32_t sarNodeId;
        uint32_t civilianId;
        char status[16];
        double timestamp;
        double sarX, sarY, sarZ;
        double civilianX, civilianY, civilianZ;
    } safeStatus = {
        sarNode->GetId(),
        civilianNode->GetId(),
        "Safe",
        timestamp,
        sarPosition.x, sarPosition.y, sarPosition.z,
        civilianPosition.x, civilianPosition.y, civilianPosition.z
    };

    //  5. Create ONE unicast socket on the base for this wave of sends

    Ptr<Socket> unicastSocket =GetCachedUdpSocket(baseNode, tos);


    //  6. Fan-out to other SAR nodes
    for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
        Ptr<Node> targetSarNode = sarNodes.Get(i);
        if (targetSarNode->GetId() == sarNode->GetId()) {
            continue; // skip sender
        }

        Ptr<Ipv4> targetIpv4 = targetSarNode->GetObject<Ipv4>();
        if (!targetIpv4) {
            NS_LOG_WARN("No Ipv4 object for Node " << targetSarNode->GetId());
            continue;
        }

        Ipv4Address targetIp = targetIpv4->GetAddress(1, 0).GetLocal();

        // Unique communication ID for this particular target SAR node
        uint32_t individualCommId = safetyCommIdBase + i;

        StartCommunicationSession(
            individualCommId,
            baseStationIp,             // Source IP
            targetIp,                  // Destination IP
            "Civilian Safety Status Update",
            Simulator::Now().GetSeconds(),
            "UDP",
            CivilianSafePort
        );

        Ptr<Packet> packet = Create<Packet>((uint8_t*)&safeStatus, sizeof(safeStatus));

        uint32_t packetId = packet->GetUid();
        double sendTime   = Simulator::Now().GetSeconds();

        // ⚠ If you do not absolutely need per-packet delay for these,
        // you can comment out the next line to avoid map growth:
        sendTimestamps[packetId] = sendTime;

        NodeIdTag tag;
        tag.SetNodeId(unicastSocket->GetNode()->GetId());
        packet->AddPacketTag(tag);

        CommunicationIdTag commIdTag(individualCommId);
        packet->AddPacketTag(commIdTag);

        InetSocketAddress remote(targetIp, CivilianSafePort);
        unicastSocket->SendTo(packet, 0, remote);

        LogPacketEvent(
            unicastSocket,
            baseStationIp,
            targetIp,
            packetId,
            0.0,             
            0.0,             
            packet->GetSize(),
            CivilianSafePort,
            "Civilian safety status sent",
            individualCommId,"TX"
        );
    }


    //  7. Clean up assignment & tracking maps (this is already good!)
    const uint32_t sarId = sarNode->GetId();
    assignedSarNodes.erase(sarId);
    rescueAssignments.erase(civilianId);
    sarToCivilianOffset.erase(sarId);
    sarPreviousPositions.erase(sarId);
    trackedConvergencePositions.erase(sarId);
    g_civilianAttachedState.erase(civilianId);
    g_civilianToSarNodeMap.erase(sarId);


    //  8. Debug print + reassign pending civilians
    std::cout << "[PENDING @ " << Simulator::Now().GetSeconds() << "] "
              << "count=" << pendingCivilians.size() << " | ";

    for (uint32_t pendingId : pendingCivilians) {
        std::cout << pendingId << " ";
    }
    std::cout << std::endl;

    TryAssignPendingCivilians();
}
        struct MovementUpdate {
        uint32_t sarNodeId;
        uint32_t civilianId;
        double timestamp;

        double sarX, sarY, sarZ; // SAR Node Position
        double distanceToBase;
        double civilianX, civilianY, civilianZ;  // Civilian Position
    };
    
void MoveTowardBaseStation(
        Ptr<Node> sarNode, 
        Ptr<Node> civilianNode, 
        Ptr<Node> baseNode, 
        Ptr<Socket> broadcastSocket, 
        uint16_t rescuemovmentport )
 {
        // 1) Initialize movement communication ID
        uint8_t tos = 0x60;
       
        uint32_t movementCommId = 40000 + sarNode->GetId(); 
    
        // Retrieve IP addresses for logging and communication
        Ipv4Address sarNodeIp      = sarNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        Ipv4Address baseStationIp  = baseNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    
        StartCommunicationSession(
            movementCommId,
            sarNodeIp,         
            baseStationIp,     
            "Move to safety",  
            Simulator::Now().GetSeconds(),
            "UDP",             
            rescuemovmentport
        );
    
        // 2) Get current positions
        Vector sarPosition      = sarNode->GetObject<MobilityModel>()->GetPosition();
        Vector basePosition     = baseNode->GetObject<MobilityModel>()->GetPosition();
        Vector civilianPosition = civilianNode->GetObject<MobilityModel>()->GetPosition();
    
        // 3) Store the initial offset between SAR & Civilian (only once)
        if (sarToCivilianOffset.find(sarNode->GetId()) == sarToCivilianOffset.end()) {
            sarToCivilianOffset[sarNode->GetId()] = civilianPosition - sarPosition;
        }
    
        // Ensure SAR position logging is tracked correctly
        if (sarPreviousPositions.find(sarNode->GetId()) == sarPreviousPositions.end()) {
            sarPreviousPositions[sarNode->GetId()] = {};
        }
    
        //  F Start tracking movement only from the first step of rescue
        auto &hist = sarPreviousPositions[sarNode->GetId()];
        hist.push_back(sarPosition);

        static const size_t MAX_RES_CUE_TRACK = 600; // e.g., keep last 10 minutes at 1 Hz
        if (hist.size() > MAX_RES_CUE_TRACK)
        {
            hist.erase(hist.begin(), hist.begin() + (hist.size() - MAX_RES_CUE_TRACK));
        }


        // Compute the distance to the base station
        double sarDistanceToBase      = ::CalculateDistance(sarPosition, basePosition, true);
        double civilianDistanceToBase = ::CalculateDistance(civilianPosition, basePosition, true);
    
        // 4) Check if they have reached the base
        if (sarDistanceToBase <= sarProxmitytoBase &&
            civilianDistanceToBase <= civilianProxmitytoBase) 
        {
            uint32_t civilianNodeId = civilianNode->GetId();
            if (g_civilianToEventIdMap.find(civilianNodeId) == g_civilianToEventIdMap.end()) {
                NS_LOG_ERROR("Event ID for Civilian Node " << civilianNodeId << " not found!");
                return;
            }
            uint32_t eventId = g_civilianToEventIdMap[civilianNodeId];
    
            // Record rescue completion
            RecordRescueComplete(eventId, Simulator::Now().GetSeconds(),
                                 sarNode, baseNode, civilianNode,
                                 sarPreviousPositions[sarNode->GetId()]);
            sarPreviousPositions[sarNode->GetId()].clear();
    
            MarkCivilianAsSafe(sarNode, civilianNode, baseNode);
    
            std::cout << " [FINAL LOG] SAR & Civilian have reached base. Movement stopped. " 
                      << std::endl;
            return; 
        }
    
        // 5) Compute movement direction (shared movement)
        Vector moveDirection = {
            basePosition.x - sarPosition.x,
            basePosition.y - sarPosition.y,
            basePosition.z - sarPosition.z
        };
        double moveMagnitude = std::sqrt(
            moveDirection.x * moveDirection.x +
            moveDirection.y * moveDirection.y +
            moveDirection.z * moveDirection.z
        );
        if (moveMagnitude > 0.0) {
            moveDirection.x /= moveMagnitude;
            moveDirection.y /= moveMagnitude;
            moveDirection.z /= moveMagnitude;
        }
    
        // Move the group together
        double stepSize = std::min(8.0, sarDistanceToBase / 2.0);
        double minZ = 2.0;
    
        Vector newSarPosition = {
            sarPosition.x + moveDirection.x * stepSize,
            sarPosition.y + moveDirection.y * stepSize,
            std::max(sarPosition.z + moveDirection.z * stepSize, minZ)
        };
    
        // 6) Move Civilian to Maintain Fixed Offset
        Ptr<MobilityModel> sarMobility = sarNode->GetObject<MobilityModel>();
        Ptr<MobilityModel> civilianMobility = civilianNode->GetObject<MobilityModel>();
    
        // Maintain the **same offset** for the civilian relative to SAR
        Vector newCivilianPosition = newSarPosition + sarToCivilianOffset[sarNode->GetId()];
    
        // Assign new positions
        sarMobility->SetPosition(newSarPosition);
        civilianMobility->SetPosition(newCivilianPosition);
    
        // 7) Compute new distances after movement
        double newSarDistanceToBase      = ::CalculateDistance(newSarPosition, basePosition, true);
        double newCivilianDistanceToBase = ::CalculateDistance(newCivilianPosition, basePosition, true);
        double distanceBetweenSARAndCivilian = ::CalculateDistance(newSarPosition, newCivilianPosition, true);
            Ptr<Socket> movementSocket =GetCachedUdpSocket(sarNode, tos);

        InetSocketAddress remote = InetSocketAddress(baseStationIp, rescuemovmentport);
    
        std::ostringstream msg;
        msg << "MovementUpdate,"
        << Simulator::Now().GetSeconds()  // Timestamp
        << ",SAR " << sarNode->GetId()
        << ",Position=(" << newSarPosition.x << "," << newSarPosition.y << "," << newSarPosition.z << ")"
        << ",DistanceToBase=" << newSarDistanceToBase
        << ",Civilian " << civilianNode->GetId()
        << ",Position=(" << newCivilianPosition.x << "," << newCivilianPosition.y << "," << newCivilianPosition.z << ")"
        << ",DistanceToBase=" << newCivilianDistanceToBase;
    
        MovementUpdate update = {
            sarNode->GetId(),
            civilianNode->GetId(),
            Simulator::Now().GetSeconds(),
            newSarPosition.x, newSarPosition.y, newSarPosition.z,
            newSarDistanceToBase,
            newCivilianPosition.x, newCivilianPosition.y, newCivilianPosition.z
        };
    
        Ptr<Packet> packet = Create<Packet>((uint8_t*)&update, sizeof(update));
        uint32_t packetId = packet->GetUid();
        sendTimestamps[packetId] = Simulator::Now().GetSeconds();

        NodeIdTag tag;
        tag.SetNodeId(sarNode->GetId());
        packet->AddPacketTag(tag);
        movementSocket->SendTo(packet, 0, remote);
        // Store updated civilian distances in a global map
        g_civilianLatestDistanceToBase[civilianNode->GetId()] = newCivilianDistanceToBase;
    

       const uint32_t pktSize = packet->GetSize();
       const uint32_t pktId   = packet->GetUid();

        // Throughput at TX is not meaningful here unless you define it.
        LogPacketEvent(
            movementSocket,
            sarNodeIp,
            baseStationIp,
            pktId,
            0.0,           
            0.0,            
            pktSize,
            rescuemovmentport,
            "SAR Move to safety Update Sent",
            movementCommId,
            "TX"

        );

    
        // NS_LOG_INFO("📤 Sent Movement Update from SAR " << sarNode->GetId() 
        //              << " to Base Station at " << Simulator::Now().GetSeconds() << "s.");
        // 8) Log movement step-by-step
        std::cout << "[STEP LOG] Time=" << Simulator::Now().GetSeconds() << "s, "
                  << "SAR Node " << sarNode->GetId()
                  << " -> (" << newSarPosition.x << ", " << newSarPosition.y << ", " << newSarPosition.z << "); "
                  << "DistToBase=" << newSarDistanceToBase
                  << " | Civ Node " << civilianNode->GetId()
                  << " -> (" << newCivilianPosition.x << ", " << newCivilianPosition.y << ", " << newCivilianPosition.z << "); "
                  << "DistToBase=" << newCivilianDistanceToBase
                  << " | Distance Between SAR & Civilian=" << distanceBetweenSARAndCivilian
                  << std::endl;
    
        // Log to file
        movementLogFile << Simulator::Now().GetSeconds() << ","
                        << "Movement Update Sent"
                        << "SAR Node " << sarNode->GetId() << "," << newSarPosition.x << "," 
                        << newSarPosition.y << "," 
                        << newSarPosition.z << ","
                        << "Civilian Node " << civilianNode->GetId() << "," << newCivilianPosition.x << "," 
                        << newCivilianPosition.y << "," 
                        << newCivilianPosition.z << "," 
                        << "DistToBase," << newSarDistanceToBase
                        << "\n";
        movementLogFile.flush();
    
        // 9) Schedule the next movement step
        Simulator::Schedule(Seconds(1.0), &MoveTowardBaseStation,
                            sarNode, civilianNode, baseNode,
                            broadcastSocket, rescuemovmentport);
    }
 
void LogResucerMovmentUpdate(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet;
    while ((packet = socket->RecvFrom(from))) {
    

   // Ptr<Packet> packet = socket->RecvFrom(from);

     uint32_t packetId = packet->GetUid();
    double receiveTime = Simulator::Now().GetSeconds();

    // Retrieve the CommunicationIdTag:
    CommunicationIdTag commIdTag;
    uint32_t commId = 0;
    if (packet->PeekPacketTag(commIdTag)) {
        commId = commIdTag.GetCommunicationId();
    }
         // Parse the incoming movement update
    MovementUpdate receivedUpdate;
    packet->CopyData(reinterpret_cast<uint8_t*>(&receivedUpdate), sizeof(receivedUpdate));

    // Retrieve sender's address
    InetSocketAddress senderAddress = InetSocketAddress::ConvertFrom(from);
    Ipv4Address senderIp = senderAddress.GetIpv4();
     double delay = 0.0;
    auto itTs = sendTimestamps.find(packetId);
    if (itTs != sendTimestamps.end())
    {
        delay = receiveTime - itTs->second;
        sendTimestamps.erase(itTs); //  critical
    }

    double throughput = 0.0;
    if (delay > 1e-9) { throughput = (packet->GetSize() * 8.0) / delay;}

    // Then call LogPacketEvent again, but now filling in the actual delay & throughput
    LogPacketEvent(
        socket,
        socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
        senderIp,
        packetId,
        delay,
        throughput,
        packet->GetSize(),
        rescuemovmentport,
        "SAR Move to safety Update Received",
        commId,"RX"
    );

   
      //  Log movement data for analysis (SAR & Civilian)
            movementLogFile << receiveTime << ","
            << "Movement update Received,"
            <<senderIp<<","
            << receivedUpdate.sarX << "," << receivedUpdate.sarY << "," << receivedUpdate.sarZ << ","
            << "Civilian " << receivedUpdate.civilianId << ","

            << receivedUpdate.civilianX << "," << receivedUpdate.civilianY << "," << receivedUpdate.civilianZ
            << receivedUpdate.distanceToBase <<","
            <<","
            << "\n";

       
        movementLogFile.flush();

    //NS_LOG_INFO("Base Station received movement update: Rescuer Node " << receivedUpdate.sarNodeId
                //  << " Civilian Node " << receivedUpdate.civilianId
                //  << " Position (" << receivedUpdate.sarX << ", " << receivedUpdate.sarY << ", " << receivedUpdate.sarZ
                //  << "), Distance to Base: " << receivedUpdate.distanceToBase);
}}

void HandleSafetyBroadcast(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet;

    while ((packet = socket->RecvFrom(from))) {
        if (!packet) {
            NS_LOG_ERROR("Received null packet");
            continue;
        }

        uint32_t packetId    = packet->GetUid();
        double   receiveTime = Simulator::Now().GetSeconds();

        // Get Communication ID tag (optional)
        CommunicationIdTag commIdTag;
        uint32_t commId = 0;
        if (packet->PeekPacketTag(commIdTag)) {
            commId = commIdTag.GetCommunicationId();
        } else {
            NS_LOG_WARN("CommunicationIdTag not found in packet");
        }

       
        //Use iterator so we don't do double lookup or accidentally insert
        double delay = 0.0;
        auto it = sendTimestamps.find(packetId);
        if (it != sendTimestamps.end()) {
            delay = receiveTime - it->second;

            // We’re done with this packetId → free memory
            sendTimestamps.erase(it);
        } else {
            NS_LOG_WARN("Packet ID " << packetId << " not found in sendTimestamps");
        }

        // Log event with updated delay and throughput
        Ptr<Ipv4> localIpv4 = socket->GetNode()->GetObject<Ipv4>();
        if (!localIpv4) {
            NS_LOG_ERROR("No Ipv4 on node receiving safety broadcast");
            continue;
        }

        Ipv4Address dstIp = localIpv4->GetAddress(1, 0).GetLocal();
        Ipv4Address srcIp = InetSocketAddress::ConvertFrom(from).GetIpv4();

        double throughput = 0.0;
        if (delay > 0.0) {
            throughput = (packet->GetSize() * 8.0) / delay;  // bits per second
        }

        LogPacketEvent(
            socket,
            dstIp,          
            srcIp,         
            packetId,
            delay,
            throughput,
            packet->GetSize(),
            CivilianSafePort,
            "Civilian safety broadcast received",
            commId,"RX"
        );
    }
}


void CheckAllCiviliansSafe(const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes,
    const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes) {  
    uint32_t safeCount = 0;
    uint32_t totalCivilians = civilianSafetyStatus.size();
    Vector basePosition = GetBaseStationPosition(); // Get dynamic base station position

    //  Count Safe Civilians BEFORE printing the status
    for (const auto& entry : civilianSafetyStatus) {
        if (entry.second == true) {  
            safeCount++;  //  Correctly count safe civilians
        }
    }

    uint32_t remaining = totalCivilians - safeCount;  //  Compute remaining civilians

    std::cout << "\n==========================" << std::endl;
    std::cout << "[🔍 STATUS UPDATE] Safe Civilians: " << safeCount
              << " / " << totalCivilians
              << " | Remaining: " << remaining << std::endl;

    std::cout << "----------------------------" << std::endl;

    std::cout << "🟢 Safe Civilians (Rescued):" << std::endl;

    for (const auto& entry : civilianSafetyStatus) {
        if (entry.second) {  //  Civilian is marked as safe
            uint32_t civilianId = entry.first;
            std::cout << " Civilian " << civilianId << " is SAFE." << std::endl;
        }
    }
    std::cout << "----------------------------" << std::endl;
    std::cout << "📌 Attached Civilians & Their SAR Nodes:" << std::endl;

    //  Remove civilians from attached list if they are safe
    std::vector<uint32_t> civiliansToRemove;
    for (const auto& pair : g_civilianToSarNodeMap) {
        uint32_t sarNodeId = pair.first;
        uint32_t civilianId = pair.second;

        if (civilianSafetyStatus[civilianId]) { 
            civiliansToRemove.push_back(sarNodeId); //  Mark for removal
            continue;  //  Skip printing safe civilians
        }

        Ptr<Node> civilianNode = NodeList::GetNode(civilianId);
        Ptr<Node> sarNode = NodeList::GetNode(sarNodeId);

        if (civilianNode && sarNode) {
            Vector civilianPos = civilianNode->GetObject<MobilityModel>()->GetPosition();
            double distanceToBase = g_civilianLatestDistanceToBase.count(civilianId) ?  
                                    g_civilianLatestDistanceToBase[civilianId] :  
                                    CalculateDistance(civilianPos, basePosition, true);
                                 
            std::cout << "🟢 Civilian " << civilianId
                      << " is attached to SAR Node " << sarNodeId
                      << " | Distance to Base: " << distanceToBase << "m" << std::endl;
        }
    }

    //  Remove safe civilians from the attached list
    for (uint32_t sarNodeId : civiliansToRemove) {
        g_civilianToSarNodeMap.erase(sarNodeId);
    }

    std::cout << "----------------------------" << std::endl;
    std::cout << "❌ Missing Civilians (Not Rescued Yet):" << std::endl;
    
    for (const auto& entry : civilianSafetyStatus) {
        uint32_t civilianId = entry.first;
    
        if (!entry.second) {  //  Only print missing civilians
            Ptr<Node> civilianNode = NodeList::GetNode(civilianId);
            if (civilianNode) {
                Vector civilianPos = civilianNode->GetObject<MobilityModel>()->GetPosition();
                
                //  Get latest known distance to base
                double distanceToBase = g_civilianLatestDistanceToBase.count(civilianId) ?  
                                        g_civilianLatestDistanceToBase[civilianId] :  
                                        CalculateDistance(civilianPos, basePosition, true);
    
                 bool isDiscovered = false;
                std::vector<uint32_t> discoveringNodes;

                auto it = g_civilianToEventIdMap.find(civilianId);
                if (it != g_civilianToEventIdMap.end()) {
                    uint32_t eventId = it->second;
                    // eventId is 1-based indexing into g_discoveryConvergenceEvents
                    if (eventId > 0 && eventId <= g_discoveryConvergenceEvents.size()) {
                        const auto& event = g_discoveryConvergenceEvents[eventId - 1];
                        isDiscovered = true;
                        discoveringNodes = event.discoveringNodeIds;
                    }
                }

    
                //  Format discovery status
                std::ostringstream discoveryStatus;
                if (isDiscovered) {
                    discoveryStatus << " - DISCOVERED by SAR Nodes: ";
                    for (uint32_t nodeId : discoveringNodes) {
                        discoveryStatus << nodeId << " ";
                    }
                } else {
                    discoveryStatus << " - NOT DISCOVERED";
                }
    
                std::cout << "❌ Civilian " << civilianId
                          << " | Distance to Base: " << distanceToBase << "m"
                          << discoveryStatus.str() << std::endl;
    
                //  Now Calculate Distance to Each SAR Node
                double minDistance = std::numeric_limits<double>::max(); // Track closest SAR node
                uint32_t closestSarId = std::numeric_limits<uint32_t>::max(); // Set to the maximum possible value
    
                for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
                    Ptr<Node> sarNode = sarNodes.Get(i);
                    uint32_t sarId = sarNode->GetId();
                    
                    Vector sarPos = sarNode->GetObject<MobilityModel>()->GetPosition();
                    double distanceToSAR = ns3::CalculateDistance(sarPos, civilianPos);
    
                    if (distanceToSAR < minDistance) {
                        minDistance = distanceToSAR;
                        closestSarId = sarId;
                    }
                }
    
                //  Print the closest SAR node distance + thermal & discovery range
            if (closestSarId != std::numeric_limits<uint32_t>::max()) {
                Ptr<Node> closestSarNode = NodeList::GetNode(closestSarId);
                double thermalRange = GetThermalImagingRange(closestSarNode, droneNodes, vehicleNodes, footNodes, helicopterNodes);
                double losRange = GetDiscoveryRadius(closestSarNode, droneNodes, vehicleNodes, footNodes, helicopterNodes);

                std::cout << "   🔍 Closest SAR Node: " << closestSarId 
                        << " | Distance: " << minDistance << "m"
                        << " |  Thermal Imaging Range: " << thermalRange << "m"
                        << " |  LOS Discovery Range: " << losRange << "m"
                        << std::endl;
            }

            }
        }
    }
    
    
    std::cout << "==========================\n" << std::endl;
    

    //  If all civilians are safe, order SAR nodes to return
    if (safeCount == totalCivilians) {
        NS_LOG_INFO(" All civilians are safe! Ordering SAR units to return to base...");
    

       Simulator::Stop();

        return;
    }

    //  Recheck in 1 second
    Simulator::Schedule(Seconds(1.0), &CheckAllCiviliansSafe, droneNodes,vehicleNodes, footNodes, helicopterNodes);
}
