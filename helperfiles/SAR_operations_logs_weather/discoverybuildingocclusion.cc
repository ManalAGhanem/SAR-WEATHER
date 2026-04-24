void CheckLosBasedDiscoveryForUndiscovered(
    const NodeContainer& sarNodes,
    const NodeContainer& civilianNodes,
    Ptr<Node> baseStation,
    uint16_t discoveryPort,
    const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes,
    const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes,
    Ptr<WeatherManager> weatherManager,
    const BuildingContainer& allBuildings
) {
    // Unified file name (one file for everything)
    static bool headerWritten = false;
    std::ofstream csv(routingsize+"los_discovery_events_finalv3.csv", std::ios_base::app);
    if (!csv.is_open()) {
        std::cerr << "⚠️ ERROR: Cannot open los_discovery_events.csv!\n";
        return;
    }
    if (!headerWritten) {
        csv << "seed,run,routing,time_s,sar_id,sar_type,civilian_id,"
               "distance_m,nominal_los_m,weather_los_m,"
               "within_nominal,within_weather,occluded_by_building,success,"
               "rain_mm_h,fog_g_m3,snow_mm_h,wind_m_s,humidity_g_m3,"
               "blocking_x,blocking_y,blocking_z\n";
        headerWritten = true;
    }

    // Weather snapshot (0 if not provided/stored)
    auto W = [&](const char* k)->double {
        return weatherManager ? weatherManager->GetWeatherCondition(k) : 0.0;
    };
    const double rain     = W("RainRate");    // mm/h
    const double fog      = W("FogDensity");  // g/m^3
    const double snow     = W("SnowRate");    // mm/h (if used)
    const double wind     = W("WindSpeed");   // m/s  (if used)
    const double humidity = W("Humidity");    // g/m^3

    const double now = Simulator::Now().GetSeconds();

    for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
        Ptr<Node> sarNode = sarNodes.Get(i);
        if (sarNode == baseStation) continue;
        const char* sarType = NodeTypeOf(sarNode, droneNodes, vehicleNodes, footNodes, helicopterNodes);

        // 1) Clear-weather LOS (nominal)
        const double nominalLos = ComputeNominalDiscoveryRadius(
            sarNode, droneNodes, vehicleNodes, footNodes, helicopterNodes);

        // 2) Weather-adjusted LOS using your *existing* GetDiscoveryRadius
        const double weatherLos = GetDiscoveryRadius(
            sarNode, droneNodes, vehicleNodes, footNodes, helicopterNodes, weatherManager);
   uint8_t tos = 0xE0; 
        Ptr<Socket> discoverySocket = GetCachedUdpSocket(sarNode, tos);
        if (!discoverySocket) { continue; }

        //Ptr<Socket> discoverySocket = Socket::CreateSocket(sarNode, UdpSocketFactory::GetTypeId());
        Vector sarPos = sarNode->GetObject<MobilityModel>()->GetPosition();
        const uint32_t sarId = sarNode->GetId();

        for (uint32_t j = 0; j < civilianNodes.GetN(); ++j) {
            Ptr<Node> civNode = civilianNodes.Get(j);
            const uint32_t civId = civNode->GetId();

            if (discoveryState[sarId][civId]) continue; // already discovered

            Vector civPos = civNode->GetObject<MobilityModel>()->GetPosition();
            const double distance = ns3::CalculateDistance(sarPos, civPos);

            // Building occlusion (keep your behavior: blocked => no discovery)
            Vector blockingBuildingPos = {0, 0, 0};
            const bool occluded = IsLineOfSightBlocked(sarPos, civPos, allBuildings, blockingBuildingPos);

            // Opportunity flags
            const bool within_nominal = (distance <= nominalLos) && !occluded;
            const bool within_weather = (distance <= weatherLos) && !occluded;

            bool success = false;
            if (within_weather) {
                ProcessDiscoveryEvent(sarId, civId, distance, "LOS", civPos, discoverySocket, baseStation, discoveryPort);
                discoveryState[sarId][civId] = true;
                success = true;
                NS_LOG_INFO(" LOS DISCOVERY: Civilian " << civId << " found by SAR " << sarId);
            } else if (occluded) {
                NS_LOG_INFO("⛔ LOS BLOCKED for Civilian " << civId << " by Building at ("
                            << blockingBuildingPos.x << ", "
                            << blockingBuildingPos.y << ", "
                            << blockingBuildingPos.z << ")");
            }
         

            // Log a row if: opportunity exists (within_nominal), or occluded, or success
            if (within_nominal || occluded || success) {
                csv << seed << "," 
                    << runNumber << ","  //  Tracks different runs of the same seed
                     << routingsize << ","  //  Tracks different runs of the same see
                    << now << ","
                    << sarId << "," << sarType << "," << civId << ","
                    << distance << ","
                    << nominalLos << ","
                    << weatherLos << ","
                    << (within_nominal ? 1 : 0) << ","
                    << (within_weather ? 1 : 0) << ","
                    << (occluded ? 1 : 0) << ","
                    << (success ? 1 : 0) << ","
                    << rain << "," << fog << "," << snow << "," << wind << "," << humidity << ","
                    << (occluded ? blockingBuildingPos.x : 0) << ","
                    << (occluded ? blockingBuildingPos.y : 0) << ","
                    << (occluded ? blockingBuildingPos.z : 0) << "\n";
            }
        }
    }

    csv.close();
}
