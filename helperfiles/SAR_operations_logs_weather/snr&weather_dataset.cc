

void LogWeatherMetricsForAllNodes(const std::string& filename,Ptr<WeatherAttenuationModel> weatherModel, 
                                    std::map<uint32_t, NodeInfo>& nodeDataset,
    const NodeContainer& allNodes)
{
    for (uint32_t i = 0; i < allNodes.GetN(); ++i) {
    for (uint32_t j = 0; j < allNodes.GetN(); ++j) {
    if (i == j) continue; // Avoid self-transmission

    Ptr<Node> sender = allNodes.Get(i);
    Ptr<Node> receiver = allNodes.Get(j);

    Ptr<MobilityModel> senderMobility = sender->GetObject<MobilityModel>();
    Ptr<MobilityModel> receiverMobility = receiver->GetObject<MobilityModel>();

    if (!senderMobility || !receiverMobility) continue;

    //  Get nodeId and lookup in `nodeDataset`
    uint32_t nodeId = sender->GetId();

    if (nodeDataset.find(nodeId) == nodeDataset.end()) {
    NS_LOG_WARN("Node ID " << nodeId << " not found in dataset.");
    continue; // Skip if node not found
    }

    NodeInfo& nodeInfo = nodeDataset[nodeId];  //  Lookup sender node's info

    double txPowerDbm = nodeInfo.transmitPower;  //  Get transmit power

    //  Call WriteCsvRecord for logging
    weatherModel->WriteCsvRecord(filename, txPowerDbm, senderMobility, receiverMobility);
    }
    }

    //  Reschedule for periodic updates
    Simulator::Schedule(Seconds(10.0), &LogWeatherMetricsForAllNodes,filename, weatherModel, std::ref(nodeDataset), std::ref(allNodes));
}
// Map to track SNR and RSSI sums and counts per rxNodeId
struct NodeStats {
    double rssiSum = 0.0;
    double snrSum = 0.0;
    double distanceSum = 0.0;
    uint64_t packetCount = 0;
};
static Ptr<OutputStreamWrapper> g_phyLogStream;
static std::map<uint32_t, NodeStats> g_nodeStats; // Key: rxNodeId
static std::map<std::pair<uint32_t, uint32_t>, NodeStats> g_nodePairStats; // Key: (rxNodeId, txNodeId)

void LogAverageSnrRssi() {
    Ptr<OutputStreamWrapper> avgStream = Create<OutputStreamWrapper>("Average_sar_SNR_finalv3.csv", std::ios::app);

    *avgStream->GetStream() << "Seed, RunNumber, Routing, RxNodeId,AvgRSSI,AvgSNR,PacketCount\n";

     for (const auto& entry : g_nodeStats) {
        uint32_t rxNodeId = entry.first;
        const NodeStats& stats = entry.second;
        double avgRssi = (stats.packetCount > 0) ? stats.rssiSum / stats.packetCount : 0.0;
        double avgSnr = (stats.packetCount > 0) ? stats.snrSum / stats.packetCount : 0.0;
        *avgStream->GetStream() 
             <<seed<<    ","
        <<runNumber<<    ","
        <<routingsize<<    ","<< rxNodeId << "," << avgRssi << "," << avgSnr << "," << stats.packetCount <<  "\n";
    }
    NS_LOG_INFO("Average SNR and RSSI logged to Average_sar_SNR_finalv4small.csv");
}

void LogAverageSnrRssiNodes() {
    std::cout << "Logging pair-wise averages (with NA) to Average_SNR_finalv3.csv" << std::endl;
    Ptr<OutputStreamWrapper> avgStream = Create<OutputStreamWrapper>("OVERALL_Averages_SNR_finalv3.csv", std::ios::app);
    *avgStream->GetStream() << "SEED,RUNNUMBER,ROUTING,RxNodeId,TxNodeId,AvgDistance,AvgRSSI,AvgSNR,PacketCount\n";

        for (const auto& entry : g_nodePairStats) {
        uint32_t rxNodeId = entry.first.first;
        uint32_t txNodeId = entry.first.second;
        const NodeStats& stats = entry.second;
        double avgRssi = (stats.packetCount > 0) ? stats.rssiSum / stats.packetCount : 0.0;
        double avgSnr = (stats.packetCount > 0) ? stats.snrSum / stats.packetCount : 0.0;
        double avgDist = (stats.packetCount > 0) ? stats.distanceSum / stats.packetCount : 0.0;

        *avgStream->GetStream()
        <<seed<<    ","
        <<runNumber<<    ","
        <<routingsize<<    ","
        << rxNodeId << ","
        << (txNodeId == UINT32_MAX ? "NA" : std::to_string(txNodeId)) << ","
        << avgDist << "," << avgRssi << "," << avgSnr << "," << stats.packetCount <<"\n";
    }
    NS_LOG_INFO("Average SNR, RSSI, and distance per node pair (including NA cases) logged to Average_SNR_finalv3.csv");
}
void PhyRxLogCallback(
    std::string context,
    Ptr<const Packet> packet,
    unsigned short channelFreqMhz,
    WifiTxVector txVector,
    MpduInfo mpduInfo,
    SignalNoiseDbm signalNoise,
    unsigned short staId)
{
    // ---------- STATIC CONTROL ----------
    static std::map<uint32_t, double> lastLogTime;
    //static std::vector<std::string> phyRxBuffer;
    const double LOG_PERIOD = 1.0;        // seconds
    const size_t FLUSH_THRESHOLD = 2000;  // lines

    // ---------- PARSE RX NODE ----------
    uint32_t rxNodeId = 0, rxDeviceId = 0;
    if (sscanf(context.c_str(),
               "/NodeList/%u/DeviceList/%u/Phy/MonitorSnifferRx",
               &rxNodeId, &rxDeviceId) != 2)
    {
        return;
    }

    if (rxNodeId >= NodeList::GetNNodes())
        return;

    Ptr<Node> rxNode = NodeList::GetNode(rxNodeId);
    if (!rxNode)
        return;

    // ---------- TX NODE FROM TAG ----------
    NodeIdTag tag;
    int32_t txNodeId = -1;
    if (packet->PeekPacketTag(tag))
        txNodeId = tag.GetNodeId();

    // ---------- DISTANCE ----------
    double distance = -1.0;
    if (txNodeId >= 0 && (uint32_t)txNodeId < NodeList::GetNNodes())
    {
        Ptr<Node> txNode = NodeList::GetNode(txNodeId);
        if (txNode)
        {
            Ptr<MobilityModel> rxMob = rxNode->GetObject<MobilityModel>();
            Ptr<MobilityModel> txMob = txNode->GetObject<MobilityModel>();
            if (rxMob && txMob)
                distance = CalculateDistance(
                    rxMob->GetPosition(),
                    txMob->GetPosition(),
                    true);
        }
    }

    // ---------- SIGNAL METRICS ----------
    double rssi  = signalNoise.signal;
    double noise = signalNoise.noise;
    double snr   = rssi - noise;

    // ---------- AGGREGATION (ALWAYS) ----------
    g_nodeStats[rxNodeId].packetCount++;
    g_nodeStats[rxNodeId].rssiSum += rssi;
    g_nodeStats[rxNodeId].snrSum  += snr;

    uint32_t pairKey =
        (txNodeId >= 0) ? txNodeId : UINT32_MAX;

    auto& pairStats =
        g_nodePairStats[std::make_pair(rxNodeId, pairKey)];

    pairStats.packetCount++;
    pairStats.rssiSum += rssi;
    pairStats.snrSum  += snr;
    pairStats.distanceSum += distance;

    // ---------- THROTTLE LOGGING ----------
    double now = Simulator::Now().GetSeconds();
    if (now - lastLogTime[rxNodeId] < LOG_PERIOD)
        return;

    lastLogTime[rxNodeId] = now;

    // ---------- BUILD CSV LINE (NO I/O) ----------
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << now << ","
        << rxNodeId << ","
        << (txNodeId >= 0 ? std::to_string(txNodeId) : "NA") << ","
        << (distance >= 0 ? std::to_string(distance) : "NA") << ","
        << rssi << "," << noise << "," << snr
        << "\n";

    phyRxBuffer.push_back(oss.str());

    // ---------- PERIODIC FLUSH ----------
    if (phyRxBuffer.size() >= FLUSH_THRESHOLD)
    {
        auto& out = *g_phyLogStream->GetStream();
        for (const auto& line : phyRxBuffer)
            out << line;
        phyRxBuffer.clear();
    }
}


static void
EnsureSnrWeatherFileOpen(const std::string& filename)
{
    if (!g_snrWeatherOut.is_open()) {
        g_snrWeatherOut.open(filename, std::ios::out | std::ios::app);
    }
    if (!g_snrWeatherOut.is_open()) {
        NS_LOG_ERROR("Unable to open " << filename << " for writing SNR+weather logs.");
    }
}

static void
EnsureSnrWeatherHeader(const std::string& filename)
{
    if (g_snrWeatherHeaderWritten) {
        return;
    }

    EnsureSnrWeatherFileOpen(filename);
    if (!g_snrWeatherOut.is_open()) {
        return;
    }

    // If file is empty, write header
    g_snrWeatherOut.seekp(0, std::ios::end);
    if (g_snrWeatherOut.tellp() == 0) {
        g_snrWeatherOut
            << "Seed,Run,Routing,Time,TxNodeId,RxNodeId,AvgDistance,TxPowerDbm,"
               "AvgRSSI,AvgSNR,PacketCount,"
               "AvgRain,AvgFog,AvgSnow,AvgWind,AvgHumidity\n";
    }
    g_snrWeatherHeaderWritten = true;
}

void
LogWeatherMetricsForAllNodeswithSNR(const std::string& filename,
                                    Ptr<WeatherAttenuationModel> weatherModel,
                                    std::map<uint32_t, NodeInfo>& nodeDataset,
                                    const NodeContainer& allNodes)
{
    const uint32_t n = allNodes.GetN();
    if (n == 0) return;

    EnsureSnrWeatherHeader(filename);
    if (!g_snrWeatherOut.is_open()) {
        return;
    }

    // ---- Define the time window for this logging step ----
    double now = Simulator::Now().GetSeconds();

    // First call: approximate a 10 s window from [now-10, now]
    double windowStart =
        (g_lastSnrWeatherLogTime > 0.0)
        ? g_lastSnrWeatherLogTime
        : std::max(0.0, now - 10.0);

    double windowEnd = now;

    // ---- Get time-weighted average weather over [windowStart, windowEnd] ----
    // Assumes WeatherManager has:
    //   std::map<std::string, double> GetAverageWeatherConditions(double startTime, double endTime) const;
    std::map<std::string, double> avgWeather =
        weather->GetAverageWeatherConditions(windowStart, windowEnd);

    double avgRain     = avgWeather.count("RainRate")   ? avgWeather["RainRate"]   : 0.0;
    double avgFog      = avgWeather.count("FogDensity") ? avgWeather["FogDensity"] : 0.0;
    double avgSnow     = avgWeather.count("SnowRate")   ? avgWeather["SnowRate"]   : 0.0;
    double avgWind     = avgWeather.count("WindSpeed")  ? avgWeather["WindSpeed"]  : 0.0;
    double avgHumidity = avgWeather.count("Humidity")   ? avgWeather["Humidity"]   : 0.0;

    // ---- Per-node / per-pair SNR + distance averages over the same window ----
    for (uint32_t i = 0; i < n; ++i)
    {
        Ptr<Node> sender = allNodes.Get(i);
        uint32_t txNodeId = sender->GetId();

        auto itInfo = nodeDataset.find(txNodeId);
        if (itInfo == nodeDataset.end()) {
            NS_LOG_WARN("Node ID " << txNodeId << " not found in dataset.");
            continue;
        }
        NodeInfo& nodeInfo = itInfo->second;
        double txPowerDbm = nodeInfo.transmitPower;

        Ptr<MobilityModel> senderMob = sender->GetObject<MobilityModel>();
        if (!senderMob) continue;

        for (uint32_t j = 0; j < n; ++j)
        {
            if (i == j) continue;

            Ptr<Node> receiver = allNodes.Get(j);
            uint32_t rxNodeId = receiver->GetId();

            Ptr<MobilityModel> receiverMob = receiver->GetObject<MobilityModel>();
            if (!receiverMob) continue;

            // Same keying as your PhyRxLogCallback / g_nodePairStats usage
            auto key = std::make_pair(rxNodeId, txNodeId);
            auto it = g_nodePairStats.find(key);

            if (it == g_nodePairStats.end() || it->second.packetCount == 0) {
                // No real traffic between this pair in this window - skip
                continue;
            }

            NodeStats& stats = it->second;

            double avgRssi     = stats.rssiSum     / stats.packetCount;
            double avgSnr      = stats.snrSum      / stats.packetCount;
            double avgDistance = stats.distanceSum / stats.packetCount;
            uint64_t packetCount = stats.packetCount;

            // Reset stats so next interval is a fresh window
            stats = NodeStats{};

            g_snrWeatherOut
                << seed        << ","
                << runNumber   << ","
                << routingsize << ","
                << now         << ","
                << txNodeId    << ","
                << rxNodeId    << ","
                << avgDistance << ","
                << txPowerDbm  << ","
                << avgRssi     << ","
                << avgSnr      << ","
                << packetCount << ","
                << avgRain     << ","
                << avgFog      << ","
                << avgSnow     << ","
                << avgWind     << ","
                << avgHumidity << "\n";
        }
    }

    // Update the last window end for next call
    g_lastSnrWeatherLogTime = now;

    // Schedule next interval (10 s window)
    Simulator::Schedule(Seconds(10.0),
                        &LogWeatherMetricsForAllNodeswithSNR,
                        filename, weatherModel, std::ref(nodeDataset), std::ref(allNodes));
}
