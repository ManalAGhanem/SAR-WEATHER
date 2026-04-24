
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


