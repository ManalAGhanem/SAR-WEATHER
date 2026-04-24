
//----------------------------communication dataset---------------------------------------------
// Define a custom tag for communication ID
class CommunicationIdTag : public Tag {
public:
    CommunicationIdTag() : m_communicationId(0) {}
    CommunicationIdTag(uint32_t commId) : m_communicationId(commId) {}

    void SetCommunicationId(uint32_t commId) { m_communicationId = commId; }
    uint32_t GetCommunicationId() const { return m_communicationId; }

    // Required implementation of pure virtual functions
    void Serialize(TagBuffer i) const override {
        i.WriteU32(m_communicationId);
    }

    void Deserialize(TagBuffer i) override {
        m_communicationId = i.ReadU32();
    }

    uint32_t GetSerializedSize() const override {
        return sizeof(uint32_t);
    }

    void Print(std::ostream &os) const override {
        os << "CommunicationId=" << m_communicationId;
    }

    static TypeId GetTypeId(void) {
        static TypeId tid = TypeId("CommunicationIdTag")
            .SetParent<Tag>() // Inherit from the Tag class
            .AddConstructor<CommunicationIdTag>();
        return tid;
    }

    TypeId GetInstanceTypeId(void) const override {
        return GetTypeId();
    }

private:
    uint32_t m_communicationId; // Communication ID
};
namespace std {
    template <>
    struct hash<ns3::Ipv4Address> {
        std::size_t operator()(const ns3::Ipv4Address& address) const noexcept {
            return std::hash<uint32_t>()(address.Get());
        }
    };
}



std::unordered_map<Ipv4Address, uint32_t> ipToCommIdMap; // Map of IP addresses to Communication IDs

// Define the CommunicationSession structure
struct CommunicationSession {
    uint32_t communicationId;        // Unique Communication ID
    Ipv4Address source;              // Source IP Address
    Ipv4Address destination;         // Destination IP Address
    std::string type;                // Communication type (e.g., Voice, Video)
    double startTime;                // Session start time
    double endTime;                  // Session end time
    std::string protocol = "UDP";    // Protocol Used
    uint16_t port;                  // 
    uint32_t totalPacketsSent = 0;   // Total packets sent
    uint32_t totalPacketsReceived = 0; // Total packets received
    uint32_t totalBytes = 0;         // Total bytes transmitted
    double totalDelay = 0.0;         // Total packet delay
    uint32_t totalBroadcastFanout; // Add this member variable

};

// Map to track active sessions
std::unordered_map<uint32_t, CommunicationSession> activeSessions;

// Log file for communication dataset
std::ofstream communicationLogFile("communication_details_logfinalv3.csv", std::ios::out);

void InitializeCommunicationLogFile() {
    communicationLogFile << "Communication ID,Source IP,Destination IP,Communication Type,"
                         << "Start Time (s),End Time (s),Protocol,port,Total Packets Sent,Total Packets Received,"
                         << "Packet Loss Rate (%),Total Bytes (bytes),Average Delay (ms),Throughput (bps)\n";
    communicationLogFile.flush();
}

void StartCommunicationSession(uint32_t communicationId, Ipv4Address source, Ipv4Address destination,
                               const std::string& type, double startTime,
                               const std::string& protocol, uint16_t port) {
    if (activeSessions.find(communicationId) != activeSessions.end()) {
      //  std::cerr << "Warning: Communication session " << communicationId << " already exists." << std::endl;
        return;
    }

    // Initialize the communication session
    CommunicationSession session = {
        communicationId,
        source,
        destination,
        type,
        startTime,
        0.0,    // End time
        protocol,
        port,
        0,      // Total packets sent
        0,      // Total packets received
        0,      // Total bytes
        0.0,    // Total delay
        0       // Total broadcast fanout
    };

    // Map the source IP to the Communication ID
    ipToCommIdMap[source] = communicationId;

    // Map the destination IP to the Communication ID
    if (destination == Ipv4Address("255.255.255.255")) {
        ipToCommIdMap[destination] = communicationId;

        // If the destination is a broadcast address, find all nodes listening on the specified port
        std::vector<Ipv4Address> receivingNodes;

        for (const auto &entry : activeSessions) {
            const CommunicationSession &existingSession = entry.second;

            // Check if the session port matches
            if (existingSession.port == port) {
                receivingNodes.push_back(existingSession.source); // Add the source IP of the session as a receiving node
                ipToCommIdMap[existingSession.source] = communicationId;

                // Increment broadcast fanout counter
                session.totalBroadcastFanout++;
            }
        }

        if (!receivingNodes.empty()) {
           // std::cout << "Broadcast destination processed. Actual receiving nodes for Communication ID " << communicationId << ": ";
            for (const auto &node : receivingNodes) {
                std::cout << node << " ";
            }
            std::cout << std::endl;
        } else {
           // std::cerr << "Warning: No receiving nodes found for broadcast on port " << port << std::endl;
        }
    } else {
        // Unicast: Map destination IP
        ipToCommIdMap[destination] = communicationId;
    }

    // Add the session to active sessions
    activeSessions[communicationId] = session;

    // Logging the start of the session
//     std::cout << "Started communication session: " << communicationId
//               << " (Source: " << source
//               << ", Destination: " << destination
//               << ", Port: " << port
//               << ", Type: " << type
//               << ", Protocol: " << protocol
//               << ")" << std::endl;
// 
}

void LogPacketEvent(
    Ptr<Socket> socket,
    Ipv4Address sourceAddress,
    Ipv4Address destinationAddress,
    uint32_t packetId,
    double delay,
    double throughput,
    uint32_t packetSize,
    uint16_t packetPort,               // Added port to function parameters
    std::string packetType = "",
    uint32_t communicationId = 0,
    const std::string& direction= ""
) {
    // Check if the communication session exists
    if (activeSessions.find(communicationId) != activeSessions.end()) {
        CommunicationSession &session = activeSessions[communicationId];

        // Ensure the packet's port matches the session's port
        if (session.port != packetPort) {
            // Debug log if port mismatch occurs
            // std::cerr << "Warning: Packet port " << packetPort
            //           << " does not match session port " << session.port
            //           << " for Communication ID " << communicationId << "\n";
            return;
        }

        // Update session metrics
        if (destinationAddress == Ipv4Address("255.255.255.255")) {
            // Special handling for broadcast packets
            session.totalPacketsSent++;
            session.totalBytes += packetSize;
            session.totalDelay += delay;

            // Increment totalBroadcastFanout for each node in the network
            for (const auto &node : ipToCommIdMap) {
                if (node.second == communicationId) {
                    CommunicationSession &receiverSession = activeSessions[node.second];

                    if (receiverSession.port == packetPort) { // Check port match
                        session.totalBroadcastFanout++;
                        session.totalPacketsReceived++;
                    }
                }
            }
        } else if (destinationAddress == session.destination || destinationAddress == Ipv4Address::GetAny()) {
            // Handle unicast packets or packets directed to the session's destination
            session.totalPacketsSent++;
            session.totalBytes += packetSize;
            session.totalDelay += delay;

            if (destinationAddress != Ipv4Address::GetAny()) {
                session.totalPacketsReceived++;
            }
        }

        // Log packet information to a file
        packetLogFile <<seed<<","
                      <<runNumber<<","
                      <<routingsize<<","
                      << Simulator::Now().GetSeconds() << ","  // Current simulation time
                      << communicationId << ","               // Communication ID
                      << packetId << ","                      // Packet ID
                      << packetPort << ","                   // Packet Port
                      << sourceAddress << ","                 // Source IP
                      << destinationAddress << ","            // Destination IP
                      << packetType << ","                    // Packet Type (e.g., Voice, Video, Environmental)
                      << delay   << ","                      // Delay
                      << direction   << ","  
                      <<packetSize<< "\n"; 
        packetLogFile.flush();

        // Debug log
        // std::cout << "Packet logged: Communication ID " << communicationId
        //           << ", Packet ID " << packetId
        //           << ", Source " << sourceAddress
        //           << ", Destination " << destinationAddress
        //           << ", Packet Type " << packetType
        //           << ", Port " << packetPort
        //           << ", Delay ms " << delay << " s\n";
    } else {
        // Debug log if the session does not exist
        // std::cerr << "Warning: Communication session " << communicationId
        //           << " not found. Packet not logged.\n";
    }
}

void FinalizeCommunicationSession(uint32_t communicationId, double endTime) {
    // Check if the session exists
    if (activeSessions.find(communicationId) == activeSessions.end()) {
        std::cerr << "Warning: Communication session " << communicationId << " does not exist or has already been finalized." << std::endl;
        return; // Avoid invalid access
    }


    // Retrieve the session
    CommunicationSession &session = activeSessions[communicationId];
    session.endTime = endTime;

    // Calculate aggregate metrics
    double duration = session.endTime - session.startTime;
    double packetLossRate = session.totalPacketsSent > 0
        ? (100.0 * (session.totalPacketsSent - session.totalPacketsReceived) / session.totalPacketsSent)
        : 0.0;
    double averageDelay = session.totalPacketsReceived > 0
        ? (session.totalDelay / session.totalPacketsReceived)
        : 0.0;
    double throughput = (session.totalBytes * 8.0) / (duration > 0 ? duration : 1); // Prevent divide-by-zero

    // Log the session to the file
    communicationLogFile << session.communicationId << ","
                         << session.source << ","
                         << session.destination << ","
                         << session.type << ","
                         << session.startTime << ","
                         << session.endTime << ","
                         <<session.protocol<<","
                         << session.port<<","
                         << session.totalPacketsSent << ","
                         << session.totalPacketsReceived << ","
                         << packetLossRate << ","
                         << session.totalBytes << ","
                         << averageDelay * 1000 << "," // Convert delay to ms
                         << throughput << "\n";
    communicationLogFile.flush();
 
    
    // Remove associated IP mappings from ipToCommIdMap
    for (auto it = ipToCommIdMap.begin(); it != ipToCommIdMap.end();) {
        if (it->second == communicationId) {
            std::cout << "Removing IP-to-Communication mapping for IP: " << it->first << " (Communication ID: " << communicationId << ")" << std::endl;
            it = ipToCommIdMap.erase(it); // Remove the entry and advance the iterator
        } else {
            ++it; // Advance the iterator
        }
    }

    // Remove session from active tracking
    activeSessions.erase(communicationId);
    std::cerr << "Communication session " << communicationId << " finalized and removed from active sessions." << std::endl;


}

void FinalizeAllCommunicationSessions() {
    if (activeSessions.empty()) {
        std::cerr << "No active sessions to finalize." << std::endl;
        return; // Safeguard against empty map
    }

    std::cerr << "Finalizing all communication sessions. Total sessions: " << activeSessions.size() << std::endl;

    // Create a copy of the keys to avoid iterator invalidation
    std::vector<uint32_t> sessionIds;
    for (const auto &entry : activeSessions) {
        sessionIds.push_back(entry.first);
    }

    // Finalize each session
    for (uint32_t communicationId : sessionIds) {
        std::cerr << "Finalizing session: " << communicationId << std::endl;
        FinalizeCommunicationSession(communicationId, Simulator::Now().GetSeconds());
    }

    std::cerr << "All sessions finalized and cleared." << std::endl;
}



//-------------------------------------end communication dataset----------------------------------
