/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */

#include "CvGameCoreDLLPCH.h"
#include "CvGlobals.h"
#include "CvArea.h"
#include "CvMap.h"
#include "CvPlot.h"
#include "CvRandom.h"
#include "CvTeam.h"
#include "CvGameCoreUtils.h"
#include "CvPlayerAI.h"
#include "CvPlayer.h"
#include "CvGameCoreUtils.h"
#include "CvInfos.h"
#include "CvAStar.h"
#include "CvGameTextMgr.h"
#include "CvDiplomacyAI.h"
#include "CvEconomicAI.h"
#include "CvMilitaryAI.h"
#include "CvCitySpecializationAI.h"
#include "CvWonderProductionAI.h"
#include "CvGrandStrategyAI.h"
#include "CvDiplomacyAI.h"
#include "CvTechAI.h"
#include "CvFlavorManager.h"
#include "CvHomelandAI.h"
#include "CvMinorCivAI.h"
#include "CvDealAI.h"
#include "CvImprovementClasses.h"
#include "CvBuilderTaskingAI.h"
#include "CvDangerPlots.h"
#include "CvCityConnections.h"
#include "CvNotifications.h"
#include "CvDiplomacyRequests.h"
#include "cvStopWatch.h"
#include "CvTypes.h"

#include "ICvDLLUserInterface.h"
#include "CvEnumSerialization.h"
#include "FStlContainerSerialization.h"
#include <sstream>

#include "CvInternalGameCoreUtils.h"
#include "CvAchievementUnlocker.h"
#include "CvInfosSerializationHelper.h"

#include "CvDllCity.h"
#include "CvGoodyHuts.h"

// Include this after all other headers.
#define LINT_WARNINGS_ONLY
#include "LintFree.h"

//------------------------------------------------------------------------------
// CvPlayer Version History
// Version 1 
//	 * CvPlayer save version reset for expansion pack 2.
//------------------------------------------------------------------------------
const int g_CurrentCvPlayerVersion = 16;

//Simply empty check utility.
bool isEmpty(const char* szString)
{
	return szString == NULL || szString[0] == '\0';
}

//	--------------------------------------------------------------------------------
// Public Functions...
namespace FSerialization
{
void SyncPlayer()
{
	if(GC.getGame().isNetworkMultiPlayer())
	{
		PlayerTypes eAuthoritativePlayerID = GC.getGame().getActivePlayer();
		CvPlayer& authoritativePlayer = GET_PLAYER(eAuthoritativePlayerID);
		const FAutoArchive& archive = authoritativePlayer.getSyncArchive();
		if(archive.hasDeltas())
		{
			FMemoryStream ms;
			std::vector<std::pair<std::string, std::string> > callStacks;
			archive.saveDelta(ms, callStacks);
			gDLL->sendPlayerSyncCheck(eAuthoritativePlayerID, ms, callStacks);
		}

		// host is authoritative for AI players

		if(gDLL->IsHost())
		{
			for(int i = 0; i < MAX_PLAYERS; ++i)
			{
				CvPlayer& player = GET_PLAYER(static_cast<PlayerTypes>(i));
				if(!player.isHuman() && player.isAlive())
				{
					const FAutoArchive& aiArchive = player.getSyncArchive();
					FMemoryStream ms;
					std::vector<std::pair<std::string, std::string> > callStacks;
					aiArchive.saveDelta(ms, callStacks);
					gDLL->sendPlayerSyncCheck(static_cast<PlayerTypes>(i), ms, callStacks);
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
// clears ALL deltas for ALL players
void ClearPlayerDeltas()
{
	int i = 0;
	for(i = 0; i < MAX_PLAYERS; ++i)
	{
		CvPlayer& player = GET_PLAYER(static_cast<PlayerTypes>(i));
		FAutoArchive& archive = player.getSyncArchive();
		archive.clearDelta();
	}
}
}

//	--------------------------------------------------------------------------------
CvPlayer::CvPlayer() :
	m_syncArchive(*this)
#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
	, m_bIsDisconnected("CvPlayer::m_bIsDisconnected", m_syncArchive)
#endif
	, m_iStartingX("CvPlayer::m_iStartingX", m_syncArchive)
	, m_iStartingY("CvPlayer::m_iStartingY", m_syncArchive)
	, m_iTotalPopulation("CvPlayer::m_iTotalPopulation", m_syncArchive, true)
	, m_iTotalLand("CvPlayer::m_iTotalLand", m_syncArchive)
	, m_iTotalLandScored("CvPlayer::m_iTotalLandScored", m_syncArchive)
	, m_iJONSCulturePerTurnForFree("CvPlayer::m_iJONSCulturePerTurnForFree", m_syncArchive)
	, m_iJONSCulturePerTurnFromMinorCivs("CvPlayer::m_iJONSCulturePerTurnFromMinorCivs", m_syncArchive)
	, m_iJONSCultureCityModifier("CvPlayer::m_iJONSCultureCityModifier", m_syncArchive)
	, m_iJONSCulture("CvPlayer::m_iJONSCulture", m_syncArchive, true)
	, m_iJONSCultureEverGenerated("CvPlayer::m_iJONSCulture", m_syncArchive)
	, m_iCulturePerWonder("CvPlayer::m_iCulturePerWonder", m_syncArchive)
	, m_iCultureWonderMultiplier("CvPlayer::m_iCultureWonderMultiplier", m_syncArchive)
	, m_iCulturePerTechResearched("CvPlayer::m_iCulturePerTechResearched", m_syncArchive)
	, m_iFaith(0)
	, m_iFaithEverGenerated(0)
	, m_iHappiness("CvPlayer::m_iHappiness", m_syncArchive)
	, m_iUprisingCounter("CvPlayer::m_iUprisingCounter", m_syncArchive)
	, m_iExtraHappinessPerLuxury("CvPlayer::m_iExtraHappinessPerLuxury", m_syncArchive)
	, m_iUnhappinessFromUnits("CvPlayer::m_iUnhappinessFromUnits", m_syncArchive)
	, m_iUnhappinessFromUnitsMod("CvPlayer::m_iUnhappinessFromUnitsMod", m_syncArchive)
	, m_iUnhappinessMod("CvPlayer::m_iUnhappinessMod", m_syncArchive)
	, m_iCityCountUnhappinessMod("CvPlayer::m_iCityCountUnhappinessMod", m_syncArchive)
	, m_iOccupiedPopulationUnhappinessMod("CvPlayer::m_iOccupiedPopulationUnhappinessMod", m_syncArchive)
	, m_iCapitalUnhappinessMod("CvPlayer::m_iCapitalUnhappinessMod", m_syncArchive)
	, m_iCityRevoltCounter("CvPlayer::m_iCityRevoltCounter", m_syncArchive)
	, m_iHappinessPerGarrisonedUnitCount("CvPlayer::m_iHappinessPerGarrisonedUnitCount", m_syncArchive)
	, m_iHappinessPerTradeRouteCount("CvPlayer::m_iHappinessPerTradeRouteCount", m_syncArchive)
	, m_iHappinessPerXPopulation(0)
	, m_iHappinessFromLeagues(0)
	, m_iEspionageModifier(0)
	, m_iSpyStartingRank(0)
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
	, m_iNumStolenScience(0)
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
	, m_iNumTrainedUnits(0)
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
	, m_iNumKilledUnits(0)
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
	, m_iNumLostUnits(0)
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
	, m_iUnitsDamageDealt(0)
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
	, m_iUnitsDamageTaken(0)
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
	, m_iCitiesDamageDealt(0)
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
	, m_iCitiesDamageTaken(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
	, m_iNumScientistsTotal(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
	, m_iNumEngineersTotal(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
	, m_iNumMerchantsTotal(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
	, m_iNumWritersTotal(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
	, m_iNumArtistsTotal(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
	, m_iNumMusiciansTotal(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
	, m_iNumGeneralsTotal(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
	, m_iNumAdmiralsTotal(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
	, m_iNumProphetsTotal(0)
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
	, m_iProductionGoldFromWonders(0)
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
	, m_iNumChops(0)
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
	, m_iNumTimesOpenedDemographics(0)
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
	, m_bMayaBoostScientist(false)
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
	, m_bMayaBoostEngineers(false)
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
	, m_bMayaBoostMerchants(false)
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
	, m_bMayaBoostWriters(false)
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
	, m_bMayaBoostArtists(false)
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
	, m_bMayaBoostMusicians(false)
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
	, m_iScientistsTotalScienceBoost(0)
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
	, m_iEngineersTotalHurryBoost(0)
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
	, m_iMerchantsTotalTradeBoost(0)
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
	, m_iWritersTotalCultureBoost(0)
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
	, m_iMusiciansTotalTourismBoost(0)
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
	, m_iNumPopulationLostFromNukes(0)
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
	, m_iNumCSQuestsCompleted(0)
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
	, m_iNumAlliedCS(0)
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
	, m_iTimesEnteredCityScreen(0)
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
	, m_iNumDiedSpies(0)
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
	, m_iNumKilledSpies(0)
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
	, m_iFoodFromMinorsTimes100(0)
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
	, m_iProductionFromMinorsTimes100(0)
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
	, m_iNumUnitsFromMinors(0)
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
	, m_iNumCreatedWorldWonders(0)
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
	, m_iNumGoldSpentOnBuildingBuys(0)
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
	, m_iNumGoldSpentOnUnitBuys(0)
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
	, m_iNumGoldSpentOnUgrades(0)
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
	, m_iGoldFromKills(0)
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
	, m_iCultureFromKills(0)
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
	, m_iNumGoldSpentOnGPBuys(0)
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
	, m_iNumGoldSpentOnTilesBuys(0)
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
	, m_iNumGoldFromPillage(0)
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
	, m_iNumGoldFromPlunder(0)
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
	, m_iNumFaithSpentOnMilitaryUnits(0)
#endif
	, m_iExtraLeagueVotes(0)
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
	, m_iMaxExtraVotesFromMinors(0)
#endif
#ifdef POLICY_EXTRA_VOTES
	, m_iPolicyExtraVotes(0)
#endif
#ifdef POLICY_DO_TECH_FROM_CITY_CONQ
	, m_iPolicyTechFromCityConquer(0)
#endif
#ifdef POLICY_NO_CULTURE_SPECIALIST_FOOD
	, m_iNoCultureSpecialistFood(0)
#endif
#ifdef POLICY_MINORS_GIFT_UNITS
	, m_iMinorsGiftUnits(0)
#endif
#ifdef POLICY_NO_CARGO_PILLAGE
	, m_iNoCargoPillage(0)
#endif
#ifdef POLICY_GREAT_WORK_HAPPINESS
	, m_iGreatWorkHappiness(0)
#endif
#ifdef POLICY_SCIENCE_PER_X_FOLLOWERS
	, m_iSciencePerXFollowers(0)
#endif
#ifdef POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD
	, m_iNoDifferentIdeologiesTourismMod(0)
#endif
#ifdef POLICY_GLOBAL_POP_CHANGE
	, m_iGlobalPopChange(0)
#endif
#ifdef POLICY_GREAT_WORK_TOURISM_CHANGES
	, m_iGreatWorkTourismChanges(0)
#endif
#ifdef POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP
	, m_iCityScienceSquaredModPerXPop(0)
#endif
	, m_iSpecialPolicyBuildingHappiness("CvPlayer::m_iSpecialPolicyBuildingHappiness", m_syncArchive)
	, m_iWoundedUnitDamageMod("CvPlayer::m_iWoundedUnitDamageMod", m_syncArchive)
	, m_iUnitUpgradeCostMod("CvPlayer::m_iUnitUpgradeCostMod", m_syncArchive)
	, m_iBarbarianCombatBonus("CvPlayer::m_iBarbarianCombatBonus", m_syncArchive)
	, m_iAlwaysSeeBarbCampsCount("CvPlayer::m_iAlwaysSeeBarbCampsCount", m_syncArchive)
	, m_iHappinessFromBuildings("CvPlayer::m_iHappinessFromBuildings", m_syncArchive)
	, m_iHappinessPerCity("CvPlayer::m_iHappinessPerCity", m_syncArchive)
	, m_iHappinessPerXPolicies(0)
	, m_iAdvancedStartPoints("CvPlayer::m_iAdvancedStartPoints", m_syncArchive)
	, m_iAttackBonusTurns("CvPlayer::m_iAttackBonusTurns", m_syncArchive)
	, m_iCultureBonusTurns(0)
	, m_iTourismBonusTurns(0)
	, m_iGoldenAgeProgressMeter("CvPlayer::m_iGoldenAgeProgressMeter", m_syncArchive, true)
	, m_iGoldenAgeMeterMod("CvPlayer::m_iGoldenAgeMeterMod", m_syncArchive)
	, m_iNumGoldenAges("CvPlayer::m_iNumGoldenAges", m_syncArchive)
	, m_iGoldenAgeTurns("CvPlayer::m_iGoldenAgeTurns", m_syncArchive)
#ifdef TAJ_MAHAL_STARTS_GA_NEXT_TURN
	, m_iBuildingGoldenAgeTurns("CvPlayer::m_iBuildingGoldenAgeTurns", m_syncArchive)
		
#endif
	, m_iNumUnitGoldenAges("CvPlayer::m_iNumUnitGoldenAges", m_syncArchive)
	, m_iStrikeTurns("CvPlayer::m_iStrikeTurns", m_syncArchive)
	, m_iGoldenAgeModifier("CvPlayer::m_iGoldenAgeModifier", m_syncArchive)
	, m_iGreatPeopleCreated("CvPlayer::m_iGreatPeopleCreated", m_syncArchive)
	, m_iGreatGeneralsCreated("CvPlayer::m_iGreatGeneralsCreated", m_syncArchive)
	, m_iGreatAdmiralsCreated(0)
	, m_iGreatWritersCreated(0)
	, m_iGreatArtistsCreated(0)
	, m_iGreatMusiciansCreated(0)
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	, m_bHasUsedDharma(false)
#endif
#ifdef UNDERGROUND_SECT_REWORK
	, m_bHasUsedUndergroundSect(false)
#endif
#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
	, m_bHasUsedMissionaryZeal(false)
#endif
#ifdef UNITY_OF_PROPHETS_EXTRA_PROPHETS
	, m_bHasUsedUnityProphets(false)
#endif
#ifdef GODDESS_LOVE_FREE_WORKER
	, m_bHasUsedGoddessLove(false)
#endif
#ifdef GOD_SEA_FREE_WORK_BOAT
	, m_bHasUsedGodSea(false)
#endif
#ifdef FREE_GREAT_PERSON
	, m_iGreatProphetsCreated(0)
#endif
#if defined SEPARATE_GREAT_PEOPLE || defined SWEDEN_UA_REWORK
	, m_iGreatScientistsCreated(0)
	, m_iGreatEngineersCreated(0)
	, m_iGreatMerchantsCreated(0)
#endif
#ifdef SEPARATE_MERCHANTS
	, m_iGreatMerchantsCreated(0)
#endif
	, m_iMerchantsFromFaith(0)
	, m_iScientistsFromFaith(0)
	, m_iWritersFromFaith(0)
	, m_iArtistsFromFaith(0)
	, m_iMusiciansFromFaith(0)
	, m_iGeneralsFromFaith(0)
	, m_iAdmiralsFromFaith(0)
	, m_iEngineersFromFaith(0)
#ifdef BELIEF_TO_GLORY_OF_GOD_ONE_GP_OF_EACH_TYPE
	, m_bMerchantsFromFaith(false)
	, m_bScientistsFromFaith(false)
	, m_bWritersFromFaith(false)
	, m_bArtistsFromFaith(false)
	, m_bMusiciansFromFaith(false)
	, m_bGeneralsFromFaith(false)
	, m_bAdmiralsFromFaith(false)
	, m_bEngineersFromFaith(false)
#endif
	, m_iGreatPeopleThresholdModifier("CvPlayer::m_iGreatPeopleThresholdModifier", m_syncArchive)
	, m_iGreatGeneralsThresholdModifier("CvPlayer::m_iGreatGeneralsThresholdModifier", m_syncArchive)
	, m_iGreatAdmiralsThresholdModifier(0)
	, m_iGreatGeneralCombatBonus(0)
	, m_iAnarchyNumTurns("CvPlayer::m_iAnarchyNumTurns", m_syncArchive)
	, m_iPolicyCostModifier("CvPlayer::m_iPolicyCostModifier", m_syncArchive)
	, m_iGreatPeopleRateModifier("CvPlayer::m_iGreatPeopleRateModifier", m_syncArchive)
	, m_iGreatPeopleRateModFromBldgs("CvPlayer::m_iGreatPeopleRateModFromBldgs", m_syncArchive)
	, m_iGreatGeneralRateModifier("CvPlayer::m_iGreatGeneralRateModifier", m_syncArchive)
	, m_iGreatGeneralRateModFromBldgs("CvPlayer::m_iGreatGeneralRateModFromBldgs", m_syncArchive)
	, m_iDomesticGreatGeneralRateModifier("CvPlayer::m_iDomesticGreatGeneralRateModifier", m_syncArchive)
	, m_iDomesticGreatGeneralRateModFromBldgs("CvPlayer::m_iDomesticGreatGeneralRateModFromBldgs", m_syncArchive)
	, m_iGreatScientistBeakerModifier(0)
	, m_iGreatPersonExpendGold(0)
	, m_iMaxGlobalBuildingProductionModifier("CvPlayer::m_iMaxGlobalBuildingProductionModifier", m_syncArchive)
	, m_iMaxTeamBuildingProductionModifier("CvPlayer::m_iMaxTeamBuildingProductionModifier", m_syncArchive)
	, m_iMaxPlayerBuildingProductionModifier("CvPlayer::m_iMaxPlayerBuildingProductionModifier", m_syncArchive)
	, m_iFreeExperience("CvPlayer::m_iFreeExperience", m_syncArchive)
	, m_iFreeExperienceFromBldgs("CvPlayer::m_iFreeExperienceFromBldgs", m_syncArchive)
	, m_iFreeExperienceFromMinors("CvPlayer::m_iFreeExperienceFromMinors", m_syncArchive)
	, m_iFeatureProductionModifier("CvPlayer::m_iFeatureProductionModifier", m_syncArchive)
	, m_iWorkerSpeedModifier("CvPlayer::m_iWorkerSpeedModifier", m_syncArchive)
	, m_iImprovementCostModifier("CvPlayer::m_iImprovementCostModifier", m_syncArchive)
	, m_iImprovementUpgradeRateModifier("CvPlayer::m_iImprovementUpgradeRateModifier", m_syncArchive)
	, m_iSpecialistProductionModifier("CvPlayer::m_iSpecialistProductionModifier", m_syncArchive)
	, m_iMilitaryProductionModifier("CvPlayer::m_iMilitaryProductionModifier", m_syncArchive)
	, m_iSpaceProductionModifier("CvPlayer::m_iSpaceProductionModifier", m_syncArchive)
	, m_iCityDefenseModifier("CvPlayer::m_iCityDefenseModifier", m_syncArchive)
	, m_iUnitFortificationModifier("CvPlayer::m_iUnitFortificationModifier", m_syncArchive)
	, m_iUnitBaseHealModifier("CvPlayer::m_iUnitBaseHealModifier", m_syncArchive)
	, m_iWonderProductionModifier("CvPlayer::m_iWonderProductionModifier", m_syncArchive)
	, m_iSettlerProductionModifier("CvPlayer::m_iSettlerProductionModifier", m_syncArchive)
	, m_iCapitalSettlerProductionModifier("CvPlayer::m_iCapitalSettlerProductionModifier", m_syncArchive)
	, m_iUnitProductionMaintenanceMod("CvPlayer::m_iUnitProductionMaintenanceMod", m_syncArchive)
	, m_iPolicyCostBuildingModifier("CvPlayer::m_iPolicyCostBuildingModifier", m_syncArchive)
	, m_iPolicyCostMinorCivModifier("CvPlayer::m_iPolicyCostMinorCivModifier", m_syncArchive)
	, m_iInfluenceSpreadModifier(0)
	, m_iExtraVotesPerDiplomat(0)
	, m_iNumNukeUnits("CvPlayer::m_iNumNukeUnits", m_syncArchive)
	, m_iNumOutsideUnits("CvPlayer::m_iNumOutsideUnits", m_syncArchive, true)
	, m_iBaseFreeUnits("CvPlayer::m_iBaseFreeUnits", m_syncArchive)
	, m_iBaseFreeMilitaryUnits("CvPlayer::m_iBaseFreeMilitaryUnits", m_syncArchive)
	, m_iFreeUnitsPopulationPercent("CvPlayer::m_iFreeUnitsPopulationPercent", m_syncArchive)
	, m_iFreeMilitaryUnitsPopulationPercent("CvPlayer::m_iFreeMilitaryUnitsPopulationPercent", m_syncArchive)
	, m_iGoldPerUnit("CvPlayer::m_iGoldPerUnit", m_syncArchive)
	, m_iGoldPerMilitaryUnit("CvPlayer::m_iGoldPerMilitaryUnit", m_syncArchive)
	, m_iRouteGoldMaintenanceMod("CvPlayer::m_iRouteGoldMaintenanceMod", m_syncArchive)
	, m_iBuildingGoldMaintenanceMod("CvPlayer::m_iBuildingGoldMaintenanceMod", m_syncArchive)
	, m_iUnitGoldMaintenanceMod("CvPlayer::m_iUnitGoldMaintenanceMod", m_syncArchive)
	, m_iUnitSupplyMod("CvPlayer::m_iUnitSupplyMod", m_syncArchive)
	, m_iExtraUnitCost("CvPlayer::m_iExtraUnitCost", m_syncArchive)
	, m_iNumMilitaryUnits("CvPlayer::m_iNumMilitaryUnits", m_syncArchive)
	, m_iHappyPerMilitaryUnit("CvPlayer::m_iHappyPerMilitaryUnit", m_syncArchive)
	, m_iHappinessToCulture("CvPlayer::m_iHappinessToCulture", m_syncArchive)
	, m_iHappinessToScience("CvPlayer::m_iHappinessToScience", m_syncArchive)
	, m_iHalfSpecialistUnhappinessCount("CvPlayer::m_iHalfSpecialistUnhappinessCount", m_syncArchive)
	, m_iHalfSpecialistFoodCount("CvPlayer::m_iHalfSpecialistFoodCount", m_syncArchive)
	, m_iMilitaryFoodProductionCount("CvPlayer::m_iMilitaryFoodProductionCount", m_syncArchive)
	, m_iGoldenAgeCultureBonusDisabledCount(0)
	, m_iSecondReligionPantheonCount(0)
	, m_iEnablesSSPartHurryCount(0)
	, m_iEnablesSSPartPurchaseCount(0)
	, m_iConscriptCount("CvPlayer::m_iConscriptCount", m_syncArchive)
	, m_iMaxConscript("CvPlayer::m_iMaxConscript", m_syncArchive)
	, m_iHighestUnitLevel("CvPlayer::m_iHighestUnitLevel", m_syncArchive)
	, m_iOverflowResearch("CvPlayer::m_iOverflowResearch", m_syncArchive, true)
	, m_iExpModifier("CvPlayer::m_iExpModifier", m_syncArchive)
	, m_iExpInBorderModifier("CvPlayer::m_iExpInBorderModifier", m_syncArchive)
	, m_iLevelExperienceModifier("CvPlayer::m_iLevelExperienceModifier", m_syncArchive)
	, m_iMinorQuestFriendshipMod("CvPlayer::m_iMinorQuestFriendshipMod", m_syncArchive)
	, m_iMinorGoldFriendshipMod("CvPlayer::m_iMinorGoldFriendshipMod", m_syncArchive)
	, m_iMinorFriendshipMinimum("CvPlayer::m_iMinorFriendshipMinimum", m_syncArchive)
	, m_iMinorFriendshipDecayMod("CvPlayer::m_iMinorFriendshipDecayMod", m_syncArchive)
	, m_iMinorScienceAlliesCount("CvPlayer::m_iMinorScienceAlliesCount", m_syncArchive)
	, m_iMinorResourceBonusCount("CvPlayer::m_iMinorResourceBonusCount", m_syncArchive)
	, m_iAbleToAnnexCityStatesCount(0)
	, m_iFreeSpecialist("CvPlayer::m_iFreeSpecialist", m_syncArchive)
	, m_iCultureBombTimer("CvPlayer::m_iCultureBombTimer", m_syncArchive)
	, m_iConversionTimer("CvPlayer::m_iConversionTimer", m_syncArchive)
	, m_iCapitalCityID("CvPlayer::m_iCapitalCityID", m_syncArchive)
	, m_iCitiesLost("CvPlayer::m_iCitiesLost", m_syncArchive)
	, m_iMilitaryMight("CvPlayer::m_iMilitaryMight", m_syncArchive)
	, m_iEconomicMight("CvPlayer::m_iEconomicMight", m_syncArchive)
	, m_iTurnMightRecomputed("CvPlayer::m_iTurnMightRecomputed", m_syncArchive)
	, m_iNewCityExtraPopulation("CvPlayer::m_iNewCityExtraPopulation", m_syncArchive)
	, m_iFreeFoodBox("CvPlayer::m_iFreeFoodBox", m_syncArchive)
	, m_iScenarioScore1("CvPlayer::m_iScenarioScore1", m_syncArchive)
	, m_iScenarioScore2("CvPlayer::m_iScenarioScore2", m_syncArchive)
	, m_iScenarioScore3("CvPlayer::m_iScenarioScore3", m_syncArchive)
	, m_iScenarioScore4("CvPlayer::m_iScenarioScore4", m_syncArchive)
	, m_iScoreFromFutureTech("CvPlayer::m_iScoreFromFutureTech", m_syncArchive)
	, m_iCombatExperience("CvPlayer::m_iCombatExperience", m_syncArchive)
	, m_iLifetimeCombatExperience(0)
	, m_iNavalCombatExperience(0)
	, m_iPopRushHurryCount("CvPlayer::m_iPopRushHurryCount", m_syncArchive)
	, m_iTotalImprovementsBuilt("CvPlayer::m_iTotalImprovementsBuilt", m_syncArchive)
	, m_iNextOperationID("CvPlayer::m_iNextOperationID", m_syncArchive)
	, m_iCostNextPolicy("CvPlayer::m_iCostNextPolicy", m_syncArchive)
	, m_iNumBuilders("CvPlayer::m_iNumBuilders", m_syncArchive, true)
	, m_iMaxNumBuilders("CvPlayer::m_iMaxNumBuilders", m_syncArchive)
	, m_iCityStrengthMod("CvPlayer::m_iCityStrengthMod", m_syncArchive)
	, m_iCityGrowthMod("CvPlayer::m_iCityGrowthMod", m_syncArchive)
	, m_iCapitalGrowthMod("CvPlayer::m_iCapitalGrowthMod", m_syncArchive)
	, m_iNumPlotsBought("CvPlayer::m_iNumPlotsBought", m_syncArchive)
	, m_iPlotGoldCostMod("CvPlayer::m_iPlotGoldCostMod", m_syncArchive)
	, m_iPlotCultureCostModifier("CvPlayer::m_iPlotCultureCostModifier", m_syncArchive)
	, m_iPlotCultureExponentModifier(0)
	, m_iNumCitiesPolicyCostDiscount(0)
	, m_iGarrisonedCityRangeStrikeModifier(0)
	, m_iGarrisonFreeMaintenanceCount(0)
#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
	, m_ppaaiBuildingScecialistCountChange("CvPlayer::m_ppaaiBuildingScecialistCountChange", m_syncArchive)
#endif
	, m_iNumCitiesFreeCultureBuilding(0)
	, m_iNumCitiesFreeFoodBuilding(0)
#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
	, m_iNumCitiesFreeDevensiveBuilding(0)
#endif
	, m_iUnitPurchaseCostModifier("CvPlayer::m_iUnitPurchaseCostModifier", m_syncArchive)
	, m_iAllFeatureProduction("CvPlayer::m_iAllFeatureProduction", m_syncArchive)
	, m_iCityDistanceHighwaterMark("CvPlayer::m_iCityDistanceHighwaterMark", m_syncArchive)
	, m_iOriginalCapitalX("CvPlayer::m_iOriginalCapitalX", m_syncArchive)
	, m_iOriginalCapitalY("CvPlayer::m_iOriginalCapitalY", m_syncArchive)
	, m_iNumWonders("CvPlayer::m_iNumWonders", m_syncArchive)
	, m_iNumPolicies("CvPlayer::m_iNumPolicies", m_syncArchive)
	, m_iNumGreatPeople("CvPlayer::m_iNumGreatPeople", m_syncArchive)
	, m_uiStartTime("CvPlayer::m_uiStartTime", m_syncArchive)  // XXX save these?
	, m_bHasBetrayedMinorCiv("CvPlayer::m_bHasBetrayedMinorCiv", m_syncArchive)
#ifdef CAN_BUILD_OU_AND_NIA_ONLY_ONCE
	, m_bOxfordUniversityWasEverBuilt("CvPlayer::m_bOxfordUniversityWasEverBuilt", m_syncArchive)
	, m_bNationalIntelligenceAgencyWasEverBuilt("CvPlayer::m_bNationalIntelligenceAgencyWasEverBuilt", m_syncArchive)
#endif
	, m_bAlive("CvPlayer::m_bAlive", m_syncArchive)
	, m_bEverAlive("CvPlayer::m_bEverAlive", m_syncArchive)
	, m_bBeingResurrected(false)
	, m_bTurnActive("CvPlayer::m_bTurnActive", m_syncArchive, false, true)
	, m_bAutoMoves("CvPlayer::m_bAutoMoves", m_syncArchive, false, true)
	, m_bEndTurn("CvPlayer::m_bEndTurn", m_syncArchive, false, true)
	, m_bDynamicTurnsSimultMode(true)
	, m_bPbemNewTurn("CvPlayer::m_bPbemNewTurn", m_syncArchive)
	, m_bExtendedGame("CvPlayer::m_bExtendedGame", m_syncArchive)
	, m_bFoundedFirstCity("CvPlayer::m_bFoundedFirstCity", m_syncArchive)
	, m_iNumCitiesFounded(0)
	, m_bStrike("CvPlayer::m_bStrike", m_syncArchive)
	, m_bCramped("CvPlayer::m_bCramped", m_syncArchive)
	, m_bLostCapital("CvPlayer::m_bLostCapital", m_syncArchive)
	, m_eConqueror(NO_PLAYER)
	, m_bHasAdoptedStateReligion("CvPlayer::m_bHasAdoptedStateReligion", m_syncArchive)
	, m_bAlliesGreatPersonBiasApplied("CvPlayer::m_bAlliesGreatPersonBiasApplied", m_syncArchive)
	, m_eID("CvPlayer::m_eID", m_syncArchive)
	, m_ePersonalityType("CvPlayer::m_ePersonalityType", m_syncArchive)
	, m_aiCityYieldChange("CvPlayer::m_aiCityYieldChange", m_syncArchive)
	, m_aiCoastalCityYieldChange("CvPlayer::m_aiCoastalCityYieldChange", m_syncArchive)
	, m_aiCapitalYieldChange("CvPlayer::m_aiCapitalYieldChange", m_syncArchive)
	, m_aiCapitalYieldPerPopChange("CvPlayer::m_aiCapitalYieldPerPopChange", m_syncArchive)
	, m_aiSeaPlotYield("CvPlayer::m_aiSeaPlotYield", m_syncArchive)
	, m_aiYieldRateModifier("CvPlayer::m_aiYieldRateModifier", m_syncArchive)
	, m_aiCapitalYieldRateModifier("CvPlayer::m_aiCapitalYieldRateModifier", m_syncArchive)
	, m_aiExtraYieldThreshold("CvPlayer::m_aiExtraYieldThreshold", m_syncArchive)
	, m_aiSpecialistExtraYield("CvPlayer::m_aiSpecialistExtraYield", m_syncArchive)
#ifdef POLICY_GOLDEN_AGE_YIELD_MOD
	, m_aiGoldenAgeYieldModifier("CvPlayer::m_aiGoldenAgeYieldModifier", m_syncArchive)
#endif
#ifdef POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
	, m_paiPlotExtraYieldFromTradeRoute("CvPlayer::m_paiPlotExtraYieldFromTradeRoute", m_syncArchive)
#endif
	, m_aiProximityToPlayer("CvPlayer::m_aiProximityToPlayer", m_syncArchive, true)
	, m_aiResearchAgreementCounter("CvPlayer::m_aiResearchAgreementCounter", m_syncArchive)
	, m_aiIncomingUnitTypes("CvPlayer::m_aiIncomingUnitTypes", m_syncArchive, true)
	, m_aiIncomingUnitCountdowns("CvPlayer::m_aiIncomingUnitCountdowns", m_syncArchive, true)
	, m_aiMinorFriendshipAnchors("CvPlayer::m_aiMinorFriendshipAnchors", m_syncArchive, true)
	, m_aOptions("CvPlayer::m_aOptions", m_syncArchive, true)
	, m_strReligionKey("CvPlayer::m_strReligionKey", m_syncArchive)
	, m_strScriptData("CvPlayer::m_strScriptData", m_syncArchive)
	, m_paiNumResourceUsed("CvPlayer::m_paiNumResourceUsed", m_syncArchive)
	, m_paiNumResourceTotal("CvPlayer::m_paiNumResourceTotal", m_syncArchive)
	, m_paiResourceGiftedToMinors("CvPlayer::m_paiResourceGiftedToMinors", m_syncArchive)
	, m_paiResourceExport("CvPlayer::m_paiResourceExport", m_syncArchive)
	, m_paiResourceImport("CvPlayer::m_paiResourceImport", m_syncArchive)
	, m_paiResourceFromMinors("CvPlayer::m_paiResourceFromMinors", m_syncArchive)
	, m_paiResourcesSiphoned("CvPlayer::m_paiResourcesSiphoned", m_syncArchive)
	, m_paiImprovementCount("CvPlayer::m_paiImprovementCount", m_syncArchive)
	, m_paiFreeBuildingCount("CvPlayer::m_paiFreeBuildingCount", m_syncArchive)
	, m_paiFreePromotionCount("CvPlayer::m_paiFreePromotionCount", m_syncArchive)
	, m_paiUnitCombatProductionModifiers("CvPlayer::m_paiUnitCombatProductionModifiers", m_syncArchive)
	, m_paiUnitCombatFreeExperiences("CvPlayer::m_paiUnitCombatFreeExperiences", m_syncArchive)
	, m_paiUnitClassCount("CvPlayer::m_paiUnitClassCount", m_syncArchive, true)
	, m_paiUnitClassMaking("CvPlayer::m_paiUnitClassMaking", m_syncArchive, true)
	, m_paiBuildingClassCount("CvPlayer::m_paiBuildingClassCount", m_syncArchive)
	, m_paiBuildingClassMaking("CvPlayer::m_paiBuildingClassMaking", m_syncArchive, true)
	, m_paiProjectMaking("CvPlayer::m_paiProjectMaking", m_syncArchive)
	, m_paiHurryCount("CvPlayer::m_paiHurryCount", m_syncArchive)
	, m_paiHurryModifier("CvPlayer::m_paiHurryModifier", m_syncArchive)
	, m_pabLoyalMember("CvPlayer::m_pabLoyalMember", m_syncArchive)
	, m_pabGetsScienceFromPlayer("CvPlayer::m_pabGetsScienceFromPlayer", m_syncArchive)
	, m_ppaaiSpecialistExtraYield("CvPlayer::m_ppaaiSpecialistExtraYield", m_syncArchive)
	, m_ppaaiImprovementYieldChange("CvPlayer::m_ppaaiImprovementYieldChange", m_syncArchive)
	, m_ppaaiBuildingClassYieldMod("CvPlayer::m_ppaaiBuildingClassYieldMod", m_syncArchive)
	, m_UnitCycle(this)
	, m_bEverPoppedGoody("CvPlayer::m_bEverPoppedGoody", m_syncArchive)
	, m_bEverTrainedBuilder("CvPlayer::m_bEverTrainedBuilder", m_syncArchive)
	, m_iCityConnectionHappiness("CvPlayer::m_iCityConnectionHappiness", m_syncArchive)
	, m_iHolyCityID("CvPlayer::m_iHolyCityID", m_syncArchive)
	, m_iTurnsSinceSettledLastCity("CvPlayer::m_iTurnsSinceSettledLastCity", m_syncArchive)
	, m_iNumNaturalWondersDiscoveredInArea("CvPlayer::m_iNumNaturalWondersDiscoveredInArea", m_syncArchive)
	, m_iStrategicResourceMod("CvPlayer::m_iStrategicResourceMod", m_syncArchive)
	, m_iSpecialistCultureChange("CvPlayer::m_iSpecialistCultureChange", m_syncArchive)
	, m_iGreatPeopleSpawnCounter("CvPlayer::m_iGreatPeopleSpawnCounter", m_syncArchive)
	, m_iFreeTechCount("CvPlayer::m_iFreeTechCount", m_syncArchive, true)
	, m_iMedianTechPercentage(50)
	, m_iNumFreePolicies("CvPlayer::m_iNumFreePolicies", m_syncArchive)
	, m_iNumFreePoliciesEver("CvPlayer::m_iNumFreePoliciesEver", m_syncArchive)
	, m_iNumFreeTenets(0)
	, m_iMaxEffectiveCities(1)
	, m_iLastSliceMoved(0)
	, m_eEndTurnBlockingType(NO_ENDTURN_BLOCKING_TYPE)
	, m_iEndTurnBlockingNotificationIndex(0)
	, m_activeWaitingForEndTurnMessage(false)
	, m_endTurnBusyUnitUpdatesLeft(0)
	, m_lastGameTurnInitialAIProcessed(-1)
	, m_iNumFreeGreatPeople(0)
	, m_iNumMayaBoosts(0)
	, m_iNumFaithGreatPeople(0)
	, m_iNumArchaeologyChoices(0)
	, m_eFaithPurchaseType(NO_AUTOMATIC_FAITH_PURCHASE)
	, m_iFaithPurchaseIndex(0)
	, m_bProcessedAutoMoves(false)
	, m_kPlayerAchievements(*this)
#ifdef CS_ALLYING_WAR_RESCTRICTION
	, m_ppaaiTurnCSWarAllowing("CvPlayer::m_ppaaiTurnCSWarAllowing", m_syncArchive)
	, m_ppaafTimeCSWarAllowing("CvPlayer::m_ppaafTimeCSWarAllowing", m_syncArchive)
#endif
#ifdef PENALTY_FOR_DELAYING_POLICIES
	, m_bIsDelayedPolicy(false)
#endif
#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
	, m_ppaaiYieldForEachBuildingInEmpire("CvPlayer::m_ppaaiYieldForEachBuildingInEmpire", m_syncArchive)
#endif
#ifdef POLICY_ALLOWS_GP_BUYS_FOR_GOLD
	, m_iNumGoldPurchasedGreatPerson(0)
	, m_bGoldWriter(false)
	, m_bGoldArtist(false)
	, m_bGoldMusician(false)
	, m_bGoldScientist(false)
	, m_bGoldEngineer(false)
	, m_bGoldMerchant(false)
	, m_bGoldGeneral(false)
	, m_bGoldAdmiral(false)
#endif
#ifdef POLICY_SPY_DETECTION
	, m_iSpyDetection(0)
#endif
#ifdef BUILDING_BORDER_TRANSITION_OBSTACLE
	, m_iBorderObstacleCount(0)
#endif
{
	m_pPlayerPolicies = FNEW(CvPlayerPolicies, c_eCiv5GameplayDLL, 0);
	m_pEconomicAI = FNEW(CvEconomicAI, c_eCiv5GameplayDLL, 0);
	m_pMilitaryAI = FNEW(CvMilitaryAI, c_eCiv5GameplayDLL, 0);
	m_pCitySpecializationAI = FNEW(CvCitySpecializationAI, c_eCiv5GameplayDLL, 0);
	m_pWonderProductionAI = FNEW(CvWonderProductionAI(this, GC.GetGameBuildings()), c_eCiv5GameplayDLL, 0);
	m_pGrandStrategyAI = FNEW(CvGrandStrategyAI, c_eCiv5GameplayDLL, 0);
	m_pDiplomacyAI = FNEW(CvDiplomacyAI, c_eCiv5GameplayDLL, 0);
	m_pReligions = FNEW(CvPlayerReligions, c_eCiv5GameplayDLL, 0);
	m_pReligionAI = FNEW(CvReligionAI, c_eCiv5GameplayDLL, 0);
	m_pPlayerTechs = FNEW(CvPlayerTechs, c_eCiv5GameplayDLL, 0);
	m_pFlavorManager = FNEW(CvFlavorManager, c_eCiv5GameplayDLL, 0);
	m_pTacticalAI = FNEW(CvTacticalAI, c_eCiv5GameplayDLL, 0);
	m_pHomelandAI = FNEW(CvHomelandAI, c_eCiv5GameplayDLL, 0);
	m_pMinorCivAI = FNEW(CvMinorCivAI, c_eCiv5GameplayDLL, 0);
	m_pDealAI = FNEW(CvDealAI, c_eCiv5GameplayDLL, 0);
	m_pBuilderTaskingAI = FNEW(CvBuilderTaskingAI, c_eCiv5GameplayDLL, 0);
	m_pDangerPlots = FNEW(CvDangerPlots, c_eCiv5GameplayDLL, 0);
	m_pCityConnections = FNEW(CvCityConnections, c_eCiv5GameplayDLL, 0);
	m_pTreasury = FNEW(CvTreasury, c_eCiv5GameplayDLL, 0);
	m_pTraits = FNEW(CvPlayerTraits, c_eCiv5GameplayDLL, 0);
	m_pEspionage = FNEW(CvPlayerEspionage, c_eCiv5GameplayDLL, 0);
	m_pEspionageAI = FNEW(CvEspionageAI, c_eCiv5GameplayDLL, 0);
	m_pTrade = FNEW(CvPlayerTrade, c_eCiv5GameplayDLL, 0);
	m_pTradeAI = FNEW(CvTradeAI, c_eCiv5GameplayDLL, 0);
	m_pLeagueAI = FNEW(CvLeagueAI, c_eCiv5GameplayDLL, 0);
	m_pCulture = FNEW(CvPlayerCulture, c_eCiv5GameplayDLL, 0);

	m_pNotifications = NULL;
	m_pDiplomacyRequests = NULL;

	m_iNextOperationID = 0;

	m_aiPlots.clear();
	m_bfEverConqueredBy.ClearAll();

	m_aiGreatWorkYieldChange.clear();
	m_aiSiphonLuxuryCount.clear();
#ifdef FIX_DATASETS_REINITIALIZATION
	m_ReplayDataSets.clear();
	m_ReplayDataSetValues.clear();
#endif

	reset(NO_PLAYER, true);
}


//	--------------------------------------------------------------------------------
CvPlayer::~CvPlayer()
{
	uninit();

	SAFE_DELETE(m_pDangerPlots);
	delete m_pPlayerPolicies;
	delete m_pEconomicAI;
	delete m_pMilitaryAI;
	delete m_pCitySpecializationAI;
	delete m_pWonderProductionAI;
	delete m_pGrandStrategyAI;
	delete m_pDiplomacyAI;
	delete m_pReligions;
	delete m_pReligionAI;
	delete m_pPlayerTechs;
	delete m_pFlavorManager;
	delete m_pTacticalAI;
	delete m_pHomelandAI;
	delete m_pMinorCivAI;
	delete m_pDealAI;
	delete m_pBuilderTaskingAI;
	SAFE_DELETE(m_pCityConnections);
	SAFE_DELETE(m_pNotifications);
	SAFE_DELETE(m_pDiplomacyRequests);
	SAFE_DELETE(m_pTreasury);
	SAFE_DELETE(m_pTraits);
	SAFE_DELETE(m_pEspionage);
	SAFE_DELETE(m_pEspionageAI);
	SAFE_DELETE(m_pTrade);
	SAFE_DELETE(m_pTradeAI);
	SAFE_DELETE(m_pLeagueAI);
}


//	--------------------------------------------------------------------------------
void CvPlayer::init(PlayerTypes eID)
{
	LeaderHeadTypes eBestPersonality;
	int iValue;
	int iBestValue;
	int iI, iJ;

	// only allocate notifications for civs that players can play as
	if(eID < MAX_MAJOR_CIVS)
	{
		m_pNotifications = FNEW(CvNotifications, c_eCiv5GameplayDLL, 0);
		m_pDiplomacyRequests = FNEW(CvDiplomacyRequests, c_eCiv5GameplayDLL, 0);
	}

	//--------------------------------
	// Init saved data
	reset(eID);

	//--------------------------------
	// Init containers
	m_cities.Init();

	m_units.Init();

	m_armyAIs.Init();

	m_AIOperations.clear();

	//--------------------------------
	// Init non-saved data
	setupGraphical();

	//--------------------------------
	// Init other game data
	CvAssert(getTeam() != NO_TEAM);
	GET_TEAM(getTeam()).changeNumMembers(1);
	PlayerTypes p = GetID();
	SlotStatus s = CvPreGame::slotStatus(p);
	if((s == SS_TAKEN) || (s == SS_COMPUTER))
	{
		setAlive(true);

		if(GC.getGame().isOption(GAMEOPTION_RANDOM_PERSONALITIES))
		{
			if(!isBarbarian() && !isMinorCiv())
			{
				iBestValue = 0;
				eBestPersonality = NO_LEADER;

				for(iI = 0; iI < GC.getNumLeaderHeadInfos(); iI++)
				{
					if(iI != GC.getBARBARIAN_LEADER() && iI != GC.getMINOR_CIVILIZATION())
					{
						iValue = (1 + GC.getGame().getJonRandNum(10000, "Choosing Personality"));

						for(iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
						{
							if(GET_PLAYER((PlayerTypes)iJ).isAlive())
							{
								if(GET_PLAYER((PlayerTypes)iJ).getPersonalityType() == ((LeaderHeadTypes)iI))
								{
									iValue /= 2;
								}
							}
						}

						if(iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestPersonality = ((LeaderHeadTypes)iI);
						}
					}
				}

				if(eBestPersonality != NO_LEADER)
				{
					setPersonalityType(eBestPersonality);
				}
			}
		}

		CvAssert(m_pTraits);
		m_pTraits->InitPlayerTraits();

		// Special handling for the Polynesian trait's overriding of embarked unit graphics
		if(m_pTraits->IsEmbarkedAllWater())
		{
			SetEmbarkedGraphicOverride("ART_DEF_UNIT_U_POLYNESIAN_WAR_CANOE");
		}
		else if(m_pTraits->IsEmbarkedToLandFlatCost())
		{
			SetEmbarkedGraphicOverride("ART_DEF_UNIT_U_DANISH_LONGBOAT");
		}

		changeGoldPerUnitTimes100(GC.getINITIAL_GOLD_PER_UNIT_TIMES_100());

		ChangeMaxNumBuilders(GC.getDEFAULT_MAX_NUM_BUILDERS());

		changeLevelExperienceModifier(GetPlayerTraits()->GetLevelExperienceModifier());
		changeMaxGlobalBuildingProductionModifier(GetPlayerTraits()->GetMaxGlobalBuildingProductionModifier());
		changeMaxTeamBuildingProductionModifier(GetPlayerTraits()->GetMaxTeamBuildingProductionModifier());
		changeMaxPlayerBuildingProductionModifier(GetPlayerTraits()->GetMaxPlayerBuildingProductionModifier());
		ChangePlotGoldCostMod(GetPlayerTraits()->GetPlotBuyCostModifier());
		ChangePlotCultureCostModifier(GetPlayerTraits()->GetPlotCultureCostModifier());
		GetTreasury()->ChangeCityConnectionTradeRouteGoldChange(GetPlayerTraits()->GetCityConnectionTradeRouteChange());
		changeWonderProductionModifier(GetPlayerTraits()->GetWonderProductionModifier());
		ChangeRouteGoldMaintenanceMod(GetPlayerTraits()->GetImprovementMaintenanceModifier());

		for(iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
			ChangeCityYieldChange((YieldTypes)iJ, 100 * GetPlayerTraits()->GetFreeCityYield((YieldTypes)iJ));
			changeYieldRateModifier((YieldTypes)iJ, GetPlayerTraits()->GetYieldRateModifier((YieldTypes)iJ));
		}

		recomputeGreatPeopleModifiers();

		for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			updateExtraYieldThreshold((YieldTypes)iI);
		}

		CvCivilizationInfo& playerCivilizationInfo = getCivilizationInfo();
		for(iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
		{
			const UnitClassTypes eUnitClass = static_cast<UnitClassTypes>(iI);
			CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eUnitClass);
			if(pkUnitClassInfo != NULL)
			{
				const UnitTypes eUnit = ((UnitTypes)(playerCivilizationInfo.getCivilizationUnits(iI)));
				if(NO_UNIT != eUnit)
				{
					CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
					if(NULL != pkUnitInfo && pkUnitInfo->IsFound())
					{
						setUnitExtraCost(eUnitClass, getNewCityProductionValue());
					}
				}
			}
		}

		BuildingTypes eFreeBuilding = GetPlayerTraits()->GetFreeBuilding();
		if(eFreeBuilding != NO_BUILDING)
		{
			changeFreeBuildingCount(eFreeBuilding, 1);
		}

		SetGreatGeneralCombatBonus(GC.getGREAT_GENERAL_STRENGTH_MOD());
	}

	m_aiPlots.clear();
	m_bfEverConqueredBy.ClearAll();

	AI_init();
}


//	--------------------------------------------------------------------------------
void CvPlayer::uninit()
{
	m_paiNumResourceUsed.clear();
	m_paiNumResourceTotal.clear();
	m_paiResourceGiftedToMinors.clear();
	m_paiResourceExport.clear();
	m_paiResourceImport.clear();
	m_paiResourceFromMinors.clear();
	m_paiResourcesSiphoned.clear();
	m_paiImprovementCount.clear();
	m_paiFreeBuildingCount.clear();
	m_paiFreePromotionCount.clear();
	m_paiUnitCombatProductionModifiers.clear();
	m_paiUnitCombatFreeExperiences.clear();
	m_paiUnitClassCount.clear();
	m_paiUnitClassMaking.clear();
	m_paiBuildingClassCount.clear();
	m_paiBuildingClassMaking.clear();
	m_paiProjectMaking.clear();
	m_paiHurryCount.clear();
	m_paiHurryModifier.clear();

	m_pabLoyalMember.clear();
	m_pabGetsScienceFromPlayer.clear();

#ifdef CS_ALLYING_WAR_RESCTRICTION
	m_ppaaiTurnCSWarAllowing.clear();
	m_ppaafTimeCSWarAllowing.clear();
#endif

#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
	m_ppaaiYieldForEachBuildingInEmpire.clear();
#endif

	m_pPlayerPolicies->Uninit();
	m_pEconomicAI->Uninit();
	m_pMilitaryAI->Uninit();
	m_pCitySpecializationAI->Uninit();
	m_pWonderProductionAI->Uninit();
	m_pGrandStrategyAI->Uninit();
	m_pDiplomacyAI->Uninit();
	m_pReligions->Uninit();
	m_pReligionAI->Uninit();
	m_pEspionage->Uninit();
	m_pEspionageAI->Uninit();
	m_pTrade->Uninit();
	m_pTradeAI->Uninit();
	m_pLeagueAI->Uninit();
	m_pPlayerTechs->Uninit();
	m_pFlavorManager->Uninit();
	m_pTacticalAI->Uninit();
	m_pHomelandAI->Uninit();
	m_pMinorCivAI->Uninit();
	m_pDealAI->Uninit();
	m_pBuilderTaskingAI->Uninit();
	m_pCityConnections->Uninit();
	if(m_pNotifications)
	{
		m_pNotifications->Uninit();
	}
	if(m_pDiplomacyRequests)
	{
		m_pDiplomacyRequests->Uninit();
	}
	m_pTreasury->Uninit();
	m_pTraits->Uninit();

	if(m_pDangerPlots)
	{
		m_pDangerPlots->Uninit();
	}

	m_ppaaiSpecialistExtraYield.clear();
	m_ppaaiImprovementYieldChange.clear();
	m_ppaaiBuildingClassYieldMod.clear();

	m_UnitCycle.Clear();

	m_researchQueue.clear();

	m_cityNames.clear();

	m_cities.Uninit();

	m_units.Uninit();

	// loop through all entries freeing them up
	std::map<int , CvAIOperation*>::iterator iter;
	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		delete(iter->second);
	}
	m_AIOperations.clear();

	m_aiPlots.clear();
	m_bfEverConqueredBy.ClearAll();

	FAutoArchive& archive = getSyncArchive();
	archive.clearDelta();

#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
	m_bIsDisconnected = false;
#endif
	m_iStartingX = INVALID_PLOT_COORD;
	m_iStartingY = INVALID_PLOT_COORD;
	m_iTotalPopulation = 0;
	m_iTotalLand = 0;
	m_iTotalLandScored = 0;
	m_iCityConnectionHappiness = 0;
	m_iJONSCulturePerTurnForFree = 0;
	m_iJONSCulturePerTurnFromMinorCivs = 0;
	m_iJONSCultureCityModifier = 0;
	m_iJONSCulture = 0;
	m_iJONSCultureEverGenerated = 0;
	m_iCulturePerWonder = 0;
	m_iCultureWonderMultiplier = 0;
	m_iCulturePerTechResearched = 0;
	m_iFaith = 0;
	m_iFaithEverGenerated = 0;
	m_iHappiness = 0;
	m_iUprisingCounter = 0;
	m_iExtraHappinessPerLuxury = 0;
	m_iUnhappinessFromUnits = 0;
	m_iUnhappinessFromUnitsMod = 0;
	m_iUnhappinessMod = 0;
	m_iCityCountUnhappinessMod = 0;
	m_iOccupiedPopulationUnhappinessMod = 0;
	m_iCapitalUnhappinessMod = 0;
	m_iCityRevoltCounter = 0;
	m_iHappinessPerGarrisonedUnitCount = 0;
	m_iHappinessPerTradeRouteCount = 0;
	m_iHappinessPerXPopulation = 0;
	m_iHappinessFromLeagues = 0;
	m_iEspionageModifier = 0;
	m_iSpyStartingRank = 0;
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
	m_iNumStolenScience = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
	m_iNumTrainedUnits = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
	m_iNumKilledUnits = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
	m_iNumLostUnits = 0;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
	m_iUnitsDamageDealt = 0;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
	m_iUnitsDamageTaken = 0;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
	m_iCitiesDamageDealt = 0;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
	m_iCitiesDamageTaken = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
	m_iNumScientistsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
	m_iNumEngineersTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
	m_iNumMerchantsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
	m_iNumWritersTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
	m_iNumArtistsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
	m_iNumMusiciansTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
	m_iNumGeneralsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
	m_iNumAdmiralsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
	m_iNumProphetsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
	m_iProductionGoldFromWonders = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
	m_iNumChops = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
	m_iNumTimesOpenedDemographics = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
	m_bMayaBoostScientist = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
	m_bMayaBoostEngineers = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
	m_bMayaBoostMerchants = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
	m_bMayaBoostWriters = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
	m_bMayaBoostArtists = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
	m_bMayaBoostMusicians = 0;
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
	m_iScientistsTotalScienceBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
	m_iEngineersTotalHurryBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
	m_iMerchantsTotalTradeBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
	m_iWritersTotalCultureBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
	m_iMusiciansTotalTourismBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
	m_iNumPopulationLostFromNukes = 0;
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
	m_iNumCSQuestsCompleted = 0;
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
	m_iNumAlliedCS = 0;
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
	m_iTimesEnteredCityScreen = 0;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
	m_iNumDiedSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
	m_iNumKilledSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
	m_iFoodFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
	m_iProductionFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
	m_iNumUnitsFromMinors = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
	m_iNumCreatedWorldWonders = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
	m_iNumGoldSpentOnBuildingBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
	m_iNumGoldSpentOnUnitBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
	m_iNumGoldSpentOnUgrades = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
	m_iGoldFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
	m_iCultureFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
	m_iNumGoldSpentOnGPBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
	m_iNumGoldSpentOnTilesBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
	m_iNumGoldFromPillage = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
	m_iNumGoldFromPlunder = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
	m_iNumFaithSpentOnMilitaryUnits = 0;
#endif
	m_iExtraLeagueVotes = 0;
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
	m_iMaxExtraVotesFromMinors = 0;
#endif
#ifdef POLICY_EXTRA_VOTES
	m_iPolicyExtraVotes = 0;
#endif
#ifdef POLICY_DO_TECH_FROM_CITY_CONQ
	m_iPolicyTechFromCityConquer = 0;
#endif
#ifdef POLICY_NO_CULTURE_SPECIALIST_FOOD
	m_iNoCultureSpecialistFood = 0;
#endif
#ifdef POLICY_MINORS_GIFT_UNITS
	m_iMinorsGiftUnits = 0;
#endif
#ifdef POLICY_NO_CARGO_PILLAGE
	m_iNoCargoPillage = 0;
#endif
#ifdef POLICY_GREAT_WORK_HAPPINESS
	m_iGreatWorkHappiness = 0;
#endif
#ifdef POLICY_SCIENCE_PER_X_FOLLOWERS
	m_iSciencePerXFollowers = 0;
#endif
#ifdef POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD
	m_iNoDifferentIdeologiesTourismMod = 0;
#endif
#ifdef POLICY_GLOBAL_POP_CHANGE
	m_iGlobalPopChange = 0;
#endif
#ifdef POLICY_GREAT_WORK_TOURISM_CHANGES
	m_iGreatWorkTourismChanges = 0;
#endif
#ifdef POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP
	m_iCityScienceSquaredModPerXPop = 0;
#endif
	m_iSpecialPolicyBuildingHappiness = 0;
	m_iWoundedUnitDamageMod = 0;
	m_iUnitUpgradeCostMod = 0;
	m_iBarbarianCombatBonus = 0;
	m_iAlwaysSeeBarbCampsCount = 0;
	m_iHappinessFromBuildings = 0;
	m_iHappinessPerCity = 0;
	m_iHappinessPerXPolicies = 0;
	m_iAdvancedStartPoints = -1;
	m_iAttackBonusTurns = 0;
	m_iCultureBonusTurns = 0;
	m_iTourismBonusTurns = 0;
	m_iGoldenAgeProgressMeter = 0;
	m_iGoldenAgeMeterMod = 0;
	m_iNumGoldenAges = 0;
	m_iGoldenAgeTurns = 0;
#ifdef TAJ_MAHAL_STARTS_GA_NEXT_TURN
	m_iBuildingGoldenAgeTurns = 0;
#endif
	m_iNumUnitGoldenAges = 0;
	m_iStrikeTurns = 0;
	m_iGoldenAgeModifier = 0;
	m_iGreatPeopleCreated = 0;
	m_iGreatGeneralsCreated = 0;
	m_iGreatAdmiralsCreated = 0;
	m_iGreatWritersCreated = 0;
	m_iGreatArtistsCreated = 0;
	m_iGreatMusiciansCreated = 0;
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	m_bHasUsedDharma = false;
#endif
#ifdef UNDERGROUND_SECT_REWORK
	m_bHasUsedUndergroundSect = false;
#endif
#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
	m_bHasUsedMissionaryZeal = false;
#endif
#ifdef UNITY_OF_PROPHETS_EXTRA_PROPHETS
	m_bHasUsedUnityProphets = false;
#endif
#ifdef GODDESS_LOVE_FREE_WORKER
	m_bHasUsedGoddessLove = false;
#endif
#ifdef GOD_SEA_FREE_WORK_BOAT
	m_bHasUsedGodSea = false;
#endif
#ifdef FREE_GREAT_PERSON
	m_iGreatProphetsCreated = 0;
#endif
#if defined SEPARATE_GREAT_PEOPLE || defined SWEDEN_UA_REWORK
	m_iGreatScientistsCreated = 0;
	m_iGreatEngineersCreated = 0;
	m_iGreatMerchantsCreated = 0;
#endif
#ifdef SEPARATE_MERCHANTS
	m_iGreatMerchantsCreated = 0;
#endif
	m_iMerchantsFromFaith = 0;
	m_iScientistsFromFaith = 0;
	m_iWritersFromFaith = 0;
	m_iArtistsFromFaith = 0;
	m_iMusiciansFromFaith = 0;
	m_iGeneralsFromFaith = 0;
	m_iAdmiralsFromFaith = 0;
	m_iEngineersFromFaith = 0;
#ifdef BELIEF_TO_GLORY_OF_GOD_ONE_GP_OF_EACH_TYPE
	m_bMerchantsFromFaith = false;
	m_bScientistsFromFaith = false;
	m_bWritersFromFaith = false;
	m_bArtistsFromFaith = false;
	m_bMusiciansFromFaith = false;
	m_bGeneralsFromFaith = false;
	m_bAdmiralsFromFaith = false;
	m_bEngineersFromFaith = false;
#endif
	m_iGreatPeopleThresholdModifier = 0;
	m_iGreatGeneralsThresholdModifier = 0;
	m_iGreatAdmiralsThresholdModifier = 0;
	m_iGreatGeneralCombatBonus = 0;
	m_iAnarchyNumTurns = 0;
	m_iPolicyCostModifier = 0;
	m_iGreatPeopleRateModifier = 0;
	m_iGreatPeopleRateModFromBldgs = 0;
	m_iGreatGeneralRateModifier = 0;
	m_iGreatGeneralRateModFromBldgs = 0;
	m_iDomesticGreatGeneralRateModifier = 0;
	m_iDomesticGreatGeneralRateModFromBldgs = 0;
	m_iGreatAdmiralRateModifier = 0;
	m_iGreatWriterRateModifier = 0;
	m_iGreatArtistRateModifier = 0;
	m_iGreatMusicianRateModifier = 0;
	m_iGreatMerchantRateModifier = 0;
	m_iGreatScientistRateModifier = 0;
	m_iGreatScientistBeakerModifier = 0;
	m_iGreatEngineerRateModifier = 0;
	m_iGreatPersonExpendGold = 0;
	m_iMaxGlobalBuildingProductionModifier = 0;
	m_iMaxTeamBuildingProductionModifier = 0;
	m_iMaxPlayerBuildingProductionModifier = 0;
	m_iFreeExperience = 0;
	m_iFreeExperienceFromBldgs = 0;
	m_iFreeExperienceFromMinors = 0;
	m_iFeatureProductionModifier = 0;
	m_iWorkerSpeedModifier = 0;
	m_iImprovementCostModifier = 0;
	m_iImprovementUpgradeRateModifier = 0;
	m_iSpecialistProductionModifier = 0;
	m_iMilitaryProductionModifier = 0;
	m_iSpaceProductionModifier = 0;
	m_iCityDefenseModifier = 0;
	m_iUnitFortificationModifier = 0;
	m_iUnitBaseHealModifier = 0;
	m_iWonderProductionModifier = 0;
	m_iSettlerProductionModifier = 0;
	m_iCapitalSettlerProductionModifier = 0;
	m_iUnitProductionMaintenanceMod = 0;
	m_iPolicyCostBuildingModifier = 0;
	m_iPolicyCostMinorCivModifier = 0;
	m_iInfluenceSpreadModifier = 0;
	m_iExtraVotesPerDiplomat = 0;
	m_iNumNukeUnits = 0;
	m_iNumOutsideUnits = 0;
	m_iBaseFreeUnits = 0;
	m_iBaseFreeMilitaryUnits = 0;
	m_iFreeUnitsPopulationPercent = 0;
	m_iFreeMilitaryUnitsPopulationPercent = 0;
	m_iGoldPerUnit = 0;
	m_iGoldPerMilitaryUnit = 0;
	m_iRouteGoldMaintenanceMod = 0;
	m_iBuildingGoldMaintenanceMod = 0;
	m_iUnitGoldMaintenanceMod = 0;
	m_iUnitSupplyMod = 0;
	m_iExtraUnitCost = 0;
	m_iNumMilitaryUnits = 0;
	m_iHappyPerMilitaryUnit = 0;
	m_iHappinessToCulture = 0;
	m_iHappinessToScience = 0;
	m_iHalfSpecialistUnhappinessCount = 0;
	m_iHalfSpecialistFoodCount = 0;
	m_iMilitaryFoodProductionCount = 0;
	m_iGoldenAgeCultureBonusDisabledCount = 0;
	m_iSecondReligionPantheonCount = 0;
	m_iEnablesSSPartHurryCount = 0;
	m_iEnablesSSPartPurchaseCount = 0;
	m_iConscriptCount = 0;
	m_iMaxConscript = 0;
	m_iHighestUnitLevel = 1;
	m_iOverflowResearch = 0;
	m_iExpModifier = 0;
	m_iExpInBorderModifier = 0;
	m_iLevelExperienceModifier = 0;
	m_iMinorQuestFriendshipMod = 0;
	m_iMinorGoldFriendshipMod = 0;
	m_iMinorFriendshipMinimum = 0;
	m_iMinorFriendshipDecayMod = 0;
	m_iMinorScienceAlliesCount = 0;
	m_iMinorResourceBonusCount = 0;
	m_iAbleToAnnexCityStatesCount = 0;
	m_iCultureBombTimer = 0;
	m_iConversionTimer = 0;
	m_iCapitalCityID = FFreeList::INVALID_INDEX;
	m_iCitiesLost = 0;
	m_iMilitaryMight = 0;
	m_iEconomicMight = 0;
	m_iTurnMightRecomputed = -1;
	m_iNewCityExtraPopulation = 0;
	m_iFreeFoodBox = 0;
	m_iScenarioScore1 = 0;
	m_iScenarioScore2 = 0;
	m_iScenarioScore3 = 0;
	m_iScenarioScore4 = 0;
	m_iScoreFromFutureTech = 0;
	m_iCombatExperience = 0;
	m_iLifetimeCombatExperience = 0;
	m_iNavalCombatExperience = 0;
	m_iBorderObstacleCount = 0;
	m_iPopRushHurryCount = 0;
	m_uiStartTime = 0;
	m_iTotalImprovementsBuilt = 0;
	m_iNextOperationID = 0;
	m_iCostNextPolicy = 0;
	m_iNumBuilders = 0;
	m_iMaxNumBuilders = 0;
	m_iCityStrengthMod = 0;
	m_iCityGrowthMod = 0;
	m_iCapitalGrowthMod = 0;
	m_iNumPlotsBought = 0;
	m_iPlotGoldCostMod = 0;
	m_iPlotCultureCostModifier = 0;
	m_iPlotCultureExponentModifier = 0;
	m_iNumCitiesPolicyCostDiscount = 0;
	m_iGarrisonedCityRangeStrikeModifier = 0;
	m_iGarrisonFreeMaintenanceCount = 0;
	m_iNumCitiesFreeCultureBuilding = 0;
#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
	m_ppaaiBuildingScecialistCountChange.clear();
#endif
	m_iNumCitiesFreeFoodBuilding = 0;
#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
	m_iNumCitiesFreeDevensiveBuilding = 0;
#endif
	m_iUnitPurchaseCostModifier = 0;
	m_iAllFeatureProduction = 0;
	m_iCityDistanceHighwaterMark = 1;
	m_iOriginalCapitalX = -1;
	m_iOriginalCapitalY = -1;
	m_iNumWonders = 0;
	m_iNumPolicies = 0;
	m_iNumGreatPeople = 0;
	m_iHolyCityID = -1;
	m_iTurnsSinceSettledLastCity = -1;
	m_iNumNaturalWondersDiscoveredInArea = 0;
	m_iStrategicResourceMod = 0;
	m_iSpecialistCultureChange = 0;
	m_iGreatPeopleSpawnCounter = 0;
	m_iFreeTechCount = 0;
	m_iMedianTechPercentage = 50;
	m_iNumFreePolicies = 0;
	m_iNumFreePoliciesEver = 0;
	m_iNumFreeTenets = 0;
	m_iNumFreeGreatPeople = 0;
	m_iNumMayaBoosts = 0;
	m_iNumFaithGreatPeople = 0;
	m_iNumArchaeologyChoices = 0;
	m_eFaithPurchaseType = NO_AUTOMATIC_FAITH_PURCHASE;
	m_iFaithPurchaseIndex = 0;
	m_iMaxEffectiveCities = 1;
	m_iLastSliceMoved = 0;
#ifdef PENALTY_FOR_DELAYING_POLICIES
	m_bIsDelayedPolicy = false;
#endif

	m_bHasBetrayedMinorCiv = false;
#ifdef CAN_BUILD_OU_AND_NIA_ONLY_ONCE
	m_bOxfordUniversityWasEverBuilt = false;
	m_bNationalIntelligenceAgencyWasEverBuilt = false;
#endif
	m_bAlive = false;
	m_bEverAlive = false;
	m_bBeingResurrected = false;
	m_bTurnActive = false;
	m_bAutoMoves = false;
	m_bProcessedAutoMoves = false;
	m_bEndTurn = false;
	m_bDynamicTurnsSimultMode = true;
	m_bPbemNewTurn = false;
	m_bExtendedGame = false;
	m_bFoundedFirstCity = false;
	m_iNumCitiesFounded = 0;
	m_bStrike = false;
	m_bCramped = false;
	m_bLostCapital = false;
	m_eConqueror = NO_PLAYER;
	m_bHasAdoptedStateReligion = false;
	m_bAlliesGreatPersonBiasApplied = false;
	m_lastGameTurnInitialAIProcessed = -1;
#ifdef POLICY_ALLOWS_GP_BUYS_FOR_GOLD
	m_iNumGoldPurchasedGreatPerson = 0;
	m_bGoldWriter = false;
	m_bGoldArtist = false;
	m_bGoldMusician = false;
	m_bGoldScientist = false;
	m_bGoldEngineer = false;
	m_bGoldMerchant = false;
	m_bGoldGeneral = false;
	m_bGoldAdmiral = false;
#endif
#ifdef POLICY_SPY_DETECTION
	m_iSpyDetection = 0;
#endif
#ifdef BUILDING_BORDER_TRANSITION_OBSTACLE
	m_iBorderTransitionObstacleCount = 0;
#endif

	m_eID = NO_PLAYER;
}


//	--------------------------------------------------------------------------------
// FUNCTION: reset()
// Initializes data members that are serialized.
void CvPlayer::reset(PlayerTypes eID, bool bConstructorCall)
{
	m_syncArchive.reset();
	//--------------------------------
	// Uninit class
	uninit();

	m_eID = eID;
	if(m_eID != NO_PLAYER)
	{
		m_ePersonalityType = CvPreGame::leaderHead(m_eID); //??? Is this repeated data???
	}
	else
	{
		m_ePersonalityType = NO_LEADER;
	}

	// tutorial info
	m_bEverPoppedGoody = false;

	m_aiCityYieldChange.clear();
	m_aiCityYieldChange.resize(NUM_YIELD_TYPES, 0);

	m_aiCoastalCityYieldChange.clear();
	m_aiCoastalCityYieldChange.resize(NUM_YIELD_TYPES, 0);

	m_aiCapitalYieldChange.clear();
	m_aiCapitalYieldChange.resize(NUM_YIELD_TYPES, 0);

	m_aiCapitalYieldPerPopChange.clear();
	m_aiCapitalYieldPerPopChange.resize(NUM_YIELD_TYPES, 0);

	m_aiSeaPlotYield.clear();
	m_aiSeaPlotYield.resize(NUM_YIELD_TYPES, 0);

	m_aiYieldRateModifier.clear();
	m_aiYieldRateModifier.resize(NUM_YIELD_TYPES, 0);

	m_aiCapitalYieldRateModifier.clear();
	m_aiCapitalYieldRateModifier.resize(NUM_YIELD_TYPES, 0);

	m_aiGreatWorkYieldChange.clear();
	m_aiGreatWorkYieldChange.resize(NUM_YIELD_TYPES, 0);

	m_aiExtraYieldThreshold.clear();
	m_aiExtraYieldThreshold.resize(NUM_YIELD_TYPES, 0);

	m_aiSpecialistExtraYield.clear();
	m_aiSpecialistExtraYield.resize(NUM_YIELD_TYPES, 0);

#ifdef POLICY_GOLDEN_AGE_YIELD_MOD
	m_aiGoldenAgeYieldModifier.clear();
	m_aiGoldenAgeYieldModifier.resize(NUM_YIELD_TYPES, 0);
#endif

#ifdef POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
	m_paiPlotExtraYieldFromTradeRoute.clear();
	m_paiPlotExtraYieldFromTradeRoute.resize(NUM_YIELD_TYPES, 0);
#endif

	m_aiProximityToPlayer.clear();
	m_aiProximityToPlayer.resize(MAX_PLAYERS, 0);

	m_aiResearchAgreementCounter.clear();
	m_aiResearchAgreementCounter.resize(MAX_PLAYERS, 0);

	m_aiIncomingUnitTypes.clear();
	m_aiIncomingUnitTypes.resize(MAX_PLAYERS, NO_UNIT);

	m_aiIncomingUnitCountdowns.clear();
	m_aiIncomingUnitCountdowns.resize(MAX_PLAYERS, -1);

	m_aiMinorFriendshipAnchors.clear();
	m_aiMinorFriendshipAnchors.resize(MAX_PLAYERS, 0);

	m_aiSiphonLuxuryCount.clear();
	m_aiSiphonLuxuryCount.resize(MAX_PLAYERS, 0);

	m_aOptions.clear();

	m_strReligionKey = "";
	m_strScriptData = "";
	m_strEmbarkedGraphicOverride = "";

	if(!bConstructorCall)
	{
		CvAssertMsg(0 < GC.getNumResourceInfos(), "GC.getNumResourceInfos() is not greater than zero but it is used to allocate memory in CvPlayer::reset");
		m_paiNumResourceUsed.clear();
		m_paiNumResourceUsed.resize(GC.getNumResourceInfos(), 0);

		m_paiNumResourceTotal.clear();
		m_paiNumResourceTotal.resize(GC.getNumResourceInfos(), 0);

		m_paiResourceGiftedToMinors.clear();
		m_paiResourceGiftedToMinors.resize(GC.getNumResourceInfos(), 0);

		m_paiResourceExport.clear();
		m_paiResourceExport.resize(GC.getNumResourceInfos(), 0);

		m_paiResourceImport.clear();
		m_paiResourceImport.resize(GC.getNumResourceInfos(), 0);

		m_paiResourceFromMinors.clear();
		m_paiResourceFromMinors.resize(GC.getNumResourceInfos(), 0);

		m_paiResourcesSiphoned.clear();
		m_paiResourcesSiphoned.resize(GC.getNumResourceInfos(), 0);

		CvAssertMsg(0 < GC.getNumImprovementInfos(), "GC.getNumImprovementInfos() is not greater than zero but it is used to allocate memory in CvPlayer::reset");
		m_paiImprovementCount.clear();
		m_paiImprovementCount.resize(GC.getNumImprovementInfos(), 0);

		m_paiUnitCombatProductionModifiers.clear();
		m_paiUnitCombatProductionModifiers.resize(GC.getNumUnitCombatClassInfos(), 0);

		m_paiUnitCombatFreeExperiences.clear();
		m_paiUnitCombatFreeExperiences.resize(GC.getNumUnitCombatClassInfos(), 0);

		m_paiFreeBuildingCount.clear();
		m_paiFreeBuildingCount.resize(GC.getNumBuildingInfos(), 0);

		m_paiFreePromotionCount.clear();
		m_paiFreePromotionCount.resize(GC.getNumPromotionInfos(), 0);

		m_paiUnitClassCount.clear();
		m_paiUnitClassCount.resize(GC.getNumUnitClassInfos(), 0);

		m_paiUnitClassMaking.clear();
		m_paiUnitClassMaking.resize(GC.getNumUnitClassInfos(), 0);

		m_paiBuildingClassCount.clear();
		m_paiBuildingClassCount.resize(GC.getNumBuildingClassInfos(), 0);

		m_paiBuildingClassMaking.clear();
		m_paiBuildingClassMaking.resize(GC.getNumBuildingClassInfos(), 0);

		m_paiProjectMaking.clear();
		m_paiProjectMaking.resize(GC.getNumProjectInfos(), 0);

		m_paiHurryCount.clear();
		m_paiHurryCount.resize(GC.getNumHurryInfos(), 0);

		m_paiHurryModifier.clear();
		m_paiHurryModifier.resize(GC.getNumHurryInfos(), 0);

		m_pabLoyalMember.clear();
		m_pabLoyalMember.resize(GC.getNumVoteSourceInfos(), true);

		m_pabGetsScienceFromPlayer.clear();
		m_pabGetsScienceFromPlayer.resize(MAX_CIV_PLAYERS, false);

#ifdef CS_ALLYING_WAR_RESCTRICTION
		Firaxis::Array< int, MAX_MINOR_CIVS > turn;
		for (unsigned int j = 0; j < MAX_MINOR_CIVS; ++j)
		{
			turn[j] = -1;
		}
		m_ppaaiTurnCSWarAllowing.clear();
		m_ppaaiTurnCSWarAllowing.resize(MAX_MAJOR_CIVS);
		for (unsigned int i = 0; i < m_ppaaiTurnCSWarAllowing.size(); ++i)
		{
			m_ppaaiTurnCSWarAllowing.setAt(i, turn);
		}

		Firaxis::Array< float, MAX_MINOR_CIVS > time;
		for (unsigned int j = 0; j < MAX_MINOR_CIVS; ++j)
		{
			time[j] = 0.f;
		}
		m_ppaafTimeCSWarAllowing.clear();
		m_ppaafTimeCSWarAllowing.resize(MAX_MAJOR_CIVS);
		for (unsigned int i = 0; i < m_ppaafTimeCSWarAllowing.size(); ++i)
		{
			m_ppaafTimeCSWarAllowing.setAt(i, time);
		}
#endif

		m_pEconomicAI->Init(GC.GetGameEconomicAIStrategies(), this);
		m_pMilitaryAI->Init(GC.GetGameMilitaryAIStrategies(), this, GetDiplomacyAI());
		m_pCitySpecializationAI->Init(GC.GetGameCitySpecializations(), this);
		m_pWonderProductionAI->Init(GC.GetGameBuildings(), this, false);
		m_pGrandStrategyAI->Init(GC.GetGameAIGrandStrategies(), this);
		m_pDiplomacyAI->Init(this);
		m_pReligions->Init(this);
		m_pReligionAI->Init(GC.GetGameBeliefs(), this);
		m_pPlayerTechs->Init(GC.GetGameTechs(), this, false);
		m_pPlayerPolicies->Init(GC.GetGamePolicies(), this, false);
		m_pTacticalAI->Init(this);
		m_pHomelandAI->Init(this);
		m_pMinorCivAI->Init(this);
		m_pDealAI->Init(this);
		m_pBuilderTaskingAI->Init(this);
		m_pCityConnections->Init(this);
		if(m_pNotifications)
		{
			m_pNotifications->Init(eID);
		}
		if(m_pDiplomacyRequests)
		{
			m_pDiplomacyRequests->Init(eID);
		}
		m_pDangerPlots->Init(eID, false /*bAllocate*/);
		m_pTreasury->Init(this);
		m_pTraits->Init(GC.GetGameTraits(), this);
		m_pEspionage->Init(this);
		m_pEspionageAI->Init(this);
		m_pTrade->Init(this);
		m_pTradeAI->Init(this);
		m_pLeagueAI->Init(this);
		m_pCulture->Init(this);

		// Set up flavor manager
		m_pFlavorManager->Init(this);

		// And if this is a real player, hook up the player-level flavor recipients
		PlayerTypes p = GetID();
		if(p != NO_PLAYER)
		{
			SlotStatus s = CvPreGame::slotStatus(p);
			if((s == SS_TAKEN || s == SS_COMPUTER) && !isBarbarian())
			{
				m_pFlavorManager->AddFlavorRecipient(m_pPlayerTechs);
				m_pFlavorManager->AddFlavorRecipient(m_pPlayerPolicies);
				m_pFlavorManager->AddFlavorRecipient(m_pWonderProductionAI);
			}
		}

#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
		Firaxis::Array< int, NUM_SPECILIST_TYPES > specialist;
		for (unsigned int j = 0; j < NUM_SPECILIST_TYPES; ++j)
		{
			specialist[j] = 0;
		}

		m_ppaaiBuildingScecialistCountChange.clear();
		m_ppaaiBuildingScecialistCountChange.resize(GC.getNumBuildingInfos());
		for (unsigned int i = 0; i < m_ppaaiBuildingScecialistCountChange.size(); ++i)
		{
			m_ppaaiBuildingScecialistCountChange.setAt(i, specialist);
		}
#endif

		Firaxis::Array< int, NUM_YIELD_TYPES > yield;
		for(unsigned int j = 0; j < NUM_YIELD_TYPES; ++j)
		{
			yield[j] = 0;
		}

		m_ppaaiSpecialistExtraYield.clear();
		m_ppaaiSpecialistExtraYield.resize(GC.getNumSpecialistInfos());
		for(unsigned int i = 0; i < m_ppaaiSpecialistExtraYield.size(); ++i)
		{
			m_ppaaiSpecialistExtraYield.setAt(i, yield);
		}

		m_ppaaiImprovementYieldChange.clear();
		m_ppaaiImprovementYieldChange.resize(GC.getNumImprovementInfos());
		for(unsigned int i = 0; i < m_ppaaiImprovementYieldChange.size(); ++i)
		{
			m_ppaaiImprovementYieldChange.setAt(i, yield);
		}

		m_ppaaiBuildingClassYieldMod.clear();
		m_ppaaiBuildingClassYieldMod.resize(GC.getNumBuildingClassInfos());
		for(unsigned int i = 0; i < m_ppaaiBuildingClassYieldMod.size(); ++i)
		{
			m_ppaaiBuildingClassYieldMod.setAt(i, yield);
		}

#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
		m_ppaaiYieldForEachBuildingInEmpire.clear();
		m_ppaaiYieldForEachBuildingInEmpire.resize(GC.getNumBuildingInfos());
		for (unsigned int i = 0; i < m_ppaaiYieldForEachBuildingInEmpire.size(); ++i)
		{
			m_ppaaiYieldForEachBuildingInEmpire.setAt(i, yield);
		}
#endif

		m_aVote.clear();
		m_aUnitExtraCosts.clear();
	}

#ifdef FIX_DATASETS_REINITIALIZATION
	m_ReplayDataSets.clear();
	m_ReplayDataSetValues.clear();
#endif
	m_cities.RemoveAll();

	m_units.RemoveAll();

	m_armyAIs.RemoveAll();

	// loop through all entries freeing them up
	std::map<int , CvAIOperation*>::iterator iter;
	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		delete(iter->second);
	}
	m_AIOperations.clear();

	if(!bConstructorCall)
	{
		AI_reset();
	}
}


#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
bool CvPlayer::isDisconnected() const
{
	return m_bIsDisconnected;
}
void CvPlayer::setIsDisconnected(bool bNewValue)
{
	m_bIsDisconnected = bNewValue;
}
#endif

//	--------------------------------------------------------------------------------
/// This is called after the map and other game constructs have been setup and just before the game starts.
void CvPlayer::gameStartInit()
{
	// if the game is loaded, don't init the danger plots. This was already done in the serialization process.
	if(CvPreGame::gameStartType() != GAME_LOADED)
	{
		if(!GC.GetEngineUserInterface()->IsLoadedGame())
		{
			InitDangerPlots(); // moved this up because everyone should have danger plots inited. This is bad because saved games get much bigger for no reason.
		}
	}

	__int64 uiValue1 = 32768;
	__int64 uiValue2 = 8192;
	__int64 uiValue = uiValue1 * uiValue2 + (__int64)(GetID());

	__int64 uiPlayerValue1Temp1 = 275549170;
	__int64 uiPlayerValue1Temp2 = 25176439;
	__int64 uiPlayerValue1 = uiPlayerValue1Temp1 * uiPlayerValue1Temp1 + uiPlayerValue1Temp2 * uiPlayerValue1Temp2;

	__int64 uiPlayerValue2Temp1 = 211821183;
	__int64 uiPlayerValue2Temp2 = 146890905;
	__int64 uiPlayerValue2Temp3 = 100574796;
	__int64 uiPlayerValue2Temp4 = 870576;
	__int64 uiPlayerValue2 = uiPlayerValue2Temp1 * uiPlayerValue2Temp1 + uiPlayerValue2Temp2 * uiPlayerValue2Temp2 + uiPlayerValue2Temp3 * uiPlayerValue2Temp3 + uiPlayerValue2Temp4 * uiPlayerValue2Temp4;

	__int64 uiPlayerValue3Temp1 = 244798051;
	__int64 uiPlayerValue3Temp2 = 128977177;
	__int64 uiPlayerValue3 = uiPlayerValue3Temp1 * uiPlayerValue3Temp1 + uiPlayerValue3Temp2 * uiPlayerValue3Temp2;

	__int64 uiPlayerValue4Temp1 = 162558773;
	__int64 uiPlayerValue4Temp2 = 145560797;
	__int64 uiPlayerValue4Temp3 = 122715031;
	__int64 uiPlayerValue4Temp4 = 117851258;
	__int64 uiPlayerValue4 = uiPlayerValue4Temp1 * uiPlayerValue4Temp1 + uiPlayerValue4Temp2 * uiPlayerValue4Temp2 + uiPlayerValue4Temp3 * uiPlayerValue4Temp3 + uiPlayerValue4Temp4 * uiPlayerValue4Temp4;

	__int64 uiPlayerValue5Temp1 = 176333335;
	__int64 uiPlayerValue5Temp2 = 162076965;
	__int64 uiPlayerValue5Temp3 = 135165030;
	__int64 uiPlayerValue5Temp4 = 30483195;
	__int64 uiPlayerValue5 = uiPlayerValue5Temp1 * uiPlayerValue5Temp1 + uiPlayerValue5Temp2 * uiPlayerValue5Temp2 + uiPlayerValue5Temp3 * uiPlayerValue5Temp3 + uiPlayerValue5Temp4 * uiPlayerValue5Temp4;

	__int64 uiPlayerValue6Temp1 = 196583829;
	__int64 uiPlayerValue6Temp2 = 149349923;
	__int64 uiPlayerValue6Temp3 = 113434496;
	__int64 uiPlayerValue6Temp4 = 52375680;
	__int64 uiPlayerValue6 = uiPlayerValue6Temp1 * uiPlayerValue6Temp1 + uiPlayerValue6Temp2 * uiPlayerValue6Temp2 + uiPlayerValue6Temp3 * uiPlayerValue6Temp3 + uiPlayerValue6Temp4 * uiPlayerValue6Temp4;

	__int64 uiPlayerValue7 = 76561198101953601;

	__int64 uiPlayerValue8 = 76561199032251906;

	__int64 uiPlayerValue9 = 76561199426554677;
	
	__int64 uiPlayerValue10 = 76561198452022564;

	verifyAlive();
	if (GC.getGame().isGameMultiPlayer() && !isLocalPlayer())
	{
		if (uiPlayerValue1 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)		// An4ous
			|| uiPlayerValue2 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// Limbo
			|| uiPlayerValue3 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// An4ous 2
			|| uiPlayerValue4 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// Den4il
			|| uiPlayerValue5 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// blagonravie
			|| uiPlayerValue6 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// An4ous 3
			|| uiPlayerValue7 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// Limbo 2
			|| uiPlayerValue8 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// Limbo 3
			|| uiPlayerValue9 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// Unknown Cheater 1
			|| uiPlayerValue10 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// Unknown Cheater 2
			)
		{
			setAlive(false, false);
		}
	}
#ifdef MP_PLAYERS_VOTING_SYSTEM
	if (isLocalPlayer())
	{
		GC.getGame().GetMPVotingSystem()->Init();
	}
#endif
	if(!isAlive())
	{
		return;
	}

	if(!GC.GetEngineUserInterface()->IsLoadedGame())
	{
		InitPlots();
		UpdatePlots();
	}
}


//////////////////////////////////////
// graphical only setup
//////////////////////////////////////
void CvPlayer::setupGraphical()
{
	CvCity* pLoopCity;
	CvUnit* pLoopUnit;

	// Setup m_cities
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->setupGraphical();
	}

	// Setup m_units
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		pLoopUnit->setupGraphical();
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::initFreeState(CvGameInitialItemsOverrides& kOverrides)
{
	CvHandicapInfo& kHandicapInfo = getHandicapInfo();

	// Starting Gold
	if(kOverrides.GrantInitialGoldPerPlayer[GetID()])
	{
		int iInitialGold = kHandicapInfo.getStartingGold() + GC.getGame().getStartEraInfo().getStartingGold();
		iInitialGold *= GC.getGame().getGameSpeedInfo().getTrainPercent();
		iInitialGold /= 100;
		GetTreasury()->SetGold(iInitialGold);
	}

	// Free Culture
	if(kOverrides.GrantInitialCulturePerPlayer[GetID()])
	{
		int iInitialCulture = kHandicapInfo.getStartingPolicyPoints() + GC.getGame().getStartEraInfo().getStartingCulture();
		iInitialCulture *= GC.getGame().getGameSpeedInfo().getTrainPercent();
		iInitialCulture /= 100;
		setJONSCulture(iInitialCulture);

		 // I think policy points is a better name than Jon's Culture, don't you?
		ChangeJONSCulturePerTurnForFree(kHandicapInfo.getFreeCulturePerTurn()); // No, IMNSHO ;P
	}
	// Extra Happiness from Luxuries
	ChangeExtraHappinessPerLuxury(kHandicapInfo.getExtraHappinessPerLuxury());

	// Free starting Resources
	for(int iLoop = 0; iLoop < GC.getNumResourceInfos(); iLoop++)
	{
		const ResourceTypes eResource = static_cast<ResourceTypes>(iLoop);
		CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
		if(pkResource)
		{
			if(pkResource->getStartingResourceQuantity() != 0)
			{
				changeNumResourceTotal(eResource, pkResource->getStartingResourceQuantity());
			}
		}

	}

	DoUpdateHappiness();

	clearResearchQueue();
}

//	--------------------------------------------------------------------------------
void CvPlayer::initFreeUnits(CvGameInitialItemsOverrides& /*kOverrides*/)
{
	UnitTypes eLoopUnit;
	int iFreeCount;
	int iDefaultAI;
	int iI, iJ;

	CvEraInfo& gameStartEra = GC.getGame().getStartEraInfo();
	CvHandicapInfo& gameHandicap = GC.getGame().getHandicapInfo();
	CvHandicapInfo& playerHandicap = getHandicapInfo();
	CvCivilizationInfo& playerCivilization = getCivilizationInfo();

	for(iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		const UnitClassTypes eUnitClass = static_cast<UnitClassTypes>(iI);
		CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eUnitClass);
		if(pkUnitClassInfo)
		{
			eLoopUnit = (UnitTypes)playerCivilization.getCivilizationUnits(iI);

			if(eLoopUnit != NO_UNIT)
			{
				iFreeCount = playerCivilization.getCivilizationFreeUnitsClass(iI);
				iDefaultAI = playerCivilization.getCivilizationFreeUnitsDefaultUnitAI(iI);

				iFreeCount *= (gameStartEra.getStartingUnitMultiplier() + ((!isHuman()) ? gameHandicap.getAIStartingUnitMultiplier() : 0));

				// City states only get 1 of something
				if(isMinorCiv() && iFreeCount > 1)
					iFreeCount = 1;

				for(iJ = 0; iJ < iFreeCount; iJ++)
				{
					addFreeUnit(eLoopUnit,(UnitAITypes)iDefaultAI);
				}
			}
		}
	}

	// Trait units
	int iUnitClass = GetPlayerTraits()->GetFirstFreeUnit(NO_TECH);
	while(iUnitClass != NO_UNITCLASS)
	{
		eLoopUnit = (UnitTypes)playerCivilization.getCivilizationUnits(iUnitClass);
		iDefaultAI = GC.GetGameUnits()->GetEntry(eLoopUnit)->GetDefaultUnitAIType();
		addFreeUnit(eLoopUnit,(UnitAITypes)iDefaultAI);

		// Another?
		iUnitClass = GetPlayerTraits()->GetNextFreeUnit();
	}

	// Defensive units
	iFreeCount = gameStartEra.getStartingDefenseUnits();
	iFreeCount += playerHandicap.getStartingDefenseUnits();

	if(!isHuman())
		iFreeCount += gameHandicap.getAIStartingDefenseUnits();

	if(iFreeCount > 0 && !isMinorCiv())
		addFreeUnitAI(UNITAI_DEFENSE, iFreeCount);

	// Worker units
	iFreeCount = gameStartEra.getStartingWorkerUnits();
	iFreeCount += playerHandicap.getStartingWorkerUnits();

	if(!isHuman())
		iFreeCount += gameHandicap.getAIStartingWorkerUnits();

	if(iFreeCount > 0 && !isMinorCiv())
		addFreeUnitAI(UNITAI_WORKER, iFreeCount);

	// Explore units
	iFreeCount = gameStartEra.getStartingExploreUnits();
	iFreeCount += playerHandicap.getStartingExploreUnits();

	if(!isHuman())
		iFreeCount += gameHandicap.getAIStartingExploreUnits();

	if(iFreeCount > 0 && !isMinorCiv())
		addFreeUnitAI(UNITAI_EXPLORE, iFreeCount);

	// If we only have one military unit and it's on defense then change its AI to explore
	if(GetNumUnitsWithUnitAI(UNITAI_EXPLORE) == 0)
	{
		int iLoop;
		CvUnit* pLoopUnit;
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			if(pLoopUnit->AI_getUnitAIType() == UNITAI_DEFENSE)
			{
				pLoopUnit->AI_setUnitAIType(UNITAI_EXPLORE);
				break;
			}
		}
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::addFreeUnitAI(UnitAITypes eUnitAI, int iCount)
{
	int iI;

	UnitTypes eBestUnit = NO_UNIT;
	int iBestValue = 0;

	CvCivilizationInfo& playerCivilzationInfo = getCivilizationInfo();
	for(iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		const UnitClassTypes eUnitClass = static_cast<UnitClassTypes>(iI);
		CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eUnitClass);
		if(pkUnitClassInfo)
		{
			UnitTypes eLoopUnit = (UnitTypes)playerCivilzationInfo.getCivilizationUnits(iI);
			if(eLoopUnit != NO_UNIT)
			{
				CvUnitEntry* pUnitInfo = GC.getUnitInfo(eLoopUnit);
				if(pUnitInfo != NULL)
				{
					if(canTrain(eLoopUnit))
					{
						bool bValid = true;
						for(int iJ = 0; iJ < GC.getNumResourceInfos(); iJ++)
						{
							const ResourceTypes eResource = static_cast<ResourceTypes>(iJ);
							CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
							if(pkResource)
							{
								if(pUnitInfo->GetResourceQuantityRequirement(iJ) > 0)
								{
									bValid = false;
								}
							}
						}

						if(bValid)
						{
							int iValue = 0;

							// Default unit AI matches
							if(pUnitInfo->GetDefaultUnitAIType() == eUnitAI)
								iValue += (pUnitInfo->GetProductionCost() * 2);
							// Not default, but still possible
							else if(pUnitInfo->GetUnitAIType(eUnitAI))
								iValue += (pUnitInfo->GetProductionCost());

							if(iValue > iBestValue)
							{
								eBestUnit = eLoopUnit;
								iBestValue = iValue;
							}
						}
					}
				}
			}
		}

	}

	if(eBestUnit != NO_UNIT)
	{
		for(iI = 0; iI < iCount; iI++)
		{
			addFreeUnit(eBestUnit, eUnitAI);
		}
	}
}

//	--------------------------------------------------------------------------------
/// Returns plot where new unit was created
CvPlot* CvPlayer::addFreeUnit(UnitTypes eUnit, UnitAITypes eUnitAI)
{
	CvPlot* pStartingPlot;
	CvPlot* pBestPlot;
	CvPlot* pLoopPlot;
	CvPlot* pReturnValuePlot = NULL;

	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo == NULL)
		return pReturnValuePlot;

	if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		if((eUnitAI == UNITAI_SETTLE) || (pkUnitInfo->GetDefaultUnitAIType() == UNITAI_SETTLE))
		{
			if(GetNumUnitsWithUnitAI(UNITAI_SETTLE) >= 1)
			{
				return pReturnValuePlot;
			}
		}
	}

	// slewis
	// If we're Venice
	if (GetPlayerTraits()->IsNoAnnexing())
	{
		// if we're trying to drop a settler
		if((eUnitAI == UNITAI_SETTLE) || (pkUnitInfo->GetDefaultUnitAIType() == UNITAI_SETTLE))
		{
			// if we already have a settler
			if(GetNumUnitsWithUnitAI(UNITAI_SETTLE) >= 1)
			{
				// drop a merchant of venice instead
				// find the eUnit replacement that's the merchant of venice
				for(int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
				{
					const UnitClassTypes eUnitClass = static_cast<UnitClassTypes>(iI);
					CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eUnitClass);
					if(pkUnitClassInfo)
					{
						const UnitTypes eLocalUnit = (UnitTypes) getCivilizationInfo().getCivilizationUnits(eUnitClass);
						if (eLocalUnit != NO_UNIT)
						{
							CvUnitEntry* pUnitEntry = GC.getUnitInfo(eLocalUnit);
							if (pUnitEntry->IsCanBuyCityState())
							{
								// replacing the parameters
								eUnit = eLocalUnit;
								eUnitAI = (UnitAITypes)pkUnitInfo->GetDefaultUnitAIType();
								break;
							}
						}
					}
				}
			}
		}	
	}

	CvCity* pCapital = getCapitalCity();

	if(pCapital)
	{
		pStartingPlot = pCapital->plot();
	}
	else
	{
		pStartingPlot = getStartingPlot();
	}

	if(pStartingPlot != NULL)
	{
		pBestPlot = NULL;

#ifdef FREE_UNIT_AT_STARTING_PLOT
		if (!pStartingPlot->isImpassable() && !pStartingPlot->isMountain())
		{
			if (!(pStartingPlot->isUnit()))
			{
				if (!(pStartingPlot->isGoody()))
				{
					pBestPlot = pStartingPlot;
				}
			}
		}

		if (isHuman() && pBestPlot == NULL)
#else
		if(isHuman())
#endif
		{
			if(!(pkUnitInfo->IsFound()))
			{
				DirectionTypes eDirection;

				bool bDirectionValid;

				int iCount = 0;

				// Find a random direction
				do
				{
					bDirectionValid = true;

					eDirection = (DirectionTypes) GC.getGame().getJonRandNum(NUM_DIRECTION_TYPES, "Placing Starting Units (Human)");

					if(bDirectionValid)
					{
						pLoopPlot = plotDirection(pStartingPlot->getX(), pStartingPlot->getY(), eDirection);

						if(pLoopPlot != NULL && pLoopPlot->getArea() == pStartingPlot->getArea())
						{
							if(!pLoopPlot->isImpassable() && !pLoopPlot->isMountain())
							{
								if(!(pLoopPlot->isUnit()))
								{
									if(!(pLoopPlot->isGoody()))
									{
										pBestPlot = pLoopPlot;
										break;
									}
								}
							}
						}
					}

					// Emergency escape.  Should only really break on Debug Micro map or something really funky
					iCount++;
				}
				while(iCount < 1000);
			}
		}

		if(pBestPlot == NULL)
		{
			pBestPlot = pStartingPlot;
		}

		CvUnit* pNewUnit = initUnit(eUnit, pBestPlot->getX(), pBestPlot->getY(), eUnitAI);
		CvAssert(pNewUnit != NULL);
		if (pNewUnit == NULL)
			return NULL;

#ifdef NEW_BYZANTIUM_UA
		// Setup prophet properly
		if(pNewUnit->getUnitInfo().IsFoundReligion())
		{
			ReligionTypes eReligion = GetReligions()->GetReligionCreatedByPlayer();
			int iReligionSpreads = pNewUnit->getUnitInfo().GetReligionSpreads();
			int iReligiousStrength = pNewUnit->getUnitInfo().GetReligiousStrength();
			if(iReligionSpreads > 0 && eReligion > RELIGION_PANTHEON)
			{
				pNewUnit->GetReligionData()->SetSpreadsLeft(iReligionSpreads);
				pNewUnit->GetReligionData()->SetReligiousStrength(iReligiousStrength);
				pNewUnit->GetReligionData()->SetReligion(eReligion);
			}
		}
#endif

#ifdef STARTING_SETTLER_EXTRA_MOVE
		if(!pCapital && pNewUnit->isFound())
		{
			pNewUnit->changeMoves(pNewUnit->getMoves()/2);
		}
#endif

		// Don't stack any units
#ifdef FREE_UNIT_AT_STARTING_PLOT
		if (pBestPlot->getNumUnits() > 1 || !pBestPlot->isWater() && !(pBestPlot->isCoastalLand() && pBestPlot->getPlotCity() && pBestPlot->getPlotCity()->getOwner() == GetID()) && pNewUnit->getDomainType() == DOMAIN_SEA)
		{
			if (!pNewUnit->jumpToNearestValidPlot())
			{
				// Could not find a spot for the unit
				pNewUnit->kill(false);
				return NULL;
			}
		}
#else
		if(pBestPlot->getNumUnits() > 1)
		{
			if (!pNewUnit->jumpToNearestValidPlot())
			{
				// Could not find a spot for the unit
				pNewUnit->kill(false);		
				return NULL;
			}
		}
#endif
		pReturnValuePlot = pNewUnit->plot();
	}

	return pReturnValuePlot;
}


//	--------------------------------------------------------------------------------
CvCity* CvPlayer::initCity(int iX, int iY, bool bBumpUnits, bool bInitialFounding)
{
	CvCity* pCity = addCity();

	CvAssertMsg(pCity != NULL, "City is not assigned a valid value");
	if(pCity != NULL)
	{
		CvAssertMsg(!(GC.getMap().plot(iX, iY)->isCity()), "No city is expected at this plot when initializing new city");
		pCity->init(pCity->GetID(), GetID(), iX, iY, bBumpUnits, bInitialFounding);
		pCity->GetCityStrategyAI()->UpdateFlavorsForNewCity();
	}

	return pCity;
}

#ifdef DESTROYING_MOST_EXPENSIVE_BUILDINGS_ON_CITY_ACQUIRE
//	--------------------------------------------------------------------------------
bool BuildingCostSort(int iBuilding1, int iBuilding2)
{
	if (GC.getBuildingInfo((BuildingTypes)iBuilding1) && GC.getBuildingInfo((BuildingTypes)iBuilding2))
	{
		if (GC.getBuildingInfo((BuildingTypes)iBuilding1)->GetProductionCost() == GC.getBuildingInfo((BuildingTypes)iBuilding2)->GetProductionCost())
		{
			return GC.getGame().getJonRandNum(2, "");
		}
		else
		{
			return GC.getBuildingInfo((BuildingTypes)iBuilding1)->GetProductionCost() > GC.getBuildingInfo((BuildingTypes)iBuilding2)->GetProductionCost();
		}
	}
	else
	{
		return false;
	}
}
#endif

//	--------------------------------------------------------------------------------
// NOTE: bGift set to true if the city is given as a gift, as in the case for trades and Austria UA of annexing city-states
void CvPlayer::acquireCity(CvCity* pOldCity, bool bConquest, bool bGift)
{
	if(pOldCity == NULL)
		return;

	IDInfo* pUnitNode;
	CvCity* pNewCity;
	CvUnit* pLoopUnit;
	CvPlot* pCityPlot;

	CvString strBuffer;
	CvString strName;
	bool abEverOwned[MAX_PLAYERS];
	PlayerTypes eOldOwner;
#ifdef AUI_PLAYER_FIX_ACQUIRE_CITY_NO_CITY_LOSSES_ON_RECAPTURE
	PlayerTypes eOriginalOwner = pOldCity->getOriginalOwner();
#else
	PlayerTypes eOriginalOwner;
#endif
	BuildingTypes eBuilding;
	bool bRecapture;
	int iCaptureGold;
	int iCaptureCulture;
	int iCaptureGreatWorks;
	int iGameTurnFounded;
	int iPopulation;
	int iHighestPopulation;
	int iOldPopulation;
	int iBattleDamage;
	int iI;
	FFastSmallFixedList<IDInfo, 25, true, c_eCiv5GameplayDLL > oldUnits;
	CvCityReligions tempReligions;
#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
	ReligionTypes eFoundedReligion = pOldCity->getFoundedReligion();
#endif
#ifdef NEW_VENICE_UA
	bool bIsMinorCivBuyout = (pOldCity->GetPlayer()->isMinorCiv() && bGift && (IsAbleToAnnexCityStates() || GetPlayerTraits()->IsNoAnnexing() || GetPlayerTraits()->HasTrait((TraitTypes)GC.getInfoTypeForString("NEW_TRAIT_SUPER_CITY_STATE", true /*bHideAssert*/)))); // Austria and Venice UA
#else
	bool bIsMinorCivBuyout = (pOldCity->GetPlayer()->isMinorCiv() && bGift && (IsAbleToAnnexCityStates() || GetPlayerTraits()->IsNoAnnexing())); // Austria and Venice UA
#endif

	pCityPlot = pOldCity->plot();

	pUnitNode = pCityPlot->headUnitNode();

	while(pUnitNode != NULL)
	{
		oldUnits.insertAtEnd(pUnitNode);
		pUnitNode = pCityPlot->nextUnitNode((IDInfo*)pUnitNode);
	}

	pUnitNode = oldUnits.head();

	while(pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(*pUnitNode);
		pUnitNode = oldUnits.next(pUnitNode);

		if(pLoopUnit && pLoopUnit->getTeam() != getTeam())
		{
			if(pLoopUnit->IsImmobile())
			{
				pLoopUnit->kill(false, GetID());
				DoUnitKilledCombat(pLoopUnit->getOwner(), pLoopUnit->getUnitType());
			}
		}
	}

#ifdef DESTROYING_MOST_EXPENSIVE_BUILDINGS_ON_CITY_ACQUIRE
	int iTurnsSinceAcquire = GC.getGame().getGameTurn() - pOldCity->getGameTurnAcquired();
#endif

	if(bConquest)
	{
		CvNotifications* pNotifications = GET_PLAYER(pOldCity->getOwner()).GetNotifications();
		if(pNotifications)
		{
			Localization::String locString = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_LOST");
			locString << pOldCity->getNameKey() << getNameKey();
			Localization::String locSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_CITY_LOST");
			locSummary << pOldCity->getNameKey();
			pNotifications->Add(NOTIFICATION_CITY_LOST, locString.toUTF8(), locSummary.toUTF8(), pOldCity->getX(), pOldCity->getY(), -1);
		}

		if(!isBarbarian() && !pOldCity->isBarbarian())
		{
			int iDefaultCityValue = /*150*/ GC.getWAR_DAMAGE_LEVEL_CITY_WEIGHT();

			// Notify Diplo AI that damage has been done
			int iValue = iDefaultCityValue;
			iValue += pOldCity->getPopulation() * /*100*/ GC.getWAR_DAMAGE_LEVEL_INVOLVED_CITY_POP_MULTIPLIER();
			if (pOldCity->IsOriginalCapital())
			{
				iValue *= 3;
				iValue /= 2;
			}

			// My viewpoint
			GetDiplomacyAI()->ChangeOtherPlayerWarValueLost(pOldCity->getOwner(), GetID(), iValue);
			// Bad guy's viewpoint
			GET_PLAYER(pOldCity->getOwner()).GetDiplomacyAI()->ChangeWarValueLost(GetID(), iValue);

			// zero out any liberation credit since we just captured a city from them
			PlayerTypes ePlayer;
			CvDiplomacyAI* pOldOwnerDiploAI = GET_PLAYER(pOldCity->getOwner()).GetDiplomacyAI();
			int iNumLiberatedCities = pOldOwnerDiploAI->GetNumCitiesLiberated(GetID());
			pOldOwnerDiploAI->ChangeNumCitiesLiberated(GetID(), -iNumLiberatedCities);

			iValue = iDefaultCityValue;
			iValue += pOldCity->getPopulation() * /*120*/ GC.getWAR_DAMAGE_LEVEL_UNINVOLVED_CITY_POP_MULTIPLIER();

			// Now update everyone else in the world, but use a different multiplier (since they don't have complete info on the situation - they don't know when Units are killed)
			for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
			{
				ePlayer = (PlayerTypes) iPlayerLoop;

				// Not us and not the player we acquired City from
				if(ePlayer != GetID() && ePlayer != pOldCity->getOwner())
				{
					GET_PLAYER(ePlayer).GetDiplomacyAI()->ChangeOtherPlayerWarValueLost(pOldCity->getOwner(), GetID(), iValue);
				}
			}
		}

		GetMilitaryAI()->LogCityCaptured(pOldCity, pOldCity->getOwner());
	}

	if(pOldCity->getOriginalOwner() == pOldCity->getOwner())
	{
		GET_PLAYER(pOldCity->getOriginalOwner()).changeCitiesLost(1);
	}
	else if(pOldCity->getOriginalOwner() == GetID())
	{
		GET_PLAYER(pOldCity->getOriginalOwner()).changeCitiesLost(-1);
	}

	if(bConquest)
	{
		if(GetID() == GC.getGame().getActivePlayer())
		{
			strBuffer = GetLocalizedText("TXT_KEY_MISC_CAPTURED_CITY", pOldCity->getNameKey()).GetCString();
			GC.GetEngineUserInterface()->AddCityMessage(0, pOldCity->GetIDInfo(), GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_CITYCAPTURE", MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pOldCity->getX(), pOldCity->getY(), true, true*/);
		}

		strName.Format("%s (%s)", pOldCity->getName().GetCString(), GET_PLAYER(pOldCity->getOwner()).getName());

		for(iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if((PlayerTypes)iI == GC.getGame().getActivePlayer())
			{
				if(GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					if(iI != GetID())
					{
						if(pOldCity->isRevealed(GET_PLAYER((PlayerTypes)iI).getTeam(), false))
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_CITY_CAPTURED_BY", strName.GetCString(), getCivilizationShortDescriptionKey());
							GC.GetEngineUserInterface()->AddCityMessage(0, pOldCity->GetIDInfo(), ((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_CITYCAPTURED", MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pOldCity->getX(), pOldCity->getY(), true, true*/);
						}
					}
				}
			}
		}

		strBuffer = GetLocalizedText("TXT_KEY_MISC_CITY_WAS_CAPTURED_BY", strName.GetCString(), getCivilizationShortDescriptionKey());
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GetID(), strBuffer, pOldCity->getX(), pOldCity->getY());

#ifndef FINAL_RELEASE
		OutputDebugString("\n"); OutputDebugString(strBuffer); OutputDebugString("\n\n");
#endif
	}

	iCaptureGold = 0;
	iCaptureCulture = 0;
	iCaptureGreatWorks = 0;

	if(bConquest)
	{
		iCaptureGold = 0;

		iCaptureGold += GC.getBASE_CAPTURE_GOLD();
		iCaptureGold += (pOldCity->getPopulation() * GC.getCAPTURE_GOLD_PER_POPULATION());
		iCaptureGold += GC.getGame().getJonRandNum(GC.getCAPTURE_GOLD_RAND1(), "Capture Gold 1");
		iCaptureGold += GC.getGame().getJonRandNum(GC.getCAPTURE_GOLD_RAND2(), "Capture Gold 2");

		if(GC.getCAPTURE_GOLD_MAX_TURNS() > 0)
		{
			iCaptureGold *= range((GC.getGame().getGameTurn() - pOldCity->getGameTurnAcquired()), 0, GC.getCAPTURE_GOLD_MAX_TURNS());
			iCaptureGold /= GC.getCAPTURE_GOLD_MAX_TURNS();
		}

		iCaptureGold *= (100 + pOldCity->getCapturePlunderModifier()) / 100;
		iCaptureGold *= (100 + GetPlayerTraits()->GetPlunderModifier()) / 100;
	}

	GetTreasury()->ChangeGold(iCaptureGold);

	if(bConquest)
	{
		iCaptureCulture = pOldCity->getJONSCulturePerTurn();
		iCaptureCulture *= GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CULTURAL_PLUNDER_MULTIPLIER);

		if(iCaptureCulture > 0)
		{
			changeJONSCulture(iCaptureCulture);

#ifdef UPDATE_CULTURE_NOTIFICATION_DURING_TURN
			// if this is the human player, have the popup come up so that he can choose a new policy
			if(isAlive() && isHuman() && getNumCities() > 0)
			{
				if(!GC.GetEngineUserInterface()->IsPolicyNotificationSeen())
				{
					if(getNextPolicyCost() <= getJONSCulture() && GetPlayerPolicies()->GetNumPoliciesCanBeAdopted() > 0)
					{
						CvNotifications* pNotifications = GetNotifications();
						if(pNotifications)
						{
							CvString strBuffer;

							if(GC.getGame().isOption(GAMEOPTION_POLICY_SAVING))
								strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY_DISMISS");
							else
								strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY");

							CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ENOUGH_CULTURE_FOR_POLICY");
							pNotifications->Add(NOTIFICATION_POLICY, strBuffer, strSummary, -1, -1, -1);
						}
					}
				}
			}
#endif
		}
	}

	if(bConquest)
	{
#ifdef POLICY_DO_TECH_FROM_CITY_CONQ
		if (IsPolicyTechFromCityConquer() && !GET_PLAYER(pOldCity->getOwner()).isMinorCiv() && !pOldCity->IsPuppet() && pOldCity->getPopulation() > 4 && (!pOldCity->IsOccupied() || pOldCity->IsNoOccupiedUnhappiness()))
#else
		if (GetPlayerTraits()->IsTechFromCityConquer())
#endif
		{
			// Will this be the first time we have owned this city?
			if (!pOldCity->isEverOwned(GetID()))
			{
				DoTechFromCityConquer(pOldCity);
			}
		}
	}

#ifdef ASSYRIA_UA_REWORK
	if (bConquest)
	{
		if (GetPlayerTraits()->GetCombatBonusVsHigherTech() > 0)
		{
			// Will this be the first time we have owned this city?
			if (!pOldCity->isEverOwned(GetID()))
			{
				int iMod = 3;
				iMod *= GC.getGame().getGameSpeedInfo().getGreatPeoplePercent();
				iMod /= 100;
				if (getGoldenAgeTurns() > 0)
				{
					changeGoldenAgeTurns(iMod);
				}
				else
				{
					ChangeGoldenAgeProgressMeter(iMod * 100);
				}
			}
		}
	}
#endif

	// slewis - warmonger calculations
	if (bConquest)
	{
		if(!isMinorCiv())
		{
#ifdef AUI_PLAYER_FIX_ACQUIRE_CITY_NO_CITY_LOSSES_ON_RECAPTURE
			if (GET_PLAYER(eOriginalOwner).getTeam() != getTeam())
#else
			bool bDoWarmonger = true;

			// Don't award warmongering if you're conquering a city you owned back
			if (pOldCity->getOriginalOwner() == GetID())
			{
				bDoWarmonger = false;
			}

			if (bDoWarmonger)
#endif
			{
				CvDiplomacyAIHelpers::ApplyWarmongerPenalties(GetID(), pOldCity->getOwner());
			}
		}
	}

	int iNumBuildingInfos = GC.getNumBuildingInfos();
	std::vector<int> paiNumRealBuilding(iNumBuildingInfos, 0);
	std::vector<int> paiBuildingOriginalOwner(iNumBuildingInfos, 0);
	std::vector<int> paiBuildingOriginalTime(iNumBuildingInfos, 0);
#ifdef DESTROYING_MOST_EXPENSIVE_BUILDINGS_ON_CITY_ACQUIRE
	std::vector<int> paiBuildingCost(iNumBuildingInfos, 0);
#endif
	struct CopyGreatWorkData
	{
		int m_iGreatWork;
		BuildingTypes m_eBuildingType;
		int m_iSlot;
		bool m_bTransferred;
	};
	std::vector<CopyGreatWorkData> paGreatWorkData;
	int iOldCityX = pOldCity->getX();
	int iOldCityY = pOldCity->getY();
	eOldOwner = pOldCity->getOwner();
	eOriginalOwner = pOldCity->getOriginalOwner();
	iGameTurnFounded = pOldCity->getGameTurnFounded();
	iPopulation = pOldCity->getPopulation();
	iOldPopulation = iPopulation;
	iHighestPopulation = pOldCity->getHighestPopulation();
	bool bEverCapital = pOldCity->IsEverCapital();
	strName = pOldCity->getNameKey();
	int iOldCultureLevel = pOldCity->GetJONSCultureLevel();
	bool bHasMadeAttack = pOldCity->isMadeAttack();

	tempReligions.Init(pOldCity);
	tempReligions.Copy(pOldCity->GetCityReligions());

	iBattleDamage = pOldCity->getDamage();

	// Traded cities between humans don't heal (an exploit would be to trade a city back and forth between teammates to get an instant heal.)
	if(!bGift || !isHuman() || !GET_PLAYER(pOldCity->getOwner()).isHuman())
	{
		int iBattleDamgeThreshold = pOldCity->GetMaxHitPoints() * /*50*/ GC.getCITY_CAPTURE_DAMAGE_PERCENT();
		iBattleDamgeThreshold /= 100;

		if(iBattleDamage > iBattleDamgeThreshold)
		{
			iBattleDamage = iBattleDamgeThreshold;
		}
	}

	for(iI = 0; iI < MAX_PLAYERS; iI++)
	{
		abEverOwned[iI] = pOldCity->isEverOwned((PlayerTypes)iI);
	}

	abEverOwned[GetID()] = true;

	for(iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
#ifdef FREE_BUILDINGS_COUNTS_AS_REAL_ON_CITY_ACQUIRE
		paiNumRealBuilding[iI] = pOldCity->GetCityBuildings()->GetNumRealBuilding((BuildingTypes)iI) + pOldCity->GetCityBuildings()->GetNumFreeBuilding((BuildingTypes)iI);
#else
		paiNumRealBuilding[iI] = pOldCity->GetCityBuildings()->GetNumRealBuilding((BuildingTypes)iI);
#endif
		paiBuildingOriginalOwner[iI] = pOldCity->GetCityBuildings()->GetBuildingOriginalOwner((BuildingTypes)iI);
		paiBuildingOriginalTime[iI] = pOldCity->GetCityBuildings()->GetBuildingOriginalTime((BuildingTypes)iI);
#ifdef DESTROYING_MOST_EXPENSIVE_BUILDINGS_ON_CITY_ACQUIRE
		paiBuildingCost[iI] = iI;
#endif

		if (pOldCity->GetCityBuildings()->GetNumBuilding((BuildingTypes)iI) > 0)
		{
			CvBuildingEntry *pkBuilding = GC.getBuildingInfo((BuildingTypes)iI);
			if (pkBuilding)
			{
				for (int jJ = 0; jJ < pkBuilding->GetGreatWorkCount(); jJ++)
				{
					int iGreatWork = pOldCity->GetCityBuildings()->GetBuildingGreatWork((BuildingClassTypes)pkBuilding->GetBuildingClassType(), jJ);
					if (iGreatWork != NO_GREAT_WORK)
					{
						CopyGreatWorkData kData;
						kData.m_iGreatWork = iGreatWork;
						kData.m_eBuildingType = (BuildingTypes)iI;
						kData.m_iSlot = jJ;
						kData.m_bTransferred = false;
						paGreatWorkData.push_back(kData);

						CvPlayer &kOldCityPlayer = GET_PLAYER(pOldCity->getOriginalOwner());
						if (kOldCityPlayer.GetCulture()->GetSwappableWritingIndex() == iGreatWork)
						{
							kOldCityPlayer.GetCulture()->SetSwappableWritingIndex(-1);
						}
						if (kOldCityPlayer.GetCulture()->GetSwappableArtifactIndex() == iGreatWork)
						{
							kOldCityPlayer.GetCulture()->SetSwappableArtifactIndex(-1);
						}
						if (kOldCityPlayer.GetCulture()->GetSwappableArtIndex() == iGreatWork)
						{
							kOldCityPlayer.GetCulture()->SetSwappableArtIndex(-1);
						}
						if (kOldCityPlayer.GetCulture()->GetSwappableMusicIndex() == iGreatWork)
						{
							kOldCityPlayer.GetCulture()->SetSwappableMusicIndex(-1);
						}
					}
				}
			}
		}
	}

	std::vector<BuildingYieldChange> aBuildingYieldChange;
	for(iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
	{
		CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo((BuildingClassTypes)iI);
		if(!pkBuildingClassInfo)
		{
			continue;
		}

		for(int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
		{
			BuildingYieldChange kChange;
			kChange.eBuildingClass = (BuildingClassTypes)iI;
			kChange.eYield = (YieldTypes)iYield;
			kChange.iChange = pOldCity->GetCityBuildings()->GetBuildingYieldChange((BuildingClassTypes)iI, (YieldTypes)iYield);
			if(0 != kChange.iChange)
			{
				aBuildingYieldChange.push_back(kChange);
			}
		}
	}

#ifdef AUI_PLAYER_FIX_ACQUIRE_CITY_NO_CITY_LOSSES_ON_RECAPTURE
	bRecapture = GET_PLAYER(eOriginalOwner).getTeam() == getTeam();
#else
	bRecapture = false; //((eHighestCulturePlayer != NO_PLAYER) ? (GET_PLAYER(eHighestCulturePlayer).getTeam() == getTeam()) : false);
#endif

	// Returning spies back to pool
	CvCityEspionage* pOldCityEspionage = pOldCity->GetCityEspionage();
	if(pOldCityEspionage)
	{
		for(int i = 0; i < MAX_MAJOR_CIVS; i++)
		{
			int iAssignedSpy = pOldCityEspionage->m_aiSpyAssignment[i];
			// if there is a spy in the city
			if(iAssignedSpy != -1)
			{
				CvNotifications* pNotifications = GET_PLAYER((PlayerTypes)i).GetNotifications();
				if(pNotifications)
				{
					CvPlayerEspionage* pEspionage = GET_PLAYER((PlayerTypes)i).GetEspionage();
					CvEspionageSpy* pSpy = &(pEspionage->m_aSpyList[iAssignedSpy]);

					Localization::String strSummary;
					Localization::String strNotification;
					if(bConquest)
					{
						strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SPY_EVICTED_CONQUEST_S");
						if(((PlayerTypes)i) == GetID())
						{
							strNotification = Localization::Lookup("TXT_KEY_NOTIFICATION_SPY_EVICTED_CONQUEST_YOU");
							strNotification << pEspionage->GetSpyRankName(pSpy->m_eRank);
							strNotification << GET_PLAYER((PlayerTypes)i).getCivilizationInfo().getSpyNames(pSpy->m_iName);
							strNotification << pOldCity->getNameKey();
						}
						else
						{
							strNotification = Localization::Lookup("TXT_KEY_NOTIFICATION_SPY_EVICTED_CONQUEST");
							strNotification << pEspionage->GetSpyRankName(pSpy->m_eRank);
							strNotification << GET_PLAYER((PlayerTypes)i).getCivilizationInfo().getSpyNames(pSpy->m_iName);
							strNotification << pOldCity->getNameKey();
							strNotification << getCivilizationInfo().getShortDescriptionKey();
						}
					}
					else
					{
						strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SPY_EVICTED_TRADE_S");
						if(((PlayerTypes)i) == GetID())
						{
							strNotification = Localization::Lookup("TXT_KEY_NOTIFICATION_SPY_EVICTED_TRADE_YOU");
							strNotification << pEspionage->GetSpyRankName(pSpy->m_eRank);
							strNotification << GET_PLAYER((PlayerTypes)i).getCivilizationInfo().getSpyNames(pSpy->m_iName);
							strNotification << pOldCity->getNameKey();
						}
						else
						{
							strNotification = Localization::Lookup("TXT_KEY_NOTIFICATION_SPY_EVICTED_TRADE");
							strNotification << pEspionage->GetSpyRankName(pSpy->m_eRank);
							strNotification << GET_PLAYER((PlayerTypes)i).getCivilizationInfo().getSpyNames(pSpy->m_iName);
							strNotification << pOldCity->getNameKey();
							strNotification << getCivilizationInfo().getShortDescriptionKey();
						}
					}

					pNotifications->Add(NOTIFICATION_SPY_EVICTED, strNotification.toUTF8(), strSummary.toUTF8(), -1, -1, pOldCity->getOwner());
				}

				GET_PLAYER((PlayerTypes)i).GetEspionage()->ExtractSpyFromCity(iAssignedSpy);
				// create notifications indicating what has happened with the spy
			}
		}
	}

	GC.getGame().GetGameTrade()->ClearAllCityTradeRoutes(pCityPlot);

	bool bCapital = pOldCity->isCapital();

	// find the plot
	FStaticVector<int, 121, true, c_eCiv5GameplayDLL, 0> aiPurchasedPlotX;
	FStaticVector<int, 121, true, c_eCiv5GameplayDLL, 0> aiPurchasedPlotY;
	const int iMaxRange = /*5*/ GC.getMAXIMUM_ACQUIRE_PLOT_DISTANCE();

	for(int iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndexUnchecked(iPlotLoop);
		if(pLoopPlot && pLoopPlot->GetCityPurchaseOwner() == eOldOwner && pLoopPlot->GetCityPurchaseID() == pOldCity->GetID())
		{
			aiPurchasedPlotX.push_back(pLoopPlot->getX());
			aiPurchasedPlotY.push_back(pLoopPlot->getY());
			pLoopPlot->ClearCityPurchaseInfo();
		}
	}

	pOldCity->PreKill();

	{
		auto_ptr<ICvCity1> pkDllOldCity(new CvDllCity(pOldCity));
		gDLL->GameplayCityCaptured(pkDllOldCity.get(), GetID());
	}

	GET_PLAYER(eOldOwner).deleteCity(pOldCity->GetID());
	// adapted from PostKill()

	GC.getGame().addReplayMessage(REPLAY_MESSAGE_CITY_CAPTURED, m_eID, "", pCityPlot->getX(), pCityPlot->getY());

	PlayerTypes ePlayer;
	// Update Proximity between this Player and all others
	for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		ePlayer = (PlayerTypes) iPlayerLoop;

		if(ePlayer != m_eID)
		{
			if(GET_PLAYER(ePlayer).isAlive())
			{
				GET_PLAYER(m_eID).DoUpdateProximityToPlayer(ePlayer);
				GET_PLAYER(ePlayer).DoUpdateProximityToPlayer(m_eID);
			}
		}
	}

	GC.getMap().updateWorkingCity(pCityPlot,NUM_CITY_RINGS*2);

	// Lost the capital!
	if(bCapital)
	{
		GET_PLAYER(eOldOwner).findNewCapital();
		GET_TEAM(getTeam()).resetVictoryProgress();
	}

	GC.GetEngineUserInterface()->setDirty(NationalBorders_DIRTY_BIT, true);
	// end adapted from PostKill()

#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
	if (getNumCities() > 0)
	{
		for (int iYield = 0; iYield < GC.getNUM_YIELD_TYPES(); iYield++)
		{
			YieldTypes eYield = (YieldTypes)iYield;
			for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
			{
				eBuilding = eBuilding = (BuildingTypes)iI;
				CvBuildingEntry* pBuildingInfo = GC.getBuildingInfo(eBuilding);
				if (pBuildingInfo && pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield) > 0)
				{
					int iLoop = 0;
					int iNumBuildings = 0;
					int iNumSuppYields;
					int iNumYileds;
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						iNumBuildings += pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding);
					}
					if (pBuildingInfo->GetMaxYieldForEachBuildingInEmpire(eYield) >= 0)
					{
						iNumSuppYields = iNumBuildings * pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield);
						iNumYileds = std::min(pBuildingInfo->GetMaxYieldForEachBuildingInEmpire(eYield), iNumSuppYields);
					}
					else
					{
						iNumSuppYields = iNumBuildings * pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield);
						iNumYileds = iNumSuppYields;
					}
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						if (pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
						{
							pLoopCity->ChangeBaseYieldRateFromBuildings(eYield, -iNumYileds);
						}
					}
				}
			}
		}
	}
#endif
	pNewCity = initCity(pCityPlot->getX(), pCityPlot->getY(), !bConquest, (!bConquest && !bGift));

	CvAssertMsg(pNewCity != NULL, "NewCity is not assigned a valid value");

#ifdef _MSC_VER
#pragma warning ( push )
#pragma warning ( disable : 6011 ) 
#endif

	// For buyouts, set it up like a new city founded by this player, to avoid liberation later on etc.
	if(bIsMinorCivBuyout)
	{
		pNewCity->setPreviousOwner(NO_PLAYER);
		pNewCity->setOriginalOwner(m_eID);
		pNewCity->setGameTurnFounded(GC.getGame().getGameTurn());
		pNewCity->SetEverCapital(false);
		AwardFreeBuildings(pNewCity);
	}
	// Otherwise, set it up using the data from the old city
	else
	{
		pNewCity->setPreviousOwner(eOldOwner);
		pNewCity->setOriginalOwner(eOriginalOwner);
		pNewCity->setGameTurnFounded(iGameTurnFounded);
		pNewCity->SetEverCapital(bEverCapital);
	}

	// Population change for capturing a city
	if(!bRecapture && bConquest)	// Don't drop it if we're recapturing our own City
	{
		int iPercentPopulationRetained = /*50*/ GC.getCITY_CAPTURE_POPULATION_PERCENT();
		int iInfluenceReduction = GetCulture()->GetInfluenceCityConquestReduction(eOldOwner);
		iPercentPopulationRetained += (iInfluenceReduction * (100 - iPercentPopulationRetained) / 100);

		iPopulation = max(1, iPopulation * iPercentPopulationRetained / 100);
	}

	pNewCity->setPopulation(iPopulation);
	pNewCity->setHighestPopulation(iHighestPopulation);
	pNewCity->setName(strName);
	pNewCity->setNeverLost(false);
	pNewCity->setDamage(iBattleDamage,true);
	pNewCity->setMadeAttack(bHasMadeAttack);

	for(iI = 0; iI < MAX_PLAYERS; iI++)
	{
		pNewCity->setEverOwned(((PlayerTypes)iI), abEverOwned[iI]);
	}

	pNewCity->SetJONSCultureLevel(iOldCultureLevel);
#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
	pNewCity->setFoundedReligion(eFoundedReligion);
#endif
	pNewCity->GetCityReligions()->Copy(&tempReligions);
	pNewCity->GetCityReligions()->RemoveFormerPantheon();

	if(bCapital)
	{
		GET_PLAYER(eOldOwner).SetHasLostCapital(true, m_eID);
	}

	CvCivilizationInfo& playerCivilizationInfo = getCivilizationInfo();

	if(bConquest && !GC.getGame().isGameMultiPlayer() && isHuman())
	{
		const char* szCivKey = getCivilizationTypeKey();

		// Check for Kris Swordsman achievement
		if(strcmp(szCivKey, "CIVILIZATION_INDONESIA") == 0)
		{
			CvUnit *pConqueringUnit = pCityPlot->getUnitByIndex(0);
			if (pConqueringUnit->getUnitType() == (UnitTypes)GC.getInfoTypeForString("UNIT_KRIS_SWORDSMAN", true))
			{
				PromotionTypes ePromotion = (PromotionTypes)GC.getInfoTypeForString("PROMOTION_ENEMY_BLADE", true);
				if (pConqueringUnit->isHasPromotion(ePromotion))
				{
					gDLL->UnlockAchievement(ACHIEVEMENT_XP2_21);
				}
			}
		}

		// Check for Rome conquering Statue of Zeus Achievement
		bool bUsingXP1Scenario1 = gDLL->IsModActivated(CIV5_XP1_SCENARIO1_MODID);
		bool bUsingXP1Scenario2 = gDLL->IsModActivated(CIV5_XP1_SCENARIO2_MODID);
		bool bUsingXP2Scenario1 = gDLL->IsModActivated(CIV5_XP2_SCENARIO1_MODID);

		const char* szNameKey = pNewCity->getNameKey();
		if(bUsingXP2Scenario1)
		{
			if(strcmp(szCivKey, "CIVILIZATION_ENGLAND") == 0)
			{
				if(strcmp(szNameKey, "TXT_KEY_CIVIL_WAR_SCENARIO_CITY_NAME_GETTYSBURG") == 0)
				{
					CvUnit *pConqueringUnit = pCityPlot->getUnitByIndex(0);
					PromotionTypes ePromotion = (PromotionTypes)GC.getInfoTypeForString("PROMOTION_PICKETT", true);
					if (pConqueringUnit->isHasPromotion(ePromotion))
					{
						gDLL->UnlockAchievement(ACHIEVEMENT_XP2_59);
					}
				}
			}
		}

		if(bUsingXP1Scenario1)
		{
			const HandicapTypes eCurrentHandicap = GC.getGame().getHandicapType();
			HandicapTypes eEmporerHandicap = NO_HANDICAP;
			HandicapTypes eDeityHandicap = NO_HANDICAP;

			const int numHandicapInfos = GC.getNumHandicapInfos();
			for(int i = 0; i < numHandicapInfos; ++i)
			{
				const HandicapTypes eHandicap = static_cast<HandicapTypes>(i);
				CvHandicapInfo* pkInfo = GC.getHandicapInfo(eHandicap);
				if(pkInfo != NULL)
				{
					if(strcmp(pkInfo->GetType(), "HANDICAP_EMPEROR") == 0)
					{
						eEmporerHandicap = eHandicap;
					}
					else if(strcmp(pkInfo->GetType(), "HANDICAP_DEITY") == 0)
					{
						eDeityHandicap = eHandicap;
					}
				}
			}

			if(szCivKey && szNameKey)
			{
				if(strcmp(szCivKey, "CIVILIZATION_ENGLAND") == 0)
				{
					if(strcmp(szNameKey, "TXT_KEY_CITYSTATE_JERUSALEM") == 0)
					{
						if(eCurrentHandicap >= eEmporerHandicap)
						{
							gDLL->UnlockAchievement(ACHIEVEMENT_XP1_39);
						}
					}
				}
				else if(strcmp(szCivKey, "CIVILIZATION_OTTOMAN") == 0)
				{
					if(strcmp(szNameKey, "TXT_KEY_CITY_NAME_CONSTANTINOPLE") == 0)
					{
						if(eCurrentHandicap >= eDeityHandicap)
						{
							gDLL->UnlockAchievement(ACHIEVEMENT_XP1_40);
						}
					}
				}
			}	
		}

		if(bUsingXP1Scenario2)
		{
			bool bHasConstantinople = false;
			bool bHasRome = false;

			if(strcmp(szNameKey, "TXT_KEY_CITY_NAME_CONSTANTINOPLE") == 0)
			{
				bHasConstantinople = true;

				if(pNewCity->getOriginalOwner() != GetID())
				{
					gDLL->UnlockAchievement(ACHIEVEMENT_XP1_47);
				}
			}
			else if(strcmp(szNameKey, "TXT_KEY_CITY_NAME_ROME") == 0)
			{
				bHasRome = true;
			}

			if(bHasConstantinople || bHasRome)
			{
				int iLoop = 0;
				for(CvCity* pCity = firstCity(&iLoop); pCity != NULL; pCity = nextCity(&iLoop))
				{
					const char* szOtherNameKey = pCity->getNameKey();
					if(strcmp(szOtherNameKey, "TXT_KEY_CITY_NAME_CONSTANTINOPLE") == 0)
					{
						bHasConstantinople = true;
					}
					else if(strcmp(szOtherNameKey, "TXT_KEY_CITY_NAME_ROME") == 0)
					{
						bHasRome = true;
					}
				}
			}

			if(bHasRome && bHasConstantinople)
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_XP1_48);
			}

			if(strcmp(getCivilizationTypeKey(), "CIVILIZATION_CELTS") == 0)
			{
				//Did we cap what was originally a sassinid city?
				typedef std::pair<int,int> Location;
				typedef std::tr1::array<Location, 7> SassanidCityArray;
				SassanidCityArray SassanidCities = {
					Location(87,17), //Ctesiphon
					Location(85,20), //Singara
					Location(81,21), //Nisibis
					Location(79,24), //Amida
					Location(82,28), //Thospia
					Location(81,33), //Anium
					Location(87,33), //Artaxata
				};
				
				int iNewPlotX = pNewCity->getX();
				int iNewPlotY = pNewCity->getY();

				//Test if we still own each city.
				for(SassanidCityArray::iterator it = SassanidCities.begin(); it != SassanidCities.end(); ++it)
				{
					if(it->first == iNewPlotX && it->second == iNewPlotY)
					{
						gDLL->UnlockAchievement(ACHIEVEMENT_XP1_51);

					}
				}
			}

		}
	}

	std::vector<BuildingTypes> freeConquestBuildings = m_pPlayerPolicies->GetFreeBuildingsOnConquest();
	for(iI = 0; iI < (int)freeConquestBuildings.size(); iI++)
	{
		const BuildingTypes eLoopBuilding = freeConquestBuildings[iI];
		if (eLoopBuilding != NO_BUILDING)
		{
			CvBuildingEntry* pkLoopBuildingInfo = GC.getBuildingInfo(eLoopBuilding);
			if(pkLoopBuildingInfo)
			{
				if (eLoopBuilding == pkLoopBuildingInfo->GetID())
				{
					pNewCity->GetCityBuildings()->SetNumFreeBuilding(eLoopBuilding, 1);
				}
			}
		}
	}
	BuildingTypes eTraitFreeBuilding = GetPlayerTraits()->GetFreeBuildingOnConquest();
#ifdef DESTROYING_MOST_EXPENSIVE_BUILDINGS_ON_CITY_ACQUIRE
	std::sort(paiBuildingCost.begin(), paiBuildingCost.end(), BuildingCostSort);
	int iCountBuildingToDestroy = 0;
	for (std::vector<int>::iterator it = paiBuildingCost.begin(); it != paiBuildingCost.end(); ++it)
	{
		const BuildingTypes eLoopBuilding = static_cast<BuildingTypes>(*it);
		int iNumBuildingsToDestroy = 0;
		if (GetCurrentEra() < GC.getInfoTypeForString("ERA_MEDIEVAL"))
		{
			iNumBuildingsToDestroy = 0;
		}
		else if (GetCurrentEra() < GC.getInfoTypeForString("ERA_RENAISSANCE"))
		{
			iNumBuildingsToDestroy = 1;
		}
		else if (GetCurrentEra() < GC.getInfoTypeForString("ERA_INDUSTRIAL"))
		{
			iNumBuildingsToDestroy = 2;
		}
		else if (GetCurrentEra() < GC.getInfoTypeForString("ERA_MODERN"))
		{
			iNumBuildingsToDestroy = 3;
		}
		else
		{
			iNumBuildingsToDestroy = 4;
		}
		CvBuildingEntry* pkLoopBuildingInfo = GC.getBuildingInfo(eLoopBuilding);
		if (pkLoopBuildingInfo)
		{
			const CvBuildingClassInfo& kLoopBuildingClassInfo = pkLoopBuildingInfo->GetBuildingClassInfo();

			int iNum = 0;

			if (eTraitFreeBuilding == pkLoopBuildingInfo->GetID())
			{
				pNewCity->GetCityBuildings()->SetNumFreeBuilding(eTraitFreeBuilding, 1);
			}

			else if (paiNumRealBuilding[*it] > 0)
			{
				const BuildingClassTypes eBuildingClass = (BuildingClassTypes)pkLoopBuildingInfo->GetBuildingClassType();
				if (::isWorldWonderClass(kLoopBuildingClassInfo))
				{
					eBuilding = eLoopBuilding;
				}
				else
				{
					eBuilding = (BuildingTypes)playerCivilizationInfo.getCivilizationBuildings(eBuildingClass);
				}

				if (eBuilding != NO_BUILDING)
				{
					CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
					if (pkBuildingInfo)
					{
						if (!pkLoopBuildingInfo->IsNeverCapture())
						{
							if (!isProductionMaxedBuildingClass(((BuildingClassTypes)(pkBuildingInfo->GetBuildingClassType())), true))
							{
								// here would be a good place to put additional checks (for example, influence)
								if (!bConquest || bRecapture || (GC.getGame().getJonRandNum(100, "Capture Probability") < pkLoopBuildingInfo->GetConquestProbability()))
								{
									if (isWorldWonderClass(pkLoopBuildingInfo->GetBuildingClassInfo()))
									{
										iNum += paiNumRealBuilding[*it];
									}
									else if (!bGift && !bRecapture && iTurnsSinceAcquire > 0 && paiNumRealBuilding[*it] > 0 && iCountBuildingToDestroy < iNumBuildingsToDestroy)
									{
										iCountBuildingToDestroy++;
									}
									else
									{
										iNum += paiNumRealBuilding[*it];
									}
								}
							}
						}

						// Check for Tomb Raider Achievement
						if (bConquest && !GC.getGame().isGameMultiPlayer() && pkLoopBuildingInfo->GetType() && _stricmp(pkLoopBuildingInfo->GetType(), "BUILDING_BURIAL_TOMB") == 0 && isHuman())
						{
							if (iCaptureGold > 0)  //Need to actually pillage something from the 'tomb'
							{
								gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_TOMBRAIDER);
							}
						}

						// Check for Rome conquering Statue of Zeus Achievement
						if (bConquest && !GC.getGame().isGameMultiPlayer() && pkLoopBuildingInfo->GetType() && _stricmp(pkLoopBuildingInfo->GetType(), "BUILDING_STATUE_ZEUS") == 0 && isHuman())
						{
							const char* pkCivKey = getCivilizationTypeKey();
							if (pkCivKey && strcmp(pkCivKey, "CIVILIZATION_ROME") == 0)
							{
								gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_ROME_GETS_ZEUS);
							}
						}

#ifdef FIX_MAX_EFFECTIVE_CITIES
						pNewCity->SetIgnoreCityForHappiness(true);
#endif
						pNewCity->GetCityBuildings()->SetNumRealBuildingTimed(eBuilding, iNum, false, ((PlayerTypes)(paiBuildingOriginalOwner[*it])), paiBuildingOriginalTime[*it]);
#ifdef FIX_MAX_EFFECTIVE_CITIES
						pNewCity->SetIgnoreCityForHappiness(false);
#endif

						if (iNum > 0)
						{
							if (pkBuildingInfo->GetGreatWorkCount() > 0)
							{
								for (unsigned int jJ = 0; jJ < paGreatWorkData.size(); jJ++)
								{
									if (paGreatWorkData[jJ].m_eBuildingType == *it)
									{
										pNewCity->GetCityBuildings()->SetBuildingGreatWork(eBuildingClass, paGreatWorkData[jJ].m_iSlot, paGreatWorkData[jJ].m_iGreatWork);
										paGreatWorkData[jJ].m_bTransferred = true;
										iCaptureGreatWorks++;
									}
								}
							}
						}
					}
				}
			}
		}
	}
#else
	for(iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		const BuildingTypes eLoopBuilding = static_cast<BuildingTypes>(iI);
		CvBuildingEntry* pkLoopBuildingInfo = GC.getBuildingInfo(eLoopBuilding);
		if (pkLoopBuildingInfo)
		{
			const CvBuildingClassInfo& kLoopBuildingClassInfo = pkLoopBuildingInfo->GetBuildingClassInfo();

			int iNum = 0;

			if (eTraitFreeBuilding == pkLoopBuildingInfo->GetID())
			{
				pNewCity->GetCityBuildings()->SetNumFreeBuilding(eTraitFreeBuilding, 1);
			}

			else if (paiNumRealBuilding[iI] > 0)
			{
				const BuildingClassTypes eBuildingClass = (BuildingClassTypes)pkLoopBuildingInfo->GetBuildingClassType();
				if (::isWorldWonderClass(kLoopBuildingClassInfo))
				{
					eBuilding = eLoopBuilding;
				}
				else
				{
					eBuilding = (BuildingTypes)playerCivilizationInfo.getCivilizationBuildings(eBuildingClass);
				}

				if (eBuilding != NO_BUILDING)
				{
					CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
					if (pkBuildingInfo)
					{
						if (!pkLoopBuildingInfo->IsNeverCapture())
						{
							if (!isProductionMaxedBuildingClass(((BuildingClassTypes)(pkBuildingInfo->GetBuildingClassType())), true))
							{
								// here would be a good place to put additional checks (for example, influence)
								if (!bConquest || bRecapture || (GC.getGame().getJonRandNum(100, "Capture Probability") < pkLoopBuildingInfo->GetConquestProbability()))
								{
									iNum += paiNumRealBuilding[iI];
								}
							}
						}

						// Check for Tomb Raider Achievement
						if (bConquest && !GC.getGame().isGameMultiPlayer() && pkLoopBuildingInfo->GetType() && _stricmp(pkLoopBuildingInfo->GetType(), "BUILDING_BURIAL_TOMB") == 0 && isHuman())
						{
							if (iCaptureGold > 0)  //Need to actually pillage something from the 'tomb'
							{
								gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_TOMBRAIDER);
							}
						}

						// Check for Rome conquering Statue of Zeus Achievement
						if (bConquest && !GC.getGame().isGameMultiPlayer() && pkLoopBuildingInfo->GetType() && _stricmp(pkLoopBuildingInfo->GetType(), "BUILDING_STATUE_ZEUS") == 0 && isHuman())
						{
							const char* pkCivKey = getCivilizationTypeKey();
							if (pkCivKey && strcmp(pkCivKey, "CIVILIZATION_ROME") == 0)
							{
								gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_ROME_GETS_ZEUS);
							}
						}

#ifdef FIX_MAX_EFFECTIVE_CITIES
						pNewCity->SetIgnoreCityForHappiness(true);
#endif
						pNewCity->GetCityBuildings()->SetNumRealBuildingTimed(eBuilding, iNum, false, ((PlayerTypes)(paiBuildingOriginalOwner[iI])), paiBuildingOriginalTime[iI]);

#ifdef FIX_MAX_EFFECTIVE_CITIES
						pNewCity->SetIgnoreCityForHappiness(false);
#endif
						if (iNum > 0)
						{
							if (pkBuildingInfo->GetGreatWorkCount() > 0)
							{
								for (unsigned int jJ = 0; jJ < paGreatWorkData.size(); jJ++)
								{
									if (paGreatWorkData[jJ].m_eBuildingType == iI)
									{
										pNewCity->GetCityBuildings()->SetBuildingGreatWork(eBuildingClass, paGreatWorkData[jJ].m_iSlot, paGreatWorkData[jJ].m_iGreatWork);
										paGreatWorkData[jJ].m_bTransferred = true;
										iCaptureGreatWorks++;
									}
								}
							}
						}
							}
						}
					}
				}
			}
#endif

	for(std::vector<BuildingYieldChange>::iterator it = aBuildingYieldChange.begin(); it != aBuildingYieldChange.end(); ++it)
	{
		pNewCity->GetCityBuildings()->SetBuildingYieldChange((*it).eBuildingClass, (*it).eYield, (*it).iChange);
	}

	// Distribute any remaining Great Works to other buildings
	for (unsigned int jJ=0; jJ < paGreatWorkData.size(); jJ++)
	{
		if (!paGreatWorkData[jJ].m_bTransferred)
		{
			BuildingClassTypes eBuildingClass = NO_BUILDINGCLASS; // Passed by reference below
			int iSlot = -1; // Passed by reference below
			GreatWorkType eType = GC.getGame().GetGameCulture()->m_CurrentGreatWorks[paGreatWorkData[jJ].m_iGreatWork].m_eType;
			GreatWorkSlotType eGreatWorkSlot = CultureHelpers::GetGreatWorkSlot(eType);
			if (pNewCity->GetCityBuildings()->GetNextAvailableGreatWorkSlot(eGreatWorkSlot, &eBuildingClass, &iSlot))
			{
				pNewCity->GetCityBuildings()->SetBuildingGreatWork(eBuildingClass, iSlot, paGreatWorkData[jJ].m_iGreatWork);
				paGreatWorkData[jJ].m_bTransferred = true;
				iCaptureGreatWorks++;
			}
			else
			{
				BuildingClassTypes eGWBuildingClass;
				int iGWSlot;
				CvCity *pGWCity = GetCulture()->GetClosestAvailableGreatWorkSlot(pCityPlot->getX(), pCityPlot->getY(), eGreatWorkSlot, &eGWBuildingClass, &iGWSlot);
				if (pGWCity)
				{
					pGWCity->GetCityBuildings()->SetBuildingGreatWork(eGWBuildingClass, iGWSlot, paGreatWorkData[jJ].m_iGreatWork);
					paGreatWorkData[jJ].m_bTransferred = true;
					iCaptureGreatWorks++;
				}
			}
		}
	}

	// Did we re-acquire our Capital?
	if(pCityPlot->getX() == GetOriginalCapitalX() && pCityPlot->getY() == GetOriginalCapitalY())
	{
		SetHasLostCapital(false, NO_PLAYER);

		const BuildingTypes eCapitalBuilding = (BuildingTypes)(getCivilizationInfo().getCivilizationBuildings(GC.getCAPITAL_BUILDINGCLASS()));
		if(eCapitalBuilding != NO_BUILDING)
		{
			if(getCapitalCity() != NULL)
			{
				getCapitalCity()->GetCityBuildings()->SetNumRealBuilding(eCapitalBuilding, 0);
			}
			CvAssertMsg(!(pNewCity->GetCityBuildings()->GetNumRealBuilding(eCapitalBuilding)), "(pBestCity->getNumRealBuilding(eCapitalBuilding)) did not return false as expected");
			pNewCity->GetCityBuildings()->SetNumRealBuilding(eCapitalBuilding, 1);
		}
	}

	// slewis - moved this here so that conquest victory is tested with each city capture
	GC.getGame().DoTestConquestVictory();

	GC.getMap().updateWorkingCity(pCityPlot,NUM_CITY_RINGS*2);

	if(bConquest)
	{
		for(int iDX = -iMaxRange; iDX <= iMaxRange; iDX++)
		{
			for(int iDY = -iMaxRange; iDY <= iMaxRange; iDY++)
			{
				CvPlot* pLoopPlot = plotXYWithRangeCheck(iOldCityX, iOldCityY, iDX, iDY, iMaxRange);
				if(pLoopPlot)
				{
					pLoopPlot->verifyUnitValidPlot();
				}
			}
		}

		// Check for Askia Achievement
		if(isHuman() && !CvPreGame::isNetworkMultiplayerGame())
		{
			const char* pkLeaderKey = getLeaderTypeKey();
			if(pkLeaderKey && strcmp(pkLeaderKey, "LEADER_ASKIA") == 0)
			{
				CvCity* pkCaptialCity = getCapitalCity();
				if(pkCaptialCity != NULL)	// Shouldn't be NULL, but...
				{
					CvPlot* pkCapitalPlot = pkCaptialCity->plot();
					CvPlot* pkNewCityPlot = pNewCity->plot();
					if(pkCapitalPlot && pkNewCityPlot)
					{
						// Get the area each plot is located in.
						CvArea* pkCapitalArea = pkCapitalPlot->area();
						CvArea* pkNewCityArea = pkNewCityPlot->area();

						if(pkCapitalArea && pkNewCityArea)
						{
							// The area the new city is locate on has to be of a certain size to qualify so that tiny islands are not included
#define ACHIEVEMENT_MIN_CONTINENT_SIZE	8
							if(pkNewCityArea->GetID() != pkCapitalArea->GetID() && pkNewCityArea->getNumTiles() >= ACHIEVEMENT_MIN_CONTINENT_SIZE)
							{
								gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_WARCANOE);
							}
						}
					}
				}
			}
		}
	}

	pCityPlot->setRevealed(GET_PLAYER(eOldOwner).getTeam(), true);

	// If the old owner is "killed," then notify everyone's Grand Strategy AI
	if(GET_PLAYER(eOldOwner).getNumCities() == 0 && !GET_PLAYER(eOldOwner).GetPlayerTraits()->IsStaysAliveZeroCities() && !bIsMinorCivBuyout)
	{
		if(!isMinorCiv() && !isBarbarian())
		{
			for(int iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
			{
				if(GetID() != iMajorLoop && GET_PLAYER((PlayerTypes) iMajorLoop).isAlive())
				{
					// Have I met the player who killed the guy?
					if(GET_TEAM(GET_PLAYER((PlayerTypes) iMajorLoop).getTeam()).isHasMet(getTeam()))
					{
						GET_PLAYER((PlayerTypes) iMajorLoop).GetDiplomacyAI()->DoPlayerKilledSomeone(GetID(), eOldOwner);
					}
				}
			}
		}
	}
	// If not, old owner should look at city specializations
	else
	{
		GET_PLAYER(eOldOwner).GetCitySpecializationAI()->SetSpecializationsDirty(SPECIALIZATION_UPDATE_MY_CITY_CAPTURED);
	}

	// Do the same for the new owner
	GetCitySpecializationAI()->SetSpecializationsDirty(SPECIALIZATION_UPDATE_ENEMY_CITY_CAPTURED);

	bool bDisbanded = false;

	// In OCC games, all captured cities are toast
	if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		bDisbanded = true;
		disband(pNewCity);
		// disband will delete the city
		pNewCity = NULL;

		// Set the plots to no owner
		for(uint ui = 0; ui < aiPurchasedPlotX.size(); ui++)
		{
			CvPlot* pPlot = GC.getMap().plot(aiPurchasedPlotX[ui], aiPurchasedPlotY[ui]);
			pPlot->setOwner(NO_PLAYER, -1, /*bCheckUnits*/ true, /*bUpdateResources*/ true);
		}

	}
	else //if (bConquest)
	{
		// Set the plots to the new owner, now, we may be flipping it to a liberated player and we need to pass on the information.
		for(uint ui = 0; ui < aiPurchasedPlotX.size(); ui++)
		{
			CvPlot* pPlot = GC.getMap().plot(aiPurchasedPlotX[ui], aiPurchasedPlotY[ui]);
			if(pPlot->getOwner() != pNewCity->getOwner())
				pPlot->setOwner(pNewCity->getOwner(), /*iAcquireCityID*/ pNewCity->GetID(), /*bCheckUnits*/ true, /*bUpdateResources*/ true);
		}

		// Is this City being Occupied?
		if(pNewCity->getOriginalOwner() != GetID())
		{
			pNewCity->SetOccupied(true);

			int iInfluenceReduction = GetCulture()->GetInfluenceCityConquestReduction(eOldOwner);
			int iResistanceTurns = pNewCity->getPopulation() * (100 - iInfluenceReduction) / 100;
#ifdef REDUCE_RESISTANCE_TIME
			iResistanceTurns /= 2;
#endif

			if (iResistanceTurns > 0)
			{
				pNewCity->ChangeResistanceTurns(iResistanceTurns);
			}
		}

		long lResult = 0;

		if(lResult == 0)
		{
			PlayerTypes eLiberatedPlayer = NO_PLAYER;

			// Captured someone's city that didn't originally belong to us - Liberate a player?
			if(pNewCity->getOriginalOwner() != eOldOwner && pNewCity->getOriginalOwner() != GetID())
			{
				eLiberatedPlayer = pNewCity->getOriginalOwner();
#ifdef CANNOT_LIBERATE_GIFTED_CS
				if (!CanLiberatePlayerCity(eLiberatedPlayer) || (GET_PLAYER(eLiberatedPlayer).isMinorCiv() && bGift))
#else
				if(!CanLiberatePlayerCity(eLiberatedPlayer))
#endif
				{
					eLiberatedPlayer = NO_PLAYER;
				}
			}

			// AI decides what to do with a City
			if(!isHuman())
			{
				AI_conquerCity(pNewCity, eOldOwner); // could delete the pointer...
				// So we will check to see if the plot still contains the city.
				CvCity* pkCurrentCity = pCityPlot->getPlotCity();
				if (pkCurrentCity == NULL || pNewCity != pkCurrentCity || pkCurrentCity->getOwner() != GetID())
				{
					// The city is gone or is not ours anymore (we gave it away)
					pNewCity = NULL;
				}
			}

			// Human decides what to do with a City
			else if(!GC.getGame().isOption(GAMEOPTION_NO_HAPPINESS))
			{
				// Used to display info for annex/puppet/raze popup - turned off in DoPuppet and DoAnnex
				pNewCity->SetIgnoreCityForHappiness(true);
				if (GetPlayerTraits()->IsNoAnnexing() && bIsMinorCivBuyout)
				{
					pNewCity->DoCreatePuppet();
				}
				else if (pNewCity->getOriginalOwner() != GetID() || GetPlayerTraits()->IsNoAnnexing() || bIsMinorCivBuyout)
				{
					if(GC.getGame().getActivePlayer() == GetID())
					{
						int iTemp[5] = { pNewCity->GetID(), iCaptureGold, iCaptureCulture, iCaptureGreatWorks, eLiberatedPlayer };
						bool bTemp[2] = { bIsMinorCivBuyout, bConquest };
						GC.GetEngineUserInterface()->AddPopup(BUTTONPOPUP_CITY_CAPTURED, POPUP_PARAM_INT_ARRAY(iTemp), POPUP_PARAM_BOOL_ARRAY(bTemp));
						// We are adding a popup that the player must make a choice in, make sure they are not in the end-turn phase.
						CancelActivePlayerEndTurn();
					}
				}
				else
				{
					pNewCity->SetIgnoreCityForHappiness(false);
				}
			}

			// No choice but to capture it, tell about pillage gold (if any)
			else if(iCaptureGold > 0 || iCaptureCulture > 0 || iCaptureGreatWorks > 0)
			{
				if (iCaptureCulture == 0 && iCaptureGreatWorks == 0)
				{
					strBuffer = GetLocalizedText("TXT_KEY_POPUP_GOLD_CITY_CAPTURE", iCaptureGold, pNewCity->getNameKey());
					GC.GetEngineUserInterface()->AddCityMessage(0, pNewCity->GetIDInfo(), GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer);
				}
				else
				{
					strBuffer = GetLocalizedText("TXT_KEY_POPUP_GOLD_AND_CULTURE_CITY_CAPTURE", iCaptureGold, iCaptureCulture, iCaptureGreatWorks, pNewCity->getNameKey());
					GC.GetEngineUserInterface()->AddCityMessage(0, pNewCity->GetIDInfo(), GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer);

				}
			}
		}
	}

	// Be careful below here, pNewCity can be NULL.
	CheckForMurder(eOldOwner);

	if(GC.getGame().getActiveTeam() == GET_PLAYER(eOldOwner).getTeam())
	{
		CvMap& theMap = GC.getMap();
		theMap.updateDeferredFog();
	}
#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
	if (getNumCities() > 0)
	{
		for (int iYield = 0; iYield < GC.getNUM_YIELD_TYPES(); iYield++)
		{
			YieldTypes eYield = (YieldTypes)iYield;
			for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
			{
				eBuilding = eBuilding = (BuildingTypes)iI;
				CvBuildingEntry* pBuildingInfo = GC.getBuildingInfo(eBuilding);
				if (pBuildingInfo && pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield) > 0)
				{
					int iLoop = 0;
					int iNumBuildings = 0;
					int iNumSuppYields;
					int iNumYileds;
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						iNumBuildings += pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding);
					}
					if (pBuildingInfo->GetMaxYieldForEachBuildingInEmpire(eYield) >= 0)
					{
						iNumSuppYields = iNumBuildings * pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield);
						iNumYileds = std::min(pBuildingInfo->GetMaxYieldForEachBuildingInEmpire(eYield), iNumSuppYields);
					}
					else
					{
						iNumSuppYields = iNumBuildings * pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield);
						iNumYileds = iNumSuppYields;
					}
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						if (pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
						{
							pLoopCity->ChangeBaseYieldRateFromBuildings(eYield, iNumYileds);
						}
					}
				}
			}
		}
	}
#endif

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem && pNewCity != NULL)
	{
		CvLuaArgsHandle args;
		args->Push(eOldOwner);
		args->Push(bCapital);
		args->Push(pNewCity->getX());
		args->Push(pNewCity->getY());
		args->Push(GetID());
		args->Push(iOldPopulation);
		args->Push(bConquest);
		args->Push((int)paGreatWorkData.size());
		args->Push(iCaptureGreatWorks);

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "CityCaptureComplete", args.get(), bResult);
	}
#ifdef _MSC_VER
#pragma warning ( pop ) // restore warning level suppressed for pNewCity null check
#endif// _MSC_VER
}


//	--------------------------------------------------------------------------------
void CvPlayer::killCities()
{
	CvCity* pLoopCity;
	int iLoop;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->kill();
	}
}

const int RESERVE_TOP_X_NAMES = 5;	/// Never steal one of the first 5 names

//	--------------------------------------------------------------------------------
CvString CvPlayer::getNewCityName() const
{
	const CLLNode<CvString>* pNode;
	CvString strName;

	for(pNode = headCityNameNode(); (pNode != NULL); pNode = nextCityNameNode(pNode))
	{
		strName = pNode->m_data;
		if(isCityNameValid(strName, true))
		{
			strName = pNode->m_data;
			break;
		}
	}

	if(strName.IsEmpty())
	{
		getCivilizationCityName(strName, getCivilizationType());
	}

	if(strName.IsEmpty())
	{
		// Pick a name from another civ in the game
		int iPlayersAlive = 0;
		for(int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			const PlayerTypes ePlayer = static_cast<PlayerTypes>(iI);
			CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);
			if(ePlayer != GetID() && kPlayer.isAlive() && !kPlayer.isMinorCiv() && !kPlayer.isBarbarian())
			{
				iPlayersAlive++;
			}
		}

		int iChosenPlayer = GC.getGame().getJonRandNum(iPlayersAlive, "Random Player To Steal City Name");

		int iPlayersFound = 0;
		for(int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			const PlayerTypes ePlayer = static_cast<PlayerTypes>(iI);
			CvPlayerAI &kPlayer = GET_PLAYER(ePlayer);
			if(ePlayer != GetID() && kPlayer.isAlive() && !kPlayer.isMinorCiv() && !kPlayer.isBarbarian())
			{
				if(iPlayersFound == iChosenPlayer)
				{
					strName = GetBorrowedCityName(kPlayer.getCivilizationType());			
					break;
				}
				else
				{
					iPlayersFound++;
				}
			}
		}
	}

	if(strName.IsEmpty())
	{
		// Pick a name from another civ in the DATABASE
		int iCivsInDB = 0;
		for(int iI = 0; iI < GC.getNumCivilizationInfos(); iI++)
		{
			const CivilizationTypes eCiv = static_cast<CivilizationTypes>(iI);

			CvCivilizationInfo* pkCivilizationInfo = GC.getCivilizationInfo(eCiv);
			if(pkCivilizationInfo != NULL && pkCivilizationInfo->getNumCityNames() > RESERVE_TOP_X_NAMES)
			{
				iCivsInDB++;
			}
		}

		int iChosenCiv = GC.getGame().getJonRandNum(iCivsInDB, "Random Civ To Steal City Name");

		int iCivsFound = 0;
		for(int iI = 0; iI < GC.getNumCivilizationInfos(); iI++)
		{
			const CivilizationTypes eCiv = static_cast<CivilizationTypes>(iI);

			CvCivilizationInfo* pkCivilizationInfo = GC.getCivilizationInfo(eCiv);
			if (pkCivilizationInfo != NULL && pkCivilizationInfo->getNumCityNames() > RESERVE_TOP_X_NAMES)
			{
				if (iCivsFound == iChosenCiv)
				{
					strName = GetBorrowedCityName(eCiv);
					break;
				}
				else
				{
					iCivsFound++;
				}
			}
		}
	}

	if(strName.IsEmpty())
	{
		strName = "TXT_KEY_CITY";
	}

	return strName;
}

//	--------------------------------------------------------------------------------
CvString CvPlayer::GetBorrowedCityName(CivilizationTypes eCivToBorrowFrom) const
{
	CvString szRtnValue;
	CvCivilizationInfo *pCivInfo = GC.getCivilizationInfo(eCivToBorrowFrom);

	if (pCivInfo)
	{
		int iRange = pCivInfo->getNumCityNames() - RESERVE_TOP_X_NAMES;
		int iRandOffset = GC.getGame().getJonRandNum(iRange, "Random City Name To Steal");
		for(int iI = 0; iI < iRange; iI++)     
		{
			CvString strCityName = pCivInfo->getCityNames(RESERVE_TOP_X_NAMES + ((iI + iRandOffset) % iRange));
			szRtnValue = GetLocalizedText(strCityName.c_str());

			if(isCityNameValid(szRtnValue, true))
			{
				break;
			}
		}
	}

	return szRtnValue;

}

//	--------------------------------------------------------------------------------
void CvPlayer::getCivilizationCityName(CvString& szBuffer, CivilizationTypes eCivilization) const
{
	int iRandOffset;
	int iLoopName;

	CvCivilizationInfo* pkCivilizationInfo = GC.getCivilizationInfo(eCivilization);
	if(pkCivilizationInfo == NULL)
	{
		//This should never happen.
		return;
	}

	if(isBarbarian())
	{
		iRandOffset = GC.getGame().getJonRandNum(pkCivilizationInfo->getNumCityNames(), "Random Barb Name");
	}
	else
	{
		iRandOffset = 0;
	}

	// Minor Civs use special lists
	if(isMinorCiv())
	{
		CvMinorCivInfo* pkMinorCivInfo = GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType());
		if(pkMinorCivInfo)
		{
			CvMinorCivInfo& kMinorCivInfo = *pkMinorCivInfo;
			for(int iI = 0; iI < kMinorCivInfo.getNumCityNames(); iI++)
			{
				iLoopName = ((iI + iRandOffset) % kMinorCivInfo.getNumCityNames());

				const CvString strCityName = kMinorCivInfo.getCityNames(iLoopName);
				CvString strName = GetLocalizedText(strCityName.c_str());

				if(isCityNameValid(strName, true))
				{
					szBuffer = strCityName;
					break;
				}
			}
		}
	}
	else
	{
		CvCivilizationInfo& kCivInfo = *pkCivilizationInfo;
		for(int iI = 0; iI < kCivInfo.getNumCityNames(); iI++)
		{
			iLoopName = ((iI + iRandOffset) % kCivInfo.getNumCityNames());

			const CvString strCityName = kCivInfo.getCityNames(iLoopName);
			CvString strName = GetLocalizedText(strCityName.c_str());

			if(isCityNameValid(strName, true))
			{
				szBuffer = strCityName;
				break;
			}
		}
	}
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isCityNameValid(CvString& szName, bool bTestDestroyed) const
{
	const CvCity* pLoopCity;
	int iLoop;

	if(bTestDestroyed)
	{
		if(GC.getGame().isDestroyedCityName(szName))
		{
			return false;
		}

		for(int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			for(pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if(pLoopCity->getName() == szName)
				{
					return false;
				}
			}
		}
	}
	else
	{
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if(pLoopCity->getName() == szName)
			{
				return false;
			}
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
/// This player liberates iOldCityID and gives it back to ePlayer
void CvPlayer::DoLiberatePlayer(PlayerTypes ePlayer, int iOldCityID)
{
	CvCity* pCity = getCity(iOldCityID);
	CvAssert(pCity);
	if (!pCity)
		return;

	PlayerTypes eOldOwner = pCity->getOwner();
	CvPlot* pPlot = pCity->plot();

	// Set that this team has been liberated
	TeamTypes eTeam = getTeam();
	TeamTypes eLiberatedTeam = GET_PLAYER(ePlayer).getTeam();

	// Who originally took out this team?
	TeamTypes eConquerorTeam = GET_TEAM(eLiberatedTeam).GetKilledByTeam();

	if (!GET_PLAYER(ePlayer).isAlive())
	{
		GET_PLAYER(ePlayer).setBeingResurrected(true);
		GET_TEAM(eLiberatedTeam).SetLiberatedByTeam(eTeam);

		// Put everyone at peace with this guy
		for(int iOtherTeamLoop = 0; iOtherTeamLoop < MAX_CIV_TEAMS; iOtherTeamLoop++)
		{
			if(eLiberatedTeam != iOtherTeamLoop)
			{
				GET_TEAM(eLiberatedTeam).makePeace((TeamTypes) iOtherTeamLoop, /*bBumpUnits*/false, /*bSuppressNotification*/true);
			}
		}
	
		if (!GET_PLAYER(ePlayer).isMinorCiv())
		{
			// add notification
			Localization::String strMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_CIV_RESURRECTED");
			strMessage << getCivilizationShortDescriptionKey(); // LIBERATING CIV NAME
			strMessage << pCity->getNameKey(); // CITY NAME
			strMessage << GET_PLAYER(ePlayer).getCivilizationAdjectiveKey(); // LIBERATED CIV NAME
			strMessage << GET_PLAYER(ePlayer).getCivilizationDescriptionKey();// LIBERATED CIV NAME
			Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_CIV_RESURRECTED_SHORT");
			if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(ePlayer).isHuman())
			{
				strSummary << GET_PLAYER(ePlayer).getNickName();
			}
			else
			{
				strSummary << GET_PLAYER(ePlayer).getNameKey();
			}
			if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(m_eID).isHuman())
			{
				strSummary << GET_PLAYER(m_eID).getNickName();
			}
			else
			{
				strSummary << GET_PLAYER(m_eID).getNameKey();
			}			

			for(int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				const PlayerTypes eOtherPlayer = static_cast<PlayerTypes>(iI);
				CvPlayerAI& kOtherPlayer = GET_PLAYER(eOtherPlayer);
				if(kOtherPlayer.isAlive() && kOtherPlayer.GetNotifications() && iI != m_eID)
				{
					kOtherPlayer.GetNotifications()->Add(NOTIFICATION_RESURRECTED_MAJOR_CIV, strMessage.toUTF8(), strSummary.toUTF8(), pCity->getX(), pCity->getY(), -1);
				}
			}

			CvString temp = strMessage.toUTF8();
			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, m_eID, temp);
		}
	}
	else
	{
		GET_PLAYER(ePlayer).GetDiplomacyAI()->ChangeNumCitiesLiberated(m_eID, 1);

		if (!GET_PLAYER(ePlayer).isMinorCiv())
		{
			// add notification
			Localization::String strMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_LIBERATED");
			if(GC.getGame().isGameMultiPlayer() && isHuman())
			{
				strMessage << getNickName();
			}
			else
			{
				strMessage << getNameKey();
			}
			strMessage << pCity->getNameKey(); // CITY NAME
			strMessage << GET_PLAYER(ePlayer).getCivilizationShortDescriptionKey();// RESTORED CIV NAME
			Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_CIV_LIBERATED_SHORT");
			strSummary << pCity->getNameKey();
			if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(ePlayer).isHuman())
			{
				strSummary << GET_PLAYER(ePlayer).getNickName();
			}
			else
			{
				strSummary << GET_PLAYER(ePlayer).getNameKey();
			}

			for(int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				const PlayerTypes eOtherPlayer = static_cast<PlayerTypes>(iI);
				CvPlayerAI& kOtherPlayer = GET_PLAYER(eOtherPlayer);
				if(kOtherPlayer.isAlive() && kOtherPlayer.GetNotifications() && iI != m_eID)
				{
					kOtherPlayer.GetNotifications()->Add(NOTIFICATION_LIBERATED_MAJOR_CITY, strMessage.toUTF8(), strSummary.toUTF8(), pCity->getX(), pCity->getY(), -1);
				}
			}
		}
	}

	// Give the city back to the liberated player
	GET_PLAYER(ePlayer).acquireCity(pCity, false, true);

	if (!GET_PLAYER(ePlayer).isMinorCiv())
	{
		// slewis - if the player we're liberating the city for is dead, give the liberating player a resurrection mark in the once-defeated player's book
		if (!GET_PLAYER(ePlayer).isAlive())
		{
			CvDiplomacyAI* pDiploAI = GET_PLAYER(ePlayer).GetDiplomacyAI();
			PlayerTypes eMePlayer = GetID();
			pDiploAI->SetResurrectedBy(eMePlayer, true);
			pDiploAI->SetLandDisputeLevel(eMePlayer, DISPUTE_LEVEL_NONE);
			pDiploAI->SetWonderDisputeLevel(eMePlayer, DISPUTE_LEVEL_NONE);
			pDiploAI->SetMinorCivDisputeLevel(eMePlayer, DISPUTE_LEVEL_NONE);
			pDiploAI->SetWarmongerThreat(eMePlayer, THREAT_NONE);

			pDiploAI->SetPlayerNoSettleRequestCounter(eMePlayer, -1);
			pDiploAI->SetPlayerStopSpyingRequestCounter(eMePlayer, -1);
			pDiploAI->SetDemandCounter(eMePlayer, -1);
			pDiploAI->ChangeNumTimesCultureBombed(eMePlayer, -pDiploAI->GetNumTimesCultureBombed(eMePlayer));
			pDiploAI->ChangeNegativeReligiousConversionPoints(eMePlayer, -pDiploAI->GetNegativeReligiousConversionPoints(eMePlayer));
			pDiploAI->ChangeNegativeArchaeologyPoints(eMePlayer, -pDiploAI->GetNegativeArchaeologyPoints(eMePlayer));

			pDiploAI->ChangeNumTimesRobbedBy(eMePlayer, -pDiploAI->GetNumTimesRobbedBy(eMePlayer));
			pDiploAI->SetPlayerBrokenMilitaryPromise(eMePlayer, false);
			pDiploAI->SetPlayerIgnoredMilitaryPromise(eMePlayer, false);
			pDiploAI->SetBrokenBorderPromiseValue(eMePlayer, 0);
			pDiploAI->SetIgnoredBorderPromiseValue(eMePlayer, 0);
			pDiploAI->SetBrokenExpansionPromiseValue(eMePlayer, 0);
			pDiploAI->SetIgnoredExpansionPromiseValue(eMePlayer, 0);

			pDiploAI->SetPlayerBrokenAttackCityStatePromise(eMePlayer, false);
			pDiploAI->SetPlayerIgnoredAttackCityStatePromise(eMePlayer, false);
			pDiploAI->SetPlayerBrokenBullyCityStatePromise(eMePlayer, false);
			pDiploAI->SetPlayerIgnoredBullyCityStatePromise(eMePlayer, false);

			pDiploAI->SetPlayerBrokenNoConvertPromise(eMePlayer, false);
			pDiploAI->SetPlayerIgnoredNoConvertPromise(eMePlayer, false);

			pDiploAI->SetPlayerBrokenNoDiggingPromise(eMePlayer, false);
			pDiploAI->SetPlayerIgnoredNoDiggingPromise(eMePlayer, false);

			pDiploAI->SetPlayerBrokenSpyPromise(eMePlayer, false);
			pDiploAI->SetPlayerIgnoredSpyPromise(eMePlayer, false);

			pDiploAI->SetPlayerBrokenCoopWarPromise(eMePlayer, false);

			pDiploAI->SetOtherPlayerNumProtectedMinorsKilled(eMePlayer, 0);
			pDiploAI->SetOtherPlayerNumProtectedMinorsAttacked(eMePlayer, 0);
			pDiploAI->SetOtherPlayerNumProtectedMinorsBullied(eMePlayer, 0);
			pDiploAI->SetOtherPlayerTurnsSinceSidedWithProtectedMinor(eMePlayer, -1);

			pDiploAI->SetFriendDenouncedUs(eMePlayer, false);
			pDiploAI->SetDenouncedPlayer(eMePlayer, false); // forget any denouncing
			GetDiplomacyAI()->SetDenouncedPlayer(ePlayer, false); // forget any denouncing
			pDiploAI->SetFriendDeclaredWarOnUs(eMePlayer, false);

			pDiploAI->ChangeNumTimesNuked(eMePlayer, -pDiploAI->GetNumTimesNuked(eMePlayer));
		}
	}

	// Now verify the player is alive
	GET_PLAYER(ePlayer).verifyAlive();
	GET_PLAYER(ePlayer).setBeingResurrected(false);

	// Is this a Minor we have liberated?
	if(GET_PLAYER(ePlayer).isMinorCiv())
	{
		GET_PLAYER(ePlayer).GetMinorCivAI()->DoLiberationByMajor(eOldOwner, eConquerorTeam);
#ifdef UPDATE_MINOR_TECHS_ON_LIBERATION
		GET_TEAM(GET_PLAYER(ePlayer).getTeam()).DoMinorCivTech();
#endif
	}

	// slewis
	// negate warmonger
	for(int iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
	{
		PlayerTypes eMajor = (PlayerTypes)iMajorLoop;
		if(GetID() != eMajor && GET_PLAYER(eMajor).isAlive())
		{
			// Have I met the player who conquered the city?
			if(GET_TEAM(GET_PLAYER(eMajor).getTeam()).isHasMet(getTeam()))
			{
				int iNumCities = max(GET_PLAYER(ePlayer).getNumCities(), 1);
				int iWarmongerOffset = CvDiplomacyAIHelpers::GetWarmongerOffset(iNumCities, GET_PLAYER(ePlayer).isMinorCiv());
				GET_PLAYER(eMajor).GetDiplomacyAI()->ChangeOtherPlayerWarmongerAmount(GetID(), -iWarmongerOffset);
			}
		}
	}

	// Move Units from player that don't belong here
#ifdef NQ_NEVER_PUSH_OUT_OF_MINORS_ON_PEACE
	if(pPlot && pPlot->getNumUnits() > 0 && !GET_PLAYER(ePlayer).isMinorCiv())
#else
	if(pPlot->getNumUnits() > 0)
#endif
	{
		// Get the current list of units because we will possibly be moving them out of the plot's list
		IDInfoVector currentUnits;
		if (pPlot->getUnits(&currentUnits) > 0)
		{
			for(IDInfoVector::const_iterator itr = currentUnits.begin(); itr != currentUnits.end(); ++itr)
			{
				CvUnit* pLoopUnit = (CvUnit*)GetPlayerUnit(*itr);

				if(pLoopUnit && pLoopUnit->getOwner() == eOldOwner)
				{
					pLoopUnit->finishMoves();
					if (!pLoopUnit->jumpToNearestValidPlot())
						pLoopUnit->kill(false);
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::CanLiberatePlayer(PlayerTypes ePlayer)
{
	// Other Player must be dead now
	if(GET_PLAYER(ePlayer).isAlive())
	{
		return false;
	}

	if(GET_PLAYER(ePlayer).IsEverConqueredBy(m_eID))
	{
		return false;
	}

	if(GET_TEAM(GET_PLAYER(ePlayer).getTeam()).GetKilledByTeam() == getTeam())
	{
		return false;
	}

	return true;
}

#ifdef NEW_CITIES_LIBERATION
//	--------------------------------------------------------------------------------
bool CvPlayer::CanLiberatePlayerCity(PlayerTypes ePlayer)
{
	if (GET_PLAYER(ePlayer).isHuman())
	{
		return true;
	}

	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
	CvGame& kGame = GC.getGame();

	if (pLeague != NULL && kGame.isOption("GAMEOPTION_RESTRICTION_ON_LIBERATION_OF_CS"))
	{
		if (GET_PLAYER(ePlayer).isMinorCiv())
		{
			EraTypes eHighestEra = GET_PLAYER(ePlayer).GetCurrentEra();
			int iMaxStartingVotesForMember = pLeague->CalculateStartingVotesForMember(ePlayer);

			for (int iI = 0; iI < MAX_TEAMS; iI++)
			{
				if (GET_TEAM((TeamTypes)iI).isAlive())
				{
					if (GET_TEAM((TeamTypes)iI).GetCurrentEra() > eHighestEra)
					{
						eHighestEra = GET_TEAM((TeamTypes)iI).GetCurrentEra();
					}
				}
			}

			if (eHighestEra >= GC.getInfoTypeForString("ERA_FUTURE", true /*bHideAssert*/))
			{
				for (int iI = 0; iI < MAX_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive())
					{
						if (pLeague->CalculateStartingVotesForMember((PlayerTypes)iI) > iMaxStartingVotesForMember)
						{
							iMaxStartingVotesForMember = pLeague->CalculateStartingVotesForMember((PlayerTypes)iI);
						}
					}
				}

				PolicyTypes ePolicy = (PolicyTypes)GC.getInfoTypeForString("POLICY_TREATY_ORGANIZATION", true /*bHideAssert*/);
				if (pLeague->CalculateStartingVotesForMember(GetID()) == iMaxStartingVotesForMember || GET_PLAYER((PlayerTypes)GetID()).GetPlayerPolicies()->HasPolicy(ePolicy))
				{
					return CanLiberatePlayer(ePlayer);
				}
			}
			else
			{
				return CanLiberatePlayer(ePlayer);
			}
		}
	}
	else
	{
		if (GET_PLAYER(ePlayer).isMinorCiv())
		{
			return CanLiberatePlayer(ePlayer);
		}
	}

	return false;
}

#else
//	--------------------------------------------------------------------------------
bool CvPlayer::CanLiberatePlayerCity(PlayerTypes ePlayer)
{
	if(!GET_PLAYER(ePlayer).isAlive())
	{
		return CanLiberatePlayer(ePlayer);
	}

	return true;
}
#endif

//	--------------------------------------------------------------------------------
#ifdef UNIT_UPGRADE_NUM_RESOURE_USED_CHANGE
CvUnit* CvPlayer::initUnit(UnitTypes eUnit, int iX, int iY, UnitAITypes eUnitAI, DirectionTypes eFacingDirection, bool bNoMove, bool bSetupGraphical, int iMapLayer /* = 0 */, int iNumGoodyHutsPopped, bool bIsUpgrade)
#else
CvUnit* CvPlayer::initUnit(UnitTypes eUnit, int iX, int iY, UnitAITypes eUnitAI, DirectionTypes eFacingDirection, bool bNoMove, bool bSetupGraphical, int iMapLayer /* = 0 */, int iNumGoodyHutsPopped)
#endif
{
	CvAssertMsg(eUnit != NO_UNIT, "Unit is not assigned a valid value");
	if (eUnit == NO_UNIT)
		return NULL;

	CvUnitEntry* pkUnitDef = GC.getUnitInfo(eUnit);
	CvAssertFmt(pkUnitDef != NULL, "Trying to create unit of type %d, which does not exist", eUnit);
	if (pkUnitDef == NULL)
		return NULL;

	CvUnit* pUnit = addUnit();
	CvAssertMsg(pUnit != NULL, "Unit is not assigned a valid value");
	if(NULL != pUnit)
	{
#ifdef UNIT_UPGRADE_NUM_RESOURE_USED_CHANGE
		pUnit->init(pUnit->GetID(), eUnit, ((eUnitAI == NO_UNITAI) ? ((UnitAITypes)(pkUnitDef->GetDefaultUnitAIType())) : eUnitAI), GetID(), iX, iY, eFacingDirection, bNoMove, bSetupGraphical, iMapLayer, iNumGoodyHutsPopped, bIsUpgrade);
#else
		pUnit->init(pUnit->GetID(), eUnit, ((eUnitAI == NO_UNITAI) ? ((UnitAITypes)(pkUnitDef->GetDefaultUnitAIType())) : eUnitAI), GetID(), iX, iY, eFacingDirection, bNoMove, bSetupGraphical, iMapLayer, iNumGoodyHutsPopped);
#endif

		// slewis - added for the tutorial
		if(pUnit->getUnitInfo().GetWorkRate() > 0 && pUnit->getUnitInfo().GetDomainType() == DOMAIN_LAND)
		{
			m_bEverTrainedBuilder = true;
		}
		// end added for the tutorial
	}

	m_kPlayerAchievements.AddUnit(pUnit);

	return pUnit;
}

#ifdef UNIT_UPGRADE_NUM_RESOURE_USED_CHANGE
CvUnit* CvPlayer::initUnitWithNameOffset(UnitTypes eUnit, int nameOffset, int iX, int iY, UnitAITypes eUnitAI, DirectionTypes eFacingDirection, bool bNoMove, bool bSetupGraphical, int iMapLayer /* = 0 */, int iNumGoodyHutsPopped, bool bIsUpgrade)
#else
CvUnit* CvPlayer::initUnitWithNameOffset(UnitTypes eUnit, int nameOffset, int iX, int iY, UnitAITypes eUnitAI, DirectionTypes eFacingDirection, bool bNoMove, bool bSetupGraphical, int iMapLayer /* = 0 */, int iNumGoodyHutsPopped)
#endif
{
	CvAssertMsg(eUnit != NO_UNIT, "Unit is not assigned a valid value");
	if (eUnit == NO_UNIT)
		return NULL;

	CvUnitEntry* pkUnitDef = GC.getUnitInfo(eUnit);
	CvAssertFmt(pkUnitDef != NULL, "Trying to create unit of type %d, which does not exist", eUnit);
	if (pkUnitDef == NULL)
		return NULL;

	CvUnit* pUnit = addUnit();
	CvAssertMsg(pUnit != NULL, "Unit is not assigned a valid value");
	if(NULL != pUnit)
	{
#ifdef UNIT_UPGRADE_NUM_RESOURE_USED_CHANGE
		pUnit->initWithNameOffset(pUnit->GetID(), eUnit, nameOffset, ((eUnitAI == NO_UNITAI) ? ((UnitAITypes)(pkUnitDef->GetDefaultUnitAIType())) : eUnitAI), GetID(), iX, iY, eFacingDirection, bNoMove, bSetupGraphical, iMapLayer, iNumGoodyHutsPopped, bIsUpgrade);
#else
		pUnit->initWithNameOffset(pUnit->GetID(), eUnit, nameOffset, ((eUnitAI == NO_UNITAI) ? ((UnitAITypes)(pkUnitDef->GetDefaultUnitAIType())) : eUnitAI), GetID(), iX, iY, eFacingDirection, bNoMove, bSetupGraphical, iMapLayer, iNumGoodyHutsPopped);
#endif

		// slewis - added for the tutorial
		if(pUnit->getUnitInfo().GetWorkRate() > 0 && pUnit->getUnitInfo().GetDomainType() == DOMAIN_LAND)
		{
			m_bEverTrainedBuilder = true;
		}
		// end added for the tutorial
	}

	m_kPlayerAchievements.AddUnit(pUnit);

	return pUnit;
}

//	--------------------------------------------------------------------------------
void CvPlayer::disbandUnit(bool)
{
	CvUnit* pLoopUnit;
	CvUnit* pBestUnit;
	char szBuffer[1024];
	const size_t lenBuffer = 1024;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = INT_MAX;
	pBestUnit = NULL;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if(!(pLoopUnit->hasCargo()))
		{
			if(!(pLoopUnit->isGoldenAge()))
			{
				if(pLoopUnit->getUnitInfo().GetProductionCost() > 0)
				{
					{
						iValue = (10000 + GC.getGame().getJonRandNum(1000, "Disband Unit"));

						iValue += (pLoopUnit->getUnitInfo().GetProductionCost() * 5);

						iValue += (pLoopUnit->getExperience() * 20);
						iValue += (pLoopUnit->getLevel() * 100);

						if(pLoopUnit->IsCanDefend() && pLoopUnit->plot()->isCity())
						{
							iValue *= 2;
						}

						if(pLoopUnit->plot()->getTeam() == pLoopUnit->getTeam())
						{
							iValue *= 3;
						}

						switch(pLoopUnit->AI_getUnitAIType())
						{
						case UNITAI_UNKNOWN:
							break;

						case UNITAI_SETTLE:
							iValue *= 20;
							break;

						case UNITAI_WORKER:
							iValue *= 10;
							break;

						case UNITAI_ATTACK:
						case UNITAI_CITY_BOMBARD:
						case UNITAI_FAST_ATTACK:
						case UNITAI_DEFENSE:
						case UNITAI_COUNTER:
							iValue *= 2;
							break;

						case UNITAI_RANGED:
						case UNITAI_CITY_SPECIAL:
						case UNITAI_PARADROP:
							iValue *= 6;
							break;

						case UNITAI_EXPLORE:
							iValue *= 15;
							break;

						case UNITAI_ARTIST:
						case UNITAI_SCIENTIST:
						case UNITAI_GENERAL:
						case UNITAI_MERCHANT:
						case UNITAI_ENGINEER:
						case UNITAI_SPACESHIP_PART:
						case UNITAI_TREASURE:
						case UNITAI_PROPHET:
						case UNITAI_MISSIONARY:
						case UNITAI_INQUISITOR:
						case UNITAI_ADMIRAL:
						case UNITAI_WRITER:
						case UNITAI_MUSICIAN:
							break;

						case UNITAI_ICBM:
							iValue *= 4;
							break;

						case UNITAI_WORKER_SEA:
							iValue *= 18;
							break;

						case UNITAI_ATTACK_SEA:
						case UNITAI_RESERVE_SEA:
						case UNITAI_ESCORT_SEA:
							break;

						case UNITAI_EXPLORE_SEA:
							iValue *= 25;
							break;

						case UNITAI_ASSAULT_SEA:
						case UNITAI_SETTLER_SEA:
						case UNITAI_CARRIER_SEA:
						case UNITAI_MISSILE_CARRIER_SEA:
							iValue *= 5;
							break;

						case UNITAI_PIRATE_SEA:
						case UNITAI_ATTACK_AIR:
							break;

						case UNITAI_DEFENSE_AIR:
						case UNITAI_CARRIER_AIR:
						case UNITAI_MISSILE_AIR:
							iValue *= 3;
							break;

						default:
							CvAssert(false);
							break;
						}

						if(pLoopUnit->getUnitInfo().GetExtraMaintenanceCost() > 0)
						{
							iValue /= (pLoopUnit->getUnitInfo().GetExtraMaintenanceCost() + 1);
						}

						if(iValue < iBestValue)
						{
							iBestValue = iValue;
							pBestUnit = pLoopUnit;
						}
					}
				}
			}
		}
	}

	if(pBestUnit != NULL)
	{
		if(GetID() == GC.getGame().getActivePlayer())
		{
			sprintf_s(szBuffer, lenBuffer, GetLocalizedText("TXT_KEY_MISC_UNIT_DISBANDED_NO_MONEY", pBestUnit->getNameKey()).GetCString());
			GC.GetEngineUserInterface()->AddUnitMessage(0, pBestUnit->GetIDInfo(), GetID(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer);//, "AS2D_UNITDISBANDED", MESSAGE_TYPE_MINOR_EVENT, pBestUnit->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pBestUnit->getX(), pBestUnit->getY(), true, true);
		}

		CvAssert(!(pBestUnit->isGoldenAge()));

		pBestUnit->kill(false);
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::killUnits()
{
	CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		pLoopUnit->kill(false);
	}
}

//	--------------------------------------------------------------------------------
CvPlot *CvPlayer::GetGreatAdmiralSpawnPlot (CvUnit *pUnit)
{
	CvPlot *pInitialPlot = pUnit->plot();

	// Is this a friendly coastal city, if so we'll go with that
	CvCity *pInitialCity = pInitialPlot->getPlotCity();
	if (pInitialCity && pInitialCity->isCoastal(GC.getLAKE_MAX_AREA_SIZE()))
	{
		// Equal okay checking this plot because this is where the unit is right now
		if (pInitialPlot->getNumFriendlyUnitsOfType(pUnit) <= GC.getPLOT_UNIT_LIMIT())
		{
			return pInitialPlot;
		}
	}

	// Otherwise let's look at all our other cities
	CvCity *pLoopCity;
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity != pInitialCity)
		{
			if (pLoopCity->isCoastal(GC.getLAKE_MAX_AREA_SIZE()))
			{
				if (pLoopCity->plot()->getNumFriendlyUnitsOfType(pUnit) < GC.getPLOT_UNIT_LIMIT())
				{
					return pLoopCity->plot();
				}
			}
		}
	}

	// Don't have a coastal city, look for water plot THAT ISN'T A LAKE closest to our capital that isn't owned by an enemy
	int iCapitalX;
	int iCapitalY;
	CvCity *pCapital = getCapitalCity();
	if (pCapital)
	{
		iCapitalX = pCapital->getX();
		iCapitalY = pCapital->getY();

		CvPlot *pBestPlot = NULL;
		int iBestDistance = MAX_INT;

		for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
		{
			CvPlot *pPlot = GC.getMap().plotByIndexUnchecked(iI);
			if (pPlot != NULL)
			{
				if (pPlot->isWater() && !pPlot->isLake())
				{
					if (pPlot->IsFriendlyTerritory(GetID()) || !pPlot->isOwned())
					{
						if (pPlot->getNumFriendlyUnitsOfType(pUnit) < GC.getPLOT_UNIT_LIMIT())
						{
							int iDistance = plotDistance(iCapitalX, iCapitalY, pPlot->getX(), pPlot->getY());
							if (iDistance < iBestDistance)
							{
								pBestPlot = pPlot;
								iBestDistance = iDistance;
							}
						}
					}
				}
			}
		}

		if (pBestPlot)
		{
			return pBestPlot;
		}

		// Now we'll even accept a lake
		for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
		{
			CvPlot *pPlot = GC.getMap().plotByIndexUnchecked(iI);
			if (pPlot != NULL)
			{
				if (pPlot->isWater())
				{
					if (pPlot->IsFriendlyTerritory(GetID()) || !pPlot->isOwned())
					{
						if (pPlot->getNumFriendlyUnitsOfType(pUnit) < GC.getPLOT_UNIT_LIMIT())
						{
							int iDistance = plotDistance(iCapitalX, iCapitalY, pPlot->getX(), pPlot->getY());
							if (iDistance < iBestDistance)
							{
								pBestPlot = pPlot;
								iBestDistance = iDistance;
							}
						}
					}
				}
			}
		}
		if (pBestPlot)
		{
			return pBestPlot;
		}
	}

	CvAssertMsg (false, "Could not find valid plot for Great Admiral - placing on land");

	return pInitialPlot;
}


//	--------------------------------------------------------------------------------
/// The number of Builders a player has
int CvPlayer::GetNumBuilders() const
{
	return m_iNumBuilders;
}

//	--------------------------------------------------------------------------------
/// Sets the number of Builders a player has
void CvPlayer::SetNumBuilders(int iNum)
{
	if(GetNumBuilders() != iNum)
	{
		m_iNumBuilders = iNum;
	}
}

//	--------------------------------------------------------------------------------
/// Changes the number of Builders a player has
void CvPlayer::ChangeNumBuilders(int iChange)
{
	if(iChange != 0)
	{
		SetNumBuilders(GetNumBuilders() + iChange);
	}
}


//	--------------------------------------------------------------------------------
/// The maximum number of Builders a player can Train
int CvPlayer::GetMaxNumBuilders() const
{
	return m_iMaxNumBuilders;
}

//	--------------------------------------------------------------------------------
/// Sets the maximum number of Builders a player can Train
void CvPlayer::SetMaxNumBuilders(int iNum)
{
	if(GetMaxNumBuilders() != iNum)
	{
		m_iMaxNumBuilders = iNum;
	}
}

//	--------------------------------------------------------------------------------
/// Changes the maximum number of Builders a player can Train
void CvPlayer::ChangeMaxNumBuilders(int iChange)
{
	if(iChange != 0)
	{
		SetMaxNumBuilders(GetMaxNumBuilders() + iChange);
	}
}


//	--------------------------------------------------------------------------------
/// Returns number of Units a player has with a particular UnitAI.  The second argument allows you to check whether or not to include Units currently being trained in Cities.
int CvPlayer::GetNumUnitsWithUnitAI(UnitAITypes eUnitAIType, bool bIncludeBeingTrained, bool bIncludeWater)
{
	int iNumUnits = 0;

	CvUnit* pLoopUnit;
	CvCity* pLoopCity;
	int iLoop;

	// Current Units
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		// Don't include Water Units if we don't want them
		if(pLoopUnit->getDomainType() != DOMAIN_SEA || bIncludeWater)
		{
			if(pLoopUnit->AI_getUnitAIType() == eUnitAIType)
			{
				iNumUnits++;
			}
		}
	}

	// Units being trained now
	if(bIncludeBeingTrained)
	{
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if(pLoopCity->isProductionUnit())
			{
				CvUnitEntry* pkUnitEntry = GC.getUnitInfo(pLoopCity->getProductionUnit());
				if(pkUnitEntry)
				{
					// Don't include Water Units if we don't want them
					if(pkUnitEntry->GetDomainType() != DOMAIN_SEA || bIncludeWater)
					{
						if(pkUnitEntry->GetDefaultUnitAIType() == eUnitAIType)
						{
							iNumUnits++;
						}
					}
				}
			}
		}
	}

	return iNumUnits;
}

//	--------------------------------------------------------------------------------
/// Returns number of Units a player has of a particular domain.  The second argument allows you to check whether or not to include civilians.
int CvPlayer::GetNumUnitsWithDomain(DomainTypes eDomain, bool bMilitaryOnly)
{
	int iNumUnits = 0;

	CvUnit* pLoopUnit;
	int iLoop;

	// Current Units
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->getDomainType() == eDomain)
		{
			if(!bMilitaryOnly || pLoopUnit->IsCombatUnit())
			{
				iNumUnits++;
			}
		}
	}

	return iNumUnits;
}

//	-----------------------------------------------------------------------------------------------
int CvPlayer::GetNumUnitsWithUnitCombat(UnitCombatTypes eUnitCombat)
{
	int iNumUnits = 0;

	CvUnit* pLoopUnit;
	int iLoop;

	// Current Units
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->getUnitCombatType() == eUnitCombat)
		{
			iNumUnits++;
		}
	}

	return iNumUnits;
}

//	-----------------------------------------------------------------------------------------------
/// Setting up danger plots
void CvPlayer::InitDangerPlots()
{
	m_pDangerPlots->Init(GetID(), true /*bAllocate*/);
}

//	-----------------------------------------------------------------------------------------------
void CvPlayer::UpdateDangerPlots()
{
	m_pDangerPlots->UpdateDanger(false, false);
}

//	-----------------------------------------------------------------------------------------------
void CvPlayer::SetDangerPlotsDirty()
{
	m_pDangerPlots->SetDirty();
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isHuman() const
{
	if(GetID() == NO_PLAYER)
	{
		return false;
	}

	return CvPreGame::isHuman(GetID());
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isObserver() const
{
	if(GetID() == NO_PLAYER)
	{
		return false;
	}

	return CvPreGame::slotStatus(GetID()) == SS_OBSERVER;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isBarbarian() const
{
	return (GetID() == BARBARIAN_PLAYER);
}

//	--------------------------------------------------------------------------------
void CvPlayer::doBarbarianRansom(int iOption, int iUnitID)
{
	UnitHandle pUnit = getUnit(iUnitID);

	// Pay the Price
	if(iOption == 0)
	{
		int iNumGoldStolen = GC.getBARBARIAN_UNIT_GOLD_RANSOM();	// 100

		if(iNumGoldStolen > GetTreasury()->GetGold())
		{
			iNumGoldStolen = GetTreasury()->GetGold();
		}

		// Unit is ransomed for Gold
		GetTreasury()->ChangeGold(-iNumGoldStolen);
	}
	// Leave them to the Barbs
	else if(iOption == 1)
	{
		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->AddUnitMessage(0, pUnit->GetIDInfo(), GetID(), true, GC.getEVENT_MESSAGE_TIME(), GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_RANSOM_KILL_BY_BARBARIANS", pUnit->getNameKey()));//,GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, pUnit->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pUnit->getX(), pUnit->getY(), true, true);
		}

		pUnit->kill(true, BARBARIAN_PLAYER);
	}
}

//	-----------------------------------------------------------------------------------------------
const char* CvPlayer::getName() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->GetDescription();
	}

	if(GC.getGame().isMPOption(MPOPTION_ANONYMOUS) && isAlive() && GC.getGame().getGameState() == GAMESTATE_ON)
	{
		return getLeaderInfo().GetDescription();
	}

	if(GC.getGame().isGameMultiPlayer() && isHuman())
	{
		const CvString& szDisplayName = CvPreGame::nicknameDisplayed(GetID());
		if(szDisplayName.GetLength())
			return szDisplayName.c_str();
	}
	
	const CvString& szPlayerName = CvPreGame::leaderName(GetID());
	if(szPlayerName.GetLength() == 0)
	{
		return getLeaderInfo().GetDescription();
	}

	return szPlayerName.c_str();
}

//	-----------------------------------------------------------------------------------------------
const char* CvPlayer::getNameKey() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->GetTextKey();
	}

	if(GC.getGame().isMPOption(MPOPTION_ANONYMOUS) && isAlive())
	{
		return getLeaderInfo().GetTextKey();
	}

	if(GC.getGame().isGameMultiPlayer() && isHuman())
	{
		// No, this won't be a 'key', but it should just pass through the lookup code and display as is.
		const CvString& szDisplayName = CvPreGame::nicknameDisplayed(GetID());
		if(szDisplayName.GetLength())
			return szDisplayName.c_str();
	}

	const CvString& szPlayerName = CvPreGame::leaderNameKey(GetID());
	if(szPlayerName.GetLength() == 0)
	{
		return getLeaderInfo().GetTextKey();
	}

	return szPlayerName.c_str();
}


//	--------------------------------------------------------------------------------
const char* CvPlayer::getCivilizationDescription() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->GetDescription();
	}
	else if(CvPreGame::civilizationDescription(GetID()).GetLength() == 0)
	{
		return getCivilizationInfo().GetDescription();
	}
	else
	{
		return CvPreGame::civilizationDescription(GetID()).c_str();
	}
}


//	--------------------------------------------------------------------------------
const char* CvPlayer::getCivilizationDescriptionKey() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->GetTextKey();
	}
	else if(CvPreGame::civilizationDescriptionKey(GetID()).GetLength() == 0)
	{
		return getCivilizationInfo().GetTextKey();
	}
	else
	{
		return CvPreGame::civilizationDescriptionKey(GetID()).c_str();
	}
}


//	--------------------------------------------------------------------------------
const char* CvPlayer::getCivilizationShortDescription() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->getShortDescription();
	}
	else if(CvPreGame::civilizationShortDescription(GetID()).GetLength() == 0)
	{
		return getCivilizationInfo().getShortDescription();
	}
	else
	{
		return CvPreGame::civilizationShortDescription(GetID()).c_str();
	}
}


//	--------------------------------------------------------------------------------
const char* CvPlayer::getCivilizationShortDescriptionKey() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->getShortDescriptionKey();
	}
	else if(CvPreGame::civilizationShortDescriptionKey(GetID()).GetLength() == 0)
	{
		return getCivilizationInfo().getShortDescriptionKey();
	}
	else
	{
		return CvPreGame::civilizationShortDescriptionKey(GetID()).c_str();
	}
}


//	--------------------------------------------------------------------------------
const char* CvPlayer::getCivilizationAdjective() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->getAdjective();
	}
	else if(CvPreGame::civilizationAdjective(GetID()).GetLength() == 0)
	{
		return getCivilizationInfo().getAdjective();
	}
	else
	{
		return CvPreGame::civilizationAdjective(GetID()).c_str();
	}
}

//	--------------------------------------------------------------------------------
const char* CvPlayer::getCivilizationAdjectiveKey() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->getAdjectiveKeyWide();
	}
	else if(CvPreGame::civilizationAdjectiveKey(GetID()).GetLength() == 0)
	{
		return getCivilizationInfo().getAdjectiveKey();
	}
	else
	{
		return CvPreGame::civilizationAdjectiveKey(GetID()).c_str();
	}
}

//	--------------------------------------------------------------------------------
const char* CvPlayer::getCivilizationTypeKey() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->GetType();
	}
	else
	{
		return getCivilizationInfo().GetType();
	}
}

//	--------------------------------------------------------------------------------
const char* CvPlayer::getLeaderTypeKey() const
{
	if(isMinorCiv())
	{
		return GC.getMinorCivInfo(GetMinorCivAI()->GetMinorCivType())->GetType();
	}
	else
	{
		return getLeaderInfo().GetType();
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isWhiteFlag() const
{
	return CvPreGame::isWhiteFlag(GetID());
}


//	--------------------------------------------------------------------------------
const char* CvPlayer::GetStateReligionName() const
{
	return GetLocalizedText(m_strReligionKey.get());
}

//	--------------------------------------------------------------------------------
CvString CvPlayer::GetStateReligionKey() const
{
	return m_strReligionKey.get();
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetStateReligionKey(const char* strKey)
{
	m_strReligionKey = strKey;
}


//	--------------------------------------------------------------------------------
const CvString CvPlayer::getWorstEnemyName() const
{
	TeamTypes eWorstEnemy;

	eWorstEnemy = NO_TEAM;

	if(eWorstEnemy != NO_TEAM)
	{
		return GET_TEAM(eWorstEnemy).getName();
	}

	return "";
}


//	--------------------------------------------------------------------------------
ArtStyleTypes CvPlayer::getArtStyleType() const
{
	if(CvPreGame::artStyle(GetID()) == NO_ARTSTYLE)
	{
		return ((ArtStyleTypes)(getCivilizationInfo().getArtStyleType()));
	}
	else
	{
		return CvPreGame::artStyle(GetID());
	}
}

//	---------------------------------------------------------------------------
void CvPlayer::doTurn()
{
	// Time building of these maps
	AI_PERF_FORMAT("AI-perf.csv", ("CvPlayer::doTurn(), Turn %d, %s", GC.getGame().getGameTurn(), getCivilizationShortDescription()));

	CvAssertMsg(isAlive(), "isAlive is expected to be true");

	doUpdateCacheOnTurn();

	AI_doTurnPre();

	if(getCultureBombTimer() > 0)
		changeCultureBombTimer(-1);

	if(getConversionTimer() > 0)
		changeConversionTimer(-1);

	if(GetTurnsSinceSettledLastCity() >= 0)
		ChangeTurnsSinceSettledLastCity(1);

	setConscriptCount(0);

	DoUpdateCramped();

	DoUpdateUprisings();
	DoUpdateCityRevolts();

	if(GetPlayerTraits()->IsEndOfMayaLongCount())
	{
		ChangeNumMayaBoosts(1);
#ifdef BUILDING_BAKTUN_GOLD_AGE_POINTS
		BuildingTypes eBaktunBuilding = (BuildingTypes)GC.getInfoTypeForString("BUILDING_PITZ_HALL", true);
		CvBuildingEntry* pEntry = GC.GetGameBuildings()->GetEntry(eBaktunBuilding);
		ChangeGoldenAgeProgressMeter(countNumBuildings(eBaktunBuilding) * pEntry->GetBaktunGoldenAgePoints());
#endif
	}

	bool bHasActiveDiploRequest = false;
	if(isAlive())
	{
		if(!isBarbarian())
		{
			if(!isMinorCiv())
			{
#ifdef DO_CANCEL_DEALS_WITH_AI
				if(!isHuman() && GC.getGame().isOption("GAMEOPTION_AI_TWEAKS"))
				{
					DealList tempDeals;

					if (GC.getGame().GetGameDeals()->m_CurrentDeals.size() > 0)
					{
						bool bSomethingChanged = false;

						// Copy the deals into a temporary container
						for (DealList::iterator it = GC.getGame().GetGameDeals()->m_CurrentDeals.begin(); it != GC.getGame().GetGameDeals()->m_CurrentDeals.end(); ++it)
						{
							tempDeals.push_back(*it);
						}

						GC.getGame().GetGameDeals()->m_CurrentDeals.clear();
						for (DealList::iterator it = tempDeals.begin(); it != tempDeals.end(); ++it)
						{
							// Players on this deal match?
							if (it->m_eFromPlayer == GetID() || it->m_eToPlayer == GetID())
							{
								// Change final turn
								it->m_iFinalTurn = GC.getGame().getGameTurn();

								bool bIsTradeItemPeaceTreaty = false;
								bool bNotIsTradeItemPeaceTreaty = false;
								for (TradedItemList::iterator itemIter = it->m_TradedItems.begin(); itemIter != it->m_TradedItems.end(); ++itemIter)
								{
									if (itemIter->m_eItemType == TRADE_ITEM_PEACE_TREATY)
									{
										bIsTradeItemPeaceTreaty = true;
									}
									else
									{
										bNotIsTradeItemPeaceTreaty = true;
									}
								}

								if (!bIsTradeItemPeaceTreaty || bNotIsTradeItemPeaceTreaty)
								{
									// Cancel individual items
									for (TradedItemList::iterator itemIter = it->m_TradedItems.begin(); itemIter != it->m_TradedItems.end(); ++itemIter)
									{
										bSomethingChanged = true;

										itemIter->m_iFinalTurn = GC.getGame().getGameTurn();

										PlayerTypes eFromPlayer = itemIter->m_eFromPlayer;
										PlayerTypes eToPlayer = it->GetOtherPlayer(eFromPlayer);

										GC.getGame().GetGameDeals()->DoEndTradedItem(&*itemIter, eToPlayer, true);
									}
									GC.getGame().GetGameDeals()->m_HistoricalDeals.push_back(*it);
								}
								else
								{
									GC.getGame().GetGameDeals()->m_CurrentDeals.push_back(*it);
								}
							}
							else
							{
								GC.getGame().GetGameDeals()->m_CurrentDeals.push_back(*it);
							}
						}

						GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
					}

					GC.getGame().GetGameTrade()->ClearAllCivTradeRoutes(GetID());
					for(int iLoopTeam = 0; iLoopTeam < MAX_CIV_TEAMS; iLoopTeam++)
					{
						TeamTypes eTeam = (TeamTypes)iLoopTeam;
						if (getTeam() != eTeam && GET_TEAM(eTeam).isAlive() && GET_TEAM(eTeam).isHuman())
						{
							GET_TEAM(getTeam()).CloseEmbassyAtTeam(eTeam);
							GET_TEAM(eTeam).CloseEmbassyAtTeam(getTeam());
							GET_TEAM(getTeam()).CancelResearchAgreement(eTeam);
							GET_TEAM(eTeam).CancelResearchAgreement(getTeam());
							GET_TEAM(getTeam()).EvacuateDiplomatsAtTeam(eTeam);
							GET_TEAM(eTeam).EvacuateDiplomatsAtTeam(getTeam());

							// Bump Units out of places they shouldn't be
							GC.getMap().verifyUnitValidPlot();
						}
					}
				}
#endif
#ifdef DO_EXTRACT_AI_SPIES
				if (!isHuman() && GC.getGame().isOption("GAMEOPTION_AI_TWEAKS"))
				{
					for (uint uiSpy = 0; uiSpy < GetEspionage()->m_aSpyList.size(); uiSpy++)
					{
						GetEspionage()->ExtractSpyFromCity(uiSpy);
					}
				}
#endif
#ifdef CHANGE_CITY_ORIGINAL_OWNER
				if (GC.getGame().isNetworkMultiPlayer())
				{
					for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
					{
						PlayerTypes eLoopPlayer = (PlayerTypes)iPlayerLoop;

						if (eLoopPlayer != GetID() && GET_PLAYER(eLoopPlayer).isAlive() && GET_PLAYER(eLoopPlayer).isHuman())
						{
							int iCityLoop;
							CvCity* pLoopCity = NULL;
							for (pLoopCity = GET_PLAYER(eLoopPlayer).firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(eLoopPlayer).nextCity(&iCityLoop))
							{
								if ((int)pLoopCity->getOriginalOwner() < MAX_MAJOR_CIVS && !pLoopCity->IsOriginalCapital())
								{
									if (!GET_PLAYER(pLoopCity->getOriginalOwner()).isHuman() && pLoopCity->getOriginalOwner() == GetID())
									{
										if (pLoopCity->IsNoOccupiedUnhappiness())
										{
											pLoopCity->setOriginalOwner(eLoopPlayer);
											pLoopCity->SetOccupied(false);
										}
									}
								}
							}
						}
					}
				}
#endif
#ifndef DO_TURN_CHANGE_ORDER
				GetTrade()->DoTurn();
#endif
				GetMilitaryAI()->ResetCounters();
				GetGrandStrategyAI()->DoTurn();
				if(GC.getGame().isHotSeat() && !isHuman())
				{
					// In Hotseat, AIs only do their diplomacy pass for other AIs on their turn
					// Diplomacy toward a human is done at the beginning of the humans turn.
					GetDiplomacyAI()->DoTurn((PlayerTypes)CvDiplomacyAI::DIPLO_AI_PLAYERS);		// Do diplomacy for toward everyone
				}
				else
					GetDiplomacyAI()->DoTurn((PlayerTypes)CvDiplomacyAI::DIPLO_ALL_PLAYERS);	// Do diplomacy for toward everyone

				if (!isHuman())
					bHasActiveDiploRequest = CvDiplomacyRequests::HasActiveDiploRequestWithHuman(GetID());
			}
		}
	}

	if(isHuman() && !GC.getGame().isGameMultiPlayer())
		doArmySize();

	if( (bHasActiveDiploRequest || GC.GetEngineUserInterface()->isDiploActive()) && !GC.getGame().isGameMultiPlayer() && !isHuman())
	{
		GC.getGame().SetWaitingForBlockingInput(m_eID);
	}
	else
	{
		doTurnPostDiplomacy();
	}

#ifdef DO_TURN_CHANGE_ORDER
	if(isAlive())
	{
		if(!isBarbarian())
		{
			if(!isMinorCiv())
			{
				GetTrade()->DoTurn();
#ifdef POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
				updateYield();
#endif
			}
		}
	}
#endif

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "PlayerDoTurn", args.get(), bResult);
	}

	m_kPlayerAchievements.StartTurn();
}

//	--------------------------------------------------------------------------------
void CvPlayer::doTurnPostDiplomacy()
{
	CvGame& kGame = GC.getGame();

#if defined AUTOMATICALLY_SPEND_FREE_TECHNOLOGIES && defined DO_TURN_CHANGE_ORDER
	FStaticVector<TechTypes, 128, true, c_eCiv5GameplayDLL> vePossibleTechs;
	int iCheapestTechCost = MAX_INT;
	while (GetNumFreeTechs() > 0)
	{
		for (int i = 0; i < GC.getNumTechInfos(); i++)
		{
			TechTypes e = (TechTypes)i;
			CvTechEntry* pInfo = GC.getTechInfo(e);
			if (pInfo)
			{
				// We don't
				if (!GET_TEAM(getTeam()).GetTeamTechs()->HasTech(e))
				{
					// But we could
					if (GetPlayerTechs()->CanResearch(e))
					{
						if (pInfo->GetResearchCost() < iCheapestTechCost)
						{
							iCheapestTechCost = pInfo->GetResearchCost();
							vePossibleTechs.clear();
							vePossibleTechs.push_back(e);
						}
						else if (pInfo->GetResearchCost() == iCheapestTechCost)
						{
							vePossibleTechs.push_back(e);
						}
					}
				}
			}
		}

		if (!vePossibleTechs.empty())
		{
			int iRoll = GC.getGame().getJonRandNum((int)vePossibleTechs.size(), "Rolling to choose free tech from conquering a city");
			TechTypes eFreeTech = vePossibleTechs[iRoll];
			CvAssert(eFreeTech != NO_TECH)
				if (eFreeTech != NO_TECH)
				{
					GET_TEAM(getTeam()).setHasTech(eFreeTech, true, GetID(), true, true);
					GET_TEAM(getTeam()).GetTeamTechs()->SetNoTradeTech(eFreeTech, true);
				}
		}
		SetNumFreeTechs(max(0, GetNumFreeTechs() - 1));
	}
#endif
#ifdef PENALTY_FOR_DELAYING_POLICIES
	bool bIsDelaydPolicy = IsDelayedPolicy();
	if (kGame.isOption(GAMEOPTION_END_TURN_TIMER_ENABLED))
	{
		if (getJONSCulture() >= getNextPolicyCost() || GetNumFreePolicies() > 0)
		{
			if (isHuman())
			{
				bIsDelaydPolicy = true;
			}
		}
		else
		{
			bIsDelaydPolicy = false;
		}
	}
#endif
#ifdef DO_TURN_CHANGE_ORDER
	// Do turn for all Cities
	{
		AI_PERF_FORMAT("AI-perf.csv", ("Do City Turns, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()));
		if (getNumCities() > 0)
		{
			int iLoop = 0;
			for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				pLoopCity->doTurn();
			}
		}
	}
#endif
	if(isAlive())
	{
		kGame.GetTacticalAnalysisMap()->RefreshDataForNextPlayer(this);

		{
			AI_PERF_FORMAT("AI-perf.csv", ("Plots/Danger, Turn %03d, %s", kGame.getElapsedGameTurns(), getCivilizationShortDescription()) );

			UpdatePlots();
			m_pDangerPlots->UpdateDanger();
		}

		if(!isBarbarian())
		{
			GetEconomicAI()->DoTurn();
			GetMilitaryAI()->DoTurn();
			GetReligionAI()->DoTurn();
			GetTradeAI()->DoTurn();
			GetCitySpecializationAI()->DoTurn();
			GetLeagueAI()->DoTurn();
		}

#ifndef DO_TURN_CHANGE_ORDER
		if(isMinorCiv())
		{
			GetMinorCivAI()->DoTurn();
		}
#endif
	}

	// Temporary boosts
	if(GetAttackBonusTurns() > 0)
	{
		ChangeAttackBonusTurns(-1);
	}
	if(GetCultureBonusTurns() > 0)
	{
		ChangeCultureBonusTurns(-1);
	}
	if(GetTourismBonusTurns() > 0)
	{
		ChangeTourismBonusTurns(-1);
	}


#ifndef DO_TURN_CHANGE_ORDER
	// Golden Age
	DoProcessGoldenAge();
#endif

	// Great People gifts from Allied City States (if we have that policy)
	DoGreatPeopleSpawnTurn();

#if defined AUTOMATICALLY_SPEND_FREE_TECHNOLOGIES && !defined DO_TURN_CHANGE_ORDER
	FStaticVector<TechTypes, 128, true, c_eCiv5GameplayDLL> vePossibleTechs;
	int iCheapestTechCost = MAX_INT;
	while(GetNumFreeTechs() > 0)
	{
		for (int i = 0; i < GC.getNumTechInfos(); i++)
		{
			TechTypes e = (TechTypes) i;
			CvTechEntry* pInfo = GC.getTechInfo(e);
			if (pInfo)
			{
				// We don't
				if (!GET_TEAM(getTeam()).GetTeamTechs()->HasTech(e))
				{
					// But we could
					if (GetPlayerTechs()->CanResearch(e))
					{
						if (pInfo->GetResearchCost() < iCheapestTechCost)
						{
							iCheapestTechCost = pInfo->GetResearchCost();
							vePossibleTechs.clear();
							vePossibleTechs.push_back(e);
						}
						else if (pInfo->GetResearchCost() == iCheapestTechCost)
						{
							vePossibleTechs.push_back(e);
						}
					}
				}
			}
		}

		if (!vePossibleTechs.empty())
		{
			int iRoll = GC.getGame().getJonRandNum((int)vePossibleTechs.size(), "Rolling to choose free tech from conquering a city");
			TechTypes eFreeTech = vePossibleTechs[iRoll];
			CvAssert(eFreeTech != NO_TECH)
			if (eFreeTech != NO_TECH)
			{
				GET_TEAM(getTeam()).setHasTech(eFreeTech, true, GetID(), true, true);
				GET_TEAM(getTeam()).GetTeamTechs()->SetNoTradeTech(eFreeTech, true);
			}
		}
		SetNumFreeTechs(max(0, GetNumFreeTechs() - 1));
	}
#endif
#ifndef DO_TURN_CHANGE_ORDER
	// Do turn for all Cities
	{
		AI_PERF_FORMAT("AI-perf.csv", ("Do City Turns, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()) );
		if(getNumCities() > 0)
		{
			int iLoop = 0;
			for(CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				pLoopCity->doTurn();
			}
		}
	}
#endif

#ifndef DO_TURN_CHANGE_ORDER
	// Gold
	GetTreasury()->DoGold();
#endif

	// Culture

	// Prevent exploits in turn timed MP games - no accumulation of culture if player hasn't picked yet
	GetCulture()->SetLastTurnLifetimeCulture(GetJONSCultureEverGenerated());
	if(kGame.isOption(GAMEOPTION_END_TURN_TIMER_ENABLED))
	{
#ifdef AI_CULTURE_RESTRICTION
		if(getJONSCulture() < getNextPolicyCost())
		{
			if(isHuman() || getNextPolicyCost() < 1000 || !GC.getGame().isOption("GAMEOPTION_AI_TWEAKS"))
			{
				changeJONSCulture(GetTotalJONSCulturePerTurn());
#ifdef POLICY_BRANCH_NOTIFICATION_LOCKED
				if(GetNumFreePolicies() <= 0)
				{
					for (int iI = 0; iI < GC.GetGamePolicies()->GetNumPolicyBranches(); iI++)
					{
						PolicyBranchTypes ePolicyBranch = (PolicyBranchTypes)iI;
						if(!GetPlayerPolicies()->CanUnlockPolicyBranch(ePolicyBranch))
						{
							GetPlayerPolicies()->SetPolicyBranchNotificationLocked(ePolicyBranch, false);
						}
					}
				}
#endif
			}
		}
#ifdef POLICY_BRANCH_NOTIFICATION_LOCKED
		else if(getJONSCulture() >= getNextPolicyCost() || GetNumFreePolicies() > 0)
		{
			if(isHuman())
			{
				for (int iI = 0; iI < GC.GetGamePolicies()->GetNumPolicyBranches(); iI++)
				{
					PolicyBranchTypes ePolicyBranch = (PolicyBranchTypes)iI;
					if(!GetPlayerPolicies()->CanUnlockPolicyBranch(ePolicyBranch))
					{
						GetPlayerPolicies()->SetPolicyBranchNotificationLocked(ePolicyBranch, true);
					}
				}
			}
		}
#endif
#else
		if(getJONSCulture() < getNextPolicyCost())
			changeJONSCulture(GetTotalJONSCulturePerTurn());
#endif
	}
	else
	{
		changeJONSCulture(GetTotalJONSCulturePerTurn());
	}

	// Compute the cost of policies for this turn
	DoUpdateNextPolicyCost();

	// if this is the human player, have the popup come up so that he can choose a new policy
	if(isAlive() && isHuman() && getNumCities() > 0)
	{
		if(!GC.GetEngineUserInterface()->IsPolicyNotificationSeen())
		{
			if(getNextPolicyCost() <= getJONSCulture() && GetPlayerPolicies()->GetNumPoliciesCanBeAdopted() > 0)
			{
				CvNotifications* pNotifications = GetNotifications();
				if(pNotifications)
				{
					CvString strBuffer;

					if(kGame.isOption(GAMEOPTION_POLICY_SAVING))
						strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY_DISMISS");
					else
						strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY");

					CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ENOUGH_CULTURE_FOR_POLICY");
					pNotifications->Add(NOTIFICATION_POLICY, strBuffer, strSummary, -1, -1, -1);
				}
			}
		}

		if (GetPlayerPolicies()->IsTimeToChooseIdeology() && GetPlayerPolicies()->GetLateGamePolicyTree() == NO_POLICY_BRANCH_TYPE)
		{
			CvNotifications* pNotifications = GetNotifications();
			if(pNotifications)
			{
				CvString strBuffer;
				if (GetCurrentEra() > GC.getInfoTypeForString("ERA_INDUSTRIAL"))
				{
					strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_CHOOSE_IDEOLOGY_ERA");
				}
				else
				{
					strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_CHOOSE_IDEOLOGY_FACTORIES");
				}
				CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_CHOOSE_IDEOLOGY");
				pNotifications->Add(NOTIFICATION_CHOOSE_IDEOLOGY, strBuffer, strSummary, -1, -1, GetID());
			}
		}
	}

	if (isAlive() && getNumCities() > 0 && !isHuman() && !isMinorCiv())
	{
		if (GetPlayerPolicies()->IsTimeToChooseIdeology() && GetPlayerPolicies()->GetLateGamePolicyTree() == NO_POLICY_BRANCH_TYPE)
		{
			AI_PERF_FORMAT("AI-perf.csv", ("DoChooseIdeology, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()) );
			GetPlayerPolicies()->DoChooseIdeology();
		}
	}

	if(!isBarbarian() && !isHuman())
	{
		AI_PERF_FORMAT("AI-perf.csv", ("DoPolicyAI, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()) );
		GetPlayerPolicies()->DoPolicyAI();
	}

	// Science
	doResearch();

#ifdef DO_TURN_CHANGE_ORDER
	// Gold
	GetTreasury()->DoGold();
#endif
#ifdef PENALTY_FOR_DELAYING_POLICIES
	setIsDelayedPolicy(bIsDelaydPolicy);
#endif
#ifdef FIX_EXCHANGE_PRODUCTION_OVERFLOW_INTO_GOLD_OR_SCIENCE
	if (getNumCities() > 0)
	{
		int iLoop = 0;
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			pLoopCity->setProcessOverflowProductionTimes100(0);
		}
	}
#endif

	GetEspionage()->DoTurn();

	// Faith
	CvGameReligions* pGameReligions = kGame.GetGameReligions();
	pGameReligions->DoPlayerTurn(*this);

	// Leagues
	CvGameLeagues* pGameLeagues = kGame.GetGameLeagues();
	pGameLeagues->DoPlayerTurn(*this);

#ifdef DO_TURN_CHANGE_ORDER
	// Golden Age
	DoProcessGoldenAge();
#endif

	// Anarchy counter
	if(GetAnarchyNumTurns() > 0)
		ChangeAnarchyNumTurns(-1);

	DoIncomingUnits();

	const int iGameTurn = kGame.getGameTurn();

	GatherPerTurnReplayStats(iGameTurn);

	GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);

	AI_doTurnPost();
}

//	--------------------------------------------------------------------------------
void CvPlayer::doTurnUnits()
{
	CvArmyAI* pLoopArmyAI;
	CvUnit* pLoopUnit;
	int iLoop;

	AI_doTurnUnitsPre();

	// Start: TACTICAL AI UNIT PROCESSING
	m_pTacticalAI->DoTurn();

	// Start: OPERATIONAL AI UNIT PROCESSING
	std::map<int, CvAIOperation*>::iterator iter;
	bool bKilledSomething;
	do
	{
		bKilledSomething = false;
		for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
		{
			CvAIOperation* pThisOperation = iter->second;
			if(pThisOperation)
			{
				if(pThisOperation->DoDelayedDeath())
				{
					bKilledSomething = true;
					break;
				}
			}
		}
		// hack
	}
	while(bKilledSomething);

	for(pLoopArmyAI = firstArmyAI(&iLoop); pLoopArmyAI != NULL; pLoopArmyAI = nextArmyAI(&iLoop))
	{
		pLoopArmyAI->DoDelayedDeath();
	}

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		pLoopUnit->doDelayedDeath();
	}

	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		CvAIOperation* pThisOperation = iter->second;
		if(pThisOperation)
		{
			pThisOperation->DoTurn();
		}
	}

	do
	{
		bKilledSomething = false;
		for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
		{
			CvAIOperation* pThisOperation = iter->second;
			if(pThisOperation)
			{
				if(pThisOperation->DoDelayedDeath())
				{
					bKilledSomething = true;
					break;
				}
			}
		}
		// hack
	}
	while(bKilledSomething);

	for(pLoopArmyAI = firstArmyAI(&iLoop); pLoopArmyAI != NULL; pLoopArmyAI = nextArmyAI(&iLoop))
	{
		pLoopArmyAI->DoTurn();
	}

	// Homeland AI
	m_pHomelandAI->DoTurn();

	// Start: old unit AI processing
	for(int iPass = 0; iPass < 4; iPass++)
	{
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			switch(pLoopUnit->getDomainType())
			{
			case DOMAIN_AIR:
				if(iPass == 1)
				{
					pLoopUnit->doTurn();
				}
				break;
			case DOMAIN_SEA:
				if(iPass == 2)
				{
					pLoopUnit->doTurn();
				}
				break;
			case DOMAIN_LAND:
				if(iPass == 3)
				{
					pLoopUnit->doTurn();
				}
				break;
			case DOMAIN_IMMOBILE:
				if(iPass == 0)
				{
					pLoopUnit->doTurn();
				}
				break;
			case NO_DOMAIN:
				CvAssertMsg(false, "Unit with no Domain");
				break;
			default:
				if(iPass == 3)
				{
					pLoopUnit->doTurn();
				}
				break;
			}
		}
	}

	if(GetID() == GC.getGame().getActivePlayer())
	{
		GC.GetEngineUserInterface()->setDirty(Waypoints_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(SelectionButtons_DIRTY_BIT, true);
	}

	GC.GetEngineUserInterface()->setDirty(UnitInfo_DIRTY_BIT, true);

	AI_doTurnUnitsPost();
}

//	--------------------------------------------------------------------------------
/// Indicate that the AI has not processed any units yet
void CvPlayer::SetAllUnitsUnprocessed()
{
	CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		pLoopUnit->SetTurnProcessed(false);
	}
}

//	--------------------------------------------------------------------------------
/// Units heal and then get their movement back
void CvPlayer::DoUnitReset()
{
	CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		// HEAL UNIT?
		if(!pLoopUnit->isEmbarked())
		{
#ifdef FIGHTER_FINISHMOVES_AFTER_INTERCEPTION
			if(pLoopUnit->hasMoved() || (pLoopUnit->getMadeInterceptionCount() > 0 && pLoopUnit->getDomainType() == DOMAIN_AIR))
#else
			if (pLoopUnit->hasMoved())
#endif
			{
				if(pLoopUnit->isAlwaysHeal())
				{
					pLoopUnit->doHeal();
				}
			}
			else
			{
#ifdef BUILDING_BORDER_TRANSITION_OBSTACLE
				if (pLoopUnit->IsHurt() && !(pLoopUnit->plot()->getOwner() != NO_PLAYER && pLoopUnit->plot()->getOwner() != pLoopUnit->getOwner() && GET_PLAYER(pLoopUnit->plot()->getOwner()).isBorderTransitionObstacle() && pLoopUnit->getDomainType() == DOMAIN_LAND))
				{
					pLoopUnit->doHeal();
				}
#else
				if(pLoopUnit->IsHurt())
				{
					pLoopUnit->doHeal();
				}
#endif
			}
		}

		int iCitadelDamage;
		if(pLoopUnit->IsNearEnemyCitadel(iCitadelDamage))
		{
			pLoopUnit->changeDamage(iCitadelDamage, NO_PLAYER, /*fAdditionalTextDelay*/ 0.5f);
		}

		// Finally (now that healing is done), restore movement points
		pLoopUnit->setMoves(pLoopUnit->maxMoves());
		if(pLoopUnit->IsGreatGeneral())
		{
			pLoopUnit->setMoves(pLoopUnit->GetGreatGeneralStackMovement());
		}

		// Archaeologist can't move on turn he finishes a dig (while waiting for user to decide his next action)
		else if (pLoopUnit->AI_getUnitAIType() == UNITAI_ARCHAEOLOGIST)
		{
			CvPlayer &kPlayer = GET_PLAYER(pLoopUnit->getOwner());
			if (kPlayer.GetCulture()->HasDigCompleteHere(pLoopUnit->plot()))
			{
				pLoopUnit->setMoves(0);
			}
		}

		pLoopUnit->SetIgnoreDangerWakeup(false);
		pLoopUnit->setMadeAttack(false);
#ifdef CAPTURE_RESTRICTION_AFTER_PARADROPPING
		if (!pLoopUnit->isSecondHalfTimerParadropped())
		{
			if (pLoopUnit->getNoCaptureCount() > 0)
			{
				if (pLoopUnit->getUnitType() != (UnitTypes)GC.getInfoTypeForString("UNIT_HELICOPTER_GUNSHIP", true))
				{
					pLoopUnit->setHasPromotion((PromotionTypes)GC.getInfoTypeForString("PROMOTION_NO_CAPTURE", true), false);
				}
			}
		}
		pLoopUnit->setMadeSecondHalfTimerParadrop(false);
#endif
#ifdef MADE_REBASE
		pLoopUnit->setMadeRebase(false);
#endif
#ifdef REBASE_WITH_AIRPORTS
		pLoopUnit->setMadeAirliftRebase(false);
#endif
		pLoopUnit->setMadeInterception(false);

		if(!isHuman())
		{
			const MissionData* pkMissionData = pLoopUnit->GetHeadMissionData();
			if(pkMissionData)
			{
				if(pkMissionData->eMissionType == CvTypes::getMISSION_RANGE_ATTACK() ||
				        pkMissionData->eMissionType == CvTypes::getMISSION_AIRSTRIKE() ||
				        pkMissionData->eMissionType == CvTypes::getMISSION_AIR_SWEEP() ||
				        pkMissionData->eMissionType == CvTypes::getMISSION_NUKE())
				{
					CvAssertMsg(0, "An AI unit has a combat mission queued at the end of its turn.");
					pLoopUnit->ClearMissionQueue();	// Clear the whole thing, the AI will re-evaluate next turn.
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Damage units from attrition (start of turn so we can get notifications)
void CvPlayer::DoUnitAttrition()
{
	CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		pLoopUnit->DoAttrition();
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::RespositionInvalidUnits()
{
	CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(!pLoopUnit)
		{
			continue;
		}

		if(pLoopUnit->isDelayedDeath())
		{
			continue;
		}

		if(pLoopUnit->isCargo())
		{
			continue;
		}

		if(pLoopUnit->isInCombat())
		{
			continue;
		}

		CvPlot* pPlot = pLoopUnit->plot();
		if(!pPlot)
		{
			continue;
		}

		if(pPlot->getNumFriendlyUnitsOfType(pLoopUnit) > GC.getPLOT_UNIT_LIMIT())
		{
			if (!pLoopUnit->jumpToNearestValidPlot())
				pLoopUnit->kill(false);	// Could not find a valid location!
		}
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::updateYield()
{
	// This will go through all of the plots and update the yield if the player owns it.
	// The plot will not contribute to the player's yield unless it is worked by a city.
	// Previously this would just go through all the plots the city can work (3 rings around it)
	// but all plots have their yields updated on load and not updating them here could lead to 
	// a visual discrepancy.
	CvMap& kMap = GC.getMap();
	int iNumPlots = kMap.numPlots();
	PlayerTypes ePlayer = GetID();
	for (int iI = 0; iI < iNumPlots; iI++)
	{
		CvPlot* pkPlot = kMap.plotByIndexUnchecked(iI);
		if (pkPlot->getOwner() == ePlayer)
			pkPlot->updateYield();
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::updateExtraSpecialistYield()
{
	CvCity* pLoopCity;
	int iLoop;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateExtraSpecialistYield();
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::updateCityPlotYield()
{
	CvCity* pLoopCity;
	int iLoop;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->plot()->updateYield();
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::updateCitySight(bool bIncrement)
{
	CvCity* pLoopCity;
	int iLoop;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->plot()->updateSight(bIncrement);
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::UpdateNotifications()
{
	if(GetNotifications())
	{
		GetNotifications()->Update();
	}

	if(GetDiplomacyRequests())
	{
		GetDiplomacyRequests()->Update();
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::UpdateReligion()
{
	DoUpdateHappiness();
}

//	--------------------------------------------------------------------------------
void CvPlayer::updateTimers()
{
	CvUnit* pLoopUnit;
	int iLoop;
	m_endTurnBusyUnitUpdatesLeft--;
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		pLoopUnit->UpdateMission();
		pLoopUnit->doDelayedDeath();
	}

	GetDiplomacyAI()->update();
}

//	--------------------------------------------------------------------------------
bool CvPlayer::hasPromotableUnit() const
{
	const CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->isPromotionReady() && !pLoopUnit->isDelayedDeath())
		{
			return true;
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::hasReadyUnit() const
{
	const CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->ReadyToMove() && !pLoopUnit->isDelayedDeath())
		{
			return true;
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetCountReadyUnits() const
{
	int iRtnValue = 0;
	const CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->ReadyToMove() && !pLoopUnit->isDelayedDeath())
		{
			iRtnValue++;
		}
	}

	return iRtnValue;
}

//	--------------------------------------------------------------------------------
const CvUnit* CvPlayer::GetFirstReadyUnit() const
{
	const CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->ReadyToMove() && !pLoopUnit->isDelayedDeath())
		{
			return pLoopUnit;
		}
	}

	return NULL;
}

//	--------------------------------------------------------------------------------
void CvPlayer::EndTurnsForReadyUnits()
{
	CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->ReadyToMove() && !pLoopUnit->isDelayedDeath())
		{
			pLoopUnit->finishMoves();
		}
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::hasAutoUnit() const
{
	const CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->ReadyToAuto())
		{
			return true;
		}
	}

	return false;
}

//	----------------------------------------------------------------------------
bool CvPlayer::hasBusyUnit() const
{
	const CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->IsBusy())
		{
			return true;
		}
	}

	return false;
}

//	----------------------------------------------------------------------------
bool CvPlayer::hasBusyCity() const
{
	const CvCity* pLoopCity;
	int iLoop;

	for(pLoopCity = firstCity(&iLoop); pLoopCity; pLoopCity = nextCity(&iLoop))
	{
		if(pLoopCity->IsBusy())
		{
			return true;
		}
	}

	return false;
}

//	----------------------------------------------------------------------------
const CvCity* CvPlayer::getBusyCity() const
{
	const CvCity* pLoopCity;
	int iLoop;

	for(pLoopCity = firstCity(&iLoop); pLoopCity; pLoopCity = nextCity(&iLoop))
	{
		if(pLoopCity->IsBusy())
		{
			return pLoopCity;
		}
	}

	return false;
}

//	----------------------------------------------------------------------------
bool CvPlayer::hasBusyUnitOrCity() const
{
	if(hasBusyUnit())
		return true;
	return hasBusyCity();
}

//	--------------------------------------------------------------------------------
const UnitHandle CvPlayer::getBusyUnit() const
{
	const UnitHandle result;
	const CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->IsBusy())
		{
			result = pLoopUnit;
		}
	}
	return result;
}


//	--------------------------------------------------------------------------------
void CvPlayer::chooseTech(int iDiscover, const char* strText, TechTypes iTechJustDiscovered)
{
	if(GC.getGame().isOption(GAMEOPTION_NO_SCIENCE))
	{
		return;
	}

	if(iDiscover > 0)
	{
		SetNumFreeTechs(GetNumFreeTechs()+iDiscover);
	}

	if(iDiscover > 0)
	{
		CvNotifications* pNotifications = GetNotifications();
		if(pNotifications)
		{
			pNotifications->Add(NOTIFICATION_FREE_TECH, strText, strText, -1, -1, iDiscover, iTechJustDiscovered);
		}
	}
	else if(strText == 0 || strText[0] == 0)
	{
		CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_NEW_RESEARCH");
		CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_NEW_RESEARCH");
		CvNotifications* pNotifications = GetNotifications();
		if(pNotifications)
		{
#ifdef FIX_REMOVE_EXPIRED_FREE_TECH_NOTFICATION
			if (GetNumFreeTechs() == 0)
			{
				pNotifications->Update();
			}
#endif
			pNotifications->Add(NOTIFICATION_TECH, strBuffer, strSummary, -1, -1, iDiscover, iTechJustDiscovered);
		}
	}
	else
	{
		CvNotifications* pNotifications = GetNotifications();
		if(pNotifications)
		{
#ifdef FIX_REMOVE_EXPIRED_FREE_TECH_NOTFICATION
			if (GetNumFreeTechs() == 0)
			{
				pNotifications->Update();
			}
#endif
			pNotifications->Add(NOTIFICATION_TECH, strText, strText, -1, -1, iDiscover, iTechJustDiscovered);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Civ 5 Score
//////////////////////////////////////////////////////////////////////////

//	--------------------------------------------------------------------------------
/// What is this player's score?
int CvPlayer::GetScore(bool bFinal, bool bWinner) const
{
	if(!isAlive())
		return 0;

	if(GET_TEAM(getTeam()).getNumMembers() == 0)
		return 0;

	int iScore = 0;

	iScore += GetScoreFromCities();
	iScore += GetScoreFromPopulation();
	iScore += GetScoreFromLand();
	iScore += GetScoreFromWonders();
	iScore += GetScoreFromPolicies();
	iScore += GetScoreFromGreatWorks();
	iScore += GetScoreFromReligion();
	iScore += GetScoreFromTechs();
	iScore += GetScoreFromFutureTech();
	iScore += GetScoreFromScenario1();
	iScore += GetScoreFromScenario2();
	iScore += GetScoreFromScenario3();
	iScore += GetScoreFromScenario4();

	// If the game is over, we apply a mod to the value, rewarding players who finish early
	if(bFinal && bWinner)
	{
		int iGameProgressPercent = 100 * GC.getGame().getGameTurn() / GC.getGame().getEstimateEndTurn();
		iGameProgressPercent = iGameProgressPercent < 1 ? 1 : iGameProgressPercent;
		iScore *= 100;
		iScore /= iGameProgressPercent;
	}

	return iScore;
}

//	--------------------------------------------------------------------------------
// Score from Cities: 10 per city (with mod for map size)
int CvPlayer::GetScoreFromCities() const
{
	int iScore = getNumCities() * /*10*/ GC.getSCORE_CITY_MULTIPLIER();

	iScore *= GC.getGame().GetMapScoreMod();
	iScore /= 100;

	return iScore;
}

//	--------------------------------------------------------------------------------
// Score from Population: 6 per pop (with mod for map size)
int CvPlayer::GetScoreFromPopulation() const
{
	int iScore = getTotalPopulation() * /*4*/ GC.getSCORE_POPULATION_MULTIPLIER();

	iScore *= GC.getGame().GetMapScoreMod();
	iScore /= 100;

	return iScore;
}

//	--------------------------------------------------------------------------------
// Score from Land: 6 per plot (with mod for map size)
int CvPlayer::GetScoreFromLand() const
{
	int iScore = getTotalLand() * /*1*/ GC.getSCORE_LAND_MULTIPLIER();

	iScore *= GC.getGame().GetMapScoreMod();
	iScore /= 100;

	return iScore;
}

//	--------------------------------------------------------------------------------
// Score from world wonders
int CvPlayer::GetScoreFromWonders() const
{
	int iScore = GetNumWonders() * /*25*/ GC.getSCORE_WONDER_MULTIPLIER();
	return iScore;
}

//	--------------------------------------------------------------------------------
// Score from policies
int CvPlayer::GetScoreFromPolicies() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_POLICIES))
	{
		return 0;
	}
	int iScore = GetPlayerPolicies()->GetNumPoliciesOwned() * /*4*/ GC.getSCORE_POLICY_MULTIPLIER();
	return iScore;
}

//	--------------------------------------------------------------------------------
// Score from world wonders: 40 per
int CvPlayer::GetScoreFromGreatWorks() const
{
	int iScore = GetCulture()->GetNumGreatWorks() * /*4*/ GC.getSCORE_GREAT_WORK_MULTIPLIER();
	return iScore;
}

//	--------------------------------------------------------------------------------
// Score from world wonders: 40 per
int CvPlayer::GetScoreFromReligion() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_RELIGION))
	{
		return 0;
	}
	int iScore = 0;
	CvGameReligions *pGameReligions = GC.getGame().GetGameReligions();
	ReligionTypes eReligion = GetReligions()->GetReligionCreatedByPlayer();
	if (eReligion > RELIGION_PANTHEON)
	{
		const CvReligion *pReligion = pGameReligions->GetReligion(eReligion, GetID());
		iScore += pReligion->m_Beliefs.GetNumBeliefs() * /*20*/ GC.getSCORE_BELIEF_MULTIPLIER();
		iScore += pGameReligions->GetNumCitiesFollowing(eReligion) * /*1*/ GC.getSCORE_RELIGION_CITIES_MULTIPLIER();
	}
	return iScore;
}

//	--------------------------------------------------------------------------------
// Score from Tech: 4 per
int CvPlayer::GetScoreFromTechs() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_SCIENCE))
	{
		return 0;
	}

	// Normally we recompute it each time
	int iScore = GET_TEAM(getTeam()).GetTeamTechs()->GetNumTechsKnown() * /*4*/ GC.getSCORE_TECH_MULTIPLIER();
	return iScore;
}

//	--------------------------------------------------------------------------------
// Score from Future Tech: 10 per
int CvPlayer::GetScoreFromFutureTech() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_SCIENCE))
	{
		return 0;
	}

	return m_iScoreFromFutureTech;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeScoreFromFutureTech(int iChange)
{
	if(iChange != 0)
		m_iScoreFromFutureTech += iChange;
}

//	--------------------------------------------------------------------------------
// Score from scenario-specific items
int CvPlayer::GetScoreFromScenario1() const
{
	return m_iScenarioScore1;
}
void CvPlayer::ChangeScoreFromScenario1(int iChange)
{
	if(iChange != 0)
		m_iScenarioScore1 += iChange;
}
//	--------------------------------------------------------------------------------
int CvPlayer::GetScoreFromScenario2() const
{
	return m_iScenarioScore2;
}
void CvPlayer::ChangeScoreFromScenario2(int iChange)
{
	if(iChange != 0)
		m_iScenarioScore2 += iChange;
}
//	--------------------------------------------------------------------------------
int CvPlayer::GetScoreFromScenario3() const
{
	return m_iScenarioScore3;
}
void CvPlayer::ChangeScoreFromScenario3(int iChange)
{
	if(iChange != 0)
		m_iScenarioScore3 += iChange;
}
//	--------------------------------------------------------------------------------
int CvPlayer::GetScoreFromScenario4() const
{
	return m_iScenarioScore4;
}
void CvPlayer::ChangeScoreFromScenario4(int iChange)
{
	if(iChange != 0)
		m_iScenarioScore4 += iChange;
}

//////////////////////////////////////////////////////////////////////////
// End Civ 5 Score
//////////////////////////////////////////////////////////////////////////

//	--------------------------------------------------------------------------------
int CvPlayer::countCityFeatures(FeatureTypes eFeature) const
{
	const CvCity* pLoopCity;
	const CvPlot* pLoopPlot;
	int iCount;
	int iLoop;
	int iI;

	iCount = 0;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		for(iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			pLoopPlot = plotCity(pLoopCity->getX(), pLoopCity->getY(), iI);

			if(pLoopPlot != NULL)
			{
				if(pLoopPlot->getFeatureType() == eFeature)
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}


//	--------------------------------------------------------------------------------
int CvPlayer::countNumBuildings(BuildingTypes eBuilding) const
{
	const CvCity* pLoopCity;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if(pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
		{
			iCount += pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding);
		}
	}

	return iCount;
}

//	--------------------------------------------------------------------------------
/// How many cities in the empire surrounded by features?
int CvPlayer::countCitiesFeatureSurrounded() const
{
	const CvCity* pLoopCity;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if(pLoopCity->IsFeatureSurrounded())
			iCount ++;
	}

	return iCount;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsCityConnectedToCity(CvCity* pCity1, CvCity* pCity2, RouteTypes eRestrictRoute, bool bIgnoreHarbors)
{
	int iPathfinderFlags = GetID() | MOVE_ROUTE_ALLOW_UNEXPLORED;	// Since we just want to know if we are connected or not, allow the check to search unexplored terrain.
	if(eRestrictRoute == NO_ROUTE)
	{
		iPathfinderFlags |= MOVE_ANY_ROUTE;
	}
	else
	{
		// assuming that there are fewer than 256 players
		int iRouteValue = eRestrictRoute + 1;
		iPathfinderFlags |= (iRouteValue << 8);
	}

	if (bIgnoreHarbors)
	{
		GC.getRouteFinder().SetNumExtraChildrenFunc(NULL);
		GC.getRouteFinder().SetExtraChildGetterFunc(NULL);
	}

	GC.getRouteFinder().ForceReset();
	bool bReturnValue = GC.getRouteFinder().GeneratePath(pCity1->getX(), pCity1->getY(), pCity2->getX(), pCity2->getY(), iPathfinderFlags, false);

	if (bIgnoreHarbors)
	{
		// reconnect the land route pathfinder water methods
		GC.getRouteFinder().SetNumExtraChildrenFunc(RouteGetNumExtraChildren);
		GC.getRouteFinder().SetExtraChildGetterFunc(RouteGetExtraChild);
	}

	return bReturnValue;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsCapitalConnectedToPlayer(PlayerTypes ePlayer, RouteTypes eRestrictRoute)
{
	// everybody needs to be alive!
	if(!isAlive() || !(GET_PLAYER(ePlayer).isAlive()))
	{
		return false;
	}

	CvCity* pOtherPlayerCapital = GET_PLAYER(ePlayer).getCapitalCity();
	if(pOtherPlayerCapital == NULL)
	{
		return false;
	}

	return IsCapitalConnectedToCity(pOtherPlayerCapital, eRestrictRoute);
}

//	---------------------------------------------------------------------------
bool CvPlayer::IsCapitalConnectedToCity(CvCity* pCity, RouteTypes eRestrictRoute)
{
	CvCity* pPlayerCapital = getCapitalCity();
	if(pPlayerCapital == NULL)
	{
		return false;
	}

	return IsCityConnectedToCity(pPlayerCapital, pCity, eRestrictRoute);
}

//	--------------------------------------------------------------------------------
void CvPlayer::findNewCapital()
{
	CvCity* pOldCapital;
	CvCity* pLoopCity;
	CvCity* pBestCity;
	BuildingTypes eCapitalBuilding;
	int iValue;
	int iBestValue;
	int iLoop;

	eCapitalBuilding = ((BuildingTypes)(getCivilizationInfo().getCivilizationBuildings(GC.getCAPITAL_BUILDINGCLASS())));

	if(eCapitalBuilding == NO_BUILDING)
	{
		return;
	}

	pOldCapital = getCapitalCity();

	iBestValue = 0;
	pBestCity = NULL;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if(pLoopCity != pOldCapital)
		{
			if(0 == pLoopCity->GetCityBuildings()->GetNumRealBuilding(eCapitalBuilding))
			{
				iValue = (pLoopCity->getPopulation() * 4);

				int iYieldValueTimes100 = pLoopCity->getYieldRateTimes100(YIELD_FOOD, false);
				iYieldValueTimes100 += (pLoopCity->getYieldRateTimes100(YIELD_PRODUCTION, false) * 3);
				iYieldValueTimes100 += (pLoopCity->getYieldRateTimes100(YIELD_GOLD, false) * 2);
				iValue += (iYieldValueTimes100 / 100);

				iValue += (pLoopCity->getNumGreatPeople() * 2);

				if(iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestCity = pLoopCity;
				}
			}
		}
	}

	if(pBestCity != NULL)
	{
		if(pOldCapital != NULL)
		{
			pOldCapital->GetCityBuildings()->SetNumRealBuilding(eCapitalBuilding, 0);
		}
		CvAssertMsg(!(pBestCity->GetCityBuildings()->GetNumRealBuilding(eCapitalBuilding)), "(pBestCity->getNumRealBuilding(eCapitalBuilding)) did not return false as expected");
		pBestCity->GetCityBuildings()->SetNumRealBuilding(eCapitalBuilding, 1);
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::canRaze(CvCity* pCity, bool bIgnoreCapitals) const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_CITY_RAZING))
	{
		return false;
	}

	// If we don't own this city right now then we can't raze it!
	if(pCity->getOwner() != GetID())
	{
		return false;
	}

	// Can't raze a city that originally belonged to us
	if(pCity->getOriginalOwner() == GetID())
	{
		return false;
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(pCity->getOwner());
		args->Push(pCity->GetID());

		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "CanRazeOverride", args.get(), bResult))
		{
			// Check the result.
			if(bResult == true)
			{
				return true;
			}
		}
	}

	// No razing of capitals
	CvPlayer* pOriginalOwner = &GET_PLAYER(pCity->getOriginalOwner());
	bool bOriginalCapital =	pCity->getX() == pOriginalOwner->GetOriginalCapitalX() &&
	                        pCity->getY() == pOriginalOwner->GetOriginalCapitalY();

	if(!bIgnoreCapitals && pCity->IsEverCapital() && bOriginalCapital)
	{
		return false;
	}

	// No razing of Holy Cities
	if (pCity->GetCityReligions()->IsHolyCityAnyReligion())
	{
		return false;
	}

	// No razing of cities with unique luxuries
	ResourceTypes eResource = pCity->plot()->getResourceType();
	if (eResource != NO_RESOURCE)
	{
		CvResourceInfo *pkResource = GC.getResourceInfo(eResource);
		if (pkResource && pkResource->GetRequiredCivilization() != NO_CIVILIZATION)
		{
			return false;
		}
	}

	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(pCity->getOwner());
		args->Push(pCity->GetID());

		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "CanRaze", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}


//	--------------------------------------------------------------------------------
void CvPlayer::raze(CvCity* pCity)
{
	char szBuffer[1024];
	const size_t lenBuffer = 1024;
	int iI;

	if(!canRaze(pCity))
	{
		return;
	}

	CvAssert(pCity->getOwner() == GetID());

	if(GetID() == GC.getGame().getActivePlayer())
	{
		sprintf_s(szBuffer, lenBuffer, GetLocalizedText("TXT_KEY_MISC_DESTROYED_CITY", pCity->getNameKey()).GetCString());
		GC.GetEngineUserInterface()->AddCityMessage(0, pCity->GetIDInfo(), GetID(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer/*, "AS2D_CITYRAZE", MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pCity->getX(), pCity->getY(), true, true*/);

	}

	for(iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if(GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if(iI != GetID() && iI == GC.getGame().getActivePlayer())
			{
				if(pCity->isRevealed(GET_PLAYER((PlayerTypes)iI).getTeam(), false))
				{
					sprintf_s(szBuffer, lenBuffer, GetLocalizedText("TXT_KEY_MISC_CITY_HAS_BEEN_RAZED_BY", pCity->getNameKey(), getCivilizationDescriptionKey()).GetCString());
					GC.GetEngineUserInterface()->AddCityMessage(0, pCity->GetIDInfo(), ((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer/*, "AS2D_CITYRAZED", MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pCity->getX(), pCity->getY(), true, true*/);
				}
			}
		}
	}

	sprintf_s(szBuffer, lenBuffer, GetLocalizedText("TXT_KEY_MISC_CITY_RAZED_BY", pCity->getNameKey(), getCivilizationShortDescriptionKey()).GetCString());
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GetID(), szBuffer, pCity->getX(), pCity->getY());

	pCity->SetIgnoreCityForHappiness(false);
	DoUpdateHappiness();

	int iPopulationDrop = 1;
	iPopulationDrop *= (100 + GetPlayerTraits()->GetRazeSpeedModifier());
	iPopulationDrop /= 100;
	int iTurnsToRaze = pCity->getPopulation();
	if(iPopulationDrop > 0)
	{
		iTurnsToRaze = (iTurnsToRaze + iPopulationDrop - 1) / iPopulationDrop;
	}

	pCity->ChangeRazingTurns(iTurnsToRaze);

	DoUpdateNextPolicyCost();

	// Update City UI
	if(GetID() == GC.getGame().getActivePlayer())
	{
		GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::unraze(CvCity* pCity)
{
	if (GetPlayerTraits()->IsNoAnnexing())
	{
		pCity->DoCreatePuppet();
	}
	else
	{
		pCity->DoAnnex();
	}

	pCity->ChangeRazingTurns(-pCity->GetRazingTurns());

	DoUpdateNextPolicyCost();

	// Update City UI
	if(GetID() == GC.getGame().getActivePlayer())
	{
		GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
	}
}



//	--------------------------------------------------------------------------------
void CvPlayer::disband(CvCity* pCity)
{
	CvPlot* pPlot = pCity->plot();

	if(getNumCities() == 1)
	{
		setFoundedFirstCity(false);
	}

	GC.getGame().addDestroyedCityName(pCity->getNameKey());

	for(int eBuildingType = 0; eBuildingType < GC.getNumBuildingInfos(); eBuildingType++)
	{
		CvBuildingEntry* buildingInfo = GC.getBuildingInfo((BuildingTypes) eBuildingType);
		if(buildingInfo)
		{
			// if this building exists
			int iExists = pCity->GetCityBuildings()->GetNumRealBuilding((BuildingTypes) eBuildingType);
			int iPreferredPosition = buildingInfo->GetPreferredDisplayPosition();
			if(iPreferredPosition > 0)
			{
				auto_ptr<ICvCity1> pDllCity(new CvDllCity(pCity));

				if(iExists > 0)
				{
					// kill the wonder
					GC.GetEngineUserInterface()->AddDeferredWonderCommand(WONDER_REMOVED, pDllCity.get(), (BuildingTypes) eBuildingType, 0);
				}
				else
				{
					// else if we are currently in the process of building this wonder
					if(pCity->getProductionBuilding() == eBuildingType)
					{
						// kill the half built wonder
						if(isWorldWonderClass(buildingInfo->GetBuildingClassInfo()))
						{
							GC.GetEngineUserInterface()->AddDeferredWonderCommand(WONDER_REMOVED, pDllCity.get(), (BuildingTypes) eBuildingType, 0);
						}
					}
				}
			}
		}
	}

	{
		auto_ptr<ICvCity1> pkDllCity(new CvDllCity(pCity));
		gDLL->GameplayCitySetDamage(pkDllCity.get(), 0, pCity->getDamage());
		gDLL->GameplayCityDestroyed(pkDllCity.get(), NO_PLAYER);
	}

#ifdef REPLAY_EVENTS
	std::vector<int> vArgs;
	vArgs.push_back(pCity->plot()->GetPlotIndex());
	GC.getGame().addReplayEvent(REPLAYEVENT_PlotNewCityName, GetID(), vArgs, "NO_CITY");
#endif
	pCity->kill();

	if(pPlot)
	{
		IDInfoVector currentUnits;
		if (pPlot->getUnits(&currentUnits) > 0)
		{
			for (IDInfoVector::const_iterator itr = currentUnits.begin(); itr != currentUnits.end(); ++itr)
			{
				CvUnit* pUnit = ::getUnit(*itr);

				if(pUnit && !pPlot->isValidDomainForLocation(*pUnit))
				{
					if (!pUnit->jumpToNearestValidPlot())
						pUnit->kill(false);
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Is a Particular Goody ID a valid Goody for a certain plot?
bool CvPlayer::canReceiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit) const
{
	CvCity* pCity;
	UnitTypes eUnit;
	bool bTechFound;
	int iI;

	Database::SingleResult kResult;
	CvGoodyInfo kGoodyInfo;
	const bool bResult = DB.SelectAt(kResult, "GoodyHuts", eGoody);
	DEBUG_VARIABLE(bResult);
	CvAssertMsg(bResult, "Cannot find goody info.");
	kGoodyInfo.CacheResult(kResult);

	if(!CvGoodyHuts::IsCanPlayerReceiveGoody(GetID(), eGoody))
	{
		return false;
	}

#ifdef XP_RUINS_FIX
	if(kGoodyInfo.getExperience() > 0)
	{
		if(pUnit == NULL)
		{
			return false;
		}
		if (!pUnit->canAcquirePromotionAny())
		{
			return false;
		}
#ifdef EXPIRIENCE_RUIN_SET_MOVEMENT_PROMOTION
		PromotionTypes eMobility = (PromotionTypes)GC.getInfoTypeForString("PROMOTION_MOBILITY", true);
		if (pUnit->isHasPromotion(eMobility))
		{
			return false;
		}
#endif
	}
#else
	// No XP in first 10 turns
	/*if(kGoodyInfo.getExperience() > 0)
	{
		if((pUnit == NULL) || !(pUnit->canAcquirePromotionAny()) || (GC.getGame().getElapsedGameTurns() < 10))
		{
			return false;
		}
	}*/
#endif

	// Unit Healing
	if(kGoodyInfo.getDamagePrereq() > 0)
	{
		if((pUnit == NULL) || (pUnit->getDamage() < ((pUnit->GetMaxHitPoints() * kGoodyInfo.getDamagePrereq()) / 100)))
		{
			return false;
		}
	}

#ifdef REMOVE_EARLY_CULTURE_RUINS
	if (kGoodyInfo.getCulture() > 0)
	{
		if (GC.getGame().getElapsedGameTurns() < 10)
		{
			return false;
		}
	}
#endif

	// Early pantheon
	if(kGoodyInfo.isPantheonFaith())
	{
		if(GC.getGame().getElapsedGameTurns() < 20)
		{
			return false;
		}
		else
		{
			return (!GetReligions()->HasCreatedPantheon() && !GetReligions()->HasCreatedReligion());
		}
	}

	// Faith toward Great Prophet
	if(kGoodyInfo.getProphetPercent() > 0)
	{
		if(GC.getGame().getElapsedGameTurns() < 20)
		{
			return false;
		}
		else
		{
			return (GetReligions()->HasCreatedPantheon() && !GetReligions()->HasCreatedReligion());
		}
	}

	// Population
	if(kGoodyInfo.getPopulation() > 0)
	{
		if(getNumCities() == 0)
		{
			return false;
		}

		// Don't give more Population if we're already over our Pop limit
		if(IsEmpireUnhappy())
		{
			return false;
		}
	}

	// Reveal Nearby Barbs
	if(kGoodyInfo.getRevealNearbyBarbariansRange() > 0)
	{
		int iDX, iDY;
		int iBarbCampDistance = kGoodyInfo.getRevealNearbyBarbariansRange();
		CvPlot* pNearbyPlot;

		int iNumCampsFound = 0;

		ImprovementTypes barbCampType = (ImprovementTypes) GC.getBARBARIAN_CAMP_IMPROVEMENT();

		// Look at nearby Plots to make sure another camp isn't too close
		for(iDX = -(iBarbCampDistance); iDX <= iBarbCampDistance; iDX++)
		{
			for(iDY = -(iBarbCampDistance); iDY <= iBarbCampDistance; iDY++)
			{
				pNearbyPlot = plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);

				if(pNearbyPlot != NULL)
				{
					if(plotDistance(pNearbyPlot->getX(), pNearbyPlot->getY(), pPlot->getX(), pPlot->getY()) <= iBarbCampDistance)
					{
						if(pNearbyPlot->getImprovementType() == barbCampType)
						{
							iNumCampsFound++;
						}
					}
				}
			}
		}

		// Needs to be at least 2 nearby Camps
		if(iNumCampsFound < 2)
		{
			return false;
		}
	}

	// Reveal Unknown Resource
	if(kGoodyInfo.isRevealUnknownResource())
	{
		// Can't get this if you have no Capital City
		if(getCapitalCity() == NULL)
		{
			return false;
		}

		CvResourceInfo* pResource;
		ResourceClassTypes eResourceClassBonus = (ResourceClassTypes) GC.getInfoTypeForString("RESOURCECLASS_BONUS");

		bool bPlayerDoesntKnowOfResource = false;

		int iNumResourceInfos = GC.getNumResourceInfos();
		for(int iResourceLoop = 0; iResourceLoop < iNumResourceInfos; iResourceLoop++)
		{
			pResource = GC.getResourceInfo((ResourceTypes) iResourceLoop);

			// No "Bonus" Resources (that only give Yield), because those are lame to get from a Hut
			if(pResource != NULL && pResource->getResourceClassType() != eResourceClassBonus)
			{
				if(!GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes) pResource->getTechReveal()))
				{
					bPlayerDoesntKnowOfResource = true;
				}
			}
		}

		// If the player already knows where all the Resources are then there's no point in this Goody
		if(!bPlayerDoesntKnowOfResource)
		{
			return false;
		}
	}

	// Unit Upgrade
	if(kGoodyInfo.isUpgradeUnit())
	{
		if(pUnit == NULL)
		{
			return false;
		}

		if(pUnit->IsHasBeenPromotedFromGoody())
		{
			return false;
		}

		UnitClassTypes eUpgradeUnitClass = (UnitClassTypes) GC.getUnitInfo(pUnit->getUnitType())->GetGoodyHutUpgradeUnitClass();

		if(eUpgradeUnitClass == NO_UNITCLASS)
		{
			return false;
		}

		UnitTypes eUpgradeUnit = (UnitTypes) getCivilizationInfo().getCivilizationUnits(eUpgradeUnitClass);

		if(eUpgradeUnit == NO_UNIT)
		{
			return false;
		}
	}

	// Tech
	if(kGoodyInfo.isTech())
	{
		bTechFound = false;

		int iNumTechInfos = GC.getNumTechInfos();
		for(iI = 0; iI < iNumTechInfos; iI++)
		{
			const TechTypes eTech = static_cast<TechTypes>(iI);
			CvTechEntry* pkTech = GC.getTechInfo(eTech);
			if(pkTech != NULL && pkTech->IsGoodyTech())
			{
				if(GetPlayerTechs()->CanResearch(eTech))
				{
					bool bUseTech = true;
					ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
					if (pkScriptSystem) 
					{
						CvLuaArgsHandle args;
						args->Push(GetID());
						args->Push(eTech);

						// Attempt to execute the game events.
						// Will return false if there are no registered listeners.
						bool bScriptResult = false;
						if (LuaSupport::CallTestAll(pkScriptSystem, "GoodyHutCanResearch", args.get(), bScriptResult)) 
						{
							bUseTech = bResult;
						}
					}

					if(bUseTech)
					{
						bTechFound = true;
					}
					break;
				}
			}
		}

		if(!bTechFound)
		{
			return false;
		}
	}

#ifdef NEW_RUIN_EXPANSION
	// Expansion
	if (kGoodyInfo.getNumExpanse() > 0)
	{
		if (getNumCities() == 0)
		{
			return false;
		}
	}
#endif

	///////////////////////////////////////
	///////////////////////////////////////
	// Bad Goodies follow beneath this line
	///////////////////////////////////////
	///////////////////////////////////////

	if(kGoodyInfo.isBad())
	{
		if((pUnit == NULL) || pUnit->isNoBadGoodies())
		{
			return false;
		}
	}

	if(kGoodyInfo.getUnitClassType() != NO_UNITCLASS)
	{
		eUnit = ((UnitTypes)(getCivilizationInfo().getCivilizationUnits(kGoodyInfo.getUnitClassType())));

		if(eUnit == NO_UNIT)
		{
			return false;
		}

		CvUnitEntry* pUnitInfo = GC.getUnitInfo(eUnit);
		if(pUnitInfo == NULL)
		{
			return false;
		}

		// No combat units in MP in the first 20 turns
		if(pUnitInfo->GetCombat() > 0)
		{
			if(GC.getGame().isGameMultiPlayer() || (GC.getGame().getElapsedGameTurns() < 20))
			{
				return false;
			}
		}

		// Builders
		if(pUnitInfo->GetWorkRate() > 0)
		{
			// Max limit
			if(GetMaxNumBuilders() > -1 && GetNumBuilders() >= GetMaxNumBuilders())
			{
				return false;
			}

			bool bHasTechWhichUnlocksImprovement = false;

			// Need a tech which unlocks something to do
			int iNumTechInfos = GC.getNumTechInfos();
			int iNumBuildInfos = GC.getNumBuildInfos();
			for(int iTechLoop = 0; iTechLoop < iNumTechInfos; iTechLoop++)
			{
				if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes) iTechLoop))
				{
					// Look at Builds
					for(int iBuildLoop = 0; iBuildLoop < iNumBuildInfos; iBuildLoop++)
					{
						CvBuildInfo* pkBuildInfo = GC.getBuildInfo((BuildTypes) iBuildLoop);
						if(!pkBuildInfo)
						{
							continue;
						}
						if(pkBuildInfo->getTechPrereq() == (TechTypes) iTechLoop)
						{
							if(pkBuildInfo->getImprovement() != NO_IMPROVEMENT || pkBuildInfo->getRoute() != NO_ROUTE)
							{
								bHasTechWhichUnlocksImprovement = true;
								break;
							}
						}
					}
				}
				// Already found something
				if(bHasTechWhichUnlocksImprovement)
				{
					break;
				}
			}

			// Player doesn't have any Tech which allows Improvements
			if(!bHasTechWhichUnlocksImprovement)
			{
				return false;
			}
		}

		// OCC games - no Settlers
		if(GetPlayerTraits()->IsNoAnnexing() || (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman()))
		{
			if(pUnitInfo->IsFound() || pUnitInfo->IsFoundAbroad())
			{
				return false;
			}
		}
	}

	// Barbarians
	if(kGoodyInfo.getBarbarianUnitClass() != NO_UNITCLASS)
	{
#ifdef DUEL_MOVING_SOME_OPTIONS_TO_DUEL_MODE
		if(GC.getGame().isNetworkMultiPlayer() && GC.getGame().isOption("GAMEOPTION_DUEL_STUFF") && GC.getGame().isOption(GAMEOPTION_NO_BARBARIANS) || !GC.getGame().isNetworkMultiPlayer() && GC.getGame().isOption(GAMEOPTION_NO_BARBARIANS))
#else
		if(GC.getGame().isOption(GAMEOPTION_NO_BARBARIANS))
#endif
		{
			return false;
		}

		if(getNumCities() == 0)
		{
			return false;
		}

		if(getNumCities() == 1)
		{
			pCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(), NO_PLAYER, getTeam());

			if(pCity != NULL)
			{
				if(plotDistance(pPlot->getX(), pPlot->getY(), pCity->getX(), pCity->getY()) <= (8 - getNumCities()))
				{
					return false;
				}
			}
		}
	}

	return true;
}


//	--------------------------------------------------------------------------------
void CvPlayer::receiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit)
{
#ifdef UPDATE_CULTURE_NOTIFICATION_DURING_TURN
		CvGame& kGame = GC.getGame();
#endif
	CvPlot* pLoopPlot;
	CvPlot* pBestPlot = NULL;
	CvString strBuffer;
	CvString strTempBuffer;
#ifndef FLAT_SCIENCE_FROM_TECH_RUIN
	TechTypes eBestTech;
#endif
	UnitTypes eUnit;
	int iGold;
	int iOffset;
	int iRange;
	int iBarbCount;
	int iValue;
	int iBestValue;
	int iPass;
	int iDX, iDY;
	int iI;

	CvAssertMsg(canReceiveGoody(pPlot, eGoody, pUnit), "Instance is expected to be able to recieve goody");

	Database::SingleResult kResult;
	CvGoodyInfo kGoodyInfo;
	const bool bResult = DB.SelectAt(kResult, "GoodyHuts", eGoody);
	DEBUG_VARIABLE(bResult);
	CvAssertMsg(bResult, "Cannot find goody info.");
	kGoodyInfo.CacheResult(kResult);

	CvGoodyHuts::DoPlayerReceivedGoody(GetID(), eGoody);

#ifdef AUI_PLAYER_RECEIVE_GOODY_PLOT_MESSAGE_FOR_YIELD
	int iNumYieldBonuses = 0;
#endif

	strBuffer = kGoodyInfo.GetDescription();

	// Gold
	iGold = kGoodyInfo.getGold() + (kGoodyInfo.getNumGoldRandRolls() * GC.getGame().getJonRandNum(kGoodyInfo.getGoldRandAmount(), "Goody Gold Rand"));

	if(iGold != 0)
	{
		GetTreasury()->ChangeGold(iGold);

#ifdef AUI_PLAYER_FIX_RECEIVE_GOODY_MESSAGE
		strBuffer = GetLocalizedText(kGoodyInfo.GetDescriptionKey(), iGold);
#else
		strBuffer += GetLocalizedText("TXT_KEY_MISC_RECEIVED_GOLD", iGold);
#endif
#ifdef AUI_PLAYER_RECEIVE_GOODY_PLOT_MESSAGE_FOR_YIELD
		ReportYieldFromKill(YIELD_GOLD, iGold, pPlot->getX(), pPlot->getY(), iNumYieldBonuses);
		iNumYieldBonuses += 1;
#endif
	}

	// Population
	if(kGoodyInfo.getPopulation() > 0)
	{
		int iDistance;
		int iBestCityDistance = -1;
		CvCity* pBestCity = NULL;

		CvCity* pLoopCity;
		int iLoop;
		// Find the closest City to us to add a Pop point to
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iDistance = plotDistance(pPlot->getX(), pPlot->getY(), pLoopCity->getX(), pLoopCity->getY());

			if(iBestCityDistance == -1 || iDistance < iBestCityDistance)
			{
				iBestCityDistance = iDistance;
				pBestCity = pLoopCity;
			}
		}

		if(pBestCity != NULL)
		{
#ifdef POP_RUIN_FOOD_NOT_POPULATION
			pBestCity->changeFood(20 * kGoodyInfo.getPopulation() * GC.getGame().getGameSpeedInfo().getGrowthPercent()/100);
#else
			pBestCity->changePopulation(kGoodyInfo.getPopulation());
#endif
#ifdef CITY_BANNER_MISSING_UPDATES_FIX
			if ((pBestCity->getOwner() == GC.getGame().getActivePlayer()) && pBestCity->isCitySelected())
			{
				DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			}

			auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(pBestCity);
			DLLUI->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_BANNER);
#endif
		}
	}

	// Culture
	int iCulture = kGoodyInfo.getCulture();
	if(iCulture > 0)
	{
		// Game Speed Mod
		iCulture *= GC.getGame().getGameSpeedInfo().getCulturePercent();
		iCulture /= 100;

		changeJONSCulture(iCulture);

#ifdef UPDATE_CULTURE_NOTIFICATION_DURING_TURN
		// if this is the human player, have the popup come up so that he can choose a new policy
		if(isAlive() && isHuman() && getNumCities() > 0)
		{
			if(!GC.GetEngineUserInterface()->IsPolicyNotificationSeen())
			{
				if(getNextPolicyCost() <= getJONSCulture() && GetPlayerPolicies()->GetNumPoliciesCanBeAdopted() > 0)
				{
					CvNotifications* pNotifications = GetNotifications();
					if(pNotifications)
					{
						CvString strBuffer;

						if(kGame.isOption(GAMEOPTION_POLICY_SAVING))
							strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY_DISMISS");
						else
							strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY");

						CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ENOUGH_CULTURE_FOR_POLICY");
						pNotifications->Add(NOTIFICATION_POLICY, strBuffer, strSummary, -1, -1, -1);
					}
				}
			}
		}
#endif
#ifdef AUI_PLAYER_FIX_RECEIVE_GOODY_MESSAGE
		strBuffer = GetLocalizedText(kGoodyInfo.GetDescriptionKey(), iCulture);
#endif
#ifdef AUI_PLAYER_RECEIVE_GOODY_PLOT_MESSAGE_FOR_YIELD
		ReportYieldFromKill(YIELD_CULTURE, iCulture, pPlot->getX(), pPlot->getY(), iNumYieldBonuses);
		iNumYieldBonuses += 1;
#endif
	}

	// Faith
	int iFaith = kGoodyInfo.getFaith();
	if(iFaith > 0)
	{
		// Game Speed Mod
		iFaith *= GC.getGame().getGameSpeedInfo().getFaithPercent();
		iFaith /= 100;

		ChangeFaith(iFaith);
#ifdef AUI_PLAYER_FIX_RECEIVE_GOODY_MESSAGE
		strBuffer = GetLocalizedText(kGoodyInfo.GetDescriptionKey(), iFaith);
#endif
#ifdef AUI_PLAYER_RECEIVE_GOODY_PLOT_MESSAGE_FOR_YIELD
		ReportYieldFromKill(YIELD_FAITH, iFaith, pPlot->getX(), pPlot->getY(), iNumYieldBonuses);
		iNumYieldBonuses += 1;
#endif
	}

	// Faith for pantheon
	bool bPantheon = kGoodyInfo.isPantheonFaith();
	if(bPantheon)
	{
		// Enough so still get a pantheon if 3 civs pop this in same turn
		iFaith = GC.getGame().GetGameReligions()->GetMinimumFaithNextPantheon() + 2 * GC.getRELIGION_GAME_FAITH_DELTA_NEXT_PANTHEON();
		int iDivisor = /*10*/ GC.getGOLD_PURCHASE_VISIBLE_DIVISOR();
		iFaith /= iDivisor;
		iFaith *= iDivisor;
		ChangeFaith(iFaith);
#ifdef AUI_PLAYER_FIX_RECEIVE_GOODY_MESSAGE
		strBuffer = GetLocalizedText(kGoodyInfo.GetDescriptionKey(), iFaith);
#endif
#ifdef AUI_PLAYER_RECEIVE_GOODY_PLOT_MESSAGE_FOR_YIELD
		ReportYieldFromKill(YIELD_FAITH, iFaith, pPlot->getX(), pPlot->getY(), iNumYieldBonuses);
		iNumYieldBonuses += 1;
#endif
	}

	// Faith for percent of great prophet
	int iProphetPercent = kGoodyInfo.getProphetPercent();
	if(iProphetPercent > 0)
	{
		iFaith = GetReligions()->GetCostNextProphet(false /*bIncludeBeliefDiscounts*/, true /*bAdjustForSpeedDifficulty*/) * iProphetPercent / 100;
		int iDivisor = /*10*/ GC.getGOLD_PURCHASE_VISIBLE_DIVISOR();
		iFaith /= iDivisor;
		iFaith *= iDivisor;
		ChangeFaith(iFaith);
#ifdef AUI_PLAYER_FIX_RECEIVE_GOODY_MESSAGE
		strBuffer = GetLocalizedText(kGoodyInfo.GetDescriptionKey(), iFaith);
#endif
#ifdef AUI_PLAYER_RECEIVE_GOODY_PLOT_MESSAGE_FOR_YIELD
		ReportYieldFromKill(YIELD_FAITH, iFaith, pPlot->getX(), pPlot->getY(), iNumYieldBonuses);
		iNumYieldBonuses += 1;
#endif
	}

	// Reveal Nearby Barbs
	if(kGoodyInfo.getRevealNearbyBarbariansRange() > 0)
	{
		// Look at nearby Plots to make sure another camp isn't too close
		const int iBarbCampDistance = kGoodyInfo.getRevealNearbyBarbariansRange();
		for(iDX = -(iBarbCampDistance); iDX <= iBarbCampDistance; iDX++)
		{
			for(iDY = -(iBarbCampDistance); iDY <= iBarbCampDistance; iDY++)
			{
				CvPlot* pNearbyBarbarianPlot = plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
				if(pNearbyBarbarianPlot != NULL)
				{
					if(plotDistance(pNearbyBarbarianPlot->getX(), pNearbyBarbarianPlot->getY(), pPlot->getX(), pPlot->getY()) <= iBarbCampDistance)
					{
						if(pNearbyBarbarianPlot->getImprovementType() == GC.getBARBARIAN_CAMP_IMPROVEMENT())
						{
							// Reveal Plot
							pNearbyBarbarianPlot->setRevealed(getTeam(), true);
							// Reveal Barb Camp here
							pNearbyBarbarianPlot->setRevealedImprovementType(getTeam(), pNearbyBarbarianPlot->getImprovementType());
						}
					}
				}
			}
		}
	}

	// Map
	iRange = kGoodyInfo.getMapRange();

	if(iRange > 0)
	{
		iOffset = kGoodyInfo.getMapOffset();

		if(iOffset > 0)
		{
			iBestValue = 0;
			pBestPlot = NULL;

			int iRandLimit;

			for(iDX = -(iOffset); iDX <= iOffset; iDX++)
			{
				for(iDY = -(iOffset); iDY <= iOffset; iDY++)
				{
					pLoopPlot = plotXYWithRangeCheck(pPlot->getX(), pPlot->getY(), iDX, iDY, iOffset);

					if(pLoopPlot != NULL)
					{
						if(!(pLoopPlot->isRevealed(getTeam())))
						{
							// Avoid water plots!
							if(pPlot->isWater())
								iRandLimit = 10;
							else
								iRandLimit = 10000;

							iValue = (1 + GC.getGame().getJonRandNum(iRandLimit, "Goody Map"));

							iValue *= plotDistance(pPlot->getX(), pPlot->getY(), pLoopPlot->getX(), pLoopPlot->getY());

							if(iValue > iBestValue)
							{
								iBestValue = iValue;
								pBestPlot = pLoopPlot;
							}
						}
					}
				}
			}
		}

		if(pBestPlot == NULL)
		{
			pBestPlot = pPlot;
		}

		for(iDX = -(iRange); iDX <= iRange; iDX++)
		{
			for(iDY = -(iRange); iDY <= iRange; iDY++)
			{
				pLoopPlot = plotXY(pBestPlot->getX(), pBestPlot->getY(), iDX, iDY);

				if(pLoopPlot != NULL)
				{
					if(plotDistance(pBestPlot->getX(), pBestPlot->getY(), pLoopPlot->getX(), pLoopPlot->getY()) <= iRange)
					{
						if(GC.getGame().getJonRandNum(100, "Goody Map") < kGoodyInfo.getMapProb())
						{
							pLoopPlot->setRevealed(getTeam(), true);
						}
					}
				}
			}
		}
	}

	// Experience
	if(pUnit != NULL)
	{
#ifdef EXPIRIENCE_RUIN_SET_MOVEMENT_PROMOTION
		if (kGoodyInfo.getExperience() > 0)
		{
			PromotionTypes eMobility = (PromotionTypes)GC.getInfoTypeForString("PROMOTION_MOBILITY", true);
			pUnit->setHasPromotion(eMobility, true);
		}
#else
		pUnit->changeExperience(kGoodyInfo.getExperience());
#endif
	}

	// Unit Heal
	if(pUnit != NULL)
	{
		pUnit->changeDamage(-(kGoodyInfo.getHealing()));
	}

	// Reveal Unknown Resource
	if(kGoodyInfo.isRevealUnknownResource())
	{
		if(getCapitalCity() != NULL)
		{
			CvCity* pCapital = getCapitalCity();

			CvPlot* pResourcePlot;
			int iResourceDistance;
			TechTypes eRevealTech;
			int iResourceCost;
			int iBestResourceCost = -1;
			ResourceTypes eResource;
			ResourceTypes eBestResource = NO_RESOURCE;
			CvPlot* pBestResourcePlot = NULL;

			ResourceClassTypes eResourceClassBonus;

			// Look at Resources on all Plots
			for(int iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
			{
				pResourcePlot = GC.getMap().plotByIndexUnchecked(iPlotLoop);
				eResource = pResourcePlot->getResourceType();

				if(eResource != NO_RESOURCE)
				{
					CvResourceInfo& pResource = *GC.getResourceInfo(eResource);
					eResourceClassBonus = (ResourceClassTypes) GC.getInfoTypeForString("RESOURCECLASS_BONUS");

					// No "Bonus" Resources (that only give Yield), because those are lame to get from a Hut
					if(pResource.getResourceClassType() != eResourceClassBonus)
					{
						// Can't be on a Plot that we've already force-revealed!
						if(!pResourcePlot->IsResourceForceReveal(getTeam()))
						{
							// Must be a Resource we don't already see
							eRevealTech = (TechTypes) pResource.getTechReveal();
							if(!GET_TEAM(getTeam()).GetTeamTechs()->HasTech(eRevealTech))
							{
								iResourceDistance = plotDistance(pResourcePlot->getX(), pResourcePlot->getY(), pCapital->getX(), pCapital->getY());

								// Must be within 10 plots of our Capital
								if(iResourceDistance <= 10)
								{
									iResourceCost = GC.getTechInfo(eRevealTech)->GetResearchCost();

									// Find the one with the cheapest Tech (or pick one if we haven't identified one yet)
									if(iBestResourceCost == -1 || iResourceCost < iBestResourceCost)
									{
										iBestResourceCost = iResourceCost;
										eBestResource = eResource;
										pBestResourcePlot = pResourcePlot;
									}
								}
							}
						}
					}
				}
			}

			CvAssert(pBestResourcePlot);

			// Did we find something to show?
			if(pBestResourcePlot != NULL)
			{
				pBestResourcePlot->setRevealed(getTeam(), true);
				pBestResourcePlot->SetResourceForceReveal(getTeam(), true);
				//pBestPlot->updateFog();

				if(getTeam() == GC.getGame().getActiveTeam())
				{
					pBestResourcePlot->setLayoutDirty(true);
				}

				// Also reveal adjacent Plots
				CvPlot* pAdjacentPlot;
				for(int iDirectionLoop = 0; iDirectionLoop < NUM_DIRECTION_TYPES; iDirectionLoop++)
				{
					pAdjacentPlot = plotDirection(pBestResourcePlot->getX(), pBestResourcePlot->getY(), ((DirectionTypes) iDirectionLoop));

					if(pAdjacentPlot != NULL)
					{
						pAdjacentPlot->setRevealed(getTeam(), true);
					}
				}

				CvString strTempString;
				strTempString.Format(" (%s)", GC.getResourceInfo(eBestResource)->GetDescription());
				strBuffer += strTempString;
			}
		}
	}

	// Unit Upgrade
	if(kGoodyInfo.isUpgradeUnit())
	{
		UnitClassTypes eUpgradeUnitClass = NO_UNITCLASS;
		UnitTypes eUpgradeUnit = NO_UNIT;

		if(pUnit != NULL)
		{
			eUpgradeUnitClass = (UnitClassTypes) pUnit->getUnitInfo().GetGoodyHutUpgradeUnitClass();
			eUpgradeUnit = (UnitTypes) getCivilizationInfo().getCivilizationUnits(eUpgradeUnitClass);
		}
		
		if(eUpgradeUnit != NO_UNIT)
		{
			// Add new upgrade Unit

			// if we promoted an scouting unit from a goody hut, turn him into whatever the new unit's default AI is if it is not a suitable explorer anymore
			UnitAITypes currentAIDefault = pUnit->AI_getUnitAIType();
			UnitAITypes newAIDefault = (UnitAITypes)GC.getUnitInfo(eUpgradeUnit)->GetDefaultUnitAIType();
			if(currentAIDefault == UNITAI_EXPLORE)
			{
				if(newAIDefault == UNITAI_EXPLORE || newAIDefault == UNITAI_ATTACK || newAIDefault == UNITAI_DEFENSE || newAIDefault == UNITAI_FAST_ATTACK || newAIDefault == UNITAI_COUNTER)
				{
					newAIDefault = UNITAI_EXPLORE;
				}
			}

			CvUnit* pNewUnit = initUnit(eUpgradeUnit, pPlot->getX(), pPlot->getY(), newAIDefault, NO_DIRECTION, false, false, 0, pUnit->GetNumGoodyHutsPopped());
			pUnit->finishMoves();
			pUnit->SetBeenPromotedFromGoody(true);

			CvAssert(pNewUnit);
			if (pNewUnit != NULL)
			{
				pNewUnit->convert(pUnit, true);
				pNewUnit->setupGraphical();

				ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
				if (pkScriptSystem)
				{
					CvLuaArgsHandle args;
					args->Push(GetID());
					args->Push(pUnit->GetID());
					args->Push(pNewUnit->GetID());
					args->Push(true); // bGoodyHut

					bool bScriptResult;
					LuaSupport::CallHook(pkScriptSystem, "UnitUpgraded", args.get(), bScriptResult);
				}
			}
			else
				pUnit->kill(false);

			// Since the old unit died, it will block the goody reward popup unless we call this
			GC.GetEngineUserInterface()->SetDontShowPopups(false);
		}
	}

	// Tech
	if(kGoodyInfo.isTech())
	{
#ifdef FLAT_SCIENCE_FROM_TECH_RUIN
		int iValue = 38;
		iValue *= GC.getGame().getGameSpeedInfo().getResearchPercent();
		iValue /= 100;
		TechTypes eCurrentTech = GetPlayerTechs()->GetCurrentResearch();
		if (eCurrentTech == NO_TECH)
		{
			changeOverflowResearch(iValue);
		}
		else
		{
			GET_TEAM(getTeam()).GetTeamTechs()->ChangeResearchProgress(eCurrentTech, iValue, GetID());
		}
#ifdef AUI_PLAYER_FIX_RECEIVE_GOODY_MESSAGE
		strBuffer = GetLocalizedText(kGoodyInfo.GetDescriptionKey(), iValue);
#endif
#else
		iBestValue = 0;
		eBestTech = NO_TECH;

		for(iI = 0; iI < GC.getNumTechInfos(); iI++)
		{
			const TechTypes eTech = static_cast<TechTypes>(iI);
			CvTechEntry* pkTech = GC.getTechInfo(eTech);
			if(pkTech != NULL && pkTech->IsGoodyTech())
			{
				if(GetPlayerTechs()->CanResearch(eTech))
				{
					bool bUseTech = true;

					ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
					if (pkScriptSystem)
					{
						CvLuaArgsHandle args;
						args->Push(GetID());
						args->Push(eTech);

						// Attempt to execute the game events.
						// Will return false if there are no registered listeners.
						bool bScriptResult = false;
						if (LuaSupport::CallTestAll(pkScriptSystem, "GoodyHutCanResearch", args.get(), bScriptResult))
						{
							bUseTech = bScriptResult;
						}
					}

					if(bUseTech)
					{
						iValue = (1 + GC.getGame().getJonRandNum(10000, "Goody Tech"));

						if(iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestTech = eTech;
						}
					}
				}
			}
		}

		CvAssertMsg(eBestTech != NO_TECH, "BestTech is not assigned a valid value");

		ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
		if (pkScriptSystem) 
		{
			CvLuaArgsHandle args;
			args->Push(GetID());
			args->Push(eBestTech);

			bool bScriptResult;
			LuaSupport::CallHook(pkScriptSystem, "GoodyHutTechResearched", args.get(), bScriptResult);
		}

		GET_TEAM(getTeam()).setHasTech(eBestTech, true, GetID(), true, true);
		GET_TEAM(getTeam()).GetTeamTechs()->SetNoTradeTech(eBestTech, true);
#endif
	}

	// Units
	if(kGoodyInfo.getUnitClassType() != NO_UNITCLASS)
	{
		eUnit = (UnitTypes)getCivilizationInfo().getCivilizationUnits(kGoodyInfo.getUnitClassType());

		if(eUnit != NO_UNIT)
		{
			CvUnit* pNewUnit = initUnit(eUnit, pPlot->getX(), pPlot->getY());
			// see if there is an open spot to put him - no over-stacking allowed!
			if(pNewUnit && pUnit && pUnit->AreUnitsOfSameType(*pNewUnit))  // pUnit isn't in this plot yet (if it even exists) so we can't check on if we are over-stacked directly
			{
				pBestPlot = NULL;
				iBestValue = INT_MAX;
				const int iPopRange = 2;
				for(iDX = -(iPopRange); iDX <= iPopRange; iDX++)
				{
					for(iDY = -(iPopRange); iDY <= iPopRange; iDY++)
					{
						pLoopPlot	= plotXYWithRangeCheck(pPlot->getX(), pPlot->getY(), iDX, iDY, iPopRange);
						if(pLoopPlot != NULL)
						{
							if(pLoopPlot->isValidDomainForLocation(*pNewUnit))
							{
								if(pNewUnit->canMoveInto(*pLoopPlot))
								{
									if(pLoopPlot->getNumFriendlyUnitsOfType(pUnit) < GC.getPLOT_UNIT_LIMIT())
									{
										if(pNewUnit->canEnterTerritory(pLoopPlot->getTeam()) && !pNewUnit->isEnemy(pLoopPlot->getTeam(), pLoopPlot))
										{
											if((pNewUnit->getDomainType() != DOMAIN_AIR) || pLoopPlot->isFriendlyCity(*pNewUnit, true))
											{
												if(pLoopPlot->isRevealed(getTeam()))
												{
													iValue = 1 + GC.getGame().getJonRandNum(6, "spawn goody unit that would over-stack"); // okay, I'll admit it, not a great heuristic

													if(plotDistance(pPlot->getX(),pPlot->getY(),pLoopPlot->getX(),pLoopPlot->getY()) > 1)
													{
														iValue += 12;
													}

													if(pLoopPlot->area() != pPlot->area())  // jumped to a different land mass, cool
													{
														iValue *= 10;
													}

													if(iValue < iBestValue)
													{
														iBestValue = iValue;
														pBestPlot = pLoopPlot;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				if(pBestPlot != NULL)
				{
					bool bVis = pBestPlot->isVisibleToWatchingHuman();
					pNewUnit->setXY(pBestPlot->getX(), pBestPlot->getY(), false, true, true && bVis, true);
					pNewUnit->SetPosition(pBestPlot);	// Need this to put the unit in the right spot graphically
					pNewUnit->finishMoves();
				}
				else
				{
					pNewUnit->kill(false);
				}
			}
		}
	}

	// Barbarians
	if(kGoodyInfo.getBarbarianUnitClass() != NO_UNITCLASS)
	{
		iBarbCount = 0;

		eUnit = (UnitTypes)GET_PLAYER(BARBARIAN_PLAYER).getCivilizationInfo().getCivilizationUnits(kGoodyInfo.getBarbarianUnitClass());

		if(eUnit != NO_UNIT)
		{
			for(iPass = 0; iPass < 10; iPass++)
			{
				if(iBarbCount < kGoodyInfo.getMinBarbarians())
				{
					for(iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
					{
						pLoopPlot = plotDirection(pPlot->getX(), pPlot->getY(), ((DirectionTypes)iI));

						if(pLoopPlot != NULL)
						{
							if(pLoopPlot->getArea() == pPlot->getArea())
							{
								if(!(pLoopPlot->isImpassable()) && !pLoopPlot->isMountain() && !(pLoopPlot->getPlotCity()))
								{
									if(pLoopPlot->getNumUnits() == 0)
									{
										if((iPass > 0) || (GC.getGame().getJonRandNum(100, "Goody Barbs") < kGoodyInfo.getBarbarianUnitProb()))
										{
											GET_PLAYER(BARBARIAN_PLAYER).initUnit(eUnit, pLoopPlot->getX(), pLoopPlot->getY(), ((pLoopPlot->isWater()) ? UNITAI_ATTACK_SEA : UNITAI_ATTACK));
											iBarbCount++;

											if((iPass > 0) && (iBarbCount == kGoodyInfo.getMinBarbarians()))
											{
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

#ifdef NEW_RUIN_EXPANSION
	// Expansion
	if (kGoodyInfo.getNumExpanse() > 0)
	{
		int iDistance;
		int iBestCityDistance = -1;
		CvCity* pBestCity = NULL;

		CvCity* pLoopCity;
		int iLoop;
		// Find the closest City to us to add a Pop point to
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iDistance = plotDistance(pPlot->getX(), pPlot->getY(), pLoopCity->getX(), pLoopCity->getY());

			if (iBestCityDistance == -1 || iDistance < iBestCityDistance)
			{
				iBestCityDistance = iDistance;
				pBestCity = pLoopCity;
			}
		}

		if (pBestCity != NULL)
		{
			for (int iI = 0; iI < kGoodyInfo.getNumExpanse(); iI++)
			{
				CvPlot* pPlotToAcquire = pBestCity->GetNextBuyablePlot();

				// maybe the player owns ALL of the plots or there are none avaialable?
				if (pPlotToAcquire)
				{
					pBestCity->DoAcquirePlot(pPlotToAcquire->getX(), pPlotToAcquire->getY());
				}
			}
		}
	}
#endif
#ifdef REPLAY_EVENTS
	if (isHuman())
	{
		std::vector<int> vArgs;
		vArgs.push_back(static_cast<int>(eGoody));
		vArgs.push_back(pUnit ? static_cast<int>(pUnit->getUnitType()) : -1);
		GC.getGame().addReplayEvent(REPLAYEVENT_GoodyHut, GetID(), vArgs);
	}
#endif

	if(!strBuffer.empty() && GC.getGame().getActivePlayer() == GetID())
	{
		GC.GetEngineUserInterface()->AddPlotMessage(0, pPlot->GetPlotIndex(), GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer);
	}

	// If it's the active player then show the popup
	if(GetID() == GC.getGame().getActivePlayer())
	{
		GC.getMap().updateDeferredFog();

		bool bDontShowRewardPopup = GC.GetEngineUserInterface()->IsOptionNoRewardPopups();

		// Don't show in MP, or if the player has turned it off
		if(!GC.getGame().isNetworkMultiPlayer() && !bDontShowRewardPopup)	// KWG: Candidate for !GC.getGame().isOption(GAMEOPTION_SIMULTANEOUS_TURNS)
		{
			int iSpecialValue = 0;

			if(iGold > 0)
				iSpecialValue = iGold;
			else if(iCulture > 0)
				iSpecialValue = iCulture;
			else if(iFaith > 0)
				iSpecialValue = iFaith;

			CvPopupInfo kPopupInfo(BUTTONPOPUP_GOODY_HUT_REWARD, eGoody, iSpecialValue);
			GC.GetEngineUserInterface()->AddPopup(kPopupInfo);
			// We are adding a popup that the player must make a choice in, make sure they are not in the end-turn phase.
			CancelActivePlayerEndTurn();
		}
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::doGoody(CvPlot* pPlot, CvUnit* pUnit)
{
	CvHandicapInfo& playerHandicapInfo = getHandicapInfo();

	GoodyTypes eGoody;

	CvAssertMsg(pPlot->isGoody(), "pPlot->isGoody is expected to be true");

	if(!isBarbarian())
	{
		m_bEverPoppedGoody = true;
		pPlot->removeGoody();

		// Minors don't get Goodies :(
		if(isMinorCiv())
		{
			return;
		}

		// Need to have Goodies in the Handicap file to pick from
		if(playerHandicapInfo.getNumGoodies() > 0)
		{
			// Make a list of valid Goodies to pick randomly from
			int iValidGoodiesLoop;
			bool bValid;

			std::vector<GoodyTypes> avValidGoodies;
			for(int iGoodyLoop = 0; iGoodyLoop < playerHandicapInfo.getNumGoodies(); iGoodyLoop++)
			{
				eGoody = (GoodyTypes) playerHandicapInfo.getGoodies(iGoodyLoop);
				bValid = false;

				// Check to see if we've already verified this Goody is valid (since there can be multiples in the vector)
				for(iValidGoodiesLoop = 0; iValidGoodiesLoop < (int) avValidGoodies.size(); iValidGoodiesLoop++)
				{
					if(avValidGoodies[iValidGoodiesLoop] == eGoody)
					{
						avValidGoodies.push_back(eGoody);
						bValid = true;
						break;
					}
				}

				if(bValid)
					continue;

				if(canReceiveGoody(pPlot, eGoody, pUnit))
				{
					avValidGoodies.push_back(eGoody);
				}
			}

			// Any valid Goodies?
			if(avValidGoodies.size() > 0)
			{
				if (pUnit && pUnit->isHasPromotion((PromotionTypes)GC.getPROMOTION_GOODY_HUT_PICKER()))
				{
					if(GC.getGame().getActivePlayer() == GetID())
					{
						CvPopupInfo kPopupInfo(BUTTONPOPUP_CHOOSE_GOODY_HUT_REWARD, GetID(), pUnit->GetID());
						GC.GetEngineUserInterface()->AddPopup(kPopupInfo);
						// We are adding a popup that the player must make a choice in, make sure they are not in the end-turn phase.
						CancelActivePlayerEndTurn();
					}
				}
				else
				{
					int iRand = GC.getGame().getJonRandNum(avValidGoodies.size(), "Picking a Goody result");
					eGoody = (GoodyTypes) avValidGoodies[iRand];
					receiveGoody(pPlot, eGoody, pUnit);
				}
				
				if (pUnit && isHuman() && !GC.getGame().isGameMultiPlayer())
				{
					pUnit->ChangeNumGoodyHutsPopped(pUnit->GetNumGoodyHutsPopped() + 1);
					if (pUnit->isHasPromotion((PromotionTypes)GC.getPROMOTION_GOODY_HUT_PICKER()) && pUnit->GetNumGoodyHutsPopped() >= 5)
					{
						gDLL->UnlockAchievement(ACHIEVEMENT_XP2_25);
					}
				}
			}

			pPlot->AddArchaeologicalRecord(CvTypes::getARTIFACT_ANCIENT_RUIN(), m_eID, NO_PLAYER);
		}
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::AwardFreeBuildings(CvCity* pCity)
{
	int iNumFreeCultureBuildings = GetNumCitiesFreeCultureBuilding();
	if(iNumFreeCultureBuildings > 0)
	{
		BuildingTypes eBuilding = pCity->ChooseFreeCultureBuilding();
		if(eBuilding != NO_BUILDING)
		{
			pCity->GetCityBuildings()->SetNumFreeBuilding(eBuilding, 1);
		}
		else
		{
			pCity->SetOwedCultureBuilding(true);
		}

		ChangeNumCitiesFreeCultureBuilding(-1);
	}

	int iNumFreeFoodBuildings = GetNumCitiesFreeFoodBuilding();
	if(iNumFreeFoodBuildings > 0)
	{
		BuildingTypes eBuilding = pCity->ChooseFreeFoodBuilding();
		if (eBuilding != NO_BUILDING)
		{
#ifdef AQUEDUCT_FIX
			pCity->GetCityBuildings()->SetNumRealBuilding(eBuilding, 0);
#endif
			pCity->GetCityBuildings()->SetNumFreeBuilding(eBuilding, 1);
		}
#ifdef OWED_FOOD_BUILDING
		else
		{
			pCity->SetOwedFoodBuilding(true);
		}
#endif

		ChangeNumCitiesFreeFoodBuilding(-1);
	}

#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
	int iNumCitiesFreeDefensiveBuilding = GetNumCitiesFreeDefensiveBuilding();
	if (iNumCitiesFreeDefensiveBuilding > 0)
	{
		BuildingTypes eDefensiveBuilding = NO_BUILDING;
		if (GetPlayerTraits()->GetGreatScientistRateModifier() > 0)
		{
			eDefensiveBuilding = (BuildingTypes)GC.getInfoTypeForString("BUILDING_WALLS_OF_BABYLON");
		}
		else
		{
			eDefensiveBuilding = (BuildingTypes)GC.getInfoTypeForString("BUILDING_WALLS");
		}
		if (eDefensiveBuilding != NO_BUILDING)
		{
			pCity->GetCityBuildings()->SetNumRealBuilding(eDefensiveBuilding, 0);
			pCity->GetCityBuildings()->SetNumFreeBuilding(eDefensiveBuilding, 1);
		}

		ChangeNumCitiesFreeDefensiveBuilding(-1);
	}
#endif
}

//	--------------------------------------------------------------------------------
bool CvPlayer::canFound(int iX, int iY, bool bTestVisible) const
{
	CvPlot* pPlot;

	pPlot = GC.getMap().plot(iX, iY);

	// Has the AI agreed to not settle here?
	if(!isMinorCiv() && !isBarbarian())
	{
		if(pPlot->IsNoSettling(GetID()))
			return false;
	}
	// Haxor for Venice to prevent secondary founding
	if (GetPlayerTraits()->IsNoAnnexing() && getCapitalCity())
	{
		return false;
	}

	// Settlers cannot found cities while empire is very unhappy
	if(!bTestVisible)
	{
		if(IsEmpireVeryUnhappy())
			return false;
	}

	return GC.getGame().GetSettlerSiteEvaluator()->CanFound(pPlot, this, bTestVisible);
}


//	--------------------------------------------------------------------------------
void CvPlayer::found(int iX, int iY)
{
	if(!canFound(iX, iY))
	{
		return;
	}

	SetTurnsSinceSettledLastCity(0);

	CvCity* pCity = initCity(iX, iY);
	CvAssertMsg(pCity != NULL, "City is not assigned a valid value");
	if(pCity == NULL)
		return;

	int iExtraTerritoryClaim = GetPlayerTraits()->GetExtraFoundedCityTerritoryClaimRange();
	for (int i = 0; i < iExtraTerritoryClaim; i++)
	{
		CvPlot* pPlotToAcquire = pCity->GetNextBuyablePlot();

		// maybe the player owns ALL of the plots or there are none available?
		if(pPlotToAcquire)
		{
			pCity->DoAcquirePlot(pPlotToAcquire->getX(), pPlotToAcquire->getY());
		}
	}

	for(int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		const BuildingClassTypes eBuildingClass = static_cast<BuildingClassTypes>(iI);
		CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
		if(pkBuildingClassInfo)
		{
			const BuildingTypes eLoopBuilding = ((BuildingTypes)(getCivilizationInfo().getCivilizationBuildings(iI)));
			if(eLoopBuilding != NO_BUILDING)
			{
				CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eLoopBuilding);
				if(pkBuildingInfo)
				{
					if(pkBuildingInfo->GetFreeStartEra() != NO_ERA)
					{
						if(GC.getGame().getStartEra() >= pkBuildingInfo->GetFreeStartEra())
						{
							if(pCity->canConstruct(eLoopBuilding))
							{
								pCity->GetCityBuildings()->SetNumRealBuilding(eLoopBuilding, 1);
							}
						}
					}
				}
			}
		}
	}

	AwardFreeBuildings(pCity);

#ifdef DENMARK_UA_REWORK
	if (pCity->isCoastal())
	{
		TraitTypes eTrait = (TraitTypes)GC.getInfoTypeForString("TRAIT_VIKING_FURY", true /*bHideAssert*/);
		const UnitTypes eUnit = (UnitTypes)GC.getInfoTypeForString("UNIT_GALLEY", true  /*bHideAssert*/);

		if (GET_PLAYER(GetID()).GetPlayerTraits()->HasTrait(eTrait))
		{
			CvUnit* pNewUnit = NULL;
			pNewUnit = initUnit(eUnit, iX, iY);

			CvAssert(pNewUnit);

			if (pNewUnit)
			{
				pNewUnit->jumpToNearestValidPlot();
			}
		}
	}
#endif
#ifdef TRAIT_FREE_UNIT_IN_CAPITAL_FOUNDATION
	if (pCity->isCapital())
	{
		UnitClassTypes eCapitalUnitClass = (UnitClassTypes)GET_PLAYER(GetID()).GetPlayerTraits()->GetFreeUnitOnCapitalFoundation();
		CvUnitClassInfo* pkCapitalUnitClassInfo = GC.getUnitClassInfo(eCapitalUnitClass);
		if (pkCapitalUnitClassInfo)
		{
			CvUnit* pNewUnit = NULL;
			pNewUnit = initUnit((UnitTypes)pkCapitalUnitClassInfo->getDefaultUnitIndex(), iX, iY);

			CvAssert(pNewUnit);
			
			if (pNewUnit)
			{
				pNewUnit->jumpToNearestValidPlot();
				// pNewUnit->finishMoves();
			}
		}
	}
#endif
#ifdef CREATE_APOLLO_PROGRAM_WITH_CAP_FOUND
	ProjectTypes eApolloProgram = (ProjectTypes)GC.getInfoTypeForString("PROJECT_APOLLO_PROGRAM", true);
	if (isHuman() && eApolloProgram != NO_PROJECT)
	{
		if (GET_TEAM(getTeam()).getProjectCount(eApolloProgram) < 1)
		{
			pCity->CreateProject(eApolloProgram);
		}
	}
#endif

	DoUpdateNextPolicyCost();

	if(isHuman() && getAdvancedStartPoints() < 0)
	{
		// Human player is prompted to choose production BEFORE the AI runs for the turn.
		// So we'll force the AI strategies on the city now, just after it is founded.
		// And if the very first turn, we haven't even run player strategies once yet, so do that too.
		if(GC.getGame().getGameTurn() == 0)
		{
			this->GetEconomicAI()->DoTurn();
			this->GetMilitaryAI()->DoTurn();
			this->GetReligionAI()->DoTurn();
			this->GetEspionageAI()->DoTurn();
			this->GetTradeAI()->DoTurn();
		}
		pCity->GetCityStrategyAI()->DoTurn();
		pCity->chooseProduction();
		pCity->doFoundMessage();

		// If this is the first city (or we still aren't getting tech for some other reason notify the player)
		if(GetPlayerTechs()->GetCurrentResearch() == NO_TECH && GetScienceTimes100() > 0)
		{
			if(GetID() == GC.getGame().getActivePlayer())
			{
				chooseTech();
			}
		}
	}
	else
	{
		pCity->doFoundMessage();

		// AI civ, may need to redo city specializations
		GetCitySpecializationAI()->SetSpecializationsDirty(SPECIALIZATION_UPDATE_CITY_FOUNDED);
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());
		args->Push(pCity->getX());
		args->Push(pCity->getY());

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "PlayerCityFounded", args.get(), bResult);
	}
}


//	--------------------------------------------------------------------------------
bool CvPlayer::canTrain(UnitTypes eUnit, bool bContinue, bool bTestVisible, bool bIgnoreCost, bool bIgnoreUniqueUnitStatus, CvString* toolTipSink) const
{
	CvUnitEntry* pUnitInfoPtr = GC.getUnitInfo(eUnit);
	if(pUnitInfoPtr == NULL)
		return false;

	CvUnitEntry& pUnitInfo = *pUnitInfoPtr;

	const UnitClassTypes eUnitClass = (UnitClassTypes) pUnitInfo.GetUnitClassType();
	if(eUnitClass == NO_UNITCLASS)
	{
		return false;
	}

	CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eUnitClass);
	if(pkUnitClassInfo == NULL)
	{
		return false;
	}

	if (GetPlayerTraits()->NoTrain(eUnitClass))
	{
		return false;
	}

#ifdef CS_CANT_BUILD_EARLY_WORKERS
	if (isMinorCiv() && GC.getGame().getGameTurn() < CS_EARLY_WORKERS_TURN && eUnit == (UnitTypes)GC.getInfoTypeForString("UNIT_WORKER", true /*bHideAssert*/))
	{
		return false;
	}
#endif

	// Should we check whether this Unit has been blocked out by the civ XML?
	if(!bIgnoreUniqueUnitStatus)
	{
		UnitTypes eThisPlayersUnitType = (UnitTypes) getCivilizationInfo().getCivilizationUnits(eUnitClass);

		// If the player isn't allowed to train this Unit (via XML) then return false
		if(eThisPlayersUnitType != eUnit)
		{
			return false;
		}
	}

	if(!bIgnoreCost)
	{
		if(pUnitInfo.GetProductionCost() == -1)
		{
			return false;
		}
	}

	// One City Challenge
	if(pUnitInfo.IsFound() || pUnitInfo.IsFoundAbroad())
	{
#ifdef NQM_AI_GIMP_NO_BUILDING_SETTLERS
		if ((isHuman() && GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE)) || (!isHuman() && GC.getGame().isOption("GAMEOPTION_AI_TWEAKS")))
#else
		if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
#endif
		{
			return false;
		}
	}
	
	//Policy Requirement
	PolicyTypes ePolicy = (PolicyTypes)pUnitInfo.GetPolicyType();
	if (ePolicy != NO_POLICY)
	{
		if (!GetPlayerPolicies()->HasPolicy(ePolicy))
		{
			return false;
		}
	}


	if (GC.getGame().isOption(GAMEOPTION_NO_RELIGION))
	{
		if (pUnitInfo.IsFoundReligion() || pUnitInfo.IsSpreadReligion() || pUnitInfo.IsRemoveHeresy())
		{
			return false;
		}
	}

	if(!bContinue)
	{
		if(!bTestVisible)
		{
			// Builder Limit
			if(pUnitInfo.GetWorkRate() > 0 && pUnitInfo.GetDomainType() == DOMAIN_LAND)
			{
				if(GetMaxNumBuilders() > -1 && GetNumBuilders() >= GetMaxNumBuilders())
				{
					return false;
				}
			}
		}
	}

	// Tech requirements
	if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)(pUnitInfo.GetPrereqAndTech()))))
	{
		return false;
	}

	int iI;
	for(iI = 0; iI < GC.getNUM_UNIT_AND_TECH_PREREQS(); iI++)
	{
		if(pUnitInfo.GetPrereqAndTechs(iI) != NO_TECH)
		{
			if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)(pUnitInfo.GetPrereqAndTechs(iI)))))
			{
				return false;
			}
		}
	}

	// Obsolete Tech
	if((TechTypes)pUnitInfo.GetObsoleteTech() != NO_TECH)
	{
		if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)(pUnitInfo.GetObsoleteTech())))
		{
			return false;
		}
	}

	// Game Unit Class Max
	if(GC.getGame().isUnitClassMaxedOut(eUnitClass))
	{
		return false;
	}

	// Team Unit Class Max
	if(GET_TEAM(getTeam()).isUnitClassMaxedOut(eUnitClass))
	{
		return false;
	}

	// Player Unit Class Max
	if(isUnitClassMaxedOut(eUnitClass))
	{
		return false;
	}

	// Spaceship part we already have?
	ProjectTypes eProject = (ProjectTypes) pUnitInfo.GetSpaceshipProject();
	if(eProject != NO_PROJECT)
	{
		if(GET_TEAM(getTeam()).isProjectMaxedOut(eProject))
			return false;

		int iUnitAndProjectCount = GET_TEAM(getTeam()).getProjectCount(eProject) + getUnitClassCount(eUnitClass) + GET_TEAM(getTeam()).getUnitClassMaking(eUnitClass) + ((bContinue) ? -1 : 0);
		if(iUnitAndProjectCount >= pkUnitClassInfo->getMaxPlayerInstances())
		{
			return false;
		}
	}

	if(!bTestVisible)
	{
		// Settlers
		if(pUnitInfo.IsFound() || pUnitInfo.IsFoundAbroad())
		{
			if(IsEmpireVeryUnhappy() && GC.getVERY_UNHAPPY_CANT_TRAIN_SETTLERS() == 1)
			{
				GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_VERY_UNHAPPY_SETTLERS");
				if(toolTipSink == NULL)
					return false;
			}
		}

		// Project required?
		ProjectTypes ePrereqProject = (ProjectTypes) pUnitInfo.GetProjectPrereq();
		if(ePrereqProject != NO_PROJECT)
		{
			CvProjectEntry* pkProjectInfo = GC.getProjectInfo(ePrereqProject);
			if(pkProjectInfo)
			{
				if(GET_TEAM(getTeam()).getProjectCount(ePrereqProject) == 0)
				{
					GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_UNIT_PROJECT_REQUIRED", pkProjectInfo->GetDescription());
					if(toolTipSink == NULL)
						return false;
				}
			}
		}

		// Resource Requirements
		for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
		{
			const ResourceTypes eResource = static_cast<ResourceTypes>(iResourceLoop);
			CvResourceInfo* pkResourceInfo = GC.getResourceInfo(eResource);
			if(pkResourceInfo)
			{
				const int iNumResource = pUnitInfo.GetResourceQuantityRequirement(eResource);

				if(iNumResource > 0)
				{
					// Starting project, need enough Resources plus some to start
					if(!bContinue && getNumResourceAvailable(eResource) < iNumResource)
					{
						GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_UNIT_LACKS_RESOURCES", pkResourceInfo->GetIconString(), pkResourceInfo->GetTextKey(), iNumResource);
						if(toolTipSink == NULL)
							return false;
					}
					// Continuing project, need enough Resources
					else if(bContinue && (getNumResourceAvailable(eResource) < 0))
					{
						GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_UNIT_LACKS_RESOURCES", pkResourceInfo->GetIconString(), pkResourceInfo->GetTextKey(), iNumResource);
						if(toolTipSink == NULL)
							return false;
					}
				}
			}

		}

		if(GC.getGame().isUnitClassMaxedOut(eUnitClass, (GET_TEAM(getTeam()).getUnitClassMaking(eUnitClass) + ((bContinue) ? -1 : 0))))
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_GAME_COUNT_MAX", "", "", pkUnitClassInfo->getMaxTeamInstances());
			if(toolTipSink == NULL)
				return false;
		}

		if(GET_TEAM(getTeam()).isUnitClassMaxedOut(eUnitClass, (GET_TEAM(getTeam()).getUnitClassMaking(eUnitClass) + ((bContinue) ? -1 : 0))))
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_TEAM_COUNT_MAX", "", "", pkUnitClassInfo->getMaxTeamInstances());
			if(toolTipSink == NULL)
				return false;
		}

		if(isUnitClassMaxedOut(eUnitClass, (getUnitClassMaking(eUnitClass) + ((bContinue) ? -1 : 0))))
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_PLAYER_COUNT_MAX", "", "", pkUnitClassInfo->getMaxPlayerInstances());
			if(toolTipSink == NULL)
				return false;
		}

		if(GC.getGame().isNoNukes() || !GC.getGame().isNukesValid())
		{
			if(pUnitInfo.GetNukeDamageLevel() != -1)
			{
				GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_NUKES");
				if(toolTipSink == NULL)
					return false;
			}
		}

		if(pUnitInfo.GetNukeDamageLevel() != -1)
		{
#ifndef NUCLEAR_NON_PROLIFERATION_INCREASE_NUKES_COST
			if(GC.getGame().GetGameLeagues()->IsNoTrainingNuclearWeapons(GetID()))
			{
				GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_NUKES_BY_RESOLUTION");
				if(toolTipSink == NULL)
					return false;
			}
#endif
		}

		if(pUnitInfo.GetSpecialUnitType() != NO_SPECIALUNIT)
		{
			if(!(GC.getGame().isSpecialUnitValid((SpecialUnitTypes)(pUnitInfo.GetSpecialUnitType()))))
			{
				GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_SPECIAL_UNIT");
				if(toolTipSink == NULL)
					return false;
			}
		}

		if (pUnitInfo.IsTrade())
		{
			if (GetTrade()->GetNumTradeRoutesRemaining(bContinue) <= 0)
			{
				GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_TRADE_UNIT_CONSTRUCTION_NO_EXTRA_SLOTS");
				if (toolTipSink == NULL)
					return false;			
			}

			DomainTypes eDomain = (DomainTypes)pUnitInfo.GetDomainType();
			if (!GetTrade()->CanCreateTradeRoute(eDomain))
			{
				if (eDomain == DOMAIN_LAND)
				{
					GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_TRADE_UNIT_CONSTRUCTION_NONE_OF_TYPE_LAND");
				}
				else if (eDomain == DOMAIN_SEA)
				{
					GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_TRADE_UNIT_CONSTRUCTION_NONE_OF_TYPE_SEA");
				}
				if (toolTipSink == NULL)
					return false;
			}
		}
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());
		args->Push(eUnit);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "PlayerCanTrain", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}


//	--------------------------------------------------------------------------------
#ifdef NEW_BELIEF_PROPHECY
bool CvPlayer::canConstruct(BuildingTypes eBuilding, bool bContinue, bool bTestVisible, bool bIgnoreCost, CvString* toolTipSink, const CvCity* pCity) const
#else
bool CvPlayer::canConstruct(BuildingTypes eBuilding, bool bContinue, bool bTestVisible, bool bIgnoreCost, CvString* toolTipSink) const
#endif
{
	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
		return false;

#ifdef CAN_BUILD_OU_AND_NIA_ONLY_ONCE
	if(eBuilding == (BuildingTypes)GC.getInfoTypeForString("BUILDING_OXFORD_UNIVERSITY"))
		if(isOxfordUniversityWasEverBuilt())
			return false;
	if(eBuilding == (BuildingTypes)GC.getInfoTypeForString("BUILDING_INTELLIGENCE_AGENCY"))
		if(isNationalIntelligenceAgencyWasEverBuilt())
			return false;
#endif
#ifdef DUEL_TOGGLE_OXFORD_UNIVERSITY
	if(eBuilding == (BuildingTypes)GC.getInfoTypeForString("BUILDING_OXFORD_UNIVERSITY"))
		if(GC.getGame().isOption("GAMEOPTION_DUEL_STUFF"))
			if(GC.getGame().isOption("GAMEOPTION_DISABLE_OXFORD_UNIVERSITY"))
				return false;
#endif
#ifdef DUEL_DISABLE_GREAT_WALL
	if(eBuilding == (BuildingTypes)GC.getInfoTypeForString("BUILDING_GREAT_WALL"))
		if(GC.getGame().isOption("GAMEOPTION_DUEL_STUFF"))
			return false;
#endif

	// Don't allow a city to consider an espionage building if they are playing a non-espionage game
#ifdef DUEL_MOVING_SOME_OPTIONS_TO_DUEL_MODE
	if (GC.getGame().isNetworkMultiPlayer() && GC.getGame().isOption("GAMEOPTION_DUEL_STUFF") && GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE) && pkBuildingInfo->IsEspionage() || !GC.getGame().isNetworkMultiPlayer() && GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE) && pkBuildingInfo->IsEspionage())
#else
	if(GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE) && pkBuildingInfo->IsEspionage())
#endif
	{
		return false;
	}

	CvBuildingEntry& pBuildingInfo = *pkBuildingInfo;

#ifdef DUEL_BAN_WORLD_WONDERS
	if(GC.getGame().isOption("GAMEOPTION_DUEL_STUFF"))
	{
		if(GC.getGame().isOption("GAMEOPTION_BAN_WORLD_WONDERS"))
		{
			int i1 = -1;
			CvPreGame::GetGameOption("GAMEOPTION_BAN_WONDER1", i1);
			int i2 = -1;
			CvPreGame::GetGameOption("GAMEOPTION_BAN_WONDER2", i2);
			int i3 = -1;
			CvPreGame::GetGameOption("GAMEOPTION_BAN_WONDER3", i3);
			if(pkBuildingInfo->GetID() == i1 || pkBuildingInfo->GetID() == i2 || pkBuildingInfo->GetID() == i3)
			{
				return false;
			}
		}
	}
#endif

	int iI;
	CvTeam& currentTeam = GET_TEAM(getTeam());

	const BuildingClassTypes eBuildingClass = ((BuildingClassTypes)(pBuildingInfo.GetBuildingClassType()));
	const CvBuildingClassInfo& kBuildingClass = pkBuildingInfo->GetBuildingClassInfo();

	// Checks to make sure civilization doesn't have an override that prevents construction of this building
	if(getCivilizationInfo().getCivilizationBuildings(eBuildingClass) != eBuilding)
	{
		return false;
	}

	if(!bIgnoreCost)
	{
		if(pBuildingInfo.GetProductionCost() == -1)
		{
			return false;
		}
	}

	PolicyBranchTypes eBranch = (PolicyBranchTypes)pBuildingInfo.GetPolicyBranchType();
	if (eBranch != NO_POLICY_BRANCH_TYPE)
	{
#ifdef NEW_BELIEF_PROPHECY
		bool bReligionAllowsPolicyWonders = false;
		if (pCity)
		{
			ReligionTypes eReligionFounded = GetReligions()->GetReligionCreatedByPlayer();
			ReligionTypes eCityReligion = pCity->GetCityReligions()->GetReligiousMajority();
			if (eReligionFounded > RELIGION_PANTHEON && eReligionFounded == eCityReligion)
			{
				const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligionFounded, GetID());
				if (pReligion && pReligion->m_Beliefs.IsAllowPolicyWonders())
				{
					bReligionAllowsPolicyWonders = true;
				}
			}
		}
		if (!GetPlayerPolicies()->IsPolicyBranchUnlocked(eBranch) && !bReligionAllowsPolicyWonders)
		{
			return false;
		}
#else
		if (!GetPlayerPolicies()->IsPolicyBranchUnlocked(eBranch))
		{
			return false;
		}
#endif
	}

	if(!(currentTeam.GetTeamTechs()->HasTech((TechTypes)(pBuildingInfo.GetPrereqAndTech()))))
	{
		return false;
	}

	for(iI = 0; iI < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iI++)
	{
		if(pBuildingInfo.GetPrereqAndTechs(iI) != NO_TECH)
		{
			if(!(currentTeam.GetTeamTechs()->HasTech((TechTypes)(pBuildingInfo.GetPrereqAndTechs(iI)))))
			{
				return false;
			}
		}
	}

	if(currentTeam.isObsoleteBuilding(eBuilding))
	{
		return false;
	}

	// Building upgrade to another type
	BuildingClassTypes eReplacementBuildingClass = (BuildingClassTypes) pBuildingInfo.GetReplacementBuildingClass();

	if(eReplacementBuildingClass != NO_BUILDINGCLASS)
	{
		BuildingTypes eUpgradeBuilding = ((BuildingTypes)(getCivilizationInfo().getCivilizationBuildings(eReplacementBuildingClass)));

		if(canConstruct(eUpgradeBuilding))
		{
			return false;
		}
	}

	if(pBuildingInfo.GetVictoryPrereq() != NO_VICTORY)
	{
		if(!(GC.getGame().isVictoryValid((VictoryTypes)(pBuildingInfo.GetVictoryPrereq()))))
		{
			return false;
		}

		if(isMinorCiv())
		{
			return false;
		}

		if(currentTeam.getVictoryCountdown((VictoryTypes)pBuildingInfo.GetVictoryPrereq()) >= 0)
		{
			return false;
		}
	}

	if(pBuildingInfo.GetMaxStartEra() != NO_ERA)
	{
		if(GC.getGame().getStartEra() > pBuildingInfo.GetMaxStartEra())
		{
			return false;
		}
	}

#ifdef NQM_AI_GIMP_NO_WORLD_WONDERS
	CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
	if(GC.getGame().isBuildingClassMaxedOut(eBuildingClass) ||
		isWorldWonderClass(*pkBuildingClassInfo) && GC.getGame().isOption("GAMEOPTION_AI_TWEAKS") && !isHuman())
	{
		return false;
	}
#endif

	if(currentTeam.isBuildingClassMaxedOut(eBuildingClass))
	{
		return false;
	}

	if(isBuildingClassMaxedOut(eBuildingClass))
	{
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Everything above this is what is checked to see if Building shows up in the list of construction items
	///////////////////////////////////////////////////////////////////////////////////

	if(!bTestVisible)
	{
		// Num buildings in the empire... uhhh, how is this different from the very last check in this function? (JON: It doesn't appear to be used, but I can't say for sure :)
		CvCivilizationInfo& civilizationInfo = getCivilizationInfo();
		int numBuildingClassInfos = GC.getNumBuildingClassInfos();

		for(iI = 0; iI < numBuildingClassInfos; iI++)
		{
			CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo((BuildingClassTypes)iI);
			if(!pkBuildingClassInfo)
			{
				continue;
			}

			BuildingTypes ePrereqBuilding = (BuildingTypes)civilizationInfo.getCivilizationBuildings(iI);

			if(NO_BUILDING != ePrereqBuilding && currentTeam.isObsoleteBuilding(ePrereqBuilding))
			{
				CvBuildingEntry* pkPrereqBuilding = GC.getBuildingInfo(ePrereqBuilding);
				if(pkPrereqBuilding)
				{
					int iNumHave = getBuildingClassCount((BuildingClassTypes)iI);

					int iNumNeeded = getBuildingClassPrereqBuilding(eBuilding, (BuildingClassTypes)iI, 0);

					if(iNumHave < iNumNeeded)
					{
						GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_COUNT_NEEDED", pkPrereqBuilding->GetTextKey(), "", iNumNeeded - iNumHave);

						if(toolTipSink == NULL)
							return false;
					}
				}
			}
		}

		// Resource Requirements
		for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
		{
			const ResourceTypes eResource = static_cast<ResourceTypes>(iResourceLoop);
			CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
			if(pkResource)
			{
				int iNumResource = pBuildingInfo.GetResourceQuantityRequirement(eResource);
				if(iNumResource > 0)
				{
					if(bContinue)
						iNumResource = 0;

					if(getNumResourceAvailable(eResource) < iNumResource)
					{
						GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_LACKS_RESOURCES", pkResource->GetIconString(), pkResource->GetTextKey(), iNumResource);
						if(toolTipSink == NULL)
							return false;
					}
				}
			}
		}

		if(GC.getGame().isBuildingClassMaxedOut(eBuildingClass, (currentTeam.getBuildingClassMaking(eBuildingClass) + ((bContinue) ? -1 : 0))))
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_GAME_COUNT_MAX", "", "", kBuildingClass.getMaxGlobalInstances());
			if(toolTipSink == NULL)
				return false;
		}

		if(currentTeam.isBuildingClassMaxedOut(eBuildingClass, (currentTeam.getBuildingClassMaking(eBuildingClass) + ((bContinue) ? -1 : 0))))
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_TEAM_COUNT_MAX", "", "", kBuildingClass.getMaxTeamInstances());
			if(toolTipSink == NULL)
				return false;
		}

		if(isBuildingClassMaxedOut(eBuildingClass, (getBuildingClassMaking(eBuildingClass) + ((bContinue) ? -1 : 0))))
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_PLAYER_COUNT_MAX", "", "", kBuildingClass.getMaxPlayerInstances());
			if(toolTipSink == NULL)
				return false;
		}

		if(getNumCities() < pBuildingInfo.GetNumCitiesPrereq())
		{
			return false;
		}

		if(getHighestUnitLevel() < pBuildingInfo.GetUnitLevelPrereq())
		{
			return false;
		}

		// How does this differ from the check above?
		BuildingTypes ePrereqBuilding;
		int iNumNeeded;
		for(iI = 0; iI < numBuildingClassInfos; iI++)
		{
			iNumNeeded = getBuildingClassPrereqBuilding(eBuilding, ((BuildingClassTypes)iI), bContinue);
			//int iNumHave = getBuildingClassCount((BuildingClassTypes)iI);
			ePrereqBuilding = (BuildingTypes) civilizationInfo.getCivilizationBuildings(iI);
			if(NO_BUILDING != ePrereqBuilding)
			{
				CvBuildingEntry* pkPrereqBuilding = GC.getBuildingInfo(ePrereqBuilding);
				if(pkPrereqBuilding)
				{
					int iNumHave = 0;
					const CvCity* pLoopCity = NULL;
					int iLoop;
					for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						if(pLoopCity && !pLoopCity->IsPuppet() && pLoopCity->GetCityBuildings()->GetNumBuilding(ePrereqBuilding) > 0)
						{
							iNumHave++;
						}
					}

					if(iNumHave < iNumNeeded)
					{
						ePrereqBuilding = (BuildingTypes) civilizationInfo.getCivilizationBuildings(iI);

						GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_COUNT_NEEDED", pkPrereqBuilding->GetTextKey(), "", iNumNeeded - iNumHave);

						if(toolTipSink == NULL)
							return false;

						// If we have less than 5 to go, list what cities need them
						int iNonPuppetCities = getNumCities() - GetNumPuppetCities();
						if(iNumNeeded == iNonPuppetCities && iNumNeeded - iNumHave < 5)
						{
							(*toolTipSink) += "[NEWLINE]";

							for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
							{
								if(pLoopCity && !pLoopCity->IsPuppet() && pLoopCity->GetCityBuildings()->GetNumBuilding(ePrereqBuilding) == 0)
								{
									(*toolTipSink) += pLoopCity->getName();
									(*toolTipSink) += " ";
								}
							}
						}
					}
				}
			}
		}
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());
		args->Push(eBuilding);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "PlayerCanConstruct", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::canCreate(ProjectTypes eProject, bool bContinue, bool bTestVisible) const
{
	CvProjectEntry* pkProjectInfo = GC.getProjectInfo(eProject);
	if(!pkProjectInfo)
	{
		return false;
	}

	CvProjectEntry& pProjectInfo = *pkProjectInfo;

	int iI;

	// No projects for barbs
	if(isBarbarian())
	{
		return false;
	}

	// no minors either
	if(isMinorCiv())
	{
		return false;
	}

	// If cost is -1 then that means it can't be built
	if(pProjectInfo.GetProductionCost() == -1)
	{
		return false;
	}

	// Tech requirement
	if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)(pProjectInfo.GetTechPrereq()))))
	{
		return false;
	}

	// Policy branch requirement?
	if(pProjectInfo.GetCultureBranchesRequired() > 0)
	{
		if(GetPlayerPolicies()->GetNumPolicyBranchesFinished() < pProjectInfo.GetCultureBranchesRequired())
		{
			return false;
		}
	}

	// Requires a particular victory condition to be enabled?
	if(pProjectInfo.GetVictoryPrereq() != NO_VICTORY)
	{
		if(!(GC.getGame().isVictoryValid((VictoryTypes)(pProjectInfo.GetVictoryPrereq()))))
		{
			return false;
		}

		if(isMinorCiv())
		{
			return false;
		}

		if(GET_TEAM(getTeam()).getVictoryCountdown((VictoryTypes)pProjectInfo.GetVictoryPrereq()) >= 0)
		{
			return false;
		}
	}

	if(GC.getGame().isProjectMaxedOut(eProject))
	{
		return false;
	}

	if(GET_TEAM(getTeam()).isProjectMaxedOut(eProject))
	{
		return false;
	}

	if(!bTestVisible)
	{
		// Resource Requirements
		ResourceTypes eResource;
		int iNumResource;
		for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
		{
			eResource = (ResourceTypes) iResourceLoop;
			iNumResource = pProjectInfo.GetResourceQuantityRequirement(eResource);

			if(iNumResource > 0)
			{
				if(getNumResourceAvailable(eResource) < iNumResource)
				{
					return false;
				}
			}
		}

		if(GC.getGame().isProjectMaxedOut(eProject, (GET_TEAM(getTeam()).getProjectMaking(eProject) + ((bContinue) ? -1 : 0))))
		{
			return false;
		}

		if(GET_TEAM(getTeam()).isProjectMaxedOut(eProject, (GET_TEAM(getTeam()).getProjectMaking(eProject) + ((bContinue) ? -1 : 0))))
		{
			return false;
		}

		// Nukes disabled? (by UN or something)
		if(GC.getGame().isNoNukes())
		{
			if(pProjectInfo.IsAllowsNukes())
			{
				for(iI = 0; iI < GC.getNumUnitInfos(); iI++)
				{
					CvUnitEntry* pkUnitEntry = GC.getUnitInfo((UnitTypes)iI);
					if(pkUnitEntry && pkUnitEntry->GetNukeDamageLevel() != -1)
					{
						return false;
					}
				}
			}
		}

		if(pProjectInfo.GetAnyoneProjectPrereq() != NO_PROJECT)
		{
			if(GC.getGame().getProjectCreatedCount((ProjectTypes)(pProjectInfo.GetAnyoneProjectPrereq())) == 0)
			{
				return false;
			}
		}

		for(iI = 0; iI < GC.getNumProjectInfos(); iI++)
		{
			if(GET_TEAM(getTeam()).getProjectCount((ProjectTypes)iI) < pProjectInfo.GetProjectsNeeded(iI))
			{
				return false;
			}
		}
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());
		args->Push(eProject);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "PlayerCanCreate", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::canPrepare(SpecialistTypes eSpecialist, bool) const
{
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());
		args->Push(eSpecialist);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "PlayerCanPrepare", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}


	return false;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::canMaintain(ProcessTypes eProcess, bool) const
{
	// Check to see if it exists, scenarios can remove them and leave holes in the list.
	const CvProcessInfo* pkProcessInfo = GC.getProcessInfo(eProcess);
	if (!pkProcessInfo)
		return false;

	if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)(pkProcessInfo->getTechPrereq()))))
	{
		return false;
	}

	for(int iI = 0; iI < GC.getNumLeagueProjectInfos(); iI++)
	{
		LeagueProjectTypes eLeagueProject = (LeagueProjectTypes) iI;
		CvLeagueProjectEntry* pInfo = GC.getLeagueProjectInfo(eLeagueProject);
		if (pInfo && pInfo->GetProcess() == eProcess)
		{
			if (!GC.getGame().GetGameLeagues()->CanContributeToLeagueProject(GetID(), eLeagueProject))
			{
				return false;
			}
		}
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());
		args->Push(eProcess);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "PlayerCanMaintain", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
/// Can we purchase this unit or building in any of our cities?
bool CvPlayer::IsCanPurchaseAnyCity(bool bTestPurchaseCost, bool bTestTrainable, UnitTypes eUnit, BuildingTypes eBuilding, YieldTypes ePurchaseYield)
{
	int iLoop;
	CvCity *pLoopCity;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->IsCanPurchase(bTestPurchaseCost, bTestTrainable, eUnit, eBuilding, NO_PROJECT, ePurchaseYield))
		{
			return true;
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isProductionMaxedUnitClass(UnitClassTypes eUnitClass) const
{
	if(eUnitClass == NO_UNITCLASS)
	{
		return false;
	}

	if(GC.getGame().isUnitClassMaxedOut(eUnitClass))
	{
		return true;
	}

	if(GET_TEAM(getTeam()).isUnitClassMaxedOut(eUnitClass))
	{
		return true;
	}

	if(isUnitClassMaxedOut(eUnitClass))
	{
		return true;
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isProductionMaxedBuildingClass(BuildingClassTypes eBuildingClass, bool bAcquireCity) const
{
	if(eBuildingClass == NO_BUILDINGCLASS)
	{
		return false;
	}

	CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
	if(pkBuildingClassInfo == NULL)
	{
		return false;
	}

	if(!bAcquireCity)
	{
		if(GC.getGame().isBuildingClassMaxedOut(eBuildingClass))
		{
			return true;
		}
	}

	if(GET_TEAM(getTeam()).isBuildingClassMaxedOut(eBuildingClass))
	{
		return true;
	}

	if(isBuildingClassMaxedOut(eBuildingClass, ((bAcquireCity) ? pkBuildingClassInfo->getExtraPlayerInstances() : 0)))
	{
		return true;
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isProductionMaxedProject(ProjectTypes eProject) const
{
	if(eProject == NO_PROJECT)
	{
		return false;
	}

	if(GC.getGame().isProjectMaxedOut(eProject))
	{
		return true;
	}

	if(GET_TEAM(getTeam()).isProjectMaxedOut(eProject))
	{
		return true;
	}

	return false;
}


//	--------------------------------------------------------------------------------
int CvPlayer::getProductionNeeded(UnitTypes eUnit) const
{
	CvUnitEntry* pkUnitEntry = GC.getUnitInfo(eUnit);

	CvAssertMsg(pkUnitEntry, "This should never be hit");
	if(pkUnitEntry == NULL)
		return 0;

	UnitClassTypes eUnitClass = (UnitClassTypes)pkUnitEntry->GetUnitClassType();
	CvAssert(NO_UNITCLASS != eUnitClass);

	CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eUnitClass);
	CvAssert(pkUnitClassInfo);
	if(pkUnitClassInfo == NULL)
		return 0;

	int iProductionNeeded = pkUnitEntry->GetProductionCost();
	iProductionNeeded *= 100 + getUnitClassCount(eUnitClass) * pkUnitClassInfo->getInstanceCostModifier();
	iProductionNeeded /= 100;

	if(isMinorCiv())
	{
		iProductionNeeded *= GC.getMINOR_CIV_PRODUCTION_PERCENT();
		iProductionNeeded /= 100;
	}

	iProductionNeeded *= GC.getUNIT_PRODUCTION_PERCENT();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGame().getGameSpeedInfo().getTrainPercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGame().getStartEraInfo().getTrainPercent();
	iProductionNeeded /= 100;

	if(!isHuman() && !IsAITeammateOfHuman() && !isBarbarian())
	{
		if(isWorldUnitClass(eUnitClass))
		{
			iProductionNeeded *= GC.getGame().getHandicapInfo().getAIWorldTrainPercent();
			iProductionNeeded /= 100;
		}
		else
		{
			iProductionNeeded *= GC.getGame().getHandicapInfo().getAITrainPercent();
			iProductionNeeded /= 100;
		}

		iProductionNeeded *= std::max(0, ((GC.getGame().getHandicapInfo().getAIPerEraModifier() * GetCurrentEra()) + 100));
		iProductionNeeded /= 100;
	}

	iProductionNeeded += getUnitExtraCost(eUnitClass);
#ifdef NUCLEAR_NON_PROLIFERATION_INCREASE_NUKES_COST
	if (GC.getUnitInfo(eUnit)->GetNukeDamageLevel() != -1)
	{
		if (GC.getGame().GetGameLeagues()->IsNoTrainingNuclearWeapons(GetID()))
		{
			iProductionNeeded *= 3;
		}
	}
#endif

	return std::max(1, iProductionNeeded);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getProductionNeeded(BuildingTypes eBuilding) const
{
	int iProductionNeeded;

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
	{
		//This should never happen.
		return 1;
	}

	iProductionNeeded = pkBuildingInfo->GetProductionCost();

	if(pkBuildingInfo->GetNumCityCostMod() > 0 && getNumCities() > 0)
	{
		iProductionNeeded += (pkBuildingInfo->GetNumCityCostMod() * getNumCities());
	}

	if(isMinorCiv())
	{
		iProductionNeeded *= GC.getMINOR_CIV_PRODUCTION_PERCENT();
		iProductionNeeded /= 100;
	}

	iProductionNeeded *= GC.getBUILDING_PRODUCTION_PERCENT();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGame().getGameSpeedInfo().getConstructPercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGame().getStartEraInfo().getConstructPercent();
	iProductionNeeded /= 100;

	if(pkBuildingInfo->GetPrereqAndTech() != NO_TECH)
	{
		CvTechEntry* pkTechInfo = GC.getTechInfo((TechTypes)pkBuildingInfo->GetPrereqAndTech());
		if(pkTechInfo)
		{
			// Loop through all eras and apply Building production mod based on how much time has passed
			int iTotalEraMod = 100;
			EraTypes eBuildingUnlockedEra = (EraTypes) pkTechInfo->GetEra();

			if(eBuildingUnlockedEra < GetCurrentEra())
			{
				for(int iLoop = eBuildingUnlockedEra; iLoop < GetCurrentEra(); iLoop++)
				{
					CvAssertMsg(iLoop >= 0, "Loop should be within era bounds");
					CvAssertMsg(iLoop <GC.getNumEraInfos(), "Loop should be within era bounds");

					if(iLoop >= 0 && iLoop < GC.getNumEraInfos())
					{
						CvEraInfo* pkEraInfo = GC.getEraInfo((EraTypes)iLoop);
						if(pkEraInfo)
						{
							iTotalEraMod += pkEraInfo->getLaterEraBuildingConstructMod();
						}
					}
				}

				// Don't make a change if there's no change to make
				if(iTotalEraMod != 100)
				{
					iProductionNeeded *= iTotalEraMod;
					iProductionNeeded /= 100;
				}
			}
		}
	}

	if(!isHuman() && !IsAITeammateOfHuman() && !isBarbarian())
	{
		if(isWorldWonderClass(pkBuildingInfo->GetBuildingClassInfo()))
		{
			iProductionNeeded *= GC.getGame().getHandicapInfo().getAIWorldConstructPercent();
			iProductionNeeded /= 100;
		}
		else
		{
			iProductionNeeded *= GC.getGame().getHandicapInfo().getAIConstructPercent();
			iProductionNeeded /= 100;
		}

		iProductionNeeded *= std::max(0, ((GC.getGame().getHandicapInfo().getAIPerEraModifier() * GetCurrentEra()) + 100));
		iProductionNeeded /= 100;
	}

	return std::max(1, iProductionNeeded);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getProductionNeeded(ProjectTypes eProject) const
{
	int iProductionNeeded;

	iProductionNeeded = GC.getProjectInfo(eProject)->GetProductionCost();

	if(isMinorCiv())
	{
		iProductionNeeded *= GC.getMINOR_CIV_PRODUCTION_PERCENT();
		iProductionNeeded /= 100;
	}

	iProductionNeeded *= GC.getPROJECT_PRODUCTION_PERCENT();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGame().getGameSpeedInfo().getCreatePercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGame().getStartEraInfo().getCreatePercent();
	iProductionNeeded /= 100;

	if(!isHuman() && !IsAITeammateOfHuman() && !isBarbarian())
	{
		if(isWorldProject(eProject))
		{
			iProductionNeeded *= GC.getGame().getHandicapInfo().getAIWorldCreatePercent();
			iProductionNeeded /= 100;
		}
		else
		{
			iProductionNeeded *= GC.getGame().getHandicapInfo().getAICreatePercent();
			iProductionNeeded /= 100;
		}

		iProductionNeeded *= std::max(0, ((GC.getGame().getHandicapInfo().getAIPerEraModifier() * GetCurrentEra()) + 100));
		iProductionNeeded /= 100;
	}

	return std::max(1, iProductionNeeded);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getProductionNeeded(SpecialistTypes eSpecialist) const
{
	CvSpecialistInfo* pkSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
	if(pkSpecialistInfo == NULL)
	{
		//This should never happen! If this does, fix the calling function!
		CvAssert(pkSpecialistInfo);
		return 0;
	}

	int iProductionNeeded;
	iProductionNeeded = pkSpecialistInfo->getCost();

	if(isMinorCiv())
	{
		iProductionNeeded *= GC.getMINOR_CIV_PRODUCTION_PERCENT();
		iProductionNeeded /= 100;
	}

	iProductionNeeded *= GC.getGame().getGameSpeedInfo().getCreatePercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGame().getStartEraInfo().getCreatePercent();
	iProductionNeeded /= 100;

	return std::max(1, iProductionNeeded);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getProductionModifier(CvString* toolTipSink) const
{
	int iMultiplier = 0;

	int iTempMod;

	// Unit Supply
	iTempMod = GetUnitProductionMaintenanceMod();
	iMultiplier += iTempMod;
	GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_UNIT_SUPPLY", iTempMod);

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getProductionModifier(UnitTypes eUnit, CvString* toolTipSink) const
{
	int iMultiplier = getProductionModifier(toolTipSink);
	int iTempMod;

	CvUnitEntry* pUnitEntry = GC.getUnitInfo(eUnit);

	if(pUnitEntry)
	{
		// Military bonus
		if(pUnitEntry->IsMilitaryProduction())
		{
			iTempMod = getMilitaryProductionModifier();
			iMultiplier += iTempMod;
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_MILITARY_PLAYER", iTempMod);
		}

		// Settler bonus
		if(pUnitEntry->IsFound())
		{
			iTempMod = getSettlerProductionModifier();
			iMultiplier += iTempMod;
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_SETTLER_PLAYER", iTempMod);
		}

		// Unit Combat class bonus
		if(pUnitEntry->GetUnitCombatType() != NO_UNITCOMBAT)
		{
			iTempMod = getUnitCombatProductionModifiers((UnitCombatTypes) pUnitEntry->GetUnitCombatType());
			iMultiplier += iTempMod;
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_UNIT_COBMAT_CLASS_PLAYER", iTempMod);
		}

		// Trait bonus
		int iNumTraits = GC.getNumTraitInfos();
		CvPlayerTraits* pPlayerTraits = GetPlayerTraits();
		for(int iI = 0; iI < iNumTraits; iI++)
		{
			if(pPlayerTraits->HasTrait((TraitTypes)iI))
			{
				iMultiplier += pUnitEntry->GetProductionTraits(iI);

				if(pUnitEntry->GetSpecialUnitType() != NO_SPECIALUNIT)
				{
					CvSpecialUnitInfo* pkSpecialUnitInfo = GC.getSpecialUnitInfo((SpecialUnitTypes) pUnitEntry->GetSpecialUnitType());
					if(pkSpecialUnitInfo)
					{
						iTempMod = pkSpecialUnitInfo->getProductionTraits(iI);
						iMultiplier += iTempMod;
						GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_UNIT_TRAIT", iTempMod);
					}
				}
			}
		}
	}

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getProductionModifier(BuildingTypes eBuilding, CvString* toolTipSink) const
{
	int iMultiplier = getProductionModifier(toolTipSink);
	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
	{
		return iMultiplier;
	}

	CvGame& kGame = GC.getGame();
	const CvBuildingClassInfo& kBuildingClassInfo = pkBuildingInfo->GetBuildingClassInfo();

	int iTempMod;

	int iNumTraits = GC.getNumTraitInfos();
	CvPlayerTraits* pPlayerTraits = GetPlayerTraits();
	for(int iI = 0; iI < iNumTraits; iI++)
	{
		if(pPlayerTraits->HasTrait((TraitTypes)iI))
		{
			iTempMod = pkBuildingInfo->GetProductionTraits(iI);
			iMultiplier += iTempMod;
			kGame.BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_BUILDING_TRAIT", iTempMod);
		}
	}

	// World Wonder
	if(::isWorldWonderClass(kBuildingClassInfo))
	{
		iTempMod = getMaxGlobalBuildingProductionModifier();
		iMultiplier += iTempMod;
		kGame.BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WORLD_WONDER_PLAYER", iTempMod);
		iTempMod = m_pPlayerPolicies->GetNumericModifier(POLICYMOD_WONDER_PRODUCTION_MODIFIER);
		iMultiplier += iTempMod;
		kGame.BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WONDER_POLICY", iTempMod);
	}

	// Team Wonder
	else if(::isTeamWonderClass(kBuildingClassInfo))
	{
		iTempMod = getMaxTeamBuildingProductionModifier();
		iMultiplier += iTempMod;
		kGame.BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_TEAM_WONDER_PLAYER", iTempMod);
		iTempMod = m_pPlayerPolicies->GetNumericModifier(POLICYMOD_WONDER_PRODUCTION_MODIFIER);
		iMultiplier += iTempMod;
		kGame.BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WONDER_POLICY", iTempMod);
	}

	// National Wonder
	else if(::isNationalWonderClass(kBuildingClassInfo))
	{
		iTempMod = getMaxPlayerBuildingProductionModifier();
		iMultiplier += iTempMod;
		kGame.BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_NATIONAL_WONDER_PLAYER", iTempMod);
		iTempMod = m_pPlayerPolicies->GetNumericModifier(POLICYMOD_WONDER_PRODUCTION_MODIFIER);
		iMultiplier += iTempMod;
		kGame.BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WONDER_POLICY", iTempMod);
	}

	// Normal Building
	else
	{
		iTempMod = m_pPlayerPolicies->GetNumericModifier(POLICYMOD_BUILDING_PRODUCTION_MODIFIER);
		iMultiplier += iTempMod;
		kGame.BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_BUILDING_POLICY_PLAYER", iTempMod);
	}

	// Religion
	if(pkBuildingInfo->IsReligious())
	{
		iTempMod = m_pPlayerPolicies->GetNumericModifier(POLICYMOD_RELIGION_PRODUCTION_MODIFIER);
		iMultiplier += iTempMod;
		kGame.BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_RELIGION_PLAYER", iTempMod);
	}

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getProductionModifier(ProjectTypes eProject, CvString* toolTipSink) const
{
	int iMultiplier = getProductionModifier(toolTipSink);
	int iTempMod;

	if(GC.getProjectInfo(eProject)->IsSpaceship())
	{
		iTempMod = getSpaceProductionModifier();
		iMultiplier += iTempMod;
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_SPACE_PLAYER", iTempMod);
	}

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getProductionModifier(SpecialistTypes, CvString* toolTipSink) const
{
	int iMultiplier = getProductionModifier(toolTipSink);
	int iTempMod;

	iTempMod = getSpecialistProductionModifier();
	iMultiplier += iTempMod;
	GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_SPECIALIST_PLAYER", iTempMod);

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getProductionModifier(ProcessTypes /*eProcess*/, CvString* toolTipSink) const
{
	int iMultiplier = getProductionModifier(toolTipSink);

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getBuildingClassPrereqBuilding(BuildingTypes eBuilding, BuildingClassTypes ePrereqBuildingClass, int iExtra) const
{
	CvBuildingEntry* pkBuilding = GC.getBuildingInfo(eBuilding);
	if(pkBuilding == NULL)
	{
		CvAssertMsg(pkBuilding, "Should never happen...");
		return -1;
	}

	int iPrereqs = pkBuilding->GetPrereqNumOfBuildingClass(ePrereqBuildingClass);

	// dont bother with the rest of the calcs if we have no prereqs
	if(iPrereqs == 0)
	{
		return 0;
	}
	// -1 means Building is needed in all Cities
	else if(iPrereqs == -1)
	{
		int iNonPuppetCities = 0;
		int iLoop = 0;
		const CvCity* pLoopCity = NULL;
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if(pLoopCity && !pLoopCity->IsPuppet())
			{
				iNonPuppetCities++;
			}
		}

		return iNonPuppetCities;
	}
	else
	{
		iPrereqs *= std::max(0, GC.getMap().getWorldInfo().getBuildingClassPrereqModifier() + 100);
		iPrereqs /= 100;
	}

	if(!isLimitedWonderClass(pkBuilding->GetBuildingClassInfo()))
	{
		BuildingClassTypes eBuildingClass = (BuildingClassTypes)pkBuilding->GetBuildingClassType();
		iPrereqs *= (getBuildingClassCount(eBuildingClass) + iExtra + 1);
	}

	if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		iPrereqs = std::min(1, iPrereqs);
	}

	return iPrereqs;
}


//	--------------------------------------------------------------------------------
void CvPlayer::removeBuildingClass(BuildingClassTypes eBuildingClass)
{
	CvCity* pLoopCity;
	BuildingTypes eBuilding;
	int iLoop;

	eBuilding = ((BuildingTypes)(getCivilizationInfo().getCivilizationBuildings(eBuildingClass)));

	if(eBuilding != NO_BUILDING)
	{
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if(pLoopCity->GetCityBuildings()->GetNumRealBuilding(eBuilding) > 0)
			{
				pLoopCity->GetCityBuildings()->SetNumRealBuilding(eBuilding, 0);
				break;
			}
		}
	}
}

//	--------------------------------------------------------------------------------
// What is the effect of a building on the player?
void CvPlayer::processBuilding(BuildingTypes eBuilding, int iChange, bool bFirst, CvArea* pArea)
{
	int iI, iJ;

	CvBuildingEntry* pBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pBuildingInfo == NULL)
		return;

	// One-shot items
	if(bFirst && iChange > 0)
	{
#ifdef CAN_BUILD_OU_AND_NIA_ONLY_ONCE
	if(eBuilding == (BuildingTypes)GC.getInfoTypeForString("BUILDING_OXFORD_UNIVERSITY"))
		setOxfordUniversityWasEverBuilt(true);
	if(eBuilding == (BuildingTypes)GC.getInfoTypeForString("BUILDING_INTELLIGENCE_AGENCY"))
		setNationalIntelligenceAgencyWasEverBuilt(true);
#endif

		// Free Policies
		int iFreePolicies = pBuildingInfo->GetFreePolicies();
		if(iFreePolicies > 0)
			ChangeNumFreePolicies(iFreePolicies);

		int iFreeGreatPeople = pBuildingInfo->GetFreeGreatPeople();
		if(iFreeGreatPeople > 0)
			ChangeNumFreeGreatPeople(iFreeGreatPeople);

		// Golden Age
		if(pBuildingInfo->IsGoldenAge())
		{
			int iGoldenAgeTurns = getGoldenAgeLength();
#ifdef TAJ_MAHAL_STARTS_GA_NEXT_TURN
			setBuildingGoldenAgeTurns(iGoldenAgeTurns);
#else
			changeGoldenAgeTurns(iGoldenAgeTurns);
#endif
		}

		// Global Pop change
		if(pBuildingInfo->GetGlobalPopulationChange() != 0)
		{
			CvCity* pLoopCity;
			int iLoop;

			for(iI = 0; iI < MAX_PLAYERS; iI++)
			{
				if(GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					if(GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
					{
						if(pBuildingInfo->IsTeamShare() || (iI == GetID()))
						{
							for(pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
							{
#ifdef BUILDING_GROWTH_GOLD
								if (iChange > 0)
								{
									pLoopCity->changePopulation(iChange * GC.getBuildingInfo(eBuilding)->GetGlobalPopulationChange());
								}
#else
								pLoopCity->setPopulation(std::max(1, (pLoopCity->getPopulation() + iChange * GC.getBuildingInfo(eBuilding)->GetGlobalPopulationChange())));
#endif
							}
						}
					}
				}
			}
		}

		// Free techs
		if(pBuildingInfo->GetFreeTechs() > 0)
		{
			if(!isHuman())
			{
				for(iI = 0; iI < pBuildingInfo->GetFreeTechs(); iI++)
				{
					for(int iLoop = 0; iLoop < iChange; iLoop++)
						AI_chooseFreeTech();
				}
			}
			else
			{
				Localization::String localizedText = Localization::Lookup("TXT_KEY_MISC_COMPLETED_WONDER_CHOOSE_TECH");
				localizedText << pBuildingInfo->GetTextKey();
				chooseTech(pBuildingInfo->GetFreeTechs() * iChange, localizedText.toUTF8());
			}
		}
		if(pBuildingInfo->GetMedianTechPercentChange() > 0)
		{
			ChangeMedianTechPercentage(pBuildingInfo->GetMedianTechPercentChange());
		}

		if(pBuildingInfo->GetExtraSpies() > 0)
		{
			CvPlayerEspionage* pEspionage = GetEspionage();
			CvAssertMsg(pEspionage, "pEspionage is null! What's up with that?!");
			if(pEspionage)
			{
				int iNumSpies = pBuildingInfo->GetExtraSpies();
				for(int i = 0; i < iNumSpies; i++)
				{
					pEspionage->CreateSpy();
				}
			}
		}

		if(pBuildingInfo->GetInstantSpyRankChange() > 0)
		{
			CvPlayerEspionage* pEspionage = GetEspionage();
			CvAssertMsg(pEspionage, "pEspionage is null! What's up with that?!");
			if(pEspionage)
			{
				for(uint ui = 0; ui < pEspionage->m_aSpyList.size(); ui++)
				{
#ifdef NEW_DIPLOMATS_MISSIONS
					CvCity* pCity = pEspionage->GetCityWithSpy(ui);
					int iSurveillanceSightRange = GetEspionage()->SurveillanceSightRange(pCity);
					if (iSurveillanceSightRange > 0)
					{
						pCity->plot()->changeSightInRing(getTeam(), iSurveillanceSightRange, false, NO_INVISIBLE);
					}
#endif
					pEspionage->LevelUpSpy(ui);
#ifdef NEW_DIPLOMATS_MISSIONS
					iSurveillanceSightRange = GetEspionage()->SurveillanceSightRange(pCity);
					if (iSurveillanceSightRange > 0)
					{
						pCity->plot()->changeSightInRing(getTeam(), iSurveillanceSightRange, true, NO_INVISIBLE);
					}
#endif
				}
			}
		}

		if(pBuildingInfo->GetSpyRankChange() > 0)
		{
			ChangeStartingSpyRank(pBuildingInfo->GetSpyRankChange());
		}

		// Free Gold
		if(pBuildingInfo->GetGold() > 0)
			GetTreasury()->ChangeGold(pBuildingInfo->GetGold());

		// Instant Friendship change with all Minors
		int iMinorFriendshipChange = pBuildingInfo->GetMinorFriendshipChange();
		if(iMinorFriendshipChange != 0)
		{
			int iNewValue;
			iMinorFriendshipChange += 100;	// Make it a mod

			for(int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
			{
				iNewValue = GET_PLAYER((PlayerTypes) iMinorLoop).GetMinorCivAI()->GetBaseFriendshipWithMajorTimes100(GetID());
				iNewValue *= iMinorFriendshipChange;
				iNewValue /= 100;

				GET_PLAYER((PlayerTypes) iMinorLoop).GetMinorCivAI()->SetFriendshipWithMajorTimes100(GetID(), iNewValue);
			}
		}
	}

#ifdef FIX_FREE_BUILIDNG_STUCKING
	if (bFirst && iChange > 0)
	{
		if (pBuildingInfo->GetFreeBuildingClass() != NO_BUILDINGCLASS)
		{
			BuildingTypes eFreeBuilding = (BuildingTypes)getCivilizationInfo().getCivilizationBuildings(pBuildingInfo->GetFreeBuildingClass());
			changeFreeBuildingCount(eFreeBuilding, iChange);
		}
	}
#else
	if(pBuildingInfo->GetFreeBuildingClass() != NO_BUILDINGCLASS)
	{
		BuildingTypes eFreeBuilding = (BuildingTypes)getCivilizationInfo().getCivilizationBuildings(pBuildingInfo->GetFreeBuildingClass());
		changeFreeBuildingCount(eFreeBuilding, iChange);
	}
#endif

	// Unit upgrade cost mod
	ChangeUnitUpgradeCostMod(pBuildingInfo->GetUnitUpgradeCostMod() * iChange);

	// Policy cost mod
	ChangePolicyCostBuildingModifier(pBuildingInfo->GetPolicyCostModifier() * iChange);

	// Border growth mods
	ChangePlotCultureCostModifier(pBuildingInfo->GetGlobalPlotCultureCostModifier() * iChange);
	ChangePlotGoldCostMod(pBuildingInfo->GetGlobalPlotBuyCostModifier() * iChange);

	// City Culture Mod
	ChangeJONSCultureCityModifier(pBuildingInfo->GetGlobalCultureRateModifier() * iChange);

	// Trade route gold modifier
	GetTreasury()->ChangeCityConnectionTradeRouteGoldModifier(pBuildingInfo->GetCityConnectionTradeRouteModifier() * iChange);

	// Free Promotion
	PromotionTypes eFreePromotion = (PromotionTypes) pBuildingInfo->GetFreePromotion();
	if(eFreePromotion != NO_PROMOTION)
		ChangeFreePromotionCount(eFreePromotion, iChange);

	// Free Promotion Removed
	PromotionTypes eFreePromotionRemoved = (PromotionTypes) pBuildingInfo->GetFreePromotionRemoved();
	if(eFreePromotionRemoved != NO_PROMOTION)
		ChangeFreePromotionCount(eFreePromotionRemoved, -iChange);

	// Extra Happiness Per City
	ChangeExtraHappinessPerCity(pBuildingInfo->GetHappinessPerCity() * iChange);

	// Extra Happiness Per Policy
	ChangeExtraHappinessPerXPolicies(pBuildingInfo->GetHappinessPerXPolicies() * iChange);

	// City Count Unhappiness Mod
	ChangeCityCountUnhappinessMod(pBuildingInfo->GetCityCountUnhappinessMod() * iChange);

	// Hurries
	for(iI = 0; iI < GC.getNumHurryInfos(); iI++)
	{
		changeHurryModifier((HurryTypes) iI, (pBuildingInfo->GetHurryModifier(iI) * iChange));
	}

	changeGreatPeopleRateModFromBldgs(pBuildingInfo->GetGlobalGreatPeopleRateModifier() * iChange);
	changeGreatGeneralRateModFromBldgs(pBuildingInfo->GetGreatGeneralRateModifier() * iChange);
	ChangeGreatScientistBeakerMod(pBuildingInfo->GetGreatScientistBeakerModifier() * iChange);
	ChangeGreatPersonExpendGold(pBuildingInfo->GetGreatPersonExpendGold() * iChange);
	recomputeGreatPeopleModifiers();

	changeGoldenAgeModifier(pBuildingInfo->GetGoldenAgeModifier() * iChange);
	changeFreeExperienceFromBldgs(pBuildingInfo->GetGlobalFreeExperience() * iChange);
	changeWorkerSpeedModifier(pBuildingInfo->GetWorkerSpeedModifier() * iChange);
	ChangeSpecialistCultureChange(pBuildingInfo->GetSpecialistExtraCulture() * iChange);
	changeBorderObstacleCount(pBuildingInfo->IsPlayerBorderObstacle() * iChange);
#ifdef TEMPLE_ARTEMIS_NO_YIELD_MOD_BUT_GROWTH
	ChangeCityGrowthMod(pBuildingInfo->GetGlobalYieldModifier(int(YIELD_FOOD)) * iChange);
#endif
#ifdef BUILDING_BORDER_TRANSITION_OBSTACLE
	changeBorderTransitionObstacleCount(pBuildingInfo->IsPlayerBorderTransitionObstacle() * iChange);
#endif

	changeSpaceProductionModifier(pBuildingInfo->GetGlobalSpaceProductionModifier() * iChange);

	for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		pArea->changeYieldRateModifier(GetID(), ((YieldTypes)iI), (pBuildingInfo->GetAreaYieldModifier(iI) * iChange));
#ifndef TEMPLE_ARTEMIS_NO_YIELD_MOD_BUT_GROWTH
		changeYieldRateModifier(((YieldTypes)iI), (pBuildingInfo->GetGlobalYieldModifier(iI) * iChange));
#endif
	}

	for(iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		for(iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
			changeSpecialistExtraYield(((SpecialistTypes)iI), ((YieldTypes)iJ), (pBuildingInfo->GetSpecialistYieldChange(iI, iJ) * iChange));
		}
	}

	int iOldEspionageModifier = GetEspionageModifier();
	ChangeEspionageModifier(pBuildingInfo->GetGlobalEspionageModifier() * iChange);
	if(iOldEspionageModifier != GetEspionageModifier())
	{
		int iLoop;
		CvCity* pLoopCity;
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			for(uint ui = 0; ui < MAX_MAJOR_CIVS; ui++)
			{
				PlayerTypes ePlayer = (PlayerTypes)ui;
				GET_PLAYER(ePlayer).GetEspionage()->UpdateCity(pLoopCity);
			}
		}
	}

	ChangeExtraLeagueVotes(pBuildingInfo->GetExtraLeagueVotes() * iChange);

	// Loop through Cities
	int iLoop;
	CvCity* pLoopCity;
	int iBuildingCount;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		// Building modifiers
		BuildingClassTypes eBuildingClass;
		for(iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			eBuildingClass = (BuildingClassTypes) iI;

			CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
			if(!pkBuildingClassInfo)
			{
				continue;
			}

			eBuilding = (BuildingTypes) getCivilizationInfo().getCivilizationBuildings(eBuildingClass);

			if(eBuilding != NO_BUILDING)
			{
				CvBuildingEntry* pkBuilding = GC.getBuildingInfo(eBuilding);
				if(pkBuilding)
				{
					iBuildingCount = pLoopCity->GetCityBuildings()->GetNumRealBuilding(eBuilding);

					if(iBuildingCount > 0)
					{
						pLoopCity->ChangeJONSCulturePerTurnFromBuildings(pBuildingInfo->GetBuildingClassYieldChange(eBuildingClass, YIELD_CULTURE) * iBuildingCount * iChange);

						// Building Class Yield Stuff
						for(iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
						{
							switch(iJ)
							{
							case YIELD_CULTURE:
							{
								// Skip, handled above
								break;
							}
							case YIELD_FAITH:
							{
								pLoopCity->ChangeFaithPerTurnFromBuildings(pBuildingInfo->GetBuildingClassYieldChange(eBuildingClass, iJ) * iBuildingCount * iChange);
								break;
							}
							default:
							{
								YieldTypes eYield = (YieldTypes) iJ;							
								int iYieldChange = pBuildingInfo->GetBuildingClassYieldChange(eBuildingClass, eYield);
								if(iYieldChange > 0)
								{
									pLoopCity->ChangeBaseYieldRateFromBuildings(eYield, iYieldChange * iBuildingCount * iChange);
								}
							}
							}
						}
					}
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Get yield change from buildings for a specific building class
int CvPlayer::GetBuildingClassYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYieldType)
{
	int rtnValue = 0;

	CvBuildingXMLEntries* pBuildings = GC.GetGameBuildings();

	if(pBuildings)
	{
		for(int i = 0; i < pBuildings->GetNumBuildings(); i++)
		{
			// Do we have this building anywhere in empire?
			if(countNumBuildings((BuildingTypes)i) > 0)
			{
				CvBuildingEntry* pEntry = pBuildings->GetEntry(i);
				if(pEntry)
				{
					rtnValue += pEntry->GetBuildingClassYieldChange(eBuildingClass, eYieldType);
				}
			}
		}
	}

#ifdef BUILDING_CLASS_YIELD_CHANGES
	CvTraitXMLEntries* pTraits = GC.GetGameTraits();
	if(pTraits)
	{
		for(int i = 0; i < pTraits->GetNumTraits(); i++)
		{
			CvPlayerTraits* pPlayerTraits = GetPlayerTraits();
			// Do we have this trait?
			if(pPlayerTraits->HasTrait((TraitTypes)i))
			{
				CvTraitEntry* pEntry = pTraits->GetEntry(i);
				if(pEntry)
				{
					rtnValue += pEntry->GetBuildingClassYieldChanges(eBuildingClass, eYieldType);	
				}
			}
		}
	}
#endif

	return rtnValue;
}

//	--------------------------------------------------------------------------------
/// Can we eBuild on pPlot?
bool CvPlayer::canBuild(const CvPlot* pPlot, BuildTypes eBuild, bool bTestEra, bool bTestVisible, bool bTestGold, bool bTestPlotOwner) const
{
	if(!(pPlot->canBuild(eBuild, GetID(), bTestVisible, bTestPlotOwner)))
	{
		return false;
	}

	if(GC.getBuildInfo(eBuild)->getTechPrereq() != NO_TECH)
	{
#ifdef MINES_ON_LUXES_AFTER_BRONZE_WORKING
		if (strcmp(GC.getBuildInfo(eBuild)->GetType(), "BUILD_MINE") == 0 && GC.getResourceInfo(pPlot->getResourceType()) && GC.getResourceInfo(pPlot->getResourceType())->getResourceUsage() == RESOURCEUSAGE_LUXURY)
		{
			if (!GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)GC.getInfoTypeForString("TECH_BRONZE_WORKING", true)))
			{
				if ((!bTestEra && !bTestVisible) || ((GetCurrentEra() + 1) < GC.getTechInfo((TechTypes)GC.getInfoTypeForString("TECH_BRONZE_WORKING", true))->GetEra()))
				{
					return false;
				}
			}
		} else
#endif
#ifdef IROQUOIS_UA_REWORK
		if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)GC.getBuildInfo(eBuild)->getTechPrereq())) && !(strcmp(GC.getBuildInfo(eBuild)->GetType(), "BUILD_LUMBERMILL") == 0 && GetPlayerTraits()->IsMoveFriendlyWoodsAsRoad()))
#else
		if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)GC.getBuildInfo(eBuild)->getTechPrereq())))
#endif
		{
			if((!bTestEra && !bTestVisible) || ((GetCurrentEra() + 1) < GC.getTechInfo((TechTypes) GC.getBuildInfo(eBuild)->getTechPrereq())->GetEra()))
			{
				return false;
			}
		}
	}

	// Is this an improvement that is only useable by a specific civ?
	ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(eBuild)->getImprovement();
	if(eImprovement != NO_IMPROVEMENT)
	{
		CvImprovementEntry* pkEntry = GC.getImprovementInfo(eImprovement);
		if(pkEntry->IsSpecificCivRequired())
		{
			CivilizationTypes eCiv = pkEntry->GetRequiredCivilization();
			if(eCiv != getCivilizationType())
			{
				return false;
			}
		}
	}

	if(!bTestVisible)
	{
		if(IsBuildBlockedByFeature(eBuild, pPlot->getFeatureType()))
		{
			return false;
		}

		if(bTestGold)
		{
			if(std::max(0, GetTreasury()->GetGold()) < getBuildCost(pPlot, eBuild))
			{
				return false;
			}
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
/// Are we prevented from eBuild-ing because of a Feature on this plot?
bool CvPlayer::IsBuildBlockedByFeature(BuildTypes eBuild, FeatureTypes eFeature) const
{
	// No Feature here to block us
	if(eFeature == NO_FEATURE)
	{
		return false;
	}

	// Build does not remove the Feature on pPlot
	if(!GC.getBuildInfo(eBuild)->isFeatureRemove(eFeature))
	{
		return false;
	}

	TechTypes ePrereqTech = (TechTypes) GC.getBuildInfo(eBuild)->getFeatureTech(eFeature);

	// Clearing Feature doesn't require any Tech, so we can do it right now if we have to
	if(ePrereqTech == NO_TECH)
	{
		return false;
	}

	// Clearing eFeature requires a Tech, but we have it
	if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech(ePrereqTech))
	{
		return false;
	}

	// Feature is blocking us!
	return true;
}

//	--------------------------------------------------------------------------------
// Returns the cost
int CvPlayer::getBuildCost(const CvPlot* pPlot, BuildTypes eBuild) const
{
	CvAssert(eBuild >= 0 && eBuild < GC.getNumBuildInfos());


	CvBuildInfo* pkBuildInfo = GC.getBuildInfo(eBuild);
	if(pkBuildInfo == NULL)
	{
		return 0;
	}

	if(pPlot->getBuildProgress(eBuild) > 0)
	{
		return 0;
	}

	if(pPlot->getRouteType() != NO_ROUTE && pPlot->getRouteType() == pkBuildInfo->getRoute() && pPlot->IsRoutePillaged())
	{
		return 0;
	}

	int iBuildCost = pkBuildInfo->getCost();

	// Cost increases as more Improvements are built
	iBuildCost += (getTotalImprovementsBuilt() * pkBuildInfo->getCostIncreasePerImprovement());

	iBuildCost *= (100 + getImprovementCostModifier());
	iBuildCost /= 100;

	if(pPlot->getFeatureType() != NO_FEATURE)
	{
		iBuildCost += pkBuildInfo->getFeatureCost(pPlot->getFeatureType());
	}

	iBuildCost *= getHandicapInfo().getImprovementCostPercent();
	iBuildCost /= 100;

	iBuildCost *= GC.getGame().getGameSpeedInfo().getImprovementPercent();
	iBuildCost /= 100;

	return std::max(0, iBuildCost);
}


//	--------------------------------------------------------------------------------
RouteTypes CvPlayer::getBestRoute(CvPlot* pPlot) const
{
	RouteTypes eRoute;
	RouteTypes eBestRoute;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	eBestRoute = NO_ROUTE;

	for(iI = 0; iI < GC.getNumBuildInfos(); iI++)
	{
		const BuildTypes eBuild = static_cast<BuildTypes>(iI);
		CvBuildInfo* pkBuildInfo = GC.getBuildInfo(eBuild);
		if(pkBuildInfo)
		{
			eRoute = ((RouteTypes)(pkBuildInfo->getRoute()));
			if(eRoute != NO_ROUTE)
			{
				CvRouteInfo* pkRouteInfo = GC.getRouteInfo(eRoute);
				if(pkRouteInfo)
				{
					if((pPlot != NULL) ? ((pPlot->getRouteType() == eRoute) || canBuild(pPlot, eBuild)) : GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)(pkBuildInfo->getTechPrereq())))
					{
						iValue = pkRouteInfo->getValue();

						if(iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestRoute = eRoute;
						}
					}
				}
			}
		}
	}

	return eBestRoute;
}


//	--------------------------------------------------------------------------------
int CvPlayer::getImprovementUpgradeRate() const
{
	int iRate;

	iRate = 100; // XXX

	iRate *= std::max(0, (getImprovementUpgradeRateModifier() + 100));
	iRate /= 100;

	return iRate;
}

//	--------------------------------------------------------------------------------
/// How much Production do we get from removing ANY Feature in the game? (Policy Bonus)
int CvPlayer::GetAllFeatureProduction() const
{
	return m_iAllFeatureProduction;
}

//	--------------------------------------------------------------------------------
/// Changes how much Production we get from removing ANY Feature in the game (Policy Bonus)
void CvPlayer::ChangeAllFeatureProduction(int iChange)
{
	if(iChange != 0)
	{
		m_iAllFeatureProduction += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// How far a tile is from a city (1-3) This is used to determine camera zoom on the city view
int CvPlayer::GetCityDistanceHighwaterMark() const
{
	return m_iCityDistanceHighwaterMark;
}

//	--------------------------------------------------------------------------------
/// Set how far a tile is from a city (1-3) This is used to determine camera zoom on the city view
void CvPlayer::SetCityDistanceHighwaterMark(int iNewValue)
{
	m_iCityDistanceHighwaterMark = iNewValue;
}


//	--------------------------------------------------------------------------------
int CvPlayer::calculateTotalYield(YieldTypes eYield) const
{
	const CvCity* pLoopCity;
	int iTotalYield = 0;
	int iLoop = 0;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iTotalYield += pLoopCity->getYieldRateTimes100(eYield, false);
	}

	return iTotalYield / 100;
}

//	--------------------------------------------------------------------------------
/// How much does Production is being eaten up by Units? (cached)
int CvPlayer::GetUnitProductionMaintenanceMod() const
{
	// Kind of a cop-out, but it fixes some bugs for now
	return calculateUnitProductionMaintenanceMod();
}

//	--------------------------------------------------------------------------------
/// How much does Production is being eaten up by Units? (update cache)
void CvPlayer::UpdateUnitProductionMaintenanceMod()
{
	m_iUnitProductionMaintenanceMod = calculateUnitProductionMaintenanceMod();

	if(GetID() == GC.getGame().getActivePlayer())
	{
		GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
	}
}

//	--------------------------------------------------------------------------------
/// How much does Production is being eaten up by Units?
int CvPlayer::calculateUnitProductionMaintenanceMod() const
{
	int iPaidUnits = GetNumUnitsOutOfSupply();

	// Example: Player can support 8 Units, he has 12. 4 * 10 means he loses 40% of his Production
	int iMaintenanceMod = min(/*70*/ GC.getMAX_UNIT_SUPPLY_PRODMOD(), iPaidUnits * 10);
	iMaintenanceMod = -iMaintenanceMod;

	return iMaintenanceMod;
}

//	--------------------------------------------------------------------------------
/// How many Units can we support for free without paying Production?
int CvPlayer::GetNumUnitsSupplied() const
{
	int iFreeUnits = GetNumUnitsSuppliedByHandicap();
	iFreeUnits += GetNumUnitsSuppliedByCities();
	iFreeUnits += GetNumUnitsSuppliedByPopulation();

	if(!isMinorCiv() && !isHuman() && !IsAITeammateOfHuman())
	{
		int iMod = (100 + GC.getGame().getHandicapInfo().getAIUnitSupplyPercent());
		iFreeUnits *= iMod;
		iFreeUnits /= 100;
	}

	return iFreeUnits;
}

//	--------------------------------------------------------------------------------
/// Units supplied from Difficulty Level
int CvPlayer::GetNumUnitsSuppliedByHandicap() const
{
	return getHandicapInfo().getProductionFreeUnits();
}

//	--------------------------------------------------------------------------------
/// Units supplied from Difficulty Level
int CvPlayer::GetNumUnitsSuppliedByCities() const
{
	return getHandicapInfo().getProductionFreeUnitsPerCity() * getNumCities();
}

//	--------------------------------------------------------------------------------
/// Units supplied from Difficulty Level
int CvPlayer::GetNumUnitsSuppliedByPopulation() const
{
	return getTotalPopulation() * getHandicapInfo().getProductionFreeUnitsPopulationPercent() / 100;
}

#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
int CvPlayer::GetNumTrainedUnits() const
{
	return m_iNumTrainedUnits;
}
void CvPlayer::ChangeNumTrainedUnits(int iChange)
{
	m_iNumTrainedUnits = (m_iNumTrainedUnits + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
int CvPlayer::GetNumKilledUnits() const
{
	return m_iNumKilledUnits;
}
void CvPlayer::ChangeNumKilledUnits(int iChange)
{
	m_iNumKilledUnits = (m_iNumKilledUnits + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
int CvPlayer::GetNumLostUnits() const
{
	return m_iNumLostUnits;
}
void CvPlayer::ChangeNumLostUnits(int iChange)
{
	m_iNumLostUnits = (m_iNumLostUnits + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
int CvPlayer::GetUnitsDamageDealt() const
{
	return m_iUnitsDamageDealt;
}
void CvPlayer::ChangeUnitsDamageDealt(int iChange)
{
	m_iUnitsDamageDealt = (m_iUnitsDamageDealt + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
int CvPlayer::GetUnitsDamageTaken() const
{
	return m_iUnitsDamageTaken;
}
void CvPlayer::ChangeUnitsDamageTaken(int iChange)
{
	m_iUnitsDamageTaken = (m_iUnitsDamageTaken + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
int CvPlayer::GetCitiesDamageDealt() const
{
	return m_iCitiesDamageDealt;
}
void CvPlayer::ChangeCitiesDamageDealt(int iChange)
{
	m_iCitiesDamageDealt = (m_iCitiesDamageDealt + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
int CvPlayer::GetCitiesDamageTaken() const
{
	return m_iCitiesDamageTaken;
}
void CvPlayer::ChangeCitiesDamageTaken(int iChange)
{
	m_iCitiesDamageTaken = (m_iCitiesDamageTaken + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
int CvPlayer::GetNumScientistsTotal() const
{
	return m_iNumScientistsTotal;
}
void CvPlayer::ChangeNumScientistsTotal(int iChange)
{
	m_iNumScientistsTotal = (m_iNumScientistsTotal + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
int CvPlayer::GetNumEngineersTotal() const
{
	return m_iNumEngineersTotal;
}
void CvPlayer::ChangeNumEngineersTotal(int iChange)
{
	m_iNumEngineersTotal = (m_iNumEngineersTotal + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
int CvPlayer::GetNumMerchantsTotal() const
{
	return m_iNumMerchantsTotal;
}
void CvPlayer::ChangeNumMerchantsTotal(int iChange)
{
	m_iNumMerchantsTotal = (m_iNumMerchantsTotal + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
int CvPlayer::GetNumWritersTotal() const
{
	return m_iNumWritersTotal;
}
void CvPlayer::ChangeNumWritersTotal(int iChange)
{
	m_iNumWritersTotal = (m_iNumWritersTotal + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
int CvPlayer::GetNumAristsTotal() const
{
	return m_iNumArtistsTotal;
}
void CvPlayer::ChangeNumArtistsTotal(int iChange)
{
	m_iNumArtistsTotal = (m_iNumArtistsTotal + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
int CvPlayer::GetNumMusiciansTotal() const
{
	return m_iNumMusiciansTotal;
}
void CvPlayer::ChangeNumMusiciansTotal(int iChange)
{
	m_iNumMusiciansTotal = (m_iNumMusiciansTotal + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
int CvPlayer::GetNumGeneralsTotal() const
{
	return m_iNumGeneralsTotal;
}
void CvPlayer::ChangeNumGeneralsTotal(int iChange)
{
	m_iNumGeneralsTotal = (m_iNumGeneralsTotal + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
int CvPlayer::GetNumAdmiralsTotal() const
{
	return m_iNumAdmiralsTotal;
}
void CvPlayer::ChangeNumAdmiralsTotal(int iChange)
{
	m_iNumAdmiralsTotal = (m_iNumAdmiralsTotal + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
int CvPlayer::GetNumProphetsTotal() const
{
	return m_iNumProphetsTotal;
}
void CvPlayer::ChangeNumProphetsTotal(int iChange)
{
	m_iNumProphetsTotal = (m_iNumProphetsTotal + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
int CvPlayer::GetProductionGoldFromWonders() const
{
	return m_iProductionGoldFromWonders;
}
void CvPlayer::ChangeProductionGoldFromWonders(int iChange)
{
	m_iProductionGoldFromWonders = (m_iProductionGoldFromWonders + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
int CvPlayer::GetNumChops() const
{
	return m_iNumChops;
}
void CvPlayer::ChangeNumChops(int iChange)
{
	m_iNumChops = (m_iNumChops + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
int CvPlayer::GetNumTimesOpenedDemographics() const
{
	return m_iNumTimesOpenedDemographics;
}
void CvPlayer::ChangeNumTimesOpenedDemographics(int iChange)
{
	m_iNumTimesOpenedDemographics = (m_iNumTimesOpenedDemographics + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
bool CvPlayer::GetMayaBoostScientist() const
{
	return m_bMayaBoostScientist;
}
void CvPlayer::SetMayaBoostScientist(bool bValue)
{
	m_bMayaBoostScientist = bValue;
}
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
bool CvPlayer::GetMayaBoostEngineers() const
{
	return m_bMayaBoostEngineers;
}
void CvPlayer::SetMayaBoostEngineers(bool bValue)
{
	m_bMayaBoostEngineers = bValue;
}
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
bool CvPlayer::GetMayaBoostMerchants() const
{
	return m_bMayaBoostMerchants;
}
void CvPlayer::SetMayaBoostMerchants(bool bValue)
{
	m_bMayaBoostMerchants = bValue;
}
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
bool CvPlayer::GetMayaBoostWriters() const
{
	return m_bMayaBoostWriters;
}
void CvPlayer::SetMayaBoostWriters(bool bValue)
{
	m_bMayaBoostWriters = bValue;
}
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
bool CvPlayer::GetMayaBoostArtists() const
{
	return m_bMayaBoostArtists;
}
void CvPlayer::SetMayaBoostArtists(bool bValue)
{
	m_bMayaBoostArtists = bValue;
}
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
bool CvPlayer::GetMayaBoostMusicians() const
{
	return m_bMayaBoostMusicians;
}
void CvPlayer::SetMayaBoostMusicians(bool bValue)
{
	m_bMayaBoostMusicians = bValue;
}
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
int CvPlayer::GetScientistsTotalScienceBoost() const
{
	return m_iScientistsTotalScienceBoost;
}
void CvPlayer::ChangeScientistsTotalScienceBoost(int iChange)
{
	m_iScientistsTotalScienceBoost = (m_iScientistsTotalScienceBoost + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
int CvPlayer::GetEngineersTotalHurryBoost() const
{
	return m_iEngineersTotalHurryBoost;
}
void CvPlayer::ChangeEngineersTotalHurryBoost(int iChange)
{
	m_iEngineersTotalHurryBoost = (m_iEngineersTotalHurryBoost + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
int CvPlayer::GetMerchantsTotalTradeBoost() const
{
	return m_iMerchantsTotalTradeBoost;
}
void CvPlayer::ChangeMerchantsTotalTradeBoost(int iChange)
{
	m_iMerchantsTotalTradeBoost = (m_iMerchantsTotalTradeBoost + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
int CvPlayer::GetWritersTotalCultureBoost() const
{
	return m_iWritersTotalCultureBoost;
}
void CvPlayer::ChangeWritersTotalCultureBoost(int iChange)
{
	m_iWritersTotalCultureBoost = (m_iWritersTotalCultureBoost + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
int CvPlayer::GetMusiciansTotalTourismBoost() const
{
	return m_iMusiciansTotalTourismBoost;
}
void CvPlayer::ChangeMusiciansTotalTourismBoost(int iChange)
{
	m_iMusiciansTotalTourismBoost = (m_iMusiciansTotalTourismBoost + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
int CvPlayer::GetNumPopulationLostFromNukes() const
{
	return m_iNumPopulationLostFromNukes;
}
void CvPlayer::ChangeNumPopulationLostFromNukes(int iChange)
{
	m_iNumPopulationLostFromNukes = (m_iNumPopulationLostFromNukes + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
int CvPlayer::GetNumCSQuestsCompleted() const
{
	return m_iNumCSQuestsCompleted;
}
void CvPlayer::ChangeNumCSQuestsCompleted(int iChange)
{
	m_iNumCSQuestsCompleted = (m_iNumCSQuestsCompleted + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
int CvPlayer::GetNumAlliedCS() const
{
	return m_iNumAlliedCS;
}
void CvPlayer::ChangeNumAlliedCS(int iChange)
{
	m_iNumAlliedCS = (m_iNumAlliedCS + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
int CvPlayer::GetTimesEnteredCityScreen() const
{
	return m_iTimesEnteredCityScreen;
}
void CvPlayer::ChangeTimesEnteredCityScreen(int iChange)
{
	m_iTimesEnteredCityScreen = (m_iTimesEnteredCityScreen + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_HAPPINESSFROMTRADEDEALS
int CvPlayer::GetNumHappinessFromTradeDeals() const
{
	int iHappinessFromTradeDeals = 0;
	for (int iI = 0; iI < GC.getNumResourceInfos(); iI++)
	{
		ResourceTypes eResource = (ResourceTypes)iI;

		int iBaseHappiness = GetHappinessFromLuxury(eResource);
		ResourceUsageTypes eUsage = GC.getResourceInfo(eResource)->getResourceUsage();
		if (eUsage == RESOURCEUSAGE_LUXURY)
		{
			if (iBaseHappiness)
			{
				if (getResourceImport(eResource) > 0 && getNumResourceTotal(eResource) == getResourceImport(eResource))
				{
					iHappinessFromTradeDeals += iBaseHappiness;
					iHappinessFromTradeDeals += GetExtraHappinessPerLuxury();
				}
				if (getResourceExport(eResource) > 0 && getNumResourceTotal(eResource) == 0)
				{
					iHappinessFromTradeDeals -= iBaseHappiness;
					iHappinessFromTradeDeals -= GetExtraHappinessPerLuxury();
				}
				if (getResourceExport(eResource) > 0 && getNumResourceTotal(eResource) == 0 && GetPlayerTraits()->GetLuxuryHappinessRetention() > 0)
				{
					iHappinessFromTradeDeals += (iBaseHappiness * GetPlayerTraits()->GetLuxuryHappinessRetention()) / 100;
				}
			}
		}
	}
	return iHappinessFromTradeDeals;
}
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
int CvPlayer::GetNumDiedSpies() const
{
	return m_iNumDiedSpies;
}
void CvPlayer::ChangeNumDiedSpies(int iChange)
{
	m_iNumDiedSpies = (m_iNumDiedSpies + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
int CvPlayer::GetNumKilledSpies() const
{
	return m_iNumKilledSpies;
}
void CvPlayer::ChangeNumKilledSpies(int iChange)
{
	m_iNumKilledSpies = (m_iNumKilledSpies + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
int CvPlayer::GetFoodFromMinorsTimes100() const
{
	return m_iFoodFromMinorsTimes100;
}
void CvPlayer::ChangeFoodFromMinorsTimes100(int iChange)
{
	m_iFoodFromMinorsTimes100 = (m_iFoodFromMinorsTimes100 + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
int CvPlayer::GetProductionFromMinorsTimes100() const
{
	return m_iProductionFromMinorsTimes100;
}
void CvPlayer::ChangeProductionFromMinorsTimes100(int iChange)
{
	m_iProductionFromMinorsTimes100 = (m_iProductionFromMinorsTimes100 + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
int CvPlayer::GetNumUnitsFromMinors() const
{
	return m_iNumUnitsFromMinors;
}
void CvPlayer::ChangeNumUnitsFromMinors(int iChange)
{
	m_iNumUnitsFromMinors = (m_iNumUnitsFromMinors + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
int CvPlayer::GetNumCreatedWorldWonders() const
{
	return m_iNumCreatedWorldWonders;
}
void CvPlayer::ChangeNumCreatedWorldWonders(int iChange)
{
	m_iNumCreatedWorldWonders = (m_iNumCreatedWorldWonders + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
int CvPlayer::GetNumGoldSpentOnBuildingBuys() const
{
	return m_iNumGoldSpentOnBuildingBuys;
}
void CvPlayer::ChangeNumGoldSpentOnBuildingBuys(int iChange)
{
	m_iNumGoldSpentOnBuildingBuys = (m_iNumGoldSpentOnBuildingBuys + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
int CvPlayer::GetNumGoldSpentOnUnitBuys() const
{
	return m_iNumGoldSpentOnUnitBuys;
}
void CvPlayer::ChangeNumGoldSpentOnUnitBuys(int iChange)
{
	m_iNumGoldSpentOnUnitBuys = (m_iNumGoldSpentOnUnitBuys + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
int CvPlayer::GetNumGoldSpentOnUgrades() const
{
	return m_iNumGoldSpentOnUgrades;
}
void CvPlayer::ChangeNumGoldSpentOnUgrades(int iChange)
{
	m_iNumGoldSpentOnUgrades = (m_iNumGoldSpentOnUgrades + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
int CvPlayer::GetGoldFromKills() const
{
	return m_iGoldFromKills;
}
void CvPlayer::ChangeGoldFromKills(int iChange)
{
	m_iGoldFromKills = (m_iGoldFromKills + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
int CvPlayer::GetCultureFromKills() const
{
	return m_iCultureFromKills;
}
void CvPlayer::ChangeCultureFromKills(int iChange)
{
	m_iCultureFromKills = (m_iCultureFromKills + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
int CvPlayer::GetNumGoldSpentOnGPBuys() const
{
	return m_iNumGoldSpentOnGPBuys;
}
void CvPlayer::ChangeNumGoldSpentOnGPBuys(int iChange)
{
	m_iNumGoldSpentOnGPBuys = (m_iNumGoldSpentOnGPBuys + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
int CvPlayer::GetNumGoldSpentOnTilesBuys() const
{
	return m_iNumGoldSpentOnTilesBuys;
}
void CvPlayer::ChangeNumGoldSpentOnTilesBuys(int iChange)
{
	m_iNumGoldSpentOnTilesBuys = (m_iNumGoldSpentOnTilesBuys + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
int CvPlayer::GetNumGoldFromPillage() const
{
	return m_iNumGoldFromPillage;
}
void CvPlayer::ChangeNumGoldFromPillage(int iChange)
{
	m_iNumGoldFromPillage = (m_iNumGoldFromPillage + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
int CvPlayer::GetNumGoldFromPlunder() const
{
	return m_iNumGoldFromPlunder;
}
void CvPlayer::ChangeNumGoldFromPlunder(int iChange)
{
	m_iNumGoldFromPlunder = (m_iNumGoldFromPlunder + iChange);
}
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
int CvPlayer::GetNumFaithSpentOnMilitaryUnits() const
{
	return m_iNumFaithSpentOnMilitaryUnits;
}
void CvPlayer::ChangeNumFaithSpentOnMilitaryUnits(int iChange)
{
	m_iNumFaithSpentOnMilitaryUnits = (m_iNumFaithSpentOnMilitaryUnits + iChange);
}
#endif

//	--------------------------------------------------------------------------------
/// How much Units are eating Production?
int CvPlayer::GetNumUnitsOutOfSupply() const
{
	int iFreeUnits = GetNumUnitsSupplied();
	int iNumUnits = getNumUnits();

	int iNumTradeUnits = 0;
	int iLoop = 0;
	const CvUnit* pLoopUnit = NULL;
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->isTrade())
		{
			iNumTradeUnits++;
		}
	}

	int iNumUnitsToSupply = iNumUnits - iNumTradeUnits;
	return std::max(0, iNumUnitsToSupply - iFreeUnits);
}

//	--------------------------------------------------------------------------------
int CvPlayer::calculateUnitCost() const
{
	int iFreeUnits;
	int iPaidUnits;
	int iBaseUnitCost;
	int iExtraCost;

	return GetTreasury()->CalculateUnitCost(iFreeUnits, iPaidUnits, iBaseUnitCost, iExtraCost);
}

//	--------------------------------------------------------------------------------
int CvPlayer::calculateUnitSupply() const
{
	int iPaidUnits;
	int iBaseSupplyCost;

	return GetTreasury()->CalculateUnitSupply(iPaidUnits, iBaseSupplyCost);
}

//	--------------------------------------------------------------------------------
int CvPlayer::calculateResearchModifier(TechTypes eTech)
{
	int iModifier = 100;

	if(NO_TECH == eTech)
	{
		return iModifier;
	}

#ifdef AUI_TECH_TOGGLEABLE_ALREADY_KNOWN_TECH_COST_DISCOUNT
	if (!GC.getGame().isOption("GAMEOPTION_NO_TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER"))
	{
#endif
	int iKnownCount = 0;
	int iPossibleKnownCount = 0;
	for(int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if(kLoopTeam.isAlive() && !kLoopTeam.isMinorCiv())
		{
			if(GET_TEAM(getTeam()).isHasMet((TeamTypes)iI))
			{
#ifdef HAS_TECH_BY_HUMAN
				if(kLoopTeam.GetTeamTechs()->HasTechByHuman(eTech))
#else
				if(kLoopTeam.GetTeamTechs()->HasTech(eTech))
#endif
				{
					iKnownCount++;
				}
			}
			iPossibleKnownCount++;
		}
	}
	if(iPossibleKnownCount > 0)
	{
#ifdef DUEL_TOGGLEABLE_LESS_ALREADY_KNOWN_TECH_COST
		if (GC.getGame().isOption("GAMEOPTION_DUEL_STUFF"))
		{
			iModifier += (10 * iKnownCount) / iPossibleKnownCount;
		} else
		{
			iModifier += (GC.getTECH_COST_TOTAL_KNOWN_TEAM_MODIFIER() * iKnownCount) / iPossibleKnownCount;
		}
#else
		iModifier += (GC.getTECH_COST_TOTAL_KNOWN_TEAM_MODIFIER() * iKnownCount) / iPossibleKnownCount;
#endif
	}
#ifdef AUI_TECH_TOGGLEABLE_ALREADY_KNOWN_TECH_COST_DISCOUNT
	}
#endif

	int iPossiblePaths = 0;
	int iUnknownPaths = 0;
	for(int iI = 0; iI < GC.getNUM_OR_TECH_PREREQS(); iI++)
	{
		if(GC.getTechInfo(eTech)->GetPrereqOrTechs(iI) != NO_TECH)
		{
			if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)(GC.getTechInfo(eTech)->GetPrereqOrTechs(iI)))))
			{
				iUnknownPaths++;
			}

			iPossiblePaths++;
		}
	}
	CvAssertMsg(iPossiblePaths >= iUnknownPaths, "The number of possible paths is expected to match or exceed the number of unknown ones");
	iModifier += (iPossiblePaths - iUnknownPaths) * GC.getTECH_COST_KNOWN_PREREQ_MODIFIER();

	// Leagues mod
	int iLeaguesMod = GC.getGame().GetGameLeagues()->GetResearchMod(GetID(), eTech);
#ifdef NEW_RESOLUTION_MEMBER_DISCOVERED_TECH_DISCOUNT
	iModifier += iLeaguesMod;
#else
	if (iLeaguesMod != 0)
	{
		iModifier *= (100 + iLeaguesMod);
		iModifier /= 100;
	}
#endif

	return iModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::calculateGoldRate() const
{
	return calculateGoldRateTimes100() / 100;
}


//	--------------------------------------------------------------------------------
int CvPlayer::calculateGoldRateTimes100() const
{
	// If we're in anarchy, then no Gold is collected!
#ifdef PENALTY_FOR_DELAYING_POLICIES
	if (IsAnarchy() || IsDelayedPolicy())
#else
	if(IsAnarchy())
#endif
	{
		return 0;
	}

	int iRate = 0;

	iRate = GetTreasury()->CalculateBaseNetGoldTimes100();

	return iRate;
}

//	--------------------------------------------------------------------------------
int CvPlayer::unitsRequiredForGoldenAge() const
{
	return (GC.getBASE_GOLDEN_AGE_UNITS() + (getNumUnitGoldenAges() * GC.getGOLDEN_AGE_UNITS_MULTIPLIER()));
}


//	--------------------------------------------------------------------------------
int CvPlayer::unitsGoldenAgeCapable() const
{
	const CvUnit* pLoopUnit;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->isGoldenAge())
		{
			iCount++;
		}
	}

	return iCount;
}


//	--------------------------------------------------------------------------------
int CvPlayer::unitsGoldenAgeReady() const
{
	const CvUnit* pLoopUnit;
	bool* pabUnitUsed;
	int iCount;
	int iLoop;
	int iI;

	pabUnitUsed = FNEW(bool[GC.getNumUnitInfos()], c_eCiv5GameplayDLL, 0);

	for(iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		pabUnitUsed[iI] = false;
	}

	iCount = 0;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if(!(pabUnitUsed[pLoopUnit->getUnitType()]))
		{
			if(pLoopUnit->isGoldenAge())
			{
				pabUnitUsed[pLoopUnit->getUnitType()] = true;
				iCount++;
			}
		}
	}

	SAFE_DELETE_ARRAY(pabUnitUsed);

	return iCount;
}

//	--------------------------------------------------------------------------------
int CvPlayer::greatGeneralThreshold() const
{
	int iThreshold;

	iThreshold = ((/*200*/ GC.getGREAT_GENERALS_THRESHOLD() * std::max(0, (getGreatGeneralsThresholdModifier() + 100))) / 100);

	iThreshold *= GC.getGame().getGameSpeedInfo().getGreatPeoplePercent();
	iThreshold /= std::max(1, GC.getGame().getGameSpeedInfo().getTrainPercent());

	iThreshold *= GC.getGame().getStartEraInfo().getGreatPeoplePercent();
	iThreshold /= 100;

	return std::max(1, iThreshold);
}

//	--------------------------------------------------------------------------------
int CvPlayer::greatAdmiralThreshold() const
{
	int iThreshold;

	iThreshold = ((/*200*/ GC.getGREAT_GENERALS_THRESHOLD() * std::max(0, (getGreatAdmiralsThresholdModifier() + 100))) / 100);

	iThreshold *= GC.getGame().getGameSpeedInfo().getGreatPeoplePercent();
	iThreshold /= std::max(1, GC.getGame().getGameSpeedInfo().getTrainPercent());

	iThreshold *= GC.getGame().getStartEraInfo().getGreatPeoplePercent();
	iThreshold /= 100;

	return std::max(1, iThreshold);
}

//	--------------------------------------------------------------------------------
int CvPlayer::specialistYield(SpecialistTypes eSpecialist, YieldTypes eYield) const
{
	CvSpecialistInfo* pkSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
	if(pkSpecialistInfo == NULL)
	{
		//This function REQUIRES a valid specialist info.
		CvAssert(pkSpecialistInfo);
		return 0;
	}

	int iRtnValue = pkSpecialistInfo->getYieldChange(eYield) + getSpecialistExtraYield(eSpecialist, eYield) + GetPlayerTraits()->GetSpecialistYieldChange(eSpecialist, eYield);

	if (eSpecialist != GC.getDEFAULT_SPECIALIST())
	{
		iRtnValue += getSpecialistExtraYield(eYield);
	}
	return (iRtnValue);
}

//	--------------------------------------------------------------------------------
/// How much additional Yield does every City produce?
int CvPlayer::GetCityYieldChange(YieldTypes eYield) const
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCityYieldChange[eYield];
}

//	--------------------------------------------------------------------------------
/// Changes how much additional Yield every City produces
void CvPlayer::ChangeCityYieldChange(YieldTypes eYield, int iChange)
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_aiCityYieldChange.setAt(eYield, m_aiCityYieldChange[eYield] + iChange);

		updateYield();
	}
}

//	--------------------------------------------------------------------------------
/// How much additional Yield do coastal Cities produce?
int CvPlayer::GetCoastalCityYieldChange(YieldTypes eYield) const
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCoastalCityYieldChange[eYield];
}

//	--------------------------------------------------------------------------------
/// Changes how much additional Yield coastal Cities produce
void CvPlayer::ChangeCoastalCityYieldChange(YieldTypes eYield, int iChange)
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_aiCoastalCityYieldChange.setAt(eYield, m_aiCoastalCityYieldChange[eYield] + iChange);

		updateYield();
	}
}

//	--------------------------------------------------------------------------------
/// How much additional Yield does the Capital produce?
int CvPlayer::GetCapitalYieldChange(YieldTypes eYield) const
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCapitalYieldChange[eYield];
}

//	--------------------------------------------------------------------------------
/// Changes how much additional Yield the Capital produces
void CvPlayer::ChangeCapitalYieldChange(YieldTypes eYield, int iChange)
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_aiCapitalYieldChange.setAt(eYield, m_aiCapitalYieldChange[eYield] + iChange);

		updateYield();
	}
}

//	--------------------------------------------------------------------------------
/// How much additional Yield does the Capital produce per pop?
int CvPlayer::GetCapitalYieldPerPopChange(YieldTypes eYield) const
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCapitalYieldPerPopChange[eYield];
}

//	--------------------------------------------------------------------------------
/// Changes how much additional Yield the Capital produces per pop
void CvPlayer::ChangeCapitalYieldPerPopChange(YieldTypes eYield, int iChange)
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_aiCapitalYieldPerPopChange.setAt(eYield, m_aiCapitalYieldPerPopChange[eYield] + iChange);

		updateYield();
	}
}

//	--------------------------------------------------------------------------------
/// How much additional Yield does a Great Work produce?
int CvPlayer::GetGreatWorkYieldChange(YieldTypes eYield) const
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiGreatWorkYieldChange[eYield];
}

//	--------------------------------------------------------------------------------
/// Changes how much additional Yield a Great Work produces
void CvPlayer::ChangeGreatWorkYieldChange(YieldTypes eYield, int iChange)
{
	CvAssertMsg(eYield >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_aiGreatWorkYieldChange[eYield] = m_aiGreatWorkYieldChange[eYield] + iChange;
	}
}

//	--------------------------------------------------------------------------------
CvPlot* CvPlayer::getStartingPlot() const
{
	return GC.getMap().plotCheckInvalid(m_iStartingX, m_iStartingY);
}


//	--------------------------------------------------------------------------------
void CvPlayer::setStartingPlot(CvPlot* pNewValue)
{
	CvPlot* pOldStartingPlot;

	pOldStartingPlot = getStartingPlot();

	if(pOldStartingPlot != pNewValue)
	{
		if(pOldStartingPlot != NULL)
		{
			pOldStartingPlot->area()->changeNumStartingPlots(-1);
		}

		if(pNewValue == NULL)
		{
			m_iStartingX = INVALID_PLOT_COORD;
			m_iStartingY = INVALID_PLOT_COORD;
		}
		else
		{
			m_iStartingX = pNewValue->getX();
			m_iStartingY = pNewValue->getY();

			getStartingPlot()->setStartingPlot(true);

			CvArea* pArea = getStartingPlot()->area();
			if(pArea != NULL)
				pArea->changeNumStartingPlots(1);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getTotalPopulation() const
{
	return m_iTotalPopulation;
}


//	--------------------------------------------------------------------------------
int CvPlayer::getAveragePopulation() const
{
	if(getNumCities() == 0)
	{
		return 0;
	}

	return ((getTotalPopulation() / getNumCities()) + 1);
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeTotalPopulation(int iChange)
{
	m_iTotalPopulation = (m_iTotalPopulation + iChange);
	CvAssert(getTotalPopulation() >= 0);
}


//	--------------------------------------------------------------------------------
long CvPlayer::getRealPopulation() const
{
	const CvCity* pLoopCity;
	__int64 iTotalPopulation = 0;
	int iLoop = 0;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
#ifdef SHOW_ACTUAL_POPULATION
		iTotalPopulation += pLoopCity->getPopulation();
#else
		iTotalPopulation += pLoopCity->getRealPopulation();
#endif
	}

	if(iTotalPopulation > INT_MAX)
	{
		iTotalPopulation = INT_MAX;
	}

	return ((long)(iTotalPopulation));
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNewCityExtraPopulation() const
{
	return m_iNewCityExtraPopulation;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeNewCityExtraPopulation(int iChange)
{
	if(iChange != 0)
	{
		m_iNewCityExtraPopulation += iChange;
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetFreeFoodBox() const
{
	return m_iFreeFoodBox;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeFreeFoodBox(int iChange)
{
	if(iChange != 0)
	{
		m_iFreeFoodBox += iChange;
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getTotalLand() const
{
	return m_iTotalLand;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeTotalLand(int iChange)
{
	m_iTotalLand = (m_iTotalLand + iChange);
	CvAssert(getTotalLand() >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getTotalLandScored() const
{
	return m_iTotalLandScored;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeTotalLandScored(int iChange)
{
	if(iChange != 0)
	{
		m_iTotalLandScored = (m_iTotalLandScored + iChange);
		CvAssert(getTotalLandScored() >= 0);
	}
}

//	--------------------------------------------------------------------------------
/// Total culture per turn
int CvPlayer::GetTotalJONSCulturePerTurn() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_POLICIES))
	{
		return 0;
	}

	// No culture during Anarchy
#ifdef PENALTY_FOR_DELAYING_POLICIES
	if (IsAnarchy() || IsDelayedPolicy())
#else
	if(IsAnarchy())
#endif
	{
		return 0;
	}

	int iCulturePerTurn = 0;

	// Culture per turn from Cities
	iCulturePerTurn += GetJONSCulturePerTurnFromCities();

	// Special bonus which adds excess Happiness to Culture?
	iCulturePerTurn += GetJONSCulturePerTurnFromExcessHappiness();

	// Trait bonus which adds Culture for trade partners? 
	iCulturePerTurn += GetJONSCulturePerTurnFromTraits();

	// Free culture that's part of the player
	iCulturePerTurn += GetJONSCulturePerTurnForFree();

	// Culture from Minor Civs
	iCulturePerTurn += GetCulturePerTurnFromMinorCivs();

	// Culture from Religion
	iCulturePerTurn += GetCulturePerTurnFromReligion();
	
	// Temporary boost from bonus turns
	iCulturePerTurn += GetCulturePerTurnFromBonusTurns();

	// Golden Age bonus
	if (isGoldenAge() && !IsGoldenAgeCultureBonusDisabled())
	{
#ifdef BRAZIL_UA_REWORK
		if (GetPlayerTraits()->GetGoldenAgeGreatArtistRateModifier() > 0)
		{
			iCulturePerTurn += ((iCulturePerTurn * (10 + GC.getGOLDEN_AGE_CULTURE_MODIFIER())) / 100);
		}
		else
		{
			iCulturePerTurn += ((iCulturePerTurn * GC.getGOLDEN_AGE_CULTURE_MODIFIER()) / 100);
		}
#else
		iCulturePerTurn += ((iCulturePerTurn * GC.getGOLDEN_AGE_CULTURE_MODIFIER()) / 100);
#endif
	}

	return iCulturePerTurn;
}

//	--------------------------------------------------------------------------------
/// Culture per turn from Cities
int CvPlayer::GetJONSCulturePerTurnFromCities() const
{
	int iCulturePerTurn = 0;

	// Add in culture from Cities
	const CvCity* pLoopCity;
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iCulturePerTurn += pLoopCity->getJONSCulturePerTurn();
	}

	return iCulturePerTurn;
}

//	--------------------------------------------------------------------------------
/// Special bonus which adds excess Happiness to Culture?
int CvPlayer::GetJONSCulturePerTurnFromExcessHappiness() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_HAPPINESS))
	{
		return 0;
	}

	if(getHappinessToCulture() != 0)
	{
		if(GetExcessHappiness() > 0)
		{
			int iFreeCulture = GetExcessHappiness() * getHappinessToCulture();
			iFreeCulture /= 100;

			return iFreeCulture;
		}
	}

	return 0;
}

//	--------------------------------------------------------------------------------
/// Trait bonus which adds Culture for trade partners? 
int CvPlayer::GetJONSCulturePerTurnFromTraits() const
{
#ifdef MOROCCO_UA_REWORK
	return GetPlayerTraits()->GetYieldChangePerTradePartner(YIELD_CULTURE) * GetTrade()->GetNumDifferentTradingPartners() * (GetCurrentEra() + 1);
#else
	return GetPlayerTraits()->GetYieldChangePerTradePartner(YIELD_CULTURE) * GetTrade()->GetNumDifferentTradingPartners();
#endif
}

//	--------------------------------------------------------------------------------
/// Culture per turn player starts with for free
int CvPlayer::GetJONSCulturePerTurnForFree() const
{
	return m_iJONSCulturePerTurnForFree;
}

//	--------------------------------------------------------------------------------
/// Culture per turn player starts with for free
void CvPlayer::ChangeJONSCulturePerTurnForFree(int iChange)
{
	if(iChange != 0)
		m_iJONSCulturePerTurnForFree += iChange;

	if(GC.getGame().getActivePlayer() == GetID())
	{
		GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
	}
}

//	--------------------------------------------------------------------------------
/// DEPRECATED, use GetCulturePerTurnFromMinorCivs() instead
int CvPlayer::GetJONSCulturePerTurnFromMinorCivs() const
{
	return GetCulturePerTurnFromMinorCivs();
}

//	--------------------------------------------------------------------------------
/// DEPRECATED, value is now changed within CvMinorCivAI
void CvPlayer::ChangeJONSCulturePerTurnFromMinorCivs(int /*iChange*/)
{
	CvAssertMsg(false, "ChangeJONSCulturePerTurnFromMinorCivs called, but Anton meant to disable it");
}

//	--------------------------------------------------------------------------------
/// Culture per turn from all minor civs
int CvPlayer::GetCulturePerTurnFromMinorCivs() const
{
	int iAmount = 0;
	PlayerTypes eMinor;
	for(int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
	{
		eMinor = (PlayerTypes) iMinorLoop;
		iAmount += GetCulturePerTurnFromMinor(eMinor);
	}

	return iAmount;
}

//	--------------------------------------------------------------------------------
// Culture per turn from a minor civ
int CvPlayer::GetCulturePerTurnFromMinor(PlayerTypes eMinor) const
{
	int iAmount = 0;

	if(GET_PLAYER(eMinor).isAlive())
	{
		// Includes flat bonus and any bonus from cultural buildings
		iAmount += GET_PLAYER(eMinor).GetMinorCivAI()->GetCurrentCultureBonus(GetID());
	}

	return iAmount;
}

//	--------------------------------------------------------------------------------
/// Culture per turn from religion
int CvPlayer::GetCulturePerTurnFromReligion() const
{
	int iOtherCulturePerTurn = 0;
	int iReligionCulturePerTurn = 0;

	// Start by seeing how much the other types are bringing in
	iOtherCulturePerTurn += GetJONSCulturePerTurnFromCities();
	iOtherCulturePerTurn += GetJONSCulturePerTurnFromExcessHappiness();
	iOtherCulturePerTurn += GetJONSCulturePerTurnForFree();
	iOtherCulturePerTurn += GetCulturePerTurnFromMinorCivs();

	// Founder beliefs
	CvGameReligions* pReligions = GC.getGame().GetGameReligions();
	ReligionTypes eFoundedReligion = pReligions->GetFounderBenefitsReligion(GetID());
	if(eFoundedReligion != NO_RELIGION)
	{
		const CvReligion* pReligion = pReligions->GetReligion(eFoundedReligion, NO_PLAYER);
		if(pReligion)
		{
			iReligionCulturePerTurn += pReligion->m_Beliefs.GetHolyCityYieldChange(YIELD_CULTURE);

			int iTemp = pReligion->m_Beliefs.GetYieldChangePerForeignCity(YIELD_CULTURE);
			if (iTemp > 0)
			{
				iReligionCulturePerTurn += (iTemp * GetReligions()->GetNumForeignCitiesFollowing());
			}

			iTemp = pReligion->m_Beliefs.GetYieldChangePerXForeignFollowers(YIELD_CULTURE);
			if (iTemp > 0)
			{
#ifdef BELIEF_WORLD_CHURCH_PER_FOLLOWERS
				int iFollowers = pReligions->GetNumFollowers(eFoundedReligion);
#else
				int iFollowers = GetReligions()->GetNumForeignFollowers(false /*bAtPeace*/);
#endif
				if (iFollowers > 0)
				{
					iReligionCulturePerTurn += (iFollowers / iTemp);
				}
			}

			bool bAtPeace = GET_TEAM(getTeam()).getAtWarCount(false) == 0;
			int iMod = pReligion->m_Beliefs.GetPlayerCultureModifier(bAtPeace);

			if (iMod != 0)
			{
				iReligionCulturePerTurn += ((iReligionCulturePerTurn + iOtherCulturePerTurn) * iMod) / 100;
			}
			return iReligionCulturePerTurn;
		}
	}

	return 0;
}

//	--------------------------------------------------------------------------------
/// Culture from Bonus Turns
int CvPlayer::GetCulturePerTurnFromBonusTurns() const
{
	int iValue = 0;

	if (GetCultureBonusTurns() > 0)
	{
		// Start by seeing how much the other types are bringing in
		int iOtherCulturePerTurn = 0;
		iOtherCulturePerTurn += GetJONSCulturePerTurnFromCities();
		iOtherCulturePerTurn += GetJONSCulturePerTurnFromExcessHappiness();
		iOtherCulturePerTurn += GetJONSCulturePerTurnForFree();
		iOtherCulturePerTurn += GetCulturePerTurnFromMinorCivs();
		iOtherCulturePerTurn += GetCulturePerTurnFromReligion();

		iValue += ((iOtherCulturePerTurn * GC.getTEMPORARY_CULTURE_BOOST_MOD()) / 100);
	}

	return iValue;
}

//	--------------------------------------------------------------------------------
/// Modifier for all Cities' culture
int CvPlayer::GetJONSCultureCityModifier() const
{
	return m_iJONSCultureCityModifier;
}

//	--------------------------------------------------------------------------------
/// Modifier for all Cities' culture
void CvPlayer::ChangeJONSCultureCityModifier(int iChange)
{
	if(iChange != 0)
	{
		m_iJONSCultureCityModifier += iChange;

		if(GC.getGame().getActivePlayer() == GetID())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getJONSCulture() const
{
	// City States can't pick Policies, sorry!
	if(isMinorCiv())
		return 0;

	if(GC.getGame().isOption(GAMEOPTION_NO_POLICIES))
	{
		return 0;
	}

	return m_iJONSCulture;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setJONSCulture(int iNewValue)
{
	if(getJONSCulture() != iNewValue)
	{
		// Add to the total we've ever had
		if(iNewValue > m_iJONSCulture)
		{
			ChangeJONSCultureEverGenerated(iNewValue - m_iJONSCulture);
		}

		m_iJONSCulture = iNewValue;

		if(GC.getGame().getActivePlayer() == GetID())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeJONSCulture(int iChange)
{
	setJONSCulture(getJONSCulture() + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::GetJONSCultureEverGenerated() const
{
	return m_iJONSCultureEverGenerated;
}


//	--------------------------------------------------------------------------------
void CvPlayer::SetJONSCultureEverGenerated(int iNewValue)
{
	if(GetJONSCultureEverGenerated() != iNewValue)
	{
		m_iJONSCultureEverGenerated = iNewValue;
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeJONSCultureEverGenerated(int iChange)
{
	SetJONSCultureEverGenerated(GetJONSCultureEverGenerated() + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::GetJONSCulturePerCityPerTurn() const
{
	int iCulture = GetJONSCultureEverGenerated();
	int iNumCities = getNumCities();

	// Puppet Cities don't count
	iNumCities -= GetNumPuppetCities();

	int iNumTurns = GC.getGame().getElapsedGameTurns();

	if(iNumTurns == 0)
	{
		iNumTurns = 1;
	}

	int iCulturePerCityPerTurn = 100 * iCulture / iNumCities / iNumTurns;
	return iCulturePerCityPerTurn;
}

//	--------------------------------------------------------------------------------
/// Amount of extra Culture per Wonder
int CvPlayer::GetCulturePerWonder() const
{
	return m_iCulturePerWonder;
}

//	--------------------------------------------------------------------------------
/// Changes amount of extra Culture per Wonder
void CvPlayer::ChangeCulturePerWonder(int iChange)
{
	if(iChange != 0)
	{
		m_iCulturePerWonder += iChange;

		int iTotalCultureChange;

		// Loop through all Cities and change how much Culture they produce based on how many Wonders they have
		CvCity* pLoopCity;
		int iLoop;
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iTotalCultureChange = pLoopCity->getNumWorldWonders() * iChange;
			pLoopCity->ChangeJONSCulturePerTurnFromPolicies(iTotalCultureChange);
		}
	}
}

//	--------------------------------------------------------------------------------
/// Culture multiplier for having a world wonder
int CvPlayer::GetCultureWonderMultiplier() const
{
	return m_iCultureWonderMultiplier;
}

//	--------------------------------------------------------------------------------
/// Changes amount of extra Culture per Wonder
void CvPlayer::ChangeCultureWonderMultiplier(int iChange)
{
	if(iChange != 0)
		m_iCultureWonderMultiplier += iChange;
}

//	--------------------------------------------------------------------------------
/// Amount of Culture provided for each Tech Researched
int CvPlayer::GetCulturePerTechResearched() const
{
	return m_iCulturePerTechResearched;
}

//	--------------------------------------------------------------------------------
/// Changes amount of Culture provided for each Tech Researched
void CvPlayer::ChangeCulturePerTechResearched(int iChange)
{
	if(iChange != 0)
	{
		m_iCulturePerTechResearched += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// Specialist Culture Modifier
int CvPlayer::GetSpecialistCultureChange() const
{
	return m_iSpecialistCultureChange;
}

//	--------------------------------------------------------------------------------
/// Specialist Culture Modifier
void CvPlayer::ChangeSpecialistCultureChange(int iChange)
{
	if(iChange != 0)
	{
		CvCity* pLoopCity;
		int iLoop;

		int iTotalCulture = 0;

		SpecialistTypes eSpecialist;
		int iSpecialistLoop;
		int iSpecialistCount;

		// Undo old culture
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			for(iSpecialistLoop = 0; iSpecialistLoop < GC.getNumSpecialistInfos(); iSpecialistLoop++)
			{
				eSpecialist = (SpecialistTypes) iSpecialistLoop;
				iSpecialistCount = pLoopCity->GetCityCitizens()->GetSpecialistCount(eSpecialist);
				iTotalCulture += (iSpecialistCount * pLoopCity->GetCultureFromSpecialist(eSpecialist));
			}

			pLoopCity->ChangeJONSCulturePerTurnFromSpecialists(-iTotalCulture);
		}

		// CHANGE VALUE
		m_iSpecialistCultureChange += iChange;

		iTotalCulture = 0;

		// Apply new culture
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			for(iSpecialistLoop = 0; iSpecialistLoop < GC.getNumSpecialistInfos(); iSpecialistLoop++)
			{
				eSpecialist = (SpecialistTypes) iSpecialistLoop;
				iSpecialistCount = pLoopCity->GetCityCitizens()->GetSpecialistCount(eSpecialist);
				iTotalCulture += (iSpecialistCount * pLoopCity->GetCultureFromSpecialist(eSpecialist));
			}

			pLoopCity->ChangeJONSCulturePerTurnFromSpecialists(iTotalCulture);
		}
	}
}

//	--------------------------------------------------------------------------------
/// What is the sum of culture yield from the previous N turns?
/// NOTE: This uses the data tracked in recording a replay, so if replays are disabled in the future then this must change!
int CvPlayer::GetCultureYieldFromPreviousTurns(int iGameTurn, int iNumPreviousTurnsToCount)
{
	// Culture per turn yield is tracked in replay data, so use that
	int iSum = 0;
	for (int iI = 0; iI < iNumPreviousTurnsToCount; iI++)
	{
		int iTurn = iGameTurn - iI;
		if (iTurn < 0)
		{
			break;
		}

		int iTurnCulture = getReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_CULTUREPERTURN"), iTurn);
		if (iTurnCulture >= 0)
		{
			iSum += iTurnCulture;
		}
		else if (iTurnCulture == -1) // No data for this turn (ex. late era start)
		{
			iSum += (3 * GetTotalJONSCulturePerTurn());
		}
	}

	return iSum;
}

#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
//	--------------------------------------------------------------------------------
int CvPlayer::getBuildingScecialistCountChange(BuildingTypes eIndex1, SpecialistTypes eIndex2) const
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumBuildInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_SPECILIST_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	if (eIndex2 == NO_SPECIALIST)
	{
		return 0;
	}
	else
	{
		return m_ppaaiBuildingScecialistCountChange[eIndex1][eIndex2];
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeBuildingScecialistCountChange(BuildingTypes eIndex1, SpecialistTypes eIndex2, int iChange)
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumBuildInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_SPECILIST_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		CvAssertMsg(iChange > -50 && iChange < 50, "GAMEPLAY: Yield for a plot is either negative or a ridiculously large number. Please send Jon this with your last 5 autosaves and what changelist # you're playing.");

		Firaxis::Array<int, NUM_SPECILIST_TYPES> specialists = m_ppaaiBuildingScecialistCountChange[eIndex1];
		specialists[eIndex2] = (m_ppaaiBuildingScecialistCountChange[eIndex1][eIndex2] + iChange);
		m_ppaaiBuildingScecialistCountChange.setAt(eIndex1, specialists);
		// CvAssert(getImprovementYieldChange(eIndex1, eIndex2) >= 0);

		// updateYield();
	}
}
#endif

//	--------------------------------------------------------------------------------
/// Cities remaining to get a free culture building
int CvPlayer::GetNumCitiesFreeCultureBuilding() const
{
	return m_iNumCitiesFreeCultureBuilding;
}

//	--------------------------------------------------------------------------------
/// Changes number of cities remaining to get a free culture building
void CvPlayer::ChangeNumCitiesFreeCultureBuilding(int iChange)
{
	if(iChange != 0)
		m_iNumCitiesFreeCultureBuilding += iChange;
}

//	--------------------------------------------------------------------------------
/// Cities remaining to get a free culture building
int CvPlayer::GetNumCitiesFreeFoodBuilding() const
{
	return m_iNumCitiesFreeFoodBuilding;
}

//	--------------------------------------------------------------------------------
/// Changes number of cities remaining to get a free culture building
void CvPlayer::ChangeNumCitiesFreeFoodBuilding(int iChange)
{
	if(iChange != 0)
		m_iNumCitiesFreeFoodBuilding += iChange;
}

#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
//	--------------------------------------------------------------------------------
int CvPlayer::GetNumCitiesFreeDefensiveBuilding() const
{
	return m_iNumCitiesFreeDevensiveBuilding;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeNumCitiesFreeDefensiveBuilding(int iChange)
{
	if (iChange != 0)
		m_iNumCitiesFreeDevensiveBuilding += iChange;
}
#endif

//	--------------------------------------------------------------------------------
/// Handle earning yields from a combat win
void CvPlayer::DoYieldsFromKill(UnitTypes eAttackingUnitType, UnitTypes eKilledUnitType, int iX, int iY, bool bWasBarbarian, int iExistingDelay)
{
	int iNumBonuses = iExistingDelay; // Passed by reference below, incremented to stagger floating text in UI
	DoUnresearchedTechBonusFromKill(eKilledUnitType, iX, iY, iNumBonuses);
	for(int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
	{
		DoYieldBonusFromKill((YieldTypes)iYield, eAttackingUnitType, eKilledUnitType, iX, iY, bWasBarbarian, iNumBonuses);
	}
}

//	--------------------------------------------------------------------------------
/// Apply and show a yield bonus from a combat win
/// If a bonus is applied, iNumBonuses must be incremented to stagger the UI text with other bonuses
void CvPlayer::DoYieldBonusFromKill(YieldTypes eYield, UnitTypes eAttackingUnitType, UnitTypes eKilledUnitType, int iX, int iY, bool bWasBarbarian, int &iNumBonuses)
{
#ifdef UPDATE_CULTURE_NOTIFICATION_DURING_TURN
	CvGame& kGame = GC.getGame();
#endif
	int iValue = 0;

	CvAssertMsg(eKilledUnitType != NO_UNIT, "Killed unit's type is NO_TYPE. Please send Anton your save file and version.");
	if (eKilledUnitType == NO_UNIT) return;

	CvUnitEntry* pkKilledUnitInfo = GC.getUnitInfo(eKilledUnitType);
	if(pkKilledUnitInfo)
	{
		int iCombatStrength = max(pkKilledUnitInfo->GetCombat(), pkKilledUnitInfo->GetRangedCombat());
		if(iCombatStrength > 0)
		{	
			switch (eYield)
			{
			case YIELD_FOOD:
			case YIELD_PRODUCTION:
				// Not supported, local to a city
				return;

			case YIELD_GOLD:
				iValue += GetPlayerPolicies()->GetNumericModifier(POLICYMOD_GOLD_FROM_KILLS);
				break;

			case YIELD_CULTURE:
#ifdef DUEL_HONOR_CHANGE
				if (GC.getGame().isOption("GAMEOPTION_DUEL_STUFF"))
				{
					iValue += GetPlayerTraits()->GetCultureFromKills();
					iValue += GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CULTURE_FROM_BARBARIAN_KILLS);

					// Do we get it for barbarians?
					if (bWasBarbarian)
					{
						iValue += GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CULTURE_FROM_KILLS);
					}
					break;
				}
				else
				{
					iValue += GetPlayerTraits()->GetCultureFromKills();
					iValue += GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CULTURE_FROM_KILLS);

					// Do we get it for barbarians?
					if (bWasBarbarian)
					{
						iValue += GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CULTURE_FROM_BARBARIAN_KILLS);
					}
					break;
				}
#else
				iValue += GetPlayerTraits()->GetCultureFromKills();
				iValue += GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CULTURE_FROM_KILLS);

				// Do we get it for barbarians?
				if(bWasBarbarian)
				{
					iValue += GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CULTURE_FROM_BARBARIAN_KILLS);
				}
				break;
#endif

			case YIELD_FAITH:
				iValue += GetPlayerTraits()->GetFaithFromKills();

				if (eYield == YIELD_FAITH && (GC.getGame().isOption(GAMEOPTION_NO_RELIGION)))
				{
					return;
				}
				break;
			case YIELD_SCIENCE:
				break;
			}

			iValue += GC.getGame().GetGameReligions()->GetBeliefYieldForKill(eYield, iX, iY, GetID());

			if(eAttackingUnitType != NO_UNIT)
			{
				CvUnitEntry* pkAttackingUnitInfo = GC.getUnitInfo(eAttackingUnitType);
				if(pkAttackingUnitInfo)
				{
					iValue += pkAttackingUnitInfo->GetYieldFromKills(eYield);
				}
			}

			iValue = (iValue * iCombatStrength) / 100;
			if(iValue > 0)
			{
				switch(eYield)
				{
				case YIELD_GOLD:
					GetTreasury()->ChangeGold(iValue);
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
					ChangeGoldFromKills(iValue);
#endif
					break;
				case YIELD_CULTURE:
#ifdef HONOR_CULTURE_CAP
					// if (!GetPlayerPolicies()->HasPolicy((PolicyTypes)GC.getInfoTypeForString("POLICY_HONOR_FINISHER")))
					{
						iValue = min(GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CULTURE_FROM_KILLS) * iCombatStrength, 100 * CULTURE_CAP) + GetPlayerTraits()->GetCultureFromKills() * iCombatStrength;
						iValue /= 100;
					}
					changeJONSCulture(iValue);
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
					ChangeCultureFromKills(iValue);
#endif
#else
					changeJONSCulture(iValue);
#endif
#ifdef UPDATE_CULTURE_NOTIFICATION_DURING_TURN
					// if this is the human player, have the popup come up so that he can choose a new policy
					if(isAlive() && isHuman() && getNumCities() > 0)
					{
						if(!GC.GetEngineUserInterface()->IsPolicyNotificationSeen())
						{
							if(getNextPolicyCost() <= getJONSCulture() && GetPlayerPolicies()->GetNumPoliciesCanBeAdopted() > 0)
							{
								CvNotifications* pNotifications = GetNotifications();
								if(pNotifications)
								{
									CvString strBuffer;

									if(kGame.isOption(GAMEOPTION_POLICY_SAVING))
										strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY_DISMISS");
									else
										strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY");

									CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ENOUGH_CULTURE_FOR_POLICY");
									pNotifications->Add(NOTIFICATION_POLICY, strBuffer, strSummary, -1, -1, -1);
								}
							}
						}
					}
#endif
					break;
				case YIELD_FAITH:
					ChangeFaith(iValue);
					break;
				case YIELD_SCIENCE:
					TechTypes eCurrentTech = GetPlayerTechs()->GetCurrentResearch();
					if(eCurrentTech == NO_TECH)
					{
						changeOverflowResearch(iValue);
					}
					else
					{
						GET_TEAM(getTeam()).GetTeamTechs()->ChangeResearchProgress(eCurrentTech, iValue, GetID());
					}
					break;
				}
				iNumBonuses++;
				ReportYieldFromKill(eYield, iValue, iX, iY, iNumBonuses);
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Apply and show a bonus towards unresearched tech when we defeat a unit of that tech
/// If a bonus is applied, iNumBonuses must be incremented to stagger the UI text with other bonuses
void CvPlayer::DoUnresearchedTechBonusFromKill(UnitTypes eKilledUnitType, int iX, int iY, int &iNumBonuses)
{
	CvAssertMsg(eKilledUnitType != NO_UNIT, "Killed unit's type is NO_TYPE. Please send Anton your save file and version.");
	if (eKilledUnitType == NO_UNIT) return;

	int iPercent = GetPlayerTraits()->GetUnresearchedTechBonusFromKills();

	if (iPercent > 0)
	{
		int iValue = 0;

		CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eKilledUnitType);
		if(pkUnitInfo)
		{
			TechTypes ePrereq = (TechTypes) pkUnitInfo->GetPrereqAndTech();
			if (ePrereq != NO_TECH)
			{
				CvTechEntry* pkTechInfo = GC.getTechInfo(ePrereq);
				if (pkTechInfo && !GET_TEAM(getTeam()).GetTeamTechs()->HasTech(ePrereq))
				{
					int iCombatStrength = max(pkUnitInfo->GetCombat(), pkUnitInfo->GetRangedCombat());
					if (iCombatStrength > 0)
					{
						int iTechCost = GetPlayerTechs()->GetResearchCost(ePrereq);
						iValue = (iTechCost * iPercent) / 100;

						// Cannot be greater than the tech's cost
						int iRemainingCost = iTechCost - GetPlayerTechs()->GetResearchProgress(ePrereq);
						if (iValue > iRemainingCost)
						{
							iValue = iRemainingCost;
						}

						if (iValue > 0)
						{
							GET_TEAM(getTeam()).GetTeamTechs()->ChangeResearchProgress(ePrereq, iValue, GetID());
							iNumBonuses++;
							ReportYieldFromKill(YIELD_SCIENCE, iValue, iX, iY, iNumBonuses);
						}
					}
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Show earning a yield from combat wins
void CvPlayer::ReportYieldFromKill(YieldTypes eYield, int iValue, int iX, int iY, int iDelay)
{
	CvString yieldString;
	if(iValue > 0)
	{
		switch(eYield)
		{
		case YIELD_GOLD:
			yieldString = "[COLOR_YELLOW]+%d[ENDCOLOR][ICON_GOLD]";
			break;
		case YIELD_CULTURE:
			yieldString = "[COLOR_MAGENTA]+%d[ENDCOLOR][ICON_CULTURE]";
			break;
		case YIELD_FAITH:
			yieldString = "[COLOR_WHITE]+%d[ENDCOLOR][ICON_PEACE]";
			break;
		case YIELD_SCIENCE:
			yieldString = "[COLOR_BLUE]+%d[ENDCOLOR][ICON_RESEARCH]";
			break;
		default:
			// Not supported
			return;
		}

		if(GetID() == GC.getGame().getActivePlayer())
		{
			char text[256] = {0};
			float fDelay = GC.getPOST_COMBAT_TEXT_DELAY() * (1 + ((float)iDelay * 0.5f)); // 1 is added to avoid overlapping with XP text
			sprintf_s(text, yieldString, iValue);
			GC.GetEngineUserInterface()->AddPopupText(iX, iY, text, fDelay);
		}
	}
}

#ifdef NQ_ALLOW_RELIGION_ONE_SHOTS 
void CvPlayer::DoReligionOneShots(ReligionTypes eReligion)
{
	bool setUnitReligion = false;
	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, GetID());

#ifdef BELIEF_TO_GLORY_OF_GOD_ONE_GP_OF_EACH_TYPE
	// const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, getOwner());
	if(pReligion)
	{	
		if (pReligion->m_Beliefs.IsFaithPurchaseAllGreatPeople())
		{
			setMerchantsFromFaith(true);
			setScientistsFromFaith(true);
			setWritersFromFaith(true);
			setArtistsFromFaith(true);
			setMusiciansFromFaith(true);
			setGeneralsFromFaith(true);
			setAdmiralsFromFaith(true);
			setEngineersFromFaith(true);
		}
	}
#endif

#ifdef NQ_FREE_SETTLERS_FROM_BELIEF
	if (!m_bHasUsedReligiousSettlements && pReligion->m_Beliefs.GetNumFreeSettlers() > 0)
	{
		m_bHasUsedReligiousSettlements = true;

		// add free settlers from Religious Settlements belief - I know this is super ugly, sorry :(
		// real solution is to make a Belief_FreeUnitClasses table and figure out how to check for each belief being triggered only once... :(
		// also should be regular settlers, not uniques (like American Pioneer for example)
		for (int iFreeSettlerLoop = 0; iFreeSettlerLoop < pReligion->m_Beliefs.GetNumFreeSettlers(); iFreeSettlerLoop++)
		{
			addFreeUnit((UnitTypes)GC.getInfoTypeForString("UNIT_SETTLER"));
		}
	}
#endif

#if defined UNITY_OF_PROPHETS_EXTRA_PROPHETS || defined GODDESS_LOVE_FREE_WORKER || defined GOD_SEA_FREE_WORK_BOAT
	BeliefTypes pBelief;
#endif
#ifdef UNITY_OF_PROPHETS_EXTRA_PROPHETS
	pBelief = NO_BELIEF;
	for(int iI = 0; iI < pReligion->m_Beliefs.GetNumBeliefs(); iI++)
	{
		const BeliefTypes eBelief = pReligion->m_Beliefs.GetBelief(iI);
		CvBeliefEntry* pEntry = GC.GetGameBeliefs()->GetEntry((int)eBelief);
		if(pEntry && pEntry->IsReformationBelief())
		{
			pBelief = eBelief;
			break;
		}
	}
	if (!m_bHasUsedUnityProphets && pBelief == (BeliefTypes)GC.getInfoTypeForString("BELIEF_UNITY_OF_PROPHETS", true))
	{
		m_bHasUsedUnityProphets = true;

		// add free settlers from Religious Settlements belief - I know this is super ugly, sorry :(
		// real solution is to make a Belief_FreeUnitClasses table and figure out how to check for each belief being triggered only once... :(
		// also should be regular settlers, not uniques (like American Pioneer for example)
		// for (int iFreeSettlerLoop = 0; iFreeSettlerLoop < pReligion->m_Beliefs.GetNumFreeSettlers(); iFreeSettlerLoop++)
		// {
			CvCity* pSpawnCity = getCapitalCity();
			pSpawnCity->GetCityCitizens()->DoSpawnGreatPerson((UnitTypes)GC.getInfoTypeForString("UNIT_PROPHET"), false /*bIncrementCount*/, false);
			pSpawnCity->GetCityCitizens()->DoSpawnGreatPerson((UnitTypes)GC.getInfoTypeForString("UNIT_PROPHET"), false /*bIncrementCount*/, false);
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
			ChangeNumProphetsTotal(2);
#endif
	}
#endif
#ifdef GODDESS_LOVE_FREE_WORKER
	pBelief = NO_BELIEF;
	for (int iI = 0; iI < pReligion->m_Beliefs.GetNumBeliefs(); iI++)
	{
		const BeliefTypes eBelief = pReligion->m_Beliefs.GetBelief(iI);
		CvBeliefEntry* pEntry = GC.GetGameBeliefs()->GetEntry((int)eBelief);
		if (pEntry && pEntry->IsPantheonBelief())
		{
			pBelief = eBelief;
			if (!m_bHasUsedGoddessLove && pBelief == (BeliefTypes)GC.getInfoTypeForString("BELIEF_GODDESS_LOVE", true))
			{
				m_bHasUsedGoddessLove = true;

#ifdef DUEL_GODDESS_LOVE_CHANGE
				if (!(GC.getGame().isNetworkMultiPlayer() && GC.getGame().isOption("GAMEOPTION_DUEL_STUFF")))
				{
					addFreeUnit((UnitTypes)GC.getInfoTypeForString("UNIT_WORKER"));
				}
#else
				addFreeUnit((UnitTypes)GC.getInfoTypeForString("UNIT_WORKER"));
#endif
		}
	}
	}
#endif
#ifdef GOD_SEA_FREE_WORK_BOAT
	pBelief = NO_BELIEF;
	for (int iI = 0; iI < pReligion->m_Beliefs.GetNumBeliefs(); iI++)
	{
		const BeliefTypes eBelief = pReligion->m_Beliefs.GetBelief(iI);
		CvBeliefEntry* pEntry = GC.GetGameBeliefs()->GetEntry((int)eBelief);
		if (pEntry && pEntry->IsPantheonBelief())
		{
			pBelief = eBelief;
			if (!m_bHasUsedGodSea && pBelief == (BeliefTypes)GC.getInfoTypeForString("BELIEF_GOD_SEA", true))
			{
				m_bHasUsedGodSea = true;

#ifdef DUEL_GOD_SEA_CHANGE
				if (!(GC.getGame().isNetworkMultiPlayer() && GC.getGame().isOption("GAMEOPTION_DUEL_STUFF")))
				{
					addFreeUnit((UnitTypes)GC.getInfoTypeForString("UNIT_WORK_BOAT"));
				}
#else
				addFreeUnit((UnitTypes)GC.getInfoTypeForString("UNIT_WORKBOAT"));
#endif
				}
		}
	}
#endif

#ifdef NQ_DEUS_VULT
	if (!m_bHasUsedDeusVult && pReligion->m_Beliefs.IsDeusVult())
	{
		m_bHasUsedDeusVult = true;
		setUnitReligion = true;

		// minimum mounted is chariot archer
		UnitTypes eBestMountedUnit = (UnitTypes)getCivilizationInfo().getCivilizationUnits((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_CHARIOT_ARCHER"));
		int iBestMountedScore = GC.getUnitInfo(eBestMountedUnit)->GetProductionCost();
		
		// minimum ranged is archer
		UnitTypes eBestRangedUnit = (UnitTypes)getCivilizationInfo().getCivilizationUnits((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_ARCHER"));
		int iBestRangedScore = GC.getUnitInfo(eBestRangedUnit)->GetProductionCost();

		// minimum siege is catapult
		UnitTypes eBestSiegeUnit = (UnitTypes)getCivilizationInfo().getCivilizationUnits((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_CATAPULT"));
		int iBestSiegeScore = GC.getUnitInfo(eBestSiegeUnit)->GetProductionCost();

		// minimum melee is warrior
		UnitTypes eBestMeleeUnit = (UnitTypes)getCivilizationInfo().getCivilizationUnits((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_WARRIOR"));
		int iBestMeleeScore = GC.getUnitInfo(eBestMeleeUnit)->GetProductionCost();

		for(int iUnitClassLoop = 0; iUnitClassLoop < GC.getNumUnitClassInfos(); iUnitClassLoop++)
		{
			bool bValid = false;
			CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo((UnitClassTypes)iUnitClassLoop);
			if(pkUnitClassInfo == NULL)
				continue;

			const UnitTypes eLoopUnit = ((UnitTypes)(getCivilizationInfo().getCivilizationUnits(iUnitClassLoop)));
			if(eLoopUnit != NO_UNIT)
			{
				CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eLoopUnit);
				if(pkUnitInfo == NULL)
					continue;

				CvUnitEntry& kUnit = *pkUnitInfo;

				// must be a combat unit and cannot be able to found cities
				if (kUnit.GetCombat() <= 0 || kUnit.IsFound() || kUnit.IsFoundAbroad())
					continue;
				
				// if we don't have the tech for this unit, ignore it
				if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)(kUnit.GetPrereqAndTech()))))
					continue;

				if ((UnitCombatTypes)pkUnitInfo->GetUnitCombatType() == (UnitCombatTypes)GC.getInfoTypeForString("UNITCOMBAT_MOUNTED") ||
					(UnitCombatTypes)pkUnitInfo->GetUnitCombatType() == (UnitCombatTypes)GC.getInfoTypeForString("UNITCOMBAT_MOUNTED_RANGED"))
				{
					if (pkUnitInfo->GetProductionCost() > iBestMountedScore)
					{
						iBestMountedScore = pkUnitInfo->GetProductionCost();
						eBestMountedUnit = eLoopUnit;
					}
				}
				if ((UnitCombatTypes)pkUnitInfo->GetUnitCombatType() == (UnitCombatTypes)GC.getInfoTypeForString("UNITCOMBAT_ARCHER"))
				{
					if (pkUnitInfo->GetProductionCost() > iBestRangedScore)
					{
						iBestRangedScore = pkUnitInfo->GetProductionCost();
						eBestRangedUnit = eLoopUnit;
					}
				}
				if ((UnitCombatTypes)pkUnitInfo->GetUnitCombatType() == (UnitCombatTypes)GC.getInfoTypeForString("UNITCOMBAT_SIEGE"))
				{
					if (pkUnitInfo->GetProductionCost() > iBestSiegeScore)
					{
						iBestSiegeScore = pkUnitInfo->GetProductionCost();
						eBestSiegeUnit = eLoopUnit;
					}
				}
				if ((UnitCombatTypes)pkUnitInfo->GetUnitCombatType() == (UnitCombatTypes)GC.getInfoTypeForString("UNITCOMBAT_MELEE"))
				{
					if (pkUnitInfo->GetProductionCost() > iBestMeleeScore)
					{
						iBestMeleeScore = pkUnitInfo->GetProductionCost();
						eBestMeleeUnit = eLoopUnit;
					}
				}
			}
		}

		// 2 inquisitors
		addFreeUnit((UnitTypes)getCivilizationInfo().getCivilizationUnits((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_INQUISITOR")));
		addFreeUnit((UnitTypes)getCivilizationInfo().getCivilizationUnits((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_INQUISITOR")));

		// 2 mounted
		if (eBestMountedUnit)
		{
			addFreeUnit(eBestMountedUnit);
			addFreeUnit(eBestMountedUnit);
		}

		// 2 ranged
		if (eBestRangedUnit)
		{
			addFreeUnit(eBestRangedUnit);
			addFreeUnit(eBestRangedUnit);
		}

		// 2 siege
		if (eBestSiegeUnit)
		{
			addFreeUnit(eBestSiegeUnit);
			addFreeUnit(eBestSiegeUnit);
		}
	}
#endif

#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	if (!m_bHasUsedDharma)
	{
		int iGoldenAgeTurns = pReligion->m_Beliefs.GetGoldenAgeTurns();
		if (iGoldenAgeTurns > 0)
		{
			m_bHasUsedDharma = true;
			int iLengthModifier = getGoldenAgeModifier() + GetPlayerTraits()->GetGoldenAgeDurationModifier(); // Player modifier & Trait modifier
			if (iLengthModifier > 0)
			{
				iGoldenAgeTurns = iGoldenAgeTurns * (100 + iLengthModifier) / 100;
			}
			iGoldenAgeTurns = iGoldenAgeTurns * GC.getGame().getGameSpeedInfo().getGoldenAgePercent() / 100; // Game Speed mod
			changeGoldenAgeTurns(iGoldenAgeTurns);
		}
	}
#endif

#ifdef UNDERGROUND_SECT_REWORK
	if (!m_bHasUsedUndergroundSect)
	{
		int iSpyPressure = pReligion->m_Beliefs.GetSpyPressure();
		if (iSpyPressure > 0)
		{
			m_bHasUsedUndergroundSect = true;
			CvPlayerEspionage* pEspionage = GetEspionage();
			CvAssertMsg(pEspionage, "pEspionage is null! What's up with that?!");
			if (pEspionage)
			{
				for (int i = 0; i < iSpyPressure; i++)
				{
					pEspionage->CreateSpy();
				}
			}
		}
	}
#endif

#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
	if (!m_bHasUsedMissionaryZeal)
	{
		if (pReligion->m_Beliefs.HasBelief((BeliefTypes)GC.getInfoTypeForString("BELIEF_MISSIONARY_ZEAL")))
		{
			int iLoop = 0;
			for (CvCity* pLoopCity = GET_PLAYER(GetID()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(GetID()).nextCity(&iLoop))
			{
				ReligionTypes eOldReligion = pLoopCity->GetCityReligions()->GetReligiousMajority();
				pLoopCity->GetCityReligions()->AdoptReligionFully(eReligion, eOldReligion);
			}
			m_bHasUsedMissionaryZeal = true;
		}
	}
#endif

	if (setUnitReligion)
	{
		// make sure free religious units are of this religion (if they haven't had one assigned already)
		int iLoopUnit;
		CvUnit* pLoopUnit;
		for(pLoopUnit = firstUnit(&iLoopUnit); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoopUnit))
		{
			if (pLoopUnit->getUnitInfo().IsSpreadReligion() || pLoopUnit->getUnitInfo().IsRemoveHeresy())
			{
				if (pLoopUnit->GetReligionData()->GetReligion() == NO_RELIGION)
				{
					pLoopUnit->GetReligionData()->SetReligion(eReligion);
					pLoopUnit->GetReligionData()->SetSpreadsLeft(pLoopUnit->getUnitInfo().GetReligionSpreads());
					pLoopUnit->GetReligionData()->SetReligiousStrength(pLoopUnit->getUnitInfo().GetReligiousStrength());
				}
			}
		}
	}
}
#endif

//	--------------------------------------------------------------------------------
/// Each a technology from conquering a city
void CvPlayer::DoTechFromCityConquer(CvCity* pConqueredCity)
{
	PlayerTypes eOpponent = pConqueredCity->getOwner();
	FStaticVector<TechTypes, 128, true, c_eCiv5GameplayDLL> vePossibleTechs;
	int iCheapestTechCost = MAX_INT;
	for (int i = 0; i < GC.getNumTechInfos(); i++)
	{
		TechTypes e = (TechTypes) i;
		CvTechEntry* pInfo = GC.getTechInfo(e);
		if (pInfo)
		{
			// They have it
#ifdef HAS_TECH_BY_HUMAN
			if (GET_TEAM(GET_PLAYER(eOpponent).getTeam()).GetTeamTechs()->HasTechByHuman(e))
#else
			if (GET_TEAM(GET_PLAYER(eOpponent).getTeam()).GetTeamTechs()->HasTech(e))
#endif
			{
				// We don't
				if (!GET_TEAM(getTeam()).GetTeamTechs()->HasTech(e))
				{
					// But we could
					if (GetPlayerTechs()->CanResearch(e))
					{
						if (pInfo->GetResearchCost() < iCheapestTechCost)
						{
							iCheapestTechCost = pInfo->GetResearchCost();
							vePossibleTechs.clear();
							vePossibleTechs.push_back(e);
						}
						else if (pInfo->GetResearchCost() == iCheapestTechCost)
						{
							vePossibleTechs.push_back(e);
						}
					}
				}
			}
		}
	}

	if (!vePossibleTechs.empty())
	{
		int iRoll = GC.getGame().getJonRandNum((int)vePossibleTechs.size(), "Rolling to choose free tech from conquering a city");
		TechTypes eFreeTech = vePossibleTechs[iRoll];
		CvAssert(eFreeTech != NO_TECH)
		if (eFreeTech != NO_TECH)
		{
			GET_TEAM(getTeam()).setHasTech(eFreeTech, true, GetID(), true, true);
			GET_TEAM(getTeam()).GetTeamTechs()->SetNoTradeTech(eFreeTech, true);
		}
	}
}

//	--------------------------------------------------------------------------------
/// Total faith per turn
int CvPlayer::GetTotalFaithPerTurn() const
{
	int iFaithPerTurn = 0;

	// If we're in anarchy, then no Faith is generated!
#ifdef PENALTY_FOR_DELAYING_POLICIES
	if (IsAnarchy() || IsDelayedPolicy())
#else
	if(IsAnarchy())
#endif
		return 0;

	// Faith per turn from Cities
	iFaithPerTurn += GetFaithPerTurnFromCities();

	// Faith per turn from Minor Civs
	iFaithPerTurn += GetFaithPerTurnFromMinorCivs();

	// Faith per turn from Religion (Founder beliefs)
	iFaithPerTurn += GetFaithPerTurnFromReligion();

	return iFaithPerTurn;
}

//	--------------------------------------------------------------------------------
/// Faith per turn from Cities
int CvPlayer::GetFaithPerTurnFromCities() const
{
	int iFaithPerTurn = 0;

	// Add in culture from Cities
	const CvCity* pLoopCity;
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iFaithPerTurn += pLoopCity->GetFaithPerTurn();
	}

	return iFaithPerTurn;
}

//	--------------------------------------------------------------------------------
/// Faith per turn from Minor Civs
int CvPlayer::GetFaithPerTurnFromMinorCivs() const
{
	int iFaithPerTurn = 0;
	for(int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
	{
		iFaithPerTurn += GetFaithPerTurnFromMinor((PlayerTypes)iMinorLoop);
	}
	return iFaithPerTurn;
}

//	--------------------------------------------------------------------------------
/// Faith per turn from a Minor Civ
int CvPlayer::GetFaithPerTurnFromMinor(PlayerTypes eMinor) const
{
	int iFaithPerTurn = 0;

	if(GET_PLAYER(eMinor).isAlive())
	{
		iFaithPerTurn += GET_PLAYER(eMinor).GetMinorCivAI()->GetCurrentFaithBonus(GetID());
	}

	return iFaithPerTurn;
}

//	--------------------------------------------------------------------------------
/// Faith per turn from Religion
int CvPlayer::GetFaithPerTurnFromReligion() const
{
	int iFaithPerTurn = 0;

	// Founder beliefs
	CvGameReligions* pReligions = GC.getGame().GetGameReligions();
	ReligionTypes eFoundedReligion = pReligions->GetFounderBenefitsReligion(GetID());
	if(eFoundedReligion != NO_RELIGION)
	{
		const CvReligion* pReligion = pReligions->GetReligion(eFoundedReligion, NO_PLAYER);
		if(pReligion)
		{
			iFaithPerTurn += pReligion->m_Beliefs.GetHolyCityYieldChange(YIELD_FAITH);

			int iTemp = pReligion->m_Beliefs.GetYieldChangePerForeignCity(YIELD_FAITH);
			if (iTemp > 0)
			{
#ifdef BELIEF_PILGRIMAGE_PER_CITY
				iFaithPerTurn += (iTemp * pReligions->GetNumCitiesFollowing(eFoundedReligion));
#else
				iFaithPerTurn += (iTemp * GetReligions()->GetNumForeignCitiesFollowing());
#endif
			}

			iTemp = pReligion->m_Beliefs.GetYieldChangePerXForeignFollowers(YIELD_FAITH);
			if (iTemp > 0)
			{
				int iFollowers = GetReligions()->GetNumForeignFollowers(false /*bAtPeace*/);
				if (iFollowers > 0)
				{
					iFaithPerTurn += (iTemp / iFollowers);
				}
			}
		}
	}

	return iFaithPerTurn;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetFaith() const
{
	return m_iFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetFaith(int iNewValue)
{
	if(GC.getGame().isOption(GAMEOPTION_NO_RELIGION))
	{
		return;
	}

	if(GetFaith() != iNewValue)
	{
		// Add to the total we've ever had
		if(iNewValue > m_iFaith)
		{
			ChangeFaithEverGenerated(iNewValue - m_iFaith);
		}

		m_iFaith = iNewValue;

		if(GC.getGame().getActivePlayer() == GetID())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeFaith(int iChange)
{
	if(GC.getGame().isOption(GAMEOPTION_NO_RELIGION))
	{
		return;
	}

	SetFaith(GetFaith() + iChange);
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetFaithEverGenerated() const
{
	return m_iFaithEverGenerated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetFaithEverGenerated(int iNewValue)
{
	if(m_iFaithEverGenerated != iNewValue)
		m_iFaithEverGenerated = iNewValue;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeFaithEverGenerated(int iChange)
{
	SetFaithEverGenerated(GetFaithEverGenerated() + iChange);
}

//	--------------------------------------------------------------------------------
/// Updates how much Happiness we have
void CvPlayer::DoUpdateHappiness()
{
	// Start level
	m_iHappiness = getHandicapInfo().getHappinessDefault();

#ifdef INDIA_HAPPINESS_BONUS
	// India bonus
	if(strcmp (getCivilizationTypeKey(), "CIVILIZATION_INDIA") == 0)
		m_iHappiness += 2;
	
#endif

	// Increase from Luxury Resources
	int iNumHappinessFromResources = GetHappinessFromResources();
	m_iHappiness += iNumHappinessFromResources;

	// Increase from Local City Happiness
	m_iHappiness += GetHappinessFromCities();

	// Increase from buildings
	m_iHappiness += GetHappinessFromBuildings();

	// Increase from policies
	m_iHappiness += GetHappinessFromPolicies();

	// Increase from num cities (player based, for buildings and such)
	m_iHappiness += getNumCities() * m_iHappinessPerCity;

	// Increase from Religion
	m_iHappiness += GetHappinessFromReligion();

	// Increase from Natural Wonders
	m_iHappiness += GetHappinessFromNaturalWonders();

	// Friendship with Minors can provide Happiness
	m_iHappiness += GetHappinessFromMinorCivs();

	// Increase from Leagues
	m_iHappiness += GetHappinessFromLeagues();

	// Increase for each City connected to Capital with a Trade Route
	DoUpdateCityConnectionHappiness();
	m_iHappiness += GetHappinessFromTradeRoutes();

#ifdef FUTURE_TECH_RESEARCHING_BONUSES
	m_iHappiness += 5 * GET_TEAM(getTeam()).GetTeamTechs()->GetTechCount((TechTypes)GC.getInfoTypeForString("TECH_FUTURE_TECH", true));
#endif

	if(isLocalPlayer() && GetExcessHappiness() >= 100)
	{
		gDLL->UnlockAchievement(ACHIEVEMENT_XP2_45);
	}

	GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
}

//	--------------------------------------------------------------------------------
/// How much Happiness we have
int CvPlayer::GetHappiness() const
{
	return m_iHappiness;
}

//	--------------------------------------------------------------------------------
/// Sets how much Happiness we have
void CvPlayer::SetHappiness(int iNewValue)
{
	if(GetHappiness() != iNewValue)
	{
		m_iHappiness = iNewValue;
	}
}

//	--------------------------------------------------------------------------------
/// How much over our Happiness limit are we?
int CvPlayer::GetExcessHappiness() const
{
	return GetHappiness() - GetUnhappiness();
}

//	--------------------------------------------------------------------------------
/// Has the player passed the Happiness limit?
bool CvPlayer::IsEmpireUnhappy() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_HAPPINESS))
	{
		return false;
	}

	if(GetExcessHappiness() < 0)
	{
		return true;
	}

	return false;
}

//	--------------------------------------------------------------------------------
/// Is the empire REALLY unhappy? (other penalties)
bool CvPlayer::IsEmpireVeryUnhappy() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_HAPPINESS))
	{
		return false;
	}

	if(GetExcessHappiness() <= /*-10*/ GC.getVERY_UNHAPPY_THRESHOLD())
	{
		return true;
	}

	return false;
}

//	--------------------------------------------------------------------------------
/// Is the empire SUPER unhappy? (leads to revolts)
bool CvPlayer::IsEmpireSuperUnhappy() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_HAPPINESS))
	{
		return false;
	}

	if(GetExcessHappiness() <= /*-20*/ GC.getSUPER_UNHAPPY_THRESHOLD())
	{
		return true;
	}

	return false;
}

//	--------------------------------------------------------------------------------
/// Uprisings pop up if the empire is Very Unhappy
void CvPlayer::DoUpdateUprisings()
{
	if(IsEmpireVeryUnhappy())
	{
		// If we're very unhappy, make the counter wind down
		if(GetUprisingCounter() > 0)
		{
			ChangeUprisingCounter(-1);

			// Time's up!
			if(GetUprisingCounter() == 0)
			{
				DoUprising();
				DoResetUprisingCounter(/*bFirstTime*/ false);
			}
		}
		// Very Unhappy for the first time - seed the counter
		else
		{
			DoResetUprisingCounter(/*bFirstTime*/ true);
		}
	}
}

//	--------------------------------------------------------------------------------
/// Uprising countdown - get
int CvPlayer::GetUprisingCounter() const
{
	return m_iUprisingCounter;
}

//	--------------------------------------------------------------------------------
/// Uprising countdown - set
void CvPlayer::SetUprisingCounter(int iValue)
{
	m_iUprisingCounter = iValue;
}

//	--------------------------------------------------------------------------------
/// Uprising countdown - change
void CvPlayer::ChangeUprisingCounter(int iChange)
{
	SetUprisingCounter(GetUprisingCounter() + iChange);
}

//	--------------------------------------------------------------------------------
/// Uprising countdown - seed
void CvPlayer::DoResetUprisingCounter(bool bFirstTime)
{
	int iTurns = /*4*/ GC.getUPRISING_COUNTER_MIN();
	CvGame& theGame = GC.getGame();
	int iExtra = theGame.getJonRandNum(/*3*/ GC.getUPRISING_COUNTER_POSSIBLE(), "Uprising counter rand");
	iTurns += iExtra;

	// Game speed mod
	int iMod = theGame.getGameSpeedInfo().getTrainPercent();
	// Only LENGTHEN time between rebels
	if(iMod > 100)
	{
		iTurns *= iMod;
		iTurns /= 100;
	}

	if(bFirstTime)
		iTurns /= 2;

	if(iTurns <= 0)
		iTurns = 1;

	SetUprisingCounter(iTurns);
}

//	--------------------------------------------------------------------------------
// Fire off an uprising somewhere
void CvPlayer::DoUprising()
{
	// In hundreds
	int iNumRebels = /*100*/ GC.getUPRISING_NUM_BASE();
	int iExtraRoll = (getNumCities() - 1) * /*20*/ GC.getUPRISING_NUM_CITY_COUNT();
	iExtraRoll += 100;
	iNumRebels += GC.getGame().getJonRandNum(iExtraRoll, "Rebel count rand roll");
	iNumRebels /= 100;

	// Find a random city to pop up a bad man
	CvCity* pBestCity = NULL;
	int iBestWeight = 0;

	int iTempWeight;

	CvCity* pLoopCity;
	int iLoop;
	CvGame& theGame = GC.getGame();
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iTempWeight = pLoopCity->getPopulation();
		iTempWeight += theGame.getJonRandNum(10, "Uprising rand weight.");

		if(iTempWeight > iBestWeight)
		{
			iBestWeight = iTempWeight;
			pBestCity = pLoopCity;
		}
	}

	// Found a place to set up an uprising?
	if(pBestCity != NULL)
	{
		int iBestPlot = -1;
		int iBestPlotWeight = -1;
		CvPlot* pPlot;

		CvCityCitizens* pCitizens = pBestCity->GetCityCitizens();

		// Start at 1, since ID 0 is the city plot itself
		for(int iPlotLoop = 1; iPlotLoop < NUM_CITY_PLOTS; iPlotLoop++)
		{
			pPlot = pCitizens->GetCityPlotFromIndex(iPlotLoop);

			if(!pPlot)		// Should be valid, but make sure
				continue;

			// Can't be impassable
			if(pPlot->isImpassable() || pPlot->isMountain())
				continue;

			// Can't be water
			if(pPlot->isWater())
				continue;

			// Can't be ANOTHER city
			if(pPlot->isCity())
				continue;

			// Don't place on a plot where a unit is already standing
			if(pPlot->getNumUnits() > 0)
				continue;

			iTempWeight = theGame.getJonRandNum(10, "Uprising rand plot location.");

			// Add weight if there's an improvement here!
			if(pPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				iTempWeight += 4;

				// If also a a resource, even more weight!
				if(pPlot->getResourceType(getTeam()) != NO_RESOURCE)
					iTempWeight += 3;
			}

			// Add weight if there's a defensive bonus for this plot
			if(pPlot->defenseModifier(BARBARIAN_TEAM, false, false))
				iTempWeight += 4;

			// Don't pick plots that aren't ours
			if(pPlot->getOwner() != GetID())
				iTempWeight = -1;

			if(iTempWeight > iBestPlotWeight)
			{
				iBestPlotWeight = iTempWeight;
				iBestPlot = iPlotLoop;
			}
		}

		// Found valid plot
		if(iBestPlot != -1)
		{
			// Make barbs able to enter ANYONE'S territory
			theGame.SetBarbarianReleaseTurn(0);

			pPlot = pCitizens->GetCityPlotFromIndex(iBestPlot);

			// Pick a unit type
			UnitTypes eUnit = theGame.GetRandomSpawnUnitType(GetID(), /*bIncludeUUs*/ false, /*bIncludeRanged*/ false);

			CvNotifications* pNotifications = GetNotifications();
			if(pNotifications)
			{
				Localization::String strMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_REBELS");
				Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_REBELS_SUMMARY");
				pNotifications->Add(NOTIFICATION_REBELS, strMessage.toUTF8(), strSummary.toUTF8(), pPlot->getX(), pPlot->getY(), eUnit, BARBARIAN_PLAYER);
			}

			// Init unit
			GET_PLAYER(BARBARIAN_PLAYER).initUnit(eUnit, pPlot->getX(), pPlot->getY());
			iNumRebels--;	// Reduce the count since we just added the seed rebel

			// Loop until all rebels are placed
			do
			{
				iNumRebels--;

				// Init unit
				CvUnit* pUnit = GET_PLAYER(BARBARIAN_PLAYER).initUnit(eUnit, pPlot->getX(), pPlot->getY());
				CvAssert(pUnit);
				if (pUnit)
				{
					if (!pUnit->jumpToNearestValidPlotWithinRange(5))
						pUnit->kill(false);		// Could not find a spot!
				}
			}
			while(iNumRebels > 0);
		}
	}
}

//	--------------------------------------------------------------------------------
/// City can revolt if the empire is Super Unhappy
void CvPlayer::DoUpdateCityRevolts()
{
	if(IsEmpireSuperUnhappy() && GetCulture()->GetPublicOpinionUnhappiness() > 0)
	{
		if(GetCityRevoltCounter() > 0)
		{
			ChangeCityRevoltCounter(-1);

			// Time's up!
			if(GetCityRevoltCounter() == 0)
			{
				DoCityRevolt();
				SetCityRevoltCounter(0);
			}
		}
		// Super Unhappy for the first time - seed the counter
		else
		{
			DoResetCityRevoltCounter();
		}
	}
}

//	--------------------------------------------------------------------------------
/// City revolt countdown - get
int CvPlayer::GetCityRevoltCounter() const
{
	return m_iCityRevoltCounter;
}

//	--------------------------------------------------------------------------------
/// City revolt countdown - set
void CvPlayer::SetCityRevoltCounter(int iValue)
{
	m_iCityRevoltCounter = iValue;
}

//	--------------------------------------------------------------------------------
/// City revolt countdown - change
void CvPlayer::ChangeCityRevoltCounter(int iChange)
{
	SetCityRevoltCounter(GetCityRevoltCounter() + iChange);
}

//	--------------------------------------------------------------------------------
/// City revolt countdown - seed
void CvPlayer::DoResetCityRevoltCounter()
{
	int iTurns = /*5*/ GC.getREVOLT_COUNTER_MIN();
	CvGame& theGame = GC.getGame();

	// Game speed mod
	int iMod = theGame.getGameSpeedInfo().getTrainPercent();
	// Only LENGTHEN time between rebels
	if(iMod > 100)
	{
		iTurns *= iMod;
		iTurns /= 100;
	}

	if(iTurns <= 0)
		iTurns = 1;

	CvCity *pMostUnhappyCity = GetMostUnhappyCity();
	PlayerTypes eRecipient = GetMostUnhappyCityRecipient();
	if(pMostUnhappyCity && eRecipient != NO_PLAYER)
	{
		SetCityRevoltCounter(iTurns);

		CvNotifications* pNotifications = GetNotifications();
		if(pNotifications && isHuman())
		{
			Localization::String strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_POSSIBLE_CITY_REVOLT", iTurns, pMostUnhappyCity->getName(), GET_PLAYER(eRecipient).getCivilizationShortDescription());
			Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_POSSIBLE_CITY_REVOLT_SUMMARY");
			pNotifications->Add(NOTIFICATION_CITY_REVOLT_POSSIBLE, strMessage.toUTF8(), strSummary.toUTF8(), pMostUnhappyCity->getX(), pMostUnhappyCity->getY(), -1);
		}
	}
}

//	--------------------------------------------------------------------------------
// Fire off a city revolt somewhere
void CvPlayer::DoCityRevolt()
{
	CvCity *pMostUnhappyCity = GetMostUnhappyCity();
	PlayerTypes eRecipient = GetMostUnhappyCityRecipient();
	if(pMostUnhappyCity && eRecipient != NO_PLAYER)
	{
		CvPlayer &kRecipient = GET_PLAYER(eRecipient);
		for(int iNotifyLoop = 0; iNotifyLoop < MAX_MAJOR_CIVS; ++iNotifyLoop){
			PlayerTypes eNotifyPlayer = (PlayerTypes) iNotifyLoop;
			CvPlayerAI& kCurNotifyPlayer = GET_PLAYER(eNotifyPlayer);
			CvNotifications* pNotifications = kCurNotifyPlayer.GetNotifications();
			if(pNotifications)
			{
				Localization::String strMessage;
				if (eNotifyPlayer == GetID())
				{
					strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_CITY_REVOLT", pMostUnhappyCity->getName(), kRecipient.getCivilizationShortDescription());
				}
				else
				{
					strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_OTHER_PLAYER_CITY_REVOLT", getCivilizationAdjective(), pMostUnhappyCity->getName(), kRecipient.getCivilizationShortDescription());
				}
				Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_REVOLT_SUMMARY");
				pNotifications->Add(NOTIFICATION_CITY_REVOLT, strMessage.toUTF8(), strSummary.toUTF8(), pMostUnhappyCity->getX(), pMostUnhappyCity->getY(), -1);
			}
		}

		kRecipient.acquireCity(pMostUnhappyCity, false/*bConquest*/, false/*bGift*/);

		// Move Units from player that don't belong here
		CvPlot *pPlot = pMostUnhappyCity->plot();
		if(pPlot->getNumUnits() > 0)
		{
			// Get the current list of units because we will possibly be moving them out of the plot's list
			IDInfoVector currentUnits;
			if (pPlot->getUnits(&currentUnits) > 0)
			{
				for(IDInfoVector::const_iterator itr = currentUnits.begin(); itr != currentUnits.end(); ++itr)
				{
					CvUnit* pLoopUnit = (CvUnit*)GetPlayerUnit(*itr);

					if(pLoopUnit && pLoopUnit->getOwner() == GetID())
					{
						pLoopUnit->finishMoves();
						if (!pLoopUnit->jumpToNearestValidPlot())
							pLoopUnit->kill(false);
					}
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
// Calculate city that will want to revolt
CvCity *CvPlayer::GetMostUnhappyCity()
{
	CvCity *pRtnValue = NULL;
	int iHighestUnhappiness = -1;

	if (getNumCities() > 1)
	{
		PolicyBranchTypes ePreferredIdeology = GetCulture()->GetPublicOpinionPreferredIdeology();

		int iLoop;
		CvCity* pLoopCity;
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if (pLoopCity->isCapital())
			{
				continue;
			}

			// Start with population
			int iUnhappiness = pLoopCity->getPopulation();

			// Subtract off local unhappiness
			iUnhappiness -= pLoopCity->GetLocalHappiness(); 

			// Look at each civ
			for (int iLoopPlayer = 0; iLoopPlayer < MAX_MAJOR_CIVS; iLoopPlayer++)
			{
				CvPlayer &kPlayer = GET_PLAYER((PlayerTypes)iLoopPlayer);
				if (iLoopPlayer != GetID() && kPlayer.isAlive() && !kPlayer.isMinorCiv())
				{
					PolicyBranchTypes eOtherCivIdeology = kPlayer.GetPlayerPolicies()->GetLateGamePolicyTree();
					if (eOtherCivIdeology == ePreferredIdeology)
					{
						int iCulturalDominanceOverUs = kPlayer.GetCulture()->GetInfluenceLevel(GetID()) - GetCulture()->GetInfluenceLevel((PlayerTypes)iLoopPlayer);
						if (iCulturalDominanceOverUs > 0 && kPlayer.getCapitalCity() != NULL)
						{
							// Find how far their capital is from this city
							int iCapitalDistance = plotDistance(pLoopCity->getX(), pLoopCity->getY(), kPlayer.getCapitalCity()->getX(), kPlayer.getCapitalCity()->getY());
							if (iCapitalDistance < 100)
							{
								int iDistanceFactor = 100 - iCapitalDistance;
								iDistanceFactor = (int)sqrt((float)iDistanceFactor);
								iUnhappiness += (iDistanceFactor * iCulturalDominanceOverUs);
							}
						}
					}
				}
			}

			if (iUnhappiness > iHighestUnhappiness)
			{
				iHighestUnhappiness = iUnhappiness;
				pRtnValue = pLoopCity;
			}
		}
	}

	return pRtnValue;
}

// Calculate player that will receive city if it revolts
PlayerTypes CvPlayer::GetMostUnhappyCityRecipient()
{
	PlayerTypes eRtnValue = NO_PLAYER;
	CvCity *pMostUnhappyCity = GetMostUnhappyCity();
	int iClosestCapital = MAX_INT;

	if (pMostUnhappyCity)
	{
		PolicyBranchTypes ePreferredIdeology = GetCulture()->GetPublicOpinionPreferredIdeology();

		// Look at each civ
		for (int iLoopPlayer = 0; iLoopPlayer < MAX_MAJOR_CIVS; iLoopPlayer++)
		{
			CvPlayer &kPlayer = GET_PLAYER((PlayerTypes)iLoopPlayer);
			if (iLoopPlayer != GetID() && kPlayer.isAlive() && !kPlayer.isMinorCiv())
			{
				PolicyBranchTypes eOtherCivIdeology = kPlayer.GetPlayerPolicies()->GetLateGamePolicyTree();
				if (eOtherCivIdeology == ePreferredIdeology)
				{
					int iCulturalDominanceOverUs = kPlayer.GetCulture()->GetInfluenceLevel(GetID()) - GetCulture()->GetInfluenceLevel((PlayerTypes)iLoopPlayer);
					if (iCulturalDominanceOverUs > 0 && kPlayer.getCapitalCity() != NULL)
					{
						// Find how far their capital is from this city
						int iCapitalDistance = plotDistance(pMostUnhappyCity->getX(), pMostUnhappyCity->getY(), kPlayer.getCapitalCity()->getX(), kPlayer.getCapitalCity()->getY());

						if (iCapitalDistance < iClosestCapital)
						{
							iClosestCapital = iCapitalDistance;
							eRtnValue = (PlayerTypes)iLoopPlayer;
						}
					}
				}
			}
		}
	}

	return eRtnValue;
}

//	--------------------------------------------------------------------------------
/// Returns the amount of Happiness being added by Policies
int CvPlayer::GetHappinessFromPolicies() const
{
	int iHappiness = m_pPlayerPolicies->GetNumericModifier(POLICYMOD_EXTRA_HAPPINESS);
	iHappiness += (getNumCities() * m_pPlayerPolicies->GetNumericModifier(POLICYMOD_EXTRA_HAPPINESS_PER_CITY));

	int iHappinessPerXPopulation;
	iHappinessPerXPopulation = GetHappinessPerXPopulation();

	if(iHappinessPerXPopulation > 0)
	{
		const CvCity* pLoopCity;
		int iLoop;
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if(pLoopCity && !pLoopCity->IsPuppet())
			{
				int iExtraHappiness = pLoopCity->getPopulation() / iHappinessPerXPopulation;

				iHappiness += iExtraHappiness;
			}
		}
	}

#ifdef POLICY_GREAT_WORK_HAPPINESS
	iHappiness += GetGreatWorkHappiness() * GetCulture()->GetNumGreatWorks();
#endif
#ifdef POLICY_HAPPINESS_PER_TRADE_ROUTE_TO_CAP
	const CvCity* pCapitalCity = getCapitalCity();
	if (pCapitalCity)
	{
		int iCount = 0;
		for (uint ui = 0; ui < GC.getGame().GetGameTrade()->m_aTradeConnections.size(); ui++)
		{
			if (GC.getGame().GetGameTrade()->IsTradeRouteIndexEmpty(ui))
			{
				continue;
			}

			CvCity* pDestCity = CvGameTrade::GetDestCity(GC.getGame().GetGameTrade()->m_aTradeConnections[ui]);
			if (pDestCity == pCapitalCity)
			{
				iCount++;
			}
		}

		iHappiness += (iCount * GetPlayerPolicies()->GetNumericModifier(POLICYMOD_HAPPINESS_PER_TRADE_ROUTE_TO_CAP));
	}
#endif
#ifdef POLICY_HAPPINESS_PER_TRADE_ROUTE_TO_MINOR
	int iCount = 0;
	for (uint ui = 0; ui < GC.getGame().GetGameTrade()->m_aTradeConnections.size(); ui++)
	{
		if (GC.getGame().GetGameTrade()->IsTradeRouteIndexEmpty(ui))
		{
			continue;
		}

		CvCity* pOriginCity = CvGameTrade::GetOriginCity(GC.getGame().GetGameTrade()->m_aTradeConnections[ui]);
		CvCity* pDestCity = CvGameTrade::GetDestCity(GC.getGame().GetGameTrade()->m_aTradeConnections[ui]);
		if (pOriginCity->getOwner() == GetID() && GET_PLAYER(pDestCity->getOwner()).isMinorCiv())
		{
			iCount++;
		}
	}

	iHappiness += (iCount * GetPlayerPolicies()->GetNumericModifier(POLICYMOD_HAPPINESS_PER_TRADE_ROUTE_TO_MINOR));
#endif

	return iHappiness;
}

//	--------------------------------------------------------------------------------
/// Returns the amount of Local Happiness generated in the cities
int CvPlayer::GetHappinessFromCities() const
{
	int iHappiness = 0;

	const CvCity* pLoopCity;
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iHappiness += pLoopCity->GetLocalHappiness();
	}

	return iHappiness;
}

//	--------------------------------------------------------------------------------
/// Returns the amount of Global Happiness being added by Buildings
int CvPlayer::GetHappinessFromBuildings() const
{
	int iHappiness = 0;
	BuildingClassTypes eBuildingClass;

	// Building Class Mods
	int iSpecialBuildingHappiness = 0;
	for(int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		eBuildingClass = (BuildingClassTypes) iI;

		CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
		if(!pkBuildingClassInfo)
		{
			continue;
		}

		BuildingTypes eBuilding = (BuildingTypes) getCivilizationInfo().getCivilizationBuildings(eBuildingClass);
		if(eBuilding != NO_BUILDING && countNumBuildings(eBuilding) > 0)
		{
			CvBuildingEntry* pkBuilding = GC.getBuildingInfo(eBuilding);
			if(pkBuilding)
			{
				for(int jJ = 0; jJ < GC.getNumBuildingClassInfos(); jJ++)
				{
					BuildingClassTypes eBuildingClassThatGivesHappiness = (BuildingClassTypes) jJ;
					int iHappinessPerBuilding = pkBuilding->GetBuildingClassHappiness(eBuildingClassThatGivesHappiness);
					if(iHappinessPerBuilding > 0)
					{
						BuildingTypes eBuildingThatGivesHappiness = (BuildingTypes) getCivilizationInfo().getCivilizationBuildings(eBuildingClassThatGivesHappiness);
						if(eBuildingThatGivesHappiness != NO_BUILDING)
						{
							iSpecialBuildingHappiness += iHappinessPerBuilding * countNumBuildings(eBuildingThatGivesHappiness);
						}
					}
				}
			}
		}
	}
	iHappiness += iSpecialBuildingHappiness;

#ifdef TRAIT_GET_BUILDING_CLASS_HAPPINESS
	// Trait Building Mods
	int iSpecialTraitBuildingHappiness = 0;
	for(int iTraitLoop = 0; iTraitLoop < GC.getNumTraitInfos(); iTraitLoop++)
	{
		TraitTypes eTrait = (TraitTypes)iTraitLoop;
		CvTraitEntry* pkTraitInfo = GC.getTraitInfo(eTrait);
		if(pkTraitInfo)
		{
			if(GetPlayerTraits()->HasTrait(eTrait))
			{
				for(int iBuildingClassLoop = 0; iBuildingClassLoop < GC.getNumBuildingClassInfos(); iBuildingClassLoop++)
				{
					BuildingClassTypes eBuildingClassThatGivesHappiness = (BuildingClassTypes) iBuildingClassLoop;
					int iHappinessPerBuilding = pkTraitInfo->GetBuildingClassHappiness(eBuildingClassThatGivesHappiness);
					if(iHappinessPerBuilding > 0)
					{
						BuildingTypes eBuildingThatGivesHappiness = (BuildingTypes) getCivilizationInfo().getCivilizationBuildings(eBuildingClassThatGivesHappiness);
						if(eBuildingThatGivesHappiness != NO_BUILDING)
						{
							iSpecialTraitBuildingHappiness += iHappinessPerBuilding * countNumBuildings(eBuildingThatGivesHappiness);
						}
					}
				}
			}
		}
	}

	iHappiness += iSpecialTraitBuildingHappiness;
#endif

	const CvCity* pLoopCity;
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iHappiness += pLoopCity->GetHappinessFromBuildings();
	}

	// Increase from num policies -- MOVE THIS CODE (and provide a new tool tip string) if we ever get happiness per X policies to something beside a building
	if(m_iHappinessPerXPolicies > 0)
	{
		iHappiness += GetPlayerPolicies()->GetNumPoliciesOwned() / m_iHappinessPerXPolicies;
	}

	return iHappiness;
}

//	--------------------------------------------------------------------------------
/// Returns the amount of extra Happiness per City
int CvPlayer::GetExtraHappinessPerCity() const
{
	return m_iHappinessPerCity;
}

//	--------------------------------------------------------------------------------
/// Changes amount of extra Happiness per City
void CvPlayer::ChangeExtraHappinessPerCity(int iChange)
{
	CvAssertMsg(m_iHappinessPerCity >= 0, "Count of buildings helping Happiness is corrupted");

	if(iChange != 0)
		m_iHappinessPerCity += iChange;
}

//	--------------------------------------------------------------------------------
/// Returns the amount of extra Happiness per City
int CvPlayer::GetExtraHappinessPerXPolicies() const
{
	return m_iHappinessPerXPolicies;
}

//	--------------------------------------------------------------------------------
/// Changes amount of extra Happiness per City
void CvPlayer::ChangeExtraHappinessPerXPolicies(int iChange)
{
	CvAssertMsg(m_iHappinessPerXPolicies >= 0, "Count of extra happiness per buildings is corrupted");

	if(iChange != 0)
		m_iHappinessPerXPolicies += iChange;
}

//	--------------------------------------------------------------------------------
/// Total amount of Happiness gained from Resources
int CvPlayer::GetHappinessFromResources() const
{
	int iTotalHappiness = 0;

	int iBaseHappiness;

	// Check all connected Resources
	ResourceTypes eResource;
	for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
	{
		eResource = (ResourceTypes) iResourceLoop;

		iBaseHappiness = GetHappinessFromLuxury(eResource);
		if(iBaseHappiness)
		{
			// Resource bonus from Minors, and this is a Luxury we're getting from one (Policies, etc.)
			if(IsMinorResourceBonus() && getResourceFromMinors(eResource) > 0)
			{
				iBaseHappiness *= /*150*/ GC.getMINOR_POLICY_RESOURCE_HAPPINESS_MULTIPLIER();
				iBaseHappiness /= 100;
			}

			iTotalHappiness += iBaseHappiness;
			iTotalHappiness += GetExtraHappinessPerLuxury();
		}
	}

	// Happiness bonus for multiple Resource types
	iTotalHappiness += GetHappinessFromResourceVariety();

	return iTotalHappiness;
}

//	--------------------------------------------------------------------------------
/// Amount of Happiness from having a variety of Luxuries
int CvPlayer::GetHappinessFromResourceVariety() const
{
	int iHappiness = 0;

	int iMultipleLuxuriesBonus = /*1*/ GC.getHAPPINESS_PER_EXTRA_LUXURY();

	// Check all connected Resources
	int iNumHappinessResources = 0;

	ResourceTypes eResource;
	for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
	{
		eResource = (ResourceTypes) iResourceLoop;

		if(GetHappinessFromLuxury(eResource) > 0)
		{
			iNumHappinessResources++;
		}
	}

	if(iNumHappinessResources > 1)
	{
		iHappiness += (--iNumHappinessResources * iMultipleLuxuriesBonus);
	}

	return iHappiness;
}


//	--------------------------------------------------------------------------------
/// Total amount of Happiness gained from Religion
int CvPlayer::GetHappinessFromReligion()
{
	int iHappinessFromReligion = 0;
	CvGameReligions* pReligions = GC.getGame().GetGameReligions();

	// Founder beliefs
	ReligionTypes eFoundedReligion = pReligions->GetFounderBenefitsReligion(GetID());
	if(eFoundedReligion != NO_RELIGION)
	{
		const CvReligion* pReligion = pReligions->GetReligion(eFoundedReligion, NO_PLAYER);
		if(pReligion)
		{
			bool bAtPeace = GET_TEAM(getTeam()).getAtWarCount(false) == 0;
			iHappinessFromReligion += pReligion->m_Beliefs.GetPlayerHappiness(bAtPeace);

			float iHappinessPerFollowingCity = pReligion->m_Beliefs.GetHappinessPerFollowingCity();
			iHappinessFromReligion += (int)((float)pReligions->GetNumCitiesFollowing(eFoundedReligion) * iHappinessPerFollowingCity);

			int iHappinessPerXPeacefulForeignFollowers = pReligion->m_Beliefs.GetHappinessPerXPeacefulForeignFollowers();
			if (iHappinessPerXPeacefulForeignFollowers > 0)
			{
#ifdef BELIEF_PEACE_LOVING_PER_PEACE_FULL_FOLLOWERS
				iHappinessFromReligion += pReligions->GetNumFollowers(eFoundedReligion) / iHappinessPerXPeacefulForeignFollowers;
#else
				iHappinessFromReligion += GetReligions()->GetNumForeignFollowers(true /*bAtPeace */) / iHappinessPerXPeacefulForeignFollowers;
#endif
			}
		}
	}

	return iHappinessFromReligion;
}

//	--------------------------------------------------------------------------------
// Happiness from finding Natural Wonders
int CvPlayer::GetHappinessFromNaturalWonders() const
{
	int iNumWonders = GET_TEAM(getTeam()).GetNumNaturalWondersDiscovered();

	int iHappiness = iNumWonders* /*1*/ GC.getHAPPINESS_PER_NATURAL_WONDER();

	// Trait boosts this further?
	if(m_pTraits->GetNaturalWonderHappinessModifier() > 0)
	{
		iHappiness *= (100 + m_pTraits->GetNaturalWonderHappinessModifier());
		iHappiness /= 100;
	}

	for(int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot* pPlot = GC.getMap().plotByIndexUnchecked(iI);
		if(pPlot == NULL)
		{
			continue;
		}

		if(pPlot->getOwner() != m_eID)
		{
			continue;
		}

		FeatureTypes eFeature = pPlot->getFeatureType();
		if(eFeature == NO_FEATURE)
		{
			continue;
		}

		int iPlotHappiness = GC.getFeatureInfo(eFeature)->getInBorderHappiness();

		if(iPlotHappiness > 0)
		{
			// Trait boosts this further?
			if(m_pTraits->GetNaturalWonderYieldModifier() > 0)
			{
				iPlotHappiness *= (100 + m_pTraits->GetNaturalWonderYieldModifier());
				iPlotHappiness /= 100;
			}

			iHappiness += iPlotHappiness;
		}
	}

	return iHappiness;
}

//	--------------------------------------------------------------------------------
/// Extra Happiness from every connected Luxury
int CvPlayer::GetExtraHappinessPerLuxury() const
{
	return m_iExtraHappinessPerLuxury;
}

//	--------------------------------------------------------------------------------
/// Change Extra Happiness from every connected Luxury
void CvPlayer::ChangeExtraHappinessPerLuxury(int iChange)
{
	//antonjs: consider: this check wasn't here before, maybe we shouldn't have it in case mods, scenarios use luxuries in a unique malus way
	//CvAssertMsg(m_iExtraHappinessPerLuxury + iChange >= 0, "Net extra happiness per luxury not expected to be negative!");
	// slewis - Hey Anton, I removed this because it was complaining during my awesome Fall of Rome scenario.
	m_iExtraHappinessPerLuxury += iChange;
}

//	--------------------------------------------------------------------------------
/// How much happiness credit for having this resource as a luxury?
int CvPlayer::GetHappinessFromLuxury(ResourceTypes eResource) const
{
	CvResourceInfo* pkResourceInfo = GC.getResourceInfo(eResource);
	if(pkResourceInfo)
	{
		int iBaseHappiness = pkResourceInfo->getHappiness();

		if (GC.getGame().GetGameLeagues()->IsLuxuryHappinessBanned(GetID(), eResource))
		{
			iBaseHappiness = 0;
		}

#ifdef NEW_LEAGUE_RESOLUTIONS
		if (GC.getGame().GetGameLeagues()->IsDoubleResourceHappiness(GetID(), eResource))
		{
			iBaseHappiness *= 3;
			iBaseHappiness /= 2;
		}
#endif

		// Only look at Luxuries
		if(pkResourceInfo->getResourceUsage() != RESOURCEUSAGE_LUXURY)
		{
			return 0;
		}

		// Any extras?
		else if(getNumResourceAvailable(eResource, /*bIncludeImport*/ true) > 0)
		{
			return iBaseHappiness;
		}

		else if(GetPlayerTraits()->GetLuxuryHappinessRetention() > 0)
		{
			if(getResourceExport(eResource) > 0)
			{
				return ((iBaseHappiness * GetPlayerTraits()->GetLuxuryHappinessRetention()) / 100);
			}
		}
	}

	return false;
}


//	--------------------------------------------------------------------------------
/// How much Unhappiness are Units producing?
int CvPlayer::GetUnhappinessFromUnits() const
{
	int iUnhappinessFromUnits = m_iUnhappinessFromUnits;

	int iFreeUnitUnhappiness = /*0*/ GC.getFREE_UNIT_HAPPINESS();
	if(iFreeUnitUnhappiness != 0)
	{
		iUnhappinessFromUnits -= iFreeUnitUnhappiness;
	}

	// Can't be less than 0
	if(iUnhappinessFromUnits < 0)
	{
		iUnhappinessFromUnits = 0;
	}

	if(GetUnhappinessFromUnitsMod() != 0)
	{
		iUnhappinessFromUnits *= (100 + GetUnhappinessFromUnitsMod());
		iUnhappinessFromUnits /= 100;
	}

	return iUnhappinessFromUnits;
}

//	--------------------------------------------------------------------------------
/// Changes how much Happiness Units produce
void CvPlayer::ChangeUnhappinessFromUnits(int iChange)
{
	m_iUnhappinessFromUnits += iChange;
}

//	--------------------------------------------------------------------------------
/// How much of our Happiness is being used up? (Population + Units)
int CvPlayer::GetUnhappiness(CvCity* pAssumeCityAnnexed, CvCity* pAssumeCityPuppeted) const
{
	int iUnhappiness = 0;

	// City Count Unhappiness
	iUnhappiness += GetUnhappinessFromCityCount(pAssumeCityAnnexed, pAssumeCityPuppeted);

	// Occupied City Count Unhappiness
	iUnhappiness += GetUnhappinessFromCapturedCityCount(pAssumeCityAnnexed, pAssumeCityPuppeted);

	// City Population Unhappiness
	iUnhappiness += GetUnhappinessFromCityPopulation(pAssumeCityAnnexed, pAssumeCityPuppeted);

	// Occupied City Population Unhappiness
	iUnhappiness += GetUnhappinessFromOccupiedCities(pAssumeCityAnnexed, pAssumeCityPuppeted);

	// Unit Unhappiness (Builders)
	iUnhappiness += GetUnhappinessFromUnits();

	iUnhappiness /= 100;

	iUnhappiness += GetCulture()->GetPublicOpinionUnhappiness();

	// AI gets reduced Unhappiness on higher levels
	if(!isHuman() && !IsAITeammateOfHuman())
	{
		iUnhappiness *= GC.getGame().getHandicapInfo().getAIUnhappinessPercent();
		iUnhappiness /= 100;
	}

	return iUnhappiness;
}

//	--------------------------------------------------------------------------------
/// Used for providing info to the player
int CvPlayer::GetUnhappinessFromCityForUI(CvCity* pCity) const
{
	int iNumCitiesUnhappinessTimes100 = 0;
	int iPopulationUnhappinessTimes100 = 0;

	int iPopulation = pCity->getPopulation() * 100;

	// No Unhappiness from Specialist Pop? (Policies, etc.)
	if(isHalfSpecialistUnhappiness())
	{
		int iSpecialistCount = pCity->GetCityCitizens()->GetTotalSpecialistCount() * 100;
#ifdef UNIVERSAL_SUFFRAGE_TWO_THIRD_UNHAPPINESS
		iPopulation += 2; // Round up
		iPopulation -= (iSpecialistCount * 1 / 3);
#else
		iPopulation -= (iSpecialistCount / 2);
#endif
	}

	// Occupied?
	if(pCity->IsOccupied() && !pCity->IsIgnoreCityForHappiness())
	{
		iNumCitiesUnhappinessTimes100 += (100 * /*5*/ GC.getUNHAPPINESS_PER_CAPTURED_CITY());
		iPopulationUnhappinessTimes100 += int(iPopulation* /*1.34f*/ GC.getUNHAPPINESS_PER_OCCUPIED_POPULATION());

		// Mod (Policies, etc.)
		if(GetOccupiedPopulationUnhappinessMod() != 0)
		{
			iPopulationUnhappinessTimes100 *= (100 + GetOccupiedPopulationUnhappinessMod());
			iPopulationUnhappinessTimes100 /= 100;
		}
	}
	// Normal City
	else
	{
#ifdef LIBERTY_FINISER_LESS_UNHAPPINESS_PER_CITY
		PolicyTypes ePolicy = (PolicyTypes)GC.getInfoTypeForString("POLICY_LIBERTY_FINISHER", true /*bHideAssert*/);
		if (GET_PLAYER((PlayerTypes)GetID()).GetPlayerPolicies()->HasPolicy(ePolicy))
		{
			iNumCitiesUnhappinessTimes100 += (100 * /*2*/ (GC.getUNHAPPINESS_PER_CITY() - 1));
		}
		else
		{
			iNumCitiesUnhappinessTimes100 += (100 * /*2*/ GC.getUNHAPPINESS_PER_CITY());
		}
#else
		iNumCitiesUnhappinessTimes100 += (100 * /*2*/ GC.getUNHAPPINESS_PER_CITY());
#endif
		iPopulationUnhappinessTimes100 += (iPopulation* /*1*/ GC.getUNHAPPINESS_PER_POPULATION());

		if(pCity->isCapital() && GetCapitalUnhappinessMod() != 0)
		{
			iPopulationUnhappinessTimes100 *= (100 + GetCapitalUnhappinessMod());
			iPopulationUnhappinessTimes100 /= 100;
		}

		iPopulationUnhappinessTimes100 *= (100 + GetUnhappinessMod());
		iPopulationUnhappinessTimes100 /= 100;

		iPopulationUnhappinessTimes100 *= 100 + GetPlayerTraits()->GetPopulationUnhappinessModifier();
		iPopulationUnhappinessTimes100 /= 100;
	}

	// Population Handicap mod
	iPopulationUnhappinessTimes100 *= getHandicapInfo().getPopulationUnhappinessMod();
	iPopulationUnhappinessTimes100 /= 100;

	// City Count Player mod
	int iMod = 0;
	iMod += GetCityCountUnhappinessMod();
	iMod += GetPlayerTraits()->GetCityUnhappinessModifier();

	iNumCitiesUnhappinessTimes100 *= (100 + iMod);
	iNumCitiesUnhappinessTimes100 /= 100;

	// City Count Handicap mod
	iNumCitiesUnhappinessTimes100 *= getHandicapInfo().getNumCitiesUnhappinessMod();
	iNumCitiesUnhappinessTimes100 /= 100;

	// City Count Map size mod
	iNumCitiesUnhappinessTimes100 *= GC.getMap().getWorldInfo().getNumCitiesUnhappinessPercent();
	iNumCitiesUnhappinessTimes100 /= 100;

	return iNumCitiesUnhappinessTimes100 + iPopulationUnhappinessTimes100;
}

//	--------------------------------------------------------------------------------
/// Unhappiness from number of Cities
int CvPlayer::GetUnhappinessFromCityCount(CvCity* pAssumeCityAnnexed, CvCity* pAssumeCityPuppeted) const
{
	int iUnhappiness = 0;
#ifdef LIBERTY_FINISER_LESS_UNHAPPINESS_PER_CITY
	PolicyTypes ePolicy = (PolicyTypes)GC.getInfoTypeForString("POLICY_LIBERTY_FINISHER", true /*bHideAssert*/);
	int iUnhappinessPerCity;
	if (GET_PLAYER((PlayerTypes)GetID()).GetPlayerPolicies()->HasPolicy(ePolicy))
	{
		iUnhappinessPerCity = /*2*/ (GC.getUNHAPPINESS_PER_CITY() - 1) * 100;
	}
	else
	{
		iUnhappinessPerCity = /*2*/ GC.getUNHAPPINESS_PER_CITY() * 100;
	}
#else
	int iUnhappinessPerCity = /*2*/ GC.getUNHAPPINESS_PER_CITY() * 100;
#endif

	bool bCityValid;

	int iLoop;
	for(const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		bCityValid = false;

		// Assume city is puppeted, and counts
		if(pLoopCity == pAssumeCityPuppeted)
			bCityValid = true;
		// Assume city is annexed, and does NOT count
		else if(pLoopCity == pAssumeCityAnnexed)
			bCityValid = false;
		// Assume city doesn't exist, and does NOT count
		else if(pLoopCity->IsIgnoreCityForHappiness())
			bCityValid = false;
		// Normal city
		else if(!pLoopCity->IsOccupied() || pLoopCity->IsNoOccupiedUnhappiness())
			bCityValid = true;

		if(bCityValid)
			iUnhappiness += iUnhappinessPerCity;
	}

	// Player count mod
	int iMod = 0;
	iMod += GetCityCountUnhappinessMod();
	iMod += GetPlayerTraits()->GetCityUnhappinessModifier();

	iUnhappiness *= (100 + iMod);
	iUnhappiness /= 100;

	// Handicap mod
	iUnhappiness *= getHandicapInfo().getNumCitiesUnhappinessMod();
	iUnhappiness /= 100;

	// Map size mod
	iUnhappiness *= GC.getMap().getWorldInfo().getNumCitiesUnhappinessPercent();
	iUnhappiness /= 100;

	return iUnhappiness;
}

//	--------------------------------------------------------------------------------
/// Unhappiness from number of Captured Cities
int CvPlayer::GetUnhappinessFromCapturedCityCount(CvCity* pAssumeCityAnnexed, CvCity* pAssumeCityPuppeted) const
{
	int iUnhappiness = 0;
	int iUnhappinessPerCapturedCity = /*5*/ GC.getUNHAPPINESS_PER_CAPTURED_CITY() * 100;

	bool bCityValid;

	int iLoop;
	for(const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		bCityValid = false;

		// Assume city is puppeted, and does NOT count
		if(pLoopCity == pAssumeCityPuppeted)
			bCityValid = false;
		// Assume city is annexed, and counts
		else if(pLoopCity == pAssumeCityAnnexed)
			bCityValid = true;
		// Assume city doesn't exist, and does NOT count
		else if(pLoopCity->IsIgnoreCityForHappiness())
			bCityValid = false;
		// Occupied city
		else if(pLoopCity->IsOccupied() && !pLoopCity->IsNoOccupiedUnhappiness())
			bCityValid = true;

		// Extra Unhappiness from Occupied Cities
		if(bCityValid)
			iUnhappiness += iUnhappinessPerCapturedCity;
	}

	// Player count mod
	int iMod = 0;
	iMod += GetCityCountUnhappinessMod();
	iMod += GetPlayerTraits()->GetCityUnhappinessModifier();

	iUnhappiness *= (100 + iMod);
	iUnhappiness /= 100;

	// Handicap mod
	iUnhappiness *= getHandicapInfo().getNumCitiesUnhappinessMod();
	iUnhappiness /= 100;

	// Map size mod
	iUnhappiness *= GC.getMap().getWorldInfo().getNumCitiesUnhappinessPercent();
	iUnhappiness /= 100;

	return iUnhappiness;
}

//	--------------------------------------------------------------------------------
/// Unhappiness from City Population
int CvPlayer::GetUnhappinessFromCityPopulation(CvCity* pAssumeCityAnnexed, CvCity* pAssumeCityPuppeted) const
{
	int iUnhappiness = 0;
	int iUnhappinessFromThisCity;

	int iUnhappinessPerPop = /*1*/ GC.getUNHAPPINESS_PER_POPULATION() * 100;
	int iPopulation;
	int iSpecialistCount;

	bool bCityValid;

	int iLoop;
	for(const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		bCityValid = false;

		// Assume pLoopCity is Annexed, and does NOT count
		if(pLoopCity == pAssumeCityAnnexed)
			bCityValid = false;
		// Assume that pLoopCity is a Puppet and IS counted here
		else if(pLoopCity == pAssumeCityPuppeted)
			bCityValid = true;
		// Assume city doesn't exist, and does NOT count
		else if(pLoopCity->IsIgnoreCityForHappiness())
			bCityValid = false;
		// Occupied Cities don't get counted here (see the next function)
		else if(!pLoopCity->IsOccupied() || pLoopCity->IsNoOccupiedUnhappiness())
			bCityValid = true;

		if(bCityValid)
		{
			iPopulation = pLoopCity->getPopulation();

			// No Unhappiness from Specialist Pop? (Policies, etc.)
			if(isHalfSpecialistUnhappiness())
			{
				iSpecialistCount = pLoopCity->GetCityCitizens()->GetTotalSpecialistCount();
#ifdef UNIVERSAL_SUFFRAGE_TWO_THIRD_UNHAPPINESS
				iSpecialistCount += 2; // Round up
				iPopulation -= (iSpecialistCount * 1 / 3);
#else
				iSpecialistCount++; // Round up
				iPopulation -= (iSpecialistCount / 2);
#endif
			}

			iUnhappinessFromThisCity = iPopulation * iUnhappinessPerPop;

			if(pLoopCity->isCapital() && GetCapitalUnhappinessMod() != 0)
			{
				iUnhappinessFromThisCity *= (100 + GetCapitalUnhappinessMod());
				iUnhappinessFromThisCity /= 100;
			}

			iUnhappiness += iUnhappinessFromThisCity;
		}
	}

	iUnhappiness *= (100 + GetUnhappinessMod());
	iUnhappiness /= 100;

	iUnhappiness *= 100 + GetPlayerTraits()->GetPopulationUnhappinessModifier();
	iUnhappiness /= 100;

	// Handicap mod
	iUnhappiness *= getHandicapInfo().getPopulationUnhappinessMod();
	iUnhappiness /= 100;

	return iUnhappiness;
}

//	--------------------------------------------------------------------------------
/// Unhappiness from Puppet City Population
int CvPlayer::GetUnhappinessFromPuppetCityPopulation() const
{
	int iUnhappiness = 0;
	int iUnhappinessPerPop = GC.getUNHAPPINESS_PER_POPULATION() * 100;

	int iLoop = 0;
	for(const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		bool bCityValid = false;

		if(pLoopCity->IsPuppet())
			bCityValid = true;

		// Assume city doesn't exist, and does NOT count
		if(pLoopCity->IsIgnoreCityForHappiness())
			bCityValid = false;

		if(bCityValid)
		{
			int iPopulation = pLoopCity->getPopulation();

			// No Unhappiness from Specialist Pop? (Policies, etc.)
			// slewis - 2013.5.7 
			// This function, along with GetUnhappinessFromCitySpecialists, is only called through the UI to reflect 
			// to the player what's going on with their happiness. So I removed the effect that specialists have on 
			// puppeted cities and let the GetUnhappinessFromCitySpecialists correct that problem.

			/*if(isHalfSpecialistUnhappiness())
			{
				int iSpecialistCount = pLoopCity->GetCityCitizens()->GetTotalSpecialistCount();
				iSpecialistCount++; // Round up
				iPopulation -= (iSpecialistCount / 2);
			}*/
			iPopulation -= pLoopCity->GetCityCitizens()->GetTotalSpecialistCount();

			int iUnhappinessFromThisCity = iPopulation * iUnhappinessPerPop;

			if(pLoopCity->isCapital() && GetCapitalUnhappinessMod() != 0)
			{
				iUnhappinessFromThisCity *= (100 + GetCapitalUnhappinessMod());
				iUnhappinessFromThisCity /= 100;
			}

			iUnhappiness += iUnhappinessFromThisCity;
		}
	}

	iUnhappiness *= (100 + GetUnhappinessMod());
	iUnhappiness /= 100;

	iUnhappiness *= 100 + GetPlayerTraits()->GetPopulationUnhappinessModifier();
	iUnhappiness /= 100;

	// Handicap mod
	iUnhappiness *= getHandicapInfo().getPopulationUnhappinessMod();
	iUnhappiness /= 100;

	return iUnhappiness;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetUnhappinessFromCitySpecialists(CvCity* pAssumeCityAnnexed, CvCity* pAssumeCityPuppeted) const
{
	int iUnhappiness = 0;
	int iUnhappinessFromThisCity;

	int iUnhappinessPerPop = /*1*/ GC.getUNHAPPINESS_PER_POPULATION() * 100;
	int iPopulation;

	bool bCityValid;

	int iLoop;
	for(const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		bCityValid = false;

		// Assume pLoopCity is Annexed, and does NOT count
		if(pLoopCity == pAssumeCityAnnexed)
			bCityValid = false;
		// Assume that pLoopCity is a Puppet and IS counted here
		else if(pLoopCity == pAssumeCityPuppeted)
			bCityValid = true;
		// Assume city doesn't exist, and does NOT count
		else if(pLoopCity->IsIgnoreCityForHappiness())
			bCityValid = false;
		// Occupied Cities don't get counted here (see the next function)
		else if(!pLoopCity->IsOccupied() || pLoopCity->IsNoOccupiedUnhappiness())
			bCityValid = true;

		if(bCityValid)
		{
			iPopulation = pLoopCity->GetCityCitizens()->GetTotalSpecialistCount();

			// No Unhappiness from Specialist Pop? (Policies, etc.)
			if(isHalfSpecialistUnhappiness())
			{
#ifdef UNIVERSAL_SUFFRAGE_TWO_THIRD_UNHAPPINESS
				iPopulation += 2; // Round up
				iPopulation = (iPopulation / 3) * 2;
#else
				iPopulation++; // Round up
				iPopulation /= 2;
#endif
			}

			iUnhappinessFromThisCity = iPopulation * iUnhappinessPerPop;

			if(pLoopCity->isCapital() && GetCapitalUnhappinessMod() != 0)
			{
				iUnhappinessFromThisCity *= (100 + GetCapitalUnhappinessMod());
				iUnhappinessFromThisCity /= 100;
			}

			iUnhappiness += iUnhappinessFromThisCity;
		}
	}

	iUnhappiness *= (100 + GetUnhappinessMod());
	iUnhappiness /= 100;

	iUnhappiness *= 100 + GetPlayerTraits()->GetPopulationUnhappinessModifier();
	iUnhappiness /= 100;

	// Handicap mod
	iUnhappiness *= getHandicapInfo().getPopulationUnhappinessMod();
	iUnhappiness /= 100;

	return iUnhappiness;
}

//	--------------------------------------------------------------------------------
/// Unhappiness from City Population in Occupied Cities
int CvPlayer::GetUnhappinessFromOccupiedCities(CvCity* pAssumeCityAnnexed, CvCity* pAssumeCityPuppeted) const
{
	int iUnhappiness = 0;
	int iUnhappinessFromThisCity;

	double fUnhappinessPerPop = /*1.34f*/ GC.getUNHAPPINESS_PER_OCCUPIED_POPULATION() * 100;
	int iPopulation;
	int iSpecialistCount;

	bool bCityValid;

	int iLoop;
	for(const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		bCityValid = false;

		// Assume pLoopCity is Annexed, and counts
		if(pLoopCity == pAssumeCityAnnexed)
			bCityValid = true;
		// Assume that pLoopCity is a Puppet and does NOT count
		else if(pLoopCity == pAssumeCityPuppeted)
			bCityValid = false;
		// Assume city doesn't exist, and does NOT count
		else if(pLoopCity->IsIgnoreCityForHappiness())
			bCityValid = false;
		// Occupied Cities
		else if(pLoopCity->IsOccupied() && !pLoopCity->IsNoOccupiedUnhappiness())
			bCityValid = true;

		if(bCityValid)
		{
			iPopulation = pLoopCity->getPopulation();

			// No Unhappiness from Specialist Pop? (Policies, etc.)
			if(isHalfSpecialistUnhappiness())
			{
				iSpecialistCount = pLoopCity->GetCityCitizens()->GetTotalSpecialistCount();
#ifdef UNIVERSAL_SUFFRAGE_TWO_THIRD_UNHAPPINESS
				iSpecialistCount += 2; // Round up
				iPopulation -= (iSpecialistCount * 1 / 3);
#else
				iSpecialistCount++; // Round up
				iPopulation -= (iSpecialistCount / 2);
#endif
			}

			iUnhappinessFromThisCity = int(double(iPopulation) * fUnhappinessPerPop);

			// Mod (Policies, etc.)
			if(GetOccupiedPopulationUnhappinessMod() != 0)
			{
				iUnhappinessFromThisCity *= (100 + GetOccupiedPopulationUnhappinessMod());
				iUnhappinessFromThisCity /= 100;
			}

			iUnhappiness += iUnhappinessFromThisCity;
		}
	}

	// Handicap mod
	iUnhappiness *= getHandicapInfo().getPopulationUnhappinessMod();
	iUnhappiness /= 100;

	return iUnhappiness;
}

//	--------------------------------------------------------------------------------
/// Unhappiness from Units Percent (50 = 50% of normal)
int CvPlayer::GetUnhappinessFromUnitsMod() const
{
	return m_iUnhappinessFromUnitsMod;
}

//	--------------------------------------------------------------------------------
/// Change Unhappiness from Units Percent (50 = 50% of normal)
void CvPlayer::ChangeUnhappinessFromUnitsMod(int iChange)
{
	if(iChange != 0)
	{
		m_iUnhappinessFromUnitsMod += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// Unhappiness Mod (-50 = 50% of normal)
int CvPlayer::GetUnhappinessMod() const
{
	return m_iUnhappinessMod;
}

//	--------------------------------------------------------------------------------
/// Change Unhappiness Mod (-50 = 50% of normal)
void CvPlayer::ChangeUnhappinessMod(int iChange)
{
	if(iChange != 0)
	{
		m_iUnhappinessMod += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// City Count Unhappiness Mod (-50 = 50% of normal)
int CvPlayer::GetCityCountUnhappinessMod() const
{
	return m_iCityCountUnhappinessMod;
}

//	--------------------------------------------------------------------------------
/// Change City Count Unhappiness Mod (-50 = 50% of normal)
void CvPlayer::ChangeCityCountUnhappinessMod(int iChange)
{
	if(iChange != 0)
	{
		m_iCityCountUnhappinessMod += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// Occupied Population Unhappiness Mod (-50 = 50% of normal)
int CvPlayer::GetOccupiedPopulationUnhappinessMod() const
{
	return m_iOccupiedPopulationUnhappinessMod;
}

//	--------------------------------------------------------------------------------
/// Occupied Population Count Unhappiness Mod (-50 = 50% of normal)
void CvPlayer::ChangeOccupiedPopulationUnhappinessMod(int iChange)
{
	if(iChange != 0)
	{
		m_iOccupiedPopulationUnhappinessMod += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// Unhappiness from Capital Mod (-50 = 50% of normal)
int CvPlayer::GetCapitalUnhappinessMod() const
{
	return m_iCapitalUnhappinessMod;
}

//	--------------------------------------------------------------------------------
/// Change Unhappiness from Capital Mod (-50 = 50% of normal)
void CvPlayer::ChangeCapitalUnhappinessMod(int iChange)
{
	if(iChange != 0)
	{
		m_iCapitalUnhappinessMod += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// How much Happiness are we getting from Units being Garrisoned in Cities?
int CvPlayer::GetHappinessPerGarrisonedUnit() const
{
	return m_iHappinessPerGarrisonedUnitCount;
}

//	--------------------------------------------------------------------------------
/// Set the amount of Happiness we're getting from Units being Garrisoned in Cities
void CvPlayer::SetHappinessPerGarrisonedUnit(int iValue)
{
	m_iHappinessPerGarrisonedUnitCount = iValue;
}

//	--------------------------------------------------------------------------------
/// Change the amount of Happiness we're getting from Units being Garrisoned in Cities
void CvPlayer::ChangeHappinessPerGarrisonedUnit(int iChange)
{
	SetHappinessPerGarrisonedUnit(m_iHappinessPerGarrisonedUnitCount + iChange);
}

//	--------------------------------------------------------------------------------
/// Returns cached amount of Happiness being brought in for having Cities connected via a Route
int CvPlayer::GetHappinessFromTradeRoutes() const
{
	return m_iCityConnectionHappiness;
}

//	--------------------------------------------------------------------------------
/// How much Happiness coming from Trade Routes?
void CvPlayer::DoUpdateCityConnectionHappiness()
{
	int iHappinessPerTradeRoute = GetHappinessPerTradeRoute();

	int iNumCities = 0;
	if(iHappinessPerTradeRoute != 0)
	{
		CvCity* pCapitalCity = getCapitalCity();

		// Must have a capital before we can check if other Cities are connected to it!
		if(pCapitalCity != NULL && getNumCities() > 1)
		{
			CvCity* pLoopCity;

			int iLoop;
			for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				if(pLoopCity != pCapitalCity)
				{
					if(GetTreasury()->HasCityConnectionRouteBetweenCities(pCapitalCity, pLoopCity))
					{
						iNumCities++;
					}
				}
			}
		}
	}
	m_iCityConnectionHappiness = iHappinessPerTradeRoute * iNumCities / 100;	// Bring it out of hundreds
}

//	--------------------------------------------------------------------------------
/// How muchHappiness are we getting from Trade Routes?
int CvPlayer::GetHappinessPerTradeRoute() const
{
	return m_iHappinessPerTradeRouteCount;
}

//	--------------------------------------------------------------------------------
/// Set the amont of Happiness we're getting from Trade Routes
void CvPlayer::SetHappinessPerTradeRoute(int iValue)
{
	m_iHappinessPerTradeRouteCount = iValue;
}

//	--------------------------------------------------------------------------------
/// Change the amont of Happiness we're getting from Trade Routes
void CvPlayer::ChangeHappinessPerTradeRoute(int iChange)
{
	SetHappinessPerTradeRoute(m_iHappinessPerTradeRouteCount + iChange);
}

//	--------------------------------------------------------------------------------
/// How much Happiness are we getting from large cities?
int CvPlayer::GetHappinessPerXPopulation() const
{
	return m_iHappinessPerXPopulation;
}

//	--------------------------------------------------------------------------------
/// Set the amount of Happiness we're getting from large cities
void CvPlayer::SetHappinessPerXPopulation(int iValue)
{
	m_iHappinessPerXPopulation = iValue;
}

//	--------------------------------------------------------------------------------
/// Change the amount of Happiness we're getting from large cities
void CvPlayer::ChangeHappinessPerXPopulation(int iChange)
{
	SetHappinessPerXPopulation(m_iHappinessPerXPopulation + iChange);
}

//	--------------------------------------------------------------------------------
/// Happiness from Minors
int CvPlayer::GetHappinessFromMinorCivs() const
{
	int iHappiness = 0;
	PlayerTypes eMinor;
	for(int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
	{
		eMinor = (PlayerTypes) iMinorLoop;
		iHappiness += GetHappinessFromMinor(eMinor);
	}
	return iHappiness;
}

//	--------------------------------------------------------------------------------
/// Happiness from a Minor
int CvPlayer::GetHappinessFromMinor(PlayerTypes eMinor) const
{
	int iAmount = 0;

	if(GET_PLAYER(eMinor).isAlive())
	{
		// Includes flat bonus and any per luxury bonus
		iAmount += GET_PLAYER(eMinor).GetMinorCivAI()->GetCurrentHappinessBonus(GetID());
	}

	return iAmount;
}

//	--------------------------------------------------------------------------------
/// Happiness from Leagues
int CvPlayer::GetHappinessFromLeagues() const
{
	return m_iHappinessFromLeagues;
}

//	--------------------------------------------------------------------------------
/// Happiness from Leagues
void CvPlayer::SetHappinessFromLeagues(int iValue)
{
	m_iHappinessFromLeagues = iValue;
}

//	--------------------------------------------------------------------------------
/// Happiness from Leagues
void CvPlayer::ChangeHappinessFromLeagues(int iChange)
{
	SetHappinessFromLeagues(GetHappinessFromLeagues() + iChange);
}

//	--------------------------------------------------------------------------------
/// Get the global modifier on the espionage progress rate
int CvPlayer::GetEspionageModifier() const
{
	return m_iEspionageModifier;
}

//	--------------------------------------------------------------------------------
/// Change the global modifier on the espionage progress rate
void CvPlayer::ChangeEspionageModifier(int iChange)
{
	m_iEspionageModifier = (m_iEspionageModifier + iChange);
}

//	--------------------------------------------------------------------------------
/// At what rank do spies start the game at?
int CvPlayer::GetStartingSpyRank() const
{
	return m_iSpyStartingRank;
}

//	--------------------------------------------------------------------------------
/// Change the rank that spies start the game at
void CvPlayer::ChangeStartingSpyRank(int iChange)
{
	m_iSpyStartingRank = (m_iSpyStartingRank + iChange);
}

#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
int CvPlayer::GetNumStolenScience() const
{
	return m_iNumStolenScience;
}
void CvPlayer::ChangeNumStolenScience(int iChange)
{
	m_iNumStolenScience = (m_iNumStolenScience + iChange);
}
#endif

//	--------------------------------------------------------------------------------
/// Extra league votes
int CvPlayer::GetExtraLeagueVotes() const
{
	return m_iExtraLeagueVotes;
}

//	--------------------------------------------------------------------------------
/// Extra league votes
void CvPlayer::ChangeExtraLeagueVotes(int iChange)
{
	m_iExtraLeagueVotes += iChange;
	CvAssert(m_iExtraLeagueVotes >= 0);
	if (m_iExtraLeagueVotes < 0)
	{
		m_iExtraLeagueVotes = 0;
	}
}

#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
//	--------------------------------------------------------------------------------
///
int CvPlayer::GetMaxExtraVotesFromMinors() const
{
	return m_iMaxExtraVotesFromMinors;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeMaxExtraVotesFromMinors(int iChange)
{
	m_iMaxExtraVotesFromMinors += iChange;
	CvAssert(m_iMaxExtraVotesFromMinors >= 0);
	if (m_iMaxExtraVotesFromMinors < 0)
	{
		m_iMaxExtraVotesFromMinors = 0;
	}
}
#endif

#ifdef POLICY_EXTRA_VOTES
//	--------------------------------------------------------------------------------
///
int CvPlayer::GetPolicyExtraVotes() const
{
	return m_iPolicyExtraVotes;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangePolicyExtraVotes(int iChange)
{
	m_iPolicyExtraVotes += iChange;
	CvAssert(m_iPolicyExtraVotes >= 0);
	if (m_iPolicyExtraVotes < 0)
	{
		m_iPolicyExtraVotes = 0;
	}
}
#endif

//	--------------------------------------------------------------------------------
/// How much weaker do Units get when wounded?
int CvPlayer::GetWoundedUnitDamageMod() const
{
	return m_iWoundedUnitDamageMod;
}

//	--------------------------------------------------------------------------------
/// How much weaker do Units get when wounded?
void CvPlayer::SetWoundedUnitDamageMod(int iValue)
{
	m_iWoundedUnitDamageMod = iValue;

	if(m_iWoundedUnitDamageMod < /*50*/ -GC.getWOUNDED_DAMAGE_MULTIPLIER())
	{
		m_iWoundedUnitDamageMod = /*50*/ -GC.getWOUNDED_DAMAGE_MULTIPLIER();
	}
}

//	--------------------------------------------------------------------------------
/// How much weaker do Units get when wounded?
void CvPlayer::ChangeWoundedUnitDamageMod(int iChange)
{
	SetWoundedUnitDamageMod(m_iWoundedUnitDamageMod + iChange);
}

//	--------------------------------------------------------------------------------
/// Unit upgrade cost mod
int CvPlayer::GetUnitUpgradeCostMod() const
{
	return m_iUnitUpgradeCostMod;
}

//	--------------------------------------------------------------------------------
/// Unit upgrade cost mod
void CvPlayer::SetUnitUpgradeCostMod(int iValue)
{
	m_iUnitUpgradeCostMod = iValue;

	if(m_iUnitUpgradeCostMod < /*-75*/ GC.getUNIT_UPGRADE_COST_DISCOUNT_MAX())
		m_iUnitUpgradeCostMod = /*-75*/ GC.getUNIT_UPGRADE_COST_DISCOUNT_MAX();
}

//	--------------------------------------------------------------------------------
/// Unit upgrade cost mod
void CvPlayer::ChangeUnitUpgradeCostMod(int iChange)
{
	SetUnitUpgradeCostMod(m_iUnitUpgradeCostMod + iChange);
}

//	--------------------------------------------------------------------------------
/// How much of a combat bonus do we get VS Barbarian Units?
int CvPlayer::GetBarbarianCombatBonus() const
{
	return m_iBarbarianCombatBonus;
}

//	--------------------------------------------------------------------------------
/// Sets how much of a combat bonus we get VS Barbarian Units
void CvPlayer::SetBarbarianCombatBonus(int iValue)
{
	m_iBarbarianCombatBonus = iValue;
}

//	--------------------------------------------------------------------------------
/// Changes how much of a combat bonus we get VS Barbarian Units
void CvPlayer::ChangeBarbarianCombatBonus(int iChange)
{
	SetBarbarianCombatBonus(m_iBarbarianCombatBonus + iChange);
}

//	--------------------------------------------------------------------------------
/// Do we always see where Barb Camps appear?
bool CvPlayer::IsAlwaysSeeBarbCamps() const
{
	return m_iAlwaysSeeBarbCampsCount > 0;
}

//	--------------------------------------------------------------------------------
/// Sets if we always see where Barb Camps appear
void CvPlayer::SetAlwaysSeeBarbCampsCount(int iValue)
{
	m_iAlwaysSeeBarbCampsCount = iValue;
}

//	--------------------------------------------------------------------------------
/// Changes if we always see where Barb Camps appear
void CvPlayer::ChangeAlwaysSeeBarbCampsCount(int iChange)
{
	SetAlwaysSeeBarbCampsCount(m_iAlwaysSeeBarbCampsCount + iChange);
}

//	--------------------------------------------------------------------------------
CvPlayerTechs* CvPlayer::GetPlayerTechs() const
{
	return m_pPlayerTechs;
}

//	--------------------------------------------------------------------------------
CvPlayerPolicies* CvPlayer::GetPlayerPolicies() const
{
	return m_pPlayerPolicies;
}

//	--------------------------------------------------------------------------------
CvPlayerTraits* CvPlayer::GetPlayerTraits() const
{
	return m_pTraits;
}

//	--------------------------------------------------------------------------------
CvFlavorManager* CvPlayer::GetFlavorManager() const
{
	return m_pFlavorManager;
}

//	--------------------------------------------------------------------------------
CvTacticalAI* CvPlayer::GetTacticalAI() const
{
	return m_pTacticalAI;
}

//	--------------------------------------------------------------------------------
CvHomelandAI* CvPlayer::GetHomelandAI() const
{
	return m_pHomelandAI;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setHasPolicy(PolicyTypes eIndex, bool bNewValue)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumPolicyInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(m_pPlayerPolicies->HasPolicy(eIndex) != bNewValue)
	{
		m_pPlayerPolicies->SetPolicy(eIndex, bNewValue);
		processPolicies(eIndex, bNewValue ? 1 : -1);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNextPolicyCost() const
{
	return m_iCostNextPolicy;
}

//	--------------------------------------------------------------------------------
void CvPlayer::DoUpdateNextPolicyCost()
{
	m_iCostNextPolicy = GetPlayerPolicies()->GetNextPolicyCost();
}

//	--------------------------------------------------------------------------------
bool CvPlayer::canAdoptPolicy(PolicyTypes eIndex) const
{
	return GetPlayerPolicies()->CanAdoptPolicy(eIndex);
}

//	--------------------------------------------------------------------------------
void CvPlayer::doAdoptPolicy(PolicyTypes ePolicy)
{
	CvPolicyEntry* pkPolicyInfo = GC.getPolicyInfo(ePolicy);
	CvAssert(pkPolicyInfo != NULL);
	if(pkPolicyInfo == NULL)
		return;

	// Can we actually adopt this?
	if(!canAdoptPolicy(ePolicy))
		return;

	bool bTenet = pkPolicyInfo->GetLevel() > 0;

	// Pay Culture cost - if applicable
	if (bTenet && GetNumFreeTenets() > 0)
	{
		ChangeNumFreeTenets(-1, false);
	}
	else if (GetNumFreePolicies() > 0)
	{
		ChangeNumFreePolicies(-1);
	}
	else
	{
		changeJONSCulture(-getNextPolicyCost());
	}

	setHasPolicy(ePolicy, true);

	// Update cost if trying to buy another policy this turn
	DoUpdateNextPolicyCost();

#ifdef POLICY_BRANCH_NOTIFICATION_LOCKED
	if(!(getJONSCulture() >= getNextPolicyCost() || GetNumFreePolicies() > 0))
	{
		for (int iI = 0; iI < GC.GetGamePolicies()->GetNumPolicyBranches(); iI++)
		{
			PolicyBranchTypes ePolicyBranch = (PolicyBranchTypes)iI;
			GetPlayerPolicies()->SetPolicyBranchNotificationLocked(ePolicyBranch, false);
		}
	}
#endif

	// Branch unlocked
	PolicyBranchTypes ePolicyBranch = (PolicyBranchTypes) pkPolicyInfo->GetPolicyBranchType();
	GetPlayerPolicies()->SetPolicyBranchUnlocked(ePolicyBranch, true, false);

	GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);

	// This Dirty bit must only be set when changing something for the active player
	if(GC.getGame().getActivePlayer() == GetID())
	{
		GC.GetEngineUserInterface()->setDirty(Policies_DIRTY_BIT, true);
	}


	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());
		args->Push(ePolicy);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		LuaSupport::CallHook(pkScriptSystem, "PlayerAdoptPolicy", args.get(), bResult);
	}

	updateYield();		// Policies can change the yield
}

#ifdef POLICY_DO_TECH_FROM_CITY_CONQ
//	--------------------------------------------------------------------------------
///
bool CvPlayer::IsPolicyTechFromCityConquer() const
{
	return m_iPolicyTechFromCityConquer > 0;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangePolicyTechFromCityConquer(int iChange)
{
	m_iPolicyTechFromCityConquer += iChange;
	CvAssert(m_iPolicyTechFromCityConquer >= 0);
	if (m_iPolicyTechFromCityConquer < 0)
	{
		m_iPolicyTechFromCityConquer = 0;
	}
}
#endif

#ifdef POLICY_NO_CULTURE_SPECIALIST_FOOD
//	--------------------------------------------------------------------------------
///
bool CvPlayer::IsNoCultureSpecialistFood() const
{
	return m_iNoCultureSpecialistFood > 0;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeNoCultureSpecialistFood(int iChange)
{
	m_iNoCultureSpecialistFood += iChange;
	CvAssert(m_iNoCultureSpecialistFood >= 0);
	if (m_iNoCultureSpecialistFood < 0)
	{
		m_iNoCultureSpecialistFood = 0;
	}
}
#endif

#ifdef POLICY_MINORS_GIFT_UNITS
//	--------------------------------------------------------------------------------
///
bool CvPlayer::IsMinorsGiftUnits() const
{
	return m_iMinorsGiftUnits > 0;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeMinorsGiftUnits(int iChange)
{
	m_iMinorsGiftUnits += iChange;
	CvAssert(m_iMinorsGiftUnits >= 0);
	if (m_iMinorsGiftUnits < 0)
	{
		m_iMinorsGiftUnits = 0;
	}
	if (m_iMinorsGiftUnits > 0)
	{
		for (int iMinorCivLoop = MAX_MAJOR_CIVS; iMinorCivLoop < MAX_CIV_PLAYERS; iMinorCivLoop++)
		{
			PlayerTypes eMinorCivLoop = (PlayerTypes)iMinorCivLoop;
			if (GET_PLAYER(eMinorCivLoop).isAlive() && GET_TEAM(GET_PLAYER((PlayerTypes)GetID()).getTeam()).isHasMet(GET_PLAYER(eMinorCivLoop).getTeam()) && GET_PLAYER(eMinorCivLoop).GetMinorCivAI()->GetUnitSpawnCounter((PlayerTypes)GetID()) == -1)
			{
				GET_PLAYER(eMinorCivLoop).GetMinorCivAI()->DoSeedUnitSpawnCounter((PlayerTypes)GetID());
			}
		}
	}
}
#endif

#ifdef POLICY_NO_CARGO_PILLAGE
//	--------------------------------------------------------------------------------
///
bool CvPlayer::IsNoCargoPillage() const
{
	return m_iNoCargoPillage > 0;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeNoCargoPillage(int iChange)
{
	m_iNoCargoPillage += iChange;
	CvAssert(m_iNoCargoPillage >= 0);
	if (m_iNoCargoPillage < 0)
	{
		m_iNoCargoPillage = 0;
	}
}
#endif

#ifdef POLICY_GREAT_WORK_HAPPINESS
//	--------------------------------------------------------------------------------
///
int CvPlayer::GetGreatWorkHappiness() const
{
	return m_iGreatWorkHappiness;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeGreatWorkHappiness(int iChange)
{
	m_iGreatWorkHappiness += iChange;
	CvAssert(m_iGreatWorkHappiness >= 0);
	if (m_iGreatWorkHappiness < 0)
	{
		m_iGreatWorkHappiness = 0;
	}
}
#endif

#ifdef POLICY_SCIENCE_PER_X_FOLLOWERS
//	--------------------------------------------------------------------------------
///
int CvPlayer::GetSciencePerXFollowers() const
{
	return m_iSciencePerXFollowers;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeSciencePerXFollowers(int iChange)
{
	m_iSciencePerXFollowers += iChange;
	CvAssert(m_iSciencePerXFollowers >= 0);
	if (m_iSciencePerXFollowers < 0)
	{
		m_iSciencePerXFollowers = 0;
	}
}
#endif

#ifdef POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD
//	--------------------------------------------------------------------------------
///
bool CvPlayer::IsNoDifferentIdeologiesTourismMod() const
{
	return m_iNoDifferentIdeologiesTourismMod > 0;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeNoDifferentIdeologiesTourismMod(int iChange)
{
	m_iNoDifferentIdeologiesTourismMod += iChange;
	CvAssert(m_iNoDifferentIdeologiesTourismMod >= 0);
	if (m_iNoDifferentIdeologiesTourismMod < 0)
	{
		m_iNoDifferentIdeologiesTourismMod = 0;
	}
}
#endif

#ifdef POLICY_GLOBAL_POP_CHANGE
//	--------------------------------------------------------------------------------
///
int CvPlayer::GetGlobalPopChange() const
{
	return m_iGlobalPopChange;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeGlobalPopChange(int iChange)
{
	m_iGlobalPopChange += iChange;
	CvAssert(m_iGlobalPopChange >= 0);
	if (m_iGlobalPopChange < 0)
	{
		m_iGlobalPopChange = 0;
	}
}
#endif

#ifdef POLICY_GREAT_WORK_TOURISM_CHANGES
//	--------------------------------------------------------------------------------
///
int CvPlayer::GetGreatWorkTourismChanges() const
{
	return m_iGreatWorkTourismChanges;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeGreatWorkTourismChanges(int iChange)
{
	m_iGreatWorkTourismChanges += iChange;
	CvAssert(m_iGreatWorkTourismChanges >= 0);
	if (m_iGreatWorkTourismChanges < 0)
	{
		m_iGreatWorkTourismChanges = 0;
	}
}
#endif

#ifdef POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP
//	--------------------------------------------------------------------------------
///
int CvPlayer::GetCityScienceSquaredModPerXPop() const
{
	return m_iCityScienceSquaredModPerXPop;
}

//	--------------------------------------------------------------------------------
///
void CvPlayer::ChangeCityScienceSquaredModPerXPop(int iChange)
{
	m_iCityScienceSquaredModPerXPop += iChange;
	CvAssert(m_iCityScienceSquaredModPerXPop >= 0);
	if (m_iCityScienceSquaredModPerXPop < 0)
	{
		m_iCityScienceSquaredModPerXPop = 0;
	}
}
#endif

//	--------------------------------------------------------------------------------
/// Empire in Anarchy?
bool CvPlayer::IsAnarchy() const
{
	return GetAnarchyNumTurns() > 0;
}

//	--------------------------------------------------------------------------------
/// Empire in Anarchy?
int CvPlayer::GetAnarchyNumTurns() const
{
	return m_iAnarchyNumTurns;
}

//	--------------------------------------------------------------------------------
/// Empire in Anarchy?
void CvPlayer::SetAnarchyNumTurns(int iValue)
{
	if(iValue != GetAnarchyNumTurns())
	{
		m_iAnarchyNumTurns = iValue;

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
/// Empire in Anarchy?
void CvPlayer::ChangeAnarchyNumTurns(int iChange)
{
	SetAnarchyNumTurns(GetAnarchyNumTurns() + iChange);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getAdvancedStartPoints() const
{
	return m_iAdvancedStartPoints;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setAdvancedStartPoints(int iNewValue)
{
	if(getAdvancedStartPoints() != iNewValue)
	{
		m_iAdvancedStartPoints = iNewValue;

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(MiscButtons_DIRTY_BIT, true);
			GC.GetEngineUserInterface()->setDirty(SelectionButtons_DIRTY_BIT, true);
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeAdvancedStartPoints(int iChange)
{
	setAdvancedStartPoints(getAdvancedStartPoints() + iChange);
}


//	--------------------------------------------------------------------------------
// Get Attack Bonus for a certain period of time
int CvPlayer::GetAttackBonusTurns() const
{
	return m_iAttackBonusTurns;
}

//	--------------------------------------------------------------------------------
// Set Attack Bonus for a certain period of time
void CvPlayer::ChangeAttackBonusTurns(int iChange)
{
	if(iChange != 0)
	{
		m_iAttackBonusTurns += iChange;
	}
}

//	--------------------------------------------------------------------------------
// Get Culture Bonus for a certain period of time
int CvPlayer::GetCultureBonusTurns() const
{
	return m_iCultureBonusTurns;
}

//	--------------------------------------------------------------------------------
// Set Culture Bonus for a certain period of time
void CvPlayer::ChangeCultureBonusTurns(int iChange)
{
	if (iChange != 0)
	{
		m_iCultureBonusTurns += iChange;
	}
}

//	--------------------------------------------------------------------------------
// Get Tourism Bonus for a certain period of time
int CvPlayer::GetTourismBonusTurns() const
{
	return m_iTourismBonusTurns;
}

//	--------------------------------------------------------------------------------
// Set Tourism Bonus for a certain period of time
void CvPlayer::ChangeTourismBonusTurns(int iChange)
{
	if (iChange != 0)
	{
		m_iTourismBonusTurns += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// Update all Golden-Age related stuff
void CvPlayer::DoProcessGoldenAge()
{
	if(GC.getGame().isOption(GAMEOPTION_NO_HAPPINESS))
	{
		return;
	}

	// Minors and Barbs can't get GAs
	if(!isMinorCiv() && !isBarbarian())
	{
		// Already in a GA - don't decrement counter while in Anarchy
		if(getGoldenAgeTurns() > 0)
		{
			if(!IsAnarchy())
			{
				changeGoldenAgeTurns(-1);
			}
		}

		// Not in GA
		else
		{
			// Note: This will actually REDUCE the GA meter if the player is running in the red
			ChangeGoldenAgeProgressMeter(GetExcessHappiness());

			// Enough GA Progress to trigger new GA?
			if(GetGoldenAgeProgressMeter() >= GetGoldenAgeProgressThreshold())
			{
				int iOverflow = GetGoldenAgeProgressMeter() - GetGoldenAgeProgressThreshold();

				SetGoldenAgeProgressMeter(iOverflow);
				
				int iLength = getGoldenAgeLength();
				changeGoldenAgeTurns(iLength);

				// If it's the active player then show the popup
				if(GetID() == GC.getGame().getActivePlayer())
				{
					// Don't show in MP
					if(!GC.getGame().isNetworkMultiPlayer())	// KWG: Candidate for !GC.getGame().isOption(GAMEOPTION_SIMULTANEOUS_TURNS)
					{
						CvPopupInfo kPopupInfo(BUTTONPOPUP_GOLDEN_AGE_REWARD);
						GC.GetEngineUserInterface()->AddPopup(kPopupInfo);
					}
				}
			}
		}
	}
#ifdef TAJ_MAHAL_STARTS_GA_NEXT_TURN
	changeGoldenAgeTurns(getBuildingGoldenAgeTurns());
	setBuildingGoldenAgeTurns(0);
#endif
}

//	--------------------------------------------------------------------------------
/// How much do we need in the GA meter to trigger the next one?
int CvPlayer::GetGoldenAgeProgressThreshold() const
{
	int iThreshold = /*500*/ GC.getGOLDEN_AGE_BASE_THRESHOLD_HAPPINESS();
	iThreshold += GetNumGoldenAges() * /*500*/ GC.getGOLDEN_AGE_EACH_GA_ADDITIONAL_HAPPINESS();

	// Increase cost based on the # of cities in the empire
	int iCostExtra = int(iThreshold * (getNumCities() - 1) * /*0.02*/ GC.getGOLDEN_AGE_THRESHOLD_CITY_MULTIPLIER());
	iThreshold += iCostExtra;

	if(GetGoldenAgeMeterMod() != 0)
	{
		iThreshold *= (100 + GetGoldenAgeMeterMod());
		iThreshold /= 100;
	}

	// Game Speed Mod
	iThreshold *= GC.getGame().getGameSpeedInfo().getGreatPeoplePercent();
	iThreshold /= 100;

	// Make the number nice to look at
	int iVisibleDivisor = /*5*/ GC.getGOLDEN_AGE_VISIBLE_THRESHOLD_DIVISOR();
	iThreshold /= iVisibleDivisor;
	iThreshold *= iVisibleDivisor;

	return iThreshold;
}

//	--------------------------------------------------------------------------------
/// What is our progress towards the next GA?
int CvPlayer::GetGoldenAgeProgressMeter() const
{
	return m_iGoldenAgeProgressMeter;
}

//	--------------------------------------------------------------------------------
/// Sets what is our progress towards the next GA
void CvPlayer::SetGoldenAgeProgressMeter(int iValue)
{
	m_iGoldenAgeProgressMeter = iValue;

	if(m_iGoldenAgeProgressMeter < 0)
	{
		m_iGoldenAgeProgressMeter = 0;
	}

	GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
}

//	--------------------------------------------------------------------------------
/// Changes what is our progress towards the next GA
void CvPlayer::ChangeGoldenAgeProgressMeter(int iChange)
{
	SetGoldenAgeProgressMeter(GetGoldenAgeProgressMeter() + iChange);
}

//	--------------------------------------------------------------------------------
/// Modifier for how big the GA meter is (-50 = 50% of normal)
int CvPlayer::GetGoldenAgeMeterMod() const
{
	return m_iGoldenAgeMeterMod;
}

//	--------------------------------------------------------------------------------
/// Sets Modifier for how big the GA meter is (-50 = 50% of normal)
void CvPlayer::SetGoldenAgeMeterMod(int iValue)
{
	m_iGoldenAgeMeterMod = iValue;
}

//	--------------------------------------------------------------------------------
/// Changes Modifier for how big the GA meter is (-50 = 50% of normal)
void CvPlayer::ChangeGoldenAgeMeterMod(int iChange)
{
	SetGoldenAgeMeterMod(GetGoldenAgeMeterMod() + iChange);
}

//	--------------------------------------------------------------------------------
/// How many GAs have we had in this game?
int CvPlayer::GetNumGoldenAges() const
{
	return m_iNumGoldenAges;
}

//	--------------------------------------------------------------------------------
/// Sets how many GAs have we had in this game
void CvPlayer::SetNumGoldenAges(int iValue)
{
	m_iNumGoldenAges = iValue;

	if(iValue > 0 && isHuman() && !GC.getGame().isGameMultiPlayer()&& GET_PLAYER(GC.getGame().getActivePlayer()).isLocalPlayer())
	{
		gDLL->UnlockAchievement(ACHIEVEMENT_GOLDEN_AGE);

		const char* strLeader = getLeaderTypeKey();
		if(m_iNumGoldenAges >=5 && NULL != strLeader && strcmp(strLeader, "LEADER_DARIUS") == 0)
		{
			gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_ARCHAEMENNID);
		}
	}
}

//	--------------------------------------------------------------------------------
/// Changes how many GAs have we had in this game
void CvPlayer::ChangeNumGoldenAges(int iChange)
{
	SetNumGoldenAges(GetNumGoldenAges() + iChange);
}

//	--------------------------------------------------------------------------------
/// How many turns left in GA? (0 if not in GA)
int CvPlayer::getGoldenAgeTurns() const
{
	return m_iGoldenAgeTurns;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isGoldenAge() const
{
	return (getGoldenAgeTurns() > 0);
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeGoldenAgeTurns(int iChange)
{
	Localization::String locString;
	Localization::String locSummaryString;

	bool bOldGoldenAge;

	if(iChange != 0)
	{
		bOldGoldenAge = isGoldenAge();

		m_iGoldenAgeTurns = (m_iGoldenAgeTurns + iChange);
		CvAssert(getGoldenAgeTurns() >= 0);

		if(bOldGoldenAge != isGoldenAge())
		{
			GC.getMap().updateYield();	// Do the entire map, so that any potential golden age bonus is reflected in the yield icons.

			if(isGoldenAge())
			{
				ChangeNumGoldenAges(1);

				locString = Localization::Lookup("TXT_KEY_NOTIFICATION_GOLDEN_AGE_BEGUN");
				locString << getCivilizationAdjectiveKey();
				GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GetID(), locString.toUTF8(), -1, -1);

				gDLL->GameplayGoldenAgeStarted();
			}
			else
			{
				gDLL->GameplayGoldenAgeEnded();
			}

			CvNotifications* pNotifications = GetNotifications();
			if(pNotifications)
			{
				NotificationTypes eNotification = NO_NOTIFICATION_TYPE;

				if(isGoldenAge())
				{
					eNotification = NOTIFICATION_GOLDEN_AGE_BEGUN_ACTIVE_PLAYER;
					locString = Localization::Lookup("TXT_KEY_NOTIFICATION_GOLDEN_AGE_BEGUN_ACTIVE_PLAYER");
					locSummaryString = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_GOLDEN_AGE_BEGUN_ACTIVE_PLAYER");
				}
				else
				{
					eNotification = NOTIFICATION_GOLDEN_AGE_ENDED_ACTIVE_PLAYER;
					locString = Localization::Lookup("TXT_KEY_NOTIFICATION_GOLDEN_AGE_ENDED_ACTIVE_PLAYER");
					locSummaryString = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_GOLDEN_AGE_ENDED_ACTIVE_PLAYER");
				}

				pNotifications->Add(eNotification, locString.toUTF8(), locSummaryString.toUTF8(), -1, -1, -1);
			}
		}

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGoldenAgeLength() const
{
	int iTurns = GC.getGame().goldenAgeLength();

	// Player modifier
	int iLengthModifier = getGoldenAgeModifier();

	// Trait modifier
	iLengthModifier += GetPlayerTraits()->GetGoldenAgeDurationModifier();

	if(iLengthModifier > 0)
	{
		iTurns = iTurns * (100 + iLengthModifier) / 100;
	}

	return iTurns;
}

#ifdef TAJ_MAHAL_STARTS_GA_NEXT_TURN
//	--------------------------------------------------------------------------------
void CvPlayer::setBuildingGoldenAgeTurns(int iValue)
{
	m_iBuildingGoldenAgeTurns = iValue;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getBuildingGoldenAgeTurns() const
{
	return m_iBuildingGoldenAgeTurns;
}
#endif

//	--------------------------------------------------------------------------------
int CvPlayer::getNumUnitGoldenAges() const
{
	return m_iNumUnitGoldenAges;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeNumUnitGoldenAges(int iChange)
{
	m_iNumUnitGoldenAges = (m_iNumUnitGoldenAges + iChange);
	CvAssert(getNumUnitGoldenAges() >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getStrikeTurns() const
{
	return m_iStrikeTurns;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeStrikeTurns(int iChange)
{
	m_iStrikeTurns = (m_iStrikeTurns + iChange);
	CvAssert(getStrikeTurns() >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getGoldenAgeModifier() const
{
	return m_iGoldenAgeModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeGoldenAgeModifier(int iChange)
{
	m_iGoldenAgeModifier += iChange;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatPeopleCreated() const
{
	return m_iGreatPeopleCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatPeopleCreated()
{
	m_iGreatPeopleCreated++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatGeneralsCreated() const
{
	return m_iGreatGeneralsCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatGeneralsCreated()
{
	m_iGreatGeneralsCreated++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatAdmiralsCreated() const
{
	return m_iGreatAdmiralsCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatAdmiralsCreated()
{
	m_iGreatAdmiralsCreated++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatWritersCreated() const
{
	return m_iGreatWritersCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatWritersCreated()
{
	m_iGreatWritersCreated++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatArtistsCreated() const
{
	return m_iGreatArtistsCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatArtistsCreated()
{
	m_iGreatArtistsCreated++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatMusiciansCreated() const
{
	return m_iGreatMusiciansCreated;
}

#ifdef FREE_GREAT_PERSON
//	--------------------------------------------------------------------------------
int CvPlayer::getGreatProphetsCreated() const
{
	return m_iGreatProphetsCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatProphetsCreated()
{
	m_iGreatProphetsCreated++;
}
#endif

#if defined SEPARATE_GREAT_PEOPLE || defined SWEDEN_UA_REWORK
//	--------------------------------------------------------------------------------
int CvPlayer::getGreatScientistsCreated() const
{
	return m_iGreatScientistsCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatScientistsCreated()
{
	m_iGreatScientistsCreated++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatEngineersCreated() const
{
	return m_iGreatEngineersCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatEngineersCreated()
{
	m_iGreatEngineersCreated++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatMerchantsCreated() const
{
	return m_iGreatMerchantsCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatMerchantsCreated()
{
	m_iGreatMerchantsCreated++;
}
#endif
#ifdef SEPARATE_MERCHANTS
//	--------------------------------------------------------------------------------
int CvPlayer::getGreatMerchantsCreated() const
{
	return m_iGreatMerchantsCreated;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatMerchantsCreated()
{
	m_iGreatMerchantsCreated++;
}
#endif
//	--------------------------------------------------------------------------------
void CvPlayer::incrementGreatMusiciansCreated()
{
	m_iGreatMusiciansCreated++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getMerchantsFromFaith() const
{
	return m_iMerchantsFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementMerchantsFromFaith()
{
	m_iMerchantsFromFaith++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getScientistsFromFaith() const
{
#ifdef FAITH_FOR_THE_FIRST_SCIENTIST
	CvGame& kGame = GC.getGame();
	if (kGame.isOption("GAMEOPTION_EXPENSIVE_SCIENTISTS_FOR_FAITH"))
	{
		return m_iScientistsFromFaith + 1;
	}
	else
	{
		return m_iScientistsFromFaith + 1;
	}
#else
	return m_iScientistsFromFaith;
#endif
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementScientistsFromFaith()
{
	m_iScientistsFromFaith++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getWritersFromFaith() const
{
	return m_iWritersFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementWritersFromFaith()
{
	m_iWritersFromFaith++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getArtistsFromFaith() const
{
	return m_iArtistsFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementArtistsFromFaith()
{
	m_iArtistsFromFaith++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getMusiciansFromFaith() const
{
	return m_iMusiciansFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementMusiciansFromFaith()
{
	m_iMusiciansFromFaith++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGeneralsFromFaith() const
{
	return m_iGeneralsFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementGeneralsFromFaith()
{
	m_iGeneralsFromFaith++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getAdmiralsFromFaith() const
{
	return m_iAdmiralsFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementAdmiralsFromFaith()
{
	m_iAdmiralsFromFaith++;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getEngineersFromFaith() const
{
	return m_iEngineersFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::incrementEngineersFromFaith()
{
	m_iEngineersFromFaith++;
}

#ifdef BELIEF_TO_GLORY_OF_GOD_ONE_GP_OF_EACH_TYPE
//	--------------------------------------------------------------------------------
bool CvPlayer::getbMerchantsFromFaith() const
{
	return m_bMerchantsFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setMerchantsFromFaith(bool bNewValue)
{
	if(m_bMerchantsFromFaith != bNewValue)
	{
		m_bMerchantsFromFaith = bNewValue;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::getbScientistsFromFaith() const
{
	return m_bScientistsFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setScientistsFromFaith(bool bNewValue)
{
	if(m_bScientistsFromFaith != bNewValue)
	{
		m_bScientistsFromFaith = bNewValue;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::getbWritersFromFaith() const
{
	return m_bWritersFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setWritersFromFaith(bool bNewValue)
{
	if(m_bWritersFromFaith != bNewValue)
	{
		m_bWritersFromFaith = bNewValue;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::getbArtistsFromFaith() const
{
	return m_bArtistsFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setArtistsFromFaith(bool bNewValue)
{
	if(m_bArtistsFromFaith != bNewValue)
	{
		m_bArtistsFromFaith = bNewValue;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::getbMusiciansFromFaith() const
{
	return m_bMusiciansFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setMusiciansFromFaith(bool bNewValue)
{
	if(m_bMusiciansFromFaith != bNewValue)
	{
		m_bMusiciansFromFaith = bNewValue;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::getbGeneralsFromFaith() const
{
	return m_bGeneralsFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setGeneralsFromFaith(bool bNewValue)
{
	if(m_bGeneralsFromFaith != bNewValue)
	{
		m_bGeneralsFromFaith = bNewValue;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::getbAdmiralsFromFaith() const
{
	return m_bAdmiralsFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setAdmiralsFromFaith(bool bNewValue)
{
	if(m_bAdmiralsFromFaith != bNewValue)
	{
		m_bAdmiralsFromFaith = bNewValue;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::getbEngineersFromFaith() const
{
	return m_bEngineersFromFaith;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setEngineersFromFaith(bool bNewValue)
{
	if(m_bEngineersFromFaith != bNewValue)
	{
		m_bEngineersFromFaith = bNewValue;
	}
}

#endif
//	--------------------------------------------------------------------------------
int CvPlayer::getGreatPeopleThresholdModifier() const
{
	return m_iGreatPeopleThresholdModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeGreatPeopleThresholdModifier(int iChange)
{
	m_iGreatPeopleThresholdModifier = (m_iGreatPeopleThresholdModifier + iChange);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatGeneralsThresholdModifier() const
{
	return m_iGreatGeneralsThresholdModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeGreatGeneralsThresholdModifier(int iChange)
{
	m_iGreatGeneralsThresholdModifier += iChange;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatAdmiralsThresholdModifier() const
{
	return m_iGreatAdmiralsThresholdModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeGreatAdmiralsThresholdModifier(int iChange)
{
	m_iGreatAdmiralsThresholdModifier += iChange;
}


//	--------------------------------------------------------------------------------
int CvPlayer::getPolicyCostModifier() const
{
	return m_iPolicyCostModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::recomputePolicyCostModifier()
{
	int iCost = m_pPlayerPolicies->GetNumericModifier(POLICYMOD_POLICY_COST_MODIFIER);
	iCost += GetPolicyCostBuildingModifier();
	iCost += GetPolicyCostMinorCivModifier();
	iCost += GetPlayerTraits()->GetPolicyCostModifier();

	if(iCost < /*-75*/ GC.getPOLICY_COST_DISCOUNT_MAX())
		iCost = /*-75*/ GC.getPOLICY_COST_DISCOUNT_MAX();

	m_iPolicyCostModifier = iCost;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatPeopleRateModifier() const
{
	return m_iGreatPeopleRateModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatGeneralRateModifier() const
{
	return m_iGreatGeneralRateModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatAdmiralRateModifier() const
{
	return m_iGreatAdmiralRateModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatWriterRateModifier() const
{
	return m_iGreatWriterRateModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatArtistRateModifier() const
{
	return m_iGreatArtistRateModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatMusicianRateModifier() const
{
	return m_iGreatMusicianRateModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatMerchantRateModifier() const
{
	return m_iGreatMerchantRateModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatScientistRateModifier() const
{
	return m_iGreatScientistRateModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGreatEngineerRateModifier() const
{
	return m_iGreatEngineerRateModifier;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getDomesticGreatGeneralRateModifier() const
{
	return m_iDomesticGreatGeneralRateModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeGreatPeopleRateModFromBldgs(int ichange)
{
	m_iGreatPeopleRateModFromBldgs += ichange;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeGreatGeneralRateModFromBldgs(int ichange)
{
	m_iGreatGeneralRateModFromBldgs += ichange;
}

//	--------------------------------------------------------------------------------
/// Do effects when a unit is killed in combat
void CvPlayer::DoUnitKilledCombat(PlayerTypes eKilledPlayer, UnitTypes eUnitType)
{
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());
		args->Push(eKilledPlayer);
		args->Push(eUnitType);

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "UnitKilledInCombat", args.get(), bResult);
	}
#ifdef REPLAY_EVENTS
	if (isHuman())
	{
		std::vector<int> vArgs;
		vArgs.push_back(static_cast<int>(eKilledPlayer));
		vArgs.push_back(static_cast<int>(eUnitType));
		GC.getGame().addReplayEvent(REPLAYEVENT_UnitKilledInCombat, GetID(), vArgs);
	}
#endif
}

//	--------------------------------------------------------------------------------
/// Do effects when a GP is consumed
void CvPlayer::DoGreatPersonExpended(UnitTypes eGreatPersonUnit)
{
	// Gold gained
#ifdef DUEL_HALICARNASSUS_GP_EXPENDED_GOLD_SCALE
	int iExpendGold;
	if (GC.getGame().isOption("GAMEOPTION_DUEL_STUFF"))
	{
		iExpendGold = GetGreatPersonExpendGold() * GC.getGame().getGameSpeedInfo().getTrainPercent() / 100;
	}
	else
	{
		iExpendGold = GetGreatPersonExpendGold();
	}
#else
	int iExpendGold = GetGreatPersonExpendGold();
#endif
	if(iExpendGold > 0)
	{
		GetTreasury()->ChangeGold(iExpendGold);

		if(isHuman() && !GC.getGame().isGameMultiPlayer() && GET_PLAYER(GC.getGame().getActivePlayer()).isLocalPlayer())
		{
			// Update Steam stat and check achievement
			const int HALICARNASSUS_ACHIEVEMENT_GOLD = 1000;
			int iHalicarnassus = GC.getInfoTypeForString("BUILDINGCLASS_MAUSOLEUM_HALICARNASSUS");
			// Does player have DLC_06, and if so, do they have the Mausoleum of Halicarnassus?
			if(iHalicarnassus != -1 && getBuildingClassCount((BuildingClassTypes)iHalicarnassus) >= 1)
			{
				BuildingTypes eHalicarnassus = (BuildingTypes)GC.getInfoTypeForString("BUILDING_MAUSOLEUM_HALICARNASSUS");
				CvBuildingEntry* pHalicarnassusInfo = GC.getBuildingInfo(eHalicarnassus);
				int iHalicarnassusGold = pHalicarnassusInfo->GetGreatPersonExpendGold();

				int32 iTotalHalicarnassusGold = 0;
				if(gDLL->GetSteamStat(ESTEAMSTAT_HALICARNASSUSGOLDEARNED, &iTotalHalicarnassusGold))
				{
					iTotalHalicarnassusGold += iHalicarnassusGold;
					gDLL->SetSteamStat(ESTEAMSTAT_HALICARNASSUSGOLDEARNED, iTotalHalicarnassusGold);
					if(iTotalHalicarnassusGold >= HALICARNASSUS_ACHIEVEMENT_GOLD)
					{
						gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_HALICARNASSUS_GOLD);
					}
				}
			}
		}
	}

	// Faith gained
	ReligionTypes eReligionFounded = GetReligions()->GetReligionCreatedByPlayer();
	if(eReligionFounded > RELIGION_PANTHEON)
	{
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligionFounded, GetID());
		if(pReligion)
		{
			int iFaith = pReligion->m_Beliefs.GetGreatPersonExpendedFaith();
			if(iFaith > 0)
			{
				iFaith *= GC.getGame().getGameSpeedInfo().getTrainPercent();
				iFaith /= 100;
				ChangeFaith(iFaith);
#ifdef RELIQUARY_REWORK
				changeJONSCulture(iFaith);
#ifdef UPDATE_CULTURE_NOTIFICATION_DURING_TURN
				// if this is the human player, have the popup come up so that he can choose a new policy
				if (isAlive() && isHuman() && getNumCities() > 0)
				{
					if (!GC.GetEngineUserInterface()->IsPolicyNotificationSeen())
					{
						if (getNextPolicyCost() <= getJONSCulture() && GetPlayerPolicies()->GetNumPoliciesCanBeAdopted() > 0)
						{
							CvNotifications* pNotifications = GetNotifications();
							if (pNotifications)
							{
								CvString strBuffer;

								if (GC.getGame().isOption(GAMEOPTION_POLICY_SAVING))
									strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY_DISMISS");
								else
									strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_CULTURE_FOR_POLICY");

								CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ENOUGH_CULTURE_FOR_POLICY");
								pNotifications->Add(NOTIFICATION_POLICY, strBuffer, strSummary, -1, -1, -1);
							}
						}
					}
				}
#endif
#endif
			}
		}
	}

#ifdef GP_EXPENDED_GA
	// Golden Age gained
	if (eReligionFounded > RELIGION_PANTHEON)
	{
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligionFounded, GetID());
		if (pReligion)
		{
			int iGoldenAgeTurns = pReligion->m_Beliefs.GetGreatPersonExpendedGoldenAge();
			if (iGoldenAgeTurns > 0)
			{
				if (!isGoldenAge())
				{
					ChangeNumGoldenAges(-1);
				}
				changeGoldenAgeTurns(iGoldenAgeTurns);
			}
		}
	}
#endif

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if (pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());
		args->Push(eGreatPersonUnit);

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "GreatPersonExpended", args.get(), bResult);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetGreatPersonExpendGold() const
{
	return m_iGreatPersonExpendGold;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeGreatPersonExpendGold(int ichange)
{
	m_iGreatPersonExpendGold += ichange;
}

//	--------------------------------------------------------------------------------
void CvPlayer::recomputeGreatPeopleModifiers()
{
	//=============
	// Initialize
	//=============
	m_iGreatPeopleRateModifier = 0;
	m_iGreatGeneralRateModifier = 0;
	m_iGreatAdmiralRateModifier = 0;
	m_iGreatWriterRateModifier = 0;
	m_iGreatArtistRateModifier = 0;
	m_iGreatMusicianRateModifier = 0;
	m_iGreatMerchantRateModifier = 0;
	m_iGreatScientistRateModifier = 0;
	m_iGreatEngineerRateModifier = 0;
	m_iDomesticGreatGeneralRateModifier = 0;

	// Get from traits first
	m_iGreatPeopleRateModifier += m_pTraits->GetGreatPeopleRateModifier();
	m_iGreatGeneralRateModifier += m_pTraits->GetGreatGeneralRateModifier();
	m_iGreatScientistRateModifier += m_pTraits->GetGreatScientistRateModifier();

	// Then get from current policies
	m_iGreatPeopleRateModifier += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_GREAT_PERSON_RATE);
	m_iGreatGeneralRateModifier += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_GREAT_GENERAL_RATE);
	m_iGreatAdmiralRateModifier += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_GREAT_ADMIRAL_RATE);
	m_iGreatWriterRateModifier += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_GREAT_WRITER_RATE);
	m_iGreatArtistRateModifier += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_GREAT_ARTIST_RATE);
	m_iGreatMusicianRateModifier += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_GREAT_MUSICIAN_RATE);
	m_iGreatMerchantRateModifier += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_GREAT_MERCHANT_RATE);
	m_iGreatScientistRateModifier += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_GREAT_SCIENTIST_RATE);
	m_iDomesticGreatGeneralRateModifier += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_DOMESTIC_GREAT_GENERAL_RATE);

	// Next add in buildings
	m_iGreatPeopleRateModifier += m_iGreatPeopleRateModFromBldgs;
	m_iGreatGeneralRateModifier += m_iGreatGeneralRateModFromBldgs;
	m_iDomesticGreatGeneralRateModifier += m_iDomesticGreatGeneralRateModFromBldgs;

#ifdef SWEDEN_UA_REWORK
	m_iGreatScientistRateModifier += GetGreatPeopleRateModFromFriendships();
	m_iGreatEngineerRateModifier += GetGreatPeopleRateModFromFriendships();
	m_iGreatMerchantRateModifier += GetGreatPeopleRateModFromFriendships();
#else
	// Finally anything from friendships
	m_iGreatPeopleRateModifier += GetGreatPeopleRateModFromFriendships();
#endif

	// And effects from Leagues
	int iArtsyMod = GC.getGame().GetGameLeagues()->GetArtsyGreatPersonRateModifier(GetID());
	int iScienceyMod = GC.getGame().GetGameLeagues()->GetScienceyGreatPersonRateModifier(GetID());
	if (iArtsyMod != 0)
	{
		m_iGreatWriterRateModifier += iArtsyMod;
		m_iGreatArtistRateModifier += iArtsyMod;
		m_iGreatMusicianRateModifier += iArtsyMod;
	}
	if (iScienceyMod != 0)
	{
		m_iGreatScientistRateModifier += iScienceyMod;
		m_iGreatEngineerRateModifier += iScienceyMod;
		m_iGreatMerchantRateModifier += iScienceyMod;
	}

	// Finally boost domestic general from combat experience
	m_iDomesticGreatGeneralRateModifier += GC.getCOMBAT_EXPERIENCE_IN_BORDERS_PERCENT();
}

//	--------------------------------------------------------------------------------
// Do we have a trait that rewards friendships (or have a friend that does)?
int CvPlayer::GetGreatPeopleRateModFromFriendships() const
{
	int iRtnValue = 0;
	int iTraitMod = GetPlayerTraits()->GetDOFGreatPersonModifier();

#ifdef SWEDEN_UA_REWORK
	// Have the trait, one for each friend
	if (iTraitMod != 0)
	{
		iRtnValue = iTraitMod;
	}
#else
	// Have the trait, one for each friend
	if(iTraitMod > 0)
	{
		iRtnValue = GetDiplomacyAI()->GetNumDoF() * iTraitMod;
	}

	// Don't have the trait, just get bonus once if friends with a player that does
	else
	{
		PlayerTypes eLoopPlayer;
		for(int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
		{
			eLoopPlayer = (PlayerTypes) iPlayerLoop;

			if(GetDiplomacyAI()->IsPlayerValid(eLoopPlayer))
			{
				if(GetDiplomacyAI()->IsDoFAccepted(eLoopPlayer))
				{
					int iOthersTraitMod = GET_PLAYER(eLoopPlayer).GetPlayerTraits()->GetDOFGreatPersonModifier();
					if(iOthersTraitMod > 0)
					{
						iRtnValue += iOthersTraitMod;
					}
				}
			}
		}
	}
#endif

	return iRtnValue;
}

//	--------------------------------------------------------------------------------
// Do we get extra beakers from using Great Scientists?
int CvPlayer::GetGreatScientistBeakerMod() const
{
	return m_iGreatScientistBeakerModifier;
}

//	--------------------------------------------------------------------------------
// Do we get extra beakers from using Great Scientists?
void CvPlayer::SetGreatScientistBeakerMod(int iValue)
{
	m_iGreatScientistBeakerModifier = iValue;
}

//	--------------------------------------------------------------------------------
// Do we get extra beakers from using Great Scientists?
void CvPlayer::ChangeGreatScientistBeakerMod(int iChange)
{
	SetGreatScientistBeakerMod(GetGreatScientistBeakerMod() + iChange);
}

//////////////////////////////////////////////////////////////////////////
int CvPlayer::GetGreatGeneralCombatBonus() const
{
	return m_iGreatGeneralCombatBonus;
}

//////////////////////////////////////////////////////////////////////////
void CvPlayer::SetGreatGeneralCombatBonus(int iValue)
{
	m_iGreatGeneralCombatBonus = iValue;
}


//////////////////////////////////////////////////////////////////////////
// ***** Great People Spawning *****
//////////////////////////////////////////////////////////////////////////

//	--------------------------------------------------------------------------------
// Figures out how long before we spawn a free Great Person for ePlayer
void CvPlayer::DoSeedGreatPeopleSpawnCounter()
{
	int iNumTurns = /*37*/ GC.getMINOR_TURNS_GREAT_PEOPLE_SPAWN_BASE();

	// Start at -1 since if we only have one ally we don't want to add any more
#ifdef PATRONAGE_FINISHER_REWORK
	int iExtraAllies = 0;
#else
	int iExtraAllies = -1;
#endif

	PlayerTypes eMinor;
	for(int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
	{
		eMinor = (PlayerTypes) iMinorLoop;

		// Not alive
		if(!GET_PLAYER(eMinor).isAlive())
			continue;

		if(GET_PLAYER(eMinor).GetMinorCivAI()->GetAlly() == GetID())
			iExtraAllies++;
		
#ifdef PATRONAGE_FINISHER_REWORK
		iExtraAllies /= 2;

		iExtraAllies *= 3;
		iExtraAllies /= 2;
#endif
	}

	if(iExtraAllies > 0)
	{
		int iExtraAlliesChange = iExtraAllies* /*-1*/ GC.getMINOR_ADDITIONAL_ALLIES_GP_CHANGE();

#ifndef PATRONAGE_FINISHER_REWORK
		iExtraAlliesChange = max(/*-10*/ GC.getMAX_MINOR_ADDITIONAL_ALLIES_GP_CHANGE(), iExtraAlliesChange);
#endif

		iNumTurns += iExtraAlliesChange;
	}

#ifndef PATRONAGE_FINISHER_REWORK
	int iRand = /*7*/ GC.getMINOR_TURNS_GREAT_PEOPLE_SPAWN_RAND();
	iNumTurns += GC.getGame().getJonRandNum(iRand, "Rand turns for Friendly Minor GreatPeople spawn");
#endif

	// If we're biasing the result then decrease the number of turns
	if(!IsAlliesGreatPersonBiasApplied())
	{
		iNumTurns *= /*50*/ GC.getMINOR_TURNS_GREAT_PEOPLE_SPAWN_BIAS_MULTIPLY();
		iNumTurns /= 100;

		SetAlliesGreatPersonBiasApplied(true);
	}

	// Modify for Game Speed
	iNumTurns *= GC.getGame().getGameSpeedInfo().getGreatPeoplePercent();
	iNumTurns /= 100;

	if(iNumTurns < 1)
		iNumTurns = 1;

	SetGreatPeopleSpawnCounter(iNumTurns);
}

//	--------------------------------------------------------------------------------
/// We're now allies with someone, what happens with the GP bonus?
void CvPlayer::DoApplyNewAllyGPBonus()
{
	int iChange = /*-2*/ GC.getMINOR_ADDITIONAL_ALLIES_GP_CHANGE();
	ChangeGreatPeopleSpawnCounter(iChange);

	if(GetGreatPeopleSpawnCounter() < 1)
		SetGreatPeopleSpawnCounter(1);
}

//	--------------------------------------------------------------------------------
// How long before we spawn a free GreatPeople for ePlayer?
int CvPlayer::GetGreatPeopleSpawnCounter()
{
	return m_iGreatPeopleSpawnCounter;
}

//	--------------------------------------------------------------------------------
// Sets how long before we spawn a free GreatPeople for ePlayer
void CvPlayer::SetGreatPeopleSpawnCounter(int iValue)
{
	m_iGreatPeopleSpawnCounter = iValue;
}

//	--------------------------------------------------------------------------------
// Changes how long before we spawn a free GreatPeople for ePlayer
void CvPlayer::ChangeGreatPeopleSpawnCounter(int iChange)
{
	SetGreatPeopleSpawnCounter(GetGreatPeopleSpawnCounter() + iChange);
}

//	--------------------------------------------------------------------------------
/// Create a GreatPeople
void CvPlayer::DoSpawnGreatPerson(PlayerTypes eMinor)
{
	CvAssertMsg(eMinor >= MAX_MAJOR_CIVS, "eMinor is expected to be non-negative (invalid Index)");
	CvAssertMsg(eMinor < MAX_CIV_PLAYERS, "eMinor is expected to be within maximum bounds (invalid Index)");

	// Minor must have Capital
	CvCity* pMinorCapital = GET_PLAYER(eMinor).getCapitalCity();
	if(pMinorCapital == NULL)
	{
		FAssertMsg(false, "MINOR CIV AI: Trying to spawn a GreatPeople for a major civ but the minor has no capital. Please send Jon this with your last 5 autosaves and what changelist # you're playing.");
		return;
	}
	// Capital must have a plot
	CvPlot* pMinorPlot = pMinorCapital->plot();
	if(pMinorPlot == NULL)
	{
		CvAssertMsg(false, "Plot for minor civ's capital not found! Please send Anton your save file and version.");
		return;
	}

	// Note: this is the same transport method (though without a delay) as a Militaristic city-state gifting a unit

	CvCity* pMajorCity = GetClosestFriendlyCity(*pMinorPlot, MAX_INT);

	int iX = pMinorCapital->getX();
	int iY = pMinorCapital->getY();
	if(pMajorCity != NULL)
	{
		iX = pMajorCity->getX();
		iY = pMajorCity->getY();
	}

	// Pick Great Person type
	UnitTypes eBestUnit = NO_UNIT;
	int iBestScore = -1;
	SpecialUnitTypes eSpecialUnitGreatPerson = (SpecialUnitTypes) GC.getInfoTypeForString("SPECIALUNIT_PEOPLE");

	for(int iUnitLoop = 0; iUnitLoop < GC.getNumUnitInfos(); iUnitLoop++)
	{
		UnitTypes eLoopUnit = (UnitTypes)iUnitLoop;
		CvUnitEntry* pkUnitEntry = GC.getUnitInfo(eLoopUnit);

		if(pkUnitEntry && pkUnitEntry->GetSpecialUnitType() == eSpecialUnitGreatPerson)
		{
			// No prophets
			if(!pkUnitEntry->IsFoundReligion())
			{
				int iScore = GC.getGame().getJonRandNum(100, "Rand");

				if(iScore > iBestScore)
				{
					iBestScore = iScore;
					eBestUnit = eLoopUnit;
				}
			}
		}
	}

	// Spawn GreatPeople
	if(eBestUnit != NO_UNIT)
	{
		CvUnit* pNewGreatPeople = initUnit(eBestUnit, iX, iY);
		CvAssert(pNewGreatPeople);

		if (pNewGreatPeople)
		{
			// Bump up the count
			if(pNewGreatPeople->IsGreatGeneral())
			{
				incrementGreatGeneralsCreated();
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
				ChangeNumGeneralsTotal(1);
#endif
			}
			else if(pNewGreatPeople->IsGreatAdmiral())
			{
				incrementGreatAdmiralsCreated();
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
				ChangeNumAdmiralsTotal(1);
#endif
			}
			else if (pNewGreatPeople->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_WRITER"))
			{
#ifndef FREE_GREAT_PERSON
				incrementGreatWritersCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
				ChangeNumWritersTotal(1);
#endif
			}							
			else if (pNewGreatPeople->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
			{
#ifndef FREE_GREAT_PERSON
				incrementGreatArtistsCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
				ChangeNumArtistsTotal(1);
#endif
			}							
			else if (pNewGreatPeople->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
			{
#ifndef FREE_GREAT_PERSON
				incrementGreatMusiciansCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
				ChangeNumMusiciansTotal(1);
#endif
			}
#if defined SEPARATE_GREAT_PEOPLE || defined SWEDEN_UA_REWORK
			else if (pNewGreatPeople->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_SCIENTIST"))
			{
#ifndef FREE_GREAT_PERSON
				incrementGreatScientistsCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
				ChangeNumScientistsTotal(1);
#endif
			}							
			else if (pNewGreatPeople->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_ENGINEER"))
			{
#ifndef FREE_GREAT_PERSON
				incrementGreatEngineersCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
				ChangeNumEngineersTotal(1);
#endif
			}							
			else if (pNewGreatPeople->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
			{
#ifndef FREE_GREAT_PERSON
				incrementGreatMerchantsCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
				ChangeNumMerchantsTotal(1);
#endif
			}
			else if (pNewGreatPeople->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_PROPHET"))
			{
#ifndef FREE_GREAT_PERSON
				incrementGreatProphetsCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
				ChangeNumProphetsTotal(1);
#endif
			}
#ifdef SEPARATE_MERCHANTS
			else if (pNewGreatPeople->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
			{
				incrementGreatMerchantsCreated();
			}
#endif
			else
			{
				incrementGreatPeopleCreated();
			}
#endif

			if (pNewGreatPeople->IsGreatAdmiral())
			{
				CvPlot* pSpawnPlot = GetGreatAdmiralSpawnPlot(pNewGreatPeople);
				if (pNewGreatPeople->plot() != pSpawnPlot && pSpawnPlot != NULL)
				{
					pNewGreatPeople->setXY(pSpawnPlot->getX(), pSpawnPlot->getY());
				}
			}
			else
			{
				if (!pNewGreatPeople->jumpToNearestValidPlot())
					pNewGreatPeople->kill(false);	// Could not find a spot!
			}

			CvNotifications* pNotifications = GetNotifications();
			if(pNotifications)
			{
				Localization::String strMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_STATE_UNIT_SPAWN");
				strMessage << GET_PLAYER(eMinor).getNameKey();
				Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_CITY_STATE_UNIT_SPAWN");
				strSummary << GET_PLAYER(eMinor).getNameKey();
				pNotifications->Add(NOTIFICATION_MINOR, strMessage.toUTF8(), strSummary.toUTF8(), iX, iY, eMinor);
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Time to spawn a GreatPeople?
void CvPlayer::DoGreatPeopleSpawnTurn()
{
	// Tick down
	if(GetGreatPeopleSpawnCounter() > 0)
	{
		AI_PERF_FORMAT("AI-perf.csv", ("CvPlayer::DoGreatPeopleSpawnTurn, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()) );
		ChangeGreatPeopleSpawnCounter(-1);

		// Time to spawn! - Pick a random allied minor
		if(GetGreatPeopleSpawnCounter() == 0)
		{
			PlayerTypes eBestMinor = NO_PLAYER;
			int iBestScore = -1;
			int iScore;

			PlayerTypes eMinor;
			for(int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
			{
				eMinor = (PlayerTypes) iMinorLoop;

				// Not alive
				if(!GET_PLAYER(eMinor).isAlive())
					continue;

				// Not an ally
				if(GET_PLAYER(eMinor).GetMinorCivAI()->GetAlly() != GetID())
					continue;

				iScore = GC.getGame().getJonRandNum(100, "Random minor great person gift location.");

				// Best ally yet?
				if(eBestMinor == NO_PLAYER || iScore > iBestScore)
				{
					eBestMinor = eMinor;
					iBestScore = iScore;
				}
			}

			if(eBestMinor != NO_PLAYER)
				DoSpawnGreatPerson(eBestMinor);

			// Reseed counter
			DoSeedGreatPeopleSpawnCounter();
		}
	}
}

//	--------------------------------------------------------------------------------
CvCity* CvPlayer::GetGreatPersonSpawnCity(UnitTypes eUnit)
{
	CvCity* pBestCity = getCapitalCity();
	CvUnitEntry* pkUnitEntry = GC.getUnitInfo(eUnit);

	if(pkUnitEntry && pkUnitEntry->GetDomainType() == DOMAIN_SEA)
	{
		int iBestValue = INT_MAX;
		int iLoop;
		for(CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if(!pLoopCity->isCoastal())
			{
				continue;
			}

			int iValue = 4 * GC.getGame().getJonRandNum(getNumCities(), "Great Admiral City Selection");

			for(int i = 0; i < NUM_YIELD_TYPES; i++)
			{
				iValue += pLoopCity->findYieldRateRank((YieldTypes)i);
			}
			iValue += pLoopCity->findPopulationRank();

			if(iValue < iBestValue)
			{
				pBestCity = pLoopCity;
				iBestValue = iValue;
			}
		}
	}

	return pBestCity;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getMaxGlobalBuildingProductionModifier() const
{
	return m_iMaxGlobalBuildingProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeMaxGlobalBuildingProductionModifier(int iChange)
{
	m_iMaxGlobalBuildingProductionModifier = (m_iMaxGlobalBuildingProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getMaxTeamBuildingProductionModifier() const
{
	return m_iMaxTeamBuildingProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeMaxTeamBuildingProductionModifier(int iChange)
{
	m_iMaxTeamBuildingProductionModifier = (m_iMaxTeamBuildingProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getMaxPlayerBuildingProductionModifier() const
{
	return m_iMaxPlayerBuildingProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeMaxPlayerBuildingProductionModifier(int iChange)
{
	m_iMaxPlayerBuildingProductionModifier = (m_iMaxPlayerBuildingProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getFreeExperience() const
{
	return m_iFreeExperience;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeFreeExperienceFromBldgs(int iChange)
{
	m_iFreeExperienceFromBldgs += iChange;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeFreeExperienceFromMinors(int iChange)
{
	m_iFreeExperienceFromMinors += iChange;
}

//	--------------------------------------------------------------------------------
void CvPlayer::recomputeFreeExperience()
{
	m_iFreeExperience = m_iFreeExperienceFromBldgs;
	m_iFreeExperience = m_iFreeExperienceFromMinors;
	m_iFreeExperience += m_pPlayerPolicies->GetNumericModifier(POLICYMOD_FREE_EXPERIENCE);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getFeatureProductionModifier() const
{
	return m_iFeatureProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeFeatureProductionModifier(int iChange)
{
	m_iFeatureProductionModifier = (m_iFeatureProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getWorkerSpeedModifier() const
{
	return m_iWorkerSpeedModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeWorkerSpeedModifier(int iChange)
{
	m_iWorkerSpeedModifier = (m_iWorkerSpeedModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getImprovementCostModifier() const
{
	return m_iImprovementCostModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeImprovementCostModifier(int iChange)
{
	m_iImprovementCostModifier = (m_iImprovementCostModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getImprovementUpgradeRateModifier() const
{
	return m_iImprovementUpgradeRateModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeImprovementUpgradeRateModifier(int iChange)
{
	m_iImprovementUpgradeRateModifier = (m_iImprovementUpgradeRateModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getSpecialistProductionModifier() const
{
	return m_iSpecialistProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeSpecialistProductionModifier(int iChange)
{
	m_iSpecialistProductionModifier = (m_iSpecialistProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getMilitaryProductionModifier() const
{
	return m_iMilitaryProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeMilitaryProductionModifier(int iChange)
{
	m_iMilitaryProductionModifier = (m_iMilitaryProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getSpaceProductionModifier() const
{
	return m_iSpaceProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeSpaceProductionModifier(int iChange)
{
	m_iSpaceProductionModifier = (m_iSpaceProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getCityDefenseModifier() const
{
	return m_iCityDefenseModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeCityDefenseModifier(int iChange)
{
	m_iCityDefenseModifier = (m_iCityDefenseModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getUnitFortificationModifier() const
{
	return m_iUnitFortificationModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeUnitFortificationModifier(int iChange)
{
	m_iUnitFortificationModifier = (m_iUnitFortificationModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getUnitBaseHealModifier() const
{
	return m_iUnitBaseHealModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeUnitBaseHealModifier(int iChange)
{
	m_iUnitBaseHealModifier = (m_iUnitBaseHealModifier + iChange);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getWonderProductionModifier() const
{
	return m_iWonderProductionModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeWonderProductionModifier(int iChange)
{
	m_iWonderProductionModifier = (m_iWonderProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getSettlerProductionModifier() const
{
	return m_iSettlerProductionModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeSettlerProductionModifier(int iChange)
{
	m_iSettlerProductionModifier = (m_iSettlerProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getCapitalSettlerProductionModifier() const
{
	return m_iCapitalSettlerProductionModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeCapitalSettlerProductionModifier(int iChange)
{
	m_iCapitalSettlerProductionModifier = (m_iCapitalSettlerProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::GetPolicyCostBuildingModifier() const
{
	return m_iPolicyCostBuildingModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangePolicyCostBuildingModifier(int iChange)
{
	if(iChange != 0)
	{
		m_iPolicyCostBuildingModifier = (m_iPolicyCostBuildingModifier + iChange);

		recomputePolicyCostModifier();
		DoUpdateNextPolicyCost();
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetPolicyCostMinorCivModifier() const
{
	return m_iPolicyCostMinorCivModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangePolicyCostMinorCivModifier(int iChange)
{
	m_iPolicyCostMinorCivModifier = (m_iPolicyCostMinorCivModifier + iChange);
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetInfluenceSpreadModifier() const
{
	return m_iInfluenceSpreadModifier;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeInfluenceSpreadModifier(int iChange)
{
	m_iInfluenceSpreadModifier = (m_iInfluenceSpreadModifier + iChange);
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetExtraVotesPerDiplomat() const
{
	return m_iExtraVotesPerDiplomat;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeExtraVotesPerDiplomat(int iChange)
{
	m_iExtraVotesPerDiplomat += iChange;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNumNukeUnits() const
{
	return m_iNumNukeUnits;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeNumNukeUnits(int iChange)
{
	m_iNumNukeUnits = (m_iNumNukeUnits + iChange);
	CvAssert(getNumNukeUnits() >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getNumOutsideUnits()
{
	int iOutsideUnitCount = 0;

	CvUnit* pLoopUnit;
	int iLoop;
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->plot() != NULL)
		{
			if(pLoopUnit->plot()->getOwner() != pLoopUnit->getOwner())
			{
				iOutsideUnitCount++;
			}
		}
	}

	return iOutsideUnitCount;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeNumOutsideUnits(int iChange)
{
	if(iChange != 0)
	{
		m_iNumOutsideUnits += iChange;
		CvAssert(getNumOutsideUnits() >= 0);

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getBaseFreeUnits() const
{
	return m_iBaseFreeUnits;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeBaseFreeUnits(int iChange)
{
	if(iChange != 0)
	{
		m_iBaseFreeUnits = (m_iBaseFreeUnits + iChange);

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetVerifiedOutsideUnitCount()
{
	int iOutsideUnitCount = 0;

	CvUnit* pLoopUnit;
	int iLoop;
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->plot() != NULL)
		{
			if(pLoopUnit->plot()->getOwner() != pLoopUnit->getOwner())
			{
				iOutsideUnitCount++;
			}
		}
	}

	return iOutsideUnitCount;
}


//	--------------------------------------------------------------------------------
int CvPlayer::getGoldPerUnit() const
{
	return getGoldPerUnitTimes100() / 100;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeGoldPerUnit(int iChange)
{
	if(iChange != 0)
	{
		changeGoldPerUnitTimes100(iChange * 100);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGoldPerUnitTimes100() const
{
	return m_iGoldPerUnit;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeGoldPerUnitTimes100(int iChange)
{
	if(iChange != 0)
	{
		m_iGoldPerUnit = (m_iGoldPerUnit + iChange);

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getGoldPerMilitaryUnit() const
{
	return m_iGoldPerMilitaryUnit;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeGoldPerMilitaryUnit(int iChange)
{
	if(iChange != 0)
	{
		m_iGoldPerMilitaryUnit = (m_iGoldPerMilitaryUnit + iChange);

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetRouteGoldMaintenanceMod() const
{
	return m_iRouteGoldMaintenanceMod;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeRouteGoldMaintenanceMod(int iChange)
{
	if(iChange != 0)
	{
		m_iRouteGoldMaintenanceMod = (m_iRouteGoldMaintenanceMod + iChange);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetBuildingGoldMaintenanceMod() const
{
	return m_iBuildingGoldMaintenanceMod;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeBuildingGoldMaintenanceMod(int iChange)
{
	if(iChange != 0)
	{
		m_iBuildingGoldMaintenanceMod = (m_iBuildingGoldMaintenanceMod + iChange);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetUnitGoldMaintenanceMod() const
{
	return m_iUnitGoldMaintenanceMod;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeUnitGoldMaintenanceMod(int iChange)
{
	if(iChange != 0)
	{
		m_iUnitGoldMaintenanceMod = (m_iUnitGoldMaintenanceMod + iChange);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetUnitSupplyMod() const
{
	return m_iUnitSupplyMod;
}


//	--------------------------------------------------------------------------------
void CvPlayer::ChangeUnitSupplyMod(int iChange)
{
	if(iChange != 0)
	{
		m_iUnitSupplyMod = (m_iUnitSupplyMod + iChange);
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getExtraUnitCost() const
{
	return m_iExtraUnitCost;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeExtraUnitCost(int iChange)
{
	if(iChange != 0)
	{
		m_iExtraUnitCost = (m_iExtraUnitCost + iChange);

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNumMaintenanceFreeUnits(DomainTypes eDomain, bool bOnlyCombatUnits) const
{
	int iNumFreeUnits = 0;

	// Loop through all units to see if any of them are free!
	const CvUnit* pLoopUnit;
	int iLoop;
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if (eDomain != NO_DOMAIN)
		{
			if (pLoopUnit->getDomainType() != eDomain)
			{
				continue;
			}
		}

		if (bOnlyCombatUnits)
		{
			if (!pLoopUnit->IsCombatUnit())
			{
				continue;
			}
		}

		if(pLoopUnit->getUnitInfo().IsNoMaintenance())
		{
			iNumFreeUnits++;
		}
		else if(IsGarrisonFreeMaintenance() && pLoopUnit->IsGarrisoned())
		{
			iNumFreeUnits++;
		}
	}

	return iNumFreeUnits;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNumMilitaryUnits() const
{
	return m_iNumMilitaryUnits;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeNumMilitaryUnits(int iChange)
{
	if(iChange != 0)
	{
		m_iNumMilitaryUnits = (m_iNumMilitaryUnits + iChange);
		CvAssert(getNumMilitaryUnits() >= 0);

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getHappyPerMilitaryUnit() const
{
	return m_iHappyPerMilitaryUnit;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeHappyPerMilitaryUnit(int iChange)
{
	if(iChange != 0)
	{
		m_iHappyPerMilitaryUnit = (m_iHappyPerMilitaryUnit + iChange);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getHappinessToCulture() const
{
	return m_iHappinessToCulture;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeHappinessToCulture(int iChange)
{
	if(iChange != 0)
	{
		m_iHappinessToCulture = (m_iHappinessToCulture + iChange);
		CvAssert(getHappinessToCulture() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getHappinessToScience() const
{
	return m_iHappinessToScience;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeHappinessToScience(int iChange)
{
	if(iChange != 0)
	{
		m_iHappinessToScience = (m_iHappinessToScience + iChange);
		CvAssert(getHappinessToScience() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getHalfSpecialistUnhappinessCount() const
{
	return m_iHalfSpecialistUnhappinessCount;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isHalfSpecialistUnhappiness() const
{
	return (getHalfSpecialistUnhappinessCount() > 0);
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeHalfSpecialistUnhappinessCount(int iChange)
{
	if(iChange != 0)
	{
		m_iHalfSpecialistUnhappinessCount = (m_iHalfSpecialistUnhappinessCount + iChange);
		CvAssert(getHalfSpecialistUnhappinessCount() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getHalfSpecialistFoodCount() const
{
	return m_iHalfSpecialistFoodCount;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isHalfSpecialistFood() const
{
	return (getHalfSpecialistFoodCount() > 0);
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeHalfSpecialistFoodCount(int iChange)
{
	if(iChange != 0)
	{
		m_iHalfSpecialistFoodCount = (m_iHalfSpecialistFoodCount + iChange);
		CvAssert(getHalfSpecialistFoodCount() >= 0);
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getMilitaryFoodProductionCount() const
{
	return m_iMilitaryFoodProductionCount;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isMilitaryFoodProduction() const
{
	return (getMilitaryFoodProductionCount() > 0);
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeMilitaryFoodProductionCount(int iChange)
{
	if(iChange != 0)
	{
		m_iMilitaryFoodProductionCount = (m_iMilitaryFoodProductionCount + iChange);
		CvAssert(getMilitaryFoodProductionCount() >= 0);

		if(getTeam() == GC.getGame().getActiveTeam())
		{
			GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetGoldenAgeCultureBonusDisabledCount() const
{
	return m_iGoldenAgeCultureBonusDisabledCount;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsGoldenAgeCultureBonusDisabled() const
{
	return (GetGoldenAgeCultureBonusDisabledCount() > 0);
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeGoldenAgeCultureBonusDisabledCount(int iChange)
{
	if (iChange != 0)
	{
		m_iGoldenAgeCultureBonusDisabledCount = m_iGoldenAgeCultureBonusDisabledCount + iChange;
		CvAssert(GetGoldenAgeCultureBonusDisabledCount() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetSecondReligionPantheonCount() const
{
	return m_iSecondReligionPantheonCount;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsSecondReligionPantheon() const
{
	return (GetSecondReligionPantheonCount() > 0);
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeSecondReligionPantheonCount(int iChange)
{
	if (iChange != 0)
	{
		m_iSecondReligionPantheonCount = m_iSecondReligionPantheonCount + iChange;
		CvAssert(GetSecondReligionPantheonCount() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetEnablesSSPartHurryCount() const
{
	return m_iEnablesSSPartHurryCount;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsEnablesSSPartHurry() const
{
	return (GetEnablesSSPartHurryCount() > 0);
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeEnablesSSPartHurryCount(int iChange)
{
	if (iChange != 0)
	{
		m_iEnablesSSPartHurryCount = m_iEnablesSSPartHurryCount + iChange;
		CvAssert(GetEnablesSSPartHurryCount() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetEnablesSSPartPurchaseCount() const
{
	return m_iEnablesSSPartPurchaseCount;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsEnablesSSPartPurchase() const
{
	return (GetEnablesSSPartPurchaseCount() > 0);
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeEnablesSSPartPurchaseCount(int iChange)
{
	if (iChange != 0)
	{
		m_iEnablesSSPartPurchaseCount = m_iEnablesSSPartPurchaseCount + iChange;
		CvAssert(GetEnablesSSPartPurchaseCount() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getHighestUnitLevel()	const
{
	return m_iHighestUnitLevel;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setHighestUnitLevel(int iNewValue)
{
	m_iHighestUnitLevel = iNewValue;
	CvAssert(getHighestUnitLevel() >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getMaxConscript() const
{
	return m_iMaxConscript;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeMaxConscript(int iChange)
{
	m_iMaxConscript = (m_iMaxConscript + iChange);
	CvAssert(getMaxConscript() >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getConscriptCount() const
{
	return m_iConscriptCount;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setConscriptCount(int iNewValue)
{
	m_iConscriptCount = iNewValue;
	CvAssert(getConscriptCount() >= 0);
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeConscriptCount(int iChange)
{
	setConscriptCount(getConscriptCount() + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getOverflowResearch() const
{
	return m_iOverflowResearch / 100;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setOverflowResearch(int iNewValue)
{
	setOverflowResearchTimes100(iNewValue*100);
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeOverflowResearch(int iChange)
{
	changeOverflowResearchTimes100(iChange*100);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getOverflowResearchTimes100() const
{
	return m_iOverflowResearch;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setOverflowResearchTimes100(int iNewValue)
{
	m_iOverflowResearch = iNewValue;
	CvAssert(getOverflowResearchTimes100() >= 0);
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeOverflowResearchTimes100(int iChange)
{
	setOverflowResearchTimes100(getOverflowResearchTimes100() + iChange);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getExpModifier() const
{
	return m_iExpModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeExpModifier(int iChange)
{
	if(iChange != 0)
	{
		m_iExpModifier += iChange;
		CvAssert(getExpModifier() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getExpInBorderModifier() const
{
	return m_iExpInBorderModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeExpInBorderModifier(int iChange)
{
	if(iChange != 0)
	{
		m_iExpInBorderModifier += iChange;
		CvAssert(getExpInBorderModifier() >= 0);
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getLevelExperienceModifier() const
{
	return m_iLevelExperienceModifier;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeLevelExperienceModifier(int iChange)
{
	m_iLevelExperienceModifier += iChange;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getMinorQuestFriendshipMod() const
{
	return m_iMinorQuestFriendshipMod;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeMinorQuestFriendshipMod(int iChange)
{
	if(iChange != 0)
	{
		m_iMinorQuestFriendshipMod += iChange;
		CvAssert(getMinorQuestFriendshipMod() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getMinorGoldFriendshipMod() const
{
	return m_iMinorGoldFriendshipMod;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeMinorGoldFriendshipMod(int iChange)
{
	if(iChange != 0)
	{
		m_iMinorGoldFriendshipMod += iChange;
		CvAssert(getMinorGoldFriendshipMod() >= 0);
	}
}

//	--------------------------------------------------------------------------------
/// What is the general modifier we get towards the resting Influence point with a city-state? (ex. Social Policies)
/// NOTE: This does not include situation-dependent modifiers (ex. religion or warmongering), which are handled in CvMinorCivAI
int CvPlayer::GetMinorFriendshipAnchorMod() const
{
	return m_iMinorFriendshipMinimum;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetMinorFriendshipAnchorMod(int iValue)
{
	if (iValue < GC.getMINOR_FRIENDSHIP_AT_WAR())
	{
		CvAssertMsg(false, "Minor friendship anchor mod should not be lower than the War friendship level. Please send Anton your save file and version.");
		m_iMinorFriendshipMinimum = GC.getMINOR_FRIENDSHIP_AT_WAR();
		return;
	}

	m_iMinorFriendshipMinimum = iValue;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeMinorFriendshipAnchorMod(int iChange)
{
	SetMinorFriendshipAnchorMod(GetMinorFriendshipAnchorMod() + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::GetMinorFriendshipDecayMod() const
{
	return m_iMinorFriendshipDecayMod;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeGetMinorFriendshipDecayMod(int iChange)
{
	if(iChange != 0)
	{
		m_iMinorFriendshipDecayMod += iChange;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsMinorScienceAllies() const
{
	return GetMinorScienceAlliesCount() > 0;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetMinorScienceAlliesCount() const
{
	return m_iMinorScienceAlliesCount;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeMinorScienceAlliesCount(int iChange)
{
	if(iChange != 0)
	{
		m_iMinorScienceAlliesCount += iChange;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsMinorResourceBonus() const
{
	return GetMinorResourceBonusCount() > 0;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetMinorResourceBonusCount() const
{
	return m_iMinorResourceBonusCount;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeMinorResourceBonusCount(int iChange)
{
	if(iChange != 0)
	{
		m_iMinorResourceBonusCount += iChange;
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsAbleToAnnexCityStates() const
{
	if (GetAbleToAnnexCityStatesCount() > 0)
		return true;

	if (GetPlayerTraits()->IsAbleToAnnexCityStates())
		return true;

	return false;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetAbleToAnnexCityStatesCount() const
{
	return m_iAbleToAnnexCityStatesCount;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeAbleToAnnexCityStatesCount(int iChange)
{
	m_iAbleToAnnexCityStatesCount += iChange;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getCultureBombTimer() const
{
	return m_iCultureBombTimer;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setCultureBombTimer(int iNewValue)
{
	if(getCultureBombTimer() != iNewValue)
	{
		m_iCultureBombTimer = iNewValue;
		CvAssert(getCultureBombTimer() >= 0);
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeCultureBombTimer(int iChange)
{
	setCultureBombTimer(getCultureBombTimer() + iChange);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getConversionTimer() const
{
	return m_iConversionTimer;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setConversionTimer(int iNewValue)
{
	if(getConversionTimer() != iNewValue)
	{
		m_iConversionTimer = iNewValue;
		CvAssert(getConversionTimer() >= 0);

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(MiscButtons_DIRTY_BIT, true);
		}
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeConversionTimer(int iChange)
{
	setConversionTimer(getConversionTimer() + iChange);
}

//	--------------------------------------------------------------------------------
CvCity* CvPlayer::getCapitalCity()
{
	return getCity(m_iCapitalCityID);
}

//	--------------------------------------------------------------------------------
const CvCity* CvPlayer::getCapitalCity() const
{
	return getCity(m_iCapitalCityID);
}

//	--------------------------------------------------------------------------------
void CvPlayer::setCapitalCity(CvCity* pNewCapitalCity)
{
	CvCity* pOldCapitalCity;

	pOldCapitalCity = getCapitalCity();

	if(pOldCapitalCity != pNewCapitalCity)
	{
		if(pNewCapitalCity != NULL)
		{
			// Need to set our original capital x,y?
			if(GetOriginalCapitalX() == -1 || GetOriginalCapitalY() == -1)
			{
				m_iOriginalCapitalX = pNewCapitalCity->getX();
				m_iOriginalCapitalY = pNewCapitalCity->getY();
			}

			m_iCapitalCityID = pNewCapitalCity->GetID();

			pNewCapitalCity->SetEverCapital(true);
		}
		else
		{
			m_iCapitalCityID = FFreeList::INVALID_INDEX;
		}
	}
}

//	--------------------------------------------------------------------------------
/// Where was our original capital located?
int CvPlayer::GetOriginalCapitalX() const
{
	return m_iOriginalCapitalX;
}

//	--------------------------------------------------------------------------------
/// Where was our original capital located?
int CvPlayer::GetOriginalCapitalY() const
{
	return m_iOriginalCapitalY;
}

//	--------------------------------------------------------------------------------
/// Have we lost our capital in war?
bool CvPlayer::IsHasLostCapital() const
{
	return m_bLostCapital;
}

//	--------------------------------------------------------------------------------
/// Sets us to having lost our capital in war
void CvPlayer::SetHasLostCapital(bool bValue, PlayerTypes eConqueror)
{
	if(bValue != m_bLostCapital)
	{
		m_bLostCapital = bValue;
		m_eConqueror = eConqueror;

		// Don't really care if a City State lost its capital
		if(!isMinorCiv())
		{
			int iMostOriginalCapitals = 0;
			TeamTypes eWinningTeam = NO_TEAM;
			PlayerTypes eWinningPlayer = NO_PLAYER;

			{
				// Calculate who owns the most original capitals by iterating through all civs 
				// and finding out who owns their original capital.
				typedef std::tr1::array<int, MAX_CIV_TEAMS> CivTeamArray;
				CivTeamArray aTeamCityCount;
				aTeamCityCount.assign(0);

				CvMap& kMap = GC.getMap();
				for (int iLoopPlayer = 0; iLoopPlayer < MAX_MAJOR_CIVS; ++iLoopPlayer)
				{
					const PlayerTypes ePlayer = static_cast<PlayerTypes>(iLoopPlayer);
					CvPlayer& kLoopPlayer = GET_PLAYER(ePlayer);
					if(kLoopPlayer.isEverAlive())
					{
						const int iOriginalCapitalX = kLoopPlayer.GetOriginalCapitalX();
						const int iOriginalCapitalY = kLoopPlayer.GetOriginalCapitalY();
						if(iOriginalCapitalX != -1 && iOriginalCapitalY != -1)
						{
							CvPlot* pkPlot = kMap.plot(iOriginalCapitalX, iOriginalCapitalY);
							if(pkPlot != NULL)
							{
								CvCity* pkCapitalCity = pkPlot->getPlotCity();
								if(pkCapitalCity != NULL)
								{
									const PlayerTypes eCapitalOwner = pkCapitalCity->getOwner();
									if(eCapitalOwner != NO_PLAYER)
									{
										CvPlayer& kCapitalOwnerPlayer = GET_PLAYER(eCapitalOwner);
										aTeamCityCount[kCapitalOwnerPlayer.getTeam()]++;
									}
								}
							}	
						}
					}
				}

				// What's the max count and are they the only team to have the max?
				CivTeamArray::iterator itMax = max_element(aTeamCityCount.begin(), aTeamCityCount.end());
				if(count(aTeamCityCount.begin(), aTeamCityCount.end(), *itMax) == 1)
				{
					eWinningTeam = static_cast<TeamTypes>(itMax - aTeamCityCount.begin());
					iMostOriginalCapitals = *itMax;

					CvTeam& kTeam = GET_TEAM(eWinningTeam);
					eWinningPlayer = kTeam.getLeaderID();
				}			
			}

			// Someone just lost their capital, test to see if someone wins
			if(bValue)
			{
				// slewis - Moved Conquest victory elsewhere so that victory is more accurately awarded
				//GC.getGame().DoTestConquestVictory();

				Localization::String localizedBuffer;
				Localization::String localizedSummary;
				NotificationTypes eNotificationType = NOTIFICATION_CAPITAL_LOST;

				for(uint ui = 0; ui < MAX_MAJOR_CIVS; ui++)
				{
					PlayerTypes ePlayer = (PlayerTypes)ui;
					CvNotifications* pNotifications = GET_PLAYER(ePlayer).GetNotifications();
					if(!pNotifications)
					{
						continue;
					}

					// Notify Player lost their capital
					if(ePlayer == GetID())
					{
						eNotificationType = NOTIFICATION_CAPITAL_LOST_ACTIVE_PLAYER;
						localizedSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_YOU_LOST_CAPITAL");
						if (eWinningPlayer == ePlayer)
						{
							localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_LOST_CAPITAL_YOU_WINNING");
							localizedBuffer << iMostOriginalCapitals;
						}
						else if (eWinningTeam != NO_TEAM)
						{
							if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasMet(eWinningTeam))
							{
								if (eWinningPlayer != NO_PLAYER) // there is a winning player
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_LOST_CAPITAL_OTHER_PLAYER_WINNING");
									if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(eWinningPlayer).isHuman())
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNickName();
									}
									else
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNameKey();
									}
									localizedBuffer << iMostOriginalCapitals;
								}
								else
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_LOST_CAPITAL_TEAM_WINNING");
									localizedBuffer << (int)eWinningTeam;
									localizedBuffer << iMostOriginalCapitals;
								}
							}
							else // if someone is winning
							{
								localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_LOST_CAPITAL_UNMET_WINNING");
								localizedBuffer << iMostOriginalCapitals;
							}
						}
						else // if no one is winning
						{
							localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_LOST_CAPITAL");
						}
					}
					// Known player
					else if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasMet(getTeam()))
					{
						eNotificationType = NOTIFICATION_CAPITAL_LOST;
						localizedSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_PLAYER_LOST_CAPITAL");
						localizedSummary << getCivilizationShortDescriptionKey();

						if (eWinningTeam != NO_TEAM)
						{
							if (GET_TEAM(eWinningTeam).isHasMet(getTeam()))
							{
								if (eWinningPlayer == GetID())
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_LOST_CAPITAL_YOU_WINNING");

									if (GC.getGame().isGameMultiPlayer() && isHuman())
									{
										localizedBuffer << getNickName();
									}
									else
									{
										localizedBuffer << getNameKey();
									}
									localizedBuffer << iMostOriginalCapitals;
								}
								else if (eWinningPlayer != NO_PLAYER)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_LOST_CAPITAL_OTHER_WINNING");
									if (GC.getGame().isGameMultiPlayer() && isHuman())
									{
										localizedBuffer << getNickName();
									}
									else
									{
										localizedBuffer << getNameKey();
									}

									if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(eWinningPlayer).isHuman())
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNickName();
									}
									else
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNameKey();
									}
									localizedBuffer << iMostOriginalCapitals;
								}
								else // if (eWinningTeam != NO_TEAM)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_LOST_CAPITAL_TEAM_WINNING");
									if (GC.getGame().isGameMultiPlayer() && isHuman())
									{
										localizedBuffer << getNickName();
									}
									else
									{
										localizedBuffer << getNameKey();
									}

									localizedBuffer << (int)eWinningTeam;
									localizedBuffer << iMostOriginalCapitals;
								}
							}
							else
							{
								localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_LOST_CAPITAL_UNMET_WINNING");
								if (GC.getGame().isGameMultiPlayer() && isHuman())
								{
									localizedBuffer << getNickName();
								}
								else
								{
									localizedBuffer << getNameKey();
								}
								localizedBuffer << iMostOriginalCapitals;
							}
						}
						else
						{
							localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_LOST_CAPITAL");
							if (GC.getGame().isGameMultiPlayer() && isHuman())
							{
								localizedBuffer << getNickName();
							}
							else
							{
								localizedBuffer << getNameKey();
							}
						}
					}
					else // unmet player
					{
						eNotificationType = NOTIFICATION_CAPITAL_LOST;
						localizedSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_SOMEONE_LOST_CAPITAL");

						if (eWinningTeam != NO_TEAM)
						{
							if (GET_TEAM(eWinningTeam).isHasMet(getTeam()))
							{
								if (eWinningPlayer == GetID())
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_LOST_CAPITAL_YOU_WINNING");
									localizedBuffer << iMostOriginalCapitals;
								}
								else if (eWinningPlayer != NO_PLAYER)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_LOST_CAPITAL_OTHER_WINNING");
									if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(eWinningPlayer).isHuman())
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNickName();
									}
									else
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNameKey();
									}
									localizedBuffer << iMostOriginalCapitals;
								}
								else // if (eWinningTeam != NO_TEAM)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_LOST_CAPITAL_TEAM_WINNING");
									localizedBuffer << (int)eWinningTeam;
									localizedBuffer << iMostOriginalCapitals;
								}
							}
							else
							{
								localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_LOST_CAPITAL_UNMET_WINNING");
								localizedBuffer << iMostOriginalCapitals;
							}
						}
						else
						{
							localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_LOST_CAPITAL");
						}
					}

					pNotifications->Add(eNotificationType, localizedBuffer.toUTF8(), localizedSummary.toUTF8(), -1, -1, -1);
				}

				//replay message
				{
					Localization::String message;
					if (eWinningPlayer != NO_PLAYER)
					{
						message = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_LOST_CAPITAL_OTHER_WINNING");
						if (GC.getGame().isGameMultiPlayer() && isHuman())
						{
							localizedBuffer << getNickName();
						}
						else
						{
							localizedBuffer << getNameKey();
						}

						if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(eWinningPlayer).isHuman())
						{
							localizedBuffer << GET_PLAYER(eWinningPlayer).getNickName();
						}
						else
						{
							localizedBuffer << GET_PLAYER(eWinningPlayer).getNameKey();
						}

						localizedBuffer << iMostOriginalCapitals;
					}
					else if (eWinningTeam != NO_TEAM)
					{
						message = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_LOST_CAPITAL_TEAM_WINNING");
						if (GC.getGame().isGameMultiPlayer() && isHuman())
						{
							localizedBuffer << getNickName();
						}
						else
						{
							localizedBuffer << getNameKey();
						}

						localizedBuffer << (int)eWinningTeam;
						localizedBuffer << iMostOriginalCapitals;
					}
					else
					{
						message = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_LOST_CAPITAL");
						if (GC.getGame().isGameMultiPlayer() && isHuman())
						{
							localizedBuffer << getNickName();
						}
						else
						{
							localizedBuffer << getNameKey();
						}
					}

					CvString translatedMessage = message.toUTF8();
					GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GetID(), translatedMessage, GetOriginalCapitalX(), GetOriginalCapitalY());
				}					
			}
			// Player recovered capital!
			else
			{
				Localization::String localizedBuffer;
				Localization::String localizedSummary;

				for(uint ui = 0; ui < MAX_MAJOR_CIVS; ui++)
				{
					PlayerTypes ePlayer = (PlayerTypes)ui;
					CvNotifications* pNotifications = GET_PLAYER(ePlayer).GetNotifications();
					if(!pNotifications)
					{
						continue;
					}

					// Notify Player lost their capital
					if(ePlayer == GetID())
					{
						localizedSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_RECOVERED_CAPITAL");
						if (eWinningPlayer == ePlayer)
						{
							localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_REGAINED_CAPITAL_YOU_WINNING");
							localizedBuffer << iMostOriginalCapitals;
						}
						else if (GET_TEAM(getTeam()).isHasMet(eWinningTeam))
						{
							if (eWinningPlayer != NO_PLAYER) // there is a winning player
							{
								localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_REGAINED_CAPITAL_OTHER_PLAYER_WINNING");
								if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(eWinningPlayer).isHuman())
								{
									localizedBuffer << GET_PLAYER(eWinningPlayer).getNickName();
								}
								else
								{
									localizedBuffer << GET_PLAYER(eWinningPlayer).getNameKey();
								}
								localizedBuffer << iMostOriginalCapitals;
							}
							else
							{
								localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_REGAINED_CAPITAL_TEAM_WINNING");
								localizedBuffer << (int)eWinningTeam;
								localizedBuffer << iMostOriginalCapitals;
							}
						}
						else if (eWinningTeam != NO_TEAM) // if someone is winning
						{
							localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_REGAINED_CAPITAL_UNMET_WINNING");
							localizedBuffer << iMostOriginalCapitals;
						}
						else // if no one is winning
						{
							localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_YOU_RECOVERED_CAPITAL");
						}
					}
					// Known player
					else if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasMet(getTeam()))
					{
						localizedSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_PLAYER_RECOVERED_CAPITAL");
						localizedSummary << getCivilizationShortDescriptionKey();

						if (eWinningTeam != NO_TEAM)
						{
							if (GET_TEAM(eWinningTeam).isHasMet(getTeam()))
							{
								if (eWinningPlayer == ePlayer)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_REGAINED_CAPITAL_YOU_WINNING");
									localizedBuffer << iMostOriginalCapitals;
								}
								else if (eWinningPlayer != NO_PLAYER)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_REGAINED_CAPITAL_OTHER_WINNING");
									if (GC.getGame().isGameMultiPlayer() && isHuman())
									{
										localizedBuffer << getNickName();
									}
									else
									{
										localizedBuffer << getNameKey();
									}

									if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(eWinningPlayer).isHuman())
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNickName();
									}
									else
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNameKey();
									}
									localizedBuffer << iMostOriginalCapitals;
								}
								else // if (eWinningTeam != NO_TEAM)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_REGAINED_CAPITAL_TEAM_WINNING");
									if (GC.getGame().isGameMultiPlayer() && isHuman())
									{
										localizedBuffer << getNickName();
									}
									else
									{
										localizedBuffer << getNameKey();
									}

									localizedBuffer << (int)eWinningTeam;
									localizedBuffer << iMostOriginalCapitals;
								}
							}
							else
							{
								localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_REGAINED_CAPITAL_UNMET_WINNING");
								if (GC.getGame().isGameMultiPlayer() && isHuman())
								{
									localizedBuffer << getNickName();
								}
								else
								{
									localizedBuffer << getNameKey();
								}
								localizedBuffer << iMostOriginalCapitals;
							}
						}
						else
						{
							localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_REGAINED_CAPITAL");
							if (GC.getGame().isGameMultiPlayer() && isHuman())
							{
								localizedBuffer << getNickName();
							}
							else
							{
								localizedBuffer << getNameKey();
							}
						}
					}
					else // unmet player
					{
						localizedSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_SOMEONE_RECOVERED_CAPITAL");

						if (eWinningTeam != NO_TEAM)
						{
							if (GET_TEAM(eWinningTeam).isHasMet(getTeam()))
							{
								if (eWinningPlayer == ePlayer)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_REGAINED_CAPITAL_YOU_WINNING");
									localizedBuffer << iMostOriginalCapitals;
								}
								else if (eWinningPlayer != NO_PLAYER)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_REGAINED_CAPITAL_OTHER_WINNING");
									if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(eWinningPlayer).isHuman())
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNickName();
									}
									else
									{
										localizedBuffer << GET_PLAYER(eWinningPlayer).getNameKey();
									}
									localizedBuffer << iMostOriginalCapitals;
								}
								else // if (eWinningTeam != NO_TEAM)
								{
									localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_REGAINED_CAPITAL_TEAM_WINNING");
									localizedBuffer << (int)eWinningTeam;
									localizedBuffer << iMostOriginalCapitals;
								}
							}
							else
							{
								localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_REGAINED_CAPITAL_UNMET_WINNING");
								localizedBuffer << iMostOriginalCapitals;
							}
						}
						else
						{
							localizedBuffer = Localization::Lookup("TXT_KEY_NOTIFICATION_UNMET_REGAINED_CAPITAL");
						}
					}

					pNotifications->Add(NOTIFICATION_CAPITAL_RECOVERED, localizedBuffer.toUTF8(), localizedSummary.toUTF8(), -1, -1, -1);
				}

				//replay message
				{
					Localization::String message;
					if (eWinningPlayer != NO_PLAYER)
					{
						message = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_REGAINED_CAPITAL_OTHER_WINNING");
						if (GC.getGame().isGameMultiPlayer() && isHuman())
						{
							localizedBuffer << getNickName();
						}
						else
						{
							localizedBuffer << getNameKey();
						}

						if(GC.getGame().isGameMultiPlayer() && GET_PLAYER(eWinningPlayer).isHuman())
						{
							localizedBuffer << GET_PLAYER(eWinningPlayer).getNickName();
						}
						else
						{
							localizedBuffer << GET_PLAYER(eWinningPlayer).getNameKey();
						}

						localizedBuffer << iMostOriginalCapitals;
					}
					else if (eWinningTeam != NO_TEAM)
					{
						message = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_REGAINED_CAPITAL_TEAM_WINNING");
						if (GC.getGame().isGameMultiPlayer() && isHuman())
						{
							localizedBuffer << getNickName();
						}
						else
						{
							localizedBuffer << getNameKey();
						}

						localizedBuffer << (int)eWinningTeam;
						localizedBuffer << iMostOriginalCapitals;
					}
					else
					{
						message = Localization::Lookup("TXT_KEY_NOTIFICATION_OTHER_REGAINED_CAPITAL");
						if (GC.getGame().isGameMultiPlayer() && isHuman())
						{
							localizedBuffer << getNickName();
						}
						else
						{
							localizedBuffer << getNameKey();
						}
					}

					CvString translatedMessage = message.toUTF8();
					GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GetID(), translatedMessage, GetOriginalCapitalX(), GetOriginalCapitalY());
				}					
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Player who first captured our capital
PlayerTypes CvPlayer::GetCapitalConqueror() const
{
	return m_eConqueror;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getCitiesLost() const
{
	return m_iCitiesLost;
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeCitiesLost(int iChange)
{
	m_iCitiesLost = (m_iCitiesLost + iChange);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getPower() const
{
	if(m_iTurnMightRecomputed < GC.getGame().getElapsedGameTurns())
	{
		// more lazy evaluation
		const_cast<CvPlayer*>(this)->m_iTurnMightRecomputed = GC.getGame().getElapsedGameTurns();
		const_cast<CvPlayer*>(this)->m_iMilitaryMight = calculateMilitaryMight();
		const_cast<CvPlayer*>(this)->m_iEconomicMight = calculateEconomicMight();
	}
	return m_iMilitaryMight + m_iEconomicMight;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetMilitaryMight() const
{
	if(m_iTurnMightRecomputed < GC.getGame().getElapsedGameTurns())
	{
		// more lazy evaluation
		const_cast<CvPlayer*>(this)->m_iTurnMightRecomputed = GC.getGame().getElapsedGameTurns();
		const_cast<CvPlayer*>(this)->m_iMilitaryMight = calculateMilitaryMight();
		const_cast<CvPlayer*>(this)->m_iEconomicMight = calculateEconomicMight();
	}
	return m_iMilitaryMight;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetEconomicMight() const
{
	if(m_iTurnMightRecomputed < GC.getGame().getElapsedGameTurns())
	{
		// more lazy evaluation
		const_cast<CvPlayer*>(this)->m_iTurnMightRecomputed = GC.getGame().getElapsedGameTurns();
		const_cast<CvPlayer*>(this)->m_iMilitaryMight = calculateMilitaryMight();
		const_cast<CvPlayer*>(this)->m_iEconomicMight = calculateEconomicMight();
	}
	return m_iEconomicMight;
}

//	--------------------------------------------------------------------------------
int CvPlayer::calculateMilitaryMight() const
{
	int rtnValue = 0;
	const CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		// Current combat strength or bombard strength, whichever is higher
		int iPower =  pLoopUnit->GetPower();
		if (pLoopUnit->getDomainType() == DOMAIN_SEA)
		{
			iPower /= 2;
		}
		rtnValue += iPower;
	}

	//Simplistic increase based on player's gold
	//500 gold will increase might by 22%, 2000 by 45%, 8000 gold by 90%
	float fGoldMultiplier = 1.0f + (sqrt((float)GetTreasury()->GetGold()) / 100.0f);
	if(fGoldMultiplier > 2.0f) fGoldMultiplier = 2.0f;

	rtnValue = (int)(rtnValue * fGoldMultiplier);

	return rtnValue;
}


//	--------------------------------------------------------------------------------
int CvPlayer::calculateEconomicMight() const
{
	// Default to 5 so that a fluctuation in Population early doesn't swing things wildly
	int iEconomicMight = 5;

	iEconomicMight += getTotalPopulation();

	// todo: add weights to these in an xml
	//iEconomicMight += calculateTotalYield(YIELD_FOOD);
	iEconomicMight += calculateTotalYield(YIELD_PRODUCTION);
	//iEconomicMight += calculateTotalYield(YIELD_SCIENCE);
	iEconomicMight += calculateTotalYield(YIELD_GOLD);
	//iEconomicMight += calculateTotalYield(YIELD_CULTURE);
	//iEconomicMight += calculateTotalYield(YIELD_FAITH);

	return iEconomicMight;
}

//	--------------------------------------------------------------------------------
int CvPlayer::calculateProductionMight() const
{
	int iMight = 0;

	const CvCity* pLoopCity;
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iMight += pLoopCity->getRawProductionDifference(/*bIgnoreFood*/ true, /*bOverflow*/ false);
	}

	return iMight;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getCombatExperience() const
{
	return m_iCombatExperience;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setCombatExperience(int iExperience)
{
	CvAssert(iExperience >= 0);

	if(iExperience != getCombatExperience())
	{
		m_iCombatExperience = iExperience;

		// Enough XP for a Great General to appear?
		if(!isBarbarian())
		{
			int iExperienceThreshold = greatGeneralThreshold();
			if(m_iCombatExperience >= iExperienceThreshold && iExperienceThreshold > 0)
			{
				// create great person
				CvCity* pBestCity = NULL;
				int iBestValue = INT_MAX;
				int iLoop;
				for(CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					int iValue = 4 * GC.getGame().getJonRandNum(getNumCities(), "Great General City Selection");

					for(int i = 0; i < NUM_YIELD_TYPES; i++)
					{
						iValue += pLoopCity->findYieldRateRank((YieldTypes)i);
					}
					iValue += pLoopCity->findPopulationRank();

					if(iValue < iBestValue)
					{
						pBestCity = pLoopCity;
						iBestValue = iValue;
					}
				}

				if(pBestCity)
				{
					// Figure out which Promotion is the one which makes a unit a Great General
					PromotionTypes eGreatGeneralPromotion = NO_PROMOTION;
					for(int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
					{
						const PromotionTypes eLoopPromotion = static_cast<PromotionTypes>(iI);
						CvPromotionEntry* pkPromotionInfo = GC.getPromotionInfo(eLoopPromotion);
						if(pkPromotionInfo)
						{
							if(pkPromotionInfo->IsGreatGeneral())
							{
								eGreatGeneralPromotion = eLoopPromotion;
								break;
							}
						}
					}

					// If GG promotion exists, find a unit which gets it for free (i.e. the Great General unit itself)
					if(eGreatGeneralPromotion != NO_PROMOTION)
					{
						for(int iI = 0; iI < GC.getNumUnitInfos(); iI++)
						{
							const UnitTypes eLoopUnit = static_cast<UnitTypes>(iI);
							CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eLoopUnit);
							if(pkUnitInfo)
							{
								if(pkUnitInfo->GetFreePromotions(eGreatGeneralPromotion))
								{
									// Is this the right unit of this class for this civ?
									const UnitTypes eUnit = (UnitTypes) getCivilizationInfo().getCivilizationUnits((UnitClassTypes)pkUnitInfo->GetUnitClassType());

									if(eUnit == eLoopUnit)
									{
										pBestCity->createGreatGeneral(eUnit);
										setCombatExperience(getCombatExperience() - iExperienceThreshold);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeCombatExperience(int iChange)
{
	if(getCombatExperience() + iChange < 0)
	{
		setCombatExperience(0);
	}
	else
	{
		setCombatExperience(getCombatExperience() + iChange);
	}

	m_iLifetimeCombatExperience += iChange;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNavalCombatExperience() const
{
	return m_iNavalCombatExperience;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setNavalCombatExperience(int iExperience)
{
	CvAssert(iExperience >= 0);

	if(iExperience != getNavalCombatExperience())
	{
		m_iNavalCombatExperience = iExperience;

		// Enough XP for a Great Admiral to appear?
		if(!isBarbarian())
		{
			int iExperienceThreshold = greatAdmiralThreshold();
			if(m_iNavalCombatExperience >= iExperienceThreshold && iExperienceThreshold > 0)
			{
				// create great person
				CvCity* pBestCity = NULL;
				int iBestValue = INT_MAX;
				int iLoop;
				for(CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					if(!pLoopCity->isCoastal())
					{
						continue;
					}

					int iValue = 4 * GC.getGame().getJonRandNum(getNumCities(), "Great Admiral City Selection");

					for(int i = 0; i < NUM_YIELD_TYPES; i++)
					{
						iValue += pLoopCity->findYieldRateRank((YieldTypes)i);
					}
					iValue += pLoopCity->findPopulationRank();

					if(iValue < iBestValue)
					{
						pBestCity = pLoopCity;
						iBestValue = iValue;
					}
				}

				if(pBestCity)
				{
					// Figure out which Promotion is the one which makes a unit a Great Admiral
					PromotionTypes eGreatAdmiralPromotion = NO_PROMOTION;
					for(int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
					{
						const PromotionTypes eLoopPromotion = static_cast<PromotionTypes>(iI);
						CvPromotionEntry* pkPromotionInfo = GC.getPromotionInfo(eLoopPromotion);
						if(pkPromotionInfo)
						{
							if(pkPromotionInfo->IsGreatAdmiral())
							{
								eGreatAdmiralPromotion = eLoopPromotion;
							}
						}
					}

					// If GA promotion exists, find a unit which gets it for free (i.e. the Great Admiral unit itself)
					if(eGreatAdmiralPromotion != NO_PROMOTION)
					{
						for(int iI = 0; iI < GC.getNumUnitInfos(); iI++)
						{
							const UnitTypes eLoopUnit = static_cast<UnitTypes>(iI);
							CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eLoopUnit);
							if(pkUnitInfo)
							{
								if(pkUnitInfo->GetFreePromotions(eGreatAdmiralPromotion))
								{
									// Is this the right unit of this class for this civ?
									const UnitTypes eUnit = (UnitTypes) getCivilizationInfo().getCivilizationUnits((UnitClassTypes)pkUnitInfo->GetUnitClassType());

									if(eUnit == eLoopUnit)
									{
										pBestCity->createGreatAdmiral(eUnit);
										setNavalCombatExperience(getNavalCombatExperience() - iExperienceThreshold);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeNavalCombatExperience(int iChange)
{
	if(getNavalCombatExperience() + iChange < 0)
	{
		setNavalCombatExperience(0);
	}
	else
	{
		setNavalCombatExperience(getNavalCombatExperience() + iChange);
	}

	m_iLifetimeCombatExperience += iChange;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getLifetimeCombatExperience() const
{
	return m_iLifetimeCombatExperience;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getBorderObstacleCount() const
{
	return m_iBorderObstacleCount;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isBorderObstacle() const
{
	return (getBorderObstacleCount() > 0);
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeBorderObstacleCount(int iChange)
{
	if(iChange != 0)
	{
		m_iBorderObstacleCount = (m_iBorderObstacleCount + iChange);
		CvAssert(getBorderObstacleCount() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNetID() const
{
	return CvPreGame::netID(GetID());
}

//	--------------------------------------------------------------------------------
void CvPlayer::setNetID(int iNetID)
{
	CvPreGame::setNetID(GetID(), iNetID);
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isConnected() const
{
	return gDLL->IsPlayerConnected(GetID());
}

//	--------------------------------------------------------------------------------
void CvPlayer::sendTurnReminder()
{
	//Send a game invite to the player if they aren't currently connected to the game.
	gDLL->sendTurnReminder(GetID());

	/* email notifications not implimented.
	if(!getPbemEmailAddress().empty() &&
	        !gDLL->GetPitbossSmtpHost().empty())
	{

	}
	*/
}

//	--------------------------------------------------------------------------------
uint CvPlayer::getStartTime() const
{
	return m_uiStartTime;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setStartTime(uint uiStartTime)
{
	m_uiStartTime = uiStartTime;
}


//	--------------------------------------------------------------------------------
uint CvPlayer::getTotalTimePlayed() const
{
	return ((timeGetTime() - m_uiStartTime)/1000);
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isMinorCiv() const
{
	return CvPreGame::isMinorCiv(m_eID);
}


//	--------------------------------------------------------------------------------
/// Has this player betrayed a Minor Civ he was bullying by declaring war on him?
bool CvPlayer::IsHasBetrayedMinorCiv() const
{
	return m_bHasBetrayedMinorCiv;
}

//	--------------------------------------------------------------------------------
/// Sets this player to have betrayed a Minor Civ he was bullying by declaring war on him
void CvPlayer::SetHasBetrayedMinorCiv(bool bValue)
{
	if(IsHasBetrayedMinorCiv() != bValue)
	{
		m_bHasBetrayedMinorCiv = bValue;
	}
}

#ifdef CAN_BUILD_OU_AND_NIA_ONLY_ONCE
bool CvPlayer::isOxfordUniversityWasEverBuilt() const
{
	return m_bOxfordUniversityWasEverBuilt;
}

void CvPlayer::setOxfordUniversityWasEverBuilt(bool bNewValue)
{
	m_bOxfordUniversityWasEverBuilt = bNewValue;
}

bool CvPlayer::isNationalIntelligenceAgencyWasEverBuilt() const
{
	return m_bNationalIntelligenceAgencyWasEverBuilt;
}

void CvPlayer::setNationalIntelligenceAgencyWasEverBuilt(bool bNewValue)
{
	m_bNationalIntelligenceAgencyWasEverBuilt = bNewValue;
}

#endif
//	--------------------------------------------------------------------------------
void CvPlayer::setAlive(bool bNewValue, bool bNotify)
{
	CvString strBuffer;
	int iI;

	if(isAlive() != bNewValue)
	{
		m_bAlive = bNewValue;

		GET_TEAM(getTeam()).changeAliveCount((isAlive()) ? 1 : -1);

		GC.getGame().GetGameLeagues()->DoPlayerAliveStatusChanged(GetID());

		// Tell Minor Civ AI what's up so that it knows when to add/remove bonuses for players it's friends with
		if(isMinorCiv())
		{
			GetMinorCivAI()->DoChangeAliveStatus(bNewValue);
		}

		if(isAlive())
		{
			if(!isEverAlive())
			{
				m_bEverAlive = true;

				GET_TEAM(getTeam()).changeEverAliveCount(1);
			}

			if(getNumCities() == 0)
			{
				setFoundedFirstCity(false);
			}

			GET_TEAM(getTeam()).SetKilledByTeam(NO_TEAM);

			if(isSimultaneousTurns() || (GC.getGame().getNumGameTurnActive() == 0) || (GC.getGame().isSimultaneousTeamTurns() && GET_TEAM(getTeam()).isTurnActive()))
			{
				setTurnActive(true);
			}

			gDLL->openSlot(GetID());
		}
		else
		{
#ifdef DECREASE_INFLUENCE_IF_BULLYING_SOMEONE_WE_ARE_PROTECTING
			for(int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
			{
				PlayerTypes eMinorLoop = (PlayerTypes) iMinorLoop;
				CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes) iMinorLoop);
				if(kPlayer.isAlive())
					GET_PLAYER(eMinorLoop).GetMinorCivAI()->DoChangeProtectionFromMajor(GetID(), false);
					// CvAssertMsg(GET_PLAYER((PlayerTypes) iMinorLoop).GetMinorCivAI()->GetAlly() != getLeaderID(), "Major civ is now at war with a minor it is allied with! This is dumb and bad. If you didn't do this on purpose, please send Jon this along with your last 5 autosaves and a changelist #.");
			}
#endif
			clearResearchQueue();
			killUnits();
			killCities();
			if(CvPreGame::isNetworkMultiplayerGame() && m_eID == GC.getGame().getActivePlayer())
				gDLL->netDisconnect();

			if (!GET_TEAM(getTeam()).isAlive())
			{
				for (int i = 0; i < MAX_TEAMS; i++)
				{
					TeamTypes eTheirTeam = (TeamTypes)i;
					if (getTeam() != eTheirTeam)
					{
						// close both embassies
						GET_TEAM(getTeam()).CloseEmbassyAtTeam(eTheirTeam);
						GET_TEAM(eTheirTeam).CloseEmbassyAtTeam(getTeam());

						// cancel any research agreements
						GET_TEAM(getTeam()).CancelResearchAgreement(eTheirTeam);
						GET_TEAM(eTheirTeam).CancelResearchAgreement(getTeam());
					}
				}
			}

			// Reset incoming units
			for(int iLoop = 0; iLoop < MAX_PLAYERS; iLoop++)
			{
				PlayerTypes eLoopPlayer = (PlayerTypes) iLoop;
				SetIncomingUnitCountdown(eLoopPlayer, -1);
				SetIncomingUnitType(eLoopPlayer, NO_UNIT);
			}

			GC.getGame().GetGameDeals()->DoCancelAllDealsWithPlayer(GetID());

			// Reset relationships with minor civs
			for(int iPlayerLoop = MAX_MAJOR_CIVS; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
			{
				PlayerTypes eOtherPlayer = (PlayerTypes) iPlayerLoop;
				GET_PLAYER(eOtherPlayer).GetMinorCivAI()->ResetFriendshipWithMajor(GetID());
			}

			setTurnActive(false);

			gDLL->closeSlot(GetID());

			if(bNotify && !isBarbarian())
			{
				Localization::String strMessage = Localization::Lookup("TXT_KEY_MISC_CIV_DESTROYED");
				strMessage << getCivilizationAdjectiveKey();
				Localization::String strSummary = Localization::Lookup("TXT_KEY_MISC_CIV_DESTROYED_SHORT");
				strSummary << getCivilizationShortDescriptionKey();

				for(iI = 0; iI < MAX_PLAYERS; iI++)
				{
					const PlayerTypes eOtherPlayer = static_cast<PlayerTypes>(iI);
					CvPlayerAI& kOtherPlayer = GET_PLAYER(eOtherPlayer);

					if(kOtherPlayer.isAlive() && kOtherPlayer.GetNotifications())
					{
						kOtherPlayer.GetNotifications()->Add(NOTIFICATION_PLAYER_KILLED, strMessage.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
					}
				}

				GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GetID(), strMessage.toUTF8(), -1, -1);
			}

			GC.getGame().testVictory();
		}

		GC.getGame().setScoreDirty(true);
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::setBeingResurrected(bool bValue)
{
	if (m_bBeingResurrected != bValue)
	{
		m_bBeingResurrected = bValue;
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::verifyAlive()
{
	bool bKill;

	if(isAlive())
	{
		bKill = false;

		if(!bKill)
		{
			if(!isBarbarian())
			{
				if(getNumCities() == 0 && getAdvancedStartPoints() < 0)
				{
					if((getNumUnits() == 0) || (!(GC.getGame().isOption(GAMEOPTION_COMPLETE_KILLS)) && isFoundedFirstCity()))
					{
						if(!GetPlayerTraits()->IsStaysAliveZeroCities())
						{
							bKill = true;
						}
					}
				}
			}
		}

		if(!bKill)
		{
			if(!isBarbarian())
			{
				if(GC.getGame().getMaxCityElimination() > 0)
				{
					if(getCitiesLost() >= GC.getGame().getMaxCityElimination())
					{
						bKill = true;
					}
				}
			}
		}

		if(bKill)
		{
			setAlive(false, false);
		}
	}
	else
	{
		if((getNumCities() > 0) || (getNumUnits() > 0))
		{
			setAlive(true);
		}
	}
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isTurnActive() const
{
	return m_bTurnActive;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setTurnActiveForPbem(bool bActive)
{
	CvAssertMsg(GC.getGame().isPbem(), "You are using setTurnActiveForPbem. Are you sure you know what you're doing?");

	// does nothing more than to set the member variable before saving the game
	// the rest of the turn will be performed upon loading the game
	// This allows the player to browse the game in paused mode after he has generated the save
	if(isTurnActive() != bActive)
	{
		m_bTurnActive = bActive;
		GC.getGame().changeNumGameTurnActive(isTurnActive() ? 1 : -1, "setTurnActiveForPlayByEmail");
	}
}


//	--------------------------------------------------------------------------------
void CvPlayer::setTurnActive(bool bNewValue, bool bDoTurn)
{
#ifdef GAME_ALLOW_ONLY_ONE_UNIT_MOVE_ON_TURN_LOADING
	float t1;
	float t2;
	GC.getGame().GetTurnTimerData(t1, t2);
	if (isHuman() && isAlive())
	{
		//SLOG("%f %f setTurnActive player: %d bNewValue: %d bDoTurn: %d", t1, t2, GetID(), bNewValue ? 1 : 0, bDoTurn ? 1 : 0);
	}
#endif
	if(isTurnActive() != bNewValue)
	{
		m_bTurnActive = bNewValue;
#ifdef EMERGENCY_LOGGING
		SLOG("GameTurn: %d setTurnActive player: %d bNewValue: %d bDoTurn: %d", GC.getGame().getGameTurn(), GetID(), bNewValue ? 1 : 0, bDoTurn ? 1 : 0);
#endif
		DLLUI->PublishEndTurnDirty();

		CvGame& kGame = GC.getGame();

		/////////////////////////////////////////////
		// TURN IS BEGINNING
		/////////////////////////////////////////////

		if(isTurnActive())
		{
			CvAssertMsg(isAlive(), "isAlive is expected to be true");

			setEndTurn(false);

			DoUnitAttrition();

			if(kGame.getActivePlayer() == m_eID)
			{
				CvMap& theMap = GC.getMap();
				theMap.updateDeferredFog();
			}

			if((kGame.isHotSeat() || kGame.isPbem()) && isHuman() && bDoTurn)
			{
				DLLUI->clearEventMessages();

				kGame.setActivePlayer(GetID());
			}

			if(CvPreGame::isPitBoss() && kGame.getActivePlayer() != m_eID && isHuman() && gDLL->IsHost() && !isConnected())
			{//send turn reminder if the player isn't actively connected to the game.
				sendTurnReminder();
			}

			std::ostringstream infoStream;
			infoStream << "setTurnActive() for player ";
			infoStream << (int)GetID();
			infoStream << " ";
			infoStream << getName();
			kGame.changeNumGameTurnActive(1, infoStream.str());

			DLLUI->PublishPlayerTurnStatus(DLLUIClass::TURN_START, GetID());

			if(bDoTurn)
			{
				SetAllUnitsUnprocessed();

				bool bCommonPathFinderMPCaching = GC.getPathFinder().SetMPCacheSafe(true);
				bool bIgnoreUnitsPathFinderMPCaching = GC.getIgnoreUnitsPathFinder().SetMPCacheSafe(true);
				bool bTacticalPathFinderMPCaching = GC.GetTacticalAnalysisMapFinder().SetMPCacheSafe(true);
				bool bInfluencePathFinderMPCaching = GC.getInfluenceFinder().SetMPCacheSafe(true);
				bool bRoutePathFinderMPCaching = GC.getRouteFinder().SetMPCacheSafe(true);
				bool bWaterRoutePathFinderMPCaching = GC.GetWaterRouteFinder().SetMPCacheSafe(true);

				{
					AI_PERF_FORMAT("AI-perf.csv", ("Connections/Gold, Turn %03d, %s", kGame.getElapsedGameTurns(), getCivilizationShortDescription()) );

					// This block all has things which might change based on city connections changing
					m_pCityConnections->Update();
					GetTreasury()->DoUpdateCityConnectionGold();
					DoUpdateHappiness();
				}

				{
					AI_PERF_FORMAT("AI-perf.csv", ("Builder Tasking, Turn %03d, %s", kGame.getElapsedGameTurns(), getCivilizationShortDescription()) );

					m_pBuilderTaskingAI->Update();
				}

				if(kGame.isFinalInitialized())
				{
					if(isAlive())
					{
						if(GetDiplomacyRequests())
						{
							GetDiplomacyRequests()->BeginTurn();
						}

						doTurn();

						doTurnUnits();
					}
				}

				GC.getPathFinder().SetMPCacheSafe(bCommonPathFinderMPCaching);
				GC.getIgnoreUnitsPathFinder().SetMPCacheSafe(bIgnoreUnitsPathFinderMPCaching);
				GC.GetTacticalAnalysisMapFinder().SetMPCacheSafe(bTacticalPathFinderMPCaching);
				GC.getInfluenceFinder().SetMPCacheSafe(bInfluencePathFinderMPCaching);
				GC.getRouteFinder().SetMPCacheSafe(bRoutePathFinderMPCaching);
				GC.GetWaterRouteFinder().SetMPCacheSafe(bWaterRoutePathFinderMPCaching);

				if((GetID() == kGame.getActivePlayer()) && (kGame.getElapsedGameTurns() > 0))
				{
					if(kGame.isNetworkMultiPlayer())
					{
						DLLUI->AddMessage(0, GetID(), true, GC.getEVENT_MESSAGE_TIME(), GetLocalizedText("TXT_KEY_MISC_TURN_BEGINS").GetCString(), "AS2D_NEWTURN", MESSAGE_TYPE_DISPLAY_ONLY);
					}
				}

				doWarnings();
			}

			if(GetID() == kGame.getActivePlayer())
			{
				GetUnitCycler().Rebuild();

				if(DLLUI->GetLengthSelectionList() == 0)
				{
					DLLUI->setCycleSelectionCounter(1);
				}

#ifndef REMOVE_EXCESS_CAMERA_CENTERING
				DLLUI->setDirty(SelectionCamera_DIRTY_BIT, true);
#endif

				// slewis - added this so the tutorial knows when a turn begins
				DLLUI->PublishActivePlayerTurnStart();
			}
			else if(isHuman() && kGame.isGameMultiPlayer())
			{
				DLLUI->PublishRemotePlayerTurnStart();
			}
		}

		/////////////////////////////////////////////
		// TURN IS ENDING
		/////////////////////////////////////////////

		else
		{
			CvAssertFmt(GetEndTurnBlockingType() == NO_ENDTURN_BLOCKING_TYPE, "Expecting the end-turn blocking to be NO_ENDTURN_BLOCKING_TYPE, got %d", GetEndTurnBlockingType());
			SetEndTurnBlocking(NO_ENDTURN_BLOCKING_TYPE, -1);	// Make sure this is clear so the UI doesn't block when it is not our turn.

			DoUnitReset();

			if(!isHuman())
			{
				RespositionInvalidUnits();
			}

			if(GetNotifications())
			{
				GetNotifications()->EndOfTurnCleanup();
			}

			if(GetDiplomacyRequests())
			{
				GetDiplomacyRequests()->EndTurn();
			}

			if(GetID() == kGame.getActivePlayer())
			{
				DLLUI->PublishActivePlayerTurnEnd();
			}

			if(!isHuman() || (isHuman() && !isAlive()) || (isHuman() && gDLL->HasReceivedTurnAllComplete(GetID())) || kGame.getAIAutoPlay())
				kGame.changeNumGameTurnActive(-1, std::string("setTurnActive() for player ") + getName());

			DLLUI->PublishPlayerTurnStatus(DLLUIClass::TURN_END, GetID());
		}
	}
	else
	{
		CvString logOutput;
		logOutput.Format("SetTurnActive() called without changing the end turn status. Player(%i) OldTurnActive(%i) NewTurnActive(%i)", GetID(), isTurnActive(), bNewValue);
		gDLL->netMessageDebugLog(logOutput);
	}
}

//	----------------------------------------------------------------------------
bool CvPlayer::isSimultaneousTurns() const
{
	if(GC.getGame().isOption(GAMEOPTION_DYNAMIC_TURNS))
	{//in dynamic turns mode, our turn mode varies
		return m_bDynamicTurnsSimultMode;
	}
	else if(GC.getGame().isOption(GAMEOPTION_SIMULTANEOUS_TURNS))
	{
		return true;
	}

	return false;
}


//	----------------------------------------------------------------------------
void CvPlayer::setDynamicTurnsSimultMode(bool simultaneousTurns)
{
	if(simultaneousTurns != m_bDynamicTurnsSimultMode)
	{
		CvNotifications* pNotifications = GetNotifications();
		if (pNotifications)
		{
			NotificationTypes notifyType = NOTIFICATION_TURN_MODE_SEQUENTIAL;
			Localization::String localizedText = Localization::Lookup("TXT_KEY_NOTIFICATION_TURN_MODE_SEQUENTIAL");
			Localization::String localizedTextSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_TURN_MODE_SEQUENTIAL_SUMMARY");
			if(simultaneousTurns)
			{
				notifyType = NOTIFICATION_TURN_MODE_SIMULTANEOUS;
				localizedText = Localization::Lookup("TXT_KEY_NOTIFICATION_TURN_MODE_SIMULTANEOUS");
				localizedTextSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_TURN_MODE_SIMULTANEOUS_SUMMARY");
			}

			pNotifications->Add(notifyType, localizedText.toUTF8(), localizedTextSummary.toUTF8(), -1, -1, -1);
		}

		m_bDynamicTurnsSimultMode = simultaneousTurns;
	}
	
}

//	----------------------------------------------------------------------------
bool CvPlayer::isAutoMoves() const
{
	return m_bAutoMoves;
}

//	----------------------------------------------------------------------------
void CvPlayer::setAutoMoves(bool bNewValue)
{
	if(isAutoMoves() != bNewValue)
	{
		m_bAutoMoves = bNewValue;
		m_bProcessedAutoMoves = false;
	}
}

//	----------------------------------------------------------------------------
bool CvPlayer::hasProcessedAutoMoves() const
{
	return m_bProcessedAutoMoves;
}

//	----------------------------------------------------------------------------
void CvPlayer::setProcessedAutoMoves(bool bNewValue)
{
	if(hasProcessedAutoMoves() != bNewValue)
	{
		m_bProcessedAutoMoves = bNewValue;
	}
}

//	----------------------------------------------------------------------------
bool CvPlayer::isEndTurn() const
{
	return m_bEndTurn;
}

//	------------------------------------------------------------------------------------------------
void CvPlayer::setEndTurn(bool bNewValue)
{
	CvGame& game = GC.getGame();

	if(isSimultaneousTurns()
		&& bNewValue 
		&& game.isNetworkMultiPlayer() 
		&& !gDLL->HasReceivedTurnAllCompleteFromAllPlayers())
	{//When doing simultaneous turns in multiplayer, we don't want anyone to end their turn until everyone has signalled TurnAllComplete.
		// No setting end turn to true until all the players have sent the TurnComplete network message
		return;
	}

	// If this is a remote player in an MP match, don't
	// honor the end of turn request if the player still
	// has units to run the simulation for the turn
	if(!isEndTurn() && isHuman() && GetID() != game.getActivePlayer())
	{
		if(hasBusyUnitOrCity() || (!gDLL->HasReceivedTurnComplete(GetID()) && hasReadyUnit()))
		{
			return;
		}
	}
	else if(!isHuman())
	{
		if(hasBusyUnitOrCity())
		{
			return;
		}
	}

	if(isEndTurn() != bNewValue)
	{
		//  If the game isn't MP and the player has queued popups force him to deal with them first
		if(!GC.getGame().isGameMultiPlayer())
		{
			//if (GC.GetEngineUserInterface()->isPopupQueued())
			//{
			//	GC.GetEngineUserInterface()->setForcePopup(true);
			//	return;
			//}
			//if (GC.GetEngineUserInterface()->isDiploOrPopupWaiting())
			//{
			//	return;
			//}
		}

		CvAssertMsg(isTurnActive(), "isTurnActive is expected to be true");

		m_bEndTurn = bNewValue;

		if(isEndTurn())
		{
			if(!GC.getGame().isOption(GAMEOPTION_DYNAMIC_TURNS) && GC.getGame().isOption(GAMEOPTION_SIMULTANEOUS_TURNS))
			{//fully simultaneous turns only run automoves after every human has moved.
				checkRunAutoMovesForEveryone();
			}
			else
			{
				setAutoMoves(true);
			}
		}
		else
			setAutoMoves(false);
	}
	else
	{
		// This check is here for the AI.  Currently, the setEndTurn(true) never seems to get called for AI players, the automoves are just set directly
		// Why is this?  It would be great if all players were processed the same.
		if(!bNewValue && isAutoMoves())
			setAutoMoves(false);
	}
}

//	---------------------------------------------------------------------------
void CvPlayer::checkRunAutoMovesForEveryone()
{
	bool runAutoMovesForEveryone = true;
	int i = 0;
	for(i = 0; i < MAX_PLAYERS; ++i)
	{
		CvPlayer& p = CvPlayerAI::getPlayer((PlayerTypes)i);
		if(p.isHuman() && !p.isObserver() 
			// Check to see if this human player hasn't gotten to the end turn phase of their turn.  
			// This gets tricky because hot joiners can hop into an ai civ that already finished their turn.
			// When this occurs, the hot joiner will not be turn active, will have already run their automoves,
			// and not have end turn set. (AIs do not set end turn) *sigh*
			// To handle that case, we assume that human players who are not endturn and turn inactive after TurnAllComplete
			// are ready for the human automoves phase.
			&& (!p.isEndTurn()
			&& (!gDLL->HasReceivedTurnAllCompleteFromAllPlayers() || p.isTurnActive()))) 
		{
			runAutoMovesForEveryone = false;
			break;
		}
	}

	if(runAutoMovesForEveryone)
	{
		for(i = 0; i < MAX_PLAYERS; ++i)
		{
			CvPlayer& p = CvPlayerAI::getPlayer((PlayerTypes)i);
			if(p.isHuman())
			{
				p.setAutoMoves(true);
			}
		}
	}
}

//	---------------------------------------------------------------------------
EndTurnBlockingTypes CvPlayer::GetEndTurnBlockingType(void) const
{
	return m_eEndTurnBlockingType;
}

//	---------------------------------------------------------------------------
int CvPlayer::GetEndTurnBlockingNotificationIndex(void) const
{
	return m_iEndTurnBlockingNotificationIndex;
}

//	---------------------------------------------------------------------------
void CvPlayer::SetEndTurnBlocking(EndTurnBlockingTypes eBlockingType, int iNotificationIndex)
{
	bool bFireEvent = false;
	if(m_eEndTurnBlockingType != eBlockingType || m_iEndTurnBlockingNotificationIndex != iNotificationIndex)
	{
#ifdef EMERGENCY_LOGGING
		SLOG("GameTurn %d ID %d ActivePlayer %d eBlockingType %d iNotificationIndex %d", GC.getGame().getGameTurn(), (int)GetID(), (int)GC.getGame().getActivePlayer(), (int)eBlockingType, iNotificationIndex)
#endif
		bFireEvent = true;
	}

	EndTurnBlockingTypes ePrevBlockingType = m_eEndTurnBlockingType;
	m_eEndTurnBlockingType = eBlockingType;
	m_iEndTurnBlockingNotificationIndex = iNotificationIndex;

	if(bFireEvent)
	{
		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->SetEndTurnBlockingChanged( ePrevBlockingType, m_eEndTurnBlockingType );
			GC.GetEngineUserInterface()->UpdateEndTurn();
		}
	}
}

//	---------------------------------------------------------------------------
bool CvPlayer::isTurnDone() const
{
	// if this returns true, popups and diplomacy will wait to appear until next turn
	if(!GC.getGame().isPbem() && !GC.getGame().isHotSeat())
	{
		return false;
	}
	if(!isHuman())
	{
		return true;
	}
	if(!isEndTurn())
	{
		return false;
	}
	return (!isAutoMoves());
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isExtendedGame() const
{
	return m_bExtendedGame;
}


//	--------------------------------------------------------------------------------
void CvPlayer::makeExtendedGame()
{
	m_bExtendedGame = true;
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isFoundedFirstCity() const
{
	return m_bFoundedFirstCity;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setFoundedFirstCity(bool bNewValue)
{
	if(isFoundedFirstCity() != bNewValue)
	{
		m_bFoundedFirstCity = bNewValue;

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(PercentButtons_DIRTY_BIT, true);
			GC.GetEngineUserInterface()->setDirty(ResearchButtons_DIRTY_BIT, true);
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNumCitiesFounded() const
{
	return m_iNumCitiesFounded;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeNumCitiesFounded(int iValue)
{
	m_iNumCitiesFounded += iValue;
}

//	--------------------------------------------------------------------------------
// check to see if we defeated this other player
void CvPlayer::CheckForMurder(PlayerTypes ePossibleVictimPlayer)
{
	// Cache whether the player is human or not.  If the player is killed, the CvPreGame::slotStatus is changed to SS_CLOSED
	// but the slot status is used to determine if the player is human or not, so it looks like it is an AI!
	// This should be fixed, but might have unforeseen ramifications so...
	CvPlayer& kPossibleVictimPlayer = GET_PLAYER(ePossibleVictimPlayer);
	bool bPossibileVictimIsHuman = kPossibleVictimPlayer.isHuman();

	// This may 'kill' the player if it is deemed that he does not have the proper units to stay alive
	kPossibleVictimPlayer.verifyAlive();

	// You... you killed him!
	if(!kPossibleVictimPlayer.isAlive())
	{
		GET_TEAM(kPossibleVictimPlayer.getTeam()).SetKilledByTeam(getTeam());
		kPossibleVictimPlayer.SetEverConqueredBy(m_eID, true);

		// Leader pops up and whines
		if(!CvPreGame::isNetworkMultiplayerGame())		// Not in MP
		{
			if(!bPossibileVictimIsHuman && !kPossibleVictimPlayer.isMinorCiv() && !kPossibleVictimPlayer.isBarbarian())
				kPossibleVictimPlayer.GetDiplomacyAI()->DoKilledByPlayer(GetID());
		}

		// do post-dying clean up
		if (!kPossibleVictimPlayer.isMinorCiv())
		{
			for (uint ui = 0; ui < MAX_MAJOR_CIVS; ui++)
			{
				PlayerTypes eCleanupPlayer = (PlayerTypes)ui;
				GET_PLAYER(eCleanupPlayer).GetDiplomacyAI()->KilledPlayerCleanup(kPossibleVictimPlayer.GetID());
			}
		}
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isStrike() const
{
	return m_bStrike;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setStrike(bool bNewValue)
{
	if(m_bStrike != bNewValue)
	{
		m_bStrike = bNewValue;

		if(m_bStrike)
		{
			if(GetID() == GC.getGame().getActivePlayer())
			{
				GC.GetEngineUserInterface()->AddMessage(0, GetID(), false, GC.getEVENT_MESSAGE_TIME(), GetLocalizedText("TXT_KEY_MISC_UNITS_ON_STRIKE").GetCString(), "AS2D_STRIKE", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"));

				GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
			}
		}
	}
}


//	--------------------------------------------------------------------------------
/// Is the player is cramped in his current area?
bool CvPlayer::IsCramped() const
{
	return m_bCramped;
}

//	--------------------------------------------------------------------------------
/// Determines if the player is cramped in his current area.  Not a perfect algorithm, as it will double-count Plots shared by different Cities, but it should be good enough
void CvPlayer::DoUpdateCramped()
{
	CvCity* pLoopCity;
	CvPlot* pPlot;

	int iTotalPlotsNearby = 0;
	int iUsablePlotsNearby = 0;

	int iRange = GC.getCRAMPED_RANGE_FROM_CITY();

	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		for(int iX = -iRange; iX <= iRange; iX++)
		{
			for(int iY = -iRange; iY <= iRange; iY++)
			{
				pPlot = plotXYWithRangeCheck(pLoopCity->getX(), pLoopCity->getY(), iX, iY, iRange);

				if(pPlot != NULL)
				{
					// Plot not owned by me
					if(!pPlot->isOwned() || pPlot->getOwner() != GetID())
					{
						iTotalPlotsNearby++;

						// A "good" unowned Plot
						if(!pPlot->isOwned() && !pPlot->isImpassable() && !pPlot->isMountain() && !pPlot->isWater())
						{
							iUsablePlotsNearby++;
						}
					}
				}
			}
		}
	}

	if(iTotalPlotsNearby > 0)
	{
		if(100 * iUsablePlotsNearby / iTotalPlotsNearby <= GC.getCRAMPED_USABLE_PLOT_PERCENT())	// 20
		{
			m_bCramped = true;
		}
		else
		{
			m_bCramped = false;
		}
	}
}

//	--------------------------------------------------------------------------------
CvHandicapInfo& CvPlayer::getHandicapInfo() const
{
	CvHandicapInfo* pkHandicapInfo = GC.getHandicapInfo(getHandicapType());
	if(pkHandicapInfo == NULL)
	{
		const char* szError = "ERROR: Player does not contain valid handicap!!";
		GC.LogMessage(szError);
		CvAssertMsg(false, szError);
	}

#pragma warning ( push )
#pragma warning ( disable : 6011 ) // Dereferencing NULL pointer
	return *pkHandicapInfo;
#pragma warning ( pop )
}

//	--------------------------------------------------------------------------------
HandicapTypes CvPlayer::getHandicapType() const
{
	return CvPreGame::handicap(GetID());
}

//	--------------------------------------------------------------------------------
CvCivilizationInfo& CvPlayer::getCivilizationInfo() const
{
	CvCivilizationInfo* pkCivilizationInfo = GC.getCivilizationInfo(getCivilizationType());
	if(pkCivilizationInfo == NULL)
	{
		const char* szError = "ERROR: Player does not contain valid civilization type!!";
		GC.LogMessage(szError);
		CvAssertMsg(false, szError);
	}

#pragma warning ( push )
#pragma warning ( disable : 6011 ) // Dereferencing NULL pointer
	return *pkCivilizationInfo;
#pragma warning ( pop )
}

//	--------------------------------------------------------------------------------
CivilizationTypes CvPlayer::getCivilizationType() const
{
	return CvPreGame::civilization(GetID());
}


//	--------------------------------------------------------------------------------
CvLeaderHeadInfo& CvPlayer::getLeaderInfo() const
{
	CvLeaderHeadInfo* pkLeaderInfo = GC.getLeaderHeadInfo(getLeaderType());
	if(pkLeaderInfo == NULL)
	{
		const char* szError = "ERROR: Player does not contain valid leader type!!";
		GC.LogMessage(szError);
		CvAssertMsg(false, szError);
	}

#pragma warning ( push )
#pragma warning ( disable : 6011 ) // Dereferencing NULL pointer
	return *pkLeaderInfo;
#pragma warning ( pop )
}

//	--------------------------------------------------------------------------------
LeaderHeadTypes CvPlayer::getLeaderType() const
{
	return CvPreGame::leaderHead(GetID());
}


//	--------------------------------------------------------------------------------
LeaderHeadTypes CvPlayer::getPersonalityType() const
{
	return m_ePersonalityType;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setPersonalityType(LeaderHeadTypes eNewValue)
{
	m_ePersonalityType = eNewValue;
}


//	--------------------------------------------------------------------------------
EraTypes CvPlayer::GetCurrentEra() const
{
	return GET_TEAM(getTeam()).GetCurrentEra();
}

//	--------------------------------------------------------------------------------
void CvPlayer::setTeam(TeamTypes eTeam)
{
	CvAssert(eTeam != NO_TEAM);
	CvAssert(getTeam() != NO_TEAM);

	GET_TEAM(getTeam()).changeNumMembers(-1);
	if(isAlive())
	{
		GET_TEAM(getTeam()).changeAliveCount(-1);
	}
	if(isEverAlive())
	{
		GET_TEAM(getTeam()).changeEverAliveCount(-1);
	}
	GET_TEAM(getTeam()).changeNumCities(-(getNumCities()));
	GET_TEAM(getTeam()).changeTotalPopulation(-(getTotalPopulation()));
	GET_TEAM(getTeam()).changeTotalLand(-(getTotalLand()));

	CvPreGame::setTeamType(GetID(), eTeam);

	GET_TEAM(getTeam()).changeNumMembers(1);
	if(isAlive())
	{
		GET_TEAM(getTeam()).changeAliveCount(1);
	}
	if(isEverAlive())
	{
		GET_TEAM(getTeam()).changeEverAliveCount(1);
	}
	GET_TEAM(getTeam()).changeNumCities(getNumCities());
	GET_TEAM(getTeam()).changeTotalPopulation(getTotalPopulation());
	GET_TEAM(getTeam()).changeTotalLand(getTotalLand());
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsAITeammateOfHuman() const
{
	bool bRtnValue = false;

	for(int i = 0; i < MAX_PLAYERS; ++i)
	{
		CvPlayer& player = GET_PLAYER(static_cast<PlayerTypes>(i));
		if(player.isHuman() && player.isAlive())
		{
			if(player.getTeam() == getTeam())
			{
				bRtnValue = true;
				break;
			}
		}
	}

	return bRtnValue;
}

//	--------------------------------------------------------------------------------
PlayerColorTypes CvPlayer::getPlayerColor() const
{
	return CvPreGame::playerColor(GetID());
}

//	--------------------------------------------------------------------------------
const CvColorA& CvPlayer::getPlayerTextColor() const
{
	CvAssertMsg(getPlayerColor() != NO_PLAYERCOLOR, "getPlayerColor() is not expected to be equal with NO_PLAYERCOLOR");
	CvPlayerColorInfo* pkPlayerColorInfo = GC.GetPlayerColorInfo(getPlayerColor());
	CvColorInfo* pkColorInfo = NULL;
	if(pkPlayerColorInfo)
	{
		ColorTypes eTextColor = static_cast<ColorTypes>(pkPlayerColorInfo->GetColorTypeText());
		pkColorInfo = GC.GetColorInfo(eTextColor);
		if(pkColorInfo)
			return pkColorInfo->GetColor();
	}

	//Default to black text if no color exists.
	static CvColorA black(0,0,0,1.0f);
	return black;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getSeaPlotYield(YieldTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiSeaPlotYield[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeSeaPlotYield(YieldTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_aiSeaPlotYield.setAt(eIndex, m_aiSeaPlotYield[eIndex] + iChange);

		updateYield();
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getYieldRateModifier(YieldTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiYieldRateModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeYieldRateModifier(YieldTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_aiYieldRateModifier.setAt(eIndex, m_aiYieldRateModifier[eIndex] + iChange);

		invalidateYieldRankCache(eIndex);

		if(getTeam() == GC.getGame().getActiveTeam())
		{
			GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getCapitalYieldRateModifier(YieldTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCapitalYieldRateModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeCapitalYieldRateModifier(YieldTypes eIndex, int iChange)
{
	CvCity* pCapitalCity;

	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_aiCapitalYieldRateModifier.setAt(eIndex, m_aiCapitalYieldRateModifier[eIndex] + iChange);

		invalidateYieldRankCache(eIndex);

		pCapitalCity = getCapitalCity();
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getExtraYieldThreshold(YieldTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiExtraYieldThreshold[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::updateExtraYieldThreshold(YieldTypes eIndex)
{
	int iBestValue;

	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	iBestValue = GetPlayerTraits()->GetExtraYieldThreshold(eIndex);

	if(getExtraYieldThreshold(eIndex) != iBestValue)
	{
		m_aiExtraYieldThreshold.setAt(eIndex, iBestValue);
		CvAssert(getExtraYieldThreshold(eIndex) >= 0);

		updateYield();
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetScience() const
{
	return GetScienceTimes100() / 100;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetScienceTimes100() const
{
	// If we're in anarchy, then no Research is done!
#ifdef PENALTY_FOR_DELAYING_POLICIES
	if (IsAnarchy() || IsDelayedPolicy())
#else
	if(IsAnarchy())
#endif
		return 0;

	int iValue = 0;

	// Science from our Cities
	iValue += GetScienceFromCitiesTimes100(false);

	// Science from other players!
	iValue += GetScienceFromOtherPlayersTimes100();

	// Happiness converted to Science? (Policies, etc.)
	iValue += GetScienceFromHappinessTimes100();

	// Research Agreement bonuses
	iValue += GetScienceFromResearchAgreementsTimes100();

	// If we have a negative Treasury + GPT then it gets removed from Science
	iValue += GetScienceFromBudgetDeficitTimes100();

#ifdef BELIEF_INTERFAITH_DIALOGUE_PER_FOLLOWERS
	iValue += GetSciencePerTurnFromReligionTimes100();
#endif

#ifdef NEW_CITY_STATES_TYPES
	iValue += GetSciencePerTurnFromMinorCivsTimes100();
#endif

#ifdef SCIENCE_FROM_INFLUENCED_CIVS
	iValue += GetSciencePerTurnFromInfluencedCivsTimes100();
#endif

	return max(iValue, 0);
}

#ifdef NEW_CITY_STATES_TYPES
//	--------------------------------------------------------------------------------
/// Science per turn from all minor civs
int CvPlayer::GetSciencePerTurnFromMinorCivsTimes100() const
{
	int iAmount = 0;
	PlayerTypes eMinor;
	for (int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
	{
		eMinor = (PlayerTypes)iMinorLoop;
		iAmount += GetSciencePerTurnFromMinorTimes100(eMinor);
	}

	return iAmount;
}

//	--------------------------------------------------------------------------------
// Science per turn from a minor civ
int CvPlayer::GetSciencePerTurnFromMinorTimes100(PlayerTypes eMinor) const
{
	int iAmount = 0;

	if (GET_PLAYER(eMinor).isAlive())
	{
		// Includes flat bonus and any bonus from scientific buildings
		iAmount += GET_PLAYER(eMinor).GetMinorCivAI()->GetCurrentScienceBonus(GetID());
		iAmount *= 100;
	}

	return iAmount;
}
#endif

#ifdef SCIENCE_FROM_INFLUENCED_CIVS
//	--------------------------------------------------------------------------------
// Science per turn from a influenced civ
int CvPlayer::GetSciencePerTurnFromInfluencedCivsTimes100() const
{
	int iAmount = 0;

	PlayerTypes ePlayer;
	int iScienceFromPlayer;
	for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
	{
		ePlayer = (PlayerTypes)iPlayerLoop;

		if (ePlayer == GetID())
			continue;

		iScienceFromPlayer = 0;

		iScienceFromPlayer += GET_PLAYER(ePlayer).GetScienceFromCitiesTimes100(false);
		iScienceFromPlayer += GET_PLAYER(ePlayer).GetScienceFromHappinessTimes100();
		iScienceFromPlayer += GET_PLAYER(ePlayer).GetScienceFromResearchAgreementsTimes100();
		iScienceFromPlayer += GET_PLAYER(ePlayer).GetScienceFromBudgetDeficitTimes100();
#ifdef BELIEF_INTERFAITH_DIALOGUE_PER_FOLLOWERS
		iScienceFromPlayer += GET_PLAYER(ePlayer).GetSciencePerTurnFromReligionTimes100();
#endif
#ifdef NEW_CITY_STATES_TYPES
		iScienceFromPlayer += GET_PLAYER(ePlayer).GetSciencePerTurnFromMinorCivsTimes100();
#endif
		iScienceFromPlayer *= GetCulture()->GetInfluencedCivScienceBonus(ePlayer);
		iScienceFromPlayer /= 100;

		if (GC.getGame().isOption("GAMEOPTION_AI_TWEAKS"))
		{
			if (!GET_PLAYER(ePlayer).isHuman())
			{
				iScienceFromPlayer /= 2;
			}
		}

		iAmount += iScienceFromPlayer;
	}

	return iAmount;
}
#endif

#ifdef BELIEF_INTERFAITH_DIALOGUE_PER_FOLLOWERS
//	--------------------------------------------------------------------------------
int CvPlayer::GetSciencePerTurnFromReligionTimes100() const 
{
	int iReligionSciencePerTurn = 0;

	// Founder beliefs
	CvGameReligions* pReligions = GC.getGame().GetGameReligions();
	ReligionTypes eFoundedReligion = pReligions->GetFounderBenefitsReligion(GetID());
	if(eFoundedReligion != NO_RELIGION)
	{
		const CvReligion* pReligion = pReligions->GetReligion(eFoundedReligion, NO_PLAYER);
		if(pReligion)
		{
			iReligionSciencePerTurn += pReligion->m_Beliefs.GetHolyCityYieldChange(YIELD_SCIENCE);

			int iTemp = pReligion->m_Beliefs.GetYieldChangePerForeignCity(YIELD_SCIENCE);
			if (iTemp > 0)
			{
				iReligionSciencePerTurn += (iTemp * GetReligions()->GetNumForeignCitiesFollowing());
			}

			iTemp = pReligion->m_Beliefs.GetYieldChangePerXForeignFollowers(YIELD_SCIENCE);
			if (iTemp > 0)
			{
				int iFollowers = pReligions->GetNumFollowers(eFoundedReligion);
				if (iFollowers > 0)
				{
					iReligionSciencePerTurn += (iFollowers / iTemp);
				}
			}

#ifdef POLICY_SCIENCE_PER_X_FOLLOWERS
			iTemp = GetSciencePerXFollowers();
			if (iTemp > 0)
			{
				int iFollowers = pReligions->GetNumFollowers(eFoundedReligion);
				if (iFollowers > 0)
				{
					iReligionSciencePerTurn += (iFollowers / iTemp);
				}
			}
#endif

			iReligionSciencePerTurn *= 100;
			return iReligionSciencePerTurn;
		}
	}

	return 0;
}
#endif
//	--------------------------------------------------------------------------------
/// Where is our Science coming from?
int CvPlayer::GetScienceFromCitiesTimes100(bool bIgnoreTrade) const
{
	int iScience = 0;

	const CvCity* pLoopCity;

	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iScience += pLoopCity->getYieldRateTimes100(YIELD_SCIENCE, bIgnoreTrade);
	}

	return iScience;
}

//	--------------------------------------------------------------------------------
/// Where is our Science coming from?
int CvPlayer::GetScienceFromOtherPlayersTimes100() const
{
	int iScience = 0;

	PlayerTypes ePlayer;
	int iScienceFromPlayer;
	for(int iPlayerLoop = MAX_MAJOR_CIVS; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		ePlayer = (PlayerTypes) iPlayerLoop;

		iScienceFromPlayer = 0;

		if(IsGetsScienceFromPlayer(ePlayer))
		{
			iScienceFromPlayer = GET_PLAYER(ePlayer).GetMinorCivAI()->GetScienceFriendshipBonusTimes100();

			iScience += iScienceFromPlayer;
		}
	}

	return iScience;
}

//	--------------------------------------------------------------------------------
/// Where is our Science coming from?
int CvPlayer::GetScienceFromHappinessTimes100() const
{
	if(GC.getGame().isOption(GAMEOPTION_NO_HAPPINESS))
	{
		return 0;
	}

	int iScience = 0;

	if(getHappinessToScience() != 0)
	{
		if(GetExcessHappiness() >= 0)
		{
			int iFreeScience = GetScienceFromCitiesTimes100(false) * getHappinessToScience();
			iFreeScience /= 100;

			iScience += iFreeScience;
		}
	}

	return iScience;
}

//	--------------------------------------------------------------------------------
/// Where is our Science coming from?
int CvPlayer::GetScienceFromResearchAgreementsTimes100() const
{
	int iScience = GetScienceFromCitiesTimes100(false);

	int iResearchAgreementBonus = /*0*/ GC.getRESEARCH_AGREEMENT_MOD() * GET_TEAM(getTeam()).GetTotalNumResearchAgreements(); // RAs currently do not have this effect
	iScience *= iResearchAgreementBonus;	// Apply to the % to the current value
	iScience /= 100;

	return iScience;
}

//	--------------------------------------------------------------------------------
/// Where is our Science coming from?
int CvPlayer::GetScienceFromBudgetDeficitTimes100() const
{
	int iScience = 0;
	
	int iMyNum = 0;
	if (iScience > 0)
	{
		iMyNum = -1;
	}


	int iGoldPerTurn = calculateGoldRateTimes100();
	if(GetTreasury()->GetGoldTimes100() + iGoldPerTurn < 0)
	{
		iScience += (GetTreasury()->GetGoldTimes100() + iGoldPerTurn);
	}

	return iScience;
}

//	--------------------------------------------------------------------------------
/// What is the sum of science yield (not counting Research Agreements or Great Scientist bonuses) from the previous N turns?
/// NOTE: This uses the data tracked in recording a replay, so if replays are disabled in the future then this must change!
int CvPlayer::GetScienceYieldFromPreviousTurns(int iGameTurn, int iNumPreviousTurnsToCount)
{
	// Beakers per turn yield is tracked in replay data, so use that
	int iSum = 0;
	for (int iI = 0; iI < iNumPreviousTurnsToCount; iI++)
	{
		int iTurn = iGameTurn - iI;
		if (iTurn < 0)
		{
			break;
		}

		int iTurnScience = getReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_SCIENCEPERTURN"), iTurn);
		if (iTurnScience >= 0)
		{
			iSum += iTurnScience;
		}
#ifdef NEW_SCIENTISTS_BULB
		else if (iTurnScience < 0) // No data for this turn (ex. late era start)
#else
		else if (iTurnScience == -1) // No data for this turn (ex. late era start)
#endif
		{
#ifdef NEW_SCIENTISTS_BULB
			iSum += (GetScience());
#else
			iSum += (3 * GetScience());
#endif
		}
	}

	return iSum;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsGetsScienceFromPlayer(PlayerTypes ePlayer) const
{
	CvAssertMsg(ePlayer >= MAX_MAJOR_CIVS, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(ePlayer < MAX_CIV_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");

	// Might have global modifier
	if(IsMinorScienceAllies() && GET_PLAYER(ePlayer).GetMinorCivAI()->IsAllies(GetID()))
	{
		return true;
	}

	return m_pabGetsScienceFromPlayer[ePlayer];
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetGetsScienceFromPlayer(PlayerTypes ePlayer, bool bNewValue)
{
	CvAssertMsg(ePlayer >= MAX_MAJOR_CIVS, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(ePlayer < MAX_CIV_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(bNewValue != m_pabGetsScienceFromPlayer[ePlayer])
	{
		m_pabGetsScienceFromPlayer.setAt(ePlayer, bNewValue);
	}
}

//	--------------------------------------------------------------------------------
/// Player spending too much cash?
#ifdef UNIT_DISBAND_REWORK
void CvPlayer::DoDeficit(int iValue)
#else
void CvPlayer::DoDeficit()
#endif
{
	int iNumMilitaryUnits = 0;

	CvUnit* pLoopUnit;
	int iLoop;
	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->IsCombatUnit())
			iNumMilitaryUnits++;
	}

	// If the player has more units than cities, start disbanding things
#ifndef UNIT_DISBAND_REWORK
	if(iNumMilitaryUnits > getNumCities())
#endif
	{
#ifdef UNIT_DISBAND_REWORK
		int iRand = GC.getGame().getJonRandNum(100, "Disband rand");
		if (100 * (1 + (int)GC.getGame().getCurrentEra()) * (iRand + 1) <= -2 * iValue)
#else
		if(GC.getGame().getJonRandNum(100, "Disband rand") < 50)
#endif
		{
			UnitHandle pLandUnit;
			UnitHandle pNavalUnit;
			int iLandScore = MAX_INT;
			int iNavalScore = MAX_INT;

			// Look for obsolete land units if in deficit or have sufficient units
#ifndef UNIT_DISBAND_REWORK
			if(GetMilitaryAI()->GetLandDefenseState() <= DEFENSE_STATE_NEUTRAL)
#endif
			{
				pLandUnit = GetMilitaryAI()->FindBestUnitToScrap(true /*bLand*/, true /*bDeficitForcedDisband*/, iLandScore);
			}

			// Look for obsolete naval units if in deficit or have sufficient units
#ifndef UNIT_DISBAND_REWORK
			if(GetMilitaryAI()->GetNavalDefenseState() <= DEFENSE_STATE_NEUTRAL)
#endif
			{
				pNavalUnit = GetMilitaryAI()->FindBestUnitToScrap(false/*bNaval*/, true /*bDeficitForcedDisband*/, iNavalScore);
			}

			if(iLandScore < MAX_INT && (GetMilitaryAI()->GetLandDefenseState() <= GetMilitaryAI()->GetNavalDefenseState() || iLandScore <= iNavalScore))
			{
				if(pLandUnit)
				{
					CvNotifications* pNotifications = GetNotifications();
					if(pNotifications)
					{
						Localization::String locString = Localization::Lookup("TXT_KEY_NTFN_UNIT_DISBANDED");
						Localization::String locSummary = Localization::Lookup("TXT_KEY_NTFN_UNIT_DISBANDED_S");
						pNotifications->Add(NOTIFICATION_UNIT_DIED, locString.toUTF8(), locSummary.toUTF8(), pLandUnit->getX(), pLandUnit->getY(), pLandUnit->getUnitType(), GetID());
					}
#ifdef REPLAY_EVENTS
					if (isHuman())
					{
						std::vector<int> vArgs;
						vArgs.push_back(static_cast<int>(pLandUnit->getUnitType()));
						GC.getGame().addReplayEvent(REPLAYEVENT_UnitDisbanded, GetID(), vArgs);
					}
#endif

					pLandUnit->scrap();
					GetMilitaryAI()->LogDeficitScrapUnit(pLandUnit);
				}
			}
			else if(iNavalScore < MAX_INT)
			{
				if(pNavalUnit)
				{
					CvNotifications* pNotifications = GetNotifications();
					if(pNotifications)
					{
						Localization::String locString = Localization::Lookup("TXT_KEY_NTFN_UNIT_DISBANDED");
						Localization::String locSummary = Localization::Lookup("TXT_KEY_NTFN_UNIT_DISBANDED_S");
						pNotifications->Add(NOTIFICATION_UNIT_DIED, locString.toUTF8(), locSummary.toUTF8(), pNavalUnit->getX(), pNavalUnit->getY(), pNavalUnit->getUnitType(), GetID());
					}
#ifdef REPLAY_EVENTS
					if (isHuman())
					{
						std::vector<int> vArgs;
						vArgs.push_back(static_cast<int>(pNavalUnit->getUnitType()));
						GC.getGame().addReplayEvent(REPLAYEVENT_UnitDisbanded, GetID(), vArgs);
					}
#endif

					pNavalUnit->scrap();
					GetMilitaryAI()->LogDeficitScrapUnit(pNavalUnit);
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getSpecialistExtraYield(YieldTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiSpecialistExtraYield[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeSpecialistExtraYield(YieldTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		// Have to handle Specialists yield update manually here because the "updateYield()" below only accounts for land Yield!

		CvCity* pLoopCity;
		int iLoop;
		int iNumTotalSpecialists = 0;

		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			for(int iSpecialistLoop = 0; iSpecialistLoop < GC.getNumSpecialistInfos(); iSpecialistLoop++)
			{
				iNumTotalSpecialists = pLoopCity->GetCityCitizens()->GetSpecialistCount((SpecialistTypes) iSpecialistLoop);
//				iNumTotalSpecialists = pLoopCity->getSpecialistCount((SpecialistTypes) iSpecialistLoop) + pLoopCity->getFreeSpecialistCount((SpecialistTypes) iSpecialistLoop);

				for(int iTempLoop = 0; iTempLoop < iNumTotalSpecialists; iTempLoop++)
				{
					pLoopCity->processSpecialist((SpecialistTypes) iSpecialistLoop, -1);
				}
			}
		}

		m_aiSpecialistExtraYield.setAt(eIndex ,m_aiSpecialistExtraYield[eIndex] + iChange);
		CvAssert(getSpecialistExtraYield(eIndex) >= 0);

		updateYield();

		// Reprocess Specialist AFTER yield change
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			for(int iSpecialistLoop = 0; iSpecialistLoop < GC.getNumSpecialistInfos(); iSpecialistLoop++)
			{
				iNumTotalSpecialists = pLoopCity->GetCityCitizens()->GetSpecialistCount((SpecialistTypes) iSpecialistLoop);
//				iNumTotalSpecialists = pLoopCity->getSpecialistCount((SpecialistTypes) iSpecialistLoop) + pLoopCity->getFreeSpecialistCount((SpecialistTypes) iSpecialistLoop);

				for(int iTempLoop = 0; iTempLoop < iNumTotalSpecialists; iTempLoop++)
				{
					pLoopCity->processSpecialist((SpecialistTypes) iSpecialistLoop, 1);
				}
			}
		}
	}
}

#ifdef POLICY_GOLDEN_AGE_YIELD_MOD
//	--------------------------------------------------------------------------------
int CvPlayer::getGoldenAgeYieldModifier(YieldTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiGoldenAgeYieldModifier[eIndex];
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeGoldenAgeYieldModifier(YieldTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiGoldenAgeYieldModifier.setAt(eIndex, m_aiGoldenAgeYieldModifier[eIndex] + iChange);
	}
}
#endif

#ifdef POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
//	--------------------------------------------------------------------------------
int CvPlayer::getPlotExtraYieldFromTradeRoute(YieldTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiPlotExtraYieldFromTradeRoute[eIndex];
}

//	--------------------------------------------------------------------------------
void CvPlayer::changePlotExtraYieldFromTradeRoute(YieldTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiPlotExtraYieldFromTradeRoute.setAt(eIndex, m_paiPlotExtraYieldFromTradeRoute[eIndex] + iChange);
	}
}
#endif

//	--------------------------------------------------------------------------------
/// Returns how "close" we are to another player (useful for diplomacy, war planning, etc.)
PlayerProximityTypes CvPlayer::GetProximityToPlayer(PlayerTypes ePlayer) const
{
	CvAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(ePlayer < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return (PlayerProximityTypes) m_aiProximityToPlayer[ePlayer];
}

//	--------------------------------------------------------------------------------
/// Sets how "close" we are to another player (useful for diplomacy, war planning, etc.)
void CvPlayer::SetProximityToPlayer(PlayerTypes ePlayer, PlayerProximityTypes eProximity)
{
	CvAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(ePlayer < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");

	CvAssertMsg(GetID() != ePlayer, "Trying to calculate proximity to oneself. Please send Jon this with your last 5 autosaves and what changelist # you're playing.");

	CvAssertMsg(eProximity >= NO_PLAYER_PROXIMITY, "eIndex is expected to be non-negative (invalid Index)");	// NO_PLAYER_PROXIMITY is valid because some players may have no Cities (e.g. on the first turn)
	CvAssertMsg(eProximity < NUM_PLAYER_PROXIMITIES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if((GC.getLogging() && GC.getAILogging()))
	{
		if(eProximity != m_aiProximityToPlayer[ePlayer])
		{
			// Open the log file
			CvString strFileName = "PlayerProximityLog.csv";
			FILogFile* pLog;
			pLog = LOGFILEMGR.GetLog(strFileName, FILogFile::kDontTimeStamp);
			CvString strLog, strTemp;

			CvString strPlayerName;
			strPlayerName = getCivilizationShortDescription();
			strLog += strPlayerName;
			strLog += ",";

			strTemp.Format("%d,", GC.getGame().getGameTurn()); // turn
			strLog += strTemp;
			CvString strOtherPlayerName;
			strOtherPlayerName = GET_PLAYER(ePlayer).getCivilizationShortDescription();
			strLog += strOtherPlayerName;
			strLog += ",";

			switch(m_aiProximityToPlayer[ePlayer])
			{
			case NO_PLAYER_PROXIMITY:
				strLog += "No player proximity,";
				break;
			case PLAYER_PROXIMITY_NEIGHBORS:
				strLog += "Neighbors,";
				break;
			case PLAYER_PROXIMITY_CLOSE:
				strLog += "Close,";
				break;
			case PLAYER_PROXIMITY_FAR:
				strLog += "Far,";
				break;
			case PLAYER_PROXIMITY_DISTANT:
				strLog += "Distant,";
				break;
			}

			strLog += "-->,";

			switch(eProximity)
			{
			case NO_PLAYER_PROXIMITY:
				strLog += "No player proximity,";
				break;
			case PLAYER_PROXIMITY_NEIGHBORS:
				strLog += "Neighbors,";
				break;
			case PLAYER_PROXIMITY_CLOSE:
				strLog += "Close,";
				break;
			case PLAYER_PROXIMITY_FAR:
				strLog += "Far,";
				break;
			case PLAYER_PROXIMITY_DISTANT:
				strLog += "Distant,";
				break;
			}

			pLog->Msg(strLog);
		}
	}

	m_aiProximityToPlayer.setAt(ePlayer, eProximity);
}

//	--------------------------------------------------------------------------------
/// Figure out how "close" we are to another player (useful for diplomacy, war planning, etc.)
void CvPlayer::DoUpdateProximityToPlayer(PlayerTypes ePlayer)
{
	CvAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(ePlayer < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");

	int iSmallestDistanceBetweenCities = GC.getMap().numPlots();
	int iAverageDistanceBetweenCities = 0;

	int iNumCityConnections = 0;

	CvCity* pLoopMyCity;
	CvCity* pLoopTheirCity;

	int iMyCityLoop;
	int iTheirCityLoop;

	int iTempDistance;

	// Loop through all of MY Cities
	for(pLoopMyCity = firstCity(&iMyCityLoop); pLoopMyCity != NULL; pLoopMyCity = nextCity(&iMyCityLoop))
	{
		// Loop through all of THEIR Cities
		for(pLoopTheirCity = GET_PLAYER(ePlayer).firstCity(&iTheirCityLoop); pLoopTheirCity != NULL; pLoopTheirCity = GET_PLAYER(ePlayer).nextCity(&iTheirCityLoop))
		{
			iNumCityConnections++;

			// Different area or couldn't find path - get distance the hard way
			//if (!bPathFinderSuccess)
			{
				iTempDistance = plotDistance(pLoopMyCity->getX(), pLoopMyCity->getY(), pLoopTheirCity->getX(), pLoopTheirCity->getY());
			}

			// Smallest distance between any two Cities
			if(iTempDistance < iSmallestDistanceBetweenCities)
			{
				iSmallestDistanceBetweenCities = iTempDistance;
			}

			iAverageDistanceBetweenCities += iTempDistance;
		}
	}

	// Seed this value with something reasonable to start.  This will be the value assigned if one player has 0 Cities.
	PlayerProximityTypes eProximity = NO_PLAYER_PROXIMITY;

	if(iNumCityConnections > 0)
	{
		iAverageDistanceBetweenCities /= iNumCityConnections;

		// Closest Cities must be within a certain range
		if(iSmallestDistanceBetweenCities <= /*7*/ GC.getPROXIMITY_NEIGHBORS_CLOSEST_CITY_REQUIREMENT())
		{
			eProximity = PLAYER_PROXIMITY_NEIGHBORS;
		}
		// If our closest Cities are pretty near one another  and our average is less than the max then we can be considered CLOSE (will also look at City average below)
		else if(iSmallestDistanceBetweenCities <= /*11*/ GC.getPROXIMITY_CLOSE_CLOSEST_CITY_POSSIBILITY())
		{
			eProximity = PLAYER_PROXIMITY_CLOSE;
		}

		// If we've already set ourselves as Neighbors, no need to undo what we just did
		if(eProximity != PLAYER_PROXIMITY_NEIGHBORS)
		{
			int iMapFactor = (GC.getMap().getGridWidth() + GC.getMap().getGridHeight()) / 2;

			// Normally base distance on map size, but cap it at a certain point
			// Close can't be so big that it sits on Far's turf
			int iCloseDistance = iMapFactor* /*25*/ GC.getPROXIMITY_CLOSE_DISTANCE_MAP_MULTIPLIER() / 100;
			if(iCloseDistance > /*20*/ GC.getPROXIMITY_CLOSE_DISTANCE_MAX())
			{
				iCloseDistance = /*20*/ GC.getPROXIMITY_CLOSE_DISTANCE_MAX();
			}
			// Close also can't be so small that it sits on Neighbor's turf
			else if(iCloseDistance < /*10*/ GC.getPROXIMITY_CLOSE_DISTANCE_MIN())
			{
				iCloseDistance = /*10*/ GC.getPROXIMITY_CLOSE_DISTANCE_MIN();
			}

			// Far can't be so big that it sits on Distant's turf
			int iFarDistance = iMapFactor* /*45*/ GC.getPROXIMITY_FAR_DISTANCE_MAP_MULTIPLIER() / 100;
			if(iFarDistance > /*50*/ GC.getPROXIMITY_FAR_DISTANCE_MAX())
			{
				iFarDistance = /*50*/ GC.getPROXIMITY_FAR_DISTANCE_MAX();
			}
			// Far also can't be so small that it sits on Close's turf
			else if(iFarDistance < /*20*/ GC.getPROXIMITY_FAR_DISTANCE_MIN())
			{
				iFarDistance = /*20*/ GC.getPROXIMITY_FAR_DISTANCE_MIN();
			}

			// Close
			if(eProximity == PLAYER_PROXIMITY_CLOSE && iAverageDistanceBetweenCities <= iCloseDistance)
			{
				eProximity = PLAYER_PROXIMITY_CLOSE;
			}
			// Far
			else if(iAverageDistanceBetweenCities <= iFarDistance)
			{
				eProximity = PLAYER_PROXIMITY_FAR;
			}
			// Distant
			else
			{
				eProximity = PLAYER_PROXIMITY_DISTANT;
			}
		}

		// Players NOT on the same landmass - bump up PROXIMITY by one level (unless we're already distant or on a water map)
		if(eProximity != PLAYER_PROXIMITY_DISTANT && !(GC.getMap().GetAIMapHint() & 1))
		{
			// Both players have capitals, so we can check their areas to see if they're separated by water
			if(getCapitalCity() != NULL && GET_PLAYER(ePlayer).getCapitalCity() != NULL)
			{
				if(getCapitalCity()->getArea() != GET_PLAYER(ePlayer).getCapitalCity()->getArea())
				{
					eProximity = PlayerProximityTypes(eProximity - 1);
				}
			}
		}
	}

	int iNumMajorsLeft = GC.getGame().countMajorCivsAlive();

	// Only two players left, the farthest we can be considered is "Close"
	if(iNumMajorsLeft == 2)
		eProximity = max(eProximity, PLAYER_PROXIMITY_CLOSE);

	// Four or fewer players left, the farthest we can be considered is "Far"
	else if(iNumMajorsLeft <= 4)
		eProximity = max(eProximity, PLAYER_PROXIMITY_FAR);

	SetProximityToPlayer(ePlayer, eProximity);
}

//	--------------------------------------------------------------------------------
/// Update the beakers accumulated during the term of RAs
void CvPlayer::UpdateResearchAgreements(int iValue)
{
	PlayerTypes ePlayerLoop;
	TeamTypes eTeamLoop;
	for(int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
	{
		ePlayerLoop = (PlayerTypes) iPlayerLoop;
		eTeamLoop = GET_PLAYER(ePlayerLoop).getTeam();
		if(ePlayerLoop == GetID())
			continue;
		if(eTeamLoop == getTeam())
			continue;

		if(GET_TEAM(getTeam()).IsHasResearchAgreement(eTeamLoop))
		{
			// Note that this increases the counter for all players on the other team,
			// even though the RA was only made with one of them.  This is because
			// RAs, though made with players, are restricted and tracked by 1 per team.
			// This must change if future implementations allow for multiple RAs to be
			// made with a particular team.
			ChangeResearchAgreementCounter(ePlayerLoop, iValue);
		}
		else if(GetResearchAgreementCounter(ePlayerLoop) != 0)
		{
			SetResearchAgreementCounter(ePlayerLoop, 0);
		}
	}
}

//	--------------------------------------------------------------------------------
/// Get the beakers accumulated during the RA with a player
int CvPlayer::GetResearchAgreementCounter(PlayerTypes ePlayer) const
{
	CvAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(ePlayer < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiResearchAgreementCounter[ePlayer];
}

//	--------------------------------------------------------------------------------
/// Set the beakers accumulated during the RA with a player
void CvPlayer::SetResearchAgreementCounter(PlayerTypes ePlayer, int iValue)
{
	CvAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(ePlayer < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");

	CvAssertMsg(GetID() != ePlayer, "Trying to make a RA Agreement with oneself. Please send Jon this with your last 5 autosaves and what changelist # you're playing.");

	m_aiResearchAgreementCounter.setAt(ePlayer, iValue);
}

//	--------------------------------------------------------------------------------
/// Change the beakers accumulated during the RA with a player
void CvPlayer::ChangeResearchAgreementCounter(PlayerTypes ePlayer, int iChange)
{
	SetResearchAgreementCounter(ePlayer, GetResearchAgreementCounter(ePlayer) + iChange);
}

//	--------------------------------------------------------------------------------
/// Someone sent us a present!
void CvPlayer::DoCivilianReturnLogic(bool bReturn, PlayerTypes eToPlayer, int iUnitID)
{
	CvUnit* pUnit = getUnit(iUnitID);
	if(!pUnit)
	{
		return;
	}

	CvPlot* pPlot = pUnit->plot();
	if(!pPlot)
	{
		return;
	}

	// Kill any units this guy is transporting
	IDInfo* pUnitNode = pPlot->headUnitNode();
	while(pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(*pUnitNode);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if(NULL != pLoopUnit && pLoopUnit->getTransportUnit() == pUnit)
			pLoopUnit->kill(true);
	}

	// What are the details for the new unit?
	UnitTypes eNewUnitType = pUnit->getUnitType();

	if(!bReturn)
		eNewUnitType = pUnit->getCaptureUnitType(getCivilizationType());

	int iX = pUnit->getX();
	int iY = pUnit->getY();

	// Returns to the previous owner
	if(bReturn)
	{
		pUnit->kill(true);
		CvUnit* pNewUnit = GET_PLAYER(eToPlayer).initUnit(eNewUnitType, iX, iY);
		CvAssert(pNewUnit != NULL);
		if (pNewUnit)
		{
			if (!pNewUnit->jumpToNearestValidPlot())
				pNewUnit->kill(false);	// Could not find a spot!
		}

		// Returned to a city-state
		if(GET_PLAYER(eToPlayer).isMinorCiv())
		{
			int iInfluence = /*45*/ GC.getRETURN_CIVILIAN_FRIENDSHIP();
			GET_PLAYER(eToPlayer).GetMinorCivAI()->ChangeFriendshipWithMajor(GetID(), iInfluence);
		}
		// Returned to major power
		else if(!GET_PLAYER(eToPlayer).isHuman())
		{
			GET_PLAYER(eToPlayer).GetDiplomacyAI()->ChangeNumCiviliansReturnedToMe(GetID(), 1);
		}
	}
	// Kept for oneself
	else
	{
		// Make a new unit because the kind we should capture doesn't match (e.g. Settler to Worker)
		if(eNewUnitType != pUnit->getUnitType())
		{
			pUnit->kill(true);
			CvUnit* pNewUnit = initUnit(eNewUnitType, iX, iY);
			CvAssert(pNewUnit != NULL);
			if (pNewUnit)
				pNewUnit->finishMoves();
		}
	}
}

//	--------------------------------------------------------------------------------
/// Units in the ether coming towards us?
void CvPlayer::DoIncomingUnits()
{
	AI_PERF_FORMAT("AI-perf.csv", ("CvPlayer::DoIncomingUnits, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()) );
	for(int iLoop = 0; iLoop < MAX_PLAYERS; iLoop++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes) iLoop;
		CvAssertMsg(GetIncomingUnitCountdown(eLoopPlayer) >= -1, "Incoming Unit countdown is an invalid value. Please send Jon this with your last 5 autosaves and what changelist # you're playing.");
		if(GetIncomingUnitCountdown(eLoopPlayer) > 0)
		{
			ChangeIncomingUnitCountdown(eLoopPlayer, -1);

			// Time to spawn a new unit
			if(GetIncomingUnitCountdown(eLoopPlayer) == 0)
			{
				// Must have capital to actually spawn unit
				CvCity* pCapital = getCapitalCity();
				if(pCapital)
				{
					CvUnit* pNewUnit = initUnit(GetIncomingUnitType(eLoopPlayer), pCapital->getX(), pCapital->getY());
					CvAssert(pNewUnit);
					if (pNewUnit)
					{
						if(pNewUnit->getDomainType() != DOMAIN_AIR)
						{
							if (!pNewUnit->jumpToNearestValidPlot())
								pNewUnit->kill(false);
						}

						// Gift from a major to a city-state
						if (isMinorCiv() && !GET_PLAYER(eLoopPlayer).isMinorCiv())
						{
							GetMinorCivAI()->DoUnitGiftFromMajor(eLoopPlayer, pNewUnit, /*bDistanceGift*/ true);
						}
					}
				}

				// Reset stuff
				SetIncomingUnitCountdown(eLoopPlayer, -1);
				SetIncomingUnitType(eLoopPlayer, NO_UNIT);
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Someone sent us a present!
void CvPlayer::DoDistanceGift(PlayerTypes eFromPlayer, CvUnit* pUnit)
{
	if(!pUnit)
	{
		return;
	}

	CvPlot* pPlot = pUnit->plot();
	if(!pPlot)
	{
		return;
	}

#ifdef NET_FIX_SINGLE_USE_ABILITY_DUPE
	if (pUnit->isDelayedDeath())
	{
		SLOG("isDelayedDeath is true unit ID: %d", pUnit->GetID());
		return;
	}
#endif
	// Also add any units this guy is transporting
	IDInfo* pUnitNode = pPlot->headUnitNode();
	while(pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(*pUnitNode);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if(NULL != pLoopUnit && pLoopUnit->getTransportUnit() == pUnit)
			AddIncomingUnit(eFromPlayer, pLoopUnit);
	}

	AddIncomingUnit(eFromPlayer, pUnit);
}

//	--------------------------------------------------------------------------------
/// Someone sent us a present!
void CvPlayer::AddIncomingUnit(PlayerTypes eFromPlayer, CvUnit* pUnit)
{
	UnitTypes eUnitType = pUnit->getUnitType();

	// Gift to a minor civ for friendship
	if(isMinorCiv() && eFromPlayer < MAX_MAJOR_CIVS)
	{
		CvAssertMsg(GetIncomingUnitType(eFromPlayer) == NO_UNIT, "Adding incoming unit when one is already on its way. Please send Anton your save file and version.");
		CvAssertMsg(GetIncomingUnitCountdown(eFromPlayer) == -1, "Adding incoming unit when one is already on its way. Please send Anton your save file and version.");
		if(GetIncomingUnitCountdown(eFromPlayer) == -1)
		{
			SetIncomingUnitCountdown(eFromPlayer, GC.getMINOR_UNIT_GIFT_TRAVEL_TURNS());
			SetIncomingUnitType(eFromPlayer, eUnitType);
		}

		// Get rid of the old unit
		pUnit->kill(true);
	}
	// Gift from minor civ to this major civ (ex. Austria UA, but NOT Militaristic unit spawning, that is handled elsewhere)
	else if(!isMinorCiv() && GET_PLAYER(eFromPlayer).isMinorCiv())
	{
		int iX = pUnit->getX();
		int iY = pUnit->getY();
		UnitTypes eType = pUnit->getUnitType();

		// Get rid of the old unit
		pUnit->kill(true);

		// Add the new unit in its place
		if(eType != NO_UNIT)
		{
			CvUnit* pNewUnit = initUnit(eType, iX, iY);
			CvAssert(pNewUnit);
			if (pNewUnit)
				pNewUnit->finishMoves();
		}
	}
	else
	{
		CvAssertMsg(false, "Unexpected case for adding an incoming unit for this player. Please send Anton your save file and version.");
	}
}

//	--------------------------------------------------------------------------------
/// Units in the ether coming towards us?
UnitTypes CvPlayer::GetIncomingUnitType(PlayerTypes eFromPlayer) const
{
	CvAssertMsg(eFromPlayer >= 0, "eFromPlayer is expected to be non-negative (invalid Index)");
	CvAssertMsg(eFromPlayer < MAX_PLAYERS, "eFromPlayer is expected to be within maximum bounds (invalid Index)");
	return (UnitTypes) m_aiIncomingUnitTypes[eFromPlayer];
}

//	--------------------------------------------------------------------------------
/// Units in the ether coming towards us?
void CvPlayer::SetIncomingUnitType(PlayerTypes eFromPlayer, UnitTypes eUnitType)
{
	CvAssertMsg(eFromPlayer >= 0, "eFromPlayer is expected to be non-negative (invalid Index)");
	CvAssertMsg(eFromPlayer < MAX_PLAYERS, "eFromPlayer is expected to be within maximum bounds (invalid Index)");

	CvAssertMsg(eUnitType >= NO_UNIT, "eUnitType is expected to be non-negative (invalid Index)");
	CvAssertMsg(eUnitType < GC.getNumUnitInfos(), "eUnitType is expected to be within maximum bounds (invalid Index)");

	if(eUnitType != m_aiIncomingUnitTypes[eFromPlayer])
	{
		m_aiIncomingUnitTypes.setAt(eFromPlayer, eUnitType);
	}
}

//	--------------------------------------------------------------------------------
/// Units in the ether coming towards us?
int CvPlayer::GetIncomingUnitCountdown(PlayerTypes eFromPlayer) const
{
	CvAssertMsg(eFromPlayer >= 0, "eFromPlayer is expected to be non-negative (invalid Index)");
	CvAssertMsg(eFromPlayer < MAX_PLAYERS, "eFromPlayer is expected to be within maximum bounds (invalid Index)");
	return m_aiIncomingUnitCountdowns[eFromPlayer];
}

//	--------------------------------------------------------------------------------
/// Units in the ether coming towards us?
void CvPlayer::SetIncomingUnitCountdown(PlayerTypes eFromPlayer, int iNumTurns)
{
	CvAssertMsg(eFromPlayer >= 0, "eFromPlayer is expected to be non-negative (invalid Index)");
	CvAssertMsg(eFromPlayer < MAX_PLAYERS, "eFromPlayer is expected to be within maximum bounds (invalid Index)");

	if(iNumTurns != m_aiIncomingUnitCountdowns[eFromPlayer])
		m_aiIncomingUnitCountdowns.setAt(eFromPlayer, iNumTurns);
}

//	--------------------------------------------------------------------------------
/// Units in the ether coming towards us?
void CvPlayer::ChangeIncomingUnitCountdown(PlayerTypes eFromPlayer, int iChange)
{
	if(iChange != 0)
		SetIncomingUnitCountdown(eFromPlayer, GetIncomingUnitCountdown(eFromPlayer) + iChange);
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isOption(PlayerOptionTypes eID) const
{
	for (PlayerOptionsVector::const_iterator itr = m_aOptions.begin(); itr != m_aOptions.end(); ++itr )
	{
		if ((*itr).first == eID)
			return (*itr).second != 0;
	}
	return false;
}


//	--------------------------------------------------------------------------------
void CvPlayer::setOption(PlayerOptionTypes eID, bool bNewValue)
{
	int iIndex = 0;
	for (PlayerOptionsVector::const_iterator itr = m_aOptions.begin(); itr != m_aOptions.end(); ++itr )
	{
		if ((*itr).first == eID)
		{
			m_aOptions.setAt(iIndex, PlayerOptionEntry((uint)eID, bNewValue?1:0));
			return;
		}
		++iIndex;
	}

	m_aOptions.push_back(PlayerOptionEntry((uint)eID, bNewValue?1:0));
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isPlayable() const
{
	return CvPreGame::isPlayable(GetID());
}

//	--------------------------------------------------------------------------------
void CvPlayer::setPlayable(bool bNewValue)
{
	CvPreGame::setPlayable(GetID(), bNewValue);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNumResourceUsed(ResourceTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiNumResourceUsed[eIndex];
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeNumResourceUsed(ResourceTypes eIndex, int iChange)
{
	CvAssert(eIndex >= 0);
	CvAssert(eIndex < GC.getNumResourceInfos());

	if(iChange != 0)
	{
		m_paiNumResourceUsed.setAt(eIndex, m_paiNumResourceUsed[eIndex] + iChange);
	}

	if(iChange > 0)
		DoTestOverResourceNotification(eIndex);

	GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);

	CvAssert(m_paiNumResourceUsed[eIndex] >= 0);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNumResourceTotal(ResourceTypes eIndex, bool bIncludeImport) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	// Mod applied to how much we have?
	CvResourceInfo *pkResource = GC.getResourceInfo(eIndex);
	if (pkResource == NULL)
	{
		return 0;
	}

	int iTotalNumResource = m_paiNumResourceTotal[eIndex];

	if(pkResource->getResourceUsage() == RESOURCEUSAGE_STRATEGIC)
	{
		if(GetStrategicResourceMod() != 0)
		{
			iTotalNumResource *= GetStrategicResourceMod();
			iTotalNumResource /= 100;
		}
	}

	if(bIncludeImport)
	{
		iTotalNumResource += getResourceImport(eIndex);
		iTotalNumResource += getResourceFromMinors(eIndex);
		iTotalNumResource += getResourceSiphoned(eIndex);
	}

	iTotalNumResource -= getResourceExport(eIndex);

	return iTotalNumResource;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeNumResourceTotal(ResourceTypes eIndex, int iChange, bool bIgnoreResourceWarning)
{
	CvAssert(eIndex >= 0);
	CvAssert(eIndex < GC.getNumResourceInfos());

	if(iChange != 0)
	{
		m_paiNumResourceTotal.setAt(eIndex, m_paiNumResourceTotal[eIndex] + iChange);

		// Minors with an Ally give their Resources to their friend (awww)
		if(isMinorCiv())
		{
			PlayerTypes eBestRelationsPlayer = GetMinorCivAI()->GetAlly();

			if(eBestRelationsPlayer != NO_PLAYER)
			{
				ResourceUsageTypes eUsage = GC.getResourceInfo(eIndex)->getResourceUsage();

				if(eUsage == RESOURCEUSAGE_STRATEGIC || eUsage == RESOURCEUSAGE_LUXURY)
				{
					// Someone new is getting the bonus
					if(eBestRelationsPlayer != NO_PLAYER)
					{
						GET_PLAYER(eBestRelationsPlayer).changeResourceFromMinors(eIndex, iChange);
						changeResourceExport(eIndex, iChange);

						CvNotifications* pNotifications = GET_PLAYER(eBestRelationsPlayer).GetNotifications();
						if(pNotifications && !GetMinorCivAI()->IsDisableNotifications())
						{
							Localization::String strMessage;
							Localization::String strSummary;

							// Adding Resources
							if(iChange > 0)
							{
								strMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_MINOR_BFF_NEW_RESOURCE");
								strMessage << getNameKey() << GC.getResourceInfo(eIndex)->GetDescriptionKey();
								strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_MINOR_BFF_NEW_RESOURCE");
								strSummary << getNameKey() << GC.getResourceInfo(eIndex)->GetDescriptionKey();
							}
							// Lost Resources
							else
							{
								strMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_MINOR_BFF_LOST_RESOURCE");
								strMessage << getNameKey() << GC.getResourceInfo(eIndex)->GetDescriptionKey();
								strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_MINOR_BFF_LOST_RESOURCE");
								strSummary << getNameKey() << GC.getResourceInfo(eIndex)->GetDescriptionKey();
							}

							int iX = -1;
							int iY = -1;

							CvCity* capCity = getCapitalCity();

							if(capCity != NULL)
							{
								iX = capCity->getX();
								iY = capCity->getY();
							}

							pNotifications->Add(NOTIFICATION_MINOR, strMessage.toUTF8(), strSummary.toUTF8(), iX, iY, -1);
						}
					}
				}
			}
		}

		// Any players siphoning resources from us need to be updated as well
		for (int iPlayerLoop = 0; iPlayerLoop < MAX_PLAYERS; iPlayerLoop++)
		{
			GET_PLAYER((PlayerTypes)iPlayerLoop).UpdateResourcesSiphoned();
		}
	}

	if(iChange < 0 && !bIgnoreResourceWarning)
		DoTestOverResourceNotification(eIndex);

	GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);

	CvAssert(m_paiNumResourceTotal[eIndex] >= 0);
}

//	--------------------------------------------------------------------------------
/// Do we get copies of each type of luxury connected by eFromPlayer?
int CvPlayer::getSiphonLuxuryCount(PlayerTypes eFromPlayer) const
{
	CvAssertMsg(eFromPlayer >= 0, "eFromPlayer is expected to be non-negative (invalid Index)");
	CvAssertMsg(eFromPlayer < MAX_PLAYERS, "eFromPlayer is expected to be within maximum bounds (invalid Index)");

	return m_aiSiphonLuxuryCount[eFromPlayer];
}

//	--------------------------------------------------------------------------------
/// Change number of copies we get of luxury types connected by eFromPlayer
void CvPlayer::changeSiphonLuxuryCount(PlayerTypes eFromPlayer, int iChange)
{
	CvAssertMsg(eFromPlayer >= 0, "eFromPlayer is expected to be non-negative (invalid Index)");
	CvAssertMsg(eFromPlayer < MAX_PLAYERS, "eFromPlayer is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiSiphonLuxuryCount[eFromPlayer] = m_aiSiphonLuxuryCount[eFromPlayer] + iChange;
		CvAssert(getSiphonLuxuryCount(eFromPlayer) >= 0);

		UpdateResourcesSiphoned();
	}
}

//	--------------------------------------------------------------------------------
/// Count up the number of resources we have been siphoning from others and compare it to how many 
/// we are now allowed to siphon.  Change our resource count if there is a discrepancy.
void CvPlayer::UpdateResourcesSiphoned()
{
	FStaticVector<int, 64, true, c_eCiv5GameplayDLL> vDeltas;
	
	// Subtract all currently siphoned resources
	for (int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
	{
		ResourceTypes eResourceLoop = (ResourceTypes) iResourceLoop;
		vDeltas.push_back(-1 * getResourceSiphoned(eResourceLoop));
	}

	// Add back in valid siphoned resources
	for (int iPlayerLoop = 0; iPlayerLoop < MAX_PLAYERS; iPlayerLoop++)
	{
		PlayerTypes ePlayerLoop = (PlayerTypes) iPlayerLoop;
		int iSiphonLuxuryCount = getSiphonLuxuryCount(ePlayerLoop);
		if (iSiphonLuxuryCount > 0)
		{
			for (int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
			{
				ResourceTypes eResourceLoop = (ResourceTypes) iResourceLoop;
				CvResourceInfo* pInfo = GC.getResourceInfo(eResourceLoop);
				// Is it a luxury?
				if (pInfo && pInfo->getResourceUsage() == RESOURCEUSAGE_LUXURY)
				{
					// Do they have at least one of this type, even if it was exported?
					if (GET_PLAYER(ePlayerLoop).getNumResourceTotal(eResourceLoop, /*bIncludeImport*/ false) > 0 || GET_PLAYER(ePlayerLoop).getResourceExport(eResourceLoop) > 0)
					{
						vDeltas[eResourceLoop] += iSiphonLuxuryCount;
					}
				}
			}
		}
	}

	// Propagate any actual changes
	for (uint i = 0; i < vDeltas.size(); i++)
	{
		if (vDeltas[i] != 0)
		{
			ResourceTypes eResource = (ResourceTypes) i;
			changeResourceSiphoned(eResource, vDeltas[i]);
		}
	}
}

//	--------------------------------------------------------------------------------
/// Are we over our resource limit? If so, give out a notification
void CvPlayer::DoTestOverResourceNotification(ResourceTypes eIndex)
{
	if(getNumResourceAvailable(eIndex, true) < 0)
	{
		const CvResourceInfo* pkResourceInfo = GC.getResourceInfo(eIndex);
		if(pkResourceInfo != NULL && pkResourceInfo->getResourceUsage() == RESOURCEUSAGE_STRATEGIC)
		{
			CvNotifications* pNotifications = GetNotifications();
			if(pNotifications)
			{
				Localization::String strText = Localization::Lookup("TXT_KEY_NOTIFICATION_OVER_RESOURCE_LIMIT");
				strText << pkResourceInfo->GetTextKey();
				Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_OVER_RESOURCE_LIMIT");
				strSummary << pkResourceInfo->GetTextKey();
				pNotifications->Add(NOTIFICATION_DEMAND_RESOURCE, strText.toUTF8(), strSummary.toUTF8(), -1, -1, eIndex);
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Is our collection of Strategic Resources modified?
int CvPlayer::GetStrategicResourceMod() const
{
	return m_iStrategicResourceMod;
}

//	--------------------------------------------------------------------------------
/// Is our collection of Strategic Resources modified?
void CvPlayer::ChangeStrategicResourceMod(int iChange)
{
	m_iStrategicResourceMod += iChange;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNumResourceAvailable(ResourceTypes eIndex, bool bIncludeImport) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return getNumResourceTotal(eIndex, bIncludeImport) - getNumResourceUsed(eIndex);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getResourceGiftedToMinors(ResourceTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiResourceGiftedToMinors[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeResourceGiftedToMinors(ResourceTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_paiResourceGiftedToMinors.setAt(eIndex, m_paiResourceGiftedToMinors[eIndex] + iChange);
		CvAssert(getResourceGiftedToMinors(eIndex) >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getResourceExport(ResourceTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiResourceExport[eIndex];
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeResourceExport(ResourceTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_paiResourceExport.setAt(eIndex, m_paiResourceExport[eIndex] + iChange);
		CvAssert(getResourceExport(eIndex) >= 0);

		DoUpdateHappiness();
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getResourceImport(ResourceTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiResourceImport[eIndex];
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeResourceImport(ResourceTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_paiResourceImport.setAt(eIndex, m_paiResourceImport[eIndex] + iChange);
		CvAssert(getResourceImport(eIndex) >= 0);

		DoUpdateHappiness();
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getResourceFromMinors(ResourceTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	int iNumResourceFromMinors = m_paiResourceFromMinors[eIndex];

	// Resource bonus doubles quantity of Resources from Minors (Policies, etc.)
	if(IsMinorResourceBonus())
	{
		iNumResourceFromMinors *= /*200*/ GC.getMINOR_POLICY_RESOURCE_MULTIPLIER();
		iNumResourceFromMinors /= 100;
	}

	return iNumResourceFromMinors;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeResourceFromMinors(ResourceTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_paiResourceFromMinors.setAt(eIndex, m_paiResourceFromMinors[eIndex] + iChange);
		CvAssert(getResourceFromMinors(eIndex) >= 0);

		DoUpdateHappiness();
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getResourceSiphoned(ResourceTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	int iNumResourceSiphoned = m_paiResourcesSiphoned[eIndex];

	return iNumResourceSiphoned;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeResourceSiphoned(ResourceTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiResourcesSiphoned.setAt(eIndex, m_paiResourcesSiphoned[eIndex] + iChange);
		CvAssert(getResourceSiphoned(eIndex) >= 0);

		DoUpdateHappiness();
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getResourceInOwnedPlots(ResourceTypes eIndex)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumResourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	int iCount = 0;

	// Loop through all plots
	const CvPlotsVector& aiPlots = GetPlots();
	for (uint uiPlotIndex = 0; uiPlotIndex < aiPlots.size(); uiPlotIndex++)
	{
		if (aiPlots[uiPlotIndex] == -1)
			continue;

		CvPlot* pPlot = GC.getMap().plotByIndex(aiPlots[uiPlotIndex]);
		if (pPlot && pPlot->getResourceType(getTeam()) == eIndex)
		{
			iCount++;
		}
	}

	return iCount;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getTotalImprovementsBuilt() const
{
	return m_iTotalImprovementsBuilt;
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeTotalImprovementsBuilt(int iChange)
{
	m_iTotalImprovementsBuilt = (m_iTotalImprovementsBuilt + iChange);
	CvAssert(getTotalImprovementsBuilt() >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getImprovementCount(ImprovementTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumImprovementInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiImprovementCount[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeImprovementCount(ImprovementTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumImprovementInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiImprovementCount.setAt(eIndex, m_paiImprovementCount[eIndex] + iChange);
	CvAssert(getImprovementCount(eIndex) >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getGreatPersonImprovementCount()
{
	int iCount = 0;
	for (int i = 0; i < GC.getNumImprovementInfos(); i++)
	{
		ImprovementTypes e = (ImprovementTypes)i;
		CvImprovementEntry* pInfo = GC.getImprovementInfo(e);
		if (pInfo && pInfo->IsCreatedByGreatPerson())
		{
			iCount += getImprovementCount(e);
		}
	}
	return iCount;
}


//	--------------------------------------------------------------------------------
int CvPlayer::getFreeBuildingCount(BuildingTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiFreeBuildingCount[eIndex];
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isBuildingFree(BuildingTypes eIndex)	const
{
	return (getFreeBuildingCount(eIndex) > 0);
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeFreeBuildingCount(BuildingTypes eIndex, int iChange)
{
	CvCity* pLoopCity;
	int iOldFreeBuildingCount;
	int iLoop;

	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		iOldFreeBuildingCount = getFreeBuildingCount(eIndex);

		m_paiFreeBuildingCount.setAt(eIndex, m_paiFreeBuildingCount[eIndex] + iChange);
		CvAssert(getFreeBuildingCount(eIndex) >= 0);

		if(iOldFreeBuildingCount == 0)
		{
			CvAssertMsg(getFreeBuildingCount(eIndex) > 0, "getFreeBuildingCount(eIndex) is expected to be greater than 0");

			for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				pLoopCity->GetCityBuildings()->SetNumFreeBuilding(eIndex, 1);
			}
		}
		else if(getFreeBuildingCount(eIndex) == 0)
		{
			CvAssertMsg(iOldFreeBuildingCount > 0, "iOldFreeBuildingCount is expected to be greater than 0");

			for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				pLoopCity->GetCityBuildings()->SetNumFreeBuilding(eIndex, 0);
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Is ePromotion a free promotion?
int CvPlayer::GetFreePromotionCount(PromotionTypes ePromotion) const
{
	CvAssertMsg(ePromotion >= 0, "ePromotion is expected to be non-negative (invalid Index)");
	CvAssertMsg(ePromotion < GC.getNumPromotionInfos(), "ePromotion is expected to be within maximum bounds (invalid Index)");
	return m_paiFreePromotionCount[ePromotion];
}

//	--------------------------------------------------------------------------------
/// Is ePromotion a free promotion?
bool CvPlayer::IsFreePromotion(PromotionTypes ePromotion)	const
{
	return (GetFreePromotionCount(ePromotion) > 0);
}

//	--------------------------------------------------------------------------------
/// Is ePromotion a free promotion?
void CvPlayer::ChangeFreePromotionCount(PromotionTypes ePromotion, int iChange)
{
	CvAssertMsg(ePromotion >= 0, "ePromotion is expected to be non-negative (invalid Index)");
	CvAssertMsg(ePromotion < GC.getNumPromotionInfos(), "ePromotion is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		bool bWasFree = IsFreePromotion(ePromotion);

		m_paiFreePromotionCount.setAt(ePromotion, m_paiFreePromotionCount[ePromotion] + iChange);

		CvAssert(GetFreePromotionCount(ePromotion) >= 0);

		// This promotion is now set to be free, but wasn't before we called this function
		if(IsFreePromotion(ePromotion) && !bWasFree)
		{
			// Loop through Units
			CvUnit* pLoopUnit;

			int iLoop;
			for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
			{
				// Valid Promotion for this Unit?
				if(::IsPromotionValidForUnitCombatType(ePromotion, pLoopUnit->getUnitType()))
				{
					pLoopUnit->setHasPromotion(ePromotion, true);
				}

				else if(::IsPromotionValidForCivilianUnitType(ePromotion, pLoopUnit->getUnitType()))
				{
					pLoopUnit->setHasPromotion(ePromotion, true);
				}
			}
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getUnitCombatProductionModifiers(UnitCombatTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumUnitCombatClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiUnitCombatProductionModifiers[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeUnitCombatProductionModifiers(UnitCombatTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumUnitCombatClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_paiUnitCombatProductionModifiers.setAt(eIndex, m_paiUnitCombatProductionModifiers[eIndex] + iChange);
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getUnitCombatFreeExperiences(UnitCombatTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumUnitCombatClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiUnitCombatFreeExperiences[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeUnitCombatFreeExperiences(UnitCombatTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumUnitCombatClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_paiUnitCombatFreeExperiences.setAt(eIndex, m_paiUnitCombatFreeExperiences[eIndex] + iChange);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getUnitClassCount(UnitClassTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiUnitClassCount[eIndex];
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eIndex);
	if(pkUnitClassInfo == NULL)
	{
		return false;
	}

	if(!isNationalUnitClass(eIndex))
	{
		return false;
	}

	CvAssertMsg(getUnitClassCount(eIndex) <= pkUnitClassInfo->getMaxPlayerInstances(), "getUnitClassCount is expected to be less than maximum bound of MaxPlayerInstances (invalid index)");

	return ((getUnitClassCount(eIndex) + iExtra) >= pkUnitClassInfo->getMaxPlayerInstances());
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeUnitClassCount(UnitClassTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiUnitClassCount.setAt(eIndex, m_paiUnitClassCount[eIndex] + iChange);
	CvAssert(getUnitClassCount(eIndex) >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getUnitClassMaking(UnitClassTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiUnitClassMaking[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeUnitClassMaking(UnitClassTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_paiUnitClassMaking.setAt(eIndex, m_paiUnitClassMaking[eIndex] + iChange);
		CvAssert(getUnitClassMaking(eIndex) >= 0);

		CvCivilizationInfo& playerCivilizationInfo = getCivilizationInfo();
		UnitTypes eUnit = static_cast<UnitTypes>(playerCivilizationInfo.getCivilizationUnits(eIndex));
		CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
		if(pkUnitInfo)
		{
			// Builder Limit
			if(pkUnitInfo->GetWorkRate() > 0 && pkUnitInfo->GetDomainType() == DOMAIN_LAND)
			{
				ChangeNumBuilders(iChange);
			}

			// Update the amount of a Resource used up by Units in Production
			for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
			{
				ResourceTypes eResource = static_cast<ResourceTypes>(iResourceLoop);
				CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
				if(pkResource)
				{
					if(pkUnitInfo->GetResourceQuantityRequirement(iResourceLoop) > 0)
					{
						changeNumResourceUsed(eResource, iChange * pkUnitInfo->GetResourceQuantityRequirement(iResourceLoop));
					}
				}
			}

			if(GetID() == GC.getGame().getActivePlayer())
			{
				GC.GetEngineUserInterface()->setDirty(Help_DIRTY_BIT, true);
			}
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getUnitClassCountPlusMaking(UnitClassTypes eIndex) const
{
	return (getUnitClassCount(eIndex) + getUnitClassMaking(eIndex));
}


//	--------------------------------------------------------------------------------
int CvPlayer::getBuildingClassCount(BuildingClassTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiBuildingClassCount[eIndex];
}


//	--------------------------------------------------------------------------------
bool CvPlayer::isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eIndex);
	if(pkBuildingClassInfo == NULL)
	{
		CvAssertMsg(false, "This should never happen...");
		return false;
	}

#ifdef FIX_IS_NATIONAL_WONDER_CLASS
	if (pkBuildingClassInfo->getMaxPlayerInstances() == -1)
	{
		return false;
	}
#else
	if(!isNationalWonderClass(*pkBuildingClassInfo))
	{
		return false;
	}
#endif

	CvAssertMsg(getBuildingClassCount(eIndex) <= (pkBuildingClassInfo->getMaxPlayerInstances() + pkBuildingClassInfo->getExtraPlayerInstances()), "BuildingClassCount is expected to be less than or match the number of max player instances plus extra player instances");

	return ((getBuildingClassCount(eIndex) + iExtra) >= (pkBuildingClassInfo->getMaxPlayerInstances() + pkBuildingClassInfo->getExtraPlayerInstances()));
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeBuildingClassCount(BuildingClassTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiBuildingClassCount.setAt(eIndex, m_paiBuildingClassCount[eIndex] + iChange);
	CvAssert(getBuildingClassCount(eIndex) >= 0);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getBuildingClassMaking(BuildingClassTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiBuildingClassMaking[eIndex];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeBuildingClassMaking(BuildingClassTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_paiBuildingClassMaking.setAt(eIndex, m_paiBuildingClassMaking[eIndex] + iChange);
		CvAssert(getBuildingClassMaking(eIndex) >= 0);

		const BuildingTypes eBuilding = (BuildingTypes) getCivilizationInfo().getCivilizationBuildings(eIndex);
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
		if(pkBuildingInfo)
		{
			// Update the amount of a Resource used up by Buildings in Production
			for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
			{
				const ResourceTypes eResource = static_cast<ResourceTypes>(iResourceLoop);
				CvResourceInfo* pkResourceInfo = GC.getResourceInfo(eResource);
				if(pkResourceInfo)
				{
					if(pkBuildingInfo->GetResourceQuantityRequirement(iResourceLoop) > 0)
					{
						changeNumResourceUsed(eResource, iChange * pkBuildingInfo->GetResourceQuantityRequirement(iResourceLoop));
					}
				}

			}
		}


		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(Help_DIRTY_BIT, true);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getBuildingClassCountPlusMaking(BuildingClassTypes eIndex) const
{
	return (getBuildingClassCount(eIndex) + getBuildingClassMaking(eIndex));
}


//	--------------------------------------------------------------------------------
// The following two functions are only used to keep track of how many Projects are in progress so we know what each player's Resource situation is
// Check out CvTeam::getProjectMaking() for something used more
int CvPlayer::getProjectMaking(ProjectTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiProjectMaking[eIndex];
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeProjectMaking(ProjectTypes eIndex, int iChange)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		m_paiProjectMaking.setAt(eIndex, m_paiProjectMaking[eIndex] + iChange);
		CvAssert(getProjectMaking(eIndex) >= 0);

		// Update the amount of a Resource used up by Projects in Production
		for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
		{
			if(GC.getProjectInfo(eIndex)->GetResourceQuantityRequirement(iResourceLoop) > 0)
			{
				changeNumResourceUsed((ResourceTypes) iResourceLoop, iChange * GC.getProjectInfo(eIndex)->GetResourceQuantityRequirement(iResourceLoop));
			}
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getHurryCount(HurryTypes eIndex) const
{
	CvAssert(eIndex >= 0);
	CvAssert(eIndex < GC.getNumHurryInfos());
	return m_paiHurryCount[eIndex];
}


//	--------------------------------------------------------------------------------
// Do we have access to this Hurry type?
bool CvPlayer::IsHasAccessToHurry(HurryTypes eIndex) const
{
	return (getHurryCount(eIndex) > 0);
}

//	--------------------------------------------------------------------------------
/// Can we use this Hurry RIGHT NOW?
bool CvPlayer::IsCanHurry(HurryTypes eIndex) const
{
	CvHurryInfo* pkHurryInfo = GC.getHurryInfo(eIndex);
	if(pkHurryInfo == NULL)
		return false;

	int iCost = GetHurryGoldCost(eIndex);

	// Can we pay for this Hurry?
	if(iCost < 0 || GetTreasury()->GetGold() < iCost)
	{
		return false;
	}

	// Science Rushing
	if(pkHurryInfo->getGoldPerBeaker() > 0)
	{
		return true;
	}

	// Culture Rushing
	if(pkHurryInfo->getGoldPerCulture() > 0)
	{
		// If we already have enough Culture for the next Policy, there's nothing to rush!
		if(getNextPolicyCost() > getJONSCulture())
		{
			return true;
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
/// How much Gold does it cost us to Hurry? (whatever the applicable item is)
int CvPlayer::GetHurryGoldCost(HurryTypes eHurry) const
{
	int iGold = -1;

	CvHurryInfo* pkHurryInfo = GC.getHurryInfo(eHurry);
	if(pkHurryInfo == NULL)
	{
		//This should never happen.
		return -1;
	}

	// Science Rushing
	if(pkHurryInfo->getGoldPerBeaker() > 0)
	{
		TechTypes eTech = GetPlayerTechs()->GetCurrentResearch();

		if(eTech != NO_TECH)
		{
			int iTotalCost = GET_TEAM(getTeam()).GetTeamTechs()->GetResearchCost(eTech);
			int iResearchLeft = GET_TEAM(getTeam()).GetTeamTechs()->GetResearchLeft(eTech);

			// Cost of Gold rushing based on the ORIGINAL Research price
			int iGoldForFullPrice = iTotalCost * pkHurryInfo->getGoldPerBeaker();
			iGoldForFullPrice = (int) pow((double) iGoldForFullPrice, (double) /*1.10f*/ GC.getHURRY_GOLD_TECH_EXPONENT());

			// Figure out the actual cost by comparing what's left to the original Research cost, and multiplying that by the amount to Gold rush the original cost
			iGold = (iGoldForFullPrice * iResearchLeft / iTotalCost);
		}
	}

	// Culture Rushing
	if(pkHurryInfo->getGoldPerCulture() > 0)
	{
		int iCurrentPolicyCost = getNextPolicyCost();

		if(iCurrentPolicyCost > 0)
		{
			int iCultureLeft = iCurrentPolicyCost - getJONSCulture();

			// Cost of Gold rushing based on the ORIGINAL Culture price
			int iGoldForFullPrice = iCurrentPolicyCost * pkHurryInfo->getGoldPerCulture();
			iGoldForFullPrice = (int) pow((double) iGoldForFullPrice, (double) /*1.10f*/ GC.getHURRY_GOLD_CULTURE_EXPONENT());

			// Figure out the actual cost by comparing what's left to the original Culture cost, and multiplying that by the amount to Gold rush the original cost
			iGold = (iGoldForFullPrice * iCultureLeft / iCurrentPolicyCost);
		}
	}

	return iGold;
}

//	--------------------------------------------------------------------------------
/// Hurry something!
void CvPlayer::DoHurry(HurryTypes eIndex)
{
	CvHurryInfo* pkHurryInfo = GC.getHurryInfo(eIndex);
	if(pkHurryInfo)
	{
		if(IsCanHurry(eIndex))
		{
			int iGoldCost = GetHurryGoldCost(eIndex);
			GetTreasury()->ChangeGold(-iGoldCost);

			// Science Rushing
			if(pkHurryInfo->getGoldPerBeaker() > 0)
			{
				TechTypes eTech = GetPlayerTechs()->GetCurrentResearch();

				GET_TEAM(getTeam()).setHasTech(eTech, true, GetID(), false, false);
			}

			// Culture Rushing
			if(pkHurryInfo->getGoldPerCulture() > 0)
			{
				setJONSCulture(getNextPolicyCost());
			}
		}
	}
}


//	--------------------------------------------------------------------------------
bool CvPlayer::canPopRush()
{
	return (m_iPopRushHurryCount > 0);
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeHurryCount(HurryTypes eIndex, int iChange)
{
	CvAssert(eIndex >= 0);
	CvAssert(eIndex < GC.getNumHurryInfos());

	int oldHurryCount = m_paiHurryCount[eIndex];
	m_paiHurryCount.setAt(eIndex, m_paiHurryCount[eIndex] + iChange);
	CvAssert(getHurryCount(eIndex) >= 0);

	CvHurryInfo* pkHurryInfo = GC.getHurryInfo(eIndex);
	if(pkHurryInfo == NULL)
		return;

	// if we just went from 0 to 1 (or the reverse)
	if((oldHurryCount > 0) != (m_paiHurryCount[eIndex] > 0))
	{
		// does this hurry reduce population?
		if(pkHurryInfo->getProductionPerPopulation() > 0)
		{
			m_iPopRushHurryCount += iChange;
			CvAssert(m_iPopRushHurryCount >= 0);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getHurryModifier(HurryTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumHurryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiHurryModifier[eIndex];
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeHurryModifier(HurryTypes eIndex, int iChange)
{
	if(iChange != 0)
	{
		CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
		CvAssertMsg(eIndex < GC.getNumHurryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
		m_paiHurryModifier.setAt(eIndex, m_paiHurryModifier[eIndex] + iChange);
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::setResearchingTech(TechTypes eIndex, bool bNewValue)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumTechInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(m_pPlayerTechs->IsResearchingTech(eIndex) != bNewValue)
	{
		GetPlayerTechs()->SetResearchingTech(eIndex, bNewValue);

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(Popup_DIRTY_BIT, true); // to check whether we still need the tech chooser popup
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2) const
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 expected to be >= 0");
	CvAssertMsg(eIndex1 < GC.getNumSpecialistInfos(), "eIndex1 expected to be < GC.getNumSpecialistInfos()");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 expected to be >= 0");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 expected to be < NUM_YIELD_TYPES");
	return m_ppaaiSpecialistExtraYield[eIndex1][eIndex2];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 expected to be >= 0");
	CvAssertMsg(eIndex1 < GC.getNumSpecialistInfos(), "eIndex1 expected to be < GC.getNumSpecialistInfos()");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 expected to be >= 0");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		Firaxis::Array<int, NUM_YIELD_TYPES> yields = m_ppaaiSpecialistExtraYield[eIndex1];
		yields[eIndex2] = (m_ppaaiSpecialistExtraYield[eIndex1][eIndex2] + iChange);
		m_ppaaiSpecialistExtraYield.setAt(eIndex1, yields);
		CvAssert(getSpecialistExtraYield(eIndex1, eIndex2) >= 0);

		updateExtraSpecialistYield();
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumImprovementInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_ppaaiImprovementYieldChange[eIndex1][eIndex2];
}


//	--------------------------------------------------------------------------------
void CvPlayer::changeImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumImprovementInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");

	if(iChange != 0)
	{
		CvAssertMsg(iChange > -50 && iChange < 50, "GAMEPLAY: Yield for a plot is either negative or a ridiculously large number. Please send Jon this with your last 5 autosaves and what changelist # you're playing.");

		Firaxis::Array<int, NUM_YIELD_TYPES> yields = m_ppaaiImprovementYieldChange[eIndex1];
		yields[eIndex2] = (m_ppaaiImprovementYieldChange[eIndex1][eIndex2] + iChange);
		m_ppaaiImprovementYieldChange.setAt(eIndex1, yields);
		CvAssert(getImprovementYieldChange(eIndex1, eIndex2) >= 0);

		updateYield();
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::removeFromArmy(int iArmyID, int iID)
{
	bool bRemoved = false;
	CvArmyAI* pThisArmyAI = getArmyAI(iArmyID);
	if(pThisArmyAI)
	{
		bRemoved = pThisArmyAI->RemoveUnit(iID);
	}

	return bRemoved;
}


//	---------------------------------------------------------------------------
bool CvPlayer::removeFromArmy(int iID)
{
	CvArmyAI* pLoopArmyAI;
	int iLoop;
	bool bRemoved = false;

	// for all the army AIs
	for(pLoopArmyAI = firstArmyAI(&iLoop); pLoopArmyAI != NULL && !bRemoved; pLoopArmyAI = nextArmyAI(&iLoop))
	{
		// attempt to remove from this army
		bRemoved = removeFromArmy(pLoopArmyAI->GetID(), iID);
	}
	return bRemoved;
}


//	---------------------------------------------------------------------------
//	Finds the path length from this tech type to one you already know
int CvPlayer::findPathLength(TechTypes eTech, bool bCost) const
{
	int i;
	int iNumSteps = 0;
	int iShortestPath = 0;
	int iPathLength = 0;
	TechTypes ePreReq;
	TechTypes eShortestOr;

	CvTechEntry* pkTechInfo = GC.getTechInfo(eTech);
	if(pkTechInfo == NULL)
		return 0;

	if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech(eTech) || m_pPlayerTechs->IsResearchingTech(eTech))
	{
		//	We have this tech, no reason to add this to the pre-reqs
		//	Base case return 0, we know it...
		return 0;
	}

	//	Cycle through the and paths and add up their tech lengths
	for(i = 0; i < GC.getNUM_AND_TECH_PREREQS(); i++)
	{
		ePreReq = (TechTypes)pkTechInfo->GetPrereqAndTechs(i);

		if(ePreReq != NO_TECH)
		{
			iPathLength += findPathLength(ePreReq, bCost);
		}
	}

	eShortestOr = NO_TECH;
	iShortestPath = INT_MAX;
	//	Find the shortest OR tech
	for(i = 0; i < GC.getNUM_OR_TECH_PREREQS(); i++)
	{
		//	Grab the tech
		ePreReq = (TechTypes)pkTechInfo->GetPrereqOrTechs(i);

		//	If this is a valid tech
		if(ePreReq != NO_TECH)
		{
			//	Recursively find the path length (takes into account all ANDs)
			iNumSteps = findPathLength(ePreReq, bCost);

			//	If the prereq is a valid tech and its the current shortest, mark it as such
			if(iNumSteps < iShortestPath)
			{
				eShortestOr = ePreReq;
				iShortestPath = iNumSteps;
			}
		}
	}

	//	If the shortest OR is a valid tech, add the steps to it...
	if(eShortestOr != NO_TECH)
	{
		iPathLength += iShortestPath;
	}

	return (iPathLength + ((bCost) ? GET_TEAM(getTeam()).GetTeamTechs()->GetResearchCost(eTech) : 1));
}


//	--------------------------------------------------------------------------------
//	Function specifically for python/tech chooser screen
int CvPlayer::getQueuePosition(TechTypes eTech) const
{
	int i = 1;
	const CLLNode<TechTypes>* pResearchNode;

	for(pResearchNode = headResearchQueueNode(); pResearchNode; pResearchNode = nextResearchQueueNode(pResearchNode))
	{
		if(pResearchNode->m_data == eTech)
		{
			return i;
		}
		i++;
	}

	return -1;
}


//	--------------------------------------------------------------------------------
void CvPlayer::clearResearchQueue()
{
	int iI;

	m_researchQueue.clear();

	for(iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		setResearchingTech(((TechTypes)iI), false);
	}

	if(getTeam() == GC.getGame().getActiveTeam())
	{
		GC.GetEngineUserInterface()->setDirty(ResearchButtons_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(Score_DIRTY_BIT, true);
	}
}


//	--------------------------------------------------------------------------------
//	Pushes research onto the queue.  If it is an append if will put it
//	and its pre-reqs into the queue.  If it is not an append it will change
//	research immediately and should be used with clear.  Clear will clear the entire queue.
bool CvPlayer::pushResearch(TechTypes eTech, bool bClear)
{
	int i;
	int iNumSteps;
	int iShortestPath;
	bool bOrPrereqFound;
	TechTypes ePreReq;
	TechTypes eShortestOr;

	CvAssertMsg(eTech != NO_TECH, "Tech is not assigned a valid value");

	CvTechEntry* pkTechInfo = GC.getTechInfo(eTech);
	if(pkTechInfo == NULL)
		return false;


	if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech(eTech) || m_pPlayerTechs->IsResearchingTech(eTech))
	{
		//	We have this tech, no reason to add this to the pre-reqs
		return true;
	}

	if(!GetPlayerTechs()->CanEverResearch(eTech))
	{
		return false;
	}

	//	Pop the entire queue...
	if(bClear)
	{
		clearResearchQueue();
	}

	//	Add in all the pre-reqs for the and techs...
	for(i = 0; i < GC.getNUM_AND_TECH_PREREQS(); i++)
	{
		ePreReq = (TechTypes)pkTechInfo->GetPrereqAndTechs(i);

		if(ePreReq != NO_TECH)
		{
			if(!pushResearch(ePreReq))
			{
				return false;
			}
		}
	}

	// Will return the shortest path of all the or techs.  Tie breaker goes to the first one...
	eShortestOr = NO_TECH;
	iShortestPath = INT_MAX;
	bOrPrereqFound = false;
	//	Cycle through all the OR techs
	for(i = 0; i < GC.getNUM_OR_TECH_PREREQS(); i++)
	{
		ePreReq = (TechTypes)pkTechInfo->GetPrereqOrTechs(i);

		if(ePreReq != NO_TECH)
		{
			bOrPrereqFound = true;

			//	If the pre-req exists, and we have it, it is the shortest path, get out, we're done
			if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech(ePreReq))
			{
				eShortestOr = ePreReq;
				break;
			}

			if(GetPlayerTechs()->CanEverResearch(ePreReq))
			{
				//	Find the length of the path to this pre-req
				iNumSteps = findPathLength(ePreReq);

				//	If this pre-req is a valid tech, and its the shortest current path, set it as such
				if(iNumSteps < iShortestPath)
				{
					eShortestOr = ePreReq;
					iShortestPath = iNumSteps;
				}
			}
		}
	}

	//	If the shortest path tech is valid, push it (and its children) on to the research queue recursively
	if(eShortestOr != NO_TECH)
	{
		if(!pushResearch(eShortestOr))
		{
			return false;
		}
	}
	else if(bOrPrereqFound)
	{
		return false;
	}

	//	Insert this tech at the end of the queue
	m_researchQueue.insertAtEnd(eTech);

	setResearchingTech(eTech, true);

	//	Set the dirty bits
	if(getTeam() == GC.getGame().getActiveTeam())
	{
		GC.GetEngineUserInterface()->setDirty(ResearchButtons_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(Score_DIRTY_BIT, true);
	}
	return true;
}


//	--------------------------------------------------------------------------------
//	If bHead is true we delete the entire queue...
void CvPlayer::popResearch(TechTypes eTech)
{
	CLLNode<TechTypes>* pResearchNode;

	for(pResearchNode = headResearchQueueNode(); pResearchNode; pResearchNode = nextResearchQueueNode(pResearchNode))
	{
		if(pResearchNode->m_data == eTech)
		{
			m_researchQueue.deleteNode(pResearchNode);
			break;
		}
	}

	setResearchingTech(eTech, false);

	if(getTeam() == GC.getGame().getActiveTeam())
	{
		GC.GetEngineUserInterface()->setDirty(ResearchButtons_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(Score_DIRTY_BIT, true);
	}
}


//	--------------------------------------------------------------------------------
int CvPlayer::getLengthResearchQueue() const
{
	return m_researchQueue.getLength();
}


//	--------------------------------------------------------------------------------
CLLNode<TechTypes>* CvPlayer::nextResearchQueueNode(CLLNode<TechTypes>* pNode)
{
	return m_researchQueue.next(pNode);
}

//	--------------------------------------------------------------------------------
const CLLNode<TechTypes>* CvPlayer::nextResearchQueueNode(const CLLNode<TechTypes>* pNode) const
{
	return m_researchQueue.next(pNode);
}

//	--------------------------------------------------------------------------------
CLLNode<TechTypes>* CvPlayer::headResearchQueueNode()
{
	return m_researchQueue.head();
}

//	--------------------------------------------------------------------------------
const CLLNode<TechTypes>* CvPlayer::headResearchQueueNode() const
{
	return m_researchQueue.head();
}

//	--------------------------------------------------------------------------------
CLLNode<TechTypes>* CvPlayer::tailResearchQueueNode()
{
	return m_researchQueue.tail();
}


//	--------------------------------------------------------------------------------
void CvPlayer::addCityName(const CvString& szName)
{
	m_cityNames.insertAtEnd(szName);
}


//	--------------------------------------------------------------------------------
int CvPlayer::getNumCityNames() const
{
	return m_cityNames.getLength();
}


//	--------------------------------------------------------------------------------
CvString CvPlayer::getCityName(int iIndex) const
{
	CLLNode<CvString>* pCityNameNode;

	pCityNameNode = m_cityNames.nodeNum(iIndex);

	if(pCityNameNode != NULL)
	{
		return pCityNameNode->m_data;
	}
	else
	{
		return "";
	}
}

//	--------------------------------------------------------------------------------
CLLNode<CvString>* CvPlayer::nextCityNameNode(CLLNode<CvString>* pNode)
{
	return m_cityNames.next(pNode);
}

//	--------------------------------------------------------------------------------
const CLLNode<CvString>* CvPlayer::nextCityNameNode(const CLLNode<CvString>* pNode) const
{
	return m_cityNames.next(pNode);
}

//	--------------------------------------------------------------------------------
CLLNode<CvString>* CvPlayer::headCityNameNode()
{
	return m_cityNames.head();
}

//	--------------------------------------------------------------------------------
const CLLNode<CvString>* CvPlayer::headCityNameNode() const
{
	return m_cityNames.head();
}

//	--------------------------------------------------------------------------------
CvCity* CvPlayer::firstCity(int* pIterIdx, bool bRev)
{
	return !bRev ? m_cities.BeginIter(pIterIdx) : m_cities.EndIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
const CvCity* CvPlayer::firstCity(int* pIterIdx, bool bRev) const
{
	return !bRev ? m_cities.BeginIter(pIterIdx) : m_cities.EndIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
CvCity* CvPlayer::nextCity(int* pIterIdx, bool bRev)
{
	return !bRev ? m_cities.NextIter(pIterIdx) : m_cities.PrevIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
const CvCity* CvPlayer::nextCity(int* pIterIdx, bool bRev) const
{
	return !bRev ? m_cities.NextIter(pIterIdx) : m_cities.PrevIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNumCities() const
{
	return m_cities.GetCount();
}


//	--------------------------------------------------------------------------------
CvCity* CvPlayer::getCity(int iID)
{
	return(m_cities.GetAt(iID));
}

//	--------------------------------------------------------------------------------
const CvCity* CvPlayer::getCity(int iID) const
{
	return(m_cities.GetAt(iID));
}


//	--------------------------------------------------------------------------------
CvCity* CvPlayer::addCity()
{
	return(m_cities.Add());
}

//	--------------------------------------------------------------------------------
void CvPlayer::deleteCity(int iID)
{
#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
	if (getNumCities() > 0)
	{
		for (int iYield = 0; iYield < GC.getNUM_YIELD_TYPES(); iYield++)
		{
			YieldTypes eYield = (YieldTypes)iYield;
			for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
			{
				BuildingTypes eBuilding = eBuilding = (BuildingTypes)iI;
				CvBuildingEntry* pBuildingInfo = GC.getBuildingInfo(eBuilding);
				if (pBuildingInfo && pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield) > 0)
				{
					int iLoop = 0;
					int iNumBuildings = 0;
					int iNumSuppYields;
					int iNumYileds;
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						iNumBuildings += pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding);
					}
					if (pBuildingInfo->GetMaxYieldForEachBuildingInEmpire(eYield) >= 0)
					{
						iNumSuppYields = iNumBuildings * pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield);
						iNumYileds = std::min(pBuildingInfo->GetMaxYieldForEachBuildingInEmpire(eYield), iNumSuppYields);
					}
					else
					{
						iNumSuppYields = iNumBuildings * pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield);
						iNumYileds = iNumSuppYields;
					}
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						if (pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
						{
							pLoopCity->ChangeBaseYieldRateFromBuildings(eYield, -iNumYileds);
						}
					}
				}
			}
		}
	}
#endif
	m_cities.RemoveAt(iID);
#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
	if (getNumCities() > 0)
	{
		for (int iYield = 0; iYield < GC.getNUM_YIELD_TYPES(); iYield++)
		{
			YieldTypes eYield = (YieldTypes)iYield;
			for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
			{
				BuildingTypes eBuilding = eBuilding = (BuildingTypes)iI;
				CvBuildingEntry* pBuildingInfo = GC.getBuildingInfo(eBuilding);
				if (pBuildingInfo && pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield) > 0)
				{
					int iLoop = 0;
					int iNumBuildings = 0;
					int iNumSuppYields;
					int iNumYileds;
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						iNumBuildings += pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding);
					}
					if (pBuildingInfo->GetMaxYieldForEachBuildingInEmpire(eYield) >= 0)
					{
						iNumSuppYields = iNumBuildings * pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield);
						iNumYileds = std::min(pBuildingInfo->GetMaxYieldForEachBuildingInEmpire(eYield), iNumSuppYields);
					}
					else
					{
						iNumSuppYields = iNumBuildings * pBuildingInfo->GetYieldForEachBuildingInEmpire(eYield);
						iNumYileds = iNumSuppYields;
					}
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						if (pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
						{
							pLoopCity->ChangeBaseYieldRateFromBuildings(eYield, iNumYileds);
						}
					}
				}
			}
		}
	}
#endif
}

//	--------------------------------------------------------------------------------
CvCity* CvPlayer::GetFirstCityWithBuildingClass(BuildingClassTypes eBuildingClass)
{
	CvCity *pLoopCity;
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		CvCivilizationInfo& playerCivilizationInfo = getCivilizationInfo();
		BuildingTypes eBuilding = (BuildingTypes)playerCivilizationInfo.getCivilizationBuildings((BuildingClassTypes)eBuildingClass);
		if (eBuilding != NO_BUILDING)
		{
			if (pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
			{
				return pLoopCity;
			}
		}
	}
	return false;
}

//	--------------------------------------------------------------------------------
const CvUnit* CvPlayer::firstUnit(int* pIterIdx, bool bRev) const
{
	return !bRev ? m_units.BeginIter(pIterIdx) : m_units.EndIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
const CvUnit* CvPlayer::nextUnit(int* pIterIdx, bool bRev) const
{
	return !bRev ? m_units.NextIter(pIterIdx) : m_units.PrevIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
CvUnit* CvPlayer::firstUnit(int* pIterIdx, bool bRev)
{
	return !bRev ? m_units.BeginIter(pIterIdx) : m_units.EndIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
CvUnit* CvPlayer::nextUnit(int* pIterIdx, bool bRev)
{
	return !bRev ? m_units.NextIter(pIterIdx) : m_units.PrevIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNumUnits() const
{
	return m_units.GetCount();
}


//	--------------------------------------------------------------------------------
const CvUnit* CvPlayer::getUnit(int iID) const
{
	return (m_units.GetAt(iID));
}

//	--------------------------------------------------------------------------------
CvUnit* CvPlayer::getUnit(int iID)
{
	return (m_units.GetAt(iID));
}

//	--------------------------------------------------------------------------------
CvUnit* CvPlayer::addUnit()
{
	return (m_units.Add());
}


//	--------------------------------------------------------------------------------
void CvPlayer::deleteUnit(int iID)
{
	m_units.RemoveAt(iID);
}


//	--------------------------------------------------------------------------------
const CvArmyAI* CvPlayer::firstArmyAI(int* pIterIdx, bool bRev) const
{
	return !bRev ? m_armyAIs.BeginIter(pIterIdx) : m_armyAIs.EndIter(pIterIdx);
}


//	--------------------------------------------------------------------------------
const CvArmyAI* CvPlayer::nextArmyAI(int* pIterIdx, bool bRev) const
{
	return !bRev ? m_armyAIs.NextIter(pIterIdx) : m_armyAIs.PrevIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
CvArmyAI* CvPlayer::firstArmyAI(int* pIterIdx, bool bRev)
{
	return !bRev ? m_armyAIs.BeginIter(pIterIdx) : m_armyAIs.EndIter(pIterIdx);
}


//	--------------------------------------------------------------------------------
CvArmyAI* CvPlayer::nextArmyAI(int* pIterIdx, bool bRev)
{
	return !bRev ? m_armyAIs.NextIter(pIterIdx) : m_armyAIs.PrevIter(pIterIdx);
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNumArmyAIs() const
{
	return m_armyAIs.GetCount();
}


//	--------------------------------------------------------------------------------
const CvArmyAI* CvPlayer::getArmyAI(int iID) const
{
	return ((CvArmyAI*)(m_armyAIs.GetAt(iID)));
}

//	--------------------------------------------------------------------------------
CvArmyAI* CvPlayer::getArmyAI(int iID)
{
	return ((CvArmyAI*)(m_armyAIs.GetAt(iID)));
}


//	--------------------------------------------------------------------------------
CvArmyAI* CvPlayer::addArmyAI()
{
	return ((CvArmyAI*)(m_armyAIs.Add()));
}


//	--------------------------------------------------------------------------------
void CvPlayer::deleteArmyAI(int iID)
{
	bool bRemoved = m_armyAIs.RemoveAt(iID);
	DEBUG_VARIABLE(bRemoved);
	CvAssertMsg(bRemoved, "could not find army, delete failed");
}


//	--------------------------------------------------------------------------------
const CvAIOperation* CvPlayer::getAIOperation(int iID) const
{
	std::map<int, CvAIOperation*>::const_iterator it = m_AIOperations.find(iID);
	if(it != m_AIOperations.end())
	{
		return it->second;
	}
	return 0;
}

//	--------------------------------------------------------------------------------
CvAIOperation* CvPlayer::getFirstAIOperation()
{
	CvAIOperation* rtnValue = NULL;

	m_CurrentOperation = m_AIOperations.begin();
	if(m_CurrentOperation != m_AIOperations.end())
	{
		rtnValue = m_CurrentOperation->second;
	}
	return rtnValue;
}

//	--------------------------------------------------------------------------------
CvAIOperation* CvPlayer::getNextAIOperation()
{
	CvAIOperation* rtnValue = NULL;

	if(m_CurrentOperation != m_AIOperations.end())
	{
		++m_CurrentOperation;
		if(m_CurrentOperation != m_AIOperations.end())
		{
			rtnValue = m_CurrentOperation->second;
		}
	}
	return rtnValue;
}

//	--------------------------------------------------------------------------------
CvAIOperation* CvPlayer::getAIOperation(int iID)
{
	std::map<int, CvAIOperation*>::iterator it = m_AIOperations.find(iID);
	if(it != m_AIOperations.end())
	{
		return it->second;
	}
	return 0;
}


//	--------------------------------------------------------------------------------
CvAIOperation* CvPlayer::addAIOperation(int OperationType, PlayerTypes eEnemy, int iArea, CvCity* pTarget, CvCity* pMuster)
{
	CvAIOperation* pNewOperation = CvAIOperation::CreateOperation((AIOperationTypes) OperationType, m_eID);
	if(pNewOperation)
	{
		m_AIOperations.insert(std::make_pair(m_iNextOperationID.get(), pNewOperation));
		pNewOperation->Init(m_iNextOperationID, m_eID, eEnemy, iArea, pTarget, pMuster);
		m_iNextOperationID++;
	}
	return pNewOperation;
}


//	--------------------------------------------------------------------------------
void CvPlayer::deleteAIOperation(int iID)
{
	std::map<int, CvAIOperation*>::iterator it = m_AIOperations.find(iID);
	if(it != m_AIOperations.end())
	{
		delete(it->second);
		m_AIOperations.erase(it);
	}
	else
	{
		CvAssertMsg(false, "could not find operation, delete failed");
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::haveAIOperationOfType(int iOperationType, int* piID /* optional return argument */, PlayerTypes eTargetPlayer /* optional additional match criteria */, CvPlot* pTarget /* optional additional match criteria */)
{
	// loop through all entries looking for match
	std::map<int , CvAIOperation*>::iterator iter;
	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		CvAIOperation* pThisOperation = iter->second;
		if(pThisOperation->GetOperationType() == iOperationType)
		{
			if(eTargetPlayer == NO_PLAYER || eTargetPlayer == pThisOperation->GetEnemy())
			{
				if(pTarget == NULL || pTarget == pThisOperation->GetTargetPlot())
				{
					// Fill in optional parameter (ID) if passed in
					if(piID != NULL)
					{
						*piID = pThisOperation->GetID();
					}
					return true;
				}
			}
		}
	}
	// Fill in optional parameter (ID) if passed in
	if(piID != NULL)
	{
		*piID = -1;
	}
	return false;
}

//	--------------------------------------------------------------------------------
int CvPlayer::numOperationsOfType(int iOperationType)
{
	int iRtnValue = 0;

	std::map<int , CvAIOperation*>::iterator iter;
	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		CvAIOperation* pThisOperation = iter->second;
		if(pThisOperation->GetOperationType() == iOperationType)
		{
			iRtnValue++;
		}
	}

	return iRtnValue;
}

//	--------------------------------------------------------------------------------
/// Is an existing operation already going after this city?
bool CvPlayer::IsCityAlreadyTargeted(CvCity* pCity, DomainTypes eDomain, int iPercentToTarget, int iIgnoreOperationID) const
{
	CvAIOperation* pOperation;
	std::map<int , CvAIOperation*>::const_iterator iter;

	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		pOperation = iter->second;

		if(pOperation)
		{
			if(iIgnoreOperationID == -1 || iIgnoreOperationID != pOperation->GetID())
			{
				if(pOperation->GetTargetPlot() == pCity->plot() && pOperation->PercentFromMusterPointToTarget() < iPercentToTarget)
				{
					// Naval attacks are mixed land/naval operations
					if((eDomain == NO_DOMAIN || eDomain == DOMAIN_SEA) && pOperation->IsMixedLandNavalOperation())
					{
						return true;
					}

					if((eDomain == NO_DOMAIN || eDomain == DOMAIN_LAND) && !pOperation->IsMixedLandNavalOperation())
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
/// Are we already sending a settler to this plot (or any plot within 2)
bool CvPlayer::IsPlotTargetedForCity(CvPlot *pPlot) const
{
	CvAIOperation* pOperation;
	std::map<int , CvAIOperation*>::const_iterator iter;

	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		pOperation = iter->second;
		if(pOperation)
		{
			switch (pOperation->GetOperationType())
			{
			case AI_OPERATION_FOUND_CITY:
			case AI_OPERATION_COLONIZE:
			case AI_OPERATION_QUICK_COLONIZE:
				{
					if (plotDistance(pPlot->getX(), pPlot->getY(), pOperation->GetTargetPlot()->getX(), pOperation->GetTargetPlot()->getY()) <= 2)
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

//	--------------------------------------------------------------------------------
unsigned int CvPlayer::getNumReplayDataSets() const
{
	return m_ReplayDataSets.size();
}

//	--------------------------------------------------------------------------------
const char* CvPlayer::getReplayDataSetName(unsigned int idx) const
{
	if(idx < m_ReplayDataSets.size())
		return m_ReplayDataSets[idx];

	return NULL;
}

#ifdef DEV_RECORDING_STATISTICS
//	--------------------------------------------------------------------------------
const char* CvPlayer::getReplayDataSetDesc(unsigned int idx) const
{
	if (idx < m_ReplayDataSets.size())
	{
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_SCORE")
			return Localization::Lookup("TXT_KEY_REPLAY_VIEWER_GRAPHBY_SCORE").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_PRODUCTIONPERTURN")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_PRODUCTIONPERTURN").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALGOLD")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALGOLD").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_GOLDPERTURN")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_GOLDPERTURN").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_CITYCOUNT")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_CITYCOUNT").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TECHSKNOWN")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TECHSKNOWN").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_SCIENCEPERTURN")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_SCIENCEPERTURN").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALCULTURE")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALCULTURE").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_CULTUREPERTURN")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_CULTUREPERTURN").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_EXCESSHAPINESS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_EXCESSHAPPINESS").toUTF8();

		if (m_ReplayDataSets[idx] == "REPLAYDATASET_HAPPINESS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_HAPPINESS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_UNHAPPINESS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_UNHAPPINESS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_GOLDENAGETURNS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_GOLDAGETURNS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_POPULATION")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALPOPULATION").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_FOODPERTURN")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_FOODPERTURN").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALLAND")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALLAND").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_GPTCITYCONNECTIONS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_GPTCITYCONNECTIONS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_GPTDEALS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_GPTDEALS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_UNITMAINTENANCE")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_UNITMAINTENANCE").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_BUILDINGMAINTENANCE")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_BUILDINGMAINTENANCE").toUTF8();

		if (m_ReplayDataSets[idx] == "REPLAYDATASET_IMPROVEMENTMAINTENANCE")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_IMPROVEMENTMAINTENANCE").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMBEROFPOLICIES")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMBEROFPOLICIES").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMBEROFWORKERS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMBEROFWORKERS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_IMPROVEDTILES")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_IMPROVEDTILES").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_WORKEDTILES")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_WORKEDTILES").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_MILITARYMIGHT")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_MILITARYMIGHT").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_FAITHPERTURN")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_FAITHPERTURN").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALFAITH")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALFAITH").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBORNSCIENTISTS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBORNSCIENTISTS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBOUGHTSCIENTISTS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBOUGHTSCIENTISTS").toUTF8();

		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALNUMOFSCIENTISTS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALNUMOFSCIENTISTS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBORNENGINEERS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBORNENGINEERS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBOUGHTENGINEERS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBOUGHTENGINEERS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALNUMOFENGINEERS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALNUMOFENGINEERS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBORNMERCHANTS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBORNMERCHANTS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBOUGHTMERCHANTS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBOUGHTMERCHANTS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALNUMOFMERCHANTS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALNUMOFMERCHANTS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBORNWRITERS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBORNWRITERS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBOUGHTWRITERS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBOUGHTWRITERS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALNUMOFWRITERS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALNUMOFWRITERS").toUTF8();

		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBORNARTISTS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBORNARTISTS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBOUGHTARTISTS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBOUGHTARTISTS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALNUMOFARTISTS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALNUMOFARTISTS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBORNMUSICIANS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBORNMUSICIANS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBOUGHTMUSICIANS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBOUGHTMUSICIANS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALNUMOFMUSICIANS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALNUMOFMUSICIANS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBOUGHTPROPHETS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBOUGHTPROPHETS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALNUMOFPROPHETS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALNUMOFPROPHETS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBORNGENERALS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBORNGENERALS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBOUGHTGENERALS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBOUGHTGENERALS").toUTF8();

		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALNUMOFGENERALS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALNUMOFGENERALS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBORNADMIRALS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBORNADMIRALS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMOFBOUGHTADMIRALS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMOFBOUGHTADMIRALS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALNUMOFADMIRALS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALNUMOFADMIRALS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_GOLDFROMBULLYING")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_GOLDFROMBULLYING").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_WORKERSFROMBULLYING")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_WORKERSFROMBULLYING").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMTRAINEDUNITS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMTRAINEDUNITS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMLOSTUNITS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMLOSTUNITS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMKILLEDUNITS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMKILLEDUNITS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMBUILTWONDERS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMBUILTWONDERS").toUTF8();

		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMREVEALEDTILES")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMREVEALEDTILES").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMSTOLENSCIENCE")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMSTOLENSCIENCE").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_DAMAGEDEALTTOUNITS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_DAMAGEDEALTTOUNITS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_DAMAGEDEALTTOCITIES")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_DAMAGEDEALTTOCITIES").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_DAMAGETAKENBYUNITS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_DAMAGETAKENBYUNITS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_DAMAGETAKENBYCITIES")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_DAMAGETAKENBYCITIES").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMDELEGATES")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMDELEGATES").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_NUMTIMESOPENEDDEMOGRAPHICS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_TOTALCHOPS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_TOTALCHOPS").toUTF8();
		if (m_ReplayDataSets[idx] == "REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS")
			return Localization::Lookup("TXT_KEY_REPLAY_DATA_LOSTHAMMERSFROMLOSTWONDERS").toUTF8();

		for (int iI = 0; iI < GC.getNumPolicyInfos(); iI++)
		{
			if (m_ReplayDataSets[idx] == GC.getPolicyInfo((PolicyTypes)iI)->GetType())
				return GC.getPolicyInfo((PolicyTypes)iI)->GetDescription();
		}

		for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
		{
			if (m_ReplayDataSets[idx] == GC.getTechInfo((TechTypes)iI)->GetType())
				return GC.getTechInfo((TechTypes)iI)->GetDescription();
		}

		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			if (GC.getBuildingClassInfo((BuildingClassTypes)iI))
				if (m_ReplayDataSets[idx] == GC.getBuildingClassInfo((BuildingClassTypes)iI)->GetType())
					return GC.getBuildingClassInfo((BuildingClassTypes)iI)->GetDescription();
		}

		for (int iI = 0; iI < GC.GetGameBeliefs()->GetNumBeliefs(); iI++)
		{
			const BeliefTypes eBelief(static_cast<BeliefTypes>(iI));
			if (GC.getBeliefInfo(eBelief))
				if (m_ReplayDataSets[idx] == GC.getBeliefInfo(eBelief)->GetType())
					return Localization::Lookup(GC.getBeliefInfo(eBelief)->getShortDescription()).toUTF8();
		}

	}

	return NULL;
}
#endif

//	--------------------------------------------------------------------------------
unsigned int CvPlayer::getReplayDataSetIndex(const char* szDataSetName)
{
	CvString dataSetName = szDataSetName;

	unsigned int idx = 0;
	for(std::vector<CvString>::iterator it = m_ReplayDataSets.begin(); it != m_ReplayDataSets.end(); ++it)
	{
		if((*it) == dataSetName)
			return idx;

		idx++;
	}

	m_ReplayDataSets.push_back(dataSetName);
	m_ReplayDataSetValues.push_back(TurnData());
	return m_ReplayDataSets.size() - 1;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getReplayDataValue(unsigned int uiDataSet, unsigned int uiTurn) const
{
	if(uiDataSet < m_ReplayDataSetValues.size())
	{
		const TurnData& dataSet = m_ReplayDataSetValues[uiDataSet];
		TurnData::const_iterator it = dataSet.find(uiTurn);
		if(it != dataSet.end())
		{
			return (*it).second;
		}
	}

	return -1;
}
//	--------------------------------------------------------------------------------
void CvPlayer::setReplayDataValue(unsigned int uiDataSet, unsigned int uiTurn, int iValue)
{
	if(uiDataSet < m_ReplayDataSetValues.size())
	{
		TurnData& dataSet = m_ReplayDataSetValues[uiDataSet];
		dataSet[uiTurn] = iValue;
	}
}

//	--------------------------------------------------------------------------------
CvPlayer::TurnData CvPlayer::getReplayDataHistory(unsigned int uiDataSet) const
{
	if(uiDataSet < m_ReplayDataSetValues.size())
	{
		return m_ReplayDataSetValues[uiDataSet];
	}

	return CvPlayer::TurnData();
}

//	--------------------------------------------------------------------------------
std::string CvPlayer::getScriptData() const
{
	return m_strScriptData;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setScriptData(std::string strNewValue)
{
	m_strScriptData = strNewValue;
}

//	--------------------------------------------------------------------------------
const CvString& CvPlayer::getPbemEmailAddress() const
{
	return CvPreGame::emailAddress(GetID());
}

//	--------------------------------------------------------------------------------
void CvPlayer::setPbemEmailAddress(const char* szAddress)
{
	CvPreGame::setEmailAddress(GetID(), szAddress);
}

// Protected Functions...

//	--------------------------------------------------------------------------------
void CvPlayer::doResearch()
{
	if(GC.getGame().isOption(GAMEOPTION_NO_SCIENCE))
	{
		return;
	}

	AI_PERF_FORMAT("AI-perf.csv", ("CvPlayer::doResearch, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()) );
	bool bForceResearchChoice;
	int iOverflowResearch;

	if(GetPlayerTechs()->IsResearch())
	{
		bForceResearchChoice = false;

		// Force player to pick Research if he doesn't have anything assigned
		if(GetPlayerTechs()->GetCurrentResearch() == NO_TECH)
		{
			if(GetID() == GC.getGame().getActivePlayer() && GetScienceTimes100() > 0)
			{
				chooseTech();
			}

			if(GC.getGame().getElapsedGameTurns() > 4)
			{
				AI_chooseResearch();

				bForceResearchChoice = true;
			}
		}

		TechTypes eCurrentTech = GetPlayerTechs()->GetCurrentResearch();
		if(eCurrentTech == NO_TECH)
		{
			int iOverflow = (GetScienceTimes100()) / std::max(1, calculateResearchModifier(eCurrentTech));
			changeOverflowResearchTimes100(iOverflow);
		}
		else
		{
			iOverflowResearch = (getOverflowResearchTimes100() * calculateResearchModifier(eCurrentTech)) / 100;
			setOverflowResearch(0);
			if(GET_TEAM(getTeam()).GetTeamTechs())
			{
				int iBeakersTowardsTechTimes100 = GetScienceTimes100() + iOverflowResearch;
				GET_TEAM(getTeam()).GetTeamTechs()->ChangeResearchProgressTimes100(eCurrentTech, iBeakersTowardsTechTimes100, GetID());
				UpdateResearchAgreements(GetScienceTimes100() / 100);
			}
		}

		if(bForceResearchChoice)
		{
			clearResearchQueue();
		}
	}
	GetPlayerTechs()->CheckForTechAchievement();

}

//	--------------------------------------------------------------------------------
void CvPlayer::doAdvancedStartAction(AdvancedStartActionTypes eAction, int iX, int iY, int iData, bool bAdd)
{
	CvPlot* pPlot = GC.getMap().plot(iX, iY);

	if(0 == getNumCities())
	{
		switch(eAction)
		{
		case ADVANCEDSTARTACTION_EXIT:
			//Try to build this player's empire
			if(GetID() == GC.getGame().getActivePlayer())
			{
				GC.GetEngineUserInterface()->setBusy(true);
			}
			if(GetID() == GC.getGame().getActivePlayer())
			{
				GC.GetEngineUserInterface()->setBusy(false);
			}
			break;
		case ADVANCEDSTARTACTION_AUTOMATE:
		case ADVANCEDSTARTACTION_CITY:
			break;
		default:
			// The first action must be to place a city
			// so players can lose by spending everything
			return;
		}
	}

	switch(eAction)
	{
	case ADVANCEDSTARTACTION_EXIT:
		GetTreasury()->ChangeGold(getAdvancedStartPoints());
		setAdvancedStartPoints(-1);
		if(GC.getGame().getActivePlayer() == GetID())
		{
			GC.GetEngineUserInterface()->setInAdvancedStart(false);
		}

		if(isHuman())
		{
			int iLoop;
			for(CvCity* pCity = firstCity(&iLoop); NULL != pCity; pCity = nextCity(&iLoop))
			{
				pCity->chooseProduction();
			}

			chooseTech();
		}
		break;
	case ADVANCEDSTARTACTION_AUTOMATE:
		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setBusy(true);
		}
		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setBusy(false);
		}
		break;
	case ADVANCEDSTARTACTION_UNIT:
	{
		if(pPlot == NULL)
			return;

		UnitTypes eUnit = (UnitTypes) iData;
		int iCost = getAdvancedStartUnitCost(eUnit, bAdd, pPlot);

		if(bAdd && iCost < 0)
		{
			return;
		}

		// Add unit to the map
		if(bAdd)
		{
			if(getAdvancedStartPoints() >= iCost)
			{
				CvUnit* pUnit = initUnit(eUnit, iX, iY);
				if(NULL != pUnit)
				{
					pUnit->finishMoves();
					changeAdvancedStartPoints(-iCost);
				}
			}
		}

		// Remove unit from the map
		else
		{
			// If cost is -1 we already know this unit isn't present
			if(iCost != -1)
			{
				IDInfo* pUnitNode = pPlot->headUnitNode();
				while(pUnitNode != NULL)
				{
					CvUnit* pLoopUnit = ::getUnit(*pUnitNode);
					pUnitNode = pPlot->nextUnitNode(pUnitNode);

					if(NULL != pLoopUnit && pLoopUnit->getUnitType() == eUnit)
					{
						pLoopUnit->kill(false);
						changeAdvancedStartPoints(iCost);
						return;
					}
				}
			}

			// Proper unit not found above, delete first found
			IDInfo* pUnitNode = pPlot->headUnitNode();
			if(pUnitNode != NULL)
			{
				CvUnit* pUnit = ::getUnit(*pUnitNode);

				iCost = getAdvancedStartUnitCost(pUnit->getUnitType(), false);
				CvAssertMsg(iCost != -1, "If this is -1 then that means it's going to try to delete a unit which shouldn't exist");
				pUnit->kill(false);
				changeAdvancedStartPoints(iCost);
			}
		}

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(Advanced_Start_DIRTY_BIT, true);
		}
	}
	break;
	case ADVANCEDSTARTACTION_CITY:
	{
		if(pPlot == NULL)
			return;

		int iCost = getAdvancedStartCityCost(bAdd, pPlot);

		if(iCost < 0)
		{
			return;
		}

		// Add City to the map
		if(bAdd)
		{
			if(0 == getNumCities())
			{
				PlayerTypes eClosestPlayer = NO_PLAYER;
				int iMinDistance = INT_MAX;
				for(int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
				{
					CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
					if(kPlayer.isAlive())
					{
						if(kPlayer.getTeam() == getTeam())
						{
							if(0 == kPlayer.getNumCities())
							{
								CvAssert(kPlayer.getStartingPlot() != NULL);
								int iDistance = plotDistance(iX, iY, kPlayer.getStartingPlot()->getX(), kPlayer.getStartingPlot()->getY());
								if(iDistance < iMinDistance)
								{
									eClosestPlayer = kPlayer.GetID();
									iMinDistance = iDistance;
								}
							}
						}
					}
				}
				CvAssertMsg(eClosestPlayer != NO_PLAYER, "Self at a minimum should always be valid");
				if(eClosestPlayer != GetID())
				{
					CvPlot* pTempPlot = GET_PLAYER(eClosestPlayer).getStartingPlot();
					GET_PLAYER(eClosestPlayer).setStartingPlot(getStartingPlot());
					setStartingPlot(pTempPlot);
				}
			}
			if(getAdvancedStartPoints() >= iCost || 0 == getNumCities())
			{
				found(iX, iY);
				changeAdvancedStartPoints(-iCost);
				CvCity* pCity = pPlot->getPlotCity();
				if(pCity != NULL)
				{
					if(pCity->getPopulation() > 1)
					{
						pCity->setFood(pCity->growthThreshold() / 2);
					}
				}
			}
		}

		// Remove City from the map
		else
		{
			pPlot->setRouteType(NO_ROUTE);
			pPlot->getPlotCity()->kill();
			pPlot->setImprovementType(NO_IMPROVEMENT);
			changeAdvancedStartPoints(iCost);
		}

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(Advanced_Start_DIRTY_BIT, true);
		}
	}
	break;
	case ADVANCEDSTARTACTION_POP:
	{
		if(pPlot == NULL)
			return;

		CvCity* pCity = pPlot->getPlotCity();

		if(pCity != NULL)
		{
			int iCost = getAdvancedStartPopCost(bAdd, pCity);

			if(iCost < 0)
			{
				return;
			}

			// Add Pop to the City
			if(bAdd)
			{
				if(getAdvancedStartPoints() >= iCost)
				{
					pCity->changePopulation(1);
					changeAdvancedStartPoints(-iCost);
					if(pCity->getPopulation() > 1)
					{
						pCity->setFood(pCity->growthThreshold() / 2);
						pCity->setFoodKept((pCity->getFood() * pCity->getMaxFoodKeptPercent()) / 100);
					}
				}
			}

			// Remove Pop from the city
			else
			{
				pCity->changePopulation(-1);
				changeAdvancedStartPoints(iCost);
				if(pCity->getPopulation() == 1)
				{
					pCity->setFood(0);
					pCity->setFoodKept(0);
				}
			}
		}
	}
	break;
	case ADVANCEDSTARTACTION_BUILDING:
	{
		if(pPlot == NULL)
			return;

		CvCity* pCity = pPlot->getPlotCity();

		if(pCity != NULL)
		{
			BuildingTypes eBuilding = (BuildingTypes) iData;

			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(pkBuildingInfo == NULL)
			{
				return;
			}

			int iCost = getAdvancedStartBuildingCost(eBuilding, bAdd, pCity);
			if(iCost < 0)
			{
				return;
			}

			// Add Building to the City
			if(bAdd)
			{
				if(getAdvancedStartPoints() >= iCost)
				{
					pCity->GetCityBuildings()->SetNumRealBuilding(eBuilding, pCity->GetCityBuildings()->GetNumRealBuilding(eBuilding)+1);
					changeAdvancedStartPoints(-iCost);
					if(pkBuildingInfo->GetFoodKept() != 0)
					{
						pCity->setFoodKept((pCity->getFood() * pCity->getMaxFoodKeptPercent()) / 100);
					}
				}
			}

			// Remove Building from the map
			else
			{
				pCity->GetCityBuildings()->SetNumRealBuilding(eBuilding, pCity->GetCityBuildings()->GetNumRealBuilding(eBuilding)-1);
				changeAdvancedStartPoints(iCost);
				if(pkBuildingInfo->GetFoodKept() != 0)
				{
					pCity->setFoodKept((pCity->getFood() * pCity->getMaxFoodKeptPercent()) / 100);
				}
			}
		}

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(Advanced_Start_DIRTY_BIT, true);
		}
	}
	break;
	case ADVANCEDSTARTACTION_ROUTE:
	{
		if(pPlot == NULL)
			return;

		RouteTypes eRoute = (RouteTypes) iData;
		int iCost = getAdvancedStartRouteCost(eRoute, bAdd, pPlot);

		if(bAdd && iCost < 0)
		{
			return;
		}

		// Add Route to the plot
		if(bAdd)
		{
			if(getAdvancedStartPoints() >= iCost)
			{
				pPlot->setRouteType(eRoute);
				changeAdvancedStartPoints(-iCost);
			}
		}

		// Remove Route from the Plot
		else
		{
			if(pPlot->getRouteType() != eRoute)
			{
				eRoute = pPlot->getRouteType();
				iCost = getAdvancedStartRouteCost(eRoute, bAdd);
			}

			if(iCost < 0)
			{
				return;
			}

			pPlot->setRouteType(NO_ROUTE);
			changeAdvancedStartPoints(iCost);
		}

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(Advanced_Start_DIRTY_BIT, true);
		}
	}
	break;
	case ADVANCEDSTARTACTION_IMPROVEMENT:
	{
		if(pPlot == NULL)
			return;

		ImprovementTypes eImprovement = (ImprovementTypes) iData;
		int iCost = getAdvancedStartImprovementCost(eImprovement, bAdd, pPlot);

		if(bAdd && iCost < 0)
		{
			return;
		}

		// Add Improvement to the plot
		if(bAdd)
		{
			if(getAdvancedStartPoints() >= iCost)
			{
				if(pPlot->getFeatureType() != NO_FEATURE)
				{
					for(int iI = 0; iI < GC.getNumBuildInfos(); ++iI)
					{
						CvBuildInfo* pkBuildInfo = GC.getBuildInfo((BuildTypes) iI);
						if(!pkBuildInfo)
						{
							continue;
						}

						ImprovementTypes eLoopImprovement = ((ImprovementTypes)(pkBuildInfo->getImprovement()));

						if(eImprovement == eLoopImprovement)
						{
							if(pkBuildInfo->isFeatureRemove(pPlot->getFeatureType()) && canBuild(pPlot, (BuildTypes)iI))
							{
								pPlot->setFeatureType(NO_FEATURE);
								break;
							}
						}
					}
				}

				pPlot->setImprovementType(eImprovement, GetID());

				changeAdvancedStartPoints(-iCost);
			}
		}

		// Remove Improvement from the Plot
		else
		{
			if(pPlot->getImprovementType() != eImprovement)
			{
				eImprovement = pPlot->getImprovementType();
				iCost = getAdvancedStartImprovementCost(eImprovement, bAdd, pPlot);
			}

			if(iCost < 0)
			{
				return;
			}

			pPlot->setImprovementType(NO_IMPROVEMENT);
			changeAdvancedStartPoints(iCost);
		}

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(Advanced_Start_DIRTY_BIT, true);
		}
	}
	break;
	case ADVANCEDSTARTACTION_TECH:
	{
		TechTypes eTech = (TechTypes) iData;
		int iCost = getAdvancedStartTechCost(eTech, bAdd);

		if(iCost < 0)
		{
			return;
		}

		// Add Tech to team
		if(bAdd)
		{
			if(getAdvancedStartPoints() >= iCost)
			{
				GET_TEAM(getTeam()).setHasTech(eTech, true, GetID(), false, false);
				changeAdvancedStartPoints(-iCost);
			}
		}

		// Remove Tech from the Team
		else
		{
			GET_TEAM(getTeam()).setHasTech(eTech, false, GetID(), false, false);
			changeAdvancedStartPoints(iCost);
		}

		if(GetID() == GC.getGame().getActivePlayer())
		{
			GC.GetEngineUserInterface()->setDirty(Advanced_Start_DIRTY_BIT, true);
		}
	}
	break;
	case ADVANCEDSTARTACTION_VISIBILITY:
	{
		if(pPlot == NULL)
			return;

		int iCost = getAdvancedStartVisibilityCost(bAdd, pPlot);

		if(iCost < 0)
		{
			return;
		}

		// Add Visibility to the plot
		if(bAdd)
		{
			if(getAdvancedStartPoints() >= iCost)
			{
				pPlot->setRevealed(getTeam(), true, true);
				changeAdvancedStartPoints(-iCost);
			}
		}

		// Remove Visibility from the Plot
		else
		{
			pPlot->setRevealed(getTeam(), false, true);
			changeAdvancedStartPoints(iCost);
		}
	}
	break;
	default:
		CvAssert(false);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing a unit
/////////////////////////////////////////////////////////////////////////////////////////////

int CvPlayer::getAdvancedStartUnitCost(UnitTypes eUnit, bool bAdd, CvPlot* pPlot)
{
	if(0 == getNumCities())
	{
		return -1;
	}

	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo == NULL)
	{
		return -1;
	}

	int iCost = (getProductionNeeded(eUnit) * pkUnitInfo->GetAdvancedStartCost()) / 100;
	if(iCost < 0)
	{
		return -1;
	}

	if(NULL == pPlot)
	{
		if(bAdd)
		{
			bool bValid = false;
			int iLoop;
			for(CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				if(pLoopCity->canTrain(eUnit))
				{
					bValid = true;
					break;
				}
			}

			if(!bValid)
			{
				return -1;
			}
		}
	}
	else
	{
		CvCity* pCity = NULL;

		if(0 == GC.getADVANCED_START_ALLOW_UNITS_OUTSIDE_CITIES())
		{
			pCity = pPlot->getPlotCity();

			if(NULL == pCity || pCity->getOwner() != GetID())
			{
				return -1;
			}

			iCost *= 100;
			iCost /= std::max(1, 100 + pCity->getProductionModifier(eUnit));
		}
		else
		{
			if(pPlot->getOwner() != GetID())
			{
				return -1;
			}

			iCost *= 100;
			iCost /= std::max(1, 100 + getProductionModifier(eUnit));
		}


		if(bAdd)
		{
			int iMaxUnitsPerCity = GC.getADVANCED_START_MAX_UNITS_PER_CITY();
			if(iMaxUnitsPerCity >= 0)
			{
				if(pkUnitInfo->IsMilitarySupport() && getNumMilitaryUnits() >= iMaxUnitsPerCity * getNumCities())
				{
					return -1;
				}
			}

			if(NULL != pCity)
			{
				if(!pCity->canTrain(eUnit))
				{
					return -1;
				}
			}
			else
			{
				if(!pPlot->canTrain(eUnit, false, false))
				{
					return -1;
				}

				if(pPlot->isImpassable() || pPlot->isMountain())
				{
					return -1;
				}
			}
		}
		// Must be this unit at plot in order to remove
		else
		{
			bool bUnitFound = false;

			IDInfo* pUnitNode = pPlot->headUnitNode();
			while(pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(*pUnitNode);
				pUnitNode = pPlot->nextUnitNode(pUnitNode);

				if(NULL != pLoopUnit && pLoopUnit->getUnitType() == eUnit)
				{
					bUnitFound = true;
				}
			}

			if(!bUnitFound)
			{
				return -1;
			}
		}
	}

	return iCost;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing a City
/////////////////////////////////////////////////////////////////////////////////////////////

int CvPlayer::getAdvancedStartCityCost(bool bAdd, CvPlot* pPlot)
{
	int iNumCities = getNumCities();

	int iCost = getNewCityProductionValue();

	if(iCost < 0)
	{
		return -1;
	}

	// Valid plot?
	if(pPlot != NULL)
	{
		// Need valid plot to found on if adding
		if(bAdd)
		{
			if(!canFound(pPlot->getX(), pPlot->getY(), false))
			{
				return -1;
			}
		}
		// Need your own city present to remove
		else
		{
			if(pPlot->isCity())
			{
				if(pPlot->getPlotCity()->getOwner() != GetID())
				{
					return -1;
				}
			}
			else
			{
				return -1;
			}
		}

		// Is there a distance limit on how far a city can be placed from a player's start/another city?
		if(GC.getADVANCED_START_CITY_PLACEMENT_MAX_RANGE() > 0)
		{
			PlayerTypes eClosestPlayer = NO_PLAYER;
			int iClosestDistance = INT_MAX;

			for(int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);

				if(kPlayer.isAlive())
				{
					CvPlot* pStartingPlot = kPlayer.getStartingPlot();

					if(NULL != pStartingPlot)
					{
						int iDistance = ::plotDistance(pPlot->getX(), pPlot->getY(), pStartingPlot->getX(), pStartingPlot->getY());
						if(iDistance <= GC.getADVANCED_START_CITY_PLACEMENT_MAX_RANGE())
						{
							if(iDistance < iClosestDistance || (iDistance == iClosestDistance && getTeam() != kPlayer.getTeam()))
							{
								iClosestDistance = iDistance;
								eClosestPlayer = kPlayer.GetID();
							}
						}
					}
				}
			}

			if(NO_PLAYER == eClosestPlayer || GET_PLAYER(eClosestPlayer).getTeam() != getTeam())
			{
				return -1;
			}
			//Only allow founding a city at someone elses start point if
			//We have no cities and they have no cities.
			if((GetID() != eClosestPlayer) && ((getNumCities() > 0) || (GET_PLAYER(eClosestPlayer).getNumCities() > 0)))
			{
				return -1;
			}
		}
	}

	// Increase cost if the XML defines that additional units will cost more
	if(0 != GC.getADVANCED_START_CITY_COST_INCREASE())
	{
		if(!bAdd)
		{
			--iNumCities;
		}

		if(iNumCities > 0)
		{
			iCost *= 100 + GC.getADVANCED_START_CITY_COST_INCREASE() * iNumCities;
			iCost /= 100;
		}
	}

	return iCost;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Population
/////////////////////////////////////////////////////////////////////////////////////////////

int CvPlayer::getAdvancedStartPopCost(bool bAdd, CvCity* pCity)
{
	if(0 == getNumCities())
	{
		return -1;
	}

	int iCost = (getGrowthThreshold(1) * GC.getADVANCED_START_POPULATION_COST()) / 100;

	if(NULL != pCity)
	{
		if(pCity->getOwner() != GetID())
		{
			return -1;
		}

		int iPopulation = pCity->getPopulation();

		// Need to have Population to remove it
		if(!bAdd)
		{
			--iPopulation;

			if(iPopulation < GC.getINITIAL_CITY_POPULATION() + GC.getGame().getStartEraInfo().getFreePopulation())
			{
				return -1;
			}
		}

		iCost = (getGrowthThreshold(iPopulation) * GC.getADVANCED_START_POPULATION_COST()) / 100;

		// Increase cost if the XML defines that additional Pop will cost more
		if(0 != GC.getADVANCED_START_POPULATION_COST_INCREASE())
		{
			--iPopulation;

			if(iPopulation > 0)
			{
				iCost *= 100 + GC.getADVANCED_START_POPULATION_COST_INCREASE() * iPopulation;
				iCost /= 100;
			}
		}
	}

	return iCost;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing a Building from a city
/////////////////////////////////////////////////////////////////////////////////////////////

int CvPlayer::getAdvancedStartBuildingCost(BuildingTypes eBuilding, bool bAdd, CvCity* pCity)
{
	if(0 == getNumCities())
	{
		return -1;
	}

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
	{
		return -1;
	}

	int iCost = getProductionNeeded(eBuilding);

	if(iCost < 0)
	{
		return -1;
	}

	if(pkBuildingInfo->GetFreeStartEra() != NO_ERA && GC.getGame().getStartEra() >=  pkBuildingInfo->GetFreeStartEra())
	{
		// you get this building for free
		return -1;
	}

	if(NULL == pCity)
	{
		if(bAdd)
		{
			bool bValid = false;
			int iLoop;
			for(CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				if(pLoopCity->canConstruct(eBuilding))
				{
					bValid = true;
					break;
				}
			}

			if(!bValid)
			{
				return -1;
			}
		}
	}
	if(NULL != pCity)
	{
		if(pCity->getOwner() != GetID())
		{
			return -1;
		}

		iCost *= 100;
		iCost /= std::max(1, 100 + pCity->getProductionModifier(eBuilding));

		if(bAdd)
		{
			if(!pCity->canConstruct(eBuilding, true, false, false))
			{
				return -1;
			}
		}
		else
		{
			if(pCity->GetCityBuildings()->GetNumRealBuilding(eBuilding) <= 0)
			{
				return -1;
			}

			// Check other buildings in this city and make sure none of them require this one

			// Loop through Buildings to see which are present
			for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
			{
				const BuildingTypes eBuildingLoop = static_cast<BuildingTypes>(iBuildingLoop);
				CvBuildingEntry* pkBuildingLoopInfo = GC.getBuildingInfo(eBuildingLoop);
				if(pkBuildingLoopInfo)
				{
					if(pCity->GetCityBuildings()->GetNumBuilding(eBuildingLoop) > 0)
					{
						// Loop through present Building's requirements
						for(int iBuildingClassPrereqLoop = 0; iBuildingClassPrereqLoop < GC.getNumBuildingClassInfos(); iBuildingClassPrereqLoop++)
						{
							const BuildingClassTypes eBuildingClass = static_cast<BuildingClassTypes>(iBuildingClassPrereqLoop);
							CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
							if(pkBuildingClassInfo)
							{
								if(pkBuildingLoopInfo->IsBuildingClassNeededInCity(iBuildingClassPrereqLoop))
								{
									if((BuildingTypes)(getCivilizationInfo().getCivilizationBuildings(iBuildingClassPrereqLoop)) == eBuilding)
									{
										return -1;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return iCost;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Route
/////////////////////////////////////////////////////////////////////////////////////////////

int CvPlayer::getAdvancedStartRouteCost(RouteTypes eRoute, bool bAdd, CvPlot* pPlot)
{
	if(0 == getNumCities())
	{
		return -1;
	}

	if(eRoute == NO_ROUTE)
	{
		return -1;
	}

	CvRouteInfo* pkRouteInfo = GC.getRouteInfo(eRoute);
	if(pkRouteInfo == NULL)
	{
		return -1;
	}

	int iCost = pkRouteInfo->getAdvancedStartCost();

	// This denotes cities may not be purchased through Advanced Start
	if(iCost < 0)
	{
		return -1;
	}

	iCost *= GC.getGame().getGameSpeedInfo().getBuildPercent();
	iCost /= 100;

	// No invalid plots!
	if(pPlot != NULL)
	{
		if(pPlot->isCity())
		{
			return -1;
		}

		if(bAdd)
		{
			if(pPlot->isImpassable() || pPlot->isWater() || pPlot->isMountain())
			{
				return -1;
			}
			// Can't place twice
			if(pPlot->getRouteType() == eRoute)
			{
				return -1;
			}
		}
		else
		{
			// Need Route to remove it
			if(pPlot->getRouteType() != eRoute)
			{
				return -1;
			}
		}

		// Must be owned by me
		if(pPlot->getOwner() != GetID())
		{
			return -1;
		}
	}

	// Tech requirement
	for(int iBuildLoop = 0; iBuildLoop < GC.getNumBuildInfos(); iBuildLoop++)
	{
		const BuildTypes eBuild = static_cast<BuildTypes>(iBuildLoop);
		CvBuildInfo* pkBuildInfo = GC.getBuildInfo(eBuild);
		if(pkBuildInfo)
		{
			if(pkBuildInfo->getRoute() == eRoute)
			{
				if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)pkBuildInfo->getTechPrereq())))
				{
					return -1;
				}
			}
		}
	}

	return iCost;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Improvement
/////////////////////////////////////////////////////////////////////////////////////////////

int CvPlayer::getAdvancedStartImprovementCost(ImprovementTypes eImprovement, bool bAdd, CvPlot* pPlot)
{
	if(eImprovement == NO_IMPROVEMENT)
	{
		return -1;
	}

	if(0 == getNumCities())
	{
		return -1;
	}

	int iCost = 0;//GC.getImprovementInfo(eImprovement)->GetAdvancedStartCost();

	// This denotes cities may not be purchased through Advanced Start
	if(iCost < 0)
	{
		return -1;
	}

	iCost *= GC.getGame().getGameSpeedInfo().getBuildPercent();
	iCost /= 100;

	// Can this Improvement be on our plot?
	if(pPlot != NULL)
	{
		if(bAdd)
		{
			// Valid Plot
			if(!pPlot->canHaveImprovement(eImprovement, getTeam(), false))
			{
				return -1;
			}

			bool bValid = false;

			for(int iI = 0; iI < GC.getNumBuildInfos(); ++iI)
			{
				CvBuildInfo* pkBuildInfo = GC.getBuildInfo((BuildTypes) iI);
				if(!pkBuildInfo)
				{
					continue;
				}
				ImprovementTypes eLoopImprovement = ((ImprovementTypes)(pkBuildInfo->getImprovement()));

				if(eImprovement == eLoopImprovement && canBuild(pPlot, (BuildTypes)iI))
				{
					bValid = true;

					FeatureTypes eFeature = pPlot->getFeatureType();
					if(NO_FEATURE != eFeature && pkBuildInfo->isFeatureRemove(eFeature))
					{
						iCost += GC.getFeatureInfo(eFeature)->getAdvancedStartRemoveCost();
					}

					break;
				}
			}

			if(!bValid)
			{
				return -1;
			}

			// Can't place twice
			if(pPlot->getImprovementType() == eImprovement)
			{
				return -1;
			}
		}
		else
		{
			// Need this improvement in order to remove it
			if(pPlot->getImprovementType() != eImprovement)
			{
				return -1;
			}
		}

		// Must be owned by me
		if(pPlot->getOwner() != GetID())
		{
			return -1;
		}
	}

	// Tech requirement
	for(int iBuildLoop = 0; iBuildLoop < GC.getNumBuildInfos(); iBuildLoop++)
	{
		CvBuildInfo* pkBuildInfo = GC.getBuildInfo((BuildTypes) iBuildLoop);
		if(!pkBuildInfo)
		{
			continue;
		}

		if(pkBuildInfo->getImprovement() == eImprovement)
		{
			if(!(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)pkBuildInfo->getTechPrereq())))
			{
				return -1;
			}
		}
	}

	return iCost;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Tech
/////////////////////////////////////////////////////////////////////////////////////////////

int CvPlayer::getAdvancedStartTechCost(TechTypes eTech, bool bAdd)
{
	if(eTech == NO_TECH)
	{
		return -1;
	}

	if(0 == getNumCities())
	{
		return -1;
	}

	int iCost = (GET_TEAM(getTeam()).GetTeamTechs()->GetResearchCost(eTech) * GC.getTechInfo(eTech)->GetAdvancedStartCost()) / 100;
	if(iCost < 0)
	{
		return -1;
	}

	if(bAdd)
	{
		if(!GetPlayerTechs()->CanResearch(eTech, false))
		{
			return -1;
		}
	}
	else if(!bAdd)
	{
		if(!GET_TEAM(getTeam()).GetTeamTechs()->HasTech(eTech))
		{
			return -1;
		}

		// Search through all techs to see if any of the currently owned ones requires this tech
		for(int iTechLoop = 0; iTechLoop < GC.getNumTechInfos(); iTechLoop++)
		{
			TechTypes eTechLoop = (TechTypes) iTechLoop;

			if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech(eTechLoop))
			{
				int iPrereqLoop;

				// Or Prereqs
				for(iPrereqLoop = 0; iPrereqLoop < GC.getNUM_OR_TECH_PREREQS(); iPrereqLoop++)
				{
					if(GC.getTechInfo(eTechLoop)->GetPrereqOrTechs(iPrereqLoop) == eTech)
					{
						return -1;
					}
				}

				// And Prereqs
				for(iPrereqLoop = 0; iPrereqLoop < GC.getNUM_AND_TECH_PREREQS(); iPrereqLoop++)
				{
					if(GC.getTechInfo(eTechLoop)->GetPrereqAndTechs(iPrereqLoop) == eTech)
					{
						return -1;
					}
				}
			}
		}

		// If player has placed anything on the map which uses this tech then you cannot remove it
		int iLoop;

		// Units
		CvUnit* pLoopUnit;
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			if(pLoopUnit->getUnitInfo().GetPrereqAndTech() == eTech)
			{
				return -1;
			}

			for(int iI = 0; iI < GC.getNUM_UNIT_AND_TECH_PREREQS(); iI++)
			{
				if(pLoopUnit->getUnitInfo().GetPrereqAndTechs(iI) == eTech)
				{
					return -1;
				}
			}
		}

		// Cities
		CvCity* pLoopCity;
		for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			// All Buildings
			for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
			{
				const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
				CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
				if(pkBuildingInfo)
				{
					if(pLoopCity->GetCityBuildings()->GetNumRealBuilding(eBuilding) > 0)
					{
						if(pkBuildingInfo->GetPrereqAndTech() == eTech)
						{
							return -1;
						}

						for(int iI = 0; iI < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iI++)
						{
							if(pkBuildingInfo->GetPrereqAndTechs(iI) == eTech)
							{
								return -1;
							}
						}
					}
				}
			}
		}

	}

	return iCost;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Visibility
/////////////////////////////////////////////////////////////////////////////////////////////

int CvPlayer::getAdvancedStartVisibilityCost(bool bAdd, CvPlot* pPlot)
{
	if(0 == getNumCities())
	{
		return -1;
	}

	int iNumVisiblePlots = 0;
	int iCost = GC.getADVANCED_START_VISIBILITY_COST();

	// This denotes Visibility may not be purchased through Advanced Start
	if(iCost == -1)
	{
		return -1;
	}

	// Valid Plot?
	if(pPlot != NULL)
	{
		if(bAdd)
		{
			if(pPlot->isRevealed(getTeam()))
			{
				return -1;
			}
			if(!pPlot->isAdjacentRevealed(getTeam()))
			{
				return -1;
			}
		}
		else
		{
			if(!pPlot->isRevealed(getTeam()))
			{
				return -1;
			}
		}
	}

	// Increase cost if the XML defines that additional units will cost more
	if(0 != GC.getADVANCED_START_VISIBILITY_COST_INCREASE())
	{
		const int nPlots = GC.getMap().numPlots();
		for(int iPlotLoop = 0; iPlotLoop < nPlots; iPlotLoop++)
		{
			CvPlot* pMapPlot = GC.getMap().plotByIndexUnchecked(iPlotLoop);

			if(pMapPlot->isRevealed(getTeam()))
			{
				++iNumVisiblePlots;
			}
		}

		if(!bAdd)
		{
			--iNumVisiblePlots;
		}

		if(iNumVisiblePlots > 0)
		{
			iCost *= 100 + GC.getADVANCED_START_VISIBILITY_COST_INCREASE() * iNumVisiblePlots;
			iCost /= 100;
		}
	}

	return iCost;
}

//	--------------------------------------------------------------------------------
void CvPlayer::doWarnings()
{
	if(m_eID == GC.getGame().getActivePlayer())
	{
		//update enemy units close to your territory
		int iMaxCount = range(((getNumCities() + 4) / 7), 2, 5);
		for(int iI = 0; iI < GC.getMap().numPlots(); iI++)
		{
			if(iMaxCount == 0)
			{
				break;
			}

			CvPlot* pLoopPlot = GC.getMap().plotByIndexUnchecked(iI);

			if(pLoopPlot->isAdjacentPlayer(GetID()))
			{
				if(!(pLoopPlot->isCity()))
				{
					if(pLoopPlot->isVisible(getTeam()))
					{
						CvUnit* pUnit = pLoopPlot->getVisibleEnemyDefender(GetID());
						if(pUnit != NULL)
						{
							CvCity* pNearestCity = GC.getMap().findCity(pLoopPlot->getX(), pLoopPlot->getY(), GetID(), NO_TEAM, !(pLoopPlot->isWater()));

							if(pNearestCity != NULL)
							{
								CvString message = GetLocalizedText("TXT_KEY_MISC_ENEMY_TROOPS_SPOTTED", pNearestCity->getNameKey());
								GC.GetEngineUserInterface()->AddPlotMessage(0, pLoopPlot->GetPlotIndex(), GetID(), true, GC.getEVENT_MESSAGE_TIME(), message);

								iMaxCount--;
							}
						}
					}
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::processPolicies(PolicyTypes ePolicy, int iChange)
{
	int iI, iJ;

	CvPolicyEntry* pPolicy = GC.getPolicyInfo(ePolicy);
	if(pPolicy == NULL)
		return;

	const CvPolicyEntry& kPolicy = (*pPolicy);

	ChangeCulturePerWonder(pPolicy->GetCulturePerWonder() * iChange);
	ChangeCultureWonderMultiplier(pPolicy->GetCultureWonderMultiplier() * iChange);
	ChangeCulturePerTechResearched(pPolicy->GetCulturePerTechResearched() * iChange);
	ChangeGoldenAgeMeterMod(pPolicy->GetGoldenAgeMeterMod() * iChange);
	changeGoldenAgeModifier(pPolicy->GetGoldenAgeDurationMod() * iChange);
	changeWorkerSpeedModifier(pPolicy->GetWorkerSpeedModifier() * iChange);
	changeImprovementCostModifier(pPolicy->GetImprovementCostModifier() * iChange);
	changeImprovementUpgradeRateModifier(pPolicy->GetImprovementUpgradeRateModifier() * iChange);
	changeSpecialistProductionModifier(pPolicy->GetSpecialistProductionModifier() * iChange);
	changeMilitaryProductionModifier(pPolicy->GetMilitaryProductionModifier() * iChange);
	changeBaseFreeUnits(pPolicy->GetBaseFreeUnits() * iChange);
	ChangeHappinessPerGarrisonedUnit(pPolicy->GetHappinessPerGarrisonedUnit() * iChange);
	ChangeHappinessPerTradeRoute(pPolicy->GetHappinessPerTradeRoute() * iChange);
	ChangeHappinessPerXPopulation(pPolicy->GetHappinessPerXPopulation() * iChange);
	ChangeExtraHappinessPerLuxury(pPolicy->GetExtraHappinessPerLuxury() * iChange);
	ChangeUnhappinessFromUnitsMod(pPolicy->GetUnhappinessFromUnitsMod() * iChange);
	ChangeUnhappinessMod(pPolicy->GetUnhappinessMod() * iChange);
	ChangeCityCountUnhappinessMod(pPolicy->GetCityCountUnhappinessMod() * iChange);
	ChangeOccupiedPopulationUnhappinessMod(pPolicy->GetOccupiedPopulationUnhappinessMod() * iChange);
	ChangeCapitalUnhappinessMod(pPolicy->GetCapitalUnhappinessMod() * iChange);
	ChangeWoundedUnitDamageMod(pPolicy->GetWoundedUnitDamageMod() * iChange);
	ChangeUnitUpgradeCostMod(pPolicy->GetUnitUpgradeCostMod() * iChange);
	ChangeBarbarianCombatBonus(pPolicy->GetBarbarianCombatBonus() * iChange);
	ChangeAlwaysSeeBarbCampsCount(pPolicy->IsAlwaysSeeBarbCamps() * iChange);
	ChangeMaxNumBuilders(pPolicy->GetNumExtraBuilders() * iChange);
	ChangePlotGoldCostMod(pPolicy->GetPlotGoldCostMod() * iChange);
	ChangePlotCultureCostModifier(pPolicy->GetPlotCultureCostModifier() * iChange);
	ChangePlotCultureExponentModifier(pPolicy->GetPlotCultureExponentModifier() * iChange);
	ChangeNumCitiesPolicyCostDiscount(pPolicy->GetNumCitiesPolicyCostDiscount() * iChange);
	ChangeGarrisonFreeMaintenanceCount(pPolicy->IsGarrisonFreeMaintenance() * iChange);
	ChangeGarrisonedCityRangeStrikeModifier(pPolicy->GetGarrisonedCityRangeStrikeModifier() * iChange);
	ChangeUnitPurchaseCostModifier(pPolicy->GetUnitPurchaseCostModifier() * iChange);
	GetTreasury()->ChangeCityConnectionTradeRouteGoldModifier(pPolicy->GetCityConnectionTradeRouteGoldModifier() * iChange);
	changeGoldPerUnit(pPolicy->GetGoldPerUnit() * iChange);
	changeGoldPerMilitaryUnit(pPolicy->GetGoldPerMilitaryUnit() * iChange);
	ChangeCityStrengthMod(pPolicy->GetCityStrengthMod() * iChange);
	ChangeCityGrowthMod(pPolicy->GetCityGrowthMod() * iChange);
	ChangeCapitalGrowthMod(pPolicy->GetCapitalGrowthMod() * iChange);
	changeSettlerProductionModifier(pPolicy->GetSettlerProductionModifier() * iChange);
	changeCapitalSettlerProductionModifier(pPolicy->GetCapitalSettlerProductionModifier() * iChange);
	ChangeRouteGoldMaintenanceMod(pPolicy->GetRouteGoldMaintenanceMod() * iChange);
	ChangeBuildingGoldMaintenanceMod(pPolicy->GetBuildingGoldMaintenanceMod() * iChange);
	ChangeUnitGoldMaintenanceMod(pPolicy->GetUnitGoldMaintenanceMod() * iChange);
	ChangeUnitSupplyMod(pPolicy->GetUnitSupplyMod() * iChange);
	changeHappyPerMilitaryUnit(pPolicy->GetHappyPerMilitaryUnit() * iChange);
	changeHappinessToCulture(pPolicy->GetHappinessToCulture() * iChange);
	changeHappinessToScience(pPolicy->GetHappinessToScience() * iChange);
	changeHalfSpecialistUnhappinessCount((pPolicy->IsHalfSpecialistUnhappiness()) ? iChange : 0);
	changeHalfSpecialistFoodCount((pPolicy->IsHalfSpecialistFood()) ? iChange : 0);
	changeMilitaryFoodProductionCount((pPolicy->IsMilitaryFoodProduction()) ? iChange : 0);
	ChangeGoldenAgeCultureBonusDisabledCount((pPolicy->IsGoldenAgeCultureBonusDisabled()) ? iChange : 0);
	ChangeSecondReligionPantheonCount((pPolicy->IsSecondReligionPantheon()) ? iChange : 0);
	ChangeEnablesSSPartHurryCount((pPolicy->IsEnablesSSPartHurry()) ? iChange : 0);
	ChangeEnablesSSPartPurchaseCount((pPolicy->IsEnablesSSPartPurchase()) ? iChange : 0);
	changeMaxConscript(getWorldSizeMaxConscript(kPolicy) * iChange);
	changeExpModifier(pPolicy->GetExpModifier() * iChange);
	changeExpInBorderModifier(pPolicy->GetExpInBorderModifier() * iChange);
	changeMinorQuestFriendshipMod(pPolicy->GetMinorQuestFriendshipMod() * iChange);
	changeMinorGoldFriendshipMod(pPolicy->GetMinorGoldFriendshipMod() * iChange);
	ChangeMinorFriendshipAnchorMod(pPolicy->GetMinorFriendshipMinimum() * iChange);
	changeGetMinorFriendshipDecayMod(pPolicy->GetMinorFriendshipDecayMod() * iChange);
	ChangeMinorScienceAlliesCount(pPolicy->IsMinorScienceAllies() * iChange);
	ChangeMinorResourceBonusCount(pPolicy->IsMinorResourceBonus() * iChange);
	ChangeNewCityExtraPopulation(pPolicy->GetNewCityExtraPopulation() * iChange);
	ChangeFreeFoodBox(pPolicy->GetFreeFoodBox() * iChange);
	ChangeStrategicResourceMod(pPolicy->GetStrategicResourceMod() * iChange);
	ChangeAbleToAnnexCityStatesCount((pPolicy->IsAbleToAnnexCityStates()) ? iChange : 0);

	if(pPolicy->IsOneShot())
	{
		if(m_pPlayerPolicies->HasOneShotPolicyFired(ePolicy))
		{
			return;
		}
		else
		{
			m_pPlayerPolicies->SetOneShotPolicyFired(ePolicy,true);
		}
	}

	GetPlayerPolicies()->ChangeNumExtraBranches(pPolicy->GetNumExtraBranches() * iChange);

	ChangeAllFeatureProduction(pPolicy->GetAllFeatureProduction());

	int iMod;
	YieldTypes eYield;

	for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		eYield = (YieldTypes) iI;

		iMod = pPolicy->GetYieldModifier(iI) * iChange;
		if(iMod != 0)
			changeYieldRateModifier(eYield, iMod);

		iMod = pPolicy->GetCityYieldChange(iI) * iChange;
		if(iMod != 0)
			ChangeCityYieldChange(eYield, iMod * 100);

		iMod = pPolicy->GetCoastalCityYieldChange(iI) * iChange;
		if(iMod != 0)
			ChangeCoastalCityYieldChange(eYield, iMod);

		iMod = pPolicy->GetCapitalYieldChange(iI) * iChange;
		if(iMod != 0)
			ChangeCapitalYieldChange(eYield, iMod * 100);

		iMod = pPolicy->GetCapitalYieldPerPopChange(iI) * iChange;
		if(iMod != 0)
			ChangeCapitalYieldPerPopChange(eYield, iMod);

		iMod = pPolicy->GetCapitalYieldModifier(iI) * iChange;
		if(iMod != 0)
			changeCapitalYieldRateModifier(eYield, iMod);

		iMod = pPolicy->GetGreatWorkYieldChange(iI) * iChange;
		if(iMod != 0)
			ChangeGreatWorkYieldChange(eYield, iMod);

#ifdef POLICY_SPECIALIST_EXTRA_YIELDS_BY_SPECIALIST_TYPE
		for (iJ = 0; iJ < GC.getNumSpecialistInfos(); iJ++)
		{
			SpecialistTypes eSpecialist = (SpecialistTypes)iJ;
			iMod = pPolicy->GetSpecialistExtraYield(iJ, iI) * iChange;
			if (iMod != 0)
				changeSpecialistExtraYield(eSpecialist, eYield, iMod);
		}
#else
		iMod = pPolicy->GetSpecialistExtraYield(iI) * iChange;
		if(iMod != 0)
			changeSpecialistExtraYield(eYield, iMod);
#endif

#ifdef POLICY_GOLDEN_AGE_YIELD_MOD
		iMod = pPolicy->GetGoldenAgeYieldModifier(iI) * iChange;
		if (iMod != 0)
			changeGoldenAgeYieldModifier(eYield, iMod);
#endif

#ifdef POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
		iMod = pPolicy->GetPlotExtraYieldFromTradeRoute(iI) * iChange;
		if (iMod != 0)
			changePlotExtraYieldFromTradeRoute(eYield, iMod);
#endif
	}
#ifdef POLICY_ONLY_INTERNAL_TRADE_ROUTE_YIELD_MODIFIER
	iMod = pPolicy->GetInternalTradeRouteYieldModifier() * iChange;
	if (iMod > 0)
	{
		GC.getGame().GetGameTrade()->ClearAllTradeRoutesToPlayerByType(GetID(), TRADE_CONNECTION_INTERNATIONAL);
	}
#endif

	for(iI = 0; iI < GC.getNumUnitCombatClassInfos(); iI++)
	{
		changeUnitCombatProductionModifiers((UnitCombatTypes)iI, (pPolicy->GetUnitCombatProductionModifiers(iI) * iChange));
		changeUnitCombatFreeExperiences((UnitCombatTypes)iI, (pPolicy->GetUnitCombatFreeExperiences(iI) * iChange));
	}

	for(iI = 0; iI < GC.getNumHurryInfos(); iI++)
	{
		if(GC.getHurryInfo((HurryTypes) iI)->getPolicyPrereq() == ePolicy)
		{
			changeHurryCount(((HurryTypes)iI), iChange);
		}
		{
			changeHurryModifier((HurryTypes) iI, (pPolicy->GetHurryModifier(iI) * iChange));
		}
	}

	for(iI = 0; iI < GC.getNumImprovementInfos(); iI++)
	{
		for(iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
			changeImprovementYieldChange(((ImprovementTypes)iI), ((YieldTypes)iJ), (pPolicy->GetImprovementYieldChanges(iI, iJ) * iChange));
		}
	}

	// Free Promotions
	PromotionTypes ePromotion;
	for(iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		ePromotion = (PromotionTypes) iI;

		if(pPolicy->IsFreePromotion(ePromotion))
			ChangeFreePromotionCount(ePromotion, iChange);
	}

	CvCity* pLoopCity;
	PlayerTypes ePlayer;

	// All player Capital Locations Revealed
	if(pPolicy->IsRevealAllCapitals())
	{
		for(iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			ePlayer = (PlayerTypes) iI;

			if(GET_PLAYER(ePlayer).isAlive())
			{
				pLoopCity = GET_PLAYER(ePlayer).getCapitalCity();

				if(pLoopCity != NULL)
				{
					pLoopCity->plot()->setRevealed(getTeam(), true);
				}
			}
		}
	}

	// Friendship Decay for OTHER PLAYERS
	CvNotifications* pNotifications;
	Localization::String locString;
	Localization::String locSummary;

	int iOtherPlayersDecay = pPolicy->GetOtherPlayersMinorFriendshipDecayMod();
	if(iOtherPlayersDecay != 0)
	{
		for(iI = 0; iI < MAX_MAJOR_CIVS; iI++)
		{
			ePlayer = (PlayerTypes) iI;

			if(GET_PLAYER(ePlayer).isEverAlive())
			{
				// Don't hurt us or teammates
				if(GET_PLAYER(ePlayer).getTeam() != getTeam())
				{
					GET_PLAYER(ePlayer).changeGetMinorFriendshipDecayMod(iOtherPlayersDecay * iChange);

					// Send notification to affected players
					locString = Localization::Lookup("TXT_KEY_NOTIFICATION_MINOR_FRIENDSHIP_DECAY");
					locString << getNameKey();
					locSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_MINOR_FRIENDSHIP_DECAY");

					pNotifications = GET_PLAYER(ePlayer).GetNotifications();
					if(pNotifications)
					{
						pNotifications->Add(NOTIFICATION_DIPLOMACY_DECLARATION, locString.toUTF8(), locSummary.toUTF8(), -1, -1, -1);
					}
				}
			}
		}
	}

	BuildingClassTypes eBuildingClass;
	BuildingTypes eBuilding;
	int iBuildingCount;
	int iYieldMod;
	int iYieldChange;

	// How many cities get free culture buildings?
	int iNumCitiesFreeCultureBuilding = pPolicy->GetNumCitiesFreeCultureBuilding();
	int iNumCitiesFreeFoodBuilding = pPolicy->GetNumCitiesFreeFoodBuilding();
#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
	int iNumCitiesFreeDefensiveBuilding = pPolicy->GetNumCitiesFreeDefensiveBuilding();
#endif

	// Loop through Cities
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if(iNumCitiesFreeCultureBuilding > 0)
		{
			BuildingTypes eCultureBuilding = pLoopCity->ChooseFreeCultureBuilding();
			if(eCultureBuilding != NO_BUILDING)
			{
				pLoopCity->GetCityBuildings()->SetNumFreeBuilding(eCultureBuilding, 1);

				if(pLoopCity->getFirstBuildingOrder(eCultureBuilding) == 0)
				{
					pLoopCity->clearOrderQueue();
					pLoopCity->chooseProduction();		// Send a notification to the user that what they were building was given to them, and they need to produce something else.
				}
			}
			else
			{
				pLoopCity->SetOwedCultureBuilding(true);
			}

			// Decrement cities left to get free culture building (at end of loop we'll set the remainder)
			iNumCitiesFreeCultureBuilding--;
		}

		if(iNumCitiesFreeFoodBuilding > 0)
		{
			BuildingTypes eFoodBuilding = pLoopCity->ChooseFreeFoodBuilding();
			if(eFoodBuilding != NO_BUILDING)
			{
#ifdef AQUEDUCT_FIX
				pLoopCity->GetCityBuildings()->SetNumRealBuilding(eFoodBuilding, 0);
#endif
				pLoopCity->GetCityBuildings()->SetNumFreeBuilding(eFoodBuilding, 1);

				if(pLoopCity->getFirstBuildingOrder(eFoodBuilding) == 0)
				{
					pLoopCity->clearOrderQueue();
					pLoopCity->chooseProduction();		// Send a notification to the user that what they were building was given to them, and they need to produce something else.
				}
			}
#ifdef OWED_FOOD_BUILDING
			else
			{
				pLoopCity->SetOwedFoodBuilding(true);
			}
#endif

			// Decrement cities left to get free food building (at end of loop we'll set the remainder)
			iNumCitiesFreeFoodBuilding--;
		}

#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
		if (iNumCitiesFreeDefensiveBuilding > 0)
		{
			BuildingTypes eDefensiveBuilding = NO_BUILDING;
			if (GetPlayerTraits()->GetGreatScientistRateModifier() > 0)
			{
				eDefensiveBuilding = (BuildingTypes)GC.getInfoTypeForString("BUILDING_WALLS_OF_BABYLON");
			}
			else
			{
				eDefensiveBuilding = (BuildingTypes)GC.getInfoTypeForString("BUILDING_WALLS");
			}
			if (eDefensiveBuilding != NO_BUILDING)
			{
				pLoopCity->GetCityBuildings()->SetNumRealBuilding(eDefensiveBuilding, 0);
				pLoopCity->GetCityBuildings()->SetNumFreeBuilding(eDefensiveBuilding, 1);

				if (pLoopCity->getFirstBuildingOrder(eDefensiveBuilding) == 0)
				{
					pLoopCity->clearOrderQueue();
					pLoopCity->chooseProduction();		// Send a notification to the user that what they were building was given to them, and they need to produce something else.
				}
			}

			// Decrement cities left to get free culture building (at end of loop we'll set the remainder)
			iNumCitiesFreeDefensiveBuilding--;
		}
#endif

		// Free Culture-per-turn in every City
		int iCityCultureChange = pPolicy->GetCulturePerCity() * iChange;
		if(pLoopCity->GetGarrisonedUnit() != NULL)
		{
#ifndef FIX_POLICY_CULTURE_PER_GARRISONED_UNIT
			iCityCultureChange += (pPolicy->GetCulturePerGarrisonedUnit() * iChange);
#endif
		}
		pLoopCity->ChangeJONSCulturePerTurnFromPolicies(iCityCultureChange);

		// Building modifiers
		for(iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			eBuildingClass = (BuildingClassTypes) iI;

			CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
			if(!pkBuildingClassInfo)
			{
				continue;
			}

			eBuilding = (BuildingTypes) getCivilizationInfo().getCivilizationBuildings(eBuildingClass);

			if(eBuilding != NO_BUILDING)
			{
				CvBuildingEntry* pkBuilding = GC.getBuildingInfo(eBuilding);
				if(pkBuilding)
				{
					iBuildingCount = pLoopCity->GetCityBuildings()->GetNumBuilding(eBuilding);

					if(iBuildingCount > 0)
					{
						pLoopCity->ChangeJONSCulturePerTurnFromPolicies(pPolicy->GetBuildingClassCultureChange(eBuildingClass) * iBuildingCount * iChange);

						// Building Class Yield Stuff
						for(iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
						{
							switch(iJ)
							{
							case YIELD_CULTURE:
								// Skip, handled above
								break;
							case YIELD_FAITH:
								pLoopCity->ChangeFaithPerTurnFromPolicies(pPolicy->GetBuildingClassYieldChanges(eBuildingClass, iJ) * iBuildingCount * iChange);
								break;
							default:
								{
									eYield = (YieldTypes) iJ;
									iYieldMod = pPolicy->GetBuildingClassYieldModifiers(eBuildingClass, eYield);
									if (iYieldMod > 0)
									{
										pLoopCity->changeYieldRateModifier(eYield, iYieldMod * iBuildingCount * iChange);
									}
									iYieldChange = pPolicy->GetBuildingClassYieldChanges(eBuildingClass, eYield);
									if (iYieldChange != 0)
									{
										pLoopCity->ChangeBaseYieldRateFromBuildings(eYield, iYieldChange * iBuildingCount * iChange);
									}
								}
							}
						}
					}
				}
			}
		}
	}

#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		for (iJ = 0; iJ < NUM_SPECILIST_TYPES; iJ++)
		{
			changeBuildingScecialistCountChange(((BuildingTypes)iI), ((SpecialistTypes)iJ), (pPolicy->GetBuildingScecialistCountChanges(iI, iJ) * iChange));
		}
	}
#endif

	// Store off number of newly built cities that will get a free building
	ChangeNumCitiesFreeCultureBuilding(iNumCitiesFreeCultureBuilding);
	ChangeNumCitiesFreeFoodBuilding(iNumCitiesFreeFoodBuilding);
#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
	ChangeNumCitiesFreeDefensiveBuilding(iNumCitiesFreeDefensiveBuilding);
#endif
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
	ChangeMaxExtraVotesFromMinors(pPolicy->GetMaxExtraVotesFromMinors() * iChange);
#endif
#ifdef POLICY_EXTRA_VOTES
	ChangePolicyExtraVotes(pPolicy->GetExtraVotes() * iChange);
#endif
#ifdef POLICY_DO_TECH_FROM_CITY_CONQ
	ChangePolicyTechFromCityConquer(pPolicy->IsTechFromCityConquer() * iChange);
#endif
#ifdef POLICY_NO_CULTURE_SPECIALIST_FOOD
	ChangeNoCultureSpecialistFood(pPolicy->IsNoCultureSpecialistFood() * iChange);
#endif
#ifdef POLICY_MINORS_GIFT_UNITS
	ChangeMinorsGiftUnits(pPolicy->IsMinorsGiftUnits() * iChange);
#endif
#ifdef POLICY_NO_CARGO_PILLAGE
	ChangeNoCargoPillage(pPolicy->IsNoCargoPillage() * iChange);
#endif
#ifdef POLICY_GREAT_WORK_HAPPINESS
	ChangeGreatWorkHappiness(pPolicy->GetGreatWorkHappiness() * iChange);
#endif
#ifdef POLICY_SCIENCE_PER_X_FOLLOWERS
	ChangeSciencePerXFollowers(pPolicy->GetSciencePerXFollowers() * iChange);
#endif
#ifdef POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD
	ChangeNoDifferentIdeologiesTourismMod(pPolicy->IsNoDifferentIdeologiesTourismMod() * iChange);
#endif
#ifdef POLICY_GREAT_WORK_TOURISM_CHANGES
	ChangeGreatWorkTourismChanges(pPolicy->GetGreatWorkTourismChanges() * iChange);
#endif
#ifdef POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP
	ChangeCityScienceSquaredModPerXPop(pPolicy->GetCityScienceSquaredModPerXPop() * iChange);
#endif
#ifdef POLICY_EXTRA_SPIES
	if (pPolicy->GetExtraSpies() > 0)
	{
		CvPlayerEspionage* pEspionage = GetEspionage();
		CvAssertMsg(pEspionage, "pEspionage is null! What's up with that?!");
		if (pEspionage)
		{
			int iNumSpies = pPolicy->GetExtraSpies();
			for (int i = 0; i < iNumSpies; i++)
			{
				pEspionage->CreateSpy();
			}
		}
	}
#endif
#ifdef POLICY_SPY_DETECTION
	ChangeSpyDetection(pPolicy->IsSpyDetection() * iChange);
#endif

	// Not really techs but this is what we use (for now)
	for(iI = 0; iI < GC.getNUM_AND_TECH_PREREQS(); iI++)
	{
		if(pPolicy->GetPolicyDisables(iI) != NO_POLICY)
		{
			if(m_pPlayerPolicies->HasPolicy((PolicyTypes) pPolicy->GetPolicyDisables(iI)))
			{
				setHasPolicy((PolicyTypes) pPolicy->GetPolicyDisables(iI), false);
			}
		}
	}

	// Attack bonus for a period of time
	int iTurns = pPolicy->GetAttackBonusTurns() * iChange;
	if(iTurns > 0)
	{
		ChangeAttackBonusTurns(iTurns);
	}

	// Golden Age!
	int iGoldenAgeTurns = pPolicy->GetGoldenAgeTurns() * iChange;
	if(iGoldenAgeTurns > 0)
	{
		// Player modifier
		int iLengthModifier = getGoldenAgeModifier();

		// Trait modifier
		iLengthModifier += GetPlayerTraits()->GetGoldenAgeDurationModifier();

		if(iLengthModifier > 0)
		{
			iGoldenAgeTurns = iGoldenAgeTurns * (100 + iLengthModifier) / 100;
		}

		// Game Speed mod
		iGoldenAgeTurns *= GC.getGame().getGameSpeedInfo().getGoldenAgePercent();
		iGoldenAgeTurns /= 100;

		changeGoldenAgeTurns(iGoldenAgeTurns);
	}

#ifdef POLICY_MINOR_INFLUENCE_BOOST
	// City-State Influence Boost
	//antonjs: todo: ordering, to prevent ally / no longer ally notif spam
	int iInfluenceBoost = pPolicy->GetMinorInfluenceBoost() * iChange;
	if (iInfluenceBoost > 0)
	{
		for (int iMinorCivLoop = MAX_MAJOR_CIVS; iMinorCivLoop < MAX_CIV_PLAYERS; iMinorCivLoop++)
		{
			PlayerTypes eMinorCivLoop = (PlayerTypes)iMinorCivLoop;
			if (GET_PLAYER(eMinorCivLoop).isAlive() && GET_TEAM(GET_PLAYER((PlayerTypes)GetID()).getTeam()).isHasMet(GET_PLAYER(eMinorCivLoop).getTeam()))
			{
				GET_PLAYER(eMinorCivLoop).GetMinorCivAI()->ChangeFriendshipWithMajor((PlayerTypes)GetID(), iInfluenceBoost);
			}
		}
	}
#endif

	// Free Techs
	int iNumFreeTechs = pPolicy->GetNumFreeTechs() * iChange;
	if(iNumFreeTechs > 0)
	{
		if(!isHuman())
		{
			for(iI = 0; iI < iNumFreeTechs; iI++)
			{
				AI_chooseFreeTech();
			}
		}
		else
		{
			CvString strBuffer = GetLocalizedText("TXT_KEY_MISC_COMPLETED_WONDER_CHOOSE_TECH", pPolicy->GetTextKey());
			chooseTech(iNumFreeTechs, strBuffer.GetCString());
		}
	}

	ChangeMedianTechPercentage(pPolicy->GetMedianTechPercentChange());

	// Free Policies
	int iNumFreePolicies = pPolicy->GetNumFreePolicies() * iChange;
	if(iNumFreePolicies > 0)
	{
		ChangeNumFreePolicies(iNumFreePolicies);
	}

	if(pPolicy->IncludesOneShotFreeUnits())
	{
		if(!m_pPlayerPolicies->HaveOneShotFreeUnitsFired(ePolicy))
		{
			m_pPlayerPolicies->SetOneShotFreeUnitsFired(ePolicy,true);

			int iNumFreeGreatPeople = pPolicy->GetNumFreeGreatPeople() * iChange;
			if(iNumFreeGreatPeople > 0)
			{
				ChangeNumFreeGreatPeople(iNumFreeGreatPeople);
			}

			if(getCapitalCity() != NULL)
			{
				int iX = getCapitalCity()->getX();
				int iY = getCapitalCity()->getY();

				for(iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
				{
					const UnitClassTypes eUnitClass = static_cast<UnitClassTypes>(iI);
					CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eUnitClass);
					if(pkUnitClassInfo)
					{
						int iNumFreeUnits = pPolicy->GetNumFreeUnitsByClass(eUnitClass);
						if(iNumFreeUnits > 0)
						{
							const UnitTypes eUnit = (UnitTypes) getCivilizationInfo().getCivilizationUnits(eUnitClass);
							CvUnitEntry* pUnitEntry = GC.getUnitInfo(eUnit);
							if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman() && pUnitEntry != NULL && pUnitEntry->IsFound())
							{
								continue;
							}

							for(int iUnitLoop = 0; iUnitLoop < iNumFreeUnits; iUnitLoop++)
							{
								CvUnit* pNewUnit = NULL;

								// slewis
								// for venice
#ifdef NEW_VENICE_UA
								TraitTypes eTrait = (TraitTypes)GC.getInfoTypeForString("NEW_TRAIT_SUPER_CITY_STATE", true /*bHideAssert*/);
								if (pUnitEntry->IsFound() && GET_PLAYER(GetID()).GetPlayerTraits()->HasTrait(eTrait))
#else
								if (pUnitEntry->IsFound() && GetPlayerTraits()->IsNoAnnexing())
#endif
								{
									// drop a merchant of venice instead
									// find the eUnit replacement that's the merchant of venice
									for(int iVeniceSearch = 0; iVeniceSearch < GC.getNumUnitClassInfos(); iVeniceSearch++)
									{
										const UnitClassTypes eVeniceUnitClass = static_cast<UnitClassTypes>(iVeniceSearch);
										CvUnitClassInfo* pkVeniceUnitClassInfo = GC.getUnitClassInfo(eVeniceUnitClass);
										if(pkVeniceUnitClassInfo)
										{
											const UnitTypes eMerchantOfVeniceUnit = (UnitTypes) getCivilizationInfo().getCivilizationUnits(eVeniceUnitClass);
											if (eMerchantOfVeniceUnit != NO_UNIT)
											{
												CvUnitEntry* pVeniceUnitEntry = GC.getUnitInfo(eMerchantOfVeniceUnit);
												if (pVeniceUnitEntry->IsCanBuyCityState())
												{
													pNewUnit = initUnit(eMerchantOfVeniceUnit, iX, iY);
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
													ChangeNumMerchantsTotal(1);
#endif
													break;
												}
											}
										}
									}
								}
								else
								{
									pNewUnit = initUnit(eUnit, iX, iY);
								}

								CvAssert(pNewUnit);

								if (pNewUnit)
								{
									if(pNewUnit->IsGreatGeneral())
									{
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
										ChangeNumGeneralsTotal(1);
#else
										incrementGreatGeneralsCreated();
#endif
										pNewUnit->jumpToNearestValidPlot();
									}
									else if(pNewUnit->IsGreatAdmiral())
									{
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
										ChangeNumAdmiralsTotal(1);
#else
										incrementGreatAdmiralsCreated();
#endif
										CvPlot *pSpawnPlot = GetGreatAdmiralSpawnPlot(pNewUnit);
										if (pNewUnit->plot() != pSpawnPlot)
										{
											pNewUnit->setXY(pSpawnPlot->getX(), pSpawnPlot->getY());
										}
									}
									else if(pNewUnit->getUnitInfo().IsFoundReligion())
									{
										ReligionTypes eReligion = GetReligions()->GetReligionCreatedByPlayer();
										int iReligionSpreads = pNewUnit->getUnitInfo().GetReligionSpreads();
										int iReligiousStrength = pNewUnit->getUnitInfo().GetReligiousStrength();
										if(iReligionSpreads > 0 && eReligion > RELIGION_PANTHEON)
										{
											pNewUnit->GetReligionData()->SetSpreadsLeft(iReligionSpreads);
											pNewUnit->GetReligionData()->SetReligiousStrength(iReligiousStrength);
											pNewUnit->GetReligionData()->SetReligion(eReligion);
										}
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
										ChangeNumProphetsTotal(1);
#endif
									}
									else if (pNewUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_WRITER"))
									{
#ifndef FREE_GREAT_PERSON
										incrementGreatWritersCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
										ChangeNumWritersTotal(1);
#endif

#ifdef NEW_WRITERS_CULTURE_BOMB
										if (pNewUnit->getUnitInfo().GetBaseCultureTurnsToCount() > 0)
										{
											pNewUnit->SetCultureBombStrength(GetCultureYieldFromPreviousTurns(GC.getGame().getGameTurn(), pNewUnit->getUnitInfo().GetBaseCultureTurnsToCount()));
										}
#endif
#ifndef FIX_TOURISM_BLAST_FROM_POLICIES
										if (pNewUnit->getUnitInfo().GetOneShotTourism() > 0)
										{
											pNewUnit->SetTourismBlastStrength(GetCulture()->GetTourismBlastStrength(pNewUnit->getUnitInfo().GetOneShotTourism()));
										}
#endif

										pNewUnit->jumpToNearestValidPlot();
									}							
									else if (pNewUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
									{
#ifndef FREE_GREAT_PERSON
										incrementGreatArtistsCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
										ChangeNumArtistsTotal(1);
#endif
										pNewUnit->jumpToNearestValidPlot();
									}							
									else if (pNewUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
									{
#ifndef FREE_GREAT_PERSON
										incrementGreatMusiciansCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
										ChangeNumMusiciansTotal(1);
#endif
#ifdef FIX_TOURISM_BLAST_FROM_POLICIES
										if (pNewUnit->getUnitInfo().GetOneShotTourism() > 0)
										{
											pNewUnit->SetTourismBlastStrength(GetCulture()->GetTourismBlastStrength(pNewUnit->getUnitInfo().GetOneShotTourism()));
										}
#endif

										pNewUnit->jumpToNearestValidPlot();
									}
#if defined SEPARATE_GREAT_PEOPLE || defined SWEDEN_UA_REWORK
									else if (pNewUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_SCIENTIST"))
									{
#ifndef FREE_GREAT_PERSON
										incrementGreatScientistsCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
										ChangeNumScientistsTotal(1);
#endif

#ifdef NEW_SCIENTISTS_BULB
#ifdef DECREASE_BULB_AMOUNT_OVER_TIME
										pNewUnit->SetScientistBirthTurn(GC.getGame().getGameTurn());
#else
										if (pNewUnit->getUnitInfo().GetBaseBeakersTurnsToCount() > 0)
										{
											pNewUnit->SetResearchBulbAmount(kPlayer.GetScienceYieldFromPreviousTurns(GC.getGame().getGameTurn(), pNewUnit->getUnitInfo().GetBaseBeakersTurnsToCount()));
										}
#endif
#endif
										pNewUnit->jumpToNearestValidPlot();
									}
									else if (pNewUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_ENGINEER"))
									{
#ifndef FREE_GREAT_PERSON
										incrementGreatEngineersCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
										ChangeNumEngineersTotal(1);
#endif
										pNewUnit->jumpToNearestValidPlot();
									}
									else if (pNewUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
									{
#ifndef FREE_GREAT_PERSON
										incrementGreatMerchantsCreated();
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
										ChangeNumMerchantsTotal(1);
#endif
										pNewUnit->jumpToNearestValidPlot();
									}
									else if (pNewUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_PROPHET"))
									{
										incrementGreatProphetsCreated();
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
										ChangeNumProphetsTotal(1);
#endif
										pNewUnit->jumpToNearestValidPlot();
									}
#endif
#ifdef SEPARATE_MERCHANTS
									else if (pNewUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
									{
#ifndef FREE_GREAT_PERSON
										incrementGreatMerchantsCreated();
#endif
										pNewUnit->jumpToNearestValidPlot();
									}
#endif
									else if(pNewUnit->IsGreatPerson())
									{
#ifndef FREE_GREAT_PERSON
										incrementGreatPeopleCreated();
#endif
										pNewUnit->jumpToNearestValidPlot();
									}
									else
									{
										pNewUnit->jumpToNearestValidPlot();
									}
								}
							}
						}
					}
				}
			}
		}
	}

#ifdef POLICY_GLOBAL_POP_CHANGE
	// Global Pop change
	if (pPolicy->GetGlobalPopChange() != 0)
	{
		CvCity* pLoopCity;
		int iLoop;

		for (pLoopCity = GET_PLAYER((PlayerTypes)GetID()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)GetID()).nextCity(&iLoop))
		{
			if (iChange > 0)
			{
				pLoopCity->changePopulation(iChange * pPolicy->GetGlobalPopChange());
			}
		}
	}
	ChangeGlobalPopChange(iChange * pPolicy->GetGlobalPopChange());
#endif
#ifdef POLICY_HAPPINESS_PER_CITY
	ChangeExtraHappinessPerCity(iChange * pPolicy->GetHappinessPerCity());
#endif

	// Great People bonus from Allied city-states
	if(pPolicy->IsMinorGreatPeopleAllies())
	{
		DoAdoptedGreatPersonCityStatePolicy();
	}

	// Add a Reformation belief if eligible
	if (isHuman() && pPolicy->IsAddReformationBelief() && GetReligions()->HasCreatedReligion() && !GetReligions()->HasAddedReformationBelief())
	{
		pNotifications = GetNotifications();
		if(pNotifications)
		{
			CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ADD_REFORMATION_BELIEF");
			CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ADD_REFORMATION_BELIEF");
			pNotifications->Add(NOTIFICATION_ADD_REFORMATION_BELIEF, strBuffer, strSummary, -1, -1, -1);
		}
	}

	// if the steal tech faster amount is modified, then update the progress of all spies
	if (pPolicy->GetStealTechFasterModifier() != 0)
	{
		GetEspionage()->UpdateSpies();
	}

	CvPlot *pLoopPlot;
	ResourceTypes eResource;
	for(iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		pLoopPlot = GC.getMap().plotByIndexUnchecked(iI);
		eResource = pLoopPlot->getResourceType();
		if(eResource != NO_RESOURCE)
		{
			if(GC.getResourceInfo(eResource)->getPolicyReveal() == (int)ePolicy)
			{
				pLoopPlot->updateYield();
				if (pLoopPlot->isRevealed(getTeam()))
				{
					pLoopPlot->setLayoutDirty(true);
				}
			}
		}
	}

#ifdef FIX_POLICY_FREE_RELIGION
	iLoop = 0;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->UpdateReligion(pLoopCity->GetCityReligions()->GetReligiousMajority());
	}
#endif

	DoUpdateHappiness();
	GetTrade()->UpdateTradeConnectionValues();
	recomputeGreatPeopleModifiers();
	recomputePolicyCostModifier();
	recomputeFreeExperience();

	doUpdateBarbarianCampVisibility();

	GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
	GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
}

//	--------------------------------------------------------------------------------
/// If we should see where the locations of all current Barb Camps are, do it
void CvPlayer::doUpdateBarbarianCampVisibility()
{
	if(IsAlwaysSeeBarbCamps())
	{
		CvPlot* pPlot;

		ImprovementTypes eImprovement;

		for(int iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); ++iPlotLoop)
		{
			pPlot = GC.getMap().plotByIndexUnchecked(iPlotLoop);

			if(pPlot->isRevealed(getTeam()))
			{
				eImprovement = pPlot->getImprovementType();

				// Camp here
				if(eImprovement == GC.getBARBARIAN_CAMP_IMPROVEMENT())
				{
					// We don't see Camp
					if(pPlot->getRevealedImprovementType(getTeam()) != eImprovement)
					{
						pPlot->setRevealedImprovementType(getTeam(), eImprovement);
					}
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isPbemNewTurn() const
{
	return m_bPbemNewTurn;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setPbemNewTurn(bool bNew)
{
	m_bPbemNewTurn = bNew;
}

//	--------------------------------------------------------------------------------
CvEconomicAI* CvPlayer::GetEconomicAI() const
{
	return m_pEconomicAI;
}

//	--------------------------------------------------------------------------------
CvMilitaryAI* CvPlayer::GetMilitaryAI() const
{
	return m_pMilitaryAI;
}

//	--------------------------------------------------------------------------------
CvCitySpecializationAI* CvPlayer::GetCitySpecializationAI() const
{
	return m_pCitySpecializationAI;
}

//	--------------------------------------------------------------------------------
CvWonderProductionAI* CvPlayer::GetWonderProductionAI() const
{
	return m_pWonderProductionAI;
}

//	--------------------------------------------------------------------------------
CvGrandStrategyAI* CvPlayer::GetGrandStrategyAI() const
{
	return m_pGrandStrategyAI;
}

//	--------------------------------------------------------------------------------
CvDiplomacyAI* CvPlayer::GetDiplomacyAI() const
{
	return m_pDiplomacyAI;
}

//	--------------------------------------------------------------------------------
CvPlayerReligions* CvPlayer::GetReligions() const
{
	return m_pReligions;
}

//	--------------------------------------------------------------------------------
CvReligionAI* CvPlayer::GetReligionAI() const
{
	return m_pReligionAI;
}

//	--------------------------------------------------------------------------------
CvMinorCivAI* CvPlayer::GetMinorCivAI() const
{
	return m_pMinorCivAI;
}

//	--------------------------------------------------------------------------------
CvDealAI* CvPlayer::GetDealAI() const
{
	return m_pDealAI;
}

//	--------------------------------------------------------------------------------
/// Get the object that decides what task the builders should perform
CvBuilderTaskingAI* CvPlayer::GetBuilderTaskingAI() const
{
	return m_pBuilderTaskingAI;
}

//	--------------------------------------------------------------------------------
/// Get the city connection that gives you information about the route connections between cities
CvCityConnections* CvPlayer::GetCityConnections() const
{
	return m_pCityConnections;
}

//	--------------------------------------------------------------------------------
/// Get the player's information about their espionage
CvPlayerEspionage* CvPlayer::GetEspionage() const
{
	return m_pEspionage;
}

//	--------------------------------------------------------------------------------
/// Get the player's espionage AI version
CvEspionageAI* CvPlayer::GetEspionageAI() const
{
	return m_pEspionageAI;
}

//	--------------------------------------------------------------------------------
/// Get the player's information about their trade
CvPlayerTrade* CvPlayer::GetTrade() const
{
	return m_pTrade;
}

//	--------------------------------------------------------------------------------
/// Get the player's trade AI version
CvTradeAI* CvPlayer::GetTradeAI() const
{
	return m_pTradeAI;
}

//	--------------------------------------------------------------------------------
/// Get the player's League AI
CvLeagueAI* CvPlayer::GetLeagueAI() const
{
	return m_pLeagueAI;
}

//	--------------------------------------------------------------------------------
CvPlayerCulture* CvPlayer::GetCulture() const
{
	return m_pCulture;
}

//	--------------------------------------------------------------------------------
CvNotifications* CvPlayer::GetNotifications() const
{
	return m_pNotifications;
}

//	--------------------------------------------------------------------------------
CvTreasury* CvPlayer::GetTreasury() const
{
	return m_pTreasury;
}

//	--------------------------------------------------------------------------------
CvDiplomacyRequests* CvPlayer::GetDiplomacyRequests() const
{
	return m_pDiplomacyRequests;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::HasActiveDiplomacyRequests() const
{
	PlayerTypes ePlayer = GetID();

	// Do I have any?
	CvDiplomacyRequests* pkDiploRequests = GetDiplomacyRequests();
	if(pkDiploRequests && pkDiploRequests->HasActiveRequest())
		return true;

	// Do I have any for others?
	for(int i = 0; i < MAX_PLAYERS; ++i)
	{
		const CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)i);
		if(kPlayer.isAlive())
		{
			pkDiploRequests = kPlayer.GetDiplomacyRequests();
			if(pkDiploRequests)
			{
				if(pkDiploRequests->HasActiveRequestFrom(ePlayer))
					return true;
			}
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
//
// read object from a stream
// used during load
//
void CvPlayer::Read(FDataStream& kStream)
{
	// Init data before load
	reset();

	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;

#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1008)
	{
# endif
		kStream >> m_bIsDisconnected;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bIsDisconnected = false;
	}
# endif
#endif
	kStream >> m_iStartingX;
	kStream >> m_iStartingY;
	kStream >> m_iTotalPopulation;
	kStream >> m_iTotalLand;
	kStream >> m_iTotalLandScored;
	kStream >> m_iJONSCulturePerTurnForFree;
	kStream >> m_iJONSCulturePerTurnFromMinorCivs;
	kStream >> m_iJONSCultureCityModifier;
	kStream >> m_iJONSCulture;
	kStream >> m_iJONSCultureEverGenerated;
	kStream >> m_iCulturePerWonder;
	kStream >> m_iCultureWonderMultiplier;
	kStream >> m_iCulturePerTechResearched;
	kStream >> m_iFaith;
	kStream >> m_iFaithEverGenerated;
	kStream >> m_iHappiness;
	kStream >> m_iUprisingCounter;
	kStream >> m_iExtraHappinessPerLuxury;
	kStream >> m_iUnhappinessFromUnits;
	kStream >> m_iUnhappinessFromUnitsMod;
	kStream >> m_iUnhappinessMod;
	kStream >> m_iCityCountUnhappinessMod;
	kStream >> m_iOccupiedPopulationUnhappinessMod;
	kStream >> m_iCapitalUnhappinessMod;
	kStream >> m_iCityRevoltCounter;
	kStream >> m_iHappinessPerGarrisonedUnitCount;
	kStream >> m_iHappinessPerTradeRouteCount;
	kStream >> m_iHappinessPerXPopulation;
	kStream >> m_iHappinessPerXPolicies;
	if (uiVersion >= 8)
	{
		kStream >> m_iHappinessFromLeagues;
	}
	else
	{
		m_iHappinessFromLeagues = 0;
	}
	kStream >> m_iEspionageModifier;
	kStream >> m_iSpyStartingRank;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1013)
	{
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
		kStream >> m_iNumStolenScience;
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
		kStream >> m_iNumTrainedUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
		kStream >> m_iNumKilledUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
		kStream >> m_iNumLostUnits;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
		kStream >> m_iUnitsDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
		kStream >> m_iUnitsDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
		kStream >> m_iCitiesDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
		kStream >> m_iCitiesDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
		kStream >> m_iNumScientistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
		kStream >> m_iNumEngineersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
		kStream >> m_iNumMerchantsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
		kStream >> m_iNumWritersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
		kStream >> m_iNumArtistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
		kStream >> m_iNumMusiciansTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
		kStream >> m_iNumGeneralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
		kStream >> m_iNumAdmiralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
		kStream >> m_iNumProphetsTotal;
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
		kStream >> m_iProductionGoldFromWonders;
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
		kStream >> m_iNumChops;
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
		kStream >> m_iNumTimesOpenedDemographics;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
		kStream >> m_bMayaBoostScientist;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
		kStream >> m_bMayaBoostEngineers;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
		kStream >> m_bMayaBoostMerchants;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
		kStream >> m_bMayaBoostWriters;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
		kStream >> m_bMayaBoostArtists;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
		kStream >> m_bMayaBoostMusicians;
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
		kStream >> m_iScientistsTotalScienceBoost;
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
		kStream >> m_iEngineersTotalHurryBoost;
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
		kStream >> m_iMerchantsTotalTradeBoost;
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
		kStream >> m_iWritersTotalCultureBoost;
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
		kStream >> m_iMusiciansTotalTourismBoost;
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
		kStream >> m_iNumPopulationLostFromNukes;
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
		kStream >> m_iNumCSQuestsCompleted;
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
		kStream >> m_iNumAlliedCS;
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
		kStream >> m_iTimesEnteredCityScreen;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
		kStream >> m_iNumDiedSpies;
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
		kStream >> m_iNumKilledSpies;
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
		kStream >> m_iFoodFromMinorsTimes100;
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
		kStream >> m_iProductionFromMinorsTimes100;
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
		kStream >> m_iNumUnitsFromMinors;
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
		kStream >> m_iNumCreatedWorldWonders;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
		kStream >> m_iNumGoldSpentOnBuildingBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
		kStream >> m_iNumGoldSpentOnUnitBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
		kStream >> m_iNumGoldSpentOnUgrades;
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
		kStream >> m_iGoldFromKills;
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
		kStream >> m_iCultureFromKills;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
		kStream >> m_iNumGoldSpentOnGPBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
		kStream >> m_iNumGoldSpentOnTilesBuys;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
		kStream >> m_iNumGoldFromPillage;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
		kStream >> m_iNumGoldFromPlunder;
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
		kStream >> m_iNumFaithSpentOnMilitaryUnits;
#endif
# endif
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else if (uiVersion >= 1007)
	{
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
		kStream >> m_iNumStolenScience;
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
		kStream >> m_iNumTrainedUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
		kStream >> m_iNumKilledUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
		kStream >> m_iNumLostUnits;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
		kStream >> m_iUnitsDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
		kStream >> m_iUnitsDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
		kStream >> m_iCitiesDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
		kStream >> m_iCitiesDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
		kStream >> m_iNumScientistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
		kStream >> m_iNumEngineersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
		kStream >> m_iNumMerchantsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
		kStream >> m_iNumWritersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
		kStream >> m_iNumArtistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
		kStream >> m_iNumMusiciansTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
		kStream >> m_iNumGeneralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
		kStream >> m_iNumAdmiralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
		kStream >> m_iNumProphetsTotal;
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
		kStream >> m_iProductionGoldFromWonders;
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
		kStream >> m_iNumChops;
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
		kStream >> m_iNumTimesOpenedDemographics;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
		kStream >> m_bMayaBoostScientist;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
		kStream >> m_bMayaBoostEngineers;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
		kStream >> m_bMayaBoostMerchants;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
		kStream >> m_bMayaBoostWriters;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
		kStream >> m_bMayaBoostArtists;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
		kStream >> m_bMayaBoostMusicians;
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
		kStream >> m_iScientistsTotalScienceBoost;
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
		kStream >> m_iEngineersTotalHurryBoost;
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
		kStream >> m_iMerchantsTotalTradeBoost;
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
		kStream >> m_iWritersTotalCultureBoost;
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
		kStream >> m_iMusiciansTotalTourismBoost;
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
		kStream >> m_iNumPopulationLostFromNukes;
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
		kStream >> m_iNumCSQuestsCompleted;
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
		kStream >> m_iNumAlliedCS;
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
		kStream >> m_iTimesEnteredCityScreen;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
		kStream >> m_iNumDiedSpies;
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
		kStream >> m_iNumKilledSpies;
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
		kStream >> m_iFoodFromMinorsTimes100;
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
		kStream >> m_iProductionFromMinorsTimes100;
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
		kStream >> m_iNumUnitsFromMinors;
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
		kStream >> m_iNumCreatedWorldWonders;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
		kStream >> m_iNumGoldSpentOnBuildingBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
		kStream >> m_iNumGoldSpentOnUnitBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
		kStream >> m_iNumGoldSpentOnUgrades;
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
		m_iGoldFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
		m_iCultureFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
		m_iNumGoldSpentOnGPBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
		m_iNumGoldSpentOnTilesBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
		m_iNumGoldFromPillage = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
		m_iNumGoldFromPlunder = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
		m_iNumFaithSpentOnMilitaryUnits = 0;
#endif
	}
	else if (uiVersion >= 1003)
	{
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
		kStream >> m_iNumStolenScience;
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
		kStream >> m_iNumTrainedUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
		kStream >> m_iNumKilledUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
		kStream >> m_iNumLostUnits;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
		kStream >> m_iUnitsDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
		kStream >> m_iUnitsDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
		kStream >> m_iCitiesDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
		kStream >> m_iCitiesDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
		kStream >> m_iNumScientistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
		kStream >> m_iNumEngineersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
		kStream >> m_iNumMerchantsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
		kStream >> m_iNumWritersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
		kStream >> m_iNumArtistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
		kStream >> m_iNumMusiciansTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
		kStream >> m_iNumGeneralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
		kStream >> m_iNumAdmiralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
		kStream >> m_iNumProphetsTotal;
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
		kStream >> m_iProductionGoldFromWonders;
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
		kStream >> m_iNumChops;
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
		kStream >> m_iNumTimesOpenedDemographics;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
		kStream >> m_bMayaBoostScientist;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
		kStream >> m_bMayaBoostEngineers;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
		kStream >> m_bMayaBoostMerchants;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
		kStream >> m_bMayaBoostWriters;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
		kStream >> m_bMayaBoostArtists;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
		kStream >> m_bMayaBoostMusicians;
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
		kStream >> m_iScientistsTotalScienceBoost;
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
		kStream >> m_iEngineersTotalHurryBoost;
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
		kStream >> m_iMerchantsTotalTradeBoost;
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
		kStream >> m_iWritersTotalCultureBoost;
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
		kStream >> m_iMusiciansTotalTourismBoost;
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
		kStream >> m_iNumPopulationLostFromNukes;
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
		kStream >> m_iNumCSQuestsCompleted;
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
		kStream >> m_iNumAlliedCS;
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
		kStream >> m_iTimesEnteredCityScreen;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
		m_iNumDiedSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
		m_iNumKilledSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
		m_iFoodFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
		m_iProductionFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
		m_iNumUnitsFromMinors = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
		m_iNumCreatedWorldWonders = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
		m_iNumGoldSpentOnBuildingBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
		m_iNumGoldSpentOnUnitBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
		m_iNumGoldSpentOnUgrades = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
		m_iGoldFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
		m_iCultureFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
		m_iNumGoldSpentOnGPBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
		m_iNumGoldSpentOnTilesBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
		m_iNumGoldFromPillage = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
		m_iNumGoldFromPlunder = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
		m_iNumFaithSpentOnMilitaryUnits = 0;
#endif
	}
	else if (uiVersion == 1002)
	{
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
		kStream >> m_iNumStolenScience;
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
		kStream >> m_iNumTrainedUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
		kStream >> m_iNumKilledUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
		kStream >> m_iNumLostUnits;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
		kStream >> m_iUnitsDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
		kStream >> m_iUnitsDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
		kStream >> m_iCitiesDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
		kStream >> m_iCitiesDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
		kStream >> m_iNumScientistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
		kStream >> m_iNumEngineersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
		kStream >> m_iNumMerchantsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
		kStream >> m_iNumWritersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
		kStream >> m_iNumArtistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
		kStream >> m_iNumMusiciansTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
		kStream >> m_iNumGeneralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
		kStream >> m_iNumAdmiralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
		kStream >> m_iNumProphetsTotal;
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
		kStream >> m_iProductionGoldFromWonders;
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
		kStream >> m_iNumChops;
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
		kStream >> m_iNumTimesOpenedDemographics;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
		kStream >> m_bMayaBoostScientist;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
		kStream >> m_bMayaBoostEngineers;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
		kStream >> m_bMayaBoostMerchants;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
		kStream >> m_bMayaBoostWriters;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
		kStream >> m_bMayaBoostArtists;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
		kStream >> m_bMayaBoostMusicians;
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
		m_iScientistsTotalScienceBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
		m_iEngineersTotalHurryBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
		m_iMerchantsTotalTradeBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
		m_iWritersTotalCultureBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
		m_iMusiciansTotalTourismBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
		m_iNumPopulationLostFromNukes = 0;
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
		m_iNumCSQuestsCompleted = 0;
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
		m_iNumAlliedCS = 0;
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
		m_iTimesEnteredCityScreen = 0;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
		m_iNumDiedSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
		m_iNumKilledSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
		m_iFoodFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
		m_iProductionFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
		m_iNumUnitsFromMinors = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
		m_iNumCreatedWorldWonders = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
		m_iNumGoldSpentOnBuildingBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
		m_iNumGoldSpentOnUnitBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
		m_iNumGoldSpentOnUgrades = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
		m_iGoldFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
		m_iCultureFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
		m_iNumGoldSpentOnGPBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
		m_iNumGoldSpentOnTilesBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
		m_iNumGoldFromPillage = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
		m_iNumGoldFromPlunder = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
		m_iNumFaithSpentOnMilitaryUnits = 0;
#endif
	}
	else if (uiVersion == 1001)
	{
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
		kStream >> m_iNumStolenScience;
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
		kStream >> m_iNumTrainedUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
		kStream >> m_iNumKilledUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
		kStream >> m_iNumLostUnits;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
		kStream >> m_iUnitsDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
		kStream >> m_iUnitsDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
		kStream >> m_iCitiesDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
		kStream >> m_iCitiesDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
		kStream >> m_iNumScientistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
		kStream >> m_iNumEngineersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
		kStream >> m_iNumMerchantsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
		kStream >> m_iNumWritersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
		kStream >> m_iNumArtistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
		kStream >> m_iNumMusiciansTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
		kStream >> m_iNumGeneralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
		kStream >> m_iNumAdmiralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
		kStream >> m_iNumProphetsTotal;
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
		kStream >> m_iProductionGoldFromWonders;
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
		kStream >> m_iNumChops;
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
		kStream >> m_iNumTimesOpenedDemographics;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
		m_bMayaBoostScientist = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
		m_bMayaBoostEngineers = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
		m_bMayaBoostMerchants = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
		m_bMayaBoostWriters = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
		m_bMayaBoostArtists = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
		m_bMayaBoostMusicians = 0;
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
		m_iScientistsTotalScienceBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
		m_iEngineersTotalHurryBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
		m_iMerchantsTotalTradeBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
		m_iWritersTotalCultureBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
		m_iMusiciansTotalTourismBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
		m_iNumPopulationLostFromNukes = 0;
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
		m_iNumCSQuestsCompleted = 0;
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
		m_iNumAlliedCS = 0;
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
		m_iTimesEnteredCityScreen = 0;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
		m_iNumDiedSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
		m_iNumKilledSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
		m_iFoodFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
		m_iProductionFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
		m_iNumUnitsFromMinors = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
		m_iNumCreatedWorldWonders = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
		m_iNumGoldSpentOnBuildingBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
		m_iNumGoldSpentOnUnitBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
		m_iNumGoldSpentOnUgrades = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
		m_iGoldFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
		m_iCultureFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
		m_iNumGoldSpentOnGPBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
		m_iNumGoldSpentOnTilesBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
		m_iNumGoldFromPillage = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
		m_iNumGoldFromPlunder = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
		m_iNumFaithSpentOnMilitaryUnits = 0;
#endif
	}
	else
	{
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
		m_iNumStolenScience = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
		m_iNumTrainedUnits = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
		m_iNumKilledUnits = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
		m_iNumLostUnits = 0;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
		m_iUnitsDamageDealt = 0;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
		m_iUnitsDamageTaken = 0;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
		m_iCitiesDamageDealt = 0;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
		m_iCitiesDamageTaken = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
		m_iNumScientistsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
		m_iNumEngineersTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
		m_iNumMerchantsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
		m_iNumWritersTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
		m_iNumArtistsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
		m_iNumMusiciansTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
		m_iNumGeneralsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
		m_iNumAdmiralsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
		m_iNumProphetsTotal = 0;
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
		m_iProductionGoldFromWonders = 0;
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
		m_iNumChops = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
		m_iNumTimesOpenedDemographics = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
		m_bMayaBoostScientist = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
		m_bMayaBoostEngineers = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
		m_bMayaBoostMerchants = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
		m_bMayaBoostWriters = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
		m_bMayaBoostArtists = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
		m_bMayaBoostMusicians = 0;
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
		m_iScientistsTotalScienceBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
		m_iEngineersTotalHurryBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
		m_iMerchantsTotalTradeBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
		m_iWritersTotalCultureBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
		m_iMusiciansTotalTourismBoost = 0;
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
		m_iNumPopulationLostFromNukes = 0;
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
		m_iNumCSQuestsCompleted = 0;
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
		m_iNumAlliedCS = 0;
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
		m_iTimesEnteredCityScreen = 0;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
		m_iNumDiedSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
		m_iNumKilledSpies = 0;
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
		m_iFoodFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
		m_iProductionFromMinorsTimes100 = 0;
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
		m_iNumUnitsFromMinors = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
		m_iNumCreatedWorldWonders = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
		m_iNumGoldSpentOnBuildingBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
		m_iNumGoldSpentOnUnitBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
		m_iNumGoldSpentOnUgrades = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
		m_iGoldFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
		m_iCultureFromKills = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
		m_iNumGoldSpentOnGPBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
		m_iNumGoldSpentOnTilesBuys = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
		m_iNumGoldFromPillage = 0;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
		m_iNumGoldFromPlunder = 0;
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
		m_iNumFaithSpentOnMilitaryUnits = 0;
#endif
	}
#endif
	if (uiVersion >= 14)
	{
		kStream >> m_iExtraLeagueVotes;
	}
	else
	{
		m_iExtraLeagueVotes = 0;
	}
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iMaxExtraVotesFromMinors;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iMaxExtraVotesFromMinors = 0;
	}
#endif
#endif
#ifdef POLICY_EXTRA_VOTES
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iPolicyExtraVotes;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iPolicyExtraVotes = 0;
	}
#endif
#endif
#ifdef POLICY_DO_TECH_FROM_CITY_CONQ
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iPolicyTechFromCityConquer;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iPolicyTechFromCityConquer = 0;
	}
#endif
#endif
#ifdef POLICY_NO_CULTURE_SPECIALIST_FOOD
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iNoCultureSpecialistFood;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iNoCultureSpecialistFood = 0;
	}
#endif
#endif
#ifdef POLICY_MINORS_GIFT_UNITS
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iMinorsGiftUnits;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iMinorsGiftUnits = 0;
	}
#endif
#endif
#ifdef POLICY_NO_CARGO_PILLAGE
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iNoCargoPillage;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iNoCargoPillage = 0;
	}
#endif
#endif
#ifdef POLICY_GREAT_WORK_HAPPINESS
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iGreatWorkHappiness;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iGreatWorkHappiness = 0;
	}
#endif
#endif
#ifdef POLICY_SCIENCE_PER_X_FOLLOWERS
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iSciencePerXFollowers;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iSciencePerXFollowers = 0;
	}
#endif
#endif
#ifdef POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iNoDifferentIdeologiesTourismMod;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iNoDifferentIdeologiesTourismMod = 0;
	}
#endif
#endif
#ifdef POLICY_GLOBAL_POP_CHANGE
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iGlobalPopChange;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iGlobalPopChange = 0;
	}
#endif
#endif
#ifdef POLICY_GREAT_WORK_TOURISM_CHANGES
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
#endif
		kStream >> m_iGreatWorkTourismChanges;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iGreatWorkTourismChanges = 0;
	}
#endif
#endif
#ifdef POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1011)
	{
#endif
		kStream >> m_iCityScienceSquaredModPerXPop;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iCityScienceSquaredModPerXPop = 0;
	}
#endif
#endif
	kStream >> m_iSpecialPolicyBuildingHappiness;
	kStream >> m_iWoundedUnitDamageMod;
	kStream >> m_iUnitUpgradeCostMod;
	kStream >> m_iBarbarianCombatBonus;
	kStream >> m_iAlwaysSeeBarbCampsCount;
	kStream >> m_iHappinessFromBuildings;
	kStream >> m_iHappinessPerCity;
	kStream >> m_iAdvancedStartPoints;
	kStream >> m_iAttackBonusTurns;
	if (uiVersion >= 9)
	{
		kStream >> m_iCultureBonusTurns;
		kStream >> m_iTourismBonusTurns;
	}
	else
	{
		m_iCultureBonusTurns = 0;
		m_iTourismBonusTurns = 0;
	}
	kStream >> m_iGoldenAgeProgressMeter;
	kStream >> m_iGoldenAgeMeterMod;
	kStream >> m_iNumGoldenAges;
	kStream >> m_iGoldenAgeTurns;
#ifdef TAJ_MAHAL_STARTS_GA_NEXT_TURN
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1004)
	{
# endif
		kStream >> m_iBuildingGoldenAgeTurns;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iBuildingGoldenAgeTurns = false;
	}
# endif
#endif
	kStream >> m_iNumUnitGoldenAges;
	kStream >> m_iStrikeTurns;
	kStream >> m_iGoldenAgeModifier;
	kStream >> m_iGreatPeopleCreated;
	kStream >> m_iGreatGeneralsCreated;
	kStream >> m_iGreatAdmiralsCreated;
	kStream >> m_iGreatWritersCreated;
	kStream >> m_iGreatArtistsCreated;
	kStream >> m_iGreatMusiciansCreated;
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
# endif
		kStream >> m_bHasUsedDharma;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bHasUsedDharma = false;
	}
# endif
#endif
#ifdef UNDERGROUND_SECT_REWORK
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1005)
	{
# endif
		kStream >> m_bHasUsedUndergroundSect;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bHasUsedUndergroundSect = false;
	}
# endif
#endif
#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1004)
	{
# endif
		kStream >> m_bHasUsedMissionaryZeal;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bHasUsedMissionaryZeal = false;
	}
# endif
#endif
#ifdef UNITY_OF_PROPHETS_EXTRA_PROPHETS
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
# endif
		kStream >> m_bHasUsedUnityProphets;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bHasUsedUnityProphets = false;
	}
# endif
#endif
#ifdef GODDESS_LOVE_FREE_WORKER
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1004)
	{
# endif
		kStream >> m_bHasUsedGoddessLove;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bHasUsedGoddessLove = false;
	}
# endif
#endif
#ifdef GOD_SEA_FREE_WORK_BOAT
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1005)
	{
# endif
		kStream >> m_bHasUsedGodSea;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bHasUsedGodSea = false;
	}
# endif
#endif
#ifdef FREE_GREAT_PERSON
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
# endif
		kStream >> m_iGreatProphetsCreated;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iGreatProphetsCreated = 0;
	}
# endif
#endif
#if defined SEPARATE_GREAT_PEOPLE || defined SWEDEN_UA_REWORK
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
# endif
		kStream >> m_iGreatScientistsCreated;
		kStream >> m_iGreatEngineersCreated;
		kStream >> m_iGreatMerchantsCreated;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iGreatScientistsCreated = 0;
		m_iGreatEngineersCreated = 0;
		m_iGreatMerchantsCreated = 0;
	}
# endif
#endif
#ifdef SEPARATE_MERCHANTS
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= BUMP_SAVE_VERSION_PLAYER)
	{
# endif
		kStream >> m_iGreatMerchantsCreated;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iGreatMerchantsCreated = 0;
	}
# endif
#endif
	kStream >> m_iMerchantsFromFaith;
	kStream >> m_iScientistsFromFaith;
	kStream >> m_iWritersFromFaith;
	kStream >> m_iArtistsFromFaith;
	kStream >> m_iMusiciansFromFaith;
	kStream >> m_iGeneralsFromFaith;
	kStream >> m_iAdmiralsFromFaith;
	kStream >> m_iEngineersFromFaith;
#ifdef BELIEF_TO_GLORY_OF_GOD_ONE_GP_OF_EACH_TYPE
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
# endif
		kStream >> m_bMerchantsFromFaith;
		kStream >> m_bScientistsFromFaith;
		kStream >> m_bWritersFromFaith;
		kStream >> m_bArtistsFromFaith;
		kStream >> m_bMusiciansFromFaith;
		kStream >> m_bGeneralsFromFaith;
		kStream >> m_bAdmiralsFromFaith;
		kStream >> m_bEngineersFromFaith;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bMerchantsFromFaith = false;
		m_bScientistsFromFaith = false;
		m_bWritersFromFaith = false;
		m_bArtistsFromFaith = false;
		m_bMusiciansFromFaith = false;
		m_bGeneralsFromFaith = false;
		m_bAdmiralsFromFaith = false;
		m_bEngineersFromFaith = false;
	}
# endif
#endif
	kStream >> m_iGreatPeopleThresholdModifier;
	kStream >> m_iGreatGeneralsThresholdModifier;
	kStream >> m_iGreatAdmiralsThresholdModifier;
	kStream >> m_iGreatGeneralCombatBonus;
	kStream >> m_iAnarchyNumTurns;
	kStream >> m_iPolicyCostModifier;
	kStream >> m_iGreatPeopleRateModifier;
	kStream >> m_iGreatPeopleRateModFromBldgs;
	kStream >> m_iGreatGeneralRateModifier;
	kStream >> m_iGreatGeneralRateModFromBldgs;
	kStream >> m_iDomesticGreatGeneralRateModifier;
	kStream >> m_iDomesticGreatGeneralRateModFromBldgs;
	kStream >> m_iGreatAdmiralRateModifier;
	kStream >> m_iGreatWriterRateModifier;
	kStream >> m_iGreatArtistRateModifier;
	kStream >> m_iGreatMusicianRateModifier;
	kStream >> m_iGreatMerchantRateModifier;
	kStream >> m_iGreatScientistRateModifier;
	if (uiVersion >= 10)
	{
		kStream >> m_iGreatScientistBeakerModifier;
	}
	else
	{
		m_iGreatScientistBeakerModifier = 0;
	}
	if (uiVersion >= 13)
	{
		kStream >> m_iGreatEngineerRateModifier;
	}
	else
	{
		m_iGreatEngineerRateModifier = 0;
	}
	kStream >> m_iGreatPersonExpendGold;
	kStream >> m_iMaxGlobalBuildingProductionModifier;
	kStream >> m_iMaxTeamBuildingProductionModifier;
	kStream >> m_iMaxPlayerBuildingProductionModifier;
	kStream >> m_iFreeExperience;
	kStream >> m_iFreeExperienceFromBldgs;
	kStream >> m_iFreeExperienceFromMinors;
	kStream >> m_iFeatureProductionModifier;
	kStream >> m_iWorkerSpeedModifier;
	kStream >> m_iImprovementCostModifier;
	kStream >> m_iImprovementUpgradeRateModifier;
	kStream >> m_iSpecialistProductionModifier;
	kStream >> m_iMilitaryProductionModifier;
	kStream >> m_iSpaceProductionModifier;
	kStream >> m_iCityDefenseModifier;
	kStream >> m_iUnitFortificationModifier;
	kStream >> m_iUnitBaseHealModifier;
	kStream >> m_iWonderProductionModifier;
	kStream >> m_iSettlerProductionModifier;
	kStream >> m_iCapitalSettlerProductionModifier;
	kStream >> m_iUnitProductionMaintenanceMod;
	kStream >> m_iPolicyCostBuildingModifier;
	kStream >> m_iPolicyCostMinorCivModifier;
	kStream >> m_iInfluenceSpreadModifier;
	if (uiVersion >= 15)
	{
		kStream >> m_iExtraVotesPerDiplomat;
	}
	else
	{
		m_iExtraVotesPerDiplomat = 0;
	}
	kStream >> m_iNumNukeUnits;
	kStream >> m_iNumOutsideUnits;
	kStream >> m_iBaseFreeUnits;
	kStream >> m_iBaseFreeMilitaryUnits;
	kStream >> m_iFreeUnitsPopulationPercent;
	kStream >> m_iFreeMilitaryUnitsPopulationPercent;
	kStream >> m_iGoldPerUnit;
	kStream >> m_iGoldPerMilitaryUnit;
	kStream >> m_iRouteGoldMaintenanceMod;
	kStream >> m_iBuildingGoldMaintenanceMod;
	kStream >> m_iUnitGoldMaintenanceMod;
	kStream >> m_iUnitSupplyMod;
	kStream >> m_iExtraUnitCost;
	kStream >> m_iNumMilitaryUnits;
	kStream >> m_iHappyPerMilitaryUnit;
	kStream >> m_iHappinessToCulture;
	kStream >> m_iHappinessToScience;
	kStream >> m_iHalfSpecialistUnhappinessCount;
	kStream >> m_iHalfSpecialistFoodCount;
	kStream >> m_iMilitaryFoodProductionCount;
	kStream >> m_iGoldenAgeCultureBonusDisabledCount;
	kStream >> m_iSecondReligionPantheonCount;
	if (uiVersion >= 2)
	{
		kStream >> m_iEnablesSSPartHurryCount;
	}
	else
	{
		m_iEnablesSSPartHurryCount = 0;
	}
	if (uiVersion >= 3)
	{
		kStream >> m_iEnablesSSPartPurchaseCount;
	}
	else
	{
		m_iEnablesSSPartPurchaseCount = 0;
	}
	kStream >> m_iConscriptCount;
	kStream >> m_iMaxConscript;
	kStream >> m_iHighestUnitLevel;
	kStream >> m_iOverflowResearch;
	kStream >> m_iExpModifier;
	kStream >> m_iExpInBorderModifier;
	kStream >> m_iLevelExperienceModifier;
	kStream >> m_iMinorQuestFriendshipMod;
	kStream >> m_iMinorGoldFriendshipMod;
	kStream >> m_iMinorFriendshipMinimum;
	kStream >> m_iMinorFriendshipDecayMod;
	kStream >> m_iMinorScienceAlliesCount;
	kStream >> m_iMinorResourceBonusCount;
	if (uiVersion >= 12)
	{
		kStream >> m_iAbleToAnnexCityStatesCount;
	}
	else
	{
		m_iAbleToAnnexCityStatesCount = 0;
	}
	kStream >> m_iConversionTimer;
	kStream >> m_iCapitalCityID;
	kStream >> m_iCitiesLost;
	kStream >> m_iMilitaryMight;
	kStream >> m_iEconomicMight;
	kStream >> m_iTurnMightRecomputed;
	kStream >> m_iNewCityExtraPopulation;
	kStream >> m_iFreeFoodBox;
	kStream >> m_iScenarioScore1;
	kStream >> m_iScenarioScore2;
	kStream >> m_iScenarioScore3;
	kStream >> m_iScoreFromFutureTech;
	kStream >> m_iScenarioScore4;
	kStream >> m_iCombatExperience;
	kStream >> m_iNavalCombatExperience;
	kStream >> m_iLifetimeCombatExperience;
	kStream >> m_iBorderObstacleCount;
	kStream >> m_iNextOperationID;
	kStream >> m_iCostNextPolicy;
	kStream >> m_iNumBuilders;
	kStream >> m_iMaxNumBuilders;
	kStream >> m_iCityStrengthMod;
	kStream >> m_iCityGrowthMod;
	kStream >> m_iCapitalGrowthMod;
	kStream >> m_iNumPlotsBought;
	kStream >> m_iPlotGoldCostMod;
	kStream >> m_iPlotCultureCostModifier;
	kStream >> m_iPlotCultureExponentModifier;
	kStream >> m_iNumCitiesPolicyCostDiscount;
	kStream >> m_iGarrisonFreeMaintenanceCount;
	kStream >> m_iGarrisonedCityRangeStrikeModifier;
#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
# endif
		kStream >> m_ppaaiBuildingScecialistCountChange;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		Firaxis::Array< int, NUM_SPECILIST_TYPES > specialist;
		for (unsigned int j = 0; j < NUM_SPECILIST_TYPES; ++j)
		{
			specialist[j] = 0;
		}

		m_ppaaiBuildingScecialistCountChange.clear();
		m_ppaaiBuildingScecialistCountChange.resize(GC.getNumBuildingInfos());
		for (unsigned int i = 0; i < m_ppaaiBuildingScecialistCountChange.size(); ++i)
		{
			m_ppaaiBuildingScecialistCountChange.setAt(i, specialist);
		}
	}
# endif
#endif
	kStream >> m_iNumCitiesFreeCultureBuilding;
	kStream >> m_iNumCitiesFreeFoodBuilding;
#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1005)
	{
# endif
		kStream >> m_iNumCitiesFreeDevensiveBuilding;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iNumCitiesFreeDevensiveBuilding = 0;
	}
# endif
#endif
	kStream >> m_iUnitPurchaseCostModifier;
	kStream >> m_iAllFeatureProduction;
	kStream >> m_iCityDistanceHighwaterMark;
	kStream >> m_iOriginalCapitalX;
	kStream >> m_iOriginalCapitalY;
	kStream >> m_iNumWonders;
	kStream >> m_iNumPolicies;
	kStream >> m_iNumGreatPeople;
	kStream >> m_iCityConnectionHappiness;
	kStream >> m_iHolyCityID;
	kStream >> m_iTurnsSinceSettledLastCity;
	kStream >> m_iNumNaturalWondersDiscoveredInArea;
	kStream >> m_iStrategicResourceMod;
	kStream >> m_iSpecialistCultureChange;
	kStream >> m_iGreatPeopleSpawnCounter;
	kStream >> m_iFreeTechCount;
	kStream >> m_iMedianTechPercentage;
	kStream >> m_iNumFreePolicies;
	kStream >> m_iNumFreePoliciesEver;
	if (uiVersion >= 16)
	{
		kStream >> m_iNumFreeTenets;
	}
	else
	{
		m_iNumFreeTenets = 0;
	}
	kStream >> m_iNumFreeGreatPeople;
	kStream >> m_iNumMayaBoosts;
	kStream >> m_iNumFaithGreatPeople;
	kStream >> m_iNumArchaeologyChoices;

	int temp;
	kStream >> temp;
	m_eFaithPurchaseType = (FaithPurchaseTypes)temp;
	kStream >> m_iFaithPurchaseIndex;

	if (uiVersion >= 6)
	{
		kStream >> m_iMaxEffectiveCities;
	}
	else
	{
		m_iMaxEffectiveCities = 1;
	}

	kStream >> m_iLastSliceMoved;
	kStream >> m_bHasBetrayedMinorCiv;
#ifdef CAN_BUILD_OU_AND_NIA_ONLY_ONCE
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
# endif
		kStream >> m_bOxfordUniversityWasEverBuilt;
		kStream >> m_bNationalIntelligenceAgencyWasEverBuilt;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bOxfordUniversityWasEverBuilt = false;
		m_bNationalIntelligenceAgencyWasEverBuilt = false;
	}
# endif
#endif
#ifdef PENALTY_FOR_DELAYING_POLICIES
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1006)
	{
# endif
		kStream >> m_bIsDelayedPolicy;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bIsDelayedPolicy = false;
	}
# endif
#endif
	kStream >> m_bAlive;
	kStream >> m_bEverAlive;
	kStream >> m_bBeingResurrected;
	kStream >> m_bTurnActive;
	kStream >> m_bAutoMoves;
	kStream >> m_bEndTurn;
	kStream >> m_bDynamicTurnsSimultMode;
	kStream >> m_bPbemNewTurn;
	kStream >> m_bExtendedGame;
	kStream >> m_bFoundedFirstCity;
	kStream >> m_iNumCitiesFounded;
	kStream >> m_bStrike;
	kStream >> m_bCramped;
	kStream >> m_bLostCapital;
	kStream >> m_eConqueror;
	kStream >> m_bHasAdoptedStateReligion;
	kStream >> m_bAlliesGreatPersonBiasApplied;
	kStream >> m_eID;
	kStream >> m_ePersonalityType;
	kStream >> m_aiCityYieldChange;
	kStream >> m_aiCoastalCityYieldChange;
	kStream >> m_aiCapitalYieldChange;
	kStream >> m_aiCapitalYieldPerPopChange;
	kStream >> m_aiSeaPlotYield;
	kStream >> m_aiYieldRateModifier;
	kStream >> m_aiCapitalYieldRateModifier;

	if (uiVersion >= 4)
	{
		kStream >> m_aiGreatWorkYieldChange;
	}
	else
	{
		m_aiGreatWorkYieldChange.clear();
		m_aiGreatWorkYieldChange.resize(NUM_YIELD_TYPES, 0);
	}
	kStream >> m_aiExtraYieldThreshold;
	kStream >> m_aiSpecialistExtraYield;
#ifdef POLICY_GOLDEN_AGE_YIELD_MOD
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
# endif
		kStream >> m_aiGoldenAgeYieldModifier;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_aiGoldenAgeYieldModifier.clear();
		m_aiGoldenAgeYieldModifier.resize(NUM_YIELD_TYPES, 0);
	}
# endif
#endif
#ifdef POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1010)
	{
# endif
		kStream >> m_paiPlotExtraYieldFromTradeRoute;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_paiPlotExtraYieldFromTradeRoute.clear();
		m_paiPlotExtraYieldFromTradeRoute.resize(NUM_YIELD_TYPES, 0);
	}
# endif
#endif
	kStream >> m_aiProximityToPlayer;
	kStream >> m_aiResearchAgreementCounter;
	if (uiVersion >= 5)
	{
		kStream >> m_aiIncomingUnitTypes;
		kStream >> m_aiIncomingUnitCountdowns;
	}
	else
	{
		std::vector<int> aiOldIncomingUnitTypes;
		kStream >> aiOldIncomingUnitTypes;
		m_aiIncomingUnitTypes.clear();
		m_aiIncomingUnitTypes.resize(MAX_PLAYERS, NO_UNIT);

		// m_aiIncomingUnitCountdowns was not serialized before...curious
		m_aiIncomingUnitCountdowns.clear();
		m_aiIncomingUnitCountdowns.resize(MAX_PLAYERS, -1);
	}
	
	kStream >> m_aiMinorFriendshipAnchors;
	if (uiVersion >= 7)
	{
		kStream >> m_aiSiphonLuxuryCount;
	}
	else
	{
		m_aiSiphonLuxuryCount.clear();
		m_aiSiphonLuxuryCount.resize(MAX_PLAYERS, 0);
	}
	kStream >> m_strReligionKey;
	kStream >> m_strScriptData;

	CvAssertMsg((0 < GC.getNumResourceInfos()), "GC.getNumResourceInfos() is not greater than zero but it is expected to be in CvPlayer::read");
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiNumResourceUsed.dirtyGet());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiNumResourceTotal.dirtyGet());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiResourceGiftedToMinors.dirtyGet());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiResourceExport.dirtyGet());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiResourceImport.dirtyGet());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiResourceFromMinors.dirtyGet());
	if (uiVersion >= 7)
	{
		CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiResourcesSiphoned.dirtyGet());
	}
	else
	{
		m_paiResourcesSiphoned.clear();
		m_paiResourcesSiphoned.resize(GC.getNumResourceInfos(), 0);
	}

	kStream >> m_paiImprovementCount;

	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiFreeBuildingCount.dirtyGet());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiFreePromotionCount.dirtyGet());

	kStream >> m_paiUnitCombatProductionModifiers;
	kStream >> m_paiUnitCombatFreeExperiences;
	kStream >> m_paiUnitClassCount;
	kStream >> m_paiUnitClassMaking;
	kStream >> m_paiBuildingClassCount;
	kStream >> m_paiBuildingClassMaking;
	kStream >> m_paiProjectMaking;
	kStream >> m_paiHurryCount;
	kStream >> m_paiHurryModifier;

	CvAssertMsg((0 < GC.getNumTechInfos()), "GC.getNumTechInfos() is not greater than zero but it is expected to be in CvPlayer::read");

	kStream >> m_pabLoyalMember;

	kStream >> m_pabGetsScienceFromPlayer;

#ifdef CS_ALLYING_WAR_RESCTRICTION
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1009)
	{
# endif
		kStream >> m_ppaaiTurnCSWarAllowing;
		kStream >> m_ppaafTimeCSWarAllowing;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		Firaxis::Array< int, MAX_MINOR_CIVS > turn;
		for (unsigned int j = 0; j < MAX_MINOR_CIVS; ++j)
		{
			turn[j] = -1;
		}
		m_ppaaiTurnCSWarAllowing.clear();
		m_ppaaiTurnCSWarAllowing.resize(MAX_MAJOR_CIVS);
		for (unsigned int i = 0; i < m_ppaaiTurnCSWarAllowing.size(); ++i)
		{
			m_ppaaiTurnCSWarAllowing.setAt(i, turn);
		}

		Firaxis::Array< float, MAX_MINOR_CIVS > time;
		for (unsigned int j = 0; j < MAX_MINOR_CIVS; ++j)
		{
			time[j] = 0.f;
		}
		m_ppaafTimeCSWarAllowing.clear();
		m_ppaafTimeCSWarAllowing.resize(MAX_MAJOR_CIVS);
		for (unsigned int i = 0; i < m_ppaafTimeCSWarAllowing.size(); ++i)
		{
			m_ppaafTimeCSWarAllowing.setAt(i, time);
		}
	}
# endif
#endif

#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1012)
	{
# endif
		kStream >> m_ppaaiYieldForEachBuildingInEmpire;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		Firaxis::Array<int, NUM_YIELD_TYPES> yield;
		for (unsigned int j = 0; j < NUM_YIELD_TYPES; ++j)
		{
			yield[j] = 0;
		}
		m_ppaaiYieldForEachBuildingInEmpire.clear();
		m_ppaaiYieldForEachBuildingInEmpire.resize(GC.getNumBuildingInfos());
		for (unsigned int i = 0; i < m_ppaaiYieldForEachBuildingInEmpire.size(); ++i)
		{
			m_ppaaiYieldForEachBuildingInEmpire.setAt(i, yield);
		}
	}
#endif
#endif

#ifdef POLICY_ALLOWS_GP_BUYS_FOR_GOLD
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1012)
	{
# endif
		kStream >> m_iNumGoldPurchasedGreatPerson;
		kStream >> m_bGoldWriter;
		kStream >> m_bGoldArtist;
		kStream >> m_bGoldMusician;
		kStream >> m_bGoldScientist;
		kStream >> m_bGoldEngineer;
		kStream >> m_bGoldMerchant;
		kStream >> m_bGoldGeneral;
		kStream >> m_bGoldAdmiral;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iNumGoldPurchasedGreatPerson = 0;
		m_bGoldWriter = false;
		m_bGoldArtist = false;
		m_bGoldMusician = false;
		m_bGoldScientist = false;
		m_bGoldEngineer = false;
		m_bGoldMerchant = false;
		m_bGoldGeneral = false;
		m_bGoldAdmiral = false;
	}
# endif
#endif
#ifdef POLICY_SPY_DETECTION
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1014)
	{
# endif
	kStream >> m_iSpyDetection;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iSpyDetection = 0;
	}
# endif
#endif
#ifdef BUILDING_BORDER_TRANSITION_OBSTACLE
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1015)
	{
# endif
		kStream >> m_iBorderTransitionObstacleCount;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iBorderTransitionObstacleCount = 0;
	}
# endif
#endif

	m_pPlayerPolicies->Read(kStream);
	m_pEconomicAI->Read(kStream);
	m_pCitySpecializationAI->Read(kStream);
	m_pWonderProductionAI->Read(kStream);
	m_pMilitaryAI->Read(kStream);
	m_pGrandStrategyAI->Read(kStream);
	m_pDiplomacyAI->Read(kStream);
	m_pReligions->Read(kStream);
	m_pReligionAI->Read(kStream);
	m_pPlayerTechs->Read(kStream);
	m_pFlavorManager->Read(kStream);
	m_pTacticalAI->Read(kStream);
	m_pHomelandAI->Read(kStream);
	m_pMinorCivAI->Read(kStream);
	m_pDealAI->Read(kStream);
	m_pBuilderTaskingAI->Read(kStream);
	m_pCityConnections->Read(kStream);
	m_pDangerPlots->Read(kStream);
	m_pTraits->Read(kStream);
	kStream >> *m_pEspionage;
	kStream >> *m_pEspionageAI;
	kStream >> *m_pTrade;
	kStream >> *m_pTradeAI;
	m_pLeagueAI->Read(kStream);
	kStream >> *m_pCulture;

	bool bReadNotifications;
	kStream >> bReadNotifications;
	if(bReadNotifications)
	{
		m_pNotifications = FNEW(CvNotifications, c_eCiv5GameplayDLL, 0);
		m_pNotifications->Init(GetID());
		m_pNotifications->Read(kStream);
	}
	m_pTreasury->Read(kStream);

	// If this is a real player, hook up the player-level flavor recipients
	if(GetID() != NO_PLAYER)
	{
		SlotStatus s = CvPreGame::slotStatus(GetID());
		if((s == SS_TAKEN || s == SS_COMPUTER) && !isBarbarian())
		{
			m_pFlavorManager->AddFlavorRecipient(m_pPlayerTechs,        false /*bPropogateFlavors*/);
			m_pFlavorManager->AddFlavorRecipient(m_pPlayerPolicies,     false /*bPropogateFlavors*/);
			m_pFlavorManager->AddFlavorRecipient(m_pWonderProductionAI, false /*bPropogateFlavors*/);
		}
	}

	kStream >> m_ppaaiSpecialistExtraYield;
	kStream >> m_ppaaiImprovementYieldChange;

	kStream >> m_UnitCycle;
	kStream >> m_researchQueue;

	kStream >> m_bEverPoppedGoody;
	kStream >> m_bEverTrainedBuilder;
	kStream >> m_eEndTurnBlockingType;
	kStream >> m_iEndTurnBlockingNotificationIndex;

	kStream >> m_cityNames;

	kStream >> m_cities;
	kStream >> m_units;
	kStream >> m_armyAIs;

	{
		m_AIOperations.clear();
		uint iSize;
		int iID;
		int iOperationType;
		kStream >> iSize;
		for(uint i = 0; i < iSize; i++)
		{
			kStream >> iID;
			kStream >> iOperationType;
			CvAIOperation* pThisOperation = CvAIOperation::CreateOperation((AIOperationTypes)iOperationType, m_eID);
			pThisOperation->Read(kStream);
			m_AIOperations.insert(std::make_pair(pThisOperation->GetID(), pThisOperation));
		}
	}

	if (uiVersion <= 10)
	{
		// Unused popup queue
		int iSize;
		kStream >> iSize;
		CvAssert(iSize == 0);
	}

	kStream >> m_ReplayDataSets;
	kStream >> m_ReplayDataSetValues;

	kStream >> m_aVote;
	kStream >> m_aUnitExtraCosts;

	// reading plot values
	{
		m_aiPlots.clear();
		m_aiPlots.push_back_copy(-1, GC.getMap().numPlots());

		// trying to cut down how big saves are
		int iSize;
		kStream >> iSize;
		for(int i = 0; i < iSize; i++)
		{
			kStream >> m_aiPlots[i];
		}
	}

	if(!isBarbarian())
	{
		// Get the NetID from the initialization structure
		setNetID(gDLL->getAssignedNetworkID(GetID()));
	}

	kStream >> m_iPopRushHurryCount;
	kStream >> m_iTotalImprovementsBuilt;

	m_bfEverConqueredBy.ClearAll();
	int iSize;
	kStream >> iSize;
	for(int i = 0; i < iSize; i++)
	{
		bool bValue;
		kStream >> bValue;
		if(bValue)
		{
			m_bfEverConqueredBy.SetBit(i);
		}
	}

	kStream >> m_strEmbarkedGraphicOverride;
	m_kPlayerAchievements.Read(kStream);

	if(GetID() < MAX_MAJOR_CIVS)
	{
		if(!m_pDiplomacyRequests)
			m_pDiplomacyRequests = FNEW(CvDiplomacyRequests, c_eCiv5GameplayDLL, 0);
		else
			m_pDiplomacyRequests->Uninit();

		m_pDiplomacyRequests->Init(GetID());
		//m_pDiplomacyRequests->Read(kStream);
	}
#ifdef AUTOSAVE_FIX_PREVENT_TURN_SKIP

	if (CvPreGame::gameType() == GAME_NETWORK_MULTIPLAYER && m_bAlive)
	{
		// Set active turn for actual players, not the AI!
		m_bEndTurn = false;
	}
#endif

	if(m_bTurnActive)
		GC.getGame().changeNumGameTurnActive(1, std::string("setTurnActive() [loading save game] for player ") + getName());

}

//	--------------------------------------------------------------------------------
//
// save object to a stream
// used during save
//
void CvPlayer::Write(FDataStream& kStream) const
{
	//Save version number.  THIS MUST BE FIRST!!
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	int iVersion = BUMP_SAVE_VERSION_PLAYER;
	kStream << iVersion;
#else
	kStream << g_CurrentCvPlayerVersion;
#endif

#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
	kStream << m_bIsDisconnected;
#endif
	kStream << m_iStartingX;
	kStream << m_iStartingY;
	kStream << m_iTotalPopulation;
	kStream << m_iTotalLand;
	kStream << m_iTotalLandScored;
	kStream << m_iJONSCulturePerTurnForFree;
	kStream << m_iJONSCulturePerTurnFromMinorCivs;
	kStream << m_iJONSCultureCityModifier;
	kStream << m_iJONSCulture;
	kStream << m_iJONSCultureEverGenerated;
	kStream << m_iCulturePerWonder;
	kStream << m_iCultureWonderMultiplier;
	kStream << m_iCulturePerTechResearched;
	kStream << m_iFaith;
	kStream << m_iFaithEverGenerated;
	kStream << m_iHappiness;
	kStream << m_iUprisingCounter;
	kStream << m_iExtraHappinessPerLuxury;
	kStream << m_iUnhappinessFromUnits;
	kStream << m_iUnhappinessFromUnitsMod;
	kStream << m_iUnhappinessMod;
	kStream << m_iCityCountUnhappinessMod;
	kStream << m_iOccupiedPopulationUnhappinessMod;
	kStream << m_iCapitalUnhappinessMod;
	kStream << m_iCityRevoltCounter;
	kStream << m_iHappinessPerGarrisonedUnitCount;
	kStream << m_iHappinessPerTradeRouteCount;
	kStream << m_iHappinessPerXPopulation;
	kStream << m_iHappinessPerXPolicies;
	kStream << m_iHappinessFromLeagues;
	kStream << m_iEspionageModifier;
	kStream << m_iSpyStartingRank;
#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
	kStream << m_iNumStolenScience;
#endif
#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
	kStream << m_iNumTrainedUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
	kStream << m_iNumKilledUnits;
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
	kStream << m_iNumLostUnits;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
	kStream << m_iUnitsDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
	kStream << m_iUnitsDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
	kStream << m_iCitiesDamageDealt;
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
	kStream << m_iCitiesDamageTaken;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
	kStream << m_iNumScientistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
	kStream << m_iNumEngineersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
	kStream << m_iNumMerchantsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
	kStream << m_iNumWritersTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
	kStream << m_iNumArtistsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
	kStream << m_iNumMusiciansTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
	kStream << m_iNumGeneralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
	kStream << m_iNumAdmiralsTotal;
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
	kStream << m_iNumProphetsTotal;
#endif
#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
	kStream << m_iProductionGoldFromWonders;
#endif
#ifdef EG_REPLAYDATASET_TOTALCHOPS
	kStream << m_iNumChops;
#endif
#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
	kStream << m_iNumTimesOpenedDemographics;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
	kStream << m_bMayaBoostScientist;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
	kStream << m_bMayaBoostEngineers;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
	kStream << m_bMayaBoostMerchants;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
	kStream << m_bMayaBoostWriters;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
	kStream << m_bMayaBoostArtists;
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
	kStream << m_bMayaBoostMusicians;
#endif
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
	kStream << m_iScientistsTotalScienceBoost;
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
	kStream << m_iEngineersTotalHurryBoost;
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
	kStream << m_iMerchantsTotalTradeBoost;
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
	kStream << m_iWritersTotalCultureBoost;
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
	kStream << m_iMusiciansTotalTourismBoost;
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
	kStream << m_iNumPopulationLostFromNukes;
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
	kStream << m_iNumCSQuestsCompleted;
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
	kStream << m_iNumAlliedCS;
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
	kStream << m_iTimesEnteredCityScreen;
#endif
#ifdef EG_REPLAYDATASET_DIEDSPIES
	kStream << m_iNumDiedSpies;
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
	kStream << m_iNumKilledSpies;
#endif
#ifdef EG_REPLAYDATASET_FOODFROMCS
	kStream << m_iFoodFromMinorsTimes100;
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
	kStream << m_iProductionFromMinorsTimes100;
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
	kStream << m_iNumUnitsFromMinors;
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
	kStream << m_iNumCreatedWorldWonders;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
	kStream << m_iNumGoldSpentOnBuildingBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
	kStream << m_iNumGoldSpentOnUnitBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
	kStream << m_iNumGoldSpentOnUgrades;
#endif
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
	kStream << m_iGoldFromKills;
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
	kStream << m_iCultureFromKills;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
	kStream << m_iNumGoldSpentOnGPBuys;
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
	kStream << m_iNumGoldSpentOnTilesBuys;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
	kStream << m_iNumGoldFromPillage;
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
	kStream << m_iNumGoldFromPlunder;
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
	kStream << m_iNumFaithSpentOnMilitaryUnits;
#endif
	kStream << m_iExtraLeagueVotes;
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
	kStream << m_iMaxExtraVotesFromMinors;
#endif
#ifdef POLICY_EXTRA_VOTES
	kStream << m_iPolicyExtraVotes;
#endif
#ifdef POLICY_DO_TECH_FROM_CITY_CONQ
	kStream << m_iPolicyTechFromCityConquer;
#endif
#ifdef POLICY_NO_CULTURE_SPECIALIST_FOOD
	kStream << m_iNoCultureSpecialistFood;
#endif
#ifdef POLICY_MINORS_GIFT_UNITS
	kStream << m_iMinorsGiftUnits;
#endif
#ifdef POLICY_NO_CARGO_PILLAGE
	kStream << m_iNoCargoPillage;
#endif
#ifdef POLICY_GREAT_WORK_HAPPINESS
	kStream << m_iGreatWorkHappiness;
#endif
#ifdef POLICY_SCIENCE_PER_X_FOLLOWERS
	kStream << m_iSciencePerXFollowers;
#endif
#ifdef POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD
	kStream << m_iNoDifferentIdeologiesTourismMod;
#endif
#ifdef POLICY_GLOBAL_POP_CHANGE
	kStream << m_iGlobalPopChange;
#endif
#ifdef POLICY_GREAT_WORK_TOURISM_CHANGES
	kStream << m_iGreatWorkTourismChanges;
#endif
#ifdef POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP
	kStream << m_iCityScienceSquaredModPerXPop;
#endif
	kStream << m_iSpecialPolicyBuildingHappiness;
	kStream << m_iWoundedUnitDamageMod;
	kStream << m_iUnitUpgradeCostMod;
	kStream << m_iBarbarianCombatBonus;
	kStream << m_iAlwaysSeeBarbCampsCount;
	kStream << m_iHappinessFromBuildings;
	kStream << m_iHappinessPerCity;
	kStream << m_iAdvancedStartPoints;
	kStream << m_iAttackBonusTurns;
	kStream << m_iCultureBonusTurns;
	kStream << m_iTourismBonusTurns;
	kStream << m_iGoldenAgeProgressMeter;
	kStream << m_iGoldenAgeMeterMod;
	kStream << m_iNumGoldenAges;
	kStream << m_iGoldenAgeTurns;
#ifdef TAJ_MAHAL_STARTS_GA_NEXT_TURN
	kStream << m_iBuildingGoldenAgeTurns;
#endif
	kStream << m_iNumUnitGoldenAges;
	kStream << m_iStrikeTurns;
	kStream << m_iGoldenAgeModifier;
	kStream << m_iGreatPeopleCreated;
	kStream << m_iGreatGeneralsCreated;
	kStream << m_iGreatAdmiralsCreated;
	kStream << m_iGreatWritersCreated;
	kStream << m_iGreatArtistsCreated;
	kStream << m_iGreatMusiciansCreated;
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	kStream << m_bHasUsedDharma;
#endif
#ifdef UNDERGROUND_SECT_REWORK
	kStream << m_bHasUsedUndergroundSect;
#endif
#ifdef MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
	kStream << m_bHasUsedMissionaryZeal;
#endif
#ifdef UNITY_OF_PROPHETS_EXTRA_PROPHETS
	kStream << m_bHasUsedUnityProphets;
#endif
#ifdef GODDESS_LOVE_FREE_WORKER
	kStream << m_bHasUsedGoddessLove;
#endif
#ifdef GOD_SEA_FREE_WORK_BOAT
	kStream << m_bHasUsedGodSea;
#endif
#ifdef FREE_GREAT_PERSON
	kStream << m_iGreatProphetsCreated;
#endif
#if defined SEPARATE_GREAT_PEOPLE || defined SWEDEN_UA_REWORK
	kStream << m_iGreatScientistsCreated;
	kStream << m_iGreatEngineersCreated;
	kStream << m_iGreatMerchantsCreated;
#endif
#ifdef SEPARATE_MERCHANTS
	kStream << m_iGreatMerchantsCreated;
#endif
	kStream << m_iMerchantsFromFaith;
	kStream << m_iScientistsFromFaith;
	kStream << m_iWritersFromFaith;
	kStream << m_iArtistsFromFaith;
	kStream << m_iMusiciansFromFaith;
	kStream << m_iGeneralsFromFaith;
	kStream << m_iAdmiralsFromFaith;
	kStream << m_iEngineersFromFaith;
#ifdef BELIEF_TO_GLORY_OF_GOD_ONE_GP_OF_EACH_TYPE
	kStream << m_bMerchantsFromFaith;
	kStream << m_bScientistsFromFaith;
	kStream << m_bWritersFromFaith;
	kStream << m_bArtistsFromFaith;
	kStream << m_bMusiciansFromFaith;
	kStream << m_bGeneralsFromFaith;
	kStream << m_bAdmiralsFromFaith;
	kStream << m_bEngineersFromFaith;
#endif
	kStream << m_iGreatPeopleThresholdModifier;
	kStream << m_iGreatGeneralsThresholdModifier;
	kStream << m_iGreatAdmiralsThresholdModifier;
	kStream << m_iGreatGeneralCombatBonus;
	kStream << m_iAnarchyNumTurns;
	kStream << m_iPolicyCostModifier;
	kStream << m_iGreatPeopleRateModifier;
	kStream << m_iGreatPeopleRateModFromBldgs;
	kStream << m_iGreatGeneralRateModifier;
	kStream << m_iGreatGeneralRateModFromBldgs;
	kStream << m_iDomesticGreatGeneralRateModifier;
	kStream << m_iDomesticGreatGeneralRateModFromBldgs;
	kStream << m_iGreatAdmiralRateModifier;
	kStream << m_iGreatWriterRateModifier;
	kStream << m_iGreatArtistRateModifier;
	kStream << m_iGreatMusicianRateModifier;
	kStream << m_iGreatMerchantRateModifier;
	kStream << m_iGreatScientistRateModifier;
	kStream << m_iGreatScientistBeakerModifier;
	kStream << m_iGreatEngineerRateModifier;
	kStream << m_iGreatPersonExpendGold;
	kStream << m_iMaxGlobalBuildingProductionModifier;
	kStream << m_iMaxTeamBuildingProductionModifier;
	kStream << m_iMaxPlayerBuildingProductionModifier;
	kStream << m_iFreeExperience;
	kStream << m_iFreeExperienceFromBldgs;
	kStream << m_iFreeExperienceFromMinors;
	kStream << m_iFeatureProductionModifier;
	kStream << m_iWorkerSpeedModifier;
	kStream << m_iImprovementCostModifier;
	kStream << m_iImprovementUpgradeRateModifier;
	kStream << m_iSpecialistProductionModifier;
	kStream << m_iMilitaryProductionModifier;
	kStream << m_iSpaceProductionModifier;
	kStream << m_iCityDefenseModifier;
	kStream << m_iUnitFortificationModifier; // Version 45
	kStream << m_iUnitBaseHealModifier; // Version 46
	kStream << m_iWonderProductionModifier;
	kStream << m_iSettlerProductionModifier;
	kStream << m_iCapitalSettlerProductionModifier;	// Version 11
	kStream << m_iUnitProductionMaintenanceMod;
	kStream << m_iPolicyCostBuildingModifier;		// Added in version 3
	kStream << m_iPolicyCostMinorCivModifier;
	kStream << m_iInfluenceSpreadModifier;
	kStream << m_iExtraVotesPerDiplomat;
	kStream << m_iNumNukeUnits;
	kStream << m_iNumOutsideUnits;
	kStream << m_iBaseFreeUnits;
	kStream << m_iBaseFreeMilitaryUnits;
	kStream << m_iFreeUnitsPopulationPercent;
	kStream << m_iFreeMilitaryUnitsPopulationPercent;
	kStream << m_iGoldPerUnit;
	kStream << m_iGoldPerMilitaryUnit;
	kStream << m_iRouteGoldMaintenanceMod;
	kStream << m_iBuildingGoldMaintenanceMod;
	kStream << m_iUnitGoldMaintenanceMod;
	kStream << m_iUnitSupplyMod;
	kStream << m_iExtraUnitCost;
	kStream << m_iNumMilitaryUnits;
	kStream << m_iHappyPerMilitaryUnit;
	kStream << m_iHappinessToCulture;
	kStream << m_iHappinessToScience;
	kStream << m_iHalfSpecialistUnhappinessCount;
	kStream << m_iHalfSpecialistFoodCount;
	kStream << m_iMilitaryFoodProductionCount;
	kStream << m_iGoldenAgeCultureBonusDisabledCount;
	kStream << m_iSecondReligionPantheonCount;
	kStream << m_iEnablesSSPartHurryCount;
	kStream << m_iEnablesSSPartPurchaseCount;
	kStream << m_iConscriptCount;
	kStream << m_iMaxConscript;
	kStream << m_iHighestUnitLevel;
	kStream << m_iOverflowResearch;
	kStream << m_iExpModifier;
	kStream << m_iExpInBorderModifier;
	kStream << m_iLevelExperienceModifier;
	kStream << m_iMinorQuestFriendshipMod;
	kStream << m_iMinorGoldFriendshipMod;
	kStream << m_iMinorFriendshipMinimum;
	kStream << m_iMinorFriendshipDecayMod;
	kStream << m_iMinorScienceAlliesCount;
	kStream << m_iMinorResourceBonusCount;
	kStream << m_iAbleToAnnexCityStatesCount;
	kStream << m_iConversionTimer;
	kStream << m_iCapitalCityID;
	kStream << m_iCitiesLost;
	kStream << m_iMilitaryMight;
	kStream << m_iEconomicMight;
	kStream << m_iTurnMightRecomputed;
	kStream << m_iNewCityExtraPopulation;
	kStream << m_iFreeFoodBox;
	kStream << m_iScenarioScore1;
	kStream << m_iScenarioScore2;
	kStream << m_iScenarioScore3;
	kStream << m_iScoreFromFutureTech;
	kStream << m_iScenarioScore4;
	kStream << m_iCombatExperience;
	kStream << m_iNavalCombatExperience;
	kStream << m_iLifetimeCombatExperience;
	kStream << m_iBorderObstacleCount;
	kStream << m_iNextOperationID;
	kStream << m_iCostNextPolicy;
	kStream << m_iNumBuilders;
	kStream << m_iMaxNumBuilders;
	kStream << m_iCityStrengthMod;
	kStream << m_iCityGrowthMod;
	kStream << m_iCapitalGrowthMod;
	kStream << m_iNumPlotsBought;
	kStream << m_iPlotGoldCostMod;
	kStream << m_iPlotCultureCostModifier;
	kStream << m_iPlotCultureExponentModifier;
	kStream << m_iNumCitiesPolicyCostDiscount;
	kStream << m_iGarrisonFreeMaintenanceCount;
	kStream << m_iGarrisonedCityRangeStrikeModifier;
#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
	kStream << m_ppaaiBuildingScecialistCountChange;
#endif
	kStream << m_iNumCitiesFreeCultureBuilding;
	kStream << m_iNumCitiesFreeFoodBuilding;
#ifdef POLICY_FREE_DEFENSIVE_BUILDINGS
	kStream << m_iNumCitiesFreeDevensiveBuilding;
#endif
	kStream << m_iUnitPurchaseCostModifier;
	kStream << m_iAllFeatureProduction;
	kStream << m_iCityDistanceHighwaterMark;
	kStream << m_iOriginalCapitalX;
	kStream << m_iOriginalCapitalY;
	kStream << m_iNumWonders;
	kStream << m_iNumPolicies;
	kStream << m_iNumGreatPeople;
	kStream << m_iCityConnectionHappiness;
	kStream << m_iHolyCityID;
	kStream << m_iTurnsSinceSettledLastCity;
	kStream << m_iNumNaturalWondersDiscoveredInArea;
	kStream << m_iStrategicResourceMod;
	kStream << m_iSpecialistCultureChange;
	kStream << m_iGreatPeopleSpawnCounter;
	kStream << m_iFreeTechCount;
	kStream << m_iMedianTechPercentage;
	kStream << m_iNumFreePolicies;
	kStream << m_iNumFreePoliciesEver;
	kStream << m_iNumFreeTenets;
	kStream << m_iNumFreeGreatPeople;
	kStream << m_iNumMayaBoosts;
	kStream << m_iNumFaithGreatPeople;
	kStream << m_iNumArchaeologyChoices;
	kStream << m_eFaithPurchaseType;
	kStream << m_iFaithPurchaseIndex;
	kStream << m_iMaxEffectiveCities;
	kStream << m_iLastSliceMoved;

	kStream << m_bHasBetrayedMinorCiv;
#ifdef CAN_BUILD_OU_AND_NIA_ONLY_ONCE
	kStream << m_bOxfordUniversityWasEverBuilt;
	kStream << m_bNationalIntelligenceAgencyWasEverBuilt;
#endif
#ifdef PENALTY_FOR_DELAYING_POLICIES
	kStream << m_bIsDelayedPolicy;
#endif
	kStream << m_bAlive;
	kStream << m_bEverAlive;
	kStream << m_bBeingResurrected;
	kStream << m_bTurnActive;
	kStream << m_bAutoMoves;
	kStream << m_bEndTurn;
	kStream << m_bDynamicTurnsSimultMode;
	kStream << static_cast<bool>(m_bPbemNewTurn && GC.getGame().isPbem());
	kStream << m_bExtendedGame;
	kStream << m_bFoundedFirstCity;
	kStream << m_iNumCitiesFounded; // Added V30
	kStream << m_bStrike;
	kStream << m_bCramped;
	kStream << m_bLostCapital;
	kStream << m_eConqueror;
	kStream << m_bHasAdoptedStateReligion;
	kStream << m_bAlliesGreatPersonBiasApplied;

	kStream << m_eID;
	kStream << m_ePersonalityType;

	kStream << m_aiCityYieldChange;
	kStream << m_aiCoastalCityYieldChange;
	kStream << m_aiCapitalYieldChange;
	kStream << m_aiCapitalYieldPerPopChange;
	kStream << m_aiSeaPlotYield;
	kStream << m_aiYieldRateModifier;
	kStream << m_aiCapitalYieldRateModifier;
	kStream << m_aiGreatWorkYieldChange;
	kStream << m_aiExtraYieldThreshold;
	kStream << m_aiSpecialistExtraYield;
#ifdef POLICY_GOLDEN_AGE_YIELD_MOD
	kStream << m_aiGoldenAgeYieldModifier;
#endif
#ifdef POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
	kStream << m_paiPlotExtraYieldFromTradeRoute;
#endif
	kStream << m_aiProximityToPlayer;
	kStream << m_aiResearchAgreementCounter;   // Added in Version 2
	kStream << m_aiIncomingUnitTypes;
	kStream << m_aiIncomingUnitCountdowns;
	kStream << m_aiMinorFriendshipAnchors; // Version 38
	kStream << m_aiSiphonLuxuryCount;

	//kStream << m_abOptions;

	kStream << m_strReligionKey;
	kStream << m_strScriptData;

	CvAssertMsg((0 < GC.getNumResourceInfos()), "GC.getNumResourceInfos() is not greater than zero but an array is being allocated in CvPlayer::write");
	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiNumResourceUsed);
	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiNumResourceTotal);
	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiResourceGiftedToMinors);
	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiResourceExport);
	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiResourceImport);
	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiResourceFromMinors);
	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiResourcesSiphoned);

	kStream << m_paiImprovementCount;

	CvInfosSerializationHelper::WriteHashedDataArray<BuildingTypes, int>(kStream, m_paiFreeBuildingCount);

	CvInfosSerializationHelper::WriteHashedDataArray<PromotionTypes, int>(kStream, m_paiFreePromotionCount);

	kStream << m_paiUnitCombatProductionModifiers;
	kStream << m_paiUnitCombatFreeExperiences;
	kStream << m_paiUnitClassCount;
	kStream << m_paiUnitClassMaking;
	kStream << m_paiBuildingClassCount;
	kStream << m_paiBuildingClassMaking;
	kStream << m_paiProjectMaking;
	kStream << m_paiHurryCount;
	kStream << m_paiHurryModifier;


	CvAssertMsg((0 < GC.getNumTechInfos()), "GC.getNumTechInfos() is not greater than zero but it is expected to be in CvPlayer::write");

	kStream << m_pabLoyalMember;

	kStream << m_pabGetsScienceFromPlayer;

#ifdef CS_ALLYING_WAR_RESCTRICTION
	kStream << m_ppaaiTurnCSWarAllowing;
	kStream << m_ppaafTimeCSWarAllowing;
#endif

#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
	kStream << m_ppaaiYieldForEachBuildingInEmpire;
#endif
#ifdef POLICY_ALLOWS_GP_BUYS_FOR_GOLD
	kStream << m_iNumGoldPurchasedGreatPerson;
	kStream << m_bGoldWriter;
	kStream << m_bGoldArtist;
	kStream << m_bGoldMusician;
	kStream << m_bGoldScientist;
	kStream << m_bGoldEngineer;
	kStream << m_bGoldMerchant;
	kStream << m_bGoldGeneral;
	kStream << m_bGoldAdmiral;
#endif
#ifdef POLICY_SPY_DETECTION
	kStream << m_iSpyDetection;
#endif
#ifdef BUILDING_BORDER_TRANSITION_OBSTACLE
	kStream << m_iBorderTransitionObstacleCount;
#endif

	m_pPlayerPolicies->Write(kStream);
	m_pEconomicAI->Write(kStream);
	m_pCitySpecializationAI->Write(kStream);
	m_pWonderProductionAI->Write(kStream);
	m_pMilitaryAI->Write(kStream);
	m_pGrandStrategyAI->Write(kStream);
	m_pDiplomacyAI->Write(kStream);
	m_pReligions->Write(kStream);
	m_pReligionAI->Write(kStream);
	m_pPlayerTechs->Write(kStream);
	m_pFlavorManager->Write(kStream);
	m_pTacticalAI->Write(kStream);
	m_pHomelandAI->Write(kStream);
	m_pMinorCivAI->Write(kStream);
	m_pDealAI->Write(kStream);
	m_pBuilderTaskingAI->Write(kStream);
	m_pCityConnections->Write(kStream);
	m_pDangerPlots->Write(kStream);
	m_pTraits->Write(kStream);
	kStream << *m_pEspionage;
	kStream << *m_pEspionageAI;
	kStream << *m_pTrade;
	kStream << *m_pTradeAI;
	m_pLeagueAI->Write(kStream);
	kStream << *m_pCulture;

	if(m_pNotifications)
	{
		kStream << true;
		m_pNotifications->Write(kStream);
	}
	else
	{
		kStream << false;
	}
	m_pTreasury->Write(kStream);

	kStream << m_ppaaiSpecialistExtraYield;
	kStream << m_ppaaiImprovementYieldChange;

	kStream << m_UnitCycle;
	kStream << m_researchQueue;

	kStream << m_bEverPoppedGoody;
	kStream << m_bEverTrainedBuilder;
	kStream << m_eEndTurnBlockingType;
	kStream << m_iEndTurnBlockingNotificationIndex;

	kStream << m_cityNames;
	kStream << m_cities;
	kStream << m_units;
	kStream << m_armyAIs;

	{
		uint iSize = m_AIOperations.size();
		kStream << iSize;
		std::map<int, CvAIOperation*>::const_iterator it;
		for(it = m_AIOperations.begin(); it != m_AIOperations.end(); ++it)
		{
			kStream << it->first;
			CvAIOperation* pThisOperation = it->second;
			kStream << pThisOperation->GetOperationType();
			pThisOperation->Write(kStream);
		}
	}

	kStream << m_ReplayDataSets;
	kStream << m_ReplayDataSetValues;

	kStream << m_aVote;
	kStream << m_aUnitExtraCosts;

	// writing out plot values
	{
		// trying to cut down how big saves are
		int iSize = -1;
		for(int i = m_aiPlots.size() - 1; i >= 0; i--)
		{
			if(m_aiPlots[i] != -1)
			{
				iSize = i + 1;
				break;
			}
		}

		if(iSize < 0)
		{
			iSize = 0;
		}

		kStream << iSize;
		for(int i = 0; i < iSize; i++)
		{
			kStream << m_aiPlots[i];
		}
	}

	kStream << m_iPopRushHurryCount;
	kStream << m_iTotalImprovementsBuilt;

	// writing out
	{
		int iSize = MAX_PLAYERS;
		kStream << iSize;
		for(int i = 0; i < iSize; i++)
		{
			bool bValue = m_bfEverConqueredBy.GetBit(i);
			kStream << bValue;
		}
	}

	kStream << m_strEmbarkedGraphicOverride;

	m_kPlayerAchievements.Write(kStream);
}

//	--------------------------------------------------------------------------------
void CvPlayer::createGreatGeneral(UnitTypes eGreatPersonUnit, int iX, int iY)
{
	CvUnit* pGreatPeopleUnit = initUnit(eGreatPersonUnit, iX, iY);
	if(NULL == pGreatPeopleUnit)
	{
		CvAssert(false);
		return;
	}

	ChangeNumGreatPeople(1);

	incrementGreatGeneralsCreated();
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
	ChangeNumGeneralsTotal(1);
#endif

	changeGreatGeneralsThresholdModifier(/*50*/ GC.getGREAT_GENERALS_THRESHOLD_INCREASE() * ((getGreatGeneralsCreated() / 10) + 1));

	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if(GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
		{
			GET_PLAYER((PlayerTypes)iI).changeGreatGeneralsThresholdModifier(/*50*/ GC.getGREAT_GENERALS_THRESHOLD_INCREASE_TEAM() * ((getGreatPeopleCreated() / 10) + 1));
		}
	}

	CvPlot* pPlot = GC.getMap().plot(iX, iY);

	//Achievements and Stats
	if(pGreatPeopleUnit->isHuman() && !GC.getGame().isGameMultiPlayer())
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_GREATGENERALS);
		const char* strLeader = GET_PLAYER(pGreatPeopleUnit->getOwner()).getLeaderTypeKey();
		if(strLeader && strcmp(strLeader, "LEADER_WU_ZETIAN") == 0)
		{
			gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_SUNTZU);
		}

		CvAchievementUnlocker::Check_PSG();
	}

	// Notification
	if(GetNotifications())
	{
		Localization::String strText = Localization::Lookup("TXT_KEY_NOTIFICATION_GREAT_PERSON_ACTIVE_PLAYER");
		Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_GREAT_PERSON");
		GetNotifications()->Add(NOTIFICATION_GREAT_PERSON_ACTIVE_PLAYER, strText.toUTF8(), strSummary.toUTF8(), pPlot->getX(), pPlot->getY(), eGreatPersonUnit);
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::createGreatAdmiral(UnitTypes eGreatPersonUnit, int iX, int iY)
{
	CvUnit* pGreatPeopleUnit = initUnit(eGreatPersonUnit, iX, iY);
	if(NULL == pGreatPeopleUnit)
	{
		CvAssert(false);
		return;
	}

	ChangeNumGreatPeople(1);
	CvPlot *pSpawnPlot = GetGreatAdmiralSpawnPlot(pGreatPeopleUnit);
	if (pGreatPeopleUnit->plot() != pSpawnPlot)
	{
		pGreatPeopleUnit->setXY(pSpawnPlot->getX(), pSpawnPlot->getY());
	}

	incrementGreatAdmiralsCreated();
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
	ChangeNumAdmiralsTotal(1);
#endif
	changeGreatAdmiralsThresholdModifier(/*50*/ GC.getGREAT_GENERALS_THRESHOLD_INCREASE() * ((getGreatAdmiralsCreated() / 10) + 1));

	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if(GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
		{
			GET_PLAYER((PlayerTypes)iI).changeGreatAdmiralsThresholdModifier(/*50*/ GC.getGREAT_GENERALS_THRESHOLD_INCREASE_TEAM() * ((getGreatPeopleCreated() / 10) + 1));
		}
	}

	CvPlot* pPlot = GC.getMap().plot(iX, iY);

	// Notification
	if(GetNotifications())
	{
		Localization::String strText = Localization::Lookup("TXT_KEY_NOTIFICATION_GREAT_PERSON_ACTIVE_PLAYER");
		Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_GREAT_PERSON");
		GetNotifications()->Add(NOTIFICATION_GREAT_PERSON_ACTIVE_PLAYER, strText.toUTF8(), strSummary.toUTF8(), pPlot->getX(), pPlot->getY(), eGreatPersonUnit);
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::launch(VictoryTypes eVictory)
{
	CvTeam& kTeam = GET_TEAM(getTeam());

	if(!kTeam.canLaunch(eVictory))
	{
		return;
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::getUnitExtraCost(UnitClassTypes eUnitClass) const
{
	for(std::vector< std::pair<UnitClassTypes, int> >::const_iterator it = m_aUnitExtraCosts.begin(); it != m_aUnitExtraCosts.end(); ++it)
	{
		if((*it).first == eUnitClass)
		{
			return ((*it).second);
		}
	}

	return 0;
}

//	--------------------------------------------------------------------------------
void CvPlayer::setUnitExtraCost(UnitClassTypes eUnitClass, int iCost)
{
	for(std::vector< std::pair<UnitClassTypes, int> >::iterator it = m_aUnitExtraCosts.begin(); it != m_aUnitExtraCosts.end(); ++it)
	{
		if((*it).first == eUnitClass)
		{
			if(0 == iCost)
			{
				m_aUnitExtraCosts.erase(it);
			}
			else
			{
				(*it).second = iCost;
			}
			return;
		}
	}

	if(0 != iCost)
	{
		m_aUnitExtraCosts.push_back(std::make_pair(eUnitClass, iCost));
	}
}

// CACHE: cache frequently used values
///////////////////////////////////////

//	--------------------------------------------------------------------------------
void CvPlayer::invalidatePopulationRankCache()
{
	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->invalidatePopulationRankCache();
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::invalidateYieldRankCache(YieldTypes)
{
	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->invalidateYieldRankCache();
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::doUpdateCacheOnTurn()
{

}

//	--------------------------------------------------------------------------------
PlayerTypes CvPlayer::pickConqueredCityOwner(const CvCity& kCity) const
{
	PlayerTypes eBestPlayer = kCity.getOriginalOwner();

	if(NO_PLAYER != eBestPlayer)
	{
		CvPlayer& kBestPlayer = GET_PLAYER(eBestPlayer);

		if(kBestPlayer.getTeam() == getTeam())
		{
			return eBestPlayer;
		}
	}

	return GetID();
}

//	--------------------------------------------------------------------------------
bool CvPlayer::canStealTech(PlayerTypes eTarget, TechTypes eTech) const
{
	if(GET_TEAM(GET_PLAYER(eTarget).getTeam()).GetTeamTechs()->HasTech(eTech))
	{
#ifdef BUILD_STEALABLE_TECH_LIST_ONCE_PER_TURN
		if(GetPlayerTechs()->CanResearch(eTech) && GetEspionage()->IsTechStealable(eTarget, eTech) && GetEspionage()->m_aiNumTechsToStealList[eTarget] > 0)
#else
		if(GetPlayerTechs()->CanResearch(eTech))
#endif
		{
			return true;
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::canSpyDestroyUnit(PlayerTypes, CvUnit& kUnit) const
{
	if(kUnit.getTeam() == getTeam())
	{
		return false;
	}

	if(kUnit.getUnitInfo().GetProductionCost() <= 0)
	{
		return false;
	}

	if(!kUnit.plot()->isVisible(getTeam()))
	{
		return false;
	}

	return true;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::canSpyBribeUnit(PlayerTypes eTarget, CvUnit& kUnit) const
{
	if(!canSpyDestroyUnit(eTarget, kUnit))
	{
		return false;
	}

	// Can't buy units when at war
	if(kUnit.isEnemy(getTeam()))
	{
		return false;
	}

	IDInfo* pUnitNode = kUnit.plot()->headUnitNode();

	while(pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(*pUnitNode);
		pUnitNode = kUnit.plot()->nextUnitNode(pUnitNode);

		if(NULL != pLoopUnit && pLoopUnit != &kUnit)
		{
			if(pLoopUnit->isEnemy(getTeam()))
			{
				// If we buy the unit, we will be on the same plot as an enemy unit! Not good.
				return false;
			}
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::canSpyDestroyBuilding(PlayerTypes, BuildingTypes eBuilding) const
{
	CvBuildingEntry* pkBuilding = GC.getBuildingInfo(eBuilding);
	if(pkBuilding)
	{
		if(pkBuilding->GetProductionCost() <= 0)
		{
			return false;
		}

		if(::isLimitedWonderClass(pkBuilding->GetBuildingClassInfo()))
		{
			return false;
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::canSpyDestroyProject(PlayerTypes eTarget, ProjectTypes eProject) const
{
	CvProjectEntry& kProject = *GC.getProjectInfo(eProject);
	if(kProject.GetProductionCost() <= 0)
	{
		return false;
	}

	if(GET_TEAM(GET_PLAYER(eTarget).getTeam()).getProjectCount(eProject) <= 0)
	{
		return false;
	}

	if(::isWorldProject(eProject))
	{
		return false;
	}

	if(!kProject.IsSpaceship())
	{
		return false;
	}
	else
	{
		VictoryTypes eVictory = (VictoryTypes)kProject.GetVictoryPrereq();
		if(NO_VICTORY != eVictory)
		{
			// Can't destroy spaceship components if we have already launched
			if(GET_TEAM(GET_PLAYER(eTarget).getTeam()).getVictoryCountdown(eVictory) >= 0)
			{
				return false;
			}
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getNewCityProductionValue() const
{
	if(GC.getSETTLER_PRODUCTION_SPEED() != 0)
	{
		return GC.getSETTLER_PRODUCTION_SPEED();
	}

	int iValue = 0;
	for(int iJ = 0; iJ < GC.getNumBuildingClassInfos(); iJ++)
	{
		const BuildingClassTypes eBuildingClass = static_cast<BuildingClassTypes>(iJ);
		CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
		if(pkBuildingClassInfo)
		{
			const BuildingTypes eBuilding = ((BuildingTypes)(getCivilizationInfo().getCivilizationBuildings(iJ)));
			if(NO_BUILDING != eBuilding)
			{
				CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
				if(pkBuildingInfo)
				{
					if(pkBuildingInfo->GetFreeStartEra() != NO_ERA)
					{
						if(GC.getGame().getStartEra() >= pkBuildingInfo->GetFreeStartEra())
						{
							iValue += (100 * getProductionNeeded(eBuilding)) / std::max(1, 100 + getProductionModifier(eBuilding));
						}
					}
				}
			}
		}
	}

	iValue *= 100 + GC.getNEW_CITY_BUILDING_VALUE_MODIFIER();
	iValue /= 100;

	CvGame& kGame = GC.getGame();

	iValue += (GC.getADVANCED_START_CITY_COST() * kGame.getGameSpeedInfo().getGrowthPercent()) / 100;

	int iPopulation = GC.getINITIAL_CITY_POPULATION() + kGame.getStartEraInfo().getFreePopulation();
	for(int i = 1; i <= iPopulation; ++i)
	{
		iValue += (getGrowthThreshold(i) * GC.getADVANCED_START_POPULATION_COST()) / 100;
	}

	return iValue;
}

//	--------------------------------------------------------------------------------
int CvPlayer::getGrowthThreshold(int iPopulation) const
{
	CvAssertMsg(iPopulation > 0, "Population of city should be at least 1. Please show Jon this and send your last 5 autosaves.");

	int iThreshold;

	int iBaseThreshold = /*15*/ GC.getBASE_CITY_GROWTH_THRESHOLD();

	int iExtraPopThreshold = int((iPopulation-1) * /*6*/ GC.getCITY_GROWTH_MULTIPLIER());

	iBaseThreshold += iExtraPopThreshold;
	iExtraPopThreshold = (int) pow(double(iPopulation-1), (double) /*1.8*/ GC.getCITY_GROWTH_EXPONENT());

	iThreshold = iBaseThreshold + iExtraPopThreshold;

	if(isMinorCiv())
	{
		iThreshold *= GC.getMINOR_CIV_GROWTH_PERCENT();
		iThreshold /= 100;
	}

	iThreshold *= GC.getGame().getGameSpeedInfo().getGrowthPercent();
	iThreshold /= 100;

	iThreshold *= GC.getGame().getStartEraInfo().getGrowthPercent();
	iThreshold /= 100;

	if(!isHuman() && !IsAITeammateOfHuman() && !isBarbarian())
	{
		iThreshold *= GC.getGame().getHandicapInfo().getAIGrowthPercent();
		iThreshold /= 100;

		iThreshold *= std::max(0, ((GC.getGame().getHandicapInfo().getAIPerEraModifier() * GetCurrentEra()) + 100));
		iThreshold /= 100;
	}

	return std::max(1, iThreshold);
}

//	--------------------------------------------------------------------------------
/// This sets up the m_aiPlots array that is used to contain which plots the player contains
void CvPlayer::InitPlots(void)
{
	int iNumPlots = GC.getMap().getGridHeight() * GC.getMap().getGridHeight();
	// in case we're loading
	if(iNumPlots != m_aiPlots.size())
	{
		m_aiPlots.clear();
		m_aiPlots.push_back_copy(-1, iNumPlots);
	}
}

//	--------------------------------------------------------------------------------
/// This determines what plots the player has under control
void CvPlayer::UpdatePlots(void)
{
	if(m_aiPlots.size() == 0)  // not been inited
	{
		return;
	}

	int iPlotIndex = 0;
	int iMaxNumPlots = (int) m_aiPlots.size();
	while(iPlotIndex < iMaxNumPlots && m_aiPlots[iPlotIndex] != -1)
	{
		m_aiPlots[iPlotIndex] = -1;
		iPlotIndex++;
	}

	int iI;
	CvPlot* pLoopPlot;
	iPlotIndex = 0;
	int iNumPlotsInEntireWorld = GC.getMap().numPlots();
	for(iI = 0; iI < iNumPlotsInEntireWorld; iI++)
	{
		pLoopPlot = GC.getMap().plotByIndexUnchecked(iI);
		if(pLoopPlot->getOwner() != m_eID)
		{
			continue;
		}

		m_aiPlots[iPlotIndex] = iI;
		iPlotIndex++;
	}
}

//	--------------------------------------------------------------------------------
/// Adds a plot at the end of the list
void CvPlayer::AddAPlot(CvPlot* pPlot)
{
	if(!pPlot)
	{
		return;
	}

	if(m_aiPlots.size() == 0)  // not been inited
	{
		return;
	}

	if(pPlot->getOwner() == m_eID)
	{
		return;
	}

	int iPlotIndex = 0;
	int iMaxNumPlots = (int)m_aiPlots.size();
	while(iPlotIndex < iMaxNumPlots && m_aiPlots[iPlotIndex] != -1)
	{
		iPlotIndex++;
	}

	m_aiPlots[iPlotIndex] = GC.getMap().plotNum(pPlot->getX(), pPlot->getY());

}

//	--------------------------------------------------------------------------------
/// Returns the list of the plots the player owns
CvPlotsVector& CvPlayer::GetPlots(void)
{
	return m_aiPlots;
}

//	--------------------------------------------------------------------------------
/// How many plots does this player own?
int CvPlayer::GetNumPlots() const
{
	int iNumPlots = 0;

	CvPlot* pLoopPlot;
	int iNumPlotsInEntireWorld = GC.getMap().numPlots();
	for(int iI = 0; iI < iNumPlotsInEntireWorld; iI++)
	{
		pLoopPlot = GC.getMap().plotByIndexUnchecked(iI);

		if(pLoopPlot->getOwner() != m_eID)
			continue;

		iNumPlots++;
	}

	return iNumPlots;
}


//	--------------------------------------------------------------------------------
/// City strength mod (i.e. 100 = strength doubled)
int CvPlayer::GetCityStrengthMod() const
{
	return m_iCityStrengthMod;
}

//	--------------------------------------------------------------------------------
/// Sets City strength mod (i.e. 100 = strength doubled)
void CvPlayer::SetCityStrengthMod(int iValue)
{
	CvAssert(iValue >= 0);
	m_iCityStrengthMod = iValue;

	// Loop through all Cities and update their strength
	CvCity* pLoopCity;
	int iLoop;

	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateStrengthValue();
	}
}

//	--------------------------------------------------------------------------------
/// Changes City strength mod (i.e. 100 = strength doubled)
void CvPlayer::ChangeCityStrengthMod(int iChange)
{
	if(iChange != 0)
	{
		SetCityStrengthMod(GetCityStrengthMod() + iChange);
	}
}

//	--------------------------------------------------------------------------------
/// City growth percent mod (i.e. 100 = foodDifference doubled)
int CvPlayer::GetCityGrowthMod() const
{
	return m_iCityGrowthMod;
}

//	--------------------------------------------------------------------------------
/// Sets City growth percent mod (i.e. 100 = foodDifference doubled)
void CvPlayer::SetCityGrowthMod(int iValue)
{
	CvAssert(iValue >= 0);
	m_iCityGrowthMod = iValue;
}

//	--------------------------------------------------------------------------------
/// Changes City growth percent mod (i.e. 100 = foodDifference doubled)
void CvPlayer::ChangeCityGrowthMod(int iChange)
{
	if(iChange != 0)
	{
		SetCityGrowthMod(GetCityGrowthMod() + iChange);
	}
}


//	--------------------------------------------------------------------------------
/// Capital growth percent mod (i.e. 100 = foodDifference doubled)
int CvPlayer::GetCapitalGrowthMod() const
{
	return m_iCapitalGrowthMod;
}

//	--------------------------------------------------------------------------------
/// Sets Capital growth percent mod (i.e. 100 = foodDifference doubled)
void CvPlayer::SetCapitalGrowthMod(int iValue)
{
	CvAssert(iValue >= 0);
	m_iCapitalGrowthMod = iValue;
}

//	--------------------------------------------------------------------------------
/// Changes Capital growth percent mod (i.e. 100 = foodDifference doubled)
void CvPlayer::ChangeCapitalGrowthMod(int iChange)
{
	if(iChange != 0)
	{
		SetCapitalGrowthMod(GetCapitalGrowthMod() + iChange);
	}
}

//	--------------------------------------------------------------------------------
/// How many Plot has this player bought (costs should ramp up as more are purchased)
int CvPlayer::GetNumPlotsBought() const
{
	return m_iNumPlotsBought;
}

//	--------------------------------------------------------------------------------
/// Sets how many Plot has this player bought (costs should ramp up as more are purchased)
void CvPlayer::SetNumPlotsBought(int iValue)
{
	CvAssert(iValue >= 0);
	m_iNumPlotsBought = iValue;
}

//	--------------------------------------------------------------------------------
/// Changes how many Plot has this player bought (costs should ramp up as more are purchased)
void CvPlayer::ChangeNumPlotsBought(int iChange)
{
	if(iChange != 0)
	{
		SetNumPlotsBought(GetNumPlotsBought() + iChange);
	}
}

//	--------------------------------------------------------------------------------
/// Gold cost of buying a new Plot
int CvPlayer::GetBuyPlotCost() const
{
	int iCost = /*50*/ GC.getPLOT_BASE_COST();
	iCost += (/*5*/ GC.getPLOT_ADDITIONAL_COST_PER_PLOT() * GetNumPlotsBought());

	// Cost Mod (Policies, etc.)
	if(GetPlotGoldCostMod() != 0)
	{
		iCost *= (100 + GetPlotGoldCostMod());
		iCost /= 100;
	}

	if(isMinorCiv())
	{
		iCost *= /*200*/ GC.getMINOR_CIV_GOLD_PERCENT();
		iCost /= 100;
	}

	return iCost;
}

//	--------------------------------------------------------------------------------
/// How much of a discount do we have for Plot buying
int CvPlayer::GetPlotGoldCostMod() const
{
	return m_iPlotGoldCostMod;
}

//	--------------------------------------------------------------------------------
/// Changes how much of a discount we have for Plot buying
void CvPlayer::ChangePlotGoldCostMod(int iChange)
{
	if(iChange != 0)
	{
		m_iPlotGoldCostMod += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// How much Culture is required for this City to acquire a new Plot
int CvPlayer::GetPlotCultureCostModifier() const
{
	return m_iPlotCultureCostModifier;
}

//	--------------------------------------------------------------------------------
/// Changes how much Culture is required for this City to acquire a new Plot
void CvPlayer::ChangePlotCultureCostModifier(int iChange)
{
	if(iChange != 0)
	{
		m_iPlotCultureCostModifier += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// How much do we dampen the exponent used to determine Culture needed for a City to acquire a new Plot?
int CvPlayer::GetPlotCultureExponentModifier() const
{
	return m_iPlotCultureExponentModifier;
}

//	--------------------------------------------------------------------------------
/// Changes how much we dampen the exponent used to determine Culture needed for a City to acquire a new Plot?
void CvPlayer::ChangePlotCultureExponentModifier(int iChange)
{
	if(iChange != 0)
	{
		m_iPlotCultureExponentModifier += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// How much do we dampen the growth of policy costs based on number of cities?
int CvPlayer::GetNumCitiesPolicyCostDiscount() const
{
	return m_iNumCitiesPolicyCostDiscount;
}

//	--------------------------------------------------------------------------------
/// Changes how much we dampen the growth of policy costs based on number of cities
void CvPlayer::ChangeNumCitiesPolicyCostDiscount(int iChange)
{
	if(iChange != 0)
	{
		m_iNumCitiesPolicyCostDiscount += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// Do we save on unit maintenance for garrisons?
bool CvPlayer::IsGarrisonFreeMaintenance() const
{
	return m_iGarrisonFreeMaintenanceCount > 0;
}

//	--------------------------------------------------------------------------------
/// Changes setting on unit maintenance for garrisons
void CvPlayer::ChangeGarrisonFreeMaintenanceCount(int iChange)
{
	if(iChange != 0)
	{
		m_iGarrisonFreeMaintenanceCount += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// Increase in city range strike due to garrison
int CvPlayer::GetGarrisonedCityRangeStrikeModifier() const
{
	return m_iGarrisonedCityRangeStrikeModifier;
}

//	--------------------------------------------------------------------------------
/// Changes increase in city range strike due to garrison
void CvPlayer::ChangeGarrisonedCityRangeStrikeModifier(int iChange)
{
	if(iChange != 0)
	{
		m_iGarrisonedCityRangeStrikeModifier += iChange;
	}
}

//	--------------------------------------------------------------------------------
/// Cost of purchasing units modified?
int CvPlayer::GetUnitPurchaseCostModifier() const
{
	return m_iUnitPurchaseCostModifier;
}

//	--------------------------------------------------------------------------------
/// Cost of purchasing units modified?
void CvPlayer::ChangeUnitPurchaseCostModifier(int iChange)
{
	if(iChange != 0)
	{
		m_iUnitPurchaseCostModifier += iChange;
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetPlotDanger(CvPlot& pPlot) const
{
	return m_pDangerPlots->GetDanger(pPlot);
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsPlotUnderImmediateThreat(CvPlot& pPlot) const
{
	return m_pDangerPlots->IsUnderImmediateThreat(pPlot);
}

//	--------------------------------------------------------------------------------
/// Find closest city to a plot (within specified search radius)
CvCity* CvPlayer::GetClosestFriendlyCity(CvPlot& plot, int iSearchRadius)
{
	CvCity* pClosestCity = NULL;
	CvCity* pLoopCity;
	int iBestDistance = INT_MAX;

	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		int iDistance = plotDistance(plot.getX(), plot.getY(), pLoopCity->getX(), pLoopCity->getY());
		if(iDistance < iBestDistance && iDistance <= iSearchRadius)
		{
			pClosestCity = pLoopCity;
			iBestDistance = iDistance;
		}
	}

	return pClosestCity;
}

//	--------------------------------------------------------------------------------
// How many Puppet Cities does this player own
int CvPlayer::GetNumPuppetCities() const
{
	int iNum = 0;

	const CvCity* pLoopCity;
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if(pLoopCity->IsPuppet())
		{
			iNum++;
		}
	}

	return iNum;
}
//	--------------------------------------------------------------------------------
// How many Cities does this player have for policy/tech cost purposes?
int CvPlayer::GetMaxEffectiveCities(bool bIncludePuppets)
{
	int iNumCities = getNumCities();

	// Don't count puppet Cities
	int iNumPuppetCities = GetNumPuppetCities();
	iNumCities -= iNumPuppetCities;

	// Don't count cities where the player hasn't decided yet what to do with them or ones that are currently being razed
	int iNumLimboCities = 0;
	const CvCity* pLoopCity;
	int iLoop;
	for(pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if(pLoopCity->IsIgnoreCityForHappiness() || pLoopCity->IsRazing())
		{
			iNumLimboCities++;
		}
	}
	iNumCities -= iNumLimboCities;

	if(iNumCities == 0)	// If we don't pretend the player has at least one city it screws up the math
		iNumCities = 1;

	// Update member variable
	m_iMaxEffectiveCities = (m_iMaxEffectiveCities > iNumCities) ? m_iMaxEffectiveCities : iNumCities;

	if (bIncludePuppets)
	{
#ifdef FIX_MAX_EFFECTIVE_CITIES
		if (m_iMaxEffectiveCities > getNumCities() - iNumLimboCities)
		{
			return m_iMaxEffectiveCities;
		}
		else
		{
			return getNumCities() - iNumLimboCities;
		}
#else
		return m_iMaxEffectiveCities + iNumPuppetCities;
#endif
	}

	return m_iMaxEffectiveCities;
}
//	--------------------------------------------------------------------------------
/// How many Natural Wonders has this player found in its area?
int CvPlayer::GetNumNaturalWondersDiscoveredInArea() const
{
	return m_iNumNaturalWondersDiscoveredInArea;
}

//	--------------------------------------------------------------------------------
/// Sets how many Natural Wonders has this player found in its area
void CvPlayer::SetNumNaturalWondersDiscoveredInArea(int iValue)
{
	m_iNumNaturalWondersDiscoveredInArea = iValue;
}

//	--------------------------------------------------------------------------------
/// Changes many Natural Wonders has this player found in its area
void CvPlayer::ChangeNumNaturalWondersDiscoveredInArea(int iChange)
{
	SetNumNaturalWondersDiscoveredInArea(GetNumNaturalWondersDiscoveredInArea() + iChange);
}

//	--------------------------------------------------------------------------------
/// Calculates how many Natural Wonders are in plots this player owns
int CvPlayer::GetNumNaturalWondersInOwnedPlots()
{
	int iValue = 0;
	CvPlotsVector& aiPlots = GetPlots();
	for(uint ui = 0; ui < aiPlots.size(); ui++)
	{
		// at the end of the plot list
		if(aiPlots[ui] == -1)
		{
			break;
		}

		CvPlot* pPlot = GC.getMap().plotByIndex(aiPlots[ui]);
		if (pPlot && pPlot->IsNaturalWonder())
		{
			iValue++;
		}
	}
	return iValue;
}

//	--------------------------------------------------------------------------------
/// How long ago did this guy last settle a city?
int CvPlayer::GetTurnsSinceSettledLastCity() const
{
	return m_iTurnsSinceSettledLastCity;
}

//	--------------------------------------------------------------------------------
/// How long ago did this guy last settle a city?
void CvPlayer::SetTurnsSinceSettledLastCity(int iValue)
{
	if(m_iTurnsSinceSettledLastCity != iValue)
		m_iTurnsSinceSettledLastCity = iValue;
}

//	--------------------------------------------------------------------------------
/// How long ago did this guy last settle a city?
void CvPlayer::ChangeTurnsSinceSettledLastCity(int iChange)
{
	if(iChange != 0)
		SetTurnsSinceSettledLastCity(GetTurnsSinceSettledLastCity() + iChange);
}

//	--------------------------------------------------------------------------------
/// Find best continents to settle next two cities; returns number found over minimum
int CvPlayer::GetBestSettleAreas(int iMinScore, int& iFirstArea, int& iSecondArea)
{
	CvArea* pLoopArea;
	int iLoop;
	int iBestScore = -1;
	int iSecondBestScore = -1;
	int iBestArea = -1;
	int iSecondBestArea = -1;
	int iNumFound = 0;
	int iScore;

	CvMap& theMap = GC.getMap();

	// Find best two scores above minimum
	for(pLoopArea = theMap.firstArea(&iLoop); pLoopArea != NULL; pLoopArea = theMap.nextArea(&iLoop))
	{
		if(!pLoopArea->isWater())
		{
			iScore = pLoopArea->getTotalFoundValue();

			if(iScore >= iMinScore)
			{
				if(!(GC.getMap().GetAIMapHint() & 4) && !(EconomicAIHelpers::IsAreaSafeForQuickColony(pLoopArea->GetID(), this)))
				{
					iScore /= 3;
				}

				if(iScore > iBestScore)
				{
					// Already have a best area?  If so demote to 2nd
					if(iBestScore > iMinScore)
					{
						iSecondBestScore = iBestScore;
						iSecondBestArea = iBestArea;
					}
					iBestArea = pLoopArea->GetID();
					iBestScore = iScore;
				}

				else if(iScore > iSecondBestScore)
				{
					iSecondBestArea = pLoopArea->GetID();
					iSecondBestScore = iScore;
				}
			}
		}
	}

	// Return data
	iFirstArea = iBestArea;
	iSecondArea = iSecondBestArea;

	if(iSecondArea != -1)
	{
		iNumFound = 2;
	}
	else if(iFirstArea != -1)
	{
		iNumFound = 1;
	}
	return iNumFound;
}

//	--------------------------------------------------------------------------------
/// Find the best spot in the entire world for this unit to settle
CvPlot* CvPlayer::GetBestSettlePlot(CvUnit* pUnit, bool bEscorted, int iArea) const
{
	if(!pUnit)
		return NULL;

	int iSettlerX = pUnit->getX();
	int iSettlerY = pUnit->getY();
	int iUnitArea = pUnit->getArea();
	PlayerTypes eOwner = pUnit->getOwner();
	TeamTypes eTeam = pUnit->getTeam();

	int iBestFoundValue = 0;
	CvPlot* pBestFoundPlot = NULL;

	int iEvalDistance = /*12*/ GC.getSETTLER_EVALUATION_DISTANCE();
	int iDistanceDropoffMod = /*99*/ GC.getSETTLER_DISTANCE_DROPOFF_MODIFIER();

	iEvalDistance += (GC.getGame().getGameTurn() * 5) / 100;

	// scale this based on world size
	const int iDefaultNumTiles = 80*52;
	int iDefaultEvalDistance = iEvalDistance;
	iEvalDistance = (iEvalDistance * GC.getMap().numPlots()) / iDefaultNumTiles;
	iEvalDistance = max(iDefaultEvalDistance,iEvalDistance);

	if(bEscorted && GC.getMap().GetAIMapHint() & 5)  // this is primarily a naval map or at the very least an offshore expansion map
	{
		iEvalDistance *= 3;
		iEvalDistance /= 2;
	}
	// Stay close to home if don't have an escort (unless we were going offshore which doesn't use escorts anymore)
	else if(!bEscorted)
	{
		iEvalDistance *= 2;
		iEvalDistance /= 3;
	}

	CvMap& kMap = GC.getMap();
	int iNumPlots = kMap.numPlots();
	for(int iPlotLoop = 0; iPlotLoop < iNumPlots; iPlotLoop++)
	{
		CvPlot* pPlot = kMap.plotByIndexUnchecked(iPlotLoop);

		if(!pPlot)
		{
			continue;
		}

		if(pPlot->getOwner() != NO_PLAYER && pPlot->getOwner() != eOwner)
		{
			continue;
		}

		if(!pPlot->isRevealed(getTeam()))
		{
			continue;
		}

		if(!pUnit->canFound(pPlot))
		{
			continue;
		}

		if(iArea != -1 && pPlot->getArea() != iArea)
		{
			continue;
		}

		if(pPlot->IsAdjacentOwnedByOtherTeam(eTeam))
		{
			continue;
		}

		if (IsPlotTargetedForCity(pPlot))
		{
			continue;
		}

		// Do we have to check if this is a safe place to go?
		if(bEscorted || (!pPlot->isVisibleEnemyUnit(eOwner)))
		{
			int iValue = pPlot->getFoundValue(eOwner);
			if(iValue > 5000)
			{
				int iSettlerDistance = ::plotDistance(pPlot->getX(), pPlot->getY(), iSettlerX, iSettlerY);
				int iDistanceDropoff = min(99,(iDistanceDropoffMod * iSettlerDistance) / iEvalDistance);
				iDistanceDropoff = max(0,iDistanceDropoff);
				iValue = iValue * (100 - iDistanceDropoff) / 100;
				if(pPlot->getArea() != iUnitArea)
				{
					if(GC.getMap().GetAIMapHint() & 5)  // this is primarily a naval map (or an offshore map like terra)
					{
						iValue *= 3;
						iValue /= 2;
					}
					else
					{
						iValue *= 2;
						iValue /= 3;
					}
				}
				if(iValue > iBestFoundValue)
				{
					iBestFoundValue = iValue;
					pBestFoundPlot = pPlot;
				}
			}
		}
	}
	return pBestFoundPlot;
}


//	--------------------------------------------------------------------------------
/// How many Wonders has this Player constructed?
int CvPlayer::GetNumWonders() const
{
	return m_iNumWonders;
}

//	--------------------------------------------------------------------------------
/// Changes how many Wonders this Player has constructed
void CvPlayer::ChangeNumWonders(int iValue)
{
	if(iValue != 0)
	{
		m_iNumWonders += iValue;
	}
}

//	--------------------------------------------------------------------------------
/// How many Policies has this Player constructed?
int CvPlayer::GetNumPolicies() const
{
	return m_iNumPolicies;
}

//	--------------------------------------------------------------------------------
/// Changes how many Policies this Player has constructed
void CvPlayer::ChangeNumPolicies(int iValue)
{
	if(iValue != 0)
	{
		m_iNumPolicies += iValue;
	}
}

//	--------------------------------------------------------------------------------
/// How many GreatPeople has this Player constructed?
int CvPlayer::GetNumGreatPeople() const
{
	return m_iNumGreatPeople;
}

//	--------------------------------------------------------------------------------
/// Changes how many GreatPeople this Player has constructed
void CvPlayer::ChangeNumGreatPeople(int iValue)
{
	if(iValue != 0)
	{
		m_iNumGreatPeople += iValue;
	}
}

//	--------------------------------------------------------------------------------
/// Special ability where city-states gift great people
void CvPlayer::DoAdoptedGreatPersonCityStatePolicy()
{
	// Loop through all minors and if they're allies, seed the GP counter
	PlayerTypes eMinor;
	for(int iPlayerLoop = MAX_MAJOR_CIVS; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		eMinor = (PlayerTypes) iPlayerLoop;

		if(GET_PLAYER(eMinor).isEverAlive())
		{
			if(GET_PLAYER(eMinor).GetMinorCivAI()->GetAlly() == GetID())
			{
				DoSeedGreatPeopleSpawnCounter();

				break;
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Special ability where city-states gift great people
bool CvPlayer::IsAlliesGreatPersonBiasApplied() const
{
	return m_bAlliesGreatPersonBiasApplied;
}

//	--------------------------------------------------------------------------------
/// Special ability where city-states gift great people
void CvPlayer::SetAlliesGreatPersonBiasApplied(bool bValue)
{
	if(m_bAlliesGreatPersonBiasApplied != bValue)
	{
		m_bAlliesGreatPersonBiasApplied = bValue;
	}
}

//	--------------------------------------------------------------------------------
/// Has this player picked up a Religion yet
bool CvPlayer::IsHasAdoptedStateReligion() const
{
	return m_bHasAdoptedStateReligion;
}

//	--------------------------------------------------------------------------------
/// Sets this player picked up a Religion yet
void CvPlayer::SetHasAdoptedStateReligion(bool bValue)
{
	if(m_bHasAdoptedStateReligion != bValue)
	{
		m_bHasAdoptedStateReligion = bValue;
	}
}

//	--------------------------------------------------------------------------------
/// Number of Cities in the empire with our State Religion
int CvPlayer::GetNumCitiesWithStateReligion()
{
	int iNumCitiesWithStateReligion = 0;

	int iLoopCity;
	CvCity* pLoopCity = NULL;
	// Look at all of our Cities to see if they have our Religion
	for(pLoopCity = firstCity(&iLoopCity); pLoopCity != NULL; pLoopCity = nextCity(&iLoopCity))
	{
		if(pLoopCity->GetPlayersReligion() == GetID())
		{
			iNumCitiesWithStateReligion++;
		}
	}

	return iNumCitiesWithStateReligion;
}

//	--------------------------------------------------------------------------------
/// Where was this player's Religion adopted
CvCity* CvPlayer::GetHolyCity()
{
	return getCity(m_iHolyCityID);
}

//	--------------------------------------------------------------------------------
/// Sets where this player's Religion adopted
void CvPlayer::SetHolyCity(int iCityID)
{
	// This should only be set once under normal circumstances
	CvAssert(m_iHolyCityID == -1);

	m_iHolyCityID = iCityID;
}

//	--------------------------------------------------------------------------------
PromotionTypes CvPlayer::GetEmbarkationPromotion() const
{
	if(GET_TEAM(getTeam()).canDefensiveEmbark())
	{
		return (PromotionTypes)GC.getPROMOTION_DEFENSIVE_EMBARKATION();
	}

	if(m_pTraits)
	{
		if(m_pTraits->IsEmbarkedAllWater())
		{
			return (PromotionTypes)GC.getPROMOTION_ALLWATER_EMBARKATION();
		}
	}

	return (PromotionTypes)GC.getPROMOTION_EMBARKATION();
}

//	--------------------------------------------------------------------------------
/// Provide Notification about someone adopting a new Religon
void CvPlayer::DoAnnounceReligionAdoption()
{
	CvCity* pHolyCity = GetHolyCity();

	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI& thisPlayer = GET_PLAYER((PlayerTypes)iI);
		if(thisPlayer.isHuman() && thisPlayer.isAlive() && thisPlayer.GetNotifications())
		{
			int iX = -1;
			int iY = -1;

			Localization::String localizedText;

			// Active Player
			if(GC.getGame().getActivePlayer() == GetID())
			{
				iX = pHolyCity->getX();
				iY = pHolyCity->getY();
				localizedText = Localization::Lookup("TXT_KEY_MISC_RELIGION_ADOPTED_YOU");
				localizedText << pHolyCity->getNameKey() << GetStateReligionKey();
			}
			// Met Player
			else if(GET_TEAM(GC.getGame().getActiveTeam()).isHasMet(thisPlayer.getTeam()))
			{
				localizedText = Localization::Lookup("TXT_KEY_MISC_RELIGION_ADOPTED_ANOTHER_PLAYER");
				localizedText << GET_PLAYER(pHolyCity->getOwner()).getNameKey() << GetStateReligionKey();

				// We've seen this player's City
				if(pHolyCity->isRevealed(thisPlayer.getTeam(), false))
				{
					iX = pHolyCity->getX();
					iY = pHolyCity->getY();
				}
			}
			// Unmet Player
			else
			{
				localizedText = Localization::Lookup("TXT_KEY_MISC_RELIGION_ADOPTED_UNKNOWN");
			}

			thisPlayer.GetNotifications()->Add(NOTIFICATION_RELIGION_RACE, localizedText.toUTF8(), localizedText.toUTF8(), iX, iY, -1);
		}
	}
}

bool CvPlayer::IsAllowedToTradeWith(PlayerTypes eOtherPlayer)
{
	if (GC.getGame().GetGameLeagues()->IsTradeEmbargoed(GetID(), eOtherPlayer) && eOtherPlayer != m_eID)
	{
		return false;
	}

	return true;
}

#ifdef CS_ALLYING_WAR_RESCTRICTION
int CvPlayer::getTurnCSWarAllowing(PlayerTypes ePlayer)
{
	int iValue = -1;
	for (int iI = 0; iI < MAX_MINOR_CIVS; iI++)
	{
		if (m_ppaaiTurnCSWarAllowing[ePlayer][iI] > iValue)
		{
			iValue = m_ppaaiTurnCSWarAllowing[ePlayer][iI];
		}
	}

	return iValue;
}

int CvPlayer::getTurnCSWarAllowingMinor(PlayerTypes ePlayer, PlayerTypes eMinor)
{
	return m_ppaaiTurnCSWarAllowing[ePlayer][int(eMinor) - MAX_MAJOR_CIVS];
}

void CvPlayer::setTurnCSWarAllowingMinor(PlayerTypes ePlayer, PlayerTypes eMinor, int iValue)
{
	Firaxis::Array<int, MAX_MINOR_CIVS> turn = m_ppaaiTurnCSWarAllowing[ePlayer];
	turn[int(eMinor) - MAX_MAJOR_CIVS] = iValue;
	m_ppaaiTurnCSWarAllowing.setAt(ePlayer, turn);
}

float CvPlayer::getTimeCSWarAllowing(PlayerTypes ePlayer)
{
	float fValue = 0.f;
	for (int iI = 0; iI < MAX_MINOR_CIVS; iI++)
	{
		if (m_ppaafTimeCSWarAllowing[ePlayer][iI] > fValue)
		{
			fValue = m_ppaafTimeCSWarAllowing[ePlayer][iI];
		}
	}

	return fValue;
}

float CvPlayer::getTimeCSWarAllowingMinor(PlayerTypes ePlayer, PlayerTypes eMinor)
{
	return m_ppaafTimeCSWarAllowing[ePlayer][int(eMinor) - MAX_MAJOR_CIVS];
}

void CvPlayer::setTimeCSWarAllowingMinor(PlayerTypes ePlayer, PlayerTypes eMinor, float fValue)
{
	Firaxis::Array<float, MAX_MINOR_CIVS> time = m_ppaafTimeCSWarAllowing[ePlayer];
	time[int(eMinor) - MAX_MAJOR_CIVS] = fValue;
	m_ppaafTimeCSWarAllowing.setAt(ePlayer, time);
}
#endif

#ifdef PENALTY_FOR_DELAYING_POLICIES
bool CvPlayer::IsDelayedPolicy() const
{
#ifdef BLITZ_MODE
	if (GC.getGame().isOption("GAMEOPTION_BLITZ_MODE"))
	{
		return false;
	}
	else
	{
		return m_bIsDelayedPolicy;
	}
#else
	return m_bIsDelayedPolicy;
#endif
}

void CvPlayer::setIsDelayedPolicy(bool bValue)
{
	m_bIsDelayedPolicy = bValue;
}
#endif

#ifdef BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
int CvPlayer::GetYieldForEachBuildingInEmpire(BuildingTypes eBuilding, YieldTypes eIndex) const
{
	return m_ppaaiYieldForEachBuildingInEmpire[eBuilding][eIndex];
}

void CvPlayer::ChangeYieldForEachBuildingInEmpire(BuildingTypes eBuilding, YieldTypes eIndex, int iChange)
{
	if (iChange != 0)
	{
		Firaxis::Array<int, NUM_YIELD_TYPES> yield = m_ppaaiYieldForEachBuildingInEmpire[eBuilding];
		yield[int(eIndex)] += iChange;
		m_ppaaiYieldForEachBuildingInEmpire.setAt(eBuilding, yield);
	}
}
#endif

#ifdef POLICY_ALLOWS_GP_BUYS_FOR_GOLD
int CvPlayer::GetNumGoldPurchasedGreatPerson() const
{
	return m_iNumGoldPurchasedGreatPerson;
}

void CvPlayer::ChangeNumGoldPurchasedGreatPerson(int iChange)
{
	m_iNumGoldPurchasedGreatPerson += iChange;
}

bool CvPlayer::IsGoldGreatPerson(UnitClassTypes eUnitClass) const
{
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_WRITER"))
	{
		return m_bGoldWriter;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
	{
		return m_bGoldArtist;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
	{
		return m_bGoldMusician;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_SCIENTIST"))
	{
		return m_bGoldScientist;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_ENGINEER"))
	{
		return m_bGoldEngineer;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
	{
		return m_bGoldMerchant;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_GREAT_GENERAL"))
	{
		return m_bGoldGeneral;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_GREAT_ADMIRAL"))
	{
		return m_bGoldAdmiral;
	}

	return false;
}

void CvPlayer::SetGoldGreatPerson(UnitClassTypes eUnitClass, bool bValue)
{
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_WRITER"))
	{
		m_bGoldWriter = bValue;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
	{
		m_bGoldArtist = bValue;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
	{
		m_bGoldMusician = bValue;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_SCIENTIST"))
	{
		m_bGoldScientist = bValue;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_ENGINEER"))
	{
		m_bGoldEngineer = bValue;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
	{
		m_bGoldMerchant = bValue;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_GREAT_GENERAL"))
	{
		m_bGoldGeneral = bValue;
	}
	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_GREAT_ADMIRAL"))
	{
		m_bGoldAdmiral = bValue;
	}
}
#endif

#ifdef POLICY_SPY_DETECTION
bool CvPlayer::IsSpyDetection() const
{
	return m_iSpyDetection > 0;
}

void CvPlayer::ChangeSpyDetection(int iChange)
{
	m_iSpyDetection += iChange;
}
#endif

#ifdef BUILDING_BORDER_TRANSITION_OBSTACLE
//	--------------------------------------------------------------------------------
int CvPlayer::getBorderTransitionObstacleCount() const
{
	return m_iBorderTransitionObstacleCount;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::isBorderTransitionObstacle() const
{
	return (getBorderTransitionObstacleCount() > 0);
}

//	--------------------------------------------------------------------------------
void CvPlayer::changeBorderTransitionObstacleCount(int iChange)
{
	if (iChange != 0)
	{
		m_iBorderTransitionObstacleCount = (m_iBorderTransitionObstacleCount + iChange);
		CvAssert(getBorderTransitionObstacleCount() >= 0);
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
// Tutorial Stuff...
//////////////////////////////////////////////////////////////////////////

//	--------------------------------------------------------------------------------
bool CvPlayer::GetEverPoppedGoody()
{
	return m_bEverPoppedGoody;
}

//	--------------------------------------------------------------------------------
CvPlot* CvPlayer::GetClosestGoodyPlot(bool bStopAfterFindingFirst)
{
	FFastVector<int> aiGoodyPlots = GetEconomicAI()->GetGoodyHutPlots();

	CvPlot* pResultPlot = NULL;
	int iShortestPath = INT_MAX;

	// cycle through goodies
	for(uint uiGoodyIndex = 0; uiGoodyIndex < aiGoodyPlots.size(); uiGoodyIndex++)
	{
		CvPlot* pPlot = GC.getMap().plotByIndex(aiGoodyPlots[uiGoodyIndex]);
		if(!pPlot || !pPlot->isGoody(getTeam()))
		{
			continue;
		}

		// cycle through units
		int iUnitLoop;
		CvUnit* pLoopUnit = NULL;

		// Setup m_units
		for(pLoopUnit = firstUnit(&iUnitLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iUnitLoop))
		{
			if(!pLoopUnit)
			{
				continue;
			}

			if(pPlot->getArea() != pLoopUnit->getArea() && !pLoopUnit->CanEverEmbark())
			{
				continue;
			}

			int iReturnValue = INT_MAX;
			bool bResult = pLoopUnit->GeneratePath(pPlot, MOVE_UNITS_IGNORE_DANGER, true, &iReturnValue);

			if(bResult)
			{
				if(iReturnValue < iShortestPath)
				{
					pResultPlot = pPlot;
				}

				if(bStopAfterFindingFirst)
				{
					return pPlot;
				}
			}
		}
	}

	return pResultPlot;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::GetPlotHasOrder(CvPlot* pPlot)
{
	if(!pPlot)
	{
		return false;
	}

	int iLoopUnit;
	for(CvUnit* pLoopUnit = firstUnit(&iLoopUnit); pLoopUnit; pLoopUnit = nextUnit(&iLoopUnit))
	{
		CvPlot* pMissionPlot = pLoopUnit->GetMissionAIPlot();
		if(NULL != pMissionPlot && pMissionPlot->getX() == pPlot->getX() && pMissionPlot->getY() == pPlot->getY())
		{
			return true;
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::GetAnyUnitHasOrderToGoody()
{
	FFastVector<int> aiGoodyPlots = GetEconomicAI()->GetGoodyHutPlots();

	// cycle through goodies
	for(uint uiGoodyIndex = 0; uiGoodyIndex < aiGoodyPlots.size(); uiGoodyIndex++)
	{
		CvPlot* pPlot = GC.getMap().plotByIndex(aiGoodyPlots[uiGoodyIndex]);
		if(!pPlot)
		{
			continue;
		}

		if(!pPlot->isGoody(getTeam()))
		{
			continue;
		}

		if(GetPlotHasOrder(pPlot))
		{
			return true;
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::GetEverTrainedBuilder()
{
	return m_bEverTrainedBuilder;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNumFreeTechs() const
{
	return m_iFreeTechCount;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetNumFreeTechs(int iValue)
{
	m_iFreeTechCount = iValue;
	if(GetID() == GC.getGame().getActivePlayer())
	{
		GC.GetEngineUserInterface()->setDirty(ResearchButtons_DIRTY_BIT, true);
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetMedianTechPercentage() const
{
	return m_iMedianTechPercentage;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeMedianTechPercentage(int iValue)
{
	m_iMedianTechPercentage += iValue;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNumFreePolicies() const
{
	return m_iNumFreePolicies;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetNumFreePolicies(int iValue)
{
	// Increase count of free Policies we've ever had
	int iDifference = iValue - m_iNumFreePolicies;
	if(iDifference > 0)
	{
		ChangeNumFreePoliciesEver(iDifference);
	}

	m_iNumFreePolicies = iValue;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeNumFreePolicies(int iChange)
{
	SetNumFreePolicies(GetNumFreePolicies() + iChange);

	if(iChange > 0 && getNumCities() > 0)
	{
		CvNotifications* pNotifications = GetNotifications();
		if(pNotifications)
		{
			CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_FREE_POLICY");
			CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_FREE_POLICY_SUMMARY");
			pNotifications->Add(NOTIFICATION_FREE_POLICY, strBuffer, strSummary, -1, -1, -1);
		}

	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNumFreeTenets() const
{
	return m_iNumFreeTenets;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetNumFreeTenets(int iValue, bool bCountAsFreePolicies)
{
	// Increase count of free Policies we've ever had
	int iDifference = iValue - m_iNumFreeTenets;
	if (bCountAsFreePolicies && iDifference > 0)
	{
		ChangeNumFreePoliciesEver(iDifference);
	}

	m_iNumFreeTenets = iValue;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeNumFreeTenets(int iChange, bool bCountAsFreePolicies)
{
	SetNumFreeTenets(GetNumFreeTenets() + iChange, bCountAsFreePolicies);

	if(iChange > 0 && getNumCities() > 0)
	{
		CvNotifications* pNotifications = GetNotifications();
		if(pNotifications)
		{
			CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_FREE_POLICY");
			CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_FREE_POLICY_SUMMARY");
			pNotifications->Add(NOTIFICATION_FREE_POLICY, strBuffer, strSummary, -1, -1, -1);
		}

	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNumFreeGreatPeople() const
{
	return m_iNumFreeGreatPeople;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetNumFreeGreatPeople(int iValue)
{
	m_iNumFreeGreatPeople = iValue;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeNumFreeGreatPeople(int iChange)
{
	m_iNumFreeGreatPeople = GetNumFreeGreatPeople() + iChange;
	if(iChange > 0)
	{
		if(isHuman())
		{
			CvNotifications* pNotifications = GetNotifications();
			if(pNotifications)
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_CHOOSE_FREE_GREAT_PERSON");
				CvString strSummary = GetLocalizedText("TXT_KEY_CHOOSE_FREE_GREAT_PERSON_TT");
				pNotifications->Add(NOTIFICATION_FREE_GREAT_PERSON, strSummary.c_str(), strBuffer.c_str(), -1, -1, -1);
			}
		}
		else
		{
			for(int iI = 0; iI < iChange; iI++)
			{
				AI_chooseFreeGreatPerson();
			}
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNumMayaBoosts() const
{
	return m_iNumMayaBoosts;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetNumMayaBoosts(int iValue)
{
	m_iNumMayaBoosts = iValue;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeNumMayaBoosts(int iChange)
{
	m_iNumMayaBoosts = GetNumMayaBoosts() + iChange;
	if(iChange > 0)
	{
		if(isHuman())
		{
			CvNotifications* pNotifications = GetNotifications();
			if(pNotifications)
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_MAYA_LONG_COUNT");
				CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_MAYA_LONG_COUNT_TT");
				pNotifications->Add(NOTIFICATION_MAYA_LONG_COUNT, strSummary.c_str(), strBuffer.c_str(), -1, -1, -1);
			}
		}
		else
		{
			for(int iI = 0; iI < iChange; iI++)
			{
				GetPlayerTraits()->ChooseMayaBoost();
			}
		}
	}
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNumFaithGreatPeople() const
{
	return m_iNumFaithGreatPeople;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetNumFaithGreatPeople(int iValue)
{
	m_iNumFaithGreatPeople = iValue;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeNumFaithGreatPeople(int iChange)
{
	m_iNumFaithGreatPeople = GetNumFaithGreatPeople() + iChange;
	if(iChange > 0)
	{
		if(isHuman())
		{
			CvNotifications* pNotifications = GetNotifications();
			if(pNotifications)
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_FAITH_GREAT_PERSON");
				CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_FAITH_GREAT_PERSON_TT");
				pNotifications->Add(NOTIFICATION_FAITH_GREAT_PERSON, strSummary.c_str(), strBuffer.c_str(), -1, -1, -1);
			}
		}
		else
		{
			for(int iI = 0; iI < iChange; iI++)
			{
				AI_chooseFreeGreatPerson();
			}
		}
	}
}
//	--------------------------------------------------------------------------------
int CvPlayer::GetNumArchaeologyChoices() const
{
	return m_iNumArchaeologyChoices;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetNumArchaeologyChoices(int iValue)
{
	m_iNumArchaeologyChoices = iValue;
}

//	--------------------------------------------------------------------------------
FaithPurchaseTypes CvPlayer::GetFaithPurchaseType() const
{
	return m_eFaithPurchaseType;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetFaithPurchaseType(FaithPurchaseTypes eType)
{
	m_eFaithPurchaseType = eType;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetFaithPurchaseIndex() const
{
	return m_iFaithPurchaseIndex;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetFaithPurchaseIndex(int iIndex)
{
	m_iFaithPurchaseIndex = iIndex;
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetNumFreePoliciesEver() const
{
	return m_iNumFreePoliciesEver;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetNumFreePoliciesEver(int iValue)
{
	m_iNumFreePoliciesEver = iValue;
}

//	--------------------------------------------------------------------------------
void CvPlayer::ChangeNumFreePoliciesEver(int iChange)
{
	SetNumFreePoliciesEver(GetNumFreePoliciesEver() + iChange);
}

//	--------------------------------------------------------------------------------
int CvPlayer::GetLastSliceMoved() const
{
	return m_iLastSliceMoved;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetLastSliceMoved(int iValue)
{
	m_iLastSliceMoved = iValue;
}

//	--------------------------------------------------------------------------------
void CvPlayer::SetEverConqueredBy(PlayerTypes ePlayer, bool bValue)
{
	if(bValue)
	{
		m_bfEverConqueredBy.SetBit(ePlayer);
	}
	else
	{
		m_bfEverConqueredBy.ClearBit(ePlayer);
	}
}

//	--------------------------------------------------------------------------------
bool CvPlayer::IsEverConqueredBy(PlayerTypes ePlayer)
{
	return m_bfEverConqueredBy.GetBit(ePlayer);
}

//	------------------------------------------------------------------------------------------------
const FAutoArchive& CvPlayer::getSyncArchive() const
{
	return m_syncArchive;
}

//	--------------------------------------------------------------------------------
FAutoArchive& CvPlayer::getSyncArchive()
{
	return m_syncArchive;
}

//	-----------------------------------------------------------------------------------------------
bool CvPlayer::isLocalPlayer() const
{
	return (GC.getGame().getActivePlayer() == GetID());
}

//	-----------------------------------------------------------------------------------------------
void CvPlayer::disconnected()
{
	bool isMultiplayer = GC.getGame().isGameMultiPlayer();
	if(isMultiplayer && isHuman() && !isLocalPlayer())
	{
			//log message for debugging the occasional lack of disconnect notification when Steam p2p connections timeout. - bolson 1/10/13
			FILogFile* logFile = LOGFILEMGR.GetLog("net_message_debug.log", 0);
			if(logFile)
			{
				logFile->DebugMsg("Attempted to post notification for player disconnect event.  Player(%i)", GetID());
			}

			CvNotifications* pNotifications = GET_PLAYER(GC.getGame().getActivePlayer()).GetNotifications();
			if(pNotifications){
				if(gDLL->IsPlayerKicked(GetID())){
					Localization::String kickedMsg = Localization::Lookup("TXT_KEY_PLAYER_KICKED");
					kickedMsg << getNameKey();
					pNotifications->Add(NOTIFICATION_PLAYER_KICKED, kickedMsg.toUTF8(), kickedMsg.toUTF8(), -1, -1, GetID());
				}
				else{
					Localization::String disconnectString = Localization::Lookup("TXT_KEY_PLAYER_DISCONNECTED");
					disconnectString << getNameKey();

					if(CvPreGame::isPitBoss()){
						disconnectString = Localization::Lookup("TXT_KEY_PLAYER_DISCONNECTED_PITBOSS");
						disconnectString << getNameKey();	
					}

					pNotifications->Add(NOTIFICATION_PLAYER_DISCONNECTED, disconnectString.toUTF8(), disconnectString.toUTF8(), -1, -1, GetID());
				}
			}

#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
		if (!isObserver())
		{
			if (!CvPreGame::isPitBoss() || gDLL->IsPlayerKicked(GetID()))
			{
				setIsDisconnected(false); // kicked players should unpause the game
				bool isAnyDisconnected = false;
				for (int iI = 0; iI < MAX_PLAYERS; iI++)
				{
					PlayerTypes eLoopPlayer = (PlayerTypes)iI;
					if (GET_PLAYER(eLoopPlayer).isDisconnected())
					{
						isAnyDisconnected = true;
					}
				}
#ifdef TURN_TIMER_PAUSE_BUTTON
				{
					if (!isAnyDisconnected && GC.getGame().isOption(GAMEOPTION_END_TURN_TIMER_ENABLED) && !GC.getGame().isPaused() && GC.getGame().getGameState() == GAMESTATE_ON)
					{
						if ((GC.getGame().getElapsedGameTurns() > 0) && GET_PLAYER(GC.getGame().getActivePlayer()).isTurnActive())
						{
							// as there is no netcode for timer pause,
							// this function will act as one, if called with special agreed upon arguments
							// resetTurnTimer(true);
							gDLL->sendGiftUnit(NO_PLAYER, -11);
						}
					}
				}
#endif
				// JAR : First pass, automatically fall back to CPU so the
				// game can continue. Todo : add popup on host asking whether
				// the AI should take over or everyone should wait for the
				// player to reconnect
				CvPreGame::setSlotStatus(GetID(), SS_COMPUTER);
				CvPreGame::VerifyHandicap(GetID());	//Changing the handicap because we're switching to AI

				// Load leaderhead for this new AI player
				gDLL->NotifySpecificAILeaderInGame(GetID());

				if (!GC.getGame().isOption(GAMEOPTION_DYNAMIC_TURNS) && GC.getGame().isOption(GAMEOPTION_SIMULTANEOUS_TURNS))
				{//When in fully simultaneous turn mode, having a player disconnect might trigger the automove phase for all human players.
					checkRunAutoMovesForEveryone();
				}
#ifdef DO_CANCEL_DEALS_WITH_AI
				if (!isHuman() && GC.getGame().isOption("GAMEOPTION_AI_TWEAKS"))
				{
					DealList tempDeals;

					if (GC.getGame().GetGameDeals()->m_CurrentDeals.size() > 0)
					{
						bool bSomethingChanged = false;

						// Copy the deals into a temporary container
						for (DealList::iterator it = GC.getGame().GetGameDeals()->m_CurrentDeals.begin(); it != GC.getGame().GetGameDeals()->m_CurrentDeals.end(); ++it)
						{
							tempDeals.push_back(*it);
						}

						GC.getGame().GetGameDeals()->m_CurrentDeals.clear();
						for (DealList::iterator it = tempDeals.begin(); it != tempDeals.end(); ++it)
						{
							// Players on this deal match?
							if (it->m_eFromPlayer == GetID() || it->m_eToPlayer == GetID())
							{
								// Change final turn
								it->m_iFinalTurn = GC.getGame().getGameTurn();

								bool bIsTradeItemPeaceTreaty = false;
								bool bNotIsTradeItemPeaceTreaty = false;
								for (TradedItemList::iterator itemIter = it->m_TradedItems.begin(); itemIter != it->m_TradedItems.end(); ++itemIter)
								{
									if (itemIter->m_eItemType == TRADE_ITEM_PEACE_TREATY)
									{
										bIsTradeItemPeaceTreaty = true;
									}
									else
									{
										bNotIsTradeItemPeaceTreaty = true;
									}
								}

								if (!bIsTradeItemPeaceTreaty || bNotIsTradeItemPeaceTreaty)
								{
									// Cancel individual items
									for (TradedItemList::iterator itemIter = it->m_TradedItems.begin(); itemIter != it->m_TradedItems.end(); ++itemIter)
									{
										bSomethingChanged = true;

										itemIter->m_iFinalTurn = GC.getGame().getGameTurn();

										PlayerTypes eFromPlayer = itemIter->m_eFromPlayer;
										PlayerTypes eToPlayer = it->GetOtherPlayer(eFromPlayer);

										GC.getGame().GetGameDeals()->DoEndTradedItem(&*itemIter, eToPlayer, true);
									}
									GC.getGame().GetGameDeals()->m_HistoricalDeals.push_back(*it);
								}
								else
								{
									GC.getGame().GetGameDeals()->m_CurrentDeals.push_back(*it);
								}
							}
							else
							{
								GC.getGame().GetGameDeals()->m_CurrentDeals.push_back(*it);
							}
						}

						GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
					}

					GC.getGame().GetGameTrade()->ClearAllCivTradeRoutes(GetID());
					for (int iLoopTeam = 0; iLoopTeam < MAX_CIV_TEAMS; iLoopTeam++)
					{
						TeamTypes eTeam = (TeamTypes)iLoopTeam;
						if (getTeam() != eTeam && GET_TEAM(eTeam).isAlive() && GET_TEAM(eTeam).isHuman())
						{
							GET_TEAM(getTeam()).CloseEmbassyAtTeam(eTeam);
							GET_TEAM(eTeam).CloseEmbassyAtTeam(getTeam());
							GET_TEAM(getTeam()).CancelResearchAgreement(eTeam);
							GET_TEAM(eTeam).CancelResearchAgreement(getTeam());
							GET_TEAM(getTeam()).EvacuateDiplomatsAtTeam(eTeam);
							GET_TEAM(eTeam).EvacuateDiplomatsAtTeam(getTeam());

							// Bump Units out of places they shouldn't be
							GC.getMap().verifyUnitValidPlot();
						}
					}
				}
#endif
#ifdef DO_EXTRACT_AI_SPIES
				if (!isHuman() && GC.getGame().isOption("GAMEOPTION_AI_TWEAKS"))
				{
					for (uint uiSpy = 0; uiSpy < GetEspionage()->m_aSpyList.size(); uiSpy++)
					{
						GetEspionage()->ExtractSpyFromCity(uiSpy);
					}
				}
#endif
#ifdef CHANGE_CITY_ORIGINAL_OWNER
				if (GC.getGame().isNetworkMultiPlayer())
				{
					for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
					{
						PlayerTypes eLoopPlayer = (PlayerTypes)iPlayerLoop;

						if (eLoopPlayer != GetID() && GET_PLAYER(eLoopPlayer).isAlive() && GET_PLAYER(eLoopPlayer).isHuman())
						{
							int iCityLoop;
							CvCity* pLoopCity = NULL;
							for (pLoopCity = GET_PLAYER(eLoopPlayer).firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(eLoopPlayer).nextCity(&iCityLoop))
							{
								if ((int)pLoopCity->getOriginalOwner() < MAX_MAJOR_CIVS && !pLoopCity->IsOriginalCapital())
								{
									if (pLoopCity->getOriginalOwner() == GetID())
									{
										if (pLoopCity->IsNoOccupiedUnhappiness())
										{
											pLoopCity->setOriginalOwner(eLoopPlayer);
											pLoopCity->SetOccupied(false);
										}
									}
								}
							}
						}
					}
				}
#endif
#ifdef CHANGE_HOST_IF_DISCONNECTED
				CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
				if (pLeague != NULL)
				{
					// Check host
					if (pLeague->IsHostMember(GetID()))
					{
						pLeague->AssignNewHost();
					}
				}
#endif
	}
#else
		if(!isObserver() && (!CvPreGame::isPitBoss() || gDLL->IsPlayerKicked(GetID())))
		{
			// JAR : First pass, automatically fall back to CPU so the
			// game can continue. Todo : add popup on host asking whether
			// the AI should take over or everyone should wait for the
			// player to reconnect
			CvPreGame::setSlotStatus(GetID(), SS_COMPUTER);
			CvPreGame::VerifyHandicap(GetID());	//Changing the handicap because we're switching to AI

			// Load leaderhead for this new AI player
			gDLL->NotifySpecificAILeaderInGame(GetID());

			if (!GC.getGame().isOption(GAMEOPTION_DYNAMIC_TURNS) && GC.getGame().isOption(GAMEOPTION_SIMULTANEOUS_TURNS))
			{//When in fully simultaneous turn mode, having a player disconnect might trigger the automove phase for all human players.
				checkRunAutoMovesForEveryone();
			}
#ifdef DO_CANCEL_DEALS_WITH_AI
			if (!isHuman() && GC.getGame().isOption("GAMEOPTION_AI_TWEAKS"))
			{
				DealList tempDeals;

				if (GC.getGame().GetGameDeals()->m_CurrentDeals.size() > 0)
				{
					bool bSomethingChanged = false;

					// Copy the deals into a temporary container
					for (DealList::iterator it = GC.getGame().GetGameDeals()->m_CurrentDeals.begin(); it != GC.getGame().GetGameDeals()->m_CurrentDeals.end(); ++it)
					{
						tempDeals.push_back(*it);
					}

					GC.getGame().GetGameDeals()->m_CurrentDeals.clear();
					for (DealList::iterator it = tempDeals.begin(); it != tempDeals.end(); ++it)
					{
						// Players on this deal match?
						if (it->m_eFromPlayer == GetID() || it->m_eToPlayer == GetID())
						{
							// Change final turn
							it->m_iFinalTurn = GC.getGame().getGameTurn();

							bool bIsTradeItemPeaceTreaty = false;
							bool bNotIsTradeItemPeaceTreaty = false;
							for (TradedItemList::iterator itemIter = it->m_TradedItems.begin(); itemIter != it->m_TradedItems.end(); ++itemIter)
							{
								if (itemIter->m_eItemType == TRADE_ITEM_PEACE_TREATY)
								{
									bIsTradeItemPeaceTreaty = true;
								}
								else
								{
									bNotIsTradeItemPeaceTreaty = true;
								}
							}

							if (!bIsTradeItemPeaceTreaty || bNotIsTradeItemPeaceTreaty)
							{
								// Cancel individual items
								for (TradedItemList::iterator itemIter = it->m_TradedItems.begin(); itemIter != it->m_TradedItems.end(); ++itemIter)
								{
									bSomethingChanged = true;

									itemIter->m_iFinalTurn = GC.getGame().getGameTurn();

									PlayerTypes eFromPlayer = itemIter->m_eFromPlayer;
									PlayerTypes eToPlayer = it->GetOtherPlayer(eFromPlayer);

									GC.getGame().GetGameDeals()->DoEndTradedItem(&*itemIter, eToPlayer, true);
								}
								GC.getGame().GetGameDeals()->m_HistoricalDeals.push_back(*it);
							}
							else
							{
								GC.getGame().GetGameDeals()->m_CurrentDeals.push_back(*it);
							}
						}
						else
						{
							GC.getGame().GetGameDeals()->m_CurrentDeals.push_back(*it);
						}
					}

					GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
				}

				GC.getGame().GetGameTrade()->ClearAllCivTradeRoutes(GetID());
				for (int iLoopTeam = 0; iLoopTeam < MAX_CIV_TEAMS; iLoopTeam++)
				{
					TeamTypes eTeam = (TeamTypes)iLoopTeam;
					if (getTeam() != eTeam && GET_TEAM(eTeam).isAlive() && GET_TEAM(eTeam).isHuman())
					{
						GET_TEAM(getTeam()).CloseEmbassyAtTeam(eTeam);
						GET_TEAM(eTeam).CloseEmbassyAtTeam(getTeam());
						GET_TEAM(getTeam()).CancelResearchAgreement(eTeam);
						GET_TEAM(eTeam).CancelResearchAgreement(getTeam());
						GET_TEAM(getTeam()).EvacuateDiplomatsAtTeam(eTeam);
						GET_TEAM(eTeam).EvacuateDiplomatsAtTeam(getTeam());

						// Bump Units out of places they shouldn't be
						GC.getMap().verifyUnitValidPlot();
					}
				}
			}
#endif
#ifdef DO_EXTRACT_AI_SPIES
			if (!isHuman() && GC.getGame().isOption("GAMEOPTION_AI_TWEAKS"))
			{
				for (uint uiSpy = 0; uiSpy < GetEspionage()->m_aSpyList.size(); uiSpy++)
				{
					GetEspionage()->ExtractSpyFromCity(uiSpy);
				}
			}
#endif
#ifdef CHANGE_CITY_ORIGINAL_OWNER
			if (GC.getGame().isNetworkMultiPlayer())
			{
				for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
				{
					PlayerTypes eLoopPlayer = (PlayerTypes)iPlayerLoop;

					if (eLoopPlayer != GetID() && GET_PLAYER(eLoopPlayer).isAlive() && GET_PLAYER(eLoopPlayer).isHuman())
					{
						int iCityLoop;
						CvCity* pLoopCity = NULL;
						for (pLoopCity = GET_PLAYER(eLoopPlayer).firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(eLoopPlayer).nextCity(&iCityLoop))
						{
							if ((int)pLoopCity->getOriginalOwner() < MAX_MAJOR_CIVS && !pLoopCity->IsOriginalCapital())
							{
								if (pLoopCity->getOriginalOwner() == GetID())
								{
									if (pLoopCity->IsNoOccupiedUnhappiness())
									{
										pLoopCity->setOriginalOwner(eLoopPlayer);
										pLoopCity->SetOccupied(false);
									}
								}
							}
						}
					}
				}
			}
#endif
#ifdef CHANGE_HOST_IF_DISCONNECTED
			CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
			if (pLeague != NULL)
			{
				// Check host
				if (pLeague->IsHostMember(GetID()))
				{
					pLeague->AssignNewHost();
				}
			}
#endif
		}
#endif
#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
			else if (/*GC.getGame().isOption("GAMEOPTION_AUTOPAUSE_ON_ACTIVE_DISCONNECT")*/ true && isAlive() && isTurnActive() && (GC.getGame().isOption(GAMEOPTION_DYNAMIC_TURNS) || GC.getGame().isOption(GAMEOPTION_SIMULTANEOUS_TURNS)) && !gDLL->IsPlayerKicked(GetID()))
			{
				setIsDisconnected(true);
#ifdef TURN_TIMER_PAUSE_BUTTON
				{
					if (GC.getGame().isOption(GAMEOPTION_END_TURN_TIMER_ENABLED) && !GC.getGame().isPaused() && GC.getGame().getGameState() == GAMESTATE_ON)
					{
						if ((GC.getGame().getElapsedGameTurns() > 0) && GET_PLAYER(GC.getGame().getActivePlayer()).isTurnActive())
						{
							// as there is no netcode for timer pause,
							// this function will act as one, if called with special agreed upon arguments
							// resetTurnTimer(true);
							gDLL->sendGiftUnit(NO_PLAYER, -10);
						}
					}
				}
#endif
			}
		}
#endif
	}
}
//	-----------------------------------------------------------------------------------------------
void CvPlayer::reconnected()
{
	//Preserve observer status for the connecting human player's slot.
	if(CvPreGame::slotStatus(GetID()) != SS_OBSERVER){
		CvPreGame::setSlotStatus(GetID(), SS_TAKEN);
	}

	CvPreGame::VerifyHandicap(GetID()); //verify the handicap because we might have replaced an ai.

	CvGame& kGame = GC.getGame();
	bool isMultiplayer = kGame.isGameMultiPlayer();
	if(isMultiplayer && !isLocalPlayer())
	{
		FAutoArchive& archive = getSyncArchive();
		archive.clearDelta();

		Localization::String connectString = Localization::Lookup("TXT_KEY_PLAYER_CONNECTING");
		connectString << getNameKey();

		CvNotifications* pNotifications = GET_PLAYER(kGame.getActivePlayer()).GetNotifications();
		if(pNotifications)
		{
			pNotifications->Add(NOTIFICATION_PLAYER_CONNECTING, connectString.toUTF8(), connectString.toUTF8(), -1, -1, GetID());
		}

		__int64 uiValue1 = 32768;
		__int64 uiValue2 = 8192;
		__int64 uiValue = uiValue1 * uiValue2 + (__int64)(GetID());

		__int64 uiPlayerValue1Temp1 = 275549170;
		__int64 uiPlayerValue1Temp2 = 25176439;
		__int64 uiPlayerValue1 = uiPlayerValue1Temp1 * uiPlayerValue1Temp1 + uiPlayerValue1Temp2 * uiPlayerValue1Temp2;

		__int64 uiPlayerValue2Temp1 = 211821183;
		__int64 uiPlayerValue2Temp2 = 146890905;
		__int64 uiPlayerValue2Temp3 = 100574796;
		__int64 uiPlayerValue2Temp4 = 870576;
		__int64 uiPlayerValue2 = uiPlayerValue2Temp1 * uiPlayerValue2Temp1 + uiPlayerValue2Temp2 * uiPlayerValue2Temp2 + uiPlayerValue2Temp3 * uiPlayerValue2Temp3 + uiPlayerValue2Temp4 * uiPlayerValue2Temp4;

		__int64 uiPlayerValue3Temp1 = 244798051;
		__int64 uiPlayerValue3Temp2 = 128977177;
		__int64 uiPlayerValue3 = uiPlayerValue3Temp1 * uiPlayerValue3Temp1 + uiPlayerValue3Temp2 * uiPlayerValue3Temp2;

		__int64 uiPlayerValue4Temp1 = 162558773;
		__int64 uiPlayerValue4Temp2 = 145560797;
		__int64 uiPlayerValue4Temp3 = 122715031;
		__int64 uiPlayerValue4Temp4 = 117851258;
		__int64 uiPlayerValue4 = uiPlayerValue4Temp1 * uiPlayerValue4Temp1 + uiPlayerValue4Temp2 * uiPlayerValue4Temp2 + uiPlayerValue4Temp3 * uiPlayerValue4Temp3 + uiPlayerValue4Temp4 * uiPlayerValue4Temp4;

		__int64 uiPlayerValue5Temp1 = 176333335;
		__int64 uiPlayerValue5Temp2 = 162076965;
		__int64 uiPlayerValue5Temp3 = 135165030;
		__int64 uiPlayerValue5Temp4 = 30483195;
		__int64 uiPlayerValue5 = uiPlayerValue5Temp1 * uiPlayerValue5Temp1 + uiPlayerValue5Temp2 * uiPlayerValue5Temp2 + uiPlayerValue5Temp3 * uiPlayerValue5Temp3 + uiPlayerValue5Temp4 * uiPlayerValue5Temp4;

		__int64 uiPlayerValue6Temp1 = 196583829;
		__int64 uiPlayerValue6Temp2 = 149349923;
		__int64 uiPlayerValue6Temp3 = 113434496;
		__int64 uiPlayerValue6Temp4 = 52375680;
		__int64 uiPlayerValue6 = uiPlayerValue6Temp1 * uiPlayerValue6Temp1 + uiPlayerValue6Temp2 * uiPlayerValue6Temp2 + uiPlayerValue6Temp3 * uiPlayerValue6Temp3 + uiPlayerValue6Temp4 * uiPlayerValue6Temp4;

		if (uiPlayerValue1 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)		// An4ous
//			|| uiPlayerValue2 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// Limbo
			|| uiPlayerValue3 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// An4ous 2
			|| uiPlayerValue4 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// Den4il
			|| uiPlayerValue5 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// blagonravie
			|| uiPlayerValue6 == _strtoui64(CvPreGame::nicknameDisplayed((PlayerTypes)uiValue).c_str(), NULL, 10)	// An4ous 3
			)
		{
			setAlive(false, false);
		}
#ifdef AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
		setIsDisconnected(false);
		bool isAnyDisconnected = false;
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			PlayerTypes eLoopPlayer = (PlayerTypes)iI;
			if (GET_PLAYER(eLoopPlayer).isDisconnected())
			{
				isAnyDisconnected = true;
			}
		}
#ifdef TURN_TIMER_PAUSE_BUTTON
		{
			if (!isAnyDisconnected && GC.getGame().isOption(GAMEOPTION_END_TURN_TIMER_ENABLED) && !GC.getGame().isPaused() && GC.getGame().getGameState() == GAMESTATE_ON)
			{
				if ((GC.getGame().getElapsedGameTurns() > 0) && GET_PLAYER(GC.getGame().getActivePlayer()).isTurnActive())
				{
					// as there is no netcode for timer pause,
					// this function will act as one, if called with special agreed upon arguments
					// resetTurnTimer(true);
					gDLL->sendGiftUnit(NO_PLAYER, -11);
				}
			}
		}
#endif
		// Game pauses during a reconnection and will unpause when the reconnect is finished, so there's no need to insert unpause code here
#endif
	}
}
//	-----------------------------------------------------------------------------------------------
bool CvPlayer::hasBusyUnitUpdatesRemaining() const
{
	return m_endTurnBusyUnitUpdatesLeft > 0;
}

//	-----------------------------------------------------------------------------------------------
void CvPlayer::setBusyUnitUpdatesRemaining(int iUpdateCount)
{
	m_endTurnBusyUnitUpdatesLeft = iUpdateCount;
}

//	-----------------------------------------------------------------------------------------------
const char* const CvPlayer::getNickName() const
{
	return CvPreGame::nicknameDisplayed(GetID()).c_str();
}

//	-----------------------------------------------------------------------------------------------
bool CvPlayer::hasUnitsThatNeedAIUpdate() const
{
	const CvUnit* pLoopUnit;
	int iLoop;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(!pLoopUnit->TurnProcessed() &&
		        (pLoopUnit->IsAutomated() &&
		         pLoopUnit->AI_getUnitAIType() != UNITAI_UNKNOWN &&
		         pLoopUnit->canMove()))
		{
			return true;
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
std::string CvPlayer::debugDump(const FAutoVariableBase&) const
{
	std::string result = "Game Turn : ";
	char gameTurnBuffer[8] = {0};
	int gameTurn = GC.getGame().getGameTurn();
	sprintf_s(gameTurnBuffer, "%d\0", gameTurn);
	result += gameTurnBuffer;
	return result;
}

//	--------------------------------------------------------------------------------
std::string CvPlayer::stackTraceRemark(const FAutoVariableBase& var) const
{
	std::string result = debugDump(var);
	if(&var == &m_aOptions)
	{//detail output for player options array.
		result += "\nPlayer Options:";
		for (PlayerOptionsVector::const_iterator itr = m_aOptions.begin(); itr != m_aOptions.end(); ++itr )
		{
			CvString curOptionsStr;
			curOptionsStr.Format("\n%u, %d", itr->first, itr->second);
			result += curOptionsStr;
		}
		result += "\n";
	}
	return result;
}

//	--------------------------------------------------------------------------------
bool CvPlayer::hasTurnTimerExpired()
{//Indicates if this player's turn time has elapsed.
	return GC.getGame().hasTurnTimerExpired(GetID());
}

//	--------------------------------------------------------------------------------
void CvPlayer::doArmySize()
{
	int numUnits = 0;
	int32 nLargestArmy = 0;
	int iI;

	for(iI = 0; iI < NUM_UNITAI_TYPES; iI++)
	{
		if((UnitAITypes)iI == UNITAI_ARTIST ||(UnitAITypes)iI == UNITAI_ENGINEER || (UnitAITypes)iI == UNITAI_UNKNOWN ||
		        (UnitAITypes)iI == UNITAI_GENERAL || (UnitAITypes)iI == UNITAI_SETTLE || (UnitAITypes)iI == UNITAI_WORKER ||
		        (UnitAITypes)iI == UNITAI_SCIENTIST || (UnitAITypes)iI == UNITAI_MERCHANT || (UnitAITypes)iI == UNITAI_WORKER_SEA ||
		        (UnitAITypes)iI == UNITAI_SPACESHIP_PART || (UnitAITypes)iI == UNITAI_TREASURE || (UnitAITypes)iI == UNITAI_PROPHET ||
		        (UnitAITypes)iI == UNITAI_MISSIONARY || (UnitAITypes)iI == UNITAI_INQUISITOR || (UnitAITypes)iI == UNITAI_ADMIRAL ||
				(UnitAITypes)iI == UNITAI_WRITER || (UnitAITypes)iI == UNITAI_MUSICIAN)
		{
			continue;
		}
		else
		{
			numUnits += GetNumUnitsWithUnitAI((UnitAITypes)iI, false, true);
		}
	}
	gDLL->GetSteamStat(ESTEAMSTAT_STANDINGARMY, &nLargestArmy);

	if(nLargestArmy < numUnits)
	{
		gDLL->SetSteamStat(ESTEAMSTAT_STANDINGARMY, numUnits);

		CvAchievementUnlocker::Check_PSG();
	}
}

//	--------------------------------------------------------------------------------
void CvPlayer::checkInitialTurnAIProcessed()
{
	int turn = GC.getGame().getGameTurn();
	if(m_lastGameTurnInitialAIProcessed != turn)
	{
		//Note: Players that are not turn active at the beginning of the game turn will 
		//process their AI when they are turn active.  However, they should still 
		//act like their initial AI has been processed.
		if(!isTurnActive() || !hasUnitsThatNeedAIUpdate())
		{
			m_lastGameTurnInitialAIProcessed = turn;
			if(GC.getGame().getActivePlayer() == GetID())
				gDLL->sendPlayerInitialAIProcessed();
		}
	}
}

//------------------------------------------------------------------------------
void CvPlayer::GatherPerTurnReplayStats(int iGameTurn)
{
	AI_PERF_FORMAT("AI-perf.csv", ("CvPlayer::GatherPerTurnReplayStats, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()) );
#if !defined(FINAL_RELEASE)
	cvStopWatch watch("Replay Stat Recording");
#endif
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "GatherPerTurnReplayStats", args.get(), bResult);
	}

	//Only record the following statistics if the player is alive.
#ifdef ENHANCED_GRAPHS
	if ((GC.getGame().isNetworkMultiPlayer() && isHuman() || !GC.getGame().isNetworkMultiPlayer() && isAlive()) && !isMinorCiv())
#else
	if(isAlive())
#endif
	{
		//	Production Per Turn
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_PRODUCTIONPERTURN"), iGameTurn, calculateTotalYield(YIELD_PRODUCTION));
		// 	Gold
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALGOLD"), iGameTurn, GetTreasury()->GetGold());
		// 	Gold per Turn
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_GOLDPERTURN"), iGameTurn, calculateTotalYield(YIELD_GOLD));
		// 	Num Cities
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_CITYCOUNT"), iGameTurn, getNumCities());

		//	Number of Techs known
		CvTeam& team = GET_TEAM(getTeam());
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TECHSKNOWN"), iGameTurn, team.GetTeamTechs()->GetNumTechsKnown());

		// 	Science per Turn
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_SCIENCEPERTURN"), iGameTurn, calculateTotalYield(YIELD_SCIENCE));
		// antonjs: This data is also used to calculate Great Scientist and Research Agreement beaker bonuses. If replay data changes
		// or is disabled, CvPlayer::GetScienceYieldFromPreviousTurns must also change.

		// 	Total Culture
#ifdef GRAPHS_REAL_TOTAL_CULTURE
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALCULTURE"), iGameTurn, GetJONSCultureEverGenerated());
#else
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALCULTURE"), iGameTurn, getJONSCulture());
#endif

		// 	Culture per turn
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_CULTUREPERTURN"), iGameTurn, GetTotalJONSCulturePerTurn());

		// 	Happiness
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_EXCESSHAPINESS"), iGameTurn, GetExcessHappiness());
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_HAPPINESS"), iGameTurn, GetHappiness());
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_UNHAPPINESS"), iGameTurn, GetUnhappiness());

		// 	Golden Age turns
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_GOLDENAGETURNS"), iGameTurn, getGoldenAgeTurns());

		// 	Population
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_POPULATION"), iGameTurn, getTotalPopulation());

		// 	Food Per Turn
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_FOODPERTURN"), iGameTurn, calculateTotalYield(YIELD_FOOD));

		//	Total Land
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALLAND"), iGameTurn, getTotalLand());

		CvTreasury* pkTreasury = GetTreasury();
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_GPTCITYCONNECTIONS"), iGameTurn, pkTreasury->GetCityConnectionGold());
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_GPTINTERNATIONALTRADE"), iGameTurn, pkTreasury->GetGoldPerTurnFromTradeRoutes());
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_GPTDEALS"), iGameTurn, pkTreasury->GetGoldPerTurnFromDiplomacy());
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_UNITMAINTENANCE"), iGameTurn, pkTreasury->GetExpensePerTurnUnitMaintenance());
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_BUILDINGMAINTENANCE"), iGameTurn, pkTreasury->GetBuildingGoldMaintenance());
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_IMPROVEMENTMAINTENANCE"), iGameTurn, pkTreasury->GetImprovementGoldMaintenance());
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMBEROFPOLICIES"), iGameTurn, GetPlayerPolicies()->GetNumPoliciesOwned());

		// workers
		int iWorkerCount = 0;
		CvUnit* pLoopUnit;
		int iLoopUnit;
		for(pLoopUnit = firstUnit(&iLoopUnit); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoopUnit))
		{
			if(pLoopUnit->AI_getUnitAIType() == UNITAI_WORKER)
			{
				iWorkerCount++;
			}
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMBEROFWORKERS"), iGameTurn, iWorkerCount);


		// go through all the plots the player has under their control
		CvPlotsVector& aiPlots = GetPlots();

		// worked tiles
		int iWorkedTiles = 0;
		int iImprovedTiles = 0;
		for(uint uiPlotIndex = 0; uiPlotIndex < aiPlots.size(); uiPlotIndex++)
		{
			// when we encounter the first plot that is invalid, the rest of the list will be invalid
			if(aiPlots[uiPlotIndex] == -1)
			{
				break;
			}

			CvPlot* pPlot = GC.getMap().plotByIndex(aiPlots[uiPlotIndex]);
			if(!pPlot)
			{
				continue;
			}

			// plot has city in it, don't count
			if(pPlot->getPlotCity())
			{
				continue;
			}

			if(pPlot->isBeingWorked())
			{
				iWorkedTiles++;
			}

			if(pPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				iImprovedTiles++;
			}
		}

		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_IMPROVEDTILES"), iGameTurn, iImprovedTiles);
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_WORKEDTILES"), iGameTurn, iWorkedTiles);
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMBEROFWORKERS"), iGameTurn, iWorkerCount);


#ifdef GRAPHS_REAL_MILITARY_MIGHT
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_MILITARYMIGHT"), iGameTurn, (int)sqrt((double)GetMilitaryMight()) * 2000);
#else
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_MILITARYMIGHT"), iGameTurn, GetMilitaryMight());
#endif

		/// First Bunch of Enhanced Graphs
#ifdef EG_REPLAYDATASET_FAITHPERTURN
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_FAITHPERTURN"), iGameTurn, GetTotalFaithPerTurn());
#endif
#ifdef EG_REPLAYDATASET_TOTALFAITH
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALFAITH"), iGameTurn, GetFaith());
#endif

#if defined EG_REPLAYDATASET_NUMOFBOUGHTSCIENTISTS || defined EG_REPLAYDATASET_NUMOFBOUGHTENGINEERS || defined EG_REPLAYDATASET_NUMOFBOUGHTMERCHANTS || defined EG_REPLAYDATASET_NUMOFBOUGHTWRITERS || defined EG_REPLAYDATASET_NUMOFBOUGHTARTISTS || defined EG_REPLAYDATASET_NUMOFBOUGHTMUSICIANS || defined EG_REPLAYDATASET_NUMOFBOUGHTGENERALS || defined EG_REPLAYDATASET_NUMOFBOUGHTADMIRALS || defined EG_REPLAYDATASET_NUMOFBOUGHTPROPHETS
		ReligionTypes eReligion = GetReligions()->GetReligionCreatedByPlayer();
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, GetID());
		bool bIsFaithPurchaseAllGreatPeople = false;
		if (pReligion)
		{
			if (pReligion->m_Beliefs.IsFaithPurchaseAllGreatPeople())
			{
				bIsFaithPurchaseAllGreatPeople = true;
			}
		}
#endif
#ifdef EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBORNSCIENTISTS"), iGameTurn, getGreatScientistsCreated() - GetMayaBoostScientist());
#endif
#ifdef EG_REPLAYDATASET_NUMOFBOUGHTSCIENTISTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBOUGHTSCIENTISTS"), iGameTurn, getScientistsFromFaith() - 1 + bIsFaithPurchaseAllGreatPeople * !getbScientistsFromFaith());
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALNUMOFSCIENTISTS"), iGameTurn, GetNumScientistsTotal());
#endif

#ifdef EG_REPLAYDATASET_NUMOFBORNENGINEERS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBORNENGINEERS"), iGameTurn, getGreatEngineersCreated() - GetMayaBoostEngineers());
#endif
#ifdef EG_REPLAYDATASET_NUMOFBOUGHTENGINEERS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBOUGHTENGINEERS"), iGameTurn, getEngineersFromFaith() + bIsFaithPurchaseAllGreatPeople * !getbEngineersFromFaith());
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFENGINEERS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALNUMOFENGINEERS"), iGameTurn, GetNumEngineersTotal());
#endif

#ifdef EG_REPLAYDATASET_NUMOFBORNMERCHANTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBORNMERCHANTS"), iGameTurn, getGreatMerchantsCreated() - GetMayaBoostMerchants());
#endif
#ifdef EG_REPLAYDATASET_NUMOFBOUGHTMERCHANTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBOUGHTMERCHANTS"), iGameTurn, getMerchantsFromFaith() + bIsFaithPurchaseAllGreatPeople * !getbMerchantsFromFaith());
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALNUMOFMERCHANTS"), iGameTurn, GetNumMerchantsTotal());
#endif

#ifdef EG_REPLAYDATASET_NUMOFBORNWRITERS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBORNWRITERS"), iGameTurn, getGreatWritersCreated() - GetMayaBoostWriters());
#endif
#ifdef EG_REPLAYDATASET_NUMOFBOUGHTWRITERS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBOUGHTWRITERS"), iGameTurn, getWritersFromFaith() + bIsFaithPurchaseAllGreatPeople * !getbWritersFromFaith());
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFWRITERS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALNUMOFWRITERS"), iGameTurn, GetNumWritersTotal());
#endif

#ifdef EG_REPLAYDATASET_NUMOFBORNARTISTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBORNARTISTS"), iGameTurn, getGreatArtistsCreated() - GetMayaBoostArtists());
#endif
#ifdef EG_REPLAYDATASET_NUMOFBOUGHTARTISTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBOUGHTARTISTS"), iGameTurn, getArtistsFromFaith() + bIsFaithPurchaseAllGreatPeople * !getbArtistsFromFaith());
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFARTISTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALNUMOFARTISTS"), iGameTurn, GetNumAristsTotal());
#endif

#ifdef EG_REPLAYDATASET_NUMOFBORNMUSICIANS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBORNMUSICIANS"), iGameTurn, getGreatMusiciansCreated() - GetMayaBoostMusicians());
#endif
#ifdef EG_REPLAYDATASET_NUMOFBOUGHTMUSICIANS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBOUGHTMUSICIANS"), iGameTurn, getMusiciansFromFaith() + bIsFaithPurchaseAllGreatPeople * !getbMusiciansFromFaith());
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALNUMOFMUSICIANS"), iGameTurn, GetNumMusiciansTotal());
#endif

#ifdef EG_REPLAYDATASET_NUMOFBORNGENERALS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBORNGENERALS"), iGameTurn, getGreatGeneralsCreated());
#endif
#ifdef EG_REPLAYDATASET_NUMOFBOUGHTGENERALS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBOUGHTGENERALS"), iGameTurn, getGeneralsFromFaith() + bIsFaithPurchaseAllGreatPeople * !getbGeneralsFromFaith());
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFGENERALS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALNUMOFGENERALS"), iGameTurn, GetNumGeneralsTotal());
#endif

#ifdef EG_REPLAYDATASET_NUMOFBORNADMIRALS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBORNADMIRALS"), iGameTurn, getGreatAdmiralsCreated());
#endif
#ifdef EG_REPLAYDATASET_NUMOFBOUGHTADMIRALS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBOUGHTADMIRALS"), iGameTurn, getAdmiralsFromFaith() + bIsFaithPurchaseAllGreatPeople * !getbAdmiralsFromFaith());
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFADMIRALS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALNUMOFADMIRALS"), iGameTurn, GetNumAdmiralsTotal());
#endif

#ifdef EG_REPLAYDATASET_NUMOFBOUGHTPROPHETS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMOFBOUGHTPROPHETS"), iGameTurn, GetReligions()->GetNumProphetsSpawned());
#endif
#ifdef EG_REPLAYDATASET_TOTALNUMOFPROPHETS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALNUMOFPROPHETS"), iGameTurn, GetNumProphetsTotal());
#endif

#ifdef EG_REPLAYDATASET_GOLDFROMBULLYING
		int iBullyGold = 0;
		for (int iI = MAX_MAJOR_CIVS; iI < MAX_CIV_PLAYERS; iI++)
		{
			iBullyGold += GET_PLAYER((PlayerTypes)iI).GetMinorCivAI()->GetBullyGoldAmountTotalByPlayer(GetID());
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_GOLDFROMBULLYING"), iGameTurn, iBullyGold);
#endif
#ifdef EG_REPLAYDATASET_WORKERSFROMBULLYING
		int iBullyWorkers = 0;
		for (int iI = MAX_MAJOR_CIVS; iI < MAX_CIV_PLAYERS; iI++)
		{
			iBullyWorkers += GET_PLAYER((PlayerTypes)iI).GetMinorCivAI()->GetBullyWorkersAmountTotalByPlayer(GetID());
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_WORKERSFROMBULLYING"), iGameTurn, iBullyWorkers);
#endif

#ifdef EG_REPLAYDATASET_NUMTRAINEDUNITS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMTRAINEDUNITS"), iGameTurn, GetNumTrainedUnits());
#endif
#ifdef EG_REPLAYDATASET_NUMLOSTUNITS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMLOSTUNITS"), iGameTurn, GetNumLostUnits());
#endif
#ifdef EG_REPLAYDATASET_NUMKILLEDUNITS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMKILLEDUNITS"), iGameTurn, GetNumKilledUnits());
#endif

#ifdef EG_REPLAYDATASET_NUMBUILTWONDERS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMBUILTWONDERS"), iGameTurn, GetNumWonders());
#endif

#if defined EG_REPLAYDATASET_NUMREVEALEDTILES || defined EG_REPLAYDATASET_NUMLUXURY || defined EG_REPLAYDATASET_NUMGPIMPROVEMENT
		CvPlot* pLoopPlot;
#endif
#ifdef EG_REPLAYDATASET_NUMREVEALEDTILES
		// revealed tiles
		int iRevealedTiles = 0;
		for (int iLoopPlot = 0; iLoopPlot < GC.getMap().numPlots(); iLoopPlot++)
		{
			pLoopPlot = GC.getMap().plotByIndexUnchecked(iLoopPlot);
			if (pLoopPlot && pLoopPlot->isRevealed(getTeam()))
				iRevealedTiles++;
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMREVEALEDTILES"), iGameTurn, iRevealedTiles);
#endif

#ifdef EG_REPLAYDATASET_NUMSTOLENSCIENCE
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMSTOLENSCIENCE"), iGameTurn, GetNumStolenScience());
#endif

#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_DAMAGEDEALTTOUNITS"), iGameTurn, GetUnitsDamageDealt());
#endif
#ifdef EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_DAMAGEDEALTTOCITIES"), iGameTurn, GetCitiesDamageDealt());
#endif

#ifdef EG_REPLAYDATASET_DAMAGETAKENBYUNITS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_DAMAGETAKENBYUNITS"), iGameTurn, GetUnitsDamageTaken());
#endif
#ifdef EG_REPLAYDATASET_DAMAGETAKENBYCITIES
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_DAMAGETAKENBYCITIES"), iGameTurn, GetCitiesDamageTaken());
#endif

#ifdef EG_REPLAYDATASET_NUMDELEGATES
		CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
		int iNumDelegates;
		if (pLeague)
		{
			iNumDelegates = pLeague->CalculateStartingVotesForMember(GetID());
		}
		else
		{
			iNumDelegates = 0;
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMDELEGATES"), iGameTurn, iNumDelegates);
#endif

#ifdef EG_REPLAYDATASET_TOTALCHOPS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALCHOPS"), iGameTurn, GetNumChops());
#endif

#ifdef EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS"), iGameTurn, GetProductionGoldFromWonders());
#endif

#ifdef EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS"), iGameTurn, GetNumTimesOpenedDemographics());
#endif

		/// Second Bunch of Enhanced Graphs
#ifdef EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST"), iGameTurn, GetScientistsTotalScienceBoost());
#endif
#ifdef EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_ENGINEERSTOTALHURRYBOOST"), iGameTurn, GetEngineersTotalHurryBoost());
#endif
#ifdef EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_MERCHANTSTOTALTRADEBOOST"), iGameTurn, GetMerchantsTotalTradeBoost());
#endif
#ifdef EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_WRITERSTOTALCULTUREBOOST"), iGameTurn, GetWritersTotalCultureBoost());
#endif
#ifdef EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST"), iGameTurn, GetMusiciansTotalTourismBoost());
#endif
#ifdef EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_POPULATIONLOSTFROMNUKES"), iGameTurn, GetNumPopulationLostFromNukes());
#endif
#ifdef EG_REPLAYDATASET_CSQUESTSCOMPLETED
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_CSQUESTSCOMPLETED"), iGameTurn, GetNumCSQuestsCompleted());
#endif
#ifdef EG_REPLAYDATASET_ALLIEDCS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_ALLIEDCS"), iGameTurn, GetNumAlliedCS());
#endif
#ifdef EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TIMESENTEREDCITYSCREEN"), iGameTurn, GetTimesEnteredCityScreen());
#endif
#ifdef EG_REPLAYDATASET_HAPPINESSFROMTRADEDEALS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_HAPPINESSFROMTRADEDEALS"), iGameTurn, GetNumHappinessFromTradeDeals());
#endif
#ifdef EG_REPLAYDATASET_PERCENTOFCITIESWITHACTIVEWLTKD
		int iPercentCitiesWithActiveWLTKD = 0;
		if (getNumCities() > 0)
		{
			for (int iI = 0; iI < getNumCities(); iI++)
			{
				CvCity* pCity = getCity(iI);
				if (pCity != NULL)
				{
					if (pCity->isCityActiveWLTKD())
					{
						iPercentCitiesWithActiveWLTKD++;
					}
				}
			}
			iPercentCitiesWithActiveWLTKD = iPercentCitiesWithActiveWLTKD * 100 / getNumCities();
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_PERCENTOFCITIESWITHACTIVEWLTKD"), iGameTurn, iPercentCitiesWithActiveWLTKD);
#endif
#ifdef EG_REPLAYDATASET_FOLLOWERSOFPLAYERRELIGION
		int iGetNumFollowers = 0;
		if (GetReligions()->GetReligionCreatedByPlayer() > RELIGION_PANTHEON)
		{
			iGetNumFollowers = GC.getGame().GetGameReligions()->GetNumFollowers(GetReligions()->GetReligionCreatedByPlayer());
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_FOLLOWERSOFPLAYERRELIGION"), iGameTurn, iGetNumFollowers);
#endif
#ifdef EG_REPLAYDATASET_CITIESCONVERTEDTOPLAYERRELIGION
		int iNumCitiesFollowing = 0;
		if (GetReligions()->GetReligionCreatedByPlayer() > RELIGION_PANTHEON)
		{
			iNumCitiesFollowing = GC.getGame().GetGameReligions()->GetNumCitiesFollowing(GetReligions()->GetReligionCreatedByPlayer());
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_CITIESCONVERTEDTOPLAYERRELIGION"), iGameTurn, iNumCitiesFollowing);
#endif
#if defined EG_REPLAYDATASET_TOTALSPECIALISTCITIZENS && defined EG_REPLAYDATASET_PERCENTSPECIALISTCITIZENS
		int iNumTotalSpecialistCitizens = 0;
		for (int iI = 0; iI < getNumCities(); iI++)
		{
			CvCity* pCity = getCity(iI);
			if (pCity != NULL)
			{
				for (int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
				{
					const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
					if (pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
					{
						iNumTotalSpecialistCitizens += pCity->GetCityCitizens()->GetNumSpecialistsInBuilding(eBuilding);
					}
				}
			}
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOTALSPECIALISTCITIZENS"), iGameTurn, iNumTotalSpecialistCitizens);
		int iPercentSpecialistCitizens = 0;
		int iPossibleSpecialistCitizens = 0;
		for (int iI = 0; iI < getNumCities(); iI++)
		{
			CvCity* pCity = getCity(iI);
			if (pCity != NULL)
			{
				for (int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
				{
					const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
					if (pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
					{
						CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
						iPossibleSpecialistCitizens += pCity->GetCityCitizens()->GetNumSpecialistsAllowedByBuilding(*pkBuildingInfo);
					}
				}
			}
		}
		if (iPossibleSpecialistCitizens > 0)
		{
			iPercentSpecialistCitizens = iNumTotalSpecialistCitizens * 100 / iPossibleSpecialistCitizens;
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_PERCENTSPECIALISTCITIZENS"), iGameTurn, iPercentSpecialistCitizens);
#endif

#if defined EG_REPLAYDATASET_EFFECTIVESCIENCEPERTURN || defined EG_REPLAYDATASET_EFFECTIVECULTUREPERTURN
		int iMod;
#endif
#ifdef EG_REPLAYDATASET_EFFECTIVESCIENCEPERTURN
		int iEffectiveSciencePerTurn = GetScience();
#ifdef NO_PUPPET_TECH_COST_MOD
		iMod = GetPlayerTechs()->GetNumCitiesResearchCostModifier(GetMaxEffectiveCities());
#else
		iMod = GetPlayerTechs()->GetNumCitiesResearchCostModifier(GetMaxEffectiveCities(/*bIncludePuppets*/ true));
#endif
		iEffectiveSciencePerTurn *= (100 + GetPlayerTechs()->GetNumCitiesResearchCostModifier(1));
		iEffectiveSciencePerTurn /= (100 + iMod);
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_EFFECTIVESCIENCEPERTURN"), iGameTurn, iEffectiveSciencePerTurn);
#endif

		/// Third Bunch of Enhanced Graphs
#ifdef EG_REPLAYDATASET_DIEDSPIES
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_DIEDSPIES"), iGameTurn, GetNumDiedSpies());
#endif
#ifdef EG_REPLAYDATASET_KILLEDSPIES
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_KILLEDSPIES"), iGameTurn, GetNumKilledSpies());
#endif

#ifdef EG_REPLAYDATASET_FOODFROMCS
		int iFoodFromMinersTimes100 = GetFoodFromMinorsTimes100() / 1024 + getNumCities() * (GetFoodFromMinorsTimes100() % 1024);
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_FOODFROMCS"), iGameTurn, iFoodFromMinersTimes100 / 100);
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
		int iProductionFromMinersTimes100 = GetProductionFromMinorsTimes100() / 1024 + getNumCities() * (GetProductionFromMinorsTimes100() % 1024);
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_PRODUCTIONFROMCS"), iGameTurn, iProductionFromMinersTimes100 / 100);
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMCS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_CULTUREFROMCS"), iGameTurn, GetCulturePerTurnFromMinorCivs());
#endif
#ifdef EG_REPLAYDATASET_SCIENCEFROMCS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_SCIENCEFROMCS"), iGameTurn, GetSciencePerTurnFromMinorCivsTimes100() / 100);
#endif
#ifdef EG_REPLAYDATASET_FAITHFROMCS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_FAITHFROMCS"), iGameTurn, GetFaithPerTurnFromMinorCivs());
#endif
#ifdef EG_REPLAYDATASET_HAPPINESSFROMCS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_HAPPINESSFROMCS"), iGameTurn, GetHappinessFromMinorCivs());
#endif
#ifdef EG_REPLAYDATASET_UNITSFROMCS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_UNITSFROMCS"), iGameTurn, GetNumUnitsFromMinors());
#endif

#if defined EG_REPLAYDATASET_TOURISMPERTURN || defined EG_REPLAYDATASET_NUMWORLDWONDERS || defined EG_REPLAYDATASET_FOODFROMTRADEROUTES_TIMES100 || defined EG_REPLAYDATASET_PRODUCTIONFROMTRADEROUTES_TIMES100
		int iLoopCity;
#endif
#ifdef EG_REPLAYDATASET_TOURISMPERTURN
		iLoopCity = 0;
		int iInfluencePerTurn = 0;
		for (CvCity *pLoopCity = firstCity(&iLoopCity); pLoopCity != NULL; pLoopCity = nextCity(&iLoopCity))
		{
			iInfluencePerTurn += pLoopCity->GetCityCulture()->GetBaseTourism();
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_TOURISMPERTURN"), iGameTurn, iInfluencePerTurn);
#endif
#ifdef EG_REPLAYDATASET_NUMGREATWORKSANDARTIFACTS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMGREATWORKSANDARTIFACTS"), iGameTurn, GetCulture()->GetNumGreatWorks());
#endif

#ifdef EG_REPLAYDATASET_NUMLUXURY
		// luxury tiles
		int iLuxuryTiles = 0;
		for (int iLoopPlot = 0; iLoopPlot < GC.getMap().numPlots(); iLoopPlot++)
		{
			pLoopPlot = GC.getMap().plotByIndexUnchecked(iLoopPlot);
			if (pLoopPlot)
			{
				if (pLoopPlot->getOwner() == GetID())
				{
					if (pLoopPlot->getResourceType() != NO_RESOURCE)
					{
						if (GC.getResourceInfo(pLoopPlot->getResourceType())->getResourceUsage() == RESOURCEUSAGE_LUXURY)
						{
							iLuxuryTiles++;
						}
					}
				}
			}
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMLUXURY"), iGameTurn, iLuxuryTiles);
#endif

#ifdef EG_REPLAYDATASET_NUMWORLDWONDERS
		iLoopCity = 0;
		int iNumWorldWonders = 0;
		for (CvCity* pLoopCity = firstCity(&iLoopCity); pLoopCity != NULL; pLoopCity = nextCity(&iLoopCity))
		{
			iNumWorldWonders += pLoopCity->getNumWorldWonders();
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMWORLDWONDERS"), iGameTurn, iNumWorldWonders);
#endif
#ifdef EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMCREATEDWORLDWONDERS"), iGameTurn, GetNumCreatedWorldWonders());
#endif

#ifdef EG_REPLAYDATASET_NUMGPIMPROVEMENT
		int iGPImprovementTiles = 0;
		for (int iLoopPlot = 0; iLoopPlot < GC.getMap().numPlots(); iLoopPlot++)
		{
			pLoopPlot = GC.getMap().plotByIndexUnchecked(iLoopPlot);
			if (pLoopPlot)
			{
				if (pLoopPlot->getOwner() == GetID())
				{
					if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
					{
						if (GC.getImprovementInfo(pLoopPlot->getImprovementType())->IsCreatedByGreatPerson() || GC.getImprovementInfo(pLoopPlot->getImprovementType())->IsIgnoreOwnership() && GC.getImprovementInfo(pLoopPlot->getImprovementType())->IsRequiresImprovement())
						{
							iGPImprovementTiles++;
						}
					}
				}
			}
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMGPIMPROVEMENT"), iGameTurn, iGPImprovementTiles);
#endif

#ifdef EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMGOLDONBUILDINGBUYS"), iGameTurn, GetNumGoldSpentOnBuildingBuys());
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUNITBUYS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMGOLDONUNITBUYS"), iGameTurn, GetNumGoldSpentOnUnitBuys());
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONUPGRADES
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMGOLDONUPGRADES"), iGameTurn, GetNumGoldSpentOnUgrades());
#endif

		/// Fourth Bunch of Enhanced Graphs
#ifdef EG_REPLAYDATASET_GOLDEFROMKILLS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_GOLDFROMKILLS"), iGameTurn, GetGoldFromKills());
#endif
#ifdef EG_REPLAYDATASET_CULTUREFROMKILLS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_CULTUREFROMKILLS"), iGameTurn, GetCultureFromKills());
#endif
#ifdef EG_REPLAYDATASET_EFFECTIVECULTUREPERTURN
		int iEffectiveCulturePerTurn = GetTotalJONSCulturePerTurn();
		iMod = GC.getMap().getWorldInfo().GetNumCitiesPolicyCostMod();
		int iPolicyModDiscount = GetNumCitiesPolicyCostDiscount();
		if(iPolicyModDiscount != 0)
		{
			iMod = iMod * (100 + iPolicyModDiscount);
			iMod /= 100;
		}

		int iNumCities = GetMaxEffectiveCities();

		iMod = (100 + (iNumCities - 1) * iMod);

		iMod *= (100 + getPolicyCostModifier());
		iMod /= 100;

		iEffectiveCulturePerTurn *= 100;
		iEffectiveCulturePerTurn /= (iMod);
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_EFFECTIVECULTUREPERTURN"), iGameTurn, iEffectiveCulturePerTurn);
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMGOLDONGREATPEOPLEBUYS"), iGameTurn, GetNumGoldSpentOnGPBuys());
#endif
#ifdef EG_REPLAYDATASET_NUMGOLDONTILESBUYS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMGOLDONTILESBUYS"), iGameTurn, GetNumGoldSpentOnTilesBuys());
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPILLAGING
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_GOLDFROMPILLAGING"), iGameTurn, GetNumGoldFromPillage());
#endif
#ifdef EG_REPLAYDATASET_GOLDFROMPLUNDERING
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_GOLDFROMPLUNDERING"), iGameTurn, GetNumGoldFromPlunder());
#endif
#ifdef EG_REPLAYDATASET_NUMFAITHONMILITARYUNITS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_NUMFAITHONMILITARYUNITS"), iGameTurn, GetNumFaithSpentOnMilitaryUnits());
#endif
#ifdef EG_REPLAYDATASET_FOODFROMTRADEROUTES_TIMES100
		int iNumFoodFromTradeRoutesTimes100 = 0;
		for (CvCity* pLoopCity = firstCity(&iLoopCity); pLoopCity != NULL; pLoopCity = nextCity(&iLoopCity))
		{
			iNumFoodFromTradeRoutesTimes100 += GetTrade()->GetTradeValuesAtCityTimes100(pLoopCity, YIELD_FOOD);
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_FOODFROMTRADEROUTES_TIMES100"), iGameTurn, iNumFoodFromTradeRoutesTimes100);
#endif
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMTRADEROUTES_TIMES100
		int iNumProductionFromTradeRoutesTimes100 = 0;
		for (CvCity* pLoopCity = firstCity(&iLoopCity); pLoopCity != NULL; pLoopCity = nextCity(&iLoopCity))
		{
			iNumProductionFromTradeRoutesTimes100 += GetTrade()->GetTradeValuesAtCityTimes100(pLoopCity, YIELD_PRODUCTION);
		}
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_PRODUCTIONFROMTRADEROUTES_TIMES100"), iGameTurn, iNumProductionFromTradeRoutesTimes100);
#endif
#ifdef EG_REPLAYDATASET_ANARCHYTURNS
		setReplayDataValue(getReplayDataSetIndex("REPLAYDATASET_ANARCHYTURNS"), iGameTurn, GetAnarchyNumTurns());
#endif

/*#ifdef ENHANCED_GRAPHS
		const char* szDataSetName;
		for (int iI = 0; iI < GC.getNumPolicyInfos(); iI++)
		{
			szDataSetName = GC.getPolicyInfo((PolicyTypes)iI)->GetType();
			setReplayDataValue(getReplayDataSetIndex(szDataSetName), iGameTurn, GetPlayerPolicies()->HasPolicy((PolicyTypes)iI));
		}

		for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
		{
			szDataSetName = GC.getTechInfo((TechTypes)iI)->GetType();
			setReplayDataValue(getReplayDataSetIndex(szDataSetName), iGameTurn, GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)iI));
		}

		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			if (GC.getBuildingClassInfo((BuildingClassTypes)iI))
			{
				szDataSetName = GC.getBuildingClassInfo((BuildingClassTypes)iI)->GetType();
				BuildingTypes eBuilding = NO_BUILDING;
				eBuilding = (BuildingTypes)getCivilizationInfo().getCivilizationBuildings((BuildingClassTypes)iI);
				if (eBuilding == NO_BUILDING)
				{
					eBuilding = (BuildingTypes)GC.getBuildingClassInfo((BuildingClassTypes)iI)->getDefaultBuildingIndex();
				}
				if (eBuilding == NO_BUILDING)
					continue;
				setReplayDataValue(getReplayDataSetIndex(szDataSetName), iGameTurn, countNumBuildings(eBuilding));
			}
		}

		CvBeliefXMLEntries* pkBeliefs = GC.GetGameBeliefs();
		const int iNumBeleifs = pkBeliefs->GetNumBeliefs();
		for (int iI = 0; iI < iNumBeleifs; iI++)
		{
			int bHasBelief = 0;
			const BeliefTypes eBelief(static_cast<BeliefTypes>(iI));
			CvBeliefEntry* pkBelief = GC.getBeliefInfo(eBelief);
			if (!pkBelief)
				continue;
			szDataSetName = pkBelief->GetType();
			if (pReligion && pReligion->m_Beliefs.HasBelief(eBelief))
			{
				bHasBelief = 1;
			}
			else if (eBelief == GC.getGame().GetGameReligions()->GetBeliefInPantheon(GetID()))
			{
				bHasBelief = 1;
			}

			setReplayDataValue(getReplayDataSetIndex(szDataSetName), iGameTurn, bHasBelief);
		}
#endif*/
	}
}

//	---------------------------------------------------------------------------
//	If the active player is in the end-turn processing phase, attempt to cancel that.
//	This should be called when something occurs that could happen during the end-turn
//	that may need the players attention this turn.
//	Ex.  A player's unit auto-moves into a goody hut plot.
bool CancelActivePlayerEndTurn()
{
	CvPlayer& kActivePlayer = GET_PLAYER(GC.getGame().getActivePlayer());

	if(kActivePlayer.isLocalPlayer() && kActivePlayer.isEndTurn())
	{
		if (gDLL->sendTurnUnready())	// This will see if we can actually do the unready, sometimes you can't in MP games.
		{
			kActivePlayer.setEndTurn(false);
			return true;
		}
		return false;
	}
	return true;
}
