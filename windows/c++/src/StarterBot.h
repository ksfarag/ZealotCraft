#pragma once

#include "MapTools.h"

#include <BWAPI.h>

class StarterBot
{
    MapTools m_mapTools;

	BWAPI::Unit workerScout;
	BWAPI::Position nullPos = { -1,-1 };
	BWAPI::Position enemyBasePos = nullPos;
	BWAPI::Position startingPos;
	BWAPI::Position naturalExpansionPos = startingPos;
	BWAPI::TilePosition naturalExpansionTP;//Tile Position of the natural Expansion site
	BWAPI::TilePosition startingTP;// = BWAPI::Broodwar->self()->getStartLocation();
	BWAPI::Unitset allAttackers; // all Zealots owned
	BWAPI::Unitset baseAttackers; // Zealots in our base
	//BWAPI::Unitset attackers; // Units ready to attack
	BWAPI::Unitset allMinerals;
	bool attackPerformed = false;

public:

    StarterBot();

    void sendIdleWorkersToMinerals();
	void positionIdleZealots();
	void sendWorkersToGas();
	bool baseUnderattack();
	bool expansionUnderattack();
	bool readyForAttack();
	void attack();
	bool atEnemyBase(BWAPI::Unit unit);
    void trainAdditionalWorkers();
	void trainDragoons();
	void trainZealots();
	void upgradeUnits();
    void buildAdditionalSupply();
	void buildGateway();
	void buildExpansionBuildings();
	void buildCannon();
	void buildCyberCore();
	void buildAssimilator();
	void getExpansionLoc();
	void scoutEnemy();
	bool foundEnemyBase();
    void drawDebugInformation();
	void debug();

    // functions that are triggered by various BWAPI events from main.cpp
	void onStart();
	void onFrame();
	void onEnd(bool isWinner);
	void onUnitDestroy(BWAPI::Unit unit);
	void onUnitMorph(BWAPI::Unit unit);
	void onSendText(std::string text);
	void onUnitCreate(BWAPI::Unit unit);
	void onUnitComplete(BWAPI::Unit unit);
	void onUnitShow(BWAPI::Unit unit);
	void onUnitHide(BWAPI::Unit unit);
	void onUnitRenegade(BWAPI::Unit unit);
};