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
	BWAPI::TilePosition startingTP;         
	BWAPI::Unitset allFighters;              // all attacking units owned
	BWAPI::Unitset baseAttackers;            // attacking units sitting in our base
	BWAPI::Unitset allMinerals;
	BWAPI::Unitset myUnits;
	bool attackPerformed = false;
	int gasFarmersCount = 0;
	int mineralFarmersCount = 0;

public:

    StarterBot();

    void sendIdleWorkersToMinerals();
	void positionIdleZealots();
	void sendWorkersToGas();
	bool isBaseUnderAttack();
	bool isExpansionUnderAttack();
	bool isReadyForAttack();
	void attack();
	bool atEnemyBase(BWAPI::Unit unit);
    void trainAdditionalWorkers();
	void trainDragoons();
	void trainZealots();
	void upgradeUnits();
    void buildAdditionalSupply();
	void buildGateways();
	void buildExpansionBuildings();
	void buildCannon();
	void buildCyberCore();
	void buildAssimilator();
	void buildExpansionNexus();
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