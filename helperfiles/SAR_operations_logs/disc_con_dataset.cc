

struct DiscoveryConvergenceEvent
{
    uint32_t eventId;                // Primary key
    uint32_t civilianId;             // Foreign key from Node Info
    ns3::Ipv4Address civilianIp;     
   // uint32_t discoveryNodeId;        // Foreign key from Node Info
    //ns3::Ipv4Address discoveryIp;
    double discoveryTime;            // When the civilian was discovered (s)
    std::string signalType; 
    std::vector<uint32_t> discoveringNodeIds; // IDs of all SAR nodes discovering the civilian
    std::vector<ns3::Ipv4Address> discoveringNodeIps; // Add corresponding IPs
   // std::vector<double> discoveringDistances; // Distances of each SAR node from the civilian
    uint32_t numDiscoveringNodes;             // Total number of discovering nodes

    // Convergence-related fields
    std::vector<uint32_t> convergenceNodeIds; // IDs of the SAR nodes that responded
    ns3::Ipv4Address assignedNode;     
    double convergenceStartTime;              // When SAR nodes started converging
    double convergenceCompletionTime;         // When SAR nodes finished converging
    uint32_t nodesResponding;                 // Number of nodes that responded

    double avgDistanceCovered;                // Summation of distances traveled by responding SAR / respondingCount
    double avgConvergenceDistance;            // Average distance during convergence


    // Rescue-related fields
    double rescueStartTime;                   // When rescue starts
    double rescueEndTime;                     // When rescue completes
    double avgRescueDistance;                 // Average distance covered during rescue
    double totalTravelDistanceToBase;
    double  safetyLocation;

     bool isFinalised = false;
};

// A container to hold all discovery/convergence events
static std::vector<DiscoveryConvergenceEvent> g_discoveryConvergenceEvents;

// A map to track (civilianId) -> eventId, so if the same civilian is discovered or converged upon multiple times,
// we either update an existing event or create a new one, depending on your scenario logic.
static std::map<uint32_t, uint32_t> g_civilianToEventIdMap;

// A global counter to generate unique Discovery Event IDs
static uint32_t g_nextDiscoveryEventId = 1;
static std::map<uint32_t, uint32_t> g_civilianToDiscoveringNodesCount; // Civilian ID -> Count of discovering nodes
static std::map<uint32_t, std::set<ns3::Ipv4Address>> g_civilianToDiscoveringNodeIps; // Civilian ID -> Set of discovering node IPs
std::unordered_map<uint32_t, double> g_civilianLatestDistanceToBase; // Stores latest distance to base


/* ---------------------------------------------------------------------- */
/*  HELPER FUNCTIONS TO LOG DISCOVERY & CONVERGENCE                       */
/* ---------------------------------------------------------------------- */

// Create or update the record for a discovered civilian.
uint32_t RecordDiscoveryEvent(uint32_t civilianId,
                              ns3::Ipv4Address civilianIp,
                              const std::vector<uint32_t> &discoveringNodeIds, // IDs of SAR nodes discovering the civilian
                              const std::vector<ns3::Ipv4Address> &discoveringNodeIps, // Corresponding IPs
                              double discoveryTime,
                              const std::string &signalType
                              )
{
    // Check if this civilian already has an event in the map
    if (g_civilianToEventIdMap.find(civilianId) == g_civilianToEventIdMap.end())
    {
        // Create a new event record
        DiscoveryConvergenceEvent event;
        event.eventId = g_nextDiscoveryEventId++;
        event.civilianId = civilianId;
        event.civilianIp = civilianIp;
        event.discoveryTime = discoveryTime;
        event.signalType = signalType;
        event.numDiscoveringNodes = discoveringNodeIds.size();

        // Add discovering nodes and distances
        for (size_t i = 0; i < discoveringNodeIds.size(); i++)
        {
            event.discoveringNodeIds.push_back(discoveringNodeIds[i]);
            if (i < discoveringNodeIps.size()) // Check if IPs are available
            {
                event.discoveringNodeIps.push_back(discoveringNodeIps[i]);
            }
        }


        // Initialize convergence and rescue fields to default
        event.convergenceStartTime = 0.0;
        event.convergenceCompletionTime = 0.0;
        event.nodesResponding = 0;
        event.avgDistanceCovered = 0.0;        // Will be set in RecordConvergenceStart
        event.avgConvergenceDistance = 0.0;   // Will be set in RecordConvergenceComplete
        event.avgRescueDistance = 0.0;        // Will be set in RecordRescueComplete
        event.totalTravelDistanceToBase=0.0;
        event.safetyLocation = 0.0;           // Will be set in RecordRescueComplete
        event.rescueStartTime = 0.0;
        event.rescueEndTime = 0.0;

        // Add the new event to the dataset
        g_discoveryConvergenceEvents.push_back(event);

        // Track this civilian in the map
        g_civilianToEventIdMap[civilianId] = event.eventId;

        return event.eventId;
    }
    else
    {
        // If there's already an event for this civilian, update the existing record
        uint32_t eventId = g_civilianToEventIdMap[civilianId];
        for (auto &rec : g_discoveryConvergenceEvents)
        {
            if (rec.eventId == eventId)
            {
                // Add new discovering nodes and distances if not already logged
                for (size_t i = 0; i < discoveringNodeIds.size(); i++)
                {
                    auto it = std::find(rec.discoveringNodeIds.begin(), rec.discoveringNodeIds.end(), discoveringNodeIds[i]);
                    if (it == rec.discoveringNodeIds.end())
                    {
                        rec.discoveringNodeIds.push_back(discoveringNodeIds[i]);
                        if (i < discoveringNodeIps.size()) // Check if IPs are available
                        {
                            rec.discoveringNodeIps.push_back(discoveringNodeIps[i]);
                        }
                        rec.numDiscoveringNodes = rec.discoveringNodeIds.size(); // Update count
                    }
                }

                // Update discovery time if earlier
                if (rec.discoveryTime > discoveryTime)
                {
                    rec.discoveryTime = discoveryTime;
                }

                break;
            }
        }
        return eventId;
    }
}

// Mark the start of convergence: which SAR nodes are moving, etc.
void RecordConvergenceStart(uint32_t eventId,
                            const std::vector<uint32_t> &sarNodeIds,
                            const std::vector<ns3::Ipv4Address> &sarNodeIps,
                            double startTime,
                            double distanceToCivilian)
{
    for (auto &rec : g_discoveryConvergenceEvents)
    {
        if (rec.eventId == eventId)
        {
            rec.convergenceNodeIds = sarNodeIds;
            rec.assignedNode = sarNodeIps.empty() ? ns3::Ipv4Address() : sarNodeIps[0];
            rec.convergenceStartTime = startTime;
            rec.nodesResponding = sarNodeIds.size();

            // Log the distance between SAR node and civilian (avgDistanceCovered)
            rec.avgDistanceCovered = distanceToCivilian;

            break;
        }
    }
}

// Mark the completion of convergence: fill out the end time, distance, etc.
void RecordConvergenceComplete(
    uint32_t eventId,
    double completionTime,
    Ptr<Node> assignedSarNodePtr,
    Ptr<Node> civilianNodePtr,
    const std::vector<Vector>& trackedPositions // List of positions during convergence
) {
    for (auto& rec : g_discoveryConvergenceEvents) {
        if (rec.eventId == eventId) {
            // Set the completion time for convergence
            rec.convergenceCompletionTime = completionTime;

            // Ensure both the assigned SAR node and civilian node have mobility models
            Ptr<MobilityModel> sarMobility = assignedSarNodePtr->GetObject<MobilityModel>();
            Ptr<MobilityModel> civilianMobility = civilianNodePtr->GetObject<MobilityModel>();

            if (!sarMobility || !civilianMobility) {
                NS_LOG_ERROR("Mobility model not found for SAR or civilian node!");
                rec.avgConvergenceDistance = 0.0; // Default to zero if mobility models are missing
                return;
            }

            // Calculate the total distance traveled by the assigned SAR node
            double totalDistance = 0.0;

            // Iterate through the tracked positions to compute the traveled path
            for (size_t i = 1; i < trackedPositions.size(); ++i) {
                totalDistance += ns3::CalculateDistance(trackedPositions[i - 1], trackedPositions[i]);
            }

        
            // Calculate the final distance between the SAR node and civilian at attachment
            Vector sarFinalPosition = sarMobility->GetPosition();
            Vector civilianFinalPosition = civilianMobility->GetPosition();
            double finalDistance = ns3::CalculateDistance(sarFinalPosition, civilianFinalPosition);

            // Ensure final attachment distance is reflected correctly
            rec.avgConvergenceDistance = totalDistance + finalDistance;

            // Log completion information (optional)
            //NS_LOG_INFO("Convergence completed for Event ID: " << eventId 
                        //  << " | Total Distance: " << rec.avgConvergenceDistance 
                        //  << " | Final Distance: " << rec.avgConvergenceDistance);

            break;
        }
    }
}


// Mark the start of rescue phase
void RecordRescueStart(uint32_t eventId,
                       const std::vector<uint32_t> &sarNodeIds,
                       double startTime)
{
    for (auto &rec : g_discoveryConvergenceEvents)
    {
        if (rec.eventId == eventId)
        {
            rec.rescueStartTime = startTime;
            break;
        }
    }
}


void RecordRescueComplete( 
    uint32_t eventId, 
    double completionTime, 
    Ptr<Node> rescuerNode, 
    Ptr<Node> baseNode, 
    Ptr<Node> civilianNode, 
    const std::vector<Vector>& sarPreviousPositions
) {
    for (auto &rec : g_discoveryConvergenceEvents) {
        if (rec.eventId == eventId) {
            rec.rescueEndTime = completionTime;

            Ptr<MobilityModel> rescuerMobility = rescuerNode->GetObject<MobilityModel>();
            Ptr<MobilityModel> baseMobility = baseNode->GetObject<MobilityModel>();
            Ptr<MobilityModel> civilianMobility = civilianNode->GetObject<MobilityModel>();

            if (rescuerMobility && baseMobility && civilianMobility) {
                Vector rescuerPosition = rescuerMobility->GetPosition();
                Vector basePosition = baseMobility->GetPosition();
                Vector civilianPosition = civilianMobility->GetPosition();

                //  FIX: Ensure we calculate distance from **first recorded position**
                double totalDistance = 0.0;
                if (!sarPreviousPositions.empty()) {
                    //  FIX: Start rescue distance from the very first recorded SAR position
                    totalDistance += ns3::CalculateDistance(sarPreviousPositions[0], rescuerPosition);

                    for (size_t i = 1; i < sarPreviousPositions.size(); ++i) {
                        totalDistance += ns3::CalculateDistance(sarPreviousPositions[i - 1], sarPreviousPositions[i]);
                    }
                }

                rec.totalTravelDistanceToBase = totalDistance;

                rec.avgRescueDistance = totalDistance / (double)sarPreviousPositions.size();

                rec.safetyLocation = ns3::CalculateDistance(civilianPosition, basePosition);

            } else {
                NS_LOG_ERROR("Mobility model not found for rescuer, civilian, or base station!");
                rec.avgRescueDistance = 0.0;
                rec.totalTravelDistanceToBase = 0.0;
                rec.safetyLocation = 0.0;
            }
            break;
        }
    }
}


/*  WRITING DISCOVERY & CONVERGENCE DATASET TO CSV                       */


void WriteDiscoveryConvergenceDataset(const std::string &filename)
{
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open())
    {
        // std::cerr << "Error: Could not open discovery/convergence dataset file: "
        //           << filename << std::endl;
        return;
    }

   
    // Write the CSV headers
    file << "seed,"
    << "runtime,"
    << "routing,"
    << "DiscoveryEventID,"
         << "CivilianID,"
         << "CivilianIP,"
         << "DiscoveryNodeIDs,"
         << "DiscoveryIPs,"
         << "NumDiscoveringNodes,"
         << "DiscoveryTime(s),"
         << "DiscoveryType,"
         << "ConvergenceNodeIDs,"
         << "AssignedNodeIP,"
         << "RescuerCivilianDistanceatassignment(m) ,"
          << "ConvergenceStartTime(s),"
         << "ConvergenceCompletionTime(s),"
         << "NodesResponding,"
        << "TotalConvergenceDistance(m),"

         << "RescueStartTime(s),"
         << "RescueEndTime(s),"
         << "TotalRescueDistance(m),"
         <<"SARTOBASE,"
         << "cIVILIANTOBASE,"
         << "\n";

    // Write one row per event
auto it = g_discoveryConvergenceEvents.begin();
while (it != g_discoveryConvergenceEvents.end())
{
    auto &rec = *it;

       // ---------- Discovering Node IDs ----------
    std::ostringstream discoveringNodeIdsStr;
    discoveringNodeIdsStr << "[";
    for (size_t i = 0; i < rec.discoveringNodeIds.size(); ++i)
    {
        discoveringNodeIdsStr << rec.discoveringNodeIds[i];
        if (i + 1 < rec.discoveringNodeIds.size())
        {
            discoveringNodeIdsStr << ";";
        }
    }
    discoveringNodeIdsStr << "]";

    // ---------- Discovering Node IPs (unique) ----------
    std::set<ns3::Ipv4Address> uniqueDiscoveringNodeIps(
        rec.discoveringNodeIps.begin(), rec.discoveringNodeIps.end());

    std::ostringstream discoveringNodeIpsStr;
    discoveringNodeIpsStr << "[";
    for (auto ipIt = uniqueDiscoveringNodeIps.begin();
         ipIt != uniqueDiscoveringNodeIps.end();
         ++ipIt)
    {
        discoveringNodeIpsStr << *ipIt;
        if (std::next(ipIt) != uniqueDiscoveringNodeIps.end())
        {
            discoveringNodeIpsStr << ";";
        }
    }
    discoveringNodeIpsStr << "]";

    // ---------- Convergence Node IDs ----------
    std::ostringstream convNodeIdsStr;
    convNodeIdsStr << "[";
    for (size_t i = 0; i < rec.convergenceNodeIds.size(); ++i)
    {
        convNodeIdsStr << rec.convergenceNodeIds[i];
        if (i + 1 < rec.convergenceNodeIds.size())
        {
            convNodeIdsStr << ";";
        }
    }
    convNodeIdsStr << "]";

    // ---------- WRITE ROW ----------
    file << seed << ","
         << runNumber << ","
         << routingsize << ","
         << rec.eventId << ","                 // Discovery Event ID
         << rec.civilianId << ","              // Civilian ID
         << rec.civilianIp << ","              // Civilian IP
      << "\"" << discoveringNodeIdsStr.str() << "\"" << ","// Discovering Node IDs
        << "\"" << discoveringNodeIpsStr.str() << "\"" << "," // Discovering Node IPs (unique)
          << rec.numDiscoveringNodes << ","     // Number of discovering nodes
         << rec.discoveryTime << ","           // Discovery time
         << rec.signalType << ","              // Signal type
          << "\"" << convNodeIdsStr.str()  << "\"" << ","       // Convergence node IDs
         << rec.assignedNode << ","            // Assigned node
         << rec.avgDistanceCovered << ","      // Avg distance covered
         << rec.convergenceStartTime << ","
         << rec.convergenceCompletionTime << ","
         << rec.nodesResponding << ","
         << rec.avgConvergenceDistance << ","
         << rec.rescueStartTime << ","
         << rec.rescueEndTime << ","
         << rec.totalTravelDistanceToBase << ","
         << rec.avgRescueDistance << ","
         << rec.safetyLocation << ","
         << "\n";

    it = g_discoveryConvergenceEvents.erase(it);
}


file.close();
    std::cout << "Discovery & Convergence dataset written to " << filename << std::endl;
}

size_t GetDiscConDebugSize()
{
    return g_discoveryConvergenceEvents.size();
}
