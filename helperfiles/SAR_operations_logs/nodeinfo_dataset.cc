double frequency = 2.4e9;  // Frequency in Hz (2.4 GHz)


struct NodeInfo {
    uint32_t nodeId;
    std::string nodeType;
    std::string nodeDevice;
    std::string nodeRole;
    double initialX, initialY, initialZ;
    double finalX, finalY, finalZ;
    std::string mobilityModel;
    double speed;
    std::string connectivityLevel;
    std::string powerType;
     double initialEnergy=0;  // Primary battery energy (Joules)
    double backupEnergy=0;   // Backup battery energy (Joules)
     double communicationRange;
    std::string customAttributes;
    std::string nodeBehavior;
    double transmitPower;           // dBm
    double receiverSensitivity;     // dBm
    double receivedPower;     // dBm

    double antennaGainTx;           // dB
    double antennaGainRx;           // dB
    double noiseFigure ;
    double distance ;
    double channelBandwidth;  // Hz
    std::string pathLossModel ;
    double maxBandwidth;     // dBm


};


std::map<std::string, NodeInfo> nodeTypeAttributes; // Map to store attributes based on roles


// Simulation time
//double simulationTime =40.0;
// Global map: nodeId -> NodeInfo
static std::map<uint32_t, NodeInfo> nodeDataset;
//


// To track initial/final positions, packet counts, connectivity, etc.
static std::unordered_map<uint32_t, bool> initialPosRecorded;
static std::unordered_map<uint32_t, Vector> initialPositions;
static std::unordered_map<uint32_t, Vector> finalPositions;

// Track how long each node is "connected" to any other node
static std::unordered_map<uint32_t, double> connectedTime;

// Track maximum distance of successful reception for each node to estimate communication range
static std::unordered_map<uint32_t, double> maxReceptionDistance;

// Track packets sent and received for behavior assessment
static std::unordered_map<uint32_t, uint32_t> packetSentCount;
static std::unordered_map<uint32_t, uint32_t> packetReceivedCount;


std::map<ns3::Ipv4Address, uint32_t> ipToNodeIdMap; // Global map

std::map<std::string, double> mcsToMaxBandwidth = {
    {"HtMcs0", 6.5},  // Mbps
    {"HtMcs1", 13.0},
    {"HtMcs2", 19.5},
    {"HtMcs3", 26.0},
    {"HtMcs4", 39.0},
    {"HtMcs5", 52.0},
    {"HtMcs6", 58.5},
    {"HtMcs7", 65.0}  // Max for 802.11n
};
void AssignMaxBandwidthToNodes(NodeContainer nodes, std::string dataRate)
{
    double maxBandwidth = mcsToMaxBandwidth[dataRate];  // Lookup in mapping

    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        uint32_t nodeId = nodes.Get(i)->GetId();
        if (nodeDataset.find(nodeId) != nodeDataset.end()) {
            nodeDataset[nodeId].maxBandwidth = maxBandwidth;  // Assign max bandwidth
        }
    }
}


void PopulateIpToNodeIdMap(NodeContainer nodes) {
    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        Ptr<Node> node = nodes.Get(i);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        if (ipv4) {
            for (uint32_t j = 0; j < ipv4->GetNInterfaces(); ++j) {
                for (uint32_t k = 0; k < ipv4->GetNAddresses(j); ++k) {
                    ns3::Ipv4Address ip = ipv4->GetAddress(j, k).GetLocal();
                    if (ip != Ipv4Address::GetZero()) { // Skip unassigned interfaces
                        ipToNodeIdMap[ip] = node->GetId(); // Map IP to node ID
                    }
                }
            }
        }
    }
}


// uint32_t GetNodeIdFromIp(ns3::Ipv4Address ip) {
//     auto it = ipToNodeIdMap.find(ip);
//     if (it != ipToNodeIdMap.end()) {
//         return it->second; // Return the node ID
//     } else {
//         NS_LOG_WARN("IP address " << ip << " not found in the map.");
//         return -1; // Or handle the error as needed
//     }
// }


// Helper function to calculate received power using a path loss model
double CalculateReceivedPower(double transmitPower, double distance, double antennaGainTx, double antennaGainRx, const std::string& pathLossModel) {


    if (distance <= 0) {
        NS_LOG_WARN("Invalid distance (" << distance << "). Returning low power.");
        return -1e9; // Return very low power for invalid distance
    }

    if (pathLossModel == "FreeSpace" || pathLossModel == "Friis") {
        double wavelength = 3e8 / frequency;  // Wavelength = speed of light / frequency

        // Friis Path Loss formula
        double pathLoss = 20 * std::log10(4 * M_PI * distance / wavelength);  // dB

        // Total received power (including transmit power and antenna gains)
        double receivedPower = transmitPower + antennaGainTx + antennaGainRx - pathLoss;

        // Debugging
        //NS_LOG_INFO("Friis Model -> Distance: " << distance << " m, Path Loss: " << pathLoss
                    // << " dB, Received Power: " << receivedPower << " dBm");
        return receivedPower;
    } 
    else if (pathLossModel == "LogDistance") {
        double pathLossExponent = 3.0;  // Path loss exponent for urban environments
        double referenceDistance = 1.0;  // Reference distance in meters
        double referencePower = transmitPower + antennaGainTx + antennaGainRx - 20 * std::log10(referenceDistance);

        double pathLoss = 10 * pathLossExponent * std::log10(distance / referenceDistance);
        double receivedPower = referencePower - pathLoss;

        //NS_LOG_INFO("Log-Distance Model -> Distance: " << distance << " m, Path Loss: " << pathLoss
                  //   << " dB, Received Power: " << receivedPower << " dBm");
        return receivedPower;
    } 
    else if (pathLossModel == "TwoRayGround") {
        double ht = 1.5;  // Transmitter antenna height in meters
        double hr = 1.5;  // Receiver antenna height in meters

        double pathLoss = 10 * std::log10((ht * hr) * (ht * hr) / (distance * distance * distance * distance));
        double receivedPower = transmitPower + antennaGainTx + antennaGainRx - pathLoss;

        //NS_LOG_INFO("Two-Ray Ground Model -> Distance: " << distance << " m, Path Loss: " << pathLoss
                   //  << " dB, Received Power: " << receivedPower << " dBm");
        return receivedPower;
    }

    NS_LOG_WARN("Unknown path loss model: " << pathLossModel << ". Returning 0.");
    return -1e9; // Default invalid value
}

// Helper function to calculate noise power
double CalculateNoisePower(double bandwidth, double noiseFigure) {
    double k = 1.38e-23;  // Boltzmann constant
    double T = 290;  // Room temperature in Kelvin
    return 10 * std::log10(k * T * bandwidth) + noiseFigure;  // dBm
}

// Helper function to determine connectivity level based on SNR

std::string GetDynamicConnectivityLevel(const NodeInfo& nodeInfo, double distance, double bandwidth, double noiseFigure, const std::string& pathLossModel) {
    // Open a debug log file in append mode
    std::ofstream debugFile("connectivity_debug.log");

    // Check if the file opened successfully
    if (!debugFile.is_open()) {
        std::cerr << "Error: Could not open debug log file." << std::endl;
        return "unknown";
    }

    // Write debug information to the file
    debugFile << "---- Debug: Connectivity Level Calculation ----" << std::endl;
    debugFile << "Transmit Power (dBm): " << nodeInfo.transmitPower << std::endl;
    debugFile << "Distance (m): " << distance << std::endl;
    debugFile << "Antenna Gain Tx (dB): " << nodeInfo.antennaGainTx << std::endl;
    debugFile << "Antenna Gain Rx (dB): " << nodeInfo.antennaGainRx << std::endl;
    debugFile << "Bandwidth (Hz): " << bandwidth << std::endl;
    debugFile << "Noise Figure (dB): " << noiseFigure << std::endl;
    debugFile << "Path Loss Model: " << pathLossModel << std::endl;

    // Calculate received power
    double receivedPower = CalculateReceivedPower(
        nodeInfo.transmitPower,    // Transmit Power
        distance,                  // Distance
        nodeInfo.antennaGainTx,    // Transmitter Antenna Gain
        nodeInfo.antennaGainRx,    // Receiver Antenna Gain
        pathLossModel              // Path Loss Model
    );
    debugFile << "Calculated Received Power (dBm): " << receivedPower << std::endl;

    // Calculate noise power
    double noisePower = CalculateNoisePower(bandwidth, noiseFigure);
    debugFile << "Calculated Noise Power (dBm): " << noisePower << std::endl;

    // Calculate SNR
    double snr = 10 * std::log10(std::pow(10, receivedPower / 10) / std::pow(10, noisePower / 10));
    debugFile << "SNR (dB): " << snr << std::endl;

    // Determine connectivity level
    std::string connectivityLevel;
    if (snr <= 15) 
        connectivityLevel = "An unreliable connection (" + std::to_string(snr) + ")";
    else if (snr > 15 && snr < 25) 
        connectivityLevel = "A poor connection (" + std::to_string(snr) + ")";
    else if (snr > 24 && snr < 40) 
        connectivityLevel = "A good connection (" + std::to_string(snr) + ")";
    else if (snr > 40) 
        connectivityLevel = "An excellent connection (" + std::to_string(snr) + ")";
    else
        connectivityLevel = "unknown";

    // Write connectivity level to the debug file
    debugFile << "Connectivity Level: " << connectivityLevel << std::endl;
    debugFile << "-------------------------------------------------" << std::endl;

    // Close the debug file
    debugFile.close();

    return connectivityLevel;
}

// Helper function to calculate communication range dynamically
double CalculateCommunicationRange(const NodeInfo& nodeInfo, double pathLossExponent) {
    return std::pow(10, (nodeInfo.transmitPower - nodeInfo.receiverSensitivity) / (10 * pathLossExponent));
}


void PositionChangeCallback(std::string context, Ptr<const MobilityModel> mobility) {
    uint32_t nodeId = mobility->GetObject<Node>()->GetId();
    Vector position = mobility->GetPosition();
    Vector velocity = mobility->GetVelocity();
    double speed = std::sqrt(velocity.x * velocity.x +
                             velocity.y * velocity.y +
                             velocity.z * velocity.z);

    // Initial position tracking
    if (!initialPosRecorded[nodeId]) {
        initialPositions[nodeId] = position;
        initialPosRecorded[nodeId] = true;
    }
    finalPositions[nodeId] = position;

    // ✅ Log to file
    if (positionLogFile.is_open()) {
        positionLogFile <<seed<<","
                        <<runNumber<<","
                        <<routingsize<<","
                        << Simulator::Now().GetSeconds() << ","  // Timestamp
                        << nodeId << ","
                        << position.x << "," << position.y << "," << position.z << ","
                        << velocity.x << "," << velocity.y << "," << velocity.z << ","
                        << speed << "\n";  // New: logs velocity components and speed
    } else {
        NS_LOG_ERROR("Could not open position_logfinalv1.csv for writing.");
    }
}



void UpdateNodeAttributes(uint32_t nodeId) {
    NodeInfo& nodeInfo = nodeDataset[nodeId];  // Access the node's information

    // Adjust path loss exponent dynamically based on node type
    double pathLossExponent = (nodeInfo.nodeRole == "Drone" || nodeInfo.nodeRole == "Helicopter") ? 2.0 : 3.0;

    // Update communication range dynamically
    nodeInfo.communicationRange = CalculateCommunicationRange(nodeInfo, pathLossExponent);

    // Use dynamically updated distance for calculations
    double distance = maxReceptionDistance[nodeId];
    if (distance <= 0) {
        distance = 1.0;  // Avoid zero distance
    }
    nodeInfo.distance = distance;  // Store the updated distance

    // Open a debug log file in append mode
    std::ofstream debugFile("connectivity_debugs.log");
    if (!debugFile.is_open()) {
        std::cerr << "Error: Could not open debug log file." << std::endl;
    } else {
        debugFile << "---- Debug: UpdateNodeAttributes ----" << std::endl;
        debugFile << "Node ID: " << nodeId << std::endl;
        debugFile << "Distance: " << distance << " meters" << std::endl;
    }

    // Update connectivity level
    nodeInfo.connectivityLevel = GetDynamicConnectivityLevel(
        nodeInfo, distance, nodeInfo.channelBandwidth, nodeInfo.noiseFigure, nodeInfo.pathLossModel
    );

    if (debugFile.is_open()) {
        debugFile << "Updated Connectivity Level: " << nodeInfo.connectivityLevel << std::endl;
    }

    // Calculate Received Power (P_r) dynamically using the Friis model
    if (distance > 0) {
        nodeInfo.receivedPower = CalculateReceivedPower(
            nodeInfo.transmitPower,    // Transmit Power
            distance,                  // Distance between nodes
            nodeInfo.antennaGainTx,    // Transmitter Antenna Gain
            nodeInfo.antennaGainRx,    // Receiver Antenna Gain
            nodeInfo.pathLossModel     // Path Loss Model
        );
    } else {
        nodeInfo.receivedPower = 0.0; // Default for invalid distances
    }

    if (debugFile.is_open()) {
        debugFile << "Calculated Received Power: " << nodeInfo.receivedPower << " dBm" << std::endl;
        debugFile << "Communication Range: " << nodeInfo.communicationRange << " meters" << std::endl;
        debugFile << "-------------------------------------------------\n";
        debugFile.close();  // Close the debug file
    }

    // Log updates for debugging in the simulator log
    //NS_LOG_INFO("Node " << nodeId 
        // << ": Connectivity Level = " << nodeInfo.connectivityLevel
        // << ", Communication Range = " << nodeInfo.communicationRange
        // << ", Received Power = " << nodeInfo.receivedPower << " dBm");
}

void ScheduleNodeUpdates() {
    for (auto& entry : nodeDataset) {
        uint32_t nodeId = entry.first;
        UpdateNodeAttributes(nodeId);
    }
    Simulator::Schedule(Seconds(1.0), &ScheduleNodeUpdates);  // Schedule updates every second
}
void InitializeNodeDeviceAttributes(NodeInfo& nodeInfo) {
    if (nodeInfo.nodeDevice == "Handheld Radio") {
        nodeInfo.pathLossModel = "Friis";  // Set path loss model
        nodeInfo.transmitPower = 15.0;
        nodeInfo.receiverSensitivity = -85.0;
        nodeInfo.antennaGainTx = 2.0;
        nodeInfo.antennaGainRx = 2.0;
        nodeInfo.initialEnergy = 0;   
        nodeInfo.backupEnergy = 0;  
    } else if (nodeInfo.nodeDevice == "Onboard WiFi") {
        nodeInfo.pathLossModel = "Friis";
        nodeInfo.transmitPower = 20.0;
        nodeInfo.receiverSensitivity = -90.0;
        nodeInfo.antennaGainTx = 3.0;
        nodeInfo.antennaGainRx = 3.0;
        nodeInfo.initialEnergy = 0;   
         nodeInfo.backupEnergy = 0;  
    } else if (nodeInfo.nodeDevice == "Aerial WiFi") {
        nodeInfo.pathLossModel = "Friis";
        nodeInfo.transmitPower = 25.0;
        nodeInfo.receiverSensitivity = -95.0;
        nodeInfo.antennaGainTx = 5.0;
        nodeInfo.antennaGainRx = 5.0;
        nodeInfo.initialEnergy = 0;   
        nodeInfo.backupEnergy = 0;  
    } else if (nodeInfo.nodeDevice == "High-Gain Relay") {
        nodeInfo.pathLossModel = "Friis";
        nodeInfo.transmitPower = 30.0;
        nodeInfo.receiverSensitivity = -100.0;
        nodeInfo.antennaGainTx = 8.0;
        nodeInfo.antennaGainRx = 8.0;
        nodeInfo.initialEnergy = 0;   
         nodeInfo.backupEnergy = 0;  
    }else if (nodeInfo.nodeDevice == "Fixed High-Gain") {
        nodeInfo.pathLossModel = "Friis";
        nodeInfo.transmitPower = 30.0;
        nodeInfo.receiverSensitivity = -120.0;
        nodeInfo.antennaGainTx = 10.0;
        nodeInfo.antennaGainRx = 10.0;
        nodeInfo.initialEnergy = 0;   
        nodeInfo.backupEnergy = 0;  
        
     }else if (nodeInfo.nodeDevice == "PLB") {
        nodeInfo.pathLossModel = "Friis";
        nodeInfo.transmitPower = 20.0;
        nodeInfo.receiverSensitivity = -100.0;
        nodeInfo.antennaGainTx = 2.0;
        nodeInfo.antennaGainRx = 2.0;
        nodeInfo.initialEnergy = 0;   
        nodeInfo.backupEnergy = 0;  
    }else if (nodeInfo.nodeDevice == "Hybrid msgs reciever") {
        nodeInfo.pathLossModel = "Friis";
        nodeInfo.transmitPower = 15.0;
        nodeInfo.receiverSensitivity = -90.0;
        nodeInfo.antennaGainTx = 2.0;
        nodeInfo.antennaGainRx = 2.0;
        nodeInfo.initialEnergy = 0;   
        nodeInfo.backupEnergy = 0;  
    } else {
        nodeInfo.pathLossModel = "Friis";  // Default path loss model
        nodeInfo.transmitPower = 10.0;
        nodeInfo.receiverSensitivity = -80.0;
        nodeInfo.antennaGainTx = 1.0;
        nodeInfo.antennaGainRx = 1.0;
        nodeInfo.initialEnergy = 0;   
         nodeInfo.backupEnergy = 0;  
    }
}
void ConfigureNodeWiFiAttributes(NodeContainer &container) {
    std::ofstream logFile("WiFiConfigurationLogfinalv1.txt", std::ios::app); // Open log file in append mode

    if (!logFile.is_open()) {
        std::cerr << "Error: Unable to open WiFi configuration log file." << std::endl;
        return;
    }

    for (uint32_t i = 0; i < container.GetN(); ++i) {
        Ptr<Node> node = container.Get(i);
        uint32_t nodeId = node->GetId();

        // Check if the node exists in the dataset
        if (nodeDataset.find(nodeId) != nodeDataset.end()) {
            NodeInfo& info = nodeDataset[nodeId];

            // Retrieve the device and its PHY
            Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice>(node->GetDevice(0)); // Assumes first device is WiFi
            if (wifiDevice) {
                Ptr<WifiPhy> phy = wifiDevice->GetPhy();
                if (phy) {


                    DoubleValue txPower, rxSensitivity, txGain, rxGain;
                    phy->GetAttribute("TxPowerStart", txPower);
                    phy->GetAttribute("RxSensitivity", rxSensitivity);
                    phy->GetAttribute("TxGain", txGain);
                    phy->GetAttribute("RxGain", rxGain);

                     logFile<< "BEFORE ASSIGNING NODES PARAMTERES Node ID: " << nodeId
                            << " | TxPower: " << txPower.Get()
                            << " | RxSensitivity: " << rxSensitivity.Get()
                            << " | TxGain: " << txGain.Get()
                            << " | RxGain: " << rxGain.Get() << "\n";

                    phy->SetAttribute("TxPowerStart", DoubleValue(info.transmitPower));
                    phy->SetAttribute("TxPowerEnd", DoubleValue(info.transmitPower));
                    phy->SetAttribute("RxSensitivity", DoubleValue(info.receiverSensitivity));
                    
                    phy->SetAttribute("TxGain", DoubleValue(info.antennaGainTx));
                    phy->SetAttribute("RxGain", DoubleValue(info.antennaGainRx));

                    //   DoubleValue txPower, rxSensitivity, txGain, rxGain;
                    phy->GetAttribute("TxPowerStart", txPower);
                    phy->GetAttribute("RxSensitivity", rxSensitivity);
                    phy->GetAttribute("TxGain", txGain);
                    phy->GetAttribute("RxGain", rxGain);

                    // Log configuration details to the file AND CHECKING AGINST DEFULAT VALUES
                    logFile << "Configured Node ID: " << nodeId
                            << " | TxPower: " << info.transmitPower
                            << " | RxSensitivity: " << info.receiverSensitivity
                            << " | TxGain: " << info.antennaGainTx
                            << " | RxGain: " << info.antennaGainRx << "\n"
                            << "AFTER ASSIGNING PARAMETERS Node ID: " << nodeId
                            << " | TxPower: " << txPower.Get()
                            << " | RxSensitivity: " << rxSensitivity.Get()
                            << " | TxGain: " << txGain.Get()
                            << " | RxGain: " << rxGain.Get() << "\n";




                } else {
                    logFile << "Error: No PHY layer found for Node ID: " << nodeId << "\n";
                }
            } else {
                logFile << "Error: No WiFi device found for Node ID: " << nodeId << "\n";
            }
        } else {
            logFile << "Node " << nodeId << " not found in dataset, skipping WiFi configuration.\n";
        }
    }

    logFile.close(); // Close the file after processing all nodes
}


    // A helper function to assign NodeType and NodeRole based on container
void AssignTypeAndRole(NodeContainer& container, const std::string& type, const std::string& role, const std::string& device) {
    for (uint32_t i = 0; i < container.GetN(); ++i) {
        uint32_t nodeId = container.Get(i)->GetId();
        NodeInfo info;
        info.nodeId = nodeId;
        info.nodeType = type;
        info.nodeRole = role;
        info.nodeDevice= device;
      
// Initialize device-specific attributes
        InitializeNodeDeviceAttributes(info);
        // Store in dataset
        nodeDataset[nodeId] = info;

           }
}
std::string GetCustomAttribute(const std::string &nodeType) {
    if (nodeType == "Base Station") return "CentralHub";
    if (nodeType == "Drone" || nodeType == "Helicopter") return "AerialUnit";
    if (nodeType == "Vehicle" || nodeType == "Foot Team") return "GroundTeam";
    if (nodeType == "Civilian") return "CivilianCluster";
    return "None";
}

//void DeductCommunicationEnergy(uint32_t nodeId, bool isTransmitting);
void TrackPacketSent(std::string context, Ptr<const Packet> packet) {
    uint32_t nodeId = Simulator::GetContext();
  //  DeductCommunicationEnergy(nodeId, true);

    packetSentCount[nodeId]++;
    std::ofstream out("MacTxRxLogfinalv1.txt");
    out << "Node " << nodeId << " sent a packet. Context: " << context 
        << ". Total sent: " << packetSentCount[nodeId] << "\n";
    out.close();
}


void TrackPacketReceived(std::string context, Ptr<const Packet> packet) {
    uint32_t nodeId = Simulator::GetContext();
   // DeductCommunicationEnergy(nodeId, false); // Deduct energy for reception

    packetReceivedCount[nodeId]++;
    std::ofstream out("MacTxRxLogfinalv1.txt");
    out << "Node " << nodeId << " received a packet. Context: " << context 
        << ". Total received: " << packetReceivedCount[nodeId] << "\n";
    out.close();
}
// void LogPacketSize(Ptr<const Packet> packet) {
//     uint32_t packetSizeBytes = packet->GetSize();
//     std::cout << "Packet size: " << packetSizeBytes << " bytes" << std::endl;

//     // Optionally log to a file
//     std::ofstream logFile("PacketSizeLogfinalv1.csv", std::ios::app);
//     if (logFile.tellp() == 0) { // Add a header if the file is empty
//         logFile << "Time (s),Packet Size (bytes)\n";
//     }
//     logFile << Simulator::Now().GetSeconds() << "," << packetSizeBytes << "\n";
//     logFile.close();
// }


// void LogDataRate(Ptr<WifiMac> wifiMac) {
//     DataRate dataRate = wifiMac->GetDataRate();
//     double dataRateMbps = dataRate.GetBitRate() / 1e6; // Convert to Mbps
//     std::cout << "Data rate: " << dataRateMbps << " Mbps" << std::endl;

//     // Optionally log to a file
//     std::ofstream logFile("DataRateLogfinalv1.csv", std::ios::app);
//     if (logFile.tellp() == 0) { // Add a header if the file is empty
//         logFile << "Time (s),Data Rate (Mbps)\n";
//     }
//     logFile << Simulator::Now().GetSeconds() << "," << dataRateMbps << "\n";
//     logFile.close();
// }



std::string GetNodeBehavior(uint32_t nodeId) {
    Vector initialPos = initialPositions[nodeId];
    Vector finalPos = finalPositions[nodeId];
    double distMoved = std::sqrt((finalPos.x - initialPos.x)*(finalPos.x - initialPos.x) +
                                 (finalPos.y - initialPos.y)*(finalPos.y - initialPos.y) +
                                 (finalPos.z - initialPos.z)*(finalPos.z - initialPos.z));

    uint32_t sent = packetSentCount[nodeId];
    uint32_t recv = packetReceivedCount[nodeId];

    //NS_LOG_INFO("Node " << nodeId << ": distMoved=" << distMoved << ", sent=" << sent << ", recv=" << recv);

    if (distMoved == 0.0 && (sent > 0 || recv > 0)) {
        return "Static movement but sending and receiving";
    } else if (distMoved == 0.0 &&  (sent == 0 || recv == 0)) {
        return "Static and IDLE";
    } else if (distMoved > 0.0 &&  (sent == 0 || recv == 0)) {
        return "Moving but not sending or receiving";
    } else if (distMoved > 0.0 && (sent > 0 || recv > 0)) {
        return "Moving and sending or receiving";
    
    } else {
        return "Unknown";
    }
}

// CONNECTING MOBILITY TRACE TO GET THE SPEED AVERAGE:

static void VelocityTrace (Ptr<const MobilityModel> mobility)
{
  // The MobilityModel belongs to a certain Node, so we find that node ID
  Ptr<Node> node = mobility->GetObject<Node> ();
  uint32_t nodeId = node->GetId ();
  
  // Get the velocity
  Vector velocity = mobility->GetVelocity();
  
  // Calculate speed in meters per second
  double speedMps = std::sqrt(velocity.x * velocity.x +
                              velocity.y * velocity.y +
                              velocity.z * velocity.z);
  
  // Convert to km/h
  double speedKph = speedMps * 3.6;

  // Accumulate into global sums
  g_nodeSpeedSum[nodeId]   += speedKph;  // Store in km/h
  g_nodeSpeedCount[nodeId] += 1;
}




    std::string GetMobilityModelName(Ptr<Node> node) {
    Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
    if (DynamicCast<RandomWalk2dMobilityModel>(mob)) return "RandomWalk2dMobilityModel";
    if (DynamicCast<GaussMarkovMobilityModel>(mob)) return "GaussMarkovMobilityModel";
    if (DynamicCast<RandomWaypointMobilityModel>(mob)) return "RandomWaypointMobilityModel";
    if (DynamicCast<ConstantPositionMobilityModel>(mob)) return "ConstantPositionMobilityModel";
    // Add more as needed
    return "Unknown";
}

// std::string GetConnectivityLevel(uint32_t nodeId) {
//     double ratio = (connectedTime.count(nodeId) > 0) ? connectedTime[nodeId] / simulationTime : 0.0;
//     if (ratio >= 0.5) return "High";
//     else if (ratio >= 0.2) return "Medium";
//     else return "Low";
// }


void WriteNodeInfoDataset(const std::string& filename) {
    std::ofstream file(filename);
    file << "NodeID,IP Address,NodeType,NodeRole,Device,Primary Battery Capacitiy, Backup Battery Capacity,TransmitPower,ReceiverSensitivity,MaxBandwidth(Mbps),Channel Bandwidth (MHz),Noise,Path Loss Model,antennaGainTx ,antennaGainRx,InitialX,InitialY,InitialZ,FinalX,FinalY,FinalZ,"
         << "MobilityModel,Speed KM/H,ConnectivityLevel,CommunicationRange (KM),CustomAttributes,NodeBehavior\n";

    for (auto &entry : nodeDataset) {
        uint32_t nodeId = entry.first;
        NodeInfo &info = entry.second;

        // Retrieve IP address if available
        std::string ipAddress = "Unknown";
        for (const auto &ipEntry : ipToNodeIdMap) {
            if (ipEntry.second == nodeId) {
                std::ostringstream ipStream;
                ipStream << ipEntry.first; // Converts Ipv4Address to string
                ipAddress = ipStream.str();
                break;
            }
        }

        file << info.nodeId << "," << ipAddress << "," << info.nodeType << "," << info.nodeRole << "," << info.nodeDevice << ","
            << info.initialEnergy << "," << info.backupEnergy << "," 
             << info.transmitPower << "," << /*info.receivedPower << "," <<*/ info.receiverSensitivity << ","<<info.maxBandwidth<<"," <<( (info.channelBandwidth)/1000000) << "," << info.noiseFigure << ","
             << info.pathLossModel << "," << info.antennaGainTx << "," << info.antennaGainRx << "," //<< info.distance << ","
             << info.initialX << "," << info.initialY << "," << info.initialZ << ","
             << info.finalX << "," << info.finalY << "," << info.finalZ << ","
             << info.mobilityModel << "," << info.speed << "," << info.connectivityLevel << ","
             << (info.communicationRange/1000) << "," << info.customAttributes << ","
             << info.nodeBehavior << "\n";
    }

    file.close();
}
