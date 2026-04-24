
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
//          else if (nodeInfo.nodeDevice == "Handheld Radio") {

//     battery = DynamicCast<ns3::energy::GenericBatteryModel>(
//                 batteryHelper.Install(node, ns3::energy::PANASONIC_HHR650D_NIMH));

//     // Force battery to start almost empty for debugging
//     double qMax = 7.0;  // Ah for HHR650D preset
//     double startRemaining = 0.1 * qMax;  // 10%
//     double drainedCapacity = qMax - startRemaining;

//     battery->SetDrainedCapacity(drainedCapacity);

//      NS_LOG_UNCOND("DEBUG: Handheld Radio battery forced to start at 10% capacity: "
//                   << "drained=" << drainedCapacity << " Ah");

//                   NS_LOG_UNCOND("RemainingEnergy after drain = " 
//               << battery->GetRemainingEnergy() 
//               << " J  (Node " << nodeInfo.nodeId << ")");
   

// }

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

    // ---------- WEATHER SNAPSHOT ----------
    double rain     = weather->GetWeatherCondition("RainRate");
    double fog      = weather->GetWeatherCondition("FogDensity");
    double snow     = weather->GetWeatherCondition("SnowRate");
    double wind     = weather->GetWeatherCondition("WindSpeed");
    double humidity = weather->GetWeatherCondition("Humidity");

    EnergyMetrics &metrics = energyDataset[nodeId]; // lazy insert

    // ---------- INITIAL ENERGY ----------
    if (metrics.initialEnergy <= 0.0)
    {
        metrics.nodeId = nodeId;
        metrics.initialEnergy = newValue;
        metrics.residualEnergy = newValue;
        metrics.totalEnergyUsed = 0.0;

        NS_LOG_UNCOND("✅ Initial energy set for Node " << nodeId
                         << " = " << newValue << " J");

        // std::ofstream initLog("EnergyInitializationlogfinalv3.csv", std::ios::app);
        // if (initLog.tellp() == 0)
        //     initLog << "Node ID,Initial Energy (J)\n";

        // initLog << nodeId << "," << std::fixed << std::setprecision(2)
        //         << newValue << "\n";
        // initLog.close();
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
        << rain << ","
        << fog << ","
        << snow << ","
        << wind << ","
        << humidity
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
                       "OldEnergyJ,NewEnergyJ,DeltaJ,"
                       "Rain,Fog,Snow,Wind,Humidity\n";
        }

        for (const auto &line : energyBuffer)
            stepLog << line;

        energyBuffer.clear();
        stepLog.close();
    }
}

// void ResidualEnergyCallback(uint32_t nodeId, double oldValue, double newValue)
// {

//     const double now = Simulator::Now().GetSeconds();

//     // ---------- STATIC CONTROL STATE ----------
//     static std::map<uint32_t, double> lastLogTime;
//     const double ENERGY_LOG_PERIOD = 1.0;   // seconds
//     const size_t FLUSH_THRESHOLD = 1000;    // lines

//     // ---------- ALWAYS UPDATE METRICS ----------
//     EnergyMetrics &metrics = energyDataset[nodeId];

//     // ---------- INITIAL ENERGY ----------
//     if (metrics.initialEnergy <= 0.0)
//     {
//         metrics.nodeId = nodeId;
//         metrics.initialEnergy = newValue;
//         metrics.residualEnergy = newValue;
//         metrics.totalEnergyUsed = 0.0;

//         NS_LOG_UNCOND("✅ Initial energy set for Node " << nodeId
//                         << " = " << newValue << " J");

//         std::ofstream initLog("EnergyInitializationlogfinalv3.csv", std::ios::app);
//         if (initLog.tellp() == 0)
//             initLog << "Node ID,Initial Energy (J)\n";

//         initLog << nodeId << "," << std::fixed << std::setprecision(2)
//                 << newValue << "\n";
//         initLog.close();
//         return;
//     }

//     // ---------- UPDATE METRICS ----------
//     const double delta = std::max(0.0, metrics.residualEnergy - newValue);
//     metrics.residualEnergy = newValue;
//     metrics.totalEnergyUsed = metrics.initialEnergy - newValue;

//     // ---------- THROTTLE EARLY ----------
//     auto itLast = lastLogTime.find(nodeId);
//     if (itLast != lastLogTime.end())
//     {
//         if (now - itLast->second < ENERGY_LOG_PERIOD)
//             return;
//         itLast->second = now;
//     }
//     else
//     {
//         lastLogTime.emplace(nodeId, now);
//     }

//     // ---------- WEATHER SNAPSHOT (ONLY WHEN LOGGING) ----------
//     const double rain     = weather->GetWeatherCondition("RainRate");
//     const double fog      = weather->GetWeatherCondition("FogDensity");
//     const double snow     = weather->GetWeatherCondition("SnowRate");
//     const double wind     = weather->GetWeatherCondition("WindSpeed");
//     const double humidity = weather->GetWeatherCondition("Humidity");

//     // ---------- BUILD CSV LINE (STACK BUFFER, NO HEAP) ----------
//     char line[512];
//     const int len = std::snprintf(
//         line, sizeof(line),
//         "%.0f,%u,%s,%.4f,%u,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
//         static_cast<double>(seed),
//         runNumber,
//         routingsize.c_str(),
//         now,
//         nodeId,
//         oldValue,
//         newValue,
//         delta,
//         rain,
//         fog,
//         snow,
//         wind,
//         humidity
//     );

//     if (len <= 0 || static_cast<size_t>(len) >= sizeof(line))
//     {
//         // line truncated or formatting error; skip safely
//         return;
//     }

//     energyBuffer.emplace_back(line, len);

//     // ---------- FLUSH PERIODICALLY ----------
//     if (energyBuffer.size() >= FLUSH_THRESHOLD)
//     {
//         const std::string filename =
//             routingsize + "," + std::to_string(runNumber) + "StepEnergylogfinalv3.csv";

//         static std::map<std::string, std::ofstream> openFiles;
//         static std::set<std::string> headerWritten;

//         std::ofstream &stepLog = openFiles[filename];
//         if (!stepLog.is_open())
//         {
//             stepLog.open(filename, std::ios::app);
//         }

//         if (headerWritten.insert(filename).second)
//         {
//             stepLog.seekp(0, std::ios::end);
//             if (stepLog.tellp() == 0)
//             {
//                 stepLog << "Seed,RunNumber,Routing,Time(s),NodeID,"
//                            "OldEnergyJ,NewEnergyJ,DeltaJ,"
//                            "Rain,Fog,Snow,Wind,Humidity\n";
//             }
//         }

//         for (const auto &s : energyBuffer)
//             stepLog << s;

//         energyBuffer.clear();
//         stepLog.flush();
//     }
// }

// void ResidualEnergyCallback(uint32_t nodeId, double oldValue, double newValue)
// {
//     using namespace ns3;

//     const double now = Simulator::Now().GetSeconds();

//     // ---------- STATIC CONTROL STATE ----------
//     static std::map<uint32_t, double> lastLogTime;
//     const double ENERGY_LOG_PERIOD = 1.0;   // seconds
//     const size_t FLUSH_THRESHOLD = 1000;    // lines

//     // ---------- ALWAYS UPDATE METRICS (CHEAP) ----------
//     EnergyMetrics &metrics = energyDataset[nodeId]; // one entry per node

//     // ---------- INITIAL ENERGY ----------
//     if (metrics.initialEnergy <= 0.0)
//     {
//         metrics.nodeId = nodeId;
//         metrics.initialEnergy = newValue;
//         metrics.residualEnergy = newValue;
//         metrics.totalEnergyUsed = 0.0;

//         NS_LOG_UNCOND("✅ Initial energy set for Node " << nodeId
//                         << " = " << newValue << " J");

//         std::ofstream initLog("EnergyInitializationlogfinalv3.csv", std::ios::app);
//         if (initLog.tellp() == 0)
//         {
//             initLog << "Node ID,Initial Energy (J)\n";
//         }
//         initLog << nodeId << "," << std::fixed << std::setprecision(2) << newValue << "\n";
//         initLog.close();
//         return;
//     }

//     // Update metrics every callback (no heavy work)
//     const double delta = std::max(0.0, metrics.residualEnergy - newValue);
//     metrics.residualEnergy = newValue;
//     metrics.totalEnergyUsed = metrics.initialEnergy - newValue;

//     // ---------- THROTTLE EARLY (BIGGEST RAM/CPU WIN) ----------
//     auto itLast = lastLogTime.find(nodeId);
//     if (itLast != lastLogTime.end())
//     {
//         if (now - itLast->second < ENERGY_LOG_PERIOD)
//         {
//             return; // stop here: no weather snapshot, no string build
//         }
//         itLast->second = now;
//     }
//     else
//     {
//         lastLogTime.emplace(nodeId, now);
//     }

//     // ---------- WEATHER SNAPSHOT (ONLY WHEN WE WILL LOG) ----------
//     // NOTE: this is now moved AFTER throttling to avoid heavy calls
//     double rain     = weather->GetWeatherCondition("RainRate");
//     double fog      = weather->GetWeatherCondition("FogDensity");
//     double snow     = weather->GetWeatherCondition("SnowRate");
//     double wind     = weather->GetWeatherCondition("WindSpeed");
//     double humidity = weather->GetWeatherCondition("Humidity");

//     // ---------- BUILD CSV LINE (NO I/O) ----------
//     std::ostringstream oss;
//     oss << std::fixed << std::setprecision(4)
//         << seed << ","
//         << runNumber << ","
//         << routingsize << ","
//         << now << ","
//         << nodeId << ","
//         << oldValue << ","
//         << newValue << ","
//         << delta << ","
//         << rain << ","
//         << fog << ","
//         << snow << ","
//         << wind << ","
//         << humidity
//         << "\n";

//     energyBuffer.push_back(oss.str());

//     // ---------- FLUSH PERIODICALLY ----------
//     if (energyBuffer.size() >= FLUSH_THRESHOLD)
//     {
//         // Build the filename exactly as you currently do
//         const std::string filename =
//             routingsize + "," + std::to_string(runNumber) + "StepEnergylogfinalv3.csv";

//         // Keep file handles open (reduces heap churn and syscalls)
//         static std::map<std::string, std::ofstream> openFiles;
//         static std::set<std::string> headerWritten;

//         std::ofstream &stepLog = openFiles[filename];
//         if (!stepLog.is_open())
//         {
//             stepLog.open(filename, std::ios::app);
//         }

//         // Write header once per file
//         if (headerWritten.find(filename) == headerWritten.end())
//         {
//             stepLog.seekp(0, std::ios::end);
//             if (stepLog.tellp() == 0)
//             {
//                 stepLog << "Seed,RunNumber,Routing,Time(s),NodeID,"
//                            "OldEnergyJ,NewEnergyJ,DeltaJ,"
//                            "Rain,Fog,Snow,Wind,Humidity\n";
//             }
//             headerWritten.insert(filename);
//         }

//         for (const auto &line : energyBuffer)
//         {
//             stepLog << line;
//         }
//         energyBuffer.clear();

//         // Optional: flush to reduce data loss risk (still cheaper than open/close)
//         stepLog.flush();
//     }
// }

// void ResidualEnergyCallback(uint32_t nodeId, double oldValue, double newValue)
// {
//     double now = Simulator::Now().GetSeconds();

//     // ---------- STATIC CONTROL STATE ----------
//     static std::map<uint32_t, double> lastLogTime;
//     const double ENERGY_LOG_PERIOD = 1.0;     // seconds
//     const size_t FLUSH_THRESHOLD = 1000;      // lines

//     // ---------- WEATHER SNAPSHOT ----------
//     double rain     = weather->GetWeatherCondition("RainRate");
//     double fog      = weather->GetWeatherCondition("FogDensity");
//     double snow     = weather->GetWeatherCondition("SnowRate");
//     double wind     = weather->GetWeatherCondition("WindSpeed");
//     double humidity = weather->GetWeatherCondition("Humidity");

//     EnergyMetrics &metrics = energyDataset[nodeId]; // lazy insert

//     // ---------- INITIAL ENERGY ----------
//     if (metrics.initialEnergy <= 0.0)
//     {
//         metrics.nodeId = nodeId;
//         metrics.initialEnergy = newValue;
//         metrics.residualEnergy = newValue;
//         metrics.totalEnergyUsed = 0.0;

//         NS_LOG_UNCOND("✅ Initial energy set for Node " << nodeId
//                          << " = " << newValue << " J");

//         std::ofstream initLog("EnergyInitializationlogfinalv3.csv", std::ios::app);
//         if (initLog.tellp() == 0)
//             initLog << "Node ID,Initial Energy (J)\n";

//         initLog << nodeId << "," << std::fixed << std::setprecision(2)
//                 << newValue << "\n";
//         initLog.close();
//         return;
//     }

//     // ---------- UPDATE METRICS ----------
//     double delta = std::max(0.0, metrics.residualEnergy - newValue);
//     metrics.residualEnergy = newValue;
//     metrics.totalEnergyUsed = metrics.initialEnergy - newValue;

//     // ---------- THROTTLE LOGGING ----------
//     if (now - lastLogTime[nodeId] < ENERGY_LOG_PERIOD)
//         return;

//     lastLogTime[nodeId] = now;

//     // ---------- BUILD CSV LINE (NO I/O) ----------
//     std::ostringstream oss;
//     oss << std::fixed << std::setprecision(4)
//         << seed << ","
//         << runNumber << ","
//         << routingsize << ","
//         << now << ","
//         << nodeId << ","
//         << oldValue << ","
//         << newValue << ","
//         << delta << ","
//         << rain << ","
//         << fog << ","
//         << snow << ","
//         << wind << ","
//         << humidity
//         << "\n";

//     energyBuffer.push_back(oss.str());

//     // ---------- FLUSH PERIODICALLY ----------
//     if (energyBuffer.size() >= FLUSH_THRESHOLD)
//     {
//         std::ofstream stepLog(
//             routingsize + "," +
//             std::to_string(runNumber) +
//             "StepEnergylogfinalv3.csv",
//             std::ios::app);

//         if (stepLog.tellp() == 0)
//         {
//             stepLog << "Seed,RunNumber,Routing,Time(s),NodeID,"
//                        "OldEnergyJ,NewEnergyJ,DeltaJ,"
//                        "Rain,Fog,Snow,Wind,Humidity\n";
//         }

//         for (const auto &line : energyBuffer)
//             stepLog << line;

//         energyBuffer.clear();
//         stepLog.close();
//     }
// }

static std::map<uint32_t, Ptr<WifiRadioEnergyModel>> wifiEnergyModels;

void UpdateEnergyModelsBasedOnWeather() {
   
   // NS_LOG_INFO("🔄 Updating WifiRadioEnergyModel at t=" << now << "s");

    for (const auto& pair : nodeEnergySources) {
        uint32_t nodeId = pair.first;
        Ptr<Node> node = NodeList::GetNode(nodeId);

        if (!nodeDataset.count(nodeId)) continue;
        std::string deviceType = nodeDataset[nodeId].nodeDevice;

        double rain     = weather->GetWeatherCondition("RainRate");
        double fog      = weather->GetWeatherCondition("FogDensity");
        double snow     = weather->GetWeatherCondition("SnowRate");
        double wind     = weather->GetWeatherCondition("WindSpeed");
        double humidity = weather->GetWeatherCondition("Humidity");

        double weatherFactor = 1.0
            + 0.004 * rain
            + 0.007 * fog
            + 0.005 * snow
            + 0.003 * wind
            + 0.001 * humidity;

        Ptr<WifiRadioEnergyModel> model = wifiEnergyModels[nodeId];
        if (!model) continue;

        for (uint32_t d = 0; d < node->GetNDevices(); ++d) {
            Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice>(node->GetDevice(d));
            if (dev) {
                Ptr<WifiPhy> phy = dev->GetPhy();
            Ptr<WifiRadioEnergyModel> model = wifiEnergyModels[nodeId];
            if (!model) continue;

                if (deviceType == "Onboard WiFi" || deviceType == "Fixed High-Gain" || deviceType == "High-Gain Relay") {
                    model->SetTxCurrentA(1.2 * weatherFactor);
                    model->SetRxCurrentA(0.6 * weatherFactor);
                    model->SetIdleCurrentA(0.15 * weatherFactor);
                    model->SetSleepCurrentA(0.003 * weatherFactor);
                } else if (deviceType == "Aerial WiFi") {
                    model->SetTxCurrentA(0.9 * weatherFactor);
                    model->SetRxCurrentA(0.4 * weatherFactor);
                    model->SetIdleCurrentA(0.08 * weatherFactor);
                    model->SetSleepCurrentA(0.002 * weatherFactor);
                } else if (deviceType == "Handheld Radio") {
                    model->SetTxCurrentA(0.6 * weatherFactor);
                    model->SetRxCurrentA(0.25 * weatherFactor);
                    model->SetIdleCurrentA(0.06 * weatherFactor);
                    model->SetSleepCurrentA(0.0015 * weatherFactor);
                } else if (deviceType == "Hybrid msgs reciever") {
                    model->SetTxCurrentA(0.7 * weatherFactor);
                    model->SetRxCurrentA(0.5 * weatherFactor);
                    model->SetIdleCurrentA(0.06 * weatherFactor);
                    model->SetSleepCurrentA(0.002 * weatherFactor);
                } else if (deviceType == "PLB") {
                    model->SetTxCurrentA(0.5 * weatherFactor);
                    model->SetRxCurrentA(0.2 * weatherFactor);
                    model->SetIdleCurrentA(0.03 * weatherFactor);
                    model->SetSleepCurrentA(0.0015 * weatherFactor);
                } else {
                    model->SetTxCurrentA(0.0 * weatherFactor);
                    model->SetRxCurrentA(0.0 * weatherFactor);
                    model->SetIdleCurrentA(0.002 * weatherFactor);
                    model->SetSleepCurrentA(0.001 * weatherFactor);
                }
            }
        }
    }

    // Re-schedule for next second
    Simulator::Schedule(Seconds(1.0), &UpdateEnergyModelsBasedOnWeather);
}

// void DebugPhyState(Time t, WifiPhyState oldState, WifiPhyState newState) {
//     std::cout << "NODE PHY STATE CHANGE: time=" << t.GetSeconds()
//               << " old=" << oldState
//               << " new=" << newState << std::endl;
// }

// void DebugPhyTx(Ptr<const Packet> p, double txPowerW) {
//     std::cout << "NODE PHY TX: packet uid=" << p->GetUid()
//               << " power=" << txPowerW
//               << " at time=" << Simulator::Now().GetSeconds()
//               << std::endl;
// }


void AttachWifiModel(Ptr<Node> node, Ptr<ns3::energy::EnergySource> source, const std::string& deviceType) {
    Ptr<WifiRadioEnergyModel> wifiEnergy = CreateObject<WifiRadioEnergyModel>();
   
    wifiEnergy->SetEnergySource(source);
    source->AppendDeviceEnergyModel(wifiEnergy);

    wifiEnergyModels[node->GetId()] = wifiEnergy;


    double rain     = weather->GetWeatherCondition("RainRate");
    double fog      = weather->GetWeatherCondition("FogDensity");
    double snow     = weather->GetWeatherCondition("SnowRate");
    double wind     = weather->GetWeatherCondition("WindSpeed");
    double humidity = weather->GetWeatherCondition("Humidity");

    double weatherFactor = 1.0
   + 0.004 * rain      // mm/h
    + 0.007 * fog       // g/m³
    + 0.005 * snow      // mm/h
    + 0.003 * wind      // m/s
    + 0.001 * humidity; // g/m³


    if (deviceType == "Onboard WiFi" || deviceType == "Fixed High-Gain" || deviceType == "High-Gain Relay") {
        // Lead Acid: Peak load current is ~5C, large infrastructure nodes
        wifiEnergy->SetTxCurrentA(1.2*weatherFactor);    // Higher transmission current for infrastructure nodes
        wifiEnergy->SetRxCurrentA(0.6*weatherFactor);    // Higher reception demand for constant communication
        wifiEnergy->SetIdleCurrentA(0.15*weatherFactor); // Higher idle current due to constant uptime
        wifiEnergy->SetSleepCurrentA(0.003*weatherFactor); // Minimal sleep current
    } else if (deviceType == "Aerial WiFi") {
        // Li-ion: Peak load current >30C, optimized for lightweight and high power
        wifiEnergy->SetTxCurrentA(0.9*weatherFactor);  // High transmission demand for drones
        wifiEnergy->SetRxCurrentA(0.4*weatherFactor);  // Moderate reception demand
        wifiEnergy->SetIdleCurrentA(2.08*weatherFactor);  // Slightly higher idle consumption for flight systems
        wifiEnergy->SetSleepCurrentA(2.002*weatherFactor);  // Low sleep current
    } else if (deviceType == "Handheld Radio") {
        // NiMH: Peak load current ~1C, reliable but lower capacity
         wifiEnergy->SetTxCurrentA(0.6*weatherFactor);  // Medium transmission demand
        wifiEnergy->SetRxCurrentA(0.25*weatherFactor); // Moderate reception demand
        wifiEnergy->SetIdleCurrentA(0.06*weatherFactor); // Slightly higher idle consumption
        wifiEnergy->SetSleepCurrentA(0.0015*weatherFactor); // Minimal sleep current
    } else if (deviceType == "Hybrid msgs reciever") {
        // Li-ion (Mobile phones): High energy density, optimized for compact devices
        wifiEnergy->SetTxCurrentA(0.7*weatherFactor);  // Moderate transmission demand
        wifiEnergy->SetRxCurrentA(0.5*weatherFactor);  // Higher reception demand for real-time communication
        wifiEnergy->SetIdleCurrentA(0.06*weatherFactor); // Standard idle consumption
        wifiEnergy->SetSleepCurrentA(0.002*weatherFactor); // Low sleep current
    } else if (deviceType == "PLB") {
        // NiCd: Reliable under harsh conditions, lower energy density
        wifiEnergy->SetTxCurrentA(0.5*weatherFactor);  // Lower transmission demand
        wifiEnergy->SetRxCurrentA(0.2*weatherFactor);  // Moderate reception demand
        wifiEnergy->SetIdleCurrentA(0.03*weatherFactor); // Low idle consumption
        wifiEnergy->SetSleepCurrentA(0.0015*weatherFactor); // Minimal sleep current
    } else {
        // Default values for unknown devices
        NS_LOG_WARN("Unknown device type. Using default energy model configuration.");
        wifiEnergy->SetTxCurrentA(0.0*weatherFactor);        // Passive → never transmits
        wifiEnergy->SetRxCurrentA(0.0*weatherFactor);        // Passive → never receives
        wifiEnergy->SetIdleCurrentA(0.002*weatherFactor);    // 2 mA — minimal idle draw
        wifiEnergy->SetSleepCurrentA(0.001*weatherFactor);   // 1 mA — deeper sleep

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

//std::ofstream verificationLog("Verificationlogfinal3.txt");

// void LogCallbackConnection(uint32_t nodeId) {
//     verificationLog << nodeId << ",Callback Connected\n";
//     verificationLog.flush();
// }

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
//with weather averages
void FinalizeEnergyLogs(const std::map<uint32_t, Ptr<ns3::energy::EnergySource>>& nodeEnergySources) {
    std::ofstream aggregateLog(routingsize+","+std::to_string(runNumber)+"AggregateEnergyfinalv3.csv", std::ios_base::app);

     // Fetch average values from WeatherManager
    std::map<std::string, double> avgWeather = weather->GetAverageWeatherConditions();

    double avgRain     = avgWeather.count("RainRate")   ? avgWeather["RainRate"]   : 0.0;
    double avgFog      = avgWeather.count("FogDensity") ? avgWeather["FogDensity"] : 0.0;
    double avgSnow     = avgWeather.count("SnowRate")   ? avgWeather["SnowRate"]   : 0.0;
    double avgWind     = avgWeather.count("WindSpeed")  ? avgWeather["WindSpeed"]  : 0.0;
    double avgHumidity = avgWeather.count("Humidity")   ? avgWeather["Humidity"]   : 0.0;

    // Write CSV header
    if (aggregateLog.tellp() == 0) {
        aggregateLog << "Seed,Runtime, routing,Node ID,Initial Energy (J),Total Energy Used (J),Remaining Energy (J),"
                     << "Initial Battery (%),Final Battery (%),"
                     << "AvgRain,AvgFog,AvgSnow,AvgWind,AvgHumidity\n";
    }

    // Write energy + weather per node
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
                     << avgRain << "," << avgFog << "," << avgSnow << "," << avgWind << "," << avgHumidity << "\n";
    }

    aggregateLog.close();

    NS_LOG_INFO("✅ Final energy log complete with weather summary per node.");
}

//-----------------------end energy setup-------------------------------

