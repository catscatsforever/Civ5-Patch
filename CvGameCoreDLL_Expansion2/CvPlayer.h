/*	-------------------------------------------------------------------------------------------------------
	� 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

// player.h

#ifndef CIV5_PLAYER_H
#define CIV5_PLAYER_H

#define SAFE_ESTIMATE_NUM_IMPROVEMENTS 50
#define SAFE_ESTIMATE_NUM_CITIES       64
#define MAX_INCOMING_UNITS	20

#include "CvCityAI.h"
#include "CvUnit.h"
#include "CvArmyAI.h"
#include "LinkedList.h"
#include "FFastVector.h"
#include "CvPreGame.h"
#include "CvAchievementUnlocker.h"
#include "CvUnitCycler.h"

class CvPlayerPolicies;
class CvEconomicAI;
class CvMilitaryAI;
class CvCitySpecializationAI;
class CvWonderProductionAI;
class CvGrandStrategyAI;
class CvDiplomacyAI;
class CvPlayerReligions;
class CvReligionAI;
class CvPlayerTechs;
class CvFlavorManager;
class CvTacticalAI;
class CvHomelandAI;
class CvMinorCivAI;
class CvDealAI;
class CvBuilderTaskingAI;
class CvDangerPlots;
class CvCityConnections;
class CvNotifications;
class CvTreasury;
class CvPlayerTraits;
class CvGameInitialItemsOverrides;
class CvDiplomacyRequests;
class CvPlayerEspionage;
class CvEspionageAI;
class CvPlayerTrade;
class CvTradeAI;
class CvLeagueAI;
class CvPlayerCulture;

typedef std::list<CvPopupInfo*> CvPopupQueue;

typedef std::vector< std::pair<UnitCombatTypes, PromotionTypes> > UnitCombatPromotionArray;
typedef std::vector< std::pair<UnitClassTypes, PromotionTypes> > UnitClassPromotionArray;
typedef std::vector< std::pair<CivilizationTypes, LeaderHeadTypes> > CivLeaderArray;
typedef FStaticVector<int, 152* 96, true, c_eCiv5GameplayDLL, 0> CvPlotsVector; // allocate the size of HUGE Terra world just in case (this is max that we ship with)

class CvPlayer
{
	friend class CvPlayerPolicies;

public:
	typedef std::map<unsigned int, int> TurnData;


	CvPlayer();
	virtual ~CvPlayer();

	void init(PlayerTypes eID);
	void setupGraphical();
	void reset(PlayerTypes eID = NO_PLAYER, bool bConstructorCall = false);
	void gameStartInit();
	void uninit();
#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
	bool isDisconnected() const;
	void setIsDisconnected(bool bNewValue);
#endif

	void initFreeState(CvGameInitialItemsOverrides& kOverrides);
	void initFreeUnits(CvGameInitialItemsOverrides& kOverrides);
	void addFreeUnitAI(UnitAITypes eUnitAI, int iCount);
	CvPlot* addFreeUnit(UnitTypes eUnit, UnitAITypes eUnitAI = NO_UNITAI);

	CvCity* initCity(int iX, int iY, bool bBumpUnits = true, bool bInitialFounding = true);
	void acquireCity(CvCity* pCity, bool bConquest, bool bGift);
	void killCities();
	CvString getNewCityName() const;
	CvString GetBorrowedCityName(CivilizationTypes eCivToBorrowFrom) const;
	void getCivilizationCityName(CvString& szBuffer, CivilizationTypes eCivilization) const;
	bool isCityNameValid(CvString& szName, bool bTestDestroyed = true) const;

	void DoLiberatePlayer(PlayerTypes ePlayer, int iOldCityID);
	bool CanLiberatePlayer(PlayerTypes ePlayer);
	bool CanLiberatePlayerCity(PlayerTypes ePlayer);

#ifdef UNIT_UPGRADE_NUM_RESOURE_USED_CHANGE
	CvUnit* initUnit(UnitTypes eUnit, int iX, int iY, UnitAITypes eUnitAI = NO_UNITAI, DirectionTypes eFacingDirection = NO_DIRECTION, bool bNoMove = false, bool bSetupGraphical = true, int iMapLayer = 0, int iNumGoodyHutsPopped = 0, bool bIsUpgrade = false);
	CvUnit* initUnitWithNameOffset(UnitTypes eUnit, int nameOffset, int iX, int iY, UnitAITypes eUnitAI = NO_UNITAI, DirectionTypes eFacingDirection = NO_DIRECTION, bool bNoMove = false, bool bSetupGraphical = true, int iMapLayer = 0, int iNumGoodyHutsPopped = 0, bool bIsUpgrade = false);
#else
	CvUnit* initUnit(UnitTypes eUnit, int iX, int iY, UnitAITypes eUnitAI = NO_UNITAI, DirectionTypes eFacingDirection = NO_DIRECTION, bool bNoMove=false, bool bSetupGraphical=true, int iMapLayer = 0, int iNumGoodyHutsPopped = 0);
	CvUnit* initUnitWithNameOffset(UnitTypes eUnit, int nameOffset, int iX, int iY, UnitAITypes eUnitAI = NO_UNITAI, DirectionTypes eFacingDirection = NO_DIRECTION, bool bNoMove = false, bool bSetupGraphical = true, int iMapLayer = 0, int iNumGoodyHutsPopped = 0);
#endif

	void disbandUnit(bool bAnnounce);
	void killUnits();

	CvPlot *GetGreatAdmiralSpawnPlot (CvUnit *pUnit);

	int GetNumBuilders() const;
	void SetNumBuilders(int iNum);
	void ChangeNumBuilders(int iChange);
	int GetMaxNumBuilders() const;
	void SetMaxNumBuilders(int iNum);
	void ChangeMaxNumBuilders(int iChange);

	int GetNumUnitsWithUnitAI(UnitAITypes eUnitAIType, bool bIncludeBeingTrained = false, bool bIncludeWater = true);
	int GetNumUnitsWithDomain(DomainTypes eDomain, bool bMilitaryOnly);
	int GetNumUnitsWithUnitCombat(UnitCombatTypes eDomain);

	void InitDangerPlots();
	void UpdateDangerPlots();
	void SetDangerPlotsDirty();

	bool isHuman() const;
	bool isObserver() const;
	bool isBarbarian() const;
	void doBarbarianRansom(int iOption, int iUnitID);

	const char* getName() const;
	const char* getNameKey() const;
	const char* const getNickName() const;
	const char* getCivilizationDescription() const;
	const char* getCivilizationDescriptionKey() const;
	const char* getCivilizationShortDescription() const;
	const char* getCivilizationShortDescriptionKey() const;
	const char* getCivilizationAdjective() const;
	const char* getCivilizationAdjectiveKey() const;
	const char* getCivilizationTypeKey() const;
	const char* getLeaderTypeKey() const;

	bool isWhiteFlag() const;
	const char* GetStateReligionName() const;
	CvString GetStateReligionKey() const;
	void SetStateReligionKey(const char* strKey);
	const CvString getWorstEnemyName() const;
	ArtStyleTypes getArtStyleType() const;

	void doTurn();
	void doTurnPostDiplomacy();
	void doTurnUnits();
	void SetAllUnitsUnprocessed();
	void DoUnitReset();
	void DoUnitAttrition();
	void RespositionInvalidUnits();

	void updateYield();
	void updateExtraSpecialistYield();
	void updateCityPlotYield();
	void updateCitySight(bool bIncrement);
	void UpdateNotifications();
	void UpdateReligion();

	void updateTimers();

	bool hasPromotableUnit() const;

	bool hasReadyUnit() const;
	int GetCountReadyUnits() const;
	const CvUnit* GetFirstReadyUnit() const;
	void EndTurnsForReadyUnits();
	bool hasAutoUnit() const;
	bool hasBusyUnit() const;
	const UnitHandle getBusyUnit() const;
	bool hasBusyCity() const;
	bool hasBusyUnitOrCity() const;
	const CvCity* getBusyCity() const;
	void chooseTech(int iDiscover = 0, const char* strTxt=0, TechTypes iTechJustDiscovered=NO_TECH);

	// Civ 5 Score
	int GetScore(bool bFinal = false, bool bVictory = false) const;

	int GetScoreFromCities() const;
	int GetScoreFromPopulation() const;
	int GetScoreFromLand() const;
	int GetScoreFromWonders() const;
	int GetScoreFromPolicies() const;
	int GetScoreFromGreatWorks() const;
	int GetScoreFromReligion() const;
	int GetScoreFromTechs() const;
	int GetScoreFromFutureTech() const;
	void ChangeScoreFromFutureTech(int iChange);
	int GetScoreFromScenario1() const;
	void ChangeScoreFromScenario1(int iChange);
	int GetScoreFromScenario2() const;
	void ChangeScoreFromScenario2(int iChange);
	int GetScoreFromScenario3() const;
	void ChangeScoreFromScenario3(int iChange);
	int GetScoreFromScenario4() const;
	void ChangeScoreFromScenario4(int iChange);
	// End Civ 5 Score

	int countCityFeatures(FeatureTypes eFeature) const;
	int countNumBuildings(BuildingTypes eBuilding) const;
	//int countNumCitiesConnectedToCapital() const;

	int countCitiesFeatureSurrounded() const;

	bool IsCityConnectedToCity(CvCity* pCity1, CvCity* pCity2, RouteTypes eRestrictRouteType = NO_ROUTE, bool bIgnoreHarbors = false);
	bool IsCapitalConnectedToPlayer(PlayerTypes ePlayer, RouteTypes eRestrictRouteType = NO_ROUTE);
	bool IsCapitalConnectedToCity(CvCity* pCity, RouteTypes eRestrictRouteType = NO_ROUTE);

	void findNewCapital();

	bool canRaze(CvCity* pCity, bool bIgnoreCapitals = false) const;
	void raze(CvCity* pCity);
	void unraze(CvCity* pCity);
	void disband(CvCity* pCity);

	bool canReceiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit) const;
	void receiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit);
	void doGoody(CvPlot* pPlot, CvUnit* pUnit);

	void AwardFreeBuildings(CvCity* pCity); // slewis - broken out so that Venice can get free buildings when they purchase something
	bool canFound(int iX, int iY, bool bTestVisible = false) const;
	void found(int iX, int iY);

	bool canTrain(UnitTypes eUnit, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, bool bIgnoreUniqueUnitStatus = false, CvString* toolTipSink = NULL) const;
#ifdef NEW_BELIEF_PROPHECY
	bool canConstruct(BuildingTypes eBuilding, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, CvString* toolTipSink = NULL, const CvCity* pCity = NULL) const;
#else
	bool canConstruct(BuildingTypes eBuilding, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, CvString* toolTipSink = NULL) const;
#endif
	bool canCreate(ProjectTypes eProject, bool bContinue = false, bool bTestVisible = false) const;
	bool canPrepare(SpecialistTypes eSpecialist, bool bContinue = false) const;
	bool canMaintain(ProcessTypes eProcess, bool bContinue = false) const;
	bool IsCanPurchaseAnyCity(bool bTestPurchaseCost, bool bTestTrainable, UnitTypes eUnit, BuildingTypes eBuilding, YieldTypes ePurchaseYield);
	bool isProductionMaxedUnitClass(UnitClassTypes eUnitClass) const;
	bool isProductionMaxedBuildingClass(BuildingClassTypes eBuildingClass, bool bAcquireCity = false) const;
	bool isProductionMaxedProject(ProjectTypes eProject) const;
	int getProductionNeeded(UnitTypes eUnit) const;
	int getProductionNeeded(BuildingTypes eBuilding) const;
	int getProductionNeeded(ProjectTypes eProject) const;
	int getProductionNeeded(SpecialistTypes eSpecialist) const;

	int getProductionModifier(CvString* toolTipSink = NULL) const;
	int getProductionModifier(UnitTypes eUnit, CvString* toolTipSink = NULL) const;
	int getProductionModifier(BuildingTypes eBuilding, CvString* toolTipSink = NULL) const;
	int getProductionModifier(ProjectTypes eProject, CvString* toolTipSink = NULL) const;
	int getProductionModifier(SpecialistTypes eSpecialist, CvString* toolTipSink = NULL) const;
	int getProductionModifier(ProcessTypes eProcess, CvString* toolTipSink = NULL) const;

	int getBuildingClassPrereqBuilding(BuildingTypes eBuilding, BuildingClassTypes ePrereqBuildingClass, int iExtra = 0) const;
	void removeBuildingClass(BuildingClassTypes eBuildingClass);
	void processBuilding(BuildingTypes eBuilding, int iChange, bool bFirst, CvArea* pArea);
	int GetBuildingClassYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYieldType);

	bool canBuild(const CvPlot* pPlot, BuildTypes eBuild, bool bTestEra = false, bool bTestVisible = false, bool bTestGold = true, bool bTestPlotOwner = true) const;
	bool IsBuildBlockedByFeature(BuildTypes eBuild, FeatureTypes eFeature) const;
	int getBuildCost(const CvPlot* pPlot, BuildTypes eBuild) const;
	RouteTypes getBestRoute(CvPlot* pPlot = NULL) const;
	int getImprovementUpgradeRate() const;

	int GetAllFeatureProduction() const;
	void ChangeAllFeatureProduction(int iChange);

	int calculateTotalYield(YieldTypes eYield) const;

	int GetUnitProductionMaintenanceMod() const;
	void UpdateUnitProductionMaintenanceMod();
	int calculateUnitProductionMaintenanceMod() const;

	int GetNumUnitsSupplied() const;
	int GetNumUnitsSuppliedByHandicap() const;
	int GetNumUnitsSuppliedByCities() const;
	int GetNumUnitsSuppliedByPopulation() const;
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
	int GetNumTrainedUnits() const;
	void ChangeNumTrainedUnits(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
	int GetNumKilledUnits() const;
	void ChangeNumKilledUnits(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
	int GetNumLostUnits() const;
	void ChangeNumLostUnits(int iChange);
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
	int GetUnitsDamageDealt() const;
	void ChangeUnitsDamageDealt(int iChange);
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
	int GetUnitsDamageTaken() const;
	void ChangeUnitsDamageTaken(int iChange);
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
	int GetCitiesDamageDealt() const;
	void ChangeCitiesDamageDealt(int iChange);
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
	int GetCitiesDamageTaken() const;
	void ChangeCitiesDamageTaken(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
	int GetNumScientistsTotal() const;
	void ChangeNumScientistsTotal(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
	int GetNumEngineersTotal() const;
	void ChangeNumEngineersTotal(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
	int GetNumMerchantsTotal() const;
	void ChangeNumMerchantsTotal(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
	int GetNumWritersTotal() const;
	void ChangeNumWritersTotal(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
	int GetNumAristsTotal() const;
	void ChangeNumArtistsTotal(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
	int GetNumMusiciansTotal() const;
	void ChangeNumMusiciansTotal(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
	int GetNumGeneralsTotal() const;
	void ChangeNumGeneralsTotal(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
	int GetNumAdmiralsTotal() const;
	void ChangeNumAdmiralsTotal(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
	int GetNumProphetsTotal() const;
	void ChangeNumProphetsTotal(int iChange);
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
	int GetProductionGoldFromWonders() const;
	void ChangeProductionGoldFromWonders(int iChange);
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
	int GetNumChops() const;
	void ChangeNumChops(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
	int GetNumTimesOpenedDemographics() const;
	void ChangeNumTimesOpenedDemographics(int Change);
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
	bool GetMayaBoostScientist() const;
	void SetMayaBoostScientist(bool bValue);
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
	bool GetMayaBoostEngineers() const;
	void SetMayaBoostEngineers(bool bValue);
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
	bool GetMayaBoostMerchants() const;
	void SetMayaBoostMerchants(bool bValue);
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
	bool GetMayaBoostWriters() const;
	void SetMayaBoostWriters(bool bValue);
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
	bool GetMayaBoostArtists() const;
	void SetMayaBoostArtists(bool bValue);
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
	bool GetMayaBoostMusicians() const;
	void SetMayaBoostMusicians(bool bValue);
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
	int GetScientistsTotalScienceBoost() const;
	void ChangeScientistsTotalScienceBoost(int Change);
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
	int GetEngineersTotalHurryBoost() const;
	void ChangeEngineersTotalHurryBoost(int Change);
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
	int GetMerchantsTotalTradeBoost() const;
	void ChangeMerchantsTotalTradeBoost(int Change);
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
	int GetWritersTotalCultureBoost() const;
	void ChangeWritersTotalCultureBoost(int Change);
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
	int GetMusiciansTotalTourismBoost() const;
	void ChangeMusiciansTotalTourismBoost(int Change);
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
	int GetNumPopulationLostFromNukes() const;
	void ChangeNumPopulationLostFromNukes(int Change);
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
	int GetNumCSQuestsCompleted() const;
	void ChangeNumCSQuestsCompleted(int Change);
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
	int GetNumAlliedCS() const;
	void ChangeNumAlliedCS(int Change);
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
	int GetTimesEnteredCityScreen() const;
	void ChangeTimesEnteredCityScreen(int Change);
#endif
#ifdef EG_REPLAYDATASET_HAPPINESSFROMTRADEDEALS
	int GetNumHappinessFromTradeDeals() const;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
	int GetNumDiedSpies() const;
	void ChangeNumDiedSpies(int iChange);
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
	int GetNumKilledSpies() const;
	void ChangeNumKilledSpies(int iChange);
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
	int GetFoodFromMinorsTimes100() const;
	void ChangeFoodFromMinorsTimes100(int iChange);
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
	int GetProductionFromMinorsTimes100() const;
	void ChangeProductionFromMinorsTimes100(int iChange);
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
	int GetNumUnitsFromMinors() const;
	void ChangeNumUnitsFromMinors(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
	int GetNumCreatedWorldWonders() const;
	void ChangeNumCreatedWorldWonders(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
	int GetNumGoldSpentOnBuildingBuys() const;
	void ChangeNumGoldSpentOnBuildingBuys(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
	int GetNumGoldSpentOnUnitBuys() const;
	void ChangeNumGoldSpentOnUnitBuys(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
	int GetNumGoldSpentOnUgrades() const;
	void ChangeNumGoldSpentOnUgrades(int iChange);
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
	int GetGoldFromKills() const;
	void ChangeGoldFromKills(int iChange);
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
	int GetCultureFromKills() const;
	void ChangeCultureFromKills(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
	int GetNumGoldSpentOnGPBuys() const;
	void ChangeNumGoldSpentOnGPBuys(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
	int GetNumGoldSpentOnTilesBuys() const;
	void ChangeNumGoldSpentOnTilesBuys(int iChange);
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
	int GetNumGoldFromPillage() const;
	void ChangeNumGoldFromPillage(int iChange);
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
	int GetNumGoldFromPlunder() const;
	void ChangeNumGoldFromPlunder(int iChange);
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
	int GetNumFaithSpentOnMilitaryUnits() const;
	void ChangeNumFaithSpentOnMilitaryUnits(int iChange);
#endif

	int GetNumUnitsOutOfSupply() const;

	int calculateUnitCost() const;
	int calculateUnitSupply() const;
	int calculateResearchModifier(TechTypes eTech);
	int calculateGoldRate() const;
	int calculateGoldRateTimes100() const;

	int unitsRequiredForGoldenAge() const;
	int unitsGoldenAgeCapable() const;
	int unitsGoldenAgeReady() const;

	int greatGeneralThreshold() const;
	int greatAdmiralThreshold() const;

	int specialistYield(SpecialistTypes eSpecialist, YieldTypes eYield) const;

	int GetCityYieldChange(YieldTypes eYield) const;
	void ChangeCityYieldChange(YieldTypes eYield, int iChange);

	int GetCoastalCityYieldChange(YieldTypes eYield) const;
	void ChangeCoastalCityYieldChange(YieldTypes eYield, int iChange);

	int GetCapitalYieldChange(YieldTypes eYield) const;
	void ChangeCapitalYieldChange(YieldTypes eYield, int iChange);

	int GetCapitalYieldPerPopChange(YieldTypes eYield) const;
	void ChangeCapitalYieldPerPopChange(YieldTypes eYield, int iChange);

	int GetGreatWorkYieldChange(YieldTypes eYield) const;
	void ChangeGreatWorkYieldChange(YieldTypes eYield, int iChange);

	CvPlot* getStartingPlot() const;
	void setStartingPlot(CvPlot* pNewValue);

	int getTotalPopulation() const;
	int getAveragePopulation() const;
	void changeTotalPopulation(int iChange);
	long getRealPopulation() const;

	int GetNewCityExtraPopulation() const;
	void ChangeNewCityExtraPopulation(int iChange);

	int GetFreeFoodBox() const;
	void ChangeFreeFoodBox(int iChange);

	int getTotalLand() const;
	void changeTotalLand(int iChange);

	int getTotalLandScored() const;
	void changeTotalLandScored(int iChange);

	int GetHappinessFromTradeRoutes() const;
	void DoUpdateCityConnectionHappiness();

	// Culture

	int GetTotalJONSCulturePerTurn() const;

	int GetJONSCulturePerTurnFromCities() const;

	int GetJONSCulturePerTurnFromExcessHappiness() const;
	int GetJONSCulturePerTurnFromTraits() const;

	int GetJONSCulturePerTurnForFree() const;
	void ChangeJONSCulturePerTurnForFree(int iChange);

	int GetJONSCulturePerTurnFromMinorCivs() const; // DEPRECATED, use GetCulturePerTurnFromMinorCivs() instead
	void ChangeJONSCulturePerTurnFromMinorCivs(int iChange); // DEPRECATED, does nothing
	int GetCulturePerTurnFromMinorCivs() const;
	int GetCulturePerTurnFromMinor(PlayerTypes eMinor) const;

	int GetCulturePerTurnFromReligion() const;

	int GetCulturePerTurnFromBonusTurns() const;

	int GetJONSCultureCityModifier() const;
	void ChangeJONSCultureCityModifier(int iChange);

	int getJONSCulture() const;
	void setJONSCulture(int iNewValue);
	void changeJONSCulture(int iChange);

	int GetJONSCultureEverGenerated() const;
	void SetJONSCultureEverGenerated(int iNewValue);
	void ChangeJONSCultureEverGenerated(int iChange);

	int GetJONSCulturePerCityPerTurn() const;

	int GetCulturePerWonder() const;
	void ChangeCulturePerWonder(int iChange);

	int GetCultureWonderMultiplier() const;
	void ChangeCultureWonderMultiplier(int iChange);

	int GetCulturePerTechResearched() const;
	void ChangeCulturePerTechResearched(int iChange);

	int GetSpecialistCultureChange() const;
	void ChangeSpecialistCultureChange(int iChange);

	int GetCultureYieldFromPreviousTurns(int iGameTurn, int iNumPreviousTurnsToCount);

#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
	int getBuildingScecialistCountChange(BuildingTypes eIndex1, SpecialistTypes eIndex2) const;
	void changeBuildingScecialistCountChange(BuildingTypes eIndex1, SpecialistTypes eIndex2, int iChange);
#endif

	int GetNumCitiesFreeCultureBuilding() const;
	void ChangeNumCitiesFreeCultureBuilding(int iChange);
	int GetNumCitiesFreeFoodBuilding() const;
	void ChangeNumCitiesFreeFoodBuilding(int iChange);
#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
	int GetNumCitiesFreeDefensiveBuilding() const;
	void ChangeNumCitiesFreeDefensiveBuilding(int iChange);
#endif

	void DoYieldsFromKill(UnitTypes eAttackingUnitType, UnitTypes eKilledUnitType, int iX, int iY, bool bWasBarbarian, int iExistingDelay);
	void DoYieldBonusFromKill(YieldTypes eYield, UnitTypes eAttackingUnitType, UnitTypes eKilledUnitType, int iX, int iY, bool bWasBarbarian, int &iNumBonuses);
	void DoUnresearchedTechBonusFromKill(UnitTypes eKilledUnitType, int iX, int iY, int &iNumBonuses);
	void ReportYieldFromKill(YieldTypes eYield, int iValue, int iX, int iY, int iDelay);

#ifdef NQ_ALLOW_RELIGION_ONE_SHOTS 
	void DoReligionOneShots(ReligionTypes eReligion);
#endif
	void DoTechFromCityConquer(CvCity* pConqueredCity);

	// Faith
	int GetTotalFaithPerTurn() const;
	int GetFaithPerTurnFromCities() const;
	int GetFaithPerTurnFromMinorCivs() const;
	int GetFaithPerTurnFromMinor(PlayerTypes eMinor) const;
	int GetFaithPerTurnFromReligion() const;
	int GetFaith() const;
	void SetFaith(int iNewValue);
	void ChangeFaith(int iChange);
	int GetFaithEverGenerated() const;
	void SetFaithEverGenerated(int iNewValue);
	void ChangeFaithEverGenerated(int iChange);

	// Happiness

	void DoUpdateHappiness();
	int GetHappiness() const;
	void SetHappiness(int iNewValue);

	int GetExcessHappiness() const;
	bool IsEmpireUnhappy() const;
	bool IsEmpireVeryUnhappy() const;
	bool IsEmpireSuperUnhappy() const;

	void DoUpdateUprisings();
	int GetUprisingCounter() const;
	void SetUprisingCounter(int iValue);
	void ChangeUprisingCounter(int iChange);
	void DoResetUprisingCounter(bool bFirstTime);
	void DoUprising();

	void DoUpdateCityRevolts();
	int GetCityRevoltCounter() const;
	void SetCityRevoltCounter(int iValue);
	void ChangeCityRevoltCounter(int iChange);
	void DoResetCityRevoltCounter();
	void DoCityRevolt();
	CvCity *GetMostUnhappyCity();
	PlayerTypes GetMostUnhappyCityRecipient();

	int GetHappinessFromPolicies() const;
	int GetHappinessFromCities() const;
	int GetHappinessFromBuildings() const;

	int GetExtraHappinessPerCity() const;
	void ChangeExtraHappinessPerCity(int iChange);
	int GetExtraHappinessPerXPolicies() const;
	void ChangeExtraHappinessPerXPolicies(int iChange);

	int GetHappinessFromResources() const;
	int GetHappinessFromResourceVariety() const;
	int GetHappinessFromReligion();
	int GetHappinessFromNaturalWonders() const;

	int GetExtraHappinessPerLuxury() const;
	void ChangeExtraHappinessPerLuxury(int iChange);

	int GetHappinessFromLuxury(ResourceTypes eResource) const;

	int GetUnhappiness(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL) const;

	int GetUnhappinessFromCityForUI(CvCity* pCity) const;

	int GetUnhappinessFromCityCount(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL) const;
	int GetUnhappinessFromCapturedCityCount(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL) const;
	int GetUnhappinessFromCityPopulation(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL) const;
	int GetUnhappinessFromCitySpecialists(CvCity* pAssumeCityAnnexed, CvCity* pAssumeCityPuppeted) const;
	int GetUnhappinessFromPuppetCityPopulation() const;
	int GetUnhappinessFromOccupiedCities(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL) const;

	int GetUnhappinessFromUnits() const;
	void ChangeUnhappinessFromUnits(int iChange);

	int GetUnhappinessFromUnitsMod() const;
	void ChangeUnhappinessFromUnitsMod(int iChange);

	int GetUnhappinessMod() const;
	void ChangeUnhappinessMod(int iChange);

	int GetCityCountUnhappinessMod() const;
	void ChangeCityCountUnhappinessMod(int iChange);

	int GetOccupiedPopulationUnhappinessMod() const;
	void ChangeOccupiedPopulationUnhappinessMod(int iChange);

	int GetCapitalUnhappinessMod() const;
	void ChangeCapitalUnhappinessMod(int iChange);

	int GetHappinessPerGarrisonedUnit() const;
	void SetHappinessPerGarrisonedUnit(int iValue);
	void ChangeHappinessPerGarrisonedUnit(int iChange);

	int GetHappinessPerTradeRoute() const;
	void SetHappinessPerTradeRoute(int iValue);
	void ChangeHappinessPerTradeRoute(int iChange);

	int GetHappinessPerXPopulation() const;
	void SetHappinessPerXPopulation(int iValue);
	void ChangeHappinessPerXPopulation(int iChange);

	int GetHappinessFromMinorCivs() const;
	int GetHappinessFromMinor(PlayerTypes eMinor) const;

	int GetHappinessFromLeagues() const;
	void SetHappinessFromLeagues(int iValue);
	void ChangeHappinessFromLeagues(int iChange);

	// END Happiness

	// Espionage
	int GetEspionageModifier() const;
	void ChangeEspionageModifier(int iChange);
	int GetStartingSpyRank() const;
	void ChangeStartingSpyRank(int iChange);
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
	int GetNumStolenScience() const;
	void ChangeNumStolenScience(int iChange);
#endif
	// END Espionage

	int GetExtraLeagueVotes() const;
	void ChangeExtraLeagueVotes(int iChange);

#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
	int GetMaxExtraVotesFromMinors() const;
	void ChangeMaxExtraVotesFromMinors(int iChange);
#endif

#ifdef POLICY_EXTRA_VOTES
	int GetPolicyExtraVotes() const;
	void ChangePolicyExtraVotes(int iChange);
#endif

	int GetWoundedUnitDamageMod() const;
	void SetWoundedUnitDamageMod(int iValue);
	void ChangeWoundedUnitDamageMod(int iChange);

	int GetUnitUpgradeCostMod() const;
	void SetUnitUpgradeCostMod(int iValue);
	void ChangeUnitUpgradeCostMod(int iChange);

	int GetBarbarianCombatBonus() const;
	void SetBarbarianCombatBonus(int iValue);
	void ChangeBarbarianCombatBonus(int iChange);

	bool IsAlwaysSeeBarbCamps() const;
	void SetAlwaysSeeBarbCampsCount(int iValue);
	void ChangeAlwaysSeeBarbCampsCount(int iChange);

	void setHasPolicy(PolicyTypes eIndex, bool bNewValue);
	int getNextPolicyCost() const;
	void DoUpdateNextPolicyCost();
	bool canAdoptPolicy(PolicyTypes ePolicy) const;
	void doAdoptPolicy(PolicyTypes ePolicy);
	void DoBuyNewBranch(PolicyBranchTypes eBranch);

#ifdef POLICY_DO_TECH_FROM_CITY_CONQ
	bool IsPolicyTechFromCityConquer() const;
	void ChangePolicyTechFromCityConquer(int iChange);
#endif

#ifdef POLICY_NO_CULTURE_SPECIALIST_FOOD
	bool IsNoCultureSpecialistFood() const;
	void ChangeNoCultureSpecialistFood(int iChange);
#endif

#ifdef POLICY_MINORS_GIFT_UNITS
	bool IsMinorsGiftUnits() const;
	void ChangeMinorsGiftUnits(int iChange);
#endif

#ifdef POLICY_NO_CARGO_PILLAGE
	bool IsNoCargoPillage() const;
	void ChangeNoCargoPillage(int iChange);
#endif

#ifdef POLICY_GREAT_WORK_HAPPINESS
	int GetGreatWorkHappiness() const;
	void ChangeGreatWorkHappiness(int iChange);
#endif

#ifdef POLICY_SCIENCE_PER_X_FOLLOWERS
	int GetSciencePerXFollowers() const;
	void ChangeSciencePerXFollowers(int iChange);
#endif

#ifdef POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD
	bool IsNoDifferentIdeologiesTourismMod() const;
	void ChangeNoDifferentIdeologiesTourismMod(int iChange);
#endif

#ifdef POLICY_GLOBAL_POP_CHANGE
	int GetGlobalPopChange() const;
	void ChangeGlobalPopChange(int iChange);
#endif

#ifdef POLICY_GREAT_WORK_TOURISM_CHANGES
	int GetGreatWorkTourismChanges() const;
	void ChangeGreatWorkTourismChanges(int iChange);
#endif

#ifdef POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP
	int GetCityScienceSquaredModPerXPop() const;
	void ChangeCityScienceSquaredModPerXPop(int iChange);
#endif

	bool IsAnarchy() const;
	int GetAnarchyNumTurns() const;
	void SetAnarchyNumTurns(int iValue);
	void ChangeAnarchyNumTurns(int iChange);

	int getAdvancedStartPoints() const;
	void setAdvancedStartPoints(int iNewValue);
	void changeAdvancedStartPoints(int iChange);

	bool canStealTech(PlayerTypes eTarget, TechTypes eTech) const;
	bool canSpyDestroyUnit(PlayerTypes eTarget, CvUnit& kUnit) const;
	bool canSpyBribeUnit(PlayerTypes eTarget, CvUnit& kUnit) const;
	bool canSpyDestroyBuilding(PlayerTypes eTarget, BuildingTypes eBuilding) const;
	bool canSpyDestroyProject(PlayerTypes eTarget, ProjectTypes eProject) const;

	void doAdvancedStartAction(AdvancedStartActionTypes eAction, int iX, int iY, int iData, bool bAdd);
	int getAdvancedStartUnitCost(UnitTypes eUnit, bool bAdd, CvPlot* pPlot = NULL);
	int getAdvancedStartCityCost(bool bAdd, CvPlot* pPlot = NULL);
	int getAdvancedStartPopCost(bool bAdd, CvCity* pCity = NULL);
	int getAdvancedStartBuildingCost(BuildingTypes eBuilding, bool bAdd, CvCity* pCity = NULL);
	int getAdvancedStartImprovementCost(ImprovementTypes eImprovement, bool bAdd, CvPlot* pPlot = NULL);
	int getAdvancedStartRouteCost(RouteTypes eRoute, bool bAdd, CvPlot* pPlot = NULL);
	int getAdvancedStartTechCost(TechTypes eTech, bool bAdd);
	int getAdvancedStartVisibilityCost(bool bAdd, CvPlot* pPlot = NULL);

	// Temporary Bonuses
	int GetAttackBonusTurns() const;
	void ChangeAttackBonusTurns(int iChange);
	int GetCultureBonusTurns() const;
	void ChangeCultureBonusTurns(int iChange);
	int GetTourismBonusTurns() const;
	void ChangeTourismBonusTurns(int iChange);

	// Golden Age Stuff

	void DoProcessGoldenAge();

	int GetGoldenAgeProgressThreshold() const;

	int GetGoldenAgeProgressMeter() const;
	void SetGoldenAgeProgressMeter(int iValue);
	void ChangeGoldenAgeProgressMeter(int iChange);

	int GetGoldenAgeMeterMod() const;
	void SetGoldenAgeMeterMod(int iValue);
	void ChangeGoldenAgeMeterMod(int iChange);

	int GetNumGoldenAges() const;
	void SetNumGoldenAges(int iValue);
	void ChangeNumGoldenAges(int iChange);

	int getGoldenAgeTurns() const;
	bool isGoldenAge() const;
	void changeGoldenAgeTurns(int iChange);
	int getGoldenAgeLength() const;
#ifdef TAJ_MAHAL_STARTS_GA_NEXT_TURN
	void setBuildingGoldenAgeTurns(int iValue);
	int getBuildingGoldenAgeTurns() const;
#endif

	int getNumUnitGoldenAges() const;
	void changeNumUnitGoldenAges(int iChange);

	int getStrikeTurns() const;
	void changeStrikeTurns(int iChange);

	int getGoldenAgeModifier() const;
	void changeGoldenAgeModifier(int iChange);

	// Great People Stuff
	void createGreatGeneral(UnitTypes eGreatPersonUnit, int iX, int iY);
	void createGreatAdmiral(UnitTypes eGreatPersonUnit, int iX, int iY);

	int getGreatPeopleCreated() const;
	void incrementGreatPeopleCreated();

	int getGreatGeneralsCreated() const;
	void incrementGreatGeneralsCreated();
	int getGreatAdmiralsCreated() const;
	void incrementGreatAdmiralsCreated();
	int getGreatWritersCreated() const;
	void incrementGreatWritersCreated();
	int getGreatArtistsCreated() const;
	void incrementGreatArtistsCreated();
	int getGreatMusiciansCreated() const;
	void incrementGreatMusiciansCreated();
#ifdef FREE_GREAT_PERSON
	int getGreatProphetsCreated() const;
	void incrementGreatProphetsCreated();
#endif
#if defined SEPARATE_GREAT_PEOPLE || defined SWEDEN_UA_REWORK
	int getGreatScientistsCreated() const;
	void incrementGreatScientistsCreated();
	int getGreatEngineersCreated() const;
	void incrementGreatEngineersCreated();
	int getGreatMerchantsCreated() const;
	void incrementGreatMerchantsCreated();
#endif

#ifdef SEPARATE_MERCHANTS
	int getGreatMerchantsCreated() const;
	void incrementGreatMerchantsCreated();
#endif

	int getMerchantsFromFaith() const;
	void incrementMerchantsFromFaith();
	int getScientistsFromFaith() const;
	void incrementScientistsFromFaith();
	int getWritersFromFaith() const;
	void incrementWritersFromFaith();
	int getArtistsFromFaith() const;
	void incrementArtistsFromFaith();
	int getMusiciansFromFaith() const;
	void incrementMusiciansFromFaith();
	int getGeneralsFromFaith() const;
	void incrementGeneralsFromFaith();
	int getAdmiralsFromFaith() const;
	void incrementAdmiralsFromFaith();
	int getEngineersFromFaith() const;
	void incrementEngineersFromFaith();
#ifdef BELIEF_TO_GLORY_OF_GOD_ONE_GP_OF_EACH_TYPE
	bool getbMerchantsFromFaith() const;
	void setMerchantsFromFaith(bool bNewValue);
	bool getbScientistsFromFaith() const;
	void setScientistsFromFaith(bool bNewValue);
	bool getbWritersFromFaith() const;
	void setWritersFromFaith(bool bNewValue);
	bool getbArtistsFromFaith() const;
	void setArtistsFromFaith(bool bNewValue);
	bool getbMusiciansFromFaith() const;
	void setMusiciansFromFaith(bool bNewValue);
	bool getbGeneralsFromFaith() const;
	void setGeneralsFromFaith(bool bNewValue);
	bool getbAdmiralsFromFaith() const;
	void setAdmiralsFromFaith(bool bNewValue);
	bool getbEngineersFromFaith() const;
	void setEngineersFromFaith(bool bNewValue);
#endif

	int getGreatPeopleThresholdModifier() const;
	void changeGreatPeopleThresholdModifier(int iChange);

	int getGreatGeneralsThresholdModifier() const;
	void changeGreatGeneralsThresholdModifier(int iChange);
	int getGreatAdmiralsThresholdModifier() const;
	void changeGreatAdmiralsThresholdModifier(int iChange);

	int getPolicyCostModifier() const;
	void recomputePolicyCostModifier();

	int getGreatPeopleRateModifier() const;
	int getGreatGeneralRateModifier() const;
	int getGreatAdmiralRateModifier() const;
	int getGreatWriterRateModifier() const;
	int getGreatArtistRateModifier() const;
	int getGreatMusicianRateModifier() const;
	int getGreatMerchantRateModifier() const;
	int getGreatScientistRateModifier() const;
	int getGreatEngineerRateModifier() const;
	int getDomesticGreatGeneralRateModifier() const;
	void changeGreatPeopleRateModFromBldgs(int ichange);
	void changeGreatGeneralRateModFromBldgs(int ichange);
	void recomputeGreatPeopleModifiers();
	int GetGreatPeopleRateModFromFriendships() const;

	int GetGreatScientistBeakerMod() const;
	void SetGreatScientistBeakerMod(int iValue);
	void ChangeGreatScientistBeakerMod(int iChange);

	int GetGreatGeneralCombatBonus() const;
	void SetGreatGeneralCombatBonus(int iValue);

	// Unit Killed in Combat
	void DoUnitKilledCombat(PlayerTypes eKilledPlayer, UnitTypes eUnit);

	// Great People Expenditure
	void DoGreatPersonExpended(UnitTypes eGreatPersonUnit);
	int GetGreatPersonExpendGold() const;
	void ChangeGreatPersonExpendGold(int iChange);

	// Great People Spawning
	void DoSeedGreatPeopleSpawnCounter();
	void DoApplyNewAllyGPBonus();
	int GetGreatPeopleSpawnCounter();
	void SetGreatPeopleSpawnCounter(int iValue);
	void ChangeGreatPeopleSpawnCounter(int iChange);

	void DoSpawnGreatPerson(PlayerTypes eMinor);
	void DoGreatPeopleSpawnTurn();
	CvCity* GetGreatPersonSpawnCity(UnitTypes eUnit);

	// End Great People Stuff

	int getMaxGlobalBuildingProductionModifier() const;
	void changeMaxGlobalBuildingProductionModifier(int iChange);

	int getMaxTeamBuildingProductionModifier() const;
	void changeMaxTeamBuildingProductionModifier(int iChange);

	int getMaxPlayerBuildingProductionModifier() const;
	void changeMaxPlayerBuildingProductionModifier(int iChange);

	int getFreeExperience() const;
	void changeFreeExperienceFromBldgs(int ichange);
	void changeFreeExperienceFromMinors(int ichange);
	void recomputeFreeExperience();

	void doUpdateBarbarianCampVisibility();

	int getFeatureProductionModifier() const;
	void changeFeatureProductionModifier(int iChange);

	int getWorkerSpeedModifier() const;
	void changeWorkerSpeedModifier(int iChange);

	int getImprovementCostModifier() const;
	void changeImprovementCostModifier(int iChange);

	int getImprovementUpgradeRateModifier() const;
	void changeImprovementUpgradeRateModifier(int iChange);

	int getSpecialistProductionModifier() const;
	void changeSpecialistProductionModifier(int iChange);

	int getMilitaryProductionModifier() const;
	void changeMilitaryProductionModifier(int iChange);

	int getSpaceProductionModifier() const;
	void changeSpaceProductionModifier(int iChange);

	int getCityDefenseModifier() const;
	void changeCityDefenseModifier(int iChange);

	int getUnitFortificationModifier() const;
	void changeUnitFortificationModifier(int iChange);

	int getUnitBaseHealModifier() const;
	void changeUnitBaseHealModifier(int iChange);

	int getWonderProductionModifier() const;
	void changeWonderProductionModifier(int iChange);

	int getSettlerProductionModifier() const;
	void changeSettlerProductionModifier(int iChange);

	int getCapitalSettlerProductionModifier() const;
	void changeCapitalSettlerProductionModifier(int iChange);

	int GetPolicyCostBuildingModifier() const;
	void ChangePolicyCostBuildingModifier(int iChange);

	int GetPolicyCostMinorCivModifier() const;
	void ChangePolicyCostMinorCivModifier(int iChange);

	int GetInfluenceSpreadModifier() const;
	void ChangeInfluenceSpreadModifier(int iChange);

	int GetExtraVotesPerDiplomat() const;
	void ChangeExtraVotesPerDiplomat(int iChange);

	int getNumNukeUnits() const;
	void changeNumNukeUnits(int iChange);

	int getBaseFreeUnits() const;
	void changeBaseFreeUnits(int iChange);

	int getNumOutsideUnits();
	void changeNumOutsideUnits(int iChange);
	int GetVerifiedOutsideUnitCount();

	int getGoldPerUnit() const;
	void changeGoldPerUnit(int iChange);
	int getGoldPerUnitTimes100() const;
	void changeGoldPerUnitTimes100(int iChange);

	int getGoldPerMilitaryUnit() const;
	void changeGoldPerMilitaryUnit(int iChange);

	int GetRouteGoldMaintenanceMod() const;
	void ChangeRouteGoldMaintenanceMod(int iChange);

	int GetBuildingGoldMaintenanceMod() const;
	void ChangeBuildingGoldMaintenanceMod(int iChange);

	int GetUnitGoldMaintenanceMod() const;
	void ChangeUnitGoldMaintenanceMod(int iChange);

	int GetUnitSupplyMod() const;
	void ChangeUnitSupplyMod(int iChange);

	int getExtraUnitCost() const;
	void changeExtraUnitCost(int iChange);

	int GetNumMaintenanceFreeUnits(DomainTypes eDomain = NO_DOMAIN, bool bOnlyCombatUnits = false) const;

	int getNumMilitaryUnits() const;
	void changeNumMilitaryUnits(int iChange);

	int getHappyPerMilitaryUnit() const;
	void changeHappyPerMilitaryUnit(int iChange);

	int getHappinessToCulture() const;
	void changeHappinessToCulture(int iChange);

	int getHappinessToScience() const;
	void changeHappinessToScience(int iChange);

	int getHalfSpecialistUnhappinessCount() const;
	bool isHalfSpecialistUnhappiness() const;
	void changeHalfSpecialistUnhappinessCount(int iChange);

	int getHalfSpecialistFoodCount() const;
	bool isHalfSpecialistFood() const;
	void changeHalfSpecialistFoodCount(int iChange);

	int getMilitaryFoodProductionCount() const;
	bool isMilitaryFoodProduction() const;
	void changeMilitaryFoodProductionCount(int iChange);

	int GetGoldenAgeCultureBonusDisabledCount() const;
	bool IsGoldenAgeCultureBonusDisabled() const;
	void ChangeGoldenAgeCultureBonusDisabledCount(int iChange);

	int GetSecondReligionPantheonCount() const;
	bool IsSecondReligionPantheon() const;
	void ChangeSecondReligionPantheonCount(int iChange);

	int GetEnablesSSPartHurryCount() const;
	bool IsEnablesSSPartHurry() const;
	void ChangeEnablesSSPartHurryCount(int iChange);

	int GetEnablesSSPartPurchaseCount() const;
	bool IsEnablesSSPartPurchase() const;
	void ChangeEnablesSSPartPurchaseCount(int iChange);

	int getHighestUnitLevel() const;
	void setHighestUnitLevel(int iNewValue);

	int getConscriptCount() const;
	void setConscriptCount(int iNewValue);
	void changeConscriptCount(int iChange);

	int getMaxConscript() const;
	void changeMaxConscript(int iChange);

	int getOverflowResearch() const;
	void setOverflowResearch(int iNewValue);
	void changeOverflowResearch(int iChange);
	int getOverflowResearchTimes100() const;
	void setOverflowResearchTimes100(int iNewValue);
	void changeOverflowResearchTimes100(int iChange);

	int getExpModifier() const;
	void changeExpModifier(int iChange);

	int getExpInBorderModifier() const;
	void changeExpInBorderModifier(int iChange);

	int getLevelExperienceModifier() const;
	void changeLevelExperienceModifier(int iChange);

	int getMinorQuestFriendshipMod() const;
	void changeMinorQuestFriendshipMod(int iChange);

	int getMinorGoldFriendshipMod() const;
	void changeMinorGoldFriendshipMod(int iChange);

	int GetMinorFriendshipAnchorMod() const;
	void SetMinorFriendshipAnchorMod(int iValue);
	void ChangeMinorFriendshipAnchorMod(int iChange);

	int GetMinorFriendshipDecayMod() const;
	void changeGetMinorFriendshipDecayMod(int iChange);

	bool IsMinorScienceAllies() const;
	int GetMinorScienceAlliesCount() const;
	void ChangeMinorScienceAlliesCount(int iChange);

	bool IsMinorResourceBonus() const;
	int GetMinorResourceBonusCount() const;
	void ChangeMinorResourceBonusCount(int iChange);

	bool IsAbleToAnnexCityStates() const;
	int GetAbleToAnnexCityStatesCount() const;
	void ChangeAbleToAnnexCityStatesCount(int iChange);

	int getCultureBombTimer() const;
	void setCultureBombTimer(int iNewValue);
	void changeCultureBombTimer(int iChange);

	int getConversionTimer() const;
	void setConversionTimer(int iNewValue);
	void changeConversionTimer(int iChange);

	CvCity* getCapitalCity();
	const CvCity* getCapitalCity() const;
	void setCapitalCity(CvCity* pNewCapitalCity);

	int GetOriginalCapitalX() const;
	int GetOriginalCapitalY() const;

	bool IsHasLostCapital() const;
	void SetHasLostCapital(bool bValue, PlayerTypes eConqueror);
	PlayerTypes GetCapitalConqueror() const;

	int getCitiesLost() const;
	void changeCitiesLost(int iChange);

	int getPower() const;
	int GetMilitaryMight() const;
	int GetEconomicMight() const;
	int calculateMilitaryMight() const;
	int calculateEconomicMight() const;
	int calculateProductionMight() const;

	int getCombatExperience() const;
	void setCombatExperience(int iExperience);
	void changeCombatExperience(int iChange);
	int getLifetimeCombatExperience() const;
	int getNavalCombatExperience() const;
	void setNavalCombatExperience(int iExperience);
	void changeNavalCombatExperience(int iChange);

	int getBorderObstacleCount() const;
	bool isBorderObstacle() const;
	void changeBorderObstacleCount(int iChange);

	int getNetID() const;
	void setNetID(int iNetID);
	bool isConnected() const;
	void sendTurnReminder();

	uint getStartTime() const;
	void setStartTime(uint uiStartTime);
	uint getTotalTimePlayed() const;

	bool isMinorCiv() const;
	bool IsHasBetrayedMinorCiv() const;
	void SetHasBetrayedMinorCiv(bool bValue);

#ifdef CAN_BUILD_OU_AND_NIA_ONLY_ONCE
	bool isOxfordUniversityWasEverBuilt() const;
	void setOxfordUniversityWasEverBuilt(bool bNewValue);
	bool isNationalIntelligenceAgencyWasEverBuilt() const;
	void setNationalIntelligenceAgencyWasEverBuilt(bool bNewValue);
#endif

	void setAlive(bool bNewValue, bool bNotify = true);
	void verifyAlive();
	bool isAlive() const
	{
		return m_bAlive;
	}

	bool isEverAlive() const
	{
		return m_bEverAlive;
	}

	void setBeingResurrected(bool bNewValue);
	bool isBeingResurrected() const
	{
		return m_bBeingResurrected;
	}

	bool isTurnActive() const;
	void setTurnActive(bool bNewValue, bool bDoTurn = true);
	bool isSimultaneousTurns() const;
	void setDynamicTurnsSimultMode(bool simultaneousTurns);

	bool isAutoMoves() const;
	void setAutoMoves(bool bNewValue);
	bool hasProcessedAutoMoves() const;
	void setProcessedAutoMoves(bool bNewValue);

	void setTurnActiveForPbem(bool bActive);

	bool isPbemNewTurn() const;
	void setPbemNewTurn(bool bNew);

	bool isEndTurn() const;
	void setEndTurn(bool bNewValue);

	EndTurnBlockingTypes GetEndTurnBlockingType(void) const;
	int GetEndTurnBlockingNotificationIndex(void) const;
	void SetEndTurnBlocking(EndTurnBlockingTypes eBlockingType, int iNotificationIndex);

	bool isTurnDone() const;
	bool isLocalPlayer() const;
	bool isExtendedGame() const;
	void makeExtendedGame();

	bool isFoundedFirstCity() const;
	void setFoundedFirstCity(bool bNewValue);
	int GetNumCitiesFounded() const;
	void ChangeNumCitiesFounded(int iValue);

	// slewis - centralizing code where a player gets whacked by another player
	void CheckForMurder(PlayerTypes ePossibleVictimPlayer); // check to see if we defeated this other player

	bool isStrike() const;
	void setStrike(bool bNewValue);

	bool IsCramped() const;
	void DoUpdateCramped();

	CvHandicapInfo& getHandicapInfo() const;
	HandicapTypes getHandicapType() const;

	CvCivilizationInfo& getCivilizationInfo() const;
	CivilizationTypes getCivilizationType() const;

	CvLeaderHeadInfo& getLeaderInfo() const;
	LeaderHeadTypes getLeaderType() const;

	LeaderHeadTypes getPersonalityType() const;
	void setPersonalityType(LeaderHeadTypes eNewValue);

	EraTypes GetCurrentEra() const;

	PlayerTypes GetID() const
	{
		return m_eID;
	}

	static TeamTypes getTeam(PlayerTypes ePlayerID)
	{
		return CvPreGame::teamType(ePlayerID);
	}

	TeamTypes getTeam() const
	{
		return CvPreGame::teamType(m_eID);
	}

	void setTeam(TeamTypes eTeam);
	bool IsAITeammateOfHuman() const;

	PlayerColorTypes getPlayerColor() const;
	const CvColorA& getPlayerTextColor() const;

	int getSeaPlotYield(YieldTypes eIndex) const;
	void changeSeaPlotYield(YieldTypes eIndex, int iChange);

	int getYieldRateModifier(YieldTypes eIndex) const;
	void changeYieldRateModifier(YieldTypes eIndex, int iChange);

	int getCapitalYieldRateModifier(YieldTypes eIndex) const;
	void changeCapitalYieldRateModifier(YieldTypes eIndex, int iChange);

	int getExtraYieldThreshold(YieldTypes eIndex) const;
	void updateExtraYieldThreshold(YieldTypes eIndex);

	// Science

	int GetScience() const;
	int GetScienceTimes100() const;

#ifdef NEW_CITY_STATES_TYPES
	int GetSciencePerTurnFromMinorCivsTimes100() const;
	int GetSciencePerTurnFromMinorTimes100(PlayerTypes eMinor) const;
#endif

#ifdef SCIENCE_FROM_INFLUENCED_CIVS
	int GetSciencePerTurnFromInfluencedCivsTimes100() const;
#endif

#ifdef BELIEF_INTERFAITH_DIALOGUE_PER_FOLLOWERS
	int GetSciencePerTurnFromReligionTimes100() const;
#endif

	int GetScienceFromCitiesTimes100(bool bIgnoreTrade) const;
	int GetScienceFromOtherPlayersTimes100() const;
	int GetScienceFromHappinessTimes100() const;
	int GetScienceFromResearchAgreementsTimes100() const;
	int GetScienceFromBudgetDeficitTimes100() const;

	int GetScienceYieldFromPreviousTurns(int iGameTurn, int iNumPreviousTurnsToCount);

	bool IsGetsScienceFromPlayer(PlayerTypes ePlayer) const;
	void SetGetsScienceFromPlayer(PlayerTypes ePlayer, bool bValue);

	// END Science

#ifdef UNIT_DISBAND_REWORK
	void DoDeficit(int iValue);
#else
	void DoDeficit();
#endif

	int getSpecialistExtraYield(YieldTypes eIndex) const;
	void changeSpecialistExtraYield(YieldTypes eIndex, int iChange);

#ifdef POLICY_GOLDEN_AGE_YIELD_MOD
	int getGoldenAgeYieldModifier(YieldTypes eIndex) const;
	void changeGoldenAgeYieldModifier(YieldTypes eIndex, int iChange);
#endif

#ifdef POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
	int getPlotExtraYieldFromTradeRoute(YieldTypes eIndex) const;
	void changePlotExtraYieldFromTradeRoute(YieldTypes eIndex, int iChange);
#endif

	PlayerProximityTypes GetProximityToPlayer(PlayerTypes ePlayer) const;
	void SetProximityToPlayer(PlayerTypes ePlayer, PlayerProximityTypes eProximity);
	void DoUpdateProximityToPlayer(PlayerTypes ePlayer);

	void UpdateResearchAgreements(int iValue);
	int GetResearchAgreementCounter(PlayerTypes ePlayer) const;
	void SetResearchAgreementCounter(PlayerTypes ePlayer, int iValue);
	void ChangeResearchAgreementCounter(PlayerTypes ePlayer, int iChange);

	void DoCivilianReturnLogic(bool bReturn, PlayerTypes eToPlayer, int iUnitID);

	// Incoming Units
	void DoIncomingUnits();

	void DoDistanceGift(PlayerTypes eFromPlayer, CvUnit* pUnit);
	void AddIncomingUnit(PlayerTypes eFromPlayer, CvUnit* pUnit);

	UnitTypes GetIncomingUnitType(PlayerTypes eFromPlayer) const;
	void SetIncomingUnitType(PlayerTypes eFromPlayer, UnitTypes eUnitType);

	int GetIncomingUnitCountdown(PlayerTypes eFromPlayer) const;
	void SetIncomingUnitCountdown(PlayerTypes eFromPlayer, int iNumTurns);
	void ChangeIncomingUnitCountdown(PlayerTypes eFromPlayer, int iChange);

	bool isOption(PlayerOptionTypes eIndex) const;
	void setOption(PlayerOptionTypes eIndex, bool bNewValue);

	bool isPlayable() const;
	void setPlayable(bool bNewValue);

	int getNumResourceUsed(ResourceTypes eIndex) const;
	void changeNumResourceUsed(ResourceTypes eIndex, int iChange);
	int getNumResourceTotal(ResourceTypes eIndex, bool bIncludeImport = true) const;
	void changeNumResourceTotal(ResourceTypes eIndex, int iChange, bool bIgnoreResourceWarning = false);

	int getSiphonLuxuryCount(PlayerTypes eFromPlayer) const;
	void changeSiphonLuxuryCount(PlayerTypes eFromPlayer, int iChange);
	
	void UpdateResourcesSiphoned();

	void DoTestOverResourceNotification(ResourceTypes eIndex);

	int GetStrategicResourceMod() const;
	void ChangeStrategicResourceMod(int iChange);

	int getNumResourceAvailable(ResourceTypes eIndex, bool bIncludeImport = true) const;

	int getResourceGiftedToMinors(ResourceTypes eIndex) const;
	void changeResourceGiftedToMinors(ResourceTypes eIndex, int iChange);

	int getResourceExport(ResourceTypes eIndex) const;
	void changeResourceExport(ResourceTypes eIndex, int iChange);

	int getResourceImport(ResourceTypes eIndex) const;
	void changeResourceImport(ResourceTypes eIndex, int iChange);

	int getResourceFromMinors(ResourceTypes eIndex) const;
	void changeResourceFromMinors(ResourceTypes eIndex, int iChange);

	int getResourceSiphoned(ResourceTypes eIndex) const;
	void changeResourceSiphoned(ResourceTypes eIndex, int iChange);

	int getResourceInOwnedPlots(ResourceTypes eIndex);

	int getTotalImprovementsBuilt() const;
	void changeTotalImprovementsBuilt(int iChange);
	int getImprovementCount(ImprovementTypes eIndex) const;
	void changeImprovementCount(ImprovementTypes eIndex, int iChange);

	int getGreatPersonImprovementCount();

	int getFreeBuildingCount(BuildingTypes eIndex) const;
	bool isBuildingFree(BuildingTypes eIndex) const;
	void changeFreeBuildingCount(BuildingTypes eIndex, int iChange);

	int GetFreePromotionCount(PromotionTypes eIndex) const;
	bool IsFreePromotion(PromotionTypes eIndex) const;
	void ChangeFreePromotionCount(PromotionTypes eIndex, int iChange);

	int getUnitCombatProductionModifiers(UnitCombatTypes eIndex) const;
	void changeUnitCombatProductionModifiers(UnitCombatTypes eIndex, int iChange);
	int getUnitCombatFreeExperiences(UnitCombatTypes eIndex) const;
	void changeUnitCombatFreeExperiences(UnitCombatTypes eIndex, int iChange);

	int getUnitClassCount(UnitClassTypes eIndex) const;
	bool isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra = 0) const;
	void changeUnitClassCount(UnitClassTypes eIndex, int iChange);
	int getUnitClassMaking(UnitClassTypes eIndex) const;
	void changeUnitClassMaking(UnitClassTypes eIndex, int iChange);
	int getUnitClassCountPlusMaking(UnitClassTypes eIndex) const;

	int getBuildingClassCount(BuildingClassTypes eIndex) const;
	bool isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra = 0) const;
	void changeBuildingClassCount(BuildingClassTypes eIndex, int iChange);
	int getBuildingClassMaking(BuildingClassTypes eIndex) const;
	void changeBuildingClassMaking(BuildingClassTypes eIndex, int iChange);
	int getBuildingClassCountPlusMaking(BuildingClassTypes eIndex) const;

	int getProjectMaking(ProjectTypes eIndex) const;
	void changeProjectMaking(ProjectTypes eIndex, int iChange);

	int getHurryCount(HurryTypes eIndex) const;
	bool IsHasAccessToHurry(HurryTypes eIndex) const;

	bool IsCanHurry(HurryTypes eIndex) const;
	int GetHurryGoldCost(HurryTypes eHurry) const;
	void DoHurry(HurryTypes eIndex);

	bool canPopRush();
	void changeHurryCount(HurryTypes eIndex, int iChange);
	int getHurryModifier(HurryTypes eIndex) const;
	void changeHurryModifier(HurryTypes eIndex, int iChange);

	void setResearchingTech(TechTypes eIndex, bool bNewValue);

	int getSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2) const;
	void changeSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const;
	void changeImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange);

	CvUnitCycler& GetUnitCycler() { return m_UnitCycle; };

	bool removeFromArmy(int iArmyID, int iID);
	bool removeFromArmy(int iID);

	int findPathLength(TechTypes eTech, bool bCost = true) const;
	int getQueuePosition(TechTypes eTech) const;
	void clearResearchQueue();
	bool pushResearch(TechTypes eTech, bool bClear = false);
	void popResearch(TechTypes eTech);
	int getLengthResearchQueue() const;
	CLLNode<TechTypes>* nextResearchQueueNode(CLLNode<TechTypes>* pNode);
	const CLLNode<TechTypes>* nextResearchQueueNode(const CLLNode<TechTypes>* pNode) const;
	CLLNode<TechTypes>* headResearchQueueNode();
	const CLLNode<TechTypes>* headResearchQueueNode() const;
	CLLNode<TechTypes>* tailResearchQueueNode();

	void addCityName(const CvString& szName);
	int getNumCityNames() const;
	CvString getCityName(int iIndex) const;
	CLLNode<CvString>* nextCityNameNode(CLLNode<CvString>* pNode);
	const CLLNode<CvString>* nextCityNameNode(const CLLNode<CvString>* pNode) const;
	CLLNode<CvString>* headCityNameNode();
	const CLLNode<CvString>* headCityNameNode() const;

	// city iteration
	const CvCity* firstCity(int* pIterIdx, bool bRev=false) const;
	CvCity* firstCity(int* pIterIdx, bool bRev=false);
	const CvCity* nextCity(int* pIterIdx, bool bRev=false) const;
	CvCity* nextCity(int* pIterIdx, bool bRev=false);
	int getNumCities() const;
	CvCity* getCity(int iID);
	const CvCity* getCity(int iID) const;
	CvCity* addCity();
	void deleteCity(int iID);
	CvCity* GetFirstCityWithBuildingClass(BuildingClassTypes eBuildingClass);

	// unit iteration
	int getNumUnits() const;
	const CvUnit* firstUnit(int* pIterIdx, bool bRev=false) const;
	const CvUnit* nextUnit(int* pIterIdx, bool bRev=false) const;
	const CvUnit* getUnit(int iID) const;
	CvUnit* firstUnit(int* pIterIdx, bool bRev=false);
	CvUnit* nextUnit(int* pIterIdx, bool bRev=false);
	CvUnit* getUnit(int iID);
	CvUnit* addUnit();
	void deleteUnit(int iID);

	// army iteration
	const CvArmyAI* firstArmyAI(int* pIterIdx, bool bRev=false) const;
	const CvArmyAI* nextArmyAI(int* pIterIdx, bool bRev=false) const;
	CvArmyAI* firstArmyAI(int* pIterIdx, bool bRev=false);
	CvArmyAI* nextArmyAI(int* pIterIdx, bool bRev=false);
	int getNumArmyAIs() const;
	const CvArmyAI* getArmyAI(int iID) const;
	CvArmyAI* getArmyAI(int iID);
	CvArmyAI* addArmyAI();
	void deleteArmyAI(int iID);

	// operations
	CvAIOperation* getFirstAIOperation();
	CvAIOperation* getNextAIOperation();
	CvAIOperation* getAIOperation(int iID);
	const CvAIOperation* getAIOperation(int iID) const;
	CvAIOperation* addAIOperation(int iOperationType, PlayerTypes eEnemy=NO_PLAYER, int iArea=-1, CvCity* pTarget=NULL, CvCity* pMuster=NULL);
	void deleteAIOperation(int iID);
	bool haveAIOperationOfType(int iOperationType, int* piID=NULL, PlayerTypes eTargetPlayer = NO_PLAYER, CvPlot* pTargetPlot=NULL);
	int numOperationsOfType(int iOperationType);
	bool IsCityAlreadyTargeted(CvCity* pCity, DomainTypes eDomain=NO_DOMAIN, int iPercentToTarget=100, int iIgnoreOperationID=-1) const;
	bool IsPlotTargetedForCity(CvPlot *pPlot) const;
	void GatherPerTurnReplayStats(int iGameTurn);
	unsigned int getNumReplayDataSets() const;
	const char* getReplayDataSetName(unsigned int uiDataSet) const;
#ifdef DEV_RECORDING_STATISTICS
	const char* getReplayDataSetDesc(unsigned int uiDataSet) const;
#endif
	unsigned int getReplayDataSetIndex(const char* szDataSetName);
	int getReplayDataValue(unsigned int uiDataSet, unsigned int uiTurn) const;
	void setReplayDataValue(unsigned int uiDataSet, unsigned int uiTurn, int iValue);
	TurnData getReplayDataHistory(unsigned int uiDataSet) const;

	// Arbitrary Script Data
	std::string getScriptData() const;
	void setScriptData(std::string szNewValue);

	const CvString& getPbemEmailAddress() const;
	void setPbemEmailAddress(const char* szAddress);

	int getUnitExtraCost(UnitClassTypes eUnitClass) const;
	void setUnitExtraCost(UnitClassTypes eUnitClass, int iCost);

	void launch(VictoryTypes victoryType);

	void invalidatePopulationRankCache();
	void invalidateYieldRankCache(YieldTypes eYield = NO_YIELD);

	PlayerTypes pickConqueredCityOwner(const CvCity& kCity) const;

	int getNewCityProductionValue() const;

	int getGrowthThreshold(int iPopulation) const;

	int GetCityStrengthMod() const;
	void SetCityStrengthMod(int iValue);
	void ChangeCityStrengthMod(int iChange);

	int GetCityGrowthMod() const;
	void SetCityGrowthMod(int iValue);
	void ChangeCityGrowthMod(int iChange);

	int GetCapitalGrowthMod() const;
	void SetCapitalGrowthMod(int iValue);
	void ChangeCapitalGrowthMod(int iChange);

	void InitPlots();  // this needs to be called after the map is inited. It makes the list of how many plots the player controls
	void UpdatePlots();  // Modifies the list of plots and sets which ones the player owns
	void AddAPlot(CvPlot* pPlot); // adds a plot at the end of the list
	CvPlotsVector& GetPlots();  // gets the list of plots the player owns
	int GetNumPlots() const;

	int GetNumPlotsBought() const;
	void SetNumPlotsBought(int iValue);
	void ChangeNumPlotsBought(int iChange);

	int GetBuyPlotCost() const;

	int GetPlotGoldCostMod() const;
	void ChangePlotGoldCostMod(int iChange);

	int GetPlotCultureCostModifier() const;
	void ChangePlotCultureCostModifier(int iChange);
	int GetPlotCultureExponentModifier() const;
	void ChangePlotCultureExponentModifier(int iChange);

	int GetNumCitiesPolicyCostDiscount() const;
	void ChangeNumCitiesPolicyCostDiscount(int iChange);

	int GetGarrisonedCityRangeStrikeModifier() const;
	void ChangeGarrisonedCityRangeStrikeModifier(int iChange);
	bool IsGarrisonFreeMaintenance() const;
	void ChangeGarrisonFreeMaintenanceCount(int iChange);

	int GetUnitPurchaseCostModifier() const;
	void ChangeUnitPurchaseCostModifier(int iChange);

	int GetPlotDanger(CvPlot& Plot) const;
	bool IsPlotUnderImmediateThreat(CvPlot& Plot) const;
	CvCity* GetClosestFriendlyCity(CvPlot& plot, int iSearchRadius);

	int GetNumPuppetCities() const;
	int GetMaxEffectiveCities(bool bIncludePuppets = false);

	int GetNumNaturalWondersDiscoveredInArea() const;
	void SetNumNaturalWondersDiscoveredInArea(int iValue);
	void ChangeNumNaturalWondersDiscoveredInArea(int iChange);

	int GetNumNaturalWondersInOwnedPlots();

	int GetTurnsSinceSettledLastCity() const;
	void SetTurnsSinceSettledLastCity(int iValue);
	void ChangeTurnsSinceSettledLastCity(int iChange);

	int GetBestSettleAreas(int iMinScore, int& iFirstArea, int& iSecondArea);
	CvPlot* GetBestSettlePlot(CvUnit* pUnit, bool bEscorted, int iArea = -1) const;

	// New Victory Stuff
	int GetNumWonders() const;
	void ChangeNumWonders(int iValue);
	int GetNumPolicies() const;
	void ChangeNumPolicies(int iValue);
	int GetNumGreatPeople() const;
	void ChangeNumGreatPeople(int iValue);
	// End New Victory Stuff

	void DoAdoptedGreatPersonCityStatePolicy();
	bool IsAlliesGreatPersonBiasApplied() const;
	void SetAlliesGreatPersonBiasApplied(bool bValue);

	// New Religion Stuff
	bool IsHasAdoptedStateReligion() const;
	void SetHasAdoptedStateReligion(bool bValue);

	int GetNumCitiesWithStateReligion();

	CvCity* GetHolyCity();
	void SetHolyCity(int iCityID);

	PromotionTypes GetEmbarkationPromotion() const;

	void DoAnnounceReligionAdoption();
	// End New Religion Stuff

	int GetNumFreeTechs() const;
	void SetNumFreeTechs(int iValue);

	int GetMedianTechPercentage() const;
	void ChangeMedianTechPercentage(int iValue);

	int GetNumFreeGreatPeople() const;
	void SetNumFreeGreatPeople(int iValue);
	void ChangeNumFreeGreatPeople(int iChange);

	int GetNumMayaBoosts() const;
	void SetNumMayaBoosts(int iValue);
	void ChangeNumMayaBoosts(int iChange);

	int GetNumFaithGreatPeople() const;
	void SetNumFaithGreatPeople(int iValue);
	void ChangeNumFaithGreatPeople(int iChange);

	int GetNumArchaeologyChoices() const;
	void SetNumArchaeologyChoices(int iValue);

	FaithPurchaseTypes GetFaithPurchaseType() const;
	void SetFaithPurchaseType(FaithPurchaseTypes eType);
	int GetFaithPurchaseIndex() const;
	void SetFaithPurchaseIndex(int iIndex);

	int GetNumFreePolicies() const;
	void SetNumFreePolicies(int iValue);
	void ChangeNumFreePolicies(int iChange);

	int GetNumFreePoliciesEver() const;
	void SetNumFreePoliciesEver(int iValue);
	void ChangeNumFreePoliciesEver(int iChange);

	int GetNumFreeTenets() const;
	void SetNumFreeTenets(int iValue, bool bCountAsFreePolicies);
	void ChangeNumFreeTenets(int iChange, bool bCountAsFreePolicies);

	int GetLastSliceMoved() const;
	void SetLastSliceMoved(int iValue);

	void SetEverConqueredBy(PlayerTypes ePlayer, bool bValue);
	bool IsEverConqueredBy(PlayerTypes ePlayer);

	// slewis Tutorial functions
	bool GetEverPoppedGoody(void);  // has this player ever popped a goody hut
	CvPlot* GetClosestGoodyPlot(bool bStopAfterFindingFirst = false);  // find the goody plot that has the closest unit that can reach it, null means none could be found
	bool GetPlotHasOrder(CvPlot* Plot);  // are any of the player's units directed to this plot?
	bool GetAnyUnitHasOrderToGoody(void);
	bool GetEverTrainedBuilder(void);
	// end Tutorial functions

	// International Trade
	bool IsAllowedToTradeWith(PlayerTypes eOtherPlayer);
	// end International Trade

#ifdef CS_ALLYING_WAR_RESCTRICTION
	int getTurnCSWarAllowing(PlayerTypes ePlayer);
	int getTurnCSWarAllowingMinor(PlayerTypes ePlayer, PlayerTypes eMinor);
	void setTurnCSWarAllowingMinor(PlayerTypes ePlayer, PlayerTypes eMinor, int iValue);
	float getTimeCSWarAllowing(PlayerTypes ePlayer);
	float getTimeCSWarAllowingMinor(PlayerTypes ePlayer, PlayerTypes eMinor);
	void setTimeCSWarAllowingMinor(PlayerTypes ePlayer, PlayerTypes eMinor, float fValue);
#endif

#ifdef PENALTY_FOR_DELAYING_POLICIES
	bool IsDelayedPolicy() const;
	void setIsDelayedPolicy(bool bValue);
#endif

#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
	int GetYieldForEachBuildingInEmpire(BuildingTypes eBuilding, YieldTypes eIndex) const;
	void ChangeYieldForEachBuildingInEmpire(BuildingTypes eBuilding, YieldTypes eIndex, int iChange);
#endif

#ifdef POLICY_ALLOWS_GP_BUYS_FOR_GOLD
	int GetNumGoldPurchasedGreatPerson() const;
	void ChangeNumGoldPurchasedGreatPerson(int iChange);
	bool IsGoldGreatPerson(UnitClassTypes eUnitClass) const;
	void SetGoldGreatPerson(UnitClassTypes eUnitClass, bool bValue);
#endif

	CvPlayerPolicies* GetPlayerPolicies() const;
	CvPlayerTraits* GetPlayerTraits() const;
	CvEconomicAI* GetEconomicAI() const;
	CvMilitaryAI* GetMilitaryAI() const;
	CvCitySpecializationAI* GetCitySpecializationAI() const;
	CvWonderProductionAI* GetWonderProductionAI() const;
	CvGrandStrategyAI* GetGrandStrategyAI() const;
	CvDiplomacyAI* GetDiplomacyAI() const;
	CvPlayerReligions* GetReligions() const;
	CvPlayerCulture* GetCulture() const;
	CvReligionAI* GetReligionAI() const;
	CvPlayerTechs* GetPlayerTechs() const;
	CvFlavorManager* GetFlavorManager() const;
	CvTacticalAI* GetTacticalAI() const;
	CvHomelandAI* GetHomelandAI() const;
	CvMinorCivAI* GetMinorCivAI() const;
	CvDealAI* GetDealAI() const;
	CvBuilderTaskingAI* GetBuilderTaskingAI() const;
	CvCityConnections* GetCityConnections() const;
	CvPlayerEspionage* GetEspionage() const;
	CvEspionageAI* GetEspionageAI() const;
	CvPlayerTrade* GetTrade() const;
	CvTradeAI* GetTradeAI() const;
	CvLeagueAI* GetLeagueAI() const;
	CvNotifications* GetNotifications() const;
	CvDiplomacyRequests* GetDiplomacyRequests() const;
	bool HasActiveDiplomacyRequests() const;

	CvTreasury* GetTreasury() const;

	int GetCityDistanceHighwaterMark() const;
	void SetCityDistanceHighwaterMark(int iNewValue);

	void SetEmbarkedGraphicOverride(CvString szGraphicName)
	{
		m_strEmbarkedGraphicOverride = szGraphicName;
	};
	const CvString& GetEmbarkedGraphicOverride() const
	{
		return m_strEmbarkedGraphicOverride;
	};

	// for serialization
	virtual void Read(FDataStream& kStream);
	virtual void Write(FDataStream& kStream) const;

	virtual void AI_init() = 0;
	virtual void AI_reset() = 0;
	virtual void AI_doTurnPre() = 0;
	virtual void AI_doTurnPost() = 0;
	virtual void AI_doTurnUnitsPre() = 0;
	virtual void AI_doTurnUnitsPost() = 0;
	virtual void AI_updateFoundValues(bool bStartingLoc = false) = 0;
	virtual void AI_unitUpdate() = 0;
	virtual void AI_conquerCity(CvCity* pCity, PlayerTypes eOldOwner) = 0;
	virtual int AI_foundValue(int iX, int iY, int iMinUnitRange = -1, bool bStartingLoc = false) = 0;
	virtual void AI_chooseFreeGreatPerson() = 0;
	virtual void AI_chooseFreeTech() = 0;
	virtual void AI_chooseResearch() = 0;
	virtual int AI_plotTargetMissionAIs(CvPlot* pPlot, MissionAITypes eMissionAI, int iRange = 0) = 0;
	virtual void AI_launch(VictoryTypes eVictory) = 0;

	virtual OperationSlot PeekAtNextUnitToBuildForOperationSlot(int iAreaID) = 0;
	virtual OperationSlot CityCommitToBuildUnitForOperationSlot(int iAreaID, int iTurns, CvCity* pCity) = 0;
	virtual void CityUncommitToBuildUnitForOperationSlot(OperationSlot thisSlot) = 0;
	virtual void CityFinishedBuildingUnitForOperationSlot(OperationSlot thisSlot, CvUnit* pThisUnit) = 0;
	virtual int GetNumUnitsNeededToBeBuilt() = 0;
	const FAutoArchive& getSyncArchive() const;
	FAutoArchive& getSyncArchive();
	void disconnected();
	void reconnected();
	bool hasBusyUnitUpdatesRemaining() const;
	void setBusyUnitUpdatesRemaining(int iUpdateCount);
	bool hasUnitsThatNeedAIUpdate() const;

	void checkInitialTurnAIProcessed();
	void checkRunAutoMovesForEveryone();
	std::string debugDump(const FAutoVariableBase&) const;
	std::string stackTraceRemark(const FAutoVariableBase&) const;

	CvPlayerAchievements& GetPlayerAchievements(){return m_kPlayerAchievements;}

	bool hasTurnTimerExpired();

protected:
	class ConqueredByBoolField
	{
	public:
		enum { eCount = 4, eSize = 32 };
		DWORD m_dwBits[eCount];

		bool GetBit(const uint uiEntry) const
		{
			const uint uiOffset = uiEntry/eSize;
			return m_dwBits[uiOffset] & 1<<(uiEntry-(eSize*uiOffset));
		}
		void SetBit(const uint uiEntry)
		{
			const uint uiOffset = uiEntry/eSize;
			m_dwBits[uiOffset] |= 1<<(uiEntry-(eSize*uiOffset));
		}
		void ClearBit(const uint uiEntry)
		{
			const uint uiOffset = uiEntry/eSize;
			m_dwBits[uiOffset] &= ~(1<<(uiEntry-(eSize*uiOffset)));
		}
		void ToggleBit(const uint uiEntry)
		{
			const uint uiOffset = uiEntry/eSize;
			m_dwBits[uiOffset] ^= 1<<(uiEntry-(eSize*uiOffset));
		}
		void ClearAll()
		{
			for(uint i = 0; i <eCount; ++i)
			{
				m_dwBits[i] = 0;
			}
		}

		bool ValidateFromBoolArray(const bool* pBools, uint uiCount) const
		{
			for(uint i = 0; i < uiCount; ++i)
				if(GetBit(i) != pBools[i]) return false;

			return true;
		}

		void InitFromBoolArray(bool* pBools, uint uiCount)
		{
			for(uint i = 0; i < uiCount; ++i)
				if(GetBit(i) != pBools[i]) ToggleBit(i);
		}
	};

	FAutoArchiveClassContainer<CvPlayer> m_syncArchive;

	FAutoVariable<PlayerTypes, CvPlayer> m_eID;
	FAutoVariable<LeaderHeadTypes, CvPlayer> m_ePersonalityType;

#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
	FAutoVariable<bool, CvPlayer> m_bIsDisconnected;
#endif
	FAutoVariable<int, CvPlayer> m_iStartingX;
	FAutoVariable<int, CvPlayer> m_iStartingY;
	FAutoVariable<int, CvPlayer> m_iTotalPopulation;
	FAutoVariable<int, CvPlayer> m_iTotalLand;
	FAutoVariable<int, CvPlayer> m_iTotalLandScored;
	FAutoVariable<int, CvPlayer> m_iJONSCulturePerTurnForFree;
	FAutoVariable<int, CvPlayer> m_iJONSCulturePerTurnFromMinorCivs;
	FAutoVariable<int, CvPlayer> m_iJONSCultureCityModifier;
	FAutoVariable<int, CvPlayer> m_iJONSCulture;
	FAutoVariable<int, CvPlayer> m_iJONSCultureEverGenerated;
	FAutoVariable<int, CvPlayer> m_iCulturePerWonder;
	FAutoVariable<int, CvPlayer> m_iCultureWonderMultiplier;
	FAutoVariable<int, CvPlayer> m_iCulturePerTechResearched;
	int m_iFaith;
	int m_iFaithEverGenerated;
	FAutoVariable<int, CvPlayer> m_iHappiness;
	FAutoVariable<int, CvPlayer> m_iUprisingCounter;
	FAutoVariable<int, CvPlayer> m_iExtraHappinessPerLuxury;
	FAutoVariable<int, CvPlayer> m_iUnhappinessFromUnits;
	FAutoVariable<int, CvPlayer> m_iUnhappinessFromUnitsMod;
	FAutoVariable<int, CvPlayer> m_iUnhappinessMod;
	FAutoVariable<int, CvPlayer> m_iCityCountUnhappinessMod;
	FAutoVariable<int, CvPlayer> m_iOccupiedPopulationUnhappinessMod;
	FAutoVariable<int, CvPlayer> m_iCapitalUnhappinessMod;
	FAutoVariable<int, CvPlayer> m_iCityRevoltCounter;
	FAutoVariable<int, CvPlayer> m_iHappinessPerGarrisonedUnitCount;
	FAutoVariable<int, CvPlayer> m_iHappinessPerTradeRouteCount;
	int m_iHappinessPerXPopulation;
	int m_iHappinessFromLeagues;
	FAutoVariable<int, CvPlayer> m_iSpecialPolicyBuildingHappiness;  //unused
	FAutoVariable<int, CvPlayer> m_iWoundedUnitDamageMod;
	FAutoVariable<int, CvPlayer> m_iUnitUpgradeCostMod;
	FAutoVariable<int, CvPlayer> m_iBarbarianCombatBonus;
	FAutoVariable<int, CvPlayer> m_iAlwaysSeeBarbCampsCount;
	FAutoVariable<int, CvPlayer> m_iHappinessFromBuildings;
	FAutoVariable<int, CvPlayer> m_iHappinessPerCity;
	int m_iHappinessPerXPolicies;
	int m_iEspionageModifier;
	int m_iSpyStartingRank;
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
	int m_iNumStolenScience;
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
	int m_iNumTrainedUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
	int m_iNumKilledUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
	int m_iNumLostUnits;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
	int m_iUnitsDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
	int m_iUnitsDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
	int m_iCitiesDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
	int m_iCitiesDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
	int m_iNumScientistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
	int m_iNumEngineersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
	int m_iNumMerchantsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
	int m_iNumWritersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
	int m_iNumArtistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
	int m_iNumMusiciansTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
	int m_iNumGeneralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
	int m_iNumAdmiralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
	int m_iNumProphetsTotal;
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
	int m_iProductionGoldFromWonders;
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
	int m_iNumChops;
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
	int m_iNumTimesOpenedDemographics;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
	int m_bMayaBoostScientist;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
	int m_bMayaBoostEngineers;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
	int m_bMayaBoostMerchants;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
	int m_bMayaBoostWriters;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
	int m_bMayaBoostArtists;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
	int m_bMayaBoostMusicians;
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
	int m_iScientistsTotalScienceBoost;
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
	int m_iEngineersTotalHurryBoost;
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
	int m_iMerchantsTotalTradeBoost;
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
	int m_iWritersTotalCultureBoost;
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
	int m_iMusiciansTotalTourismBoost;
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
	int m_iNumPopulationLostFromNukes;
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
	int m_iNumCSQuestsCompleted;
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
	int m_iNumAlliedCS;
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
	int m_iTimesEnteredCityScreen;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
	int m_iNumDiedSpies;
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
	int m_iNumKilledSpies;
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
	int m_iFoodFromMinorsTimes100;
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
	int m_iProductionFromMinorsTimes100;
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
	int m_iNumUnitsFromMinors;
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
	int m_iNumCreatedWorldWonders;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
	int m_iNumGoldSpentOnBuildingBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
	int m_iNumGoldSpentOnUnitBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
	int m_iNumGoldSpentOnUgrades;
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
	int m_iGoldFromKills;
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
	int m_iCultureFromKills;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
	int m_iNumGoldSpentOnGPBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
	int m_iNumGoldSpentOnTilesBuys;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
	int m_iNumGoldFromPillage;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
	int m_iNumGoldFromPlunder;
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
	int m_iNumFaithSpentOnMilitaryUnits;
#endif
	int m_iExtraLeagueVotes;
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
	int m_iMaxExtraVotesFromMinors;
#endif
#ifdef POLICY_EXTRA_VOTES
	int m_iPolicyExtraVotes;
#endif
#ifdef POLICY_DO_TECH_FROM_CITY_CONQ
	int m_iPolicyTechFromCityConquer;
#endif
#ifdef POLICY_NO_CULTURE_SPECIALIST_FOOD
	int m_iNoCultureSpecialistFood;
#endif
#ifdef POLICY_MINORS_GIFT_UNITS
	int m_iMinorsGiftUnits;
#endif
#ifdef POLICY_NO_CARGO_PILLAGE
	int m_iNoCargoPillage;
#endif
#ifdef POLICY_GREAT_WORK_HAPPINESS
	int m_iGreatWorkHappiness;
#endif
#ifdef POLICY_SCIENCE_PER_X_FOLLOWERS
	int m_iSciencePerXFollowers;
#endif
#ifdef POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD
	int m_iNoDifferentIdeologiesTourismMod;
#endif
#ifdef POLICY_GLOBAL_POP_CHANGE
	int m_iGlobalPopChange;
#endif
#ifdef POLICY_GREAT_WORK_TOURISM_CHANGES
	int m_iGreatWorkTourismChanges;
#endif
#ifdef POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP
	int m_iCityScienceSquaredModPerXPop;
#endif
	FAutoVariable<int, CvPlayer> m_iAdvancedStartPoints;
	FAutoVariable<int, CvPlayer> m_iAttackBonusTurns;
	int m_iCultureBonusTurns;
	int m_iTourismBonusTurns;
	FAutoVariable<int, CvPlayer> m_iGoldenAgeProgressMeter;
	FAutoVariable<int, CvPlayer> m_iGoldenAgeMeterMod;
	FAutoVariable<int, CvPlayer> m_iNumGoldenAges;
	FAutoVariable<int, CvPlayer> m_iGoldenAgeTurns;
#ifdef TAJ_MAHAL_STARTS_GA_NEXT_TURN
	FAutoVariable<int, CvPlayer> m_iBuildingGoldenAgeTurns;
#endif
	FAutoVariable<int, CvPlayer> m_iNumUnitGoldenAges;
	FAutoVariable<int, CvPlayer> m_iStrikeTurns;
	FAutoVariable<int, CvPlayer> m_iGoldenAgeModifier;
	FAutoVariable<int, CvPlayer> m_iGreatPeopleCreated;
	FAutoVariable<int, CvPlayer> m_iGreatGeneralsCreated;
	int m_iGreatAdmiralsCreated;
	int m_iGreatWritersCreated;
	int m_iGreatArtistsCreated;
	int m_iGreatMusiciansCreated;
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	bool m_bHasUsedDharma;
#endif
#ifdef UNDERGROUND_SECT_REWORK
	bool m_bHasUsedUndergroundSect;
#endif
#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
	bool m_bHasUsedMissionaryZeal;
#endif
#ifdef UNITY_OF_PROPHETS_EXTRA_PROPHETS
	bool m_bHasUsedUnityProphets;
#endif
#ifdef GODDESS_LOVE_FREE_WORKER
	bool m_bHasUsedGoddessLove;
#endif
#ifdef GOD_SEA_FREE_WORK_BOAT
	bool m_bHasUsedGodSea;
#endif
#ifdef FREE_GREAT_PERSON
	int m_iGreatProphetsCreated;
#endif
#if defined SEPARATE_GREAT_PEOPLE || defined SWEDEN_UA_REWORK
	int m_iGreatScientistsCreated;
	int m_iGreatEngineersCreated;
	int m_iGreatMerchantsCreated;
#endif
#ifdef SEPARATE_MERCHANTS
	int m_iGreatMerchantsCreated;
#endif
	int m_iMerchantsFromFaith;
	int m_iScientistsFromFaith;
	int m_iWritersFromFaith;
	int m_iArtistsFromFaith;
	int m_iMusiciansFromFaith;
	int m_iGeneralsFromFaith;
	int m_iAdmiralsFromFaith;
	int m_iEngineersFromFaith;
#ifdef BELIEF_TO_GLORY_OF_GOD_ONE_GP_OF_EACH_TYPE
	bool m_bMerchantsFromFaith;
	bool m_bScientistsFromFaith;
	bool m_bWritersFromFaith;
	bool m_bArtistsFromFaith;
	bool m_bMusiciansFromFaith;
	bool m_bGeneralsFromFaith;
	bool m_bAdmiralsFromFaith;
	bool m_bEngineersFromFaith;
#endif
	FAutoVariable<int, CvPlayer> m_iGreatPeopleThresholdModifier;
	FAutoVariable<int, CvPlayer> m_iGreatGeneralsThresholdModifier;
	int m_iGreatAdmiralsThresholdModifier;
	int m_iGreatGeneralCombatBonus;
	FAutoVariable<int, CvPlayer> m_iAnarchyNumTurns;
	FAutoVariable<int, CvPlayer> m_iPolicyCostModifier;
	FAutoVariable<int, CvPlayer> m_iGreatPeopleRateModifier;
	FAutoVariable<int, CvPlayer> m_iGreatPeopleRateModFromBldgs;
	FAutoVariable<int, CvPlayer> m_iGreatGeneralRateModifier;
	FAutoVariable<int, CvPlayer> m_iGreatGeneralRateModFromBldgs;
	FAutoVariable<int, CvPlayer> m_iDomesticGreatGeneralRateModifier;
	FAutoVariable<int, CvPlayer> m_iDomesticGreatGeneralRateModFromBldgs;
	int m_iGreatAdmiralRateModifier;
	int m_iGreatWriterRateModifier;
	int m_iGreatArtistRateModifier;
	int m_iGreatMusicianRateModifier;
	int m_iGreatMerchantRateModifier;
	int m_iGreatScientistRateModifier;
	int m_iGreatScientistBeakerModifier;
	int m_iGreatEngineerRateModifier;
	int m_iGreatPersonExpendGold;
	FAutoVariable<int, CvPlayer> m_iMaxGlobalBuildingProductionModifier;
	FAutoVariable<int, CvPlayer> m_iMaxTeamBuildingProductionModifier;
	FAutoVariable<int, CvPlayer> m_iMaxPlayerBuildingProductionModifier;
	FAutoVariable<int, CvPlayer> m_iFreeExperience;
	FAutoVariable<int, CvPlayer> m_iFreeExperienceFromBldgs;
	FAutoVariable<int, CvPlayer> m_iFreeExperienceFromMinors;
	FAutoVariable<int, CvPlayer> m_iFeatureProductionModifier;
	FAutoVariable<int, CvPlayer> m_iWorkerSpeedModifier;
	FAutoVariable<int, CvPlayer> m_iImprovementCostModifier;
	FAutoVariable<int, CvPlayer> m_iImprovementUpgradeRateModifier;
	FAutoVariable<int, CvPlayer> m_iSpecialistProductionModifier;
	FAutoVariable<int, CvPlayer> m_iMilitaryProductionModifier;
	FAutoVariable<int, CvPlayer> m_iSpaceProductionModifier;
	FAutoVariable<int, CvPlayer> m_iCityDefenseModifier;
	FAutoVariable<int, CvPlayer> m_iUnitFortificationModifier;
	FAutoVariable<int, CvPlayer> m_iUnitBaseHealModifier;
	FAutoVariable<int, CvPlayer> m_iWonderProductionModifier;
	FAutoVariable<int, CvPlayer> m_iSettlerProductionModifier;
	FAutoVariable<int, CvPlayer> m_iCapitalSettlerProductionModifier;
	FAutoVariable<int, CvPlayer> m_iUnitProductionMaintenanceMod;
	FAutoVariable<int, CvPlayer> m_iPolicyCostBuildingModifier;
	FAutoVariable<int, CvPlayer> m_iPolicyCostMinorCivModifier;
	int m_iInfluenceSpreadModifier;
	int m_iExtraVotesPerDiplomat;
	FAutoVariable<int, CvPlayer> m_iNumNukeUnits;
	FAutoVariable<int, CvPlayer> m_iNumOutsideUnits;
	FAutoVariable<int, CvPlayer> m_iBaseFreeUnits;
	FAutoVariable<int, CvPlayer> m_iBaseFreeMilitaryUnits;
	FAutoVariable<int, CvPlayer> m_iFreeUnitsPopulationPercent;
	FAutoVariable<int, CvPlayer> m_iFreeMilitaryUnitsPopulationPercent;
	FAutoVariable<int, CvPlayer> m_iGoldPerUnit;
	FAutoVariable<int, CvPlayer> m_iGoldPerMilitaryUnit;
	FAutoVariable<int, CvPlayer> m_iRouteGoldMaintenanceMod;
	FAutoVariable<int, CvPlayer> m_iBuildingGoldMaintenanceMod;
	FAutoVariable<int, CvPlayer> m_iUnitGoldMaintenanceMod;
	FAutoVariable<int, CvPlayer> m_iUnitSupplyMod;
	FAutoVariable<int, CvPlayer> m_iExtraUnitCost;
	FAutoVariable<int, CvPlayer> m_iNumMilitaryUnits;
	FAutoVariable<int, CvPlayer> m_iHappyPerMilitaryUnit;
	FAutoVariable<int, CvPlayer> m_iHappinessToCulture;
	FAutoVariable<int, CvPlayer> m_iHappinessToScience;
	FAutoVariable<int, CvPlayer> m_iHalfSpecialistUnhappinessCount;
	FAutoVariable<int, CvPlayer> m_iHalfSpecialistFoodCount;
	FAutoVariable<int, CvPlayer> m_iMilitaryFoodProductionCount;
	int m_iGoldenAgeCultureBonusDisabledCount;
	int m_iSecondReligionPantheonCount;
	int m_iEnablesSSPartHurryCount;
	int m_iEnablesSSPartPurchaseCount;
	FAutoVariable<int, CvPlayer> m_iConscriptCount;
	FAutoVariable<int, CvPlayer> m_iMaxConscript;
	FAutoVariable<int, CvPlayer> m_iHighestUnitLevel;
	FAutoVariable<int, CvPlayer> m_iOverflowResearch;
	FAutoVariable<int, CvPlayer> m_iExpModifier;
	FAutoVariable<int, CvPlayer> m_iExpInBorderModifier;
	FAutoVariable<int, CvPlayer> m_iLevelExperienceModifier;
	FAutoVariable<int, CvPlayer> m_iMinorQuestFriendshipMod;
	FAutoVariable<int, CvPlayer> m_iMinorGoldFriendshipMod;
	FAutoVariable<int, CvPlayer> m_iMinorFriendshipMinimum;
	FAutoVariable<int, CvPlayer> m_iMinorFriendshipDecayMod;
	FAutoVariable<int, CvPlayer> m_iMinorScienceAlliesCount;
	FAutoVariable<int, CvPlayer> m_iMinorResourceBonusCount;
	int m_iAbleToAnnexCityStatesCount;
	FAutoVariable<int, CvPlayer> m_iFreeSpecialist;
	FAutoVariable<int, CvPlayer> m_iCultureBombTimer;
	FAutoVariable<int, CvPlayer> m_iConversionTimer;
	FAutoVariable<int, CvPlayer> m_iCapitalCityID;
	FAutoVariable<int, CvPlayer> m_iCitiesLost;
	FAutoVariable<int, CvPlayer> m_iMilitaryMight;
	FAutoVariable<int, CvPlayer> m_iEconomicMight;
	FAutoVariable<int, CvPlayer> m_iTurnMightRecomputed;
	FAutoVariable<int, CvPlayer> m_iNewCityExtraPopulation;
	FAutoVariable<int, CvPlayer> m_iFreeFoodBox;
	FAutoVariable<int, CvPlayer> m_iScenarioScore1;
	FAutoVariable<int, CvPlayer> m_iScenarioScore2;
	FAutoVariable<int, CvPlayer> m_iScenarioScore3;
	FAutoVariable<int, CvPlayer> m_iScenarioScore4;
	FAutoVariable<int, CvPlayer> m_iScoreFromFutureTech;
	FAutoVariable<int, CvPlayer> m_iCombatExperience;
	int m_iLifetimeCombatExperience;
	int m_iNavalCombatExperience;
	int m_iBorderObstacleCount;
	FAutoVariable<int, CvPlayer> m_iPopRushHurryCount;
	FAutoVariable<int, CvPlayer> m_iTotalImprovementsBuilt;
	FAutoVariable<int, CvPlayer> m_iNextOperationID;
	FAutoVariable<int, CvPlayer> m_iCostNextPolicy;
	FAutoVariable<int, CvPlayer> m_iNumBuilders;
	FAutoVariable<int, CvPlayer> m_iMaxNumBuilders;
	FAutoVariable<int, CvPlayer> m_iCityStrengthMod;
	FAutoVariable<int, CvPlayer> m_iCityGrowthMod;
	FAutoVariable<int, CvPlayer> m_iCapitalGrowthMod;
	FAutoVariable<int, CvPlayer> m_iNumPlotsBought;
	FAutoVariable<int, CvPlayer> m_iPlotGoldCostMod;
	FAutoVariable<int, CvPlayer> m_iPlotCultureCostModifier;
	int m_iPlotCultureExponentModifier;
	int m_iNumCitiesPolicyCostDiscount;
	int m_iGarrisonedCityRangeStrikeModifier;
	int m_iGarrisonFreeMaintenanceCount;
#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
	FAutoVariable< std::vector< Firaxis::Array<int, NUM_SPECILIST_TYPES > >, CvPlayer> m_ppaaiBuildingScecialistCountChange;
#endif
	int m_iNumCitiesFreeCultureBuilding;
	int m_iNumCitiesFreeFoodBuilding;
#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
	int m_iNumCitiesFreeDevensiveBuilding;
#endif
	FAutoVariable<int, CvPlayer> m_iUnitPurchaseCostModifier;
	FAutoVariable<int, CvPlayer> m_iAllFeatureProduction;
	FAutoVariable<int, CvPlayer> m_iCityDistanceHighwaterMark; // this is used to determine camera zoom
	FAutoVariable<int, CvPlayer> m_iOriginalCapitalX;
	FAutoVariable<int, CvPlayer> m_iOriginalCapitalY;
	FAutoVariable<int, CvPlayer> m_iNumWonders;
	FAutoVariable<int, CvPlayer> m_iNumPolicies;
	FAutoVariable<int, CvPlayer> m_iNumGreatPeople;
	FAutoVariable<int, CvPlayer> m_iCityConnectionHappiness;
	FAutoVariable<int, CvPlayer> m_iHolyCityID;
	FAutoVariable<int, CvPlayer> m_iTurnsSinceSettledLastCity;
	FAutoVariable<int, CvPlayer> m_iNumNaturalWondersDiscoveredInArea;
	FAutoVariable<int, CvPlayer> m_iStrategicResourceMod;
	FAutoVariable<int, CvPlayer> m_iSpecialistCultureChange;
	FAutoVariable<int, CvPlayer> m_iGreatPeopleSpawnCounter;

	FAutoVariable<int, CvPlayer> m_iFreeTechCount;
	int m_iMedianTechPercentage;
	FAutoVariable<int, CvPlayer> m_iNumFreePolicies;
	FAutoVariable<int, CvPlayer> m_iNumFreePoliciesEver; 
	int m_iNumFreeTenets;
    int m_iMaxEffectiveCities;

	int m_iLastSliceMoved;

	FAutoVariable<uint, CvPlayer> m_uiStartTime;  // XXX save these?

	FAutoVariable<bool, CvPlayer> m_bHasBetrayedMinorCiv;
#ifdef CAN_BUILD_OU_AND_NIA_ONLY_ONCE
	FAutoVariable<bool, CvPlayer> m_bOxfordUniversityWasEverBuilt;
	FAutoVariable<bool, CvPlayer> m_bNationalIntelligenceAgencyWasEverBuilt;
#endif
	FAutoVariable<bool, CvPlayer> m_bAlive;
	FAutoVariable<bool, CvPlayer> m_bEverAlive;
	bool m_bBeingResurrected;
	FAutoVariable<bool, CvPlayer> m_bTurnActive;
	FAutoVariable<bool, CvPlayer> m_bAutoMoves;					// Signal that we can process the auto moves when ready.
	bool						  m_bProcessedAutoMoves;		// Signal that we have processed the auto moves
	FAutoVariable<bool, CvPlayer> m_bEndTurn;					// Signal that the player has completed their turn.  The turn will still be active until the auto-moves have been processed.
	bool						  m_bDynamicTurnsSimultMode;
	FAutoVariable<bool, CvPlayer> m_bPbemNewTurn;
	FAutoVariable<bool, CvPlayer> m_bExtendedGame;
	FAutoVariable<bool, CvPlayer> m_bFoundedFirstCity;
	int m_iNumCitiesFounded;
	FAutoVariable<bool, CvPlayer> m_bStrike;
	FAutoVariable<bool, CvPlayer> m_bCramped;
	FAutoVariable<bool, CvPlayer> m_bLostCapital;
	PlayerTypes m_eConqueror;
	FAutoVariable<bool, CvPlayer> m_bHasAdoptedStateReligion;
	FAutoVariable<bool, CvPlayer> m_bAlliesGreatPersonBiasApplied;

	FAutoVariable<std::vector<int>, CvPlayer> m_aiCityYieldChange;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiCoastalCityYieldChange;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiCapitalYieldChange;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiCapitalYieldPerPopChange;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiSeaPlotYield;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiYieldRateModifier;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiCapitalYieldRateModifier;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiExtraYieldThreshold;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiSpecialistExtraYield;
#ifdef POLICY_GOLDEN_AGE_YIELD_MOD
	FAutoVariable<std::vector<int>, CvPlayer> m_aiGoldenAgeYieldModifier;
#endif
#ifdef POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
	FAutoVariable<std::vector<int>, CvPlayer> m_paiPlotExtraYieldFromTradeRoute;
#endif
	FAutoVariable<std::vector<int>, CvPlayer> m_aiProximityToPlayer;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiResearchAgreementCounter;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiIncomingUnitTypes;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiIncomingUnitCountdowns;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiMinorFriendshipAnchors; // DEPRECATED
	std::vector<int> m_aiSiphonLuxuryCount;
	std::vector<int> m_aiGreatWorkYieldChange;

	typedef std::pair<uint, int> PlayerOptionEntry;
	typedef std::vector< PlayerOptionEntry > PlayerOptionsVector;
	FAutoVariable<PlayerOptionsVector, CvPlayer> m_aOptions;

	FAutoVariable<CvString, CvPlayer> m_strReligionKey;
	FAutoVariable<CvString, CvPlayer> m_strScriptData;

	CvString m_strEmbarkedGraphicOverride;

	FAutoVariable<std::vector<int>, CvPlayer> m_paiNumResourceUsed;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiNumResourceTotal;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourceGiftedToMinors;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourceExport;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourceImport;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourceFromMinors;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourcesSiphoned;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiImprovementCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiFreeBuildingCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiFreePromotionCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiUnitCombatProductionModifiers;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiUnitCombatFreeExperiences;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiUnitClassCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiUnitClassMaking;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiBuildingClassCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiBuildingClassMaking;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiProjectMaking;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiHurryCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiHurryModifier;

	FAutoVariable<std::vector<bool>, CvPlayer> m_pabLoyalMember;

	FAutoVariable<std::vector<bool>, CvPlayer> m_pabGetsScienceFromPlayer;

	FAutoVariable< std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > >, CvPlayer> m_ppaaiSpecialistExtraYield;
	FAutoVariable< std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > >, CvPlayer> m_ppaaiImprovementYieldChange;

	// Obsolete: only used to read old saves
	FAutoVariable< std::vector< Firaxis::Array< int, NUM_YIELD_TYPES > >, CvPlayer> m_ppaaiBuildingClassYieldMod;

	CvUnitCycler	m_UnitCycle;	

	// slewis's tutorial variables!
	FAutoVariable<bool, CvPlayer> m_bEverPoppedGoody;
	FAutoVariable<bool, CvPlayer> m_bEverTrainedBuilder;
	// end slewis's tutorial variables

	EndTurnBlockingTypes  m_eEndTurnBlockingType;
	int  m_iEndTurnBlockingNotificationIndex;

	CLinkList<TechTypes> m_researchQueue;

	CLinkList<CvString> m_cityNames;

	FFreeListTrashArray<CvCityAI> m_cities;

	FFreeListTrashArray<CvUnit> m_units;

	FFreeListTrashArray<CvArmyAI> m_armyAIs;

	std::map<int, CvAIOperation*> m_AIOperations;
	std::map<int, CvAIOperation*>::iterator m_CurrentOperation;

	std::vector< std::pair<int, PlayerVoteTypes> > m_aVote;
	std::vector< std::pair<UnitClassTypes, int> > m_aUnitExtraCosts;

	std::vector<CvString> m_ReplayDataSets;
	std::vector< TurnData > m_ReplayDataSetValues;

	void doResearch();
	void doWarnings();

	// Danger plots!
	CvDangerPlots* m_pDangerPlots;

	// Policies
	CvPlayerPolicies* m_pPlayerPolicies;
	void processPolicies(PolicyTypes ePolicy, int iChange);

	// AI Strategies
	CvEconomicAI* m_pEconomicAI;
	CvMilitaryAI* m_pMilitaryAI;
	CvCitySpecializationAI* m_pCitySpecializationAI;
	CvWonderProductionAI* m_pWonderProductionAI;

	// AI Grand Strategies
	CvGrandStrategyAI* m_pGrandStrategyAI;

	// Diplomacy AI
	CvDiplomacyAI* m_pDiplomacyAI;

	// Religion AI
	CvPlayerReligions* m_pReligions;
	CvReligionAI* m_pReligionAI;

	// AI Tactics
	CvTacticalAI* m_pTacticalAI;
	CvHomelandAI* m_pHomelandAI;

	// Techs
	CvPlayerTechs* m_pPlayerTechs;

	// Flavor Manager
	CvFlavorManager* m_pFlavorManager;

	// Minor Civ AI
	CvMinorCivAI* m_pMinorCivAI;

	// Deal AI
	CvDealAI* m_pDealAI;

	// Builder Tasking AI
	CvBuilderTaskingAI* m_pBuilderTaskingAI;

	// City Connections
	CvCityConnections* m_pCityConnections;

	// Espionage
	CvPlayerEspionage* m_pEspionage;
	CvEspionageAI* m_pEspionageAI;

	// Trade
	CvPlayerTrade* m_pTrade;
	CvTradeAI* m_pTradeAI;

	// League AI
	CvLeagueAI* m_pLeagueAI;

	// Culture
	CvPlayerCulture* m_pCulture;

	CvNotifications* m_pNotifications;
	CvDiplomacyRequests* m_pDiplomacyRequests;

	CvPlotsVector m_aiPlots;

	// Treasury
	CvTreasury* m_pTreasury;

	CvPlayerTraits* m_pTraits;

	// human player wanted to end turn processing but hasn't received
	// the net turn complete message
	bool m_activeWaitingForEndTurnMessage;
	int  m_endTurnBusyUnitUpdatesLeft;

	int m_lastGameTurnInitialAIProcessed;

	ConqueredByBoolField m_bfEverConqueredBy;

	int m_iNumFreeGreatPeople;
	int m_iNumMayaBoosts;
	int m_iNumFaithGreatPeople;
    int m_iNumArchaeologyChoices;

	FaithPurchaseTypes m_eFaithPurchaseType;
	int m_iFaithPurchaseIndex;

	void doUpdateCacheOnTurn();

	void doArmySize();

	friend class CvPlayerManager;
	friend CvUnit* GetPlayerUnit(IDInfo& unit);
	friend const CvUnit* GetPlayerUnit(const IDInfo& unit);

	CvPlayerAchievements m_kPlayerAchievements;

#ifdef CS_ALLYING_WAR_RESCTRICTION
	FAutoVariable <std::vector< Firaxis::Array< int, MAX_MINOR_CIVS > >, CvPlayer> m_ppaaiTurnCSWarAllowing;
	FAutoVariable <std::vector< Firaxis::Array< float, MAX_MINOR_CIVS > >, CvPlayer> m_ppaafTimeCSWarAllowing;
#endif
#ifdef PENALTY_FOR_DELAYING_POLICIES
	bool m_bIsDelayedPolicy;
#endif
#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
	FAutoVariable <std::vector< Firaxis::Array< int, NUM_YIELD_TYPES > >, CvPlayer> m_ppaaiYieldForEachBuildingInEmpire;
#endif
#ifdef POLICY_ALLOWS_GP_BUYS_FOR_GOLD
	int m_iNumGoldPurchasedGreatPerson;
	bool m_bGoldWriter;
	bool m_bGoldArtist;
	bool m_bGoldMusician;
	bool m_bGoldScientist;
	bool m_bGoldEngineer;
	bool m_bGoldMerchant;
	bool m_bGoldGeneral;
	bool m_bGoldAdmiral;
#endif
};

extern bool CancelActivePlayerEndTurn();

namespace FSerialization
{
void SyncPlayer();
void ClearPlayerDeltas();
}

#endif
