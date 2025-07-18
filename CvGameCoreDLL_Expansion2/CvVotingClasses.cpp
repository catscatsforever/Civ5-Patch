/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
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
#include "CvMinorCivAI.h"
#include "CvGrandStrategyAI.h"
#include "CvEconomicAI.h"
#include "CvTechAI.h"
#include "cvStopWatch.h"

#include "LintFree.h"


// ================================================================================
//			LeagueHelpers
// ================================================================================
CvString LeagueHelpers::GetTextForChoice(ResolutionDecisionTypes eDecision, int iChoice)
{
	CvString s = "";
	switch (eDecision)
	{
	case RESOLUTION_DECISION_YES_OR_NO:
	case RESOLUTION_DECISION_REPEAL:
		{
			CvAssertMsg(iChoice == CHOICE_NO || iChoice == CHOICE_YES, "Unexpected choice when handling a Yes/No decision. Please send Anton your save file and version.");
			if (iChoice == CHOICE_NO)
			{
				s = Localization::Lookup("TXT_KEY_RESOLUTION_CHOICE_NO").toUTF8();
			}
			else if (iChoice == CHOICE_YES)
			{
				s = Localization::Lookup("TXT_KEY_RESOLUTION_CHOICE_YES").toUTF8();
			}
			break;
		}
	case RESOLUTION_DECISION_ANY_MEMBER:
	case RESOLUTION_DECISION_MAJOR_CIV_MEMBER:
	case RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER:
		{
			CvAssertMsg(iChoice >= 0 && iChoice < MAX_CIV_PLAYERS, "Unexpected choice when handling a decision on a player. Please send Anton your save file and version.");
			if (iChoice >= 0 && iChoice < MAX_CIV_PLAYERS)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_RESOLUTION_CHOICE_PLAYER");
				sTemp << GET_PLAYER((PlayerTypes)iChoice).getCivilizationShortDescriptionKey();
				s = sTemp.toUTF8();
			}
			break;
		}
	case RESOLUTION_DECISION_CITY:
		{
			s = Localization::Lookup("TXT_KEY_RESOLUTION_CHOICE_CITY").toUTF8();
			break;
		}
	case RESOLUTION_DECISION_ANY_LUXURY_RESOURCE:
		{		
			CvResourceInfo* pInfo = GC.getResourceInfo((ResourceTypes)iChoice);
			CvAssertMsg(pInfo != NULL, "Unexpected choice when handling a decision on a resource. Please send Anton your save file and version.");
			if (pInfo != NULL)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_RESOLUTION_CHOICE_RESOURCE");
				sTemp << pInfo->GetIconString() << pInfo->GetDescriptionKey();
				s = sTemp.toUTF8();
			}
			break;
		}
	case RESOLUTION_DECISION_RELIGION:
		{
			CvReligionEntry* pInfo = GC.getReligionInfo((ReligionTypes)iChoice);
			CvAssertMsg(pInfo != NULL, "Unexpected choice when handling a decision on a religion. Please send Anton your save file and version.");
			const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion((ReligionTypes)iChoice, NO_PLAYER);
			CvAssert(pReligion != NULL);
			if (pInfo != NULL && pReligion != NULL)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_RESOLUTION_CHOICE_RELIGION");
				sTemp << pInfo->GetIconString() << pReligion->GetName();
				s = sTemp.toUTF8();
			}
			break;
		}
	case RESOLUTION_DECISION_IDEOLOGY:
		{
			CvPolicyBranchEntry* pInfo = GC.getPolicyBranchInfo((PolicyBranchTypes)iChoice);
			CvAssertMsg(pInfo != NULL, "Unexpected choice when handling a decision on an ideology. Please send Anton your save file and version.");
			if (pInfo != NULL)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_RESOLUTION_CHOICE_IDEOLOGY");
				sTemp << pInfo->GetDescriptionKey();
				s = sTemp.toUTF8();
			}
			break;
		}
	default:
		{
			break;
		}
	}
	return s;
}

EraTypes LeagueHelpers::GetGameEraForTrigger()
{
	EraTypes eGameEra = NO_ERA;

	// Game era is one era less than the most advanced player
	EraTypes eMostAdvancedEra = NO_ERA;
	for (int i = 0; i < MAX_MAJOR_CIVS; i++)
	{
		if (GET_PLAYER((PlayerTypes)i).isAlive())
		{
			EraTypes e = GET_PLAYER((PlayerTypes)i).GetCurrentEra();
			if (GET_PLAYER((PlayerTypes)i).GetCurrentEra() > eMostAdvancedEra)
			{
				eMostAdvancedEra = e;
			}
		}
	}
	if (eMostAdvancedEra - 1 > NO_ERA)
	{
		eGameEra = (EraTypes) ((int)eMostAdvancedEra - 1);
	}

	// Unless half or more civs are in this era too, then it is this era
	int iInMostAdvancedEra = 0;
	for (int i = 0; i < MAX_MAJOR_CIVS; i++)
	{
		if (GET_PLAYER((PlayerTypes)i).isAlive())
		{
			EraTypes e = GET_PLAYER((PlayerTypes)i).GetCurrentEra();
			CvAssert(e <= eMostAdvancedEra);
			if (e == eMostAdvancedEra)
			{
				iInMostAdvancedEra++;
			}
		}
	}
	if (iInMostAdvancedEra >= (GC.getGame().countMajorCivsAlive() / 2))
	{
		eGameEra = eMostAdvancedEra;
	}

	return eGameEra;
}

EraTypes LeagueHelpers::GetNextGameEraForTrigger()
{
	return LeagueHelpers::GetNextGameEraForTrigger(LeagueHelpers::GetGameEraForTrigger());
}

EraTypes LeagueHelpers::GetNextGameEraForTrigger(EraTypes eThisEra)
{
	EraTypes eNextEra = (EraTypes) ((int)eThisEra + 1);

	if (eNextEra >= GC.getNumEraInfos())
	{
		eNextEra = NO_ERA;
	}

	return eNextEra;
}


// ================================================================================
//			CvResolutionEffects
// ================================================================================
CvResolutionEffects::CvResolutionEffects(void)
{
	bDiplomaticVictory = false;
	bChangeLeagueHost = false;
	iOneTimeGold = 0;
	iOneTimeGoldPercent = 0;
	bRaiseCityStateInfluenceToNeutral = false;
	eLeagueProjectEnabled = NO_LEAGUE_PROJECT;
	iGoldPerTurn = 0;
	iResourceQuantity = 0;
	bEmbargoCityStates = false;
	bEmbargoPlayer = false;
	bNoResourceHappiness = false;
	iUnitMaintenanceGoldPercent = 0;
	iMemberDiscoveredTechMod = 0;
	iCulturePerWonder = 0;
	iCulturePerNaturalWonder = 0;
	bNoTrainingNuclearWeapons = false;
	iVotesForFollowingReligion = 0;
	iHolyCityTourism = 0;
	iReligionSpreadStrengthMod = 0;
	iVotesForFollowingIdeology = 0;
	iOtherIdeologyRebellionMod = 0;
	iArtsyGreatPersonRateMod = 0;
	iScienceyGreatPersonRateMod = 0;
	iGreatPersonTileImprovementCulture = 0;
	iLandmarkCulture = 0;
#ifdef NEW_LEAGUE_RESOLUTIONS
	iTradeRouteGoldModifier = 0;
	iCSBonuModifier = 0;
	bNoSpiesInCS = false;
	bDoubleResourceHappiness = false;
#endif
}

CvResolutionEffects::CvResolutionEffects(ResolutionTypes eType)
{
	CvResolutionEntry* pInfo = GC.getResolutionInfo(eType);	
	CvAssertMsg(pInfo, "Resolution info is null when instantiating ResolutionEffects. Please send Anton your save file and version.");
	if (pInfo)
	{
		bDiplomaticVictory					= pInfo->IsDiplomaticVictory();
		bChangeLeagueHost					= pInfo->IsChangeLeagueHost();
		iOneTimeGold						= pInfo->GetOneTimeGold();
		iOneTimeGoldPercent					= pInfo->GetOneTimeGoldPercent();
		bRaiseCityStateInfluenceToNeutral	= pInfo->IsRaiseCityStateInfluenceToNeutral();
		eLeagueProjectEnabled				= pInfo->GetLeagueProjectEnabled();
		iGoldPerTurn						= pInfo->GetGoldPerTurn();
		iResourceQuantity					= pInfo->GetResourceQuantity();
		bEmbargoCityStates					= pInfo->IsEmbargoCityStates();
		bEmbargoPlayer						= pInfo->IsEmbargoPlayer();
		bNoResourceHappiness				= pInfo->IsNoResourceHappiness();
		iUnitMaintenanceGoldPercent			= pInfo->GetUnitMaintenanceGoldPercent();
		iMemberDiscoveredTechMod			= pInfo->GetMemberDiscoveredTechMod();
		iCulturePerWonder					= pInfo->GetCulturePerWonder();
		iCulturePerNaturalWonder			= pInfo->GetCulturePerNaturalWonder();
		bNoTrainingNuclearWeapons			= pInfo->IsNoTrainingNuclearWeapons();
		iVotesForFollowingReligion			= pInfo->GetVotesForFollowingReligion();
		iHolyCityTourism					= pInfo->GetHolyCityTourism();
		iReligionSpreadStrengthMod			= pInfo->GetReligionSpreadStrengthMod();
		iVotesForFollowingIdeology			= pInfo->GetVotesForFollowingIdeology();
		iOtherIdeologyRebellionMod			= pInfo->GetOtherIdeologyRebellionMod();
		iArtsyGreatPersonRateMod			= pInfo->GetArtsyGreatPersonRateMod();
		iScienceyGreatPersonRateMod			= pInfo->GetScienceyGreatPersonRateMod();
		iGreatPersonTileImprovementCulture	= pInfo->GetGreatPersonTileImprovementCulture();
		iLandmarkCulture					= pInfo->GetLandmarkCulture();
#ifdef NEW_LEAGUE_RESOLUTIONS
		iTradeRouteGoldModifier				= pInfo->GetTradeRouteGoldModifier();
		iCSBonuModifier						= pInfo->GetCSBonuModifier();
		bNoSpiesInCS						= pInfo->GetNoSpiesInCS();
		bDoubleResourceHappiness			= pInfo->GetDoubleResourceHappiness();
#endif
	}
}

CvResolutionEffects::~CvResolutionEffects(void)
{
}

bool CvResolutionEffects::HasOngoingEffects() const
{
	if (iGoldPerTurn != 0)
		return true;

	if (iResourceQuantity != 0)
		return true;

	if (bEmbargoCityStates)
		return true;

	if (bEmbargoPlayer)
		return true;

	if (bNoResourceHappiness)
		return true;

	if (iUnitMaintenanceGoldPercent != 0)
		return true;

	if (iMemberDiscoveredTechMod != 0)
		return true;

	if (iCulturePerWonder != 0)
		return true;

	if (iCulturePerNaturalWonder != 0)
		return true;

	if (bNoTrainingNuclearWeapons)
		return true;

	if (iVotesForFollowingReligion != 0)
		return true;

	if (iHolyCityTourism != 0)
		return true;

	if (iReligionSpreadStrengthMod != 0)
		return true;

	if (iVotesForFollowingIdeology != 0)
		return true;

	if (iOtherIdeologyRebellionMod != 0)
		return true;

	if (iArtsyGreatPersonRateMod != 0)
		return true;

	if (iScienceyGreatPersonRateMod != 0)
		return true;

	if (iGreatPersonTileImprovementCulture != 0)
		return true;

	if (iLandmarkCulture != 0)
		return true;

#ifdef NEW_LEAGUE_RESOLUTIONS
	if (iTradeRouteGoldModifier != 0)
		return true;

	if (iCSBonuModifier != 0)
		return true;

	if (bNoSpiesInCS)
		return true;

	if (bDoubleResourceHappiness)
		return true;
#endif

	return false;
}

// Used to sum multiple CvResolutionEffects and reflect them in a single struct.
// Some information may be incomplete (ex. trade embargo which targets a player
// chosen by proposer), so client should track additional information as necessary.
void CvResolutionEffects::AddOngoingEffects(const CvResolutionEffects* pOtherEffects)
{
	if (!pOtherEffects->HasOngoingEffects())
		return;

	iGoldPerTurn							+= pOtherEffects->iGoldPerTurn;
	iResourceQuantity						+= pOtherEffects->iResourceQuantity; // target resource
	bEmbargoCityStates						|= pOtherEffects->bEmbargoCityStates;
	bEmbargoPlayer							|= pOtherEffects->bEmbargoPlayer; // target player
	bNoResourceHappiness					|= pOtherEffects->bNoResourceHappiness; // target resource
	iUnitMaintenanceGoldPercent				+= pOtherEffects->iUnitMaintenanceGoldPercent;
	iMemberDiscoveredTechMod				+= pOtherEffects->iMemberDiscoveredTechMod;
	iCulturePerWonder						+= pOtherEffects->iCulturePerWonder;
	iCulturePerNaturalWonder				+= pOtherEffects->iCulturePerNaturalWonder;
	bNoTrainingNuclearWeapons				|= pOtherEffects->bNoTrainingNuclearWeapons;
	iVotesForFollowingReligion				+= pOtherEffects->iVotesForFollowingReligion; // target religion
	iHolyCityTourism						+= pOtherEffects->iHolyCityTourism; // target religion
	iReligionSpreadStrengthMod				+= pOtherEffects->iReligionSpreadStrengthMod; // target religion
	iVotesForFollowingIdeology				+= pOtherEffects->iVotesForFollowingIdeology; // target ideology
	iOtherIdeologyRebellionMod				+= pOtherEffects->iOtherIdeologyRebellionMod; // target ideology
	iArtsyGreatPersonRateMod				+= pOtherEffects->iArtsyGreatPersonRateMod;
	iScienceyGreatPersonRateMod				+= pOtherEffects->iScienceyGreatPersonRateMod;
	iGreatPersonTileImprovementCulture		+= pOtherEffects->iGreatPersonTileImprovementCulture;
	iLandmarkCulture						+= pOtherEffects->iLandmarkCulture;
#ifdef NEW_LEAGUE_RESOLUTIONS
	iTradeRouteGoldModifier					+= pOtherEffects->iTradeRouteGoldModifier;
	iCSBonuModifier							+= pOtherEffects->iCSBonuModifier;
	bNoSpiesInCS							|= pOtherEffects->bNoSpiesInCS;
	bDoubleResourceHappiness				|= pOtherEffects->bDoubleResourceHappiness;
#endif
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvResolutionEffects& writeTo)
{
	uint uiVersion;

	loadFrom >> uiVersion;
	loadFrom >> writeTo.bDiplomaticVictory;
	if (uiVersion >= 2)
	{
		loadFrom >> writeTo.bChangeLeagueHost;
	}
	else
	{
		writeTo.bChangeLeagueHost = false;
	}
	loadFrom >> writeTo.iOneTimeGold;
	loadFrom >> writeTo.iOneTimeGoldPercent;
	loadFrom >> writeTo.bRaiseCityStateInfluenceToNeutral;
	if (uiVersion >= 3)
	{
		loadFrom >> writeTo.eLeagueProjectEnabled;
	}
	else
	{
		writeTo.eLeagueProjectEnabled = NO_LEAGUE_PROJECT;
	}
	loadFrom >> writeTo.iGoldPerTurn;
	loadFrom >> writeTo.iResourceQuantity;
	loadFrom >> writeTo.bEmbargoCityStates;
	loadFrom >> writeTo.bEmbargoPlayer;
	loadFrom >> writeTo.bNoResourceHappiness;
	loadFrom >> writeTo.iUnitMaintenanceGoldPercent;
	loadFrom >> writeTo.iMemberDiscoveredTechMod;
	if (uiVersion >= 4)
	{
		loadFrom >> writeTo.iCulturePerWonder;
		loadFrom >> writeTo.iCulturePerNaturalWonder;
	}
	else
	{
		writeTo.iCulturePerWonder = 0;
		writeTo.iCulturePerNaturalWonder = 0;
	}
	if (uiVersion >= 5)
	{
		loadFrom >> writeTo.bNoTrainingNuclearWeapons;
	}
	else
	{
		writeTo.bNoTrainingNuclearWeapons = false;
	}
	if (uiVersion >= 6)
	{
		loadFrom >> writeTo.iVotesForFollowingReligion;
		loadFrom >> writeTo.iHolyCityTourism;
	}
	else
	{
		writeTo.iVotesForFollowingReligion = 0;
		writeTo.iHolyCityTourism = 0;		
	}
	if (uiVersion >= 9)
	{
		loadFrom >> writeTo.iReligionSpreadStrengthMod;
	}
	else
	{
		writeTo.iReligionSpreadStrengthMod = 0;
	}
	if (uiVersion >= 6)
	{
		loadFrom >> writeTo.iVotesForFollowingIdeology;
		loadFrom >> writeTo.iOtherIdeologyRebellionMod;
	}
	else
	{
		writeTo.iVotesForFollowingIdeology = 0;
		writeTo.iOtherIdeologyRebellionMod = 0;
	}
	if (uiVersion >= 7)
	{
		loadFrom >> writeTo.iArtsyGreatPersonRateMod;
		loadFrom >> writeTo.iScienceyGreatPersonRateMod;
	}
	else
	{
		writeTo.iArtsyGreatPersonRateMod = 0;
		writeTo.iScienceyGreatPersonRateMod = 0;
	}
	if (uiVersion >= 8)
	{
		loadFrom >> writeTo.iGreatPersonTileImprovementCulture;
		loadFrom >> writeTo.iLandmarkCulture;
	}
	else
	{
		writeTo.iGreatPersonTileImprovementCulture = 0;
		writeTo.iLandmarkCulture = 0;
	}
#ifdef NEW_LEAGUE_RESOLUTIONS
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
#endif
		loadFrom >> writeTo.iTradeRouteGoldModifier;
		loadFrom >> writeTo.iCSBonuModifier;
		loadFrom >> writeTo.bNoSpiesInCS;
		loadFrom >> writeTo.bDoubleResourceHappiness;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		writeTo.iTradeRouteGoldModifier = 0;
		writeTo.iCSBonuModifier = 0;
		writeTo.bNoSpiesInCS = false;
		writeTo.bDoubleResourceHappiness = false;
	}
#endif
#endif
	
	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvResolutionEffects& readFrom)
{
	uint uiVersion = 9;

#ifdef SAVE_BACKWARDS_COMPATIBILITY
	uiVersion = BUMP_SAVE_VERSION_RESOLUTIONEFFECTS;
#endif
	saveTo << uiVersion;
	saveTo << readFrom.bDiplomaticVictory;
	saveTo << readFrom.bChangeLeagueHost;
	saveTo << readFrom.iOneTimeGold;
	saveTo << readFrom.iOneTimeGoldPercent;
	saveTo << readFrom.bRaiseCityStateInfluenceToNeutral;
	saveTo << readFrom.eLeagueProjectEnabled;
	saveTo << readFrom.iGoldPerTurn;
	saveTo << readFrom.iResourceQuantity;
	saveTo << readFrom.bEmbargoCityStates;
	saveTo << readFrom.bEmbargoPlayer;
	saveTo << readFrom.bNoResourceHappiness;
	saveTo << readFrom.iUnitMaintenanceGoldPercent;
	saveTo << readFrom.iMemberDiscoveredTechMod;
	saveTo << readFrom.iCulturePerWonder;
	saveTo << readFrom.iCulturePerNaturalWonder;
	saveTo << readFrom.bNoTrainingNuclearWeapons;
	saveTo << readFrom.iVotesForFollowingReligion;
	saveTo << readFrom.iHolyCityTourism;
	saveTo << readFrom.iReligionSpreadStrengthMod;
	saveTo << readFrom.iVotesForFollowingIdeology;
	saveTo << readFrom.iOtherIdeologyRebellionMod;
	saveTo << readFrom.iArtsyGreatPersonRateMod;
	saveTo << readFrom.iScienceyGreatPersonRateMod;
	saveTo << readFrom.iGreatPersonTileImprovementCulture;
	saveTo << readFrom.iLandmarkCulture;
#ifdef NEW_LEAGUE_RESOLUTIONS
	saveTo << readFrom.iTradeRouteGoldModifier;
	saveTo << readFrom.iCSBonuModifier;
	saveTo << readFrom.bNoSpiesInCS;
	saveTo << readFrom.bDoubleResourceHappiness;
#endif

	return saveTo;
}


// ================================================================================
//			CvResolutionDecision
// ================================================================================
CvResolutionDecision::CvResolutionDecision(void)
{
	m_eType = RESOLUTION_DECISION_NONE;
}

CvResolutionDecision::CvResolutionDecision(ResolutionDecisionTypes eType)
{
	m_eType = eType;
}

CvResolutionDecision::~CvResolutionDecision(void)
{
}

CvResolutionDecision::PlayerVote::PlayerVote(void)
{
	ePlayer = NO_PLAYER;
	iNumVotes = 0;
	iChoice = LeagueHelpers::CHOICE_NONE;
}

CvResolutionDecision::PlayerVote::~PlayerVote(void)
{
}

ResolutionDecisionTypes CvResolutionDecision::GetType() const
{
	return m_eType;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvResolutionDecision::PlayerVote& writeTo)
{
	uint uiVersion;

	loadFrom >> uiVersion;
	loadFrom >> writeTo.ePlayer;
	loadFrom >> writeTo.iNumVotes;
	loadFrom >> writeTo.iChoice;

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvResolutionDecision::PlayerVote& readFrom)
{
	uint uiVersion = 1;

	saveTo << uiVersion;
	saveTo << readFrom.ePlayer;
	saveTo << readFrom.iNumVotes;
	saveTo << readFrom.iChoice;

	return saveTo;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvResolutionDecision& writeTo)
{
	uint uiVersion;

	loadFrom >> uiVersion;
	loadFrom >> writeTo.m_eType;

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvResolutionDecision& readFrom)
{
	uint uiVersion = 1;

	saveTo << uiVersion;
	saveTo << readFrom.m_eType;

	return saveTo;
}


// ================================================================================
//			CvProposerDecision
// ================================================================================
CvProposerDecision::CvProposerDecision(void)
{
}

// Constructor sets the proposer's decision, made beforehand
CvProposerDecision::CvProposerDecision(ResolutionDecisionTypes eType, PlayerTypes eProposalPlayer, int iChoice) : CvResolutionDecision(eType)
{
	m_sVote.ePlayer = eProposalPlayer;
	m_sVote.iNumVotes = 1;
	m_sVote.iChoice = iChoice;
}

CvProposerDecision::~CvProposerDecision(void)
{
}

int CvProposerDecision::GetDecision()
{
	return m_sVote.iChoice;
}

PlayerTypes CvProposerDecision::GetProposer()
{
	return m_sVote.ePlayer;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvProposerDecision& writeTo)
{
	loadFrom >> (CvResolutionDecision&) writeTo;

	uint uiVersion;

	loadFrom >> uiVersion;
	loadFrom >> writeTo.m_sVote;

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvProposerDecision& readFrom)
{
	saveTo << (CvResolutionDecision&) readFrom;

	uint uiVersion = 1;

	saveTo << uiVersion;
	saveTo << readFrom.m_sVote;

	return saveTo;
}


// ================================================================================
//			CvVoterDecision
// ================================================================================
CvVoterDecision::CvVoterDecision(void)
{
	m_vVotes.clear();
}

CvVoterDecision::CvVoterDecision(ResolutionDecisionTypes eType) : CvResolutionDecision(eType)
{
	m_vVotes.clear();
}

CvVoterDecision::~CvVoterDecision(void)
{
}

int CvVoterDecision::GetDecision()
{
	CvWeightedVector<int, 64, true> vChoices;
	for (PlayerVoteList::iterator it = m_vVotes.begin(); it != m_vVotes.end(); it++)
	{
		bool bFirst = true;
		
		// Add the votes to an existing choice tally
		for (int i = 0; i < vChoices.size(); i++)
		{
			if (vChoices.GetElement(i) == it->iChoice)
			{
				bFirst = false;
				vChoices.SetWeight(i, vChoices.GetWeight(i) + it->iNumVotes);
				break;
			}
		}

		// Add the votes to a new tally for a new choice
		if (bFirst)
		{
			vChoices.push_back(it->iChoice, it->iNumVotes);
		}
	}

	if (vChoices.size() > 0)
	{
		vChoices.SortItems();
		
		// Is there a tie?
		if (vChoices.size() > 1)
		{
			if (vChoices.GetWeight(0) == vChoices.GetWeight(1))
			{
				switch (GetType())
				{
					// Decisions about players go to the host in a tie, as long as the host is part of the tie
					case RESOLUTION_DECISION_ANY_MEMBER:
					case RESOLUTION_DECISION_MAJOR_CIV_MEMBER:
					case RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER:
						{
							PlayerTypes eHost = NO_PLAYER;
							CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
							if (pLeague != NULL)
							{
								eHost = pLeague->GetHostMember();
							}

							int iWeightAtTop = vChoices.GetWeight(0);
							for (int i = 0; i < vChoices.size(); i++)
							{
								CvAssert(vChoices.GetWeight(i) <= iWeightAtTop);
								if (vChoices.GetWeight(i) < iWeightAtTop)
								{
									break;
								}

								if ((PlayerTypes)vChoices.GetElement(i) == eHost)
								{
									return eHost;
								}
							}
						}
						break;
					// All other decisions are CHOICE_NONE in a tie, so nothing happens
					default:
						{
							return LeagueHelpers::CHOICE_NONE;
						}
				}
			}
		}
		
		return vChoices.GetElement(0);
	}
	return LeagueHelpers::CHOICE_NONE;
}

bool CvVoterDecision::IsTie()
{
	std::vector<int> vTop = GetTopVotedChoices(1);

	return (vTop.size() != 1);
}

std::vector<int> CvVoterDecision::GetTopVotedChoices(int iNumTopChoices)
{
	CvWeightedVector<int, 64, true> vChoices;
	for (PlayerVoteList::iterator it = m_vVotes.begin(); it != m_vVotes.end(); it++)
	{
		bool bFirst = true;

		// Add the votes to an existing choice tally
		for (int i = 0; i < vChoices.size(); i++)
		{
			if (vChoices.GetElement(i) == it->iChoice)
			{
				bFirst = false;
				vChoices.SetWeight(i, vChoices.GetWeight(i) + it->iNumVotes);
				break;
			}
		}

		// Add the votes to a new tally for a new choice
		if (bFirst)
		{
			vChoices.push_back(it->iChoice, it->iNumVotes);
		}
	}

	std::vector<int> vTopChoices;
	if (vChoices.size() > 0 && iNumTopChoices > 0)
	{
		vChoices.SortItems();
		int iCurrentWeight = 0;
		for (int i = 0; i < vChoices.size(); i++)
		{
			if ((int)vTopChoices.size() < iNumTopChoices)
			{
				vTopChoices.push_back(vChoices.GetElement(i));
				iCurrentWeight = vChoices.GetWeight(i);
			}
			else if (iCurrentWeight == vChoices.GetWeight(i))
			{
				// Include tied choices
				vTopChoices.push_back(vChoices.GetElement(i));
			}
			else
			{
				break;
			}
		}
	}

	return vTopChoices;
}

int CvVoterDecision::GetVotesCast()
{
	int iCount = 0;
	for (PlayerVoteList::iterator it = m_vVotes.begin(); it != m_vVotes.end(); it++)
	{
		iCount += it->iNumVotes;
	}
	return iCount;
}

int CvVoterDecision::GetVotesCastForChoice(int iChoice)
{
	int iCount = 0;
	for (PlayerVoteList::iterator it = m_vVotes.begin(); it != m_vVotes.end(); it++)
	{
		if (it->iChoice == iChoice)
		{
			iCount += it->iNumVotes;
		}
	}
	return iCount;
}

int CvVoterDecision::GetVotesMarginOfTopChoice()
{
	int iDelta = 0;
	std::vector<int> vTopChoices = GetTopVotedChoices(2);
	if (vTopChoices.size() >= 2)
	{
		iDelta = GetVotesCastForChoice(vTopChoices[0]) - GetVotesCastForChoice(vTopChoices[1]);
	}
	else if (vTopChoices.size() >= 1)
	{
		iDelta = GetVotesCastForChoice(vTopChoices[0]);
	}
	CvAssert(iDelta >= 0);
	return iDelta;
}

int CvVoterDecision::GetVotesCastByPlayer(PlayerTypes ePlayer)
{
	int iCount = 0;
	for (PlayerVoteList::iterator it = m_vVotes.begin(); it != m_vVotes.end(); ++it)
	{
		if (it->ePlayer == ePlayer)
		{
			iCount += it->iNumVotes;
		}
	}
	return iCount;
}

LeagueHelpers::PlayerList CvVoterDecision::GetPlayersVotingForChoice(int iChoice)
{
	LeagueHelpers::PlayerList v;
	for (PlayerVoteList::iterator it = m_vVotes.begin(); it != m_vVotes.end(); ++it)
	{
		if (it->iChoice == iChoice)
		{
			v.push_back(it->ePlayer);
		}
	}
	return v;
}

void CvVoterDecision::ProcessVote(PlayerTypes eVoter, int iNumVotes, int iChoice)
{
	bool bFound = false;
	for (PlayerVoteList::iterator it = m_vVotes.begin(); it != m_vVotes.end(); ++it)
	{
		if (it->ePlayer == eVoter && it->iChoice == iChoice)
		{
			bFound = true;
			it->iNumVotes += iNumVotes;
		}
	}

	if (!bFound)
	{
		PlayerVote vote;
		vote.ePlayer = eVoter;
		vote.iNumVotes = iNumVotes;
		vote.iChoice = iChoice;
		m_vVotes.push_back(vote);
	}
}

// For use with notifications
CvString CvVoterDecision::GetVotesAsText(CvLeague* pLeague)
{
	CvString s = "";

	std::vector<LeagueHelpers::VoteTextSortElement> vVoteText;
	for (PlayerVoteList::iterator it = m_vVotes.begin(); it != m_vVotes.end(); it++)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_MEMBER_VOTE");
		sTemp << it->iNumVotes << pLeague->GetTextForChoice(GetType(), it->iChoice) << GET_PLAYER(it->ePlayer).getCivilizationShortDescriptionKey();
		
		LeagueHelpers::VoteTextSortElement voteTextData;
		voteTextData.sText = sTemp.toUTF8();
		voteTextData.iChoice = it->iChoice;
		voteTextData.iChoiceVotesTotal = GetVotesCastForChoice(it->iChoice);
		voteTextData.iChoiceVotesThisPlayer = it->iNumVotes;

		vVoteText.push_back(voteTextData);
	}

	std::stable_sort(vVoteText.begin(), vVoteText.end(), LeagueHelpers::VoteTextSorter());
	int iNumToShow = MIN(16, (int)vVoteText.size());
	for (int i = 0; i < iNumToShow; i++)
	{
		s += vVoteText[i].sText;
		if (i < iNumToShow - 1)
		{
			s += "[NEWLINE]";
		}
	}

	return s;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvVoterDecision& writeTo)
{
	loadFrom >> (CvResolutionDecision&) writeTo;

	uint uiVersion;

	loadFrom >> uiVersion;
	
	int iNumVotes;
	loadFrom >> iNumVotes;
	for (int i = 0; i < iNumVotes; i++)
	{
		CvResolutionDecision::PlayerVote temp;
		loadFrom >> temp;
		writeTo.m_vVotes.push_back(temp);
	}

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvVoterDecision& readFrom)
{
	saveTo << (CvResolutionDecision&) readFrom;

	uint uiVersion = 1;

	saveTo << uiVersion;
	saveTo << readFrom.m_vVotes.size();
	for (CvResolutionDecision::PlayerVoteList::const_iterator it = readFrom.m_vVotes.begin(); it != readFrom.m_vVotes.end(); it++)
	{
		saveTo << *it;
	}

	return saveTo;
}


// ================================================================================
//			CvResolution
// ================================================================================
CvResolution::CvResolution(void)
{
	m_iID = -1;
	m_eType = NO_RESOLUTION;
	m_eLeague = NO_LEAGUE;
	m_sEffects = CvResolutionEffects();
}

CvResolution::CvResolution(int iID, ResolutionTypes eType, LeagueTypes eLeague)
{
	m_iID = iID;
	m_eType = eType;
	m_eLeague = eLeague;
	m_sEffects = CvResolutionEffects(m_eType);
}

CvResolution::~CvResolution(void)
{
}

int CvResolution::GetID() const
{
	return m_iID;
}

ResolutionTypes CvResolution::GetType() const
{
	return m_eType;
}

LeagueTypes CvResolution::GetLeague() const
{
	return m_eLeague;
}

CvResolutionEffects* CvResolution::GetEffects()
{
	return &m_sEffects;
}

CvVoterDecision* CvResolution::GetVoterDecision()
{
	return &m_VoterDecision;
}

CvProposerDecision* CvResolution::GetProposerDecision()
{
	return &m_ProposerDecision;
}

CvString CvResolution::GetName()
{
	CvString s = "";

	CvResolutionEntry* pInfo = GC.getResolutionInfo(GetType());
	CvAssert(pInfo);
	if (pInfo)
	{
		s += Localization::Lookup(pInfo->GetDescriptionKey()).toUTF8();
		if (GetProposerDecision()->GetDecision() != LeagueHelpers::CHOICE_NONE)
		{
			s += ": "; //antonjs: temp
			s += LeagueHelpers::GetTextForChoice(GetProposerDecision()->GetType(), GetProposerDecision()->GetDecision());
		}
	}

	return s;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvResolution& writeTo)
{
	uint uiVersion;

	loadFrom >> uiVersion;
	if (uiVersion >= 2)
	{
		loadFrom >> writeTo.m_iID;
	}
	else
	{
		writeTo.m_iID = GC.getGame().GetGameLeagues()->GenerateResolutionUniqueID();
	}
	loadFrom >> writeTo.m_eType;
	loadFrom >> writeTo.m_eLeague;
	loadFrom >> writeTo.m_sEffects;
	loadFrom >> writeTo.m_VoterDecision;
	loadFrom >> writeTo.m_ProposerDecision;

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvResolution& readFrom)
{
	uint uiVersion = 2;

	saveTo << uiVersion;
	saveTo << readFrom.m_iID;
	saveTo << readFrom.m_eType;
	saveTo << readFrom.m_eLeague;
	saveTo << readFrom.m_sEffects;
	saveTo << readFrom.m_VoterDecision;
	saveTo << readFrom.m_ProposerDecision;

	return saveTo;
}


// ================================================================================
//			CvProposal
// ================================================================================
CvProposal::CvProposal(void)
{
	m_eProposalPlayer = NO_PLAYER;
}

CvProposal::CvProposal(int iID, ResolutionTypes eType, LeagueTypes eLeague, PlayerTypes eProposalPlayer) : CvResolution(iID, eType, eLeague)
{
	m_eProposalPlayer = eProposalPlayer;
}

CvProposal::~CvProposal(void)
{
}

PlayerTypes CvProposal::GetProposalPlayer() const
{
	return m_eProposalPlayer;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvProposal& writeTo)
{
	loadFrom >> (CvResolution&) writeTo;

	uint uiVersion;

	loadFrom >> uiVersion;
	loadFrom >> writeTo.m_eProposalPlayer;

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvProposal& readFrom)
{
	saveTo << (CvResolution&) readFrom;

	uint uiVersion = 1;

	saveTo << uiVersion;
	saveTo << readFrom.m_eProposalPlayer;

	return saveTo;
}


// ================================================================================
//			CvEnactProposal
// ================================================================================
CvEnactProposal::CvEnactProposal(void)
{
}

CvEnactProposal::CvEnactProposal(int iID, ResolutionTypes eType, LeagueTypes eLeague, PlayerTypes eProposalPlayer, int iChoice) : CvProposal(iID, eType, eLeague, eProposalPlayer)
{
	CvResolutionEntry* pInfo = GC.getResolutionInfo(eType);
	CvAssertMsg(pInfo, "Resolution info is null. Please send Anton your save file and version.");
	if (pInfo)
	{
		m_VoterDecision = CvVoterDecision(pInfo->GetVoterDecision());
		m_ProposerDecision = CvProposerDecision(pInfo->GetProposerDecision(), eProposalPlayer, iChoice);
	}
}

CvEnactProposal::~CvEnactProposal(void)
{
}

void CvEnactProposal::Init()
{
}

bool CvEnactProposal::IsPassed(int iTotalSessionVotes)
{
	CvResolutionEntry* pInfo = GC.getResolutionInfo(GetType());
	CvAssert(pInfo);
	if (!pInfo)
	{
		return false;
	}

	int iQuorumPercent = pInfo->GetQuorumPercent();
	bool bQuorum = ((float)GetVoterDecision()->GetVotesCast() / (float)iTotalSessionVotes) >= ((float)iQuorumPercent / 100.0f);
	if (!bQuorum)
	{
		return false;
	}

	int iDecision = GetVoterDecision()->GetDecision();

	if (GetEffects()->bDiplomaticVictory)
	{
		// World Leader only passes if there is no tie and someone has enough votes cast for them
		if (!GetVoterDecision()->IsTie())
		{
			int iVotesForTop = GetVoterDecision()->GetVotesCastForChoice(iDecision);
			int iVotesNeededToWin = GC.getGame().GetVotesNeededForDiploVictory();
			if (iVotesForTop >= iVotesNeededToWin)
			{
				return true;
			}
		}
		return false;
	}
	else if (GetEffects()->bChangeLeagueHost)
	{
		// Choose Host always passes, it's just a matter of who won
		return true;
	}
	else if (GetVoterDecision()->GetType() == RESOLUTION_DECISION_YES_OR_NO)
	{
		// Normal resolutions only pass on YES, not NO or NONE
		return (iDecision == LeagueHelpers::CHOICE_YES);
	}

	CvAssertMsg(false, "Unexpected case when determining whether an enact proposal has passed. Please send Anton your save file and version.");
	return false;
}

CvString CvEnactProposal::GetProposalName(bool bForLogging)
{
	CvString s = "";

	if (bForLogging)
	{
		s += "ENACT: ";
	}
	else
	{
		s += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PREFIX_ENACT").toUTF8();
	}
	s += CvResolution::GetName();

	return s;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvEnactProposal& writeTo)
{
	loadFrom >> (CvProposal&) writeTo;
	
	uint uiVersion;

	loadFrom >> uiVersion;

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvEnactProposal& readFrom)
{
	saveTo << (CvProposal&) readFrom;
	
	uint uiVersion = 1;

	saveTo << uiVersion;

	return saveTo;
}


// ================================================================================
//			CvActiveResolution
// ================================================================================
CvActiveResolution::CvActiveResolution(void)
{
	m_iTurnEnacted = -1;
}

CvActiveResolution::CvActiveResolution(CvEnactProposal* pResolution) : CvResolution(pResolution->GetID(), pResolution->GetType(), pResolution->GetLeague())
{
	m_iTurnEnacted = -1;
	m_VoterDecision = (*(pResolution->GetVoterDecision()));
	m_ProposerDecision = (*(pResolution->GetProposerDecision()));
}

CvActiveResolution::~CvActiveResolution(void)
{
}

void CvActiveResolution::Init()
{
}

void CvActiveResolution::DoEffects(PlayerTypes ePlayer)
{
	CvPlayer* pPlayer = &GET_PLAYER(ePlayer);
	CvAssertMsg(pPlayer != NULL, "Player is null when doing effects of an active resolution. Please send Anton your save file and version.");
	if (pPlayer == NULL) return;

	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetLeague(GetLeague());
	CvAssertMsg(pLeague != NULL, "League is null when doing effects of an active resolution. Please send Anton your save file and version.");
	if (pLeague == NULL) return;

	// == Proposer Choices ==
	ResolutionDecisionTypes eProposerDecision = GetProposerDecision()->GetType();
	PlayerTypes eTargetPlayer = NO_PLAYER;
	if (eProposerDecision == RESOLUTION_DECISION_ANY_MEMBER ||
		eProposerDecision == RESOLUTION_DECISION_MAJOR_CIV_MEMBER ||
		eProposerDecision == RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER)
	{
		eTargetPlayer = (PlayerTypes) GetProposerDecision()->GetDecision();
	}
	ResourceTypes eTargetLuxury = NO_RESOURCE;
	if (eProposerDecision == RESOLUTION_DECISION_ANY_LUXURY_RESOURCE)
	{
		CvResourceInfo* pInfo = GC.getResourceInfo((ResourceTypes) GetProposerDecision()->GetDecision());
		if (pInfo && pInfo->getResourceUsage() == RESOURCEUSAGE_LUXURY)
		{
			eTargetLuxury = (ResourceTypes) GetProposerDecision()->GetDecision();
		}
	}
	ReligionTypes eTargetReligion = NO_RELIGION;
	if (eProposerDecision == RESOLUTION_DECISION_RELIGION)
	{
		eTargetReligion = (ReligionTypes) GetProposerDecision()->GetDecision();
	}
	PolicyBranchTypes eTargetIdeology = NO_POLICY_BRANCH_TYPE;
	if (eProposerDecision == RESOLUTION_DECISION_IDEOLOGY)
	{
		eTargetIdeology = (PolicyBranchTypes) GetProposerDecision()->GetDecision();
	}

	// == Voter Choices ==
	ResolutionDecisionTypes eVoterDecision = GetVoterDecision()->GetType();
	PlayerTypes eVotedPlayer = NO_PLAYER;
	if (eVoterDecision == RESOLUTION_DECISION_ANY_MEMBER ||
		eVoterDecision == RESOLUTION_DECISION_MAJOR_CIV_MEMBER ||
		eVoterDecision == RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER)
	{
		eVotedPlayer = (PlayerTypes) GetVoterDecision()->GetDecision();
	}

	// == One Time Effects ==
	if (GetEffects()->bDiplomaticVictory)
	{
		CvAssertMsg(eTargetPlayer != NO_PLAYER || eVotedPlayer != NO_PLAYER, "Diplomatic Victory voted for NO_PLAYER. Please send Anton your save file and version.");
		CvAssertMsg(eTargetPlayer == NO_PLAYER || eVotedPlayer == NO_PLAYER, "Ambiguous target when setting Diplomatic Victory. Please send Anton your save file and version");
		if (ePlayer == eVotedPlayer || ePlayer == eTargetPlayer)
		{
			GC.getGame().GetGameLeagues()->SetDiplomaticVictor(ePlayer);
		}
	}
	if (GetEffects()->bChangeLeagueHost)
	{
		CvAssertMsg(eTargetPlayer != NO_PLAYER || eVotedPlayer != NO_PLAYER, "Changing host to NO_PLAYER. Please send Anton your save file and version.");
		CvAssertMsg(eTargetPlayer == NO_PLAYER || eVotedPlayer == NO_PLAYER, "Ambiguous target when changing host. Please send Anton your save file and version");
		if (ePlayer == eVotedPlayer || ePlayer == eTargetPlayer)
		{
			pLeague->SetHostMember(ePlayer);
		}
	}
	if (GetEffects()->iOneTimeGold != 0)
	{
		pPlayer->GetTreasury()->ChangeGold(GetEffects()->iOneTimeGold);
	}
	if (GetEffects()->iOneTimeGoldPercent != 0)
	{
		int iGold = pPlayer->GetTreasury()->GetGold();
		int iChange = (iGold * GetEffects()->iOneTimeGoldPercent) / 100;
		pPlayer->GetTreasury()->ChangeGold(iChange);
	}
	if (GetEffects()->bRaiseCityStateInfluenceToNeutral)
	{
		if (!GET_PLAYER(ePlayer).isMinorCiv())
		{
			int iNeutral = GC.getMINOR_FRIENDSHIP_ANCHOR_DEFAULT();
			for (int iMinor = MAX_MAJOR_CIVS; iMinor < MAX_CIV_PLAYERS; iMinor++)
			{
				PlayerTypes eMinor = (PlayerTypes) iMinor;
				if (GET_PLAYER(eMinor).isAlive() && GET_PLAYER(eMinor).GetMinorCivAI()->GetBaseFriendshipWithMajor(ePlayer) < iNeutral)
				{
					if (pLeague->IsMember(eMinor))
					{
						GET_PLAYER(eMinor).GetMinorCivAI()->SetFriendshipWithMajor(ePlayer, iNeutral);
					}
				}
			}
		}
	}

	// == Ongoing Effects ==
	if (GetEffects()->iGoldPerTurn != 0)
	{
		pPlayer->GetTreasury()->ChangeGoldPerTurnFromDiplomacy(GetEffects()->iGoldPerTurn);
	}
	if (GetEffects()->iResourceQuantity != 0)
	{
		CvAssertMsg(eTargetLuxury != NO_RESOURCE, "Adding NO_RESOURCE for a player. Please send Anton your save file and version.");
		pPlayer->changeNumResourceTotal(eTargetLuxury, GetEffects()->iResourceQuantity);
	}
	if (GetEffects()->bEmbargoCityStates)
	{	
		// Refresh trade routes
		GC.getGame().GetGameTrade()->ClearAllCityStateTradeRoutes();
	}
	if (GetEffects()->bEmbargoPlayer)
	{
		CvAssertMsg(eTargetPlayer != NO_PLAYER, "Making an embargo on NO_PLAYER. Please send Anton your save file and version.");
		// Refresh trade routes
		GC.getGame().GetGameTrade()->ClearAllCivTradeRoutes(eTargetPlayer);
	}
	if (GetEffects()->bNoResourceHappiness)
	{
		CvAssertMsg(eTargetLuxury != NO_RESOURCE, "Banning Happiness for NO_RESOURCE. Please send Anton your save file and version.");
		// Refresh happiness
	}
	if (GetEffects()->iUnitMaintenanceGoldPercent != 0)
	{
		pPlayer->ChangeUnitGoldMaintenanceMod(GetEffects()->iUnitMaintenanceGoldPercent);
	}
	if (GetEffects()->iMemberDiscoveredTechMod != 0)
	{
		// Refresh research
	}
	if (GetEffects()->iCulturePerWonder != 0)
	{
		// Refresh yields
	}
	if (GetEffects()->iCulturePerNaturalWonder != 0)
	{
		// Refresh yields
	}
	if (GetEffects()->bNoTrainingNuclearWeapons)
	{
	}
	if (GetEffects()->iVotesForFollowingReligion != 0)
	{
		CvAssertMsg(eTargetReligion != NO_RELIGION, "No target religion when one was expected. Please send Anton your save file and version.");
	}
	if (GetEffects()->iHolyCityTourism != 0)
	{
		CvAssertMsg(eTargetReligion != NO_RELIGION, "No target religion when one was expected. Please send Anton your save file and version.");
		/*
		if (eTargetReligion != NO_RELIGION)
		{
			const CvReligion* pTargetReligion = GC.getGame().GetGameReligions()->GetReligion(eTargetReligion, ePlayer);
			if (pTargetReligion != NULL)
			{
				CvPlot* pHolyCityPlot = GC.getMap().plot(pTargetReligion->m_iHolyCityX, pTargetReligion->m_iHolyCityY);
				if (pHolyCityPlot != NULL && pHolyCityPlot->isCity())
				{
					CvCity* pHolyCity = pHolyCityPlot->getPlotCity();
					if (pHolyCity != NULL && pHolyCity->getOwner() == ePlayer)
					{
						//antonjs: temp:
						GET_PLAYER(ePlayer).GetTreasury()->ChangeGoldPerTurnFromDiplomacy(GetEffects()->iHolyCityTourism);
					}
				}
			}
		}
		//antonjs: todo: OnHolyCityCaptured()
		*/
	}
	if (GetEffects()->iReligionSpreadStrengthMod != 0)
	{
		CvAssertMsg(eTargetReligion != NO_RELIGION, "No target religion when one was expected. Please send Anton your save file and version.");
		// Updated on CvGameReligions::DoTurn
	}
	if (GetEffects()->iVotesForFollowingIdeology != 0)
	{
		CvAssertMsg(eTargetIdeology != NO_POLICY_BRANCH_TYPE, "No target ideology when one was expected. Please send Anton your save file and version.");
	}
	if (GetEffects()->iOtherIdeologyRebellionMod != 0)
	{
		CvAssertMsg(eTargetIdeology != NO_POLICY_BRANCH_TYPE, "No target ideology when one was expected. Please send Anton your save file and version.");
		/*
		if (eTargetIdeology != NO_POLICY_BRANCH_TYPE)
		{
			if (GET_PLAYER(ePlayer).GetPlayerPolicies()->GetLateGamePolicyTree() == eTargetIdeology)
			{
				//antonjs: temp:
				GET_PLAYER(ePlayer).GetTreasury()->ChangeGoldPerTurnFromDiplomacy(GetEffects()->iOtherIdeologyRebellionMod);
			}
		}
		//antonjs: todo: OnIdeologyChanged()
		*/
	}
	if (GetEffects()->iArtsyGreatPersonRateMod != 0)
	{
	}
	if (GetEffects()->iScienceyGreatPersonRateMod != 0)
	{
	}
	if (GetEffects()->iGreatPersonTileImprovementCulture != 0)
	{
		// Loop through all Great Person tile improvements
		for (int i = 0; i < GC.getNumImprovementInfos(); i++)
		{
			CvImprovementEntry* pInfo = GC.getImprovementInfo((ImprovementTypes)i);
			if (pInfo != NULL && pInfo->IsCreatedByGreatPerson())
			{
				GET_PLAYER(ePlayer).changeImprovementYieldChange((ImprovementTypes)pInfo->GetID(), YIELD_CULTURE, GetEffects()->iGreatPersonTileImprovementCulture);
			}
		}
		// Refresh yield
	}
	if (GetEffects()->iLandmarkCulture != 0)
	{
		CvImprovementEntry* pLandmarkInfo = GC.getImprovementInfo((ImprovementTypes)GC.getInfoTypeForString("IMPROVEMENT_LANDMARK"));
		if (pLandmarkInfo != NULL)
		{
			GET_PLAYER(ePlayer).changeImprovementYieldChange((ImprovementTypes)pLandmarkInfo->GetID(), YIELD_CULTURE, GetEffects()->iLandmarkCulture);
		}
		// Refresh yield
	}
#ifdef NEW_LEAGUE_RESOLUTIONS
	if (GetEffects()->iTradeRouteGoldModifier != 0)
	{
	}
	if (GetEffects()->iCSBonuModifier != 0)
	{
		for (int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
		{
			PlayerTypes eLoopMinor = (PlayerTypes)iMinorLoop;

			// Minor not alive
			if (!GET_PLAYER(eLoopMinor).isAlive())
				continue;
			
			MinorCivTraitTypes eTrait = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetTrait();
			// MARITIME
			if (eTrait == MINOR_CIV_TRAIT_MARITIME)
			{
				// Friends
#ifdef RELIGIOUS_UNITY_CS_BONUS
				ReligionTypes eFoundedReligion = GC.getGame().GetGameReligions()->GetReligionCreatedByPlayer(ePlayer);
				ReligionTypes eMajority = NO_RELIGION;
				if (GET_PLAYER(eLoopMinor).getCapitalCity())
				{
					eMajority = GET_PLAYER(eLoopMinor).getCapitalCity()->GetCityReligions()->GetReligiousMajority();
				}
				if (eFoundedReligion > NO_RELIGION && eFoundedReligion == eMajority && GC.getGame().GetGameReligions()->GetReligion(eFoundedReligion, NO_PLAYER)->m_Beliefs.HasBelief((BeliefTypes)GC.getInfoTypeForString("BELIEF_RELIGIOUS_UNITY"))
					|| GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsFriends(ePlayer))
#else
				if (GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsFriends(ePlayer))
#endif
				{
					int iOldFood, iNewFood;

					// Capital
					iOldFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsCapitalFoodBonus(ePlayer);
					iNewFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsCapitalFoodBonus(ePlayer, NO_ERA, GetEffects()->iCSBonuModifier);

					if (iOldFood != iNewFood)
					{
						GET_PLAYER(ePlayer).ChangeCapitalYieldChange(YIELD_FOOD, iNewFood - iOldFood);
#ifdef EG_REPLAYDATASET_FOODFROMCS
						GET_PLAYER(ePlayer).ChangeFoodFromMinorsTimes100(1024 * (iNewFood - iOldFood));
#endif
					}

					// Other Cities
					iOldFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsOtherCityFoodBonus(ePlayer);
					iNewFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsOtherCityFoodBonus(ePlayer, NO_ERA, GetEffects()->iCSBonuModifier);

					if (iOldFood != iNewFood)
					{
						GET_PLAYER(ePlayer).ChangeCityYieldChange(YIELD_FOOD, iNewFood - iOldFood);
#ifdef EG_REPLAYDATASET_FOODFROMCS
						GET_PLAYER(ePlayer).ChangeFoodFromMinorsTimes100(iNewFood - iOldFood);
#endif
					}
				}

				// Allies
#ifdef RELIGIOUS_UNITY_CS_BONUS
				if (eFoundedReligion > NO_RELIGION && eFoundedReligion == eMajority && GC.getGame().GetGameReligions()->GetReligion(eFoundedReligion, NO_PLAYER)->m_Beliefs.HasBelief((BeliefTypes)GC.getInfoTypeForString("BELIEF_RELIGIOUS_UNITY"))
					|| GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsAllies(ePlayer))
#else
				if (GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsAllies(ePlayer))
#endif
				{
					int iOldFood, iNewFood;

					// Capital
					iOldFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesCapitalFoodBonus(ePlayer);
					iNewFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesCapitalFoodBonus(ePlayer, GetEffects()->iCSBonuModifier);

					if (iOldFood != iNewFood)
					{
						GET_PLAYER(ePlayer).ChangeCapitalYieldChange(YIELD_FOOD, iNewFood - iOldFood);
#ifdef EG_REPLAYDATASET_FOODFROMCS
						GET_PLAYER(ePlayer).ChangeFoodFromMinorsTimes100(1024 * (iNewFood - iOldFood));
#endif
					}

					// Other Cities
					iOldFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesOtherCityFoodBonus(ePlayer);
					iNewFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesOtherCityFoodBonus(ePlayer, GetEffects()->iCSBonuModifier);

					if (iOldFood != iNewFood)
					{
						GET_PLAYER(ePlayer).ChangeCityYieldChange(YIELD_FOOD, iNewFood - iOldFood);
#ifdef EG_REPLAYDATASET_FOODFROMCS
						GET_PLAYER(ePlayer).ChangeFoodFromMinorsTimes100(iNewFood - iOldFood);
#endif
					}
				}
			}

			// MANUFACTORY
			if (eTrait == MINOR_CIV_TRAIT_MANUFACTORY)
			{
				// Friends
#ifdef RELIGIOUS_UNITY_CS_BONUS
				ReligionTypes eFoundedReligion = GC.getGame().GetGameReligions()->GetReligionCreatedByPlayer(ePlayer);
				ReligionTypes eMajority = NO_RELIGION;
				if (GET_PLAYER(eLoopMinor).getCapitalCity())
				{
					eMajority = GET_PLAYER(eLoopMinor).getCapitalCity()->GetCityReligions()->GetReligiousMajority();
				}
				if (eFoundedReligion > NO_RELIGION && eFoundedReligion == eMajority && GC.getGame().GetGameReligions()->GetReligion(eFoundedReligion, NO_PLAYER)->m_Beliefs.HasBelief((BeliefTypes)GC.getInfoTypeForString("BELIEF_RELIGIOUS_UNITY"))
					|| GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsFriends(ePlayer))
#else
				if (GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsFriends(ePlayer))
#endif
				{
					int iOldProduction, iNewProduction;

					// Capital
					iOldProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsCapitalProductionBonus(ePlayer);
					iNewProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsCapitalProductionBonus(ePlayer, NO_ERA, GetEffects()->iCSBonuModifier);

					if (iOldProduction != iNewProduction)
					{
						GET_PLAYER(ePlayer).ChangeCapitalYieldChange(YIELD_PRODUCTION, iNewProduction - iOldProduction);
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
						GET_PLAYER(ePlayer).ChangeProductionFromMinorsTimes100(1024 * (iNewProduction - iOldProduction));
#endif
					}

					// Other Cities
					iOldProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsOtherCityProductionBonus(ePlayer);
					iNewProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsOtherCityProductionBonus(ePlayer, NO_ERA, GetEffects()->iCSBonuModifier);

					if (iOldProduction != iNewProduction)
					{
						GET_PLAYER(ePlayer).ChangeCityYieldChange(YIELD_PRODUCTION, iNewProduction - iOldProduction);
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
						GET_PLAYER(ePlayer).ChangeProductionFromMinorsTimes100(iNewProduction - iOldProduction);
#endif
					}
				}

				// Allies
#ifdef RELIGIOUS_UNITY_CS_BONUS
				if (eFoundedReligion > NO_RELIGION && eFoundedReligion == eMajority && GC.getGame().GetGameReligions()->GetReligion(eFoundedReligion, NO_PLAYER)->m_Beliefs.HasBelief((BeliefTypes)GC.getInfoTypeForString("BELIEF_RELIGIOUS_UNITY"))
					|| GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsAllies(ePlayer))
#else
				if (GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsAllies(ePlayer))
#endif
				{
					int iOldProduction, iNewProduction;

					// Capital
					iOldProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesCapitalProductionBonus(ePlayer);
					iNewProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesCapitalProductionBonus(ePlayer, GetEffects()->iCSBonuModifier);

					if (iOldProduction != iNewProduction)
					{
						GET_PLAYER(ePlayer).ChangeCapitalYieldChange(YIELD_PRODUCTION, iNewProduction - iOldProduction);
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
						GET_PLAYER(ePlayer).ChangeProductionFromMinorsTimes100(1024 * (iNewProduction - iOldProduction));
#endif
					}

					// Other Cities
					iOldProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesOtherCityProductionBonus(ePlayer);
					iNewProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesOtherCityProductionBonus(ePlayer, GetEffects()->iCSBonuModifier);

					if (iOldProduction != iNewProduction)
					{
						GET_PLAYER(ePlayer).ChangeCityYieldChange(YIELD_PRODUCTION, iNewProduction - iOldProduction);
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
						GET_PLAYER(ePlayer).ChangeProductionFromMinorsTimes100(iNewProduction - iOldProduction);
#endif
					}
				}
			}
		}
	}
	if (GetEffects()->bNoSpiesInCS)
	{
		for (int iMinor = MAX_MAJOR_CIVS; iMinor < MAX_CIV_PLAYERS; iMinor++)
		{
			PlayerTypes eMinor = (PlayerTypes)iMinor;
			if (GET_PLAYER(eMinor).isAlive())
			{
				if (GET_PLAYER(ePlayer).isAlive())
				{
					if (GET_PLAYER(ePlayer).getTeam() == GET_PLAYER(eMinor).getTeam())
					{
						continue;
					}

					int iSpyIndex = GET_PLAYER(ePlayer).GetEspionage()->GetSpyIndexInCity(GET_PLAYER(eMinor).getCapitalCity());
					if (iSpyIndex != -1)
					{
						CvNotifications* pNotifications = GET_PLAYER(ePlayer).GetNotifications();
						if (pNotifications)
						{
							CvPlayerEspionage* pEspionage = GET_PLAYER(ePlayer).GetEspionage();
							int iSpyName = pEspionage->m_aSpyList[iSpyIndex].m_iName;
							CvSpyRank eSpyRank = pEspionage->m_aSpyList[iSpyIndex].m_eRank;
							Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SPY_EJECTED_LEAGUE");
							Localization::String strNotification = Localization::Lookup("TXT_KEY_NOTIFICATION_SPY_EJECTED_LEAGUE_TT");
							strNotification << pEspionage->GetSpyRankName(eSpyRank);
							strNotification << GET_PLAYER(ePlayer).getCivilizationInfo().getSpyNames(iSpyName);
							strNotification << GET_PLAYER(eMinor).getCapitalCity()->getNameKey();
							pNotifications->Add(NOTIFICATION_SPY_CANT_STEAL_TECH, strNotification.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
						}
						GET_PLAYER(ePlayer).GetEspionage()->ExtractSpyFromCity(iSpyIndex);
					}
				}
			}
		}
	}
	if (GetEffects()->bDoubleResourceHappiness)
	{
	}
#endif

	m_iTurnEnacted = GC.getGame().getGameTurn();
}

void CvActiveResolution::RemoveEffects(PlayerTypes ePlayer)
{
	CvPlayer* pPlayer = &GET_PLAYER(ePlayer);
	CvAssertMsg(pPlayer != NULL, "Player is null when doing effects of an active resolution. Please send Anton your save file and version.");
	if (pPlayer == NULL) return;

	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetLeague(GetLeague());
	CvAssertMsg(pLeague != NULL, "League is null when doing effects of an active resolution. Please send Anton your save file and version.");
	if (pLeague == NULL) return;

	// == Proposer Choices ==
	ResolutionDecisionTypes eProposerDecision = GetProposerDecision()->GetType();
	PlayerTypes eTargetPlayer = NO_PLAYER;
	if (eProposerDecision == RESOLUTION_DECISION_ANY_MEMBER ||
		eProposerDecision == RESOLUTION_DECISION_MAJOR_CIV_MEMBER ||
		eProposerDecision == RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER)
	{
		eTargetPlayer = (PlayerTypes) GetProposerDecision()->GetDecision();
	}
	ResourceTypes eTargetLuxury = NO_RESOURCE;
	if (eProposerDecision == RESOLUTION_DECISION_ANY_LUXURY_RESOURCE)
	{
		CvResourceInfo* pInfo = GC.getResourceInfo((ResourceTypes) GetProposerDecision()->GetDecision());
		if (pInfo && pInfo->getResourceUsage() == RESOURCEUSAGE_LUXURY)
		{
			eTargetLuxury = (ResourceTypes) GetProposerDecision()->GetDecision();
		}
	}
	ReligionTypes eTargetReligion = NO_RELIGION;
	if (eProposerDecision == RESOLUTION_DECISION_RELIGION)
	{
		eTargetReligion = (ReligionTypes) GetProposerDecision()->GetDecision();
	}
	PolicyBranchTypes eTargetIdeology = NO_POLICY_BRANCH_TYPE;
	if (eProposerDecision == RESOLUTION_DECISION_IDEOLOGY)
	{
		eTargetIdeology = (PolicyBranchTypes) GetProposerDecision()->GetDecision();
	}

	// == Voter Choices ==
	ResolutionDecisionTypes eVoterDecision = GetVoterDecision()->GetType();
	PlayerTypes eVotedPlayer = NO_PLAYER;
	if (eVoterDecision == RESOLUTION_DECISION_ANY_MEMBER ||
		eVoterDecision == RESOLUTION_DECISION_MAJOR_CIV_MEMBER ||
		eVoterDecision == RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER)
	{
		eVotedPlayer = (PlayerTypes) GetVoterDecision()->GetDecision();
	}

	// == One Time Effects are not removed ==

	// == Ongoing Effects ==
	if (GetEffects()->iGoldPerTurn != 0)
	{
		pPlayer->GetTreasury()->ChangeGoldPerTurnFromDiplomacy(-1 * GetEffects()->iGoldPerTurn);
	}
	if (GetEffects()->iResourceQuantity != 0)
	{
		CvAssertMsg(eTargetLuxury != NO_RESOURCE, "Subtracting NO_RESOURCE for a player. Please send Anton your save file and version.");
		pPlayer->changeNumResourceTotal(eTargetLuxury, -1 * GetEffects()->iResourceQuantity);
	}
	if (GetEffects()->bEmbargoCityStates)
	{
		// Refresh trade routes
	}
	if (GetEffects()->bEmbargoPlayer)
	{
		CvAssertMsg(eTargetPlayer != NO_PLAYER, "Repealing an embargo on NO_PLAYER. Please send Anton your save file and version.");
		// Refresh trade routes
	}
	if (GetEffects()->bNoResourceHappiness)
	{
		CvAssertMsg(eTargetLuxury != NO_RESOURCE, "Repealing a band on Happiness for NO_RESOURCE. Please send Anton your save file and version.");
		// Refresh happiness
	}
	if (GetEffects()->iUnitMaintenanceGoldPercent != 0)
	{
		pPlayer->ChangeUnitGoldMaintenanceMod(-1 * GetEffects()->iUnitMaintenanceGoldPercent);
	}
	if (GetEffects()->iMemberDiscoveredTechMod != 0)
	{
		// Refresh research
	}
	if (GetEffects()->iCulturePerWonder != 0)
	{
		// Refresh yields
	}
	if (GetEffects()->iCulturePerNaturalWonder != 0)
	{
		// Refresh yields
	}
	if (GetEffects()->bNoTrainingNuclearWeapons)
	{
	}
	if (GetEffects()->iVotesForFollowingReligion != 0)
	{
	}
	if (GetEffects()->iHolyCityTourism != 0)
	{
		CvAssertMsg(eTargetReligion != NO_RELIGION, "No target religion when one was expected. Please send Anton your save file and version.");
		/*
		if (eTargetReligion != NO_RELIGION)
		{
			const CvReligion* pTargetReligion = GC.getGame().GetGameReligions()->GetReligion(eTargetReligion, ePlayer);
			if (pTargetReligion != NULL)
			{
				CvPlot* pHolyCityPlot = GC.getMap().plot(pTargetReligion->m_iHolyCityX, pTargetReligion->m_iHolyCityY);
				if (pHolyCityPlot != NULL && pHolyCityPlot->isCity())
				{
					CvCity* pHolyCity = pHolyCityPlot->getPlotCity();
					if (pHolyCity != NULL && pHolyCity->getOwner() == ePlayer)
					{
						//antonjs: temp:
						GET_PLAYER(ePlayer).GetTreasury()->ChangeGoldPerTurnFromDiplomacy(-1 * 100);
					}
				}
			}
		}
		//antonjs: todo: OnHolyCityCaptured()
		*/
	}
	if (GetEffects()->iReligionSpreadStrengthMod != 0)
	{
		CvAssertMsg(eTargetReligion != NO_RELIGION, "No target religion when one was expected. Please send Anton your save file and version.");
		// Updated on CvGameReligions::DoTurn
	}
	if (GetEffects()->iVotesForFollowingIdeology != 0)
	{
	}
	if (GetEffects()->iOtherIdeologyRebellionMod != 0)
	{
		CvAssertMsg(eTargetIdeology != NO_POLICY_BRANCH_TYPE, "No target ideology when one was expected. Please send Anton your save file and version.");
		/*
		if (eTargetIdeology != NO_POLICY_BRANCH_TYPE)
		{
			if (GET_PLAYER(ePlayer).GetPlayerPolicies()->GetLateGamePolicyTree() == eTargetIdeology)
			{
				//antonjs: temp:
				GET_PLAYER(ePlayer).GetTreasury()->ChangeGoldPerTurnFromDiplomacy(-1 * 100);
			}
		}
		//antonjs: todo: OnIdeologyChanged()
		*/
	}
	if (GetEffects()->iArtsyGreatPersonRateMod != 0)
	{
	}
	if (GetEffects()->iScienceyGreatPersonRateMod != 0)
	{
	}
	if (GetEffects()->iGreatPersonTileImprovementCulture != 0)
	{
		// Loop through all Great Person tile improvements
		for (int i = 0; i < GC.getNumImprovementInfos(); i++)
		{
			CvImprovementEntry* pInfo = GC.getImprovementInfo((ImprovementTypes)i);
			if (pInfo != NULL && pInfo->IsCreatedByGreatPerson())
			{
				GET_PLAYER(ePlayer).changeImprovementYieldChange((ImprovementTypes)pInfo->GetID(), YIELD_CULTURE, -1 * GetEffects()->iGreatPersonTileImprovementCulture);
			}
		}
		// Refresh yield
	}
	if (GetEffects()->iLandmarkCulture != 0)
	{
		CvImprovementEntry* pLandmarkInfo = GC.getImprovementInfo((ImprovementTypes)GC.getInfoTypeForString("IMPROVEMENT_LANDMARK"));
		if (pLandmarkInfo != NULL)
		{
			GET_PLAYER(ePlayer).changeImprovementYieldChange((ImprovementTypes)pLandmarkInfo->GetID(), YIELD_CULTURE, -1 * GetEffects()->iLandmarkCulture);
		}
		// Refresh yield
	}
#ifdef NEW_LEAGUE_RESOLUTIONS
	if (GetEffects()->iTradeRouteGoldModifier != 0)
	{
	}
	if (GetEffects()->iCSBonuModifier != 0)
	{
		for (int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
		{
			PlayerTypes eLoopMinor = (PlayerTypes)iMinorLoop;

			// Minor not alive
			if (!GET_PLAYER(eLoopMinor).isAlive())
				continue;

			MinorCivTraitTypes eTrait = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetTrait();
			// MARITIME
			if (eTrait == MINOR_CIV_TRAIT_MARITIME)
			{
				// Friends
#ifdef RELIGIOUS_UNITY_CS_BONUS
				ReligionTypes eFoundedReligion = GC.getGame().GetGameReligions()->GetReligionCreatedByPlayer(ePlayer);
				ReligionTypes eMajority = NO_RELIGION;
				if (GET_PLAYER(eLoopMinor).getCapitalCity())
				{
					eMajority = GET_PLAYER(eLoopMinor).getCapitalCity()->GetCityReligions()->GetReligiousMajority();
				}
				if (eFoundedReligion > NO_RELIGION && eFoundedReligion == eMajority && GC.getGame().GetGameReligions()->GetReligion(eFoundedReligion, NO_PLAYER)->m_Beliefs.HasBelief((BeliefTypes)GC.getInfoTypeForString("BELIEF_RELIGIOUS_UNITY"))
					|| GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsFriends(ePlayer))
#else
				if (GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsFriends(ePlayer))
#endif
				{
					int iOldFood, iNewFood;

					// Capital
					iOldFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsCapitalFoodBonus(ePlayer, NO_ERA, true);
					iNewFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsCapitalFoodBonus(ePlayer);

					if (iOldFood != iNewFood)
					{
						GET_PLAYER(ePlayer).ChangeCapitalYieldChange(YIELD_FOOD, iNewFood - iOldFood);
#ifdef EG_REPLAYDATASET_FOODFROMCS
						GET_PLAYER(ePlayer).ChangeFoodFromMinorsTimes100(1024 * (iNewFood - iOldFood));
#endif
					}

					// Other Cities
					iOldFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsOtherCityFoodBonus(ePlayer, NO_ERA, true);
					iNewFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsOtherCityFoodBonus(ePlayer);

					if (iOldFood != iNewFood)
					{
						GET_PLAYER(ePlayer).ChangeCityYieldChange(YIELD_FOOD, iNewFood - iOldFood);
#ifdef EG_REPLAYDATASET_FOODFROMCS
						GET_PLAYER(ePlayer).ChangeFoodFromMinorsTimes100(iNewFood - iOldFood);
#endif
					}
				}

				// Allies
#ifdef RELIGIOUS_UNITY_CS_BONUS
				if (eFoundedReligion > NO_RELIGION && eFoundedReligion == eMajority && GC.getGame().GetGameReligions()->GetReligion(eFoundedReligion, NO_PLAYER)->m_Beliefs.HasBelief((BeliefTypes)GC.getInfoTypeForString("BELIEF_RELIGIOUS_UNITY"))
					|| GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsAllies(ePlayer))
#else
				if (GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsAllies(ePlayer))
#endif
				{
					int iOldFood, iNewFood;

					// Capital
					iOldFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesCapitalFoodBonus(ePlayer, true);
					iNewFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesCapitalFoodBonus(ePlayer);

					if (iOldFood != iNewFood)
					{
						GET_PLAYER(ePlayer).ChangeCapitalYieldChange(YIELD_FOOD, iNewFood - iOldFood);
#ifdef EG_REPLAYDATASET_FOODFROMCS
						GET_PLAYER(ePlayer).ChangeFoodFromMinorsTimes100(1024 * (iNewFood - iOldFood));
#endif
					}

					// Other Cities
					iOldFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesOtherCityFoodBonus(ePlayer, true);
					iNewFood = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesOtherCityFoodBonus(ePlayer);

					if (iOldFood != iNewFood)
					{
						GET_PLAYER(ePlayer).ChangeCityYieldChange(YIELD_FOOD, iNewFood - iOldFood);
#ifdef EG_REPLAYDATASET_FOODFROMCS
						GET_PLAYER(ePlayer).ChangeFoodFromMinorsTimes100(iNewFood - iOldFood);
#endif
					}
				}
			}

			// MANUFACTORY
			if (eTrait == MINOR_CIV_TRAIT_MANUFACTORY)
			{
				// Friends
#ifdef RELIGIOUS_UNITY_CS_BONUS
				ReligionTypes eFoundedReligion = GC.getGame().GetGameReligions()->GetReligionCreatedByPlayer(ePlayer);
				ReligionTypes eMajority = NO_RELIGION;
				if (GET_PLAYER(eLoopMinor).getCapitalCity())
				{
					eMajority = GET_PLAYER(eLoopMinor).getCapitalCity()->GetCityReligions()->GetReligiousMajority();
				}
				if (eFoundedReligion > NO_RELIGION && eFoundedReligion == eMajority && GC.getGame().GetGameReligions()->GetReligion(eFoundedReligion, NO_PLAYER)->m_Beliefs.HasBelief((BeliefTypes)GC.getInfoTypeForString("BELIEF_RELIGIOUS_UNITY"))
					|| GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsFriends(ePlayer))
#else
				if (GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsFriends(ePlayer))
#endif
				{
					int iOldProduction, iNewProduction;

					// Capital
					iOldProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsCapitalProductionBonus(ePlayer, NO_ERA, true);
					iNewProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsCapitalProductionBonus(ePlayer);

					if (iOldProduction != iNewProduction)
					{
						GET_PLAYER(ePlayer).ChangeCapitalYieldChange(YIELD_PRODUCTION, iNewProduction - iOldProduction);
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
						GET_PLAYER(ePlayer).ChangeProductionFromMinorsTimes100(1024 * (iNewProduction - iOldProduction));
#endif
					}

					// Other Cities
					iOldProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsOtherCityProductionBonus(ePlayer, NO_ERA, true);
					iNewProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetFriendsOtherCityProductionBonus(ePlayer);

					if (iOldProduction != iNewProduction)
					{
						GET_PLAYER(ePlayer).ChangeCityYieldChange(YIELD_PRODUCTION, iNewProduction - iOldProduction);
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
						GET_PLAYER(ePlayer).ChangeProductionFromMinorsTimes100(iNewProduction - iOldProduction);
#endif
					}
				}

				// Allies
#ifdef RELIGIOUS_UNITY_CS_BONUS
				if (eFoundedReligion > NO_RELIGION && eFoundedReligion == eMajority && GC.getGame().GetGameReligions()->GetReligion(eFoundedReligion, NO_PLAYER)->m_Beliefs.HasBelief((BeliefTypes)GC.getInfoTypeForString("BELIEF_RELIGIOUS_UNITY"))
					|| GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsAllies(ePlayer))
#else
				if (GET_PLAYER(eLoopMinor).GetMinorCivAI()->IsAllies(ePlayer))
#endif
				{
					int iOldProduction, iNewProduction;

					// Capital
					iOldProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesCapitalProductionBonus(ePlayer, true);
					iNewProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesCapitalProductionBonus(ePlayer);

					if (iOldProduction != iNewProduction)
					{
						GET_PLAYER(ePlayer).ChangeCapitalYieldChange(YIELD_PRODUCTION, iNewProduction - iOldProduction);
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
						GET_PLAYER(ePlayer).ChangeProductionFromMinorsTimes100(1024 * (iNewProduction - iOldProduction));
#endif
					}

					// Other Cities
					iOldProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesOtherCityProductionBonus(ePlayer, true);
					iNewProduction = GET_PLAYER(eLoopMinor).GetMinorCivAI()->GetAlliesOtherCityProductionBonus(ePlayer);

					if (iOldProduction != iNewProduction)
					{
						GET_PLAYER(ePlayer).ChangeCityYieldChange(YIELD_PRODUCTION, iNewProduction - iOldProduction);
#ifdef EG_REPLAYDATASET_PRODUCTIONFROMCS
						GET_PLAYER(ePlayer).ChangeProductionFromMinorsTimes100(iNewProduction - iOldProduction);
#endif
					}
				}
			}
		}
		if (GetEffects()->bNoSpiesInCS)
		{
		}
		if (GetEffects()->bDoubleResourceHappiness)
		{
		}
	}
#endif

	m_iTurnEnacted = -1;
}

bool CvActiveResolution::HasOngoingEffects()
{
	return GetEffects()->HasOngoingEffects();
}

int CvActiveResolution::GetTurnEnacted() const
{
	return m_iTurnEnacted;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvActiveResolution& writeTo)
{
	loadFrom >> (CvResolution&) writeTo;

	uint uiVersion;

	loadFrom >> uiVersion;
	if (uiVersion < 2)
	{
		// Since version 2, m_iID was moved to a different class
		int iTrash;
		loadFrom >> iTrash;
	}
	loadFrom >> writeTo.m_iTurnEnacted;

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvActiveResolution& readFrom)
{
	saveTo << (CvResolution&) readFrom;

	uint uiVersion = 2;

	saveTo << uiVersion;
	saveTo << readFrom.m_iTurnEnacted;

	return saveTo;
}


// ================================================================================
//			CvRepealProposal
// ================================================================================
CvRepealProposal::CvRepealProposal(void)
{
	m_iTargetResolutionID = -1;
}

CvRepealProposal::CvRepealProposal(CvActiveResolution* pResolution, PlayerTypes eProposalPlayer) : CvProposal(pResolution->GetID(), pResolution->GetType(), pResolution->GetLeague(), eProposalPlayer)
{
	m_iTargetResolutionID = pResolution->GetID();
	m_VoterDecision = (*(pResolution->GetVoterDecision()));
	m_ProposerDecision = (*(pResolution->GetProposerDecision()));
	m_RepealDecision = CvVoterDecision(RESOLUTION_DECISION_REPEAL);
}

CvRepealProposal::~CvRepealProposal(void)
{
}

void CvRepealProposal::Init()
{
}

bool CvRepealProposal::IsPassed(int iTotalSessionVotes)
{
	CvResolutionEntry* pInfo = GC.getResolutionInfo(GetType());
	CvAssert(pInfo);
	if (!pInfo)
	{
		return false;
	}

	int iQuorumPercent = pInfo->GetQuorumPercent();
	bool bQuorum = ((float)GetRepealDecision()->GetVotesCast() / (float)iTotalSessionVotes) >= ((float)iQuorumPercent / 100.0f);
	if (!bQuorum)
	{
		return false;
	}

	int iDecision = GetRepealDecision()->GetDecision();
	if (iDecision == LeagueHelpers::CHOICE_NONE)
	{
		// No votes, tied votes, etc.
		return false;
	}

	CvAssertMsg(GetRepealDecision()->GetType() == RESOLUTION_DECISION_REPEAL, "Unexpected decision type for repeal proposal. Please send Anton your save file and version.");
	if (GetRepealDecision()->GetType() == RESOLUTION_DECISION_REPEAL)
	{
		return (iDecision == LeagueHelpers::CHOICE_YES);
	}
	else
	{
		return false;
	}
}

CvString CvRepealProposal::GetProposalName(bool bForLogging)
{
	CvString s = "";

	if (bForLogging)
	{
		s += "REPEAL: ";
	}
	else
	{
		s += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PREFIX_REPEAL").toUTF8();
	}
	s += CvResolution::GetName();

	return s;
}

int CvRepealProposal::GetTargetResolutionID() const
{
	return m_iTargetResolutionID;
}

CvVoterDecision* CvRepealProposal::GetRepealDecision()
{
	return &m_RepealDecision;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvRepealProposal& writeTo)
{
	loadFrom >> (CvProposal&) writeTo;

	uint uiVersion;

	loadFrom >> uiVersion;
	loadFrom >> writeTo.m_iTargetResolutionID;
	loadFrom >> writeTo.m_RepealDecision;

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvRepealProposal& readFrom)
{
	saveTo << (CvProposal&) readFrom;

	uint uiVersion = 1;

	saveTo << uiVersion;
	saveTo << readFrom.m_iTargetResolutionID;
	saveTo << readFrom.m_RepealDecision;

	return saveTo;
}


// ================================================================================
//			CvLeague
// ================================================================================
CvLeague::CvLeague(void)
{
	m_eID = NO_LEAGUE;
	m_bUnitedNations = false;
	m_bInSession = false;
#ifdef POLICY_LEAGUE_SESSION_YIELD_BOOST_PER_DELEGATE
	m_bResolveSession = false;
#endif
	m_iTurnsUntilSession = MAX_INT;
	m_iNumResolutionsEverEnacted = 0;
	m_vEnactProposals.clear();
	m_vRepealProposals.clear();
	m_vActiveResolutions.clear();
	m_vMembers.clear();
	m_eHost = NO_PLAYER;
	m_vProjects.clear();
	m_iConsecutiveHostedSessions = 0;
	m_eAssignedName = NO_LEAGUE_NAME;
	ZeroMemory(m_szCustomName, sizeof(m_szCustomName));
	m_eLastSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
	m_eCurrentSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
	m_vEnactProposalsOnHold.clear();
	m_vRepealProposalsOnHold.clear();
}

CvLeague::CvLeague(LeagueTypes eID)
{
	m_eID = eID;
	m_bUnitedNations = false;
	m_bInSession = false;
#ifdef POLICY_LEAGUE_SESSION_YIELD_BOOST_PER_DELEGATE
	m_bResolveSession = false;
#endif
	m_iTurnsUntilSession = MAX_INT;
	m_iNumResolutionsEverEnacted = 0;
	m_vEnactProposals.clear();
	m_vRepealProposals.clear();
	m_vActiveResolutions.clear();
	m_vMembers.clear();
	m_eHost = NO_PLAYER;
	m_vProjects.clear();
	m_iConsecutiveHostedSessions = 0;
	m_eAssignedName = NO_LEAGUE_NAME;
	ZeroMemory(m_szCustomName, sizeof(m_szCustomName));
	m_eLastSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
	m_eCurrentSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
	m_vEnactProposalsOnHold.clear();
	m_vRepealProposalsOnHold.clear();
}

CvLeague::~CvLeague(void)
{
}

CvLeague::Member::Member(void)
{
	ePlayer = NO_PLAYER;
	iExtraVotes = 0;
	sVoteSources = "";
	bMayPropose = false;
	iProposals = 0;
	iVotes = 0;
	iAbstainedVotes = 0;
	bEverBeenHost = false;
	bAlwaysBeenHost = true;
}

CvLeague::Member::~Member(void)
{
}

CvLeague::Project::Project(void)
{
	eType = NO_LEAGUE_PROJECT;
	for (int i = 0; i < MAX_MAJOR_CIVS; i++)
	{
		vProductionList.push_back(0);
	}
	bComplete = false;
	bProgressWarningSent = false;
}

CvLeague::Project::~Project(void)
{
}

void CvLeague::Init(LeagueSpecialSessionTypes eGoverningSpecialSession)
{
	CvAssertMsg(m_eID != NO_LEAGUE, "Initializing a CvLeague without a proper ID. Please send Anton your save file and verison.");
	CvAssert(eGoverningSpecialSession != NO_LEAGUE_SPECIAL_SESSION);
	m_eLastSpecialSession = eGoverningSpecialSession; // Fake the last special session so we have data to inform the World Congress's status
	AssignProposalPrivileges();
#ifdef CAN_PROPOSE_ENACT_UPDATES_ONCE_PER_SESSION
	UpdateCanProposeEnact();
#endif
	ResetTurnsUntilSession();
}

void CvLeague::DoTurn(LeagueSpecialSessionTypes eTriggeredSpecialSession)
{
	CvAssert(eTriggeredSpecialSession == NO_LEAGUE_SPECIAL_SESSION || CanStartSpecialSession(eTriggeredSpecialSession));

	// Special Session this turn, put everything else on hold
	if (eTriggeredSpecialSession != NO_LEAGUE_SPECIAL_SESSION && CanStartSpecialSession(eTriggeredSpecialSession))
	{
		CheckStartSpecialSession(eTriggeredSpecialSession);
	}
	else if (!IsInSession())
	{
		ChangeTurnsUntilSession(-1);
		CvAssertMsg(GetTurnsUntilSession() >= 0, "Turns left until League session is negative. Please send Anton your save file and version.");
		if (GetTurnsUntilSession() <= 0)
		{
			CheckStartSession();
		}
		else if (GetTurnsUntilSession() == GC.getLEAGUE_SESSION_SOON_WARNING_TURNS())
		{
			NotifySessionSoon(GetTurnsUntilSession());
		}
	}
	else
	{
#ifdef FINISH_LEAGUE_SESSION_EVEN_IF_SOMEONE_HAS_VOTES
		for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
		{
			if (CanVote(it->ePlayer))
				DoVoteAbstain(it->ePlayer, GetRemainingVotesForMember(it->ePlayer));
		}
#endif
		CheckFinishSession();
	}

#ifndef FIX_PROJECTS_NOTIFICATIONS_PERCENTAGE
	CheckProjectsProgress();
#endif
}

LeagueTypes CvLeague::GetID() const
{
	return m_eID;
}

Localization::String CvLeague::GetName()
{
	Localization::String sName = Localization::Lookup("TXT_KEY_LEAGUE_WORLD_CONGRESS_GENERIC");
	if (m_szCustomName == NULL || strlen(m_szCustomName) == 0)
	{
		if (IsUnitedNations())
		{
			sName = Localization::Lookup("TXT_KEY_LEAGUE_UNITED_NATIONS");
		}
		else if (HasHostMember() && m_eAssignedName != NO_LEAGUE_NAME)
		{
			PlayerTypes eHost = GetHostMember();
			CvCity* pCapital = GET_PLAYER(eHost).getCapitalCity();
			CvLeagueNameEntry* pInfo = GC.getLeagueNameInfo(m_eAssignedName);
			if (pCapital && pInfo)
			{
				CvString sOrdinalKey = "";
				sOrdinalKey.Format("TXT_KEY_LEAGUE_ORDINAL_%d", GetConsecutiveHostedSessions() + 1);
				Localization::String sOrdinal = Localization::Lookup(sOrdinalKey);

				sName = Localization::Lookup(pInfo->GetNameKey());
				//antonjs: temp: Did our lookup return the same thing (ie. we don't have that text key)?
				if (sOrdinalKey.compare(sOrdinal.toUTF8()) == 0)
				{
					sName << "" << pCapital->getNameKey();
				}
				else
				{
					sName << sOrdinal << pCapital->getNameKey();
				}
			}
		}
	}
	// Custom name
	else
	{
		return m_szCustomName;
	}

	return sName;
}

bool CvLeague::CanChangeCustomName(PlayerTypes ePlayer)
{
	if (!IsInSession())
	{
		if (IsHostMember(ePlayer))
		{
			return true;
		}
	}
	
	return false;
}

void CvLeague::DoChangeCustomName(PlayerTypes ePlayer, const char* szCustomName)
{
	CvAssertMsg(CanChangeCustomName(ePlayer), "Player is not allowed to edit league name.");
	if (CanChangeCustomName(ePlayer))
	{
		CvAssertMsg(strlen(szCustomName) <= sizeof(m_szCustomName), "Custom name for league is too long.");
		if(szCustomName != NULL && strlen(szCustomName) <= sizeof(m_szCustomName))
		{
			strcpy_s(m_szCustomName, szCustomName);
		}
	}
}

int CvLeague::GetSessionTurnInterval()
{
	// Determined by most recent special session
	int iInterval = GC.getLEAGUE_SESSION_INTERVAL_BASE_TURNS();
	if (GetCurrentSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
	{
		CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(GetCurrentSpecialSession());
		CvAssert(pInfo != NULL);
		if (pInfo != NULL)
		{
			iInterval = pInfo->GetTurnsBetweenSessions();
		}
	}
	else if (GetLastSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
	{
		CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(GetLastSpecialSession());
		CvAssert(pInfo != NULL);
		if (pInfo != NULL)
		{
			iInterval = pInfo->GetTurnsBetweenSessions();
		}
	}

	// Modified by game speed
	iInterval = (iInterval * GC.getGame().getGameSpeedInfo().getLeaguePercent()) / 100;

	if (DEBUG_LEAGUES)
	{
		return 2;
	}

	return iInterval;
}

bool CvLeague::IsInSession()
{
	return m_bInSession;
}

bool CvLeague::IsInSpecialSession()
{
	return (IsInSession() && GetCurrentSpecialSession() != NO_LEAGUE_SPECIAL_SESSION);
}

void CvLeague::SetInSession(bool bInSession)
{
	m_bInSession = bInSession;
	if (bInSession)
	{
	}
	else
	{
		if (m_eCurrentSpecialSession != NO_LEAGUE_SPECIAL_SESSION)
		{
			m_eLastSpecialSession = m_eCurrentSpecialSession;
		}
		m_eCurrentSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
	}
}

#ifdef POLICY_LEAGUE_SESSION_YIELD_BOOST_PER_DELEGATE
bool CvLeague::IsResolveSession()
{
	return m_bResolveSession;
}

void CvLeague::SetResolveSession(bool bResolve)
{
	m_bResolveSession = bResolve;
}
#endif

void CvLeague::SetInSession(LeagueSpecialSessionTypes eSpecialSession)
{
	CvAssertMsg(eSpecialSession != NO_LEAGUE_SPECIAL_SESSION, "Attempting to set the league in session with an invalid special session type. Please send Anton your save file and version.");
	if (eSpecialSession != NO_LEAGUE_SPECIAL_SESSION)
	{
		SetInSession(true);
		m_eCurrentSpecialSession = eSpecialSession;
	}
}

int CvLeague::GetTurnsUntilSession() const
{
	return m_iTurnsUntilSession;
}

void CvLeague::SetTurnsUntilSession(int iTurns)
{
	CvAssertMsg(iTurns >= 0, "Cannot have negative turns until next League session. Please send Anton your save file and version.");

	if (iTurns >= 0)
	{
		m_iTurnsUntilSession = iTurns;
	}
}

void CvLeague::ChangeTurnsUntilSession(int iChange)
{
	SetTurnsUntilSession(GetTurnsUntilSession() + iChange);
}

void CvLeague::ResetTurnsUntilSession()
{
	SetTurnsUntilSession(GetSessionTurnInterval());
}

int CvLeague::GetTurnsUntilVictorySession()
{
	VictoryTypes eDiploVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);
	if (eDiploVictory == NO_VICTORY)
	{
		return 999;
	}
	if (!GC.getGame().isVictoryValid(eDiploVictory))
	{
		return 999;
	}

	int iValue = 999;

	bool bVictoryProposed = false;
	for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
	{
		if (it->GetEffects()->bDiplomaticVictory)
		{
			bVictoryProposed = true;
		}
	}

	// Is victory already proposed?
	if (bVictoryProposed)
	{
		iValue = GetTurnsUntilSession();
	}
	// Is there a recurring victory proposal, for the following session?
	else
	{
		LeagueSpecialSessionTypes eGoverningSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
		if (GetCurrentSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
		{
			eGoverningSpecialSession = GetCurrentSpecialSession();
		}
		else if (GetLastSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
		{
			eGoverningSpecialSession = GetLastSpecialSession();
		}
		CvAssert(eGoverningSpecialSession != NO_LEAGUE_SPECIAL_SESSION);

		bool bRecurringVictoryProposal = false;
		if (eGoverningSpecialSession != NO_LEAGUE_SPECIAL_SESSION)
		{
			CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eGoverningSpecialSession);
			CvAssert(pInfo != NULL);
			if (pInfo != NULL)
			{
				ResolutionTypes e = pInfo->GetRecurringProposal();
				if (e != NO_RESOLUTION)
				{
					CvResolutionEntry* p = GC.getResolutionInfo(e);
					if (p != NULL && p->IsDiplomaticVictory())
					{
						bRecurringVictoryProposal = true;
					}
				}
			}
		}

		if (bRecurringVictoryProposal)
		{
			// Time until next session, plus the interval until the following session
			iValue = GetTurnsUntilSession() + GetSessionTurnInterval();
		}
	}

	return iValue;
}

int CvLeague::GetVotesSpentThisSession()
{
	int iVotes = 0;
	if (IsInSession())
	{
		for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
		{
			iVotes += it->GetVoterDecision()->GetVotesCast();
		}
		for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); ++it)
		{
			iVotes += it->GetRepealDecision()->GetVotesCast();
		}
	}
	return iVotes;
}

LeagueSpecialSessionTypes CvLeague::GetLastSpecialSession() const
{
	return m_eLastSpecialSession;
}

LeagueSpecialSessionTypes CvLeague::GetCurrentSpecialSession() const
{
	return m_eCurrentSpecialSession;
}

bool CvLeague::CanStartSpecialSession(LeagueSpecialSessionTypes /*eSpecialSession*/)
{
	// Cannot already be in session
	if (IsInSession())
		return false;

	// Cannot have proposals on hold
	if (HasProposalsOnHold())
		return false;

	// Cannot be awaiting proposals or votes from anyone (UI complications with overview screen)
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
	{
		if (CanPropose(it->ePlayer) || CanVote(it->ePlayer))
		{
			return false;
		}
	}
	
	return true;
}

bool CvLeague::IsUnitedNations() const
{
	return m_bUnitedNations;
}

void CvLeague::SetUnitedNations(bool bValue)
{
	if (m_bUnitedNations != bValue)
	{
		m_bUnitedNations = bValue;
	}
}

void CvLeague::DoVoteEnact(int iID, PlayerTypes eVoter, int iNumVotes, int iChoice)
{
	bool bProcessed = false;
	if (CanVote(eVoter) && GetRemainingVotesForMember(eVoter) >= iNumVotes)
	{
		for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); it++)
		{
			if (it->GetID() == iID)
			{
				it->GetVoterDecision()->ProcessVote(eVoter, iNumVotes, iChoice);
				GetMember(eVoter)->iVotes -= iNumVotes;
				CvAssertMsg(GetRemainingVotesForMember(eVoter) >= 0, "A voter now has negative votes remaining. Please send Anton your save file and version.");
				bProcessed = true;
				break;
			}
		}
	}
	CvAssertMsg(bProcessed, "Attempt to vote on an enact resolution when not allowed to. Please send Anton your save file and version.");
	GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true);
}

void CvLeague::DoVoteRepeal(int iResolutionID, PlayerTypes eVoter, int iNumVotes, int iChoice)
{
	bool bProcessed = false;
	if (CanVote(eVoter) && GetRemainingVotesForMember(eVoter) >= iNumVotes)
	{
		if (IsRepealProposed(iResolutionID))
		{
			for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); it++)
			{
				if (it->GetTargetResolutionID() == iResolutionID)
				{
					it->GetRepealDecision()->ProcessVote(eVoter, iNumVotes, iChoice);
					GetMember(eVoter)->iVotes -= iNumVotes;
					CvAssertMsg(GetRemainingVotesForMember(eVoter) >= 0, "A voter now has negative votes remaining. Please send Anton your save file and version.");
					bProcessed = true;
					break;
				}
			}
		}
	}
	CvAssertMsg(bProcessed, "Attempt to vote on a repeal resolution when not allowed to. Please send Anton your save file and version.");
	GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true);
}

void CvLeague::DoVoteAbstain(PlayerTypes eVoter, int iNumVotes)
{
	bool bProcessed = false;
	if (CanVote(eVoter) && GetRemainingVotesForMember(eVoter) >= iNumVotes)
	{
		GetMember(eVoter)->iVotes -= iNumVotes;
		GetMember(eVoter)->iAbstainedVotes += iNumVotes;
		CvAssertMsg(GetRemainingVotesForMember(eVoter) >= 0, "A voter now has negative votes remaining. Please send Anton your save file and version.");
		bProcessed = true;
	}
	CvAssertMsg(bProcessed, "Attempt to abstain votes but not allowed to. Please send Anton your save file and version.");
	GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true);
}

void CvLeague::DoProposeEnact(ResolutionTypes eResolution, PlayerTypes eProposer, int iChoice)
{
	if (!CanProposeEnact(eResolution, eProposer, iChoice))
	{
		CvAssertMsg(false, "Attempt to propose enact resolution when not allowed to. Please send Anton your save file and version.");
		return;
	}

	if (eProposer != NO_PLAYER)
	{
		// Decrement remaining proposals
		if (IsMember(eProposer))
		{
			GetMember(eProposer)->iProposals = GetMember(eProposer)->iProposals - 1;
			CvAssertMsg(GetMember(eProposer)->iProposals >= 0, "Attempt to propose enact resolution when not allowed to. Please send Anton your save file and version.");
		}
		else
		{
			CvAssertMsg(false, "Attempt to propose enact resolution when not allowed to. Please send Anton your save file and version.");
		}

		// Proposals made by players could affect Diplomacy AI
		LeagueHelpers::PlayerList vLikers = GetMembersThatLikeProposal(eResolution, eProposer, iChoice);
		for (LeagueHelpers::PlayerList::iterator it = vLikers.begin(); it != vLikers.end(); ++it)
		{
			CvAssert((*it) != NO_PLAYER);
			CvAssert(CanEverVote(*it));
			GET_PLAYER(*it).GetDiplomacyAI()->SetTurnsSinceWeLikedTheirProposal(eProposer, 0);
		}
		LeagueHelpers::PlayerList vDislikers = GetMembersThatDislikeProposal(eResolution, eProposer, iChoice);
		for (LeagueHelpers::PlayerList::iterator it = vDislikers.begin(); it != vDislikers.end(); ++it)
		{
			CvAssert((*it) != NO_PLAYER);
			CvAssert(CanEverVote(*it));
			GET_PLAYER(*it).GetDiplomacyAI()->SetTurnsSinceWeDislikedTheirProposal(eProposer, 0);
		}
	}

	CvEnactProposal proposal(GC.getGame().GetGameLeagues()->GenerateResolutionUniqueID(), eResolution, GetID(), eProposer, iChoice);
	m_vEnactProposals.push_back(proposal);
	GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true);
}

void CvLeague::DoProposeRepeal(int iResolutionID, PlayerTypes eProposer)
{
	if (!CanProposeRepeal(iResolutionID, eProposer))
	{
		CvAssertMsg(false, "Attempt to propose repeal resolution when not allowed to. Please send Anton your save file and version.");
		return;
	}
	
	int iFound = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		if (it->GetID() == iResolutionID)
		{
			if (eProposer != NO_PLAYER)
			{
				// Decrement remaining proposals
				if (IsMember(eProposer))
				{
					GetMember(eProposer)->iProposals = GetMember(eProposer)->iProposals - 1;
					CvAssertMsg(GetMember(eProposer)->iProposals >= 0, "Attempt to propose enact resolution when not allowed to. Please send Anton your save file and version.");
				}
				else
				{
					CvAssertMsg(false, "Attempt to propose enact resolution when not allowed to. Please send Anton your save file and version.");
				}

				// Proposals made by players could affect Diplomacy AI
				LeagueHelpers::PlayerList vLikers = GetMembersThatLikeProposal(iResolutionID, eProposer);
				for (LeagueHelpers::PlayerList::iterator innerIt = vLikers.begin(); innerIt != vLikers.end(); ++innerIt)
				{
					CvAssert((*innerIt) != NO_PLAYER);
					CvAssert(CanEverVote(*innerIt));
					GET_PLAYER(*innerIt).GetDiplomacyAI()->SetTurnsSinceWeLikedTheirProposal(eProposer, 0);
				}
				LeagueHelpers::PlayerList vDislikers = GetMembersThatDislikeProposal(iResolutionID, eProposer);
				for (LeagueHelpers::PlayerList::iterator innerIt = vDislikers.begin(); innerIt != vDislikers.end(); ++innerIt)
				{
					CvAssert((*innerIt) != NO_PLAYER);
					CvAssert(CanEverVote(*innerIt));
					GET_PLAYER(*innerIt).GetDiplomacyAI()->SetTurnsSinceWeDislikedTheirProposal(eProposer, 0);
				}
			}

			CvRepealProposal proposal(it, eProposer);
			m_vRepealProposals.push_back(proposal);
			iFound++;

			// XP2 Achievement
			if (!GC.getGame().isGameMultiPlayer())
			{
				PlayerTypes eOriginalProposer = it->GetProposerDecision()->GetProposer();
				if (eOriginalProposer != NO_PLAYER && eOriginalProposer == eProposer)
				{
					if (GET_PLAYER(eProposer).isHuman() && GET_PLAYER(eProposer).isLocalPlayer())
					{
						gDLL->UnlockAchievement(ACHIEVEMENT_XP2_42);
					}
				}
			}
		}

	}
	CvAssertMsg(iFound == 1, "Unexpected number of active resolutions with this ID. Please send Anton your save file and version.");
	GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true);
}

bool CvLeague::CanProposeEnactAnyChoice(ResolutionTypes eResolution, PlayerTypes eProposer)
{
	CvAssertMsg(eResolution >= 0, "Invalid resolution type. Please send Anton your save file and version.");
	CvAssertMsg(eResolution < GC.getNumResolutionInfos(), "Resolution index out of bounds. Please send Anton your save file and version.");

	CvResolutionEntry* pInfo = GC.getResolutionInfo(eResolution);
	CvAssertMsg(pInfo, "Resolution info is null. Please send Anton your save file and version.");
	if (!pInfo) return false;

	std::vector<int> vValidChoices = GetChoicesForDecision(pInfo->GetProposerDecision(), eProposer);
	if (vValidChoices.empty())
	{
		if (pInfo->GetProposerDecision() == RESOLUTION_DECISION_NONE)
		{
			if (CanProposeEnact(eResolution, eProposer, LeagueHelpers::CHOICE_NONE))
			{
				return true;
			}
		}
	}
	else
	{
		for (uint i = 0; i < vValidChoices.size(); i++)
		{
			if (CanProposeEnact(eResolution, eProposer, vValidChoices[i]))
			{
				return true;
			}
		}
	}

	return false;
}

bool CvLeague::CanProposeEnact(ResolutionTypes eResolution, PlayerTypes eProposer, int iChoice, CvString* sTooltipSink)
{
	CvAssertMsg(eResolution >= 0, "Invalid resolution type. Please send Anton your save file and version.");
	CvAssertMsg(eResolution < GC.getNumResolutionInfos(), "Resolution index out of bounds. Please send Anton your save file and version.");

	CvResolutionEntry* pInfo = GC.getResolutionInfo(eResolution);
	CvAssertMsg(pInfo, "Resolution info is null. Please send Anton your save file and version.");
	if (!pInfo) return false;

	bool bValid = true;
	
	// Is a player proposing this?
	if (eProposer != NO_PLAYER)
	{
		// Must be a proposal that can be made by players
		if (pInfo->IsNoProposalByPlayer())
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_NO_PLAYER_PROPOSAL").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			bValid = false;
		}

		// Must be a member
		if (!IsMember(eProposer))
			bValid = false;

		// Must have right to propose
		if (!CanPropose(eProposer))
			bValid = false;
	}

	// Is this a valid choice for this proposal?
	bool bValidChoice = false;
	std::vector<int> vValidChoices = GetChoicesForDecision(pInfo->GetProposerDecision(), eProposer);
	if (vValidChoices.empty())
	{
		if (pInfo->GetProposerDecision() == RESOLUTION_DECISION_NONE && LeagueHelpers::CHOICE_NONE == iChoice)
		{
			bValidChoice = true;
		}
	}
	else
	{
		for (uint i = 0; i < vValidChoices.size(); i++)
		{
			if (vValidChoices[i] == iChoice)
			{
				bValidChoice = true;
				break;
			}
		}
	}
	if (!bValidChoice)
	{
		bValid = false;
	}
	else
	{
		// Effects must be allowed in our game
		if (!IsResolutionEffectsValid(eResolution, iChoice, sTooltipSink))
		{
			bValid = false;
		}

		// Must not already be proposed
		if (IsEnactProposed(eResolution, iChoice))
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_ALREADY_PROPOSED").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			bValid = false;
		}

		// Must not already be active
		if (IsActiveResolution(eResolution, iChoice))
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_ALREADY_ENACTED").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			bValid = false;
		}

#ifdef NEW_LEAGUE_RESOLUTIONS
		for (uint i = 0; i < m_vEnactProposals.size(); i++)
		{
			if (pInfo->GetProposerDecision() == RESOLUTION_DECISION_ANY_LUXURY_RESOURCE && GC.getResolutionInfo(m_vEnactProposals[i].GetType())->GetProposerDecision() == RESOLUTION_DECISION_ANY_LUXURY_RESOURCE)
			{
				if (m_vEnactProposals[i].GetProposerDecision()->GetDecision() == iChoice)
				{
					if (sTooltipSink != NULL)
					{
						(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
						(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_ALREADY_PROPOSED").toUTF8();
						(*sTooltipSink) += "[ENDCOLOR]";
					}
					bValid = false;
				}
			}
		}
		for (uint i = 0; i < m_vActiveResolutions.size(); i++)
		{
			if (pInfo->GetProposerDecision() == RESOLUTION_DECISION_ANY_LUXURY_RESOURCE && GC.getResolutionInfo(m_vActiveResolutions[i].GetType())->GetProposerDecision() == RESOLUTION_DECISION_ANY_LUXURY_RESOURCE)
			{
				if (m_vActiveResolutions[i].GetProposerDecision()->GetDecision() == iChoice)
				{
					if (sTooltipSink != NULL)
					{
						(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
						(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_ALREADY_ENACTED").toUTF8();
						(*sTooltipSink) += "[ENDCOLOR]";
					}
					bValid = false;
				}
			}
		}
#endif
	}

	// Prereq tech
	if (pInfo->GetTechPrereqAnyMember() != NO_TECH)
	{
		bool bMemberHasTech = false;
#ifdef CAN_PROPOSE_ENACT_UPDATES_ONCE_PER_SESSION
		for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
		{
			if (GET_TEAM(GET_PLAYER(it->ePlayer).getTeam()).GetTeamTechs()->HasTechForLeague(pInfo->GetTechPrereqAnyMember()))
			{
				bMemberHasTech = true;
				break;
			}
		}
#else
		for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
		{
			if (GET_TEAM(GET_PLAYER(it->ePlayer).getTeam()).GetTeamTechs()->HasTech(pInfo->GetTechPrereqAnyMember()))
			{
				bMemberHasTech = true;
				break;
			}
		}
#endif

		if (!bMemberHasTech)
		{
			if (sTooltipSink != NULL)
			{
				CvTechEntry* pTechInfo = GC.getTechInfo(pInfo->GetTechPrereqAnyMember());
				if (pTechInfo != NULL)
				{
					Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_NO_TECH");
					sTemp << pTechInfo->GetDescriptionKey();

					(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
					(*sTooltipSink) += sTemp.toUTF8();
					(*sTooltipSink) += "[ENDCOLOR]";
				}
			}
			bValid = false;
		}
	}

	// League Project
	if (pInfo->GetLeagueProjectEnabled() != NO_LEAGUE_PROJECT)
	{
		// Is the project already underway or completed??
		if (IsProjectActive(pInfo->GetLeagueProjectEnabled()) || IsProjectComplete(pInfo->GetLeagueProjectEnabled()))
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_ALREADY_ENACTED").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			bValid = false;
		}
	}

	return bValid;
}

bool CvLeague::CanProposeRepeal(int iResolutionID, PlayerTypes eProposer, CvString* sTooltipSink)
{
	bool bValid = true;

	// Is a player proposing this?
	if (eProposer != NO_PLAYER)
	{
		// Must be a member
		if (!IsMember(eProposer))
			bValid = false;

		// Must have right to propose
		if (!CanPropose(eProposer))
			bValid = false;
	}
	CvAssertMsg(eProposer != NO_PLAYER, "Checking to see if a NO_PLAYER can propose a repeal. Please send Anton your save file and version.");

	// Must not already be proposed
	if (IsRepealProposed(iResolutionID))
	{
		if (sTooltipSink != NULL)
		{
			(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
			(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_ALREADY_PROPOSED").toUTF8();
			(*sTooltipSink) += "[ENDCOLOR]";
		}
		bValid = false;
	}

	// Must already be active
	for (uint iIndex = 0; iIndex < m_vActiveResolutions.size(); iIndex++)
	{
		if (m_vActiveResolutions[iIndex].GetID() == iResolutionID)
		{
			return bValid;
		}
	}
	CvAssertMsg(false, "Resolution ID does not exist as an Active Resolution. Please send Anton your save file and version.");
	return false;
}

// Check if a resolution's effects are valid in the game (Game Options, Embargo on a civ that is now dead, etc.)
bool CvLeague::IsResolutionEffectsValid(ResolutionTypes eResolution, int iProposerChoice, CvString* sTooltipSink)
{
	if (eResolution == NO_RESOLUTION)
	{
		return false;
	}
	CvResolutionEntry* pInfo = GC.getResolutionInfo(eResolution);
	if (pInfo == NULL)
	{
		return false;
	}

	if (pInfo->IsDiplomaticVictory())
	{
		VictoryTypes eDiploVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);
		if (eDiploVictory == NO_VICTORY)
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_GAMEOPTION").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			return false;
		}
		else
		{
			if (!GC.getGame().isVictoryValid(eDiploVictory))
			{
				if (sTooltipSink != NULL)
				{
					(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
					(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_GAMEOPTION").toUTF8();
					(*sTooltipSink) += "[ENDCOLOR]";
				}
				return false;
			}
		}
	}
	if (pInfo->IsEmbargoCityStates())
	{
		if (GC.getGame().GetNumMinorCivsEver() <= 0)
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_GAMEOPTION").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			return false;
		}
	}
	if (pInfo->IsEmbargoPlayer())
	{
		PlayerTypes eTargetPlayer = (PlayerTypes) iProposerChoice;
		CvAssert(eTargetPlayer >= 0 && eTargetPlayer < MAX_MAJOR_CIVS);
		if (eTargetPlayer >= 0 && eTargetPlayer < MAX_MAJOR_CIVS)
		{
			// Player is dead
			if (!GET_PLAYER(eTargetPlayer).isAlive())
			{
				return false;
			}

			// Player is alive but no cities left (Complete Kills option)
			if (GET_PLAYER(eTargetPlayer).getNumCities() <= 0)
			{
				return false;
			}
		}
	}
	if (pInfo->IsNoResourceHappiness())
	{
		if (GC.getGame().isOption(GAMEOPTION_NO_HAPPINESS))
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_GAMEOPTION").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			return false;
		}
	}
	if (pInfo->GetMemberDiscoveredTechMod() != 0)
	{
		if (GC.getGame().isOption(GAMEOPTION_NO_SCIENCE))
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_GAMEOPTION").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			return false;
		}
	}
	if (pInfo->GetVotesForFollowingReligion() != 0 ||
		pInfo->GetHolyCityTourism() != 0 ||
		pInfo->GetReligionSpreadStrengthMod() != 0)
	{
		if (GC.getGame().isOption(GAMEOPTION_NO_RELIGION))
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_GAMEOPTION").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			return false;
		}
	}
	if (pInfo->GetVotesForFollowingIdeology() != 0 ||
		pInfo->GetOtherIdeologyRebellionMod() != 0)
	{
		if (GC.getGame().isOption(GAMEOPTION_NO_POLICIES))
		{
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_GAMEOPTION").toUTF8();
				(*sTooltipSink) += "[ENDCOLOR]";
			}
			return false;
		}
	}

	return true;
}

bool CvLeague::IsAnythingProposed()
{
	bool bEnactProposals = !m_vEnactProposals.empty();
	bool bRepealProposals = !m_vRepealProposals.empty();

	return (bEnactProposals || bRepealProposals);
}

//antonjs: todo: refactor:
bool CvLeague::IsActiveResolution(ResolutionTypes eResolution, int iProposerChoice)
{
	CvAssertMsg(eResolution >= 0, "Invalid resolution type. Please send Anton your save file and version.");
	CvAssertMsg(eResolution < GC.getNumResolutionInfos(), "Resolution index out of bounds. Please send Anton your save file and version.");

	for (uint i = 0; i < m_vActiveResolutions.size(); i++)
	{
		if (m_vActiveResolutions[i].GetType() == eResolution)
		{
			CvResolutionEntry* pInfo = GC.getResolutionInfo(eResolution);
			if (pInfo != NULL)
			{
				// Unique Type resolutions can only have one active resolution of its type at once
				if (pInfo->IsUniqueType())
				{
					return true;
				}

				if (m_vActiveResolutions[i].GetProposerDecision()->GetDecision() == iProposerChoice)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvLeague::IsProposed(int iResolutionID, bool bRepeal, bool bCheckOnHold)
{
	bool bFound = false;
	if (bRepeal)
	{
		for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); ++it)
		{
			if (it->GetID() == iResolutionID)
			{
				bFound = true;
				break;
			}
		}
		if (bCheckOnHold)
		{
			for (RepealProposalList::iterator it = m_vRepealProposalsOnHold.begin(); it != m_vRepealProposalsOnHold.end(); ++it)
			{
				if (it->GetID() == iResolutionID)
				{
					bFound = true;
					break;
				}
			}
		}
	}
	else
	{
		for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
		{
			if (it->GetID() == iResolutionID)
			{
				bFound = true;
				break;
			}
		}
		if (bCheckOnHold)
		{
			for (EnactProposalList::iterator it = m_vEnactProposalsOnHold.begin(); it != m_vEnactProposalsOnHold.end(); ++it)
			{
				if (it->GetID() == iResolutionID)
				{
					bFound = true;
					break;
				}
			}
		}
	}
	return bFound;
}

//antonjs: todo: refactor:
bool CvLeague::IsEnactProposed(ResolutionTypes eResolution, int iProposerChoice)
{
	CvAssertMsg(eResolution >= 0, "Invalid resolution type. Please send Anton your save file and version.");
	CvAssertMsg(eResolution < GC.getNumResolutionInfos(), "Resolution index out of bounds. Please send Anton your save file and version.");

	for (uint i = 0; i < m_vEnactProposals.size(); i++)
	{
		if (m_vEnactProposals[i].GetType() == eResolution)
		{
			CvResolutionEntry* pInfo = GC.getResolutionInfo(eResolution);
			if (pInfo != NULL)
			{
				// Unique Type resolutions can only have one active resolution of its type at once
				if (pInfo->IsUniqueType())
				{
					return true;
				}

				if (m_vEnactProposals[i].GetProposerDecision()->GetDecision() == iProposerChoice)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvLeague::IsRepealProposed(int iResolutionID) const
{
	for (uint i = 0; i < m_vRepealProposals.size(); i++)
	{
		if (m_vRepealProposals[i].GetTargetResolutionID() == iResolutionID)
		{
			return true;
		}
	}

	return false;
}

std::vector<int> CvLeague::GetChoicesForDecision(ResolutionDecisionTypes eDecision, PlayerTypes eDecider)
{
	std::vector<int> vChoices;
	switch(eDecision)
	{
	case RESOLUTION_DECISION_NONE:
		break;
	case RESOLUTION_DECISION_YES_OR_NO:
	case RESOLUTION_DECISION_REPEAL:
		vChoices.push_back(LeagueHelpers::CHOICE_YES);
		vChoices.push_back(LeagueHelpers::CHOICE_NO);
		break;
	case RESOLUTION_DECISION_ANY_MEMBER:
		for (uint i = 0; i < m_vMembers.size(); i++)
		{
			vChoices.push_back(m_vMembers[i].ePlayer);
		}
		break;
	case RESOLUTION_DECISION_CITY:
		break;
	case RESOLUTION_DECISION_ANY_LUXURY_RESOURCE:
		for (int i = 0; i < GC.getNumResourceInfos(); i++)
		{
			CvResourceInfo* pInfo = GC.getResourceInfo((ResourceTypes)i);
			if (pInfo && pInfo->getResourceUsage() == RESOURCEUSAGE_LUXURY)
			{
				if (GC.getMap().getNumResources((ResourceTypes)i) > 0)
				{
					vChoices.push_back(i);
				}
			}
		}
		break;
	case RESOLUTION_DECISION_MAJOR_CIV_MEMBER:
#ifdef DIPLO_VICTORY_VOTING
		for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
		{
			if (it->GetEffects()->bDiplomaticVictory)
			{
				for (uint i = 0; i < m_vMembers.size(); i++)
				{
					if (m_vMembers[i].ePlayer == eDecider)
					{
						vChoices.push_back(m_vMembers[i].ePlayer);
					}
				}
			}
			else
			{
				for (uint i = 0; i < m_vMembers.size(); i++)
				{
					if (!GET_PLAYER(m_vMembers[i].ePlayer).isMinorCiv())
					{
						vChoices.push_back(m_vMembers[i].ePlayer);
					}
				}
			}
		}
#else
		for (uint i = 0; i < m_vMembers.size(); i++)
		{
			if (!GET_PLAYER(m_vMembers[i].ePlayer).isMinorCiv())
			{
				vChoices.push_back(m_vMembers[i].ePlayer);
			}
		}
#endif
		break;
	case RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER:
		for (uint i = 0; i < m_vMembers.size(); i++)
		{
			if (m_vMembers[i].ePlayer != eDecider && !GET_PLAYER(m_vMembers[i].ePlayer).isMinorCiv())
			{
				vChoices.push_back(m_vMembers[i].ePlayer);
			}
		}
		break;
	case RESOLUTION_DECISION_RELIGION:
		for (uint i = 0; i < MAX_MAJOR_CIVS; i++)
		{
			PlayerTypes e = (PlayerTypes) i;
			if (GET_PLAYER(e).isAlive() && GET_PLAYER(e).GetReligions()->HasCreatedReligion())
			{
				vChoices.push_back(GET_PLAYER(e).GetReligions()->GetReligionCreatedByPlayer());
			}
		}
		break;
	case RESOLUTION_DECISION_IDEOLOGY:
		for (int i = 0; i < GC.getNumPolicyBranchInfos(); i++)
		{
			CvPolicyBranchEntry* pInfo = GC.getPolicyBranchInfo((PolicyBranchTypes)i);
			if (pInfo != NULL)
			{
				if (pInfo->IsPurchaseByLevel())
				{
					vChoices.push_back(pInfo->GetID());
				}
			}
		}
		break;
	default:
		break;
	}
	return vChoices;
}

CvString CvLeague::GetTextForChoice(ResolutionDecisionTypes eDecision, int iChoice)
{
	return LeagueHelpers::GetTextForChoice(eDecision, iChoice);
}

// Returns all possible resolutions, even ones that might already be active
std::vector<ResolutionTypes> CvLeague::GetInactiveResolutions() const
{
	std::vector<ResolutionTypes> v;
	for (int i = 0; i < GC.getNumResolutionInfos(); i++)
	{
		ResolutionTypes e = (ResolutionTypes)i;
		CvResolutionEntry* pInfo = GC.getResolutionInfo(e);
		if (pInfo)
		{
			v.push_back(e);
		}
	}

	return v;
}

CvEnactProposal* CvLeague::GetEnactProposal(int iResolutionID)
{
	for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
	{
		if (it->GetID() == iResolutionID)
		{
			return it;
		}
	}
	return NULL;
}

EnactProposalList CvLeague::GetEnactProposals() const
{
	return m_vEnactProposals;
}

EnactProposalList CvLeague::GetEnactProposalsOnHold() const
{
	return m_vEnactProposalsOnHold;
}

CvRepealProposal* CvLeague::GetRepealProposal(int iResolutionID)
{
	for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); ++it)
	{
		if (it->GetID() == iResolutionID)
		{
			return it;
		}
	}
	return NULL;
}

RepealProposalList CvLeague::GetRepealProposals() const
{
	return m_vRepealProposals;
}

RepealProposalList CvLeague::GetRepealProposalsOnHold() const
{
	return m_vRepealProposalsOnHold;
}

CvActiveResolution* CvLeague::GetActiveResolution(int iResolutionID)
{
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetID() == iResolutionID)
		{
			return it;
		}
	}
	return NULL;
}

ActiveResolutionList CvLeague::GetActiveResolutions() const
{
	return m_vActiveResolutions;
}

int CvLeague::GetNumResolutionsEverEnacted() const
{
	return m_iNumResolutionsEverEnacted;
}

int CvLeague::GetNumProposersPerSession() const
{
	int iProposers = MIN(LeagueHelpers::PROPOSERS_PER_SESSION, GetNumMembers());
	return iProposers;
}

#ifdef CAN_PROPOSE_ENACT_UPDATES_ONCE_PER_SESSION
void CvLeague::UpdateCanProposeEnact()
{
	for(int iResolutionLoop = 0; iResolutionLoop < GC.getNumResolutionInfos(); iResolutionLoop++)
	{
		CvResolutionEntry* pInfo = GC.getResolutionInfo((ResolutionTypes)iResolutionLoop);
		if (pInfo)
		{
			if (pInfo->GetTechPrereqAnyMember() != NO_TECH)
			{
				for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
				{
					if(GET_PLAYER(it->ePlayer).isAlive())
					{
						if (GET_TEAM(GET_PLAYER(it->ePlayer).getTeam()).GetTeamTechs()->HasTechByHuman(pInfo->GetTechPrereqAnyMember()))
						{
							GET_TEAM(GET_PLAYER(it->ePlayer).getTeam()).GetTeamTechs()->SetHasTechForLeague(pInfo->GetTechPrereqAnyMember(), true);
							break;
						}
					}
				}
			}
		}
	}

	return;
}

#endif
#ifdef ASSIGN_SECOND_PROPOSAL_PRIVILEGE
void CvLeague::AssignSecondProposalPrivilege()
{
	CvWeightedVector<Member*, MAX_CIV_PLAYERS, false> vpPossibleProposers;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		if (CanEverPropose(it->ePlayer))
		{
			int iVotes = CalculateStartingVotesForMember(it->ePlayer);
			vpPossibleProposers.push_back(it, iVotes);
		}
		else
		{
			CvAssertMsg(!it->bMayPropose, "Found a member with proposal rights that should not have them. Please send Anton your save file and version.");
			CvAssertMsg(it->iProposals == 0, "Found a member with remaining proposals that should not have them. Please send Anton your save file and version.");
		}
	}
	vpPossibleProposers.SortItems();

	int iPrivileges = GetNumProposersPerSession() - 1;

	/*// Host gets one
	PlayerTypes eHost = GetHostMember();
	if (eHost != NO_PLAYER)
	{
		GetMember(eHost)->bMayPropose = true;
		GetMember(eHost)->iProposals = GC.getLEAGUE_MEMBER_PROPOSALS_BASE();
		iPrivileges--;
	}*/

	// Give rest to largest delegations
	for (int i = 0; i < vpPossibleProposers.size(); i++)
	{
		if (iPrivileges == 0)
		{
			break;
		}
		
		// Only one privilege per player
		if (!vpPossibleProposers.GetElement(i)->bMayPropose)
		{
			vpPossibleProposers.GetElement(i)->bMayPropose = true;
			vpPossibleProposers.GetElement(i)->iProposals = GC.getLEAGUE_MEMBER_PROPOSALS_BASE();
			iPrivileges--;
		}
	}

	CvAssert(iPrivileges == 0);
}
#endif
void CvLeague::AddMember(PlayerTypes ePlayer)
{
	if (ePlayer < 0 || ePlayer >= MAX_CIV_PLAYERS)
	{
		CvAssertMsg(false, "Trying to add a player to a League who has an invalid Player ID.  Please send Anton your save file and version.");
		return;
	}
	if (IsMember(ePlayer))
	{
		CvAssertMsg(false, "Trying to add a player to a League that they are already a member of.  Please send Anton your save file and verison.");
		return;
	}

	Member member;
	member.ePlayer = ePlayer;
	m_vMembers.push_back(member);

	// Majors meet each other and reveal their capitals
	if (GET_PLAYER(ePlayer).isAlive() && !GET_PLAYER(ePlayer).isMinorCiv())
	{
		for (MemberList::const_iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
		{
			PlayerTypes eOtherPlayer = it->ePlayer;
			if (eOtherPlayer != NO_PLAYER && eOtherPlayer != ePlayer && GET_PLAYER(eOtherPlayer).isAlive() && !GET_PLAYER(eOtherPlayer).isMinorCiv())
			{
				TeamTypes eTeam = GET_PLAYER(ePlayer).getTeam();
				TeamTypes eOtherTeam = GET_PLAYER(eOtherPlayer).getTeam();
				if (!GET_TEAM(eTeam).isHasMet(eOtherTeam))
				{
					GET_TEAM(eTeam).meet(eOtherTeam, /*bSuppressMessages*/ false);
				}

				CvCity* pCapital = GET_PLAYER(ePlayer).getCapitalCity();
				if (pCapital && pCapital->plot() && !pCapital->plot()->isRevealed(eOtherTeam))
				{
					pCapital->plot()->setRevealed(eOtherTeam, true);
				}
				CvCity* pOtherCapital = GET_PLAYER(eOtherPlayer).getCapitalCity();
				if (pOtherCapital && pOtherCapital->plot() && !pOtherCapital->plot()->isRevealed(eTeam))
				{
					pOtherCapital->plot()->setRevealed(eTeam, true);
				}
			}
		}
	}

	// Apply effects of any existing active resolutions
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		it->DoEffects(ePlayer);
	}

	GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true);
}

void CvLeague::RemoveMember(PlayerTypes ePlayer)
{
	if (!IsMember(ePlayer))
	{
		CvAssertMsg(false, "Trying to remove a player from a League that they are not a member of.  Please send Anton your save file and version.");
		return;
	}

	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		if (it->ePlayer == ePlayer)
		{
			m_vMembers.erase(it);
			it--;
		}
	}

	// Remove effects of any existing active resolutions
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		it->RemoveEffects(ePlayer);
	}

	// Check host
	if (IsHostMember(ePlayer))
	{
		AssignNewHost();
	}

	// Check current proposals and active resolutions to see if they are still valid
	CheckProposalsValid();
	CheckResolutionsValid();

	GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true);
}

bool CvLeague::IsMember(PlayerTypes ePlayer) const
{
	for (uint i = 0; i < m_vMembers.size(); i++)
	{
		if (m_vMembers[i].ePlayer == ePlayer)
		{
			return true;
		}
	}
	return false;
}

int CvLeague::GetNumMembers() const
{
	return m_vMembers.size();
}

bool CvLeague::CanVote(PlayerTypes ePlayer)
{
	if (CanEverVote(ePlayer))
	{
		return (GetRemainingVotesForMember(ePlayer) > 0);
	}
	return false;
}

bool CvLeague::CanEverVote(PlayerTypes ePlayer)
{
	CvAssertMsg(ePlayer >= 0, "ePlayer is expected to be non-negative (invalid Index). Please send Anton your save file and version.");
	CvAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index). Please send Anton your save file and version.");

	if (IsMember(ePlayer))
	{
		if (!GET_PLAYER(ePlayer).isMinorCiv())
		{
			return true;
		}
	}
	return false;
}

int CvLeague::GetRemainingVotesForMember(PlayerTypes ePlayer)
{
	if (CanEverVote(ePlayer))
	{
		return (GetMember(ePlayer)->iVotes);
	}
	return 0;
}

int CvLeague::GetSpentVotesForMember(PlayerTypes ePlayer)
{
	int iVotes = 0;

	for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
	{
		iVotes += it->GetVoterDecision()->GetVotesCastByPlayer(ePlayer);
	}
	for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); ++it)
	{
		iVotes += it->GetRepealDecision()->GetVotesCastByPlayer(ePlayer);
	}
	
	if (IsMember(ePlayer))
	{
		iVotes += GetMember(ePlayer)->iAbstainedVotes;
	}

	if (iVotes > 0)
	{
		CvAssert(CanEverVote(ePlayer));
	}

	return iVotes;
}

int CvLeague::GetCoreVotesForMember(PlayerTypes ePlayer)
{
	int iVotes = 0;
	if (CanEverVote(ePlayer))
	{
		LeagueSpecialSessionTypes eGoverningSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
		if (GetCurrentSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
		{
			eGoverningSpecialSession = GetCurrentSpecialSession();
		}
		else if (GetLastSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
		{
			eGoverningSpecialSession = GetLastSpecialSession();
		}

		CvAssert(eGoverningSpecialSession != NO_LEAGUE_SPECIAL_SESSION);
		if (eGoverningSpecialSession == NO_LEAGUE_SPECIAL_SESSION) return GC.getLEAGUE_MEMBER_VOTES_BASE();
		CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eGoverningSpecialSession);
		CvAssert(pInfo != NULL);
		if (pInfo == NULL) return GC.getLEAGUE_MEMBER_VOTES_BASE();
		Member* pMember = GetMember(ePlayer);
		CvAssert(pMember != NULL);
		if (pMember == NULL) return GC.getLEAGUE_MEMBER_VOTES_BASE();

		iVotes += pInfo->GetCivDelegates();
	}
	return iVotes;
}

int CvLeague::CalculateStartingVotesForMember(PlayerTypes ePlayer, bool bForceUpdateSources)
{
#ifdef NO_AI_VOTES
	if (!GET_PLAYER(ePlayer).isHuman() && GC.getGame().isOption("GAMEOPTION_AI_TWEAKS"))
	{
		return 0;
	}
#endif
	int iVotes = 0;
	if (CanEverVote(ePlayer))
	{
		LeagueSpecialSessionTypes eGoverningSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
		if (GetCurrentSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
		{
			eGoverningSpecialSession = GetCurrentSpecialSession();
		}
		else if (GetLastSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
		{
			eGoverningSpecialSession = GetLastSpecialSession();
		}
		
		CvAssert(eGoverningSpecialSession != NO_LEAGUE_SPECIAL_SESSION);
		if (eGoverningSpecialSession == NO_LEAGUE_SPECIAL_SESSION) return GC.getLEAGUE_MEMBER_VOTES_BASE();
		CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eGoverningSpecialSession);
		CvAssert(pInfo != NULL);
		if (pInfo == NULL) return GC.getLEAGUE_MEMBER_VOTES_BASE();
		Member* pMember = GetMember(ePlayer);
		CvAssert(pMember != NULL);
		if (pMember == NULL) return GC.getLEAGUE_MEMBER_VOTES_BASE();

		// Base votes
		int iBaseVotes = GetCoreVotesForMember(ePlayer);
		iVotes += iBaseVotes;

		// Extra votes (ie. leading in previous failed Diplo Victory proposals)
		int iExtraVotes = pMember->iExtraVotes;;
		iVotes += iExtraVotes;

		// Hosting the league
		int iHostVotes = 0;
		if (IsHostMember(ePlayer))
		{
			iHostVotes += pInfo->GetHostDelegates();
		}
		iVotes += iHostVotes;

		// City-State allies
		int iCityStateVotes = 0;
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
		CvPlayerAI& kPlayer(GET_PLAYER(ePlayer));
		int iMaxExtraVotesFromMinors = kPlayer.GetMaxExtraVotesFromMinors();
		int iExtraAutoVotes = 0;
#endif
		for (int i = MAX_MAJOR_CIVS; i < MAX_CIV_PLAYERS; i++)
		{
			PlayerTypes eMinor = (PlayerTypes) i;
			if (GET_PLAYER(eMinor).isAlive() && GET_PLAYER(eMinor).GetMinorCivAI()->IsAllies(ePlayer))
			{
				iCityStateVotes += pInfo->GetCityStateDelegates();
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
				if (iExtraAutoVotes < iMaxExtraVotesFromMinors)
				{
					iExtraAutoVotes++;
				}
#endif
			}
		}
		iVotes += iCityStateVotes;
#if defined POLICY_MAX_EXTRA_VOTES_FROM_MINORS || defined POLICY_EXTRA_VOTES
		int iPolicyVotes = 0;
#endif
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
		iPolicyVotes += iExtraAutoVotes;
		iVotes += iExtraAutoVotes;
#endif
#ifdef POLICY_MAX_EXTRA_VOTES_FROM_MINORS
		iPolicyVotes += kPlayer.GetPolicyExtraVotes();
		iVotes += kPlayer.GetPolicyExtraVotes();
#endif

		// Diplomats after Globalization tech
		int iDiplomatVotes = 0;
		for (int i = 0; i < MAX_MAJOR_CIVS; i++)
		{
			PlayerTypes eMajor = (PlayerTypes) i;
			if (GET_PLAYER(eMajor).isAlive())
			{
				if (GET_PLAYER(ePlayer).GetEspionage()->IsMyDiplomatVisitingThem(eMajor))
				{
					iDiplomatVotes += GET_PLAYER(ePlayer).GetExtraVotesPerDiplomat();
				}
			}
		}
		iVotes += iDiplomatVotes;

		// Wonders
		int iWonderVotes = GET_PLAYER(ePlayer).GetExtraLeagueVotes();
		iVotes += iWonderVotes;

#ifdef VOTES_FOR_MOST_RELIGION_FOLLOWERS
		int iWorldReligionVotes = 0;
		EraTypes eGameEra = LeagueHelpers::GetGameEraForTrigger();
		if (eGameEra > (EraTypes)GC.getInfoTypeForString("ERA_INDUSTRIAL", true))
		{
			CvGameReligions* pReligions = GC.getGame().GetGameReligions();
			int iMostFollowers = 0;
			int iSecondMostFollowers = 0;
			ReligionTypes eFoundedReligion = NO_RELIGION;
			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				PlayerTypes loopPlayer = (PlayerTypes)iI;
				if (GET_PLAYER(loopPlayer).isAlive())
				{
					eFoundedReligion = pReligions->GetFounderBenefitsReligion(loopPlayer);
					if (eFoundedReligion > RELIGION_PANTHEON)
					{
						if (iMostFollowers == iSecondMostFollowers)
						{
							if (pReligions->GetNumFollowers(eFoundedReligion) > iMostFollowers)
							{
								iMostFollowers = pReligions->GetNumFollowers(eFoundedReligion);
							}
						}
						else
						{
							if (pReligions->GetNumFollowers(eFoundedReligion) >= iMostFollowers)
							{
								iSecondMostFollowers = iMostFollowers;
								iMostFollowers = pReligions->GetNumFollowers(eFoundedReligion);
							}
							else if (pReligions->GetNumFollowers(eFoundedReligion) > iSecondMostFollowers)
							{
								iSecondMostFollowers = pReligions->GetNumFollowers(eFoundedReligion);
							}
						}
					}
				}
			}

			eFoundedReligion = pReligions->GetFounderBenefitsReligion(ePlayer);
			if (eFoundedReligion > RELIGION_PANTHEON)
			{
				if (pReligions->GetNumFollowers(eFoundedReligion) == iMostFollowers)
				{
					iWorldReligionVotes = 2;
				}
				else if (pReligions->GetNumFollowers(eFoundedReligion) == iSecondMostFollowers && iMostFollowers > iSecondMostFollowers)
				{
					iWorldReligionVotes = 1;
				}
			}
		}
		iVotes += iWorldReligionVotes;
#else
		// World Religion
		int iWorldReligionVotes = GetExtraVotesForFollowingReligion(ePlayer);
		iVotes += iWorldReligionVotes;
#endif

		// World Ideology
		int iWorldIdeologyVotes = GetExtraVotesForFollowingIdeology(ePlayer);
		iVotes += iWorldIdeologyVotes;

		// Vote Sources - Normally this is only updated when we are not in session
		if (bForceUpdateSources || !IsInSession())
		{
			pMember->sVoteSources = "";
			if (iBaseVotes > 0)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_BASE_VOTES");
				sTemp << iBaseVotes;
				pMember->sVoteSources += sTemp.toUTF8();
			}
			if (iExtraVotes > 0)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_EXTRA_VOTES");
				sTemp << iExtraVotes;
				pMember->sVoteSources += sTemp.toUTF8();
			}
			if (iHostVotes > 0)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_HOST_VOTES");
				sTemp << iHostVotes;
				pMember->sVoteSources += sTemp.toUTF8();
			}
			if (pInfo->GetCityStateDelegates() > 0) // Show even if we have none from this source, to remind players that CS can give Delegates
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_CS_VOTES");
				sTemp << iCityStateVotes;
				sTemp << pInfo->GetCityStateDelegates();
				pMember->sVoteSources += sTemp.toUTF8();
			}
			if (iDiplomatVotes > 0)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_DIPLOMAT_VOTES");
				sTemp << iDiplomatVotes;
				sTemp << GET_PLAYER(ePlayer).GetExtraVotesPerDiplomat();
				pMember->sVoteSources += sTemp.toUTF8();
			}
			if (iWonderVotes > 0)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_WONDER_VOTES");
				sTemp << iWonderVotes;
				pMember->sVoteSources += sTemp.toUTF8();
			}
#if defined POLICY_EXTRA_VOTES || defined POLICY_MAX_EXTRA_VOTES_FROM_MINORS
			if (iPolicyVotes > 0)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_POLICY_VOTES");
				sTemp << iPolicyVotes;
				pMember->sVoteSources += sTemp.toUTF8();
			}
#endif
#ifdef VOTES_FOR_MOST_RELIGION_FOLLOWERS
			if (iWorldReligionVotes > 0)
			{
				if (iWorldReligionVotes == 2)
				{
					Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_MOST_RELIGION_VOTES");
					sTemp << iWorldReligionVotes;
					pMember->sVoteSources += sTemp.toUTF8();
				}
				else
				{
					Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_SECOND_MOST_RELIGION_VOTES");
					sTemp << iWorldReligionVotes;
					pMember->sVoteSources += sTemp.toUTF8();
				}
			}
#else
			if (iWorldReligionVotes > 0)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_WORLD_RELIGION_VOTES");
				sTemp << iWorldReligionVotes;
				pMember->sVoteSources += sTemp.toUTF8();
			}
#endif
			if (iWorldIdeologyVotes > 0)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS_WORLD_IDEOLOGY_VOTES");
				sTemp << iWorldIdeologyVotes;
				pMember->sVoteSources += sTemp.toUTF8();
			}
		}
	}
	return iVotes;
}

bool CvLeague::CanPropose(PlayerTypes ePlayer)
{
	if (CanEverPropose(ePlayer))
	{
		if (GetMember(ePlayer)->bMayPropose)
		{
			return (GetRemainingProposalsForMember(ePlayer) > 0);
		}
	}
	return false;
}

bool CvLeague::CanEverPropose(PlayerTypes ePlayer)
{
	CvAssertMsg(ePlayer >= 0, "ePlayer is expected to be non-negative (invalid Index). Please send Anton your save file and version.");
	CvAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index). Please send Anton your save file and version.");

	if (IsMember(ePlayer))
	{
		if (!GET_PLAYER(ePlayer).isMinorCiv())
		{
			return true;
		}
	}
	return false;
}

int CvLeague::GetRemainingProposalsForMember(PlayerTypes ePlayer)
{
	int iRemaining = 0;
	if (IsMember(ePlayer))
	{
		iRemaining += GetMember(ePlayer)->iProposals;
	}
	return iRemaining;
}

int CvLeague::GetNumProposalsByMember(PlayerTypes ePlayer)
{
	int iValue = 0;
	if (IsMember(ePlayer))
	{
		for (uint i = 0; i < m_vEnactProposals.size(); i++)
		{
			if (m_vEnactProposals[i].GetProposalPlayer() == ePlayer)
			{
				iValue++;
			}
		}
		for (uint i = 0; i < m_vRepealProposals.size(); i++)
		{
			if (m_vRepealProposals[i].GetProposalPlayer() == ePlayer)
			{
				iValue++;
			}
		}

		// Also check proposals on hold
		for (uint i = 0; i < m_vEnactProposalsOnHold.size(); i++)
		{
			if (m_vEnactProposalsOnHold[i].GetProposalPlayer() == ePlayer)
			{
				iValue++;
			}
		}
		for (uint i = 0; i < m_vRepealProposalsOnHold.size(); i++)
		{
			if (m_vRepealProposalsOnHold[i].GetProposalPlayer() == ePlayer)
			{
				iValue++;
			}
		}
	}
	return iValue;
}

LeagueHelpers::PlayerList CvLeague::GetMembersThatLikeProposal(ResolutionTypes eResolution, PlayerTypes eObserver, int iProposerChoice)
{
	LeagueHelpers::PlayerList vMembersSupporting;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
	{
		if (it->ePlayer != eObserver && CanEverVote(it->ePlayer) && !GET_PLAYER(it->ePlayer).isHuman())
		{
			CvLeagueAI::DesireLevels eDesire = GET_PLAYER(it->ePlayer).GetLeagueAI()->EvaluateProposalForProposer(this, eObserver, eResolution, iProposerChoice);
			if (eDesire > CvLeagueAI::DESIRE_WEAK_LIKE)
			{
				vMembersSupporting.push_back(it->ePlayer);
			}
		}
	}
	return vMembersSupporting;
}

LeagueHelpers::PlayerList CvLeague::GetMembersThatLikeProposal(int iTargetResolutionID, PlayerTypes eObserver)
{
	LeagueHelpers::PlayerList vMembersSupporting;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
	{
		if (it->ePlayer != eObserver && CanEverVote(it->ePlayer) && !GET_PLAYER(it->ePlayer).isHuman())
		{
			CvLeagueAI::DesireLevels eDesire = GET_PLAYER(it->ePlayer).GetLeagueAI()->EvaluateProposalForProposer(this, eObserver, iTargetResolutionID);
			if (eDesire > CvLeagueAI::DESIRE_WEAK_LIKE)
			{
				vMembersSupporting.push_back(it->ePlayer);
			}
		}
	}
	return vMembersSupporting;
}

LeagueHelpers::PlayerList CvLeague::GetMembersThatDislikeProposal(ResolutionTypes eResolution, PlayerTypes eObserver, int iProposerChoice)
{
	LeagueHelpers::PlayerList vMembersDissuading;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
	{
		if (it->ePlayer != eObserver && CanEverVote(it->ePlayer) && !GET_PLAYER(it->ePlayer).isHuman())
		{
			CvLeagueAI::DesireLevels eDesire = GET_PLAYER(it->ePlayer).GetLeagueAI()->EvaluateProposalForProposer(this, eObserver, eResolution, iProposerChoice);
			if (eDesire < CvLeagueAI::DESIRE_WEAK_DISLIKE)
			{
				vMembersDissuading.push_back(it->ePlayer);
			}
		}
	}
	return vMembersDissuading;
}

LeagueHelpers::PlayerList CvLeague::GetMembersThatDislikeProposal(int iTargetResolutionID, PlayerTypes eObserver)
{
	LeagueHelpers::PlayerList vMembersDissuading;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
	{
		if (it->ePlayer != eObserver && CanEverVote(it->ePlayer))
		{
			CvLeagueAI::DesireLevels eDesire = GET_PLAYER(it->ePlayer).GetLeagueAI()->EvaluateProposalForProposer(this, eObserver, iTargetResolutionID);
			if (eDesire < CvLeagueAI::DESIRE_WEAK_DISLIKE)
			{
				vMembersDissuading.push_back(it->ePlayer);
			}
		}
	}
	return vMembersDissuading;
}

bool CvLeague::HasHostMember() const
{
	return (m_eHost != NO_PLAYER);
}

bool CvLeague::IsHostMember(PlayerTypes ePlayer) const
{
	return (m_eHost == ePlayer);
}

PlayerTypes CvLeague::GetHostMember() const
{
	return m_eHost;
}

void CvLeague::SetHostMember(PlayerTypes ePlayer)
{
	CvAssertMsg(ePlayer >= 0 && ePlayer < MAX_CIV_PLAYERS, "ePlayer is out of bounds (invalid Index)");
	bool bFound = false;
	if (ePlayer >= 0 && ePlayer < MAX_CIV_PLAYERS)
	{
		for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
		{
			if (ePlayer == it->ePlayer)
			{
				bFound = true;
				if (ePlayer != GetHostMember())
				{
					m_eHost = ePlayer;
					it->bEverBeenHost = true;
					UpdateName();
#ifdef REPLAY_EVENTS
					std::vector<int> vArgs;
					GC.getGame().addReplayEvent(REPLAYEVENT_CongressHostChange, ePlayer, vArgs);
#endif
				}
			}
			else
			{
				it->bAlwaysBeenHost = false;
			}
		}
	}
	CvAssertMsg(bFound, "Could not find the correct player to set as the World Congress host. Please send Anton your save file and version.");
}

int CvLeague::GetConsecutiveHostedSessions() const
{
	return m_iConsecutiveHostedSessions;
}

void CvLeague::SetConsecutiveHostedSessions(int iValue)
{
	m_iConsecutiveHostedSessions = iValue;
}

void CvLeague::ChangeConsecutiveHostedSessions(int iChange)
{
	SetConsecutiveHostedSessions(GetConsecutiveHostedSessions() + iChange);
}

bool CvLeague::HasMemberEverBeenHost(PlayerTypes ePlayer)
{
	bool b = false;
	if (IsMember(ePlayer))
	{
		b = GetMember(ePlayer)->bEverBeenHost;
	}
	return b;
}

bool CvLeague::HasMemberAlwaysBeenHost(PlayerTypes ePlayer)
{
	bool b = false;
	if (IsMember(ePlayer))
	{
		b = GetMember(ePlayer)->bAlwaysBeenHost;
	}
	return b;
}

// Is the project underway, but not yet completed?
bool CvLeague::IsProjectActive(LeagueProjectTypes eLeagueProject) const
{
	for (ProjectList::const_iterator it = m_vProjects.begin(); it != m_vProjects.end(); it++)
	{
		if (it->eType == eLeagueProject)
		{
			if (!it->bComplete)
			{
				return true;
			}
		}
	}
	return false;
}

// Is the project completed?
bool CvLeague::IsProjectComplete(LeagueProjectTypes eLeagueProject) const
{
	for (ProjectList::const_iterator it = m_vProjects.begin(); it != m_vProjects.end(); it++)
	{
		if (it->eType == eLeagueProject)
		{
			if (it->bComplete)
			{
				return true;
			}
		}
	}
	return false;
}

int CvLeague::GetProjectCostPerPlayer(LeagueProjectTypes eLeagueProject) const
{
	CvLeagueProjectEntry* pProjectInfo = GC.getLeagueProjectInfo(eLeagueProject);
	CvAssertMsg(pProjectInfo != NULL, "Looking up project cost for a project that does not exist. Please send Anton your save file and version.");
	int iCost = 0;
	if (pProjectInfo)
	{
		iCost += pProjectInfo->GetCostPerPlayer() * 100;

		iCost *= GC.getGame().getGameSpeedInfo().getConstructPercent();
		iCost /= 100;
#ifdef PROJECT_COST_PER_PLAYER_ROUND_TO_5
		iCost /= 500;
		iCost *= 500;
#endif

		iCost *= GC.getGame().getStartEraInfo().getConstructPercent();
		iCost /= 100;
	}
	return iCost;
}

// Helper function to lookup cost per player by a BuildingType that is part of the project rewards.
// Assumes that this BuildingType is a reward for at most 1 project.
int CvLeague::GetProjectBuildingCostPerPlayer(BuildingTypes eRewardBuilding) const
{
	int iCost = 0;

	// Is it part of an international project?
	for (int i = 0; i < GC.getNumLeagueProjectInfos(); i++)
	{
		LeagueProjectTypes eProject = (LeagueProjectTypes)i;
		CvLeagueProjectEntry* pProjectInfo = GC.getLeagueProjectInfo(eProject);
		if (pProjectInfo != NULL && pProjectInfo->GetRewardTier3() != NO_LEAGUE_PROJECT_REWARD) // Only check top reward tier
		{
			CvLeagueProjectRewardEntry* pRewardInfo = GC.getLeagueProjectRewardInfo(pProjectInfo->GetRewardTier3());
			if (pRewardInfo != NULL && pRewardInfo->GetBuilding() == eRewardBuilding)
			{
				iCost = GetProjectCostPerPlayer(eProject) / 100;
				break;
			}
		}
	}

	return iCost;
}

int CvLeague::GetProjectCost(LeagueProjectTypes eLeagueProject) const
{
	CvLeagueProjectEntry* pProjectInfo = GC.getLeagueProjectInfo(eLeagueProject);
	CvAssertMsg(pProjectInfo != NULL, "Looking up project cost for a project that does not exist. Please send Anton your save file and version.");
	int iCost = 0;
	if (pProjectInfo)
	{
		iCost += GetProjectCostPerPlayer(eLeagueProject) * GC.getGame().countMajorCivsEverAlive();
	}
	return iCost;
}

int CvLeague::GetProjectProgress(LeagueProjectTypes eProject)
{
	CvLeagueProjectEntry* pProjectInfo = GC.getLeagueProjectInfo(eProject);
	CvAssertMsg(pProjectInfo != NULL, "Looking up project progress for a project that does not exist. Please send Anton your save file and version.");
	int iProgress = 0;
	if (pProjectInfo)
	{
		if (IsProjectActive(eProject) || IsProjectComplete(eProject))
		{
			Project* pProject = GetProject(eProject);
			CvAssert(pProject != NULL)
			if (pProject)
			{
				for (uint i = 0; i < pProject->vProductionList.size(); i++)
				{
					iProgress += pProject->vProductionList[i];
				}
			}
		}
	}
	return iProgress;
}

bool CvLeague::CanMemberContribute(PlayerTypes ePlayer, LeagueProjectTypes eLeagueProject) const
{
	if (IsMember(ePlayer))
	{
		if (!GET_PLAYER(ePlayer).isMinorCiv())
		{
			if (IsProjectActive(eLeagueProject))
			{
				return true;
			}
		}
	}
	return false;
}

int CvLeague::GetMemberContribution(PlayerTypes ePlayer, LeagueProjectTypes eLeagueProject) const
{
	int iValue = 0;
	if (ePlayer >= 0 && ePlayer < MAX_MAJOR_CIVS)
	{
		for (ProjectList::const_iterator it = m_vProjects.begin(); it != m_vProjects.end(); it++)
		{
			if (it->eType == eLeagueProject)
			{
				iValue += it->vProductionList[ePlayer];
				break;
			}
		}
	}
	return iValue;
}

void CvLeague::SetMemberContribution(PlayerTypes ePlayer, LeagueProjectTypes eLeagueProject, int iValue)
{
	bool bCanContribute = CanMemberContribute(ePlayer, eLeagueProject);
	CvAssertMsg(bCanContribute, "Attempting to make a contribution to a League Project when not allowed. Please send Anton your save file and version.");
	if (!bCanContribute) return;

	int iMatches = 0;
	for (ProjectList::iterator it = m_vProjects.begin(); it != m_vProjects.end(); it++)
	{
		if (it->eType == eLeagueProject)
		{
			iMatches++;
			it->vProductionList[ePlayer] = iValue;
		}
	}
	CvAssertMsg(iMatches == 1, "Unexpected case when contributing to a League Project. Please send Anton your save file and version.");
}

void CvLeague::ChangeMemberContribution(PlayerTypes ePlayer, LeagueProjectTypes eLeagueProject, int iChange)
{
	bool bCanContribute = CanMemberContribute(ePlayer, eLeagueProject);
	CvAssertMsg(bCanContribute, "Attempting to make a contribution to a League Project when not allowed. Please send Anton your save file and version.");
	if (!bCanContribute) return;

	SetMemberContribution(ePlayer, eLeagueProject, GetMemberContribution(ePlayer, eLeagueProject) + iChange);
}

CvLeague::ContributionTier CvLeague::GetMemberContributionTier(PlayerTypes ePlayer, LeagueProjectTypes eLeagueProject)
{
	float fContribution = (float) GetMemberContribution(ePlayer, eLeagueProject);
	ContributionTier eTier = CONTRIBUTION_TIER_0;
	if (fContribution >= GetContributionTierThreshold(CONTRIBUTION_TIER_3, eLeagueProject))
	{
		eTier = CONTRIBUTION_TIER_3;
	}
	else if (fContribution >= GetContributionTierThreshold(CONTRIBUTION_TIER_2, eLeagueProject))
	{
		eTier = CONTRIBUTION_TIER_2;
	}
	else if (fContribution >= GetContributionTierThreshold(CONTRIBUTION_TIER_1, eLeagueProject))
	{
		eTier = CONTRIBUTION_TIER_1;
	}
	return eTier;
}

// Get the contribution value which a player needs to meet or exceed to qualify for a reward tier level
float CvLeague::GetContributionTierThreshold(ContributionTier eTier, LeagueProjectTypes eLeagueProject)
{
	float fThreshold = 0.0f;
	Project* pProject = GetProject(eLeagueProject);
	CvAssertMsg(pProject, "Could not find league project. Please send Anton your save file and version.");
	if (!pProject) return 0.0f;

	switch (eTier)
	{
	case CONTRIBUTION_TIER_1:
		{
			fThreshold = GC.getLEAGUE_PROJECT_REWARD_TIER_1_THRESHOLD() * GetProjectCostPerPlayer(eLeagueProject);
			break;
		}
	case CONTRIBUTION_TIER_2:
		{
			fThreshold = GC.getLEAGUE_PROJECT_REWARD_TIER_2_THRESHOLD() * GetProjectCostPerPlayer(eLeagueProject);
			break;
		}
	case CONTRIBUTION_TIER_3:
		{
			int iBestContribution = 0;
			for (uint i = 0; i < pProject->vProductionList.size(); i++)
			{
				int iContribution = pProject->vProductionList[i];
				if (iContribution > iBestContribution && GET_PLAYER((PlayerTypes)i).isAlive() && IsMember((PlayerTypes)i))
				{
					iBestContribution = iContribution;
				}
			}
#ifdef LEAGUE_PROJECT_REWARD_TIER_3_THRESHOLD
			fThreshold = MAX((float)iBestContribution, GC.getLEAGUE_PROJECT_REWARD_TIER_2_THRESHOLD() * GetProjectCostPerPlayer(eLeagueProject) * 8 / 5);
#else
			fThreshold = MAX((float)iBestContribution, GC.getLEAGUE_PROJECT_REWARD_TIER_2_THRESHOLD() * GetProjectCostPerPlayer(eLeagueProject));
#endif
			break;
		}
	default:
		break;
	}
	return fThreshold;
}

bool CvLeague::IsTradeEmbargoed(PlayerTypes eTrader, PlayerTypes eRecipient)
{
	CvAssertMsg(eTrader >= 0 && eTrader < MAX_CIV_PLAYERS, "Invalid index for eTrader. Please send Anton your save file and version.");
	if (eTrader < 0 || eTrader >= MAX_CIV_PLAYERS) return false;
	CvAssertMsg(eRecipient >= 0 && eRecipient < MAX_CIV_PLAYERS, "Invalid index for eRecipient. Please send Anton your save file and version.");
	if (eRecipient < 0 || eRecipient >= MAX_CIV_PLAYERS) return false;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		// Trade route involving a minor civ
		if (GET_PLAYER(eTrader).isMinorCiv() || GET_PLAYER(eRecipient).isMinorCiv())
		{
			if (it->GetEffects()->bEmbargoCityStates)
			{
				return true;
			}
		}
		// Trade route between two majors
		else
		{			
			if (it->GetEffects()->bEmbargoPlayer)
			{
				PlayerTypes eEmbargoedMajor = (PlayerTypes) it->GetProposerDecision()->GetDecision();
				CvAssert(eEmbargoedMajor != NO_PLAYER);
				if (eEmbargoedMajor == eTrader || eEmbargoedMajor == eRecipient)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool CvLeague::IsLuxuryHappinessBanned(ResourceTypes eResource)
{
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		if (it->GetEffects()->bNoResourceHappiness && it->GetProposerDecision()->GetDecision() == eResource)
		{
			return true;
		}
	}
	return false;
}

int CvLeague::GetResearchMod(TechTypes eTech)
{
	int iValue = 0;

	int iKnownByMemberMod = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		if (it->GetEffects()->iMemberDiscoveredTechMod != 0)
		{
			iKnownByMemberMod += it->GetEffects()->iMemberDiscoveredTechMod;
		}
	}
#ifdef NEW_RESOLUTION_MEMBER_DISCOVERED_TECH_DISCOUNT
	if (iKnownByMemberMod != 0)
	{
		int iNumHasTech = 0;
		int iNumMembers = 0;
		for (uint i = 0; i < m_vMembers.size(); i++)
		{
#ifdef HAS_TECH_BY_HUMAN
			PlayerTypes eMember = m_vMembers[i].ePlayer;
			if (GC.getGame().isNetworkMultiPlayer() && GET_PLAYER(eMember).isHuman())
			{
				iNumMembers++;
			}
			else
			{
				iNumMembers = m_vMembers.size();
			}
			if (GET_TEAM(GET_PLAYER(eMember).getTeam()).GetTeamTechs()->HasTechByHuman(eTech))
#else
			iNumMembers = m_vMembers.size();
			if (GET_TEAM(GET_PLAYER(eMember).getTeam()).GetTeamTechs()->HasTech(eTech))
#endif
			{
				iNumHasTech++;
			}
		}
		if (2 * iNumHasTech >= iNumMembers)
		{
			iValue += iKnownByMemberMod;
		}
	}
#else
	if (iKnownByMemberMod != 0)
	{
		// Does any member have this tech?
		for (uint i = 0; i < m_vMembers.size(); i++)
		{
			PlayerTypes eMember = m_vMembers[i].ePlayer;
			if (GET_TEAM(GET_PLAYER(eMember).getTeam()).GetTeamTechs()->HasTech(eTech))
			{
				iValue += iKnownByMemberMod;
				break;
			}
		}
	}
#endif

	return iValue;
}

int CvLeague::GetFeatureYieldChange(FeatureTypes eFeature, YieldTypes eYield)
{
	CvAssertMsg(eFeature != NO_FEATURE, "Looking for yield changes to NO_FEATURE. Please send Anton your save file and version.");
	int iValue = 0;

	CvFeatureInfo* pInfo = GC.getFeatureInfo(eFeature);
	if (pInfo)
	{
		// Natural Wonders
		if (pInfo->IsNaturalWonder())
		{
			int iNaturalWonderMod = 0;
			if (eYield == YIELD_CULTURE)
			{
				for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
				{
					if (it->GetEffects()->iCulturePerNaturalWonder != 0)
					{
						iNaturalWonderMod += it->GetEffects()->iCulturePerNaturalWonder;
					}
				}
			}
			iValue += iNaturalWonderMod;
		}
	}

	return iValue;
}

int CvLeague::GetWorldWonderYieldChange(YieldTypes eYield)
{
	int iValue = 0;

	if (eYield == YIELD_CULTURE)
	{
		for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
		{
			if (it->GetEffects()->iCulturePerWonder != 0)
			{
				iValue += it->GetEffects()->iCulturePerWonder;
			}
		}
	}

	return iValue;
}

bool CvLeague::IsNoTrainingNuclearWeapons()
{
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		if (it->GetEffects()->bNoTrainingNuclearWeapons)
		{
			return true;
		}
	}
	return false;
}

int CvLeague::GetExtraVotesForFollowingReligion(PlayerTypes ePlayer)
{
	int iVotes = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetEffects()->iVotesForFollowingReligion != 0)
		{
			ReligionTypes eReligion = (ReligionTypes) it->GetProposerDecision()->GetDecision();
			CvAssert(eReligion != NO_RELIGION);
			if (GET_PLAYER(ePlayer).GetReligions()->HasReligionInMostCities(eReligion))
			{
				iVotes += it->GetEffects()->iVotesForFollowingReligion;
			}
		}
	}
	return iVotes;
}

int CvLeague::GetCityTourismModifier(const CvCity* pCity)
{
	int iMod = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		// Holy City tourism
		if (it->GetEffects()->iHolyCityTourism != 0)
		{
			ReligionTypes eReligion = (ReligionTypes) it->GetProposerDecision()->GetDecision();
			CvAssert(eReligion != NO_RELIGION);
			if (pCity->GetCityReligions()->IsHolyCityForReligion(eReligion))
			{
				iMod += it->GetEffects()->iHolyCityTourism;
			}
		}
	}
	return iMod;
}

int CvLeague::GetReligionSpreadStrengthModifier(ReligionTypes eReligion)
{
	int iMod = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetEffects()->iReligionSpreadStrengthMod != 0)
		{
			ReligionTypes e = (ReligionTypes) it->GetProposerDecision()->GetDecision();
			CvAssert(e != NO_RELIGION);
			if (e != NO_RELIGION && e == eReligion)
			{
				iMod += it->GetEffects()->iReligionSpreadStrengthMod;
			}
		}
	}
	return iMod;
}

int CvLeague::GetExtraVotesForFollowingIdeology(PlayerTypes ePlayer)
{
	int iVotes = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetEffects()->iVotesForFollowingIdeology != 0)
		{
			PolicyBranchTypes eIdeology = (PolicyBranchTypes) it->GetProposerDecision()->GetDecision();
			CvAssert(eIdeology != NO_POLICY_BRANCH_TYPE);
			PolicyBranchTypes ePlayerIdeology = GET_PLAYER(ePlayer).GetPlayerPolicies()->GetLateGamePolicyTree();
			if (ePlayerIdeology != NO_POLICY_BRANCH_TYPE && eIdeology != NO_POLICY_BRANCH_TYPE && ePlayerIdeology == eIdeology)
			{
				iVotes += it->GetEffects()->iVotesForFollowingIdeology;
			}
		}
	}
	return iVotes;
}

int CvLeague::GetPressureForIdeology(PolicyBranchTypes eIdeology)
{
	int iPressure = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetEffects()->iOtherIdeologyRebellionMod != 0)
		{
			PolicyBranchTypes e = (PolicyBranchTypes) it->GetProposerDecision()->GetDecision();
			CvAssert(e != NO_POLICY_BRANCH_TYPE);
			if (e != NO_POLICY_BRANCH_TYPE && eIdeology != NO_POLICY_BRANCH_TYPE && e == eIdeology)
			{
				iPressure += it->GetEffects()->iOtherIdeologyRebellionMod;
			}
		}
	}
	return iPressure;
}

int CvLeague::GetArtsyGreatPersonRateModifier()
{
	int iMod = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetEffects()->iArtsyGreatPersonRateMod != 0)
		{
			iMod += it->GetEffects()->iArtsyGreatPersonRateMod;
		}
	}
	return iMod;
}

int CvLeague::GetScienceyGreatPersonRateModifier()
{
	int iMod = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetEffects()->iScienceyGreatPersonRateMod != 0)
		{
			iMod += it->GetEffects()->iScienceyGreatPersonRateMod;
		}
	}
	return iMod;
}

#ifdef NEW_LEAGUE_RESOLUTIONS
int CvLeague::GetTradeRouteGoldModifier()
{
	int iMod = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetEffects()->iTradeRouteGoldModifier != 0)
		{
			iMod += it->GetEffects()->iTradeRouteGoldModifier;
		}
	}
	return iMod;
}

int CvLeague::GetCSBonuModifier()
{
	int iMod = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetEffects()->iCSBonuModifier != 0)
		{
			iMod += it->GetEffects()->iCSBonuModifier;
		}
	}
	return iMod;
}

bool CvLeague::IsNoSpiesInCS()
{
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		if (it->GetEffects()->bNoSpiesInCS)
		{
			return true;
		}
	}
	return false;
}

bool CvLeague::IsDoubleResourceHappiness(ResourceTypes eResource)
{
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		if (it->GetEffects()->bDoubleResourceHappiness && it->GetProposerDecision()->GetDecision() == eResource)
		{
			return true;
		}
	}
	return false;
}
#endif

CvString CvLeague::GetResolutionName(ResolutionTypes eResolution, int iResolutionID, int iProposerChoice, bool bIncludePrefix)
{
	CvString s = "";

	CvResolutionEntry* pInfo = GC.getResolutionInfo(eResolution);
	CvAssertMsg(pInfo, "Resolution info not found when assembling name text. Please send Anton your save file and version.");
	if (!pInfo)
	{
		CvAssert(false);
		return "";
	}

	CvString sPrefix = "";
	// An existing resolution
	if (iResolutionID != -1)
	{
		// Propose Enact
		if (IsProposed(iResolutionID, /*bRepeal*/false))
		{
			sPrefix += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PREFIX_ENACT").toUTF8();
		}
		// Propose Repeal
		else if (IsProposed(iResolutionID, /*bRepeal*/true))
		{
			sPrefix += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PREFIX_REPEAL").toUTF8();
		}
		// Active
		else if (GetActiveResolution(iResolutionID) != NULL)
		{
		}
	}
	// A new resolution
	else
	{
		// Inactive
	}

	CvString sSuffix = "";
	if (iProposerChoice != LeagueHelpers::CHOICE_NONE)
	{
		sSuffix += ": ";
		sSuffix += LeagueHelpers::GetTextForChoice(pInfo->GetProposerDecision(), iProposerChoice);
	}

	if (bIncludePrefix)
	{
		s += sPrefix;
	}
	s += Localization::Lookup(pInfo->GetDescriptionKey()).toUTF8();
	s += sSuffix;

	return s;
}

CvString CvLeague::GetResolutionDetails(ResolutionTypes eResolution, PlayerTypes eObserver, int iResolutionID, int iProposerChoice)
{
	CvString s = "";
	
	CvResolutionEntry* pInfo = GC.getResolutionInfo(eResolution);
	CvAssertMsg(pInfo, "Resolution info not found when assembling details text. Please send Anton your save file and version.");
	if (!pInfo)
	{
		CvAssert(false);
		return "";
	}

	// Resolution type info
	s += Localization::Lookup(pInfo->GetHelp()).toUTF8();

	// Refers to an existing resolution
	if (iResolutionID != -1)
	{
		CvString sDisabled = "";

		// Would vote to enact or repeal
		if (IsProposed(iResolutionID, /*bRepeal*/false) || IsProposed(iResolutionID, /*bRepeal*/true))
		{
			s += GetResolutionVoteOpinionDetails(eResolution, eObserver, iResolutionID);
		}
		// It's on hold
		else if (IsProposed(iResolutionID, /*bRepeal*/false, /*bCheckOnHold*/true) || IsProposed(iResolutionID, /*bRepeal*/true, /*bCheckOnHold*/true))
		{			
		}
		// Would propose repeal
		else if (CanProposeRepeal(iResolutionID, eObserver, &sDisabled))
		{
			s += GetResolutionProposeOpinionDetails(iResolutionID, eObserver);
		}
		else
		{
			s += sDisabled;
		}
	}
	// A new resolution
	else
	{
		// Check our choice
		CvString sDisabled = "";
		if (CanProposeEnact(eResolution, eObserver, iProposerChoice, &sDisabled))
		{
			s += GetResolutionProposeOpinionDetails(eResolution, eObserver, iProposerChoice);
		}
		else
		{
			s += sDisabled;
		}

		// Additional tooltip if there are no valid choices
		if (strcmp(sDisabled, "") == 0)
		{
			if (pInfo->GetProposerDecision() != RESOLUTION_DECISION_NONE && !CanProposeEnactAnyChoice(eResolution, eObserver))
			{
				s += "[NEWLINE][NEWLINE][COLOR_WARNING_TEXT]";
				s += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_INVALID_RESOLUTION_NO_CHOICES").toUTF8();
				s += "[ENDCOLOR]";
			}
		}
	}

	return s;
}

CvString CvLeague::GetResolutionVoteOpinionDetails(ResolutionTypes eResolution, PlayerTypes eObserver, int iResolutionID)
{
	CvResolutionEntry* pInfo = GC.getResolutionInfo(eResolution);
	if (!pInfo)
	{
		return "";
	}
	// Must be proposed as either enact or repeal, but not both
	if (!IsProposed(iResolutionID, /*bRepeal*/false) && !IsProposed(iResolutionID, /*bRepeal*/true))
	{
		CvAssert(false);
		return "";
	}
	if (IsProposed(iResolutionID, /*bRepeal*/false) && IsProposed(iResolutionID, /*bRepeal*/true))
	{
		CvAssert(false);
		return "";
	}
	
	CvString s = "";
	s += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_VOTE_OPINIONS").toUTF8();

	bool bEnact = IsProposed(iResolutionID, /*bRepeal*/false);
	ResolutionDecisionTypes eDecision = pInfo->GetVoterDecision();
	if (!bEnact)
	{
		eDecision = RESOLUTION_DECISION_REPEAL;
	}
	std::vector<int> vChoices = GetChoicesForDecision(eDecision, NO_PLAYER);

	// Discover what choices we can of the other players
	FStaticVector< std::pair<PlayerTypes, int>, MAX_MAJOR_CIVS, true, c_eCiv5GameplayDLL> vMemberOpinions;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
	{
		if (it->ePlayer != eObserver && CanEverVote(it->ePlayer))
		{
			int iMemberChoice = LeagueHelpers::CHOICE_NONE;
			if (bEnact)
			{
				iMemberChoice = GET_PLAYER(it->ePlayer).GetLeagueAI()->EvaluateVoteForOtherPlayerKnowledge(this, eObserver, GetEnactProposal(iResolutionID));
			}
			else
			{
				iMemberChoice = GET_PLAYER(it->ePlayer).GetLeagueAI()->EvaluateVoteForOtherPlayerKnowledge(this, eObserver, GetRepealProposal(iResolutionID));
			}
			std::pair<PlayerTypes, int> pair = std::pair<PlayerTypes, int>(it->ePlayer, iMemberChoice);
			vMemberOpinions.push_back(pair);
		}
	}

	std::vector<LeagueHelpers::VoteOpinionIntrigueElement> vChoiceCommitments;
	std::vector<LeagueHelpers::VoteOpinionIntrigueElement> vChoiceLeanings;
	for (uint iChoiceIndex = 0; iChoiceIndex < vChoices.size(); iChoiceIndex++)
	{
		int iChoice = vChoices[iChoiceIndex];
		int iNumCivsCommitted = 0;
		int iNumCivsLeaning = 0;
		int iNumDelegatesCommitted = 0;
		int iNumDelegatesLeaning = 0;

		for (uint iMemberIndex = 0; iMemberIndex < vMemberOpinions.size(); iMemberIndex++)
		{
			PlayerTypes eMember = vMemberOpinions[iMemberIndex].first;
			int iMemberChoice = vMemberOpinions[iMemberIndex].second;
			int iMemberDelegates = CalculateStartingVotesForMember(eMember);

			// Vote Commitment to us
			int iCommitted = GET_PLAYER(eMember).GetLeagueAI()->GetVoteCommitment(eObserver, iResolutionID, iChoice, bEnact); //here
			if (iCommitted > 0)
			{
				iNumCivsCommitted++;
				iNumDelegatesCommitted += iCommitted;
				iMemberDelegates -= iCommitted;
				CvAssert(iMemberDelegates >= 0);
			}

			// Known leaning
			if (iMemberChoice == iChoice)
			{
				if (iMemberDelegates > 0)
				{
					iNumCivsLeaning++;
					iNumDelegatesLeaning += iMemberDelegates;
				}
			}
		}

		if (iNumCivsCommitted > 0)
		{
			LeagueHelpers::VoteOpinionIntrigueElement temp;
			temp.iChoice = iChoice;
			temp.iNumCivs = iNumCivsCommitted;
			temp.iNumDelegates = iNumDelegatesCommitted;
			vChoiceCommitments.push_back(temp);
		}

		if (iNumCivsLeaning > 0)
		{
			LeagueHelpers::VoteOpinionIntrigueElement temp;
			temp.iChoice = iChoice;
			temp.iNumCivs = iNumCivsLeaning;
			temp.iNumDelegates = iNumDelegatesLeaning;
			vChoiceLeanings.push_back(temp);
		}
	}

	// Votes committed to us (show all of them - don't address if a player makes ones for different choices since that's weird)
	if (!vChoiceCommitments.empty())
	{
		std::stable_sort(vChoiceCommitments.begin(), vChoiceCommitments.end(), LeagueHelpers::VoteOpinionIntrigueSorter());
		for (uint i = 0; i < vChoiceCommitments.size(); i++)
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_VOTE_OPINIONS_COMMITMENTS");
			sTemp << vChoiceCommitments[i].iNumDelegates << vChoiceCommitments[i].iNumCivs << LeagueHelpers::GetTextForChoice(eDecision, vChoiceCommitments[i].iChoice);
			s += sTemp.toUTF8();
		}
	}

	// Known leanings (but only show the top ones, lest the tooltip get cut off)
	int iNumLeaningsShown = 0;
	int iNumLeaningsToShow = 11;
	if (!vChoiceLeanings.empty())
	{
		std::stable_sort(vChoiceLeanings.begin(), vChoiceLeanings.end(), LeagueHelpers::VoteOpinionIntrigueSorter());
		for (uint i = 0; i < vChoiceLeanings.size(); i++)
		{
			if (iNumLeaningsShown <= iNumLeaningsToShow)
			{
				iNumLeaningsShown++;
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_VOTE_OPINIONS_CHOICE");
				sTemp << vChoiceLeanings[i].iNumDelegates << vChoiceLeanings[i].iNumCivs << LeagueHelpers::GetTextForChoice(eDecision, vChoiceLeanings[i].iChoice);
				s += sTemp.toUTF8();
			}
			else
			{
				break;
			}
		}
	}

	// Unknown leanings
	int iNumCivsUnaccounted = 0;
	int iNumDelegatesUnaccounted = 0;
	for (uint iMemberIndex = 0; iMemberIndex < vMemberOpinions.size(); iMemberIndex++)
	{
		PlayerTypes eMember = vMemberOpinions[iMemberIndex].first;
		int iMemberChoice = vMemberOpinions[iMemberIndex].second;
		int iMemberDelegates = CalculateStartingVotesForMember(eMember);

		// Vote Commitment to us
		for (uint i = 0; i < vChoices.size(); i++)
		{
			int iCommitted = GET_PLAYER(eMember).GetLeagueAI()->GetVoteCommitment(eObserver, iResolutionID, vChoices[i], bEnact);
			if (iCommitted > 0)
			{
				iMemberDelegates -= iCommitted;
				CvAssert(iMemberDelegates >= 0);
			}
		}

		// Unknown leaning
		if (iMemberChoice == LeagueHelpers::CHOICE_NONE)
		{
			if (iMemberDelegates > 0)
			{
				iNumCivsUnaccounted++;
				iNumDelegatesUnaccounted += iMemberDelegates;
			}
		}
	}
	if (iNumCivsUnaccounted > 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_VOTE_OPINIONS_UNACCOUNTED");
		sTemp << iNumDelegatesUnaccounted << iNumCivsUnaccounted;
		s += sTemp.toUTF8();
	}

	// Our delegation
	int iOurDelegates = CalculateStartingVotesForMember(eObserver);
	if (iOurDelegates > 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_VOTE_OPINIONS_OURS");
		sTemp << iOurDelegates;
		s += sTemp.toUTF8();
	}

	return s;
}

CvString CvLeague::GetResolutionProposeOpinionDetails(ResolutionTypes eResolution, PlayerTypes eObserver, int iProposerChoice)
{
	CvString s = "";

	LeagueHelpers::PlayerList vMembersSupporting = GetMembersThatLikeProposal(eResolution, eObserver, iProposerChoice);
	LeagueHelpers::PlayerList vMembersDissuading = GetMembersThatDislikeProposal(eResolution, eObserver, iProposerChoice);

	if (!vMembersSupporting.empty())
	{
		s += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PROPOSAL_OPINIONS_POSITIVE").toUTF8();
		for (uint i = 0; i < vMembersSupporting.size(); i++)
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PROPOSAL_OPINION");
			sTemp << GET_PLAYER(vMembersSupporting[i]).getCivilizationShortDescriptionKey();
			s += sTemp.toUTF8();
		}
	}

	if (!vMembersDissuading.empty())
	{
		s += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PROPOSAL_OPINIONS_NEGATIVE").toUTF8();
		for (uint i = 0; i < vMembersDissuading.size(); i++)
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PROPOSAL_OPINION");
			sTemp << GET_PLAYER(vMembersDissuading[i]).getCivilizationShortDescriptionKey();
			s += sTemp.toUTF8();
		}
	}

	return s;
}

CvString CvLeague::GetResolutionProposeOpinionDetails(int iTargetResolutionID, PlayerTypes eObserver)
{
	CvString s = "";

	LeagueHelpers::PlayerList vMembersSupporting = GetMembersThatLikeProposal(iTargetResolutionID, eObserver);
	LeagueHelpers::PlayerList vMembersDissuading = GetMembersThatDislikeProposal(iTargetResolutionID, eObserver);

	if (!vMembersSupporting.empty())
	{
		s += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PROPOSAL_OPINIONS_POSITIVE").toUTF8();
		for (uint i = 0; i < vMembersSupporting.size(); i++)
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PROPOSAL_OPINION");
			sTemp << GET_PLAYER(vMembersSupporting[i]).getCivilizationShortDescriptionKey();
			s += sTemp.toUTF8();
		}
	}

	if (!vMembersDissuading.empty())
	{
		s += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PROPOSAL_OPINIONS_NEGATIVE").toUTF8();
		for (uint i = 0; i < vMembersDissuading.size(); i++)
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_PROPOSAL_OPINION");
			sTemp << GET_PLAYER(vMembersDissuading[i]).getCivilizationShortDescriptionKey();
			s += sTemp.toUTF8();
		}
	}

	return s;
}

CvString CvLeague::GetMemberDetails(PlayerTypes eMember, PlayerTypes eObserver)
{
	if (!IsMember(eMember) || !IsMember(eObserver))
	{
		CvAssertMsg(false, "Attempting to get detail string for a player that is not a league member. Please send Anton your save file and version.");
		return "";
	}
	CvString s = "";

	// Delegation info
	s += GetMemberDelegationDetails(eMember, eObserver);
	
	// Vote leanings and commitments
	s += GetMemberKnowledgeDetails(eMember, eObserver);
	s += GetMemberVoteOpinionDetails(eMember, eObserver);

	return s;
}

CvString CvLeague::GetMemberDelegationDetails(PlayerTypes eMember, PlayerTypes eObserver)
{
	if (!IsMember(eMember) || !IsMember(eObserver))
	{
		CvAssertMsg(false, "Attempting to get detail string for a player that is not a league member. Please send Anton your save file and version.");
		return "";
	}
	Member* pMember = GetMember(eMember);
	if (pMember == NULL)
	{
		CvAssertMsg(false, "Attempting to get detail string for a player that is has no member data. Please send Anton your save file and version.");
		return "";
	}

	CvString s = "";

	int iVotes = CalculateStartingVotesForMember(eMember); // Updates vote sources string
	if (IsInSession())
	{
		iVotes = GetRemainingVotesForMember(eMember) + GetSpentVotesForMember(eMember);
	}
	CvPlayerAI& kMember = GET_PLAYER(eMember);
	Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_DETAILS");
	sTemp << kMember.getNameKey();
	sTemp << kMember.getCivilizationShortDescriptionKey();
	sTemp << iVotes;
	s += sTemp.toUTF8();
	s += pMember->sVoteSources;

	return s;
}

CvString CvLeague::GetMemberKnowledgeDetails(PlayerTypes eMember, PlayerTypes eObserver)
{
	if (!IsMember(eMember) || !IsMember(eObserver))
	{
		CvAssertMsg(false, "Attempting to get detail string for a player that is not a league member. Please send Anton your save file and version.");
		return "";
	}

	CvString s = "";

	if (eMember != eObserver)
	{
		GET_PLAYER(eMember).GetLeagueAI()->GetKnowledgeGivenToOtherPlayer(eObserver, &s);
	}

	return s;
}

CvString CvLeague::GetMemberVoteOpinionDetails(PlayerTypes eMember, PlayerTypes eObserver)
{
	if (!IsMember(eMember) || !IsMember(eObserver))
	{
		CvAssertMsg(false, "Attempting to get detail string for a player that is not a league member. Please send Anton your save file and version.");
		return "";
	}

	CvString s = "";
	if (!GetEnactProposals().empty() || !GetRepealProposals().empty())
	{
		// Vote Leanings
		if (eMember != eObserver)
		{
			s += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_OPINIONS_VOTES").toUTF8();
			for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
			{
				GET_PLAYER(eMember).GetLeagueAI()->EvaluateVoteForOtherPlayerKnowledge(this, eObserver, it, &s);
			}
			for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); ++it)
			{
				GET_PLAYER(eMember).GetLeagueAI()->EvaluateVoteForOtherPlayerKnowledge(this, eObserver, it, &s);
			}
		}
		
		// Vote Commitments
		if (GET_PLAYER(eMember).GetLeagueAI()->HasVoteCommitment())
		{
			for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
			{
				std::vector<int> vChoices = GetChoicesForDecision(it->GetVoterDecision()->GetType(), NO_PLAYER);
				for (uint i = 0; i < vChoices.size(); i++)
				{
					int iNumVotes = 0;
					if (eMember != eObserver)
					{
						iNumVotes += GET_PLAYER(eMember).GetLeagueAI()->GetVoteCommitment(eObserver, it->GetID(), vChoices[i], /*bEnact*/ true);
					}
					else
					{
						for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
						{
							iNumVotes += GET_PLAYER(eMember).GetLeagueAI()->GetVoteCommitment((PlayerTypes)iPlayerLoop, it->GetID(), vChoices[i], /*bEnact*/ true);
						}
					}
					if (iNumVotes > 0)
					{
						Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_OPINIONS_VOTE_COMMITMENT");
						sTemp << it->GetProposalName() << LeagueHelpers::GetTextForChoice(it->GetVoterDecision()->GetType(), vChoices[i]) << iNumVotes;
						s += sTemp.toUTF8();
					}
				}
			}
			for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); ++it)
			{
				std::vector<int> vChoices = GetChoicesForDecision(it->GetRepealDecision()->GetType(), NO_PLAYER);
				for (uint i = 0; i < vChoices.size(); i++)
				{
					int iNumVotes = 0;
					if (eMember != eObserver)
					{
						iNumVotes += GET_PLAYER(eMember).GetLeagueAI()->GetVoteCommitment(eObserver, it->GetID(), vChoices[i], /*bEnact*/ false);
					}
					else
					{
						for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
						{
							iNumVotes += GET_PLAYER(eMember).GetLeagueAI()->GetVoteCommitment((PlayerTypes)iPlayerLoop, it->GetID(), vChoices[i], /*bEnact*/ false);
						}
					}
					if (iNumVotes > 0)
					{
						Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_OPINIONS_VOTE_COMMITMENT");
						sTemp << it->GetProposalName() << LeagueHelpers::GetTextForChoice(it->GetRepealDecision()->GetType(), vChoices[i]) << iNumVotes;
						s += sTemp.toUTF8();
					}
				}
			}
		}
	}

	return s;
}

CvString CvLeague::GetProjectDetails(LeagueProjectTypes eProject, PlayerTypes eObserver)
{
	CvString s = "";
	CvLeagueProjectEntry* pInfo = GC.getLeagueProjectInfo(eProject);
	if (!IsProjectActive(eProject) && !IsProjectComplete(eProject))
	{
		return "";
	}
	if (!pInfo)
	{
		return "";
	}

	// Progress text
	s += GetProjectProgressDetails(eProject, eObserver);

	// Rewards text
	s += "[NEWLINE][NEWLINE]";
	s += GetProjectRewardDetails(eProject, eObserver);

	return s;
}

CvString CvLeague::GetProjectProgressDetails(LeagueProjectTypes eProject, PlayerTypes eObserver)
{
	CvString s = "";
	if (!IsProjectActive(eProject) && !IsProjectComplete(eProject))
	{
		return "";
	}

	// Total cost
	if (eObserver != NO_PLAYER && IsProjectActive(eProject))
	{
		int iPercentCompleted = (int) (((float)GetProjectProgress(eProject) / (float)GetProjectCost(eProject)) * 100);
		iPercentCompleted = MIN(100, iPercentCompleted);
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_PROJECT_POPUP_PROGRESS_COST");
		sTemp << iPercentCompleted;
		sTemp << GetMemberContribution(eObserver, eProject) / 100;
		s += sTemp.toUTF8();
	}
	else
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_PROJECT_POPUP_TOTAL_COST");
		sTemp << GetProjectCost(eProject) / 100;
		s += sTemp.toUTF8();
	}

	return s;
}

CvString CvLeague::GetProjectRewardDetails(LeagueProjectTypes eProject, PlayerTypes eObserver)
{
	CvString s = "";
	CvLeagueProjectEntry* pInfo = GC.getLeagueProjectInfo(eProject);
	if (!IsProjectActive(eProject) && !IsProjectComplete(eProject))
	{
		return "";
	}
	if (!pInfo)
	{
		return "";
	}
	
	s += GetProjectRewardTierDetails(3, eProject, eObserver);
	s += "[NEWLINE][NEWLINE]";
	s += GetProjectRewardTierDetails(2, eProject, eObserver);
	s += "[NEWLINE][NEWLINE]";
	s += GetProjectRewardTierDetails(1, eProject, eObserver);

	return s;
}

CvString CvLeague::GetProjectRewardTierDetails(int iTier, LeagueProjectTypes eProject, PlayerTypes /*eObserver*/)
{
	CvString s = "";
	CvLeagueProjectEntry* pInfo = GC.getLeagueProjectInfo(eProject);
	if (!IsProjectActive(eProject) && !IsProjectComplete(eProject))
	{
		return "";
	}
	if (!pInfo)
	{
		return "";
	}

	CvLeagueProjectRewardEntry* pRewardInfo = NULL;
	CvString sRewardIcon = "";
	CvString sContribution = "";
	if (iTier == 3)
	{
		pRewardInfo = GC.getLeagueProjectRewardInfo(pInfo->GetRewardTier3());
		sRewardIcon = "[ICON_TROPHY_GOLD]";
#ifdef LEAGUE_PROJECT_REWARD_TIER_3_THRESHOLD
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_PROJECT_REWARD_TIER_3");
		sTemp << GetContributionTierThreshold(CONTRIBUTION_TIER_2, eProject) / 100 * 8 / 5;
#else
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_PROJECT_REWARD_TIER_3");
#endif
		sContribution = sTemp.toUTF8();
	}
	else if (iTier == 2)
	{
		pRewardInfo = GC.getLeagueProjectRewardInfo(pInfo->GetRewardTier2());
		sRewardIcon = "[ICON_TROPHY_SILVER]";
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_PROJECT_REWARD_TIER_2");
		sTemp << GetContributionTierThreshold(CONTRIBUTION_TIER_2, eProject) / 100;
		sContribution = sTemp.toUTF8();
	}
	else if (iTier == 1)
	{
		pRewardInfo = GC.getLeagueProjectRewardInfo(pInfo->GetRewardTier1());
		sRewardIcon = "[ICON_TROPHY_BRONZE]";
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_PROJECT_REWARD_TIER_1");
		sTemp << GetContributionTierThreshold(CONTRIBUTION_TIER_1, eProject) / 100;
		sContribution = sTemp.toUTF8();
	}

	CvAssert(pRewardInfo);
	if (pRewardInfo != NULL)
	{
		Localization::String sResult = Localization::Lookup("TXT_KEY_LEAGUE_PROJECT_REWARD_DESC");
		sResult << sRewardIcon << pRewardInfo->GetDescriptionKey() << sContribution << pRewardInfo->GetHelp();
		s = sResult.toUTF8();
	}

	return s;
}

std::vector<CvString> CvLeague::GetCurrentEffectsSummary(PlayerTypes /*eObserver*/)
{
	std::vector<CvString> vsEffects;

	// League status
	if (HasHostMember())
	{
		PlayerTypes eHost = GetHostMember();
		if (eHost != NO_PLAYER)
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_HOST");
			sTemp << GET_PLAYER(eHost).getCivilizationShortDescriptionKey();
			vsEffects.push_back(sTemp.toUTF8());
		}
	}

	// Active projects
	int iProjects = 0;
	CvString sProjects = "";
	for (ProjectList::const_iterator it = m_vProjects.begin(); it != m_vProjects.end(); ++it)
	{
		if (IsProjectActive(it->eType) && !IsProjectComplete(it->eType))
		{
			CvLeagueProjectEntry* pInfo = GC.getLeagueProjectInfo(it->eType);
			if (pInfo)
			{
				if (iProjects != 0)
				{
					sProjects += ", ";
				}
				iProjects++;
				sProjects += pInfo->GetDescription();
			}
		}
	}
	if (iProjects > 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_ACTIVE_PROJECTS");
		sTemp << sProjects;
		vsEffects.push_back(sTemp.toUTF8());
	}

	// Ongoing resolution effects
	ReligionTypes eWorldReligion = NO_RELIGION;
	PolicyBranchTypes eWorldIdeology = NO_POLICY_BRANCH_TYPE;
	FStaticVector<PlayerTypes, MAX_MAJOR_CIVS, true, c_eCiv5GameplayDLL> veEmbargoedPlayers;
	FStaticVector<ResourceTypes, 32, true, c_eCiv5GameplayDLL> veBannedResources;
#ifdef NEW_LEAGUE_RESOLUTIONS
	FStaticVector<ResourceTypes, 32, true, c_eCiv5GameplayDLL> veDoubledResources;
#endif
	CvResolutionEffects effects;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		effects.AddOngoingEffects(it->GetEffects());

		if (it->GetEffects()->iResourceQuantity != 0)
		{
			//antonjs: todo: for modders
		}

		if (it->GetEffects()->bEmbargoPlayer)
		{
			PlayerTypes eEmbargoedPlayer = (PlayerTypes) it->GetProposerDecision()->GetDecision();
			CvAssert(eEmbargoedPlayer != NO_PLAYER);
			veEmbargoedPlayers.push_back(eEmbargoedPlayer);
		}

		if (it->GetEffects()->bNoResourceHappiness)
		{
			ResourceTypes eBannedResource = (ResourceTypes) it->GetProposerDecision()->GetDecision();
			CvAssert(eBannedResource != NO_RESOURCE);
			veBannedResources.push_back(eBannedResource);
		}

		if (it->GetEffects()->iVotesForFollowingReligion != 0)
		{
			ReligionTypes eReligion = (ReligionTypes) it->GetProposerDecision()->GetDecision();
			CvAssert(eReligion != NO_RELIGION);
			CvAssert(eWorldReligion == NO_RELIGION);
			eWorldReligion = eReligion;
		}

		if (it->GetEffects()->iVotesForFollowingIdeology != 0)
		{
			PolicyBranchTypes eIdeology = (PolicyBranchTypes) it->GetProposerDecision()->GetDecision();
			CvAssert(eIdeology != NO_POLICY_BRANCH_TYPE);
			CvAssert(eWorldIdeology == NO_POLICY_BRANCH_TYPE);
			eWorldIdeology = eIdeology;
		}

#ifdef NEW_LEAGUE_RESOLUTIONS
		if (it->GetEffects()->bDoubleResourceHappiness)
		{
			ResourceTypes eDoubledResource = (ResourceTypes)it->GetProposerDecision()->GetDecision();
			CvAssert(eDoubledResource != NO_RESOURCE);
			veDoubledResources.push_back(eDoubledResource);
		}
#endif
	}
	if (eWorldReligion != NO_RELIGION)
	{
		CvReligionEntry* pInfo = GC.getReligionInfo(eWorldReligion);
		CvAssert(pInfo != NULL);
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eWorldReligion, NO_PLAYER);
		CvAssert(pReligion != NULL);
		if (pInfo != NULL && pReligion != NULL)
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_WORLD_RELIGION");
			sTemp << pInfo->GetIconString() << pReligion->GetName();
			vsEffects.push_back(sTemp.toUTF8());
		}
	}
	if (eWorldIdeology != NO_POLICY_BRANCH_TYPE)
	{
		CvPolicyBranchEntry* pInfo = GC.getPolicyBranchInfo(eWorldIdeology);
		CvAssert(pInfo != NULL);
		if (pInfo != NULL)
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_WORLD_IDEOLOGY");
			sTemp << pInfo->GetDescriptionKey();
			vsEffects.push_back(sTemp.toUTF8());
		}
	}
	if (effects.iGoldPerTurn != 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_GPT");
		sTemp << effects.iGoldPerTurn;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iResourceQuantity != 0)
	{
		//antonjs: todo: for modders
	}
	if (effects.bEmbargoCityStates)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_EMBARGO_CITY_STATES");
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.bEmbargoPlayer)
	{
		CvAssert(!veEmbargoedPlayers.empty());
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_EMBARGO_PLAYERS");
		CvString sEntries = "";
		for (uint i = 0; i < veEmbargoedPlayers.size(); i++)
		{
			Localization::String sTempEntry = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_EMBARGO_PLAYERS_ENTRY");
			CvAssert(veEmbargoedPlayers[i] != NO_PLAYER);
			if (veEmbargoedPlayers[i] != NO_PLAYER)
			{
				if (i != 0)
				{
					sEntries += ", ";
				}
				sTempEntry << GET_PLAYER(veEmbargoedPlayers[i]).getCivilizationShortDescriptionKey();
				sEntries += sTempEntry.toUTF8();
			}
		}
		sTemp << sEntries;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.bNoResourceHappiness)
	{
		CvAssert(!veBannedResources.empty());
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_NO_RESOURCE_HAPPINESS");
		CvString sEntries = "";
		for (uint i = 0; i < veBannedResources.size(); i++)
		{
			Localization::String sTempEntry = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_NO_RESOURCE_HAPPINESS_ENTRY");
			CvResourceInfo* pInfo = GC.getResourceInfo(veBannedResources[i]);
			CvAssert(pInfo);
			if (pInfo)
			{
				if (i != 0)
				{
					sEntries += ", ";
				}
				sTempEntry << pInfo->GetDescriptionKey();
				sEntries += pInfo->GetIconString();
				sEntries += sTempEntry.toUTF8();
			}
		}
		sTemp << sEntries;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iUnitMaintenanceGoldPercent != 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_UNIT_MAINTENANCE");
		sTemp << effects.iUnitMaintenanceGoldPercent;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iMemberDiscoveredTechMod != 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_DISCOVERED_TECH_MODIFIER");
		sTemp << effects.iMemberDiscoveredTechMod;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iCulturePerWonder != 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_CULTURE_FROM_WONDERS");
		sTemp << effects.iCulturePerWonder;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iCulturePerNaturalWonder != 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_CULTURE_FROM_NATURAL_WONDERS");
		sTemp << effects.iCulturePerNaturalWonder;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iArtsyGreatPersonRateMod != 0)
	{
		int iMod = effects.iArtsyGreatPersonRateMod;
		Localization::String sTemp = (iMod > 0) ? Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_POSITIVE_GREAT_PERSON_RATE") : Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_NEGATIVE_GREAT_PERSON_RATE");
		sTemp << iMod;
		
		CvString sList = "";
		CvUnitClassInfo* pInfo = GC.getUnitClassInfo((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_WRITER"));
		if (pInfo != NULL)
		{
			if (sList != "")
				sList += ", ";
			Localization::String sEntry = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_GREAT_PERSON_RATE_ENTRY");
			sEntry << pInfo->GetDescriptionKey();
			sList += sEntry.toUTF8();
		}
		pInfo = GC.getUnitClassInfo((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_ARTIST"));
		if (pInfo != NULL)
		{
			if (sList != "")
				sList += ", ";
			Localization::String sEntry = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_GREAT_PERSON_RATE_ENTRY");
			sEntry << pInfo->GetDescriptionKey();
			sList += sEntry.toUTF8();
		}
		pInfo = GC.getUnitClassInfo((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_MUSICIAN"));
		if (pInfo != NULL)
		{
			if (sList != "")
				sList += ", ";
			Localization::String sEntry = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_GREAT_PERSON_RATE_ENTRY");
			sEntry << pInfo->GetDescriptionKey();
			sList += sEntry.toUTF8();
		}
		sTemp << sList;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iScienceyGreatPersonRateMod != 0)
	{
		int iMod = effects.iScienceyGreatPersonRateMod;
		Localization::String sTemp = (iMod > 0) ? Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_POSITIVE_GREAT_PERSON_RATE") : Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_NEGATIVE_GREAT_PERSON_RATE");
		sTemp << iMod;

		CvString sList = "";
		CvUnitClassInfo* pInfo = GC.getUnitClassInfo((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_SCIENTIST"));
		if (pInfo != NULL)
		{
			if (sList != "")
				sList += ", ";
			Localization::String sEntry = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_GREAT_PERSON_RATE_ENTRY");
			sEntry << pInfo->GetDescriptionKey();
			sList += sEntry.toUTF8();
		}
		pInfo = GC.getUnitClassInfo((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_ENGINEER"));
		if (pInfo != NULL)
		{
			if (sList != "")
				sList += ", ";
			Localization::String sEntry = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_GREAT_PERSON_RATE_ENTRY");
			sEntry << pInfo->GetDescriptionKey();
			sList += sEntry.toUTF8();
		}
		pInfo = GC.getUnitClassInfo((UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_MERCHANT"));
		if (pInfo != NULL)
		{
			if (sList != "")
				sList += ", ";
			Localization::String sEntry = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_GREAT_PERSON_RATE_ENTRY");
			sEntry << pInfo->GetDescriptionKey();
			sList += sEntry.toUTF8();
		}
		sTemp << sList;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iGreatPersonTileImprovementCulture != 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_GREAT_PERSON_TILE_IMPROVEMENT_CULTURE");
		sTemp << effects.iGreatPersonTileImprovementCulture;
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iLandmarkCulture != 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_LANDMARK_CULTURE");
		sTemp << effects.iLandmarkCulture;
		vsEffects.push_back(sTemp.toUTF8());
	}
#ifdef SHOW_NUCLEAR_NON_PROLIFERATION_ACTIVE_EFFECT
	if (effects.bNoTrainingNuclearWeapons)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_NUCLEAR_NON_PROLIFERATION");
		vsEffects.push_back(sTemp.toUTF8());
	}
#endif
#ifdef NEW_LEAGUE_RESOLUTIONS
	if (effects.iTradeRouteGoldModifier != 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_WORLD_TRADE_ORGANISATION");
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.iCSBonuModifier != 0)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_TREATY_OF_FRIENDSHIP");
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.bNoSpiesInCS)
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_NON_ALIGN_MOVEMENT");
		vsEffects.push_back(sTemp.toUTF8());
	}
	if (effects.bDoubleResourceHappiness)
	{
		CvAssert(!veDoubledResources.empty());
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_INVEST_IN_LUXURY");
		CvString sEntries = "";
		for (uint i = 0; i < veDoubledResources.size(); i++)
		{
			Localization::String sTempEntry = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_INVEST_IN_LUXURY_ENTRY");
			CvResourceInfo* pInfo = GC.getResourceInfo(veDoubledResources[i]);
			CvAssert(pInfo);
			if (pInfo)
			{
				if (i != 0)
				{
					sEntries += ", ";
				}
				sTempEntry << pInfo->GetDescriptionKey();
				sEntries += pInfo->GetIconString();
				sEntries += sTempEntry.toUTF8();
			}
		}
		sTemp << sEntries;
		vsEffects.push_back(sTemp.toUTF8());
	}
#endif

	if (vsEffects.empty())
	{
		vsEffects.push_back(Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_EFFECT_SUMMARY_NONE").toUTF8());
	}

	return vsEffects;
}

CvString CvLeague::GetLeagueSplashTitle(LeagueSpecialSessionTypes eGoverningSpecialSession, bool bJustFounded)
{
	CvString s = "";
	CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eGoverningSpecialSession);

	if (!bJustFounded && pInfo != NULL)
	{
		Localization::String sTemp = Localization::Lookup(pInfo->GetDescriptionKey());
		s += sTemp.toUTF8();
	}
	else
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_TITLE_FOUNDED");
		s += sTemp.toUTF8();
	}

	return s;
}

CvString CvLeague::GetLeagueSplashDescription(LeagueSpecialSessionTypes eGoverningSpecialSession, bool bJustFounded)
{
	CvString s = "";
	CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eGoverningSpecialSession);
	
	if (!bJustFounded && pInfo != NULL)
	{
		EraTypes eEra = pInfo->GetEraTrigger();
		CvEraInfo* pEraInfo = GC.getEraInfo(eEra);
		CvAssert(pEraInfo != NULL);
		if (pEraInfo != NULL)
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_GAME_ERA");
			sTemp << pEraInfo->GetDescriptionKey();
			s += sTemp.toUTF8();
			s += "[NEWLINE][NEWLINE]";
		}
		
		s += Localization::Lookup(pInfo->GetHelp()).toUTF8();
	}
	else
	{
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_FOUNDED");
		CvAssert(HasHostMember());
		if (!HasHostMember())
		{
			sTemp << "Nobody";
		}
		else
		{
			sTemp << GET_PLAYER(GetHostMember()).getCivilizationShortDescriptionKey();
		}
		sTemp << GetName() << GetTurnsUntilSession();
		s += sTemp.toUTF8();
	}

	return s;
}

CvString CvLeague::GetLeagueSplashThisEraDetails(LeagueSpecialSessionTypes eGoverningSpecialSession, bool /*bJustFounded*/)
{
	CvString s = "";
	CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eGoverningSpecialSession);

	s += Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_THIS_ERA").toUTF8();

	if (pInfo != NULL)
	{
		s += "[NEWLINE]";
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_HOST_DELEGATES");
		sTemp << pInfo->GetHostDelegates() + pInfo->GetCivDelegates();
		s += sTemp.toUTF8();

		s += "[NEWLINE]";
		sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_CIV_DELEGATES");
		sTemp << pInfo->GetCivDelegates();
		s += sTemp.toUTF8();

		s += "[NEWLINE]";
		sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_CITYSTATE_DELEGATES");
		sTemp << pInfo->GetCityStateDelegates();
		s += sTemp.toUTF8();

		if (pInfo->IsUnitedNations())
		{
			VictoryTypes eDiploVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);
			if (eDiploVictory != NO_VICTORY)
			{
				if (GC.getGame().isVictoryValid(eDiploVictory))
				{
					s += "[NEWLINE]";
					sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_DIPLO_VICTORY_POSSIBLE");
					s += sTemp.toUTF8();
				}
			}
		}
	}
	else
	{
		s += "[NEWLINE]";
		s += Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_NO_CHANGE").toUTF8();
	}

	return s;
}

CvString CvLeague::GetLeagueSplashNextEraDetails(LeagueSpecialSessionTypes eGoverningSpecialSession, bool /*bJustFounded*/)
{
	CvString s = "";
	CvLeagueSpecialSessionEntry* pThisSessionInfo = GC.getLeagueSpecialSessionInfo(eGoverningSpecialSession);
	CvLeagueSpecialSessionEntry* pInfo = NULL;
	if (pThisSessionInfo != NULL)
	{
		EraTypes eNextEra = LeagueHelpers::GetNextGameEraForTrigger(pThisSessionInfo->GetEraTrigger());
		for (int i = 0; i < GC.getNumLeagueSpecialSessionInfos(); i++)
		{
			LeagueSpecialSessionTypes e = (LeagueSpecialSessionTypes)i;
			CvLeagueSpecialSessionEntry* p = GC.getLeagueSpecialSessionInfo(e);
			if (p != NULL && p->GetEraTrigger() == eNextEra)
			{
				pInfo = GC.getLeagueSpecialSessionInfo(e);
				break;
			}
		}
	}

	s += Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_NEXT_ERA").toUTF8();

	if (pInfo != NULL)
	{
		s += "[NEWLINE]";
		Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_HOST_DELEGATES");
		sTemp << pInfo->GetHostDelegates() + pInfo->GetCivDelegates();
		s += sTemp.toUTF8();

		s += "[NEWLINE]";
		sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_CIV_DELEGATES");
		sTemp << pInfo->GetCivDelegates();
		s += sTemp.toUTF8();

		s += "[NEWLINE]";
		sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_CITYSTATE_DELEGATES");
		sTemp << pInfo->GetCityStateDelegates();
		s += sTemp.toUTF8();

		if (pInfo->IsUnitedNations())
		{
			VictoryTypes eDiploVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);
			if (eDiploVictory != NO_VICTORY)
			{
				if (GC.getGame().isVictoryValid(eDiploVictory))
				{
					s += "[NEWLINE]";
					sTemp = Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_DIPLO_VICTORY_POSSIBLE");
					s += sTemp.toUTF8();
				}
			}
		}
	}
	else
	{
		s += "[NEWLINE]";
		s += Localization::Lookup("TXT_KEY_LEAGUE_SPLASH_MESSAGE_ERA_NO_CHANGE").toUTF8();
	}

	return s;
}

CvString CvLeague::GetGreatPersonRateModifierDetails(UnitClassTypes /*eGreatPersonClass*/)
{
	CvString s = "";

	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (it->GetEffects()->iArtsyGreatPersonRateMod != 0 ||
			it->GetEffects()->iScienceyGreatPersonRateMod != 0)
		{
			CvResolutionEntry* pInfo = GC.getResolutionInfo(it->GetType());
			CvAssert(pInfo);
			if (pInfo)
			{
				s += "[NEWLINE]";
				s += "[ICON_BULLET] ";
				s += Localization::Lookup(pInfo->GetDescriptionKey()).toUTF8();
				s += " (";
				s += Localization::Lookup(pInfo->GetHelp()).toUTF8();
				s += ")";
			}
		}
	}

	return s;
}

void CvLeague::CheckProjectAchievements()
{
	for (MemberList::const_iterator member = m_vMembers.begin(); member != m_vMembers.end(); ++member)
	{
		if (member->ePlayer != NO_PLAYER && GET_PLAYER(member->ePlayer).isAlive() && GET_PLAYER(member->ePlayer).isHuman() && GET_PLAYER(member->ePlayer).isLocalPlayer())
		{
			int iHighestContributorProjects = 0;

			for (ProjectList::const_iterator project = m_vProjects.begin(); project != m_vProjects.end(); ++project)
			{
				if (project->bComplete)
				{
					if (GetMemberContributionTier(member->ePlayer, project->eType) == CONTRIBUTION_TIER_3)
					{
						iHighestContributorProjects++;
					}
				}
			}

			if (iHighestContributorProjects >= GC.getNumLeagueProjectInfos() && GC.getNumLeagueProjectInfos() > 0)
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_XP2_44);
			}
		}
	}
}

void CvLeague::CheckStartSession()
{
	if (!IsInSession())
	{
		bool bCanStart = true;
		for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
		{
			if (CanPropose(it->ePlayer))
			{
				bCanStart = false;
				break;
			}
		}
		if (bCanStart)
		{
			StartSession();
		}
	}
}

void CvLeague::StartSession()
{
	SetInSession(true);

	// Distribute Votes
	AssignStartingVotes();

	CheckFinishSession();
}

void CvLeague::CheckStartSpecialSession(LeagueSpecialSessionTypes eSpecialSession)
{
	if (CanStartSpecialSession(eSpecialSession))
	{
		CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eSpecialSession);
		CvAssert(pInfo);
		if (pInfo != NULL)
		{
			CvGame& kGame = GC.getGame();
			CvPlayer& kActivePlayer = GET_PLAYER(kGame.getActivePlayer());

			if(!kGame.isNetworkMultiPlayer() && !kActivePlayer.isObserver()){
				// Show splash screen
				CvPopupInfo kPopup(BUTTONPOPUP_LEAGUE_SPLASH, GetID(), GetHostMember(), eSpecialSession, 0, /*bJustFounded*/ false);
				GC.GetEngineUserInterface()->AddPopup(kPopup);
			}

			DLLUI->AddMessage(0, kGame.getActivePlayer(), false, GC.getEVENT_MESSAGE_TIME(), 
				GetLocalizedText(pInfo->GetDescriptionKey()).GetCString());

			// Becomes United Nations?
			CvAssert(!(!pInfo->IsUnitedNations() && IsUnitedNations())); // UN shouldn't be reversible
			if (pInfo->IsUnitedNations())
			{
				SetUnitedNations(true);
			}

			// Only actually hold a session if there is a proposal to be decided
			CvAssertMsg(pInfo->GetImmediateProposal() != NO_RESOLUTION, "Cannot hold special session because there is no proposal to decide on. Please send Anton your save file and version.");
			if (pInfo->GetImmediateProposal() != NO_RESOLUTION)
			{
				StartSpecialSession(eSpecialSession);
			}
		}
	}
}

void CvLeague::StartSpecialSession(LeagueSpecialSessionTypes eSpecialSession)
{
	SetInSession(eSpecialSession);

	// Immediate Proposal
	CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eSpecialSession);
	CvAssert(pInfo != NULL);
	if (pInfo != NULL)
	{
		ResolutionTypes eProposal = pInfo->GetImmediateProposal();
		CvAssert(eProposal != NO_RESOLUTION)
		if (eProposal != NO_RESOLUTION)
		{
			// Put other proposals on hold
			DoPutProposalsOnHold();

			// Add the immediate proposal
			DoProposeEnact(eProposal, NO_PLAYER, LeagueHelpers::CHOICE_NONE);
		}
	}

	// Distribute Votes
	AssignStartingVotes();

	CheckFinishSession();
}

void CvLeague::CheckFinishSession()
{
	if (IsInSession())
	{
		bool bFinished = true;
		for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
		{
			if (CanVote(it->ePlayer))
			{
				bFinished = false;
				break;
			}
		}
		if (bFinished)
		{
			FinishSession();
		}
	}
}

void CvLeague::FinishSession()
{
	PlayerTypes eOldHost = GetHostMember();
	LeagueHelpers::PlayerList vSupportedNewHost;
	int iTotalSessionVotes = GetVotesSpentThisSession();

	// Find out if we have a recurring proposal
	ResolutionTypes eRecurringProposal = NO_RESOLUTION;
	bool bRecurringProposalThisSession = false;
	if (GetLastSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
	{
		CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(GetLastSpecialSession());
		CvAssert(pInfo != NULL);
		if (pInfo != NULL)
		{
			eRecurringProposal = pInfo->GetRecurringProposal();
		}
	}

#ifdef POLICY_LEAGUE_SESSION_YIELD_BOOST_PER_DELEGATE
	SetResolveSession(true);
#endif

	// Notify players of vote results
	NotifySessionDone();

	// Resolve Proposals on table
	for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); it++)
	{
		if (eRecurringProposal != NO_RESOLUTION && eRecurringProposal == it->GetType())
		{
			bRecurringProposalThisSession = true;
		}

		PlayerTypes eProposer = it->GetProposalPlayer();
		LeagueHelpers::PlayerList vHelpedOutcome = it->GetRepealDecision()->GetPlayersVotingForChoice(it->GetRepealDecision()->GetDecision());
		if (it->IsPassed(iTotalSessionVotes))
		{
			// Proposer is grateful to people who helped it pass
			if (eProposer != NO_PLAYER)
			{
				for (LeagueHelpers::PlayerList::iterator playerIt = vHelpedOutcome.begin(); playerIt != vHelpedOutcome.end(); ++playerIt)
				{
					GET_PLAYER(eProposer).GetDiplomacyAI()->SetTurnsSinceTheySupportedOurProposal(*playerIt, 0);
				}
			}

			DoRepealResolution(it);
		}
		else
		{
			// Proposer is angry at people who helped it fail
			if (eProposer != NO_PLAYER)
			{
				for (LeagueHelpers::PlayerList::iterator playerIt = vHelpedOutcome.begin(); playerIt != vHelpedOutcome.end(); ++playerIt)
				{
					GET_PLAYER(eProposer).GetDiplomacyAI()->SetTurnsSinceTheyFoiledOurProposal(*playerIt, 0);
				}
			}
		}

		LogProposalResolved(it);
	}
	for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); it++)
	{
		if (eRecurringProposal != NO_RESOLUTION && eRecurringProposal == it->GetType())
		{
			bRecurringProposalThisSession = true;
		}

		PlayerTypes eProposer = it->GetProposalPlayer();
		LeagueHelpers::PlayerList vHelpedOutcome = it->GetVoterDecision()->GetPlayersVotingForChoice(it->GetVoterDecision()->GetDecision());
		if (it->IsPassed(iTotalSessionVotes))
		{
			// Proposer is grateful to people who helped it pass
			if (eProposer != NO_PLAYER)
			{
				for (LeagueHelpers::PlayerList::iterator playerIt = vHelpedOutcome.begin(); playerIt != vHelpedOutcome.end(); ++playerIt)
				{
					GET_PLAYER(eProposer).GetDiplomacyAI()->SetTurnsSinceTheySupportedOurProposal(*playerIt, 0);
				}
			}

			if (it->GetEffects()->bChangeLeagueHost)
			{
				vSupportedNewHost.clear();
				vSupportedNewHost = it->GetVoterDecision()->GetPlayersVotingForChoice(it->GetVoterDecision()->GetDecision());
			}

			DoEnactResolution(it);
		}
		else
		{
			// Proposer is angry at people who helped it fail
			if (eProposer != NO_PLAYER)
			{
				for (LeagueHelpers::PlayerList::iterator playerIt = vHelpedOutcome.begin(); playerIt != vHelpedOutcome.end(); ++playerIt)
				{
					GET_PLAYER(eProposer).GetDiplomacyAI()->SetTurnsSinceTheyFoiledOurProposal(*playerIt, 0);
				}
			}

			// Since it failed, it may provide members with extra votes
			CvResolutionEntry* pInfo = GC.getResolutionInfo(it->GetType());
			CvAssert(pInfo != NULL);
			if (pInfo != NULL)
			{
				if (pInfo->GetLeadersVoteBonusOnFail() > 0)
				{
					bool bChoicesArePlayers = (pInfo->GetVoterDecision() == RESOLUTION_DECISION_MAJOR_CIV_MEMBER || pInfo->GetVoterDecision() == RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER);
					CvAssertMsg(bChoicesArePlayers, "Trying to give bonus delegates since a resolution failed, but voter decision is unexpected type. Please send Anton your save file and version.");
					if (bChoicesArePlayers)
					{
						std::vector<int> vBonusVotePlayers = it->GetVoterDecision()->GetTopVotedChoices(LeagueHelpers::NUM_LEADERS_FOR_EXTRA_VOTES);
						for (uint i = 0; i < vBonusVotePlayers.size(); i++)
						{
							Member* pMember = GetMember((PlayerTypes)vBonusVotePlayers[i]);
							CvAssert(pMember != NULL);
							if (pMember != NULL)
							{
								pMember->iExtraVotes = pMember->iExtraVotes + pInfo->GetLeadersVoteBonusOnFail();
							}
						}
					}
				}
			}
		}

		LogProposalResolved(it);
	}
	PlayerTypes eNewHost = GetHostMember();

	// A new host is grateful to people who helped him
	if (!vSupportedNewHost.empty() && eNewHost != NO_PLAYER)
	{
		for (LeagueHelpers::PlayerList::iterator playerIt = vSupportedNewHost.begin(); playerIt != vSupportedNewHost.end(); ++playerIt)
		{
			GET_PLAYER(eNewHost).GetDiplomacyAI()->SetTurnsSinceTheySupportedOurHosting(*playerIt, 0);
		}
	}

	// Update number of sessions held by consecutively by host
	if (eNewHost == eOldHost)
	{
		ChangeConsecutiveHostedSessions(1);
	}
	else
	{
		SetConsecutiveHostedSessions(0);
	}

	// Clear processed proposals, and bring back any that were on hold
	DoClearProposals();

	// Clear existing proposal privileges
	ClearProposalPrivileges();

	bool bRegularSession = (GetCurrentSpecialSession() == NO_LEAGUE_SPECIAL_SESSION);
	if (bRegularSession)
	{
		// Prepare next proposals
		if (eRecurringProposal != NO_RESOLUTION && !bRecurringProposalThisSession && IsResolutionEffectsValid(eRecurringProposal, LeagueHelpers::CHOICE_NONE))
		{
			// We have a recurring proposal that was not proposed this time, so auto propose it for next time
			DoProposeEnact(eRecurringProposal, NO_PLAYER, LeagueHelpers::CHOICE_NONE);
		}
		else
		{
			// Players get to make the proposals for next session
			CvAssertMsg(!IsAnythingProposed(), "Assigning proposal privileges to players when something is already proposed. Please send Anton your save file and version.");
			AssignProposalPrivileges();
#ifdef CAN_PROPOSE_ENACT_UPDATES_ONCE_PER_SESSION
			UpdateCanProposeEnact();
#endif
		}

		// Reset counter
		ResetTurnsUntilSession();
	}

	SetInSession(false);
}

void CvLeague::AssignStartingVotes()
{
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		if (CanEverVote(it->ePlayer))
		{
			it->iVotes = CalculateStartingVotesForMember(it->ePlayer, /*bForceUpdateSources*/ true);
			it->iAbstainedVotes = 0;
		}
	}
}

void CvLeague::ClearProposalPrivileges()
{
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		it->bMayPropose = false;
		it->iProposals = 0;
	}
}

void CvLeague::AssignProposalPrivileges()
{
	CvWeightedVector<Member*, MAX_CIV_PLAYERS, false> vpPossibleProposers;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		if (CanEverPropose(it->ePlayer))
		{
			int iVotes = CalculateStartingVotesForMember(it->ePlayer);
			vpPossibleProposers.push_back(it, iVotes);
		}
		else
		{
			CvAssertMsg(!it->bMayPropose, "Found a member with proposal rights that should not have them. Please send Anton your save file and version.");
			CvAssertMsg(it->iProposals == 0, "Found a member with remaining proposals that should not have them. Please send Anton your save file and version.");
		}
	}
	vpPossibleProposers.SortItems();

	int iPrivileges = GetNumProposersPerSession();

	// Host gets one
	PlayerTypes eHost = GetHostMember();
	if (eHost != NO_PLAYER)
	{
		GetMember(eHost)->bMayPropose = true;
		GetMember(eHost)->iProposals = GC.getLEAGUE_MEMBER_PROPOSALS_BASE();
		iPrivileges--;
	}

#ifndef ASSIGN_SECOND_PROPOSAL_PRIVILEGE
	// Give rest to largest delegations
	for (int i = 0; i < vpPossibleProposers.size(); i++)
	{
		if (iPrivileges == 0)
		{
			break;
		}
		
		// Only one privilege per player
		if (!vpPossibleProposers.GetElement(i)->bMayPropose)
		{
			vpPossibleProposers.GetElement(i)->bMayPropose = true;
			vpPossibleProposers.GetElement(i)->iProposals = GC.getLEAGUE_MEMBER_PROPOSALS_BASE();
			iPrivileges--;
		}
	}

	CvAssert(iPrivileges == 0);
#else
	if(GC.getGame().isGameMultiPlayer())
	{
		CvAssert(iPrivileges == 1);
	}
	else
	{
		for (int i = 0; i < vpPossibleProposers.size(); i++)
		{
			if (iPrivileges == 0)
			{
				break;
			}
		
			// Only one privilege per player
			if (!vpPossibleProposers.GetElement(i)->bMayPropose)
			{
				vpPossibleProposers.GetElement(i)->bMayPropose = true;
				vpPossibleProposers.GetElement(i)->iProposals = GC.getLEAGUE_MEMBER_PROPOSALS_BASE();
				iPrivileges--;
			}
		}

		CvAssert(iPrivileges == 0);
	}
#endif
}

void CvLeague::CheckProposalsValid()
{
	for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
	{
		if (!IsResolutionEffectsValid(it->GetType(), it->GetProposerDecision()->GetDecision()))
		{
			m_vEnactProposals.erase(it);
			it--;
		}
	}
	for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); ++it)
	{
		if (!IsResolutionEffectsValid(it->GetType(), it->GetProposerDecision()->GetDecision()))
		{
			m_vRepealProposals.erase(it);
			it--;
		}
	}
}

void CvLeague::CheckResolutionsValid()
{
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); ++it)
	{
		if (!IsResolutionEffectsValid(it->GetType(), it->GetProposerDecision()->GetDecision()))
		{
			for (uint i = 0; i < m_vMembers.size(); i++)
			{
				it->RemoveEffects(m_vMembers[i].ePlayer);
			}
			m_vActiveResolutions.erase(it);
			//antonjs: todo: relocate these league-level effects:
			for (uint i = 0; i < m_vMembers.size(); i++)
			{
				GET_PLAYER(m_vMembers[i].ePlayer).DoUpdateHappiness();
				GET_PLAYER(m_vMembers[i].ePlayer).updateYield();
				GET_PLAYER(m_vMembers[i].ePlayer).recomputeGreatPeopleModifiers();
			}
			it--;
		}
	}
}

void CvLeague::AssignNewHost()
{
	// Set host as remaining player with most votes
	PlayerTypes eNewHost = NO_PLAYER;
	int iHighestVotes = -1;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
	{
		int iVotes = CalculateStartingVotesForMember(it->ePlayer);
		if (iVotes > iHighestVotes)
		{
			eNewHost = it->ePlayer;
			iHighestVotes = iVotes;
		}
	}
	CvAssert(eNewHost != NO_PLAYER);
	SetHostMember(eNewHost);
}

void CvLeague::DoEnactResolution(CvEnactProposal* pProposal)
{
	CvAssertMsg(pProposal->IsPassed(GetVotesSpentThisSession()), "Doing a proposal that has not been passed. Please send Anton your save file and version.");

	CvActiveResolution resolution(pProposal);
	for (uint i = 0; i < m_vMembers.size(); i++)
	{
		resolution.DoEffects(m_vMembers[i].ePlayer);
	}
	
	// Active Resolutions with only one-time effects immediately expire
	if (resolution.HasOngoingEffects())
	{
		m_vActiveResolutions.push_back(resolution);
	}
	
	//antonjs: todo: relocate these league-level effects:
	for (uint i = 0; i < m_vMembers.size(); i++)
	{
		GET_PLAYER(m_vMembers[i].ePlayer).DoUpdateHappiness();
		GET_PLAYER(m_vMembers[i].ePlayer).updateYield();
		GET_PLAYER(m_vMembers[i].ePlayer).recomputeGreatPeopleModifiers();
	}
	LeagueProjectTypes eProject = resolution.GetEffects()->eLeagueProjectEnabled;
	if (eProject != NO_LEAGUE_PROJECT)
	{
		CvAssertMsg(!IsProjectActive(eProject), "Trying to start a League Project that is already active. Please send Anton your save file and version.");
		StartProject(eProject);
	}
#ifdef REPLAY_EVENTS
	std::vector<int> vArgs;
	vArgs.push_back(pProposal->GetID());
	vArgs.push_back(static_cast<int>(pProposal->GetType()));
	vArgs.push_back(static_cast<int>(true));
	GC.getGame().addReplayEvent(REPLAYEVENT_CongressEnactedProposalsChange, NO_PLAYER, vArgs);
#endif

	m_iNumResolutionsEverEnacted++;
}

void CvLeague::DoRepealResolution(CvRepealProposal* pProposal)
{
	CvAssertMsg(pProposal->IsPassed(GetVotesSpentThisSession()), "Doing a proposal that has not been passed. Please send Anton your save file and version.");

	int iFound = 0;
	for (ActiveResolutionList::iterator it = m_vActiveResolutions.begin(); it != m_vActiveResolutions.end(); it++)
	{
		if (it->GetID() == pProposal->GetTargetResolutionID())
		{
			for (uint i = 0; i < m_vMembers.size(); i++)
			{
				it->RemoveEffects(m_vMembers[i].ePlayer);
			}
			m_vActiveResolutions.erase(it);
			//antonjs: todo: relocate these league-level effects:
			for (uint i = 0; i < m_vMembers.size(); i++)
			{
				GET_PLAYER(m_vMembers[i].ePlayer).DoUpdateHappiness();
				GET_PLAYER(m_vMembers[i].ePlayer).updateYield();
				GET_PLAYER(m_vMembers[i].ePlayer).recomputeGreatPeopleModifiers();
			}
			it--;
			iFound++;
#ifdef REPLAY_EVENTS
			std::vector<int> vArgs;
			vArgs.push_back(pProposal->GetID());
			vArgs.push_back(static_cast<int>(pProposal->GetType()));
			vArgs.push_back(static_cast<int>(false));
			GC.getGame().addReplayEvent(REPLAYEVENT_CongressEnactedProposalsChange, NO_PLAYER, vArgs);
#endif
		}
	}
	CvAssertMsg(iFound == 1, "Unexpected number of active resolutions with this ID. Please send Anton your save file and version.");
}

void CvLeague::DoClearProposals()
{
	m_vEnactProposals.clear();
	m_vRepealProposals.clear();

	// If we had proposals on hold, put them back up
	if (HasProposalsOnHold())
	{
		for (EnactProposalList::const_iterator it = m_vEnactProposalsOnHold.begin(); it != m_vEnactProposalsOnHold.end(); ++it)
		{
			m_vEnactProposals.push_back(*it);
		}
		m_vEnactProposalsOnHold.clear();

		for (RepealProposalList::const_iterator it = m_vRepealProposalsOnHold.begin(); it != m_vRepealProposalsOnHold.end(); ++it)
		{
			m_vRepealProposals.push_back(*it);
		}
		m_vRepealProposalsOnHold.clear();
	}

	GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true);
}

void CvLeague::DoPutProposalsOnHold()
{
	// Should not already have proposals on hold!
	CvAssertMsg(!HasProposalsOnHold(), "Attempting to put proposals on hold (for a special session), but proposals are already on hold. Please send Anton your save file and version.");
	m_vEnactProposalsOnHold.clear();
	m_vRepealProposalsOnHold.clear();

	for (EnactProposalList::const_iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); ++it)
	{
		m_vEnactProposalsOnHold.push_back(*it);
	}
	m_vEnactProposals.clear();

	for (RepealProposalList::const_iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); ++it)
	{
		m_vRepealProposalsOnHold.push_back(*it);
	}
	m_vRepealProposals.clear();

	GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true);
}

bool CvLeague::HasProposalsOnHold()
{
	bool bEnactProposalOnHold = !m_vEnactProposalsOnHold.empty();
	bool bRepealProposalOnHold = !m_vRepealProposalsOnHold.empty();

	return (bEnactProposalOnHold || bRepealProposalOnHold);
}

// Should be called before the list of proposals is resolved and cleared
void CvLeague::NotifySessionDone()
{
	for (EnactProposalList::iterator it = m_vEnactProposals.begin(); it != m_vEnactProposals.end(); it++)
	{
		NotifyProposalResult(it);
	}
	for (RepealProposalList::iterator it = m_vRepealProposals.begin(); it != m_vRepealProposals.end(); it++)
	{
		NotifyProposalResult(it);
	}
}

void CvLeague::NotifyProposalResult(CvEnactProposal* pProposal)
{
	CvAssert(pProposal != NULL);
	if (pProposal == NULL) return;

	CvString sSummary = "";
	CvString sMessage = "";
	if (pProposal->GetEffects()->bDiplomaticVictory)
	{
		// World Leader results are handled a little differently
		if (pProposal->IsPassed(GetVotesSpentThisSession()))
		{
			PlayerTypes eWinner = (PlayerTypes) pProposal->GetVoterDecision()->GetDecision();
			CvAssert(eWinner != NO_PLAYER);
			if (eWinner != NO_PLAYER)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_WORLD_LEADER_PASS");
				sTemp << GET_PLAYER(eWinner).getCivilizationShortDescriptionKey();
				sSummary += sTemp.toUTF8();

				sTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_WORLD_LEADER_PASS_DETAILS");
				sTemp << GC.getGame().GetVotesNeededForDiploVictory() << GET_PLAYER(eWinner).getCivilizationShortDescriptionKey();
				sMessage += sTemp.toUTF8();
			}
		}
		else
		{
			Localization::String sTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_WORLD_LEADER_FAIL");
			sSummary += sTemp.toUTF8();
			
			sTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_WORLD_LEADER_FAIL_DETAILS");
			sTemp << GC.getGame().GetVotesNeededForDiploVictory() << LeagueHelpers::NUM_LEADERS_FOR_EXTRA_VOTES;
			sMessage += sTemp.toUTF8();
		}
		sMessage += "[NEWLINE]" + pProposal->GetVoterDecision()->GetVotesAsText(this);
	}
	else if (pProposal->GetEffects()->bChangeLeagueHost)
	{
		// Change Host results are handled a little differently
		PlayerTypes eOldHost = GetHostMember();
		PlayerTypes eNewHost = eOldHost;
		CvAssert(pProposal->IsPassed(GetVotesSpentThisSession()));
		if (pProposal->IsPassed(GetVotesSpentThisSession()))
		{
			eNewHost = (PlayerTypes) pProposal->GetVoterDecision()->GetDecision();
		}

		CvString sHostKey = "Nobody";
		CvAssertMsg(eNewHost != NO_PLAYER, "Could not determine the new host. Please send Anton your save file and version.");
		if (eNewHost != NO_PLAYER)
		{
			sHostKey = GET_PLAYER(eNewHost).getCivilizationShortDescriptionKey();
		}
		
		if (eNewHost != eOldHost)
		{
			Localization::String sSummaryTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_HOST_CHANGED_SUMMARY");
			sSummaryTemp << sHostKey;
			sSummary += sSummaryTemp.toUTF8();
		}
		else
		{
			Localization::String sSummaryTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_HOST_SAME_SUMMARY");
			sSummaryTemp << sHostKey;
			sSummary += sSummaryTemp.toUTF8();
		}

		Localization::String sMessageTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_HOST_DETAILS");
		sMessageTemp << GetName() << sHostKey;
		sMessage += sMessageTemp.toUTF8();
		sMessage += "[NEWLINE]" + pProposal->GetVoterDecision()->GetVotesAsText(this);
	}
	else
	{
		Localization::String sSummaryTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_FAIL_SUMMARY");
		Localization::String sMessageTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_ENACT_FAIL");
		CvString sName = GetResolutionName(pProposal->GetType(), pProposal->GetID(), pProposal->GetProposerDecision()->GetDecision(), /*bIncludePrefix*/ true);
		if (pProposal->IsPassed(GetVotesSpentThisSession()))
		{
			sSummaryTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_PASS_SUMMARY");
			sMessageTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_ENACT_PASS");
		}
		sSummaryTemp << sName;
		sMessageTemp << sName;
		sSummary = sSummaryTemp.toUTF8();
		sMessage = sMessageTemp.toUTF8();
		sMessage += "[NEWLINE]" + pProposal->GetVoterDecision()->GetVotesAsText(this);
	}

	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		PlayerTypes eMember = it->ePlayer;
		if (GET_PLAYER(eMember).isHuman())
		{
			CvNotifications* pNotifications = GET_PLAYER(eMember).GetNotifications();
			if (pNotifications)
			{
				pNotifications->Add(NOTIFICATION_LEAGUE_VOTING_DONE, sMessage, sSummary, -1, -1, GetID());
			}				
		}
	}
}

void CvLeague::NotifyProposalResult(CvRepealProposal* pProposal)
{
	CvAssert(pProposal != NULL);
	if (pProposal == NULL) return;

	Localization::String sSummaryTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_FAIL_SUMMARY");
	Localization::String sMessageTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_REPEAL_FAIL");
	CvString sName = GetResolutionName(pProposal->GetType(), pProposal->GetID(), pProposal->GetProposerDecision()->GetDecision(), /*bIncludePrefix*/ true);
	int iTotalSessionVotes = GetVotesSpentThisSession();
	if (pProposal->IsPassed(iTotalSessionVotes))
	{
		sSummaryTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_PASS_SUMMARY");
		sMessageTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_RESULT_REPEAL_PASS");
	}
	sSummaryTemp << sName;
	sMessageTemp << sName;
	CvString sSummary = sSummaryTemp.toUTF8();
	CvString sMessage = sMessageTemp.toUTF8();
	sMessage += "[NEWLINE]" + pProposal->GetRepealDecision()->GetVotesAsText(this);

	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		PlayerTypes eMember = it->ePlayer;
		if (GET_PLAYER(eMember).isHuman())
		{
			CvNotifications* pNotifications = GET_PLAYER(eMember).GetNotifications();
			if (pNotifications)
			{
				pNotifications->Add(NOTIFICATION_LEAGUE_VOTING_DONE, sMessage, sSummary, -1, -1, GetID());
			}				
		}
	}
}

void CvLeague::NotifySessionSoon(int iTurnsLeft)
{
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		PlayerTypes eMember = it->ePlayer;
		if (GET_PLAYER(eMember).isHuman())
		{
			CvNotifications* pNotifications = GET_PLAYER(eMember).GetNotifications();
			if (pNotifications)
			{
				CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_SOON");

				Localization::String strTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTING_SOON_TT");
				strTemp << GetName() << iTurnsLeft;
				CvString strInfo = strTemp.toUTF8();

				for (EnactProposalList::iterator itProposal = m_vEnactProposals.begin(); itProposal != m_vEnactProposals.end(); ++itProposal)
				{
					strInfo += "[NEWLINE][ICON_BULLET]";
					strInfo += GetResolutionName(itProposal->GetType(), itProposal->GetID(), itProposal->GetProposerDecision()->GetDecision(), /*bIncludePrefix*/ true);
				}
				for (RepealProposalList::iterator itProposal = m_vRepealProposals.begin(); itProposal != m_vRepealProposals.end(); ++itProposal)
				{
					strInfo += "[NEWLINE][ICON_BULLET]";
					strInfo += GetResolutionName(itProposal->GetType(), itProposal->GetID(), itProposal->GetProposerDecision()->GetDecision(), /*bIncludePrefix*/ true);
				}

				pNotifications->Add(NOTIFICATION_LEAGUE_VOTING_SOON, strInfo, strSummary, -1, -1, GetID());
			}
		}
	}
}

void CvLeague::NotifyProjectComplete(LeagueProjectTypes eProject)
{
	CvLeagueProjectEntry* pInfo = GC.getLeagueProjectInfo(eProject);
	CvAssertMsg(pInfo, "Could not find League Project info when sending out a progress update. Please send Anton your save file and version.");
	if (pInfo)
	{
		for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
		{
			PlayerTypes eMember = it->ePlayer;
			CvPlayer& kPlayer = GET_PLAYER(eMember);
			if (kPlayer.isHuman())
			{
				if (kPlayer.isLocalPlayer() && !kPlayer.isObserver())
				{
					CvNotifications* pNotifications = kPlayer.GetNotifications();
					if (pNotifications)
					{
						Localization::String sSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_PROJECT_COMPLETE");
						sSummary << pInfo->GetDescriptionKey();

						Localization::String sMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_PROJECT_COMPLETE_TT");
						sMessage << pInfo->GetDescriptionKey();

						pNotifications->Add((NotificationTypes)FString::Hash("NOTIFICATION_LEAGUE_PROJECT_COMPLETE"), sMessage.toUTF8(), sSummary.toUTF8(), -1, -1, GetID(), eProject);
					}
				}
			}
		}
	}
}

void CvLeague::NotifyProjectProgress(LeagueProjectTypes eProject)
{
	CvLeagueProjectEntry* pInfo = GC.getLeagueProjectInfo(eProject);
	CvAssertMsg(pInfo, "Could not find League Project info when sending out a progress update. Please send Anton your save file and version.");
	if (pInfo)
	{
		for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); ++it)
		{
			PlayerTypes eMember = it->ePlayer;
			CvPlayer& kPlayer = GET_PLAYER(eMember);
			if (kPlayer.isHuman())
			{
				if (kPlayer.isLocalPlayer() && !kPlayer.isObserver())
				{
					CvNotifications* pNotifications = kPlayer.GetNotifications();
					if (pNotifications)
					{
						int iPercentCompleted = (int) (((float)GetProjectProgress(eProject) / (float)GetProjectCost(eProject)) * 100);
						iPercentCompleted = MIN(100, iPercentCompleted);

						Localization::String sSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_PROJECT_PROGRESS");
						sSummary << pInfo->GetDescriptionKey();

						Localization::String sMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_PROJECT_PROGRESS_TT");
						sMessage << pInfo->GetDescriptionKey() << iPercentCompleted;

						pNotifications->Add((NotificationTypes)FString::Hash("NOTIFICATION_LEAGUE_PROJECT_PROGRESS"), sMessage.toUTF8(), sSummary.toUTF8(), -1, -1, GetID(), eProject);
					}
				}
			}
		}
	}
}

void CvLeague::StartProject(LeagueProjectTypes eLeagueProject)
{
	CvAssertMsg(!IsProjectActive(eLeagueProject), "Attempting to start a League Project which is already underway. Please send Anton your save file and version.");
	if (!IsProjectActive(eLeagueProject))
	{
		Project project;
		project.eType = eLeagueProject;
		m_vProjects.push_back(project);
	}
}

void CvLeague::CheckProjectsProgress()
{
	for (ProjectList::iterator it = m_vProjects.begin(); it != m_vProjects.end(); it++)
	{
		if (IsProjectActive(it->eType) && !IsProjectComplete(it->eType))
		{
			CvLeagueProjectEntry* pProjectInfo = GC.getLeagueProjectInfo(it->eType);
			if (pProjectInfo)
			{
				// How much do we need?
				int iNeeded = GetProjectCost(it->eType);
				CvAssertMsg(iNeeded != 0, "Invalid cost for League Project. Please send Anton your save file and version.");

				// How much do we have?
				int iTotal = GetProjectProgress(it->eType);

				// Is it finished?
				if (iTotal >= iNeeded)
				{
					CvAssert(!it->bComplete);
					if (!it->bComplete)
					{
						DoProjectRewards(it->eType);
						NotifyProjectComplete(it->eType);
						it->bComplete = true;

						CvGame& kGame = GC.getGame();
						CvPlayer& kActivePlayer = GET_PLAYER(kGame.getActivePlayer());

						if(!kGame.isNetworkMultiPlayer() && !kActivePlayer.isObserver())
						{
							CvPopupInfo kPopup(BUTTONPOPUP_LEAGUE_PROJECT_COMPLETED, GetID(), it->eType);
							GC.GetEngineUserInterface()->AddPopup(kPopup);
						}
						CheckProjectAchievements();
					}
				}
				// How close is it?
				else
				{
					int iPercentCompleted = (int) (((float)iTotal / (float)iNeeded) * 100);
					iPercentCompleted = MIN(100, iPercentCompleted);

					if (!it->bProgressWarningSent && iPercentCompleted >= LeagueHelpers::PROJECT_PROGRESS_PERCENT_WARNING)
					{
						NotifyProjectProgress(it->eType);
						it->bProgressWarningSent = true;
					}
				}
			}
		}
	}
}

void CvLeague::DoProjectRewards(LeagueProjectTypes eLeagueProject)
{
	CvLeagueProjectEntry* pProjectInfo = GC.getLeagueProjectInfo(eLeagueProject);
	CvAssert(pProjectInfo);
	if (!pProjectInfo) return;

	int iTopTierRecipients = 0;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		PlayerTypes eMember = it->ePlayer;
		if (GET_PLAYER(eMember).isAlive() && !GET_PLAYER(eMember).isMinorCiv())
		{
			ContributionTier eTier = GetMemberContributionTier(eMember, eLeagueProject);

			// Break ties for top reward tier - Only one can get it, give it to the first
			if (eTier == CONTRIBUTION_TIER_3)
			{
				iTopTierRecipients++;
			}
			if (iTopTierRecipients > 1)
			{
				eTier = CONTRIBUTION_TIER_2;
				iTopTierRecipients--;
			}

			DoProjectReward(eMember, eLeagueProject, eTier);
			
			//antonjs: todo: undo rewards when killed and/or project is repeated
		}
	}
}

void CvLeague::DoProjectReward(PlayerTypes ePlayer, LeagueProjectTypes eLeagueProject, ContributionTier eTier)
{
	CvLeagueProjectEntry* pProjectInfo = GC.getLeagueProjectInfo(eLeagueProject);
	CvAssert(pProjectInfo);
	if (!pProjectInfo) return;
	
	// Which rewards do we get?
	//antonjs: A switch statement in its natural habitat without break statements...a rare sight indeed
	FStaticVector<LeagueProjectRewardTypes, NUM_CONTRIBUTION_TIERS, true, c_eCiv5GameplayDLL> veRewards;
	switch (eTier)
	{
	case CONTRIBUTION_TIER_3:
		{
			veRewards.push_back(pProjectInfo->GetRewardTier3());
		}
	case CONTRIBUTION_TIER_2:
		{
			veRewards.push_back(pProjectInfo->GetRewardTier2());
		}
	case CONTRIBUTION_TIER_1:
		{
			veRewards.push_back(pProjectInfo->GetRewardTier1());
		}
	default:
		{
			break;
		}
	}

	for (uint i = 0; i < veRewards.size(); i++)
	{
		CvLeagueProjectRewardEntry* pRewardInfo = GC.getLeagueProjectRewardInfo(veRewards[i]);
		CvAssert(pRewardInfo);
		if (!pRewardInfo) continue;

		// Free Building in Capital
		if (pRewardInfo->GetBuilding() != NO_BUILDING)
		{
			CvCity* pCapital = GET_PLAYER(ePlayer).getCapitalCity();
			CvAssertMsg(pCapital, "Player does not have a capital city. Please send Anton your save file and version.");
			if (pCapital)
			{
				CvBuildingEntry* pBuildingInfo = GC.getBuildingInfo(pRewardInfo->GetBuilding());
				if (pBuildingInfo)
				{
					pCapital->CreateBuilding(pRewardInfo->GetBuilding());
					pCapital->CleanUpQueue(); // cleans out items from the queue that may be invalidated by the recent construction
				}
			}
		}

		// Happiness
		if (pRewardInfo->GetHappiness() != 0)
		{
			GET_PLAYER(ePlayer).ChangeHappinessFromLeagues(pRewardInfo->GetHappiness());
			GET_PLAYER(ePlayer).DoUpdateHappiness();
		}

		// Free Social Policy
		if (pRewardInfo->GetFreeSocialPolicies() > 0)
		{
			GET_PLAYER(ePlayer).ChangeNumFreePolicies(pRewardInfo->GetFreeSocialPolicies());
		}

		// Temporary Culture Modifier
		if (pRewardInfo->GetCultureBonusTurns() > 0)
		{
			GET_PLAYER(ePlayer).ChangeCultureBonusTurns(pRewardInfo->GetCultureBonusTurns());
		}

		// Temporary Tourism Modifier
		if (pRewardInfo->GetTourismBonusTurns() > 0)
		{
			GET_PLAYER(ePlayer).ChangeTourismBonusTurns(pRewardInfo->GetTourismBonusTurns());
		}

		// Golden Age Points
		if (pRewardInfo->GetGoldenAgePoints() > 0)
		{
			GET_PLAYER(ePlayer).ChangeGoldenAgeProgressMeter(pRewardInfo->GetGoldenAgePoints());
		}

		// City-State Influence Boost
		//antonjs: todo: ordering, to prevent ally / no longer ally notif spam
		if (pRewardInfo->GetCityStateInfluenceBoost() > 0)
		{
			for (int iMinorCivLoop = MAX_MAJOR_CIVS; iMinorCivLoop < MAX_CIV_PLAYERS; iMinorCivLoop++)
			{
				PlayerTypes eMinorCivLoop = (PlayerTypes) iMinorCivLoop;
				if (GET_PLAYER(eMinorCivLoop).isAlive() && GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasMet(GET_PLAYER(eMinorCivLoop).getTeam()))
				{
					GET_PLAYER(eMinorCivLoop).GetMinorCivAI()->ChangeFriendshipWithMajor(ePlayer, pRewardInfo->GetCityStateInfluenceBoost());
				}
			}
		}

		// Beaker boost based on previous turns
		if (pRewardInfo->GetBaseBeakersTurnsToCount() > 0)
		{
			int iPreviousTurnsToCount = pRewardInfo->GetBaseBeakersTurnsToCount();
			int iBeakersBonus = GET_PLAYER(ePlayer).GetScienceYieldFromPreviousTurns(GC.getGame().getGameTurn(), iPreviousTurnsToCount);
			TechTypes eCurrentTech = GET_PLAYER(ePlayer).GetPlayerTechs()->GetCurrentResearch();
			if(eCurrentTech == NO_TECH)
			{
				GET_PLAYER(ePlayer).changeOverflowResearch(iBeakersBonus);
			}
			else
			{
				GET_TEAM(GET_PLAYER(ePlayer).getTeam()).GetTeamTechs()->ChangeResearchProgress(eCurrentTech, iBeakersBonus, ePlayer);
			}
		}

		// Free unit class
		if (pRewardInfo->GetFreeUnitClass() != NO_UNITCLASS)
		{
			UnitTypes eUnit = (UnitTypes) GET_PLAYER(ePlayer).getCivilizationInfo().getCivilizationUnits(pRewardInfo->GetFreeUnitClass());
			if (eUnit != NO_UNIT)
			{
				CvCity* pCapital = GET_PLAYER(ePlayer).getCapitalCity();
				if (pCapital)
				{
					CvUnit* pUnit = GET_PLAYER(ePlayer).initUnit(eUnit, pCapital->getX(), pCapital->getY());
					pUnit->jumpToNearestValidPlot();
				}
			}
		}
	}
}

void CvLeague::UpdateName()
{
	// Roll for a new name type
	int iRoll = GC.getGame().getJonRandNum(GC.getNumLeagueNameInfos(), "Rolling for league name variation");
	CvLeagueNameEntry* pInfo = GC.getLeagueNameInfo((LeagueNameTypes)iRoll);
	if (pInfo)
	{
		m_eAssignedName = (LeagueNameTypes) GC.getInfoTypeForString(pInfo->GetType());
	}
}

// Must be called while the Congress is still in session, before the proposal is deleted
void CvLeague::LogProposalResolved(CvEnactProposal* pProposal)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return;
	CvString sMessage = "";

	sMessage += ",Congress";
	sMessage += ",- - -";
	if (pProposal->IsPassed(GetVotesSpentThisSession()))
	{
		sMessage += ",Proposal Passed";
	}
	else
	{
		sMessage += ",Proposal Failed";
	}

	CvAssert(pProposal != NULL);
	if (pProposal != NULL && pProposal->GetVoterDecision() != NULL)
	{
		sMessage += ",";
		sMessage += pProposal->GetProposalName(/*bForLogging*/ true);

		sMessage += ",";
		int iDecision = pProposal->GetVoterDecision()->GetDecision();
		if (iDecision == LeagueHelpers::CHOICE_NONE)
		{
			sMessage += "None";
		}
		else
		{
			sMessage += LeagueHelpers::GetTextForChoice(pProposal->GetVoterDecision()->GetType(), iDecision);
		}
		
		CvString sTemp;
		sTemp.Format(",Won by %d", pProposal->GetVoterDecision()->GetVotesMarginOfTopChoice());
		sMessage += sTemp;
	}

	GC.getGame().GetGameLeagues()->LogLeagueMessage(sMessage);
}

// Must be called while the Congress is still in session, before the proposal is deleted
void CvLeague::LogProposalResolved(CvRepealProposal* pProposal)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return;
	CvString sMessage = "";

	sMessage += ",Congress";
	sMessage += ",- - -";
	if (pProposal->IsPassed(GetVotesSpentThisSession()))
	{
		sMessage += ",Proposal Passed";
	}
	else
	{
		sMessage += ",Proposal Failed";
	}

	CvAssert(pProposal != NULL);
	if (pProposal != NULL && pProposal->GetRepealDecision() != NULL)
	{
		sMessage += ",";
		sMessage += pProposal->GetProposalName(/*bForLogging*/ true);

		sMessage += ",";
		int iDecision = pProposal->GetRepealDecision()->GetDecision();
		if (iDecision == LeagueHelpers::CHOICE_NONE)
		{
			sMessage += "None";
		}
		else
		{
			sMessage += LeagueHelpers::GetTextForChoice(pProposal->GetRepealDecision()->GetType(), iDecision);
		}
		
		CvString sTemp;
		sTemp.Format(",Won by %d", pProposal->GetRepealDecision()->GetVotesMarginOfTopChoice());
		sMessage += sTemp;
	}

	GC.getGame().GetGameLeagues()->LogLeagueMessage(sMessage);
}

CvLeague::Member* CvLeague::GetMember(PlayerTypes ePlayer)
{
	Member* pFound = NULL;
	for (MemberList::iterator it = m_vMembers.begin(); it != m_vMembers.end(); it++)
	{
		if (it->ePlayer == ePlayer)
		{
			pFound = it;
		}
	}
	CvAssertMsg(pFound != NULL, "Could not retrieve member based on player ID. Please send Anton your save file and version.");
	return pFound;
}

CvLeague::Project* CvLeague::GetProject(LeagueProjectTypes eLeagueProject)
{
	Project* pFound = NULL;
	for (ProjectList::iterator it = m_vProjects.begin(); it != m_vProjects.end(); it++)
	{
		if (it->eType == eLeagueProject)
		{
			pFound = it;
		}
	}
	CvAssertMsg(pFound != NULL, "Could not retrieve project based on project type. Please send Anton your save file and version.");
	return pFound;
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvLeague& writeTo)
{
	uint uiVersion;

	loadFrom >> uiVersion;
	loadFrom >> writeTo.m_eID;
	if (uiVersion >= 4)
	{
		loadFrom >> writeTo.m_bUnitedNations;
	}
	else
	{
		writeTo.m_bUnitedNations = false;
	}
	if (uiVersion >= 2)
	{
		loadFrom >> writeTo.m_bInSession;
	}
	else
	{
		writeTo.m_bInSession = false;
	}
#ifdef POLICY_LEAGUE_SESSION_YIELD_BOOST_PER_DELEGATE
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= 1000)
	{
#endif
	loadFrom >> writeTo.m_bResolveSession;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		writeTo.m_bResolveSession = false;
	}
#endif
#endif
	loadFrom >> writeTo.m_iTurnsUntilSession;
	loadFrom >> writeTo.m_iNumResolutionsEverEnacted;
	
	int iNumEnactProposals;
	loadFrom >> iNumEnactProposals;
	for (int i = 0; i < iNumEnactProposals; i++)
	{
		CvEnactProposal temp;
		loadFrom >> temp;
		writeTo.m_vEnactProposals.push_back(temp);
	}

	int iNumRepealProposals;
	loadFrom >> iNumRepealProposals;
	for (int i = 0; i < iNumRepealProposals; i++)
	{
		CvRepealProposal temp;
		loadFrom >> temp;
		writeTo.m_vRepealProposals.push_back(temp);
	}

	int iNumActiveResolutions;
	loadFrom >> iNumActiveResolutions;
	for (int i = 0; i < iNumActiveResolutions; i++)
	{
		CvActiveResolution temp;
		loadFrom >> temp;
		writeTo.m_vActiveResolutions.push_back(temp);
	}

	int iNumMembers;
	loadFrom >> iNumMembers;
	for (int i = 0; i < iNumMembers; i++)
	{
		CvLeague::Member temp;
		loadFrom >> temp.ePlayer;
		if (uiVersion >= 9)
		{
			loadFrom >> temp.iExtraVotes;
		}
		else
		{
			temp.iExtraVotes = 0;
		}
		if (uiVersion >= 10)
		{
			loadFrom >> temp.sVoteSources;
		}
		else
		{
			temp.sVoteSources = "";
		}
		loadFrom >> temp.bMayPropose;
		if (uiVersion >= 12)
		{
			loadFrom >> temp.iProposals;
		}
		else
		{
			temp.iProposals = 0;
		}
		loadFrom >> temp.iVotes;
		if (uiVersion >= 13)
		{
			loadFrom >> temp.iAbstainedVotes;
		}
		else
		{
			temp.iAbstainedVotes = 0;
		}
		if (uiVersion >= 14)
		{
			loadFrom >> temp.bEverBeenHost;
			loadFrom >> temp.bAlwaysBeenHost;
		}
		else
		{
			temp.bEverBeenHost = false;
			temp.bAlwaysBeenHost = true;
		}
		writeTo.m_vMembers.push_back(temp);
	}
	if (uiVersion >= 3)
	{
		loadFrom >> writeTo.m_eHost;
	}
	else
	{
		writeTo.m_eHost = NO_PLAYER;
	}
	if (uiVersion >= 5)
	{
		int iNumProjects;
		loadFrom >> iNumProjects;
		for (int iOuter = 0; iOuter < iNumProjects; iOuter++)
		{
			CvLeague::Project temp;
			loadFrom >> temp.eType;
			int iListSize;
			loadFrom >> iListSize;
			for (int iInner = 0; iInner < iListSize; iInner++)
			{
				int iContribution;
				loadFrom >> iContribution;
				temp.vProductionList[iInner] = iContribution;
			}
			if (uiVersion >= 6)
			{
				loadFrom >> temp.bComplete;
			}
			else
			{
				temp.bComplete = false;
			}
			if (uiVersion >= 11)
			{
				loadFrom >> temp.bProgressWarningSent;
			}
			else
			{
				temp.bProgressWarningSent = false;
			}
			writeTo.m_vProjects.push_back(temp);
		}
	}
	else
	{
		writeTo.m_vProjects.clear();
	}
	if (uiVersion >= 7)
	{
		loadFrom >> writeTo.m_iConsecutiveHostedSessions;
		loadFrom >> writeTo.m_eAssignedName;
		ZeroMemory(writeTo.m_szCustomName, sizeof(writeTo.m_szCustomName));
		loadFrom >> writeTo.m_szCustomName;
	}
	else
	{
		writeTo.m_iConsecutiveHostedSessions = 0;
		writeTo.m_eAssignedName = NO_LEAGUE_NAME;
		ZeroMemory(writeTo.m_szCustomName, sizeof(writeTo.m_szCustomName));
	}
	if (uiVersion >= 8)
	{
		loadFrom >> writeTo.m_eLastSpecialSession;

		loadFrom >> writeTo.m_eCurrentSpecialSession;

		int iNumEnactProposalsOnHold;
		loadFrom >> iNumEnactProposalsOnHold;
		for (int i = 0; i < iNumEnactProposalsOnHold; i++)
		{
			CvEnactProposal temp;
			loadFrom >> temp;
			writeTo.m_vEnactProposalsOnHold.push_back(temp);
		}

		int iNumRepealProposalsOnHold;
		loadFrom >> iNumRepealProposalsOnHold;
		for (int i = 0; i < iNumRepealProposalsOnHold; i++)
		{
			CvRepealProposal temp;
			loadFrom >> temp;
			writeTo.m_vRepealProposalsOnHold.push_back(temp);
		}
	}
	else
	{
		writeTo.m_eLastSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
		writeTo.m_eCurrentSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
		writeTo.m_vEnactProposalsOnHold.clear();
		writeTo.m_vRepealProposalsOnHold.clear();
	}

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvLeague& readFrom)
{
	uint uiVersion = 14;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	uiVersion = BUMP_SAVE_VERSION_VOTING_CLASSES;
#endif
	
	saveTo << uiVersion;
	saveTo << readFrom.m_eID;
	saveTo << readFrom.m_bUnitedNations;
	saveTo << readFrom.m_bInSession;
#ifdef POLICY_LEAGUE_SESSION_YIELD_BOOST_PER_DELEGATE
	saveTo << readFrom.m_bResolveSession;
#endif
	saveTo << readFrom.m_iTurnsUntilSession;
	saveTo << readFrom.m_iNumResolutionsEverEnacted;
	saveTo << readFrom.m_vEnactProposals.size();
	for (EnactProposalList::const_iterator it = readFrom.m_vEnactProposals.begin(); it != readFrom.m_vEnactProposals.end(); it++)
	{
		saveTo << *it;
	}
	saveTo << readFrom.m_vRepealProposals.size();
	for (RepealProposalList::const_iterator it = readFrom.m_vRepealProposals.begin(); it != readFrom.m_vRepealProposals.end(); it++)
	{
		saveTo << *it;
	}
	saveTo << readFrom.m_vActiveResolutions.size();
	for (ActiveResolutionList::const_iterator it = readFrom.m_vActiveResolutions.begin(); it != readFrom.m_vActiveResolutions.end(); it++)
	{
		saveTo << *it;
	}
	saveTo << readFrom.m_vMembers.size();
	for (CvLeague::MemberList::const_iterator it = readFrom.m_vMembers.begin(); it != readFrom.m_vMembers.end(); it++)
	{
		saveTo << it->ePlayer;
		saveTo << it->iExtraVotes;
		saveTo << it->sVoteSources;
		saveTo << it->bMayPropose;
		saveTo << it->iProposals;
		saveTo << it->iVotes;
		saveTo << it->iAbstainedVotes;
		saveTo << it->bEverBeenHost;
		saveTo << it->bAlwaysBeenHost;
	}
	saveTo << readFrom.m_eHost;
	saveTo << readFrom.m_vProjects.size();
	for (CvLeague::ProjectList::const_iterator it = readFrom.m_vProjects.begin(); it != readFrom.m_vProjects.end(); it++)
	{
		saveTo << it->eType;
		saveTo << it->vProductionList.size();
		for (CvLeague::ProjectProductionList::const_iterator innerIt = it->vProductionList.begin(); innerIt != it->vProductionList.end(); innerIt++)
		{
			saveTo << *innerIt;
		}
		saveTo << it->bComplete;
		saveTo << it->bProgressWarningSent;
	}
	saveTo << readFrom.m_iConsecutiveHostedSessions;
	saveTo << readFrom.m_eAssignedName;
	saveTo << readFrom.m_szCustomName;
	saveTo << readFrom.m_eLastSpecialSession;
	saveTo << readFrom.m_eCurrentSpecialSession;
	saveTo << readFrom.m_vEnactProposalsOnHold.size();
	for (EnactProposalList::const_iterator it = readFrom.m_vEnactProposalsOnHold.begin(); it != readFrom.m_vEnactProposalsOnHold.end(); ++it)
	{
		saveTo << *it;
	}
	saveTo << readFrom.m_vRepealProposalsOnHold.size();
	for (RepealProposalList::const_iterator it = readFrom.m_vRepealProposalsOnHold.begin(); it != readFrom.m_vRepealProposalsOnHold.end(); ++it)
	{
		saveTo << *it;
	}

	return saveTo;
}


// ================================================================================
//			CvGameLeagues
// ================================================================================
CvGameLeagues::CvGameLeagues(void)
{
	m_vActiveLeagues.clear();
	m_iNumLeaguesEverFounded = 0;
	m_eDiplomaticVictor = NO_PLAYER;
	m_iGeneratedIDCount = 0;
	m_eLastEraTrigger = NO_ERA;
}

CvGameLeagues::~CvGameLeagues(void)
{
}

void CvGameLeagues::Init()
{
}

void CvGameLeagues::DoTurn()
{
#ifdef DUEL_MOVING_SOME_OPTIONS_TO_DUEL_MODE
	if (!GC.getGame().isOption("GAMEOPTION_DUEL_STUFF") || !GC.getGame().isOption(GAMEOPTION_NO_LEAGUES))
#else
	if (!GC.getGame().isOption(GAMEOPTION_NO_LEAGUES))
#endif
	{
		GC.getGame().DoUpdateDiploVictory();

		// Not yet founded - is it time to start?
		if (GetNumActiveLeagues() == 0)
		{
#ifdef AUI_VOTING_RANDOMIZED_LEAGUE_FOUNDER
			FStaticVector<PlayerTypes, MAX_MAJOR_CIVS, true> vePossibleFounders;
#endif
			// Has any living major civ met every other living major civ?
			for (int iCiv = 0; iCiv < MAX_MAJOR_CIVS; iCiv++)
			{
				PlayerTypes eCiv = (PlayerTypes) iCiv;
				if (GET_PLAYER(eCiv).isAlive())
				{
					// Has the unlock from tech?
					if (GET_TEAM(GET_PLAYER(eCiv).getTeam()).HasTechForWorldCongress())
					{
						// Met every other civ?
						bool bMetEveryone = true;
						for (int iOtherCiv = 0; iOtherCiv < MAX_MAJOR_CIVS; iOtherCiv++)
						{
							PlayerTypes eOtherCiv = (PlayerTypes) iOtherCiv;
							if (eCiv != eOtherCiv && GET_PLAYER(eOtherCiv).isAlive())
							{
								if (!GET_TEAM(GET_PLAYER(eCiv).getTeam()).isHasMet(GET_PLAYER(eOtherCiv).getTeam()))
								{
									bMetEveryone = false;
									break;
								}
							}
						}

						if (bMetEveryone && GC.getGame().countMajorCivsAlive() > 1)
						{
#ifdef AUI_VOTING_RANDOMIZED_LEAGUE_FOUNDER
#ifdef AUI_VOTING_RANDOMIZED_LEAGUE_FOUNDER_OPTION
							if (GC.getGame().isOption("GAMEOPTION_SIMULTANEOUS_PLAYER_TURN_ACTIVATION_ORDER_RANDOMIZED"))
							{
								vePossibleFounders.push_back(eCiv);
							}
							else
							{
								FoundLeague(eCiv);
								break;
							}
#else
							vePossibleFounders.push_back(eCiv);
#endif
#else
							FoundLeague(eCiv);
							break;
#endif
						}
					}
				}
			}
#ifdef AUI_VOTING_RANDOMIZED_LEAGUE_FOUNDER
#ifdef AUI_VOTING_RANDOMIZED_LEAGUE_FOUNDER_OPTION
			if (GC.getGame().isOption("GAMEOPTION_SIMULTANEOUS_PLAYER_TURN_ACTIVATION_ORDER_RANDOMIZED") && vePossibleFounders.size() > 0)
#else
			if (vePossibleFounders.size() > 0)
#endif
			{
				FoundLeague(vePossibleFounders.at(uint(GC.getGame().getJonRandNum(int(vePossibleFounders.size()), NULL))));
			}
#endif
		}
		// Already founded - do we want to trigger a special session, or just a normal turn?
		else
		{
			EraTypes eGameEra = LeagueHelpers::GetGameEraForTrigger();
			LeagueSpecialSessionTypes eSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
			if (eGameEra > GetLastEraTrigger())
			{
				for (int i = 0; i < GC.getNumLeagueSpecialSessionInfos(); i++)
				{
					CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo((LeagueSpecialSessionTypes)i);
					if (pInfo != NULL)
					{
						EraTypes eSessionTrigger = pInfo->GetEraTrigger();
						if (eSessionTrigger <= eGameEra && eSessionTrigger > GetLastEraTrigger())
						{
							eSpecialSession = (LeagueSpecialSessionTypes)i;
						}
					}
				}
			}

			CvLeague* pLeague = GetActiveLeague();
			CvAssert(pLeague != NULL);
			if (pLeague != NULL)
			{
				// We want to trigger a special session
				if (eSpecialSession != NO_LEAGUE_SPECIAL_SESSION)
				{
					// And we can
					if (pLeague->CanStartSpecialSession(eSpecialSession))
					{
						pLeague->DoTurn(eSpecialSession);

						// Flag this era as the last era we did a special session
						CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eSpecialSession);
						CvAssert(pInfo != NULL);
						if (pInfo != NULL)
						{
							SetLastEraTrigger(pInfo->GetEraTrigger());
						}

						LogSpecialSession(eSpecialSession);
					}
					// Have to wait
					else
					{
						pLeague->DoTurn();
					}
				}
				// Just a normal turn
				else
				{
					pLeague->DoTurn();
				}
			}
		}
	}
	else
	{
		VictoryTypes eDiploVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);
		if (eDiploVictory != NO_VICTORY)
		{
			CvAssertMsg(!GC.getGame().isVictoryValid(eDiploVictory), "Diplomacy victory is valid, but leagues are disabled.  Please send Anton your save file and version.");
		}
	}
}

void CvGameLeagues::DoPlayerTurn(CvPlayer& kPlayer)
{
#ifdef DUEL_MOVING_SOME_OPTIONS_TO_DUEL_MODE
	if (!GC.getGame().isOption("GAMEOPTION_DUEL_STUFF") || !GC.getGame().isOption(GAMEOPTION_NO_LEAGUES))
#else
	if (!GC.getGame().isOption(GAMEOPTION_NO_LEAGUES))
#endif
	{
		AI_PERF_FORMAT("AI-perf.csv", ("CvGameLeagues::DoPlayerTurn, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), kPlayer.getCivilizationShortDescription()) );
		for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
		{
			if (it->IsMember(kPlayer.GetID()))
			{
				// Call for Proposals
				if (it->CanPropose(kPlayer.GetID()))
				{
					if (kPlayer.isHuman())
					{
						CvNotifications* pNotifications = kPlayer.GetNotifications();
						if (pNotifications)
						{
							CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_LEAGUE_PROPOSALS_NEEDED");

							Localization::String strTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_PROPOSALS_NEEDED_TT");
							strTemp << it->GetName();
							CvString strInfo = strTemp.toUTF8();

							pNotifications->Add(NOTIFICATION_LEAGUE_CALL_FOR_PROPOSALS, strInfo, strSummary, -1, -1, it->GetID());
						}				
					}
					else
					{
						kPlayer.GetLeagueAI()->DoProposals(it);
					}
				}
				// Call for Votes and other Session actions
				else if (it->IsInSession())
				{
					// Honor vote commitments made in deals before spending any other votes
					kPlayer.GetLeagueAI()->DoVoteCommitments(it);

					// Allocate free votes
					if (it->CanVote(kPlayer.GetID()))
					{
						if (it->IsAnythingProposed())
						{
							if (kPlayer.isHuman())
							{
								CvNotifications* pNotifications = kPlayer.GetNotifications();
								if (pNotifications)
								{
									CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_LEAGUE_VOTES_NEEDED");

									Localization::String strTemp = Localization::Lookup("TXT_KEY_NOTIFICATION_LEAGUE_VOTES_NEEDED_TT");
									strTemp << it->GetName();
									CvString strInfo = strTemp.toUTF8();

									pNotifications->Add(NOTIFICATION_LEAGUE_CALL_FOR_VOTES, strInfo, strSummary, -1, -1, it->GetID());
								}
							}
							else
							{
								kPlayer.GetLeagueAI()->DoVotes(it);
							}
						}
						else
						{
							kPlayer.GetLeagueAI()->DoAbstainAllVotes(it);
						}
					}
				}
			}
		}
	}
}

void CvGameLeagues::FoundLeague(PlayerTypes eFounder)
{
#ifdef DUEL_MOVING_SOME_OPTIONS_TO_DUEL_MODE
	if (!GC.getGame().isOption("GAMEOPTION_DUEL_STUFF") || !GC.getGame().isOption(GAMEOPTION_NO_LEAGUES))
#else
	if (!GC.getGame().isOption(GAMEOPTION_NO_LEAGUES))
#endif
	{
		CvAssertMsg(GetNumActiveLeagues() == 0, "Trying to found a second league when one is already active. Please send Anton your save file and version.");
		if (GetNumActiveLeagues() == 0)
		{
			CvLeague league((LeagueTypes)m_iNumLeaguesEverFounded++);
			
			// Add all living players as members
			for (int i = 0; i < MAX_CIV_PLAYERS; i++)
			{
				PlayerTypes ePlayer = (PlayerTypes) i;
				if (GET_PLAYER(ePlayer).isAlive())
				{
					league.AddMember(ePlayer);
				}
			}
			
			// Optional: Set initial host
			if (eFounder != NO_PLAYER)
			{
				league.SetHostMember(eFounder);
			}

			// Find which game era trigger this league begins at (must match with a special session)
			EraTypes eEarliestEraTrigger = NO_ERA;
			EraTypes eLatestEraTrigger = NO_ERA;
			for (int i = 0; i < GC.getNumLeagueSpecialSessionInfos(); i++)
			{
				CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo((LeagueSpecialSessionTypes)i);
				if (pInfo != NULL)
				{
					if (eEarliestEraTrigger == NO_ERA || (int)pInfo->GetEraTrigger() < (int)eEarliestEraTrigger)
					{
						eEarliestEraTrigger = pInfo->GetEraTrigger();
					}
					if ((int)pInfo->GetEraTrigger() > (int)eLatestEraTrigger)
					{
						eLatestEraTrigger = pInfo->GetEraTrigger();
					}
				}
			}
			// In case the game era is actually before or after the triggers in the database
			EraTypes eGoverningEraTrigger = (EraTypes) MAX((int)LeagueHelpers::GetGameEraForTrigger(), (int)eEarliestEraTrigger);
			eGoverningEraTrigger = (EraTypes) MIN((int)eGoverningEraTrigger, (int)eLatestEraTrigger);

			// Find which special session info this league begins at
			LeagueSpecialSessionTypes eGoverningSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
			bool bBeginAsUnitedNations = false;
			for (int i = 0; i < GC.getNumLeagueSpecialSessionInfos(); i++)
			{
				LeagueSpecialSessionTypes e = (LeagueSpecialSessionTypes)i;
				CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(e);
				if (pInfo != NULL)
				{
					if (pInfo->GetEraTrigger() == eGoverningEraTrigger)
					{
						eGoverningSpecialSession = e;
						bBeginAsUnitedNations = pInfo->IsUnitedNations();
						break;
					}
				}
			}
			CvAssertMsg(eGoverningSpecialSession != NO_LEAGUE_SPECIAL_SESSION, "Trying to found a league but could not determine a correct governing special session. Do the World Congress prerequisite tech and the special session era triggers in the database match? Please send Anton your save file and version.");
			CvAssertMsg(eGoverningEraTrigger != NO_ERA, "Trying to found a league but could not determine a correct governing era trigger. Do the World Congress prerequisite tech and the special session era triggers in the database match? Please send Anton your save file and version.");
			league.Init(eGoverningSpecialSession);
			SetLastEraTrigger(eGoverningEraTrigger);

			CvGame& kGame = GC.getGame();
			CvPlayer& kActivePlayer = GET_PLAYER(kGame.getActivePlayer());

			if(!kGame.isNetworkMultiPlayer() && !kActivePlayer.isObserver()){
				// Show splash screen
				CvPopupInfo kPopup(BUTTONPOPUP_LEAGUE_SPLASH, league.GetID(), league.GetHostMember(), eGoverningSpecialSession, 0, /*bJustFounded*/ true);
				GC.GetEngineUserInterface()->AddPopup(kPopup);
			}

			DLLUI->AddMessage(0, GC.getGame().getActivePlayer(), false, GC.getEVENT_MESSAGE_TIME(), 
													GetLocalizedText("TXT_KEY_LEAGUE_SPECIAL_SESSION_START_WORLD_CONGRESS").GetCString());

			// Begins as United Nations?
			if (bBeginAsUnitedNations)
			{
				league.SetUnitedNations(true);
			}

			m_vActiveLeagues.push_back(league);

			LogLeagueFounded(eFounder);
		}
	}
}

void CvGameLeagues::DoPlayerAliveStatusChanged(PlayerTypes ePlayer)
{
	bool bAlive = GET_PLAYER(ePlayer).isAlive();
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (!bAlive && it->IsMember(ePlayer))
		{
			it->RemoveMember(ePlayer);
		}
		else if (bAlive)
		{
			it->AddMember(ePlayer);
		}
	}

	GC.getGame().DoUpdateDiploVictory();
}

void CvGameLeagues::DoUnitedNationsBuilt(PlayerTypes /*eBuilder*/)
{
}

int CvGameLeagues::GetNumActiveLeagues()
{
	return m_vActiveLeagues.size();
}

int CvGameLeagues::GetNumLeaguesEverFounded() const
{
	return m_iNumLeaguesEverFounded;
}

EraTypes CvGameLeagues::GetLastEraTrigger() const
{
	return m_eLastEraTrigger;
}

void CvGameLeagues::SetLastEraTrigger(EraTypes eEraTrigger)
{
	m_eLastEraTrigger = eEraTrigger;
}

CvLeague* CvGameLeagues::GetLeague(LeagueTypes eLeague)
{
	CvLeague* pLeague = NULL;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->GetID() == eLeague)
		{
			pLeague = it;
		}
	}
	CvAssertMsg(pLeague != NULL, "CvLeague is NULL. Please send Anton your save file and version.");
	return pLeague;
}

CvLeague* CvGameLeagues::GetActiveLeague()
{
	CvLeague* pLeague = NULL;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		pLeague = it;
	}
	CvAssertMsg(GetNumActiveLeagues() == 0 || GetNumActiveLeagues() == 1, "Unexpected number of active leagues. Please send Anton your save file and version.");
	return pLeague;
}

int CvGameLeagues::GenerateResolutionUniqueID()
{
	return ++m_iGeneratedIDCount;
}

bool CvGameLeagues::CanContributeToLeagueProject(PlayerTypes ePlayer, LeagueProjectTypes eLeagueProject)
{
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->CanMemberContribute(ePlayer, eLeagueProject))
		{
			return true;
		}
	}
	return false;
}

void CvGameLeagues::DoLeagueProjectContribution(PlayerTypes ePlayer, LeagueProjectTypes eLeagueProject, int iValue)
{
	int iMatches = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->CanMemberContribute(ePlayer, eLeagueProject))
		{
			iMatches++;
			it->ChangeMemberContribution(ePlayer, eLeagueProject, iValue);
		}
	}
	CvAssertMsg(iMatches == 1, "Unexpected case when contributing to a League Project. Please send Anton your save file and version.");
}

PlayerTypes CvGameLeagues::GetDiplomaticVictor() const
{
	return m_eDiplomaticVictor;
}

void CvGameLeagues::SetDiplomaticVictor(PlayerTypes ePlayer)
{
	// There can be only one
	CvAssertMsg(m_eDiplomaticVictor == NO_PLAYER, "Player voted Diplomatic Victor when there already was one. Please send Anton your save file and version.");
	if (m_eDiplomaticVictor == NO_PLAYER)
	{
		m_eDiplomaticVictor = ePlayer;
		GC.getGame().testVictory();
	}
}

bool CvGameLeagues::IsTradeEmbargoed(PlayerTypes eTrader, PlayerTypes eRecipient)
{
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->IsMember(eTrader))
		{
			if (it->IsTradeEmbargoed(eTrader, eRecipient))
			{
				return true;
			}
		}
	}
	return false;
}

bool CvGameLeagues::IsLuxuryHappinessBanned(PlayerTypes ePlayer, ResourceTypes eLuxury)
{
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->IsMember(ePlayer))
		{
			if (it->IsLuxuryHappinessBanned(eLuxury))
			{
				return true;
			}
		}
	}
	return false;
}

int CvGameLeagues::GetResearchMod(PlayerTypes ePlayer, TechTypes eTech)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->IsMember(ePlayer))
		{
			int iLeagueMod = it->GetResearchMod(eTech);
			if (iLeagueMod != 0)
			{
				iValue += iLeagueMod;
			}
		}
	}
	return iValue;
}

int CvGameLeagues::GetFeatureYieldChange(PlayerTypes ePlayer, FeatureTypes eFeature, YieldTypes eYield)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->IsMember(ePlayer))
		{
			int iLeagueMod = it->GetFeatureYieldChange(eFeature, eYield);
			if (iLeagueMod != 0)
			{
				iValue += iLeagueMod;
			}
		}
	}
	return iValue;
}

int CvGameLeagues::GetWorldWonderYieldChange(PlayerTypes ePlayer, YieldTypes eYield)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->IsMember(ePlayer))
		{
			int iLeagueMod = it->GetWorldWonderYieldChange(eYield);
			if (iLeagueMod != 0)
			{
				iValue += iLeagueMod;
			}
		}
	}
	return iValue;
}

bool CvGameLeagues::IsNoTrainingNuclearWeapons(PlayerTypes ePlayer)
{
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->IsMember(ePlayer))
		{
			if (it->IsNoTrainingNuclearWeapons())
			{
				return true;
			}
		}
	}
	return false;
}

int CvGameLeagues::GetCityTourismModifier(PlayerTypes ePlayer, const CvCity* pCity)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); ++it)
	{
		if (it->IsMember(ePlayer))
		{
			int iLeagueMod = it->GetCityTourismModifier(pCity);
			if (iLeagueMod != 0)
			{
				iValue += iLeagueMod;
			}
		}
	}
	return iValue;
}

int CvGameLeagues::GetReligionSpreadStrengthModifier(PlayerTypes ePlayer, ReligionTypes eReligion)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); ++it)
	{
		if (it->IsMember(ePlayer))
		{
			int iLeagueMod = it->GetReligionSpreadStrengthModifier(eReligion);
			if (iLeagueMod != 0)
			{
				iValue += iLeagueMod;
			}
		}
	}
	return iValue;
}

int CvGameLeagues::GetPressureForIdeology(PlayerTypes ePlayer, PolicyBranchTypes eIdeology)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); ++it)
	{
		if (it->IsMember(ePlayer))
		{
			int iLeagueMod = it->GetPressureForIdeology(eIdeology);
			if (iLeagueMod != 0)
			{
				iValue += iLeagueMod;
			}
		}
	}
	return iValue;
}

int CvGameLeagues::GetArtsyGreatPersonRateModifier(PlayerTypes ePlayer)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); ++it)
	{
		if (it->IsMember(ePlayer))
		{
			int iMod = it->GetArtsyGreatPersonRateModifier();
			if (iMod != 0)
			{
				iValue += iMod;
			}
		}
	}
	return iValue;
}

int CvGameLeagues::GetScienceyGreatPersonRateModifier(PlayerTypes ePlayer)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); ++it)
	{
		if (it->IsMember(ePlayer))
		{
			int iMod = it->GetScienceyGreatPersonRateModifier();
			if (iMod != 0)
			{
				iValue += iMod;
			}
		}
	}
	return iValue;
}

#ifdef NEW_LEAGUE_RESOLUTIONS
int CvGameLeagues::GetTradeRouteGoldModifier(PlayerTypes ePlayer)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); ++it)
	{
		if (it->IsMember(ePlayer))
		{
			int iMod = it->GetTradeRouteGoldModifier();
			if (iMod != 0)
			{
				iValue += iMod;
			}
		}
	}
	return iValue;
}

int CvGameLeagues::GetCSBonuModifier(PlayerTypes ePlayer)
{
	int iValue = 0;
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); ++it)
	{
		if (it->IsMember(ePlayer))
		{
			int iMod = it->GetCSBonuModifier();
			if (iMod != 0)
			{
				iValue += iMod;
			}
		}
	}
	return iValue;
}

bool CvGameLeagues::IsNoSpiesInCS(PlayerTypes ePlayer)
{
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->IsMember(ePlayer))
		{
			if (it->IsNoSpiesInCS())
			{
				return true;
			}
		}
	}
	return false;
}

bool CvGameLeagues::IsDoubleResourceHappiness(PlayerTypes ePlayer, ResourceTypes eLuxury)
{
	for (LeagueList::iterator it = m_vActiveLeagues.begin(); it != m_vActiveLeagues.end(); it++)
	{
		if (it->IsMember(ePlayer))
		{
			if (it->IsDoubleResourceHappiness(eLuxury))
			{
				return true;
			}
		}
	}
	return false;
}
#endif

CvString CvGameLeagues::GetLogFileName() const
{
	CvString strLogName;
	strLogName = "WorldCongressLog.csv";
	return strLogName;
}

void CvGameLeagues::LogLeagueMessage(CvString& kMessage)
{
	if(GC.getLogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		FILogFile* pLog;

		pLog = LOGFILEMGR.GetLog(GetLogFileName(), FILogFile::kDontTimeStamp);

		// Get the leading info for this line
		strBaseString.Format("%03d, %d", GC.getGame().getElapsedGameTurns(), GC.getGame().getGameTurnYear());
		strOutBuf = strBaseString + kMessage;
		pLog->Msg(strOutBuf);
	}
}

void CvGameLeagues::LogLeagueFounded(PlayerTypes eFounder)
{
	CvString sMessage = "";

	sMessage += ",Congress";
	sMessage += ",- - -";
	sMessage += ",Congress Founded";
	if (eFounder != NO_PLAYER)
	{
		CvPlayer* pFounder = &GET_PLAYER(eFounder);
		if (pFounder != NULL)
		{
			sMessage += ",";
			sMessage += pFounder->getCivilizationShortDescription();
		}
	}

	LogLeagueMessage(sMessage);
}

void CvGameLeagues::LogSpecialSession(LeagueSpecialSessionTypes eSpecialSession)
{
	CvString sMessage = "";

	sMessage += ",Congress";
	sMessage += ",- - -";
	sMessage += ",Special Session";
	CvAssert(eSpecialSession != NO_LEAGUE_SPECIAL_SESSION);
	if (eSpecialSession != NO_LEAGUE_SPECIAL_SESSION)
	{
		CvLeagueSpecialSessionEntry* pSpecialSessionInfo = GC.getLeagueSpecialSessionInfo(eSpecialSession);
		CvAssert(pSpecialSessionInfo);
		if (pSpecialSessionInfo)
		{
			CvEraInfo* pEraInfo = GC.getEraInfo(pSpecialSessionInfo->GetEraTrigger());
			if (pEraInfo)
			{
				sMessage += ",";
				sMessage += pEraInfo->GetDescription();
			}
		}
	}

	LogLeagueMessage(sMessage);
}

// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvGameLeagues& writeTo)
{
	uint uiVersion;
	int iNumLeagues;

	loadFrom >> uiVersion;
	if (uiVersion >= 4)
	{
		loadFrom >> writeTo.m_iGeneratedIDCount;
	}
	else
	{
		writeTo.m_iGeneratedIDCount = 0;
	}
	loadFrom >> iNumLeagues;
	writeTo.m_vActiveLeagues.clear();
	for (int iLeague = 0; iLeague < iNumLeagues; iLeague++)
	{
		CvLeague tempLeague;
		loadFrom >> tempLeague;
		writeTo.m_vActiveLeagues.push_back(tempLeague);
	}
	if (uiVersion >= 2)
	{
		loadFrom >> writeTo.m_iNumLeaguesEverFounded;
	}
	else
	{
		writeTo.m_iNumLeaguesEverFounded = 0;
	}
	if (uiVersion >= 3)
	{
		loadFrom >> writeTo.m_eDiplomaticVictor;
	}
	else
	{
		writeTo.m_eDiplomaticVictor = NO_PLAYER;
	}
	if (uiVersion >= 5)
	{
		loadFrom >> writeTo.m_eLastEraTrigger;
	}
	else
	{
		writeTo.m_eLastEraTrigger = NO_ERA;
	}

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvGameLeagues& readFrom)
{
	uint uiVersion = 5;
	
	saveTo << uiVersion;
	saveTo << readFrom.m_iGeneratedIDCount;
	saveTo << readFrom.m_vActiveLeagues.size();
	for (uint iLeague = 0; iLeague < readFrom.m_vActiveLeagues.size(); iLeague++)
	{
		saveTo << readFrom.m_vActiveLeagues[iLeague];
	}
	saveTo << readFrom.m_iNumLeaguesEverFounded;
	saveTo << readFrom.m_eDiplomaticVictor;
	saveTo << readFrom.m_eLastEraTrigger;
	
	return saveTo;
}


// ================================================================================
//			CvLeagueAI
// ================================================================================
CvLeagueAI::CvLeagueAI(void)
{
	m_pPlayer = NULL;
}

CvLeagueAI::~CvLeagueAI(void)
{
	Uninit();
}

// Initialize class data
void CvLeagueAI::Init(CvPlayer* pPlayer)
{
	m_pPlayer = pPlayer;

	Reset();
}

// Cleanup
void CvLeagueAI::Uninit()
{

}

void CvLeagueAI::Reset()
{
	m_vVoteCommitmentList.clear();
}

void CvLeagueAI::Read(FDataStream& kStream)
{
	uint uiVersion;
	kStream >> uiVersion;

	if (uiVersion >= 2)
	{
		m_vVoteCommitmentList.clear();
		uint uiNumVoteCommitments;
		kStream >> uiNumVoteCommitments;
		for (uint i = 0; i < uiNumVoteCommitments; i++)
		{
			VoteCommitment temp;
			kStream >> temp.eToPlayer;
			kStream >> temp.iResolutionID;
			kStream >> temp.iVoteChoice;
			kStream >> temp.iNumVotes;
			kStream >> temp.bEnact;
			m_vVoteCommitmentList.push_back(temp);
		}
	}
	else
	{
		m_vVoteCommitmentList.clear();
	}
}

void CvLeagueAI::Write(FDataStream& kStream)
{
	uint uiVersion = 2;
	kStream << uiVersion;

	kStream << m_vVoteCommitmentList.size();
	for (VoteCommitmentList::iterator it = m_vVoteCommitmentList.begin(); it != m_vVoteCommitmentList.end(); ++it)
	{
		kStream << it->eToPlayer;
		kStream << it->iResolutionID;
		kStream << it->iVoteChoice;
		kStream << it->iNumVotes;
		kStream << it->bEnact;
	}
}

void CvLeagueAI::DoTurn()
{
	if (m_pPlayer->isHuman())
	{
		return;
	}

	// Evaluate current situation

	// Queue requests for Diplo system
}

void CvLeagueAI::DoVotes(CvLeague* pLeague)
{
	AI_PERF_FORMAT("AI-perf.csv", ("CvLeagueAI::DoVotes, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription()) );

	int iAttempts = 0;
	while (pLeague->CanVote(GetPlayer()->GetID()))
	{
		AllocateVotes(pLeague);

		iAttempts++;
		if (iAttempts > 100)
		{
			CvAssertMsg(false, "AI unable to finish doing votes. Please send Anton your save file and version.");
			DoAbstainAllVotes(pLeague);
			break;
		}
	}
}

void CvLeagueAI::DoAbstainAllVotes(CvLeague* pLeague)
{
	pLeague->DoVoteAbstain(GetPlayer()->GetID(), pLeague->GetRemainingVotesForMember(GetPlayer()->GetID()));
}

void CvLeagueAI::DoProposals(CvLeague* pLeague)
{
	int iAttempts = 0;
	while (pLeague->CanPropose(GetPlayer()->GetID()))
	{
		AllocateProposals(pLeague);
		iAttempts++;
		if (iAttempts > 100)
		{
			CvAssertMsg(false, "AI unable to finish doing proposals. Please send Anton your save file and version.");
			break;
		}
	}
}

CvLeagueAI::VoteCommitmentList CvLeagueAI::GetDesiredVoteCommitments(PlayerTypes eFromPlayer)
{
	VoteCommitmentList vDesired;

	if (GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0)
	{
		CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
		if (pLeague)
		{
			// For human players, make assumptions
			if (GetPlayer()->isHuman())
			{
				EnactProposalList vEnactProposals = pLeague->GetEnactProposals();
				for (EnactProposalList::iterator it = vEnactProposals.begin(); it != vEnactProposals.end(); ++it)
				{
					int iDesiredChoice = LeagueHelpers::CHOICE_NONE;

					// Normal Proposals
					if (it->GetVoterDecision()->GetType() == RESOLUTION_DECISION_YES_OR_NO)
					{
						if (it->GetProposalPlayer() == GetPlayer()->GetID())
						{
							iDesiredChoice = LeagueHelpers::CHOICE_YES;
						}
						if (it->GetEffects()->bEmbargoPlayer && it->GetProposerDecision()->GetDecision() == GetPlayer()->GetID())
						{
							iDesiredChoice = LeagueHelpers::CHOICE_NO;
						}
					}
					// Proposals voting on a player
					else if (it->GetVoterDecision()->GetType() == RESOLUTION_DECISION_ANY_MEMBER ||
							it->GetVoterDecision()->GetType() == RESOLUTION_DECISION_MAJOR_CIV_MEMBER ||
							it->GetVoterDecision()->GetType() == RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER)
					{
						if (it->GetEffects()->bChangeLeagueHost)
						{
							iDesiredChoice = GetPlayer()->GetID();
						}
						if (it->GetEffects()->bDiplomaticVictory)
						{
							iDesiredChoice = GetPlayer()->GetID();
						}
					}
					else
					{
						CvAssertMsg(false, "Unexpected case when evaluating a possible Delegate trade with a human player. Please send Anton your save file and version.");
					}

					if (iDesiredChoice != LeagueHelpers::CHOICE_NONE)
					{
						VoteCommitment temp;
						temp.iResolutionID = it->GetID();
						temp.iNumVotes = pLeague->GetCoreVotesForMember(eFromPlayer);
						temp.iVoteChoice = iDesiredChoice;
						temp.bEnact = true;
						vDesired.push_back(temp);
					}
				}

				RepealProposalList vRepealProposals = pLeague->GetRepealProposals();
				for (RepealProposalList::iterator it = vRepealProposals.begin(); it != vRepealProposals.end(); ++it)
				{
					int iDesiredChoice = LeagueHelpers::CHOICE_NONE;

					// Normal Proposals
					if (it->GetRepealDecision()->GetType() == RESOLUTION_DECISION_REPEAL)
					{
						if (it->GetProposalPlayer() == GetPlayer()->GetID())
						{
							iDesiredChoice = LeagueHelpers::CHOICE_YES;
						}
						if (it->GetEffects()->bEmbargoPlayer && it->GetProposerDecision()->GetDecision() == GetPlayer()->GetID())
						{
							iDesiredChoice = LeagueHelpers::CHOICE_YES;
						}
					}
					else
					{
						CvAssertMsg(false, "Unexpected case when evaluating a possible Delegate trade with a human player. Please send Anton your save file and version.");
					}

					if (iDesiredChoice != LeagueHelpers::CHOICE_NONE)
					{
						VoteCommitment temp;
						temp.iResolutionID = it->GetID();
						temp.iNumVotes = pLeague->GetCoreVotesForMember(eFromPlayer);
						temp.iVoteChoice = iDesiredChoice;
						temp.bEnact = false;
						vDesired.push_back(temp);
					}
				}
			}
			// For AI players, use calculated value
			else
			{
				EnactProposalList vEnactProposals = pLeague->GetEnactProposals();
				for (EnactProposalList::iterator it = vEnactProposals.begin(); it != vEnactProposals.end(); ++it)
				{
					std::vector<int> vChoices = pLeague->GetChoicesForDecision(it->GetVoterDecision()->GetType(), eFromPlayer);
					int iDesiredChoice = LeagueHelpers::CHOICE_NONE;
					DesireLevels eHighestDesire = DESIRE_NEUTRAL;
					for (uint i = 0; i < vChoices.size(); i++)
					{
						int iChoice = vChoices[i];
						DesireLevels eDesire = EvaluateVoteForTrade(it->GetID(), iChoice, pLeague->GetCoreVotesForMember(eFromPlayer), /*bRepeal*/false);
						if (eDesire > eHighestDesire)
						{
							iDesiredChoice = iChoice;
							eHighestDesire = eDesire;
						}
					}

					if (iDesiredChoice != LeagueHelpers::CHOICE_NONE)
					{
						VoteCommitment temp;
						temp.iResolutionID = it->GetID();
						temp.iNumVotes = pLeague->GetCoreVotesForMember(eFromPlayer);
						temp.iVoteChoice = iDesiredChoice;
						temp.bEnact = true;
						vDesired.push_back(temp);
					}
				}

				RepealProposalList vRepealProposals = pLeague->GetRepealProposals();
				for (RepealProposalList::iterator it = vRepealProposals.begin(); it != vRepealProposals.end(); ++it)
				{
					std::vector<int> vChoices = pLeague->GetChoicesForDecision(it->GetRepealDecision()->GetType(), eFromPlayer);
					int iDesiredChoice = LeagueHelpers::CHOICE_NONE;
					DesireLevels eHighestDesire = DESIRE_NEUTRAL;
					for (uint i = 0; i < vChoices.size(); i++)
					{
						int iChoice = vChoices[i];
						DesireLevels eDesire = EvaluateVoteForTrade(it->GetID(), iChoice, pLeague->GetCoreVotesForMember(eFromPlayer), /*bRepeal*/true);
						if (eDesire > eHighestDesire)
						{
							iDesiredChoice = iChoice;
							eHighestDesire = eDesire;
						}
					}

					if (iDesiredChoice != LeagueHelpers::CHOICE_NONE)
					{
						VoteCommitment temp;
						temp.iResolutionID = it->GetID();
						temp.iNumVotes = pLeague->GetCoreVotesForMember(eFromPlayer);
						temp.iVoteChoice = iDesiredChoice;
						temp.bEnact = false;
						vDesired.push_back(temp);
					}
				}
			}
		}
	}

	return vDesired;
}

bool CvLeagueAI::HasVoteCommitment() const
{
	return (!m_vVoteCommitmentList.empty());
}

// Have we committed votes to a player on a particular proposal?
int CvLeagueAI::GetVoteCommitment(PlayerTypes eToPlayer, int iResolutionID, int iVoteChoice, bool bEnact)
{
	int iNumVotes = 0;
	for (VoteCommitmentList::iterator it = m_vVoteCommitmentList.begin(); it != m_vVoteCommitmentList.end(); ++it)
	{
		if (it->eToPlayer == eToPlayer && it->iResolutionID == iResolutionID && it->iVoteChoice == iVoteChoice && it->bEnact == bEnact)
		{
			iNumVotes += it->iNumVotes;
		}
	}
	return iNumVotes;
}

// Can someone secure our vote on a certain matter through a deal?
bool CvLeagueAI::CanCommitVote(PlayerTypes eToPlayer, CvString* sTooltipSink)
{
	CvAssertMsg(eToPlayer >= 0, "eToPlayer is expected to be non-negative (invalid Index). Please send Anton your save file and version.");
	CvAssertMsg(eToPlayer < MAX_MAJOR_CIVS, "eToPlayer is expected to be within maximum bounds (invalid Index). Please send Anton your save file and version.");
	if (eToPlayer < 0 || eToPlayer >= MAX_MAJOR_CIVS) return false;

	bool bCanCommit = true;
	
	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
	if (pLeague == NULL)
	{
		bCanCommit = false;
		if (sTooltipSink != NULL)
			(*sTooltipSink) += Localization::Lookup("TXT_KEY_DIPLO_VOTE_TRADE_NO_LEAGUE").toUTF8();
	}
	else
	{
		// League is in session
		if (pLeague->IsInSession())
		{
			bCanCommit = false;
			if (sTooltipSink != NULL)
			{
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_DIPLO_VOTE_TRADE_IN_SESSION").toUTF8();
			}
		}

		// They don't have a diplomat in our capital
		if (!DEBUG_LEAGUES)
		{
			if (!GetPlayer()->GetEspionage()->IsOtherDiplomatVisitingMe(eToPlayer))
			{
				bCanCommit = false;
				if (sTooltipSink != NULL)
				{
					if (GC.getGame().getActivePlayer() == GetPlayer()->GetID())
					{
						(*sTooltipSink) += Localization::Lookup("TXT_KEY_DIPLO_VOTE_TRADE_NO_SPY_ACTIVE_PLAYER").toUTF8();
					}
					else
					{
						(*sTooltipSink) += Localization::Lookup("TXT_KEY_DIPLO_VOTE_TRADE_NO_SPY").toUTF8();
					}
				}
			}
		}

		// Not a member
		if (!pLeague->IsMember(GetPlayer()->GetID()))
		{
			bCanCommit = false;
			if (sTooltipSink != NULL)
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_DIPLO_VOTE_TRADE_NOT_MEMBER").toUTF8();
		}

		// Not allowed to vote
		if (!pLeague->CanEverVote(GetPlayer()->GetID()))
		{
			bCanCommit = false;
		}

		// No proposals made yet
		if (!pLeague->IsAnythingProposed())
		{
			bCanCommit = false;
			if (sTooltipSink != NULL)
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_DIPLO_VOTE_TRADE_NO_PROPOSALS").toUTF8();
		}

		// We already committed our votes
		if (HasVoteCommitment())
		{
			bCanCommit = false;
			if (sTooltipSink != NULL)
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_DIPLO_VOTE_TRADE_ALREADY_COMMITTED").toUTF8();
		}

		// The league is in session, and we already used our votes
		if (pLeague->IsInSession() && pLeague->GetRemainingVotesForMember(GetPlayer()->GetID()) < pLeague->GetCoreVotesForMember(GetPlayer()->GetID()))
		{
			bCanCommit = false;
			if (sTooltipSink != NULL)
				(*sTooltipSink) += Localization::Lookup("TXT_KEY_DIPLO_VOTE_TRADE_ALREADY_USED_VOTES").toUTF8();
		}
	}

	return bCanCommit;
}

// Someone secured our vote on a certain matter through a deal
void CvLeagueAI::AddVoteCommitment(PlayerTypes eToPlayer, int iResolutionID, int iVoteChoice, int iNumVotes, bool bRepeal)
{
	VoteCommitment sCommitment(eToPlayer, iResolutionID, iVoteChoice, iNumVotes, !bRepeal);
	m_vVoteCommitmentList.push_back(sCommitment);
}

// Something caused all commitments to be cancelled (ie. war)
void CvLeagueAI::CancelVoteCommitmentsToPlayer(PlayerTypes eToPlayer)
{
	for (VoteCommitmentList::iterator it = m_vVoteCommitmentList.begin(); it != m_vVoteCommitmentList.end(); ++it)
	{
		if (it->eToPlayer == eToPlayer)
		{
			m_vVoteCommitmentList.erase(it);
			it--;
		}
	}
}

// Honor our vote commitments
void CvLeagueAI::DoVoteCommitments(CvLeague* pLeague)
{
	for (VoteCommitmentList::iterator it = m_vVoteCommitmentList.begin(); it != m_vVoteCommitmentList.end(); ++it)
	{
		CvAssertMsg(pLeague->CanVote(GetPlayer()->GetID()), "Trying to honor vote commitments but not able to vote. Please send Anton your save file and version.");
		CvAssertMsg(pLeague->GetRemainingVotesForMember(GetPlayer()->GetID()) >= it->iNumVotes, "Trying to honor vote commitments but not enough votes. Please send Anton your save file and version.");
		bool bProcessed = false;
		if (pLeague->CanVote(GetPlayer()->GetID()))
		{
			// Proposal is up, time to honor the commitment
			if (pLeague->IsProposed(it->iResolutionID, !it->bEnact, /*bCheckOnHold*/ false))
			{
				if (it->bEnact)
				{
					pLeague->DoVoteEnact(it->iResolutionID, GetPlayer()->GetID(), it->iNumVotes, it->iVoteChoice);
					bProcessed = true;
				}
				else
				{
					pLeague->DoVoteRepeal(it->iResolutionID, GetPlayer()->GetID(), it->iNumVotes, it->iVoteChoice);
					bProcessed = true;
				}
			}
			// Proposal is on hold (ex. special session), so keep the commitment for once it comes back up
			else if (pLeague->IsProposed(it->iResolutionID, !it->bEnact, /*bCheckOnHold*/ true))
			{	
			}
			// Proposal is no longer around (ex. invalidated by a player being killed)
			else
			{
				bProcessed = true;
			}
		}

		// Cleanup
		if (bProcessed)
		{
			m_vVoteCommitmentList.erase(it);
			it--;
		}
	}
}

// How much do we like this vote commitment (either from us to someone else, or from someone else to us)?
CvLeagueAI::DesireLevels CvLeagueAI::EvaluateVoteForTrade(int iResolutionID, int iVoteChoice, int iNumVotes, bool bRepeal)
{
	DEBUG_VARIABLE(iNumVotes);
	DesireLevels eValue = DESIRE_NEVER;
	
	if (GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0)
	{
		CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
		CvAssert(pLeague != NULL);
		if (pLeague != NULL)
		{
			CvAssert(iNumVotes == pLeague->GetCoreVotesForMember(GetPlayer()->GetID()));
			if (pLeague->IsProposed(iResolutionID, bRepeal))
			{
				// Vote to repeal
				if (bRepeal)
				{
					RepealProposalList vProposals = pLeague->GetRepealProposals();
					for (RepealProposalList::iterator it = vProposals.begin(); it != vProposals.end(); ++it)
					{
						if (it->GetID() == iResolutionID)
						{
							eValue = EvaluateDesire(ScoreVoteChoice(it, iVoteChoice));
							break;
						}
					}
				}
				// Vote to enact
				else
				{
					EnactProposalList vProposals = pLeague->GetEnactProposals();
					for (EnactProposalList::iterator it = vProposals.begin(); it != vProposals.end(); ++it)
					{
						if (it->GetID() == iResolutionID)
						{
							eValue = EvaluateDesire(ScoreVoteChoice(it, iVoteChoice));
							break;
						}
					}
				}
			}
		}
	}

	return eValue;
}

// How much do we like an enact proposal, so that we can give a hint to the player making proposals?
CvLeagueAI::DesireLevels CvLeagueAI::EvaluateProposalForProposer(CvLeague* pLeague, PlayerTypes /*eProposer*/, ResolutionTypes eResolution, int iProposerChoice)
{
	CvAssert(pLeague);
	if (!pLeague)
	{
		return DESIRE_NEUTRAL;
	}
	DesireLevels eDesire = EvaluateDesire(ScoreProposal(pLeague, eResolution, iProposerChoice));

	return eDesire;
}

// How much do we like a repeal proposal, so that we can give a hint to the player making proposals?
CvLeagueAI::DesireLevels CvLeagueAI::EvaluateProposalForProposer(CvLeague* pLeague, PlayerTypes /*eProposer*/, int iTargetResolutionID)
{
	CvAssert(pLeague);
	if (!pLeague)
	{
		return DESIRE_NEUTRAL;
	}

	DesireLevels eDesire = DESIRE_NEUTRAL;
	bool bFound = false;
	ActiveResolutionList vActiveResolutions = pLeague->GetActiveResolutions();
	for (ActiveResolutionList::iterator it = vActiveResolutions.begin(); it != vActiveResolutions.end(); ++it)
	{
		if (it->GetID() == iTargetResolutionID)
		{
			eDesire = EvaluateDesire(ScoreProposal(pLeague, it));
			bFound = true;
			break;
		}
	}
	CvAssert(bFound);

	return eDesire;
}

// Calculate how much we think our interests are aligned with ePlayer, based on ideology, liberation, past voting patterns, etc.
CvLeagueAI::AlignmentLevels CvLeagueAI::EvaluateAlignment(PlayerTypes ePlayer)
{
	if (ePlayer == NO_PLAYER)
	{
		return ALIGNMENT_NEUTRAL;
	}
	if (!GET_PLAYER(ePlayer).isAlive())
	{
		return ALIGNMENT_NEUTRAL;
	}
	if (GET_PLAYER(ePlayer).isMinorCiv())
	{
		return ALIGNMENT_NEUTRAL;
	}
	if (GetPlayer()->GetID() == ePlayer)
	{
		return ALIGNMENT_SELF;
	}
	if (GetPlayer()->GetDiplomacyAI()->WasResurrectedBy(ePlayer))
	{
		return ALIGNMENT_LIBERATOR;
	}
	if (GetPlayer()->getTeam() == GET_PLAYER(ePlayer).getTeam())
	{
		if (GET_TEAM(GetPlayer()->getTeam()).getLeaderID() == ePlayer)
		{
			return ALIGNMENT_LEADER;
		}
		else
		{
			return ALIGNMENT_ALLY;
		}
	}
	if (GET_TEAM(GetPlayer()->getTeam()).isAtWar(GET_PLAYER(ePlayer).getTeam()))
	{
		return ALIGNMENT_WAR;
	}

	int iAlignment = 0;

	// Ideology
	PolicyBranchTypes eOurIdeology = GetPlayer()->GetPlayerPolicies()->GetLateGamePolicyTree();
	PolicyBranchTypes eTheirIdeology = GET_PLAYER(ePlayer).GetPlayerPolicies()->GetLateGamePolicyTree();
	if (eOurIdeology != NO_POLICY_BRANCH_TYPE && eTheirIdeology != NO_POLICY_BRANCH_TYPE)
	{
		if (eOurIdeology == eTheirIdeology)
		{
			iAlignment += 2;
		}
		else
		{
			iAlignment += -2;
		}
	}

	// Did they help us become host?
	if (GetPlayer()->GetDiplomacyAI()->GetSupportedMyHostingScore(ePlayer) != 0)
	{
		iAlignment += 2;
	}

	// Do we like their proposals or votes?
	if (GetPlayer()->GetDiplomacyAI()->GetLikedTheirProposalScore(ePlayer) != 0 ||
		GetPlayer()->GetDiplomacyAI()->GetSupportedMyProposalScore(ePlayer) != 0)
	{
		iAlignment += 2;
	}
	if (GetPlayer()->GetDiplomacyAI()->GetDislikedTheirProposalScore(ePlayer) != 0 ||
		GetPlayer()->GetDiplomacyAI()->GetFoiledMyProposalScore(ePlayer) != 0)
	{
		iAlignment += -2;
	}

	// DoF or Denounce
	if (GetPlayer()->GetDiplomacyAI()->IsDoFAccepted(ePlayer))
	{
		iAlignment += 1;
	}
	if (GetPlayer()->GetDiplomacyAI()->IsDenouncedPlayer(ePlayer))
	{
		iAlignment += -1;
	}

	// Opinion and approach
	MajorCivOpinionTypes eOpinion = GetPlayer()->GetDiplomacyAI()->GetMajorCivOpinion(ePlayer);
	MajorCivApproachTypes eApproach = GetPlayer()->GetDiplomacyAI()->GetMajorCivApproach(ePlayer, /*bHideTrueFeelings*/ false);
	if (eOpinion > MAJOR_CIV_OPINION_NEUTRAL || eApproach == MAJOR_CIV_APPROACH_AFRAID || eApproach == MAJOR_CIV_APPROACH_FRIENDLY)
	{
		iAlignment += 1;
	}
	else if (eOpinion <= MAJOR_CIV_OPINION_ENEMY || eApproach == MAJOR_CIV_APPROACH_HOSTILE)
	{
		iAlignment += -1;
	}

	AlignmentLevels eAlignment = ALIGNMENT_NEUTRAL;
	if (iAlignment < -6)
	{
		eAlignment = ALIGNMENT_ENEMY;
	}
	else if (iAlignment < -4)
	{
		eAlignment = ALIGNMENT_HATRED;
	}
	else if (iAlignment < -1)
	{
		eAlignment = ALIGNMENT_RIVAL;
	}
	else if (iAlignment <= 1)
	{
		eAlignment = ALIGNMENT_NEUTRAL;
	}
	else if (iAlignment <= 4)
	{
		eAlignment = ALIGNMENT_FRIEND;
	}
	else if (iAlignment <= 6)
	{
		eAlignment = ALIGNMENT_CONFIDANT;
	}
	else
	{
		eAlignment = ALIGNMENT_ALLY;
	}
	return eAlignment;
}

CvLeagueAI::KnowledgeLevels CvLeagueAI::GetKnowledgeGivenToOtherPlayer(PlayerTypes eToPlayer, CvString* sTooltipSink)
{
	// Shared Ideology
	PolicyBranchTypes eMyIdeology = GetPlayer()->GetPlayerPolicies()->GetLateGamePolicyTree();
	PolicyBranchTypes eTheirIdeology = GET_PLAYER(eToPlayer).GetPlayerPolicies()->GetLateGamePolicyTree();
	bool bShareIdeology = ((eMyIdeology == eTheirIdeology) && (eMyIdeology != NO_POLICY_BRANCH_TYPE) && (eTheirIdeology != NO_POLICY_BRANCH_TYPE));

	// Espionage
	bool bSpyVisitingUs = GetPlayer()->GetEspionage()->IsOtherDiplomatVisitingMe(eToPlayer);

	// Find the knowledge level
	KnowledgeLevels eKnowledge = KNOWLEDGE_NONE;
	if (bShareIdeology && bSpyVisitingUs)
	{
		eKnowledge = KNOWLEDGE_INTIMATE;
	}
	else if (bShareIdeology || bSpyVisitingUs)
	{
		eKnowledge = KNOWLEDGE_PARTIAL;
	}

	// Exceptions
	if (DEBUG_LEAGUES)
	{
		eKnowledge = KNOWLEDGE_INTIMATE;
	}
	if (GetPlayer()->isHuman())
	{
		// Human player intentions are never known
		eKnowledge = KNOWLEDGE_NONE;
	}

	// Tooltip
	if (sTooltipSink != NULL)
	{
		switch (eKnowledge)
		{
		case KNOWLEDGE_INTIMATE:
			(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_INTRIGUE_INTIMATE").toUTF8();
			break;
		case KNOWLEDGE_PARTIAL:
			(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_INTRIGUE_PARTIAL").toUTF8();
			break;
		case KNOWLEDGE_NONE:
		default:
			(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_INTRIGUE_NONE").toUTF8();
			break;
		}
		
		if (bShareIdeology)
		{
			(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_INTRIGUE_IDEOLOGY").toUTF8();
		}
		else
		{
			(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_INTRIGUE_NO_IDEOLOGY").toUTF8();
		}

		if (bSpyVisitingUs)
		{
			(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_INTRIGUE_SPY").toUTF8();
		}
		else
		{
			(*sTooltipSink) += Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_INTRIGUE_NO_SPY").toUTF8();
		}
	}

	return eKnowledge;
}

int CvLeagueAI::EvaluateVoteForOtherPlayerKnowledge(CvLeague* pLeague, PlayerTypes eToPlayer, CvEnactProposal* pProposal, CvString* sTooltipSink)
{
	CvAssert(pLeague);
	CvAssert(pProposal);
	if (!pLeague || !pProposal)
	{
		return LeagueHelpers::CHOICE_NONE;
	}

	// What is our preferred choice on this proposal?
	int iTopChoice = LeagueHelpers::CHOICE_NONE;
	int iTopChoiceScore = MIN_INT;
	std::vector<int> vChoices = pLeague->GetChoicesForDecision(pProposal->GetVoterDecision()->GetType(), GetPlayer()->GetID());
	for (uint i = 0; i < vChoices.size(); i++)
	{
		int iChoice = vChoices[i];
		int iChoiceScore = ScoreVoteChoice(pProposal, iChoice);
		if (iChoiceScore > iTopChoiceScore)
		{
			iTopChoice = iChoice;
			iTopChoiceScore = iChoiceScore;
		}
	}
	CvAssert(iTopChoice != LeagueHelpers::CHOICE_NONE);

	// How much are we telling them?
	int iRevealedChoice = LeagueHelpers::CHOICE_NONE;
	KnowledgeLevels eKnowledge = GetKnowledgeGivenToOtherPlayer(eToPlayer);
	switch (eKnowledge)
	{
	case KNOWLEDGE_NONE:
		{
			if (sTooltipSink != NULL)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_OPINIONS_VOTE_NONE");
				sTemp << pProposal->GetProposalName();
				(*sTooltipSink) += sTemp.toUTF8();
			}
			break;
		}
	case KNOWLEDGE_PARTIAL:
		{	
			iRevealedChoice = iTopChoice;
			if (sTooltipSink != NULL)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_OPINIONS_VOTE_PARTIAL");
				sTemp << pProposal->GetProposalName();
				sTemp << LeagueHelpers::GetTextForChoice(pProposal->GetVoterDecision()->GetType(), iTopChoice);
				(*sTooltipSink) += sTemp.toUTF8();
			}
			break;
		}
	case KNOWLEDGE_INTIMATE:
		{
			iRevealedChoice = iTopChoice;
			if (sTooltipSink != NULL)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_OPINIONS_VOTE_INTIMATE");
				sTemp << pProposal->GetProposalName();
				sTemp << LeagueHelpers::GetTextForChoice(pProposal->GetVoterDecision()->GetType(), iTopChoice);
				sTemp << GetTextForDesire(EvaluateDesire(iTopChoiceScore));
				(*sTooltipSink) += sTemp.toUTF8();
			}
			break;
		}
	}

	return iRevealedChoice;
}

int CvLeagueAI::EvaluateVoteForOtherPlayerKnowledge(CvLeague* pLeague, PlayerTypes eToPlayer, CvRepealProposal* pProposal, CvString* sTooltipSink)
{
	CvAssert(pLeague);
	CvAssert(pProposal);
	if (!pLeague || !pProposal)
	{
		return LeagueHelpers::CHOICE_NONE;
	}

	// What is our preferred choice on this proposal?
	int iTopChoice = LeagueHelpers::CHOICE_NONE;
	int iTopChoiceScore = 0;
	std::vector<int> vChoices = pLeague->GetChoicesForDecision(pProposal->GetRepealDecision()->GetType(), GetPlayer()->GetID());
	for (uint i = 0; i < vChoices.size(); i++)
	{
		int iChoice = vChoices[i];
		int iChoiceScore = ScoreVoteChoice(pProposal, iChoice);
		if (iChoiceScore > iTopChoiceScore)
		{
			iTopChoice = iChoice;
			iTopChoiceScore = iChoiceScore;
		}
	}

	// How much are we telling them?
	int iRevealedChoice = LeagueHelpers::CHOICE_NONE;
	KnowledgeLevels eKnowledge = GetKnowledgeGivenToOtherPlayer(eToPlayer);
	switch (eKnowledge)
	{
	case KNOWLEDGE_NONE:
		{
			if (sTooltipSink != NULL)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_OPINIONS_VOTE_NONE");
				sTemp << pProposal->GetProposalName();
				(*sTooltipSink) += sTemp.toUTF8();
			}
			break;
		}
	case KNOWLEDGE_PARTIAL:
		{	
			iRevealedChoice = iTopChoice;
			if (sTooltipSink != NULL)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_OPINIONS_VOTE_PARTIAL");
				sTemp << pProposal->GetProposalName();
				sTemp << LeagueHelpers::GetTextForChoice(pProposal->GetRepealDecision()->GetType(), iTopChoice);
				(*sTooltipSink) += sTemp.toUTF8();
			}
			break;
		}
	case KNOWLEDGE_INTIMATE:
		{
			iRevealedChoice = iTopChoice;
			if (sTooltipSink != NULL)
			{
				Localization::String sTemp = Localization::Lookup("TXT_KEY_LEAGUE_OVERVIEW_MEMBER_OPINIONS_VOTE_INTIMATE");
				sTemp << pProposal->GetProposalName();
				sTemp << LeagueHelpers::GetTextForChoice(pProposal->GetRepealDecision()->GetType(), iTopChoice);
				sTemp << GetTextForDesire(EvaluateDesire(iTopChoiceScore));
				(*sTooltipSink) += sTemp.toUTF8();
			}
			break;
		}
	}

	return iRevealedChoice;
}

CvLeagueAI::DiplomatUsefulnessLevels CvLeagueAI::GetDiplomatUsefulnessAtCiv(PlayerTypes ePlayer)
{
	DiplomatUsefulnessLevels eUsefulness = DIPLOMAT_USEFULNESS_NONE;

	int iScore = 0;
	if (GetExtraVotesPerDiplomat() > 0)
	{
		iScore += 1;
	}
	if (EvaluateAlignment(ePlayer) > ALIGNMENT_NEUTRAL)
	{
		iScore += 1;
	}

	if (iScore >= 2)
	{
		eUsefulness = DIPLOMAT_USEFULNESS_MEDIUM;
	}
	else if (iScore >= 1)
	{
		eUsefulness = DIPLOMAT_USEFULNESS_LOW;
	}
	
	return eUsefulness;
}

// Wrapper
int CvLeagueAI::GetExtraVotesPerDiplomat()
{
	return GetPlayer()->GetExtraVotesPerDiplomat();
}

int CvLeagueAI::GetExtraVotesPerCityStateAlly()
{
	int iVotes = 0;

	if (GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0)
	{
		CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
		if (pLeague)
		{
			LeagueSpecialSessionTypes eGoverningSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
			if (pLeague->GetCurrentSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
			{
				eGoverningSpecialSession = pLeague->GetCurrentSpecialSession();
			}
			else if (pLeague->GetLastSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
			{
				eGoverningSpecialSession = pLeague->GetLastSpecialSession();
			}

			CvAssert(eGoverningSpecialSession != NO_LEAGUE_SPECIAL_SESSION);
			if (eGoverningSpecialSession != NO_LEAGUE_SPECIAL_SESSION)
			{
				CvLeagueSpecialSessionEntry* pInfo = GC.getLeagueSpecialSessionInfo(eGoverningSpecialSession);
				CvAssert(pInfo != NULL);
				if (pInfo != NULL) 
				{
					iVotes += pInfo->GetCityStateDelegates();
				}
			}
		}
	}

	return iVotes;
}

CvString CvLeagueAI::GetCommitVoteDetails(PlayerTypes eToPlayer)
{
	CvString s = "";

	CanCommitVote(eToPlayer, &s);

	return s;
}

CvPlayer* CvLeagueAI::GetPlayer()
{
	return m_pPlayer;
}

CvLeagueAI::VoteCommitment::VoteCommitment()
{
}

CvLeagueAI::VoteCommitment::VoteCommitment(PlayerTypes player, int resolutionID, int voteChoice, int numVotes, bool enact)
{
	eToPlayer = player;
	iResolutionID = resolutionID;
	iVoteChoice = voteChoice;
	iNumVotes = numVotes;
	bEnact = enact;
}

CvLeagueAI::VoteCommitment::~VoteCommitment()
{
}

CvLeagueAI::VoteConsideration::VoteConsideration()
{
}

CvLeagueAI::VoteConsideration::VoteConsideration(bool enact, int id, int choice)
{
	bEnact = enact;
	iID = id;
	iChoice = choice;
}

CvLeagueAI::VoteConsideration::~VoteConsideration()
{
}

CvLeagueAI::ProposalConsideration::ProposalConsideration()
{
}

CvLeagueAI::ProposalConsideration::ProposalConsideration(bool enact, int index, int choice)
{
	bEnact = enact;
	iIndex = index;
	iChoice = choice;
}

CvLeagueAI::ProposalConsideration::~ProposalConsideration()
{
}

CvString CvLeagueAI::GetTextForDesire(DesireLevels eDesire)
{
	CvString s = "";

	switch (eDesire)
	{
	case DESIRE_NEVER:
		s = Localization::Lookup("TXT_KEY_LEAGUE_DESIRE_NEVER").toUTF8();
		break;
	case DESIRE_STRONG_DISLIKE:
		s = Localization::Lookup("TXT_KEY_LEAGUE_DESIRE_STRONG_DISLIKE").toUTF8();
		break;
	case DESIRE_DISLIKE:
		s = Localization::Lookup("TXT_KEY_LEAGUE_DESIRE_DISLIKE").toUTF8();
		break;
	case DESIRE_WEAK_DISLIKE:
		s = Localization::Lookup("TXT_KEY_LEAGUE_DESIRE_WEAK_DISLIKE").toUTF8();
		break;
	case DESIRE_NEUTRAL:
		s = Localization::Lookup("TXT_KEY_LEAGUE_DESIRE_NEUTRAL").toUTF8();
		break;
	case DESIRE_WEAK_LIKE:
		s = Localization::Lookup("TXT_KEY_LEAGUE_DESIRE_WEAK_LIKE").toUTF8();
		break;
	case DESIRE_LIKE:
		s = Localization::Lookup("TXT_KEY_LEAGUE_DESIRE_LIKE").toUTF8();
		break;
	case DESIRE_STRONG_LIKE:
		s = Localization::Lookup("TXT_KEY_LEAGUE_DESIRE_STRONG_LIKE").toUTF8();
		break;
	case DESIRE_ALWAYS:
		s = Localization::Lookup("TXT_KEY_LEAGUE_DESIRE_ALWAYS").toUTF8();
		break;
	}

	return s;
}

CvLeagueAI::DesireLevels CvLeagueAI::EvaluateDesire(int iRawScore)
{
	DesireLevels eDesire = DESIRE_NEVER;

	if (iRawScore < -105)
	{
		eDesire = DESIRE_NEVER;
	}
	else if (iRawScore < -75)
	{
		eDesire = DESIRE_STRONG_DISLIKE;
	}
	else if (iRawScore < -45)
	{
		eDesire = DESIRE_DISLIKE;
	}
	else if (iRawScore < -15)
	{
		eDesire = DESIRE_WEAK_DISLIKE;
	}
	else if (iRawScore <= 15)
	{
		eDesire = DESIRE_NEUTRAL;
	}
	else if (iRawScore <= 45)
	{
		eDesire = DESIRE_WEAK_LIKE;
	}
	else if (iRawScore <= 75)
	{
		eDesire = DESIRE_LIKE;
	}
	else if (iRawScore <= 105)
	{
		eDesire = DESIRE_STRONG_LIKE;
	}
	else
	{
		eDesire = DESIRE_ALWAYS;
	}

	return eDesire;
}

// Find the most preferred use of our votes in the session of pLeague, and allocate them with some randomness
void CvLeagueAI::AllocateVotes(CvLeague* pLeague)
{
	CvAssert(pLeague != NULL);
	if (!(pLeague != NULL)) 
		return;

	int iVotes = pLeague->GetRemainingVotesForMember(GetPlayer()->GetID());
	VoteConsiderationList vConsiderations;
	int iFocusResolutionID = -1;

	EnactProposalList vEnactProposals = pLeague->GetEnactProposals();
	for (EnactProposalList::iterator it = vEnactProposals.begin(); it != vEnactProposals.end(); ++it)
	{
		// Special case - If an embargo on us is proposed, use all our Delegates towards its outcome
		if (it->GetEffects()->bEmbargoPlayer && it->GetProposerDecision()->GetDecision() == GetPlayer()->GetID())
		{
			iFocusResolutionID = it->GetID();
		}

		FindBestVoteChoices(it, vConsiderations);
	}
	RepealProposalList vRepealProposals = pLeague->GetRepealProposals();
	for (RepealProposalList::iterator it = vRepealProposals.begin(); it != vRepealProposals.end(); ++it)
	{
		// Special case - If an embargo on us is proposed, use all our Delegates towards its outcome
		if (it->GetEffects()->bEmbargoPlayer && it->GetProposerDecision()->GetDecision() == GetPlayer()->GetID())
		{
			iFocusResolutionID = it->GetID();
		}

		FindBestVoteChoices(it, vConsiderations);
	}

	if (vConsiderations.size() > 0)
	{
		vConsiderations.SortItems();

		// If we want to focus on one resolution, zero out all other considerations
		if (iFocusResolutionID != -1)
		{
			bool bFound = false;
			for (int i = 0; i < vConsiderations.size(); ++i)
			{
				if (vConsiderations.GetElement(i).iID != iFocusResolutionID)
				{
					vConsiderations.SetWeight(i, 0);
				}
				else
				{
					bFound = true;
				}
			}
			CvAssertMsg(bFound, "Could not find the intended proposal when focusing all Delegates on one proposal.");
			CvAssertMsg(vConsiderations.GetTotalWeight() > 0, "Focusing all Delegates on one proposal, but it has no weight value.");
		}

		// Even if we don't like anything, make sure we have something to choose from
		if (vConsiderations.GetTotalWeight() <= 0)
		{
			for (int i = 0; i < vConsiderations.size(); i++)
			{
				vConsiderations.SetWeight(i, 1);
			}
		}

		CvWeightedVector<VoteConsideration, 4, false> vVotesAllocated;
		for (int i = 0; i < iVotes; i++)
		{
			RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
			VoteConsideration chosen = vConsiderations.ChooseByWeight(&fcn, "Choosing a vote to allocate");

			if (chosen.bEnact)
			{
				pLeague->DoVoteEnact(chosen.iID, GetPlayer()->GetID(), 1, chosen.iChoice);
			}
			else
			{
				pLeague->DoVoteRepeal(chosen.iID, GetPlayer()->GetID(), 1, chosen.iChoice);
			}

			// Zero out weight of any other choices that were considered for this proposal, since we can only allocate to one choice
			for (int j = 0; j < vConsiderations.size(); j++)
			{
				if (vConsiderations.GetWeight(j) > 0)
				{
					if (vConsiderations.GetElement(j).bEnact == chosen.bEnact && vConsiderations.GetElement(j).iID == chosen.iID)
					{
						if (vConsiderations.GetElement(j).iChoice != chosen.iChoice)
						{
							vConsiderations.SetWeight(j, 0);
						}
					}
				}
			}

			// For logging - track how many delegates we allocate towards each considered option
			bool bFirst = true;
			for (int j = 0; j < vVotesAllocated.size(); j++)
			{
				if (vVotesAllocated.GetElement(j).bEnact == chosen.bEnact &&
					vVotesAllocated.GetElement(j).iID == chosen.iID &&
					vVotesAllocated.GetElement(j).iChoice == chosen.iChoice)
				{
					bFirst = false;
					vVotesAllocated.SetWeight(j, vVotesAllocated.GetWeight(j) + 1);
				}
			}
			if (bFirst)
			{
				vVotesAllocated.push_back(chosen, 1);
			}
		}

		// Logging
		vVotesAllocated.SortItems();
		for (int i = 0; i < vVotesAllocated.size(); i++)
		{
			if (vVotesAllocated.GetElement(i).bEnact)
			{
				CvEnactProposal* pProposal = pLeague->GetEnactProposal(vVotesAllocated.GetElement(i).iID);
				CvAssert(pProposal != NULL);
				if (pProposal != NULL)
				{
					LogVoteChoiceCommitted(pProposal, vVotesAllocated.GetElement(i).iChoice, vVotesAllocated.GetWeight(i));
				}
			}
			else
			{
				CvRepealProposal* pProposal = pLeague->GetRepealProposal(vVotesAllocated.GetElement(i).iID);
				CvAssert(pProposal != NULL);
				if (pProposal != NULL)
				{
					LogVoteChoiceCommitted(pProposal, vVotesAllocated.GetElement(i).iChoice, vVotesAllocated.GetWeight(i));
				}
			}
		}
	}
}

// Find the best choices for a particular proposal, and add the data directly to the reference list parameter
void CvLeagueAI::FindBestVoteChoices(CvEnactProposal* pProposal, VoteConsiderationList& considerations)
{
	VoteConsiderationList vScoredChoices;
	int iMaxChoicesToConsider = 1;

	CvAssert(GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0);
	if (!(GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0)) return;
	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
	CvAssert(pLeague != NULL);
	if (!(pLeague != NULL)) return;

	switch (pProposal->GetVoterDecision()->GetType())
	{
	case RESOLUTION_DECISION_YES_OR_NO:
		{
			iMaxChoicesToConsider = 1;
			break;
		}
	case RESOLUTION_DECISION_ANY_MEMBER:
	case RESOLUTION_DECISION_MAJOR_CIV_MEMBER:
	case RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER:
		{
			iMaxChoicesToConsider = 1;
			break;
		}
	default:
		{
			CvAssertMsg(false, "Unexpected decision type when evaluating choices for AI on a proposed resolution.");
			break;
		}
	}

	std::vector<int> vChoices = pLeague->GetChoicesForDecision(pProposal->GetVoterDecision()->GetType(), GetPlayer()->GetID());
	for (uint i = 0; i < vChoices.size(); i++)
	{
		VoteConsideration consideration(/*bEnact*/ true, pProposal->GetID(), vChoices[i]);
		int iScore = ScoreVoteChoice(pProposal, vChoices[i]);
		iScore = MAX(iScore, 0); // No negative weights
		vScoredChoices.push_back(consideration, iScore);
	}

	if (vScoredChoices.size() > 0)
	{
		vScoredChoices.SortItems();
		for (int i = 0; i < vScoredChoices.size() && i < iMaxChoicesToConsider; i++)
		{
			considerations.push_back(vScoredChoices.GetElement(i), vScoredChoices.GetWeight(i));
			LogVoteChoiceConsidered(pProposal, vScoredChoices.GetElement(i).iChoice, vScoredChoices.GetWeight(i));
		}
	}
}

// Find the best choices for a particular proposal, and add the data directly to the reference list parameter
void CvLeagueAI::FindBestVoteChoices(CvRepealProposal* pProposal, VoteConsiderationList& considerations)
{
	VoteConsiderationList vScoredChoices;
	int iMaxChoicesToConsider = 1;

	CvAssert(GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0);
	if (!(GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0)) return;
	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
	CvAssert(pLeague != NULL);
	if (!(pLeague != NULL)) return;

	switch (pProposal->GetRepealDecision()->GetType())
	{
	case RESOLUTION_DECISION_REPEAL:
		{
			iMaxChoicesToConsider = 1;
			break;
		}
	default:
		{
			CvAssertMsg(false, "Unexpected decision type when evaluating choices for AI on a proposed resolution.");
			break;
		}
	}

	std::vector<int> vChoices = pLeague->GetChoicesForDecision(pProposal->GetRepealDecision()->GetType(), GetPlayer()->GetID());
	for (uint i = 0; i < vChoices.size(); i++)
	{
		VoteConsideration consideration(/*bEnact*/ false, pProposal->GetID(), vChoices[i]);
		int iScore = ScoreVoteChoice(pProposal, vChoices[i]);
		iScore = MAX(iScore, 0); // No negative weights
		vScoredChoices.push_back(consideration, iScore);
	}

	if (vScoredChoices.size() > 0)
	{
		vScoredChoices.SortItems();
		for (int i = 0; i < vScoredChoices.size() && i < iMaxChoicesToConsider; i++)
		{
			considerations.push_back(vScoredChoices.GetElement(i), vScoredChoices.GetWeight(i));
			LogVoteChoiceConsidered(pProposal, vScoredChoices.GetElement(i).iChoice, vScoredChoices.GetWeight(i));
		}
	}
}

// Score a particular choice on a particular proposal
int CvLeagueAI::ScoreVoteChoice(CvEnactProposal* pProposal, int iChoice)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return 0;
	CvAssert(GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0);
	if (!(GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0)) return 0;
	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
	CvAssert(pLeague != NULL);
	if (!(pLeague != NULL)) return 0;

	int iScore = 0;

	switch (pProposal->GetVoterDecision()->GetType())
	{
	case RESOLUTION_DECISION_YES_OR_NO:
		{
			iScore = ScoreVoteChoiceYesNo(pProposal, iChoice, /*bEnact*/ true);
			break;
		}
	case RESOLUTION_DECISION_ANY_MEMBER:
	case RESOLUTION_DECISION_MAJOR_CIV_MEMBER:
	case RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER:
		{
			iScore = ScoreVoteChoicePlayer(pProposal, iChoice, /*bEnact*/ true);
			break;
		}
	default:
		{
			CvAssertMsg(false, "Unexpected decision type when evaluating choices for AI on a proposed resolution.");
			break;
		}
	}
	
	return iScore;
}

// Score a particular choice on a particular proposal
int CvLeagueAI::ScoreVoteChoice(CvRepealProposal* pProposal, int iChoice)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return 0;
	CvAssert(GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0);
	if (!(GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0)) return 0;
	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
	CvAssert(pLeague != NULL);
	if (!(pLeague != NULL)) return 0;

	int iScore = 0;

	switch (pProposal->GetRepealDecision()->GetType())
	{
	case RESOLUTION_DECISION_REPEAL:
		{
			iScore = ScoreVoteChoiceYesNo(pProposal, iChoice, /*bEnact*/ false);
			break;
		}
	default:
		{
			CvAssertMsg(false, "Unexpected decision type when evaluating choices for AI on a proposed resolution.");
			break;
		}
	}

	return iScore;
}

// Score a particular choice on a particular proposal which is a decision between Yes and No
int CvLeagueAI::ScoreVoteChoiceYesNo(CvProposal* pProposal, int iChoice, bool bEnact)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return 0;

	// How much do we like this choice for this proposal?  Positive is like, negative is dislike.
	// Evaluate as if we are voting Yes to Enact the proposal.  Post-processing below to fit actual situation.
	int iScore = 0;

	// == Proposer Choice ==
	ResolutionDecisionTypes eProposerDecision = pProposal->GetProposerDecision()->GetType();
	PlayerTypes eTargetPlayer = NO_PLAYER;
	if (eProposerDecision == RESOLUTION_DECISION_ANY_MEMBER ||
		eProposerDecision == RESOLUTION_DECISION_MAJOR_CIV_MEMBER ||
		eProposerDecision == RESOLUTION_DECISION_OTHER_MAJOR_CIV_MEMBER)
	{
		eTargetPlayer = (PlayerTypes) pProposal->GetProposerDecision()->GetDecision();
	}
	ResourceTypes eTargetLuxury = NO_RESOURCE;
	if (eProposerDecision == RESOLUTION_DECISION_ANY_LUXURY_RESOURCE)
	{
		CvResourceInfo* pInfo = GC.getResourceInfo((ResourceTypes) pProposal->GetProposerDecision()->GetDecision());
		if (pInfo && pInfo->getResourceUsage() == RESOURCEUSAGE_LUXURY)
		{
			eTargetLuxury = (ResourceTypes) pProposal->GetProposerDecision()->GetDecision();
		}
	}
	ReligionTypes eTargetReligion = NO_RELIGION;
	if (eProposerDecision == RESOLUTION_DECISION_RELIGION)
	{
		eTargetReligion = (ReligionTypes) pProposal->GetProposerDecision()->GetDecision();
	}
	PolicyBranchTypes eTargetIdeology = NO_POLICY_BRANCH_TYPE;
	if (eProposerDecision == RESOLUTION_DECISION_IDEOLOGY)
	{
		eTargetIdeology = (PolicyBranchTypes) pProposal->GetProposerDecision()->GetDecision();
	}

	// == Grand Strategy ==
	AIGrandStrategyTypes eGrandStrategy = GetPlayer()->GetGrandStrategyAI()->GetActiveGrandStrategy();
	bool bSeekingDiploVictory = eGrandStrategy == GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS");
	bool bSeekingConquestVictory = eGrandStrategy == GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST");
	bool bSeekingCultureVictory = eGrandStrategy == GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE");
	bool bSeekingScienceVictory = eGrandStrategy == GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP");

	// == Gameplay Effects ==
	// International Projects
	if (pProposal->GetEffects()->eLeagueProjectEnabled != NO_LEAGUE_PROJECT)
	{
		LeagueProjectTypes eWorldsFair = (LeagueProjectTypes) GC.getInfoTypeForString("LEAGUE_PROJECT_WORLD_FAIR", true);
		LeagueProjectTypes eInternationalGames = (LeagueProjectTypes) GC.getInfoTypeForString("LEAGUE_PROJECT_WORLD_GAMES", true);
		LeagueProjectTypes eInternationalSpaceStation = (LeagueProjectTypes) GC.getInfoTypeForString("LEAGUE_PROJECT_INTERNATIONAL_SPACE_STATION", true);

		// Production might
		int iOurProductionMight = GetPlayer()->calculateProductionMight();
		int iHigherProductionCivs = 0;
		int iLowerProductionCivs = 0;
		int iAliveCivs = 0;
		for (int i = 0; i < MAX_MAJOR_CIVS; i++)
		{
			PlayerTypes e = (PlayerTypes) i;
			if (GET_PLAYER(e).isAlive() && !GET_PLAYER(e).isMinorCiv())
			{
				iAliveCivs++;
				if (GetPlayer()->GetID() != e)
				{
					int iMight = GET_PLAYER(e).calculateProductionMight();
					if (iMight > iOurProductionMight)
					{
						iHigherProductionCivs++;
					}
					else
					{
						iLowerProductionCivs++; // Include civs with equal might
					}
				}
			}
		}
		bool bStrongProduction = false;
		if (iAliveCivs > 0)
		{
			float fProductionMightRatio = ((float)iAliveCivs - (float)iHigherProductionCivs) / ((float)iAliveCivs);
			CvAssertMsg(0.0f <= fProductionMightRatio && fProductionMightRatio <= 1.0f, "Error when evaluating delegates for an international project. Please send Anton your save file and version.");
			fProductionMightRatio = MAX(fProductionMightRatio, 0.0f);
			fProductionMightRatio = MIN(fProductionMightRatio, 1.0f);

			if (fProductionMightRatio >= 0.75f)
			{
				iScore += 40;
				bStrongProduction = true;
			}
			else if (fProductionMightRatio >= 0.50f)
			{
				iScore += 20;
				bStrongProduction = true;
			}
			else if (fProductionMightRatio >= 0.25f)
			{
				iScore += -20;
			}
			else
			{
				iScore += -40;
			}
		}

		// Project specific benefits
		if (eWorldsFair == pProposal->GetEffects()->eLeagueProjectEnabled)
		{
			if (bSeekingCultureVictory && bStrongProduction)
			{
				iScore += 30;
			}
			else
			{
				iScore += 10;
			}
		}
		else if (eInternationalGames == pProposal->GetEffects()->eLeagueProjectEnabled)
		{
			if (bSeekingCultureVictory && bStrongProduction)
			{
				iScore += 40;
			}
			else
			{
				iScore += 10;
			}
		}
		else if (eInternationalSpaceStation == pProposal->GetEffects()->eLeagueProjectEnabled)
		{
			if (bSeekingScienceVictory && bStrongProduction)
			{
				iScore += 50;
			}
			else
			{
				iScore += 10;
			}
		}
	}
	// Embargo City-States
	if (pProposal->GetEffects()->bEmbargoCityStates)
	{
		// Trade connections
		int iCSDestinations = 0;
		int iCSPartners = 0;
		int iCivEmbargos = 0;
		int iCivDestinations = 0;
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			PlayerTypes e = (PlayerTypes) i;
			if (e != GetPlayer()->GetID() && GET_PLAYER(e).isAlive())
			{
				if (GET_PLAYER(e).isMinorCiv())
				{
					iCSDestinations++;
					if (GC.getGame().GetGameTrade()->IsPlayerConnectedToPlayer(GetPlayer()->GetID(), e))
					{
						iCSPartners++;
					}
				}
				else if (GC.getGame().GetGameLeagues()->IsTradeEmbargoed(GetPlayer()->GetID(), e))
				{
					iCivEmbargos++;
				}
				else
				{
					iCivDestinations += GET_PLAYER(e).getNumCities();
				}
			}
		}

		// Would we lose active CS trade routes?
		if (iCSPartners > 0)
		{
			iScore += MAX(-50, iCSPartners * -15);
		}
		else
		{
			iScore += 25;
		}

		// Can we trade with any major civs?
		if (iCivDestinations <= 0)
		{
			iScore += -50;
		}
		else
		{
			// Based on estimates, would we still have enough valid trade routes if this passed?
			int iPossibleRoutesAfter = iCivDestinations * GetPlayer()->getNumCities();
			if ((iPossibleRoutesAfter / 3) < (int) GetPlayer()->GetTrade()->GetNumTradeRoutesPossible())
			{
				iScore += -40;
			}
		}

		// Player Trait making routes to them valuable (Morocco)
		for (int i = 0; i < NUM_YIELD_TYPES; i++)
		{
			YieldTypes e = (YieldTypes) i;
			if (GetPlayer()->GetPlayerTraits()->GetYieldChangeIncomingTradeRoute(e) > 0)
			{
				iScore += -40;
				break;
			}
		}

		// Player Trait gives us extra routes, embargoes are bad for business (Venice)
		if (GetPlayer()->GetPlayerTraits()->GetNumTradeRoutesModifier() > 0)
		{
			iScore += -20;
		}
	}
	// Embargo
	if (pProposal->GetEffects()->bEmbargoPlayer)
	{
		CvAssertMsg(eTargetPlayer != NO_PLAYER, "Evaluating an embargo on NO_PLAYER. Please send Anton your save file and version.");
		// Major Civ relations
		if (eTargetPlayer == GetPlayer()->GetID())
		{
			iScore += -100;
		}
		else if (GET_PLAYER(eTargetPlayer).getTeam() == GetPlayer()->getTeam())
		{
			iScore += -70;
		}
		else if (!GET_PLAYER(eTargetPlayer).isMinorCiv())
		{
			ThreatTypes eWarmongerThreat = GetPlayer()->GetDiplomacyAI()->GetWarmongerThreat(eTargetPlayer);
			MajorCivOpinionTypes eOpinion = GetPlayer()->GetDiplomacyAI()->GetMajorCivOpinion(eTargetPlayer);
			MajorCivApproachTypes eApproach = GetPlayer()->GetDiplomacyAI()->GetMajorCivApproach(eTargetPlayer, /*bHideTrueFeelings*/ true);
			if (GET_TEAM(GetPlayer()->getTeam()).isAtWar(GET_PLAYER(eTargetPlayer).getTeam()))
			{
				iScore += 70;
			}
			else if (eWarmongerThreat >= THREAT_SEVERE)
			{
				iScore += 40;
			}
			else if (eOpinion < MAJOR_CIV_OPINION_NEUTRAL || eApproach < MAJOR_CIV_APPROACH_GUARDED)
			{
				iScore += 20;
			}
			else
			{
				iScore += -30;
			}

			// Trade connections
			if (GC.getGame().GetGameTrade()->IsPlayerConnectedToPlayer(GetPlayer()->GetID(), eTargetPlayer))
			{
				int iFactor = -10;
				iScore += MAX(-40, GC.getGame().GetGameTrade()->CountNumPlayerConnectionsToPlayer(GetPlayer()->GetID(), eTargetPlayer) * iFactor);
			}

			// Player Trait making routes to them valuable (Morocco)
			for (int i = 0; i < NUM_YIELD_TYPES; i++)
			{
				YieldTypes e = (YieldTypes) i;
				if (GetPlayer()->GetPlayerTraits()->GetYieldChangeIncomingTradeRoute(e) > 0)
				{
					iScore += -20;
					break;
				}
			}

			// Player Trait gives us extra routes, embargoes are bad for business (Venice)
			if (GetPlayer()->GetPlayerTraits()->GetNumTradeRoutesModifier() > 0)
			{
				iScore += -20;
			}
		}
	}
	// Ban Luxury
	if (pProposal->GetEffects()->bNoResourceHappiness)
	{
		CvAssertMsg(eTargetLuxury != NO_RESOURCE, "Evaluating banning Happiness for NO_RESOURCE. Please send Anton your save file and version.");

		// What other major civs have this resource?
		int iOtherPlayerResourceFactor = 0;
		bool bOwnedByAnyPlayer = false;
		for (int i = 0; i < MAX_MAJOR_CIVS; i++)
		{
			PlayerTypes e = (PlayerTypes) i;
			if (e != GetPlayer()->GetID() && GET_PLAYER(e).isAlive() && !GET_PLAYER(e).isMinorCiv() && GET_TEAM(GetPlayer()->getTeam()).isHasMet(GET_PLAYER(e).getTeam()))
			{
				if (GET_PLAYER(e).getNumResourceTotal(eTargetLuxury) > 0)
				{
					bOwnedByAnyPlayer = true;
					MajorCivOpinionTypes eOpinion = GetPlayer()->GetDiplomacyAI()->GetMajorCivOpinion(e);
					if (GET_TEAM(GetPlayer()->getTeam()).isAtWar(GET_PLAYER(e).getTeam()))
					{
						iOtherPlayerResourceFactor += -3;
					}
					else if (GetPlayer()->GetDiplomacyAI()->IsDoFAccepted(e) || GetPlayer()->getTeam() == GET_PLAYER(e).getTeam())
					{
						iOtherPlayerResourceFactor += 3;
					}
					else if (eOpinion <= MAJOR_CIV_OPINION_COMPETITOR)
					{
						if (eOpinion == MAJOR_CIV_OPINION_COMPETITOR)
						{
							iOtherPlayerResourceFactor += -1;
						}
						else
						{
							iOtherPlayerResourceFactor += -2;
						}
					}
					else if (eOpinion >= MAJOR_CIV_OPINION_FAVORABLE)
					{
						if (eOpinion == MAJOR_CIV_OPINION_FAVORABLE)
						{
							iOtherPlayerResourceFactor += 1;
						}
						else
						{
							iOtherPlayerResourceFactor += 2;
						}
					}
				}
			}
		}
		if (iOtherPlayerResourceFactor > 0)
		{
			// Our friends have this resource
			iScore += -10;
			if (iOtherPlayerResourceFactor > 3)
			{
				iScore += -10;
			}
		}
		else if (iOtherPlayerResourceFactor < 0)
		{
			// Our enemies have this resource
			iScore += 10;
			if (iOtherPlayerResourceFactor < -3)
			{
				iScore += 10;
			}
		}

		// Do we have this resource?
		if (GetPlayer()->getNumResourceTotal(eTargetLuxury) > 0)
		{
			bOwnedByAnyPlayer = true;
			iScore += -30;
			if (GetPlayer()->getResourceInOwnedPlots(eTargetLuxury) > 0)
			{
				iScore += -20;
			}
			if (GetPlayer()->IsEmpireUnhappy())
			{
				iScore += -20;
			}
		}
		else
		{
			if (GetPlayer()->getResourceInOwnedPlots(eTargetLuxury) > 0)
			{
				iScore += -20;
			}
			else
			{
				iScore += 15;
			}
		}

		if (!bOwnedByAnyPlayer)
		{
			// Hard set to 0 if nobody owns it.  No effect, so we don't care.
			iScore = 0;
		}
	}
	// Standing Army Tax
	if (pProposal->GetEffects()->iUnitMaintenanceGoldPercent != 0)
	{
		int iFactor = (pProposal->GetEffects()->iUnitMaintenanceGoldPercent > 0) ? 1 : -1;
		if (bSeekingConquestVictory)
		{
			iScore += -40 * iFactor;
		}

		// What is the ratio of our current maintenance costs to our gross GPT?
		int iUnitMaintenance = GetPlayer()->GetTreasury()->GetExpensePerTurnUnitMaintenance();
		int iGPT = GetPlayer()->GetTreasury()->CalculateGrossGold();
		float fRatio = ((float)iUnitMaintenance / (float)iGPT);
		if ((iGPT - iUnitMaintenance) < 0)
		{
			iScore += -50 * iFactor;
		}
		else if (fRatio >= 0.5f)
		{
			iScore += -40 * iFactor;
		}
		else if (fRatio >= 0.2f)
		{
			iScore += -15 * iFactor;
		}
		else
		{
			iScore += 15 * iFactor;
		}
	}
	// Scholars in Residence
	if (pProposal->GetEffects()->iMemberDiscoveredTechMod != 0)
	{
		float fTechRatio = GetPlayer()->GetPlayerTechs()->GetTechAI()->GetTechRatio();
		fTechRatio = (fTechRatio - 0.5f) * 2.0f; // -1.0 if in first, 1.0 if in last
		
		// We are better than average
		if (fTechRatio < 0.0f)
		{
			int iFactor = 30;
			iScore += (int) (fTechRatio * iFactor);
			if (bSeekingScienceVictory)
			{
				iScore += -30;
			}
		}
		// At or worse than average
		else
		{
			int iFactor = 50;
			iScore += (int) (fTechRatio * iFactor);
			if (bSeekingScienceVictory)
			{
				iScore += 40;
			}
		}
	}
	// Cultural Heritage Sites
	if (pProposal->GetEffects()->iCulturePerWonder != 0)
	{
		int iNumWonders = GetPlayer()->GetNumWonders();
		int iTempScore = -50;
		if (iNumWonders > 0)
		{
			int iFactor = bSeekingCultureVictory ? 20 : 15;
			iTempScore += iNumWonders * iFactor;
		}
		iScore += iTempScore;
		iScore = MIN(70, iScore);
	}
	// Natural Heritage Sites
	if (pProposal->GetEffects()->iCulturePerNaturalWonder != 0)
	{
		int iNumNaturalWonders = GetPlayer()->GetNumNaturalWondersInOwnedPlots();
		int iTempScore = -35;
		if (iNumNaturalWonders > 0)
		{
			int iFactor = bSeekingCultureVictory ? 20 : 15;
			iTempScore += iNumNaturalWonders * iFactor;
		}
		iScore += iTempScore;
		iScore = MIN(70, iScore);
	}
	// Nuclear Non-Proliferation
	if (pProposal->GetEffects()->bNoTrainingNuclearWeapons)
	{
		if (bSeekingConquestVictory)
		{
			iScore += -50;
		}
		else
		{
			iScore += 35;
		}
	}
	// World Religion
	if (pProposal->GetEffects()->iVotesForFollowingReligion != 0 ||
		pProposal->GetEffects()->iHolyCityTourism != 0 ||
		pProposal->GetEffects()->iReligionSpreadStrengthMod != 0)
	{
		CvAssertMsg(eTargetReligion != NO_RELIGION, "Evaluating World Religion for NO_RELIGION. Please send Anton your save file and version.");
		bool bFoundedReligion = GetPlayer()->GetReligions()->GetReligionCreatedByPlayer() == eTargetReligion;
		bool bMajorityReligion = GetPlayer()->GetReligions()->HasReligionInMostCities(eTargetReligion);
		if (bMajorityReligion)
		{
			iScore += 40;
			if (bSeekingDiploVictory)
			{
				iScore += 20;
			}
		}
		else
		{
			iScore += -30;
		}

		const CvReligion* pkTargetReligion = GC.getGame().GetGameReligions()->GetReligion(eTargetReligion, GetPlayer()->GetID());
		CvAssertMsg(pkTargetReligion, "Evaluating World Religion for an invalid religion. Please send Anton your save file and version.");
		if (pkTargetReligion)
		{
			CvPlot* pPlot = GC.getMap().plot(pkTargetReligion->m_iHolyCityX, pkTargetReligion->m_iHolyCityY);
			if(pPlot)
			{
				CvCity* pHolyCity = pPlot->getPlotCity();
				if (pHolyCity && pHolyCity->getOwner() == GetPlayer()->GetID())
				{
					iScore += 10;
					if (bSeekingCultureVictory)
					{
						iScore += 30;
					}
				}
			}
		}

		if (bFoundedReligion)
		{
			iScore += 40;
		}
		else if (GetPlayer()->GetReligions()->GetReligionCreatedByPlayer() != NO_RELIGION && GetPlayer()->GetReligions()->GetReligionCreatedByPlayer() != eTargetReligion)
		{
			iScore += -20;
		}
	}
	// World Ideology
	if (pProposal->GetEffects()->iVotesForFollowingIdeology != 0 ||
		pProposal->GetEffects()->iOtherIdeologyRebellionMod != 0)
	{
		CvAssertMsg(eTargetIdeology != NO_POLICY_BRANCH_TYPE, "Evaluating World Ideology for NO_POLICY_BRANCH_TYPE. Please send Anton your save file and version.");
		PolicyBranchTypes eOurIdeology = GetPlayer()->GetPlayerPolicies()->GetLateGamePolicyTree();
		bool bPublicOpinionUnhappiness = GetPlayer()->GetCulture()->GetPublicOpinionUnhappiness() > 0;
		if (eOurIdeology != NO_POLICY_BRANCH_TYPE)
		{
			if (eOurIdeology == eTargetIdeology)
			{
				iScore += 50;
				if (bSeekingDiploVictory)
				{
					iScore += 25;
				}
				if (bPublicOpinionUnhappiness)
				{
					iScore += 25;
				}
			}
			else
			{
				iScore += -50;
				if (bSeekingDiploVictory)
				{
					iScore += -25;
				}
				if (bPublicOpinionUnhappiness)
				{
					iScore += -25;
				}
			}
		}
	}
	// Arts Funding
	if (pProposal->GetEffects()->iArtsyGreatPersonRateMod > 0 ||
		pProposal->GetEffects()->iScienceyGreatPersonRateMod < 0)
	{
		if (bSeekingCultureVictory)
		{
			iScore += 80;
		}
		else if (bSeekingScienceVictory)
		{
			iScore += -80;
		}

		// Do we have a sciencey Great Person unique unit? (ie. Merchant of Venice)
		bool bScienceyUniqueUnit = false;
		UnitClassTypes eScienceyUnitClass = (UnitClassTypes) GC.getInfoTypeForString("UNITCLASS_MERCHANT", true);
		if (eScienceyUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pScienceyUnitClassInfo = GC.getUnitClassInfo(eScienceyUnitClass);
			if (pScienceyUnitClassInfo)
			{
				UnitTypes eScienceyUnit = (UnitTypes) GetPlayer()->getCivilizationInfo().getCivilizationUnits(eScienceyUnitClass);
				UnitTypes eDefault = (UnitTypes) pScienceyUnitClassInfo->getDefaultUnitIndex();
				if (eScienceyUnit != eDefault)
				{
					bScienceyUniqueUnit = true;
				}
			}
		}
		
		if (bScienceyUniqueUnit)
		{
			iScore += -60;
		}
	}
	// Sciences Funding
	if (pProposal->GetEffects()->iScienceyGreatPersonRateMod > 0 ||
		pProposal->GetEffects()->iArtsyGreatPersonRateMod < 0)
	{
		if (bSeekingScienceVictory)
		{
			iScore += 80;
		}
		else if (bSeekingCultureVictory)
		{
			iScore += -80;
		}

		// Do we have a sciencey Great Person unique unit? (ie. Merchant of Venice)
		bool bScienceyUniqueUnit = false;
		UnitClassTypes eScienceyUnitClass = (UnitClassTypes) GC.getInfoTypeForString("UNITCLASS_MERCHANT", true);
		if (eScienceyUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pScienceyUnitClassInfo = GC.getUnitClassInfo(eScienceyUnitClass);
			if (pScienceyUnitClassInfo)
			{
				UnitTypes eScienceyUnit = (UnitTypes) GetPlayer()->getCivilizationInfo().getCivilizationUnits(eScienceyUnitClass);
				UnitTypes eDefault = (UnitTypes) pScienceyUnitClassInfo->getDefaultUnitIndex();
				if (eScienceyUnit != eDefault)
				{
					bScienceyUniqueUnit = true;
				}
			}
		}

		if (bScienceyUniqueUnit)
		{
			iScore += 60;
		}
	}
	// Historical Landmarks
	if (pProposal->GetEffects()->iGreatPersonTileImprovementCulture != 0 ||
		pProposal->GetEffects()->iLandmarkCulture != 0)
	{
		int iNumGPImprovements = GetPlayer()->getGreatPersonImprovementCount();
		int iNumLandmarks = 0;
		ImprovementTypes eLandmark = (ImprovementTypes)GC.getInfoTypeForString("IMPROVEMENT_LANDMARK");
		if (eLandmark != NO_IMPROVEMENT)
		{
			iNumLandmarks += GetPlayer()->getImprovementCount(eLandmark);
		}

		int iTempScore = -50;
		if (iNumGPImprovements > 0 || iNumLandmarks > 0)
		{
			int iGPImprovementFactor = bSeekingCultureVictory ? 20 : 15;
			iTempScore += iNumGPImprovements * iGPImprovementFactor;
			int iLandmarkFactor = bSeekingCultureVictory ? 20 : 15;
			iTempScore += iNumLandmarks * iLandmarkFactor;
			iScore += MIN(70, iTempScore);
		}
		iScore += iTempScore;
	}

	// == Diplomat knowledge, Vote Commitments we secured ==

	// == Alignment with Proposer ==
	PlayerTypes eProposer = pProposal->GetProposalPlayer();
	if (iScore > -20 || eProposer == GetPlayer()->GetID())
	{
		if (eProposer != NO_PLAYER)
		{
			AlignmentLevels eAlignment = EvaluateAlignment(eProposer);
			switch (eAlignment)
			{
			case ALIGNMENT_SELF:
				iScore += 40;
				break;
			case ALIGNMENT_LIBERATOR:
			case ALIGNMENT_LEADER:
			case ALIGNMENT_ALLY:
				iScore += 30;
				break;
			case ALIGNMENT_CONFIDANT:
				iScore += 20;
				break;
			case ALIGNMENT_FRIEND:
				iScore += 10;
				break;
			case ALIGNMENT_RIVAL:
				iScore += -10;
				break;
			case ALIGNMENT_HATRED:
				iScore += -20;
				break;
			case ALIGNMENT_ENEMY:
			case ALIGNMENT_WAR:
				iScore += -30;
				break;
			default:
				break;
			}
		}
	}

	// == Post-Processing ==
	if (iChoice == LeagueHelpers::CHOICE_NO)
	{
		iScore *= -1; // Flip the score when we are considering NO for these effects
	}
	if (!bEnact)
	{
		iScore *= -1; // Flip the score when the proposal is to repeal these effects
	}

	return iScore;
}

// Score a particular choice on a particular proposal which is a decision between players (ex. Choose Host, World Leader)
int CvLeagueAI::ScoreVoteChoicePlayer(CvProposal* pProposal, int iChoice, bool bEnact)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return 0;
	CvAssertMsg(bEnact, "Unexpected case when evaluating vote choices for AI. Please send Anton your save file and version.");
	if (!bEnact) return 0;
	PlayerTypes eChoicePlayer = (PlayerTypes) iChoice;
	CvAssert(eChoicePlayer != NO_PLAYER);
	if (!(eChoicePlayer != NO_PLAYER)) return 0;
	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
	CvAssert(pLeague != NULL);
	if (!(pLeague != NULL)) return 0;

	// How much do we like this choice for this proposal?  Positive is like, negative is dislike
	int iScore = 0;

	// == Grand Strategy and other factors ==
	AIGrandStrategyTypes eGrandStrategy = GetPlayer()->GetGrandStrategyAI()->GetActiveGrandStrategy();
	bool bSeekingDiploVictory = eGrandStrategy == GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS");

	// == Diplomatic Victory ==
	if (pProposal->GetEffects()->bDiplomaticVictory)
	{
		AlignmentLevels eAlignment = EvaluateAlignment(eChoicePlayer);
		if (eAlignment == ALIGNMENT_LIBERATOR)
		{
			iScore += 200;
		}
		else if (eAlignment == ALIGNMENT_LEADER)
		{
			iScore += 150;
		}
		else if (eAlignment == ALIGNMENT_SELF)
		{
			iScore += 100;
		}
		else if (eAlignment == ALIGNMENT_WAR)
		{
			iScore += -500;
		}
		else
		{
			iScore += -50;

			if (bSeekingDiploVictory)
			{
				iScore += -150;
			}

			// Don't hand victory to someone
			if (pLeague->CalculateStartingVotesForMember(GetPlayer()->GetID()) + pLeague->CalculateStartingVotesForMember(eChoicePlayer) + 4
				>= GC.getGame().GetVotesNeededForDiploVictory())
			{
				iScore += -150;
			}
			
			switch (eAlignment)
			{
			case ALIGNMENT_ALLY:
				iScore += 70;
				break;
			case ALIGNMENT_CONFIDANT:
				iScore += 50;
				break;
			case ALIGNMENT_FRIEND:
				iScore += 30;
				break;
			case ALIGNMENT_RIVAL:
				iScore += -30;
				break;
			case ALIGNMENT_HATRED:
				iScore += -50;
				break;
			case ALIGNMENT_ENEMY:
				iScore += -70;
				break;
			default:
				break;
			}
		}
	}

	// == Choose Host ==
	// Mostly same as evaluating Diplomatic Victory, but with higher value to pick others you like
	if (pProposal->GetEffects()->bChangeLeagueHost)
	{
		AlignmentLevels eAlignment = EvaluateAlignment(eChoicePlayer);
		if (eAlignment == ALIGNMENT_LIBERATOR)
		{
			iScore += 200;
		}
		else if (eAlignment == ALIGNMENT_SELF)
		{
			iScore += 100;
		}
		else if (eAlignment == ALIGNMENT_LEADER)
		{
			iScore += 50;
		}
		else if (eAlignment == ALIGNMENT_WAR)
		{
			iScore += -500;
		}
		else
		{
			if (bSeekingDiploVictory)
			{
				iScore += -150;
			}

			switch (eAlignment)
			{
			case ALIGNMENT_ALLY:
				iScore += 130;
				break;
			case ALIGNMENT_CONFIDANT:
				iScore += 90;
				break;
			case ALIGNMENT_FRIEND:
				iScore += 50;
				break;
			case ALIGNMENT_RIVAL:
				iScore += -50;
				break;
			case ALIGNMENT_HATRED:
				iScore += -90;
				break;
			case ALIGNMENT_ENEMY:
				iScore += -130;
				break;
			default:
				break;
			}
		}
	}

	return iScore;
}

void CvLeagueAI::AllocateProposals(CvLeague* pLeague)
{
	ProposalConsiderationList vConsiderations;

	std::vector<ResolutionTypes> vInactive = pLeague->GetInactiveResolutions();
	ActiveResolutionList vActive = pLeague->GetActiveResolutions();

	if (!vActive.empty())
	{
		for (uint iResolutionIndex = 0; iResolutionIndex < vActive.size(); iResolutionIndex++)
		{
			int iID = vActive[iResolutionIndex].GetID();
			if (pLeague->CanProposeRepeal(iID, GetPlayer()->GetID()))
			{
				ProposalConsideration consideration(/*bEnact*/ false, iResolutionIndex, LeagueHelpers::CHOICE_NONE);
				int iScore = ScoreProposal(pLeague, &vActive[iResolutionIndex]);
				vConsiderations.push_back(consideration, iScore);
			}
		}
	}

	if (!vInactive.empty())
	{
		for (uint iResolutionIndex = 0; iResolutionIndex < vInactive.size(); iResolutionIndex++)
		{
			ResolutionTypes eResolution = vInactive[iResolutionIndex];
			CvResolutionEntry* pkInfo = GC.getResolutionInfo(eResolution);
			if (pkInfo)
			{
				std::vector<int> vAllChoices = pLeague->GetChoicesForDecision(pkInfo->GetProposerDecision(), GetPlayer()->GetID());
				if (vAllChoices.empty())
				{
					if (pLeague->CanProposeEnact(eResolution, GetPlayer()->GetID(), LeagueHelpers::CHOICE_NONE))
					{
						ProposalConsideration consideration(/*bEnact*/ true, iResolutionIndex, LeagueHelpers::CHOICE_NONE);
						int iScore = ScoreProposal(pLeague, eResolution, LeagueHelpers::CHOICE_NONE);
						vConsiderations.push_back(consideration, iScore);
					}
				}
				else
				{
					for (uint i = 0; i < vAllChoices.size(); i++)
					{
						int iChoice = vAllChoices[i];
						if (pLeague->CanProposeEnact(eResolution, GetPlayer()->GetID(), iChoice))
						{
							ProposalConsideration consideration(/*bEnact*/ true, iResolutionIndex, iChoice);
							int iScore = ScoreProposal(pLeague, eResolution, iChoice);
							vConsiderations.push_back(consideration, iScore);
						}
					}
				}
			}
		}
	}

	// Choose by weight from the top N
	CvAssertMsg(vConsiderations.size() > 0, "No proposals available for the AI to make. Please send Anton your save file and version.");
	if (vConsiderations.size() > 0)
	{
		vConsiderations.SortItems();

		// Even if we don't like anything, make sure we have something to choose from
		if (vConsiderations.GetTotalWeight() <= 0)
		{
			for (int i = 0; i < vConsiderations.size(); i++)
			{
				vConsiderations.SetWeight(i, 1);
			}
		}

		RandomNumberDelegate fn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
		ProposalConsideration proposal = vConsiderations.ChooseFromTopChoices(MIN(vConsiderations.size(), LeagueHelpers::AI_CHOOSE_PROPOSAL_FROM_TOP), &fn, "Choosing proposal from top choices");
		if (proposal.bEnact)
		{
			ResolutionTypes eResolution = vInactive[proposal.iIndex];
			pLeague->DoProposeEnact(eResolution, GetPlayer()->GetID(), proposal.iChoice);
#ifdef ASSIGN_SECOND_PROPOSAL_PRIVILEGE
			if(GC.getGame().isGameMultiPlayer())
				if(GetPlayer()->GetID() == pLeague->GetHostMember()	&& pLeague->GetNumProposersPerSession() == 2)
					pLeague->AssignSecondProposalPrivilege();
#endif
		}
		else
		{
			int iID = vActive[proposal.iIndex].GetID();
			pLeague->DoProposeRepeal(iID, GetPlayer()->GetID());
#ifdef ASSIGN_SECOND_PROPOSAL_PRIVILEGE
			if(GC.getGame().isGameMultiPlayer())
				if(GetPlayer()->GetID() == pLeague->GetHostMember() && pLeague->GetNumProposersPerSession() == 2)
					pLeague->AssignSecondProposalPrivilege();
#endif
		}
	}
}

int CvLeagueAI::ScoreProposal(CvLeague* pLeague, ResolutionTypes eResolution, int iChoice)
{
	CvEnactProposal fakeProposal(/*iID*/-1, eResolution, pLeague->GetID(), /*eProposalPlayer*/NO_PLAYER, iChoice);

	// How much do we like our YES vote on this proposal?
	int iYesScore = 0;
	bool bFoundYes = false;
	ResolutionDecisionTypes eVoteDecision = fakeProposal.GetVoterDecision()->GetType();
	if (eVoteDecision == RESOLUTION_DECISION_YES_OR_NO)
	{
		std::vector<int> vVoteChoices = pLeague->GetChoicesForDecision(fakeProposal.GetVoterDecision()->GetType(), GetPlayer()->GetID());
		CvAssert(!vVoteChoices.empty());
		for (uint i = 0; i < vVoteChoices.size(); i++)
		{
			if (vVoteChoices[i] == LeagueHelpers::CHOICE_YES)
			{
				iYesScore = ScoreVoteChoice(&fakeProposal, vVoteChoices[i]);
				bFoundYes = true;
				break;
			}
		}
	}
	CvAssert(bFoundYes);

	return iYesScore;
}

int CvLeagueAI::ScoreProposal(CvLeague* pLeague, CvActiveResolution* pResolution)
{
	CvRepealProposal fakeProposal(pResolution, /*eProposalPlayer*/NO_PLAYER);

	// How much do we like our YES vote on this proposal?
	int iYesScore = 0;
	bool bFoundYes = false;
	ResolutionDecisionTypes eVoteDecision = fakeProposal.GetRepealDecision()->GetType();
	if (eVoteDecision == RESOLUTION_DECISION_REPEAL)
	{
		std::vector<int> vVoteChoices = pLeague->GetChoicesForDecision(fakeProposal.GetRepealDecision()->GetType(), GetPlayer()->GetID());
		CvAssert(!vVoteChoices.empty());
		for (uint i = 0; i < vVoteChoices.size(); i++)
		{
			if (vVoteChoices[i] == LeagueHelpers::CHOICE_YES)
			{
				iYesScore = ScoreVoteChoice(&fakeProposal, vVoteChoices[i]);
				bFoundYes = true;
				break;
			}
		}
	}
	CvAssert(bFoundYes);

	return iYesScore;
}

void CvLeagueAI::LogVoteChoiceConsidered(CvEnactProposal* pProposal, int iChoice, int iScore)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return;
	CvString sMessage = "";

	sMessage += ",";
	sMessage += GetPlayer()->getCivilizationShortDescription();
	sMessage += ",- - -";
	sMessage += ",Evaluating for Delegates";

	sMessage += ",";
	CvAssert(pProposal != NULL);
	if (pProposal != NULL)
	{
		sMessage += pProposal->GetProposalName(/*bForLogging*/ true);
	}

	sMessage += ",";
	CvAssert(iChoice != LeagueHelpers::CHOICE_NONE);
	if (iChoice != LeagueHelpers::CHOICE_NONE)
	{
		sMessage += LeagueHelpers::GetTextForChoice(pProposal->GetVoterDecision()->GetType(), iChoice);
	}

	sMessage += ",";
	CvString sTemp;
	sTemp.Format("%d", iScore);
	sMessage += sTemp;

	GC.getGame().GetGameLeagues()->LogLeagueMessage(sMessage);
}

void CvLeagueAI::LogVoteChoiceConsidered(CvRepealProposal* pProposal, int iChoice, int iScore)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return;
	CvString sMessage = "";

	sMessage += ",";
	sMessage += GetPlayer()->getCivilizationShortDescription();
	sMessage += ",- - -";
	sMessage += ",Evaluating for Delegates";

	sMessage += ",";
	CvAssert(pProposal != NULL);
	if (pProposal != NULL)
	{
		sMessage += pProposal->GetProposalName(/*bForLogging*/ true);
	}

	sMessage += ",";
	CvAssert(iChoice != LeagueHelpers::CHOICE_NONE);
	if (iChoice != LeagueHelpers::CHOICE_NONE)
	{
		sMessage += LeagueHelpers::GetTextForChoice(pProposal->GetRepealDecision()->GetType(), iChoice);
	}

	sMessage += ",";
	CvString sTemp;
	sTemp.Format("%d", iScore);
	sMessage += sTemp;

	GC.getGame().GetGameLeagues()->LogLeagueMessage(sMessage);
}

void CvLeagueAI::LogVoteChoiceCommitted(CvEnactProposal* pProposal, int iChoice, int iVotes)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return;
	CvString sMessage = "";

	sMessage += ",";
	sMessage += GetPlayer()->getCivilizationShortDescription();
	sMessage += ",- - -";
	sMessage += ",Committing Delegates";

	sMessage += ",";
	CvAssert(pProposal != NULL);
	if (pProposal != NULL)
	{
		sMessage += pProposal->GetProposalName(/*bForLogging*/ true);
	}

	sMessage += ",";
	CvAssert(iChoice != LeagueHelpers::CHOICE_NONE);
	if (iChoice != LeagueHelpers::CHOICE_NONE)
	{
		sMessage += LeagueHelpers::GetTextForChoice(pProposal->GetVoterDecision()->GetType(), iChoice);
	}

	sMessage += ",";
	CvString sTemp;
	sTemp.Format("%d", iVotes);
	sMessage += sTemp;

	GC.getGame().GetGameLeagues()->LogLeagueMessage(sMessage);
}

void CvLeagueAI::LogVoteChoiceCommitted(CvRepealProposal* pProposal, int iChoice, int iVotes)
{
	CvAssert(pProposal != NULL);
	if (!(pProposal != NULL)) return;
	CvString sMessage = "";

	sMessage += ",";
	sMessage += GetPlayer()->getCivilizationShortDescription();
	sMessage += ",- - -";
	sMessage += ",Committing Delegates";

	sMessage += ",";
	CvAssert(pProposal != NULL);
	if (pProposal != NULL)
	{
		sMessage += pProposal->GetProposalName(/*bForLogging*/ true);
	}

	sMessage += ",";
	CvAssert(iChoice != LeagueHelpers::CHOICE_NONE);
	if (iChoice != LeagueHelpers::CHOICE_NONE)
	{
		sMessage += LeagueHelpers::GetTextForChoice(pProposal->GetRepealDecision()->GetType(), iChoice);
	}

	sMessage += ",";
	CvString sTemp;
	sTemp.Format("%d", iVotes);
	sMessage += sTemp;

	GC.getGame().GetGameLeagues()->LogLeagueMessage(sMessage);
}


// ================================================================================
//			CvLeagueSpecialSessionEntry
// ================================================================================
CvLeagueSpecialSessionEntry::CvLeagueSpecialSessionEntry(void)
{
	m_eEraTrigger						= NO_ERA;
	m_eImmediateProposal				= NO_RESOLUTION;
	m_eRecurringProposal				= NO_RESOLUTION;
	m_iTurnsBetweenSessions				= 0;
	m_iCivDelegates						= 0;
	m_iHostDelegates					= 0;
	m_iCityStateDelegates				= 0;
	m_bUnitedNations					= false;
}

CvLeagueSpecialSessionEntry::~CvLeagueSpecialSessionEntry(void)
{
}

bool CvLeagueSpecialSessionEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
	{
		return false;
	}

	m_eEraTrigger						= (EraTypes) GC.getInfoTypeForString(kResults.GetText("EraTrigger"), true);
	m_eImmediateProposal				= (ResolutionTypes) GC.getInfoTypeForString(kResults.GetText("ImmediateProposal"), true);
	m_eRecurringProposal				= (ResolutionTypes) GC.getInfoTypeForString(kResults.GetText("RecurringProposal"), true);
	m_iTurnsBetweenSessions				= kResults.GetInt("TurnsBetweenSessions");
	m_iCivDelegates						= kResults.GetInt("CivDelegates");
	m_iHostDelegates					= kResults.GetInt("HostDelegates");
	m_iCityStateDelegates				= kResults.GetInt("CityStateDelegates");
	m_bUnitedNations					= kResults.GetBool("UnitedNations");

	return true;
}

EraTypes CvLeagueSpecialSessionEntry::GetEraTrigger() const
{
	return m_eEraTrigger;
}

ResolutionTypes CvLeagueSpecialSessionEntry::GetImmediateProposal() const
{
	return m_eImmediateProposal;
}

ResolutionTypes CvLeagueSpecialSessionEntry::GetRecurringProposal() const
{
	return m_eRecurringProposal;
}

int CvLeagueSpecialSessionEntry::GetTurnsBetweenSessions() const
{
	return m_iTurnsBetweenSessions;
}

int CvLeagueSpecialSessionEntry::GetCivDelegates() const
{
	return m_iCivDelegates;
}

int CvLeagueSpecialSessionEntry::GetHostDelegates() const
{
	return m_iHostDelegates;
}

int CvLeagueSpecialSessionEntry::GetCityStateDelegates() const
{
	return m_iCityStateDelegates;
}

bool CvLeagueSpecialSessionEntry::IsUnitedNations() const
{
	return m_bUnitedNations;
}


// ================================================================================
//			CvLeagueSpecialSessionXMLEntries
// ================================================================================
CvLeagueSpecialSessionXMLEntries::CvLeagueSpecialSessionXMLEntries(void)
{
}

CvLeagueSpecialSessionXMLEntries::~CvLeagueSpecialSessionXMLEntries(void)
{
	DeleteArray();
}

// Returns vector of league project entries
std::vector<CvLeagueSpecialSessionEntry*>& CvLeagueSpecialSessionXMLEntries::GetLeagueSpecialSessionEntries()
{
	return m_paLeagueSpecialSessionEntries;
}

// Returns number of defined league projects
int CvLeagueSpecialSessionXMLEntries::GetNumLeagueSpecialSessions()
{
	return m_paLeagueSpecialSessionEntries.size();
}

// Clears league project entries
void CvLeagueSpecialSessionXMLEntries::DeleteArray()
{
	for (std::vector<CvLeagueSpecialSessionEntry*>::iterator it = m_paLeagueSpecialSessionEntries.begin(); it != m_paLeagueSpecialSessionEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paLeagueSpecialSessionEntries.clear();
}

// Returns a specific league project entry
CvLeagueSpecialSessionEntry* CvLeagueSpecialSessionXMLEntries::GetEntry(int index)
{
	return m_paLeagueSpecialSessionEntries[index];
}


// ================================================================================
//			CvLeagueNameEntry
// ================================================================================
CvLeagueNameEntry::CvLeagueNameEntry(void)
: m_iID(-1)
{
}

CvLeagueNameEntry::~CvLeagueNameEntry(void)
{
}

bool CvLeagueNameEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility&)
{
	m_iID				= kResults.GetInt("ID");
	m_strType			= kResults.GetText("Type");
	m_strNameKey = kResults.GetText("NameKey");
	
	return true;
}


// ================================================================================
//			CvLeagueNameXMLEntries
// ================================================================================
CvLeagueNameXMLEntries::CvLeagueNameXMLEntries(void)
{
}

CvLeagueNameXMLEntries::~CvLeagueNameXMLEntries(void)
{
	DeleteArray();
}

// Returns vector of league name entries
std::vector<CvLeagueNameEntry*>& CvLeagueNameXMLEntries::GetLeagueNameEntries()
{
	return m_paLeagueNameEntries;
}

// Returns number of defined league names
int CvLeagueNameXMLEntries::GetNumLeagueNames()
{
	return m_paLeagueNameEntries.size();
}

// Clears league name entries
void CvLeagueNameXMLEntries::DeleteArray()
{
	for (std::vector<CvLeagueNameEntry*>::iterator it = m_paLeagueNameEntries.begin(); it != m_paLeagueNameEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paLeagueNameEntries.clear();
}

// Returns a specific league name entry
CvLeagueNameEntry* CvLeagueNameXMLEntries::GetEntry(int index)
{
	return m_paLeagueNameEntries[index];
}


// ================================================================================
//			CvLeagueProjectRewardEntry
// ================================================================================
CvLeagueProjectRewardEntry::CvLeagueProjectRewardEntry(void)
{
	m_eBuilding							= NO_BUILDING;
	m_iHappiness						= 0;
	m_iFreeSocialPolicies				= 0;
	m_iCultureBonusTurns				= 0;
	m_iTourismBonusTurns				= 0;
	m_iGoldenAgePoints					= 0;
	m_iCityStateInfluenceBoost			= 0;
	m_iBaseBeakersTurnsToCount			= 0;
	m_eFreeUnitClass					= NO_UNITCLASS;
}

CvLeagueProjectRewardEntry::~CvLeagueProjectRewardEntry(void)
{
}

bool CvLeagueProjectRewardEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
	{
		return false;
	}

	m_eBuilding							= (BuildingTypes) GC.getInfoTypeForString(kResults.GetText("Building"), true);
	m_iHappiness						= kResults.GetInt("Happiness");
	m_iFreeSocialPolicies				= kResults.GetInt("FreeSocialPolicies");
	m_iCultureBonusTurns				= kResults.GetInt("CultureBonusTurns");
	m_iTourismBonusTurns				= kResults.GetInt("TourismBonusTurns");
	m_iGoldenAgePoints					= kResults.GetInt("GoldenAgePoints");
	m_iCityStateInfluenceBoost			= kResults.GetInt("CityStateInfluenceBoost");
	m_iBaseBeakersTurnsToCount			= kResults.GetInt("BaseBeakersTurnsToCount");
	m_eFreeUnitClass					= (UnitClassTypes) GC.getInfoTypeForString(kResults.GetText("FreeUnitClass"), true);

	return true;
}

BuildingTypes CvLeagueProjectRewardEntry::GetBuilding() const
{
	return m_eBuilding;
}

int CvLeagueProjectRewardEntry::GetHappiness() const
{
	return m_iHappiness;
}

int CvLeagueProjectRewardEntry::GetFreeSocialPolicies() const
{
	return m_iFreeSocialPolicies;
}

int CvLeagueProjectRewardEntry::GetCultureBonusTurns() const
{
	return m_iCultureBonusTurns;
}

int CvLeagueProjectRewardEntry::GetTourismBonusTurns() const
{
	return m_iTourismBonusTurns;
}

int CvLeagueProjectRewardEntry::GetGoldenAgePoints() const
{
	return m_iGoldenAgePoints;
}

int CvLeagueProjectRewardEntry::GetCityStateInfluenceBoost() const
{
	return m_iCityStateInfluenceBoost;
}

int CvLeagueProjectRewardEntry::GetBaseBeakersTurnsToCount() const
{
	return m_iBaseBeakersTurnsToCount;
}

UnitClassTypes CvLeagueProjectRewardEntry::GetFreeUnitClass() const
{
	return m_eFreeUnitClass;
}


// ================================================================================
//			CvLeagueProjectRewardXMLEntries
// ================================================================================
CvLeagueProjectRewardXMLEntries::CvLeagueProjectRewardXMLEntries(void)
{
}

CvLeagueProjectRewardXMLEntries::~CvLeagueProjectRewardXMLEntries(void)
{
	DeleteArray();
}

// Returns vector of league project entries
std::vector<CvLeagueProjectRewardEntry*>& CvLeagueProjectRewardXMLEntries::GetLeagueProjectRewardEntries()
{
	return m_paLeagueProjectRewardEntries;
}

// Returns number of defined league projects
int CvLeagueProjectRewardXMLEntries::GetNumLeagueProjectRewards()
{
	return m_paLeagueProjectRewardEntries.size();
}

// Clears league project entries
void CvLeagueProjectRewardXMLEntries::DeleteArray()
{
	for (std::vector<CvLeagueProjectRewardEntry*>::iterator it = m_paLeagueProjectRewardEntries.begin(); it != m_paLeagueProjectRewardEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paLeagueProjectRewardEntries.clear();
}

// Returns a specific league project entry
CvLeagueProjectRewardEntry* CvLeagueProjectRewardXMLEntries::GetEntry(int index)
{
	return m_paLeagueProjectRewardEntries[index];
}


// ================================================================================
//			CvLeagueProjectEntry
// ================================================================================
CvLeagueProjectEntry::CvLeagueProjectEntry(void)
{
	m_eProcess							= NO_PROCESS;
	m_iCostPerPlayer					= 0;
	m_eRewardTier1						= NO_LEAGUE_PROJECT_REWARD;
	m_eRewardTier2						= NO_LEAGUE_PROJECT_REWARD;
	m_eRewardTier3						= NO_LEAGUE_PROJECT_REWARD;
}

CvLeagueProjectEntry::~CvLeagueProjectEntry(void)
{
}

bool CvLeagueProjectEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
	{
		return false;
	}

	m_eProcess							= (ProcessTypes) GC.getInfoTypeForString(kResults.GetText("Process"), true);
	m_iCostPerPlayer					= kResults.GetInt("CostPerPlayer");
	m_eRewardTier1						= (LeagueProjectRewardTypes) GC.getInfoTypeForString(kResults.GetText("RewardTier1"), true);
	m_eRewardTier2						= (LeagueProjectRewardTypes) GC.getInfoTypeForString(kResults.GetText("RewardTier2"), true);
	m_eRewardTier3						= (LeagueProjectRewardTypes) GC.getInfoTypeForString(kResults.GetText("RewardTier3"), true);

	return true;
}

ProcessTypes CvLeagueProjectEntry::GetProcess() const
{
	return m_eProcess;
}

int CvLeagueProjectEntry::GetCostPerPlayer() const
{
	return m_iCostPerPlayer;
}

LeagueProjectRewardTypes CvLeagueProjectEntry::GetRewardTier1() const
{
	return m_eRewardTier1;
}

LeagueProjectRewardTypes CvLeagueProjectEntry::GetRewardTier2() const
{
	return m_eRewardTier2;
}

LeagueProjectRewardTypes CvLeagueProjectEntry::GetRewardTier3() const
{
	return m_eRewardTier3;
}


// ================================================================================
//			CvLeagueProjectXMLEntries
// ================================================================================
CvLeagueProjectXMLEntries::CvLeagueProjectXMLEntries(void)
{
}

CvLeagueProjectXMLEntries::~CvLeagueProjectXMLEntries(void)
{
	DeleteArray();
}

// Returns vector of league project entries
std::vector<CvLeagueProjectEntry*>& CvLeagueProjectXMLEntries::GetLeagueProjectEntries()
{
	return m_paLeagueProjectEntries;
}

// Returns number of defined league projects
int CvLeagueProjectXMLEntries::GetNumLeagueProjects()
{
	return m_paLeagueProjectEntries.size();
}

// Clears league project entries
void CvLeagueProjectXMLEntries::DeleteArray()
{
	for (std::vector<CvLeagueProjectEntry*>::iterator it = m_paLeagueProjectEntries.begin(); it != m_paLeagueProjectEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paLeagueProjectEntries.clear();
}

// Returns a specific league project entry
CvLeagueProjectEntry* CvLeagueProjectXMLEntries::GetEntry(int index)
{
	return m_paLeagueProjectEntries[index];
}


// ================================================================================
//			CvResolutionEntry
// ================================================================================
CvResolutionEntry::CvResolutionEntry(void)
{
	m_eVoterDecision					= RESOLUTION_DECISION_NONE;
	m_eProposerDecision					= RESOLUTION_DECISION_NONE;
	m_eTechPrereqAnyMember				= NO_TECH;
	m_bAutomaticProposal				= false;
	m_bUniqueType						= false;
	m_bNoProposalByPlayer				= false;
	m_iQuorumPercent					= 0;
	m_iLeadersVoteBonusOnFail			= 0;
	m_bDiplomaticVictory				= false;
	m_bChangeLeagueHost					= false;
	m_iOneTimeGold						= 0;
	m_iOneTimeGoldPercent				= 0;
	m_bRaiseCityStateInfluenceToNeutral	= false;
	m_eLeagueProjectEnabled				= NO_LEAGUE_PROJECT;
	m_iGoldPerTurn						= 0;
	m_iResourceQuantity					= 0;
	m_bEmbargoCityStates				= false;
	m_bEmbargoPlayer					= false;
	m_bNoResourceHappiness				= false;
	m_iUnitMaintenanceGoldPercent		= 0;
	m_iMemberDiscoveredTechMod			= 0;
	m_iCulturePerWonder					= 0;
	m_iCulturePerNaturalWonder			= 0;
	m_bNoTrainingNuclearWeapons			= false;
	m_iVotesForFollowingReligion		= 0;
	m_iHolyCityTourism					= 0;
	m_iReligionSpreadStrengthMod		= 0;
	m_iVotesForFollowingIdeology		= 0;
	m_iOtherIdeologyRebellionMod		= 0;
	m_iArtsyGreatPersonRateMod			= 0;
	m_iScienceyGreatPersonRateMod		= 0;
	m_iGreatPersonTileImprovementCulture= 0;
	m_iLandmarkCulture					= 0;
#ifdef NEW_LEAGUE_RESOLUTIONS
	m_iTradeRouteGoldModifier			= 0;
	m_iCSBonuModifier					= 0;
	m_bNoSpiesInCS						= false;
	m_bDoubleResourceHappiness			= false;
#endif
}

CvResolutionEntry::~CvResolutionEntry(void)
{
}

bool CvResolutionEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
	{
		return false;
	}

	m_eVoterDecision					= (ResolutionDecisionTypes) GC.getInfoTypeForString(kResults.GetText("VoterDecision"), true);
	m_eProposerDecision					= (ResolutionDecisionTypes) GC.getInfoTypeForString(kResults.GetText("ProposerDecision"), true);
	m_eTechPrereqAnyMember				= (TechTypes) GC.getInfoTypeForString(kResults.GetText("TechPrereqAnyMember"), true);
	m_bAutomaticProposal				= kResults.GetBool("AutomaticProposal");
	m_bUniqueType						= kResults.GetBool("UniqueType");
	m_bNoProposalByPlayer				= kResults.GetBool("NoProposalByPlayer");
	m_iQuorumPercent					= kResults.GetInt("QuorumPercent");
	m_iLeadersVoteBonusOnFail			= kResults.GetInt("LeadersVoteBonusOnFail");
	m_bDiplomaticVictory				= kResults.GetBool("DiplomaticVictory");
	m_bChangeLeagueHost					= kResults.GetBool("ChangeLeagueHost");
	m_iOneTimeGold						= kResults.GetInt("OneTimeGold");
	m_iOneTimeGoldPercent				= kResults.GetInt("OneTimeGoldPercent");
	m_bRaiseCityStateInfluenceToNeutral	= kResults.GetBool("RaiseCityStateInfluenceToNeutral");
	m_eLeagueProjectEnabled				= (LeagueProjectTypes) GC.getInfoTypeForString(kResults.GetText("LeagueProjectEnabled"), true);
	m_iGoldPerTurn						= kResults.GetInt("GoldPerTurn");
	m_iResourceQuantity					= kResults.GetInt("ResourceQuantity");
	m_bEmbargoCityStates				= kResults.GetBool("EmbargoCityStates");
	m_bEmbargoPlayer					= kResults.GetBool("EmbargoPlayer");
	m_bNoResourceHappiness				= kResults.GetBool("NoResourceHappiness");
	m_iUnitMaintenanceGoldPercent		= kResults.GetInt("UnitMaintenanceGoldPercent");
	m_iMemberDiscoveredTechMod			= kResults.GetInt("MemberDiscoveredTechMod");
	m_iCulturePerWonder					= kResults.GetInt("CulturePerWonder");
	m_iCulturePerNaturalWonder			= kResults.GetInt("CulturePerNaturalWonder");
	m_bNoTrainingNuclearWeapons			= kResults.GetBool("NoTrainingNuclearWeapons");
	m_iVotesForFollowingReligion		= kResults.GetInt("VotesForFollowingReligion");
	m_iHolyCityTourism					= kResults.GetInt("HolyCityTourism");
	m_iReligionSpreadStrengthMod		= kResults.GetInt("ReligionSpreadStrengthMod");
	m_iVotesForFollowingIdeology		= kResults.GetInt("VotesForFollowingIdeology");
	m_iOtherIdeologyRebellionMod		= kResults.GetInt("OtherIdeologyRebellionMod");
	m_iArtsyGreatPersonRateMod			= kResults.GetInt("ArtsyGreatPersonRateMod");
	m_iScienceyGreatPersonRateMod		= kResults.GetInt("ScienceyGreatPersonRateMod");
	m_iGreatPersonTileImprovementCulture= kResults.GetInt("GreatPersonTileImprovementCulture");
	m_iLandmarkCulture					= kResults.GetInt("LandmarkCulture");
#ifdef NEW_LEAGUE_RESOLUTIONS
	m_iTradeRouteGoldModifier			= kResults.GetInt("TradeRouteGoldModifier");
	m_iCSBonuModifier					= kResults.GetInt("CSBonuModifier");
	m_bNoSpiesInCS						= kResults.GetInt("NoSpiesInCS");
	m_bDoubleResourceHappiness			= kResults.GetInt("DoubleResourceHappiness");
#endif

	return true;
}

ResolutionDecisionTypes CvResolutionEntry::GetVoterDecision() const
{
	return m_eVoterDecision;
}

ResolutionDecisionTypes CvResolutionEntry::GetProposerDecision() const
{
	return m_eProposerDecision;
}

TechTypes CvResolutionEntry::GetTechPrereqAnyMember() const
{
	return m_eTechPrereqAnyMember;
}

bool CvResolutionEntry::IsAutomaticProposal() const
{
	return m_bAutomaticProposal;
}

bool CvResolutionEntry::IsUniqueType() const
{
	return m_bUniqueType;
}

bool CvResolutionEntry::IsNoProposalByPlayer() const
{
	return m_bNoProposalByPlayer;
}

int CvResolutionEntry::GetQuorumPercent() const
{
	return m_iQuorumPercent;
}

int CvResolutionEntry::GetLeadersVoteBonusOnFail() const
{
	return m_iLeadersVoteBonusOnFail;
}

bool CvResolutionEntry::IsDiplomaticVictory() const
{
	return m_bDiplomaticVictory;
}

bool CvResolutionEntry::IsChangeLeagueHost() const
{
	return m_bChangeLeagueHost;
}

int CvResolutionEntry::GetOneTimeGold() const
{
	return m_iOneTimeGold;
}

int CvResolutionEntry::GetOneTimeGoldPercent() const
{
	return m_iOneTimeGoldPercent;
}

bool CvResolutionEntry::IsRaiseCityStateInfluenceToNeutral() const
{
	return m_bRaiseCityStateInfluenceToNeutral;
}

LeagueProjectTypes CvResolutionEntry::GetLeagueProjectEnabled() const
{
	return m_eLeagueProjectEnabled;
}

int CvResolutionEntry::GetGoldPerTurn() const
{
	return m_iGoldPerTurn;
}

int CvResolutionEntry::GetResourceQuantity() const
{
	return m_iResourceQuantity;
}

bool CvResolutionEntry::IsEmbargoCityStates() const
{
	return m_bEmbargoCityStates;
}

bool CvResolutionEntry::IsEmbargoPlayer() const
{
	return m_bEmbargoPlayer;
}

bool CvResolutionEntry::IsNoResourceHappiness() const
{
	return m_bNoResourceHappiness;
}

int CvResolutionEntry::GetUnitMaintenanceGoldPercent() const
{
	return m_iUnitMaintenanceGoldPercent;
}

int CvResolutionEntry::GetMemberDiscoveredTechMod() const
{
	return m_iMemberDiscoveredTechMod;
}

int CvResolutionEntry::GetCulturePerWonder() const
{
	return m_iCulturePerWonder;
}

int CvResolutionEntry::GetCulturePerNaturalWonder() const
{
	return m_iCulturePerNaturalWonder;
}

bool CvResolutionEntry::IsNoTrainingNuclearWeapons() const
{
	return m_bNoTrainingNuclearWeapons;
}

int CvResolutionEntry::GetVotesForFollowingReligion() const
{
	return m_iVotesForFollowingReligion;
}

int CvResolutionEntry::GetHolyCityTourism() const
{
	return m_iHolyCityTourism;
}

int CvResolutionEntry::GetReligionSpreadStrengthMod() const
{
	return m_iReligionSpreadStrengthMod;
}

int CvResolutionEntry::GetVotesForFollowingIdeology() const
{
	return m_iVotesForFollowingIdeology;
}

int CvResolutionEntry::GetOtherIdeologyRebellionMod() const
{
	return m_iOtherIdeologyRebellionMod;
}

int CvResolutionEntry::GetArtsyGreatPersonRateMod() const
{
	return m_iArtsyGreatPersonRateMod;
}

int CvResolutionEntry::GetScienceyGreatPersonRateMod() const
{
	return m_iScienceyGreatPersonRateMod;
}

int CvResolutionEntry::GetGreatPersonTileImprovementCulture() const
{
	return m_iGreatPersonTileImprovementCulture;
}

int CvResolutionEntry::GetLandmarkCulture() const
{
	return m_iLandmarkCulture;
}

#ifdef NEW_LEAGUE_RESOLUTIONS
int CvResolutionEntry::GetTradeRouteGoldModifier() const
{
	return m_iTradeRouteGoldModifier;
}

int CvResolutionEntry::GetCSBonuModifier() const
{
	return m_iCSBonuModifier;
}

int CvResolutionEntry::GetNoSpiesInCS() const
{
	return m_bNoSpiesInCS;
}

int CvResolutionEntry::GetDoubleResourceHappiness() const
{
	return m_bDoubleResourceHappiness;
}
#endif


// ================================================================================
//			CvResolutionXMLEntries
// ================================================================================
CvResolutionXMLEntries::CvResolutionXMLEntries(void)
{
}

CvResolutionXMLEntries::~CvResolutionXMLEntries(void)
{
	DeleteArray();
}

// Returns vector of resolution entries
std::vector<CvResolutionEntry*>& CvResolutionXMLEntries::GetResolutionEntries()
{
	return m_paResolutionEntries;
}

// Returns number of defined resolutions
int CvResolutionXMLEntries::GetNumResolutions()
{
	return m_paResolutionEntries.size();
}

// Clears resolution entries
void CvResolutionXMLEntries::DeleteArray()
{
	for (std::vector<CvResolutionEntry*>::iterator it = m_paResolutionEntries.begin(); it != m_paResolutionEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paResolutionEntries.clear();
}

// Returns a specific resolution entry
CvResolutionEntry* CvResolutionXMLEntries::GetEntry(int index)
{
	return m_paResolutionEntries[index];
}
#ifdef MP_PLAYERS_VOTING_SYSTEM


// ================================================================================
//			CvMPVotingSystem
// ================================================================================
CvMPVotingSystem::CvMPVotingSystem(void)
{
	m_iLastProposalID = 0;
	m_vProposals.clear();
}


CvMPVotingSystem::~CvMPVotingSystem(void)
{
}

void CvMPVotingSystem::Init()
{
	// SLOG("CvMPVotingSystem INIT");
	if (GC.getGame().isOption("GAMEOPTION_DUEL_STUFF"))
	{
		if (GC.getGame().isOption("GAMEOPTION_ENABLE_REMAP_VOTE"))
		{
			if (!IsAnyProposalType(PROPOSAL_REMAP))
				AddProposal(PROPOSAL_REMAP, NO_PLAYER, NO_PLAYER);
		}
	}
}

CvMPVotingSystem::Proposal::Proposal(void)
{
	iID = 0;
	iCreationTurn = -1;
	iExpirationCounter = 0;
	eType = NO_PROPOSAL;
	eStatus = STATUS_ACTIVE;
	eProposalOwner = NO_PLAYER;
	eProposalSubject = NO_PLAYER;
	bComplete = false;
	for (int i = 0; i < MAX_MAJOR_CIVS; i++)
	{
		vVotes.push_back(false);
		vHasVoted.push_back(false);
		if (GET_PLAYER((PlayerTypes)i).isHuman() && GET_PLAYER((PlayerTypes)i).isAlive())
			vIsEligible.push_back(true);
		else
			vIsEligible.push_back(false);
	}
}

CvMPVotingSystem::Proposal::~Proposal(void)
{
}

int CvMPVotingSystem::GetLastProposalID()
{
	return m_iLastProposalID;
}

CvMPVotingSystem::Proposal* CvMPVotingSystem::GetProposalByID(int iProposalID)
{
	for (uint i = 0; i < m_vProposals.size(); i++)
	{
		if (m_vProposals.at(i).iID == iProposalID)
			return &m_vProposals.at(i);
	}
	return NULL;
}

int CvMPVotingSystem::GetProposalExpirationCounter(int iProposalID)
{
	return m_vProposals.at(iProposalID).iExpirationCounter;
}

int CvMPVotingSystem::GetProposalTypeCooldownResetTurn(MPVotingSystemProposalTypes eType, PlayerTypes ePlayerID)
{
	int iMaxTurn = -1;
	for (ProposalList::iterator it = m_vProposals.begin(); it != m_vProposals.end(); ++it)
	{
		if ((it->eType == eType) && (it->iCreationTurn > iMaxTurn))
		{
			if ((eType != PROPOSAL_IRR) || (it->eProposalOwner == ePlayerID))  // personal cooldown for IRR
				iMaxTurn = it->iCreationTurn;
		}
	}
	SLOG("iMaxTurn %d getGameTurn %d", iMaxTurn, GC.getGame().getGameTurn());
	if ((iMaxTurn == -1) || (eType == PROPOSAL_SCRAP))
	{
		return -1;
	}

	return iMaxTurn + PROPOSAL_COOLDOWN;
}

MPVotingSystemProposalTypes CvMPVotingSystem::GetProposalType(int iProposalID)
{
	return m_vProposals.at(iProposalID).eType;
}

MPVotingSystemProposalStatus CvMPVotingSystem::GetProposalStatus(int iProposalID)
{
	return m_vProposals.at(iProposalID).eStatus;
}

PlayerTypes CvMPVotingSystem::GetProposalOwner(int iProposalID)
{
	return m_vProposals.at(iProposalID).eProposalOwner;
}

PlayerTypes CvMPVotingSystem::GetProposalSubject(int iProposalID)
{
	return m_vProposals.at(iProposalID).eProposalSubject;
}

bool CvMPVotingSystem::GetProposalCompletion(int iProposalID)
{
	return m_vProposals.at(iProposalID).bComplete;
}

bool CvMPVotingSystem::GetVoterEligibility(int iProposalID, PlayerTypes ePlayerID)
{
	return m_vProposals.at(iProposalID).vIsEligible.at((int)ePlayerID);
}

bool CvMPVotingSystem::GetVoterHasVoted(int iProposalID, PlayerTypes ePlayerID)
{
	return m_vProposals.at(iProposalID).vHasVoted.at((int)ePlayerID);
}

bool CvMPVotingSystem::GetVoterVote(int iProposalID, PlayerTypes ePlayerID)
{
	return m_vProposals.at(iProposalID).vVotes.at((int)ePlayerID);
}


int CvMPVotingSystem::GetYesVotes(int iProposalID)
{
	int yesVotes = 0;
	for (uint i = 0; i < MAX_MAJOR_CIVS; i++)
	{
		if (GetVoterEligibility(iProposalID, (PlayerTypes)i) && GetVoterHasVoted(iProposalID, (PlayerTypes)i) && GetVoterVote(iProposalID, (PlayerTypes)i))
			yesVotes++;
	}
	return yesVotes;
}

int CvMPVotingSystem::GetNoVotes(int iProposalID)
{
	int noVotes = 0;
	Proposal pProposal = *GetProposalByID(iProposalID);
	for (uint i = 0; i < MAX_MAJOR_CIVS; i++)
	{
		if (pProposal.vIsEligible.at(i) && pProposal.vHasVoted.at(i) && !pProposal.vVotes.at(i))
			noVotes++;
	}
	return noVotes;
}

int CvMPVotingSystem::GetMaxVotes(int iProposalID)
{
	int iMaxVotes = 0;
	Proposal pProposal = *GetProposalByID(iProposalID);
	for (uint i = 0; i < MAX_MAJOR_CIVS; i++)
	{
		if (pProposal.vIsEligible.at(i))
			iMaxVotes++;
	}
	return iMaxVotes;
}

bool CvMPVotingSystem::IsPlayerHasActiveProposal(PlayerTypes ePlayerID)
{
	for (ProposalList::iterator it = m_vProposals.begin(); it != m_vProposals.end(); ++it)
	{
		if (!it->bComplete && (it->eProposalOwner == ePlayerID))
		{
			return true;
		}
	}
	return false;
}

bool CvMPVotingSystem::IsAnyActiveProposalType(MPVotingSystemProposalTypes eType)
{
	for (ProposalList::iterator it = m_vProposals.begin(); it != m_vProposals.end(); ++it)
	{
		if (!it->bComplete && (it->eType == eType))
		{
			return true;
		}
	}
	return false;
}

bool CvMPVotingSystem::IsAnyProposalType(MPVotingSystemProposalTypes eType)
{
	for (ProposalList::iterator it = m_vProposals.begin(); it != m_vProposals.end(); ++it)
	{
		if (it->eType == eType)
		{
			return true;
		}
	}
	return false;
}


bool CvMPVotingSystem::IsProposalTypeOnCooldown(MPVotingSystemProposalTypes eType, PlayerTypes ePlayerID)
{
	int iMaxTurn = -1;
	for (ProposalList::iterator it = m_vProposals.begin(); it != m_vProposals.end(); ++it)
	{
		if ((it->eType == eType) && (it->iCreationTurn > iMaxTurn))
		{
			if ((eType != PROPOSAL_IRR) || (it->eProposalOwner == ePlayerID))  // personal cooldown for IRR
				iMaxTurn = it->iCreationTurn;
		}
	}

	if (iMaxTurn == -1)
	{
		return false;
	}

	if (eType == PROPOSAL_IRR)
	{
		if (GC.getGame().getGameTurn() - iMaxTurn < PROPOSAL_COOLDOWN)
			return true;
	}
	else if (eType == PROPOSAL_CC)
	{
		if (GC.getGame().getGameTurn() - iMaxTurn < PROPOSAL_COOLDOWN)
			return true;
	}
	else if (eType == PROPOSAL_SCRAP)
	{
	}

	return false;
}

bool CvMPVotingSystem::IsProposalTypeAvailable(MPVotingSystemProposalTypes eType)
{
#ifdef TOURNAMENT_VOTING_SYSTEM_CHANGES
	if (GC.getGame().isOption("GAMEOPTION_TOURNAMENT_MODE"))
	{
		// no IRR until first player enters Industrial era
		if (eType == PROPOSAL_IRR)
		{
			EraTypes eMostAdvancedEra = NO_ERA;
			for (int i = 0; i < MAX_MAJOR_CIVS; i++)
			{
				EraTypes e = GET_PLAYER((PlayerTypes)i).GetCurrentEra();
				if (GET_PLAYER((PlayerTypes)i).GetCurrentEra() > eMostAdvancedEra)
				{
					eMostAdvancedEra = e;
				}
			}

			if (eMostAdvancedEra < GC.getInfoTypeForString("ERA_INDUSTRIAL"))
			{
				return false;
			}
		}

		// no CC until turn 100
		if (eType == PROPOSAL_CC)
		{
			if (GC.getGame().getGameTurn() < 100)
			{
				return false;
			}
		}

		// no scraps!
		if (eType == PROPOSAL_SCRAP)
		{
			return false;
		}
	}
#endif
	return true;
}

void CvMPVotingSystem::DoTurn()
{
	for (ProposalList::iterator it = m_vProposals.begin(); it != m_vProposals.end(); ++it)
	{
		PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
		CvString sMessage, sSummary;
		NotificationTypes eType = NO_NOTIFICATION_TYPE;
		if (!it->bComplete)
		{
			SetProposalExpirationCounter(it->iID, it->iExpirationCounter - 1);

			if (it->iExpirationCounter >= 0)
			{
				if (it->eType == PROPOSAL_IRR)  // first create matching notification
				{
					eType = (NotificationTypes)NOTIFICATION_MP_IRR_PROPOSAL;
					sMessage = GetLocalizedText("TXT_KEY_MP_MESSAGE_PROPOSED_IRR", GET_PLAYER(it->eProposalOwner).getName());
					sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_IRR_PROPOSAL");
				}
				else if (it->eType == PROPOSAL_CC)
				{
					eType = (NotificationTypes)NOTIFICATION_MP_CC_PROPOSAL;
					sMessage = GetLocalizedText("TXT_KEY_MP_MESSAGE_PROPOSED_CC", GET_PLAYER(it->eProposalOwner).getName());
					sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_CC_PROPOSAL");
				}
				else if (it->eType == PROPOSAL_SCRAP)
				{
					eType = (NotificationTypes)NOTIFICATION_MP_SCRAP_PROPOSAL;
					sMessage = GetLocalizedText("TXT_KEY_MP_MESSAGE_PROPOSED_SCRAP", GET_PLAYER(it->eProposalOwner).getName());
					sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_SCRAP_PROPOSAL");
				}
				else if (it->eType == PROPOSAL_REMAP)
				{
					if (CvPreGame::IsHasRemapToken(eActivePlayer))
					{
						eType = (NotificationTypes)NOTIFICATION_MP_REMAP_PROPOSAL;
						sMessage = GetLocalizedText("TXT_KEY_MP_MESSAGE_PROPOSED_REMAP");
						sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_REMAP_PROPOSAL");
					}
				}

				for (int iNotifyLoop = 0; iNotifyLoop < MAX_MAJOR_CIVS; ++iNotifyLoop) {
					CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iNotifyLoop);
					CvNotifications* pNotifications = kPlayer.GetNotifications();
					if (kPlayer.isAlive())
					{
						if ((it->eType != PROPOSAL_REMAP) || CvPreGame::IsHasRemapToken((PlayerTypes)iNotifyLoop))
						{
							if (pNotifications)
							{
								pNotifications->Add(eType, sMessage, sSummary, -1, -1, it->iID);
							}
						}
					}
				}
				DoCheckVoters(it->iID);
				DoUpdateProposalStatus(it->iID);
			}
			else  // proposal expired!
			{
				for (int i = 0; i < MAX_MAJOR_CIVS; i++)
				{
					if (GetVoterEligibility(it->iID, (PlayerTypes)i) && !GetVoterHasVoted(it->iID, (PlayerTypes)i))  // fill all missing votes with NO
					{
						SetVoterHasVoted(it->iID, (PlayerTypes)i, true);
						SetVoterVote(it->iID, (PlayerTypes)i, false);
					}
				}
				DoUpdateProposalStatus(it->iID);
			}
		}
	}
}

void CvMPVotingSystem::AddProposal(MPVotingSystemProposalTypes eProposalType, PlayerTypes eProposalOwner, PlayerTypes eProposalSubject)
{
	int ID = GetLastProposalID();
	m_iLastProposalID++;

	Proposal proposal;
	proposal.iID = ID;
	proposal.iCreationTurn = GC.getGame().getGameTurn();
	proposal.iExpirationCounter = 2;
	proposal.eType = eProposalType;
	proposal.eProposalOwner = eProposalOwner;
	proposal.eProposalSubject = eProposalSubject;
	proposal.vVotes.at((int)eProposalOwner) = true;  // owner supports proposal by default
	proposal.vHasVoted.at((int)eProposalOwner) = true;
	if (eProposalSubject != NO_PLAYER)
		proposal.vIsEligible.at(eProposalSubject) = false;  // subject can't vote

	CvString sMessage, sSummary;
	NotificationTypes eType = NO_NOTIFICATION_TYPE;

	if (eProposalType == PROPOSAL_IRR)  // first create matching notification
	{
		eType = (NotificationTypes)NOTIFICATION_MP_IRR_PROPOSAL;
		sMessage = GetLocalizedText("TXT_KEY_MP_MESSAGE_PROPOSED_IRR", GET_PLAYER(eProposalOwner).getName());
		sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_IRR_PROPOSAL");
	}
	else if (eProposalType == PROPOSAL_CC)
	{
		eType = (NotificationTypes)NOTIFICATION_MP_CC_PROPOSAL;
		sMessage = GetLocalizedText("TXT_KEY_MP_MESSAGE_PROPOSED_CC", GET_PLAYER(eProposalOwner).getName(), GET_PLAYER(eProposalSubject).getName());
		sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_CC_PROPOSAL");
	}
	else if (eProposalType == PROPOSAL_SCRAP)
	{
		eType = (NotificationTypes)NOTIFICATION_MP_SCRAP_PROPOSAL;
		sMessage = GetLocalizedText("TXT_KEY_MP_MESSAGE_PROPOSED_SCRAP", GET_PLAYER(eProposalOwner).getName());
		sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_SCRAP_PROPOSAL");
	}
	else if (eProposalType == PROPOSAL_REMAP)
	{
		proposal.iExpirationCounter = REMAP_PROPOSAL_REVEAL_TURN;
		for (int i = 0; i < MAX_MAJOR_CIVS; i++)
		{
			if (!CvPreGame::IsHasRemapToken((PlayerTypes)i))
			{
				proposal.vIsEligible.at(i) = false; 
			}
		}
		eType = (NotificationTypes)NOTIFICATION_MP_REMAP_PROPOSAL;
		sMessage = GetLocalizedText("TXT_KEY_MP_MESSAGE_PROPOSED_REMAP");
		sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_REMAP_PROPOSAL");
	}

	m_vProposals.push_back(proposal);

	for (int iNotifyLoop = 0; iNotifyLoop < MAX_MAJOR_CIVS; ++iNotifyLoop) {
		CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iNotifyLoop);
		CvNotifications* pNotifications = kPlayer.GetNotifications();
		if (kPlayer.isAlive())
		{
			if ((eProposalType != PROPOSAL_REMAP) || CvPreGame::IsHasRemapToken((PlayerTypes)iNotifyLoop))
			{
				if (pNotifications)
				{
					pNotifications->Add(eType, sMessage, sSummary, -1, -1, ID);
				}
			}
		}
	}

	DoCheckVoters(ID);
	DoUpdateProposalStatus(ID);
}

void CvMPVotingSystem::DoVote(int iProposalID, PlayerTypes ePlayerID, bool bVote)
{
	if (!GetProposalByID(iProposalID)->bComplete && GetProposalByID(iProposalID)->vIsEligible.at((int)ePlayerID))
	{
		SetVoterHasVoted(iProposalID, ePlayerID, true);
		SetVoterVote(iProposalID, ePlayerID, bVote);
		ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
		CvLuaArgsHandle args;
		bool bResult;
		if (pkScriptSystem)
		{
			args->Push(iProposalID);  // proposal id
			args->Push((int)ePlayerID);  // voter id
			args->Push(bVote);
			LuaSupport::CallHook(pkScriptSystem, "MPVotingSystemVote", args.get(), bResult);
		}
		DoCheckVoters(iProposalID);
		DoUpdateProposalStatus(iProposalID);
	}
}

void CvMPVotingSystem::SetProposalExpirationCounter(int iProposalID, int iValue)
{
	m_vProposals.at(iProposalID).iExpirationCounter = iValue;
}

void CvMPVotingSystem::SetProposalType(int iProposalID, MPVotingSystemProposalTypes eType)
{
	m_vProposals.at(iProposalID).eType = eType;
}

void CvMPVotingSystem::SetProposalStatus(int iProposalID, MPVotingSystemProposalStatus eStatus)
{
	m_vProposals.at(iProposalID).eStatus = eStatus;
}

void CvMPVotingSystem::SetProposalOwner(int iProposalID, PlayerTypes eOwner)
{
	m_vProposals.at(iProposalID).eProposalOwner = eOwner;
}

void CvMPVotingSystem::SetProposalSubject(int iProposalID, PlayerTypes eSubject)
{
	m_vProposals.at(iProposalID).eProposalSubject = eSubject;
}

void CvMPVotingSystem::SetProposalCompletion(int iProposalID, bool bValue)
{
	m_vProposals.at(iProposalID).bComplete = bValue;
}

void CvMPVotingSystem::SetVoterEligibility(int iProposalID, PlayerTypes ePlayerID, bool bValue)
{
	m_vProposals.at(iProposalID).vIsEligible.at((int)ePlayerID) = bValue;
}

void CvMPVotingSystem::SetVoterHasVoted(int iProposalID, PlayerTypes ePlayerID, bool bValue)
{
	m_vProposals.at(iProposalID).vHasVoted.at((int)ePlayerID) = bValue;
}

void CvMPVotingSystem::SetVoterVote(int iProposalID, PlayerTypes ePlayerID, bool bValue)
{
	m_vProposals.at(iProposalID).vVotes.at((int)ePlayerID) = bValue;
}

void CvMPVotingSystem::DoCheckVoters(int iProposalID)
{
	Proposal pProposal = *GetProposalByID(iProposalID);
	if (!pProposal.bComplete)
	{
		for (int i = 0; i < MAX_MAJOR_CIVS; i++)
		{
			CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)i);
			if (GetVoterEligibility(iProposalID, (PlayerTypes)i) && (!kPlayer.isHuman() || !kPlayer.isAlive()))
			{
				// was voter before but now is dead: exclude from voters list
				SetVoterEligibility(iProposalID, (PlayerTypes)i, false);

				if (pProposal.eProposalOwner == (PlayerTypes)i || pProposal.eProposalSubject == (PlayerTypes)i)
				{
					// proposal owner/subject has been defeated, repeal the proposal!
					SetProposalStatus(iProposalID, STATUS_FAILED);
					SetProposalCompletion(iProposalID, true);


					ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
					CvLuaArgsHandle args;
					bool bResult;
					if (pkScriptSystem)  // inform UI
					{
						args->Push(iProposalID);
						args->Push((int)GetProposalExpirationCounter(iProposalID));
						args->Push((int)GetProposalOwner(iProposalID));
						args->Push((int)GetProposalSubject(iProposalID));
						args->Push((int)GetProposalType(iProposalID));
						args->Push((int)GetProposalStatus(iProposalID));
						LuaSupport::CallHook(pkScriptSystem, "MPVotingSystemProposalResult", args.get(), bResult);
					}
#ifdef REPLAY_EVENTS
					std::vector<int> vArgs;
					vArgs.push_back(iProposalID);
					vArgs.push_back(static_cast<int>(GetProposalType(iProposalID)));
					vArgs.push_back(static_cast<int>(GetProposalSubject(iProposalID)));
					vArgs.push_back(GetProposalStatus(iProposalID) == STATUS_PASSED);
					GC.getGame().addReplayEvent(REPLAYEVENT_MPProposalResult, GetProposalOwner(iProposalID), vArgs);
#endif

					CvString sMessage;
					CvString sSummary;
					if (pProposal.eType == PROPOSAL_IRR)
					{
						sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_IRR_PROPOSAL_FAILED", GET_PLAYER(pProposal.eProposalOwner).getName());
						sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_IRR_PROPOSAL_FAILED");
					}
					else if (pProposal.eType == PROPOSAL_CC)
					{
						sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_CC_PROPOSAL_FAILED", GET_PLAYER(pProposal.eProposalOwner).getName());
						sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_CC_PROPOSAL_FAILED");
					}
					else if (pProposal.eType == PROPOSAL_SCRAP)
					{
						sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_SCRAP_PROPOSAL_FAILED", GET_PLAYER(pProposal.eProposalOwner).getName());
						sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_SCRAP_PROPOSAL_FAILED");
					}

					for (int iNotifyLoop = 0; iNotifyLoop < MAX_MAJOR_CIVS; ++iNotifyLoop) {
						CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iNotifyLoop);
						CvNotifications* pNotifications = kPlayer.GetNotifications();
						if (kPlayer.isAlive())
						{
							if (pNotifications)
							{
								pNotifications->Add((NotificationTypes)NOTIFICATION_MP_PROPOSAL_RESULT, sMessage, sSummary, -1, -1, iProposalID);
							}
						}
					}
				}
			}
			else if (!pProposal.vIsEligible.at(i) && kPlayer.isHuman() && kPlayer.isAlive())
			{
				if ((int)pProposal.eProposalSubject != i && (pProposal.eType != PROPOSAL_REMAP || CvPreGame::IsHasRemapToken((PlayerTypes)i)))
					SetVoterEligibility(iProposalID, (PlayerTypes)i, true);
				// new eligible voter since last check: do nothing (by now) -- TODO rebroadcast proposal notification for new players
			}
		}
	}
}

void CvMPVotingSystem::DoUpdateProposalStatus(int iProposalID)
{
	if (GetProposalCompletion(iProposalID))
		return;
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	CvLuaArgsHandle args;
	bool bResult;

	int	yesVotes = GetYesVotes(iProposalID);
	int noVotes = GetNoVotes(iProposalID);
	int maxVoters = GetMaxVotes(iProposalID);
	int totalVotes = yesVotes + noVotes;
	int irrVotersThreshold = maxVoters - (maxVoters + 1) * (100 - IRR_THRESHOLD_TIMES_100) / 100;
#ifdef TOURNAMENT_VOTING_SYSTEM_CHANGES
	// if tournament mode is on, raise threshold to 100 
	if (GC.getGame().isOption("GAMEOPTION_TOURNAMENT_MODE"))
	{
		irrVotersThreshold = maxVoters;
	}
#endif

	MPVotingSystemProposalTypes eType = GetProposalType(iProposalID);
	if (eType == NO_PROPOSAL) {
		SetProposalStatus(iProposalID, STATUS_INVALID);
	}
	else if (eType == PROPOSAL_IRR) {
		if ((totalVotes < maxVoters) && (yesVotes < irrVotersThreshold))
			SetProposalStatus(iProposalID, STATUS_ACTIVE);
		else if (yesVotes >= irrVotersThreshold)
			SetProposalStatus(iProposalID, STATUS_PASSED);
		else if (noVotes > maxVoters - irrVotersThreshold)
			SetProposalStatus(iProposalID, STATUS_FAILED);
		else
			SetProposalStatus(iProposalID, STATUS_INVALID);
	}
	else if (eType == PROPOSAL_CC || eType == PROPOSAL_SCRAP) {
		if (totalVotes < maxVoters)
			SetProposalStatus(iProposalID, STATUS_ACTIVE);
		else if (yesVotes == maxVoters)
			SetProposalStatus(iProposalID, STATUS_PASSED);
		else if (eType == PROPOSAL_SCRAP && noVotes == 1)  // scrap special case: if there is only one nay voter, make them winner
		{
			// find no voter
			Proposal pProposal = *GetProposalByID(iProposalID);
			PlayerTypes eVoter = NO_PLAYER;
			for (uint i = 0; i < MAX_MAJOR_CIVS; i++)
			{
				if (pProposal.vIsEligible.at(i) && pProposal.vHasVoted.at(i) && !pProposal.vVotes.at(i))
					eVoter = static_cast<PlayerTypes>(i);
			}
			// set winner
			SetProposalSubject(iProposalID, eVoter);
			SetProposalStatus(iProposalID, STATUS_PASSED);
		}
		else if (noVotes > 0)
			SetProposalStatus(iProposalID, STATUS_FAILED);
		else
			SetProposalStatus(iProposalID, STATUS_INVALID);
	}
	else if (eType == PROPOSAL_REMAP) {
		if (GetProposalExpirationCounter(iProposalID) >= 0)
		{
			if (totalVotes < maxVoters)
				SetProposalStatus(iProposalID, STATUS_ACTIVE);
			else if (yesVotes > 0)
				SetProposalStatus(iProposalID, STATUS_PASSED);
			else if (noVotes == maxVoters)
				SetProposalStatus(iProposalID, STATUS_FAILED);
			else
				SetProposalStatus(iProposalID, STATUS_INVALID);
		}
		else
		{
			if (yesVotes > 0)
				SetProposalStatus(iProposalID, STATUS_PASSED);
			else
				SetProposalStatus(iProposalID, STATUS_FAILED);
		}
	}
	else {
		SetProposalStatus(iProposalID, STATUS_INVALID);
	}

	if (GetProposalStatus(iProposalID) == STATUS_PASSED || GetProposalStatus(iProposalID) == STATUS_FAILED || GetProposalStatus(iProposalID) == STATUS_INVALID) {
		SetProposalCompletion(iProposalID, true);
		
		DLLUI->AddMessage(0, CvPreGame::activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), GetLocalizedText("TXT_KEY_MESSAGE_MP_PROPOSAL_FINISHED"));
		if (pkScriptSystem)  // inform UI
		{
			args->Push(iProposalID);
			args->Push((int)GetProposalExpirationCounter(iProposalID));
			args->Push((int)GetProposalOwner(iProposalID));
			args->Push((int)GetProposalSubject(iProposalID));
			args->Push((int)GetProposalType(iProposalID));
			args->Push((int)GetProposalStatus(iProposalID));
			LuaSupport::CallHook(pkScriptSystem, "MPVotingSystemProposalResult", args.get(), bResult);
		}
#ifdef REPLAY_EVENTS
		std::vector<int> vArgs;
		vArgs.push_back(iProposalID);
		vArgs.push_back(static_cast<int>(GetProposalType(iProposalID)));
		vArgs.push_back(static_cast<int>(GetProposalSubject(iProposalID)));
		vArgs.push_back(GetProposalStatus(iProposalID) == STATUS_PASSED);
		GC.getGame().addReplayEvent(REPLAYEVENT_MPProposalResult, GetProposalOwner(iProposalID), vArgs);
#endif

		CvString sMessage;
		CvString sSummary;
		if (GetProposalStatus(iProposalID) == STATUS_PASSED) {
			if (eType == PROPOSAL_IRR)
			{
				sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_IRR_PROPOSAL_PASSED", GET_PLAYER(GetProposalOwner(iProposalID)).getName());
				sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_IRR_PROPOSAL_PASSED");
			}
			else if (eType == PROPOSAL_CC)
			{
				sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_CC_PROPOSAL_PASSED", GET_PLAYER(GetProposalOwner(iProposalID)).getName());
				sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_CC_PROPOSAL_PASSED");
			}
			else if (eType == PROPOSAL_SCRAP)
			{
				sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_SCRAP_PROPOSAL_PASSED", GET_PLAYER(GetProposalOwner(iProposalID)).getName());
				sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_SCRAP_PROPOSAL_PASSED");
			}
			else if (eType == PROPOSAL_REMAP)
			{
				sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_REMAP_PROPOSAL_PASSED");
				sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_REMAP_PROPOSAL_PASSED");
			}
		}
		else if (GetProposalStatus(iProposalID) == STATUS_FAILED || GetProposalStatus(iProposalID) == STATUS_INVALID) {
			if (eType == PROPOSAL_IRR)
			{
				sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_IRR_PROPOSAL_FAILED", GET_PLAYER(GetProposalOwner(iProposalID)).getName());
				sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_IRR_PROPOSAL_FAILED");
			}
			else if (eType == PROPOSAL_CC)
			{
				sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_CC_PROPOSAL_FAILED", GET_PLAYER(GetProposalOwner(iProposalID)).getName());
				sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_CC_PROPOSAL_FAILED");
			}
			else if (eType == PROPOSAL_SCRAP)
			{
				sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_SCRAP_PROPOSAL_FAILED", GET_PLAYER(GetProposalOwner(iProposalID)).getName());
				sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_SCRAP_PROPOSAL_FAILED");
			}
			else if (eType == PROPOSAL_REMAP)
			{
				sMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_MP_REMAP_PROPOSAL_FAILED");
				sSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_MP_REMAP_PROPOSAL_FAILED");
			}
		}

		for (int iNotifyLoop = 0; iNotifyLoop < MAX_MAJOR_CIVS; ++iNotifyLoop) {
			CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iNotifyLoop);
			CvNotifications* pNotifications = kPlayer.GetNotifications();
			if (kPlayer.isAlive())
			{
				if (pNotifications)
				{
					pNotifications->Add((NotificationTypes)NOTIFICATION_MP_PROPOSAL_RESULT, sMessage, sSummary, -1, -1, iProposalID);
				}
			}
		}
	}
}
FDataStream& operator>>(FDataStream& loadFrom, MPVotingSystemProposalTypes& writeTo)
{
	int v;
	loadFrom >> v;
	writeTo = static_cast<MPVotingSystemProposalTypes>(v);
	return loadFrom;
}

FDataStream& operator<<(FDataStream& saveTo, const MPVotingSystemProposalTypes& readFrom)
{
	saveTo << static_cast<int>(readFrom);
	return saveTo;
}

//------------------------------------------------------------------------------
FDataStream& operator>>(FDataStream& loadFrom, MPVotingSystemProposalStatus& writeTo)
{
	int v;
	loadFrom >> v;
	writeTo = static_cast<MPVotingSystemProposalStatus>(v);
	return loadFrom;
}

FDataStream& operator<<(FDataStream& saveTo, const MPVotingSystemProposalStatus& readFrom)
{
	saveTo << static_cast<int>(readFrom);
	return saveTo;
}

//------------------------------------------------------------------------------
FDataStream& operator>>(FDataStream& loadFrom, CvMPVotingSystem::Proposal& writeTo)
{
	SLOG("--- proposal read invoked WTF ---");
	// Version number to maintain backwards compatibility
	uint uiVersion;
	loadFrom >> uiVersion;
	loadFrom >> writeTo.iID;
	loadFrom >> writeTo.iUIid;
	loadFrom >> writeTo.iCreationTurn;
	loadFrom >> writeTo.iExpirationCounter;
	loadFrom >> writeTo.eType;
	loadFrom >> writeTo.eStatus;
	loadFrom >> writeTo.eProposalOwner;
	loadFrom >> writeTo.eProposalSubject;
	loadFrom >> writeTo.bComplete;
	for (uint j = 0; j < MAX_MAJOR_CIVS; j++)
	{
		bool bTemp1;
		bool bTemp2;
		bool bTemp3;
		loadFrom >> bTemp1;
		loadFrom >> bTemp2;
		loadFrom >> bTemp3;
		writeTo.vIsEligible.push_back(bTemp1);
		writeTo.vHasVoted.push_back(bTemp2);
		writeTo.vVotes.push_back(bTemp3);
	}

	return loadFrom;
}

FDataStream& operator<<(FDataStream& saveTo, const CvMPVotingSystem::Proposal& readFrom)
{
	SLOG("--- proposal write invoked WTF ---");
	uint uiVersion = BUMP_SAVE_VERSION_MP_VOTING_SYSTEM;
	saveTo << uiVersion;
	saveTo << readFrom.iID;
	saveTo << readFrom.iUIid;
	saveTo << readFrom.iCreationTurn;
	saveTo << readFrom.iExpirationCounter;
	saveTo << readFrom.eType;
	saveTo << readFrom.eStatus;
	saveTo << readFrom.eProposalOwner;
	saveTo << readFrom.eProposalSubject;
	saveTo << readFrom.bComplete;
	for (uint j = 0; j < MAX_MAJOR_CIVS; j++)
	{
		saveTo << readFrom.vIsEligible.at(j);
		saveTo << readFrom.vHasVoted.at(j);
		saveTo << readFrom.vVotes.at(j);
	}

	return saveTo;
}

//------------------------------------------------------------------------------
// Serialization Read
FDataStream& operator>>(FDataStream& loadFrom, CvMPVotingSystem& writeTo)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	loadFrom >> uiVersion;
	uint uiNumProposals;
	bool bTemp;
	CvMPVotingSystem::Proposal temp;
	loadFrom >> uiNumProposals;
	writeTo.m_iLastProposalID = uiNumProposals;
	writeTo.m_vProposals.clear();
	for (uint i = 0; i < uiNumProposals; i++)
	{
		loadFrom >> temp.iID;
		loadFrom >> temp.iUIid;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
		if (uiVersion >= 1000)
#endif
		loadFrom >> temp.iCreationTurn;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
		else
		{
			temp.iCreationTurn = temp.iExpirationCounter - 2;
		}
#endif
		loadFrom >> temp.iExpirationCounter;
		loadFrom >> temp.eType;
		loadFrom >> temp.eStatus;
		loadFrom >> temp.eProposalOwner;
		loadFrom >> temp.eProposalSubject;
		loadFrom >> temp.bComplete;
		temp.vIsEligible.clear();
		temp.vHasVoted.clear();
		temp.vVotes.clear();
		for (uint j = 0; j < MAX_MAJOR_CIVS; j++)
		{
			loadFrom >> bTemp;
			temp.vIsEligible.push_back(bTemp);
			loadFrom >> bTemp;
			temp.vHasVoted.push_back(bTemp);
			loadFrom >> bTemp;
			temp.vVotes.push_back(bTemp);
		}
		writeTo.m_vProposals.push_back(temp);
	}

	return loadFrom;
}

// Serialization Write
FDataStream& operator<<(FDataStream& saveTo, const CvMPVotingSystem& readFrom)
{
	// Current version number
	uint uiVersion = 0;
#ifdef SAVE_BACKWARDS_COMPATIBILITY
	uiVersion = BUMP_SAVE_VERSION_MP_VOTING_SYSTEM;
#endif
	saveTo << uiVersion;
	uint uiNumProposals = readFrom.m_iLastProposalID;
	saveTo << uiNumProposals;
	for (uint i = 0; i < uiNumProposals; i++)
	{
		saveTo << i;
		saveTo << -1;
		saveTo << readFrom.m_vProposals.at(i).iCreationTurn;
		saveTo << readFrom.m_vProposals.at(i).iExpirationCounter;
		saveTo << readFrom.m_vProposals.at(i).eType;
		saveTo << readFrom.m_vProposals.at(i).eStatus;
		saveTo << readFrom.m_vProposals.at(i).eProposalOwner;
		saveTo << readFrom.m_vProposals.at(i).eProposalSubject;
		saveTo << readFrom.m_vProposals.at(i).bComplete;
		for (uint j = 0; j < MAX_MAJOR_CIVS; j++)
		{
			saveTo << readFrom.m_vProposals.at(i).vIsEligible.at(j);
			saveTo << readFrom.m_vProposals.at(i).vHasVoted.at(j);
			saveTo << readFrom.m_vProposals.at(i).vVotes.at(j);
		}
	}

	return saveTo;
}

#endif
