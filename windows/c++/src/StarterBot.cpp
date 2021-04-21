#include "StarterBot.h"
#include "Tools.h"
#include "MapTools.h"

//Group Members: Giovanni Sylvestre(201724598), Kerolos Farag(201759057)

StarterBot::StarterBot()
{
    workerScout = Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Probe);
}

// Called when the bot starts!
void StarterBot::onStart()
{
	BWAPI::Broodwar->setLocalSpeed(0); //same as "/speed 10" within game, each frame of the game takes the set number of MS in this function prackets
    BWAPI::Broodwar->setFrameSkip(0); //skips rendering frames for more performance
    BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput); // Enable the flag that tells BWAPI top let users enter input while bot plays
    //enableFlag enables you to control units while bot is running
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
    // Update our MapTools information
    m_mapTools.onFrame();

    sendIdleWorkersToMinerals();

    positionIdleZealots();

    trainAdditionalWorkers();

    trainZealots();

    buildAdditionalSupply();

    buildGateway();

    buildCannon();

    getExpansionLoc();

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

// Send our idle workers to mine minerals so they don't just stand there
void StarterBot::sendIdleWorkersToMinerals()
{
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    for (auto& unit : myUnits)
    {
        // Check the unit type, if it is an idle worker, then we want to send it somewhere
        if ((unit->getType().isWorker() && unit->isIdle())|| unit->getType().isWorker() && unit->isStuck())
        {
            // Player starting position (depot position)
            BWAPI::Position startPos = Tools::GetDepot()->getPosition();
            // Get the closest mineral to this worker unit
            BWAPI::Unit closestMineralToBase = Tools::GetClosestUnitTo(startPos, allMinerals);

            // If a valid mineral was found, right click it with the unit in order to start harvesting
            if (closestMineralToBase) { unit->rightClick(closestMineralToBase); }
        }
    }
}

// Position Zealots depending on current game state
void StarterBot::positionIdleZealots()
{
    const BWAPI::Unitset& enemyUnits = BWAPI::Broodwar->enemy()->getUnits();
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    for (auto& unit : myUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot && unit->isCompleted()) { allZealots.insert(unit); }
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot && unit->isCompleted() && unit->getDistance(Tools::GetDepot()) < 200) { baseZealots.insert(unit); }
        bool idelZealot = (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot && (unit->isIdle() || unit->isHoldingPosition()));
        
        //if not attacking or under attack stay in Place
        if (idelZealot && !baseUnderattack() && !readyForAttack())
        {
            unit->holdPosition();
        }
        //if base is underattack, defend
        else if (baseUnderattack()) 
        {
            for (auto& unit : myUnits)
            {
                BWAPI::Unit closestEnemy = Tools::GetClosestUnitTo(Tools::GetDepot(), enemyUnits);
                if (!unit->isAttacking() && closestEnemy->getDistance(Tools::GetDepot()) < 400)
                {
                    unit->attack(closestEnemy);
                }
                else if (!unit->isAttacking() && closestEnemy->getDistance(Tools::GetDepot()) > 400)
                {
                    unit->move(Tools::GetDepot()->getPosition()); 
                }
            }
        }

        else if (!baseUnderattack() && readyForAttack() && baseZealots.size() >= 7)
        {
            attackZealots = allZealots;
            baseZealots.clear();
            attack();
        }
    }
}

// Return true if base is under attack
bool StarterBot::baseUnderattack() 
{
    const BWAPI::Unitset& enemyUnits = BWAPI::Broodwar->enemy()->getUnits();
    for (auto& unit : enemyUnits)
    {
        if (unit->getDistance(Tools::GetDepot()->getPosition()) < 400) { return true; }
    }
    return false; 
}

bool StarterBot::expansionUnderattack()
{
    const BWAPI::Unitset& enemyUnits = BWAPI::Broodwar->enemy()->getUnits();
    for (auto& unit : enemyUnits)
    {
        if (unit->getDistance(naturalExpansionPos) < 400) { return true; }
    }
    return false;
}

// Return true if current Zealot count is 1/4 our supply
bool StarterBot::readyForAttack() 
{
    int currentSupply = BWAPI::Broodwar->self()->supplyUsed()/2;

    if (allZealots.size() >= (currentSupply / 4)) 
    {
        return true; 
    }
    return false; 
}

void StarterBot::attack() 
{
    const BWAPI::Unitset& enemyUnits = BWAPI::Broodwar->enemy()->getUnits();
    BWAPI::Unitset enemyWorkers;
    BWAPI::Unitset enemyAttackers;
    BWAPI::Unitset enemyBuildings;
    BWAPI::Unitset otherEnemyUnits;

    for (auto& unit : enemyUnits)
    {
        if (unit->getDistance(enemyBasePos) < 700 && unit->getType().isWorker())
        {
            enemyWorkers.insert(unit);
        }
        if (unit->getDistance(enemyBasePos) < 700 && unit->canAttack() && !unit->getType().isWorker())
        {
            enemyAttackers.insert(unit);
        }
        if (unit->getDistance(enemyBasePos) < 700 && unit->getType().isBuilding())
        {
            enemyBuildings.insert(unit);
        }
        if (unit->getDistance(enemyBasePos) < 700)
        {
            otherEnemyUnits.insert(unit);
        }
    }
    
    for (auto& unit : attackZealots)
    {   
        if (baseUnderattack()) { unit->move(Tools::GetDepot()->getPosition()); }
        // if not yet at enemy base and not being attacked nor attacking then go attack
        else if (!atEnemyBase(unit) && !unit->isUnderAttack() && !unit->isAttacking())
        {
            unit->attack(enemyBasePos);
        }

        else if (atEnemyBase(unit)) 
        {
            if (!enemyAttackers.empty()) { unit->attack(Tools::GetClosestUnitTo(unit,enemyAttackers)); }
            else if (!enemyWorkers.empty()) { unit->attack(Tools::GetClosestUnitTo(unit, enemyWorkers)); }
            else if (!enemyBuildings.empty()) { unit->attack(Tools::GetClosestUnitTo(unit, enemyBuildings)); }
            else { unit->attack(Tools::GetClosestUnitTo(unit, otherEnemyUnits)); }

        }

    }
}


bool StarterBot::atEnemyBase(BWAPI::Unit unit)
{
    if (unit->getDistance(enemyBasePos) <= 200) { return true; }
    return false;
}

// Train more workers so we can gather more income
void StarterBot::trainAdditionalWorkers()
{
    const int workersWanted = 20;
    const int workersOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Probe, BWAPI::Broodwar->self()->getUnits());
    

    
    if (workersOwned < workersWanted )
    {
        // get the unit pointer to my depot
        const BWAPI::Unit myDepot = Tools::GetDepot();

        // if we have a valid depot unit and it's currently not training something, train a worker
        // there is no reason for a bot to ever use the unit queueing system, it just wastes resources
        if (myDepot && !myDepot->isTraining()) { myDepot->train(BWAPI::UnitTypes::Protoss_Probe); }
    }
}


void StarterBot::trainZealots()
{
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    for (auto& unit : myUnits)
    {
        //no limit on zealots production for now
        if (unit->getType()== BWAPI::UnitTypes::Protoss_Gateway)
        {
            if (!unit->isTraining()) { unit->train(BWAPI::UnitTypes::Protoss_Zealot); }
        }
    }
}

// Build more supply if we are going to run out soon
void StarterBot::buildAdditionalSupply()
{
    // Get the amount of supply supply we currently have unused
    const int unusedSupply = Tools::GetTotalSupply(true) - BWAPI::Broodwar->self()->supplyUsed();
    // Get the amount of minerals farmed
    int mineralsCount = BWAPI::Broodwar->self()->minerals(); 
    // If we have a sufficient amount of supply, we don't need to do anything
    if (unusedSupply >= 6 || mineralsCount < 101 ) { return; } // 2 here means 1 cause supply in game is * by 2

    // Otherwise, we are going to build a supply provider
    const bool startedBuilding = Tools::BuildBuilding(BWAPI::UnitTypes::Protoss_Pylon);
    if (startedBuilding)
    {
        BWAPI::Broodwar->printf("Started Building %s", BWAPI::UnitTypes::Protoss_Pylon.getName().c_str());
    }
}

void StarterBot::buildGateway() 
{
    int mineralsCount = BWAPI::Broodwar->self()->minerals(); // Get the amount of minerals farmed

    int gateWaysOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, BWAPI::Broodwar->self()->getUnits());

    if (gateWaysOwned < 2 && mineralsCount >= 151)
    {
        const bool startedBuilding = Tools::BuildBuilding(BWAPI::UnitTypes::Protoss_Gateway);
        if (startedBuilding) {BWAPI::Broodwar->printf("Started Building %s", BWAPI::UnitTypes::Protoss_Gateway.getName().c_str());}
    }
}

void StarterBot::buildCannon() 
{
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    int gatewaysOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, myUnits);
    int forgeOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Forge, myUnits);
    int cannonsOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Photon_Cannon, myUnits);
    if (gatewaysOwned >= 1 && forgeOwned < 1)
    {
        const bool startedBuilding = Tools::BuildBuilding(BWAPI::UnitTypes::Protoss_Forge);
        if (startedBuilding) { BWAPI::Broodwar->printf("Started Building %s", BWAPI::UnitTypes::Protoss_Forge.getName().c_str()); }
    }
    if (forgeOwned >= 1 && cannonsOwned < 2) 
    {
        const bool startedBuilding = Tools::buildBuilding(BWAPI::UnitTypes::Protoss_Photon_Cannon, BWAPI::Broodwar->self()->getStartLocation(), 10);//, Tools::GetDepot()->getTilePosition());
        if (startedBuilding) { BWAPI::Broodwar->printf("Started Building %s", BWAPI::UnitTypes::Protoss_Photon_Cannon.getName().c_str()); }
    }
}

// builds a new nexus and records the new expansion location
void StarterBot::getExpansionLoc() 
{
    int nexusCount = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Nexus, BWAPI::Broodwar->self()->getUnits());
    allMinerals = BWAPI::Broodwar->getMinerals();
    const int workersOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Probe, BWAPI::Broodwar->self()->getUnits());
    if (workersOwned < 19) { return; }
    if (enemyBasePos != nullPos) 
    {
        for (auto& mineral : allMinerals) 
        {
            if (nexusCount == 2) { break; }
            if (mineral->getDistance(Tools::GetDepot()) < 235) { allMinerals.erase(mineral); }
        }
    }
    int mineralsCount = BWAPI::Broodwar->self()->minerals();
    if (mineralsCount > 400 && nexusCount < 2)
    {
        BWAPI::Unit closestMineralToBase = Tools::GetClosestUnitTo(Tools::GetDepot()->getPosition(), allMinerals);
        BWAPI::TilePosition tp = closestMineralToBase->getTilePosition();
        BWAPI::Position pos(tp);
        const bool startedBuilding = Tools::buildBuilding(BWAPI::UnitTypes::Protoss_Nexus, tp, 30);
        naturalExpansionPos = pos;
        naturalExpansionTP = tp;
        if (Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Nexus, BWAPI::Broodwar->self()->getUnits()) == 2)
        {
            allMinerals = BWAPI::Broodwar->getMinerals();
        }

    }
}


void StarterBot::scoutEnemy()
{
    // start scouting when our workers count is => 9
    const int workersOwned = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Probe, BWAPI::Broodwar->self()->getUnits());
    if (workersOwned < 9) { return; }

    if(!workerScout->exists())
    {
        // if scout died, assign new scout
        workerScout = Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Probe);
    }


    auto& startLocations = BWAPI::Broodwar->getStartLocations(); // searching all start positions
    for (BWAPI::TilePosition tilePos : startLocations)
    {
        if (BWAPI::Broodwar->isExplored(tilePos)) { continue; }
        BWAPI::Position pos(tilePos);
        // send scout to tilepos
        workerScout->move(pos);

        if (foundEnemyBase() && enemyBasePos == nullPos)
        {
            enemyBasePos = pos;
            BWAPI::Broodwar->printf("found enemy base");
        }

        break;
    }

}

bool StarterBot::foundEnemyBase()
{
    bool baseFound = false;
    for (auto unit : BWAPI::Broodwar->enemy()->getUnits())
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