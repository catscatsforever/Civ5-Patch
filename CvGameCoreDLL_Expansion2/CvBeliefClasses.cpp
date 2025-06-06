/*	-------------------------------------------------------------------------------------------------------
	� 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreDLLUtil.h"
#include "ICvDLLUserInterface.h"
#include "CvGameCoreUtils.h"
#include "CvInfosSerializationHelper.h"
#include "CvBarbarians.h"

#include "LintFree.h"

//======================================================================================================
//					CvBeliefEntry
//======================================================================================================
/// Constructor
CvBeliefEntry::CvBeliefEntry() :
	m_iMinPopulation(0),
	m_iMinFollowers(0),
	m_iMaxDistance(0),
	m_iCityGrowthModifier(0),
	m_iFaithFromKills(0),
	m_iFaithFromDyingUnits(0),
	m_iRiverHappiness(0),
	m_iHappinessPerCity(0),
	m_iHappinessPerXPeacefulForeignFollowers(0),
	m_iPlotCultureCostModifier(0),
	m_iCityRangeStrikeModifier(0),
	m_iCombatModifierEnemyCities(0),
	m_iCombatModifierFriendlyCities(0),
	m_iFriendlyHealChange(0),
	m_iCityStateFriendshipModifier(0),
	m_iLandBarbarianConversionPercent(0),
	m_iWonderProductionModifier(0),
	m_iPlayerHappiness(0),
	m_iPlayerCultureModifier(0),
	m_fHappinessPerFollowingCity(0),
	m_iGoldPerFollowingCity(0),
	m_iGoldPerXFollowers(0),
	m_iGoldWhenCityAdopts(0),
	m_iSciencePerOtherReligionFollower(0),
	m_iSpreadDistanceModifier(0),
	m_iSpreadStrengthModifier(0),
	m_iProphetStrengthModifier(0),
	m_iProphetCostModifier(0),
	m_iMissionaryStrengthModifier(0),
#ifdef BELIEF_EXTRA_TRADE_ROUTES
	m_iExtraTradeRoutes(0),
#endif
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	m_iGoldenAgeTurns(0),
#endif
	m_iMissionaryCostModifier(0),
	m_iFriendlyCityStateSpreadModifier(0),
	m_iGreatPersonExpendedFaith(0),
#ifdef GP_EXPENDED_GA
	m_iGreatPersonExpendedGoldenAge(0),
	m_iGoldenAgeCombatMod(0),
#endif
#ifdef NEW_BELIEF_PROPHECY
	m_bAllowPolicyWonders(false),
#endif
#ifdef BELIEF_HALF_FAITH_IN_CITY
	m_bHalfFaithInCity(false),
#endif
	m_iCityStateMinimumInfluence(0),
	m_iCityStateInfluenceModifier(0),
	m_iOtherReligionPressureErosion(0),
	m_iSpyPressure(0),
	m_iInquisitorPressureRetention(0),
	m_iFaithBuildingTourism(0),
#ifdef GP_RATE_MODIFIER_FROM_BELIEF
	m_iGreatPeopleRateModifier(0),
#endif


	m_bPantheon(false),
	m_bFounder(false),
	m_bFollower(false),
	m_bEnhancer(false),
	m_bReformer(false),
	m_bRequiresPeace(false),
	m_bConvertsBarbarians(false),
	m_bFaithPurchaseAllGreatPeople(false),

	m_eObsoleteEra(NO_ERA),
	m_eResourceRevealed(NO_RESOURCE),
	m_eSpreadModifierDoublingTech(NO_TECH),

	m_paiCityYieldChange(NULL),
	m_paiHolyCityYieldChange(NULL),
#ifdef BELIEF_GREAT_WORK_YIELD_CHANGES
	m_paiYieldChangeGreatWork(NULL),
#endif
	m_paiYieldChangePerForeignCity(NULL),
	m_paiYieldChangePerXForeignFollowers(NULL),
	m_piResourceQuantityModifiers(NULL),
	m_ppiImprovementYieldChanges(NULL),
	m_ppiBuildingClassYieldChanges(NULL),
	m_paiBuildingClassHappiness(NULL),
	m_paiBuildingClassTourism(NULL),
	m_ppaiFeatureYieldChange(NULL),
	m_ppaiResourceYieldChange(NULL),
	m_ppaiTerrainYieldChange(NULL),
#ifdef BELIEF_SPECIALIST_YIELD_CHANGES
	m_ppaiSpecialistYieldChange(NULL),
#endif
#ifdef BELIEF_HURRY_MODIFIERS
	m_paiHurryModifier(NULL),
#endif
	m_piResourceHappiness(NULL),
	m_piYieldChangeAnySpecialist(NULL),
	m_piYieldChangeTradeRoute(NULL),
	m_piYieldChangeNaturalWonder(NULL),
	m_piYieldChangeWorldWonder(NULL),
	m_piYieldModifierNaturalWonder(NULL),
	m_piMaxYieldModifierPerFollower(NULL),
	m_pbFaithPurchaseUnitEraEnabled(NULL),
	m_pbBuildingClassEnabled(NULL)
{
}

/// Destructor
CvBeliefEntry::~CvBeliefEntry()
{
	CvDatabaseUtility::SafeDelete2DArray(m_ppiImprovementYieldChanges);
	CvDatabaseUtility::SafeDelete2DArray(m_ppiBuildingClassYieldChanges);
	CvDatabaseUtility::SafeDelete2DArray(m_ppaiFeatureYieldChange);
	CvDatabaseUtility::SafeDelete2DArray(m_ppaiResourceYieldChange);
	CvDatabaseUtility::SafeDelete2DArray(m_ppaiTerrainYieldChange);
#ifdef BELIEF_SPECIALIST_YIELD_CHANGES
	CvDatabaseUtility::SafeDelete2DArray(m_ppaiSpecialistYieldChange);
#endif
}

/// Accessor:: Minimum population in this city for belief to be active (0 = no such requirement)
int CvBeliefEntry::GetMinPopulation() const
{
	return m_iMinPopulation;
}

/// Accessor:: Minimum followers in this city for belief to be active (0 = no such requirement)
int CvBeliefEntry::GetMinFollowers() const
{
	return m_iMinFollowers;
}

/// Accessor:: Maximum distance from a city of this religion for belief to be active (0 = no such requirement)
int CvBeliefEntry::GetMaxDistance() const
{
	return m_iMaxDistance;
}

/// Accessor:: Modifier to city growth rate
int CvBeliefEntry::GetCityGrowthModifier() const
{
	return m_iCityGrowthModifier;
}

/// Accessor:: Percentage of enemy strength received in Faith for killing him
int CvBeliefEntry::GetFaithFromKills() const
{
	return m_iFaithFromKills;
}

/// Accessor:: Faith received when a friendly unit dies
int CvBeliefEntry::GetFaithFromDyingUnits() const
{
	return m_iFaithFromDyingUnits;
}

/// Accessor:: Happiness from each city settled on a river
int CvBeliefEntry::GetRiverHappiness() const
{
	return m_iRiverHappiness;
}

/// Accessor:: Happiness per every X population in a city
int CvBeliefEntry::GetHappinessPerCity() const
{
	return m_iHappinessPerCity;
}

/// Accessor:: Happiness per every X population in a foreign city
int CvBeliefEntry::GetHappinessPerXPeacefulForeignFollowers() const
{
	return m_iHappinessPerXPeacefulForeignFollowers;
}

/// Accessor:: Boost in speed of acquiring tiles through culture
int CvBeliefEntry::GetPlotCultureCostModifier() const
{
	return m_iPlotCultureCostModifier;
}

/// Accessor:: Boost in city strike strength
int CvBeliefEntry::GetCityRangeStrikeModifier() const
{
	return m_iCityRangeStrikeModifier;
}

/// Accessor:: Boost in combat near enemy cities of this religion
int CvBeliefEntry::GetCombatModifierEnemyCities() const
{
	return m_iCombatModifierEnemyCities;
}

/// Accessor:: Boost in combat near friendly cities of this religion
int CvBeliefEntry::GetCombatModifierFriendlyCities() const
{
	return m_iCombatModifierFriendlyCities;
}

/// Accessor:: Additional healing in friendly territory
int CvBeliefEntry::GetFriendlyHealChange() const
{
	return m_iFriendlyHealChange;
}

/// Accessor:: Boost in city state influence effectiveness
int CvBeliefEntry::GetCityStateFriendshipModifier() const
{
	return m_iCityStateFriendshipModifier;
}

/// Accessor:: Chance of converting a barbarian camp guard
int CvBeliefEntry::GetLandBarbarianConversionPercent() const
{
	return m_iLandBarbarianConversionPercent;
}

/// Accessor:: boost in production speed for wonders prior to obsolete era
int CvBeliefEntry::GetWonderProductionModifier() const
{
	return m_iWonderProductionModifier;
}

/// Accessor:: boost in production speed for wonders prior to obsolete era
int CvBeliefEntry::GetPlayerHappiness() const
{
	return m_iPlayerHappiness;
}

/// Accessor:: boost in production speed for wonders prior to obsolete era
int CvBeliefEntry::GetPlayerCultureModifier() const
{
	return m_iPlayerCultureModifier;
}

/// Accessor:: amount of extra happiness from each city following this religion
float CvBeliefEntry::GetHappinessPerFollowingCity() const
{
	return m_fHappinessPerFollowingCity;
}

/// Accessor:: amount of extra gold from each city following this religion
int CvBeliefEntry::GetGoldPerFollowingCity() const
{
	return m_iGoldPerFollowingCity;
}

/// Accessor:: amount of extra gold from each city following this religion
int CvBeliefEntry::GetGoldPerXFollowers() const
{
	return m_iGoldPerXFollowers;
}

/// Accessor:: amount of extra gold from each city following this religion
int CvBeliefEntry::GetGoldWhenCityAdopts() const
{
	return m_iGoldWhenCityAdopts;
}

/// Accessor:: amount of science for each follower of another religion in city spread to
int CvBeliefEntry::GetSciencePerOtherReligionFollower() const
{
	return m_iSciencePerOtherReligionFollower;
}

/// Accessor:: extra distance in city-to-city religion spread
int CvBeliefEntry::GetSpreadDistanceModifier() const
{
	return m_iSpreadDistanceModifier;
}

/// Accessor:: extra strength in city-to-city religion spread
int CvBeliefEntry::GetSpreadStrengthModifier() const
{
	return m_iSpreadStrengthModifier;
}

/// Accessor:: prophet conversion strength modifier
int CvBeliefEntry::GetProphetStrengthModifier() const
{
	return m_iProphetStrengthModifier;
}

/// Accessor:: prophet cost discount
int CvBeliefEntry::GetProphetCostModifier() const
{
	return m_iProphetCostModifier;
}

/// Accessor:: missionary conversion strength modifier
int CvBeliefEntry::GetMissionaryStrengthModifier() const
{
	return m_iMissionaryStrengthModifier;
}

#ifdef BELIEF_EXTRA_TRADE_ROUTES
/// Accessor:: extra trade routes
int CvBeliefEntry::GetExtraTradeRoutes() const
{
	return m_iExtraTradeRoutes;
}
#endif
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
/// Accessor:: golden age turns
int CvBeliefEntry::GetGoldenAgeTurns() const
{
	return m_iGoldenAgeTurns;
}
#endif
/// Accessor:: missionary cost discount
int CvBeliefEntry::GetMissionaryCostModifier() const
{
	return m_iMissionaryCostModifier;
}

/// Accessor: speed increase of spread to friendly city states
int CvBeliefEntry::GetFriendlyCityStateSpreadModifier() const
{
	return m_iFriendlyCityStateSpreadModifier;
}

/// Accessor: faith earned for each GP expended
int CvBeliefEntry::GetGreatPersonExpendedFaith() const
{
	return m_iGreatPersonExpendedFaith;
}

#ifdef GP_EXPENDED_GA
/// Accessor: golden age turns for each GP expended
int CvBeliefEntry::GetGreatPersonExpendedGoldenAge() const
{
	return m_iGreatPersonExpendedGoldenAge;
}

/// Accessor: golden age combat modifier
int CvBeliefEntry::GetGoldenAgeCombatMod() const
{
	return m_iGoldenAgeCombatMod;
}
#endif

#ifdef NEW_BELIEF_PROPHECY
///
bool CvBeliefEntry::IsAllowPolicyWonders() const
{
	return m_bAllowPolicyWonders;
}
#endif

#ifdef BELIEF_HALF_FAITH_IN_CITY
///
bool CvBeliefEntry::IsHalfFaithInCity() const
{
	return m_bHalfFaithInCity;
}
#endif

/// Accessor: minimum influence with city states of a shared religion
int CvBeliefEntry::GetCityStateMinimumInfluence() const
{
	return m_iCityStateMinimumInfluence;
}

/// Accessor: modifier to influence boosts with city states
int CvBeliefEntry::GetCityStateInfluenceModifier() const
{
	return m_iCityStateInfluenceModifier;
}

/// Accessor: percentage of religious pressure gain that becomes a drop in pressure of other religions
int CvBeliefEntry::GetOtherReligionPressureErosion() const
{
	return m_iOtherReligionPressureErosion;
}

/// Accessor: base religious pressure (before speed multiplier) from having a spy in a city
int CvBeliefEntry::GetSpyPressure() const
{
	return m_iSpyPressure;
}

/// Accessor: percentage of religious pressure retained if one of your cities is hit with an Inquisitor
int CvBeliefEntry::GetInquisitorPressureRetention() const
{
	return m_iInquisitorPressureRetention;
}

/// Accessor: how much tourism can I get from Buildings bought with Faith?
int CvBeliefEntry::GetFaithBuildingTourism() const
{
	return m_iFaithBuildingTourism;
}

#ifdef GP_RATE_MODIFIER_FROM_BELIEF
int CvBeliefEntry::GetGreatPeopleRateModifier() const
{
	return m_iGreatPeopleRateModifier;
}

#endif

/// Accessor: is this a belief a pantheon can adopt
bool CvBeliefEntry::IsPantheonBelief() const
{
	return m_bPantheon;
}

/// Accessor: is this a belief a religion founder can adopt
bool CvBeliefEntry::IsFounderBelief() const
{
	return m_bFounder;
}

/// Accessor: is this a belief a religion follower can adopt
bool CvBeliefEntry::IsFollowerBelief() const
{
	return m_bFollower;
}

/// Accessor: is this a belief that enhances the spread of the religion
bool CvBeliefEntry::IsEnhancerBelief() const
{
	return m_bEnhancer;
}

/// Accessor: is this a belief that is added with the Reformation social policy
bool CvBeliefEntry::IsReformationBelief() const
{
	return m_bReformer;
}

/// Accessor: is this a belief that requires you to be at peace to benefit?
bool CvBeliefEntry::RequiresPeace() const
{
	return m_bRequiresPeace;
}

/// Accessor: is this a belief that allows your missionaries to convert adjacent barbarians?
bool CvBeliefEntry::ConvertsBarbarians() const
{
	return m_bConvertsBarbarians;
}

/// Accessor: is this a belief that allows you to purchase any type of Great Person with Faith?
bool CvBeliefEntry::FaithPurchaseAllGreatPeople() const
{
	return m_bFaithPurchaseAllGreatPeople;
}

/// Accessor: era when wonder production modifier goes obsolete
EraTypes CvBeliefEntry::GetObsoleteEra() const
{
	return m_eObsoleteEra;
}

/// Accessor:: resource revealed near this city
ResourceTypes CvBeliefEntry::GetResourceRevealed() const
{
	return m_eResourceRevealed;
}

/// Accessor:: technology that doubles the effect of the SpreadStrengthModifier
TechTypes CvBeliefEntry::GetSpreadModifierDoublingTech() const
{
	return m_eSpreadModifierDoublingTech;
}

/// Accessor:: Get brief text description
const char* CvBeliefEntry::getShortDescription() const
{
	return m_strShortDescription;
}

/// Accessor:: Set brief text description
void CvBeliefEntry::setShortDescription(const char* szVal)
{
	m_strShortDescription = szVal;
}

/// Accessor:: Additional yield
int CvBeliefEntry::GetCityYieldChange(int i) const
{
	return m_paiCityYieldChange ? m_paiCityYieldChange[i] : -1;
}

/// Accessor:: Additional player-level yield for controlling holy city
int CvBeliefEntry::GetHolyCityYieldChange(int i) const
{
	return m_paiHolyCityYieldChange ? m_paiHolyCityYieldChange[i] : -1;
}

#ifdef BELIEF_GREAT_WORK_YIELD_CHANGES
///
int CvBeliefEntry::GetGreatWorkYieldChange(int i) const
{
	return m_paiYieldChangeGreatWork ? m_paiYieldChangeGreatWork[i] : -1;
}

#endif
/// Accessor:: Additional player-level yield for each foreign city converted
int CvBeliefEntry::GetYieldChangePerForeignCity(int i) const
{
	return m_paiYieldChangePerForeignCity ? m_paiYieldChangePerForeignCity[i] : -1;
}

/// Accessor:: Additional player-level yield for followers in foreign cities
int CvBeliefEntry::GetYieldChangePerXForeignFollowers(int i) const
{
	return m_paiYieldChangePerXForeignFollowers ? m_paiYieldChangePerXForeignFollowers[i] : -1;
}

/// Accessor:: Additional quantity of a specific resource
int CvBeliefEntry::GetResourceQuantityModifier(int i) const
{
	CvAssertMsg(i < GC.getNumResourceInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piResourceQuantityModifiers ? m_piResourceQuantityModifiers[i] : -1;
}

/// Accessor:: Extra yield from an improvement
int CvBeliefEntry::GetImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const
{
	CvAssertMsg(eIndex1 < GC.getNumImprovementInfos(), "Index out of bounds");
	CvAssertMsg(eIndex1 > -1, "Index out of bounds");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(eIndex2 > -1, "Index out of bounds");
	return m_ppiImprovementYieldChanges ? m_ppiImprovementYieldChanges[eIndex1][eIndex2] : 0;
}

/// Yield change for a specific BuildingClass by yield type
int CvBeliefEntry::GetBuildingClassYieldChange(int i, int j) const
{
	CvAssertMsg(i < GC.getNumBuildingClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(j < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(j > -1, "Index out of bounds");
	return m_ppiBuildingClassYieldChanges[i][j];
}

/// Amount of extra Happiness per turn a BuildingClass provides
int CvBeliefEntry::GetBuildingClassHappiness(int i) const
{
	CvAssertMsg(i < GC.getNumBuildingClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_paiBuildingClassHappiness ? m_paiBuildingClassHappiness[i] : -1;
}

/// Amount of extra Tourism per turn a BuildingClass provides
int CvBeliefEntry::GetBuildingClassTourism(int i) const
{
	CvAssertMsg(i < GC.getNumBuildingClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_paiBuildingClassTourism ? m_paiBuildingClassTourism[i] : -1;
}

/// Change to Feature yield by type
int CvBeliefEntry::GetFeatureYieldChange(int i, int j) const
{
	CvAssertMsg(i < GC.getNumFeatureInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(j < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(j > -1, "Index out of bounds");
	return m_ppaiFeatureYieldChange ? m_ppaiFeatureYieldChange[i][j] : -1;
}

/// Change to Resource yield by type
int CvBeliefEntry::GetResourceYieldChange(int i, int j) const
{
	CvAssertMsg(i < GC.getNumResourceInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(j < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(j > -1, "Index out of bounds");
	return m_ppaiResourceYieldChange ? m_ppaiResourceYieldChange[i][j] : -1;
}

/// Change to yield by terrain
int CvBeliefEntry::GetTerrainYieldChange(int i, int j) const
{
	CvAssertMsg(i < GC.getNumTerrainInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(j < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(j > -1, "Index out of bounds");
	return m_ppaiTerrainYieldChange ? m_ppaiTerrainYieldChange[i][j] : -1;
}

#ifdef BELIEF_SPECIALIST_YIELD_CHANGES
/// Change to yield by spesialist
int CvBeliefEntry::GetSpecialistYieldChange(int i, int j) const
{
	CvAssertMsg(i < GC.getNumSpecialistInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(j < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(j > -1, "Index out of bounds");
	return m_ppaiSpecialistYieldChange ? m_ppaiSpecialistYieldChange[i][j] : -1;
}

#endif
#ifdef BELIEF_HURRY_MODIFIERS
/// Modifier to Hurry cost
int CvBeliefEntry::GetHurryModifier(int i) const
{
	CvAssertMsg(i < GC.getNumHurryInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_paiHurryModifier ? m_paiHurryModifier[i] : -1;
}

#endif
/// Happiness from a resource
int CvBeliefEntry::GetResourceHappiness(int i) const
{
	CvAssertMsg(i < GC.getNumResourceInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piResourceHappiness ? m_piResourceHappiness[i] : -1;
}

/// Yield boost from having a specialist of any type in city
int CvBeliefEntry::GetYieldChangeAnySpecialist(int i) const
{
	CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piYieldChangeAnySpecialist ? m_piYieldChangeAnySpecialist[i] : -1;
}

/// Yield boost from a trade route
int CvBeliefEntry::GetYieldChangeTradeRoute(int i) const
{
	CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piYieldChangeTradeRoute ? m_piYieldChangeTradeRoute[i] : -1;
}

/// Yield boost from a natural wonder
int CvBeliefEntry::GetYieldChangeNaturalWonder(int i) const
{
	CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piYieldChangeNaturalWonder ? m_piYieldChangeNaturalWonder[i] : -1;
}

/// Yield boost from a world wonder
int CvBeliefEntry::GetYieldChangeWorldWonder(int i) const
{
	CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piYieldChangeWorldWonder ? m_piYieldChangeWorldWonder[i] : -1;
}

/// Yield percentage boost from a natural wonder
int CvBeliefEntry::GetYieldModifierNaturalWonder(int i) const
{
	CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piYieldModifierNaturalWonder ? m_piYieldModifierNaturalWonder[i] : -1;
}

/// Do we get a yield modifier 
int CvBeliefEntry::GetMaxYieldModifierPerFollower(int i) const
{
	CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piMaxYieldModifierPerFollower ? m_piMaxYieldModifierPerFollower[i] : -1;
}

/// Can we buy units of this era with faith?
bool CvBeliefEntry::IsFaithUnitPurchaseEra(int i) const
{
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(i < GC.getNumEraInfos(), "Index out of bounds");
	return m_pbFaithPurchaseUnitEraEnabled ? m_pbFaithPurchaseUnitEraEnabled[i] : false;
}

/// Can we buy units of this era with faith?
bool CvBeliefEntry::IsBuildingClassEnabled(int i) const
{
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(i < GC.getNumBuildingClassInfos(), "Index out of bounds");
	return m_pbBuildingClassEnabled ? m_pbBuildingClassEnabled[i] : false;
}

/// Load XML data
bool CvBeliefEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if(!CvBaseInfo::CacheResults(kResults, kUtility))
		return false;

	//Basic Properties
	setShortDescription(kResults.GetText("ShortDescription"));

	m_iMinPopulation                  = kResults.GetInt("MinPopulation");
	m_iMinFollowers                   = kResults.GetInt("MinFollowers");
	m_iMaxDistance					  = kResults.GetInt("MaxDistance");
	m_iCityGrowthModifier		      = kResults.GetInt("CityGrowthModifier");
	m_iFaithFromKills				  = kResults.GetInt("FaithFromKills");
	m_iFaithFromDyingUnits			  = kResults.GetInt("FaithFromDyingUnits");
	m_iRiverHappiness				  = kResults.GetInt("RiverHappiness");
	m_iHappinessPerCity				  = kResults.GetInt("HappinessPerCity");
	m_iHappinessPerXPeacefulForeignFollowers  = kResults.GetInt("HappinessPerXPeacefulForeignFollowers");
	m_iPlotCultureCostModifier	      = kResults.GetInt("PlotCultureCostModifier");
	m_iCityRangeStrikeModifier	      = kResults.GetInt("CityRangeStrikeModifier");
	m_iCombatModifierEnemyCities      = kResults.GetInt("CombatModifierEnemyCities");
	m_iCombatModifierFriendlyCities   = kResults.GetInt("CombatModifierFriendlyCities");
	m_iFriendlyHealChange	          = kResults.GetInt("FriendlyHealChange");
	m_iCityStateFriendshipModifier    = kResults.GetInt("CityStateFriendshipModifier");
	m_iLandBarbarianConversionPercent = kResults.GetInt("LandBarbarianConversionPercent");
	m_iWonderProductionModifier       = kResults.GetInt("WonderProductionModifier");
	m_iPlayerHappiness			      = kResults.GetInt("PlayerHappiness");
	m_iPlayerCultureModifier          = kResults.GetInt("PlayerCultureModifier");
	m_fHappinessPerFollowingCity      = kResults.GetFloat("HappinessPerFollowingCity");
	m_iGoldPerFollowingCity           = kResults.GetInt("GoldPerFollowingCity");
	m_iGoldPerXFollowers              = kResults.GetInt("GoldPerXFollowers");
	m_iGoldWhenCityAdopts             = kResults.GetInt("GoldPerFirstCityConversion");
	m_iSciencePerOtherReligionFollower= kResults.GetInt("SciencePerOtherReligionFollower");
	m_iSpreadDistanceModifier         = kResults.GetInt("SpreadDistanceModifier");
	m_iSpreadStrengthModifier		  = kResults.GetInt("SpreadStrengthModifier");
	m_iProphetStrengthModifier        = kResults.GetInt("ProphetStrengthModifier");
	m_iProphetCostModifier            = kResults.GetInt("ProphetCostModifier");
	m_iMissionaryStrengthModifier     = kResults.GetInt("MissionaryStrengthModifier");
#ifdef BELIEF_EXTRA_TRADE_ROUTES
	m_iExtraTradeRoutes				  = kResults.GetInt("ExtraTradeRoutes");
#endif
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	m_iGoldenAgeTurns				  = kResults.GetInt("GoldenAgeTurns");
#endif
	m_iMissionaryCostModifier         = kResults.GetInt("MissionaryCostModifier");
	m_iFriendlyCityStateSpreadModifier= kResults.GetInt("FriendlyCityStateSpreadModifier");
	m_iGreatPersonExpendedFaith       = kResults.GetInt("GreatPersonExpendedFaith");
#ifdef GP_EXPENDED_GA
	m_iGreatPersonExpendedGoldenAge	  = kResults.GetInt("GreatPersonExpendedGoldenAge");
	m_iGoldenAgeCombatMod             =	kResults.GetInt("GoldenAgeCombatMod");
#endif
#ifdef NEW_BELIEF_PROPHECY
	m_bAllowPolicyWonders             = kResults.GetBool("AllowPolicyWonders");
#endif
#ifdef BELIEF_HALF_FAITH_IN_CITY
	m_bHalfFaithInCity                = kResults.GetBool("HalfFaithInCity");
#endif
	m_iCityStateMinimumInfluence      = kResults.GetInt("CityStateMinimumInfluence");
	m_iCityStateInfluenceModifier     = kResults.GetInt("CityStateInfluenceModifier");
	m_iOtherReligionPressureErosion   = kResults.GetInt("OtherReligionPressureErosion");
	m_iSpyPressure					  = kResults.GetInt("SpyPressure");
	m_iInquisitorPressureRetention    = kResults.GetInt("InquisitorPressureRetention");
	m_iFaithBuildingTourism           = kResults.GetInt("FaithBuildingTourism");
#ifdef GP_RATE_MODIFIER_FROM_BELIEF
	m_iGreatPeopleRateModifier        = kResults.GetInt("GreatPeopleRateModifier");
#endif


	m_bPantheon						  = kResults.GetBool("Pantheon");
	m_bFounder						  = kResults.GetBool("Founder");
	m_bFollower						  = kResults.GetBool("Follower");
	m_bEnhancer						  = kResults.GetBool("Enhancer");
	m_bReformer						  = kResults.GetBool("Reformation");
	m_bRequiresPeace				  = kResults.GetBool("RequiresPeace");
	m_bConvertsBarbarians			  = kResults.GetBool("ConvertsBarbarians");
	m_bFaithPurchaseAllGreatPeople	  = kResults.GetBool("FaithPurchaseAllGreatPeople");

	//References
	const char* szTextVal;
	szTextVal						  = kResults.GetText("ObsoleteEra");
	m_eObsoleteEra					  = (EraTypes)GC.getInfoTypeForString(szTextVal, true);
	szTextVal						  = kResults.GetText("ResourceRevealed");
	m_eResourceRevealed				  = (ResourceTypes)GC.getInfoTypeForString(szTextVal, true);
	szTextVal						  = kResults.GetText("SpreadModifierDoublingTech");
	m_eSpreadModifierDoublingTech     = (TechTypes)GC.getInfoTypeForString(szTextVal, true);

	//Arrays
	const char* szBeliefType = GetType();
	kUtility.SetYields(m_paiCityYieldChange, "Belief_CityYieldChanges", "BeliefType", szBeliefType);
	kUtility.SetYields(m_paiHolyCityYieldChange, "Belief_HolyCityYieldChanges", "BeliefType", szBeliefType);
#ifdef BELIEF_GREAT_WORK_YIELD_CHANGES
	kUtility.SetYields(m_paiYieldChangeGreatWork, "Belief_GreatWorkYieldChanges", "BeliefType", szBeliefType);
#endif
	kUtility.SetYields(m_piYieldChangeAnySpecialist, "Belief_YieldChangeAnySpecialist", "BeliefType", szBeliefType);
	kUtility.SetYields(m_piYieldChangeTradeRoute, "Belief_YieldChangeTradeRoute", "BeliefType", szBeliefType);
	kUtility.SetYields(m_piYieldChangeNaturalWonder, "Belief_YieldChangeNaturalWonder", "BeliefType", szBeliefType);
	kUtility.SetYields(m_piYieldChangeWorldWonder, "Belief_YieldChangeWorldWonder", "BeliefType", szBeliefType);
	kUtility.SetYields(m_piYieldModifierNaturalWonder, "Belief_YieldModifierNaturalWonder", "BeliefType", szBeliefType);
#ifdef BELIEF_HURRY_MODIFIERS
	kUtility.PopulateArrayByValue(m_paiHurryModifier, "HurryInfos", "Belief_HurryModifiers", "HurryType", "BeliefType", szBeliefType, "HurryCostModifier");
#endif
	kUtility.PopulateArrayByValue(m_piMaxYieldModifierPerFollower, "Yields", "Belief_MaxYieldModifierPerFollower", "YieldType", "BeliefType", szBeliefType, "Max");
	kUtility.PopulateArrayByValue(m_piResourceHappiness, "Resources", "Belief_ResourceHappiness", "ResourceType", "BeliefType", szBeliefType, "HappinessChange");
	kUtility.PopulateArrayByValue(m_piResourceQuantityModifiers, "Resources", "Belief_ResourceQuantityModifiers", "ResourceType", "BeliefType", szBeliefType, "ResourceQuantityModifier");
	kUtility.PopulateArrayByValue(m_paiBuildingClassHappiness, "BuildingClasses", "Belief_BuildingClassHappiness", "BuildingClassType", "BeliefType", szBeliefType, "Happiness");
	kUtility.PopulateArrayByValue(m_paiBuildingClassTourism, "BuildingClasses", "Belief_BuildingClassTourism", "BuildingClassType", "BeliefType", szBeliefType, "Tourism");
	kUtility.PopulateArrayByValue(m_paiYieldChangePerForeignCity, "Yields", "Belief_YieldChangePerForeignCity", "YieldType", "BeliefType", szBeliefType, "Yield");
	kUtility.PopulateArrayByValue(m_paiYieldChangePerXForeignFollowers, "Yields", "Belief_YieldChangePerXForeignFollowers", "YieldType", "BeliefType", szBeliefType, "ForeignFollowers");
	kUtility.PopulateArrayByExistence(m_pbFaithPurchaseUnitEraEnabled, "Eras", "Belief_EraFaithUnitPurchase", "EraType", "BeliefType", szBeliefType);
	kUtility.PopulateArrayByExistence(m_pbBuildingClassEnabled, "BuildingClasses", "Belief_BuildingClassFaithPurchase", "BuildingClassType", "BeliefType", szBeliefType);

	//ImprovementYieldChanges
	{
		kUtility.Initialize2DArray(m_ppiImprovementYieldChanges, "Improvements", "Yields");

		std::string strKey("Belief_ImprovementYieldChanges");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Improvements.ID as ImprovementID, Yields.ID as YieldID, Yield from Belief_ImprovementYieldChanges inner join Improvements on Improvements.Type = ImprovementType inner join Yields on Yields.Type = YieldType where BeliefType = ?");
		}

		pResults->Bind(1, szBeliefType);

		while(pResults->Step())
		{
			const int ImprovementID = pResults->GetInt(0);
			const int YieldID = pResults->GetInt(1);
			const int yield = pResults->GetInt(2);

			m_ppiImprovementYieldChanges[ImprovementID][YieldID] = yield;
		}
	}

	//BuildingClassYieldChanges
	{
		kUtility.Initialize2DArray(m_ppiBuildingClassYieldChanges, "BuildingClasses", "Yields");

		std::string strKey("Belief_BuildingClassYieldChanges");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select BuildingClasses.ID as BuildingClassID, Yields.ID as YieldID, YieldChange from Belief_BuildingClassYieldChanges inner join BuildingClasses on BuildingClasses.Type = BuildingClassType inner join Yields on Yields.Type = YieldType where BeliefType = ?");
		}

		pResults->Bind(1, szBeliefType);

		while(pResults->Step())
		{
			const int BuildingClassID = pResults->GetInt(0);
			const int iYieldID = pResults->GetInt(1);
			const int iYieldChange = pResults->GetInt(2);

			m_ppiBuildingClassYieldChanges[BuildingClassID][iYieldID] = iYieldChange;
		}
	}

	//FeatureYieldChanges
	{
		kUtility.Initialize2DArray(m_ppaiFeatureYieldChange, "Features", "Yields");

		std::string strKey("Belief_FeatureYieldChanges");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Features.ID as FeatureID, Yields.ID as YieldID, Yield from Belief_FeatureYieldChanges inner join Features on Features.Type = FeatureType inner join Yields on Yields.Type = YieldType where BeliefType = ?");
		}

		pResults->Bind(1, szBeliefType);

		while(pResults->Step())
		{
			const int FeatureID = pResults->GetInt(0);
			const int YieldID = pResults->GetInt(1);
			const int yield = pResults->GetInt(2);

			m_ppaiFeatureYieldChange[FeatureID][YieldID] = yield;
		}
	}

	//ResourceYieldChanges
	{
		kUtility.Initialize2DArray(m_ppaiResourceYieldChange, "Resources", "Yields");

		std::string strKey("Belief_ResourceYieldChanges");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Resources.ID as ResourceID, Yields.ID as YieldID, Yield from Belief_ResourceYieldChanges inner join Resources on Resources.Type = ResourceType inner join Yields on Yields.Type = YieldType where BeliefType = ?");
		}

		pResults->Bind(1, szBeliefType);

		while(pResults->Step())
		{
			const int ResourceID = pResults->GetInt(0);
			const int YieldID = pResults->GetInt(1);
			const int yield = pResults->GetInt(2);

			m_ppaiResourceYieldChange[ResourceID][YieldID] = yield;
		}
	}

	//TerrainYieldChanges
	{
		kUtility.Initialize2DArray(m_ppaiTerrainYieldChange, "Terrains", "Yields");

		std::string strKey("Belief_TerrainYieldChanges");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Terrains.ID as TerrainID, Yields.ID as YieldID, Yield from Belief_TerrainYieldChanges inner join Terrains on Terrains.Type = TerrainType inner join Yields on Yields.Type = YieldType where BeliefType = ?");
		}

		pResults->Bind(1, szBeliefType);

		while(pResults->Step())
		{
			const int TerrainID = pResults->GetInt(0);
			const int YieldID = pResults->GetInt(1);
			const int yield = pResults->GetInt(2);

			m_ppaiTerrainYieldChange[TerrainID][YieldID] = yield;
		}
	}

#ifdef BELIEF_SPECIALIST_YIELD_CHANGES
	//SpecialistYieldChanges
	{
		kUtility.Initialize2DArray(m_ppaiSpecialistYieldChange, "Specialists", "Yields");

		std::string strKey("Belief_SpecialistYieldChanges");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Specialists.ID as SpecialistID, Yields.ID as YieldID, Yield from Belief_SpecialistYieldChanges inner join Specialists on Specialists.Type = SpecialistType inner join Yields on Yields.Type = YieldType where BeliefType = ?");
		}

		pResults->Bind(1, szBeliefType);

		while(pResults->Step())
		{
			const int SpecialistID = pResults->GetInt(0);
			const int YieldID = pResults->GetInt(1);
			const int yield = pResults->GetInt(2);

			m_ppaiSpecialistYieldChange[SpecialistID][YieldID] = yield;
		}
	}
#endif

	return true;
}

//=====================================
// CvBeliefXMLEntries
//=====================================
/// Constructor
CvBeliefXMLEntries::CvBeliefXMLEntries(void)
{

}

/// Destructor
CvBeliefXMLEntries::~CvBeliefXMLEntries(void)
{
	DeleteArray();
}

/// Returns vector of belief entries
std::vector<CvBeliefEntry*>& CvBeliefXMLEntries::GetBeliefEntries()
{
	return m_paBeliefEntries;
}

/// Number of defined beliefs
int CvBeliefXMLEntries::GetNumBeliefs()
{
	return m_paBeliefEntries.size();
}

/// Clear belief entries
void CvBeliefXMLEntries::DeleteArray()
{
	for(std::vector<CvBeliefEntry*>::iterator it = m_paBeliefEntries.begin(); it != m_paBeliefEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paBeliefEntries.clear();
}

/// Get a specific entry
CvBeliefEntry* CvBeliefXMLEntries::GetEntry(int index)
{
	return m_paBeliefEntries[index];
}

//=====================================
// CvReligionBeliefs
//=====================================
/// Constructor
CvReligionBeliefs::CvReligionBeliefs():
	m_paiBuildingClassEnabled(NULL)
#ifdef DUEL_ALLOW_SAMETURN_BELIEFS
	, m_paiBeliefAdoptionTurn(NULL)
#endif
{
	Reset();
}

/// Destructor
CvReligionBeliefs::~CvReligionBeliefs(void)
{
	Uninit();
}

/// Copy Constructor with typical parameters
CvReligionBeliefs::CvReligionBeliefs(const CvReligionBeliefs& source)
{
	m_iFaithFromDyingUnits = source.m_iFaithFromDyingUnits;
	m_iRiverHappiness = source.m_iRiverHappiness;
	m_iPlotCultureCostModifier = source.m_iPlotCultureCostModifier;
	m_iCityRangeStrikeModifier = source.m_iCityRangeStrikeModifier;
	m_iCombatModifierEnemyCities = source.m_iCombatModifierEnemyCities;
	m_iCombatModifierFriendlyCities = source.m_iCombatModifierFriendlyCities;
	m_iFriendlyHealChange = source.m_iFriendlyHealChange;
	m_iCityStateFriendshipModifier = source.m_iCityStateFriendshipModifier;
	m_iLandBarbarianConversionPercent = source.m_iLandBarbarianConversionPercent;
	m_iSpreadDistanceModifier = source.m_iSpreadDistanceModifier;
	m_iSpreadStrengthModifier = source.m_iSpreadStrengthModifier;
	m_iProphetStrengthModifier = source.m_iProphetStrengthModifier;
	m_iProphetCostModifier = source.m_iProphetCostModifier;
	m_iMissionaryStrengthModifier = source.m_iMissionaryStrengthModifier;
#ifdef BELIEF_EXTRA_TRADE_ROUTES
	m_iExtraTradeRoutes	= source.m_iExtraTradeRoutes;
#endif
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	m_iGoldenAgeTurns = source.m_iGoldenAgeTurns;
#endif
	m_iMissionaryCostModifier = source.m_iMissionaryCostModifier;
	m_iFriendlyCityStateSpreadModifier = source.m_iFriendlyCityStateSpreadModifier;
	m_iGreatPersonExpendedFaith = source.m_iGreatPersonExpendedFaith;
#ifdef GP_EXPENDED_GA
	m_iGreatPersonExpendedGoldenAge = source.m_iGreatPersonExpendedGoldenAge;
	m_iGoldenAgeCombatMod = source.m_iGoldenAgeCombatMod;
#endif
#ifdef NEW_BELIEF_PROPHECY
	m_bAllowPolicyWonders = source.m_bAllowPolicyWonders;
#endif
#ifdef BELIEF_HALF_FAITH_IN_CITY
	m_bHalfFaithInCity = source.m_bHalfFaithInCity;
#endif
	m_iCityStateMinimumInfluence = source.m_iCityStateMinimumInfluence;
	m_iCityStateInfluenceModifier = source.m_iCityStateInfluenceModifier;
	m_iOtherReligionPressureErosion = source.m_iOtherReligionPressureErosion;
	m_iSpyPressure = source.m_iSpyPressure;
	m_iInquisitorPressureRetention = source.m_iInquisitorPressureRetention;
	m_iFaithBuildingTourism = source.m_iFaithBuildingTourism;
#ifdef GP_RATE_MODIFIER_FROM_BELIEF
	m_iGreatPeopleRateModifier = source.m_iGreatPeopleRateModifier;
#endif


	m_eObsoleteEra = source.m_eObsoleteEra;
	m_eResourceRevealed = source.m_eResourceRevealed;
	m_eSpreadModifierDoublingTech = source.m_eSpreadModifierDoublingTech;

	m_ReligionBeliefs = source.m_ReligionBeliefs;

	m_paiBuildingClassEnabled = FNEW(int[GC.getNumBuildingClassInfos()], c_eCiv5GameplayDLL, 0);
	for(int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo((BuildingClassTypes)iI);
		if(!pkBuildingClassInfo)
		{
			continue;
		}

		m_paiBuildingClassEnabled[iI] = source.m_paiBuildingClassEnabled[iI];
	}
#ifdef DUEL_ALLOW_SAMETURN_BELIEFS
	m_paiBeliefAdoptionTurn = FNEW(int[GC.getNumBeliefInfos()], c_eCiv5GameplayDLL, 0);
	for (int iI = 0; iI < GC.getNumBeliefInfos(); iI++)
	{
		CvBeliefEntry* pkBelief = GC.getBeliefInfo((BeliefTypes)iI);
		if (!pkBelief)
		{
			continue;
		}
		m_paiBeliefAdoptionTurn[iI] = source.m_paiBeliefAdoptionTurn[iI];
	}
#endif
}

/// Deallocate memory created in initialize
void CvReligionBeliefs::Uninit()
{
	SAFE_DELETE_ARRAY(m_paiBuildingClassEnabled);
#ifdef DUEL_ALLOW_SAMETURN_BELIEFS
	SAFE_DELETE_ARRAY(m_paiBeliefAdoptionTurn);
#endif
}

/// Reset data members
void CvReligionBeliefs::Reset()
{
	m_iFaithFromDyingUnits = 0;
	m_iRiverHappiness = 0;
	m_iPlotCultureCostModifier = 0;
	m_iCityRangeStrikeModifier = 0;
	m_iCombatModifierEnemyCities = 0;
	m_iCombatModifierFriendlyCities = 0;
	m_iFriendlyHealChange = 0;
	m_iCityStateFriendshipModifier = 0;
	m_iLandBarbarianConversionPercent = 0;
	m_iSpreadDistanceModifier = 0;
	m_iSpreadStrengthModifier = 0;
	m_iProphetStrengthModifier = 0;
	m_iProphetCostModifier = 0;
	m_iMissionaryStrengthModifier = 0;
#ifdef BELIEF_EXTRA_TRADE_ROUTES
	m_iExtraTradeRoutes	= 0;
#endif
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	m_iGoldenAgeTurns = 0;
#endif
	m_iMissionaryCostModifier = 0;
	m_iFriendlyCityStateSpreadModifier = 0;
	m_iGreatPersonExpendedFaith = 0;
#ifdef GP_EXPENDED_GA
	m_iGreatPersonExpendedGoldenAge = 0;
	m_iGoldenAgeCombatMod = 0;
#endif
#ifdef NEW_BELIEF_PROPHECY
	m_bAllowPolicyWonders = false;
#endif
#ifdef BELIEF_HALF_FAITH_IN_CITY
	m_bHalfFaithInCity = false;
#endif
	m_iCityStateMinimumInfluence = 0;
	m_iCityStateInfluenceModifier = 0;
	m_iOtherReligionPressureErosion = 0;
	m_iSpyPressure = 0;
	m_iInquisitorPressureRetention = 0;
	m_iFaithBuildingTourism = 0;
#ifdef GP_RATE_MODIFIER_FROM_BELIEF
	m_iGreatPeopleRateModifier = 0;
#endif


	m_eObsoleteEra = NO_ERA;
	m_eResourceRevealed = NO_RESOURCE;
	m_eSpreadModifierDoublingTech = NO_TECH;

	m_ReligionBeliefs.clear();

	m_paiBuildingClassEnabled = FNEW(int[GC.getNumBuildingClassInfos()], c_eCiv5GameplayDLL, 0);
	for(int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo((BuildingClassTypes)iI);
		if(!pkBuildingClassInfo)
		{
			continue;
		}

		m_paiBuildingClassEnabled[iI] = 0;
	}
#ifdef DUEL_ALLOW_SAMETURN_BELIEFS
	m_paiBeliefAdoptionTurn = FNEW(int[GC.getNumBeliefInfos()], c_eCiv5GameplayDLL, 0);
	for (int iI = 0; iI < GC.getNumBeliefInfos(); iI++)
	{
		CvBeliefEntry* pkBelief = GC.getBeliefInfo((BeliefTypes)iI);
		if (!pkBelief)
		{
			continue;
		}
		m_paiBeliefAdoptionTurn[iI] = -1;
	}
#endif
}

/// Store off data on bonuses from beliefs
void CvReligionBeliefs::AddBelief(BeliefTypes eBelief)
{
	CvAssert(eBelief != NO_BELIEF);
	if(eBelief == NO_BELIEF)
		return;

	CvBeliefEntry* belief = GC.GetGameBeliefs()->GetEntry(eBelief);
	CvAssert(belief != NULL);
	if(belief == NULL)
		return;

	m_iFaithFromDyingUnits += belief->GetFaithFromDyingUnits();
	m_iRiverHappiness += belief->GetRiverHappiness();
	m_iPlotCultureCostModifier += belief->GetPlotCultureCostModifier();
	m_iCityRangeStrikeModifier += belief->GetCityRangeStrikeModifier();
	m_iCombatModifierEnemyCities += belief->GetCombatModifierEnemyCities();
	m_iCombatModifierFriendlyCities += belief->GetCombatModifierFriendlyCities();
	m_iFriendlyHealChange += belief->GetFriendlyHealChange();
	m_iCityStateFriendshipModifier += belief->GetCityStateFriendshipModifier();
	m_iLandBarbarianConversionPercent += belief->GetLandBarbarianConversionPercent();
	m_iSpreadDistanceModifier += belief->GetSpreadDistanceModifier();
	m_iSpreadStrengthModifier += belief->GetSpreadStrengthModifier();
	m_iProphetStrengthModifier += belief->GetProphetStrengthModifier();
	m_iProphetCostModifier += belief->GetProphetCostModifier();
	m_iMissionaryStrengthModifier += belief->GetMissionaryStrengthModifier();
#ifdef BELIEF_EXTRA_TRADE_ROUTES
	m_iExtraTradeRoutes	+= belief->GetExtraTradeRoutes();
#endif
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	m_iGoldenAgeTurns += belief->GetGoldenAgeTurns();
#endif
	m_iMissionaryCostModifier += belief->GetMissionaryCostModifier();
	m_iFriendlyCityStateSpreadModifier += belief->GetFriendlyCityStateSpreadModifier();
	m_iGreatPersonExpendedFaith += belief->GetGreatPersonExpendedFaith();
#ifdef GP_EXPENDED_GA
	m_iGreatPersonExpendedGoldenAge += belief->GetGreatPersonExpendedGoldenAge();
	m_iGoldenAgeCombatMod += belief->GetGoldenAgeCombatMod();
#endif
#ifdef NEW_BELIEF_PROPHECY
	if (!m_bAllowPolicyWonders)
	{
		m_bAllowPolicyWonders = belief->IsAllowPolicyWonders();
	}
#endif
#ifdef BELIEF_HALF_FAITH_IN_CITY
	if (!m_bHalfFaithInCity)
	{
		m_bHalfFaithInCity = belief->IsHalfFaithInCity();
	}
#endif
	m_iCityStateMinimumInfluence += belief->GetCityStateMinimumInfluence();
	m_iCityStateInfluenceModifier += belief->GetCityStateInfluenceModifier();
	m_iOtherReligionPressureErosion += belief->GetOtherReligionPressureErosion();
	m_iSpyPressure += belief->GetSpyPressure();
	m_iInquisitorPressureRetention += belief->GetInquisitorPressureRetention();
	m_iFaithBuildingTourism += belief->GetFaithBuildingTourism();
#ifdef GP_RATE_MODIFIER_FROM_BELIEF
	m_iGreatPeopleRateModifier += belief->GetGreatPeopleRateModifier();
#endif


	m_eObsoleteEra = belief->GetObsoleteEra();
	m_eResourceRevealed = belief->GetResourceRevealed();

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		if (belief->IsBuildingClassEnabled(iI))
		{
			m_paiBuildingClassEnabled[iI]++;
		}
	}
#ifdef DUEL_ALLOW_SAMETURN_BELIEFS
	if (m_paiBeliefAdoptionTurn[eBelief] < 0)
		m_paiBeliefAdoptionTurn[eBelief] = GC.getGame().getGameTurn();
#endif

	if(belief->GetSpreadModifierDoublingTech() != NO_TECH)
	{
		m_eSpreadModifierDoublingTech = belief->GetSpreadModifierDoublingTech();
	}

	m_ReligionBeliefs.push_back((int)eBelief);
}

/// Does this religion possess a specific belief?
bool CvReligionBeliefs::HasBelief(BeliefTypes eBelief) const
{
	return (find(m_ReligionBeliefs.begin(), m_ReligionBeliefs.end(), (int)eBelief) != m_ReligionBeliefs.end());
}

/// Does this religion possess a specific belief?
BeliefTypes CvReligionBeliefs::GetBelief(int iIndex) const
{
	return (BeliefTypes)m_ReligionBeliefs[iIndex];
}

/// Does this religion possess a specific belief?
int CvReligionBeliefs::GetNumBeliefs() const
{
	return m_ReligionBeliefs.size();
}

/// Faith from kills
int CvReligionBeliefs::GetFaithFromKills(int iDistance) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;
	int iRequiredDistance;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			iRequiredDistance = pBeliefs->GetEntry(i)->GetMaxDistance();
			if(iRequiredDistance == 0 || iDistance <= iRequiredDistance)
			{
				rtnValue += pBeliefs->GetEntry(i)->GetFaithFromKills();
			}
		}
	}

	return rtnValue;
}

/// Happiness per city
int CvReligionBeliefs::GetHappinessPerCity(int iPopulation) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if(iPopulation >= pBeliefs->GetEntry(i)->GetMinPopulation())
			{
				rtnValue += pBeliefs->GetEntry(i)->GetHappinessPerCity();
			}
		}
	}

	return rtnValue;
}

/// Happiness per X followers in foreign cities of powers you are not at war with
int CvReligionBeliefs::GetHappinessPerXPeacefulForeignFollowers() const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i) && pBeliefs->GetEntry(i)->GetHappinessPerXPeacefulForeignFollowers() > 0)
		{
			return pBeliefs->GetEntry(i)->GetHappinessPerXPeacefulForeignFollowers();
		}
	}

	return 0;
}

/// Wonder production boost
int CvReligionBeliefs:: GetWonderProductionModifier(EraTypes eWonderEra) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if((int)eWonderEra < (int)pBeliefs->GetEntry(i)->GetObsoleteEra())
			{
				rtnValue += pBeliefs->GetEntry(i)->GetWonderProductionModifier();
			}
		}
	}

	return rtnValue;
}

/// Player happiness boost
int CvReligionBeliefs:: GetPlayerHappiness(bool bAtPeace) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if(bAtPeace || !pBeliefs->GetEntry(i)->RequiresPeace())
			{
				rtnValue += pBeliefs->GetEntry(i)->GetPlayerHappiness();
			}
		}
	}

	return rtnValue;
}

/// Player culture modifier
int CvReligionBeliefs:: GetPlayerCultureModifier(bool bAtPeace) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if(bAtPeace || !pBeliefs->GetEntry(i)->RequiresPeace())
			{
				rtnValue += pBeliefs->GetEntry(i)->GetPlayerCultureModifier();
			}
		}
	}

	return rtnValue;
}

/// Happiness per following city
float CvReligionBeliefs:: GetHappinessPerFollowingCity() const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	float rtnValue = 0.0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetHappinessPerFollowingCity();
		}
	}

	return rtnValue;
}

/// Gold per following city
int CvReligionBeliefs:: GetGoldPerFollowingCity() const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetGoldPerFollowingCity();
		}
	}

	return rtnValue;
}

/// Gold per following city
int CvReligionBeliefs:: GetGoldPerXFollowers() const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetGoldPerXFollowers();
		}
	}

	return rtnValue;
}

/// Gold per following city
int CvReligionBeliefs:: GetGoldWhenCityAdopts() const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetGoldWhenCityAdopts();
		}
	}

	return rtnValue;
}

/// Science per other religion follower
int CvReligionBeliefs:: GetSciencePerOtherReligionFollower() const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetSciencePerOtherReligionFollower();
		}
	}

	return rtnValue;
}

/// City growth modifier
int CvReligionBeliefs::GetCityGrowthModifier(bool bAtPeace) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if(bAtPeace || !pBeliefs->GetEntry(i)->RequiresPeace())
			{
				rtnValue += pBeliefs->GetEntry(i)->GetCityGrowthModifier();
			}
		}
	}

	return rtnValue;
}

/// Extra yield
int CvReligionBeliefs::GetCityYieldChange(int iPopulation, YieldTypes eYield) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if(iPopulation >= pBeliefs->GetEntry(i)->GetMinPopulation())
			{
				rtnValue += pBeliefs->GetEntry(i)->GetCityYieldChange(eYield);
			}
		}
	}

	return rtnValue;
}

/// Extra holy city yield
int CvReligionBeliefs::GetHolyCityYieldChange (YieldTypes eYield) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
				rtnValue += pBeliefs->GetEntry(i)->GetHolyCityYieldChange(eYield);
		}
	}

	return rtnValue;
}

#ifdef BELIEF_GREAT_WORK_YIELD_CHANGES
///
int CvReligionBeliefs::GetGreatWorkYieldChange (YieldTypes eYield) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
				rtnValue += pBeliefs->GetEntry(i)->GetGreatWorkYieldChange(eYield);
		}
	}

	return rtnValue;
}

#endif
/// Extra yield for foreign cities following religion
int CvReligionBeliefs::GetYieldChangePerForeignCity(YieldTypes eYield) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetYieldChangePerForeignCity(eYield);
		}
	}

	return rtnValue;
}

/// Extra yield for foreign followers
int CvReligionBeliefs::GetYieldChangePerXForeignFollowers(YieldTypes eYield) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetYieldChangePerXForeignFollowers(eYield);
		}
	}

	return rtnValue;
}

/// Extra yield from this improvement
int CvReligionBeliefs::GetResourceQuantityModifier(ResourceTypes eResource) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetResourceQuantityModifier(eResource);
		}
	}

	return rtnValue;
}

/// Extra yield from this improvement
int CvReligionBeliefs::GetImprovementYieldChange(ImprovementTypes eImprovement, YieldTypes eYield) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetImprovementYieldChange(eImprovement, eYield);
		}
	}

	return rtnValue;
}

/// Get yield change from beliefs for a specific building class
int CvReligionBeliefs::GetBuildingClassYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYieldType, int iFollowers) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if(iFollowers >= pBeliefs->GetEntry(i)->GetMinFollowers())
			{
				rtnValue += pBeliefs->GetEntry(i)->GetBuildingClassYieldChange(eBuildingClass, eYieldType);
			}
		}
	}

	return rtnValue;
}

/// Get Happiness from beliefs for a specific building class
int CvReligionBeliefs::GetBuildingClassHappiness(BuildingClassTypes eBuildingClass, int iFollowers) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if(iFollowers >= pBeliefs->GetEntry(i)->GetMinFollowers())
			{
				rtnValue += pBeliefs->GetEntry(i)->GetBuildingClassHappiness(eBuildingClass);
			}
		}
	}

	return rtnValue;
}

/// Get Tourism from beliefs for a specific building class
int CvReligionBeliefs::GetBuildingClassTourism(BuildingClassTypes eBuildingClass) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetBuildingClassTourism(eBuildingClass);
		}
	}

	return rtnValue;
}

/// Get yield change from beliefs for a specific feature
int CvReligionBeliefs::GetFeatureYieldChange(FeatureTypes eFeature, YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetFeatureYieldChange(eFeature, eYieldType);
		}
	}

	return rtnValue;
}

/// Get yield change from beliefs for a specific resource
int CvReligionBeliefs::GetResourceYieldChange(ResourceTypes eResource, YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetResourceYieldChange(eResource, eYieldType);
		}
	}

	return rtnValue;
}

/// Get yield change from beliefs for a specific terrain
int CvReligionBeliefs::GetTerrainYieldChange(TerrainTypes eTerrain, YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetTerrainYieldChange(eTerrain, eYieldType);
		}
	}

	return rtnValue;
}

#ifdef BELIEF_SPECIALIST_YIELD_CHANGES
/// Get yield change from beliefs for a specific Specialist
int CvReligionBeliefs::GetSpecialistYieldChange(SpecialistTypes eSpecialist, YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetSpecialistYieldChange(eSpecialist, eYieldType);
		}
	}

	return rtnValue;
}

#endif
#ifdef BELIEF_HURRY_MODIFIERS
///
int CvReligionBeliefs::GetHurryModifier(HurryTypes eHurry) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetHurryModifier(eHurry);
		}
	}

	return rtnValue;
}

#endif
// Get happiness boost from a resource
int CvReligionBeliefs::GetResourceHappiness(ResourceTypes eResource) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetResourceHappiness(eResource);
		}
	}

	return rtnValue;
}

/// Get yield change from beliefs for a specialist being present in city
int CvReligionBeliefs::GetYieldChangeAnySpecialist(YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetYieldChangeAnySpecialist(eYieldType);
		}
	}

	return rtnValue;
}

/// Get yield change from beliefs for a trade route
int CvReligionBeliefs::GetYieldChangeTradeRoute(YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetYieldChangeTradeRoute(eYieldType);
		}
	}

	return rtnValue;
}

/// Get yield change from beliefs for a natural wonder
int CvReligionBeliefs::GetYieldChangeNaturalWonder(YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetYieldChangeNaturalWonder(eYieldType);
		}
	}

	return rtnValue;
}

/// Get yield change from beliefs for a world wonder
int CvReligionBeliefs::GetYieldChangeWorldWonder(YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetYieldChangeWorldWonder(eYieldType);
		}
	}

	return rtnValue;
}

/// Get yield modifier from beliefs for a natural wonder
int CvReligionBeliefs::GetYieldModifierNaturalWonder(YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetYieldModifierNaturalWonder(eYieldType);
		}
	}

	return rtnValue;
}

/// Get yield modifier from beliefs for a natural wonder
int CvReligionBeliefs::GetMaxYieldModifierPerFollower(YieldTypes eYieldType) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();
	int rtnValue = 0;

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			rtnValue += pBeliefs->GetEntry(i)->GetMaxYieldModifierPerFollower(eYieldType);
		}
	}

	return rtnValue;
}

/// Does this belief allow a building to be constructed?
bool CvReligionBeliefs::IsBuildingClassEnabled(BuildingClassTypes eType) const
{
	return m_paiBuildingClassEnabled[(int)eType];
}

/// Is there a belief that allows faith buying of units
bool CvReligionBeliefs::IsFaithBuyingEnabled(EraTypes eEra) const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if (pBeliefs->GetEntry(i)->IsFaithUnitPurchaseEra((int)eEra))
			{
				return true;
			}
		}
	}

	return false;
}

/// Is there a belief that allows us to convert adjacent barbarians?
bool CvReligionBeliefs::IsConvertsBarbarians() const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if (pBeliefs->GetEntry(i)->ConvertsBarbarians())
			{
				return true;
			}
		}
	}

	return false;
}

/// Is there a belief that allows faith buying of all great people
bool CvReligionBeliefs::IsFaithPurchaseAllGreatPeople() const
{
	CvBeliefXMLEntries* pBeliefs = GC.GetGameBeliefs();

	for(int i = 0; i < pBeliefs->GetNumBeliefs(); i++)
	{
		if(HasBelief((BeliefTypes)i))
		{
			if (pBeliefs->GetEntry(i)->FaithPurchaseAllGreatPeople())
			{
				return true;
			}
		}
	}

	return false;
}

/// Serialization read
void CvReligionBeliefs::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;

	kStream >> m_iFaithFromDyingUnits;
	kStream >> m_iRiverHappiness;
	kStream >> m_iPlotCultureCostModifier;
	kStream >> m_iCityRangeStrikeModifier;
	kStream >> m_iCombatModifierEnemyCities;
	kStream >> m_iCombatModifierFriendlyCities;
	kStream >> m_iFriendlyHealChange;
	kStream >> m_iCityStateFriendshipModifier;
	kStream >> m_iLandBarbarianConversionPercent;
	kStream >> m_iSpreadStrengthModifier;
	kStream >> m_iSpreadDistanceModifier;
	kStream >> m_iProphetStrengthModifier;
	kStream >> m_iProphetCostModifier;
	kStream >> m_iMissionaryStrengthModifier;
#ifdef BELIEF_EXTRA_TRADE_ROUTES
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
# endif
		kStream >> m_iExtraTradeRoutes;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iExtraTradeRoutes = 0;
	}
# endif
#endif
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
# endif
		kStream >> m_iGoldenAgeTurns;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iGoldenAgeTurns = 0;
	}
# endif
#endif
	kStream >> m_iMissionaryCostModifier;
	kStream >> m_iFriendlyCityStateSpreadModifier;
	kStream >> m_iGreatPersonExpendedFaith;
#ifdef GP_EXPENDED_GA
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1001)
	{
# endif
		kStream >> m_iGreatPersonExpendedGoldenAge;
		kStream >> m_iGoldenAgeCombatMod;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iGreatPersonExpendedGoldenAge = 0;
		m_iGoldenAgeCombatMod = 0;
	}
# endif
#endif
#ifdef NEW_BELIEF_PROPHECY
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1001)
	{
# endif
		kStream >> m_bAllowPolicyWonders;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bAllowPolicyWonders = false;
	}
# endif
#endif
#ifdef BELIEF_HALF_FAITH_IN_CITY
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1003)
	{
# endif
		kStream >> m_bHalfFaithInCity;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_bHalfFaithInCity = false;
	}
# endif
#endif
	kStream >> m_iCityStateMinimumInfluence;
	kStream >> m_iCityStateInfluenceModifier;
	kStream >> m_iOtherReligionPressureErosion;
	kStream >> m_iSpyPressure;
	kStream >> m_iInquisitorPressureRetention;
	if (uiVersion >= 2)
	{
		kStream >> m_iFaithBuildingTourism;
	}
	else
	{
		m_iFaithBuildingTourism = 0;
	}
#ifdef GP_RATE_MODIFIER_FROM_BELIEF
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
# endif
		kStream >> m_iGreatPeopleRateModifier;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iGreatPeopleRateModifier = 0;
	}
# endif
#endif


	kStream >> m_eObsoleteEra;
	kStream >> m_eResourceRevealed;
	kStream >> m_eSpreadModifierDoublingTech;

	m_ReligionBeliefs.clear();
	uint uiBeliefCount;
	kStream >> uiBeliefCount;
	while(uiBeliefCount--)
	{
		int iBeliefIndex = CvInfosSerializationHelper::ReadHashed(kStream);
		m_ReligionBeliefs.push_back(iBeliefIndex);
	}

	BuildingClassArrayHelpers::Read(kStream, m_paiBuildingClassEnabled);
#ifdef DUEL_ALLOW_SAMETURN_BELIEFS
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1002)
	{
# endif
		int iNumEntries;
		int iType;

		kStream >> iNumEntries;

		for (int iI = 0; iI < iNumEntries; iI++)
		{
			bool bValid = true;
			iType = CvInfosSerializationHelper::ReadHashed(kStream, &bValid);
			if (iType != -1 || !bValid)
			{
				if (iType != -1)
				{
					kStream >> m_paiBeliefAdoptionTurn[iType];
				}
				else
				{
					CvString szError;
					szError.Format("LOAD ERROR: Belief Type not found");
					GC.LogMessage(szError.GetCString());
					CvAssertMsg(false, szError);
					int iDummy;
					kStream >> iDummy;	// Skip it.
				}
			}
		}
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		for (int iI = 0; iI < GC.getNumBeliefInfos(); iI++)
		{
			m_paiBeliefAdoptionTurn[iI] = -1;
		}
	}
# endif
#endif
}

/// Serialization write
void CvReligionBeliefs::Write(FDataStream& kStream) const
{
	// Current version number
	uint uiVersion = 2;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	uiVersion = BUMP_SAVE_VERSION_BELIEFS;
#endif
	kStream << uiVersion;

	kStream << m_iFaithFromDyingUnits;
	kStream << m_iRiverHappiness;
	kStream << m_iPlotCultureCostModifier;
	kStream << m_iCityRangeStrikeModifier;
	kStream << m_iCombatModifierEnemyCities;
	kStream << m_iCombatModifierFriendlyCities;
	kStream << m_iFriendlyHealChange;
	kStream << m_iCityStateFriendshipModifier;
	kStream << m_iLandBarbarianConversionPercent;
	kStream << m_iSpreadStrengthModifier;
	kStream << m_iSpreadDistanceModifier;
	kStream << m_iProphetStrengthModifier;
	kStream << m_iProphetCostModifier;
	kStream << m_iMissionaryStrengthModifier;
#ifdef BELIEF_EXTRA_TRADE_ROUTES
	kStream << m_iExtraTradeRoutes;
#endif
#ifdef NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
	kStream << m_iGoldenAgeTurns;
#endif
	kStream << m_iMissionaryCostModifier;
	kStream << m_iFriendlyCityStateSpreadModifier;
	kStream << m_iGreatPersonExpendedFaith;
#ifdef GP_EXPENDED_GA
	kStream << m_iGreatPersonExpendedGoldenAge;
	kStream << m_iGoldenAgeCombatMod;
#endif
#ifdef NEW_BELIEF_PROPHECY
	kStream << m_bAllowPolicyWonders;
#endif
#ifdef BELIEF_HALF_FAITH_IN_CITY
	kStream << m_bHalfFaithInCity;
#endif
	kStream << m_iCityStateMinimumInfluence;
	kStream << m_iCityStateInfluenceModifier;
	kStream << m_iOtherReligionPressureErosion;
	kStream << m_iSpyPressure;
	kStream << m_iInquisitorPressureRetention;
	kStream << m_iFaithBuildingTourism;
#ifdef GP_RATE_MODIFIER_FROM_BELIEF
	kStream << m_iGreatPeopleRateModifier;
#endif


	kStream << m_eObsoleteEra;
	kStream << m_eResourceRevealed;
	kStream << m_eSpreadModifierDoublingTech;

	// m_ReligionBeliefs contains the BeliefTypes, which are indices into the religion info table (GC.getBeliefInfo).  Write out the info hashes
	kStream << m_ReligionBeliefs.size();
	for (uint i = 0; i < m_ReligionBeliefs.size(); ++i)
	{
		CvInfosSerializationHelper::WriteHashed(kStream, (BeliefTypes)m_ReligionBeliefs[i]);
	}

	BuildingClassArrayHelpers::Write(kStream, m_paiBuildingClassEnabled, GC.getNumBuildingClassInfos());
#ifdef DUEL_ALLOW_SAMETURN_BELIEFS
	kStream << GC.getNumBeliefInfos();

	for (int iI = 0; iI < GC.getNumBeliefInfos(); iI++)
	{
		const BeliefTypes eBelief = static_cast<BeliefTypes>(iI);
		CvBeliefEntry* pkBeliefInfo = GC.getBeliefInfo(eBelief);
		if (pkBeliefInfo)
		{
			CvInfosSerializationHelper::WriteHashed(kStream, pkBeliefInfo);
			kStream << m_paiBeliefAdoptionTurn[iI];
		}
		else
		{
			kStream << (int)0;
		}
	}
#endif
}

/// BELIEF HELPER CLASSES

/// Is there an adjacent barbarian naval unit that could be converted?
bool CvBeliefHelpers::ConvertBarbarianUnit(CvPlayer *pPlayer, UnitHandle pUnit)
{
	UnitHandle pNewUnit;
	CvPlot *pPlot = pUnit->plot();

	// Convert the barbarian into our unit
	pNewUnit = pPlayer->initUnit(pUnit->getUnitType(), pUnit->getX(), pUnit->getY(), pUnit->AI_getUnitAIType(), NO_DIRECTION, true /*bNoMove*/, false);
	CvAssertMsg(pNewUnit, "pNewUnit is not assigned a valid value");
	pNewUnit->convert(pUnit.pointer(), false);
	pNewUnit->setupGraphical();
	pNewUnit->finishMoves(); // No move first turn

	if(GC.getLogging() && GC.getAILogging())
	{
		CvString logMsg;
		logMsg.Format("Converted barbarian (with belief), X: %d, Y: %d", pUnit->getX(), pUnit->getY());
		pPlayer->GetHomelandAI()->LogHomelandMessage(logMsg);
	}

	CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_HEATHEN_CONVERTS");
	CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_HEATHEN_CONVERTS");
	pPlayer->GetNotifications()->Add(NOTIFICATION_GENERIC, strBuffer, strSummary, pUnit->getX(), pUnit->getY(), -1);

	if (pPlot->getImprovementType() == GC.getBARBARIAN_CAMP_IMPROVEMENT())
	{
		pPlot->setImprovementType(NO_IMPROVEMENT);

		CvBarbarians::DoBarbCampCleared(pPlot, pPlayer->GetID());
		pPlot->SetPlayerThatClearedBarbCampHere(pPlayer->GetID());

		// Don't give gold for Camps cleared by heathen conversion
	}

	return true;
}