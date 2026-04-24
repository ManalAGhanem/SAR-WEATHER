// ========================= routing_telemetry.h =========================
// ns-3 routing telemetry (declaration): protocol-aware control overhead + route snapshots



#ifndef SARTEL_ROUTING_TELEMETRY4_H
#define SARTEL_ROUTING_TELEMETRY4_H
#include <unordered_map>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include "ns3/ipv4-address.h"
//#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv4-l3-protocol.h"



#include "ns3/core-module.h"   // Time, Ptr, OutputStreamWrapper, Simulator

namespace ns3 { class Packet; class Ipv4; class Ipv4Header; class OutputStreamWrapper;  class Ipv4Address;   }

namespace sartelemetry {
using namespace ns3;

struct RoutingTelemetryConfig {
  uint32_t seed{0};
  uint32_t run{0};
  std::string routing{""};
  std::string filePrefix{"telemetry"};
  Time snapshotPeriod{Seconds(0)};  // 0 disables snapshots
};

//static Ptr<OutputStreamWrapper> m_hopCsv;
//static std::unordered_map<uint64_t, uint8_t> m_initTtl;
enum RtProto : uint8_t { RT_UNKNOWN=0, RT_OLSR=1, RT_AODV=2, RT_DSDV=3, RT_DSR=4 };

// struct RtKey {  // key for per-node, per-protocol, per-msg aggregation
//   uint32_t node{0}; uint8_t proto{0}; uint16_t type{0};
//   bool operator<(const RtKey& o) const { return std::tie(node,proto,type) < std::tie(o.node,o.proto,o.type); }
// };

class RoutingTelemetry {
public:

  struct DebugStats
  {
    size_t ttlCache = 0;
    size_t streamKeys = 0;
    size_t pktKeys = 0;
    size_t byteKeys = 0;
  };

  static DebugStats GetDebugStats();

  static void Start(const RoutingTelemetryConfig& cfg);
  static void Stop();



  // Identification + extra fields for routing control messages
  struct RtId
  {
    RtProto     proto{RT_UNKNOWN};  // which routing protocol
    uint16_t    type{0};            // protocol-specific message type (e.g. AODVTYPE_RREQ)
    uint16_t    headerLen{0};       // length of routing header (bytes)

    // Extra AODV-style metadata (when available, else 0 / "any")
    uint32_t    msgSeq{0};          // message sequence (RREQ ID etc.)
    uint32_t    dstSeq{0};          // destination sequence number
    ns3::Ipv4Address originator;    // originator address (if known)
  };

  struct RtKey
  {
    uint32_t node;
    uint8_t  proto;
    uint16_t type;

    bool operator<(const RtKey &other) const
    {
      if (node  != other.node)  return node  < other.node;
      if (proto != other.proto) return proto < other.proto;
      return type < other.type;
    }
  };
  

//  static bool Classify(Ptr<const Packet> original, const Ipv4Header& ip, RtId& out, uint16_t& bytes);
static bool Classify(Ptr<const Packet> original,
                                const Ipv4Header& ip,
                                RtId& out,
                                uint16_t& bytes);

static void LogCtrl(uint32_t nodeId,
                    const char* dir,
                    const RtId& id,
                    uint16_t bytes,
                    const ns3::Ipv4Header& ip,
                    uint32_t ipPktBytes,
                    uint32_t ipPayloadBytes,
                    uint32_t udpPayloadBytes);

                      
// static void LogCtrl(uint32_t nodeId,
//                            const char *dir,
//                            const RtId &id,
//                            uint16_t bytes,
//                            const Ipv4Header &ip);
// TX trace
// static void OnIpv4Tx(
//     Ptr<const Packet> packet,
//     Ptr<Ipv4> ipv4,
//     uint32_t ifIndex,
//     uint32_t nodeId
// );

// // RX trace (LocalDeliver)
// static void OnIpv4Rx(
//     const Ipv4Header& ip,
//     Ptr<const Packet> packet,
//     uint32_t ifIndex,
//     uint32_t nodeId
// );

static void OnIpv4Tx(uint32_t nodeId,
                                Ptr<const Packet> p,
                                Ptr<Ipv4> /*ipv4*/,
                                uint32_t /*ifIndex*/);

static void OnIpv4Rx(uint32_t nodeId,
                                const Ipv4Header& ip,
                                Ptr<const Packet> p,
                                uint32_t /*ifIndex*/);

 static void OnIpv4Drop(uint32_t nodeId,
                       const ns3::Ipv4Header& ip,
                       ns3::Ptr<const ns3::Packet> p,
                       ns3::Ipv4L3Protocol::DropReason reason,
                       ns3::Ptr<ns3::Ipv4> ipv4,
                       uint32_t ifIndex);
                               

  // Periodic snapshots of routing tables
  static void DoSnapshot();

  // State
  static bool m_started;
  static RoutingTelemetryConfig m_cfg;
  static Ptr<OutputStreamWrapper> m_ctrlLog, m_sumLog, m_snapCsv, m_snapTxt;


  static Ptr<OutputStreamWrapper> m_stateCsv;
  static ns3::Ptr<ns3::OutputStreamWrapper> m_dropCsv;


      // Per-packet hop CSV and TTL map
    static ns3::Ptr<ns3::OutputStreamWrapper> m_hopCsv;
    struct TtlRec
    {
      uint8_t ttl;
      ns3::Time firstSeen;
    };

    static std::unordered_map<uint64_t, TtlRec> m_initTtl;

    // Stream index key = (node, proto, msgType, dest IP)
    using StreamKey = std::tuple<uint32_t, uint8_t, uint16_t, ns3::Ipv4Address>;
    static std::map<StreamKey, uint32_t> m_streamIndex;

    static std::map<RtKey, uint64_t> m_pktCount;
    static std::map<RtKey, uint64_t> m_byteCount;


};

// ---------- Convenience helpers (declarations) ----------


// Builds results/<scenario>/<routing>/seed-<seed>/<scenario>_<routing>_seed<seed>_run<run>
std::string BuildPrefix(const std::string& scenarioId,
                        const std::string& routingName,
                        uint32_t seed,
                        uint32_t runNumber,
                        const std::string& base = "results");

// Convenience: build prefix and start telemetry
void StartForRun(const std::string& scenarioId,
                 const std::string& routingName,
                 uint32_t seed,
                 uint32_t runNumber,
                 double snapshotSeconds = 2.0);


} // namespace sartelemetry

#endif // SARTEL_ROUTING_TELEMETRY_H
