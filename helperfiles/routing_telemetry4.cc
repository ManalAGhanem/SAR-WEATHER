
// ========================= routing_telemetry.cc =========================
// ns-3 routing telemetry (implementation)
// Filesystem for creating results directories
#if __has_include(<filesystem>)
  #include <filesystem>
  namespace srt_fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
  #include <experimental/filesystem>
  namespace srt_fs = std::experimental::filesystem;
#else
  #define SRT_NO_FILESYSTEM 1
  #include <cstdlib>
#endif

namespace { // internal helper, no header needed
inline void EnsureDir(const std::string& path) {
#ifdef SRT_NO_FILESYSTEM
  std::string cmd = "mkdir -p \"" + path + "\"";
  std::system(cmd.c_str());
#else
  srt_fs::create_directories(path);
#endif
}
} // anonymous namespace

#include "routing_telemetry4.h"

#include <sstream>
#include <iomanip>
#include "ns3/output-stream-wrapper.h" 
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include <vector>
#include <algorithm>
#include <regex>
#include <string>
#include <cmath>

// Conditional protocol headers
#if __has_include("ns3/olsr-header.h")
#  include "ns3/olsr-header.h"
#  include "ns3/olsr-routing-protocol.h"
#  define SRT_HAVE_OLSR 1
#endif

#if __has_include("ns3/aodv-packet.h")
#  include "ns3/aodv-routing-protocol.h"
#  include "ns3/aodv-packet.h"
#  define SRT_HAVE_AODV 1
#endif

#if __has_include("ns3/dsdv-packet.h")
#  include "ns3/dsdv-routing-protocol.h"
#  include "ns3/dsdv-packet.h"
#  define SRT_HAVE_DSDV 1
#endif


// top of routing_telemetry.cc (after includes)
static const uint16_t OLSR_UDP_PORT = 698; // ns-3 olsr::RoutingProtocol::OLSR_PORT_NUMBER
static const uint16_t AODV_UDP_PORT = 654; // ns-3 aodv::RoutingProtocol::AODV_PORT
static const uint16_t DSDV_UDP_PORT = 269; // ns-3 dsdv::RoutingProtocol::DSDV_PORT

namespace sartelemetry {
using namespace ns3;
struct SizeInfo
{
  uint32_t ipPktBytes = 0;       // p->GetSize()
  uint32_t ipPayloadBytes = 0;   // after removing IPv4 header
  uint32_t udpPayloadBytes = 0;  // after removing UDP header (if UDP)
  bool     isUdp = false;
};

static inline SizeInfo ExtractSizes(ns3::Ptr<const ns3::Packet> p)
{
  using namespace ns3;
  SizeInfo s;
  if (!p) return s;

  s.ipPktBytes = p->GetSize();

  Ptr<Packet> copy = p->Copy();

  Ipv4Header ip;
  if (!copy->PeekHeader(ip))
  {
    return s; // not IPv4 packet in this trace (rare)
  }

  copy->RemoveHeader(ip);
  s.ipPayloadBytes = copy->GetSize();

  if (ip.GetProtocol() == 17) // UDP
  {
    UdpHeader uh;
    if (copy->PeekHeader(uh))
    {
      copy->RemoveHeader(uh);
      s.udpPayloadBytes = copy->GetSize();
      s.isUdp = true;
    }
  }

  return s;
}

// ---------------- Routing Table Entry Struct ----------------
struct RtEntry {
    std::string dst;
    std::string nextHop;
    int hops;
};


// static storage
bool RoutingTelemetry::m_started = false;
RoutingTelemetryConfig RoutingTelemetry::m_cfg{};
Ptr<OutputStreamWrapper> RoutingTelemetry::m_ctrlLog{nullptr};
Ptr<OutputStreamWrapper> RoutingTelemetry::m_sumLog{nullptr};
Ptr<OutputStreamWrapper> RoutingTelemetry::m_snapCsv{nullptr};
Ptr<OutputStreamWrapper> RoutingTelemetry::m_snapTxt{nullptr};
Ptr<OutputStreamWrapper> RoutingTelemetry::m_stateCsv{nullptr};
Ptr<OutputStreamWrapper> RoutingTelemetry::m_dropCsv{nullptr};


std::map<sartelemetry::RoutingTelemetry::RtKey, uint64_t>
    sartelemetry::RoutingTelemetry::m_pktCount;

std::map<sartelemetry::RoutingTelemetry::RtKey, uint64_t>
    sartelemetry::RoutingTelemetry::m_byteCount;

Ptr<OutputStreamWrapper> RoutingTelemetry::m_hopCsv{nullptr};
std::unordered_map<uint64_t, RoutingTelemetry::TtlRec> RoutingTelemetry::m_initTtl;
std::map<RoutingTelemetry::StreamKey, uint32_t> RoutingTelemetry::m_streamIndex;

static ns3::Time g_lastTtlEvict = ns3::Seconds(0);
static const ns3::Time TTL_KEEP = ns3::Seconds(8);      // keep TTL records for 8s
static const size_t MAX_TTL_CACHE = 20000;              // hard cap (tune 10k–100k)

static uint64_t g_ctrlDetected = 0;
static uint64_t g_ctrlLogged   = 0;
static uint64_t g_ctrlUdpPortHitButParseFail = 0;



static inline bool IsUdp(uint8_t proto) { return proto == 17; }
void RoutingTelemetry::Start(const RoutingTelemetryConfig& cfg) {
  // Guard: don't start twice

  #ifdef SRT_HAVE_AODV
  NS_LOG_UNCOND("RoutingTelemetry: SRT_HAVE_AODV=1");
#else
  NS_LOG_UNCOND("RoutingTelemetry: SRT_HAVE_AODV=0");
#endif

#ifdef SRT_HAVE_OLSR
  NS_LOG_UNCOND("RoutingTelemetry: SRT_HAVE_OLSR=1");
#else
  NS_LOG_UNCOND("RoutingTelemetry: SRT_HAVE_OLSR=0");
#endif

#ifdef SRT_HAVE_DSDV
  NS_LOG_UNCOND("RoutingTelemetry: SRT_HAVE_DSDV=1");
#else
  NS_LOG_UNCOND("RoutingTelemetry: SRT_HAVE_DSDV=0");
#endif


if (m_started)
{
  NS_LOG_UNCOND("RoutingTelemetry: Start() called twice — ignoring second call.");
  return;
}
m_started = true;

  // NS_ABORT_IF(m_started);
  // m_started = true;
  m_cfg = cfg;

  // Base filename prefix (you already built this with BuildPrefix(...))
  const std::string base = cfg.filePrefix;

  // ---- Open all outputs ----
  m_ctrlLog = Create<OutputStreamWrapper>(base + "_routing_control_log.csv", std::ios::out);
  m_sumLog  = Create<OutputStreamWrapper>(base + "_routing_overhead_summary.csv", std::ios::out);
  m_snapCsv = Create<OutputStreamWrapper>(base + "_route_table_snapshot.csv", std::ios::out);
  m_snapTxt = Create<OutputStreamWrapper>(base + "_route_table_text.log", std::ios::out);
  m_dropCsv = Create<OutputStreamWrapper>(base + "_ipv4_drops.csv", std::ios::out);



  m_stateCsv = Create<OutputStreamWrapper>(base + "_routing_state_log.csv", std::ios::out);
*m_stateCsv->GetStream()
    << "Seed,Run,Routing,Time_s,NodeId,"
    << "RoutingTableSize,"
    << "RoutingTableEntropy,"
    << "SameNextHopRatio\n";


  // NEW: per-packet hop counts (derived from IP TTL delta)
  m_hopCsv  = Create<OutputStreamWrapper>(base + "_packet_hops.csv", std::ios::out);

  // ---- Write CSV headers ----

    *m_ctrlLog->GetStream()
  << "Seed,Run,Routing,Time_s,NodeId,Direction,Protocol,MsgType,"
  << "IpPktBytes,IpPayloadBytes,UdpPayloadBytes,RoutingCtrlBytes,HeaderBytes,"

  << "Src,Dest,TransferMode,Land,MsgSeq,DstSeq,IsRerr,StreamIndex\n";


  *m_sumLog->GetStream()
      << "Seed,Run,Routing,NodeId,Protocol,MsgType,Pkts,Bytes\n";

  *m_snapCsv->GetStream()
      << "Seed,Run,Routing,Time_s,NodeId,Protocol,Destination,NextHop,HopCount,ValidUntil_s\n";

  // Header for the new hops file
  *m_hopCsv->GetStream()
      << "Seed,Run,Routing,Time_s,Uid,Src,Dest,InitTtl,RxTtl,Hops\n";


  *m_dropCsv->GetStream()
  << "Seed,Run,Routing,Time_s,NodeId,Reason,"
  << "IpPktBytes,IpPayloadBytes,UdpPayloadBytes,"
  << "Src,Dest\n";




  // ---- Hook IPv4 trace sources (Tx and LocalDeliver) on every node ----
  for (uint32_t i = 0; i < NodeList::GetNNodes(); ++i) {
    Ptr<Node> n   = NodeList::GetNode(i);
    Ptr<Ipv4L3Protocol> ipv4 = n->GetObject<Ipv4L3Protocol>();
    if (!ipv4) continue;

    // "Tx" signature: (Ptr<const Packet>, Ptr<Ipv4>, uint32_t)
    ipv4->TraceConnectWithoutContext("Tx",
        MakeBoundCallback(&RoutingTelemetry::OnIpv4Tx, i));

    // "LocalDeliver" signature: (const Ipv4Header&, Ptr<const Packet>, uint32_t)
    ipv4->TraceConnectWithoutContext("LocalDeliver",
        MakeBoundCallback(&RoutingTelemetry::OnIpv4Rx, i));


    ipv4->TraceConnectWithoutContext("Drop",
    MakeBoundCallback(&RoutingTelemetry::OnIpv4Drop, i));

  }

  // ---- Kick off periodic route-table snapshots (if enabled) ----
  if (m_cfg.snapshotPeriod > Seconds(0)) {
    // small delay so tables exist before first dump
    Simulator::Schedule(Seconds(0.1), &RoutingTelemetry::DoSnapshot);
  }
}

void RoutingTelemetry::Stop() {
  if (!m_started) return;

  NS_LOG_UNCOND("RoutingTelemetry ctrl stats: detected=" << g_ctrlDetected
                << " logged=" << g_ctrlLogged
                << " udpPortHitButParseFail=" << g_ctrlUdpPortHitButParseFail);

  for (const auto& kv : m_pktCount) {
    const RtKey& k = kv.first; uint64_t pkts = kv.second; uint64_t bytes = m_byteCount[k];
    *m_sumLog->GetStream() << m_cfg.seed << ',' << m_cfg.run << ',' << m_cfg.routing << ','
                           << k.node << ',' << (int)k.proto << ',' << k.type << ','
                           << pkts << ',' << bytes << '\n';
  }
  m_ctrlLog = nullptr;
  m_sumLog  = nullptr;
  m_snapCsv = nullptr;
  m_snapTxt = nullptr;
  m_hopCsv  = nullptr;
  m_dropCsv = nullptr;
  m_stateCsv = nullptr;
  m_pktCount.clear();
  m_byteCount.clear();
std::unordered_map<uint64_t, RoutingTelemetry::TtlRec>().swap(m_initTtl);
  m_streamIndex.clear();         
  m_started = false;
}

bool RoutingTelemetry::Classify(Ptr<const Packet> original,
                                const Ipv4Header& ip,
                                RtId& out,
                                uint16_t& bytes)
{
    out = RtId{};
    Packet pkt = *original->Copy();

    // Bytes as seen at L4 in the trace (UDP + routing payload, if UDP is present)
    bytes = pkt.GetSize();

    if (!IsUdp(ip.GetProtocol()))
      return false;

    // Try to peel UDP header if present
    UdpHeader uh;
    const bool hasUdp = pkt.PeekHeader(uh);

    uint16_t sport = 0, dport = 0;
    if (hasUdp)
    {
      sport = uh.GetSourcePort();
      dport = uh.GetDestinationPort();
      pkt.RemoveHeader(uh); // now pkt is UDP payload
    }

    // "routing bytes excluding UDP" (if UDP existed)
    const uint16_t payloadLen = pkt.GetSize();

  #ifdef SRT_HAVE_OLSR
    // OLSR: if UDP missing, allow parse attempt (for weird traces), else require port match
    if (!hasUdp || sport == OLSR_UDP_PORT || dport == OLSR_UDP_PORT)
    {
      olsr::PacketHeader ph;
      if (pkt.PeekHeader(ph))
      {
        Packet tmp = pkt;
        tmp.RemoveHeader(ph);

        olsr::MessageHeader mh;
        if (tmp.PeekHeader(mh))
        {
          out.proto      = RT_OLSR;
          out.type       = static_cast<uint16_t>(mh.GetMessageType());
          out.headerLen  = payloadLen;
          out.msgSeq     = mh.GetMessageSequenceNumber();
          out.originator = mh.GetOriginatorAddress();
          return true;
        }
      }

     
    }
  #endif

  #ifdef SRT_HAVE_AODV
    // AODV: same logic
    if (!hasUdp || sport == AODV_UDP_PORT || dport == AODV_UDP_PORT)
    {
      aodv::TypeHeader th;
      if (pkt.PeekHeader(th) && th.IsValid())
      {
        
        out.proto     = RT_AODV;
        out.type      = static_cast<uint16_t>(th.Get());
        out.headerLen = payloadLen;

        Packet tmp = pkt;
        tmp.RemoveHeader(th);

        if (th.Get() == aodv::AODVTYPE_RREQ)
        {
          aodv::RreqHeader h;
          if (tmp.PeekHeader(h))
          {
            out.msgSeq     = h.GetId();
            out.dstSeq     = h.GetDstSeqno();
            out.originator = h.GetOrigin();
          }
        }
        else if (th.Get() == aodv::AODVTYPE_RREP)
        {
          aodv::RrepHeader h;
          if (tmp.PeekHeader(h))
          {
            out.dstSeq     = h.GetDstSeqno();
            out.originator = h.GetOrigin();
          }
        }
        // RERR/RREP_ACK: optional parsing if you want

        return true;
      }
    }
  #endif

  #ifdef SRT_HAVE_DSDV
    if (!hasUdp || sport == DSDV_UDP_PORT || dport == DSDV_UDP_PORT)
    {
      dsdv::DsdvHeader dh;
      if (pkt.PeekHeader(dh))
      {
       
        out.proto     = RT_DSDV;
        out.type      = 1;
        out.headerLen = payloadLen;
        out.dstSeq    = dh.GetDstSeqno();
        return true;
      }
    }
  #endif

    return false;
}
void
 RoutingTelemetry::LogCtrl(uint32_t nodeId,
                               const char* dir,
                               const RtId& id,
                               uint16_t bytes,
                               const Ipv4Header& ip,
                               uint32_t ipPktBytes,
                               uint32_t ipPayloadBytes,
                               uint32_t udpPayloadBytes)

{
  RtKey key;
  key.node  = nodeId;
  key.proto = static_cast<uint8_t>(id.proto);
  key.type  = id.type;

  m_pktCount[key]++;
  m_byteCount[key] += bytes;

  const bool isBcast =
    ip.GetDestination ().IsBroadcast () ||
    ip.GetDestination ().IsMulticast ();
  const int transferMode = isBcast ? 1 : 0;

  int land = 2;
  if (id.originator.IsAny ())
    land = 2;
  else if (ip.GetSource () == id.originator)
    land = 0;
  else
    land = 1;

  int isRerr = 0;
#ifdef SRT_HAVE_AODV
  if (id.proto == RT_AODV && id.type == aodv::AODVTYPE_RERR)
    isRerr = 1;
#endif

  StreamKey sk (nodeId,
                static_cast<uint8_t> (id.proto),
                id.type,
                ip.GetDestination ());
  uint32_t streamIdx = ++m_streamIndex[sk];

  *m_ctrlLog->GetStream ()
    << m_cfg.seed    << ','
    << m_cfg.run     << ','
    << m_cfg.routing << ','
    << Simulator::Now ().GetSeconds () << ','
    << nodeId        << ','
    << dir           << ','
    << static_cast<int> (id.proto) << ','
    << id.type       << ','

    // ✅ NEW: 3 sizes (TX/RX both have them)
    << ipPktBytes << ','
    << ipPayloadBytes << ','
    << udpPayloadBytes << ','


    // ✅ keep your existing fields
    << bytes         << ','
    << id.headerLen  << ','
    << ip.GetSource () << ','
    << ip.GetDestination () << ','
    << transferMode  << ','
    << land          << ','
    << id.msgSeq     << ','
    << id.dstSeq     << ','
    << isRerr        << ','
    << streamIdx     << '\n';
}

// void
// RoutingTelemetry::LogCtrl (uint32_t nodeId,
//                            const char *dir,
//                            const RtId &id,
//                            uint16_t bytes,
//                            const Ipv4Header &ip)
// {


//   RtKey key;
//   key.node  = nodeId;
//   key.proto = static_cast<uint8_t>(id.proto);
//   key.type  = id.type;

//   m_pktCount[key]++;
//   m_byteCount[key] += bytes;

//   // Transfer mode: 0 = unicast, 1 = broadcast/multicast
//   const bool isBcast =
//     ip.GetDestination ().IsBroadcast () ||
//     ip.GetDestination ().IsMulticast ();
//   const int transferMode = isBcast ? 1 : 0;

//   // Land: 0 = same (sender == originator),
//   //       1 = different,
//   //       2 = unknown/not available
//   int land = 2;
//   if (id.originator.IsAny ())
//     {
//       land = 2;
//     }
//   else if (ip.GetSource () == id.originator)
//     {
//       land = 0;
//     }
//   else
//     {
//       land = 1;
//     }

//   // Is this an AODV RERR?
//   int isRerr = 0;
// #ifdef SRT_HAVE_AODV
//   if (id.proto == RT_AODV && id.type == aodv::AODVTYPE_RERR)
//     {
//       isRerr = 1;
//     }
// #endif

//   // Stream index key: (node, proto, msgType, dest IP)
//   StreamKey sk (nodeId,
//                 static_cast<uint8_t> (id.proto),
//                 id.type,
//                 ip.GetDestination ());
//   uint32_t streamIdx = ++m_streamIndex[sk];

//   *m_ctrlLog->GetStream ()
//     << m_cfg.seed    << ','
//     << m_cfg.run     << ','
//     << m_cfg.routing << ','
//     << Simulator::Now ().GetSeconds () << ','
//     << nodeId        << ','
//     << dir           << ','
//     << static_cast<int> (id.proto) << ','
//     << id.type       << ','
//     << bytes         << ','
//     << id.headerLen  << ','      // header length
//     << ip.GetSource () << ','
//     << ip.GetDestination () << ','
//     << transferMode  << ','      // 0 = unicast, 1 = broadcast
//     << land          << ','      // 0 = same, 1 = different, 2 = unknown
//     << id.msgSeq     << ','      // message sequence number
//     << id.dstSeq     << ','      // destination sequence number
//     << isRerr        << ','      // convenience flag
//     << streamIdx     << '\n';

// }

void RoutingTelemetry::OnIpv4Tx(uint32_t nodeId,
                                Ptr<const Packet> p,
                                Ptr<Ipv4> /*ipv4*/,
                                uint32_t /*ifIndex*/)
{
  Packet copy = *p->Copy();

  Ipv4Header ip;
  if (!copy.PeekHeader(ip))
    return;

  copy.RemoveHeader(ip);        // now copy is L4 payload
  Ptr<Packet> l4 = copy.Copy(); // safe copy for Classify()
SizeInfo sz = ExtractSizes(p);

  RtId id;
  uint16_t bytes = 0;
  const bool isCtrl = Classify(l4, ip, id, bytes);

  if (isCtrl)
  {

LogCtrl(nodeId, "TX", id, bytes, ip,
        sz.ipPktBytes, sz.ipPayloadBytes, sz.udpPayloadBytes);

  }
  else
  {
    const uint64_t uid = p->GetUid();

// store initial TTL once
if (m_initTtl.find(uid) == m_initTtl.end())
{
  m_initTtl.emplace(uid, RoutingTelemetry::TtlRec{ip.GetTtl(), Simulator::Now()});
}

// time-based eviction (prevents dropped packets from accumulating)
const ns3::Time now = Simulator::Now();
if (now - g_lastTtlEvict >= ns3::Seconds(1))
{
  g_lastTtlEvict = now;
  for (auto it = m_initTtl.begin(); it != m_initTtl.end(); )
  {
    if (now - it->second.firstSeen > TTL_KEEP) it = m_initTtl.erase(it);
    else ++it;
  }
}

// hard cap that actually frees memory
if (m_initTtl.size() > MAX_TTL_CACHE)
{
  std::unordered_map<uint64_t, RoutingTelemetry::TtlRec>().swap(m_initTtl);
}

  }
}
void RoutingTelemetry::OnIpv4Rx(uint32_t nodeId,
                                const Ipv4Header& ip,
                                Ptr<const Packet> p,
                                uint32_t /*ifIndex*/)
{
  RtId id;
  uint16_t bytes = 0;
  const bool isCtrl = Classify(p, ip, id, bytes);
SizeInfo sz = ExtractSizes(p);

  if (isCtrl)
  {
LogCtrl(nodeId, "RX", id, bytes, ip,
        sz.ipPktBytes, sz.ipPayloadBytes, sz.udpPayloadBytes);  }
  else
  {
    const uint64_t uid = p->GetUid();
    auto it = m_initTtl.find(uid);
    if (it != m_initTtl.end())
    {




      int hops = int(it->second.ttl) - int(ip.GetTtl());
      if (hops < 0) hops = 0;

      if (m_hopCsv)
      {
        *m_hopCsv->GetStream()
          << m_cfg.seed << ',' << m_cfg.run << ',' << m_cfg.routing << ','
          << Simulator::Now().GetSeconds() << ','
          << uid << ',' << ip.GetSource() << ',' << ip.GetDestination() << ','
          << int(it->second.ttl) << ',' << int(ip.GetTtl()) << ',' << hops << '\n';
      }

      m_initTtl.erase(it);
    }
  }
}
static const char* DropReasonToStr(ns3::Ipv4L3Protocol::DropReason r)
{
  using DR = ns3::Ipv4L3Protocol::DropReason;
  switch (r)
  {
    case DR::DROP_NO_ROUTE:      return "NO_ROUTE";
    case DR::DROP_TTL_EXPIRED:   return "TTL_EXPIRED";
    case DR::DROP_BAD_CHECKSUM:  return "BAD_CHECKSUM";
    case DR::DROP_INTERFACE_DOWN:return "IF_DOWN";
    default:                     return "OTHER";
  }
}

void RoutingTelemetry::OnIpv4Drop(uint32_t nodeId,
                                  const ns3::Ipv4Header& ip,
                                  ns3::Ptr<const ns3::Packet> p,
                                  ns3::Ipv4L3Protocol::DropReason reason,
                                  ns3::Ptr<ns3::Ipv4> /*ipv4*/,
                                  uint32_t /*ifIndex*/)
{
  
  
  if (!m_dropCsv) return;

  SizeInfo sz = ExtractSizes(p);

  *m_dropCsv->GetStream()
  << m_cfg.seed << ','
  << m_cfg.run << ','
  << m_cfg.routing << ','
  << Simulator::Now().GetSeconds() << ','
  << nodeId << ','
  << DropReasonToStr(reason) << ','
  << sz.ipPktBytes << ','
<< sz.ipPayloadBytes << ','
<< sz.udpPayloadBytes << ','

  << ip.GetSource() << ','
  << ip.GetDestination() << '\n';

}

// void RoutingTelemetry::OnIpv4Tx  (uint32_t nodeId,
//     ns3::Ptr<const ns3::Packet>,
//     ns3::Ptr<ns3::Ipv4>,
//     uint32_t ifIndex)

                                

// {
//   Packet copy = *p->Copy();

//   Ipv4Header ip;
//   if (!copy.PeekHeader(ip))
//     return;

//   copy.RemoveHeader(ip);          // now copy is L4 payload
//   Ptr<Packet> l4 = copy.Copy();   // safe local copy for Classify()

//   RtId id; 
//   uint16_t bytes = 0;
//   const bool isCtrl = Classify(l4, ip, id, bytes);

//   if (isCtrl) 
//   {
//     LogCtrl(nodeId, "TX", id, bytes, ip);
//   } 
//   else 
//   {
//     const uint64_t uid = p->GetUid();

//     // store initial TTL once
//     if (m_initTtl.find(uid) == m_initTtl.end())
//     {
//       m_initTtl.emplace(uid, ip.GetTtl());
//     }

//     // ✅ RAM safety cap: if Rx never matches UID, this map can grow forever
//     static const size_t MAX_TTL_CACHE = 200000; // tune: 50k–500k depending on your scale
//     if (m_initTtl.size() > MAX_TTL_CACHE)
//     {
//       m_initTtl.clear(); // cheapest safe fallback (prevents unbounded growth)
//     }
//   }
// }

// void RoutingTelemetry::OnIpv4Rx(  uint32_t nodeId,
//                                   const ns3::Ipv4Header&,
//                                   ns3::Ptr<const ns3::Packet>,
//                                   uint32_t ifIndex
//                               )
// {
//   // At "LocalDeliver", p is already the IP payload (L4).
//   RtId id; uint16_t bytes = 0;
//   const bool isCtrl = Classify(p, ip, id, bytes);

//   if (isCtrl) {
//     LogCtrl(nodeId, "RX", id, bytes, ip);
//   } else {
//     // Data delivered locally: compute hops via TTL delta if we saw this UID at Tx
//     const uint64_t uid = p->GetUid();
//     auto it = m_initTtl.find(uid);
//     if (it != m_initTtl.end()) {
//       int hops = int(it->second) - int(ip.GetTtl());
//       if (hops < 0) hops = 0; // guard against weird edge cases
//       if (m_hopCsv) {
//         *m_hopCsv->GetStream()
//           << m_cfg.seed << ',' << m_cfg.run << ',' << m_cfg.routing << ','
//           << Simulator::Now().GetSeconds() << ','
//           << uid << ',' << ip.GetSource() << ',' << ip.GetDestination() << ','
//           << int(it->second) << ',' << int(ip.GetTtl()) << ',' << hops << '\n';
//       }
//       m_initTtl.erase(it); // done with this packet
//     }
//   }
// }

// Write one CSV row
static inline void WriteSnapshotRow(Ptr<OutputStreamWrapper> csv,
                                    const RoutingTelemetryConfig& cfg,
                                    double t, uint32_t nodeId, RtProto proto,
                                    const std::string& dst,
                                    const std::string& nextHop,
                                    int hopCount, double validUntil)
{
  *csv->GetStream() << cfg.seed << ',' << cfg.run << ',' << cfg.routing << ','
                    << std::fixed << std::setprecision(3) << t << ','
                    << nodeId << ',' << static_cast<int>(proto) << ','
                    << dst << ',' << nextHop << ','
                    << hopCount << ','
                    << std::fixed << std::setprecision(3) << validUntil << '\n';
}

// Capture a protocol’s PrintRoutingTable(...) into a string
template <typename RoutingProto>
static std::string CaptureRoutingTableText(Ptr<RoutingProto> rp) {
  std::ostringstream oss;
  Ptr<OutputStreamWrapper> sink = Create<OutputStreamWrapper>(&oss);
  rp->PrintRoutingTable(sink);   // same call you use for the text log
  return oss.str();
}
// Parse text -> CSV rows with protocol-aware patterns.
// Extracts Destination, NextHop, HopCount (when present), and ValidUntil_s (when present).

static std::vector<RtEntry> ParseTableTextToCsv(
    Ptr<OutputStreamWrapper> csv,
    const RoutingTelemetryConfig& cfg,
    double t,
    uint32_t nodeId,
    RtProto proto,
    const std::string& text)
{
    std::vector<RtEntry> entries;   // <- collect routing entries
    // SPECIAL CASE: OLSR PrintRoutingTable() format in your code:
    //   Destination        NextHop           Interface        Distance
    //   10.0.0.2           10.0.0.2          1                1
    // So we parse per-line instead of "Destination:" chunks.
    // ------------------------------------------------------------
    if (proto == RT_OLSR)
    {
        // Capture:
        //  (1) Destination IP
        //  (2) NextHop IP
        //  (3) Distance (integer)
        //
        // Interface column can be a number or a name, so we match it loosely as \S+.
        static const std::regex olsrRow(
            R"(^\s*(\d{1,3}(?:\.\d{1,3}){3})\s+(\d{1,3}(?:\.\d{1,3}){3})\s+(\S+)\s+(\d+)\s*$)",
            std::regex::icase);

        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line))
        {
            std::smatch m;
            if (std::regex_match(line, m, olsrRow) && m.size() >= 5)
            {
                std::string dst = m[1].str();
                std::string nh  = m[2].str();
                int distance    = std::stoi(m[4].str());

                // sanity filters (keep your original logic)
                if (dst != "0.0.0.0" && nh != "0.0.0.0")
                {
                    entries.push_back({dst, nh, distance});
                    WriteSnapshotRow(csv, cfg, t, nodeId, proto, dst, nh, distance, 0.0);
                }

               
            }
        }

        return entries;
    }

    
    // ============================================================
    // AODV: parse fixed-width table lines (not "Destination:" blocks)
    // ============================================================
    if (proto == RT_AODV)
    {
        auto parseExpire = [](const std::string& s) -> double {
            std::smatch m;
            static const std::regex num(R"(([0-9]+(?:\.[0-9]+)?))");
            if (std::regex_search(s, m, num)) return std::stod(m[1].str());
            return 0.0;
        };

        // Pattern A: dst nh iface flag expire hops seq
        static const std::regex aodvRowA(
            R"(^\s*(\d{1,3}(?:\.\d{1,3}){3})\s+(\d{1,3}(?:\.\d{1,3}){3})\s+(\S+)\s+(\S+)\s+([0-9]+(?:\.[0-9]+)?)\s+(\d+)\s+(\d+)\s*$)");

        // Pattern B: dst nh iface hops expire seq
        static const std::regex aodvRowB(
            R"(^\s*(\d{1,3}(?:\.\d{1,3}){3})\s+(\d{1,3}(?:\.\d{1,3}){3})\s+(\S+)\s+(\d+)\s+([0-9]+(?:\.[0-9]+)?)\s+(\d+)\s*$)");

        // Pattern C: very permissive fallback
        static const std::regex ip2(
            R"((\d{1,3}(?:\.\d{1,3}){3}).*?(\d{1,3}(?:\.\d{1,3}){3}))");


        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line))
        {
            if (line.find("Destination") != std::string::npos ||
                line.find("AODV Routing") != std::string::npos ||
                line.find("Node:") != std::string::npos)
            {
                continue;
            }

            std::smatch m;
            std::string dst, nh;
            int hops = -1;
            double validUntil = 0.0;

            if (std::regex_match(line, m, aodvRowA) && m.size() >= 8)
            {
                dst  = m[1].str();
                nh   = m[2].str();
                double expire = parseExpire(m[5].str());
                hops = std::stoi(m[6].str());
                if (expire > 0.0) validUntil = t + expire;
            }
            else if (std::regex_match(line, m, aodvRowB) && m.size() >= 7)
            {
                dst  = m[1].str();
                nh   = m[2].str();
                hops = std::stoi(m[4].str());
                double expire = parseExpire(m[5].str());
                if (expire > 0.0) validUntil = t + expire;
            }
            else
            {
                std::smatch ipm;
                if (!std::regex_search(line, ipm, ip2) || ipm.size() < 3) continue;
                dst = ipm[1].str();
                nh  = ipm[2].str();

                // heuristic: hopcount is usually a small integer near end
                {
                  static const std::regex reInt(R"((\d+))");
                  std::sregex_iterator it(line.begin(), line.end(), reInt);

                    std::sregex_iterator end;
                    std::vector<std::string> nums;
                    for (; it != end; ++it) nums.push_back((*it)[1].str());
                    for (int k = (int)nums.size() - 1; k >= 0; --k)
                    {
                        int v = std::stoi(nums[k]);
                        if (v >= 0 && v <= 255) { hops = v; break; }
                    }
                }

                // heuristic: expire is last floating number
                {


                  static const std::regex reFloat(R"(([0-9]+(?:\.[0-9]+)?))");
                  std::sregex_iterator it(line.begin(), line.end(), reFloat);


                  //  std::sregex_iterator it(line.begin(), line.end(), std::regex);
                    std::sregex_iterator end;
                    double last = 0.0; bool has=false;
                    for (; it != end; ++it) { last = std::stod((*it)[1].str()); has=true; }
                    if (has && last > 0.0) validUntil = t + last;
                }
            }

                  if (!dst.empty() && !nh.empty() && dst != "0.0.0.0" && nh != "0.0.0.0")
                      {
                        entries.push_back({dst, nh, hops});
                        WriteSnapshotRow(csv, cfg, t, nodeId, proto, dst, nh, hops, validUntil);
                      }

           
        }
        return entries;
    }


    // ============================================================
// DSDV: parse fixed-width table lines (not "Destination:" blocks)
// ============================================================
if (proto == RT_DSDV)
{
    // We don't know the exact column layout of your DSDV RoutingTable::Print()
    // (it varies across ns-3 versions), so we do a robust parse:
    //  - detect first TWO IPv4 addresses on the line: dst, nextHop
    //  - hop/metric: try to pick a small integer (1..64) from the remaining tokens
    //  - lifetime/expiry: try to pick a floating number near the end (optional)

    static const std::regex ip2(R"((\d{1,3}(?:\.\d{1,3}){3}).*?(\d{1,3}(?:\.\d{1,3}){3}))");

    auto extractHops = [](const std::string& line) -> int {
        // pick a plausible hop/metric: small integer
        static const std::regex reInt(R"((\d+))");
        std::sregex_iterator it(line.begin(), line.end(), reInt);

        //std::sregex_iterator it(line.begin(), line.end(), std::regex(R"((\d+))"));
        std::sregex_iterator end;

        std::vector<int> nums;
        for (; it != end; ++it)
        {
            nums.push_back(std::stoi((*it)[1].str()));
        }

        // Prefer values that look like hopcounts/metrics (small)
        for (int k = (int)nums.size() - 1; k >= 0; --k)
        {
            int v = nums[k];
            if (v >= 1 && v <= 64) return v;
        }

        // Fallback: if we found anything, return last; else unknown
        return nums.empty() ? -1 : nums.back();
    };

    auto extractLifetime = [](const std::string& line) -> double {
        // last floating-like number often corresponds to "lifetime/expiry"
        static const std::regex reFloat(R"(([0-9]+(?:\.[0-9]+)?))");
        std::sregex_iterator it(line.begin(), line.end(), reFloat);


       // std::sregex_iterator it(line.begin(), line.end(),
         //                       std::regex(R"(([0-9]+(?:\.[0-9]+)?))"));
        std::sregex_iterator end;

        double last = 0.0;
        bool has = false;
        for (; it != end; ++it)
        {
            last = std::stod((*it)[1].str());
            has = true;
        }
        // If there is no float-like number, return 0
        return has ? last : 0.0;
    };

    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line))
    {
        // Skip headings
        if (line.find("Node:") != std::string::npos ||
            line.find("DSDV") != std::string::npos ||
            line.find("Destination") != std::string::npos)
        {
            continue;
        }

        std::smatch m;
        if (!std::regex_search(line, m, ip2) || m.size() < 3)
        {
            continue;
        }

        std::string dst = m[1].str();
        std::string nh  = m[2].str();

        // DO NOT drop dst==nh (direct routes are valid)
        if (dst == "0.0.0.0" || nh == "0.0.0.0")
        {
            continue;
        }

        int hops = extractHops(line);

        double lifetime = extractLifetime(line);
        double validUntil = 0.0;
        // If lifetime looks sane, treat it as relative seconds
        if (lifetime > 0.0 && lifetime < 1e9)
        {
            validUntil = t + lifetime;
        }

        entries.push_back({dst, nh, hops});
        WriteSnapshotRow(csv, cfg, t, nodeId, proto, dst, nh, hops, validUntil);
    }

    return entries;
}

return entries;

}
static void ComputeRoutingStateMetrics(
    Ptr<OutputStreamWrapper> csv,
    const RoutingTelemetryConfig& cfg,
    double t,
    uint32_t nodeId,
    const std::vector<RtEntry>& table)
{
    const size_t N = table.size();
    if (N == 0) {
        *csv->GetStream() << cfg.seed << ',' << cfg.run << ',' << cfg.routing << ','
                          << t << ',' << nodeId << ','
                          << 0 << ','    // size
                          << 0 << ','    // entropy
                          << 0 << '\n';  // next-hop ratio
        return;
    }

    // --- RoutingTableSize ---
    const double size = N;

    // --- SameNextHopRatio ---
    std::map<std::string,int> nhCount;
    for (auto& e : table) nhCount[e.nextHop]++;
    int maxCount = 0;
    for (auto& kv : nhCount) maxCount = std::max(maxCount, kv.second);
    double sameHopRatio = double(maxCount) / double(N);

    // --- Entropy of next-hop distribution ---
    double entropy = 0.0;
    for (auto& kv : nhCount) {
        double p = double(kv.second) / double(N);
        entropy -= p * std::log2(p);
    }

    // ---- Write state row ----
    *csv->GetStream()
        << cfg.seed << ',' << cfg.run << ',' << cfg.routing << ','
        << t << ',' << nodeId << ','
        << size << ',' << entropy << ',' << sameHopRatio << '\n';
}

template <typename Proto>
static ns3::Ptr<Proto> FindRoutingProto(ns3::Ptr<ns3::Node> node)
{
  using namespace ns3;

  if (!node) return nullptr;

  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
  if (!ipv4) return nullptr;

  Ptr<Ipv4RoutingProtocol> rp = ipv4->GetRoutingProtocol();
  if (!rp) return nullptr;

  // Direct case
  if (Ptr<Proto> p = DynamicCast<Proto>(rp))
    return p;

  // Common case: list routing
  if (Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting>(rp))
  {
    for (uint32_t i = 0; i < list->GetNRoutingProtocols(); ++i)
    {
      int16_t prio = 0;
      Ptr<Ipv4RoutingProtocol> inner = list->GetRoutingProtocol(i, prio);
      if (Ptr<Proto> p = DynamicCast<Proto>(inner))
        return p;
    }
  }

  return nullptr;
}

void
RoutingTelemetry::DoSnapshot ()
{
  const double t = Simulator::Now ().GetSeconds ();

  for (uint32_t i = 0; i < NodeList::GetNNodes (); ++i)
    {
      Ptr<Node> n = NodeList::GetNode (i);
      bool any = false;

#ifdef SRT_HAVE_OLSR
  if (Ptr<olsr::RoutingProtocol> rp = FindRoutingProto<olsr::RoutingProtocol>(n)) 
        {
          // Capture once
          std::string tableText = CaptureRoutingTableText (rp);

          // Human-readable log
          *m_snapTxt->GetStream ()
            << "[t=" << t << "] Node " << i << " OLSR\n"
            << tableText << "\n";

          // CSV + metrics
          auto entries = ParseTableTextToCsv (m_snapCsv, m_cfg, t, i, RT_OLSR, tableText);
          ComputeRoutingStateMetrics (m_stateCsv, m_cfg, t, i, entries);

          any = true;
        }
#endif
#ifdef SRT_HAVE_AODV
  if (Ptr<aodv::RoutingProtocol> rp = FindRoutingProto<aodv::RoutingProtocol>(n))
  {
    std::string tableText = CaptureRoutingTableText(rp);

    *m_snapTxt->GetStream()
      << "[t=" << t << "] Node " << i << " AODV\n"
      << tableText << "\n";

    auto entries = ParseTableTextToCsv(m_snapCsv, m_cfg, t, i, RT_AODV, tableText);
    ComputeRoutingStateMetrics(m_stateCsv, m_cfg, t, i, entries);
    any = true;
  }
#endif


#ifdef SRT_HAVE_DSDV
  if (Ptr<dsdv::RoutingProtocol> rp = FindRoutingProto<dsdv::RoutingProtocol>(n)) 
        {
          std::string tableText = CaptureRoutingTableText (rp);

          *m_snapTxt->GetStream ()
            << "[t=" << t << "] Node " << i << " DSDV\n"
            << tableText << "\n";

          auto entries = ParseTableTextToCsv (m_snapCsv, m_cfg, t, i, RT_DSDV, tableText);
          ComputeRoutingStateMetrics (m_stateCsv, m_cfg, t, i, entries);

          any = true;
        }
#endif

#ifdef SRT_HAVE_DSR
      if (Ptr<dsr::DsrRouting> rp = n->GetObject<dsr::DsrRouting> ())
        {
          std::string tableText = CaptureRoutingTableText (rp);
          *m_snapTxt->GetStream ()
            << "[t=" << t << "] Node " << i << " DSR (snapshot fallback)\n"
            << tableText << "\n";

          any = true;
        }
#endif

      if (!any)
        {
          // Node doesn't run a known protocol; placeholder row (keeps CSV rectangular)
          WriteSnapshotRow (m_snapCsv, m_cfg, t, i, RT_UNKNOWN,
                            "0.0.0.0", "0.0.0.0", -1, 0.0);
        }
    }

  if (m_cfg.snapshotPeriod > Seconds (0))
    {
      Simulator::Schedule (m_cfg.snapshotPeriod,
                           &RoutingTelemetry::DoSnapshot);
    }
}


std::string BuildPrefix(const std::string& scenarioId,
                        const std::string& routingName,
                        uint32_t seed,
                        uint32_t runNumber,
                        const std::string& base)
{
  // folder = scenarioId + routingName  (e.g. "testolsr")
  const std::string folder = scenarioId + routingName;

  // results path: base/folder/routingName/seed-<seed>/
  const std::string outdir =
      base + "/" + folder + "/" + routingName + "/seed-" + std::to_string(seed);

  EnsureDir(outdir);  // creates the directories if missing

  // file prefix: base/folder/routingName/seed-<seed>/<folder>_seed<seed>_run<run>
  const std::string prefix = outdir + "/" + folder
      + "_seed" + std::to_string(seed)
      + "_run"  + std::to_string(runNumber);

  return prefix;
}


void StartForRun(const std::string& scenarioId,
                 const std::string& routingName,
                 uint32_t seed,
                 uint32_t runNumber,
                 double snapshotSeconds)
{
  // --- normalize names ---
  auto normScenario = scenarioId;
  for (auto& c : normScenario) { if (c == ' ') c = '_'; if (c == '/') c = '-'; }

  std::string normRouting = routingName;
  for (auto& c : normRouting) c = std::toupper(static_cast<unsigned char>(c)); // OLSR/AODV/DSDV

  // build prefix with normalized names
  auto prefix = BuildPrefix(normScenario, normRouting, seed, runNumber);

  RoutingTelemetryConfig cfg;
  cfg.seed = seed;
  cfg.run  = runNumber;
  cfg.routing = normRouting;           // <-- normalized here
  cfg.filePrefix = prefix;
  cfg.snapshotPeriod = ns3::Seconds(snapshotSeconds);
  RoutingTelemetry::Start(cfg);
}
sartelemetry::RoutingTelemetry::DebugStats
sartelemetry::RoutingTelemetry::GetDebugStats()
{
  DebugStats s;
  s.ttlCache   = m_initTtl.size();
  s.streamKeys = m_streamIndex.size();
  s.pktKeys    = m_pktCount.size();
  s.byteKeys   = m_byteCount.size();
  return s;
}




} // namespace sartelemetry
