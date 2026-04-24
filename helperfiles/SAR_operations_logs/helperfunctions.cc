bool IsLineOfSightBlocked(Vector srcPos, Vector destPos,
                          const BuildingContainer& buildings,
                          Vector &blockingBuildingPos)
{
    // Direction vector of the segment
    const double dx = destPos.x - srcPos.x;
    const double dy = destPos.y - srcPos.y;
    const double dz = destPos.z - srcPos.z;

    for (uint32_t i = 0; i < buildings.GetN(); i++) {
        Ptr<Building> building = buildings.Get(i);
        Box b = building->GetBoundaries();

        // Parametric t in [0,1] for segment; initialize to full range
        double tmin = 0.0;
        double tmax = 1.0;

        // Helper lambda to update tmin/tmax along one axis
        auto updateAxis = [&](double p0, double dp, double minB, double maxB) -> bool {
            if (std::abs(dp) < 1e-9) {
                // Segment is parallel to this axis; reject if outside slab
                if (p0 < minB || p0 > maxB) return false;
                return true; // no constraint from this axis
            }
            double t1 = (minB - p0) / dp;
            double t2 = (maxB - p0) / dp;
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            return tmin <= tmax;
        };

        // Intersect segment against slabs on x, y, z
        if (!updateAxis(srcPos.x, dx, b.xMin, b.xMax)) continue;
        if (!updateAxis(srcPos.y, dy, b.yMin, b.yMax)) continue;
        if (!updateAxis(srcPos.z, dz, b.zMin, b.zMax)) continue;

        // If we get here, tmin<=tmax: the segment intersects the box somewhere within [0,1]
        if (tmin <= tmax && tmin <= 1.0 && tmax >= 0.0) {
            // Center of the building (as you had)
            blockingBuildingPos.x = (b.xMin + b.xMax) * 0.5;
            blockingBuildingPos.y = (b.yMin + b.yMax) * 0.5;
            blockingBuildingPos.z = (b.zMin + b.zMax) * 0.5;
            return true;
        }
    }
    return false; // no AABB intersection → LOS clear
}

ns3::Ipv4Address GetNodePublicIp(Ptr<Node> node)
{
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    if (!ipv4) return ns3::Ipv4Address("0.0.0.0");

    // Start from interface 1 to skip the loopback (interface 0 is typically loopback)
    for (uint32_t i = 1; i < ipv4->GetNInterfaces(); i++)
    {
        for (uint32_t j = 0; j < ipv4->GetNAddresses(i); j++)
        {
            ns3::Ipv4Address addr = ipv4->GetAddress(i, j).GetLocal();
            if (addr != ns3::Ipv4Address("127.0.0.1"))
            {
                return addr;
            }
        }
    }
    return ns3::Ipv4Address("0.0.0.0");
}

// Function to check if a node is a drone or helicopter
bool IsDroneOrHelicopter(Ptr<Node> node) {
    uint32_t nodeId = node->GetId();
    return (nodeId == 1 || nodeId == 2);  // Example: Skip if node ID is 0 (drone) or 1 (helicopter)
}

// A single function to handle Vector-to-Vector distance in either 2D or 3D:
double CalculateDistance(const Vector &posA, const Vector &posB, bool use3D = true)
{
    double dx = posA.x - posB.x;
    double dy = posA.y - posB.y;
    double dz = posA.z - posB.z;

    if (!use3D)
    {
        // 2D (ignore z)
        return std::sqrt(dx * dx + dy * dy);
    }
    else
    {
        // 3D
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
}

// Then a wrapper that calculates Node-to-Node distance,
double CalculateDistance(Ptr<Node> node1, Ptr<Node> node2, bool use3D = true)
{
    Ptr<MobilityModel> mob1 = node1->GetObject<MobilityModel>();
    Ptr<MobilityModel> mob2 = node2->GetObject<MobilityModel>();
    Vector pos1 = mob1->GetPosition();
    Vector pos2 = mob2->GetPosition();

    return CalculateDistance(pos1, pos2, use3D);
}

// Helper function to check if a node is inside a NodeContainer
bool IsNodeInContainer(Ptr<Node> node, const NodeContainer& container) {
    for (uint32_t i = 0; i < container.GetN(); ++i) {
        if (container.Get(i) == node) {
            return true;
        }
    }
    return false;
}
double GetDiscoveryRadius(Ptr<Node> node, const NodeContainer& droneNodes,
    const NodeContainer& vehicleNodes, const NodeContainer& footNodes,
    const NodeContainer& helicopterNodes   ) 
{
    double nominalRange = 0.0;  //  Ensure it's defined locally

    if (IsNodeInContainer(node, droneNodes)) {
        nominalRange = 100.0;
    } else if (IsNodeInContainer(node, helicopterNodes)) {
        nominalRange = 120.0;
    } else if (IsNodeInContainer(node, vehicleNodes)) {
        nominalRange = 50.0;
    } else if (IsNodeInContainer(node, footNodes)) {
        nominalRange = 20.0;
    }

      return nominalRange;
}


// Helper function to get node name for the excel file
std::string GetNodeName(Ptr<Node> node, const NodeContainer& baseNodes, const NodeContainer& droneNodes,
                        const NodeContainer& helicopterNodes, const NodeContainer& vehicleNodes,
                        const NodeContainer& footNodes, const NodeContainer& civilianNodes) {
    uint32_t nodeId = node->GetId();
    if (baseNodes.Contains(nodeId)) {
        return "Base Station";
    } else if (droneNodes.Contains(nodeId)) {
        return "Drone";
    } else if (helicopterNodes.Contains(nodeId)) {
        return "Helicopter";
    } else if (vehicleNodes.Contains(nodeId)) {
        return "Vehicle";
    } else if (footNodes.Contains(nodeId)) {
        return "Foot Team";
    } else if (civilianNodes.Contains(nodeId)) {
        return "Civilian";
    } else {
        return "Unknown Node";
    }
}

Vector GetBaseStationPosition() {
    //  First, check if a global base station pointer is available
    if (g_baseNode) {
        Ptr<MobilityModel> mob = g_baseNode->GetObject<MobilityModel>();
        if (mob) {
            return mob->GetPosition();
        }
    }

    //  Second, check if we have a predefined base station Node ID
    uint32_t baseStationNodeId = 0; // Change this if base station ID is dynamic
    Ptr<Node> baseStationNode = NodeList::GetNode(baseStationNodeId);
    if (baseStationNode) {
        Ptr<MobilityModel> mob = baseStationNode->GetObject<MobilityModel>();
        if (mob) {
            return mob->GetPosition();
        }
    }

    //  Third, try to find the base station dynamically using its IP address
    Ipv4Address baseStationIp("10.1.1.1"); // Replace with actual base station IP
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
        Ptr<Node> node = *i;
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        if (!ipv4) continue;

        for (uint32_t j = 0; j < ipv4->GetNInterfaces(); j++) {
            if (ipv4->GetAddress(j, 0).GetLocal() == baseStationIp) {
                Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
                if (mob) {
                    return mob->GetPosition();
                }
            }
        }
    }

    //  If no method worked, return a default fallback position
    NS_LOG_WARN("⚠ Base Station position could not be determined dynamically. Using default (0,0,0).");
    return Vector(0, 0, 0);
}


static inline bool IsDiscoveredNoInsert(uint32_t sarId, uint32_t civId)
{
    auto itSar = discoveryState.find(sarId);
    if (itSar == discoveryState.end())
        return false;

    auto itCiv = itSar->second.find(civId);
    if (itCiv == itSar->second.end())
        return false;

    return itCiv->second;
}
static void CleanupSendTimestamps(double ttlSeconds)
{
    const double now = Simulator::Now().GetSeconds();

    for (auto it = sendTimestamps.begin(); it != sendTimestamps.end(); )
    {
        if ((now - it->second) > ttlSeconds)
        {
            it = sendTimestamps.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // run again
    Simulator::Schedule(Seconds(10.0), &CleanupSendTimestamps, ttlSeconds);
}

static ns3::Ptr<ns3::Socket> GetCachedUdpSocket(ns3::Ptr<ns3::Node> node, uint8_t tos)
{
    using namespace ns3;

    static std::unordered_map<uint32_t, Ptr<Socket>> cache;

    const uint32_t id = node->GetId();
    auto it = cache.find(id);
    if (it != cache.end() && it->second)
    {
        it->second->SetIpTos(tos);   // keep your ToS behavior
        return it->second;
    }

    Ptr<Socket> s = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
    if (!s)
    {
        NS_LOG_ERROR("GetCachedUdpSocket: failed for node " << id);
        return nullptr;
    }

    s->SetIpTos(tos);
    cache[id] = s;
    return s;
}
