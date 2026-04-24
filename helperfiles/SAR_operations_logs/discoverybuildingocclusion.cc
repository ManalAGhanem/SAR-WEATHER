void CheckLosBasedDiscoveryForUndiscovered(
    const NodeContainer& sarNodes,
    const NodeContainer& civilianNodes,
    Ptr<Node> baseStation,
    uint16_t discoveryPort,
    const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes,
    const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes,
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
               "distance_m,nominal_los_m,"
               "within_nominal,occluded_by_building,success,"
               "blocking_x,blocking_y,blocking_z\n";
        headerWritten = true;
    }

       const double now = Simulator::Now().GetSeconds();

    for (uint32_t i = 0; i < sarNodes.GetN(); ++i) {
        Ptr<Node> sarNode = sarNodes.Get(i);
        if (sarNode == baseStation) continue;
        const char* sarType = NodeTypeOf(sarNode, droneNodes, vehicleNodes, footNodes, helicopterNodes);

        const double nominalLos = ComputeNominalDiscoveryRadius(sarNode, droneNodes, vehicleNodes, footNodes, helicopterNodes);

        Ptr<Socket> discoverySocket = Socket::CreateSocket(sarNode, UdpSocketFactory::GetTypeId());
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
            //const bool within_weather = (distance <= weatherLos) && !occluded;

            bool success = false;
            if (within_nominal) {
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
                    << (within_nominal ? 1 : 0) << ","
                    << (occluded ? 1 : 0) << ","
                    << (success ? 1 : 0) << ","
                    << (occluded ? blockingBuildingPos.x : 0) << ","
                    << (occluded ? blockingBuildingPos.y : 0) << ","
                    << (occluded ? blockingBuildingPos.z : 0) << "\n";
            }
        }
    }

    csv.close();
}
