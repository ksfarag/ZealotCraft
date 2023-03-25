#include "StarterBot.h"
#include "Tools.h"
#include "MapTools.h"

StarterBot::StarterBot()
{
    workerScout = Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Probe);
}

// Called when the bot starts
void StarterBot::onStart()
{
	BWAPI::Broodwar->setLocalSpeed(0); 
    BWAPI::Broodwar->setFrameSkip(0);
    BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput); 
    m_mapTools.onStart();
}

// Called whenever the game ends and tells you if you won or not
void StarterBot::onEnd(bool isWinner) 
{
    std::cout << "We " << (isWinner ? "won!" : "lost!") << "\n";
}

// Called on each frame of the game
void StarterBot::onFrame()
{
    myUnits = BWAPI::Broodwar->self()->getUnits();
    m_mapTools.onFrame();

    sendWorkersToGas();

    sendIdleWorkersToMinerals();

    positionIdleZealots();

    trainAdditionalWorkers();

    trainDragoons();

    trainZealots();

    upgradeUnits();

    buildAdditionalSupply();

    buildGateways();

    buildCannon();

    buildAssimilator();

    buildCyberCore();

    buildExpansionBuildings();

    buildExpansionNexus();

    scoutEnemy();

    Tools::DrawUnitHealthBars();

    drawDebugInformation();

    debug();
}

void StarterBot::debug()
{ 
    int k = workerScout->getDistance(Tools::GetDepot());
    auto& pos = workerScout->getPosition();
    BWAPI::Broodwar->drawCircleMap(pos, 8, BWAPI::Colors::Red, true);
}

// Sends idle workers to gather resources
void StarterBot::sendIdleWorkersToMinerals()
{
    // Get all units in the main base
    BWAPI::Unitset mainBaseWorkers = Tools::GetDepot()->getUnitsInRadius(400);

    int mainBaseWorkerCount = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Probe, mainBaseWorkers);
    mineralFarmersCount = 0;

    for (auto& unit : myUnits)
    {
        if (unit->isGatheringMinerals()) { mineralFarmersCount++; }
    }

    BWAPI::Position mainBasePosition = Tools::GetDepot()->getPosition();
    BWAPI::Unit closestMineralToMainBase = Tools::GetClosestUnitTo(mainBasePosition, allMinerals);

    for (auto& unit : myUnits)
    {
        bool isIdleWorker = ((unit->getType().isWorker() && unit->isIdle()) || (unit->getType().isWorker() && unit->isStuck()));
        if (isIdleWorker)
        {
            // Send idle workers to close minerals to original nexus if we didn't expand yet
            if (Tools::GetNewDepot == nullptr || mainBaseWorkerCount < 20)
            {
                if (closestMineralToMainBase) { unit->rightClick(closestMineralToMainBase); }
            }
            // Send idle workers to farm at expansion if # of workers in main base is > 20
            else if (mainBaseWorkerCount > 20)
            {
                // Find the closest mineral to the expansion base
                BWAPI::Position expansionPosition = Tools::GetNewDepot()->getPosition();
                BWAPI::Unit closestMineralToExpansion = Tools::GetClosestUnitTo(expansionPosition, allMinerals);

                if (closestMineralToExpansion) { unit->rightClick(closestMineralToExpansion); }
            }
        }
        if (mineralFarmersCount < gasFarmersCount || mineralFarmersCount <= 1)
        {
            if (closestMineralToMainBase) { unit->rightClick(closestMineralToMainBase); }
        }
    }
}


// Sends a few idle workers to gather gas
void StarterBot::sendWorkersToGas()
{    
    BWAPI::Unit assimilator = Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Assimilator);
    gasFarmersCount = 0;
    for (auto& unit : myUnits)
    {
        if (unit->isGatheringGas()) { gasFarmersCount++; }
    }

    // Send idle workers to gather gas if there are less than 3 workers already doing so
    for (auto& unit : myUnits)
    {
        bool isIdleWorker = ((unit->getType().isWorker() && unit->isIdle()) || (unit->getType().isWorker() && unit->isStuck()));
        if (isIdleWorker && gasFarmersCount < 3 && assimilator || mineralFarmersCount >= 20 && gasFarmersCount < 3 && assimilator)
        {
            unit->rightClick(assimilator);
        }
    }
}

// Position Zealots depending on current game state
void StarterBot::positionIdleZealots()
{
    const BWAPI::Unitset& enemyUnits = BWAPI::Broodwar->enemy()->getUnits();
    const int maxDistanceToBase = 600;
    const int maxDistanceToNewBase = 400;

    BWAPI::Unitset closeEnemyUnits;

    for (auto& enemyUnit : enemyUnits)
    {
        if (enemyUnit->getDistance(Tools::GetDepot()) <= maxDistanceToBase ||
            enemyUnit->getDistance(Tools::GetNewDepot()) <= maxDistanceToNewBase)
        {
            closeEnemyUnits.insert(enemyUnit);
        }
    }

    // Get enemy units close to our base.
    BWAPI::Unitset baseFighters;

    for (auto& myUnit : myUnits)
    {
        const BWAPI::UnitType unitType = myUnit->getType();
        const bool isZealot = unitType == BWAPI::UnitTypes::Protoss_Zealot;
        const bool isDragoon = unitType == BWAPI::UnitTypes::Protoss_Dragoon;

        if ((isZealot || isDragoon) && myUnit->isCompleted())
        {
            allFighters.insert(myUnit);

            if ((isZealot && myUnit->getDistance(Tools::GetDepot()) <= maxDistanceToBase) ||
                (isDragoon && myUnit->getDistance(Tools::GetDepot()) <= maxDistanceToBase))
            {
                baseFighters.insert(myUnit);
            }
        }

        const bool isIdleAttacker = (isZealot || isDragoon) && (myUnit->isIdle() || myUnit->isHoldingPosition());
        const int minAttackerCount = 7;

        // If not attacking or under attack stay in Place.
        if (isIdleAttacker && !isBaseUnderAttack() && !isReadyForAttack() && !isExpansionUnderAttack())
        {
            if (Tools::GetNewDepot() == nullptr)
            {
                myUnit->holdPosition();
            }
            else
            {
                myUnit->move(Tools::GetNewDepot()->getPosition());
            }
        }
        else if (isBaseUnderAttack() || isExpansionUnderAttack())
        {
            const BWAPI::Unit closestEnemy = Tools::GetClosestUnitTo(myUnit, closeEnemyUnits);

            // Attack enemies close from base or expansion base
            if (closestEnemy->isDetected() && !myUnit->isAttacking() &&
                (closestEnemy->getDistance(Tools::GetDepot()) <= maxDistanceToBase ||
                    closestEnemy->getDistance(Tools::GetNewDepot()) <= maxDistanceToNewBase))
            {
                if (unitType != BWAPI::UnitTypes::Protoss_Dragoon)
                {
                    myUnit->attack(closestEnemy);
                }
            }

            // If enemy is getting away, don't chase and go back to base
            else if (closestEnemy->getDistance(Tools::GetDepot()) > maxDistanceToBase && closestEnemy->getDistance(Tools::GetNewDepot()) > maxDistanceToNewBase)
            {
                myUnit->move(Tools::GetDepot()->getPosition());
            }
        }

        // Call attack() if we have enough units for an attack and we are not under attack
        else if (!isBaseUnderAttack() && !isExpansionUnderAttack() && isReadyForAttack() && baseFighters.size() >= minAttackerCount)
        {
            baseFighters.clear();
            attack();
        }
    }
}



// Returns true if the base is under attack
bool StarterBot::isBaseUnderAttack()
{
    const BWAPI::Unitset& enemyUnits = BWAPI::Broodwar->enemy()->getUnits();
    const int maxDistanceToBase = 600;
    for (auto& enemyUnit : enemyUnits)
    {
        if (enemyUnit->getDistance(Tools::GetDepot()->getPosition()) < maxDistanceToBase)
        {
            return true;
        }
    }
    return false;
}


// Returns true if the natural expansion is under attack
bool StarterBot::isExpansionUnderAttack()
{
    const BWAPI::Unitset& enemyUnits = BWAPI::Broodwar->enemy()->getUnits();
    const int maxDistanceToExpansion = 600;
    const BWAPI::Unit newDepot = Tools::GetNewDepot();
    if (newDepot == nullptr)
    {
        return false;
    }
    for (auto& enemyUnit : enemyUnits)
    {
        if (enemyUnit->getDistance(newDepot) < maxDistanceToExpansion)
        {
            return true;
        }
    }
    return false;
}


//Returns true if we have enough attacking units ready for an attack
bool StarterBot::isReadyForAttack()
{
    const int earlyGameAttackUnitCount = 9;
    const int lateGameAttackUnitCount = 15;
    const int attackingUnitCount = allFighters.size();

    if (!attackPerformed && attackingUnitCount >= earlyGameAttackUnitCount)
    {
        return true;
    }
    else if (attackingUnitCount >= lateGameAttackUnitCount)
    {
        return true;
    }
    return false;
}

// Function that performs attacks on enemy base location
void StarterBot::attack()
{
    const BWAPI::Unitset& enemyUnits = BWAPI::Broodwar->enemy()->getUnits();

    // Unit sets containing different enemy unit types to help bot decide what unit type to attack first
    BWAPI::Unitset enemyWorkers;
    BWAPI::Unitset enemyAttackers;
    BWAPI::Unitset enemyBuildings;
    BWAPI::Unitset otherEnemyUnits;

    const int attackRange = 700; // Range to attack enemy units

    attackPerformed = true;

    for (auto& unit : enemyUnits)
    {
        if (unit->getDistance(enemyBasePos) < attackRange && unit->getType().isWorker())
        {
            enemyWorkers.insert(unit);
        }

        if (unit->getDistance(enemyBasePos) < attackRange && unit->canAttack() && !unit->getType().isWorker())
        {
            enemyAttackers.insert(unit);
        }

        if (unit->getDistance(enemyBasePos) < attackRange && unit->getType().isBuilding())
        {
            enemyBuildings.insert(unit);
        }

        if (unit->getDistance(enemyBasePos) < attackRange)
        {
            otherEnemyUnits.insert(unit);
        }
    }

    for (auto& unit : allFighters)
    {
        // If base or expansion is under attack, retreat
        if (isBaseUnderAttack() || isExpansionUnderAttack())
        {
            unit->move(Tools::GetDepot()->getPosition());
        }
        // If not yet at enemy base and not being attacked nor attacking then go attack
        else if (!atEnemyBase(unit) && !unit->isUnderAttack() && !unit->isAttacking())
        {
            unit->attack(enemyBasePos);
        }

        else if (atEnemyBase(unit))
        {
            if (!enemyAttackers.empty()) { unit->attack(Tools::GetClosestUnitTo(unit, enemyAttackers)); }
            else if (!enemyWorkers.empty()) { unit->attack(Tools::GetClosestUnitTo(unit, enemyWorkers)); }
            else if (!enemyBuildings.empty()) { unit->attack(Tools::GetClosestUnitTo(unit, enemyBuildings)); }
            else { unit->attack(Tools::GetClosestUnitTo(unit, otherEnemyUnits)); }

        }

    }

    allFighters.clear();
}


// Returns true if our attacking units have reached the enemy base
bool StarterBot::atEnemyBase(BWAPI::Unit unit)
{
    const int distanceThreshold = 300;
    return unit->getDistance(enemyBasePos) <= distanceThreshold;
}

// Train more workers so we can gather more income
void StarterBot::trainAdditionalWorkers()
{
    const int kGatewayThreshold = 3;
    const int kMinWorkers = 20;
    const int kMaxWorkers = 45;

    // Calculate how many workers we want to have
    int numGateways = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, myUnits);
    int numWorkersWanted = (numGateways < kGatewayThreshold) ? kMinWorkers : kMaxWorkers;
    const int numWorkersOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Probe, myUnits);

    // Train additional workers if we don't have enough
    if (numWorkersOwned < numWorkersWanted)
    {
        const BWAPI::Unit mainDepot = Tools::GetDepot();
        const BWAPI::Unit expansionDepot = Tools::GetNewDepot();

        // Train a worker from the main depot if possible
        if (mainDepot && !mainDepot->isTraining())
        {
            mainDepot->train(BWAPI::UnitTypes::Protoss_Probe);
        }

        // Train a worker from the expansion depot if possible
        if (expansionDepot && !expansionDepot->isTraining())
        {
            expansionDepot->train(BWAPI::UnitTypes::Protoss_Probe);
        }
    }
}

// Function that creates dragoons and balances the number of dragoons and zealots
void StarterBot::trainDragoons()
{
    const int kDesiredRatio = 1; // 1 dragoons per zealot
    int zealotCount = 0;
    int dragoonCount = 0;

    // Count how many zealots and dragoons we have
    for (auto& unit : myUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot)
        {
            zealotCount++;
        }
        else if (unit->getType() == BWAPI::UnitTypes::Protoss_Dragoon)
        {
            dragoonCount++;
        }
    }

    // Train dragoons until we have the desired ratio of dragoons to zealots
    for (auto& unit : myUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Gateway)
        {
            if (!unit->isTraining() && zealotCount * kDesiredRatio > dragoonCount)
            {
                unit->train(BWAPI::UnitTypes::Protoss_Dragoon);
            }
        }
    }
}

// Function that creates zealots
void StarterBot::trainZealots()
{
    for (auto& unit : myUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Gateway)
        {
            if (!unit->isTraining())
            {
                unit->train(BWAPI::UnitTypes::Protoss_Zealot);
            }
        }
    }
}

// Function that upgrades ground units
void StarterBot::upgradeUnits()
{
    BWAPI::Unit forge = Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Forge);
    BWAPI::Unit cyberCore = Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
    BWAPI::Unit expansionDepot = Tools::GetNewDepot();

    const int mineralThreshold = 200;
    int mineralsCount = BWAPI::Broodwar->self()->minerals();

    if (expansionDepot != nullptr && mineralsCount > mineralThreshold)
    {
        if (forge && forge->canUpgrade(BWAPI::UpgradeTypes::Protoss_Ground_Weapons))
        {
            forge->upgrade(BWAPI::UpgradeTypes::Protoss_Ground_Weapons);
        }
        if (forge && forge->canUpgrade(BWAPI::UpgradeTypes::Protoss_Ground_Armor))
        {
            forge->upgrade(BWAPI::UpgradeTypes::Protoss_Ground_Armor);
        }
        if (forge && forge->canUpgrade(BWAPI::UpgradeTypes::Protoss_Plasma_Shields))
        {
            forge->upgrade(BWAPI::UpgradeTypes::Protoss_Plasma_Shields);
        }
        if (cyberCore && cyberCore->canUpgrade(BWAPI::UpgradeTypes::Singularity_Charge))
        {
            cyberCore->upgrade(BWAPI::UpgradeTypes::Singularity_Charge);
        }
    }
}

// Builds more supply if we are going to run out soon
void StarterBot::buildAdditionalSupply()
{
    // Get the amount of supply we currently have unused
    const int unusedSupply = Tools::GetTotalSupply(true) - BWAPI::Broodwar->self()->supplyUsed();

    // Get the amount of minerals farmed
    int mineralsCount = BWAPI::Broodwar->self()->minerals();

    // If we have a sufficient amount of supply, we don't need to do anything
    const int supplyThreshold = 6;
    const int mineralThreshold = 101;
    if (unusedSupply >= supplyThreshold || mineralsCount < mineralThreshold)
    {
        return;
    }

    // Otherwise, we are going to build a supply provider
    const bool startedBuilding = Tools::BuildBuilding(BWAPI::UnitTypes::Protoss_Pylon);
    if (startedBuilding)
    {
        BWAPI::Broodwar->printf("Started building %s", BWAPI::UnitTypes::Protoss_Pylon.getName().c_str());
    }
}

// Builds 2 Gateways at main base to produce attacking units
void StarterBot::buildGateways()
{
    int mineralsCount = BWAPI::Broodwar->self()->minerals();

    const int gatewayThreshold = 2;
    const int mineralThreshold = 151;
    int gatewaysOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, myUnits);

    if (gatewaysOwned < gatewayThreshold && mineralsCount >= mineralThreshold)
    {
        const bool startedBuilding = Tools::BuildBuilding(BWAPI::UnitTypes::Protoss_Gateway);
        if (startedBuilding)
        {
            BWAPI::Broodwar->printf("Started building %s", BWAPI::UnitTypes::Protoss_Gateway.getName().c_str());
        }
    }
}


// Builds a cannon at main base to spot hidden units and flying units
void StarterBot::buildCannon() {
    int gatewaysOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, myUnits);
    int forgeOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Forge, myUnits);
    int cannonsOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Photon_Cannon, myUnits);
    BWAPI::Unit expansionNexus = Tools::GetNewDepot();
    const int maxCannons = 2;
    const int buildDistance = 10;

    if (gatewaysOwned >= 1 && forgeOwned < 1 && expansionNexus) {
        const bool startedBuilding = Tools::BuildBuilding(BWAPI::UnitTypes::Protoss_Forge);
        if (startedBuilding) {
            BWAPI::Broodwar->printf("Started Building %s", BWAPI::UnitTypes::Protoss_Forge.getName().c_str());
        }
    }

    if (forgeOwned >= 1 && cannonsOwned < maxCannons) {
        const bool startedBuilding = Tools::buildBuilding(BWAPI::UnitTypes::Protoss_Photon_Cannon,
            BWAPI::Broodwar->self()->getStartLocation(), buildDistance);
        if (startedBuilding) {
            BWAPI::Broodwar->printf("Started Building %s", BWAPI::UnitTypes::Protoss_Photon_Cannon.getName().c_str());
        }
    }
}

// Build an Assimilator at main base for gas extraction
void StarterBot::buildAssimilator() {
    int workersOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Probe, myUnits);
    const int minWorkersForAssimilator = 15;
    const int buildDistance = 10;

    if (workersOwned > minWorkersForAssimilator) {
        const bool startedBuilding = Tools::buildBuilding(BWAPI::UnitTypes::Protoss_Assimilator,
            BWAPI::Broodwar->self()->getStartLocation(), buildDistance);
        if (startedBuilding) {
            BWAPI::Broodwar->printf("Started Building %s", BWAPI::UnitTypes::Protoss_Assimilator.getName().c_str());
        }
    }
}

// Build a Cybernetics Core that allows the bot to produce dragoons as well as upgrade them late in game
void StarterBot::buildCyberCore() {
    int gatewaysOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, myUnits);
    int cyberCoresOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Cybernetics_Core, myUnits);
    const int buildDistance = 10;

    if (gatewaysOwned >= 2 && cyberCoresOwned < 1) {
        const bool startedBuilding = Tools::buildBuilding(BWAPI::UnitTypes::Protoss_Cybernetics_Core,
            BWAPI::Broodwar->self()->getStartLocation(), buildDistance);
        if (startedBuilding) {
            BWAPI::Broodwar->printf("Started Building %s", BWAPI::UnitTypes::Protoss_Cybernetics_Core.getName().c_str());
        }
    }
}

// Function that manages and builds the natural expansion buildings
void StarterBot::buildExpansionBuildings()
{
    BWAPI::Unit expansionNexus = Tools::GetNewDepot();
    if (expansionNexus == nullptr) 
    {
        // If no nexus unit found, return
        return;
    }

    int mineralsCount = BWAPI::Broodwar->self()->minerals();
    const int expansionRadius = 350;
    const int minMineralsBeforeExtraGateways = 900;
    const int buildRange = 28;
    BWAPI::Unitset expansionUnits = expansionNexus->getUnitsInRadius(expansionRadius);
    int numberOfPylons = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Pylon, expansionUnits);
    int numberOfGateways = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, expansionUnits);
    BWAPI::TilePosition nexusTile = expansionNexus->getTilePosition();

    // Build pylon if there are none at the expansion base
    if (numberOfPylons < 1) {
        bool startedBuilding = Tools::buildBuilding(BWAPI::UnitTypes::Protoss_Pylon, nexusTile, buildRange);
        if (startedBuilding) {
            BWAPI::Broodwar->printf("Building at Expansion Base Pylon: %s", BWAPI::UnitTypes::Protoss_Pylon.getName().c_str());
        }
    }
    // Build gateway if there is at least 1 pylon and less than 2 gateways at the expansion base
    else if (numberOfPylons > 0 && numberOfGateways < 2) {
        bool startedBuilding = Tools::buildBuilding(BWAPI::UnitTypes::Protoss_Gateway, nexusTile, buildRange);
        if (startedBuilding) {
            BWAPI::Broodwar->printf("Building at Expansion Base Gateway: %s", BWAPI::UnitTypes::Protoss_Gateway.getName().c_str());
        }
    }
    // Build gateway if there are less than 7 gateways at the expansion base and the player's mineral count is above a certain threshold
    else if (mineralsCount > minMineralsBeforeExtraGateways && numberOfGateways < 7) {
        bool startedBuilding = Tools::buildBuilding(BWAPI::UnitTypes::Protoss_Gateway, nexusTile, buildRange);
        if (startedBuilding) {
            BWAPI::Broodwar->printf("Building at Expansion Base Gateway: %s", BWAPI::UnitTypes::Protoss_Gateway.getName().c_str());
        }
    }
}


// Builds a new nexus
void StarterBot::buildExpansionNexus()
{
    const int maxNexusCount = 2;
    const int nexusMineralDistance = 235;
    const int maxWorkersOwned = 20;
    const int minMineralsCountToBuildNexus = 400;
    const int buildNexusAtTileDistance = 18;

    int nexusCount = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Nexus, myUnits);
    if (nexusCount >= maxNexusCount)
    {
        return;
    }

    allMinerals = BWAPI::Broodwar->getMinerals();
    const int workersOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Probe, myUnits);
    const bool isUnderAttack = attackPerformed == false;
    if (workersOwned < maxWorkersOwned || isUnderAttack)
    {
        return;
    }

    if (enemyBasePos != nullPos)
    {
        for (auto& mineral : allMinerals)
        {
            if (mineral->getDistance(Tools::GetDepot()) < 235) { allMinerals.erase(mineral); }
        }
    }

    int mineralsCount = BWAPI::Broodwar->self()->minerals();
    if (mineralsCount > minMineralsCountToBuildNexus && nexusCount < maxNexusCount)
    {
        BWAPI::Unit closestMineralToBase = Tools::GetClosestUnitTo(Tools::GetDepot()->getPosition(), allMinerals);
        BWAPI::TilePosition tp = closestMineralToBase->getTilePosition();
        const bool startedBuilding = Tools::buildBuilding(BWAPI::UnitTypes::Protoss_Nexus, tp, buildNexusAtTileDistance);
        if (Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Nexus, myUnits) == maxNexusCount)
        {
            allMinerals = BWAPI::Broodwar->getMinerals();
        }
    }
}

// Function that sends one of the workers to scout
void StarterBot::scoutEnemy()
{
    // Start scouting when our workers count is 9 or more
    const int workersOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Probe, myUnits);
    const int minWorkersBeforeScouting = 9;
    if (workersOwned < minWorkersBeforeScouting) { return; }

    if (!workerScout->exists())
    {
        // If scout died, assign new scout
        workerScout = Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Probe);
    }

    auto& startLocations = BWAPI::Broodwar->getStartLocations(); // Searching all start positions
    for (BWAPI::TilePosition tilePos : startLocations)
    {
        if (BWAPI::Broodwar->isExplored(tilePos)) { continue; }
        BWAPI::Position pos(tilePos);
        // Send scout to tilepos
        workerScout->move(pos);

        if (foundEnemyBase() && enemyBasePos == nullPos)
        {
            enemyBasePos = pos;
            BWAPI::Broodwar->printf("Found enemy base.");
        }

        break;
    }
}

// Return true if scout has found the enemy base
bool StarterBot::foundEnemyBase()
{
    bool baseFound = false;
    for (auto& unit : BWAPI::Broodwar->enemy()->getUnits())
    {
        if (unit->getType().isBuilding()) { baseFound = true; break; }
    }
    return baseFound;
}


// Draw some relevent information to the screen to help us debug the bot
void StarterBot::drawDebugInformation()
{
    BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 10), "\n");
    Tools::DrawUnitCommands();
    Tools::DrawUnitBoundingBoxes();
}

// Called whenever a unit is destroyed, with a pointer to the unit
void StarterBot::onUnitDestroy(BWAPI::Unit unit)
{
	
}

// Called whenever a unit is morphed, with a pointer to the unit
// Zerg units morph when they turn into other units
void StarterBot::onUnitMorph(BWAPI::Unit unit)
{
	
}

// Called whenever a text is sent to the game by a user
void StarterBot::onSendText(std::string text) 
{ 
    if (text == "/map") { m_mapTools.toggleDraw(); }
    // hotkeys to set game speed via chat
    if (text == "`") { BWAPI::Broodwar->setLocalSpeed(0); }
    if (text == "1") { BWAPI::Broodwar->setLocalSpeed(5); }
    if (text == "2") { BWAPI::Broodwar->setLocalSpeed(20); }
    if (text == "3") { BWAPI::Broodwar->setLocalSpeed(30); }
    if (text == "4") { BWAPI::Broodwar->setLocalSpeed(50); }
}

// Called whenever a unit is created, with a pointer to the destroyed unit
// Units are created in buildings like barracks before they are visible, 
// so this will trigger when you issue the build command for most units
void StarterBot::onUnitCreate(BWAPI::Unit unit)
{ 
	
}

// Called whenever a unit finished construction, with a pointer to the unit
void StarterBot::onUnitComplete(BWAPI::Unit unit)
{
	
}

// Called whenever a unit appears, with a pointer to the destroyed unit
// This is usually triggered when units appear from fog of war and become visible
void StarterBot::onUnitShow(BWAPI::Unit unit)
{ 
	
}

// Called whenever a unit gets hidden, with a pointer to the destroyed unit
// This is usually triggered when units enter the fog of war and are no longer visible
void StarterBot::onUnitHide(BWAPI::Unit unit)
{ 
	
}

// Called whenever a unit switches player control
// This usually happens when a dark archon takes control of a unit
void StarterBot::onUnitRenegade(BWAPI::Unit unit)
{ 
	
}