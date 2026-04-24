
#include "../helperfiles/SAR_operations_logs/includes.cc"
#include "../helperfiles/routing_telemetry4.h"
#include "../helperfiles/routing_telemetry4.cc"

#include "../helperfiles/auto_logdir.h"


NS_LOG_COMPONENT_DEFINE("SearchRescueSimulation_scenariofinalv2");

#include "../helperfiles/SAR_operations_logs/variables.cc"
#include "../helperfiles/SAR_operations_logs/helperfunctions.cc"

#include "../helperfiles/SAR_operations_logs/disc_con_dataset.cc"


#include "../helperfiles/SAR_operations_logs/comm_dataset.cc"

#include "../helperfiles/SAR_operations_logs/nodeinfo_dataset.cc"
#include "../helperfiles/SAR_operations_logs/energy.cc"
//----------------------------START DISCOVERY , CONVERGENCE AND RESCUE----------------------------



#include "../helperfiles/SAR_operations_logs/discovery.cc"
#include "../helperfiles/SAR_operations_logs/discoverysarquerysignal.cc"
#include "../helperfiles/SAR_operations_logs/discoveryplb.cc"

#include "../helperfiles/SAR_operations_logs/convergence.cc"

   
#include "../helperfiles/SAR_operations_logs/rescue.cc"

//----------------------------END DISCOVERY , CONVERGENCE AND RESCUE----------------------------

// //----------------------------------(START)DATA COMMUNICATION : GPS, ENVIRONMENT, VOICE, VIDEO---------------------------------------------------------------

#include "../helperfiles/SAR_operations_logs/env.cc"
#include "../helperfiles/SAR_operations_logs/video.cc"
#include "../helperfiles/SAR_operations_logs/radio.cc"
#include "../helperfiles/SAR_operations_logs/gps.cc"
#include "../helperfiles/SAR_operations_logs/base_drone_com.cc"



//----------------------------------(END)DATA COMMUNICATION : GPS, ENVIRONMENT, VOICE, VIDEO---------------------------------------------------------------



#include "../helperfiles/SAR_operations_logs/buildinghelper.cc"

//#include "../helperfiles/SAR_operations_logs/post_rescue.cc"


#include "../helperfiles/SAR_operations_logs/snr&weather_dataset.cc"
//#include "../helperfiles/SAR_operations_logs/mem_stats.cc"

using namespace ns3;
using namespace sartelemetry;
int main(int argc, char *argv[]) {
    LogComponentEnable("SearchRescueSimulation_scenariofinalv2", LOG_LEVEL_INFO);

    double snapshotSeconds = 2.0;

     // Parse command-line arguments
        CommandLine cmd(__FILE__);
        cmd.AddValue("routing",  "Routing protocol (OLSR|AODV|DSDV)", routing);
        cmd.AddValue("scenario", "Scenario identifier for results namespacing", scenarioId);
        cmd.AddValue("snapshot", "Route-table snapshot period in seconds (0=off)", snapshotSeconds);
        cmd.AddValue("RngRun", "Run number for RNG", runNumber);  
        cmd.Parse(argc, argv);

        // Set run and retrieve values 
        RngSeedManager::SetRun(runNumber);
        routingsize = routing + "_" + scenarioId;

        // Normalize routing name to uppercase (OPTIONAL)
        for (auto& c : routing)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        // Now you can build file prefix etc.
        const std::string prefix = BuildPrefix(scenarioId, routing, seed, runNumber);
        



  //  LogComponentEnable("DsdvRoutingProtocol", LOG_LEVEL_ALL);
    LogComponentEnable("DsdvRoutingTable", LOG_LEVEL_ALL);

    rescueAssignmentLogFile.open("rescue_assignment_logfinalv2.csv", std::ios::app);
    rescueAssignmentLogFile << "Timestamp,,Seed,replica,Civilian ID,Assigned SAR Node ID, Assigned SAR Node IP,Civilian X,Civilian Y,Civilian Z\n";
    receivedAssignmentsLogFile.open("recieved_assignment_logfinalv2.csv",  std::ios::app);
    receivedAssignmentsLogFile << "Time (s),Seed,replica,Receiver SAR Node ID,Sender Address,Civilian ID,Assigned SAR Node ID, Assigned SAR Node IP,Civilian X,Civilian Y,Civilian Z\n"; // Headers for local log
    positionLogFile.open(routingsize+"_position_logfinalv1.csv", std::ios::app);
    positionLogFile << "seed, runnumber, routing, Time(s),NodeID,X,Y,Z, velocityx, velocityy,velocityz,speed\n";
    signalLogFile.open("CiviliansignallogV45finalv2.csv",std::ios::app);
    signalLogFile << "Timestamp,seed,replica,Status,Signal Type,Sender Address,Receiver Address,Packet Size,"
                      << "Sender Node,Distance (meters), civilian location(X), civilian location(Y), civilian location(Z),port, interval(seconds)" << std::endl;
    movementLogFile.open("movemnetsfinalv2.csv");
    movementLogFile << "Timestamp,SAR Node ID,SAR X,SAR Y,SAR Z, Civilian ID,Civilian X,Civilian Y,Civilian Z, SAR Distance to Base, Civilian Distance to Base \n";
    rescueCompletionLogFile.open("rescue_completion_logfinalv2.csv");
    rescueCompletionLogFile << "Timestamp,SAR Node ID,Civilian ID,SAR X,SAR Y,SAR Z,Civilian X,Civilian Y,Civilian Z,SAR Distance to Base,Civilian Distance to Base,Base X,Base Y,Base Z,Status\n";
    routingLogFile.open("routing-table-logfinalv2.csv");
    systemLogFile.open("system_events_logfinalv2.csv", std::ios::out | std::ios::trunc);
    if (!systemLogFile.is_open()) {
        std::cerr << "Failed to open system log file!" << std::endl;
    } else {
        systemLogFile << "Time,Event,Details" << std::endl;
    }

    macTxRxLogFile.open("MacTxRxLogfinalv2.txt", std::ios::out);
if (!macTxRxLogFile.is_open()) {
    NS_LOG_ERROR("Unable to open file for writing MacTx/MacRx logs");
}

   attachmentLogFile.open("attachment_logfinalv2.csv");
    attachmentLogFile << "Timestamp,Action,SAR Node ID,SAR Node Position (X,Y,Z),Civilian ID,Civilian Position (X,Y,Z),Details\n";

    convergenceLogFile.open("convergence_logfinalv2.csv");
    convergenceLogFile << "Timestamp,SAR IP, SAR Node ID,Civilian ID,SAR X,SAR Y,SAR Z,Civilian X,Civilian Y,Civilian Z,Base X,Base Y,Base Z,DistanceToCivilian, DistanceToBase, Status \n";

    connectionLogFile.open("connection_status_logfinalv2.csv");
    connectionLogFile << "Time,Status" << std::endl;  // CSV headers
    gpsLogFile.open("gps_communication_logfinalv2.csv");
    gpsLogFile << "Time,Action,Type,NodeName,NodeAddress,OldX,OldY,OldZ,CurrentX,CurrentY,CurrentZ,Details" << std::endl;  // CSV headers
    videoLogFile.open("video_communication_logfinalv2.csv");
    videoLogFile << "Time,Action,Type,Source,Destination,Size (bytes)" << std::endl;  // CSV headers

    SetupLocalDiscoveryLogging();       
    SetupCentralDiscoveryLogging();     
    sarDiscoveryBroadcastLogFile.open("sar_discovery_broadcast_logfinalv2.csv");
    sarDiscoveryBroadcastLogFile << "Time,Receiving SAR Node ID,Broadcasting SAR Node ID,Discovering SAR Node ID,Civilian Node ID,Distance to civilian,Civilian X position,Civilian Y position,Civilian Z position,Status\n";

     InitializeCommunicationLogFile();

    // **City Grid Parameters**
    double cityHight=1000.0;
    double cityWidth=1000.0;
    
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
             }
    }
    }

  
        
    // creating nodes
    NodeContainer baseNode, droneNode,helicopterNode, vehicleNode, footNodes,civilianNodesPLB, civilianNodesquery, civilianNodespassive;
    baseNode.Create(1);
    droneNode.Create(1);
    helicopterNode.Create(1);  
    vehicleNode.Create(4);
    footNodes.Create(5);

    civilianNodesPLB.Create(9);
    civilianNodesquery.Create(9);
    civilianNodespassive.Create(9);
    civilianNodes.Add(civilianNodesPLB);
    civilianNodes.Add(civilianNodesquery);
    civilianNodes.Add(civilianNodespassive);

   //NS_LOG_INFO("Nodes created");
    g_baseNode = baseNode.Get(0); 
    allNodes.Add(baseNode);
    allNodes.Add(droneNode);
    allNodes.Add(helicopterNode);
    allNodes.Add(vehicleNode);
    allNodes.Add(footNodes);
    allNodes.Add(civilianNodesPLB);
    allNodes.Add(civilianNodesquery);
    allNodes.Add(civilianNodespassive);
    groundNodes.Add(vehicleNode);
    groundNodes.Add(footNodes);

   

   // Loop over all nodes in the NodeContainer and initialize their attached state to stop them from sending anything if attached
    for (uint32_t i = 0; i < allNodes.GetN(); i++) {
        Ptr<Node> node = allNodes.Get(i); // Get the node at index i
        g_civilianAttachedState[node->GetId()] = false; // Default to not attached
    }


    // After creating nodes and assigning them to containers:
    AssignTypeAndRole(baseNode, "Base Station", "Coordinator", "Fixed High-Gain");
    AssignTypeAndRole(droneNode, "Drone", "Discoverer", "Aerial WiFi");
    AssignTypeAndRole(helicopterNode, "Helicopter", "Discoverer", "High-Gain Relay");
    AssignTypeAndRole(vehicleNode, "Vehicle", "Rescuer", "Onboard WiFi");
    AssignTypeAndRole(footNodes, "Foot Team", "Rescuer", "Handheld Radio");
    AssignTypeAndRole(civilianNodesPLB, "Civilian", "Civilian", "PLB");
    AssignTypeAndRole(civilianNodesquery, "Civilian", "Civilian", "Hybrid Messages Receiver");
    AssignTypeAndRole(civilianNodespassive, "Civilian", "Civilian", "Unknown");
  

  // Initialize the safety status tracker
   InitializeCivilianSafetyStatus(civilianNodes);

    

   for (auto& entry : nodeDataset) {
    UpdateNodeAttributes(entry.first);
    }
    ScheduleNodeUpdates();  // Call function to begin periodic updates
    
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::HybridBuildingsPropagationLossModel");

    YansWifiPhyHelper wifiPhy; 
    
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11); // Enable packet capturing for analysis
   
    wifiPhy.SetChannel(wifiChannel.Create());
      
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11); // Enable packet capturing for analysis
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
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


    NetDeviceContainer allWifiDevices;
    allWifiDevices.Add(basedevices);
    allWifiDevices.Add(droneDevices);
    allWifiDevices.Add(helicopterDevices);
    allWifiDevices.Add(vehicleDevices);
    allWifiDevices.Add(footTeamDevices);
    allWifiDevices.Add(civilianDevicesPLB);
    allWifiDevices.Add(civilianDevicesquery);
    allWifiDevices.Add(civilianDevicespassive);

    NS_LOG_INFO("WIFI ready...");

    ConfigureNodeWiFiAttributes(baseNode);
    ConfigureNodeWiFiAttributes(droneNode);
    ConfigureNodeWiFiAttributes(helicopterNode);
    ConfigureNodeWiFiAttributes(vehicleNode);
    ConfigureNodeWiFiAttributes(footNodes);
    ConfigureNodeWiFiAttributes(civilianNodesPLB);
    ConfigureNodeWiFiAttributes(civilianNodesquery);
    ConfigureNodeWiFiAttributes(civilianNodespassive);

    // Initialize packet counters for all nodes
        for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
            packetSentCount[i] = 0; // Initialize sent count to 0
            packetReceivedCount[i] = 0; // Initialize received count to 0
        }
  
    // set up mobility models
     MobilityHelper mobility;
    //NS_LOG_INFO("Setting up mobility models...");

    // Set a consistent Position Allocator with larger bounds for all nodes
    Ptr<PositionAllocator> dronePositionAlloc = CreateObject<RandomRectanglePositionAllocator>();
    dronePositionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    dronePositionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    dronePositionAlloc->SetAttribute("Z", DoubleValue(100.0)); 


    // Set initial positions for all nodes using the position allocator
    mobility.SetPositionAllocator(dronePositionAlloc);


  mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                          "PositionAllocator", PointerValue(dronePositionAlloc),
                          "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=20.0]"),  // Speed range
                          "Pause", StringValue("ns3::UniformRandomVariable[Min=3.0|Max=10.0]")); // Pause duration
                           mobility.Install(droneNode);
    Ptr<PositionAllocator> helicopterPositionAlloc = CreateObject<RandomRectanglePositionAllocator>();
    helicopterPositionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    helicopterPositionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    helicopterPositionAlloc->SetAttribute("Z", DoubleValue(150.0)); 


    // Set initial positions for all nodes using the position allocator
    mobility.SetPositionAllocator(helicopterPositionAlloc);

        // helicopter Nodes Mobility (MIXED )

          mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
            "PositionAllocator", PointerValue(helicopterPositionAlloc),
            "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=30.0]"),  // Speed range
            "Pause", StringValue("ns3::UniformRandomVariable[Min=3.0|Max=20.0]")); // Pause duration

    mobility.Install(helicopterNode);
      

   


    Ptr<PositionAllocator> groundPositionAlloc = CreateObject<RandomRectanglePositionAllocator>();
    groundPositionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    groundPositionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    groundPositionAlloc->SetAttribute("Z", DoubleValue(2.0)); 


    // Set initial positions for all nodes using the position allocator
    mobility.SetPositionAllocator(groundPositionAlloc);
    
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
        "PositionAllocator", PointerValue(groundPositionAlloc),
        "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"),  // Speed range
        "Pause", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=8.0]")); // Pause duration


    mobility.Install(vehicleNode);
     PlaceNodesOutsideBuildings(vehicleNode, allBuildings, 2);

    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
        "PositionAllocator", PointerValue(groundPositionAlloc),
        "Speed", StringValue("ns3::UniformRandomVariable[Min=2.0|Max=5.0]"),  // Speed range
        "Pause", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]")); // Pause duration
 

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
    plbPositionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    plbPositionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
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
    // Assign the civilian position inside the building
    Ptr<MobilityModel> mobilityModel = civilianNodesPLB.Get(i)->GetObject<MobilityModel>();
      mobilityModel->SetPosition(buildingEdge);


    }


    for (uint32_t i = 0; i < civilianNodesPLB.GetN(); i++) {
        Ptr<MobilityModel> mob = civilianNodesPLB.Get(i)->GetObject<MobilityModel>();
        Vector pos = mob->GetPosition();
        // Record as both initial and final position (since the node never moves)
        initialPositions[civilianNodesPLB.Get(i)->GetId()] = pos;
        finalPositions[civilianNodesPLB.Get(i)->GetId()] = pos;
    }

    BuildingsHelper::Install(footNodes);
    BuildingsHelper::Install(civilianNodes);
    BuildingsHelper::Install(vehicleNode);
    BuildingsHelper::Install(helicopterNode);
    BuildingsHelper::Install(droneNode);
    BuildingsHelper::Install(baseNode);

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
    std::ofstream debugLog("mobility_debugfinalv2.txt", std::ios::app); // Open file in append mode

          //NS_LOG_INFO("Mobility models setup completed.");

    Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&PositionChangeCallback));

    // Connect the `CourseChange` trace to each node
  for (uint32_t i = 0; i < allNodes.GetN (); ++i)
    {
      // The node’s MobilityModel
      Ptr<MobilityModel> mob = allNodes.Get (i)->GetObject<MobilityModel> ();
      if (mob)
        {
          // Connect the CourseChange trace to our static function
          mob->TraceConnectWithoutContext ("CourseChange", MakeCallback (&VelocityTrace));
        }
    }


    //internet stack
    InternetStackHelper internet;
    OlsrHelper olsr; // Use OLSR as the routing protocol
    AodvHelper aodv;
    DsdvHelper dsdv;
    Ipv4ListRoutingHelper list;
    AsciiTraceHelper ascii;

    //    wifiPhy.EnableAsciiAll(ascii.CreateFileStream("mac-layer.tr"));
    if (routing == "OLSR") {
    list.Add(olsr, 100);
    } else if (routing == "AODV") {
    list.Add(aodv, 100);
    } else if (routing == "DSDV") {
    list.Add(dsdv, 100);
    } else {
    NS_FATAL_ERROR("Unknown --routing=" << routing << " (expected OLSR|AODV|DSDV)");
    }

   internet.SetRoutingHelper(list);
   internet.Install(allNodes);

     
    //files for RSSI AND SNR
    g_phyLogStream = Create<OutputStreamWrapper>("SNR_log_finalv2.csv", std::ios::out);

    *g_phyLogStream->GetStream() << "Time,RxNodeId,TxNodeId,Distance,RSSI,Noise,SNR\n";

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


// Assigning IP addresses

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer allinterfaces=ipv4.Assign(allWifiDevices); 
    InitializeEnergyModels(allNodes);
    PopulateIpToNodeIdMap(allNodes);
        // Initialize maxReceptionDistance to 0 for all nodes:
        for (uint32_t i = 0; i < allNodes.GetN(); i++) {
            maxReceptionDistance[i] = 0.0;
        }

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

// Define thresholds and fallback intervals
  double positionChangeThreshold = 10.0;  // Meters
  double fallbackInterval = 100.0;       // Seconds

  SetupGpsUpdates(sarNodes,
      baseNode.Get(0),   
      gpsPort,              
      positionChangeThreshold,              
      fallbackInterval,             
      2.0);             
   
  SetupVoiceCommunication(vehicleNode,helicopterNode, footNodes, baseNode.Get(0));
          //NS_LOG_INFO("voice sent");
  //enviornment data
    double temperature = 25.0;
    double humidity = 60.0;
    double windSpeed = 5.0;
    SetupEnvironmentalCommunication(droneNode, helicopterNode, g_baseNode, envPort, temperature, humidity, windSpeed);
    // Video Feed Communication Setup
    SetupVideoCommunication(vehicleNode, droneNode, g_baseNode, videoPort);
    // Base station sends control messages to the drone
    SetupBaseToDroneControl( droneNode,  g_baseNode,  controlMsgPort);
    //NS_LOG_INFO("Base station to drone control setup complete.");
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
        
        SetupSocketCommunication(civilianNodesPLB, PLBPort, ns3::Ipv4Address("255.255.255.255"), "PLB", 1.0);
        
        // setup Civilian answering query
        SetupHybridModeForCivilians(civilianNodesquery, SARcivilianqueryport);
        // Setup SAR nodes as receivers
        SetupReceiversForSARNodes(sarNodes, PLBPort, "PLB");        // For PLB
       // SetupReceiversForSARNodes(groundNodes, WificivilianPort, "WiFi"); // For WiFi
        SetupReceiversForSARNodes(sarNodes, QueryResponsePort, "QueryResponse"); // For QueryResponse


       // Setup SAR nodes to broadcast queries
     uint32_t SARCommunicationIdBase = 2000; // Base communication ID for SAR nodes

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

    

        // Base station: Convergence update reception
         Ptr<Socket> baseRecvSocket = Socket::CreateSocket(baseNode.Get(0), UdpSocketFactory::GetTypeId());
         baseRecvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), convergencePort));
         baseRecvSocket->SetRecvCallback(MakeCallback(&LogConvergenceUpdate));

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



Simulator::Schedule(
        Seconds(20.0),
        &DiscoverCivilianNodes,
        sarNodes,        // your SAR node container
        civilianNodes,   // all civilians
        baseNode.Get(0), // base station
        discoveryPort,
        droneNode,
        vehicleNode,
        footNodes,
        helicopterNode,
        allBuildings,
        trees
    );

    
       wifiPhy.EnablePcapAll("base-scenario-trace-finalv2-dataset");

     

        // Install FlowMonitor to track metrics
        Ptr<FlowMonitor> flowMonitor;
        FlowMonitorHelper flowHelper;
        flowMonitor = flowHelper.InstallAll();
    
    


     
       
    // Define and open flow-level CSV file
    // Open CSV files
    std::ofstream flowMonitorCsvFile(routingsize+","+std::to_string(runNumber)+"overall-flow-monitor-resultsfinalv2.csv", std::ios::app);
    flowMonitorCsvFile << "seed, runtimes,routing,Flow ID,Start Time (s),End Time (s),seed,replica,Source IP,Destination IP,Source Node ID,Destination Node ID,Source Port, Destination Port, Tx Packets,Tx Bytes,"
                   << "Rx Packets,Rx Bytes,Throughput (Mbps),Throughput (Kbps),Average Delay (s),Average Delay (ms),"
                   << "Average Jitter (s),Lost Packets,Packet Loss Ratio (%),Packet Delivery Ratio (%),Data Packet Delivery Ratio (%),"
                   << "Mean Hop Count,Packets Forwarded,Tx Rate (Kbps),Rx Rate (Kbps)\n";




 std::ofstream netPreformanceDataset(routingsize+","+std::to_string(runNumber)+"NetwrokDatasetfinalv2.csv");
    netPreformanceDataset << "seed, runtimes,routing,Flow ID,Start Time (s),End Time (s),Source IP,Destination IP,Source Node ID,Destination Node ID,Source Port,Destination Port,Protocol, Tx Packets,Tx Bytes,"
                   << "Rx Packets,Rx Bytes,Throughput (Mbps),Throughput (Kbps),Average Delay (s),Average Delay (ms),"
                   << "Average Jitter (s),Lost Packets,Packet Loss Ratio (%),Packet Delivery Ratio (%),Data Packet Delivery Ratio (%),"
                   << "Mean Hop Count,Packets Forwarded,Tx Rate (Kbps),Rx Rate (Kbps)\n";

   // Open the detailed packet log CSV file
      packetLogFile.open(routingsize+","+std::to_string(runNumber)+"_detailed-packet-logfinalv2.csv",  std::ios::app);
 
   packetLogFile << "Seed,Runnumber,routing,Timestamp,Communication ID, Packet ID, Port, Source IP,Destination IP,Packet Type,Delay (ms)"<< std::endl;

//voice log
    voiceLogFile << "Timestamp,Action,Type,Source,Destination,Size (bytes), Priority" << std::endl;

    scenarioStartTime = Simulator::Now().GetSeconds();

    Simulator::Schedule(Seconds(1.0), &CheckAllCiviliansSafe, 
    droneNode, vehicleNode, footNodes, helicopterNode);
    //files for RSSI AND SNR
g_phyLogStream = Create<OutputStreamWrapper>(routingsize+","+std::to_string(runNumber)+"SNR_log_finalv1.csv", std::ios::out);

*g_phyLogStream->GetStream()
  << "Time,RxNodeId,RxDeviceId,TxNodeId,Distance,RSSI,Noise,SNR,PacketUid,PacketSizeBytes,ChannelFreqMhz,StaId\n";


   // Simulator::Schedule(Seconds(1.0), &LogWeatherMetricsForAllNodes,"Weather_finalv2.csv", weatherLoss, std::ref(nodeDataset), allNodes);
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

for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
    Ptr<Node> node = allNodes.Get(i);
    for (uint32_t d = 0; d < node->GetNDevices(); ++d) {
        Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(node->GetDevice(d));
        if (!wifiDev) continue;
        Ptr<WifiPhy> phy = wifiDev->GetPhy();

        // Construct the config path and lambda INSIDE the loop, capturing i and d:
        std::string path = "/NodeList/" + std::to_string(i) + "/DeviceList/" + std::to_string(d) + "/Phy/MonitorSnifferRx";
    }}
       Config::Connect(
    "/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx",
    MakeCallback(&PhyRxLogCallback)
);

    //start routing files
  StartForRun(scenarioId, routing, seed, runNumber, snapshotSeconds);

//end routing files

    Simulator::Run();
    RoutingTelemetry::Stop();         // write summaries & close files

    //NS_LOG_INFO("SIM DONE...");
    FinalizeEnergyLogs(nodeEnergySources);
 LogAverageSnrRssi();
     LogAverageSnrRssiNodes();
    FinalizeAllCommunicationSessions();
    WriteDiscoveryConvergenceDataset(routingsize+"_discovery_convergence_datasetfinalv2.csv");


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
    nodeDataset[nodeId].nodeBehavior = GetNodeBehavior(nodeId);

    // Node-specific parameters
   // double distance = maxReceptionDistance[nodeId];  // 
    NodeInfo& nodeInfo = nodeDataset[nodeId];
    double noiseFigure = 7.0;  // dB
    double channelBandwidth = 20e6;  // Hz
    std::string pathLossModel = "buildings";
    nodeInfo.pathLossModel = pathLossModel;  // Store the updated pathLossModel


    // Adjust path loss exponent dynamically based on node type
    double pathLossExponent = (nodeInfo.nodeRole == "Drone" || nodeInfo.nodeRole == "Helicopter") ? 2.0 : 2.0;
    nodeDataset[nodeId].pathLossModel= pathLossModel;
    nodeDataset[nodeId].channelBandwidth=channelBandwidth;
     nodeDataset[nodeId].noiseFigure=noiseFigure;
     nodeDataset[nodeId].distance= maxReceptionDistance[nodeId];
    nodeInfo.communicationRange = CalculateCommunicationRange(nodeInfo, pathLossExponent);
    double distance = maxReceptionDistance[nodeId];
    nodeInfo.connectivityLevel = GetDynamicConnectivityLevel(nodeInfo, distance, channelBandwidth, noiseFigure, pathLossModel);
   



}





// Install FlowMonitor to track metrics
// Process FlowMonitor data after simulation ends
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
    double meanHopCount = rxPackets > 0 ? static_cast<double>(flow.second.timesForwarded) / rxPackets : 0.0;
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
    std::cout << "Flow " << flow.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Source Node ID: " << sourceNodeId << ", Destination Node ID: " << destNodeId << "\n";
    std::cout << "  Start Time: " << startTime << " s, End Time: " << endTime << " s\n";
    std::cout << "  Tx Packets: " << txPackets << ", Rx Packets: " << rxPackets << "\n";
    std::cout << "  Throughput: " << throughput << " Mbps (" << throughputKbps << " Kbps), Avg Delay: " << avgDelay << " s (" << avgDelayMs << " ms), Avg Jitter: " << avgJitter << " s\n";
    std::cout << "  Lost Packets: " << lostPackets << ", Packet Loss Ratio: " << packetLossRatio << ", PDR: " << pdr << ", DPDR: " << dpdr << "%\n";
    std::cout << "  Mean Hop Count: " << meanHopCount << ", Tx Rate: " << txRate << " Kbps, Rx Rate: " << rxRate << " Kbps\n";
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
std::cout << "\n===== Overall Network Statistics =====\n";
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



std::ofstream outFile("OVERALL_network_statisticsfinalv2.csv", std::ios::app);
std::ifstream checkFile("OVERALL_network_statisticsfinalv2.csv");
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
outFile.close();
WriteNodeInfoDataset("node_information_datasetfinalv2.csv");


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
    //hopLogFile.close();
    routingLogFile.close();
    debugLog.close();
    positionLogFile.close();
    Simulator::Destroy();

}
