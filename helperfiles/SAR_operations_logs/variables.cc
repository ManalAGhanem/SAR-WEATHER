
Ptr<WeatherManager> weather;
Ptr<WeatherAttenuationModel> weatherLoss ;
static std::ofstream g_snrWeatherOut;
// static bool g_snrWeatherHeaderWritten = false;
// static double g_lastSnrWeatherLogTime = 0.0;

// ---- Discovery ACK / Retry State ----
static std::map<std::pair<uint32_t,uint32_t>, bool> g_discoveryAcked;
// key = (sarId, civilianId)

static std::map<std::pair<uint32_t,uint32_t>, uint32_t> g_discoveryRetries;

static const uint32_t MAX_DISCOVERY_RETRIES = 3;
static const double DISCOVERY_ACK_TIMEOUT = 2.0; // seconds

uint32_t seed = ns3::RngSeedManager::GetSeed(); 
uint32_t runNumber = 1;        // Default to 1; overwritten by CLI
std::string scenarioId ;   
std::string routingsize;
std::string routing;

static std::unordered_set<uint32_t> assignedSarNodes;
std::map<uint32_t, bool> g_civilianAttachedState; // Civilian ID -> Attached state
// Store the initial offset between SAR & Civilian (so it remains fixed)
static std::unordered_map<uint32_t, Vector> sarToCivilianOffset;
bool g_activeFound = false;            // True if an active discovery (WiFi/PLB/Query) occurred recently
Time g_lastActiveTime = Seconds(0.0);  // Timestamp of the last active discovery
double g_noActiveTimeout = 5.0;        // If no active discovery in 5 seconds, run passive checks

double convergenceThresholdDistance = 12.0;  // Define the threshold distance for convergence
double sarProxmitytoBase = 22.0;  // Define the threshold distance for convergence
double civilianProxmitytoBase = 13.0;  // Define the threshold distance for convergence

NodeContainer sarNodes, allSarNodes,groundNodes,civilianNodes,civilianNodesquery,allNodes;
 
//std::ofstream adjacencyLogFile("adjacency_log.csv");
static std::vector<std::string> energyBuffer;
static std::vector<std::string> phyRxBuffer;


Ptr<Socket> baseBroadcastSocket;
Ptr<Socket> baseStationSocket;
uint16_t broadcastPort = 8000;
uint16_t convergencePort = 9000;      // For convergence updates, rescuers moving towards civilina to base updates
uint16_t rescuemovmentport = 8200;// for rescuers to update movment status to base
uint16_t voicePort = 6000;// voice port
uint16_t CivilianSafePort = 8300;// for broadcast that civiliian is safe from base to all
uint16_t assignmentPort = 8100;       // For rescue assignments
uint16_t discoveryPort = 7000;          // for discovering all Civilians by the 4 methods and sending to base
uint16_t discoveryAckPort = 7101;
uint16_t sarDiscoveryPort=7002;         // sar to sar discovery announcemnet
uint16_t discoverybasePort=7001;        // sending discovery to
uint16_t controlMsgPort= 8081;
uint16_t PLBPort = 9100;   
uint16_t WificivilianPort= 9200;
uint16_t QueryResponsePort= 9300;
uint16_t SARcivilianqueryport= 9400;
uint16_t envPort = 6001;
uint16_t videoPort = 5000;  // Port for video feed
uint16_t gpsPort = 10;  // Use port 10 for GPS updates
uint32_t comm=0;


static std::unordered_set<uint32_t> pendingCivilians;

// Track link start and end times
// std::map<std::pair<uint32_t, uint32_t>, double> linkStartTime;
// std::map<std::pair<uint32_t, uint32_t>, double> linkEndTime;
// std::vector<double> linkDurations;

// We keep a record for each node: total speed (m/s) and how many times we sampled it
static std::map<uint32_t, double> g_nodeSpeedSum; 
static std::map<uint32_t, uint64_t> g_nodeSpeedCount;

// For SAR-to-base discovery message
Ptr<Node> g_baseNode;
std::vector<Ptr<Node>> g_sarNodes;
std::map<uint32_t, uint32_t> g_civilianToSarNodeMap; // SAR Node ID -> Civilian ID

std::ofstream movementLogFile; // Global movement log file
std::ofstream rescueCompletionLogFile; // Global rescue completion log file
// Global file stream for received assignments log
std::ofstream receivedAssignmentsLogFile;
std::ofstream convergenceLogFile;
std::ofstream positionLogFile; // Open file in append mode
std::ofstream pendingAssignmentCsv;
// File stream for logging the communication data
std::ofstream gpsLogFile;
std::ofstream signalLogFile;
std::ofstream connectionLogFile;
std::ofstream videoLogFile;       // For video communication logs
std::ofstream packetLogFile; 
std::ofstream voiceLogFile("voice_communication_logfinalv3.csv"); // Voice communication log
std::map<uint32_t, double> sendTimestamps;  // Global map to track send times
std::ofstream envLogFile("environmental_data_logfinalv3.csv"); 
std::ofstream discoveryLogFile; // Log file for civilian discovery events
// Global discovery state to track which SAR nodes have discovered which civilians
std::unordered_map<uint32_t, std::unordered_map<uint32_t, bool>> discoveryState;
//Keep track of last-logged positions to avoid repeated logs 
// for a (SAR, Civilian) pair that hasn't moved.
static std::map<std::pair<uint32_t, uint32_t>, Vector> lastLoggedPosition;
// Local discovery log file for each SAR node and centralized log at the base station
std::ofstream centralDiscoveryLogFile;
std::ofstream sarDiscoveryBroadcastLogFile;
std::unordered_map<uint32_t, Vector> lastKnownPositions; // To track positions of nodes
// Global variables
std::unordered_map<uint32_t, uint32_t> rescueAssignments; // Tracks Civilian ID -> Assigned SAR Node ID
std::ofstream rescueAssignmentLogFile; // Rescue assignment log file
std::ofstream attachmentLogFile;
std::ofstream macTxRxLogFile;
static const double EPS = 1e-7; // or maybe 1e-6, 1e-9, etc.

std::ofstream  detailedEnergyLog("energyLogfinalv3.csv");
std::ofstream hopLogFile;
std::ofstream routingLogFile;
// logs for civilian send/recieve activities to check
std::ofstream systemLogFile;
// Session for the communication IDs in the communication dataset for the voice sessions
std::map<std::pair<uint32_t, uint32_t>, uint32_t> sessionTable; // Key: (Sender, Receiver), Value: voiceCommId
uint32_t voiceCommunicationId = 5000; // Start communication IDs from 5000
uint32_t sartosarCommunicationId=6000;
uint32_t nextBroadcastCommId = 60000; // Start Comm IDs for broadcast sessions




// Global map to track civilian safety status
std::map<uint32_t, bool> civilianSafetyStatus;


// Key: (SenderNodeId, ReceiverAddress, SignalType), Value: Communication ID
std::map<std::tuple<uint32_t, Ipv4Address, Ipv4Address, std::string>, uint32_t> sessionsignalTable;
uint32_t nextCommId = 3000; // Start PLB/WiFi communication IDs

    // Static variable to track positions during convergence for each SAR node
static std::map<uint32_t, std::vector<Vector>> trackedConvergencePositions;
static std::map<uint32_t, std::vector<Vector>> sarPreviousPositions;


static double scenarioStartTime;


// // A container to hold all discovery/convergence events
// static std::vector<DiscoveryConvergenceEvent> g_discoveryConvergenceEvents;

// // A map to track (civilianId) -> eventId, so if the same civilian is discovered or converged upon multiple times,
// // we either update an existing event or create a new one, depending on your scenario logic.
// static std::map<uint32_t, uint32_t> g_civilianToEventIdMap;

// // A global counter to generate unique Discovery Event IDs
// static uint32_t g_nextDiscoveryEventId = 1;
// static std::map<uint32_t, uint32_t> g_civilianToDiscoveringNodesCount; // Civilian ID -> Count of discovering nodes
// static std::map<uint32_t, std::set<ns3::Ipv4Address>> g_civilianToDiscoveringNodeIps; // Civilian ID -> Set of discovering node IPs
// std::unordered_map<uint32_t, double> g_civilianLatestDistanceToBase; // Stores latest distance to base
// double frequency = 2.4e9;  // Frequency in Hz (2.4 GHz)

// std::map<std::string, NodeInfo> nodeTypeAttributes; // Map to store attributes based on roles


// // Simulation time
// //double simulationTime =100.0;
// // Global map: nodeId -> NodeInfo
// static std::map<uint32_t, NodeInfo> nodeDataset;
// //


// // To track initial/final positions, packet counts, connectivity, etc.
// static std::unordered_map<uint32_t, bool> initialPosRecorded;
// static std::unordered_map<uint32_t, Vector> initialPositions;
// static std::unordered_map<uint32_t, Vector> finalPositions;

// // Track how long each node is "connected" to any other node
// static std::unordered_map<uint32_t, double> connectedTime;

// // Track maximum distance of successful reception for each node to estimate communication range
// static std::unordered_map<uint32_t, double> maxReceptionDistance;

// // Track packets sent and received for behavior assessment
// static std::unordered_map<uint32_t, uint32_t> packetSentCount;
// static std::unordered_map<uint32_t, uint32_t> packetReceivedCount;


// std::map<ns3::Ipv4Address, uint32_t> ipToNodeIdMap; // Global map

static std::ofstream g_debugDatasetLog;
