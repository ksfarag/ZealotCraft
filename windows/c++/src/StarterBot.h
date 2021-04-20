#pragma once

#include "MapTools.h"

#include <BWAPI.h>

class StarterBot
{
    MapTools m_mapTools;

	BWAPI::Unit workerScout;
	BWAPI::Unit expansionUnit;
	BWAPI::Position nullPos = { -1,-1 };
	BWAPI::Position enemyBasePos = nullPos;
	BWAPI::Position expansionBase = nullPos;
	BWAPI::Unitset allZealots; // all Zealots owned
	BWAPI::Unitset baseZealots; // Zealots in our base
	BWAPI::Unitset attackZealots; // Zealots ready to attack

public:

    StarterBot();

    // helper functions to get you started with bot programming and learn the API
    void sendIdleWorkersToMinerals();
	void positionIdleZealots();
	bool baseUnderattack();
	bool readyForAttack();
	void attack();
	bool atEnemyBase(BWAPI::Unit unit);
    void trainAdditionalWorkers();
	void trainZealots();
    void buildAdditionalSupply();
	void buildGateway();
	void buildCannon();
	void scoutEnemy();
	bool foundEnemyBase();
    void drawDebugInformation();
	void debug();
	void expandBase();
	BWAPI::Unit getExpansionUnit();

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