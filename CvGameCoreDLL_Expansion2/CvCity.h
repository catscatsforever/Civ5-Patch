/*	-------------------------------------------------------------------------------------------------------
	� 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

// city.h

#ifndef CIV5_CITY_H
#define CIV5_CITY_H

#include "LinkedList.h"
#include "CvUnit.h"
#include "CvAIOperation.h"
#include "CvEnumSerialization.h"
#include "FStlContainerSerialization.h"
#include "FAutoVariable.h"
#include "FAutoVector.h"

#include "CvPreGame.h"

// 0 = center of city, 1-6 = the edge of city on points, 7-12 = one tile out
#define NUM_CITY_BUILDING_DISPLAY_SLOTS (13)

class CvPlot;
class CvArea;
class CvGenericBuilding;
class CvCityBuildings;
class CvCityStrategyAI;
class CvCityCitizens;
class CvCityEmphases;
class CvCityReligions;
class CvCityEspionage;
class CvCityCulture;
class CvPlayer;

class CvCity
{

public:
	CvCity();
	virtual ~CvCity();

	void init(int iID, PlayerTypes eOwner, int iX, int iY, bool bBumpUnits = true, bool bInitialFounding = true);
	void uninit();
	void reset(int iID = 0, PlayerTypes eOwner = NO_PLAYER, int iX = 0, int iY = 0, bool bConstructorCall = false);
	void setupGraphical();
	void setupWonderGraphics();
	void setupBuildingGraphics();
	void setupSpaceshipGraphics();


	void kill();
	void PreKill();
	void PostKill(bool bCapital, CvPlot* pPlot, PlayerTypes eOwner);

	CvPlayer* GetPlayer();

	void doTurn();

	bool isCitySelected();
	bool canBeSelected() const;
	void updateSelectedCity();

	void updateYield();

	bool IsIndustrialRouteToCapital() const;
	void SetIndustrialRouteToCapital(bool bValue);
	void DoUpdateIndustrialRouteToCapital();

	void SetRouteToCapitalConnected(bool bValue);
	bool IsRouteToCapitalConnected(void);

	void createGreatGeneral(UnitTypes eGreatPersonUnit);
	void createGreatAdmiral(UnitTypes eGreatPersonUnit);

	CityTaskResult doTask(TaskTypes eTask, int iData1 = -1, int iData2 = -1, bool bOption = false, bool bAlt = false, bool bShift = false, bool bCtrl = false);

	void chooseProduction(UnitTypes eTrainUnit = NO_UNIT, BuildingTypes eConstructBuilding = NO_BUILDING, ProjectTypes eCreateProject = NO_PROJECT, bool bFinish = false, bool bFront = false);

	void clearWorkingOverride(int iIndex);
	int countNumImprovedPlots(ImprovementTypes eImprovement = NO_IMPROVEMENT, bool bPotential = false) const;
	int countNumWaterPlots() const;
	int countNumRiverPlots() const;
	int countNumForestPlots() const;

	int findPopulationRank();
	int findBaseYieldRateRank(YieldTypes eYield);
	int findYieldRateRank(YieldTypes eYield);

	UnitTypes allUpgradesAvailable(UnitTypes eUnit, int iUpgradeCount = 0) const;
	bool isWorldWondersMaxed() const;
	bool isTeamWondersMaxed() const;
	bool isNationalWondersMaxed() const;
	bool isBuildingsMaxed() const;

	bool canTrain(UnitTypes eUnit, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, bool bWillPurchase = false, CvString* toolTipSink = NULL) const;
	bool canTrain(UnitCombatTypes eUnitCombat) const;
	bool canConstruct(BuildingTypes eBuilding, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, CvString* toolTipSink = NULL) const;
	bool canCreate(ProjectTypes eProject, bool bContinue = false, bool bTestVisible = false) const;
	bool canPrepare(SpecialistTypes eSpecialist, bool bContinue = false) const;
	bool canMaintain(ProcessTypes eProcess, bool bContinue = false) const;
	bool canJoin() const;

	bool IsFeatureSurrounded() const;
	void SetFeatureSurrounded(bool bValue);
	void DoUpdateFeatureSurrounded();

	int GetResourceExtraYield(ResourceTypes eResource, YieldTypes eYield) const;
	void ChangeResourceExtraYield(ResourceTypes eResource, YieldTypes eYield, int iChange);

	int GetFeatureExtraYield(FeatureTypes eFeature, YieldTypes eYield) const;
	void ChangeFeatureExtraYield(FeatureTypes eFeature, YieldTypes eYield, int iChange);

	int GetTerrainExtraYield(TerrainTypes eTerrain, YieldTypes eYield) const;
	void ChangeTerrainExtraYield(TerrainTypes eTerrain, YieldTypes eYield, int iChange);

#ifdef BUILDING_IMPROVEMENT_YIELD_CHANGE
	int GetImprovementExtraYield(ImprovementTypes eImprovement, YieldTypes eYield) const;
	void ChangeImprovementExtraYield(ImprovementTypes eImprovement, YieldTypes eYield, int iChange);
#endif

	bool IsHasResourceLocal(ResourceTypes eResource, bool bTestVisible) const;
	void ChangeNumResourceLocal(ResourceTypes eResource, int iChange);

	bool IsBuildingLocalResourceValid(BuildingTypes eBuilding, bool bTestVisible, CvString* toolTipSink = NULL) const;

	// Resource Demanded

	ResourceTypes GetResourceDemanded(bool bHideUnknown = true) const;
	void SetResourceDemanded(ResourceTypes eResource);
	void DoPickResourceDemanded(bool bCurrentResourceInvalid = true);
	void DoTestResourceDemanded();

	void DoSeedResourceDemandedCountdown();
	int GetResourceDemandedCountdown() const;
	void SetResourceDemandedCountdown(int iValue);
	void ChangeResourceDemandedCountdown(int iChange);

	// End Resource Demanded

#ifdef NEW_FACTORIES
	bool isCityHasCoal() const;
#endif

	int getFoodTurnsLeft() const;
	bool isProduction() const;
	bool isProductionLimited() const;
	bool isProductionUnit() const;
	bool isProductionBuilding() const;
	bool isProductionProject() const;
	bool isProductionSpecialist() const;
	bool isProductionProcess() const;

	bool canContinueProduction(OrderData order);
	int getProductionExperience(UnitTypes eUnit = NO_UNIT);
	void addProductionExperience(CvUnit* pUnit, bool bConscript = false);

	UnitTypes getProductionUnit() const;
	UnitAITypes getProductionUnitAI() const;
	BuildingTypes getProductionBuilding() const;
	ProjectTypes getProductionProject() const;
	SpecialistTypes getProductionSpecialist() const;
	ProcessTypes getProductionProcess() const;
	const char* getProductionName() const;
	const char* getProductionNameKey() const;
	int getGeneralProductionTurnsLeft() const;

	bool isFoodProduction() const;
	bool isFoodProduction(UnitTypes eUnit) const;
	int getFirstUnitOrder(UnitTypes eUnit) const;
	int getFirstBuildingOrder(BuildingTypes eBuilding) const;
	int getFirstProjectOrder(ProjectTypes eProject) const;
	int getFirstSpecialistOrder(SpecialistTypes eSpecialist) const;
	int getNumTrainUnitAI(UnitAITypes eUnitAI) const;

	int getProduction() const;
	int getProductionTimes100() const;
	int getProductionNeeded() const;
	int getProductionNeeded(UnitTypes eUnit) const;
	int getProductionNeeded(BuildingTypes eBuilding) const;
	int getProductionNeeded(ProjectTypes eProject) const;
	int getProductionNeeded(SpecialistTypes eSpecialist) const;
	int getProductionTurnsLeft() const;
	int getProductionTurnsLeft(UnitTypes eUnit, int iNum) const;
	int getProductionTurnsLeft(BuildingTypes eBuilding, int iNum) const;
	int getProductionTurnsLeft(ProjectTypes eProject, int iNum) const;
	int getProductionTurnsLeft(SpecialistTypes eSpecialist, int iNum) const;
	int GetPurchaseCost(UnitTypes eUnit);
	int GetFaithPurchaseCost(UnitTypes eUnit, bool bIncludeBeliefDiscounts);
	int GetPurchaseCost(BuildingTypes eBuilding);
	int GetFaithPurchaseCost(BuildingTypes eBuilding);
	int GetPurchaseCost(ProjectTypes eProject);
#ifdef BUILDING_HURRY_COST_MODIFIER
	int GetPurchaseCostFromProduction(int iProduction, bool bIsBuilding = false);
#else
	int GetPurchaseCostFromProduction(int iProduction);
#endif

	int getProductionTurnsLeft(int iProductionNeeded, int iProduction, int iFirstProductionDifference, int iProductionDifference) const;
	void setProduction(int iNewValue);
	void changeProduction(int iChange);
	void setProductionTimes100(int iNewValue);
	void changeProductionTimes100(int iChange);

	int getProductionModifier(_In_opt_ CvString* toolTipSink = NULL) const;
	int getGeneralProductionModifiers(_In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(UnitTypes eUnit, _In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(BuildingTypes eBuilding, _In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(ProjectTypes eProject, _In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(SpecialistTypes eSpecialist, _In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(ProcessTypes eProcess, _In_opt_ CvString* toolTipSink = NULL) const;

	int getOverflowProductionDifference(int iProductionNeeded, int iProduction, int iProductionModifier, int iDiff, int iModifiedProduction) const;
	int getProductionDifference(int iProductionNeeded, int iProduction, int iProductionModifier, bool bFoodProduction, bool bOverflow) const;
	int getCurrentProductionDifference(bool bIgnoreFood, bool bOverflow) const;
	int getRawProductionDifference(bool bIgnoreFood, bool bOverflow) const;
	int getProductionDifferenceTimes100(int iProductionNeeded, int iProduction, int iProductionModifier, bool bFoodProduction, bool bOverflow) const;
	int getCurrentProductionDifferenceTimes100(bool bIgnoreFood, bool bOverflow) const;
	int getRawProductionDifferenceTimes100(bool bIgnoreFood, bool bOverflow) const;
	int getExtraProductionDifference(int iExtra) const;
	int GetFoodProduction(int iExcessFood) const;

	bool canHurry(HurryTypes eHurry, bool bTestVisible = false) const;
	void hurry(HurryTypes eHurry);

	UnitTypes getConscriptUnit() const;
	CvUnit* initConscriptedUnit();
	int getConscriptPopulation() const;
	int conscriptMinCityPopulation() const;
	bool canConscript() const;
	void conscript();

	int getResourceYieldRateModifier(YieldTypes eIndex, ResourceTypes eResource) const;

	void processResource(ResourceTypes eResource, int iChange);
	void processBuilding(BuildingTypes eBuilding, int iChange, bool bFirst, bool bObsolete = false, bool bApplyingAllCitiesBonus = false);
	void processProcess(ProcessTypes eProcess, int iChange);
	void processSpecialist(SpecialistTypes eSpecialist, int iChange);

	void UpdateReligion(ReligionTypes eNewMajority);
#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
	ReligionTypes getFoundedReligion() const;
	void setFoundedReligion(ReligionTypes eReligion);
#endif

	int GetCultureFromSpecialist(SpecialistTypes eSpecialist) const;

	CvHandicapInfo& getHandicapInfo() const;
	HandicapTypes getHandicapType() const;

	CvCivilizationInfo& getCivilizationInfo() const;
	CivilizationTypes getCivilizationType() const;

	LeaderHeadTypes getPersonalityType() const;

	ArtStyleTypes getArtStyleType() const;

	CitySizeTypes getCitySizeType() const;

	bool isBarbarian() const;
	bool isHuman() const;
	bool isVisible(TeamTypes eTeam, bool bDebug) const;

	bool isCapital() const;
	bool IsOriginalCapital() const;
	bool IsOriginalMajorCapital() const; // is the original capital of a major civ
	bool IsEverCapital() const;
	void SetEverCapital(bool bValue);

	bool isCoastal(int iMinWaterSize = -1) const;

	int foodConsumption(bool bNoAngry = false, int iExtra = 0) const;
	int foodDifference(bool bBottom = true) const;
	int foodDifferenceTimes100(bool bBottom = true, CvString* toolTipSink = NULL) const;
#ifdef EG_REPLAYDATASET_PERCENTOFCITIESWITHACTIVEWLTKD
	bool isCityActiveWLTKD() const;
#endif
	int growthThreshold() const;

	int productionLeft() const;
	int hurryCost(HurryTypes eHurry, bool bExtra) const;
	int getHurryCostModifier(HurryTypes eHurry, bool bIgnoreNew = false) const;
	int hurryGold(HurryTypes eHurry) const;
	int hurryPopulation(HurryTypes eHurry) const;
	int hurryProduction(HurryTypes eHurry) const;
	int maxHurryPopulation() const;

	bool hasActiveWorldWonder() const;

	inline int GetID() const
	{
		return m_iID;
	}

	int getIndex() const;
	IDInfo GetIDInfo() const;
	void SetID(int iID);

	inline int getX() const
	{
		return m_iX;
	}

	inline int getY() const
	{
		return m_iY;
	}

	bool at(int iX, int iY) const;
	bool at(CvPlot* pPlot) const;
	CvPlot* plot() const;
	int getArea() const;
	CvArea* area() const;
	CvArea* waterArea() const;

	CvUnit* GetGarrisonedUnit() const;

	CvPlot* getRallyPlot() const;
	void setRallyPlot(CvPlot* pPlot);

	int getGameTurnFounded() const;
	void setGameTurnFounded(int iNewValue);

	int getGameTurnAcquired() const;
	void setGameTurnAcquired(int iNewValue);

	int getGameTurnLastExpanded() const;
	void setGameTurnLastExpanded(int iNewValue);

	int getPopulation() const;
	void setPopulation(int iNewValue, bool bReassignPop = true);
	void changePopulation(int iChange, bool bReassignPop = true);

	long getRealPopulation() const;

	int getHighestPopulation() const;
	void setHighestPopulation(int iNewValue);

	int getNumGreatPeople() const;
	void changeNumGreatPeople(int iChange);

	int getBaseGreatPeopleRate() const;
	int getGreatPeopleRate() const;
	int getTotalGreatPeopleRateModifier() const;
	void changeBaseGreatPeopleRate(int iChange);

	int getGreatPeopleRateModifier() const;
	void changeGreatPeopleRateModifier(int iChange);

#ifdef BUILDING_CITY_RANGE_MODIFIER
	int getCityAttackRangeModifier() const;
	void changeCityAttackRangeModifier(int iChange);
#endif
#ifdef BUILDING_CITY_EXTRA_ATTACK
	int getCityExtraAttack() const;
	void changeCityExtraAttack(int iChange);
	int getCityCurrentExtraAttack() const;
	void changeCityCurrentExtraAttack(int iChange);
#endif
#ifdef BUILDING_CITY_EXTRA_HEAL
	int getCityExtraHeal() const;
	void changeCityExtraHeal(int iChange);
#endif

	// Culture stuff

	int GetJONSCultureStored() const;
	void SetJONSCultureStored(int iValue);
	void ChangeJONSCultureStored(int iChange);

	int GetJONSCultureLevel() const;
	void SetJONSCultureLevel(int iValue);
	void ChangeJONSCultureLevel(int iChange);
	void DoJONSCultureLevelIncrease();
	int GetJONSCultureThreshold() const;

	int getJONSCulturePerTurn() const;

	int GetBaseJONSCulturePerTurn() const;

	int GetJONSCulturePerTurnFromBuildings() const;
	void ChangeJONSCulturePerTurnFromBuildings(int iChange);

	int GetJONSCulturePerTurnFromPolicies() const;
	void ChangeJONSCulturePerTurnFromPolicies(int iChange);

	int GetJONSCulturePerTurnFromSpecialists() const;
	void ChangeJONSCulturePerTurnFromSpecialists(int iChange);

	int GetJONSCulturePerTurnFromGreatWorks() const;

	int GetJONSCulturePerTurnFromTraits() const;

	int GetJONSCulturePerTurnFromReligion() const;
	void ChangeJONSCulturePerTurnFromReligion(int iChange);

	int GetJONSCulturePerTurnFromLeagues() const;

	int getCultureRateModifier() const;
	void changeCultureRateModifier(int iChange);

	// END Culture
#ifdef BELIEF_HALF_FAITH_IN_CITY
	int GetFaithMod() const;
#endif
	int GetFaithPerTurn() const;
	int GetFaithPerTurnFromBuildings() const;
	void ChangeFaithPerTurnFromBuildings(int iChange);

	int GetFaithPerTurnFromPolicies() const;
	void ChangeFaithPerTurnFromPolicies(int iChange);

	int GetFaithPerTurnFromTraits() const;

	int GetFaithPerTurnFromReligion() const;
	void ChangeFaithPerTurnFromReligion(int iChange);

#ifdef BELIEF_GREAT_WORK_YIELD_CHANGES
	int GetFaithPerTurnFromGreatWorks() const;
#endif

	int getNumWorldWonders() const;
	void changeNumWorldWonders(int iChange);

	int getNumTeamWonders() const;
	void changeNumTeamWonders(int iChange);

	int getNumNationalWonders() const;
	void changeNumNationalWonders(int iChange);

	int GetWonderProductionModifier() const;
	void ChangeWonderProductionModifier(int iChange);

	int GetLocalResourceWonderProductionMod(BuildingTypes eBuilding, CvString* toolTipSink = NULL) const;

	int getCapturePlunderModifier() const;
	void changeCapturePlunderModifier(int iChange);

	int getPlotCultureCostModifier() const;
	void changePlotCultureCostModifier(int iChange);

	int getPlotBuyCostModifier() const;
	void changePlotBuyCostModifier(int iChange);

	int getHealRate() const;
	void changeHealRate(int iChange);

	int GetEspionageModifier() const;
	void ChangeEspionageModifier(int iChange);

	bool IsNoOccupiedUnhappiness() const;
	int GetNoOccupiedUnhappinessCount() const;
	void ChangeNoOccupiedUnhappinessCount(int iChange);

	int getFood() const;
	int getFoodTimes100() const;
	void setFood(int iNewValue);
	void setFoodTimes100(int iNewValue);
	void changeFood(int iChange);
	void changeFoodTimes100(int iChange);

	int getFoodKept() const;
	void setFoodKept(int iNewValue);
	void changeFoodKept(int iChange);

	int getMaxFoodKeptPercent() const;
	void changeMaxFoodKeptPercent(int iChange);

	int getOverflowProduction() const;
	void setOverflowProduction(int iNewValue);
	void changeOverflowProduction(int iChange);
	int getOverflowProductionTimes100() const;
	void setOverflowProductionTimes100(int iNewValue);
	void changeOverflowProductionTimes100(int iChange);

	int getFeatureProduction()const;
	void setFeatureProduction(int iNewValue);
	void changeFeatureProduction(int iChange);

	int getMilitaryProductionModifier() const;
	void changeMilitaryProductionModifier(int iChange);

	int getSpaceProductionModifier() const;
	void changeSpaceProductionModifier(int iChange);

	int getFreeExperience() const;
	void changeFreeExperience(int iChange);

	bool CanAirlift() const;

	int GetMaxAirUnits() const;
	void ChangeMaxAirUnits(int iChange);

	int getNukeModifier() const;
	void changeNukeModifier(int iChange);

	int GetTradeRouteTargetBonus() const;
	void ChangeTradeRouteTargetBonus(int iChange);

	int GetTradeRouteRecipientBonus() const;
	void ChangeTradeRouteRecipientBonus(int iChange);

	bool isAreaBorderObstacle() const;

	bool IsResistance() const;
	int GetResistanceTurns() const;
	void ChangeResistanceTurns(int iChange);
	void DoResistanceTurn();

	bool IsRazing() const;
	int GetRazingTurns() const;
	void ChangeRazingTurns(int iChange);
	bool DoRazingTurn();

	bool IsOccupied() const;
	void SetOccupied(bool bValue);

	bool IsPuppet() const;
	void SetPuppet(bool bValue);
	void DoCreatePuppet();
	void DoAnnex();

	int GetLocalHappiness() const;
	int GetHappinessFromBuildings() const;
	int GetBaseHappinessFromBuildings() const;
	void ChangeBaseHappinessFromBuildings(int iChange);
	int GetUnmoddedHappinessFromBuildings() const;
	void ChangeUnmoddedHappinessFromBuildings(int iChange);

	bool IsIgnoreCityForHappiness() const;
	void SetIgnoreCityForHappiness(bool bValue);

	BuildingTypes ChooseFreeCultureBuilding() const;
	BuildingTypes ChooseFreeFoodBuilding() const;

	int getCitySizeBoost() const;
	void setCitySizeBoost(int iBoost);

	bool isNeverLost() const;
	void setNeverLost(bool bNewValue);

	bool isDrafted() const;
	void setDrafted(bool bNewValue);

	bool IsOwedCultureBuilding() const;
	void SetOwedCultureBuilding(bool bNewValue);

#ifdef OWED_FOOD_BUILDING
	bool IsOwedFoodBuilding() const;
	void SetOwedFoodBuilding(bool bNewValue);
#endif

	bool IsBlockaded() const;

	int GetWeLoveTheKingDayCounter() const;
	void SetWeLoveTheKingDayCounter(int iValue);
	void ChangeWeLoveTheKingDayCounter(int iChange);

	int GetLastTurnGarrisonAssigned() const;
	void SetLastTurnGarrisonAssigned(int iValue);

	int GetNumThingsProduced() const;

	bool isProductionAutomated() const;
	void setProductionAutomated(bool bNewValue, bool bClear);

	bool isLayoutDirty() const;
	void setLayoutDirty(bool bNewValue);

	inline PlayerTypes getOwner() const
	{
		return m_eOwner;
	}

	TeamTypes getTeam() const;

	PlayerTypes getPreviousOwner() const;
	void setPreviousOwner(PlayerTypes eNewValue);

	PlayerTypes getOriginalOwner() const;
	void setOriginalOwner(PlayerTypes eNewValue);

	PlayerTypes GetPlayersReligion() const;
	void SetPlayersReligion(PlayerTypes eNewValue);

	// Yield

	int getSeaPlotYield(YieldTypes eIndex) const;
	void changeSeaPlotYield(YieldTypes eIndex, int iChange);

	int getRiverPlotYield(YieldTypes eIndex) const;
	void changeRiverPlotYield(YieldTypes eIndex, int iChange);

#ifdef BUILDING_NEAR_MOUNTAIN_YIELD_CHANGES
	int getNearMountainYield(YieldTypes eIndex) const;
	void changeNearMountainYield(YieldTypes eIndex, int iChange);
#endif

	int getLakePlotYield(YieldTypes eIndex) const;
	void changeLakePlotYield(YieldTypes eIndex, int iChange);

	int getSeaResourceYield(YieldTypes eIndex) const;
	void changeSeaResourceYield(YieldTypes eIndex, int iChange);

	int getBaseYieldRateModifier(YieldTypes eIndex, int iExtra = 0, CvString* toolTipSink = NULL) const;
	int getYieldRate(YieldTypes eIndex, bool bIgnoreTrade) const;
	int getYieldRateTimes100(YieldTypes eIndex, bool bIgnoreTrade) const;

	// Base Yield
	int getBaseYieldRate(YieldTypes eIndex) const;

	int GetBaseYieldRateFromTerrain(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromTerrain(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromBuildings(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromBuildings(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromSpecialists(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromSpecialists(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromMisc(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromMisc(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromReligion(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromReligion(YieldTypes eIndex, int iChange);
	// END Base Yield

	int GetYieldPerPopTimes100(YieldTypes eIndex) const;
	void ChangeYieldPerPopTimes100(YieldTypes eIndex, int iChange);

	int GetYieldPerReligionTimes100(YieldTypes eIndex) const;
	void ChangeYieldPerReligionTimes100(YieldTypes eIndex, int iChange);

	int getYieldRateModifier(YieldTypes eIndex) const;
	void changeYieldRateModifier(YieldTypes eIndex, int iChange);

	int getPowerYieldRateModifier(YieldTypes eIndex) const;
	void changePowerYieldRateModifier(YieldTypes eIndex, int iChange);

	int getResourceYieldRateModifier(YieldTypes eIndex) const;
	void changeResourceYieldRateModifier(YieldTypes eIndex, int iChange);

	int getHappinessModifier(YieldTypes eIndex) const;

	int getExtraSpecialistYield(YieldTypes eIndex) const;
	int getExtraSpecialistYield(YieldTypes eIndex, SpecialistTypes eSpecialist) const;
	void updateExtraSpecialistYield(YieldTypes eYield);
	void updateExtraSpecialistYield();

	int getProductionToYieldModifier(YieldTypes eIndex) const;
	void changeProductionToYieldModifier(YieldTypes eIndex, int iChange);
#ifdef FIX_EXCHANGE_PRODUCTION_OVERFLOW_INTO_GOLD_OR_SCIENCE
	int getProcessOverflowProductionTimes100() const;
	void setProcessOverflowProductionTimes100(int iValue);
#endif

	int GetTradeYieldModifier(YieldTypes eIndex, CvString* toolTipSink = NULL) const;

	// END Yield

	int getDomainFreeExperience(DomainTypes eIndex) const;
	void changeDomainFreeExperience(DomainTypes eIndex, int iChange);

	int getDomainFreeExperienceFromGreatWorks(DomainTypes eIndex) const;
	
	int getDomainProductionModifier(DomainTypes eIndex) const;
	void changeDomainProductionModifier(DomainTypes eIndex, int iChange);

	bool isEverOwned(PlayerTypes eIndex) const;
	void setEverOwned(PlayerTypes eIndex, bool bNewValue);

	bool isRevealed(TeamTypes eIndex, bool bDebug) const;
	bool setRevealed(TeamTypes eIndex, bool bNewValue);

	const CvString getName() const;
	const char* getNameKey() const;
	void setName(const char* szNewValue, bool bFound = false);
	void doFoundMessage();

	bool IsExtraLuxuryResources();
	void SetExtraLuxuryResources(int iNewValue);
	void ChangeExtraLuxuryResources(int iChange);

	CvCityBuildings* GetCityBuildings() const;

	int getProjectProduction(ProjectTypes eIndex) const;
	void setProjectProduction(ProjectTypes eIndex, int iNewValue);
	void changeProjectProduction(ProjectTypes eIndex, int iChange);
	int getProjectProductionTimes100(ProjectTypes eIndex) const;
	void setProjectProductionTimes100(ProjectTypes eIndex, int iNewValue);
	void changeProjectProductionTimes100(ProjectTypes eIndex, int iChange);

	int getSpecialistProduction(SpecialistTypes eIndex) const;
	void setSpecialistProduction(SpecialistTypes eIndex, int iNewValue);
	void changeSpecialistProduction(SpecialistTypes eIndex, int iChange);
	int getSpecialistProductionTimes100(SpecialistTypes eIndex) const;
	void setSpecialistProductionTimes100(SpecialistTypes eIndex, int iNewValue);
	void changeSpecialistProductionTimes100(SpecialistTypes eIndex, int iChange);

	int getUnitProduction(UnitTypes eIndex) const;
	void setUnitProduction(UnitTypes eIndex, int iNewValue);
	void changeUnitProduction(UnitTypes eIndex, int iChange);
	int getUnitProductionTimes100(UnitTypes eIndex) const;
	void setUnitProductionTimes100(UnitTypes eIndex, int iNewValue);
	void changeUnitProductionTimes100(UnitTypes eIndex, int iChange);

	int getUnitProductionTime(UnitTypes eIndex) const;
	void setUnitProductionTime(UnitTypes eIndex, int iNewValue);
	void changeUnitProductionTime(UnitTypes eIndex, int iChange);

	int getUnitCombatFreeExperience(UnitCombatTypes eIndex) const;
	void changeUnitCombatFreeExperience(UnitCombatTypes eIndex, int iChange);

	int getUnitCombatProductionModifier(UnitCombatTypes eIndex) const;
	void changeUnitCombatProductionModifier(UnitCombatTypes eIndex, int iChange);

	int getFreePromotionCount(PromotionTypes eIndex) const;
	bool isFreePromotion(PromotionTypes eIndex) const;
	void changeFreePromotionCount(PromotionTypes eIndex, int iChange);

	int getSpecialistFreeExperience() const;
	void changeSpecialistFreeExperience(int iChange);

	void updateStrengthValue();
	int getStrengthValue(bool bForRangeStrike = false) const;
	int GetPower() const;

	int getDamage() const;
	void setDamage(int iValue, bool noMessage=false);
	void changeDamage(int iChange);

	bool isMadeAttack() const;
	void setMadeAttack(bool bNewValue);

	bool canRangeStrike() const;
	bool CanRangeStrikeNow() const;
	bool IsHasBuildingThatAllowsRangeStrike() const;

	bool canRangeStrikeAt(int iX, int iY) const;
	CityTaskResult rangeStrike(int iX, int iY);
	CvUnit* rangedStrikeTarget(CvPlot* pPlot);
	bool canRangedStrikeTarget(const CvPlot& targetPlot) const;

	int rangeCombatUnitDefense(_In_ const CvUnit* pDefender) const;
	int rangeCombatDamage(const CvUnit* pDefender, CvCity* pCity = NULL, bool bIncludeRand = true) const;

	int GetAirStrikeDefenseDamage(const CvUnit* pAttacker, bool bIncludeRand = true) const;

	void DoNearbyEnemy();

	void IncrementUnitStatCount(CvUnit* pUnit);
	void CheckForAchievementBuilding(BuildingTypes eBuilding);
	bool AreAllUnitsBuilt();

	// Plot acquisition

	bool CanBuyPlot(int iPlotX = -1, int iPlotY = -1, bool bIgnoreCost = false);
	bool CanBuyAnyPlot(void);
	CvPlot* GetNextBuyablePlot();
	void GetBuyablePlotList(std::vector<int>& aiPlotList);
	int GetBuyPlotCost(int iPlotX, int iPlotY) const;
	void BuyPlot(int iPlotX, int iPlotY);
	void DoAcquirePlot(int iPlotX, int iPlotY);
	int GetBuyPlotScore(int& iBestX, int& iBestY);
	int GetIndividualPlotScore(const CvPlot* pPlot) const;

	int GetCheapestPlotInfluence() const;
	void SetCheapestPlotInfluence(int iValue);
	void DoUpdateCheapestPlotInfluence();

	// End plot acquisition

	bool isValidBuildingLocation(BuildingTypes eIndex) const;

	void SetThreatValue(int iThreatValue);
	int getThreatValue(void);

	void clearOrderQueue();
	void pushOrder(OrderTypes eOrder, int iData1, int iData2, bool bSave, bool bPop, bool bAppend, bool bRush=false);
	void popOrder(int iNum, bool bFinish = false, bool bChoose = false);
	void swapOrder(int iNum);
	void startHeadOrder();
	void stopHeadOrder();
	int getOrderQueueLength();
	OrderData* getOrderFromQueue(int iIndex);
	const OrderData* nextOrderQueueNode(const OrderData* pNode) const;
	OrderData* nextOrderQueueNode(OrderData* pNode);
	const OrderData* headOrderQueueNode() const;
	OrderData* headOrderQueueNode();
	const OrderData* tailOrderQueueNode() const;
	bool CleanUpQueue(void);  // remove items in the queue that are no longer valid

	int CreateUnit(UnitTypes eUnitType, UnitAITypes eAIType = NO_UNITAI, bool bUseToSatisfyOperation=true);
	bool CreateBuilding(BuildingTypes eBuildType);
	bool CreateProject(ProjectTypes eProjectType);

	bool CanPlaceUnitHere(UnitTypes eUnitType);
	bool IsCanPurchase(bool bTestPurchaseCost, bool bTestTrainable, UnitTypes eUnitType, BuildingTypes eBuildingType, ProjectTypes eProjectType, YieldTypes ePurchaseYield);
	void Purchase(UnitTypes eUnitType, BuildingTypes eBuildingType, ProjectTypes eProjectType, YieldTypes ePurchaseYield);

	PlayerTypes getLiberationPlayer() const;
	void liberate();

	CvCityStrategyAI* GetCityStrategyAI() const;
	CvCityCitizens* GetCityCitizens() const;
	CvCityReligions* GetCityReligions() const;
	CvCityEmphases* GetCityEmphases() const;
	CvCityEspionage* GetCityEspionage() const;
	CvCityCulture* GetCityCulture() const;

	void read(FDataStream& kStream);
	void write(FDataStream& kStream) const;

	virtual void AI_init() = 0;
	virtual void AI_reset() = 0;
	virtual void AI_doTurn() = 0;
	virtual void AI_chooseProduction(bool bInterruptWonders) = 0;
	virtual bool AI_isChooseProductionDirty() = 0;
	virtual void AI_setChooseProductionDirty(bool bNewValue) = 0;

	virtual int AI_GetNumPlotsAcquiredByOtherPlayer(PlayerTypes ePlayer) const = 0;
	virtual void AI_ChangeNumPlotsAcquiredByOtherPlayer(PlayerTypes ePlayer, int iChange) = 0;

	void invalidatePopulationRankCache();
	void invalidateYieldRankCache(YieldTypes eYield = NO_YIELD);

	bool CommitToBuildingUnitForOperation();
	UnitTypes GetUnitForOperation();
	virtual bool IsBuildingUnitForOperation() const
	{
		return m_unitBeingBuiltForOperation.IsValid();
	}

	const char* GetCityBombardEffectTag() const;
	uint GetCityBombardEffectTagHash() const;

	int GetExtraHitPoints() const;
	void ChangeExtraHitPoints(int iValue);

	int GetMaxHitPoints() const;
	const FAutoArchive& getSyncArchive() const;
	FAutoArchive& getSyncArchive();
	std::string debugDump(const FAutoVariableBase&) const;
	std::string stackTraceRemark(const FAutoVariableBase&) const;

	bool			IsBusy() const;
	// Combat related
	const CvUnit*	getCombatUnit() const;
	CvUnit*			getCombatUnit();
	void			setCombatUnit(CvUnit* pUnit, bool bAttacking = false);
	void			clearCombat();
	bool			isFighting() const;

#ifdef BUILDING_FAITH_TO_SCIENCE
	int getFaithToScience() const;
	void changeFaithToScience(int iChange);
#endif
#ifdef BUILDING_CITY_TILE_WORK_SPEED_MOD
	int getCityTileWorkSpeedModifier() const;
	void changeCityTileWorkSpeedModifier(int iChange);
#endif
#ifdef BUILDING_HURRY_COST_MODIFIER
	int getBuildingHurryCostModifier() const;
	void changeBuildingHurryCostModifier(int iChange);
#endif
#ifdef BUILDING_GROWTH_GOLD
	int getGrowthGold() const;
	void changeGrowthGold(int iChange);
#endif
#ifdef BUILDING_SCIENCE_PER_X_POP
	int getSciencePerXPop() const;
	void changeSciencePerXPop(int iChange);
#endif
#ifdef BUILDING_CULTURE_PER_X_ANCIENCT_BUILDING
	int getCulturePerXAncientBuildings() const;
	void changeCulturePerXAncientBuildings(int iChange);
#endif
#ifdef BUILDING_NO_HOLY_CITY_AND_NO_OCCUPIED_UNHAPPINESS
	int getNoHolyCityAndNoOccupiedUnhappiness() const;
	void changeNoHolyCityAndNoOccupiedUnhappiness(int iChange);
#endif
#ifdef BUILDING_NEARBY_ENEMY_DAMAGE
	int getNearbyEnemyDamage() const;
	void changeNearbyEnemyDamage(int iChange);
#endif
#ifdef BUILDING_NAVAL_COMBAT_MODIFIER_NEAR_CITY
	int getNavalCombatModifierNearCity() const;
	void changeNavalCombatModifierNearCity(int iChange);
#endif
#ifdef BUILDING_HAPPINESS_FOR_FILLED_GREAT_WORK_SLOT
	int getHappinessForFilledGreatWorkSlot() const;
	void changeHappinessForFilledGreatWorkSlot(int iChange);
#endif
#ifdef BUILDING_FOOD_BONUS_IF_NO_CITIES_AROUND
	int getFoodBonusIfNoCitiesAround() const;
	void changeFoodBonusIfNoCitiesAround(int iChange);
#endif
#ifdef BUILDING_LOCAL_CITY_CONNECTION_TRADE_ROUTE_MODIFIER
	int getLocalCityConnectionTradeRouteModifier() const;
	void changeLocalCityConnectionTradeRouteModifier(int iChange);
#endif
#ifdef BUILDING_NON_AIR_UNIT_MAX_HEAL
	int getNonAirUnitMaxHeal() const;
	void changeNonAirUnitMaxHeal(int iChange);
#endif
#ifdef BUILDING_DOUBLE_PANTHEON
	int getDoublePantheon() const;
	void changeDoublePantheon(int iChange);
#endif

	int iScratch; // know the scope of your validity

protected:
	FAutoArchiveClassContainer<CvCity> m_syncArchive;

	FAutoVariable<CvString, CvCity> m_strNameIAmNotSupposedToBeUsedAnyMoreBecauseThisShouldNotBeCheckedAndWeNeedToPreserveSaveGameCompatibility;
	FAutoVariable<PlayerTypes, CvCity> m_eOwner;
	FAutoVariable<int, CvCity> m_iX;
	FAutoVariable<int, CvCity> m_iY;
	FAutoVariable<int, CvCity> m_iID;

	FAutoVariable<int, CvCity> m_iRallyX;
	FAutoVariable<int, CvCity> m_iRallyY;
	FAutoVariable<int, CvCity> m_iGameTurnFounded;
	FAutoVariable<int, CvCity> m_iGameTurnAcquired;
	FAutoVariable<int, CvCity> m_iGameTurnLastExpanded;
	FAutoVariable<int, CvCity> m_iPopulation;
	FAutoVariable<int, CvCity> m_iHighestPopulation;
	int m_iExtraHitPoints;

	FAutoVariable<int, CvCity> m_iNumGreatPeople;
	FAutoVariable<int, CvCity> m_iBaseGreatPeopleRate;
	FAutoVariable<int, CvCity> m_iGreatPeopleRateModifier;
#ifdef BUILDING_CITY_RANGE_MODIFIER
	FAutoVariable<int, CvCity> m_iCityAttackRangeModifier;
#endif
#ifdef BUILDING_CITY_EXTRA_ATTACK
	FAutoVariable<int, CvCity> m_iCityExtraAttack;
	FAutoVariable<int, CvCity> m_iCityCurrentExtraAttack;
#endif
#ifdef BUILDING_CITY_EXTRA_HEAL
	FAutoVariable<int, CvCity> m_iCityExtraHeal;
#endif
	FAutoVariable<int, CvCity> m_iJONSCultureStored;
	FAutoVariable<int, CvCity> m_iJONSCultureLevel;
	FAutoVariable<int, CvCity> m_iJONSCulturePerTurnFromBuildings;
	FAutoVariable<int, CvCity> m_iJONSCulturePerTurnFromPolicies;
	FAutoVariable<int, CvCity> m_iJONSCulturePerTurnFromSpecialists;
	FAutoVariable<int, CvCity> m_iJONSCulturePerTurnFromReligion;
	int m_iFaithPerTurnFromBuildings;
	int m_iFaithPerTurnFromPolicies;
	int m_iFaithPerTurnFromReligion;
	FAutoVariable<int, CvCity> m_iCultureRateModifier;
	FAutoVariable<int, CvCity> m_iNumWorldWonders;
	FAutoVariable<int, CvCity> m_iNumTeamWonders;
	FAutoVariable<int, CvCity> m_iNumNationalWonders;
	FAutoVariable<int, CvCity> m_iWonderProductionModifier;
	FAutoVariable<int, CvCity> m_iCapturePlunderModifier;
	FAutoVariable<int, CvCity> m_iPlotCultureCostModifier;
	int m_iPlotBuyCostModifier;
	FAutoVariable<int, CvCity> m_iMaintenance;
	FAutoVariable<int, CvCity> m_iHealRate;
	FAutoVariable<int, CvCity> m_iNoOccupiedUnhappinessCount;
	FAutoVariable<int, CvCity> m_iFood;
	FAutoVariable<int, CvCity> m_iFoodKept;
	FAutoVariable<int, CvCity> m_iMaxFoodKeptPercent;
	FAutoVariable<int, CvCity> m_iOverflowProduction;
	FAutoVariable<int, CvCity> m_iFeatureProduction;
	FAutoVariable<int, CvCity> m_iMilitaryProductionModifier;
	FAutoVariable<int, CvCity> m_iSpaceProductionModifier;
	FAutoVariable<int, CvCity> m_iFreeExperience;
	FAutoVariable<int, CvCity> m_iCurrAirlift; // unused
	FAutoVariable<int, CvCity> m_iMaxAirUnits;
	FAutoVariable<int, CvCity> m_iAirModifier; // unused
	FAutoVariable<int, CvCity> m_iNukeModifier;
	int m_iTradeRouteTargetBonus;
	int m_iTradeRouteRecipientBonus;
	FAutoVariable<int, CvCity> m_iCultureUpdateTimer;
	FAutoVariable<int, CvCity> m_iCitySizeBoost;
	FAutoVariable<int, CvCity> m_iSpecialistFreeExperience;
	FAutoVariable<int, CvCity> m_iStrengthValue;
	FAutoVariable<int, CvCity> m_iDamage;
	FAutoVariable<int, CvCity> m_iThreatValue;
	FAutoVariable<int, CvCity> m_iGarrisonedUnit;  // unused
	FAutoVariable<int, CvCity> m_iResourceDemanded;
	FAutoVariable<int, CvCity> m_iWeLoveTheKingDayCounter;
	FAutoVariable<int, CvCity> m_iLastTurnGarrisonAssigned;
	FAutoVariable<int, CvCity> m_iThingsProduced; // total number of units, buildings, wonders, etc. this city has constructed
	FAutoVariable<int, CvCity> m_iDemandResourceCounter;
	FAutoVariable<int, CvCity> m_iResistanceTurns;
	FAutoVariable<int, CvCity> m_iRazingTurns;
	FAutoVariable<int, CvCity> m_iCountExtraLuxuries;
	FAutoVariable<int, CvCity> m_iCheapestPlotInfluence;
	int m_iEspionageModifier;

	OperationSlot m_unitBeingBuiltForOperation;

	FAutoVariable<bool, CvCity> m_bNeverLost;
	FAutoVariable<bool, CvCity> m_bDrafted;
	FAutoVariable<bool, CvCity> m_bAirliftTargeted;   // unused
	FAutoVariable<bool, CvCity> m_bProductionAutomated;
	FAutoVariable<bool, CvCity> m_bLayoutDirty;
	FAutoVariable<bool, CvCity> m_bMadeAttack;
	FAutoVariable<bool, CvCity> m_bOccupied;
	FAutoVariable<bool, CvCity> m_bPuppet;
	bool m_bIgnoreCityForHappiness;
	FAutoVariable<bool, CvCity> m_bEverCapital;
	FAutoVariable<bool, CvCity> m_bIndustrialRouteToCapital;
	FAutoVariable<bool, CvCity> m_bFeatureSurrounded;

	FAutoVariable<PlayerTypes, CvCity> m_ePreviousOwner;
	FAutoVariable<PlayerTypes, CvCity> m_eOriginalOwner;
	FAutoVariable<PlayerTypes, CvCity> m_ePlayersReligion;

	FAutoVariable<std::vector<int>, CvCity> m_aiSeaPlotYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiRiverPlotYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiLakePlotYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiSeaResourceYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromTerrain;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromBuildings;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromSpecialists;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromMisc;
	std::vector<int> m_aiBaseYieldRateFromReligion;
	FAutoVariable<std::vector<int>, CvCity> m_aiYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiYieldPerPop;
	std::vector<int> m_aiYieldPerReligion;
	FAutoVariable<std::vector<int>, CvCity> m_aiPowerYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiResourceYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiExtraSpecialistYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiProductionToYieldModifier;
#ifdef FIX_EXCHANGE_PRODUCTION_OVERFLOW_INTO_GOLD_OR_SCIENCE
	int m_iProcessOverflowProductionTimes100;
#endif
	FAutoVariable<std::vector<int>, CvCity> m_aiDomainFreeExperience;
	FAutoVariable<std::vector<int>, CvCity> m_aiDomainProductionModifier;

	FAutoVariable<std::vector<bool>, CvCity> m_abEverOwned;
	FAutoVariable<std::vector<bool>, CvCity> m_abRevealed;

	FAutoVariable<CvString, CvCity> m_strScriptData;

	FAutoVariable<std::vector<int>, CvCity> m_paiNoResource;
	FAutoVariable<std::vector<int>, CvCity> m_paiFreeResource;
	FAutoVariable<std::vector<int>, CvCity> m_paiNumResourcesLocal;
	FAutoVariable<std::vector<int>, CvCity> m_paiProjectProduction;
	FAutoVariable<std::vector<int>, CvCity> m_paiSpecialistProduction;
	FAutoVariable<std::vector<int>, CvCity> m_paiUnitProduction;
	FAutoVariable<std::vector<int>, CvCity> m_paiUnitProductionTime;
	FAutoVariable<std::vector<int>, CvCity> m_paiSpecialistCount;
	FAutoVariable<std::vector<int>, CvCity> m_paiMaxSpecialistCount;
	FAutoVariable<std::vector<int>, CvCity> m_paiForceSpecialistCount;
	FAutoVariable<std::vector<int>, CvCity> m_paiFreeSpecialistCount;
	FAutoVariable<std::vector<int>, CvCity> m_paiImprovementFreeSpecialists;
	FAutoVariable<std::vector<int>, CvCity> m_paiUnitCombatFreeExperience;
	FAutoVariable<std::vector<int>, CvCity> m_paiUnitCombatProductionModifier;
	FAutoVariable<std::vector<int>, CvCity> m_paiFreePromotionCount;

	int m_iBaseHappinessFromBuildings;
	int m_iUnmoddedHappinessFromBuildings;

	bool m_bRouteToCapitalConnectedLastTurn;
	bool m_bRouteToCapitalConnectedThisTurn;
	CvString m_strName;

	bool m_bOwedCultureBuilding;
#ifdef OWED_FOOD_BUILDING
	bool m_bOwedFoodBuilding;
#endif
#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
	ReligionTypes eReligionFoundedHere;
#endif

	mutable FFastSmallFixedList< OrderData, 25, true, c_eCiv5GameplayDLL > m_orderQueue;

	int** m_aaiBuildingSpecialistUpgradeProgresses;
	int** m_ppaiResourceYieldChange;
	int** m_ppaiFeatureYieldChange;
	int** m_ppaiTerrainYieldChange;
#ifdef BUILDING_IMPROVEMENT_YIELD_CHANGE
	int** m_ppaiImprovementYieldChange;
#endif

	CvCityBuildings* m_pCityBuildings;
	CvCityStrategyAI* m_pCityStrategyAI;
	CvCityCitizens* m_pCityCitizens;
	CvCityReligions* m_pCityReligions;
	CvCityEmphases* m_pEmphases;
	CvCityEspionage* m_pCityEspionage;
	CvCityCulture* m_pCityCulture;

	mutable int m_bombardCheckTurn;

	// CACHE: cache frequently used values
	mutable int	m_iPopulationRank;
	mutable bool m_bPopulationRankValid;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRank;
	FAutoVariable<std::vector<bool>, CvCity> m_abBaseYieldRankValid;
	FAutoVariable<std::vector<int>, CvCity> m_aiYieldRank;
	FAutoVariable<std::vector<bool>, CvCity> m_abYieldRankValid;

	IDInfo m_combatUnit;		// The unit the city is in combat with

#ifdef BUILDING_FAITH_TO_SCIENCE
	FAutoVariable<int, CvCity> m_iFaithToScience;
#endif
#ifdef BUILDING_CITY_TILE_WORK_SPEED_MOD
	FAutoVariable<int, CvCity> m_iCityTileWorkSpeedModifier;
#endif
#ifdef BUILDING_HURRY_COST_MODIFIER
	FAutoVariable<int, CvCity> m_iBuildingHurryCostModifier;
#endif
#ifdef BUILDING_GROWTH_GOLD
	FAutoVariable<int, CvCity> m_iGrowthGold;
#endif
#ifdef BUILDING_SCIENCE_PER_X_POP
	FAutoVariable<int, CvCity> m_iSciencePerXPop;
#endif
#ifdef BUILDING_CULTURE_PER_X_ANCIENCT_BUILDING
	FAutoVariable<int, CvCity> m_iCulturePerXAncientBuildings;
#endif
#ifdef BUILDING_NEAR_MOUNTAIN_YIELD_CHANGES
	FAutoVariable<std::vector<int>, CvCity> m_aiNearMountainYield;
#endif
#ifdef BUILDING_NO_HOLY_CITY_AND_NO_OCCUPIED_UNHAPPINESS
	FAutoVariable<int, CvCity> m_iNoHolyCityAndNoOccupiedUnhappiness;
#endif
#ifdef BUILDING_NEARBY_ENEMY_DAMAGE
	FAutoVariable<int, CvCity> m_iNearbyEnemyDamage;
#endif
#ifdef BUILDING_NAVAL_COMBAT_MODIFIER_NEAR_CITY
	FAutoVariable<int, CvCity> m_iNavalCombatModifierNearCity;
#endif
#ifdef BUILDING_HAPPINESS_FOR_FILLED_GREAT_WORK_SLOT
	FAutoVariable<int, CvCity> m_iHappinessForFilledGreatWorkSlot;
#endif
#ifdef BUILDING_FOOD_BONUS_IF_NO_CITIES_AROUND
	FAutoVariable<int, CvCity> m_iFoodBonusIfNoCitiesAround;
#endif
#ifdef BUILDING_LOCAL_CITY_CONNECTION_TRADE_ROUTE_MODIFIER
	FAutoVariable<int, CvCity> m_iLocalCityConnectionTradeRouteModifier;
#endif
#ifdef BUILDING_NON_AIR_UNIT_MAX_HEAL
	FAutoVariable<int, CvCity> m_iNonAirUnitMaxHeal;
#endif
#ifdef BUILDING_DOUBLE_PANTHEON
	FAutoVariable<int, CvCity> m_iDoublePantheon;
#endif

	void doGrowth();
	void doProduction(bool bAllowNoProduction);
	void doProcess();
	void doDecay();
	void doGreatPeople();
	void doMeltdown();
	bool doCheckProduction();

	int getExtraProductionDifference(int iExtra, UnitTypes eUnit) const;
	int getExtraProductionDifference(int iExtra, BuildingTypes eBuilding) const;
	int getExtraProductionDifference(int iExtra, ProjectTypes eProject) const;
	int getExtraProductionDifference(int iExtra, int iModifier) const;
	int getHurryCostModifier(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const;
	int getHurryCostModifier(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const;
#ifdef BUILDING_HURRY_COST_MODIFIER
	int getHurryCostModifier(HurryTypes eHurry, int iBaseModifier, int iProduction, bool bIgnoreNew, bool bIsBuilding = false) const;
#else
	int getHurryCostModifier(HurryTypes eHurry, int iBaseModifier, int iProduction, bool bIgnoreNew) const;
#endif
	int getHurryCost(HurryTypes eHurry, bool bExtra, UnitTypes eUnit, bool bIgnoreNew) const;
	int getHurryCost(HurryTypes eHurry, bool bExtra, BuildingTypes eBuilding, bool bIgnoreNew) const;
	int getHurryCost(bool bExtra, int iProductionLeft, int iHurryModifier, int iModifier) const;
	int getHurryPopulation(HurryTypes eHurry, int iHurryCost) const;
	int getHurryGold(HurryTypes eHurry, int iHurryCost, int iFullCost) const;
	bool canHurryUnit(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const;
	bool canHurryBuilding(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const;
};

namespace FSerialization
{
void SyncCities();
void ClearCityDeltas();
}

#endif
