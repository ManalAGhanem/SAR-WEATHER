
//-----------------------start energy setup-------------------------------
static std::map<uint32_t, Ptr<ns3::energy::EnergySource>> nodeEnergySources;

struct EnergyMetrics {
    uint32_t nodeId;
    double initialEnergy;    // Initial energy in J
    double residualEnergy; // Remaining energy in J
    double totalEnergyUsed; // Total energy used in J
    
};
static std::map<uint32_t, EnergyMetrics> energyDataset;
// Callback for energy depletion
void EnergyDepletionCallback(uint32_t nodeId) {
    double currentTime = Simulator::Now().GetSeconds();

    // Log energy depletion event
    std::ofstream logFile("EnergyDepletionLogv56.txt");
    logFile << "Time: " << currentTime << "s, Node " << nodeId
            << " has depleted its energy!" << std::endl;
    logFile.close();

    std::cout << "Time: " << currentTime << "s, Node " << nodeId
              << " has depleted its energy!" << std::endl;

    // Retrieve the node and its Ipv4 object
    Ptr<Node> node = NodeList::GetNode(nodeId);
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();

    if (!ipv4) {
        NS_LOG_ERROR("No Ipv4 object found for Node " << nodeId);
        return;
    }

    // Disable all interfaces for the node
    for (uint32_t i = 0; i < ipv4->GetNInterfaces(); ++i) {
        Ptr<Ipv4Interface> iface = ipv4->GetObject<Ipv4L3Protocol>()->GetInterface(i);
        if (iface) {
            NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "s Setting interface DOWN for Node "
                          << nodeId << ", IP: " << iface->GetAddress(0).GetLocal());
            ipv4->SetDown(i); // Disable the interface
        }
    }
}

static Ptr<ns3::energy::EnergySource> CreateBatteryForDevice(const NodeInfo &nodeInfo) {

    
    if (nodeInfo.nodeDevice == "Fixed High-Gain" ) {
        Ptr<ns3::energy::BasicEnergySource> basicSrc = CreateObject<ns3::energy::BasicEnergySource>();
        basicSrc->SetInitialEnergy(1e9); // Near-infinite energy.
        return basicSrc;
    } else {
       // Ptr<ns3::energy::GenericBatteryModel> battery = CreateObject<ns3::energy::GenericBatteryModel>();
        GenericBatteryModelHelper batteryHelper;
        Ptr<ns3::energy::GenericBatteryModel> battery;
        battery = CreateObject<ns3::energy::GenericBatteryModel>();
        Ptr<Node> node = NodeList::GetNode(nodeInfo.nodeId);
       
        
        if (nodeInfo.nodeDevice == "High-Gain Relay"|| nodeInfo.nodeDevice == "Onboard WiFi"){
          battery = DynamicCast<ns3::energy::GenericBatteryModel>
                    (batteryHelper.Install(node,ns3::energy::CSB_GP1272_LEADACID));
        }else if (nodeInfo.nodeDevice == "Aerial WiFi" ) {
          battery  = DynamicCast<ns3::energy::GenericBatteryModel>
                    (batteryHelper.Install(node,ns3::energy::PANASONIC_CGR18650DA_LION ));

        } 


        else if (nodeInfo.nodeDevice == "Handheld Radio") {
           
        battery = DynamicCast<ns3::energy::GenericBatteryModel>
                    (batteryHelper.Install(node,ns3::energy::PANASONIC_HHR650D_NIMH  ));

        } 
        else if (nodeInfo.nodeDevice == "Hybrid msgs reciever") {
               battery = DynamicCast<ns3::energy::GenericBatteryModel>
                    (batteryHelper.Install(node,ns3::energy::PANASONIC_CGR18650DA_LION ));

        } else if (nodeInfo.nodeDevice == "PLB") {
             
               battery = DynamicCast<ns3::energy::GenericBatteryModel>
                    (batteryHelper.Install(node,ns3::energy::PANASONIC_N700AAC_NICD ));



        } else {
            NS_LOG_WARN("Unknown node type. Using default battery configuration.");
            battery->SetAttribute("MaxCapacity", DoubleValue(1.0)); // 1 Ah default
            battery->SetAttribute("NominalVoltage", DoubleValue(3.6)); // 3.6 V
            battery->SetAttribute("CutoffVoltage", DoubleValue(3.0)); // 3.0 V
            battery->SetAttribute("InternalResistance", DoubleValue(0.10)); // 0.15 Ohm

        }


        NS_LOG_UNCOND("Battery installed for node " << nodeInfo.nodeId 
                 << " energy=" << battery->GetInitialEnergy());

        return battery;
    }
}

void ResidualEnergyCallback(uint32_t nodeId, double oldValue, double newValue)
{
    double now = Simulator::Now().GetSeconds();

    // ---------- STATIC CONTROL STATE ----------
    static std::map<uint32_t, double> lastLogTime;
    //static std::vector<std::string> energyBuffer;
    const double ENERGY_LOG_PERIOD = 1.0;     // seconds
    const size_t FLUSH_THRESHOLD = 1000;      // lines



    EnergyMetrics &metrics = energyDataset[nodeId]; 

    // ---------- INITIAL ENERGY ----------
    if (metrics.initialEnergy <= 0.0)
    {
        metrics.nodeId = nodeId;
        metrics.initialEnergy = newValue;
        metrics.residualEnergy = newValue;
        metrics.totalEnergyUsed = 0.0;

        NS_LOG_UNCOND("✅ Initial energy set for Node " << nodeId
                         << " = " << newValue << " J");

        
        return;
    }

    // ---------- UPDATE METRICS ----------
    double delta = std::max(0.0, metrics.residualEnergy - newValue);
    metrics.residualEnergy = newValue;
    metrics.totalEnergyUsed = metrics.initialEnergy - newValue;

    // ---------- THROTTLE LOGGING ----------
    if (now - lastLogTime[nodeId] < ENERGY_LOG_PERIOD)
        return;

    lastLogTime[nodeId] = now;

    // ---------- BUILD CSV LINE (NO I/O) ----------
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << seed << ","
        << runNumber << ","
        << routingsize << ","
        << now << ","
        << nodeId << ","
        << oldValue << ","
        << newValue << ","
        << delta << ","
       << "\n";

    energyBuffer.push_back(oss.str());

    // ---------- FLUSH PERIODICALLY ----------
    if (energyBuffer.size() >= FLUSH_THRESHOLD)
    {
        std::ofstream stepLog(
            routingsize + "," +
            std::to_string(runNumber) +
            "StepEnergylogfinalv3.csv",
            std::ios::app);

        if (stepLog.tellp() == 0)
        {
            stepLog << "Seed,RunNumber,Routing,Time(s),NodeID,"
                       "OldEnergyJ,NewEnergyJ,DeltaJ\n";
        }

        for (const auto &line : energyBuffer)
            stepLog << line;

        energyBuffer.clear();
        stepLog.close();
    }
}
static std::map<uint32_t, Ptr<WifiRadioEnergyModel>> wifiEnergyModels;

void AttachWifiModel(Ptr<Node> node, Ptr<ns3::energy::EnergySource> source, const std::string& deviceType) {
    Ptr<WifiRadioEnergyModel> wifiEnergy = CreateObject<WifiRadioEnergyModel>();
   
    wifiEnergy->SetEnergySource(source);
    source->AppendDeviceEnergyModel(wifiEnergy);

    wifiEnergyModels[node->GetId()] = wifiEnergy;




    if (deviceType == "Onboard WiFi" || deviceType == "Fixed High-Gain" || deviceType == "High-Gain Relay") {
        // Lead Acid: Peak load current is ~5C, large infrastructure nodes
        wifiEnergy->SetTxCurrentA(1.2);    // Higher transmission current for infrastructure nodes
        wifiEnergy->SetRxCurrentA(0.6);    // Higher reception demand for constant communication
        wifiEnergy->SetIdleCurrentA(0.15); // Higher idle current due to constant uptime
        wifiEnergy->SetSleepCurrentA(0.003); // Minimal sleep current
    } else if (deviceType == "Aerial WiFi") {
        // Li-ion: Peak load current >30C, optimized for lightweight and high power
        wifiEnergy->SetTxCurrentA(0.9);  // High transmission demand for drones
        wifiEnergy->SetRxCurrentA(0.4);  // Moderate reception demand
        wifiEnergy->SetIdleCurrentA(0.08);  // Slightly higher idle consumption for flight systems
        wifiEnergy->SetSleepCurrentA(0.002);  // Low sleep current
    } else if (deviceType == "Handheld Radio") {
        // NiMH: Peak load current ~1C, reliable but lower capacity
         wifiEnergy->SetTxCurrentA(0.6);  // Medium transmission demand
        wifiEnergy->SetRxCurrentA(0.25); // Moderate reception demand
        wifiEnergy->SetIdleCurrentA(0.06); // Slightly higher idle consumption
        wifiEnergy->SetSleepCurrentA(0.0015); // Minimal sleep current
    } else if (deviceType == "Hybrid msgs reciever") {
        // Li-ion (Mobile phones): High energy density, optimized for compact devices
        wifiEnergy->SetTxCurrentA(0.7);  // Moderate transmission demand
        wifiEnergy->SetRxCurrentA(0.5);  // Higher reception demand for real-time communication
        wifiEnergy->SetIdleCurrentA(0.06); // Standard idle consumption
        wifiEnergy->SetSleepCurrentA(0.002); // Low sleep current
    } else if (deviceType == "PLB") {
        // NiCd: Reliable under harsh conditions, lower energy density
        wifiEnergy->SetTxCurrentA(0.5);  // Lower transmission demand
        wifiEnergy->SetRxCurrentA(0.2);  // Moderate reception demand
        wifiEnergy->SetIdleCurrentA(0.03); // Low idle consumption
        wifiEnergy->SetSleepCurrentA(0.0015); // Minimal sleep current
    } else {
        // Default values for unknown devices
        NS_LOG_WARN("Unknown device type. Using default energy model configuration.");
        wifiEnergy->SetTxCurrentA(0.0);        // Passive → never transmits
        wifiEnergy->SetRxCurrentA(0.0);        // Passive → never receives
        wifiEnergy->SetIdleCurrentA(0.002);    // 2 mA — minimal idle draw
        wifiEnergy->SetSleepCurrentA(0.001);   // 1 mA — deeper sleep

    }
      
            // Attach WifiRadioEnergyModel to ALL WifiPhy devices on this node
                    bool phyAttached = false;

            // Get the built-in WiFi PHY listener from the energy model
            auto listener = wifiEnergy->GetPhyListener();

            for (uint32_t d = 0; d < node->GetNDevices(); ++d)
            {
                Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(node->GetDevice(d));
                if (!wifiDev)
                {
                    continue;
                }

                Ptr<WifiPhy> phy = wifiDev->GetPhy();
                if (!phy)
                {
                    continue;
                }

                // Attach the energy model (optional, but OK for logging)
                phy->SetWifiRadioEnergyModel(wifiEnergy);

                // ✅ CRITICAL: attach the WifiRadioEnergyModel PHY listener
                phy->RegisterListener(listener);

                // your debug traces (optional)
              //  phy->TraceConnectWithoutContext("PhyTxBegin", MakeCallback(&DebugPhyTx));
              //  phy->TraceConnectWithoutContext("State", MakeCallback(&DebugPhyState));

                NS_LOG_UNCOND("ENERGY MODEL + LISTENER ATTACHED: node="
                            << node->GetId()
                            << " wifiDeviceIndex=" << d);

                phyAttached = true;
                break; // only need one WifiNetDevice
            }

            if (!phyAttached)
            {
                NS_LOG_ERROR("No WifiNetDevice found on node " << node->GetId()
                            << ". Energy model NOT attached!");
            }


 
    
}



// Initialize energy models for nodes
void InitializeEnergyModels(NodeContainer nodes) {
   
    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        Ptr<Node> node = nodes.Get(i);
        uint32_t nodeId = node->GetId();

        auto it = nodeDataset.find(nodeId);
        if (it == nodeDataset.end()) {
            NS_LOG_ERROR("NodeInfo not found for Node " << nodeId);
            continue;
        }
        NodeInfo &info = it->second;

        Ptr<ns3::energy::EnergySource> battery = CreateBatteryForDevice(info);
        if (!battery) {
            NS_LOG_ERROR("Failed to create battery for Node " << nodeId);
            continue;
        }

        AttachWifiModel(node, battery, info.nodeDevice);

        battery->TraceConnectWithoutContext("RemainingEnergy", MakeBoundCallback(&ResidualEnergyCallback, nodeId));
        battery->TraceConnectWithoutContext("EnergyDepletion", MakeBoundCallback(&EnergyDepletionCallback, nodeId));
        //LogCallbackConnection(nodeId);

        //node->AggregateObject(battery);
        nodeEnergySources[nodeId] = battery;

    }

}
void FinalizeEnergyLogs(const std::map<uint32_t, Ptr<ns3::energy::EnergySource>>& nodeEnergySources) {
    std::ofstream aggregateLog(routingsize+","+std::to_string(runNumber)+"AggregateEnergyfinalv3.csv", std::ios_base::app);


    // Write CSV header
    if (aggregateLog.tellp() == 0) {
        aggregateLog << "Seed,Runtime, routing,Node ID,Initial Energy (J),Total Energy Used (J),Remaining Energy (J),"
                     << "Initial Battery (%),Final Battery (%),"
                     << "\n";
    }

    // Write energy 
    for (const auto& entry : nodeEnergySources) {
        uint32_t nodeId = entry.first;

        // Validate metrics
        if (energyDataset.find(nodeId) == energyDataset.end()) {
            NS_LOG_WARN("FinalizeEnergyLogs: Missing energy metrics for Node " << nodeId);
            continue;
        }

        const EnergyMetrics& metrics = energyDataset.at(nodeId);
        double initialEnergy = metrics.initialEnergy;
        double residualEnergy = metrics.residualEnergy;
        double totalUsed = metrics.totalEnergyUsed;

        double finalPercent = (initialEnergy > 0.0)
                              ? (residualEnergy / initialEnergy) * 100.0
                              : 0.0;

        aggregateLog << std::fixed << std::setprecision(4)
                        <<seed << "," 
                        <<runNumber<< "," 
                        <<routingsize<< "," 
                     << nodeId << "," << initialEnergy << ","
                     << totalUsed << "," << residualEnergy << ","
                     << "100.0," << finalPercent << ","
                      << "\n";
    }

    aggregateLog.close();

    NS_LOG_INFO("✅ Final energy log complete ");
}

//-----------------------end energy setup-------------------------------



struct EnergyDebugStats
{
    size_t nodeEnergySourcesSize;
    size_t energyDatasetSize;
    size_t wifiEnergyModelsSize;   // if you have it
    size_t energyBufferSize;
    size_t energyBufferCapacity;
};



EnergyDebugStats GetEnergyDebugStats()
{
    EnergyDebugStats s{};
    s.nodeEnergySourcesSize = nodeEnergySources.size();
    s.energyDatasetSize     = energyDataset.size();
    s.wifiEnergyModelsSize  = wifiEnergyModels.size(); // if exists in your file
    s.energyBufferSize      = energyBuffer.size();
    s.energyBufferCapacity  = energyBuffer.capacity();
    return s;
}
