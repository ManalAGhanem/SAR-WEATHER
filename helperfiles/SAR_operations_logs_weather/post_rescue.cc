void CheckSarNodesAtBase() {
    Vector baseStationPosition = GetBaseStationPosition();
    bool allArrived = true;

    for (auto node : g_sarNodes) {
        Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
        if (mob) {
            double distance = CalculateDistance(mob->GetPosition(), baseStationPosition, true);
            NS_LOG_INFO("🔍 Checking SAR Node " << node->GetId() << " | Distance to Base: " << distance << "m");

            if (distance > 10.0) {  //  Increased threshold to prevent errors
                allArrived = false;
            }
        }
    }

    if (allArrived) {
        NS_LOG_INFO("🚨 All SAR nodes have returned to base. Ending simulation.");
//        Simulator::Stop();
    } else {
        Simulator::Schedule(Seconds(1.0), &CheckSarNodesAtBase); //  Keep checking
    }
}

// void CheckSarNodesAtBase() {
//     Vector baseStationPosition = GetBaseStationPosition();
//     for (auto node : g_sarNodes) {
//         Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
//         if (mob && CalculateDistance(mob->GetPosition(), baseStationPosition, true) > 5.0) {
//             // At least one SAR node is still not at base, check again later
//             Simulator::Schedule(Seconds(1.0), &CheckSarNodesAtBase);
//             return;
//         }
//     }

//     NS_LOG_INFO("🚨 All SAR nodes have returned to base. Ending simulation.");
//     Simulator::Stop();
// }

void OrderSarNodesToReturn() {
    Vector baseStationPosition = GetBaseStationPosition(); 

    for (auto node : g_sarNodes) {  
        Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
        if (!mob) continue;

        Ptr<ConstantVelocityMobilityModel> velMob = node->GetObject<ConstantVelocityMobilityModel>();
        if (!velMob) {
            node->AggregateObject(CreateObject<ConstantVelocityMobilityModel>());
            velMob = node->GetObject<ConstantVelocityMobilityModel>();
        }

        //  Make SAR nodes move gradually instead of teleporting
        double speed = 10.0; 
        Vector direction = baseStationPosition - mob->GetPosition();
        double distance = CalculateDistance(mob->GetPosition(), baseStationPosition, true);
        direction = Vector(direction.x / distance * speed, direction.y / distance * speed, direction.z / distance * speed);
        velMob->SetVelocity(direction);

        NS_LOG_INFO(" SAR Node " << node->GetId() 
                     << " moving to base at speed " << speed << " m/s.");
    }
    
    Simulator::Schedule(Seconds(1.0), &CheckSarNodesAtBase); //  Start monitoring immediately
}


// void OrderSarNodesToReturn() {
//     Vector baseStationPosition = GetBaseStationPosition(); // Get the dynamically determined base station position

//     for (auto node : g_sarNodes) { // Assuming 'sarNodes' holds all SAR node pointers
//         Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
//         if (mob) {
//             mob->SetPosition(baseStationPosition); // Move SAR nodes dynamically to the base
//             NS_LOG_INFO(" SAR Node " << node->GetId() 
//                          << " returning to base at " << baseStationPosition);
//         }
//     }
// }
