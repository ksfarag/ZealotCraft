#pragma once

#include "MapTools.h"

#include <BWAPI.h>

class StarterBot
{
    MapTools m_mapTools;

	//unit used to scout
	BWAPI::Unit scout = nullptr;
	bool scouting = false;

	bool enemyFound = false;
	BWAPI::Position enemyBasePosition;
	
public:

    StarterBot();

	//method used to scout enemy
	void scoutEnemy();

    // helper functions to get you started with bot programming and learn the API
    void sendIdleWorkersToMinerals();
	void positionIdleZealots();
	bool underattack();
	bool attacking();
    void trainAdditionalWorkers();
	void trainZealots();
    void buildAdditionalSupply();
	void buildGateway();
    void drawDebugInformation();

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