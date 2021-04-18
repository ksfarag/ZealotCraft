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
    // Set our BWAPI options here    
	BWAPI::Broodwar->setLocalSpeed(0); //same as "/speed 10" within game, each frame of the game takes the set number of MS in this function prackets
    // if we setLocalSpeed(0), the game runs as fast as possible
    BWAPI::Broodwar->setFrameSkip(0); //skips rendering frames for more performance
    
    BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput); // Enable the flag that tells BWAPI top let users enter input while bot plays
    //enableFlag enables you to control units while bot is running
    // Call MapTools OnStart
    m_mapTools.onStart();
}

// Called whenever the game ends and tells you if you won or not
void StarterBot::onEnd(bool isWinner) 
{
    std::cout << "We " << (isWinner ? "won!" : "lost!") << "\n";
    //you can appened the game result to a file from this function to record win/lose statistics 
}

// Called on each frame of the game
void StarterBot::onFrame()
{
    // Update our MapTools information
    m_mapTools.onFrame();

    scoutEnemy();

    // Send our idle workers to mine minerals so they don't just stand there
    sendIdleWorkersToMinerals();

    // Position idle zealots either for base defense or attacking enemy
    positionIdleZealots();

    // Train more workers so we can gather more income
    trainAdditionalWorkers();

    // Train more zealots
    trainZealots();

    // Build more supply if we are going to run out soon
    buildAdditionalSupply();

    // Build a gateway to produce zealots
    buildGateway();

    // Draw unit health bars, which brood war unfortunately does not do
    Tools::DrawUnitHealthBars();

    // Draw some relevent information to the screen to help us debug the bot
    drawDebugInformation();
    debug();
}

void StarterBot::debug()
{

}

// Send our idle workers to mine minerals so they don't just stand there
void StarterBot::sendIdleWorkersToMinerals()
{
    // Let's send all of our starting workers to the closest mineral to them
    // First we need to loop over all of the units that we (BWAPI::Broodwar->self()) own
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    for (auto& unit : myUnits)
    {
        // Check the unit type, if it is an idle worker, then we want to send it somewhere
        if (unit->getType().isWorker() && unit->isIdle())
        {
            // Player starting position (depot position)
            BWAPI::Position startPos = Tools::GetDepot()->getPosition();
            // Get the closest mineral to this worker unit
            BWAPI::Unit closestMineralToBase = Tools::GetClosestUnitTo(startPos, BWAPI::Broodwar->getMinerals());

            // If a valid mineral was found, right click it with the unit in order to start harvesting
            if (closestMineralToBase) { unit->rightClick(closestMineralToBase); }
        }
    }
}

// Position Zealots depending on current game state
void StarterBot::positionIdleZealots()
{
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    for (auto& unit : myUnits)
    {
        bool idelZealot = (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot && (unit->isIdle() || unit->isHoldingPosition()));
        //if not attacking or under attack stay in base to defend (patrol)
        if (idelZealot && !underattack() && !attacking())
        {
            BWAPI::Position startPos = Tools::GetDepot()->getPosition();
            unit->patrol(startPos);

        }
        
        //else if (underattack()) {defend}
        
        //else if (attack()) {move unit to attack}
    }
}

// Return true if base is under attack
bool StarterBot::underattack() { return false; }

// Return true if attacking enemy base
bool StarterBot::attacking() { return false; }


// Train more workers so we can gather more income
void StarterBot::trainAdditionalWorkers()
{
    const BWAPI::UnitType workerType = BWAPI::Broodwar->self()->getRace().getWorker();
    const int workersWanted = 20;
    const int workersOwned = Tools::CountUnitsOfType(workerType, BWAPI::Broodwar->self()->getUnits());
    
    // Pause training once at the start of the game to allow building a pylon when we have 8/9 workers
    bool pauseTraining = false;
    if (Tools::GetTotalSupply(true) == 18 && BWAPI::Broodwar->self()->supplyUsed() == 16) { pauseTraining = true; BWAPI::Broodwar->printf("Training paused!!!");}
    else { pauseTraining = false; }
    
    if (workersOwned < workersWanted && !pauseTraining)
    {
        // get the unit pointer to my depot
        const BWAPI::Unit myDepot = Tools::GetDepot();

        // if we have a valid depot unit and it's currently not training something, train a worker
        // there is no reason for a bot to ever use the unit queueing system, it just wastes resources
        if (myDepot && !myDepot->isTraining()) { myDepot->train(workerType); }
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
    if (unusedSupply >= 4 || mineralsCount < 101 ) { return; } // 2 here means 1 cause supply in game is * by 2

    // Otherwise, we are going to build a supply provider
    const BWAPI::UnitType supplyProviderType = BWAPI::Broodwar->self()->getRace().getSupplyProvider();

    const bool startedBuilding = Tools::BuildBuilding(supplyProviderType);
    if (startedBuilding)
    {
        BWAPI::Broodwar->printf("Started Building %s", supplyProviderType.getName().c_str());
    }
}

// Build a gateway
void StarterBot::buildGateway() 
{
    // Get the amount of minerals farmed
    int mineralsCount = BWAPI::Broodwar->self()->minerals();
    const BWAPI::UnitType gateWay = BWAPI::UnitTypes::Protoss_Gateway;

    int gateWaysOwned = Tools::CountUnitsOfType(gateWay, BWAPI::Broodwar->self()->getUnits());

    if (gateWaysOwned < 2 && mineralsCount >= 151)
    {
        const bool startedBuilding = Tools::BuildBuilding(gateWay);

        if (startedBuilding)
        {
            BWAPI::Broodwar->printf("Started Building %s", gateWay.getName().c_str());
        }
    }
}


void StarterBot::scoutEnemy()
{
    if(!workerScout->exists())
    {
        //if scout died, we can assign another scout in next line
        //TODO: limit scout production in future
        workerScout = Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Probe);
    }

    //searching all start positions
    //startLocations = BWAPI::Broodwar->getStartLocations();
    for (BWAPI::TilePosition tilePos : startLocations)
    {
        //if (BWAPI::Broodwar->isExplored(tilePos)) { continue; }
        BWAPI::Position pos(tilePos);
        //send scout to tilepos
        workerScout->move(pos);
        //if the scout reach that position, remove it from the list to not rescout for this time
        if (workerScout->getDistance(pos) < 200) { startLocations.pop_front(); }
        //if list is empty, refill it with startign locations to re-scout 
        if (startLocations.empty()) { startLocations = BWAPI::Broodwar->getStartLocations(); }

        bool enemyFound = AtEnemyBase();

        if (enemyFound) 
        {
            enemyBasePos = pos;
        }

        break;
    }

}


bool StarterBot::AtEnemyBase()
{
    BWAPI::UnitType enemySupplyType = BWAPI::Broodwar->enemy()->getRace().getSupplyProvider();

    bool baseFound = false;

    for (auto unit : BWAPI::Broodwar->enemy()->getUnits())
    {
        //checking if unit is enemy supply unit
        BWAPI::UnitType enemyType = unit->getType();
        if (enemyType == enemySupplyType) 
        {
            baseFound = true;
            break;
        }

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
    if (text == "/map")
    {
        m_mapTools.toggleDraw();
    }
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