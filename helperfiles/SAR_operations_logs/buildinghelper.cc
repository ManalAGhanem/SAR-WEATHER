

//  Function to Create & Log Buildings
static void CreateBuildingsInBlockWithParams(
    BuildingContainer &allBuildings,
    double minX,
    double minY,
    double baseElevation,
    uint32_t gridWidth,
    double lengthX,
    double lengthY,
    double deltaX,
    double deltaY,
    double buildingHeight,
    Building::BuildingType_t buildingType,
    Building::ExtWallsType_t extWallsType,
    uint32_t nFloors,
    uint32_t nRoomsX,
    uint32_t nRoomsY,
    uint32_t totalBuildings
) {
    // 1) Create the building allocator
    Ptr<GridBuildingAllocator> alloc = CreateObject<GridBuildingAllocator>();

    // 2) Set grid parameters
    alloc->SetAttribute("GridWidth", UintegerValue(gridWidth));
    alloc->SetAttribute("LengthX", DoubleValue(lengthX));
    alloc->SetAttribute("LengthY", DoubleValue(lengthY));
    alloc->SetAttribute("DeltaX", DoubleValue(deltaX));
    alloc->SetAttribute("DeltaY", DoubleValue(deltaY));
    alloc->SetAttribute("Height", DoubleValue(buildingHeight));
    alloc->SetAttribute("MinX", DoubleValue(minX));
    alloc->SetAttribute("MinY", DoubleValue(minY));

    // 3) Create buildings
    BuildingContainer blockBuildings = alloc->Create(totalBuildings);

    //  Open log file for positions
    std::ofstream posFile("Building_Positionsfinalv3.csv", std::ios::app);
    if (!posFile.is_open()) {
        std::cerr << "⚠️ ERROR: Cannot open Building_Positionsfinalv3.csv!" << std::endl;
        return;
    }
    posFile << "Building_ID,X,Y,Z\n";

    // 4) Assign building parameters & log positions
    for (uint32_t i = 0; i < blockBuildings.GetN(); ++i) {
        Ptr<Building> b = blockBuildings.Get(i);
        Box bounds = b->GetBoundaries();

        double safeBaseElevation = std::max(1.0, baseElevation);
        b->SetBoundaries(Box(bounds.xMin, bounds.xMax, bounds.yMin, bounds.yMax, safeBaseElevation, safeBaseElevation + buildingHeight));

        b->SetBuildingType(buildingType);
        b->SetExtWallsType(extWallsType);
        b->SetNFloors(nFloors);
        b->SetNRoomsX(nRoomsX);
        b->SetNRoomsY(nRoomsY);

        Vector center((bounds.xMin + bounds.xMax) / 2, (bounds.yMin + bounds.yMax) / 2, (safeBaseElevation + buildingHeight / 2));
        posFile << i << "," << center.x << "," << center.y << "," << center.z << "\n";
    }

    posFile.close();
    allBuildings.Add(blockBuildings);
}


bool IsInsideAnyBuilding(Vector position, BuildingContainer buildings) {
    for (uint32_t i = 0; i < buildings.GetN(); i++) {
        Ptr<Building> building = buildings.Get(i);
        Box boundaries = building->GetBoundaries();

        // Log checking the building boundaries
        //NS_LOG_INFO("Checking position " << position << " against building " << i
                         //              << " with boundaries: " << boundaries);

        if (position.x >= boundaries.xMin && position.x <= boundaries.xMax &&
            position.y >= boundaries.yMin && position.y <= boundaries.yMax) {
            NS_LOG_WARN("🚨 Position " << position << " is inside building " << i);
            return true; // 🚨 Position is inside a building!
        }
    }
    //NS_LOG_INFO(" Position " << position << " is safe (outside buildings)");
    return false; //  Position is safe (outside buildings)
}

void PlaceNodesOutsideBuildings(NodeContainer nodes, BuildingContainer buildings, double height) {
    Ptr<UniformRandomVariable> randX = CreateObject<UniformRandomVariable>();
    Ptr<UniformRandomVariable> randY = CreateObject<UniformRandomVariable>();
    randX->SetAttribute("Min", DoubleValue(0.0));
    randX->SetAttribute("Max", DoubleValue(400.0));
    randY->SetAttribute("Min", DoubleValue(0.0));
    randY->SetAttribute("Max", DoubleValue(400.0));

    for (uint32_t i = 0; i < nodes.GetN(); i++) {
        Vector position;
        uint32_t retryCount = 0;
        do {
            position = Vector(randX->GetValue(), randY->GetValue(), height);
            retryCount++;
            // Log position and retry attempts
            //NS_LOG_INFO("Node " << i << " is being placed at position " << position
                              //  << ". Retry count: " << retryCount);

        } while (IsInsideAnyBuilding(position, buildings)); // 🚨 Keep retrying until position is outside

        // Log final position when successfully placed outside buildings
        //NS_LOG_INFO("Node " << i << " successfully placed at position " << position);

        Ptr<MobilityModel> mobility = nodes.Get(i)->GetObject<MobilityModel>();
        mobility->SetPosition(position);
    }
}

