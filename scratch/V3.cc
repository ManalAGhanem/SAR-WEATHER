
#include "../helperfiles/SAR_operations_logs_weather/includes.cc"
#include "../helperfiles/routing_telemetry4.h"
#include "../helperfiles/routing_telemetry4.cc"

#include "../helperfiles/auto_logdir.h"


NS_LOG_COMPONENT_DEFINE("SearchRescueSimulation_scenariofinalv3");

#include "../helperfiles/SAR_operations_logs_weather/variables.cc"
#include "../helperfiles/SAR_operations_logs_weather/helperfunctions.cc"

#include "../helperfiles/SAR_operations_logs_weather/disc_con_dataset.cc"


#include "../helperfiles/SAR_operations_logs_weather/comm_dataset.cc"

#include "../helperfiles/SAR_operations_logs_weather/nodeinfo_dataset.cc"
#include "../helperfiles/SAR_operations_logs_weather/energy.cc"
//----------------------------START DISCOVERY , CONVERGENCE AND RESCUE----------------------------



#include "../helperfiles/SAR_operations_logs_weather/discovery.cc"
#include "../helperfiles/SAR_operations_logs_weather/discoverysarquerysignal.cc"
#include "../helperfiles/SAR_operations_logs_weather/discoveryplb.cc"

#include "../helperfiles/SAR_operations_logs_weather/convergence.cc"

   
#include "../helperfiles/SAR_operations_logs_weather/rescue.cc"

//----------------------------END DISCOVERY , CONVERGENCE AND RESCUE----------------------------

// //----------------------------------(START)DATA COMMUNICATION : GPS, ENVIRONMENT, VOICE, VIDEO---------------------------------------------------------------

#include "../helperfiles/SAR_operations_logs_weather/env.cc"
#include "../helperfiles/SAR_operations_logs_weather/video.cc"
#include "../helperfiles/SAR_operations_logs_weather/radio.cc"
#include "../helperfiles/SAR_operations_logs_weather/gps.cc"
#include "../helperfiles/SAR_operations_logs_weather/base_drone_com.cc"



//----------------------------------(END)DATA COMMUNICATION : GPS, ENVIRONMENT, VOICE, VIDEO---------------------------------------------------------------



#include "../helperfiles/SAR_operations_logs_weather/buildinghelper.cc"

//#include "../helperfiles/SAR_operations_logs_weather/post_rescue.cc"


#include "../helperfiles/SAR_operations_logs_weather/snr&weather_dataset.cc"

//#include "../helperfiles/SAR_operations_logs_weather/mem_stats.cc"

using namespace ns3;
using namespace sartelemetry;


int main(int argc, char *argv[]) {
    LogComponentEnable("SearchRescueSimulation_scenariofinalv3", LOG_LEVEL_INFO);
    GlobalValue::Bind("SchedulerType",
                  StringValue("ns3::HeapScheduler"));

    // Config::SetDefault("ns3::DefaultSimulatorImpl::SchedulerType",
    //                StringValue("ns3::HeapScheduler"));


 //std::cout << "CWD = " << std::filesystem::current_path() << std::endl;

    double snapshotSeconds = 2.0;

     // Parse command-line arguments
        CommandLine cmd(__FILE__);
        cmd.AddValue("routing",  "Routing protocol (OLSR|AODV|DSDV)", routing);
        cmd.AddValue("scenario", "Scenario identifier for results namespacing", scenarioId);
        cmd.AddValue("snapshot", "Route-table snapshot period in seconds (0=off)", snapshotSeconds);
        cmd.AddValue("RngRun", "Run number for RNG", runNumber);  // ✅ Make sure to capture this
        cmd.Parse(argc, argv);

        // Set run and retrieve values **after parsing**
        RngSeedManager::SetRun(runNumber);
    // seed = RngSeedManager::GetSeed();  // ✅ This now matches the simulation's seed
        routingsize = routing + "_" + scenarioId;
        std::replace(routingsize.begin(), routingsize.end(), '/', '_');
        std::replace(routingsize.begin(), routingsize.end(), '\\', '_');

        // Normalize routing name to uppercase (OPTIONAL)
        for (auto& c : routing)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        // Now you can build file prefix etc.
        const std::string prefix = BuildPrefix(scenarioId, routing, seed, runNumber);
        

  //  LogComponentEnable("DsdvRoutingProtocol", LOG_LEVEL_ALL);
    LogComponentEnable("DsdvRoutingTable", LOG_LEVEL_ALL);


    pendingAssignmentCsv.open(routingsize+","+std::to_string(runNumber)+"pending_assignment_log.csv");
    pendingAssignmentCsv << "time,civilian_id,state\n";

// std::string debugFile = routingsize+","+std::to_string(runNumber)+"dataset_memory_debug.csv";

//     // std::string debugFile =
//     // resultsDir + "/dataset_memory_debug.csv";

//     g_debugDatasetLog.open(debugFile, std::ios::out);
//     g_debugDatasetLog
//     << "Time,"
//     << "VmRSS_KB,VmSize_KB,HeapUsed_Bytes,HeapFree_Bytes,"

//     << "PhyRxSize,PhyRxCapacity,PhyRxNodePairStats,"

//     << "PhyBufSize,PhyBufCapacity,PhyBufPeakSize,PhyBufPeakCapacity,PhyBufBytesEst,"
//     << "EnergyBufSize,EnergyBufCapacity,EnergyBufPeakSize,EnergyBufPeakCapacity,EnergyBufBytesEst,"

//     << "EnergyBufDatasetSize,EnergyBufDatasetCapacity,"
//     << "CommActiveSessions,CommIpToCommIdSize,DiscConvSize,"
//     << "DiscStateOuter,DiscStateInnerTotal,"
//     << "NodeEnergySourcesSize,EnergyDatasetSize,WifiEnergyModelsSize,"

//     << "SarPrevKeys,SarPrevTotalSize,SarPrevTotalCapacity,"
//     << "ConvKeys,ConvTotalSize,ConvTotalCapacity,"
//     << "LastLoggedPosSize,DiscoveryAckedSize,DiscoveryRetriesSize,"

//     << "RT_TtlCache,RT_StreamKeys,RT_PktKeys,RT_ByteKeys,"
//     << "FlowCount,FlowTxPackets,FlowRxPackets"
//   << std::endl;




       // assignmnet log
    rescueAssignmentLogFile.open("rescue_assignment_logfinalv3.csv");
    rescueAssignmentLogFile << "Timestamp,Civilian ID,Assigned SAR Node ID, Assigned SAR Node IP,Civilian X,Civilian Y,Civilian Z\n";
    // assignmnet broadcast log
    receivedAssignmentsLogFile.open("recieved_assignment_logfinalv3.csv");
    receivedAssignmentsLogFile << "Time (s),Receiver SAR Node ID,Sender Address,Civilian ID,Assigned SAR Node ID, Assigned SAR Node IP,Civilian X,Civilian Y,Civilian Z\n"; // Headers for local log
        positionLogFile.open(routingsize+"_"+std::to_string(runNumber)+"_position_logfinalv3.csv", std::ios::app);

    positionLogFile << "seed,runnumber,routing,Time(s),NodeID,X,Y,Z, velocityx, velocityy,velocityz,speed\n";
    signalLogFile.open("CiviliansignallogV45finalv3.csv");
    signalLogFile << "Timestamp,Status,Signal Type,Sender Address,Receiver Address,Packet Size,"
                      << "Sender Node,Distance (meters), civilian location(X), civilian location(Y), civilian location(Z),port, interval(seconds)" << std::endl;
                      
    movementLogFile.open("movemnetsfinalv3.csv");
                 
    movementLogFile << "Timestamp,SAR Node ID,SAR X,SAR Y,SAR Z, Civilian ID,Civilian X,Civilian Y,Civilian Z, SAR Distance to Base, Civilian Distance to Base \n";

    rescueCompletionLogFile.open("rescue_completion_logfinalv3.csv");
    rescueCompletionLogFile << "Timestamp,SAR Node ID,Civilian ID,SAR X,SAR Y,SAR Z,Civilian X,Civilian Y,Civilian Z,SAR Distance to Base,Civilian Distance to Base,Base X,Base Y,Base Z,Status\n";
      //  adjacencyLogFile << "Time,Node,Neighbor,Distance(m),RSSI,SNR,LOS,overall_loss\n";

    hopLogFile.open("hop-logfinalv3.csv");
   routingLogFile.open("routing-table-logfinalv3.csv");

   // Add headers if necessary
    systemLogFile.open("system_events_logfinalv3.csv", std::ios::out | std::ios::trunc);
    if (!systemLogFile.is_open()) {
        std::cerr << "Failed to open system log file!" << std::endl;
    } else {
        systemLogFile << "Time,Event,Details" << std::endl;
    }

    macTxRxLogFile.open("MacTxRxLogfinalv3.txt", std::ios::out);
if (!macTxRxLogFile.is_open()) {
    NS_LOG_ERROR("Unable to open file for writing MacTx/MacRx logs");
}



//attachement log

    attachmentLogFile.open("attachment_logfinalv3.csv");
    attachmentLogFile << "Timestamp,Action,SAR Node ID,SAR Node Position (X,Y,Z),Civilian ID,Civilian Position (X,Y,Z),Details\n";


// Open convergence log file
    convergenceLogFile.open("convergence_logfinalv3.csv");
    convergenceLogFile << "Timestamp,SAR IP, SAR Node ID,Civilian ID,SAR X,SAR Y,SAR Z,Civilian X,Civilian Y,Civilian Z,Base X,Base Y,Base Z,DistanceToCivilian, DistanceToBase, Status \n";

     //Open the log file for connection status
    connectionLogFile.open("connection_status_logfinalv3.csv");
    connectionLogFile << "Time,Status" << std::endl;  // CSV headers
   // Open the log file for writing
    gpsLogFile.open("gps_communication_logfinalv3.csv");
    gpsLogFile << "Time,Action,Type,NodeName,NodeAddress,OldX,OldY,OldZ,CurrentX,CurrentY,CurrentZ,Details" << std::endl;  // CSV headers
    videoLogFile.open("video_communication_logfinalv3.csv");
    videoLogFile << "Time,Action,Type,Source,Destination,Size (bytes)" << std::endl;  // CSV headers

    SetupLocalDiscoveryLogging();       // Initialize local log for each SAR node
    SetupCentralDiscoveryLogging();     // Initialize centralized log at the base station
    sarDiscoveryBroadcastLogFile.open("sar_discovery_broadcast_logfinalv3.csv");
    sarDiscoveryBroadcastLogFile << "Time,Receiving SAR Node ID,Broadcasting SAR Node ID,Discovering SAR Node ID,Civilian Node ID,Distance to civilian,Civilian X position,Civilian Y position,Civilian Z position,Status\n";

     InitializeCommunicationLogFile();
//     uint32_t runNumber = 1;  // Default run number

    

         // ------------------------------
    // **City Grid Parameters**
    // ------------------------------
    double cityHight=2000.0;
    double cityWidth=2000.0;

    
      
  BuildingContainer allBuildings;
  BuildingContainer trees;
  BuildingContainer obstacles;
  CreateBuildingsInBlockWithParams(
    allBuildings,
    /*minX*/ 0.0,       /*minY*/ 50.0, 1.0,
    /*gridWidth*/ 5,
    /*lengthX*/  30.0,   /*lengthY*/ 30.0,
    /*deltaX*/   20.0,   /*deltaY*/ 20.0,
    /*buildingHeight*/ 40.0,
    Building::Commercial,
    Building::ConcreteWithWindows,
    /*nFloors*/ 5,
    /*nRoomsX*/ 2,      /*nRoomsY*/ 2,
    /*totalBuildings*/ 10
);
CreateBuildingsInBlockWithParams(
    allBuildings,
    /*minX*/ 0.0,       /*minY*/ 200.0, 1.0,
    /*gridWidth*/ 5,
    /*lengthX*/  30.0,   /*lengthY*/ 30.0,
    /*deltaX*/   15.0,   /*deltaY*/ 15.0,
    /*buildingHeight*/ 50.0,
    Building::Office,                   
    Building::StoneBlocks,              
    /*nFloors*/ 4,
    /*nRoomsX*/ 3,      /*nRoomsY*/ 2,
    /*totalBuildings*/ 10
);

CreateBuildingsInBlockWithParams(
    allBuildings,
    /*minX*/ 450.0,       /*minY*/ 200.0, 1.0,
    /*gridWidth*/ 5,
    /*lengthX*/  30.0,   /*lengthY*/ 30.0,
    /*deltaX*/   15.0,   /*deltaY*/ 15.0,
    /*buildingHeight*/ 50.0,
    Building::Office,                   
    Building::StoneBlocks,              
    /*nFloors*/ 4,
    /*nRoomsX*/ 3,      /*nRoomsY*/ 2,
    /*totalBuildings*/ 10
);

CreateBuildingsInBlockWithParams(
    allBuildings,
    /*minX*/ 450.0,       /*minY*/ 50.0, 1.0,
    /*gridWidth*/ 5,
    /*lengthX*/  30.0,   /*lengthY*/ 30.0,
    /*deltaX*/   15.0,   /*deltaY*/ 15.0,
    /*buildingHeight*/ 50.0,
    Building::Office,                   
    Building::StoneBlocks,              
    /*nFloors*/ 4,
    /*nRoomsX*/ 3,      /*nRoomsY*/ 2,
    /*totalBuildings*/ 10
);

CreateBuildingsInBlockWithParams(
    allBuildings,
    /*minX*/ 450.0,   /*minY*/ 450.0, 1.0,
    /*gridWidth*/ 5,
    30.0, 30.0, 20.0, 20.0,
    30.0,
    Building::Residential,
    Building::Wood,
    2, 2, 2,
    10
);
CreateBuildingsInBlockWithParams(
    allBuildings,
    /*minX*/ 50.0,   /*minY*/ 450.0, 1.0,
    5,
    30.0, 30.0, 20.0, 20.0,
    25.0,
    Building::Commercial,
    Building::StoneBlocks,
    3, 3, 3,
    10
);


for (double x = 0; x < cityWidth; x += 100) {  // Place trees every 100
    for (double y = 0; y < cityHight; y += 50) {
        bool occupied = false;
        for (uint32_t i = 0; i < allBuildings.GetN(); i++) {
            Ptr<Building> building = allBuildings.Get(i);
            Box bounds = building->GetBoundaries();
            
            if (x >= bounds.xMin && x <= bounds.xMax &&
                y >= bounds.yMin && y <= bounds.yMax) {
                occupied = true;
                break;
            }
        }
        if (!occupied) {
            Ptr<Building> tree = CreateObject<Building>();

            tree->SetBoundaries(Box(x, x + 2, y, y + 2, 1, 30)); // Small tree (5m wide, 50m tall)
            tree->SetExtWallsType(Building::Wood); // Simulate a wooden tree
            trees.Add(tree);
        //    std::cout << "🌳 Tree placed at: X=" << x << ", Y=" << y << std::endl;   
             }
    }
    }


    // ------------------------------
    // 🏢 **Visualizing Buildings**
    // ------------------------------

    //auto orchestrator = CreateObject<netsimulyzer::Orchestrator>(routingsize+"urban_simulationfinalv3.json");

    // netsimulyzer::BuildingConfigurationHelper buildingConfigHelper(orchestrator);
    // buildingConfigHelper.Install(allBuildings);
    // //buildingConfigHelper.Install(obstacles);
    // buildingConfigHelper.Install(trees);
        
    // creating nodes
    NodeContainer baseNode, droneNode,helicopterNode, vehicleNode, footNodes,civilianNodesPLB,  civilianNodespassive;
     baseNode.Create(1);
    droneNode.Create(1);
    helicopterNode.Create(1);  
    vehicleNode.Create(10);
    footNodes.Create(14);

    civilianNodesPLB.Create(15);
   // civilianNodeswifi.Create(3);
    civilianNodesquery.Create(15);
    civilianNodespassive.Create(15);
    //civilianNodesSignal.Create(6);

    // civilianNodesSignal.Add(civilianNodesPLB);
    //civilianNodesSignal.Add(civilianNodeswifi);

     civilianNodes.Add(civilianNodesPLB);
      civilianNodes.Add(civilianNodesquery);
     civilianNodes.Add(civilianNodespassive);

   //NS_LOG_INFO("Nodes created");
    g_baseNode = baseNode.Get(0); // Assign the base station
    // NodeContainer allNodes;
    allNodes.Add(baseNode);
    allNodes.Add(droneNode);
    allNodes.Add(helicopterNode);
    allNodes.Add(vehicleNode);
    allNodes.Add(footNodes);
    allNodes.Add(civilianNodesPLB);
   // allNodes.Add(civilianNodeswifi);
    allNodes.Add(civilianNodesquery);
    allNodes.Add(civilianNodespassive);
    groundNodes.Add(vehicleNode);
    groundNodes.Add(footNodes);

   // Configure moving nodes in NetSimulyzer
//    auto infoLog = CreateObject<netsimulyzer::LogStream>(orchestrator);
//    auto eventLog = CreateObject<netsimulyzer::LogStream>(orchestrator);
//    netsimulyzer::NodeConfigurationHelper nodeConfigHelper(orchestrator);
//    nodeConfigHelper.Set("EnableMotionTrail", BooleanValue(true));
//    nodeConfigHelper.Set("Model", StringValue(netsimulyzer::models::SMARTPHONE));
//    nodeConfigHelper.Install(footNodes);
//    nodeConfigHelper.Set("Model", StringValue(netsimulyzer::models::CAR));
//    nodeConfigHelper.Install(vehicleNode);
//    nodeConfigHelper.Set("Model", StringValue(netsimulyzer::models::QUADCOPTER_UAV));
//    nodeConfigHelper.Install(droneNode);
//         nodeConfigHelper.Set("Model", StringValue(netsimulyzer::models::SPHERE));
//    nodeConfigHelper.Install(helicopterNode);

//    nodeConfigHelper.Set("Model", StringValue(netsimulyzer::models::SQUARE_PYRAMID));
//    nodeConfigHelper.Install(baseNode);

//    nodeConfigHelper.Set("Model", StringValue(netsimulyzer::models::LAPTOP));
//    nodeConfigHelper.Install(civilianNodes);




// Loop over all nodes in the NodeContainer and initialize their attached state to stop them from sending anything if attached
for (uint32_t i = 0; i < allNodes.GetN(); i++) {
    Ptr<Node> node = allNodes.Get(i); // Get the node at index i
    g_civilianAttachedState[node->GetId()] = false; // Default to not attached
}

//    // allNodes.Add(civilianNodes);
// for (auto& node : nodeDataset) {
//     InitializeNodeInfo(node.second);
// }

    // After creating nodes and assigning them to containers:
    AssignTypeAndRole(baseNode, "Base Station", "Coordinator", "Fixed High-Gain");
    AssignTypeAndRole(droneNode, "Drone", "Discoverer", "Aerial WiFi");
    AssignTypeAndRole(helicopterNode, "Helicopter", "Discoverer", "High-Gain Relay");
    AssignTypeAndRole(vehicleNode, "Vehicle", "Rescuer", "Onboard WiFi");
    AssignTypeAndRole(footNodes, "Foot Team", "Rescuer", "Handheld Radio");
    AssignTypeAndRole(civilianNodesPLB, "Civilian", "Civilian", "PLB");
  //  AssignTypeAndRole(civilianNodeswifi, "Civilian", "Civilian", "Mobile Wifi");
    AssignTypeAndRole(civilianNodesquery, "Civilian", "Civilian", "Hybrid msgs reciever");
    AssignTypeAndRole(civilianNodespassive, "Civilian", "Civilian", "Unknown");
  

  // Initialize the safety status tracker
   InitializeCivilianSafetyStatus(civilianNodes);

    

//    for (auto& entry : nodeDataset) {
//     UpdateNodeAttributes(entry.first);
// }
   // ScheduleNodeUpdates();  // Call function to begin periodic updates

    // Set Up Weather Manager and Attenuation Model.
   // moving this to global 
   //Ptr<WeatherManager> weather = CreateObject<WeatherManager>();
    //  FLASHFLOOD PLAN  0–150 s (build): Rain 20, Wind 6, Hum 17, Temp 23

// 150–450 s (peak baseline): Rain 50, Hum 17, Temp 23

// 450–510 s (pulse 1): Rain 65, Hum 17, Temp 23

// 510–750 s (peak baseline): Rain 50, Hum 17, Temp 23

// 750–810 s (pulse 2): Rain 65, Wind 6, Hum 17, Temp 23
weather= CreateObject<WeatherManager>();



// 810–1000 s (decay): Rain 20, Wind 6,Hum 17, Temp 23
   weather->SetWeatherCondition("RainRate", 0.0);
   weather->SetWeatherCondition("FogDensity", 0.8);
   weather->SetWeatherCondition("SnowRate", 0.0);
   weather->SetWeatherCondition("Humidity", 8.2);
   weather->SetWeatherCondition("WetSnow", 0.0); // 1.0 means treat as wet snow.
   weather->SetWeatherCondition("WindSpeed", 0.0);
   weather->SetWeatherCondition("WindDirection", 0.0);   
   weather->SetMetadata(routingsize, seed, runNumber);



  //weather->SetAveragesFilename("Weather_Averagesfinalv3small.csv");
 weather->SetHistoryFilename(routingsize+"_"+std::to_string(runNumber)+"_Weather_change_history_finalv3.csv");
   weather->SetSpeedLogFile(routingsize+"_"+std::to_string(runNumber)+"_speed_change_finalv3.csv");

 Ptr<HybridBuildingsPropagationLossModel> baseloss = CreateObject<HybridBuildingsPropagationLossModel>();
//   Ptr<FriisPropagationLossModel> baseloss = CreateObject<FriisPropagationLossModel>();

   weatherLoss = CreateObject<WeatherAttenuationModel>();
   weatherLoss->SetChild(baseloss);
   weatherLoss->SetWeatherManager(weather);
    weatherLoss->SetFrequency(2.5);
   weatherLoss->SetPolarization("horizontal");
   weatherLoss->SetTemperature(8.0);

// // 810–1000 s (decay): Rain 20, Wind 6,Hum 17, Temp 23
//      weather->SetWeatherCondition("RainRate", 0.0);
//    weather->SetWeatherCondition("FogDensity", 0.0);
//    weather->SetWeatherCondition("SnowRate", 0.0);
//    weather->SetWeatherCondition("Humidity", 0.0);
//    weather->SetWeatherCondition("WetSnow", 0.0); // 1.0 means treat as wet snow.
//    weather->SetWeatherCondition("WindSpeed", 0.0);
//    weather->SetWeatherCondition("WindDirection", 0.0);  
//    weather->SetMetadata(routingsize, seed, runNumber);



//   //weather->SetAveragesFilename("Weather_Averagesfinalv3small.csv");
//  weather->SetHistoryFilename(routingsize+"_"+"Weather_change_history_finalv3.csv");
//    weather->SetSpeedLogFile(routingsize+"_"+std::to_string(runNumber)+"_speed_change_finalv3.csv");

//  //Ptr<HybridBuildingsPropagationLossModel> baseloss = CreateObject<HybridBuildingsPropagationLossModel>();
//     Ptr<FriisPropagationLossModel> baseloss = CreateObject<FriisPropagationLossModel>();

    weatherLoss = CreateObject<WeatherAttenuationModel>();
   weatherLoss->SetChild(baseloss);
   weatherLoss->SetWeatherManager(weather);
   weatherLoss->SetFrequency(2.5);
   weatherLoss->SetPolarization("horizontal");
   weatherLoss->SetTemperature(23.0);

   Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();
   channel->SetPropagationDelayModel(CreateObject<ConstantSpeedPropagationDelayModel>());
   channel->SetPropagationLossModel(weatherLoss);


    YansWifiPhyHelper wifiPhy; 
    
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11); // Enable packet capturing for analysis
 

    wifiPhy.SetChannel(channel);
   
     
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    //wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac", "QosSupported", BooleanValue(true));


   // Install WiFi devices 
    NetDeviceContainer basedevices = wifi.Install(wifiPhy, wifiMac, baseNode);
    NetDeviceContainer droneDevices = wifi.Install(wifiPhy, wifiMac, droneNode);
    NetDeviceContainer helicopterDevices = wifi.Install(wifiPhy, wifiMac, helicopterNode);
    NetDeviceContainer vehicleDevices = wifi.Install(wifiPhy, wifiMac, vehicleNode);
    NetDeviceContainer footTeamDevices = wifi.Install(wifiPhy, wifiMac, footNodes);
    NetDeviceContainer civilianDevicesPLB = wifi.Install(wifiPhy, wifiMac, civilianNodesPLB);
    NetDeviceContainer civilianDevicesquery = wifi.Install(wifiPhy, wifiMac, civilianNodesquery);
    NetDeviceContainer civilianDevicespassive = wifi.Install(wifiPhy, wifiMac, civilianNodespassive);
for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
    Ptr<Node> n = allNodes.Get(i);
    std::cout << "NODE " << i << " devices:\n";
    for (uint32_t d = 0; d < n->GetNDevices(); ++d) {
        Ptr<NetDevice> dev = n->GetDevice(d);
        std::cout << "  dev " << d << ": " << dev->GetInstanceTypeId().GetName() 
                  << " (MAC=" << dev->GetAddress() << ")\n";
    }
}


    NetDeviceContainer allWifiDevices;
    allWifiDevices.Add(basedevices);
    allWifiDevices.Add(droneDevices);
    allWifiDevices.Add(helicopterDevices);
    allWifiDevices.Add(vehicleDevices);
    allWifiDevices.Add(footTeamDevices);
    allWifiDevices.Add(civilianDevicesPLB);
    allWifiDevices.Add(civilianDevicesquery);
    allWifiDevices.Add(civilianDevicespassive);

//Ipv4InterfaceContainer allInterfaces = ipv4.Assign(allWifiDevices);


       NS_LOG_INFO("WIFI ready...");



         //NS_LOG_INFO("WIFI ready...");


    ConfigureNodeWiFiAttributes(baseNode);
    ConfigureNodeWiFiAttributes(droneNode);
    ConfigureNodeWiFiAttributes(helicopterNode);
    ConfigureNodeWiFiAttributes(vehicleNode);
    ConfigureNodeWiFiAttributes(footNodes);
    ConfigureNodeWiFiAttributes(civilianNodesPLB);
    //ConfigureNodeWiFiAttributes(civilianNodeswifi);
    ConfigureNodeWiFiAttributes(civilianNodesquery);
    ConfigureNodeWiFiAttributes(civilianNodespassive);

           // Initialize packet counters for all nodes
      // Initialize packet counters for all nodes
        for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
            packetSentCount[i] = 0; // Initialize sent count to 0
            packetReceivedCount[i] = 0; // Initialize received count to 0
        }
    // Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",MakeCallback(&TrackPacketSent));

    // Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback(&TrackPacketReceived));



    // set up mobility models
    MobilityHelper mobility;
    //NS_LOG_INFO("Setting up mobility models...");

    // Set a consistent Position Allocator with larger bounds for all nodes
    Ptr<PositionAllocator> dronePositionAlloc = CreateObject<RandomRectanglePositionAllocator>();
    dronePositionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
    dronePositionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
    dronePositionAlloc->SetAttribute("Z", DoubleValue(100.0)); 


    // Set initial positions for all nodes using the position allocator
    mobility.SetPositionAllocator(dronePositionAlloc);


  mobility.SetMobilityModel("ns3::WeatherWaypointMobilityModel",
                          "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=20.0]"),
                          "Pause", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"),
                          "PositionAllocator", PointerValue(dronePositionAlloc));
                           mobility.Install(droneNode);
    Ptr<PositionAllocator> helicopterPositionAlloc = CreateObject<RandomRectanglePositionAllocator>();
    helicopterPositionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
    helicopterPositionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
    helicopterPositionAlloc->SetAttribute("Z", DoubleValue(150.0)); 


    // Set initial positions for all nodes using the position allocator
    mobility.SetPositionAllocator(helicopterPositionAlloc);

        // helicopter Nodes Mobility (MIXED )

       mobility.SetMobilityModel("ns3::WeatherWaypointMobilityModel",
                          "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=30.0]"),
                          "Pause", StringValue("ns3::ConstantRandomVariable[Constant=10.0]"),
                          "PositionAllocator", PointerValue(helicopterPositionAlloc));

    mobility.Install(helicopterNode);
      

   


    Ptr<PositionAllocator> groundPositionAlloc = CreateObject<RandomRectanglePositionAllocator>();
    groundPositionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
    groundPositionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
    groundPositionAlloc->SetAttribute("Z", DoubleValue(2.0)); 


    // Set initial positions for all nodes using the position allocator
    mobility.SetPositionAllocator(groundPositionAlloc);
    
     mobility.SetMobilityModel("ns3::WeatherWaypointMobilityModel",
                          "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"),
                          "Pause", StringValue("ns3::ConstantRandomVariable[Constant=8.0]"),
                          "PositionAllocator", PointerValue(groundPositionAlloc));


    // mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
    //     "PositionAllocator", PointerValue(groundPositionAlloc),
    //     "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"),  // Speed range
    //     "Pause", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=8.0]")); // Pause duration


    mobility.Install(vehicleNode);
     PlaceNodesOutsideBuildings(vehicleNode, allBuildings, 2);

    // mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
    //     "PositionAllocator", PointerValue(groundPositionAlloc),
    //     "Speed", StringValue("ns3::UniformRandomVariable[Min=2.0|Max=5.0]"),  // Speed range
    //     "Pause", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]")); // Pause duration
 mobility.SetMobilityModel("ns3::WeatherWaypointMobilityModel",
                          "Speed", StringValue("ns3::UniformRandomVariable[Min=2.0|Max=5.0]"),
                          "Pause", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                          "PositionAllocator", PointerValue(groundPositionAlloc));

    mobility.Install(footNodes);
    PlaceNodesOutsideBuildings(footNodes, allBuildings, 2);
    mobility.Install(civilianNodespassive);
    mobility.Install(civilianNodesquery);
    PlaceNodesOutsideBuildings(civilianNodesquery, allBuildings, 2);
    PlaceNodesOutsideBuildings(civilianNodespassive, allBuildings, 2);

   

    //base station
    Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator> ();
    posAlloc->Add (Vector (40.0, 400.0, 2.0));  // Sets the node at (50,50,2)
  
    mobility.SetPositionAllocator (posAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(baseNode);

    Ptr<PositionAllocator> plbPositionAlloc = CreateObject<RandomRectanglePositionAllocator>();
    plbPositionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
    plbPositionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
    plbPositionAlloc->SetAttribute("Z", DoubleValue(2.0)); 
   
   
       // Set initial positions for all nodes using the position allocator
       mobility.SetPositionAllocator(plbPositionAlloc);
       mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(civilianNodesPLB);  // Static civilian
    PlaceNodesOutsideBuildings(civilianNodesPLB, allBuildings, 2);



    // After installing mobility for baseNode and civilianNodesPLB:
for (uint32_t i = 0; i < baseNode.GetN(); i++) {
    Ptr<MobilityModel> mob = baseNode.Get(i)->GetObject<MobilityModel>();
    Vector pos = mob->GetPosition();
    // Record as both initial and final position (since the node never moves)
    initialPositions[baseNode.Get(i)->GetId()] = pos;
    finalPositions[baseNode.Get(i)->GetId()] = pos;
}
BuildingContainer buildings ; // Residential blocks

for (uint32_t i = 0; i < civilianNodesPLB.GetN(); i++) {
  uint32_t buildingIndex = i % allBuildings.GetN(); // Get a valid building index
  Ptr<Building> assignedBuilding = allBuildings.Get(buildingIndex); // Get the building
  
  // Get the building's center position
  Box boundaries = assignedBuilding->GetBoundaries();

  Vector buildingEdge(boundaries.xMin + 1.0, // Slight offset from the wall
    boundaries.yMin + 1.0,
    std::max(boundaries.zMin + 2.0, 2.0)); // Ensure it's above ground

//   Vector buildingCenter((boundaries.xMin + boundaries.xMax) / 2,
//                         (boundaries.yMin + boundaries.yMax) / 2,
//                         (boundaries.zMin + boundaries.zMax) / 2);

  // Assign the civilian position inside the building
  Ptr<MobilityModel> mobilityModel = civilianNodesPLB.Get(i)->GetObject<MobilityModel>();
 // Vector pos = mobilityModel->GetPosition();
//  mobilityModel->SetPosition(buildingCenter);
   mobilityModel->SetPosition(buildingEdge);

//   //mobilityModel->SetPosition(Vector(pos.x, pos.y, 100.0));

}


for (uint32_t i = 0; i < civilianNodesPLB.GetN(); i++) {
    Ptr<MobilityModel> mob = civilianNodesPLB.Get(i)->GetObject<MobilityModel>();
    Vector pos = mob->GetPosition();
    // Record as both initial and final position (since the node never moves)
    initialPositions[civilianNodesPLB.Get(i)->GetId()] = pos;
    finalPositions[civilianNodesPLB.Get(i)->GetId()] = pos;
}

// BuildingsHelper::Install(allNodes);
    BuildingsHelper::Install(footNodes);
    BuildingsHelper::Install(civilianNodes);
   BuildingsHelper::Install(vehicleNode);
    BuildingsHelper::Install(baseNode);

   BuildingsHelper::Install(helicopterNode);
  BuildingsHelper::Install(droneNode);


//For each node, retrieve the building info and call MakeConsistent()
  //  Attach MobilityBuildingInfo & Make Consistent for all ground nodes (foot, civilian, base)
  for (uint32_t i = 0; i < droneNode.GetN(); ++i) {
    Ptr<MobilityBuildingInfo> buildingInfo = droneNode.Get(i)->GetObject<MobilityBuildingInfo>();
    if (buildingInfo) {
        buildingInfo->SetOutdoor(); //  Exclude from indoor calculations
    }
}
for (uint32_t i = 0; i < helicopterNode.GetN(); ++i) {
    Ptr<MobilityBuildingInfo> buildingInfo = helicopterNode.Get(i)->GetObject<MobilityBuildingInfo>();
    if (buildingInfo) {
        buildingInfo->SetOutdoor(); //  Exclude from indoor calculations
    }
}

  for (uint32_t i = 0; i < groundNodes.GetN(); ++i) {
    Ptr<Node> node = groundNodes.Get(i);
    
    //  Attach MobilityBuildingInfo if missing
    if (!node->GetObject<MobilityBuildingInfo>()) {
        node->AggregateObject(CreateObject<MobilityBuildingInfo>());
    }

    Ptr<MobilityBuildingInfo> buildingInfo = node->GetObject<MobilityBuildingInfo>();
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();

    if (!buildingInfo || !mobility) {
        NS_LOG_WARN("Missing building info or mobility model for node " << node->GetId());
        continue;
    }

    //  Now make them consistent
    buildingInfo->MakeConsistent(mobility);
}

//  Repeat for civilian nodes
for (uint32_t i = 0; i < civilianNodes.GetN(); ++i) {
    Ptr<Node> node = civilianNodes.Get(i);
    
    if (!node->GetObject<MobilityBuildingInfo>()) {
        node->AggregateObject(CreateObject<MobilityBuildingInfo>());
    }

    Ptr<MobilityBuildingInfo> buildingInfo = node->GetObject<MobilityBuildingInfo>();
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();

    if (!buildingInfo || !mobility) {
        NS_LOG_WARN("Missing building info or mobility model for node " << node->GetId());
        continue;
    }

    buildingInfo->MakeConsistent(mobility);
}

//  Repeat for base node
for (uint32_t i = 0; i < baseNode.GetN(); ++i) {
    Ptr<Node> node = baseNode.Get(i);

    if (!node->GetObject<MobilityBuildingInfo>()) {
        node->AggregateObject(CreateObject<MobilityBuildingInfo>());
    }

    Ptr<MobilityBuildingInfo> buildingInfo = node->GetObject<MobilityBuildingInfo>();
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();

    if (mobility) {
        Vector pos = mobility->GetPosition();
        NS_LOG_UNCOND("🚨 After BuildingsHelper::Install, Node 0 Position: (" 
                     << pos.x << ", " << pos.y << ", " << pos.z << ")");
    }
    if (buildingInfo) {
        buildingInfo->SetOutdoor(); //  Exclude from indoor calculations
    }

    if (!buildingInfo || !mobility) {
        NS_LOG_WARN("Missing building info or mobility model for node " << node->GetId());
        continue;
    }

    buildingInfo->MakeConsistent(mobility);
}
//
 std::ofstream debugLog("mobility_debugfinalv3.txt", std::ios::app); // Open file in append mode



        //NS_LOG_INFO("Mobility models setup completed.");

    Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&PositionChangeCallback));

    // Connect the `CourseChange` trace to each node
//   for (uint32_t i = 0; i < allNodes.GetN (); ++i)
//     {
//       // The node’s MobilityModel
//       Ptr<MobilityModel> mob = allNodes.Get (i)->GetObject<MobilityModel> ();
//       if (mob)
//         {
//           // Connect the CourseChange trace to our static function
//           mob->TraceConnectWithoutContext ("CourseChange", MakeCallback (&VelocityTrace));
//         }
//     }


// First sample at time 1s
//Simulator::Schedule(Seconds(1.0), &SampleAdjacency, allBuildings);


   
// // weather EFFECTS ON SPEED

  for (uint32_t i = 0; i < footNodes.GetN(); i++) {

            Ptr<Node> node = footNodes.Get(i);
            Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
            Simulator::Schedule(Seconds(20.0), &WeatherManager::ScheduleMobilityReduction, weather, mob, "foot", 2.0);
            

        }

         for (uint32_t i = 0; i < vehicleNode.GetN(); i++) {

             Ptr<Node> node = vehicleNode.Get(i);
             Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
             Simulator::Schedule(Seconds(2.0), &WeatherManager::ScheduleMobilityReduction, weather, mob, "vehicle",2.0);
            

        }


           for (uint32_t i = 0; i < civilianNodesquery.GetN(); i++) {

             Ptr<Node> node = civilianNodesquery.Get(i);
             Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
             Simulator::Schedule(Seconds(2.0), &WeatherManager::ScheduleMobilityReduction, weather, mob, "foot",2.0);
            

        }
          for (uint32_t i = 0; i < civilianNodespassive.GetN(); i++) {

             Ptr<Node> node = civilianNodespassive.Get(i);
             Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
             Simulator::Schedule(Seconds(2.0), &WeatherManager::ScheduleMobilityReduction, weather, mob, "foot",2.0);
            

        }
         for (uint32_t i = 0; i < civilianNodesPLB.GetN(); i++) {

             Ptr<Node> node = civilianNodesPLB.Get(i);
             Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
             Simulator::Schedule(Seconds(2.0), &WeatherManager::ScheduleMobilityReduction, weather, mob, "foot",2.0);
            

        }
        for (uint32_t i = 0; i < droneNode.GetN(); i++) {

             Ptr<Node> node = droneNode.Get(i);
             Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
             Simulator::Schedule(Seconds(2.0), &WeatherManager::ScheduleMobilityReduction, weather, mob, "drone",2.0);
            

        }
         for (uint32_t i = 0; i < helicopterNode.GetN(); i++) {

             Ptr<Node> node = helicopterNode.Get(i);
             Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
             Simulator::Schedule(Seconds(2.0), &WeatherManager::ScheduleMobilityReduction, weather, mob, "drone",2.0);
            

        }


 //internet stack
    InternetStackHelper internet;

     OlsrHelper olsr; // Use OLSR as the routing protocol
    AodvHelper aodv;
    DsdvHelper dsdv;

    
  Ipv4ListRoutingHelper list;

    AsciiTraceHelper ascii;

    //wifiPhy.EnableAsciiAll(ascii.CreateFileStream("mac-layer.tr"));
if (routing == "OLSR") {
  list.Add(olsr, 100);
} else if (routing == "AODV") {
  list.Add(aodv, 100);
} else if (routing == "DSDV") {
  list.Add(dsdv, 100);
} else {
  NS_FATAL_ERROR("Unknown --routing=" << routing << " (expected OLSR|AODV|DSDV)");
}

   //list.Add(olsr, 100); 
   internet.SetRoutingHelper(list);
       internet.Install(allNodes);

     
    //files for RSSI AND SNR
g_phyLogStream = Create<OutputStreamWrapper>(routingsize+","+std::to_string(runNumber)+"SNR_log_finalv3.csv", std::ios::out);

//*g_phyLogStream->GetStream() << "Time,RxNodeId,RSSI,Noise,SNR\n";
//*g_phyLogStream->GetStream() << "Time,RxNodeId,TxNodeId,Distance,RSSI,Noise,SNR\n";
*g_phyLogStream->GetStream()
  << "Time,RxNodeId,RxDeviceId,TxNodeId,Distance,RSSI,Noise,SNR,PacketUid,PacketSizeBytes,ChannelFreqMhz,StaId\n";




//     internet.Install(baseNode);
//     internet.Install(droneNode);
//     internet.Install(helicopterNode);
//     internet.Install(vehicleNode);
//     internet.Install(footNodes);
//     internet.Install(civilianNodesPLB);
//    // internet.Install(civilianNodeswifi);
//     internet.Install(civilianNodesquery);
//     internet.Install(civilianNodespassive);
        //NS_LOG_INFO("OLSR ready...");

        // dsrMain.Install(dsr, droneNode);
        // dsrMain.Install(dsr, helicopterNode);
        // dsrMain.Install(dsr, vehicleNode);
        // dsrMain.Install(dsr, footNodes);
        // dsrMain.Install(dsr, civilianNodesPLB);
        // dsrMain.Install(dsr, civilianNodesquery);
        // dsrMain.Install(dsr, civilianNodespassive);

    // //  Step 1: Create a Traffic Control Helper
    // TrafficControlHelper tch;
    // uint16_t handle = tch.SetRootQueueDisc("ns3::PrioQueueDisc", 
    //     "Priomap", StringValue("0 0 0 0 1 1 1 1 2 2 2 2 3 3 3 3"));
    
    // TrafficControlHelper::ClassIdList cid = tch.AddQueueDiscClasses(handle, 4, "ns3::QueueDiscClass");  // Ensure 4 bands
    
    // // Assign QueueDiscs to each band
    // tch.AddChildQueueDisc(handle, cid[0], "ns3::TbfQueueDisc", "Rate", DataRateValue(DataRate("128kbps")));
    // tch.AddChildQueueDisc(handle, cid[1], "ns3::TbfQueueDisc", "Rate", DataRateValue(DataRate("3Mbps")));
    // tch.AddChildQueueDisc(handle, cid[2], "ns3::TbfQueueDisc", "Rate", DataRateValue(DataRate("1Mbps")));
    // tch.AddChildQueueDisc(handle, cid[3], "ns3::TbfQueueDisc", "Rate", DataRateValue(DataRate("512kbps")));
    

Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/VO_EdcaTxop/Aifsn", UintegerValue(2)); 
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/VO_EdcaTxop/TxopLimit", TimeValue(MicroSeconds(1504)));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/VO_EdcaTxop/MinCw", UintegerValue(3));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/VO_EdcaTxop/MaxCw", UintegerValue(7));
// Video (AC_VI)
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/VI_EdcaTxop/Aifsn", UintegerValue(3));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/VI_EdcaTxop/TxopLimit", TimeValue(MicroSeconds(3264)));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/VI_EdcaTxop/MinCw", UintegerValue(7));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/VI_EdcaTxop/MaxCw", UintegerValue(15));

// Best Effort (AC_BE)
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_EdcaTxop/Aifsn", UintegerValue(6));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_EdcaTxop/TxopLimit", TimeValue(MicroSeconds(0)));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_EdcaTxop/MinCw", UintegerValue(15));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_EdcaTxop/MaxCw", UintegerValue(1023));

// Background (AC_BK)
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BK_EdcaTxop/Aifsn", UintegerValue(9));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BK_EdcaTxop/TxopLimit", TimeValue(MicroSeconds(0)));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BK_EdcaTxop/MinCw", UintegerValue(15));
Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BK_EdcaTxop/MaxCw", UintegerValue(1023));


//  Step 3: Install QoS Settings on Wi-Fi Devices
//  Step 3: Install QoS Settings on Wi-Fi Devices
// QueueDiscContainer qdiscs;
// qdiscs.Add(tch.Install(basedevices));
// qdiscs.Add(tch.Install(droneDevices));
// qdiscs.Add(tch.Install(helicopterDevices));
// qdiscs.Add(tch.Install(vehicleDevices));
// qdiscs.Add(tch.Install(footTeamDevices));
// qdiscs.Add(tch.Install(civilianDevicespassive));
// qdiscs.Add(tch.Install(civilianDevicesPLB));
// qdiscs.Add(tch.Install(civilianDevicesquery));

//  Step 4: Validate Traffic ControlLayer at the Node Level
// for (uint32_t i = 0; i < allNodes.GetN(); i++) {
//     Ptr<Node> node = allNodes.Get(i);
//     Ptr<TrafficControlLayer> tc = node->GetObject<TrafficControlLayer>();

//     if (!tc) {
//         NS_LOG_WARN("⚠️ No TrafficControlLayer on node " << node->GetId());
//     } else {
//         NS_LOG_INFO(" TrafficControlLayer installed on node " << node->GetId());
//     }
// }

// //  Step 5: Validate QueueDisc Attachment to Devices
// for (uint32_t i = 0; i < vehicleDevices.GetN(); i++) {
//     Ptr<NetDevice> dev = vehicleDevices.Get(i);
//     Ptr<TrafficControlLayer> tc = dev->GetObject<TrafficControlLayer>();

//     if (!tc) {
//         NS_LOG_WARN("⚠️ No TrafficControlLayer found on device " << i);
//     } else {
//         Ptr<QueueDisc> qdisc = tc->GetRootQueueDiscOnDevice(dev);
//         if (!qdisc) {
//             NS_LOG_WARN("⚠️ No QueueDisc found on device " << i);
//         } else {
//             NS_LOG_INFO(" QueueDisc correctly installed on device " << i);
//         }
//     }
// }



// Assigning IP addresses

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer allinterfaces=ipv4.Assign(allWifiDevices); 
    //  Ipv4InterfaceContainer baseInterfaces = ipv4.Assign(basedevices);  // Assign to the drone
    // Ipv4InterfaceContainer droneInterfaces = ipv4.Assign(droneDevices);  // Assign to the drone
    // Ipv4InterfaceContainer helicopterInterfaces = ipv4.Assign(helicopterDevices);  // Assign to the helicopter
    // Ipv4InterfaceContainer vehicleInterfaces = ipv4.Assign(vehicleDevices); // Assign to the vehicle
    // Ipv4InterfaceContainer footInterfaces = ipv4.Assign(footTeamDevices);   // Assign to foot teams
    // Ipv4InterfaceContainer civilianInterfacesplb = ipv4.Assign(civilianDevicesPLB); // Assign to civilians
    // Ipv4InterfaceContainer civilianInterfacesquery = ipv4.Assign(civilianDevicesquery); // Assign to civilians
    // Ipv4InterfaceContainer civilianInterfacespassive = ipv4.Assign(civilianDevicespassive); // Assign to civilians
   

    InitializeEnergyModels(allNodes);
    

//AttachLoggingToAllNodes(allNodes);
   


//LogEnergyMetrics("energyLogfinalv3.csv");

        // After assigning IP addresses to interfaces in main():
        // For each node, get its assigned IP and store in ipToNodeIdMap:

        PopulateIpToNodeIdMap(allNodes);
        // for (uint32_t i = 0; i < allNodes.GetN(); i++) {
        //     Ptr<Node> node = allNodes.Get(i);
        //     Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        //     Ipv4Address addr = ipv4->GetAddress(1,0).GetLocal(); // Assuming interface 1,0 is data interface
        //     ipToNodeIdMap[addr] = node->GetId();
        // }

        // Initialize maxReceptionDistance to 0 for all nodes:
        for (uint32_t i = 0; i < allNodes.GetN(); i++) {
            maxReceptionDistance[i] = 0.0;
        }

// Changing weather conditions:

// Schedule Dynamic Weather Changes.
//Simulator::Schedule(Seconds(3.0), &UpdateWeather, weather, weatherLoss);
 //  FLASHFLOOD PLAN  0–150 s (build): Rain 20, Wind 6, Hum 17, Temp 23

// 150–450 s (peak baseline): Rain 50, Hum 17, Temp 23

// 450–510 s (pulse 1): Rain 65, Hum 17, Temp 23

// 510–750 s (peak baseline): Rain 50, Hum 17, Temp 23

// 750–810 s (pulse 2): Rain 65, Wind 6, Hum 17, Temp 23

// // 810–1000 s (decay): Rain 20, Wind 6,Hum 17, Temp 23

// Simulator::Schedule(Seconds(50.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 50.0);
// Simulator::Schedule(Seconds(80.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 65.0);
// Simulator::Schedule(Seconds(150.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 50.0);
// Simulator::Schedule(Seconds(200.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 65.0);
// Simulator::Schedule(Seconds(250.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 20.0);

// Simulator::Schedule(Seconds(150.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 50.0);
// Simulator::Schedule(Seconds(450.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 65.0);
// Simulator::Schedule(Seconds(510.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 50.0);
// Simulator::Schedule(Seconds(750.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 65.0);
// Simulator::Schedule(Seconds(810.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 20.0);
// // Simulator::Schedule(Seconds(100.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 10.0);
 //Simulator::Schedule(Seconds(100.0), &WeatherManager::SetWeatherCondition, weather, "FogDensity", 0.5);
// Simulator::Schedule(Seconds(100.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 5.0);
// Simulator::Schedule(Seconds(150.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 0.0);
// Simulator::Schedule(Seconds(150.0), &WeatherManager::SetWeatherCondition, weather, "FogDensity", 0.0);


// Simulator::Schedule(Seconds(151.0), &WeatherManager::SetWeatherCondition, weather, "SnowRate", 50.0);
// Simulator::Schedule(Seconds(200.0), &WeatherManager::SetWeatherCondition, weather, "SnowRate", 0.0);
// Simulator::Schedule(Seconds(201.0), &WeatherManager::SetWeatherCondition, weather, "WetSnow", 1.0);
// Simulator::Schedule(Seconds(202.0), &WeatherManager::SetWeatherCondition, weather, "SnowRate", 50.0);
// Simulator::Schedule(Seconds(250.0), &WeatherManager::SetWeatherCondition, weather, "SnowRate", 0.0);
// Simulator::Schedule(Seconds(251.0), &WeatherManager::SetWeatherCondition, weather, "WetSnow", 0.0);


// GPS starting setup
    // Ipv4Address helicopterAddress = helicopterNode.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    //   // Set up UDP server on the base station (to receive GPS updates)
    // Ptr<Socket> recvSocket = Socket::CreateSocket(baseNode.Get(0), UdpSocketFactory::GetTypeId());
    // InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), gpsPort);
    // recvSocket->Bind(local);
    // recvSocket->SetRecvCallback(MakeCallback(&LogGPSData));
  // Initialize the SAR nodes container
  sarNodes.Add(vehicleNode);
  sarNodes.Add(footNodes);
  sarNodes.Add(helicopterNode);
  sarNodes.Add(droneNode);


  for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
      g_sarNodes.push_back(sarNodes.Get(i)); // Add each node to the vector
  }
  

  allSarNodes.Add(baseNode);
  allSarNodes.Add(droneNode);
  allSarNodes.Add(helicopterNode);
  allSarNodes.Add(vehicleNode);
  allSarNodes.Add(footNodes);

// uint32_t DroneCommunicationId= 7010;
// uint32_t helicopterCommunicationId= 7020;
// uint32_t vehiclesCommunicationID= 7030;
// uint32_t vehicles2CommunicationId= 7031;
// uint32_t vehicles3CommunicationId= 7032;
// uint32_t vehicles4CommunicationId= 7033;
// uint32_t vehicles5CommunicationId= 7034;
// uint32_t footTeamsCommunicationID= 7040;
// uint32_t footTeams2CommunicationId= 7041;
//  uint32_t footTeams3CommunicationId= 7042;
// uint32_t footTeams4CommunicationId= 7043;
//  uint32_t footTeams5CommunicationId= 7044;
//  uint32_t footTeams6CommunicationId= 7045;
//  uint32_t footTeams7CommunicationId= 7046;

// // Set up UDP client sockets on each node (drone, vehicle, foot teams) to send GPS updates
// Ptr<Socket> sendSocketDrone = Socket::CreateSocket(droneNode.Get(0), UdpSocketFactory::GetTypeId());
// StartCommunicationSession(DroneCommunicationId, droneNode.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), baseAddress, "droneToBase GPS Communication", Simulator::Now().GetSeconds(), "UDP",gpsPort);

// Ptr<Socket> sendSocketHelicopter = Socket::CreateSocket(helicopterNode.Get(0), UdpSocketFactory::GetTypeId());
// StartCommunicationSession(helicopterCommunicationId, helicopterAddress, baseAddress, "HelicopterToBase GPS Communication", Simulator::Now().GetSeconds(), "UDP",gpsPort);

// Ptr<Socket> sendSocketVehicles1 = Socket::CreateSocket(vehicleNode.Get(0), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketVehicles2 = Socket::CreateSocket(vehicleNode.Get(1), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketVehicles3 = Socket::CreateSocket(vehicleNode.Get(2), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketVehicles4 = Socket::CreateSocket(vehicleNode.Get(3), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketVehicles5 = Socket::CreateSocket(vehicleNode.Get(4), UdpSocketFactory::GetTypeId());
//         for (uint32_t i = 0; i < vehicleNode.GetN(); i++) {

//         uint32_t vehicleCommunicationId = vehiclesCommunicationID + i;

//     Ipv4Address vehicleAddress = vehicleNode.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
//         Ipv4Address destinationAddress = baseAddress; 
//     // Start the communication session
//     StartCommunicationSession(vehicleCommunicationId, vehicleAddress, destinationAddress, 
//     "VehicleToBase GPS Communication", Simulator::Now().GetSeconds(), "UDP",gpsPort);}

  
// Ptr<Socket> sendSocketFootTeams1 = Socket::CreateSocket(footNodes.Get(0), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketFootTeams2 = Socket::CreateSocket(footNodes.Get(1), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketFootTeams3 = Socket::CreateSocket(footNodes.Get(2), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketFootTeams4 = Socket::CreateSocket(footNodes.Get(3), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketFootTeams5 = Socket::CreateSocket(footNodes.Get(4), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketFootTeams6 = Socket::CreateSocket(footNodes.Get(5), UdpSocketFactory::GetTypeId());
// Ptr<Socket> sendSocketFootTeams7 = Socket::CreateSocket(footNodes.Get(6), UdpSocketFactory::GetTypeId());

// for (uint32_t i = 0; i < footNodes.GetN(); i++) {

//             uint32_t footCommunicationId = footTeamsCommunicationID + i;

//         Ipv4Address footAddress = footNodes.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
//         Ipv4Address destinationAddress = baseAddress; 
//         // Start the communication session
//         StartCommunicationSession(footCommunicationId, footAddress, destinationAddress, 
//         "FootToBase GPS Communication", Simulator::Now().GetSeconds(), "UDP", gpsPort);}

// Schedule periodic GPS updates from each node to the base station using rescheduler
// Define thresholds and fallback intervals
  double positionChangeThreshold = 10.0;  // Meters
  double fallbackInterval = 100.0;       // Seconds

  SetupGpsUpdates(sarNodes,
      baseNode.Get(0),   // if baseNode is also a container
      gpsPort,              // gpsPort
      positionChangeThreshold,              // positionChangeThreshold
      fallbackInterval,             // fallbackInterval
      2.0);             // baseInterval e.g. start around 2..3s
 

      //send voice
       // Communication Session IDs
   //   uint32_t helicopterToBaseCommId = 5001;  // Unique Communication ID for helicopter-to-base
    //  ipToCommIdMap[helicopterNode.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal()] = helicopterToBaseCommId;

   //   uint32_t baseToHelicopterCommId = 5002; // Unique Communication ID for base-to-helicopter
      //ipToCommIdMap[baseNode.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal()] = baseToHelicopterCommId;


      // Create send and receive sockets for the helicopter
      // Ptr<Socket> sendVoiceSocketHelicopter = Socket::CreateSocket(helicopterNode.Get(0), UdpSocketFactory::GetTypeId());
      // Ptr<Socket> recvVoiceSocketHelicopter = Socket::CreateSocket(helicopterNode.Get(0), UdpSocketFactory::GetTypeId());

      // // Bind the receiving socket to the designated port and set the callback function for logging
      // recvVoiceSocketHelicopter->Bind(InetSocketAddress(Ipv4Address::GetAny(), voicePort));
      // recvVoiceSocketHelicopter->SetRecvCallback(MakeCallback(&LogVoiceData));

      // // Retrieve IP addresses
      // Ipv4Address helicopterAddress = helicopterNode.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
      // Ipv4Address baseAddress = baseNode.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

      // Create send and receive sockets for the base station
      // Ptr<Socket> sendVoiceSocketBase = Socket::CreateSocket(baseNode.Get(0), UdpSocketFactory::GetTypeId());
      // Ptr<Socket> recvVoiceSocketBase = Socket::CreateSocket(baseNode.Get(0), UdpSocketFactory::GetTypeId());

      // // Bind the receiving socket on the base station to the designated port
      // recvVoiceSocketBase->Bind(InetSocketAddress(Ipv4Address::GetAny(), voicePort));
      // recvVoiceSocketBase->SetRecvCallback(MakeCallback(&LogVoiceData));

// Start communication sessions
//  StartCommunicationSession(helicopterToBaseCommId, helicopterAddress, baseAddress, "HelicopterToBase Voice Communication", Simulator::Now().GetSeconds(), "UDP",voicePort);
//StartCommunicationSession(baseToHelicopterCommId, baseAddress, helicopterAddress, "BaseToHelicopter Voice Communication", Simulator::Now().GetSeconds(), "UDP",voicePort);

// Schedule periodic voice data transmission from the helicopter to the base station
// Simulator::Schedule(Seconds(1.0), &SendVoiceData, sendVoiceSocketHelicopter, baseAddress, voicePort, "HelicopterToBase", 2.0);

// Schedule periodic voice data transmission from the base station to the helicopter
//Simulator::Schedule(Seconds(1.5), &SendVoiceData, sendVoiceSocketBase, helicopterAddress, voicePort, "BaseToHelicopter", 2.0);

  // other voice communications between nodes
  // Build civilianNodes first (as you already do)
civilianNodes.Add(civilianNodesPLB);
civilianNodes.Add(civilianNodesquery);
civilianNodes.Add(civilianNodespassive);

// ----------------------------------------------------
// ✅ Start DiscoverCivilianNodes ONLY if any civilian is still active
// (i.e., NOT safe and NOT attached)
// ----------------------------------------------------
bool anyRemaining = false;

for (uint32_t i = 0; i < civilianNodes.GetN(); ++i)
{
    Ptr<Node> civ = civilianNodes.Get(i);
    if (!civ) continue;

    uint32_t cid = civ->GetId();

    bool isSafe = (civilianSafetyStatus.count(cid) && civilianSafetyStatus[cid]);
    bool isAttached = (g_civilianAttachedState.count(cid) && g_civilianAttachedState[cid]);

    if (!isSafe && !isAttached)
    {
        anyRemaining = true;
        break;
    }
}

if (anyRemaining){
  SetupVoiceCommunication(vehicleNode,helicopterNode, footNodes, baseNode.Get(0));
          //NS_LOG_INFO("voice sent");


  //enviornment data
  double temperature = 25.0;
  double humidity = 60.0;
  double windSpeed = 5.0;
  //double interval = 10.0;
  SetupEnvironmentalCommunication(droneNode, helicopterNode, g_baseNode, envPort, temperature, humidity, windSpeed);
  // Video Feed Communication Setup
  SetupVideoCommunication(vehicleNode, droneNode, g_baseNode, videoPort);


  // Base station sends control messages to the drone
   SetupBaseToDroneControl( droneNode,  g_baseNode,  controlMsgPort);
}
  //NS_LOG_INFO("Base station to drone control setup complete.");




                // Create the AnimationInterface pointer
     //   AnimationInterface *anim = new AnimationInterface("enhanced_sar_animation.xml");
    // ALL DISCOVERY , COVEREGENCE, RESCUE SETUP


    //set reciever to recieve sar to sar discovery broadcast
for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
    Ptr<Socket> recvSocket = Socket::CreateSocket(sarNodes.Get(i), UdpSocketFactory::GetTypeId());
    if (recvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), sarDiscoveryPort)) == -1) {
        NS_LOG_ERROR("Failed to bind SAR Node " << sarNodes.Get(i)->GetId() << " to discoveryPort " << sarDiscoveryPort);
    } else {
        //NS_LOG_INFO("ssssSAR Node " << sarNodes.Get(i)->GetId() << " successfully bound to discoveryPort " << sarDiscoveryPort);
    }
    recvSocket->SetRecvCallback(MakeCallback(&ReceiveBroadcast));
}
        // civilian discovery
    
        // Add civilian nodes to a single container
 //   civilianNodes.Add(civilianNodeswifi);
    // civilianNodes.Add(civilianNodesPLB);
    // civilianNodes.Add(civilianNodesquery);
    // civilianNodes.Add(civilianNodespassive);
         // Setup communication for PLB, WiFi, and Hybrid nodes

        // Setup PLB sending for  assigned nodes
        SetupSocketCommunication(civilianNodesPLB, PLBPort, ns3::Ipv4Address("255.255.255.255"), "PLB", 1.0);
        
        // Setup WiFi sending for randomly assigned nodes
      // SetupSocketCommunication(civilianNodesSignal, WificivilianPort, ns3::Ipv4Address("255.255.255.255"), "WiFi", 10.0);
        // setup Civilian answering query
        SetupHybridModeForCivilians(civilianNodesquery, SARcivilianqueryport);

        // Setup SAR nodes as receivers
        SetupReceiversForSARNodes(sarNodes, PLBPort, "PLB");        // For PLB
      // SetupReceiversForSARNodes(groundNodes, WificivilianPort, "WiFi"); // For WiFi
        SetupReceiversForSARNodes(sarNodes, QueryResponsePort, "QueryResponse"); // For QueryResponse


    // Setup SAR nodes to broadcast queries
    // Setup SAR nodes to broadcast queries
       // Setup SAR nodes to broadcast queries
     uint32_t SARCommunicationIdBase = 2000; // Base communication ID for SAR nodes

for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
    Ptr<Node> n = sarNodes.Get(i);
    Ptr<Socket> s = Socket::CreateSocket(n, UdpSocketFactory::GetTypeId());
    s->Bind(InetSocketAddress(Ipv4Address::GetAny(), discoveryAckPort));
    s->SetRecvCallback(MakeCallback(&ProcessDiscoveryAck));   // <-- THIS IS WHAT “I MEAN”
}


       for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
    Ptr<Node> sarNode = sarNodes.Get(i);

    // Create a UDP socket for broadcasting
    Ptr<Socket> querySocket = Socket::CreateSocket(sarNode, TypeId::LookupByName("ns3::UdpSocketFactory"));
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), SARcivilianqueryport);

    // Bind the socket to the specified port
    if (querySocket->Bind(local) == -1) {
        std::cerr << "Failed to bind query socket for SAR Node " << sarNode->GetId()
                  << " on port " << SARcivilianqueryport << std::endl;
        continue; // Skip this node if binding fails
    } else {
        std::cout << "Queryss socket bound successfully for SAR Node " << sarNode->GetId()
                  << " on port " << SARcivilianqueryport << std::endl;
    }

    // Allow broadcast
    querySocket->SetAllowBroadcast(true);

    // Assign a unique communication ID for each SAR node
    uint32_t sarCommunicationId = SARCommunicationIdBase + i;
    Ipv4Address sarAddress = sarNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

 // Start the communication session
    StartCommunicationSession(
        sarCommunicationId,               // Communication ID
        sarAddress,                       // Source (SAR node) IP
        Ipv4Address("255.255.255.255"),   // Destination: Broadcast
        "SARQueryBroadcast Discovery",              // Communication Type
        Simulator::Now().GetSeconds(),    // Start Time
        "UDP",                             // Protocol
        SARcivilianqueryport
    );


    Simulator::Schedule(Seconds(1.0 + i * 0.1), // Add small stagger for each SAR node
                        &BroadcastQuery,
                        querySocket, 
                        ns3::Ipv4Address("255.255.255.255"), // Broadcast IP
                        SARcivilianqueryport,               // Port
                        2.0,                                // Interval (2 seconds)
                        sarCommunicationId                  // Communication ID
    );
}


// }

        // SAR-to-SAR attachement broadcast socket setup ( recieve)
        for (uint32_t i = 0; i < allSarNodes.GetN(); ++i) {
            Ptr<Socket> recvSocket = Socket::CreateSocket(allSarNodes.Get(i), UdpSocketFactory::GetTypeId());
                //recvSocket->TraceConnectWithoutContext("MacRx", MakeCallback(&TrackPacketReceived));

            if (recvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), broadcastPort)) == -1) {
                NS_LOG_ERROR("Failed to bind SAR Node " << allSarNodes.Get(i)->GetId() << " to broadcastPort " << broadcastPort);
            } else {
                //NS_LOG_INFO("SAR Node " << allSarNodes.Get(i)->GetId() << " successfully bound to broadcastPort " << broadcastPort);
            }
            recvSocket->SetRecvCallback(MakeCallback(&ReceiveBroadcastAttachment));
        }

    //       for (uint32_t i = 0; i < sarNodes.GetN(); i++) {
    //     // Create a sending socket for each SAR node
    //     Ptr<Socket> sarSocket = Socket::CreateSocket(sarNodes.Get(i), UdpSocketFactory::GetTypeId());
    //     if (!sarSocket) {
    //         NS_FATAL_ERROR("Failed to create socket for SAR node " << i);
    //     }

    //     sarSocket->Bind(); // Bind the SAR node socket to a random available port

    //     // Schedule convergence for each SAR node, passing the baseRecvSocket
    //     Simulator::Schedule(Seconds(i + 1), &ConvergeOnCivilian, sarNodes.Get(i), civilianNodes.Get(0), sarSocket, convergencePort);
    // }

        // Base station: Convergence update reception
         Ptr<Socket> baseRecvSocket = Socket::CreateSocket(baseNode.Get(0), UdpSocketFactory::GetTypeId());
         baseRecvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), convergencePort));
         baseRecvSocket->SetRecvCallback(MakeCallback(&LogConvergenceUpdate));

         // Schedule SAR node actions
        // Simulator::Schedule(Seconds(1.0), &ConvergeOnCivilian, sarNodes.Get(0), civilianNodes.Get(0), baseRecvSocket, convergencePort);



        // Base station: Discovery message reception
        Ptr<Socket> baseStationSocket = Socket::CreateSocket(baseNode.Get(0), UdpSocketFactory::GetTypeId());
                   // baseStationSocket->"MacRx"("MacRx", MakeCallback(&TrackPacketReceived));

        if (baseStationSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), discoverybasePort)) == -1) {
            NS_LOG_ERROR("Failed to bind Base Station to discoveryPort " << discoverybasePort);
        } else {
            //NS_LOG_INFO("Base Station successfully bound to discoveryPort " << discoverybasePort);

           
        
        baseStationSocket->SetRecvCallback(MakeCallback(&ProcessDiscoveryPacketWrapper));
}
        // Base station: Rescue assignment broadcast
        baseBroadcastSocket = Socket::CreateSocket(baseNode.Get(0), UdpSocketFactory::GetTypeId());

        if (baseBroadcastSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), assignmentPort)) == -1) {
            NS_LOG_ERROR("Failed to bind Base Station to assignmentPort " << assignmentPort);
        } else {
            //NS_LOG_INFO("Base Station successfully bound to assignmentPort " << assignmentPort);
        }

        // SAR nodes: Receive rescue assignments
        for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
            Ptr<Socket> recvSocket = Socket::CreateSocket(sarNodes.Get(i), UdpSocketFactory::GetTypeId());
            //recvSocket->"MacRx"("MacRx", MakeCallback(&TrackPacketReceived));

            if (recvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), assignmentPort)) == -1) {
                NS_LOG_ERROR("Failed to bind SAR Node " << sarNodes.Get(i)->GetId() << " to assignmentPort " << assignmentPort);
            } else {
                //NS_LOG_INFO("SAR Node " << sarNodes.Get(i)->GetId() << " successfully bound to assignmentPort " << assignmentPort);
            }
            recvSocket->SetRecvCallback(MakeCallback(&ReceiveAssignment));
        }
            
            //rescuer movements updates recieved  by base
        Ptr<Socket> baseRescueSocket = Socket::CreateSocket(baseNode.Get(0), UdpSocketFactory::GetTypeId());
       // baseRescueSocket->"MacRx"("MacRx", MakeCallback(&TrackPacketReceived));

        if (baseRescueSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), rescuemovmentport)) == -1) {
            NS_LOG_ERROR("Failed to bind Base Station to rescuemovmentport");
        } else {
            //NS_LOG_INFO("Base Station successfully bound to rescuemovmentport");
        }
        baseRescueSocket->SetRecvCallback(MakeCallback(&LogResucerMovmentUpdate));

        //sarnodes recieveing saftey broadcast from base
        for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
    Ptr<Socket> recvSocket = Socket::CreateSocket(sarNodes.Get(i), UdpSocketFactory::GetTypeId());
    //recvSocket->"MacRx"("MacRx", MakeCallback(&TrackPacketReceived));


    // Bind the socket to listen to CivilianSafePort
    if (recvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), CivilianSafePort)) == -1) {
        NS_LOG_ERROR("Failed to bind SAR Node " << sarNodes.Get(i)->GetId() << " to CivilianSafePort");
    } else {
        //NS_LOG_INFO("SAR Node " << sarNodes.Get(i)->GetId() << " successfully bound to CivilianSafePort");
    }

    // Set the callback function to handle received broadcast messages
    recvSocket->SetRecvCallback(MakeCallback(&HandleSafetyBroadcast));
}





if (anyRemaining)
{
    Simulator::Schedule(Seconds(3.0), &DiscoverCivilianNodes,
                        sarNodes,
                        civilianNodes,
                        baseNode.Get(0),   // or baseStation
                        discoveryPort,
                        droneNode,
                        vehicleNode,
                        footNodes,
                        helicopterNode,
                        weather,
                        allBuildings,
                        trees);
}


// Simulator::Schedule(
//         Seconds(3.0),
//         &DiscoverCivilianNodes,
//         sarNodes,        // your SAR node container
//         civilianNodes,   // all civilians
//         baseNode.Get(0), // base station
//         discoveryPort,
//         droneNode,
//         vehicleNode,
//         footNodes,
//         helicopterNode,
//         weather,
//         allBuildings,
//         trees
//     );

    
            // Enable PCAP tracing for all nodes
        //wifiPhy.EnablePcapAll("base-scenario-trace-finalv3-dataset");

     

        // Install FlowMonitor to track metrics
        Ptr<FlowMonitor> flowMonitor;
        FlowMonitorHelper flowHelper;
        flowMonitor = flowHelper.InstallAll();
    
    


    // to check connectivity dynamically add nodes to allnodes container
    // Define WiFi range for connectivity checks
    //double wifiRange = 100.0;

//  allNodes contains all types of nodes
// for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
//     for (uint32_t j = i + 1; j < allNodes.GetN(); ++j) {
//         Ptr<Node> node1 = allNodes.Get(i);
//         Ptr<Node> node2 = allNodes.Get(j);

//         Ipv4Address node1Ip = node1->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
//         Ipv4Address node2Ip = node2->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

//         // Dynamically assign names based on node type
//         std::string node1Name = GetNodeName(node1, baseNode, droneNode, helicopterNode, vehicleNode, footNodes, civilianNodes);
//         std::string node2Name = GetNodeName(node2, baseNode, droneNode, helicopterNode, vehicleNode, footNodes, civilianNodes);

//         // Schedule connectivity check between node1 and node2
//        // Simulator::Schedule(Seconds(1.0), &CheckConnectivity, node1, node2, wifiRange, node1Name, node2Name, node1Ip, node2Ip);
//     }
// }

       
    // Define and open flow-level CSV file
    // Open CSV files
       std::ofstream flowMonitorCsvFile(routingsize+","+std::to_string(runNumber)+"overall-flow-monitor-resultsfinalv3.csv", std::ios::app);


        flowMonitorCsvFile << "Seed,Run,Routing,Flow ID,Start Time (s),End Time (s),Source IP,Destination IP,Source Node ID,Destination Node ID,Source Port, Destination Port, Tx Packets,Tx Bytes,"
                   << "Rx Packets,Rx Bytes,Throughput (Mbps),Throughput (Kbps),Average Delay (s),Average Delay (ms),"
                   << "Average Jitter (s),Lost Packets,Packet Loss Ratio (%),Packet Delivery Ratio (%),Data Packet Delivery Ratio (%),"
                   << "Mean Hop Count,Packets Forwarded,Tx Rate (Kbps),Rx Rate (Kbps)\n";



 std::ofstream netPreformanceDataset(routingsize+","+std::to_string(runNumber)+"NetwrokDatasetfinalv3.csv", std::ios::app);

    netPreformanceDataset << "Seed,Run,Routing,Flow ID,Start Time (s),End Time (s),Source IP,Destination IP,Source Node ID,Destination Node ID,Source Port,Destination Port,Protocol, Tx Packets,Tx Bytes,"
                   << "Rx Packets,Rx Bytes,Throughput (Mbps),Throughput (Kbps),Average Delay (s),Average Delay (ms),"
                   << "Average Jitter (s),Lost Packets,Packet Loss Ratio (%),Packet Delivery Ratio (%),Data Packet Delivery Ratio (%),"
                   << "Mean Hop Count,Packets Forwarded,Tx Rate (Kbps),Rx Rate (Kbps)\n";

   // Open the detailed packet log CSV file
      packetLogFile.open(routingsize+","+std::to_string(runNumber)+"_detailed-packet-logfinalv3.csv",  std::ios::app);

    packetLogFile << "Seed, RunNumber, Routing,Timestamp,CommunicationID, PacketID, Port, SourceIP,DestinationIP,PacketType,Delay(ms),direction,packetsize"<< std::endl;

//voice log
   // voiceLogFile << "Timestamp,Action,Type,Source,Destination,Size (bytes), Priority" << std::endl;
   
// Enable packet metadata and route tracking for detailed packet analysis
// anim->EnablePacketMetadata(true);
// anim->EnableIpv4RouteTracking("routing.xml", Seconds(0), Seconds(10), Seconds(1));

   // AttachForwardingTrace(allNodes);
    //  Schedule the weather computation at the end of the simulation
  // Get OLSR routing protocol instance
//   for (uint32_t i = 0; i < allNodes.GetN(); ++i)
//   {
//     Ptr<Ipv4> ipv4 = allNodes.Get(i)->GetObject<Ipv4>();
//     Ptr<Ipv4RoutingProtocol> routing = ipv4->GetRoutingProtocol();
//     // Monitor each node for link changes
//     Simulator::Schedule(Seconds(1.0), &MonitorLinkStability, routing);
//   }
// Log stability periodically
//Simulator::Schedule(Seconds(10.0), &LogLinkStability);

   //Simulator::Stop(Seconds(simulationTime)); // Run the simulation for xx seconds
            //NS_LOG_INFO("STARTING SIM...");
       // Optionally schedule periodic checks (if needed)
    //ScheduleSafetyCheck();
  

 
      // Connect your functions to the relevant events
    //Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/Mac/Tx", MakeCallback(&TrackPacketSent));
    //Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&TrackPacketReceived));
    // Simulator::Schedule(Seconds(simulationTime - 0.1), &FinalizeAllCommunicationSessions);

    scenarioStartTime = Simulator::Now().GetSeconds();
   // Simulator::Schedule(Seconds(0.0), &EnsureNodesAboveGround);
    //Simulator::Schedule(Seconds(1.0), &CheckAllCiviliansSafe);

      Simulator::Schedule(Seconds(1.0)+ MilliSeconds(17), &CheckAllCiviliansSafe, 
    droneNode, vehicleNode, footNodes, helicopterNode,weather);

     Simulator::Schedule(Seconds(1.0)+ MilliSeconds(33), &LogWeatherMetricsForAllNodes,routingsize+","+std::to_string(runNumber)+"Weather_finalv1.csv", weatherLoss, std::ref(nodeDataset), std::ref(allNodes));
         Simulator::Schedule(Seconds(1.0) + MilliSeconds(49), &LogWeatherMetricsForAllNodeswithSNR,routingsize+","+std::to_string(runNumber)+"WeatherSNR_finalv1.csv", weatherLoss, std::ref(nodeDataset), std::ref(allNodes));

    for (uint32_t i = 0; i < NodeList::GetNNodes(); i++) {
        Ptr<Node> node = NodeList::GetNode(i);
        Ptr<Ipv4RoutingProtocol> proto = node->GetObject<Ipv4>()->GetRoutingProtocol();
        Ptr<dsdv::RoutingProtocol> dsdvProto = DynamicCast<dsdv::RoutingProtocol>(proto);
        
        if (dsdvProto) {
            std::cout << "Routing table for node " << i << ":\n";
            Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(&std::cout);
dsdvProto->PrintRoutingTable(routingStream);

        }
    }




//     std::cout << "Total number of nodes: " 
//           << ns3::NodeList::GetNNodes() << std::endl;
//           std::cout << "allNodes.GetN(): " << allNodes.GetN() << std::endl;

//           std::cout << "baseNode.GetN()        = " << baseNode.GetN() << std::endl;
// std::cout << "droneNode.GetN()       = " << droneNode.GetN() << std::endl;
// std::cout << "helicopterNode.GetN()  = " << helicopterNode.GetN() << std::endl;
// std::cout << "vehicleNode.GetN()     = " << vehicleNode.GetN() << std::endl;
// std::cout << "footNodes.GetN()       = " << footNodes.GetN() << std::endl;
// std::cout << "civilianNodesPLB.GetN()= " << civilianNodesPLB.GetN() << std::endl;
// std::cout << "civilianNodesquery.GetN() = " << civilianNodesquery.GetN() << std::endl;
// std::cout << "civilianNodespassive.GetN()= " << civilianNodespassive.GetN() << std::endl;
// // and if you still have "signal" or "wifi" containers, check them too
// for (uint32_t i = 0; i < allNodes.GetN(); i++)
// {
//   Ptr<Node> node = allNodes.Get(i);
//   std::cout << "Node ID = " << node->GetId();
//   Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
//   if (!ipv4)
//   {
//     std::cout << " has no Ipv4!" << std::endl;
//     continue;
//   }
//   std::cout << ", interfaces = " << ipv4->GetNInterfaces() << std::endl;
//   for (uint32_t ifaceIdx = 0; ifaceIdx < ipv4->GetNInterfaces(); ifaceIdx++)
//   {
//     Ipv4Address ip = ipv4->GetAddress(ifaceIdx, 0).GetLocal();
//     std::cout << "   interface " << ifaceIdx 
//               << " => " << ip << std::endl;
//   }
// }

//SNR
// for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
//     Ptr<Node> node = allNodes.Get(i);
//     for (uint32_t d = 0; d < node->GetNDevices(); ++d) {
//         Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(node->GetDevice(d));
//         if (!wifiDev) continue;
//         Ptr<WifiPhy> phy = wifiDev->GetPhy();

//         Config::Connect(
//           "/NodeList/" + std::to_string(i) + "/DeviceList/" + std::to_string(d) + "/Phy/MonitorSnifferRx",
//           MakeBoundCallback(&PhyRxTraceSAR, stream, i));
//     }
// }

//snr

// for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
//     Ptr<Node> node = allNodes.Get(i);
//     for (uint32_t d = 0; d < node->GetNDevices(); ++d) {
//         Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(node->GetDevice(d));
//         if (!wifiDev) continue;
//         Ptr<WifiPhy> phy = wifiDev->GetPhy();

//         // Construct the config path and lambda INSIDE the loop, capturing i and d:
//         std::string path = "/NodeList/" + std::to_string(i) + "/DeviceList/" + std::to_string(d) + "/Phy/MonitorSnifferRx";
//     }}
       Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback(&PhyRxLogCallback));




    

//Simulator::Schedule(Seconds(1.0), &UpdateEnergyModelsBasedOnWeather);

     //start routing files
  StartForRun(prefix, routing, seed, runNumber, /*snapshotSeconds=*/2.0);
//Simulator::Schedule(Seconds(10.0), &DebugDump_All_ToFile, flowMonitor);

//end routing files
     Simulator::Stop(Seconds(300));

    Simulator::Run();
    RoutingTelemetry::Stop();         // write summaries & close files

    //NS_LOG_INFO("SIM DONE...");
     FinalizeEnergyLogs(nodeEnergySources);
     LogAverageSnrRssi();
     LogAverageSnrRssiNodes();

    FinalizeAllCommunicationSessions();
    WriteDiscoveryConvergenceDataset(routingsize+"_"+std::to_string(runNumber)+"_discovery_convergence_datasetfinalv3.csv");

for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
    Ptr<Node> node = allNodes.Get(i);
    uint32_t nodeId = node->GetId();

    // Fill initial/final positions:
    nodeDataset[nodeId].initialX = initialPositions[nodeId].x;
    nodeDataset[nodeId].initialY = initialPositions[nodeId].y;
    nodeDataset[nodeId].initialZ = initialPositions[nodeId].z;
    nodeDataset[nodeId].finalX = finalPositions[nodeId].x;
    nodeDataset[nodeId].finalY = finalPositions[nodeId].y;
    nodeDataset[nodeId].finalZ = finalPositions[nodeId].z;

    // Mobility Model
    nodeDataset[nodeId].mobilityModel = GetMobilityModelName(node);

    // Speed
   // nodeDataset[nodeId].speed = GetAverageNodeSpeed(node, simulationTime);

    for (auto & kv : g_nodeSpeedSum)
    {
      uint32_t nodeId = kv.first;
      double sumOfSpeeds = kv.second;
      uint64_t nSamples   = g_nodeSpeedCount[nodeId];

      double avgSpeed = (nSamples > 0) ? (sumOfSpeeds / nSamples) : 0.0;
    //   std::cout << "Node " << nodeId << ": Average Speed = "
    //             << avgSpeed << " m/s" << std::endl;
    nodeDataset[nodeId].speed=avgSpeed;
    }

    // Connectivity Level
   //nodeDataset[nodeId].connectivityLevel = GetConnectivityLevel(nodeId);

    // Communication Range
   // nodeDataset[nodeId].communicationRange = maxReceptionDistance[nodeId];

    // Custom Attributes
    nodeDataset[nodeId].customAttributes = GetCustomAttribute(nodeDataset[nodeId].nodeType);

    // Node Behavior
    // nodeDataset[nodeId].nodeBehavior = GetNodeBehavior(nodeId);

    // Node-specific parameters
   // double distance = maxReceptionDistance[nodeId];  // 
    //NodeInfo& nodeInfo = nodeDataset[nodeId];
    double noiseFigure = 7.0;  // dB
    double channelBandwidth = 20e6;  // Hz
    //std::string pathLossModel = "Friis +WEATHER Loss";
   // nodeInfo.pathLossModel = pathLossModel;  // Store the updated pathLossModel


    // Adjust path loss exponent dynamically based on node type
  //  double pathLossExponent = (nodeInfo.nodeRole == "Drone" || nodeInfo.nodeRole == "Helicopter") ? 2.0 : 2.0;
    //nodeDataset[nodeId].pathLossModel= pathLossModel;
    nodeDataset[nodeId].channelBandwidth=channelBandwidth;
     nodeDataset[nodeId].noiseFigure=noiseFigure;
     nodeDataset[nodeId].distance= maxReceptionDistance[nodeId];
   // nodeInfo.communicationRange = CalculateCommunicationRange(nodeInfo, pathLossExponent);
    //double distance = maxReceptionDistance[nodeId];
   // nodeInfo.connectivityLevel = GetDynamicConnectivityLevel(nodeInfo, distance, channelBandwidth, noiseFigure);
   



}


// for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
//     Ptr<Node> node = allNodes.Get(i);
//     uint32_t nodeId = node->GetId();

//     // Fill initial/final positions:
//     nodeDataset[nodeId].initialX = initialPositions[nodeId].x;
//     nodeDataset[nodeId].initialY = initialPositions[nodeId].y;
//     nodeDataset[nodeId].initialZ = initialPositions[nodeId].z;
//     nodeDataset[nodeId].finalX = finalPositions[nodeId].x;
//     nodeDataset[nodeId].finalY = finalPositions[nodeId].y;
//     nodeDataset[nodeId].finalZ = finalPositions[nodeId].z;

//     // Mobility Model
//     nodeDataset[nodeId].mobilityModel = GetMobilityModelName(node);

//     // Speed
//    // nodeDataset[nodeId].speed = GetAverageNodeSpeed(node, simulationTime);

//     for (auto & kv : g_nodeSpeedSum)
//     {
//       uint32_t nodeId = kv.first;
//       double sumOfSpeeds = kv.second;
//       uint64_t nSamples   = g_nodeSpeedCount[nodeId];

//       double avgSpeed = (nSamples > 0) ? (sumOfSpeeds / nSamples) : 0.0;
//     //   std::cout << "Node " << nodeId << ": Average Speed = "
//     //             << avgSpeed << " m/s" << std::endl;
//     nodeDataset[nodeId].speed=avgSpeed;
//     }

//     // Connectivity Level
//    //nodeDataset[nodeId].connectivityLevel = GetConnectivityLevel(nodeId);

//     // Communication Range
//    // nodeDataset[nodeId].communicationRange = maxReceptionDistance[nodeId];

//     // Custom Attributes
//     nodeDataset[nodeId].customAttributes = GetCustomAttribute(nodeDataset[nodeId].nodeType);

//     // Node Behavior
//    // nodeDataset[nodeId].nodeBehavior = GetNodeBehavior(nodeId);

//     // Node-specific parameters
//    // double distance = maxReceptionDistance[nodeId];  // 
//     NodeInfo& nodeInfo = nodeDataset[nodeId];
//     double noiseFigure = 7.0;  // dB
//     double channelBandwidth = 20e6;  // Hz
//     std::string pathLossModel = "Friis +WEATHER Loss";
//     nodeInfo.pathLossModel = pathLossModel;  // Store the updated pathLossModel


//     // Adjust path loss exponent dynamically based on node type
//     double pathLossExponent = (nodeInfo.nodeRole == "Drone" || nodeInfo.nodeRole == "Helicopter") ? 2.0 : 2.0;
//     nodeDataset[nodeId].pathLossModel= pathLossModel;
//     nodeDataset[nodeId].channelBandwidth=channelBandwidth;
//      nodeDataset[nodeId].noiseFigure=noiseFigure;
//      nodeDataset[nodeId].distance= maxReceptionDistance[nodeId];
//     nodeInfo.communicationRange = CalculateCommunicationRange(nodeInfo, pathLossExponent);
//     double distance = maxReceptionDistance[nodeId];
//     nodeInfo.connectivityLevel = GetDynamicConnectivityLevel(nodeInfo, distance, channelBandwidth, noiseFigure, pathLossModel);
   



// }





// Install FlowMonitor to track metrics
// Process FlowMonitor data after simulation ends
flowMonitor->CheckForLostPackets();
Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();


// Initialize network-wide aggregates
uint32_t totalTxPackets = 0, totalRxPackets = 0, totalLostPackets = 0;
double totalThroughput = 0.0, totalDelay = 0.0, totalJitter = 0.0;
double totalTxRate = 0.0, totalRxRate = 0.0;
uint32_t totalFlows = stats.size(); // Number of active flows

double totalRxBytes = 0.0;
//double networkThroughput = (totalRxBytes * 8.0) / (simTime * 1e6);  // Mbps



for (const auto& flow : stats) {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
    double startTime = flow.second.timeFirstTxPacket.GetSeconds();
    double endTime = flow.second.timeLastRxPacket.GetSeconds();
    uint32_t txPackets = flow.second.txPackets;
    uint32_t rxPackets = flow.second.rxPackets;
    uint32_t lostPackets = flow.second.lostPackets;

    double throughput = (endTime - startTime > 0)
        ? flow.second.rxBytes * 8.0 / (endTime - startTime) / 1024 / 1024  // Mbps
        : 0.0;

    double avgDelay = rxPackets > 0 ? flow.second.delaySum.GetSeconds() / rxPackets : 0.0;
    double avgJitter = rxPackets > 1 ? flow.second.jitterSum.GetSeconds() / (rxPackets - 1) : 0.0;
    double packetLossRatio = txPackets > 0 ? static_cast<double>(lostPackets) / txPackets : 0.0;
  //  double meanHopCount = rxPackets > 0 ? static_cast<double>(flow.second.timesForwarded) / rxPackets : 0.0;
  double meanHopCount = 0.0;
if (flow.second.rxPackets > 0)
{
    meanHopCount =
        static_cast<double>(flow.second.timesForwarded) /
        flow.second.rxPackets
        + 1.0;
}

    double txRate = (flow.second.timeLastTxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds()) > 0
        ? flow.second.txBytes * 8.0 / (flow.second.timeLastTxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds()) / 1024
        : 0.0;
    double rxRate = (endTime - flow.second.timeFirstRxPacket.GetSeconds()) > 0
        ? flow.second.rxBytes * 8.0 / (endTime - flow.second.timeFirstRxPacket.GetSeconds()) / 1024
        : 0.0;

    // Additional Metrics
    double pdr = txPackets > 0 ? (static_cast<double>(rxPackets) / txPackets) : 0.0;       // Packet Delivery Ratio
    double avgDelayMs = avgDelay * 1000.0;                                                 // End-to-End Delay in ms
    double throughputKbps = throughput * 1000.0 * 1024.0; // Convert Mbps to Kbps (1 Mbps = 1024 Kbps if using binary units, or 1000 for decimal)
    // If you prefer decimal units: throughputKbps = throughput * 1000.0;
    double dpdr = txPackets > 0 ? (static_cast<double>(lostPackets) / txPackets) * 100.0 : 0.0; // Data Packet Dropped Rate as a percentage
    
    // Node IDs (assuming you have a map or a function that returns node IDs)
    uint32_t sourceNodeId = 0;
    uint32_t destNodeId = 0;
    if (ipToNodeIdMap.find(t.sourceAddress) != ipToNodeIdMap.end()) {
        sourceNodeId = ipToNodeIdMap.at(t.sourceAddress);
    }
    if (ipToNodeIdMap.find(t.destinationAddress) != ipToNodeIdMap.end()) {
        destNodeId = ipToNodeIdMap.at(t.destinationAddress);
    }

     // Accumulate totals for network-wide stats
     totalTxPackets += txPackets;
     totalRxPackets += rxPackets;
     totalLostPackets += lostPackets;
     totalThroughput += throughput;
     totalDelay += avgDelay;
     totalJitter += avgJitter;
     totalTxRate += txRate;
     totalRxRate += rxRate;
     totalRxBytes += flow.second.rxBytes;

    // Log to console
    // std::cout << "Flow " << flow.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    // std::cout << "  Source Node ID: " << sourceNodeId << ", Destination Node ID: " << destNodeId << "\n";
    // std::cout << "  Start Time: " << startTime << " s, End Time: " << endTime << " s\n";
    // std::cout << "  Tx Packets: " << txPackets << ", Rx Packets: " << rxPackets << "\n";
    // std::cout << "  Throughput: " << throughput << " Mbps (" << throughputKbps << " Kbps), Avg Delay: " << avgDelay << " s (" << avgDelayMs << " ms), Avg Jitter: " << avgJitter << " s\n";
    // std::cout << "  Lost Packets: " << lostPackets << ", Packet Loss Ratio: " << packetLossRatio << ", PDR: " << pdr << ", DPDR: " << dpdr << "%\n";
    // std::cout << "  Mean Hop Count: " << meanHopCount << ", Tx Rate: " << txRate << " Kbps, Rx Rate: " << rxRate << " Kbps\n";
//uint32_t seed = ns3::RngSeedManager::GetSeed(); 
  //  runNumber = ns3::RngSeedManager::GetRun(); //  This changes across replicas
    
    // Write to flow-level CSV
   flowMonitorCsvFile<< seed << "," << runNumber << ","<< routingsize << ","
<< flow.first << "," << startTime << "," << endTime << ","
                       << t.sourceAddress << "," << t.destinationAddress << ","
                       << sourceNodeId << "," << destNodeId << "," // newly added columns
                       <<t.sourcePort <<","<< t.destinationPort << ","
                       << txPackets << "," << flow.second.txBytes << ","
                       << rxPackets << "," << flow.second.rxBytes << ","
                       << throughput << "," << throughputKbps << ","
                       << avgDelay << "," << avgDelayMs << ","
                       << avgJitter << ","
                       << lostPackets << "," << packetLossRatio << ","
                       << pdr << "," << dpdr << ","
                       << meanHopCount << "," << flow.second.timesForwarded << ","
                       << txRate << "," << rxRate << "\n";

      netPreformanceDataset  << seed << "," << runNumber << ","<< routingsize << ","
      << flow.first << "," << startTime << "," << endTime << ","
                       << t.sourceAddress << "," << t.destinationAddress << ","
                       << sourceNodeId << "," << destNodeId << "," // newly added columns
                       <<t.sourcePort <<","<< t.destinationPort << ","
                        <<((uint16_t) t.protocol)<<","
                       << txPackets << "," << flow.second.txBytes << ","
                       << rxPackets << "," << flow.second.rxBytes << ","
                       << throughput << "," << throughputKbps << ","
                       << avgDelay << "," << avgDelayMs << ","
                       << avgJitter << ","
                       << lostPackets << "," << packetLossRatio << ","
                       << pdr << "," << dpdr << ","
                       << meanHopCount << "," << flow.second.timesForwarded << ","
                       << txRate << "," << rxRate << "\n";


}



//  Compute overall network-wide statistics
double simTime = ns3::Simulator::Now().GetSeconds();

double avgThroughput = totalFlows > 0 ? totalThroughput / totalFlows : 0.0;
double networkThroughput = (totalRxBytes * 8.0) / (simTime * 1e6);  // Mbps

double avgDelayNetwork = totalFlows > 0 ? totalDelay / totalFlows : 0.0;
double avgJitterNetwork = totalFlows > 0 ? totalJitter / totalFlows : 0.0;
double overallPacketLossRatio = totalTxPackets > 0 ? static_cast<double>(totalLostPackets) / totalTxPackets : 0.0;
double overallPDR = totalTxPackets > 0 ? (static_cast<double>(totalRxPackets) / totalTxPackets) : 0.0;
double avgTxRate = totalFlows > 0 ? totalTxRate / totalFlows : 0.0;
double avgRxRate = totalFlows > 0 ? totalRxRate / totalFlows : 0.0;



//  Print overall network performance metrics
std::cout << "\n===== 🛰️ Overall Network Statistics 🛰️ =====\n";
std::cout << "Total Tx Packets: " << totalTxPackets << ", Total Rx Packets: " << totalRxPackets << ", Total Lost Packets: " << totalLostPackets << "\n";
std::cout << "Overall per flow Throughput: " << avgThroughput << " Mbps\n";
std::cout << "Overall Packet Delivery Ratio (PDR): " << overallPDR * 100 << " %\n";
std::cout << "Overall Packet Loss Ratio: " << overallPacketLossRatio * 100 << " %\n";
std::cout << "Average Delay: " << avgDelayNetwork << " s\n";
std::cout << "Average Jitter: " << avgJitterNetwork << " s\n";
std::cout << "Average Tx Rate: " << avgTxRate << " Kbps\n";
std::cout << "Average Rx Rate: " << avgRxRate << " Kbps\n";

std::cout << "Total Simulation Time: " << ns3::Simulator::Now().GetSeconds() << " s\n";  //  Print total simulation time

std::cout << "=====================================\n";



// uint32_t seed = ns3::RngSeedManager::GetSeed(); 
//  runNumber = ns3::RngSeedManager::GetRun(); //  This changes across replicas
std::ofstream outFile("OVERALL_network_statisticsfinalv3.csv", std::ios::app);
std::ifstream checkFile("OVERALL_network_statisticsfinalv3.csv");
bool isEmpty = checkFile.peek() == std::ifstream::traits_type::eof();

checkFile.close();

if (isEmpty) { 
    //  Write headers only if the file is empty
    outFile << "Seed,Run,Routing,TotalTxPackets,TotalRxPackets,TotalLostPackets,OverallperflowThroughput(Mbps),NetworkThroughput(Mbps),PDR(%),PacketLossRatio(%),AvgDelay(s),AvgJitter(s),AvgTxRate(Kbps),AvgRxRate(Kbps),simtime\n";
}

//  Append a new row for each replica simulation
outFile << seed << "," 
        << runNumber << ","  //  Tracks different runs of the same seed
        << routingsize << ","  //  Tracks different runs of the same seed
        << totalTxPackets << "," 
        << totalRxPackets << "," 
        << totalLostPackets << "," 
        << avgThroughput << "," 
        <<networkThroughput<<","
        << overallPDR * 100 << "," 
        << overallPacketLossRatio * 100 << "," 
        << avgDelayNetwork << "," 
        << avgJitterNetwork << "," 
        << avgTxRate << "," 
        << avgRxRate << ","
        <<ns3::Simulator::Now().GetSeconds() <<"\n";
        outFile.close(); //  Close the file after writing

WriteNodeInfoDataset("node_information_datasetfinalv3.csv");
//ValidateEnergyConsumption();
//LogCumulativeEnergyMetrics("CumulativeEnergyMetricsfinalv3.csv");
std::cout << "done all writing\n";


     // Close the log file
    gpsLogFile.close();
   videoLogFile.close();
   connectionLogFile.close();
    packetLogFile.close();
   flowMonitorCsvFile.close();
    netPreformanceDataset.close();
    voiceLogFile.close();
   envLogFile.close();
    discoveryLogFile.close();
    centralDiscoveryLogFile.close();
   sarDiscoveryBroadcastLogFile.close();
receivedAssignmentsLogFile.close();
    macTxRxLogFile.close();
    hopLogFile.close();
    routingLogFile.close();
    debugLog.close();
    positionLogFile.close();
    pendingAssignmentCsv.close();
                if (!energyBuffer.empty())
{
    std::ofstream stepLog("StepEnergylogfinalv3.csv", std::ios::app);
    for (const auto &line : energyBuffer)
        stepLog << line;
    energyBuffer.clear();
}

if (!phyRxBuffer.empty())
{
    auto& out = *g_phyLogStream->GetStream();
    for (const auto& line : phyRxBuffer)
        out << line;
    phyRxBuffer.clear();
}

    std::cout << "done all file closing\n";

    Simulator::Destroy();
   


}
