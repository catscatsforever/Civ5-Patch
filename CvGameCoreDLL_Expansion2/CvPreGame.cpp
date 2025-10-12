/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreUtils.h"
#include "CvPreGame.h"
#include "FIGameIniParser.h"
#include "FLua/Include/FLua.h"
#include "FStlContainerSerialization.h"
#include "FFileStream.h"
#include "ICvDLLDatabaseUtility.h"
#include "CvInfosSerializationHelper.h"

#include <unordered_map>
#ifdef INGAME_CIV_DRAFTER
# include <Wincrypt.h>
#endif

// Include this after all other headers.
#include "LintFree.h"

//////////////////////////////////////////////////////////////////////////
//
// WARNING: Do not use any of the GC.*Info tables in this file.
//			These Game Core tables are not always current to what the loaded
//			database set contains.  This is the case when setting up a multiplayer
//			game or even in a single player game if you have returned from a multiplayer game.
//			The reason the Game Core data is not kept in sync is because the caching of all the data
//			can take a significant amount of time.

#define PREGAMEVARDEFAULT(a, b) FAutoVariable<a, Phony> b("CvPreGame::"#b, s_preGameArchive);
#define PREGAMEVAR(a, b, c) FAutoVariable<a, Phony> b("CvPreGame::"#b, s_preGameArchive, c, false);


//Basic translation function to help with the glue between the old
//GameOptionTypes enumeration usage vs the newer system that uses strings.
const char* ConvertGameOptionTypeToString(GameOptionTypes eOption)
{
	switch(eOption)
	{
	case GAMEOPTION_NO_CITY_RAZING:
		return "GAMEOPTION_NO_CITY_RAZING";
	case GAMEOPTION_NO_BARBARIANS:
		return "GAMEOPTION_NO_BARBARIANS";
	case GAMEOPTION_RAGING_BARBARIANS:
		return "GAMEOPTION_RAGING_BARBARIANS";
	case GAMEOPTION_ALWAYS_WAR:
		return "GAMEOPTION_ALWAYS_WAR";
	case GAMEOPTION_ALWAYS_PEACE:
		return "GAMEOPTION_ALWAYS_PEACE";
	case GAMEOPTION_ONE_CITY_CHALLENGE:
		return "GAMEOPTION_ONE_CITY_CHALLENGE";
	case GAMEOPTION_NO_CHANGING_WAR_PEACE:
		return "GAMEOPTION_NO_CHANGING_WAR_PEACE";
	case GAMEOPTION_NEW_RANDOM_SEED:
		return "GAMEOPTION_NEW_RANDOM_SEED";
	case GAMEOPTION_LOCK_MODS:
		return "GAMEOPTION_LOCK_MODS";
	case GAMEOPTION_COMPLETE_KILLS:
		return "GAMEOPTION_COMPLETE_KILLS";
	case GAMEOPTION_NO_GOODY_HUTS:
		return "GAMEOPTION_NO_GOODY_HUTS";
	case GAMEOPTION_RANDOM_PERSONALITIES:
		return "GAMEOPTION_RANDOM_PERSONALITIES";
	case GAMEOPTION_POLICY_SAVING:
		return "GAMEOPTION_POLICY_SAVING";
	case GAMEOPTION_PROMOTION_SAVING:
		return "GAMEOPTION_PROMOTION_SAVING";
	case GAMEOPTION_END_TURN_TIMER_ENABLED:
		return "GAMEOPTION_END_TURN_TIMER_ENABLED";
	case GAMEOPTION_QUICK_COMBAT:
		return "GAMEOPTION_QUICK_COMBAT";
	case GAMEOPTION_NO_SCIENCE:
		return "GAMEOPTION_NO_SCIENCE";
	case GAMEOPTION_NO_POLICIES:
		return "GAMEOPTION_NO_POLICIES";
	case GAMEOPTION_NO_HAPPINESS:
		return "GAMEOPTION_NO_HAPPINESS";
	case GAMEOPTION_NO_TUTORIAL:
		return "GAMEOPTION_NO_TUTORIAL";
	case GAMEOPTION_NO_RELIGION:
		return "GAMEOPTION_NO_RELIGION";
	}

	return NULL;
}

namespace CvPreGame
{

static const GUID s_emptyGUID = { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };
static PackageIDList s_emptyGUIDList;

//------------------------------------------------------------------------------
// CustomOption
//------------------------------------------------------------------------------
CustomOption::CustomOption()
	: m_iValue(-1)
{
	memset(m_szOptionName, 0, 64);
}
//------------------------------------------------------------------------------
CustomOption::CustomOption(const char* szOptionName, int iVal)
	: m_iValue(iVal)
{
	strcpy_s(m_szOptionName, 64, szOptionName);
}
//------------------------------------------------------------------------------
CustomOption::CustomOption(const CustomOption& copy)
{
	m_iValue = copy.m_iValue;
	strcpy_s(m_szOptionName, copy.m_szOptionName);
}
//------------------------------------------------------------------------------
CustomOption& CustomOption::operator=(const CustomOption& rhs)
{
	m_iValue = rhs.m_iValue;
	strcpy_s(m_szOptionName, rhs.m_szOptionName);
	return (*this);
}
//------------------------------------------------------------------------------
bool CustomOption::operator ==(const CustomOption& option) const
{
	if(m_iValue == option.m_iValue)
	{
		if(strncmp(m_szOptionName, option.m_szOptionName, 64) == 0)
			return true;
	}

	return false;
}
//------------------------------------------------------------------------------
const char* CustomOption::GetName(size_t& bytes) const
{
	bytes = strlen(m_szOptionName);
	return m_szOptionName;
}
//------------------------------------------------------------------------------
const char* CustomOption::GetName() const
{
	return m_szOptionName;
}
//------------------------------------------------------------------------------
int CustomOption::GetValue() const
{
	return m_iValue;
}
//------------------------------------------------------------------------------
FDataStream& operator>>(FDataStream& stream, CustomOption& option)
{
	FString optionName;

	stream >> optionName;
	stream >> option.m_iValue;

	strcpy_s(option.m_szOptionName, 64, optionName.c_str());

	return stream;
}
//------------------------------------------------------------------------------
FDataStream& operator<<(FDataStream& stream, const CustomOption& option)
{
	FString optionName(option.m_szOptionName, strlen(option.m_szOptionName));
	stream << optionName;
	stream << option.m_iValue;

	return stream;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void StringToBools(const char* szString, int* iNumBools, bool** ppBools);

struct Phony
{
	std::string debugDump(const FAutoVariableBase&)
	{
		return "";
	}
	std::string stackTraceRemark(const FAutoVariableBase&)
	{
		return "";
	}
};
Phony phony;


FAutoArchiveClassContainer<Phony>                       s_preGameArchive(phony);

PREGAMEVAR(PlayerTypes,                        s_activePlayer,           NO_PLAYER);
PREGAMEVARDEFAULT(CvString,                           s_adminPassword);
PREGAMEVAR(int,                                s_advancedStartPoints,    0);
PREGAMEVARDEFAULT(CvString,                           s_alias);
PREGAMEVAR(std::vector<ArtStyleTypes>,         s_artStyles,              MAX_PLAYERS);
PREGAMEVAR(bool,                               s_autorun,                false);
PREGAMEVAR(float,                              s_autorunTurnDelay,       0.0f);
PREGAMEVAR(int,                                s_autorunTurnLimit,       0);
PREGAMEVAR(BandwidthType,                      s_bandwidth,              NO_BANDWIDTH);
PREGAMEVAR(CalendarTypes,                      s_calendar,               NO_CALENDAR);
PREGAMEVARDEFAULT(CvBaseInfo,                         s_calendarInfo);
PREGAMEVAR(std::vector<CvString>,              s_civAdjectives,          MAX_PLAYERS);
PREGAMEVAR(std::vector<CvString>,              s_civDescriptions,        MAX_PLAYERS);
PREGAMEVAR(std::vector<CivilizationTypes>,     s_civilizations,          MAX_PLAYERS);
PREGAMEVAR(std::vector<CvString>,              s_civPasswords,           MAX_PLAYERS);
PREGAMEVAR(std::vector<CvString>,              s_civShortDescriptions,   MAX_PLAYERS);
PREGAMEVAR(ClimateTypes,                       s_climate,                NO_CLIMATE);
PREGAMEVARDEFAULT(CvClimateInfo,                      s_climateInfo);
PREGAMEVAR(EraTypes,                           s_era,                    NO_ERA);
PREGAMEVAR(std::vector<CvString>,              s_emailAddresses,         MAX_PLAYERS);
PREGAMEVAR(float,                              s_endTurnTimerLength,     0.0f);
PREGAMEVAR(std::vector<CvString>,              s_flagDecals,             MAX_PLAYERS);
PREGAMEVAR(std::vector<bool>,                  s_DEPRECATEDforceControls, 7);			//This was removed during the Day 0 patch since it is no longer used anywhere.
PREGAMEVAR(GameMode,                           s_gameMode,               NO_GAMEMODE);
PREGAMEVARDEFAULT(CvString,                           s_gameName);
PREGAMEVAR(GameSpeedTypes,                     s_gameSpeed,              NO_GAMESPEED);
PREGAMEVAR(bool,                               s_gameStarted,            false);
PREGAMEVAR(int,                                s_gameTurn,               -1);
PREGAMEVAR(GameTypes,                          s_gameType,               GAME_TYPE_NONE);
PREGAMEVAR(GameMapTypes,                       s_gameMapType,            GAME_USER_PARAMETERS);
PREGAMEVAR(int,                                s_gameUpdateTime,         0);
PREGAMEVAR(std::vector<HandicapTypes>,         s_handicaps,              MAX_PLAYERS);
PREGAMEVAR(std::vector<HandicapTypes>,         s_lastHumanHandicaps,     MAX_PLAYERS);
PREGAMEVAR(bool,								s_isEarthMap,			  false);
PREGAMEVAR(bool,                               s_isInternetGame,         false);
PREGAMEVAR(std::vector<LeaderHeadTypes>,       s_leaderHeads,            MAX_PLAYERS);
PREGAMEVAR(std::vector<CvString>,              s_leaderNames,            MAX_PLAYERS);
PREGAMEVARDEFAULT(CvString,                           s_loadFileName);
PREGAMEVARDEFAULT(CvString,                           s_localPlayerEmailAddress);
PREGAMEVAR(bool,                               s_mapNoPlayers,             false);
PREGAMEVAR(unsigned int,                       s_mapRandomSeed,            0);
PREGAMEVAR(bool,                               s_loadWBScenario,           false);
PREGAMEVAR(bool,                               s_overrideScenarioHandicap, false);
PREGAMEVARDEFAULT(CvString,                           s_mapScriptName);
PREGAMEVAR(int,                                s_maxCityElimination,     0);
PREGAMEVAR(int,                                s_maxTurns,               0);
PREGAMEVAR(int,                                s_numMinorCivs,           -1);
PREGAMEVAR(std::vector<MinorCivTypes>,         s_minorCivTypes,          MAX_PLAYERS);
PREGAMEVAR(std::vector<bool>,                  s_minorNationCivs,        MAX_PLAYERS);
PREGAMEVAR(bool,                               s_dummyvalue,	          false);
PREGAMEVAR(std::vector<bool>,                  s_multiplayerOptions,     NUM_MPOPTION_TYPES);
PREGAMEVAR(std::vector<int>,                   s_netIDs,                 MAX_PLAYERS);
PREGAMEVAR(std::vector<CvString>,              s_nicknames,              MAX_PLAYERS);
PREGAMEVAR(int,                                s_numVictoryInfos,        GC.getNumVictoryInfos());
PREGAMEVAR(int,                                s_pitBossTurnTime,        0);
PREGAMEVAR(std::vector<bool>,                  s_playableCivs,           MAX_PLAYERS);
PREGAMEVAR(std::vector<PlayerColorTypes>,      s_playerColors,           MAX_PLAYERS);
PREGAMEVAR(bool,                               s_privateGame,            false);
PREGAMEVAR(bool,                               s_quickCombat,            false);
PREGAMEVAR(bool,                               s_quickCombatDefault,     false);
PREGAMEVAR(HandicapTypes,                      s_quickHandicap,          NO_HANDICAP);
PREGAMEVAR(bool,                               s_quickstart,             false);
PREGAMEVAR(bool,                               s_randomWorldSize,        false);
PREGAMEVAR(bool,                               s_randomMapScript,        false);
PREGAMEVAR(std::vector<bool>,                  s_readyPlayers,           MAX_PLAYERS);
PREGAMEVAR(SeaLevelTypes,                      s_seaLevel,               NO_SEALEVEL);
PREGAMEVARDEFAULT(CvSeaLevelInfo,                     s_seaLevelInfo);
PREGAMEVAR(bool,                               s_dummyvalue2,	          false);
PREGAMEVAR(std::vector<SlotClaim>,             s_slotClaims,             MAX_PLAYERS);
PREGAMEVAR(std::vector<SlotStatus>,            s_slotStatus,             MAX_PLAYERS);
PREGAMEVARDEFAULT(CvString,                           s_smtpHost);
PREGAMEVAR(unsigned int,                       s_syncRandomSeed,         0);
PREGAMEVAR(int,                                s_targetScore,            0);
PREGAMEVAR(std::vector<TeamTypes>,             s_teamTypes,              MAX_PLAYERS);
PREGAMEVAR(bool,                               s_transferredMap,         false);
PREGAMEVARDEFAULT(CvTurnTimerInfo,                    s_turnTimer);
PREGAMEVAR(TurnTimerTypes,                     s_turnTimerType,          NO_TURNTIMER);
PREGAMEVAR(bool,                               s_bCityScreenBlocked,     false);
PREGAMEVAR(std::vector<bool>,                  s_victories,              s_numVictoryInfos);
PREGAMEVAR(std::vector<bool>,                  s_whiteFlags,             MAX_PLAYERS);
PREGAMEVARDEFAULT(CvWorldInfo,                        s_worldInfo);
PREGAMEVAR(WorldSizeTypes,                     s_worldSize,              NO_WORLDSIZE);
PREGAMEVARDEFAULT(std::vector<CustomOption>,			s_GameOptions);
PREGAMEVARDEFAULT(std::vector<CustomOption>,			s_MapOptions);
PREGAMEVARDEFAULT(std::string,                        s_versionString);
PREGAMEVAR(std::vector<bool>,                  s_turnNotifySteamInvite,        MAX_PLAYERS);
PREGAMEVAR(std::vector<bool>,                  s_turnNotifyEmail,							MAX_PLAYERS);
PREGAMEVAR(std::vector<CvString>,              s_turnNotifyEmailAddress,    MAX_PLAYERS);


typedef std::map<uint, uint> HashToOptionMap;

HashToOptionMap s_GameOptionsHash;
HashToOptionMap s_MapOptionsHash;

bool s_multiplayerAIEnabled = true; // default for RTM, change to true on street patch


std::map<PlayerTypes, CvString> s_displayNicknames; // JAR - workaround duplicate IDs vs display names

const std::vector<TeamTypes>& sr_TeamTypes = s_teamTypes;

PackageIDList s_AllowedDLC;

std::vector<CvString> s_civilizationKeys(MAX_PLAYERS);
std::vector<GUID> s_civilizationPackageID(MAX_PLAYERS);
std::vector<bool> s_civilizationKeysAvailable(MAX_PLAYERS);
std::vector<bool> s_civilizationKeysPlayable(MAX_PLAYERS);

std::vector<CvString> s_leaderKeys(MAX_PLAYERS);
std::vector<GUID> s_leaderPackageID(MAX_PLAYERS);
std::vector<bool> s_leaderKeysAvailable(MAX_PLAYERS);
std::vector<PackageIDList> s_DLCPackagesAvailable(MAX_PLAYERS);

std::vector<CvString> s_civAdjectivesLocalized(MAX_PLAYERS);
std::vector<CvString> s_civDescriptionsLocalized(MAX_PLAYERS);
std::vector<CvString> s_civShortDescriptionsLocalized(MAX_PLAYERS);
std::vector<CvString> s_leaderNamesLocalized(MAX_PLAYERS);

GameStartTypes	s_gameStartType;

StorageLocation	s_loadFileStorage;
#ifdef INGAME_CIV_DRAFTER

CvString s_draftLocalSecret = "";
CvString s_draftLocalSecretHash = "";
CvString s_draftShuffledCivs = "";
int s_draftPlayersReady = 0;
int s_draftCurrentProgress = DRAFT_PROGRESS_INIT;
int s_draftResult = DRAFT_RESULT_NONE;
std::vector<CvString> s_draftPlayerBans;
std::vector<CvString> s_draftPlayerSecrets(MAX_MAJOR_CIVS);
std::vector<CvString> s_draftPlayerSecretHashes(MAX_MAJOR_CIVS);
std::vector<CvString> s_draftBansMsgQueue(MAX_PLAYERS);
std::vector<bool> s_draftAllBansReceived(MAX_MAJOR_CIVS);
int s_draftCurrBansMsgNumber = 0;
CvString hash(CvString m) {
	uint H[8] = { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };
	uint K[64] = {
		0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
		0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
		0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
		0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
		0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
		0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
		0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
		0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
	};
	uint size = m.size();
	uint l = size * 8;
	uint pads = 64 - size % 64;
	if (pads < 9) {
		pads += 64;
	}
	uint8 pad[64] = { 0 };
	pad[0] = 0x80;
	uint8* l2 = reinterpret_cast<uint8*>(&l);
	l2[0] ^= l2[3];
	l2[3] ^= l2[0];
	l2[0] ^= l2[3];
	l2[1] ^= l2[2];
	l2[2] ^= l2[1];
	l2[1] ^= l2[2];
	for (uint i = 0; i < 4; i++) {
		pad[pads - 4 + i] = l2[i];
	}

	uint ind = 0;
	for (uint chunk = 0; chunk < (pads + size) / 64; chunk++) {
		uint8 msg[64 * 4];
		uint* msgw = reinterpret_cast<uint*> (msg);
		for (uint i = 0; i < 64; i++) {
			if (ind < size) {
				msg[i] = m[ind];
				ind++;
			}
			else {
				msg[i] = pad[ind - size];
				ind++;
			}
		}

		for (uint i = 0; i < 16; i++) {
			msg[4 * i + 0] ^= msg[4 * i + 3];
			msg[4 * i + 3] ^= msg[4 * i + 0];
			msg[4 * i + 0] ^= msg[4 * i + 3];
			msg[4 * i + 1] ^= msg[4 * i + 2];
			msg[4 * i + 2] ^= msg[4 * i + 1];
			msg[4 * i + 1] ^= msg[4 * i + 2];
		}

		for (uint i = 16; i < 64; i++) {
			uint S0 = ((msgw[i - 15] >> 7) | (msgw[i - 15] << (32 - 7))) ^ ((msgw[i - 15] >> 18) | (msgw[i - 15] << (32 - 18))) ^ (msgw[i - 15] >> 3);
			uint S1 = ((msgw[i - 2] >> 17) | (msgw[i - 2] << (32 - 17))) ^ ((msgw[i - 2] >> 19) | (msgw[i - 2] << (32 - 19))) ^ (msgw[i - 2] >> 10);
			msgw[i] = msgw[i - 16] + msgw[i - 7] + S0 + S1;
		}

		uint block[8];
		for (uint i = 0; i < 8; i++) {
			block[i] = H[i];
		}

		for (uint i = 0; i < 64; i++) {
			uint S1 = ((block[4] >> 6) | (block[4] << (32 - 6))) ^ ((block[4] >> 11) | (block[4] << (32 - 11))) ^ ((block[4] >> 25) | (block[4] << (32 - 25)));
			uint ch = (block[4] & block[5]) ^ ((~block[4]) & block[6]);
			uint t1 = block[7] + S1 + ch + K[i] + msgw[i];
			uint S0 = ((block[0] >> 2) | (block[0] << (32 - 2))) ^ ((block[0] >> 13) | (block[0] << (32 - 13))) ^ ((block[0] >> 22) | (block[0] << (32 - 22)));
			uint maj = (block[0] & block[1]) ^ (block[0] & block[2]) ^ (block[1] & block[2]);
			uint t2 = S0 + maj;

			block[7] = block[6];
			block[6] = block[5];
			block[5] = block[4];
			block[4] = block[3] + t1;
			block[3] = block[2];
			block[2] = block[1];
			block[1] = block[0];
			block[0] = t1 + t2;
		}

		for (uint i = 0; i < 8; i++) {
			H[i] += block[i];
		}

	}
	CvString s;
	for (uint i = 0; i < 8; i++) {
		CvString t;
		CvString::format(t, "%08x", H[i]);
		s += t;
	}
	//SLOG("msg: %s hash: %s", m.c_str(), s.c_str());
	return s;
}
#endif

//	-----------------------------------------------------------------------
//	Bind a leader head key to the leader head using the current leader head ID
bool bindLeaderKeys(PlayerTypes p)
{
	LeaderHeadTypes l = leaderHead(p);

	bool bFailed = false;
	if(l != NO_LEADER)
	{
		// During the pre-game, we can't be sure the cached *Infos are current, so query the database
		Database::Connection* pDB = GC.GetGameDatabase();
		if(pDB)
		{
			Database::Results kResults;
			if(pDB->Execute(kResults, "SELECT Type, PackageID from Leaders where ID = ? LIMIT 1"))
			{
				kResults.Bind(1, l);
				if(kResults.Step())
				{
					s_leaderKeys[p] = kResults.GetText(0);
					if(!ExtractGUID(kResults.GetText(1), s_leaderPackageID[p]))
						ClearGUID(s_leaderPackageID[p]);
					s_leaderKeysAvailable[p] = true;
				}
				else
					bFailed = true;
			}
			else
				bFailed = true;
		}
	}
	else
		bFailed = true;

	if(bFailed)
	{
		s_leaderKeys[p].clear();
		ClearGUID(s_leaderPackageID[p]);
		s_leaderKeysAvailable[p] = false;
	}

	return bFailed;
}

// Set the unique key value for the civilization
bool bindCivilizationKeys(PlayerTypes p)
{
	bool bFailed = false;
	CivilizationTypes c = civilization(p);
	if(c != NO_CIVILIZATION)
	{
		// During the pre-game, we can't be sure the cached *Infos are current, so query the database
		Database::Connection* pDB = GC.GetGameDatabase();
		if(pDB)
		{
			Database::Results kResults;
			if(pDB->Execute(kResults, "SELECT Type, PackageID, Playable from Civilizations where ID = ? LIMIT 1"))
			{
				kResults.Bind(1, c);
				if(kResults.Step())
				{
					s_civilizationKeys[p] = kResults.GetText(0);
					if(!ExtractGUID(kResults.GetText(1), s_civilizationPackageID[p]))
						ClearGUID(s_civilizationPackageID[p]);

					s_civilizationKeysAvailable[p] = true;
					s_civilizationKeysPlayable[p] = kResults.GetBool(2);
				}
				else
					bFailed = true;
			}
			else
				bFailed = true;
		}
	}
	else
		bFailed = true;

	if(bFailed)
	{
		s_civilizationKeys[p].clear();
		ClearGUID(s_civilizationPackageID[p]);
		s_civilizationKeysAvailable[p] = false;
		s_civilizationKeysPlayable[p] = false;
	}
	return bFailed;
}

void writeCivilizations(FDataStream& saveTo)
{
	if(s_gameStarted || !isNetworkMultiplayerGame()){
		//full save, preserve everything.
		saveTo << s_civilizations;
	}
	else{
		//game cfg save only.  Scrub player specific data from our save output.
		//reset all non-AI players to random civ.
		int i = 0;
		std::vector<CivilizationTypes> civsTemp = s_civilizations;
		for(std::vector<CivilizationTypes>::iterator civIter = civsTemp.begin(); civIter != civsTemp.end(); ++civIter, ++i){
			if(slotStatus((PlayerTypes)i) != SS_COMPUTER){
				*civIter = NO_CIVILIZATION;
			}
		}
		saveTo << civsTemp;
	}
}

void writeNicknames(FDataStream& saveTo)
{
	if(s_gameStarted || !isNetworkMultiplayerGame()){
		//full save, preserve everything.
		saveTo << s_nicknames;
	}
	else{
		//game cfg save only.  Scrub player specific data from our save output.
		std::vector<CvString> nicksTemp = s_nicknames;
		for(std::vector<CvString>::iterator nickIter = nicksTemp.begin(); nickIter != nicksTemp.end(); ++nickIter){
			*nickIter = "";
		}
		saveTo << nicksTemp;
	}
}

void writeSlotStatus(FDataStream& saveTo)
{
	if(s_gameStarted || !isNetworkMultiplayerGame()){
		//full save, preserve everything.
		saveTo << s_slotStatus;
	}
	else{
		//game cfg save only.  Scrub player specific data from our save output.
		//Revert human occupied slots to open.
		std::vector<SlotStatus> slotTemp = s_slotStatus;
		for(std::vector<SlotStatus>::iterator slotIter = slotTemp.begin(); slotIter != slotTemp.end(); ++slotIter){
			if(*slotIter == SS_TAKEN){
				*slotIter = SS_OPEN;
			}
		}
		saveTo << slotTemp;
	}
}

//	-----------------------------------------------------------------------
void saveSlotHints(FDataStream& saveTo)
{
	uint uiVersion = 3;
	saveTo << uiVersion;
	saveTo << s_gameSpeed;
	saveTo << s_worldSize;
	saveTo << s_mapScriptName;

	writeCivilizations(saveTo);
	writeNicknames(saveTo);
	writeSlotStatus(saveTo);

	saveTo << s_slotClaims;
	saveTo << s_teamTypes;
	saveTo << s_handicaps;
	saveTo << s_civilizationKeys;
	saveTo << s_leaderKeys;
}

void ReseatConnectedPlayers()
{//This function realigns network connected players into the correct slots for the current pregame data. (Typically after loading in saved data)
	//A network player's pregame data can and will be totally wrong until this function is run and the resulting net messages are processed.
	if(isNetworkMultiplayerGame()){
		int i = 0;
		for(i = 0; i < MAX_PLAYERS; ++i){
			// reseat the network connected player in this slot.
			if(gDLL->ReseatConnectedPlayer((PlayerTypes)i)){
				//a player needs to be reseated.  We need to wait for the net message to come from the host.  We will rerun ReseatConnectedPlayers then.
				return;
			}
		}
	}
}


int calcActiveSlotCount(const std::vector<SlotStatus>& slotStatus, const std::vector<SlotClaim>& slotClaims)
{
	int iCount = 0;
	int i = 0;
	for(i = 0; i < MAX_PLAYERS; ++i)
	{
		SlotStatus eStatus = slotStatus[i];
		SlotClaim eClaim = slotClaims[i];

		if((eStatus == SS_TAKEN || eStatus == SS_COMPUTER || eStatus == SS_OBSERVER) 
			&& (eClaim == SLOTCLAIM_ASSIGNED || eClaim == SLOTCLAIM_RESERVED))
			++iCount;
	}

	return iCount;
}

//	---------------------------------------------------------------------------
static void loadSlotsHelper(
    FDataStream& loadFrom,
    uint uiVersion,
    GameSpeedTypes& gameSpeed,
    WorldSizeTypes& worldSize,
    CvString& mapScriptName,
    std::vector<CivilizationTypes>& civilizations,
    std::vector<CvString>& nicknames,
    std::vector<SlotStatus>& slotStatus,
    std::vector<SlotClaim>& slotClaims,
    std::vector<TeamTypes>& teamTypes,
    std::vector<HandicapTypes>& handicapTypes,
	std::vector<CvString>& civilizationKeys,
	std::vector<CvString>& leaderKeys)
{
	loadFrom >> gameSpeed;
	loadFrom >> worldSize;
	loadFrom >> mapScriptName;

	loadFrom >> civilizations;

	loadFrom >> nicknames;

	loadFrom >> slotStatus;
	loadFrom >> slotClaims;
	loadFrom >> teamTypes;
	if(uiVersion >= 2)
		loadFrom >> handicapTypes;
	else
		handicapTypes.clear();

	if(uiVersion >= 3)
	{
		loadFrom >> civilizationKeys;
		loadFrom >> leaderKeys;
	}
	else
	{
		civilizationKeys.clear();
		leaderKeys.clear();
	}
}

int readActiveSlotCountFromSaveGame(FDataStream& loadFrom, bool bReadVersion)
{
	uint uiVersion = 0;
	if(bReadVersion)
		loadFrom >> uiVersion;

	GameSpeedTypes	dummyGameSpeed;
	WorldSizeTypes	dummyWorldSize;
	CvString		dummyMapScriptName;
	std::vector<CivilizationTypes>	dummyCivilizations;
	std::vector<CvString> dummyNicknames;
	std::vector<TeamTypes> dummyTeamTypes;
	std::vector<SlotStatus> slotStatus;
	std::vector<SlotClaim> slotClaims;
	std::vector<HandicapTypes> dummyHandicapTypes;
	std::vector<CvString> civilizationKeys;
	std::vector<CvString> leaderKeys;

	loadSlotsHelper(loadFrom, uiVersion, dummyGameSpeed, dummyWorldSize, dummyMapScriptName, dummyCivilizations, dummyNicknames, slotStatus, slotClaims, dummyTeamTypes, dummyHandicapTypes, civilizationKeys, leaderKeys);

	return calcActiveSlotCount(slotStatus, slotClaims);
}

void loadSlotHints(FDataStream& loadFrom, bool bReadVersion)
{
	uint uiVersion = 0;
	if(bReadVersion)
		loadFrom >> uiVersion;

	GameSpeedTypes	gameSpeed;
	WorldSizeTypes	worldSize;
	CvString		mapScriptName;
	std::vector<CivilizationTypes>	civilizations;
	std::vector<CvString> nicknames;
	std::vector<TeamTypes> teamTypes;
	std::vector<SlotStatus> slotStatus;
	std::vector<SlotClaim> slotClaims;
	std::vector<HandicapTypes> handicapTypes;
	std::vector<CvString> civilizationKeys;
	std::vector<CvString> leaderKeys;

	loadSlotsHelper(loadFrom, uiVersion, gameSpeed, worldSize, mapScriptName, civilizations, nicknames, slotStatus, slotClaims, teamTypes, handicapTypes, civilizationKeys, leaderKeys);

	s_gameSpeed = gameSpeed;
	s_worldSize = worldSize;
	s_mapScriptName = mapScriptName;
	if (uiVersion >= 3)
	{
		// Set the civilizations and leaders from the key values
		if (civilizationKeys.size() > 0)
		{
			for (uint i = 0; i < civilizationKeys.size(); ++i)	
				setCivilizationKey((PlayerTypes)i, civilizationKeys[i]);
		}
		else
			// Fall back to using the indices.  This shouldn't happen.
			s_civilizations = civilizations;

		if (leaderKeys.size() > 0)
		{
			for (uint i = 0; i < leaderKeys.size(); ++i)	
				setLeaderKey((PlayerTypes)i, leaderKeys[i]);
		}
	}
	else
	{
		// Civilization saved as indices, not a good idea.
		s_civilizations = civilizations;
	}

	s_nicknames = nicknames;
	s_teamTypes = teamTypes;
	s_slotStatus = slotStatus;
	s_slotClaims = slotClaims;

	// The slots hints handicaps may be invalid, only copy them over if they are not
	if(handicapTypes.size() == MAX_PLAYERS)
		s_handicaps = handicapTypes;

	size_t i;
	for(i = 0; i < s_nicknames.size(); ++i)
	{
		PlayerTypes p = static_cast<PlayerTypes>(i);
		setNickname(p, s_nicknames[i]); // fix display names
	}

	ReseatConnectedPlayers();
}

PlayerTypes activePlayer()
{
	return s_activePlayer;
}

const CvString& adminPassword()
{
	return s_adminPassword;
}

int advancedStartPoints()
{
	return s_advancedStartPoints;
}

const CvString& alias()
{
	return s_alias;
}

ArtStyleTypes artStyle(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_artStyles[p];
	return NO_ARTSTYLE;
}

bool autorun()
{
	return s_autorun;
}

float autorunTurnDelay()
{
	return s_autorunTurnDelay;
}

int autorunTurnLimit()
{
	return s_autorunTurnLimit;
}

BandwidthType bandwidth()
{
	return s_bandwidth;
}

CvString bandwidthDescription()
{
	if(bandwidth() == BANDWIDTH_BROADBAND)
	{
		//return CvString("broadband");
		return GetLocalizedText("broadband");
	}
	else
	{
		return GetLocalizedText("modem");
		//return CvString("modem");
	}
}

CalendarTypes calendar()
{
	return s_calendar;
}

CivilizationTypes civilization(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_civilizations[p];
	return NO_CIVILIZATION;
}

const CvString& civilizationAdjective(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_civAdjectivesLocalized[p] = GetLocalizedText(s_civAdjectives[p]);
		return s_civAdjectivesLocalized[p];
	}

	static const CvString empty = "";
	return empty;
}

const CvString& civilizationAdjectiveKey(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_civAdjectives[p];
	static const CvString empty = "";
	return empty;
}

const CvString& civilizationDescription(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_civDescriptionsLocalized[p] = GetLocalizedText(s_civDescriptions[p]);
		return s_civDescriptionsLocalized[p];
	}

	static const CvString empty = "";
	return empty;
}

const CvString& civilizationDescriptionKey(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_civDescriptions[p];
	static const CvString empty = "";
	return empty;
}

const CvString& civilizationPassword(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_civPasswords[p];

	static const CvString empty = "";
	return empty;
}

const CvString& civilizationShortDescription(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_civShortDescriptionsLocalized[p] = GetLocalizedText(s_civShortDescriptions[p]);
		return s_civShortDescriptionsLocalized[p];
	}

	static const CvString empty = "";
	return empty;
}

const CvString& civilizationShortDescriptionKey(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_civShortDescriptions[p];
	static const CvString empty = "";
	return empty;
}

ClimateTypes climate()
{
	return s_climate;
}

const CvClimateInfo& climateInfo()
{
	if(s_climate != s_climateInfo.get().GetID())
	{
		Database::SingleResult kResult;
		DB.SelectAt(kResult, "Climates", s_climate);
		s_climateInfo.dirtyGet().CacheResult(kResult);
	}
	return s_climateInfo;
}

void closeAllSlots()
{
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		PlayerTypes eID = (PlayerTypes)i;
		setSlotStatus(eID, SS_CLOSED);
		setSlotClaim(eID, SLOTCLAIM_UNASSIGNED);
	}
}

void closeInactiveSlots()
{
	s_gameStarted = true;

	// Open inactive slots mean different things to different game modes and types...
	// Let's figure out what they mean for us
	gDLL->BeginSendBundle();
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		PlayerTypes eID = (PlayerTypes)i;
		if(slotStatus(eID) == SS_OPEN)
		{
			if(gameType() == GAME_NETWORK_MULTIPLAYER && gameMapType() == GAME_SCENARIO)
			{
				// Multiplayer scenario - all "open" slots should be filled with an AI player
				setSlotStatus(eID, SS_COMPUTER);
			}
			else
			{
				// If it's a normal game, all "open" slots should be closed.
				setSlotStatus(eID, SS_CLOSED);
			}
			setSlotClaim(eID, SLOTCLAIM_UNASSIGNED);

			gDLL->sendPlayerInfo(eID);
		}
	}
	gDLL->EndSendBundle();
}

const CvString& emailAddress(PlayerTypes p)
{
	static const CvString empty("");
	if(p >= 0 && p < MAX_PLAYERS)
		return s_emailAddresses[p];
	return empty;
}

const CvString& emailAddress()
{
	return s_localPlayerEmailAddress;
}

float endTurnTimerLength()
{
	return s_endTurnTimerLength;
}

EraTypes era()
{
	return s_era;
}

PlayerTypes findPlayerByNickname(const char* const name)
{
	PlayerTypes result = NO_PLAYER;
	if(name)
	{
		int i = 0;
		for(i = 0; i < MAX_PLAYERS; ++i)
		{
			if(s_nicknames[i] == name)
			{
				result = static_cast<PlayerTypes>(i);
				break;
			}
		}
	}
	return result;
}

GameMode gameMode()
{
	return s_gameMode;
}

const CvString& gameName()
{
	return s_gameName;
}

//! This does not need to be an auto var!
//! The actual game options vector s_GameOptions is an auto var.
//! This is only here to make retrieving enum-based options fast
//! since GameCore does it way to effing often.
int s_EnumBasedGameOptions[NUM_GAMEOPTION_TYPES];

//! This method sync's up an enum-based array of game options w/ the actual
//! game options structure.  This should be called every time new options
//! are set.
void SyncGameOptionsWithEnumList()
{
	const char* str;
	for(int i = 0; i < NUM_GAMEOPTION_TYPES; i++)
	{
		int value = 0;
		str = ConvertGameOptionTypeToString((GameOptionTypes)i);
		if(str)
		{
			GetGameOption(str, value);
		}
		s_EnumBasedGameOptions[i] = value;
		bool v = static_cast<bool>(value);
		switch(i)
		{
		case GAMEOPTION_QUICK_COMBAT:
			s_quickCombat = v;	// set directly, to avoid infinite recursion.
			break;
		default:
			break;
		}
	}
}

bool GetGameOption(const char* szOptionName, int& iValue)
{
	if (szOptionName == NULL || *szOptionName == 0)
		return false;

	for(std::vector<CustomOption>::const_iterator it = s_GameOptions.begin();
	        it != s_GameOptions.end(); ++it)
	{
		size_t bytes = 0;
		const char* szCurrentOptionName = (*it).GetName(bytes);
		if(strncmp(szCurrentOptionName, szOptionName, bytes) == 0)
		{
			iValue = (*it).GetValue();
			return true;
		}
	}

	//Try and lookup the default value.
	Database::Results kLookup;
	if(GC.GetGameDatabase()->Execute(kLookup, "Select \"Default\" from GameOptions where Type = ? LIMIT 1"))
	{
		kLookup.Bind(1, szOptionName);
		if(kLookup.Step())
		{
			iValue = kLookup.GetInt(0);
			return true;
		}
	}

	return false;
}

bool GetGameOption(GameOptionTypes eOption, int& iValue)
{
	if((uint)eOption >= 0 && (uint)eOption < (uint)NUM_GAMEOPTION_TYPES)
	{
		iValue = s_EnumBasedGameOptions[(size_t)eOption];
		return true;
	}
	else
	{
		// Hash lookup
		HashToOptionMap::const_iterator itr = s_GameOptionsHash.find((uint)eOption);
		if(itr != s_GameOptionsHash.end())
		{
			if(itr->second <= s_GameOptions.size())
			{
				iValue = s_GameOptions[itr->second].GetValue();
				return true;
			}
		}

		// Must get the string from the hash
		GameOptionTypes eOptionIndex = (GameOptionTypes)GC.getInfoTypeForHash((uint)eOption);
		if(eOptionIndex >= 0)
		{
			CvGameOptionInfo* pkInfo = GC.getGameOptionInfo(eOptionIndex);
			if(pkInfo)
			{
				//Try and lookup the default value.
				Database::Results kLookup;
				if(GC.GetGameDatabase()->Execute(kLookup, "Select \"Default\" from GameOptions where Type = ? LIMIT 1"))
				{
					kLookup.Bind(1, pkInfo->GetType());
					if(kLookup.Step())
					{
						iValue = kLookup.GetInt(0);
						return true;
					}
				}
			}
		}

		return false;
	}
}

const std::vector<CustomOption>& GetGameOptions()
{
	return s_GameOptions;
}

bool SetGameOption(const char* szOptionName, int iValue)
{
	//Do not allow NULL entries :P
	if(szOptionName == NULL || strlen(szOptionName) == 0)
		return false;

	for(size_t i = 0; i < s_GameOptions.size(); i++)
	{
		const CustomOption& option = s_GameOptions[i];
		size_t bytes = 0;
		const char* szCurrentOptionName = option.GetName(bytes);
		if(strncmp(szCurrentOptionName, szOptionName, bytes) == 0)
		{
			//I'd like to just set the value here, but that doesn't seem possible
			//so instead, create a new CustomOption type and assign it to this index.
			s_GameOptions.setAt(i, CustomOption(szOptionName, iValue));
			SyncGameOptionsWithEnumList();
			return true;
		}

	}

	//Didn't find the option, push it.
	s_GameOptions.push_back(CustomOption(szOptionName, iValue));
	s_GameOptionsHash[FString::Hash(szOptionName)] = s_GameOptions.size() - 1;
	SyncGameOptionsWithEnumList();

	return true;
}

bool SetGameOption(GameOptionTypes eOption, int iValue)
{
	const char* str = ConvertGameOptionTypeToString(eOption);
	if(str)
	{
		return SetGameOption(str, iValue);
	}

	return false;
}

bool SetGameOptions(const std::vector<CustomOption>& gameOptions)
{
	s_GameOptions = gameOptions;
	s_GameOptionsHash.clear();
	for(size_t i = 0; i < s_GameOptions.size(); i++)
	{
		const CustomOption& kOption = s_GameOptions[i];
		s_GameOptionsHash[FString::Hash(kOption.GetName())] = i;
	}

	SyncGameOptionsWithEnumList();
	return true;
}

GameSpeedTypes gameSpeed()
{
	return s_gameSpeed;
}

bool gameStarted()
{
	return s_gameStarted;
}

int gameTurn()
{
	return s_gameTurn;
}

GameTypes gameType()
{
	return s_gameType;
}

GameMapTypes gameMapType()
{
	return s_gameMapType;
}

GameStartTypes gameStartType()
{
	return s_gameStartType;
}

int gameUpdateTime()
{
	return s_gameUpdateTime;
}

HandicapTypes handicap(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_handicaps[p];
	return NO_HANDICAP;
}

HandicapTypes lastHumanHandicap(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_lastHumanHandicaps[p];
	return NO_HANDICAP;
}

bool isHotSeat()
{
	GameTypes g = gameType();
	return (g == GAME_HOTSEAT_MULTIPLAYER);
}

bool isHotSeatGame()
{
	return isHotSeat();
}

bool isHuman(PlayerTypes p)
{
	SlotStatus s = slotStatus(p);
	if(s == SS_TAKEN)
	{
		return true;
	}
	else if(s == SS_OPEN)
	{
		return (isHotSeat() || isPitBoss() || isPlayByEmail());
	}
	else if(s == SS_OBSERVER)
	{
		return slotClaim(p) == SLOTCLAIM_ASSIGNED;
	}
	else
	{
		return false;
	}

}

bool isInternetGame()
{
	return s_isInternetGame;
}

bool isMinorCiv(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_minorNationCivs[p];
	return false;
}

bool isNetworkMultiplayerGame()
{
	GameTypes eType = gameType();
	return eType == GAME_NETWORK_MULTIPLAYER;
}

bool isPitBoss()
{
	int iValue;
	if(GetGameOption(GAMEOPTION_PITBOSS, iValue))
		return iValue != 0;
	return false;
}

bool g_bPersistSettings = false;
bool GetPersistSettings()
{
	return g_bPersistSettings;
}

void setPersistSettings(bool bPersist)
{
	g_bPersistSettings = bPersist;
}

bool isPlayable(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		if(isWBMapScript() && !mapNoPlayers())
		{
			return s_playableCivs[p];
		}
		else
		{
			if(civilization(p) != NO_CIVILIZATION)
			{
				return s_civilizationKeysPlayable[p];
			}
			else
			{
				// Don't allow people to play the barbarian civ
				return (p < MAX_CIV_PLAYERS);
			}
		}
	}

	return false;
}

bool isPlayByEmail()
{
	GameTypes g = gameType();
	return g == GAME_EMAIL_MULTIPLAYER;
}

bool isReady(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_readyPlayers[p];
	return false;
}

bool IsCityScreenBlocked()
{
	return s_bCityScreenBlocked;
}

bool isVictory(VictoryTypes v)
{
	if(v < s_numVictoryInfos)
		return s_victories[v];
	return false;
}

bool isWBMapScript()
{
	bool bIsMapScript = false;
	const CvString& mapScript = mapScriptName();
	const size_t lenMapScript = mapScript.size();
	if(lenMapScript > 0)
	{
		size_t lenExtension = 0;
		const char* szExtension = NULL;
		for(size_t i = 0; i < lenMapScript; i++)
		{
			if(mapScript[i] == '.')
			{
				szExtension = mapScript.c_str() + i;
				lenExtension = lenMapScript - i;
			}
		}

		if(szExtension && lenExtension == strlen(CIV5_WBMAP_EXT))
		{
			bIsMapScript = (_strnicmp(szExtension, CIV5_WBMAP_EXT, lenExtension) == 0);
		}
	}

	return bIsMapScript;
}

bool isWhiteFlag(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_whiteFlags[p];
	return false;
}

bool isTurnNotifySteamInvite(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_turnNotifySteamInvite[p];
	return false;
}

bool isTurnNotifyEmail(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_turnNotifyEmail[p];
	return false;
}

const CvString& getTurnNotifyEmailAddress(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		return s_turnNotifyEmailAddress[p];
	}

	static const CvString empty = "";
	return empty;
}

LeaderHeadTypes leaderHead(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_leaderHeads[p];
	return NO_LEADER;
}

const CvString& leaderName(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_leaderNamesLocalized[p] = GetLocalizedText(s_leaderNames[p]);
		return s_leaderNamesLocalized[p];
	}

	static const CvString empty = "";
	return empty;
}

const CvString& leaderNameKey(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_leaderNames[p];
	static const CvString empty = "";
	return empty;
}

const CvString& leaderKey(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_leaderKeys[p];
	static const CvString empty = "";
	return empty;
}

// Get the package ID the player's leader is from.  If empty, it comes from the base installation.
const GUID& leaderKeyPackageID(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		return s_leaderPackageID[p];
	}

	return s_emptyGUID;
}

const CvString& loadFileName()
{
	return s_loadFileName;
}

StorageLocation loadFileStorage()
{
	return s_loadFileStorage;
}

void loadFromIni(FIGameIniParser& iniParser)
{
	if(gameStarted())
		return;
	FString szHolder;
	CvString tempString;

	int iHolder;
	iniParser.GetKeyValue("GAME", "QuickCombat", &iHolder, "Force quick combat animations", 0);
	setQuickCombatDefault(iHolder != 0);
	setQuickCombat(quickCombatDefault());

	FString szGameDefault = "";//FString(GC.getSetupData().getAlias().GetCString());
	if(szGameDefault.IsEmpty())
	{
		Localization::String strGameName = Localization::Lookup("TXT_KEY_DEFAULT_GAMENAME");
		szGameDefault = strGameName.toUTF8();
	}
	else
	{
		Localization::String strGameName = Localization::Lookup("TXT_KEY_USER_GAMENAME");
		szGameDefault += strGameName.toUTF8();
	}
	iniParser.GetKeyValue("GAME", "GameName", &szHolder, "Game Name", szGameDefault);
	tempString = szHolder;
	setGameName(tempString);
	// World Size
	iniParser.GetKeyValue("GAME", "WorldSize", &szHolder, "Worldsize options are WORLDSIZE_DUEL/WORLDSIZE_TINY/WORLDSIZE_SMALL/WORLDSIZE_STANDARD/WORLDSIZE_LARGE/WORLDSIZE_HUGE", "WORLDSIZE_SMALL");
	tempString = szHolder;
	setWorldSize(tempString);
	// Climate
	iniParser.GetKeyValue("GAME", "Climate", &szHolder, "Climate options are CLIMATE_ARID/CLIMATE_TEMPERATE/CLIMATE_TROPICAL", "CLIMATE_TEMPERATE");
	tempString = szHolder;
	setClimate(tempString);
	// Sea Level
	iniParser.GetKeyValue("GAME", "SeaLevel", &szHolder, "Sealevel options are SEALEVEL_LOW/SEALEVEL_MEDIUM/SEALEVEL_HIGH", "SEALEVEL_MEDIUM");
	tempString = szHolder;
	setSeaLevel(tempString);
	// Era
	iniParser.GetKeyValue("GAME", "Era", &szHolder, "Era options are ERA_ANCIENT/ERA_CLASSICAL/ERA_MEDIEVAL/ERA_RENAISSANCE/ERA_INDUSTRIAL/ERA_MODERN", "ERA_ANCIENT");
	tempString = szHolder;
	setEra(tempString);
	// Game speed
	iniParser.GetKeyValue("GAME", "GameSpeed", &szHolder, "GameSpeed options are GAMESPEED_QUICK/GAMESPEED_STANDARD/GAMESPEED_EPIC/GAMESPEED_MARATHON", "GAMESPEED_STANDARD");
	tempString = szHolder;
	setGameSpeed(tempString);
	// Quick Handicap

	int iNumBools;
	bool* pbBools;

	// Victory Conditions
	iniParser.GetKeyValue("GAME", "VictoryConditions", &szHolder, "Victory Conditions", "11111111");
	if(szHolder != "EMPTY")
	{
		StringToBools(szHolder, &iNumBools, &pbBools);
		iNumBools = std::min(iNumBools, GC.getNumVictoryInfos());
		int i;
		std::vector<bool> tempVBool;
		for(i = 0; i < iNumBools; i++)
		{
			tempVBool.push_back(pbBools[i]);
		}
		setVictories(tempVBool);
		SAFE_DELETE_ARRAY(pbBools);
	}

	// Game Options
	//if (!CIV.GetModName())
	{
		iniParser.GetKeyValue("GAME", "GameOptions", &szHolder, "Game Options", "EMPTY");
		if(szHolder != "EMPTY")
		{
			StringToBools(szHolder, &iNumBools, &pbBools);
			iNumBools = std::min(iNumBools, static_cast<int>(NUM_GAMEOPTION_TYPES));
			int i;
			for(i = 0; i < iNumBools; i++)
			{
				SetGameOption(((GameOptionTypes)i), pbBools[i]);
			}
			SAFE_DELETE_ARRAY(pbBools);
		}
	}

	// Max Turns
	iniParser.GetKeyValue("GAME", "MaxTurns", &iHolder, "Max number of turns (0 for no turn limit)", 0);
	setMaxTurns(iHolder);

	iniParser.GetKeyValue("GAME", "EnableMultiplayerAI", &iHolder, "Allow AI in multiplayer games", 1);
	setMultiplayerAIEnabled(iHolder != 0);

	// Pitboss SMTP info
	//iniParser.SetGroupKey("CONFIG");
	//iniParser.GetKeyValue("PitbossSMTPHost", &szHolder, "", "SMTP server for Pitboss reminder emails");
	//GC.getSetupData().setPitbossSmtpHost((szHolder.Compare("0") ? szHolder.GetCString() : ""));
	//iniParser.GetKeyValue("PitbossSMTPLogin", &szHolder, "", "SMTP server authentication login for Pitboss reminder emails");
	//GC.getSetupData().setPitbossSmtpLogin((szHolder.Compare("0") ? szHolder.GetCString() : ""));
	//iniParser.GetKeyValue("PitbossEmail", &szHolder, "", "Email address from which Pitboss reminder emails are sent");
	//GC.getSetupData().setPitbossEmail((szHolder.Compare("0") ? szHolder.GetCString() : ""));

	// Get Pitboss Turn Time
	//iniParser.SetGroupKey("GAME");
	//iniParser.GetKeyValue("PitbossTurnTime", &iHolder, 0, "Pitboss Turn Time");
	//setPitbossTurnTime(iHolder);
	// Sync Rand
#ifdef CRIPPLED_BUILD
	//Dbaker set the seed to the same thing for testing purposes
	setSyncRandomSeed(1);
	setMapRandomSeed(1);
#else
	iniParser.GetKeyValue("CONFIG", "SyncRandSeed", &iHolder, "Random seed for game sync, or '0' for default", 0);
	setSyncRandomSeed((iHolder!=0) ? iHolder : timeGetTime());
#ifdef SEED_LOGGING
	SLOG("SyncRandomSeed = %d", syncRandomSeed());
#endif
	// Map Rand
	iniParser.GetKeyValue("CONFIG", "MapRandSeed", &iHolder, "Random seed for map generation, or '0' for default", 0);
	setMapRandomSeed((iHolder!=0) ? iHolder : timeGetTime());
#ifdef SEED_LOGGING
	SLOG("MapRandomSeed = %d", mapRandomSeed());
#endif
#endif

	//	Game Type
	iniParser.GetKeyValue("GAME", "GameType", &szHolder, "GameType options are singlePlayer/spLoad", "singlePlayer");
	tempString = szHolder;
	setGameType(tempString);

	// Map Script
	iniParser.GetKeyValue("GAME", "Map", &szHolder, "Map Script file name", "Assets/Maps/Continents.lua");
	szHolder.StandardizePath(false, false);
	tempString = szHolder;
	setMapScriptName(tempString);
}

bool mapNoPlayers()
{
	return s_mapNoPlayers;
}

bool GetMapOption(const char* szOptionName, int& iValue)
{
	for(std::vector<CustomOption>::const_iterator it = s_MapOptions.begin();
	        it != s_MapOptions.end(); ++it)
	{
		size_t bytes = 0;
		const char* szCurrentOptionName = (*it).GetName(bytes);
		if(strncmp(szCurrentOptionName, szOptionName, bytes) == 0)
		{
			iValue = (*it).GetValue();
			return true;
		}
	}

	//Try and lookup the default value.
	//Not a huge fan of this being here as it adds an additional dependency, but there was really no "clean" place to put it.
	Database::Results kLookup;
	if(GC.GetGameDatabase()->Execute(kLookup, "Select DefaultValue from MapScriptOptions where FileName = ? and OptionID = ? LIMIT 1"))
	{
		kLookup.Bind(1, CvPreGame::mapScriptName().c_str());
		kLookup.Bind(2, szOptionName);
		if(kLookup.Step())
		{
			iValue = kLookup.GetInt(0);
			return true;
		}
	}

	return false;
}

const std::vector<CustomOption>& GetMapOptions()
{
	return s_MapOptions;
}

bool SetMapOption(const char* szOptionName, int iValue)
{
	//DO NOT ALLOW NULL
	if(szOptionName == NULL)
		return false;

	for(size_t i = 0; i < s_MapOptions.size(); i++)
	{
		const CustomOption& option = s_MapOptions[i];
		size_t bytes = 0;
		const char* szCurrentOptionName = option.GetName(bytes);
		if(strncmp(szCurrentOptionName, szOptionName, bytes) == 0)
		{
			//I'd like to just set the value here, but that doesn't seem possible
			//so instead, create a new CustomOption type and assign it to this index.
			s_MapOptions.setAt(i, CustomOption(szOptionName, iValue));
			return true;
		}

	}

	//Didn't find the option, push it.
	s_MapOptions.push_back(CustomOption(szOptionName, iValue));
	return true;
}

bool SetMapOptions(const std::vector<CustomOption>& mapOptions)
{
	s_MapOptions = mapOptions;
	return true;
}

unsigned int mapRandomSeed()
{
	return s_mapRandomSeed;
}

bool loadWBScenario()
{
	return s_loadWBScenario;
}

bool overrideScenarioHandicap()
{
	return s_overrideScenarioHandicap;
}

const CvString& mapScriptName()
{
	return s_mapScriptName;
}

int maxCityElimination()
{
	return s_maxCityElimination;
}

int maxTurns()
{
	return s_maxTurns;
}

MinorCivTypes minorCivType(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_minorCivTypes[p];
	return NO_MINORCIV;
}

bool multiplayerAIEnabled()
{
	return s_multiplayerAIEnabled;
}
bool multiplayerOptionEnabled(MultiplayerOptionTypes o)
{
	if(o < NUM_MPOPTION_TYPES)
		return s_multiplayerOptions[o];
	return false;
}

const std::vector<bool>& multiplayerOptions()
{
	return s_multiplayerOptions;
}

int netID(PlayerTypes p)
{
	if(p < MAX_PLAYERS)
		return s_netIDs[p];
	return -1;
}

const CvString& nickname(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_nicknames[p];
	static const CvString none("");
	return none;
}

const CvString& nicknameDisplayed(PlayerTypes p)
{
#ifdef PREGAMEAPI_GET_NETID
	// UI <- dll
	// 0001 xxxx xxxx xxxx xxxx xxxx xxxx xxxx - get steamID
	// 0010 xxxx xxxx xxxx xxxx xxxx xxxx xxxx - get drafts data
	// 0011 xxxx xxxx xxxx xxxx xxxx xxxx xxxx - get s_draftPlayersReady
	// 
	// intercept Pregame.GetNickName here
	// it actually limits max PlayerType value to 2^28 (it's OK)
	if ((((uint)p >> 28) & 15) == 1)  // check if leftmost bits are 0001
	{
		static CvString netId = "";
		PlayerTypes pId = (PlayerTypes)((uint)p & 268435455);  // all 28 rightmost bits
		size_t ind = nickname(pId).rfind("@");  // nickname format is PlayerName@SteamId, assume "@" is guaranteed
		if (ind != std::string::npos)
		{
			netId = (CvString)(nickname(pId).substr(ind + 1));  // extract steamID from nickname
		}
		return netId;
	}
#endif
#ifdef INGAME_CIV_DRAFTER
	if ((((uint)p >> 28) & 15) == 2)  // draft data requested
	{
		static CvString data;
		CvString s = "";
		uint i = 0;
		for (i = 0; i < s_draftPlayerBans.size(); i++)
		{
			if (s_draftPlayerBans[i] != "")
			{
				s += s_draftPlayerBans[i];
			}
		}

		switch (s_draftCurrentProgress) {
		case DRAFT_PROGRESS_INIT:
			for (i = 0; i < s_draftPlayerSecretHashes.size(); i++)
			{
				s.append(s_draftPlayerSecretHashes[i] == "" ? "0" : "1");
				s.append(",");
			}
			data = CvString::format("DRAFT_PROGRESS_INIT|%s", s.c_str());
			break;
		case DRAFT_PROGRESS_BANS:
			data = CvString::format("DRAFT_PROGRESS_BANS|%s", s.c_str());
			break;
		case DRAFT_PROGRESS_BUSY:
			data = CvString::format("DRAFT_PROGRESS_BUSY|%s", s.c_str());
			break;
		case DRAFT_PROGRESS_OVER:
			data = CvString::format("DRAFT_PROGRESS_OVER|%s|%s", s.c_str(), s_draftShuffledCivs.c_str());
			break;
		default:
			data = "DRAFT_PROGRESS_DEFAULT|";
			break;
		}

		return data;
	}
	if ((((uint)p >> 28) & 15) == 3)  // s_draftPlayersReady requested
	{
		static CvString data;
		data = CvString::format("%d", s_draftPlayersReady);
		return data;
	}
#endif
	if(p >= 0 && p < MAX_PLAYERS)
		return s_displayNicknames[p];
	static const CvString none("");
	return none;
}

int numDefinedPlayers()
{
	int iCount = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; ++i)
	{
		if((civilization((PlayerTypes)i) != NO_CIVILIZATION) && (leaderHead((PlayerTypes)i) != NO_LEADER))
		{
			iCount++;
		}
	}
	return iCount;
}

int numHumans()
{
	int iNumHumans = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; ++i)
	{
		if(isHuman((PlayerTypes)i))
		{
			++iNumHumans;
		}
	}
	return iNumHumans;
}

int numMinorCivs()
{
	return s_numMinorCivs;
}

int pitBossTurnTime()
{
	return s_pitBossTurnTime;
}

PlayerColorTypes playerColor(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_playerColors[p];
	return NO_PLAYERCOLOR;
}

bool privateGame()
{
	return s_privateGame;
}

HandicapTypes quickHandicap()
{
	return s_quickHandicap;
}

bool quickCombat()
{
	return s_quickCombat;
}

bool quickCombatDefault()
{
	return s_quickCombatDefault;
}

bool quickMovement()
{
	int iValue;
	if(GetGameOption(GAMEOPTION_QUICK_MOVEMENT_KEY, iValue))
		return iValue != 0;
	return false;
}

bool quickstart()
{
	return s_quickstart;
}

bool randomWorldSize()
{
	return s_randomWorldSize;
}

bool randomMapScript()
{
	return s_randomMapScript;
}

void readArchive(FDataStream& loadFrom, bool bReadVersion)
{
	uint uiVersion = 0;
	if(bReadVersion)
		loadFrom >> uiVersion;

	loadFrom >> s_activePlayer;
	loadFrom >> s_adminPassword;
	loadFrom >> s_advancedStartPoints;
	loadFrom >> s_alias;
	loadFrom >> s_artStyles;
	loadFrom >> s_autorun;
	loadFrom >> s_autorunTurnDelay;
	loadFrom >> s_autorunTurnLimit;
	loadFrom >> s_bandwidth;
	loadFrom >> s_calendar;
	loadFrom >> s_calendarInfo;
	loadFrom >> s_civAdjectives;
	loadFrom >> s_civDescriptions;
	if (uiVersion <= 2)						// Read the civilization indices.  Newer versions have the keys in the header.
		loadFrom >> s_civilizations;

	loadFrom >> s_civPasswords;
	loadFrom >> s_civShortDescriptions;
	loadFrom >> s_climate;
	loadFrom >> s_climateInfo;
	loadFrom >> s_era;
	loadFrom >> s_emailAddresses;
	loadFrom >> s_endTurnTimerLength;
	loadFrom >> s_flagDecals;
	loadFrom >> s_DEPRECATEDforceControls;
	loadFrom >> s_gameMode;
	loadFrom >> s_gameName;
	loadFrom >> s_gameSpeed;
	loadFrom >> s_gameStarted;
	loadFrom >> s_gameTurn;
	if(uiVersion >= 1)
	{
		loadFrom >> s_gameType;
		loadFrom >> s_gameMapType;
	}
	else
	{
		// Old enum that was trying to cram too much info into a sequence
		enum GameType
		{
		    GAME_NONE = -1,

		    GAME_SP_NEW,
		    GAME_SP_SCENARIO,
		    GAME_SP_LOAD,
		    GAME_MP_NEW,
		    GAME_MP_SCENARIO,
		    GAME_MP_LOAD,
		    GAME_HOTSEAT_NEW,
		    GAME_HOTSEAT_SCENARIO,
		    GAME_HOTSEAT_LOAD,
		    GAME_PBEM_NEW,
		    GAME_PBEM_SCENARIO,
		    GAME_PBEM_LOAD,

		    NUM_GAMETYPES
		};

		int iDummy;
		loadFrom >> iDummy;
		GameType eGameType = static_cast<GameType>(iDummy);

		// Convert the old one to the new format

		if((eGameType == GAME_HOTSEAT_NEW) || (eGameType == GAME_HOTSEAT_SCENARIO) || (eGameType == GAME_HOTSEAT_LOAD))
			s_gameType = GAME_HOTSEAT_MULTIPLAYER;
		else if((eGameType == GAME_MP_NEW) || (eGameType == GAME_MP_SCENARIO) || (eGameType == GAME_MP_LOAD))
			s_gameType = GAME_NETWORK_MULTIPLAYER;
		else if((eGameType == GAME_SP_NEW) || (eGameType == GAME_SP_SCENARIO) || (eGameType == GAME_SP_LOAD))
			s_gameType = GAME_SINGLE_PLAYER;
		else
		{
			if(s_gameType == GAME_TYPE_NONE)
				s_gameType = GAME_SINGLE_PLAYER;		// I've seen this saved as -1 (GAME_NONE)
		}

		s_gameMapType = GAME_USER_PARAMETERS;
		if((eGameType == GAME_MP_SCENARIO) || (eGameType == GAME_HOTSEAT_SCENARIO) || (eGameType == GAME_SP_SCENARIO))
			s_gameMapType = GAME_SCENARIO;

	}
	loadFrom >> s_gameUpdateTime;
	loadFrom >> s_handicaps;
	if(uiVersion >= 6){
		loadFrom >> s_lastHumanHandicaps;
	}

	loadFrom >> s_isEarthMap;
	loadFrom >> s_isInternetGame;
	if(uiVersion == 0)
	{
		bool bNetworkMultiplayerGame;
		loadFrom >> bNetworkMultiplayerGame;
		if(bNetworkMultiplayerGame && s_gameType != GAME_HOTSEAT_MULTIPLAYER)
		{
			s_gameType = GAME_NETWORK_MULTIPLAYER;
		}
	}
	if (uiVersion <= 2)						// Read the leader indices.  Newer versions have the keys in the header.
		loadFrom >> s_leaderHeads;
	loadFrom >> s_leaderNames;
	loadFrom >> s_loadFileName;
	loadFrom >> s_localPlayerEmailAddress;
	loadFrom >> s_mapNoPlayers;
	loadFrom >> s_mapRandomSeed;
	loadFrom >> s_loadWBScenario;
	loadFrom >> s_overrideScenarioHandicap;
	loadFrom >> s_mapScriptName;
	loadFrom >> s_maxCityElimination;
	loadFrom >> s_maxTurns;
	loadFrom >> s_numMinorCivs;
	if (uiVersion >= 3)
	{
		CvInfosSerializationHelper::ReadTypeArrayDBLookup<MinorCivTypes>(loadFrom, s_minorCivTypes.dirtyGet(), "MinorCivilizations"); 
		s_minorCivTypes.clearDelta();
	}
	else
		loadFrom >> s_minorCivTypes;
	loadFrom >> s_minorNationCivs;
	loadFrom >> s_dummyvalue;
	loadFrom >> s_multiplayerOptions;
	loadFrom >> s_netIDs;
	loadFrom >> s_nicknames;
	loadFrom >> s_numVictoryInfos;
	loadFrom >> s_pitBossTurnTime;
	loadFrom >> s_playableCivs;
	if (uiVersion >= 3)
	{
		CvInfosSerializationHelper::ReadTypeArrayDBLookup<PlayerColorTypes>(loadFrom, s_playerColors.dirtyGet(), "PlayerColors");
		s_playerColors.clearDelta();
	}
	else
		loadFrom >> s_playerColors;

	loadFrom >> s_privateGame;
	loadFrom >> s_quickCombat;
	loadFrom >> s_quickCombatDefault;
	loadFrom >> s_quickHandicap;
	loadFrom >> s_quickstart;
	loadFrom >> s_randomWorldSize;
	loadFrom >> s_randomMapScript;
	loadFrom >> s_readyPlayers;
	loadFrom >> s_seaLevel;
	loadFrom >> s_seaLevelInfo;
	loadFrom >> s_dummyvalue2;
	loadFrom >> s_slotClaims;
	loadFrom >> s_slotStatus;
	loadFrom >> s_smtpHost;
	loadFrom >> s_syncRandomSeed;
	loadFrom >> s_targetScore;
	loadFrom >> s_teamTypes;
	loadFrom >> s_transferredMap;
	loadFrom >> s_turnTimer;
	loadFrom >> s_turnTimerType;
	loadFrom >> s_bCityScreenBlocked;
	loadFrom >> s_victories;
	loadFrom >> s_whiteFlags;
	if(uiVersion <= 1)
		s_worldInfo.dirtyGet().readFromVersion0(loadFrom);
	else
		loadFrom >> s_worldInfo;

	loadFrom >> s_worldSize;
	loadFrom >> s_GameOptions;
	loadFrom >> s_MapOptions;
	loadFrom >> s_versionString;

	if (uiVersion >= 4)
		loadFrom >> s_turnNotifySteamInvite;

	if (uiVersion >= 5){
		loadFrom >> s_turnNotifyEmail;
		loadFrom >> s_turnNotifyEmailAddress;
	}

	// Rebuild the hash lookup to the options
	s_GameOptionsHash.clear();
	for(size_t i = 0; i < s_GameOptions.size(); i++)
	{
		const CustomOption& kOption = s_GameOptions[i];
		s_GameOptionsHash[FString::Hash(kOption.GetName())] = i;
	}
}

void read(FDataStream& loadFrom, bool bReadVersion)
{
	loadSlotHints(loadFrom, bReadVersion);
	readArchive(loadFrom, bReadVersion);

	SyncGameOptionsWithEnumList();
	for(int i = 0; i < MAX_PLAYERS; ++i)
	{
		bindLeaderKeys((PlayerTypes)i);
		bindCivilizationKeys((PlayerTypes)i);
	}
	ReseatConnectedPlayers();
}

void resetGame()
{
	// Descriptive strings about game and map
	s_gameStartType = GAME_NEW;
	s_gameMapType = GAME_USER_PARAMETERS;

	s_gameStarted = false;
	s_gameName = "";
	s_adminPassword = "";
	s_mapScriptName = "";
	s_loadWBScenario = false;

	s_mapNoPlayers = false;

	// Standard game parameters
	s_climate   = (ClimateTypes)0;//GC.getSTANDARD_CLIMATE();		// NO_ option?
	s_seaLevel  = (SeaLevelTypes)1;//GC.getSTANDARD_SEALEVEL();		// NO_ option?
	s_era		 = (EraTypes)GC.getSTANDARD_ERA();				// NO_ option?
	s_gameSpeed = (GameSpeedTypes)GC.getSTANDARD_GAMESPEED();	// NO_ option?
	s_turnTimerType = (TurnTimerTypes)4;//GC.getSTANDARD_TURNTIMER();	// NO_ option?
	s_calendar  = (CalendarTypes)0;//GC.getSTANDARD_CALENDAR();	// NO_ option?

	// Data-defined victory conditions
	s_numVictoryInfos = GC.getNumVictoryInfos();
	s_victories.clear();
	if(s_numVictoryInfos > 0)
	{
		for(int i = 0; i < s_numVictoryInfos; ++i)
		{
			s_victories.push_back(true);
		}
	}

	// Standard game options
	int i;
	for(i = 0; i < NUM_MPOPTION_TYPES; ++i)
	{
		s_multiplayerOptions.setAt(i, false);
	}

	//s_statReporting = false;

	// Game turn mgmt
	s_gameTurn = 0;
	s_maxTurns = 0;
	s_pitBossTurnTime = 0;
	s_targetScore = 0;

	// City Elimination
	s_maxCityElimination = 0;

	s_numMinorCivs = -1;

	s_advancedStartPoints = 0;

	// Unsaved game data
	s_syncRandomSeed = 0;
	s_mapRandomSeed = 0;
	s_activePlayer = NO_PLAYER;

	s_quickstart = false;
	s_randomWorldSize = false;
	s_randomMapScript = false;
	s_isEarthMap = false;
	s_autorun = false;
	s_transferredMap = false;

	s_autorunTurnLimit = 0;
	s_autorunTurnDelay = 0.f;
	s_gameUpdateTime = 0;

	s_bandwidth = NO_BANDWIDTH;
	s_quickHandicap = NO_HANDICAP;

	s_alias = "";
	s_localPlayerEmailAddress = "";
	s_loadFileName = "";
	s_loadFileStorage = STORAGE_LOCAL;

	s_endTurnTimerLength = 0.0f;

	s_privateGame = false;
	s_isInternetGame = false;

	ResetMapOptions();
	ResetGameOptions();
#ifdef INGAME_CIV_DRAFTER
	SLOG("===DRAFT RESET===")
	for (uint i = 0; i < s_draftPlayerSecrets.size(); i++)
	{
		s_draftPlayerSecrets[i] = "";
	}
	for (uint i = 0; i < s_draftPlayerSecretHashes.size(); i++)
	{
		s_draftPlayerSecretHashes[i] = "";
	}
	for (uint i = 0; i < s_draftAllBansReceived.size(); i++)
	{
		s_draftAllBansReceived[i] = false;
	}
	s_draftPlayerBans.clear();
	s_draftCurrentProgress = DRAFT_PROGRESS_INIT;
	s_draftResult = DRAFT_RESULT_NONE;
	s_draftShuffledCivs= "";
	s_draftPlayersReady = 0;
	CvPreGame::SetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS1", 0);
	CvPreGame::SetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS2", 0);
	s_draftBansMsgQueue.clear();
	s_draftCurrBansMsgNumber = 0;

#endif
}

void ResetGameOptions()
{
	s_GameOptions.clear();
	s_GameOptionsHash.clear();
	for(size_t i = 0; i < s_GameOptions.size(); i++)
	{
		const CustomOption& kOption = s_GameOptions[i];
		s_GameOptionsHash[FString::Hash(kOption.GetName())] = i;
	}
	SyncGameOptionsWithEnumList();

	// victory conditions
	s_numVictoryInfos = GC.getNumVictoryInfos();
	s_victories.clear();
	if(s_numVictoryInfos > 0)
	{
		for(int i = 0; i < s_numVictoryInfos; ++i)
		{
			s_victories.push_back(true);
		}
	}

}
void ResetMapOptions()
{
	s_MapOptions.clear();
}

void resetPlayer(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		// Only copy over saved data

		// Civ details
		setLeaderName(p, "");
		setCivilizationDescription(p, "");
		setCivilizationShortDescription(p, "");
		setCivilizationAdjective(p, "");
		setCivilizationPassword(p, "");
		setEmailAddress(p, "");
		setWhiteFlag(p, false);
		setCivilization(p, NO_CIVILIZATION);
		setLeaderHead(p, NO_LEADER);
		setMinorCivType(p, NO_MINORCIV);
		setTeamType(p, (TeamTypes)p); // JAR : Whisky Tango Foxtrot?

		if(isNetworkMultiplayerGame())
			setHandicap(p, (HandicapTypes)GC.getMULTIPLAYER_HANDICAP());
		else
			setHandicap(p, (HandicapTypes)GC.getSTANDARD_HANDICAP());

		setLastHumanHandicap(p, NO_HANDICAP);
		setPlayerColor(p, NO_PLAYERCOLOR);
		setArtStyle(p, NO_ARTSTYLE);


		// Slot data
		setSlotStatus(p, SS_CLOSED);
		setSlotClaim(p, SLOTCLAIM_UNASSIGNED);

		// Civ flags
		setPlayable(p, false);
		setMinorCiv(p, false);

		// Unsaved player data
		setNetID(p, -1);
		setReady(p, false);
		setNickname(p, "");

		clearDLCAvailable(p);
	}
}

void resetPlayers()
{
	for(int i = 0; i < MAX_PLAYERS; ++i)
	{
		resetPlayer((PlayerTypes)i);
	}
}

void resetSlots()
{
	//setNumMinorCivs( CvPreGame::worldInfo().getDefaultMinorCivs() );
	// adjust player slots
	int suggestedPlayerCount = CvPreGame::worldInfo().getDefaultPlayers();
	int slotsAssigned = 0;
	int i = 0;
	for(i = 0; i < MAX_CIV_PLAYERS; ++i)
	{
		PlayerTypes p = static_cast<PlayerTypes>(i);
		if(!isNetworkMultiplayerGame())
		{
			if(slotsAssigned < suggestedPlayerCount)
			{
				// don't reset slots taken by human players
				SlotStatus s = slotStatus(p);
				if(s != SS_TAKEN)
				{
					// the player may have already assigned
					// a civ for this computer slot, don't
					// overwrite those settings.
					if(s != SS_COMPUTER)
					{
						setSlotStatus(p, SS_COMPUTER);
						setSlotClaim(p, SLOTCLAIM_ASSIGNED);
						setCivilization(p, NO_CIVILIZATION); // defaults to random civ
					}
				}

				//S.S:  In single player games, slot 1 *should* be marked as taken but it currently is not.
				if(slotStatus(p) == SS_COMPUTER && i != 0)
					setHandicap(p, (HandicapTypes)GC.getAI_HANDICAP());
				else
				{
					if(isHotSeatGame())
						setHandicap(p, (HandicapTypes)GC.getMULTIPLAYER_HANDICAP());
					else
					{
						//S.S: Commenting this out to prevent handicap from getting reset everytime the map size changes.
						//setHandicap(p, (HandicapTypes)GC.getSTANDARD_HANDICAP());
					}
				}
				++slotsAssigned;
			}
			else
			{
				resetPlayer(p);
				// setup an observer slot
				if(p >= suggestedPlayerCount && p < MAX_MAJOR_CIVS)
					setSlotStatus(p, SS_OBSERVER);
			}
		}
		else
		{
			// don't reset slots taken by human players, even if we exceed the suggested slots
			SlotStatus s = slotStatus(p);
			if(s != SS_TAKEN && slotsAssigned >= suggestedPlayerCount)
			{
				resetPlayer(p);
				// setup an observer slot
				if(p >= suggestedPlayerCount && p < MAX_MAJOR_CIVS)
					setSlotStatus(p, SS_OBSERVER);
			}
			else
			{
				if(s != SS_TAKEN)
				{
					if(slotsAssigned < suggestedPlayerCount)
					{
						// the player may have already assigned
						// a civ for this computer slot, don't
						// overwrite those settings.
						setSlotStatus(p, SS_OPEN);
						setSlotClaim(p, SLOTCLAIM_ASSIGNED);
						setCivilization(p, NO_CIVILIZATION); // defaults to random civ
					}
				}
				if(slotStatus(p) == SS_COMPUTER)
					setHandicap(p, (HandicapTypes)GC.getAI_HANDICAP());
				else
					setHandicap(p, (HandicapTypes)GC.getMULTIPLAYER_HANDICAP());

				++slotsAssigned;
			}
		}
	}
}

std::vector<SlotClaim> s_savedSlotClaims;
std::vector<SlotStatus> s_savedSlotStatus;
std::vector<CivilizationTypes> s_savedCivilizations;
std::vector<LeaderHeadTypes> s_savedLeaderHeads;
PlayerTypes s_savedLocalPlayer;
std::vector<CvString> s_savedCivilizationKeys(MAX_PLAYERS);
std::vector<GUID> s_savedCivilizationPackageID(MAX_PLAYERS);
std::vector<bool> s_savedCivilizationKeysAvailable(MAX_PLAYERS);
std::vector<bool> s_savedCivilizationKeysPlayable(MAX_PLAYERS);

std::vector<CvString> s_savedLeaderKeys(MAX_PLAYERS);
std::vector<GUID> s_savedLeaderPackageID(MAX_PLAYERS);
std::vector<bool> s_savedLeaderKeysAvailable(MAX_PLAYERS);
std::vector<PackageIDList> s_savedDLCPackagesAvailable(MAX_PLAYERS);

//	------------------------------------------------------------------------------------
void restoreSlots()
{
	s_slotClaims = s_savedSlotClaims;
	s_slotStatus = s_savedSlotStatus;
	s_civilizations = s_savedCivilizations;
	s_leaderHeads = s_savedLeaderHeads;

	s_civilizationKeys = s_savedCivilizationKeys;
	s_civilizationPackageID = s_savedCivilizationPackageID;
	s_civilizationKeysAvailable = s_savedCivilizationKeysAvailable;
	s_civilizationKeysPlayable = s_savedCivilizationKeysPlayable;

	s_leaderKeys = s_savedLeaderKeys;
	s_leaderPackageID = s_savedLeaderPackageID;
	s_leaderKeysAvailable =  s_savedLeaderKeysAvailable;
	s_DLCPackagesAvailable = s_savedDLCPackagesAvailable;

	setActivePlayer(s_savedLocalPlayer);
}

//	------------------------------------------------------------------------------------
//  Save all the information needed for the current slot setup that needs to be restored
//	after a hot-join/re-sync
void saveSlots()
{
	s_savedSlotClaims = s_slotClaims;
	s_savedSlotStatus = s_slotStatus;
	s_savedCivilizations = s_civilizations;
	s_savedLeaderHeads = s_leaderHeads;
	s_savedLocalPlayer = activePlayer();

	s_savedCivilizationKeys = s_civilizationKeys;
	s_savedCivilizationPackageID = s_civilizationPackageID;
	s_savedCivilizationKeysAvailable = s_civilizationKeysAvailable;
	s_savedCivilizationKeysPlayable = s_civilizationKeysPlayable;

	s_savedLeaderKeys = s_leaderKeys;
	s_savedLeaderPackageID = s_leaderPackageID;
	s_savedLeaderKeysAvailable = s_leaderKeysAvailable;
	s_savedDLCPackagesAvailable = s_DLCPackagesAvailable;
}

SeaLevelTypes seaLevel()
{
	return s_seaLevel;
}

const CvSeaLevelInfo& seaLevelInfo()
{
	if(s_seaLevel != s_seaLevelInfo.get().GetID())
	{
		Database::SingleResult kResult;
		DB.SelectAt(kResult, "SeaLevels", s_seaLevel);
		s_seaLevelInfo.dirtyGet().CacheResult(kResult);
	}
	return s_seaLevelInfo;
}

void setActivePlayer(PlayerTypes p)
{
#ifdef INGAME_CIV_DRAFTER
	SLOG("active p %d", p);
	if (!gameStarted() && loadFileName().IsEmpty() && (isNetworkMultiplayerGame() || isHotSeatGame()))
	{
		CvString strP = CvString::format("%d", (int)p);
		gDLL->SendRenameCity(-6, strP);
	}
#endif
	s_activePlayer = p;
}

void setAdminPassword(const CvString& p)
{
	s_adminPassword = p;
}

void setAdvancedStartPoints(int a)
{
	s_advancedStartPoints = a;
}

void setAlias(const CvString& a)
{
	s_alias = a;
}

void setArtStyle(PlayerTypes p, ArtStyleTypes a)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_artStyles.setAt(p, a);
}

void setAutorun(bool isEnabled)
{
	s_autorun = isEnabled;
}

void setAutorunTurnDelay(float turnDelay)
{
	s_autorunTurnDelay = turnDelay;
}

void setAutorunTurnLimit(int turnLimit)
{
	s_autorunTurnLimit = turnLimit;
}

void setBandwidth(BandwidthType b)
{
	s_bandwidth = b;
}

void setBandwidth(const CvString& b)
{
	if(b== "modem")
	{
		setBandwidth(BANDWIDTH_MODEM);
	}
	else if(b== "broadband")
	{
		setBandwidth(BANDWIDTH_BROADBAND);
	}
	else
	{
		setBandwidth(BANDWIDTH_MODEM);
	}
}

void setCalendar(CalendarTypes c)
{
	s_calendar = c;
}

void setCalendar(const CvString& c)
{
	Database::SingleResult kResult;
	if(!DB.SelectAt(kResult, "Calendars", "Type", c.c_str()))
	{
		CvAssertMsg(false, "Cannot find calendar info.");
	}
	s_calendarInfo.dirtyGet().CacheResult(kResult);

	s_calendar = (CalendarTypes)s_calendarInfo.get().GetID();
}

void setCivilization(PlayerTypes p, CivilizationTypes c)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_civilizations.setAt(p, c);

		bool bFailed = bindCivilizationKeys(p);

		if(bFailed)
		{
			s_civilizations.setAt(p, NO_CIVILIZATION);
			s_civilizationKeys[p].clear();
			ClearGUID(s_civilizationPackageID[p]);
			s_civilizationKeysAvailable[p] = true;	// If the key is empty, we assume the selection is in the 'random' state, so it is available.
			s_civilizationKeysPlayable[p] = true;
		}
	}
}

void setCivilizationAdjective(PlayerTypes p, const CvString& a)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_civAdjectives.setAt(p, a);
}

void setCivilizationDescription(PlayerTypes p, const CvString& d)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_civDescriptions.setAt(p, d);
}

void setCivilizationPassword(PlayerTypes p, const CvString& pwd)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_civPasswords.setAt(p, pwd);
}

void setCivilizationShortDescription(PlayerTypes p, const CvString& d)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_civShortDescriptions.setAt(p, d);
}

void setClimate(ClimateTypes c)
{
	s_climate = c;
}

void setClimate(const CvString& c)
{
	//Query
	Database::SingleResult kResult;
	DB.SelectAt(kResult, "Climates", "Type", c);
	s_climateInfo.dirtyGet().CacheResult(kResult);

	s_climate = (ClimateTypes)s_climateInfo.get().GetID();
}

void setCustomWorldSize(int iWidth, int iHeight, int iPlayers, int iMinorCivs)
{
	FAssertMsg(iWidth >= 20 && iHeight >= 20, "Cannot have a map that small!");

	const int iArea = iWidth * iHeight;

	CvWorldInfo kClosestSizeType;
	int iSmallestAreaDifference = 64000; // Arbitrarily large at start

	Database::Results kWorldSizes;
	DB.SelectAll(kWorldSizes, "Worlds");
	while(kWorldSizes.Step())
	{
		CvWorldInfo kInfo;
		kInfo.CacheResult(kWorldSizes);

		int iSizeTypeArea = kInfo.getGridWidth() * kInfo.getGridHeight();
		int iAreaDifference = abs(iArea - iSizeTypeArea);
		if(iAreaDifference < iSmallestAreaDifference)
		{
			iSmallestAreaDifference = iAreaDifference;
			kClosestSizeType = kInfo;
		}
	}

	s_worldSize = (WorldSizeTypes)kClosestSizeType.GetID();
	if(iPlayers > 0 || !CvPreGame::mapNoPlayers())
		s_worldInfo = CvWorldInfo::CreateCustomWorldSize(kClosestSizeType, iWidth, iHeight, iPlayers, iMinorCivs);
	else
		s_worldInfo = CvWorldInfo::CreateCustomWorldSize(kClosestSizeType, iWidth, iHeight);
}

void setEmailAddress(PlayerTypes p, const CvString& address)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_emailAddresses.setAt(p, address);
}

void setEmailAddress(const CvString& address)
{
	s_localPlayerEmailAddress = address;
}

void setEndTurnTimerLength(float f)
{
	s_endTurnTimerLength = f;
}

void setEra(EraTypes e)
{
	s_era = e;
}

void setEra(const CvString& e)
{
	for(int i = 0; i < GC.getNumEraInfos(); i++)
	{
		const EraTypes eEra = static_cast<EraTypes>(i);
		CvEraInfo* pkEraInfo = GC.getEraInfo(eEra);
		if(pkEraInfo)
		{
			if(_stricmp(e.c_str(), pkEraInfo->GetType()) == 0)
			{
				setEra(eEra);
			}
		}

	}
}

void setGameMode(GameMode g)
{
	s_gameMode = g;
}

void setGameSpeed(GameSpeedTypes g)
{
	s_gameSpeed = g;

	switch(s_gameSpeed)
	{
	case 0: // GAMESPEED_MARATHON
	{
		// No turn timer
	}
	break;
	case 1: //  GAMESPEED_EPIC
	{
		// TURN_TIMER_SNAIL, 1
		setTurnTimer(static_cast<TurnTimerTypes>(1));
	}
	break;
	case 2: // GAMESPEED_STANDARD
	{
		// TUNRTIMER_MEDIUM, 3
		setTurnTimer(static_cast<TurnTimerTypes>(3));
	}
	break;
	case 3: // GAMESPEED_QUICK
	{
		// TURNTIMER_FAST, 4
		setTurnTimer(static_cast<TurnTimerTypes>(4));
	}
	break;
	default:
		break;
	}
}

void setGameSpeed(const CvString& g)
{
	for(int i = 0; i < GC.getNumGameSpeedInfos(); i++)
	{
		const GameSpeedTypes eGameSpeed = static_cast<GameSpeedTypes>(i);
		CvGameSpeedInfo* pkGameSpeedInfo = GC.getGameSpeedInfo(eGameSpeed);
		if(pkGameSpeedInfo)
		{
			if(_stricmp(g.GetCString(), pkGameSpeedInfo->GetType()) == 0)
			{
				setGameSpeed((GameSpeedTypes)i);
			}
		}
	}
}

void setGameName(const CvString& g)
{
	s_gameName = g;
}

void setGameStarted(bool started)
{
	s_gameStarted = started;
}

void setGameTurn(int turn)
{
	s_gameTurn = turn;
}

void setGameType(GameTypes g, GameStartTypes eStartType)
{
#ifdef INGAME_CIV_DRAFTER
	SLOG("--- setGameType %d %d", (int)g, (int)eStartType);
#endif
	s_gameType = g;
	s_gameStartType = eStartType;
	if(s_gameType != GAME_NETWORK_MULTIPLAYER)
		s_isInternetGame = false;
}

void setGameType(GameTypes g)
{
	s_gameType = g;
	if(s_gameType != GAME_NETWORK_MULTIPLAYER)
		s_isInternetGame = false;
}

void setGameType(const CvString& g)
{
	if(_stricmp(g.GetCString(), "singleplayer") == 0)
	{
		setGameType(GAME_SINGLE_PLAYER, GAME_NEW);
	}
	else if(_stricmp(g.GetCString(), "spload") == 0)
	{
		setGameType(GAME_SINGLE_PLAYER, GAME_LOADED);
	}
	else
	{
		//CvAssertMsg(false, "Invalid game type in ini file!");
		setGameType(GAME_TYPE_NONE);
	}

	if(s_gameType != GAME_NETWORK_MULTIPLAYER)
		s_isInternetGame = false;
}

void setGameStartType(GameStartTypes eStartType)
{
	s_gameStartType = eStartType;
}

void setGameUpdateTime(int updateTime)
{
	s_gameUpdateTime = updateTime;
}

void setHandicap(PlayerTypes p, HandicapTypes h)
{
	if(p >= 0 && p < MAX_PLAYERS){
		s_handicaps.setAt(p, h);

		if(slotStatus(p) == SS_TAKEN){
			//Cache the handicap of human players.  
			//We do this so we can recall the human handicap setting if the human player happens to disconnect and get replaced by an ai.
			setLastHumanHandicap(p, h);
		}
	}
}

void setLastHumanHandicap(PlayerTypes p, HandicapTypes h)
{
	if(p >= 0 && p < MAX_PLAYERS){
		s_lastHumanHandicaps.setAt(p, h);
	}
}

void setInternetGame(bool bIsInternetGame)
{
	s_isInternetGame = bIsInternetGame;
}

void setLeaderHead(PlayerTypes p, LeaderHeadTypes l)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_leaderHeads.setAt(p, l);

		bool bFailed = bindLeaderKeys(p);

		if(bFailed)
		{
			s_leaderHeads.setAt(p, NO_LEADER);
			s_leaderKeysAvailable[p] = true;	// If the key is empty, we assume the selection is in the 'random' state, so it is available.
		}
	}
}

void setLeaderName(PlayerTypes p, const CvString& n)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_leaderNames.setAt(p, n);
}
#ifdef INGAME_HOTKEY_MANAGER
// first find and edit matching ActionInfo instance, then update core DB (probably redundant)
void UpdateHotkey(int iSubType, int iIndex, const char* szHotkeyStr, bool bCtrl, bool bAlt, bool bShift/*, int iHotkeyPriority*/)
{
	typedef std::vector<CvActionInfo*> ActionInfoVector;
	ActionInfoVector& actionInfos = GC.getActionInfo();
	for (ActionInfoVector::iterator it = actionInfos.begin(); it != actionInfos.end(); ++it)
	{
		if ((*it)->getSubType() == iSubType && (*it)->getOriginalIndex() == iIndex)
		{
			//SLOG("OLD HK: %d %d '%s' %d -- %s", (*it)->getSubType(), (*it)->getOriginalIndex(), (*it)->getHotKey(), (*it)->getHotKeyVal(), (*it)->GetDescription());
			(*it)->UpdateHotkey(szHotkeyStr, bAlt, bShift, bCtrl);
			//SLOG("UPD HK: %d %d '%s' %d -- %s", (*it)->getSubType(), (*it)->getOriginalIndex(), (*it)->getHotKey(), (*it)->getHotKeyVal(), (*it)->GetDescription());

			Database::Results kData;
			Database::Connection* db = GC.GetGameDatabase();
			char szSQL[512];

			CvString ActionSubTypesStr[] = { "InterfaceModes", "Commands", "Builds", "UnitPromotions", "Specialists", "Controls", "Automates", "Missions" };
			CvString str = ActionSubTypesStr[iSubType];
			if (iSubType <= (sizeof(ActionSubTypesStr) / sizeof(*ActionSubTypesStr))) {

				//sprintf_s(szSQL, "UPDATE %s SET HotKey = '%s', CtrlDown = %d, AltDown = %d, ShiftDown = %d WHERE ID = %d", str.c_str(), szHotkeyStr, bCtrl ? 1 : 0, bAlt ? 1 : 0, bShift ? 1 : 0, iIndex);
				//SLOG(szSQL);
				if (db->Execute(kData, szSQL))
				{
					while (kData.Step())
					{
						SLOG("Step");
					}
				}
			}
			else {
				SLOG("illegal Subtype index: %d", iSubType);
			}

			break;
			//SLOG("%s", (*it)->getHotKeyDescription());  // crash

		}
	}
}
#endif
#ifdef MP_PLAYERS_VOTING_SYSTEM
bool IsHasRemapToken(PlayerTypes p)
{
	if ((int)p < 0 || (int)p > MAX_MAJOR_CIVS)
		return false;
	if (GC.getGame().isOption("GAMEOPTION_DUEL_STUFF"))
	{
		if (GC.getGame().isOption("GAMEOPTION_ENABLE_REMAP_VOTE"))
		{
			int i = -1;
			CvPreGame::GetGameOption("GAMEOPTION_REMAP_VOTE_TOKENS", i);
			return 1 == ((i >> static_cast<int>(p)) & 1);
		}
	}
	return false;
}

void SetHasRemapToken(PlayerTypes p, bool bValue)
{
	if ((int)p < 0 || (int)p > MAX_MAJOR_CIVS)
		return;
	if (GC.getGame().isOption("GAMEOPTION_DUEL_STUFF"))
	{
		if (GC.getGame().isOption("GAMEOPTION_ENABLE_REMAP_VOTE"))
		{
			int i = -1;
			CvPreGame::GetGameOption("GAMEOPTION_REMAP_VOTE_TOKENS", i);
			//SLOG("OLD REMAP TOKEN %d", i);
			if (bValue)
				((i) |= (1ULL << (static_cast<int>(p))));  // set bit
			else
				((i) &= ~(1ULL << (static_cast<int>(p))));  // clear bit
			CvPreGame::SetGameOption("GAMEOPTION_REMAP_VOTE_TOKENS", i);
			//SLOG("NEW REMAP TOKEN %d", i);
		}
	}
}

#endif
#ifdef INGAME_CIV_DRAFTER
bool IsDraftPlayerReady(PlayerTypes p)
{
	if ((int)p < 0 || (int)p > MAX_MAJOR_CIVS)
		return false;
	return 1 == ((s_draftPlayersReady >> static_cast<int>(p)) & 1);
}

void SetDraftPlayerReady(PlayerTypes p, bool bValue)
{
	if ((int)p < 0 || (int)p > MAX_MAJOR_CIVS)
		return;
	SLOG("OLD DRAFT READY %d", s_draftPlayersReady);
	if (bValue)
		((s_draftPlayersReady) |= (1ULL << (static_cast<int>(p))));  // set bit
	else
		((s_draftPlayersReady) &= ~(1ULL << (static_cast<int>(p))));  // clear bit
	SLOG("NEW DRAFT READY %d", s_draftPlayersReady);
}

void DraftResponseSecretHash(PlayerTypes p, const char* szHash)
{
	if (s_draftCurrentProgress != DRAFT_PROGRESS_INIT)
	{
		SLOG("WARN attempt to set secret hash outside DRAFT_PROGRESS_INIT stage %d %d", p, s_draftCurrentProgress);
		return;
	}
	uint uiPlayerID = (uint)p;
	if (uiPlayerID >= MAX_MAJOR_CIVS)
	{
		SLOG("WARN uiPlayerID out of bounds %d", uiPlayerID);
		return;
	}
	SetDraftPlayerReady(p, true);
	if (s_draftPlayerSecretHashes[uiPlayerID] != "")
	{
		SLOG("WARN attempt to overwrite secret hash for player %d", uiPlayerID);
		return;
	}
	s_draftPlayerSecretHashes[uiPlayerID] = CvString(szHash);
	DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), CvString::format("ready|%d", uiPlayerID).c_str());
	SLOG("set secret hash for player %d to %s", uiPlayerID, szHash);

	// check all secret hashes received
	bool bReady = true;
	for (uint i = 0; i < s_draftPlayerSecretHashes.size(); i++)
	{
		if (isHuman((PlayerTypes)i) && (s_draftPlayerSecretHashes[i] == ""))
			bReady = false;
	}
	if (bReady)
	{
		SLOG("received all secret hashes");
		s_draftCurrentProgress = DRAFT_PROGRESS_BANS;
		// TODO enable bans in UI
		DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), "DRAFT_PROGRESS_BANS|");
	}
}

void DraftSyncBans(PlayerTypes p, const char* szBans)
{
	CvString msg = CvString::format("%d|%d|%s", s_draftCurrBansMsgNumber, static_cast<int>(p), szBans);
	s_draftCurrBansMsgNumber++;
	gDLL->SendRenameCity(-5, msg);
}

void DraftResponseBans(PlayerTypes p, const char* szBans)
{
	if (s_draftCurrentProgress != DRAFT_PROGRESS_BANS)
	{
		SLOG("WARN attempt to set bans outside DRAFT_PROGRESS_BANS stage %d %d", p, s_draftCurrentProgress);
		return;
	}
	uint uiPlayerID = (uint)p;

	if (uiPlayerID >= MAX_MAJOR_CIVS)
	{
		SLOG("WARN uiPlayerID out of bounds %d", uiPlayerID);
		return;
	}

	std::vector<CvString> tokens;
	CvString s = CvString(szBans);
	size_t pos = 0;
	std::string token;
	int civs1 = 0;
	int civs2 = 0;
	uint id;

	// TODO probably redundant
	// create bans bitmap (using 2x32 bit integers)
	while ((pos = s.find(',')) != std::string::npos) {
		token = s.substr(0, pos);
		tokens.push_back(token);
		s.erase(0, pos + 1);
		if (sscanf(token.c_str(), "%d", &id) == 1)
		{
			if (id >= 0 && id <= 63)
			{
				if (id < 32)
					civs1 |= 1 << id;
				else
					civs2 |= 1 << (id - 32);
			}
		}
	}
	tokens.push_back(s);
	if (sscanf(s.c_str(), "%d", &id) == 1)
	{
		if (id >= 0 && id <= 63)
		{
			if (id < 32)
				civs1 |= 1 << id;
			else
				civs2 |= 1 << (id - 32);
		}
	}

	// TODO probably redundant
	int old1, old2;
	if (!CvPreGame::GetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS1", old1))
		old1 = 0;
	if (!CvPreGame::GetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS2", old2))
		old2 = 0;

	// verify bans uniqueness (host only)
	if (old1 & civs1 || old2 & civs2)
	{
		SLOG("WARN bans not unique 1old %d new %d 2old %d new %d", old1, civs1, old2, civs2);
		if (gDLL->IsHost())
		{
			CvString s2 = CvString::format("%d:%d:%s;", static_cast<int>(activePlayer()), uiPlayerID, szBans);
			gDLL->SendRenameCity(-4, s2);
		}
		return;
	}

	SLOG("1old %d new %d res %d", old1, civs1, old1 ^ civs1);
	SLOG("2old %d new %d res %d", old2, civs2, old2 ^ civs2);
	CvPreGame::SetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS1", old1 ^ civs1);
	CvPreGame::SetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS2", old2 ^ civs2);

	// notify UI
	DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), CvString::format("ban-success|%d|%s", uiPlayerID, szBans).c_str());
	s_draftPlayerBans.push_back(CvString::format("%d:%s;", uiPlayerID, szBans));

	// check bans ready
	bool bReady = true;
	std::vector<bool> vPlayers(MAX_MAJOR_CIVS);
	for (uint i = 0; i < s_draftPlayerBans.size(); i++)
	{
		if ((pos = s_draftPlayerBans[i].find(':')) != std::string::npos)
		{
			if (sscanf(s_draftPlayerBans[i].substr(0, pos).c_str(), "%d", &id) == 1)
			{
				if (id >= 0 && id < vPlayers.size())
				{
					vPlayers[id] = true;
				}
			}
		}
	}
	for (uint i = 0; i < vPlayers.size(); i++)
	{
		if (isHuman((PlayerTypes)i) && !vPlayers[i])
			bReady = false;
	}
	if (bReady)
	{
		// extra step required due to network jitter
		s_draftCurrentProgress = DRAFT_PROGRESS_BUSY;
		DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), "DRAFT_PROGRESS_BUSY|");
		gDLL->SendRenameCity(-7, "");
	}
}

void DraftResponseAllBansReceived(PlayerTypes p)
{
	uint uiPlayerID = (uint)p;
	if (uiPlayerID >= MAX_MAJOR_CIVS)
	{
		SLOG("WARN uiPlayerID out of bounds %d", uiPlayerID);
		return;
	}
	
	s_draftAllBansReceived[uiPlayerID] = true;

	// check AllBansReceived
	bool bReady = true;
	for (uint i = 0; i < s_draftAllBansReceived.size(); i++)
	{
		if (isHuman((PlayerTypes)i) && !s_draftAllBansReceived[i])
			bReady = false;
	}
	if (bReady)
	{
		SLOG("send local secret")
		gDLL->SendRenameCity(-3, s_draftLocalSecret);
	}
};

void DraftResponseSecret(PlayerTypes p, const char* szSecret)
{
	if (s_draftCurrentProgress != DRAFT_PROGRESS_BUSY)
	{
		SLOG("WARN attempt to set secret outside DRAFT_PROGRESS_BUSY stage %d %d", p, s_draftCurrentProgress);
		return;
	}
	uint uiPlayerID = (uint)p;
	if (uiPlayerID >= MAX_MAJOR_CIVS)
	{
		SLOG("WARN uiPlayerID out of bounds %d", uiPlayerID);
		return;
	}
	if (s_draftPlayerSecrets[uiPlayerID] != "")
	{
		SLOG("WARN attempt to overwrite secret for player %d", uiPlayerID);
		return;
	}
	if (s_draftPlayerSecretHashes[uiPlayerID] != hash(szSecret))
	{
		SLOG("WARN received secret does not match declared secret hash: player %d declared %s received %s (hash %s)", uiPlayerID, s_draftPlayerSecretHashes[uiPlayerID].c_str(), szSecret, hash(CvString(szSecret)).c_str());
		DraftLocalReset(GetLocalizedText("TXT_KEY_DRAFTS_RESET_HASH_MISMATCH", nicknameDisplayed((PlayerTypes)uiPlayerID).c_str()).c_str());
		return;
	}
	s_draftPlayerSecrets[uiPlayerID] = CvString(szSecret);
	SLOG("set secret for player %d to %s", uiPlayerID, szSecret);

	// check all secrets received
	bool bReady = true;
	for (uint i = 0; i < s_draftPlayerSecrets.size(); i++)
	{
		if (isHuman((PlayerTypes)i) && (s_draftPlayerSecrets[i] == ""))
			bReady = false;
	}
	if (bReady)
	{
		SLOG("received all secrets");
		s_draftCurrentProgress = DRAFT_PROGRESS_OVER;
		// TODO gather bans & roll civs
		int sum = 0;
		for (uint i = 0; i < s_draftPlayerSecrets.size(); i++)
		{
			CvString szSecret = s_draftPlayerSecrets[i];
			if (szSecret != "")
			{
				int iSecret;
				if (sscanf(szSecret.c_str(), "%08x", &iSecret) == 1)
				{
					sum += iSecret;
					SLOG("interim sum %d", sum);
				}
			}
		}
		SLOG("SUM: %d", sum);

		std::vector<int> civList;
		int bans1, bans2;
		if (!CvPreGame::GetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS1", bans1))
			bans1 = 0;
		if (!CvPreGame::GetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS2", bans2))
			bans2 = 0;
		for (int iCiv = 0; iCiv < GC.getNumCivilizationInfos(); ++iCiv)
		{
			CvCivilizationInfo* pkCivilization = GC.getCivilizationInfo((CivilizationTypes)iCiv);
			if (pkCivilization != NULL && pkCivilization->isPlayable() && ((((iCiv < 32) ? bans1 : bans2) >> iCiv) & 1) != 1)
			{
				civList.push_back(iCiv);
			}
		}
		SLOG("civList size: %d", civList.size());
		// shuffle civs
		std::srand(sum);
		uint uiSize = civList.size();
		for (uint k = 0; k < uiSize; k++) {
			int r = k + std::rand() % (uiSize - k);
			std::swap(civList[k], civList[r]);
		}
		CvString s;
		for (std::vector<int>::const_iterator i = civList.begin(); i != civList.end(); ++i)
		{
			s += CvString::format("%d", *i);
			s.append(",");
		}
		SLOG("shuffled civs %s", s.c_str());
		s_draftShuffledCivs = s;

		DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), CvString::format("DRAFT_PROGRESS_OVER|%s", s.c_str()).c_str());
	}
}

void DraftResponseBanRollback(PlayerTypes p, const char* szBans)
{
	if (s_draftCurrentProgress != DRAFT_PROGRESS_BANS)
	{
		SLOG("WARN attempt to set bans outside DRAFT_PROGRESS_BANS stage %d %d", p, s_draftCurrentProgress);
		return;
	}
	uint uiPlayerID = (uint)p;

	if (uiPlayerID >= MAX_MAJOR_CIVS)
	{
		SLOG("WARN uiPlayerID out of bounds %d", uiPlayerID);
		return;
	}

	DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), CvString::format("ban-failure|%s", szBans).c_str());
};

void DraftResponseSwapPlayers(PlayerTypes p, const char* szSwapWith)
{
	if (s_draftCurrentProgress != DRAFT_PROGRESS_OVER)
	{
		CvString strSwapWith = CvString(szSwapWith);
		int newp = -1;
		if (strSwapWith != "")
		{
			if (sscanf(strSwapWith.c_str(), "%d", &newp) != 1)
			{
				return;
			}
		}
		int oldp = (int)p;
		if (newp >= 0 && newp < MAX_MAJOR_CIVS)
		{
			s_draftPlayerSecretHashes[oldp] = "";
			s_draftPlayerSecretHashes[p] = "";
			SetDraftPlayerReady((PlayerTypes)oldp, false);
			SetDraftPlayerReady(p, false);
			DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), CvString::format("upd|%s", szSwapWith).c_str());
		}
	}
};


void DraftLocalReady()
{
	if (s_draftCurrentProgress != DRAFT_PROGRESS_INIT)
	{
		SLOG("WARN attempt to start drafts outside DRAFT_PROGRESS_INIT stage %d", s_draftCurrentProgress);
		return;
	}

	if (s_draftLocalSecret != "")
	{
		SLOG("WARN attempt to overwrite local secret");
		SLOG("reusing old secret instead: %s", s_draftLocalSecret.c_str());
		s_draftLocalSecretHash = hash(s_draftLocalSecret);
		DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), CvString::format("DRAFT_PROGRESS_INIT|%s", s_draftLocalSecretHash.c_str()).c_str());
		return;
	}

	// generate random secret -- as invulnerable as possible
	bool bGood = false;
	HCRYPTPROV hCryptProv;
	BYTE pbData[512];
	if (CryptAcquireContext(&hCryptProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		if (CryptGenRandom(hCryptProv, 512, pbData))
		{
			s_draftLocalSecret = hash(CvString(reinterpret_cast<char const*>(pbData)));
			SLOG("Random sequence generated. %s", s_draftLocalSecret.c_str());
			bGood = true;
		}
		else
		{
			SLOG("Error during CryptGenRandom.");
		}
		if (!CryptReleaseContext(hCryptProv, 0))
		{
			SLOG("CryptReleaseContext ERROR")
		}
	}
	if (!bGood)  // fallbacks to basic rand if WinCrypt fails
	{
		int buf = -1;
		HMODULE advapi = GetModuleHandle("advapi32.dll");
		if (advapi != NULL)
		{
			BOOLEAN(APIENTRY * RtlGenRandom)(void*, ULONG) =
				(BOOLEAN(APIENTRY*)(void*, ULONG))GetProcAddress(advapi, "SystemFunction036");
			if (RtlGenRandom != NULL)
			{
				RtlGenRandom(&buf, 32UL);
			}
		}
		SLOG("gen secret %d", buf);
		s_draftLocalSecret = hash(CvString::format("%d", buf));
	}
	s_draftLocalSecretHash = hash(s_draftLocalSecret);

	DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), CvString::format("DRAFT_PROGRESS_INIT|%s", s_draftLocalSecretHash.c_str()).c_str());
}

void DraftLocalReset(const char* szReason)
{
	SLOG("--- DRAFT LOCAL RESET ---");
	for (uint i = 0; i < s_draftPlayerSecrets.size(); i++)
	{
		s_draftPlayerSecrets[i] = "";
	}
	for (uint i = 0; i < s_draftPlayerSecretHashes.size(); i++)
	{
		s_draftPlayerSecretHashes[i] = "";
	}
	for (uint i = 0; i < s_draftAllBansReceived.size(); i++)
	{
		s_draftAllBansReceived[i] = false;
	}
	s_draftPlayerBans.clear();
	s_draftCurrentProgress = DRAFT_PROGRESS_INIT;
	s_draftResult = DRAFT_RESULT_NONE;
	s_draftShuffledCivs = "";
	s_draftPlayersReady = 0;
	CvPreGame::SetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS1", 0);
	CvPreGame::SetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS2", 0);
	s_draftBansMsgQueue.clear();
	s_draftCurrBansMsgNumber = 0;
	s_draftLocalSecret = "";
	s_draftLocalSecretHash = "";
	CvString strReason(CvString("reset|") + szReason);
	DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), strReason.c_str());
}

void DraftLocalUpdate()
{
	bool bOk = true;
	if (s_draftCurrentProgress == DRAFT_PROGRESS_INIT)
	{
		// check all secret hashes received
		bool bReady = true;
		for (uint i = 0; i < s_draftPlayerSecretHashes.size(); i++)
		{
			if (isHuman((PlayerTypes)i) && (s_draftPlayerSecretHashes[i] == ""))
				bReady = false;
		}
		if (bReady)
		{
			SLOG("received all secret hashes");
			s_draftCurrentProgress = DRAFT_PROGRESS_BANS;
			// TODO enable bans in UI
			DLLUI->AddMessage(0, activePlayer(), true, GC.getEVENT_MESSAGE_TIME(), "DRAFT_PROGRESS_BANS|");
		}
	}
	else if (s_draftCurrentProgress == DRAFT_PROGRESS_BANS || s_draftCurrentProgress == DRAFT_PROGRESS_BUSY)
	{
		// validate active players
		for (uint i = 0; i < s_draftPlayerSecretHashes.size(); i++)
		{
			if (isHuman((PlayerTypes)i))
			{
				if (s_draftPlayerSecretHashes[i] == "")
					bOk = false;  // new player joined after draft has started?
			}
			else
			{
				if (s_draftPlayerSecretHashes[i] != "")
					bOk = false;  // player dissappeared or was kicked out during drafts?
				if (s_draftPlayerSecrets[i] != "")
					bOk = false;  // player sent secret and then dissappeared?
			}
		}
	}

	if (!bOk)
	{
		DraftLocalReset("TXT_KEY_DRAFTS_RESET_MISSING_PLAYER");  // something went wrong; abort draft progress
	}
}

bool IsCivBanned(int iCivId)
{
	if (iCivId < 0 || iCivId > 63)
		return false;

	int i = -1;
	if (iCivId < 32)
		CvPreGame::GetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS1", i);
	else
		CvPreGame::GetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS2", i);

	return 1 == ((i >> iCivId) & 1);
}

void SetCivBanned(int iCivId, bool bValue)
{
	if (iCivId < 0 || iCivId > 63)
		return;

	int i = -1;
	if (iCivId < 32)
	{
		CvPreGame::GetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS1", i);
		SLOG("OLD DRAFT BANNED CIVS (0-31) %d", i);
	}
	else
	{
		CvPreGame::GetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS2", i);
		SLOG("OLD DRAFT BANNED CIVS (32-63) %d", i);
	}

	if (bValue)
		((i) |= (1ULL << (iCivId - (iCivId < 32 ? 0 : 32))));  // set bit
	else
		((i) &= ~(1ULL << (iCivId - (iCivId < 32 ? 0 : 32))));  // clear bit

	if (iCivId < 32)
	{
		CvPreGame::SetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS1", i);
		SLOG("NEW DRAFT BANNED CIVS (0-31) %d", i);
	}
	else
	{
		CvPreGame::SetGameOption("GAMEOPTION_DRAFTS_BANNED_CIVS2", i);
		SLOG("NEW DRAFT BANNED CIVS (32-63) %d", i);
	}
}

CvString getDraftShuffledCivs()
{
	return s_draftShuffledCivs;
}

int getDraftPlayersReady()
{
	return s_draftPlayersReady;
}

int getDraftCurrentProgress()
{
	return s_draftCurrentProgress;
}

int getDraftResult()
{
	return s_draftResult;
}

std::vector<CvString> getDraftPlayerBans()
{
	return s_draftPlayerBans;
}

std::vector<CvString> getDraftPlayerSecrets()
{
	return s_draftPlayerSecrets;
}

std::vector<CvString> getDraftPlayerSecretHashes()
{
	return s_draftPlayerSecretHashes;
}

std::vector<bool> getDraftAllBansReceived()
{
	return s_draftAllBansReceived;
}

void setDraftShuffledCivs(CvString value)
{
	s_draftShuffledCivs = value;
}

void setDraftPlayersReady(int value)
{
	s_draftPlayersReady = value;
}

void setDraftCurrentProgress(int value)
{
	s_draftCurrentProgress = value;
}
void setDraftResult(int value)
{
	s_draftResult = value;
}

void setDraftPlayerBans(std::vector<CvString> value)
{
	s_draftPlayerBans = value;
}

void setDraftPlayerSecrets(std::vector<CvString> value)
{
	s_draftPlayerSecrets = value;
}

void setDraftAllBansReceived(std::vector<bool> value)
{
	s_draftAllBansReceived = value;
}

void setDraftPlayerSecretHashes(std::vector<CvString> value)
{
	s_draftPlayerSecretHashes = value;
}
#endif

void setLeaderKey(PlayerTypes p, const CvString& szKey)
{
#ifdef INGAME_HOTKEY_MANAGER
	// UI -> dll
	// 0001 xxxx xxxx xxxx xxxx xxxx xxxx xxxx - update hotkey
	// 0010 xxxx xxxx xxxx xxxx xxxx xxxx xxxx - set remap token
	// 0011 xxxx xxxx xxxx xxxx xxxx xxxx xxxx - drafts local player ready
	// 0100 xxxx xxxx xxxx xxxx xxxx xxxx xxxx - drafts local reset
	// 0101 xxxx xxxx xxxx xxxx xxxx xxxx xxxx - drafts local update
	// 0110 xxxx xxxx xxxx xxxx xxxx xxxx xxxx - player disconnected
	// intercept Pregame.SetLeaderKey here, check if leftmost bits are 0001
	// it actually limits max PlayerType value to 2^28 (it's OK)
	if ((((uint)p >> 28) & 15) == 1)  // 4 left-most bits reserved for mode (0 default; 1-15 moddable)
	{
		int iSubType = (((uint)p >> 24) & 15);  // 4-bit
		int iIndex = (((uint)p >> 8) & 65535);  // 16-bit
		// bool bHotkeyAlt = (((uint)p >> 7) & 1);  // 4x1-bit flags
		bool bCtrl = (((uint)p >> 6) & 1);
		bool bAlt = (((uint)p >> 5) & 1);
		bool bShift = (((uint)p >> 4) & 1);
		// int iHotkeyPriority = (((uint)p) & 15);  // 4-bit

		const char* hotkeyStr = szKey.c_str();

		UpdateHotkey(iSubType, iIndex, hotkeyStr, bCtrl, bAlt, bShift/*, iHotkeyPriority*/);
		return;
	}
#endif
#ifdef MP_PLAYERS_VOTING_SYSTEM
	if ((((uint)p >> 28) & 15) == 2)  // 4 left-most bits reserved for mode (0 default; 1-15 moddable)
	{
		uint uiPlayerID = (((uint)p >> 1) & 134217727);  // 27-bit
		int iValue = (((uint)p) & 1);  // 1-bit

		if (uiPlayerID >= MAX_MAJOR_CIVS)
			return;
		SetHasRemapToken((PlayerTypes)uiPlayerID, (bool)iValue);
		return;
	}
#endif
#ifdef INGAME_CIV_DRAFTER
	if ((((uint)p >> 28) & 15) == 3)  // player is ready for draft
	{
		DraftLocalReady();
		return;
	}
	else if ((((uint)p >> 28) & 15) == 4)  // force reset
	{
		DraftLocalReset(szKey.c_str());
		return;
	}
	else if ((((uint)p >> 28) & 15) == 5)  // local update
	{
		DraftLocalUpdate();
		return;
	}
	else if ((((uint)p >> 28) & 15) == 6)  // player disconnected during INIT stage
	{
		uint uiPlayerID = (((uint)p) & 268435455);  // 28-bit
		if (uiPlayerID >= MAX_MAJOR_CIVS)
			return;
		if (s_draftCurrentProgress != DRAFT_PROGRESS_INIT)
		{
			SLOG("WARN attempt to handle player disconnect outside DRAFT_PROGRESS_INIT stage; should reset drafts instead");
			return;
		}
		s_draftPlayerSecretHashes[uiPlayerID] = "";
		SetDraftPlayerReady((PlayerTypes)uiPlayerID, false);
		return;
	}
#endif
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_leaderKeys[p] = szKey;
		// Check to see if this is available, if not, set the index to NO_LEADER and the package ID to invalid
		// The key will stay valid
		bool bFailed = true;
		if(szKey.length() > 0)
		{
			// During the pre-game, we can't be sure the cached *Infos are current, so query the database
			Database::Connection* pDB = GC.GetGameDatabase();
			if(pDB)
			{
				Database::Results kResults;
				if(pDB->Execute(kResults, "SELECT ID, PackageID from Leaders where Type = ? LIMIT 1"))
				{
					kResults.Bind(1, szKey.c_str());
					if(kResults.Step())
					{
						s_leaderHeads.setAt(p, (LeaderHeadTypes)kResults.GetInt(0));
						if(!ExtractGUID(kResults.GetText(1), s_leaderPackageID[p]))
							ClearGUID(s_leaderPackageID[p]);
						s_leaderKeysAvailable[p] = true;
						bFailed = false;
					}
				}
			}
		}

		if(bFailed)
		{
			s_leaderHeads.setAt(p, NO_LEADER);
			ClearGUID(s_leaderPackageID[p]);
			s_leaderKeysAvailable[p] = (szKey.length() == 0);	// If the key was empty, then it is the 'random' state so it is available
		}
	}
}

// Return true if the key for the players leader is available on this machine
bool leaderKeyAvailable(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		return s_civilizationKeysAvailable[p];
	}

	return false;
}

// Set the unique key value for the leaders's package.
void setLeaderKeyPackageID(PlayerTypes p, const GUID& kKey)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_leaderPackageID[p] = kKey;
	}
}

void setLoadFileName(const CvString& f)
{
	setLoadFileName(f, STORAGE_LOCAL);
}

void setLoadFileName(const CvString& f, StorageLocation eStorage)
{
	if(s_loadFileName.get() != f)
	{
		s_loadFileName = f;
	}

	s_loadFileStorage = eStorage;
}

bool readPlayerSlotInfo(FDataStream& loadFrom, bool bReadVersion)
{
	if(gDLL->IsHost())	// Only do this if the player is the host.
	{
		loadSlotHints(loadFrom, bReadVersion);
	}
	return true;
}

void setMapNoPlayers(bool p)
{
	s_mapNoPlayers = p;
}

void setMapRandomSeed(unsigned int newSeed)
{
	s_mapRandomSeed = newSeed;
}

void setLoadWBScenario(bool b)
{
	s_loadWBScenario = b;
}

void setOverrideScenarioHandicap(bool b)
{
	s_overrideScenarioHandicap = b;
}

void setMapScriptName(const CvString& s)
{
	if(s_mapScriptName.get() != s)
	{
		s_mapScriptName = s;
		ResetMapOptions();
	}
}

void setMaxCityElimination(int m)
{
	s_maxCityElimination = m;
}

void setMaxTurns(int m)
{
	s_maxTurns = m;
}

void setMinorCivType(PlayerTypes p, MinorCivTypes m)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_minorCivTypes.setAt(p, m);
}

void setMinorCiv(PlayerTypes p, bool isMinor)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_minorNationCivs.setAt(p, isMinor);
}

void setMultiplayerAIEnabled(bool enabled)
{
	s_multiplayerAIEnabled = enabled;
}

void setMultiplayerOption(MultiplayerOptionTypes o, bool enabled)
{
	if(o >= 0 && o < NUM_MPOPTION_TYPES)
		s_multiplayerOptions.setAt(o, enabled);
}

void setMultiplayerOptions(const std::vector<bool>& mpOptions)
{
	s_multiplayerOptions = mpOptions;
}

void setNumMinorCivs(int n)
{
	s_numMinorCivs = n;
}

void setNetID(PlayerTypes p, int id)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_netIDs.setAt(p, id);
}

void setNickname(PlayerTypes p, const CvString& n)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		string _szName(n);
		if(!n.IsEmpty())
		{
			size_t _pos = n.rfind('@');
			size_t _cNum = n.length() - _pos;
			// The max player name length is defined as 64 in ffirewiretypes, the max size of the string past that can only be 64 if we represent it as binary, currently representing it as HEX
			if((_pos > 0 && _pos < 64) && _cNum < 32)
			{
				_szName.erase(_pos, _cNum);
			}
		}
		s_displayNicknames[p] = (CvString)_szName;
		s_nicknames.setAt(p, n);
	}
}

void setPitBossTurnTime(int t)
{
	s_pitBossTurnTime = t;
}

void setPlayable(PlayerTypes p, bool playable)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_playableCivs.setAt(p, playable);
	}
}

void setPlayerColor(PlayerTypes p, PlayerColorTypes c)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_playerColors.setAt(p, c);
}

void setPrivateGame(bool isPrivateGame)
{
	s_privateGame = isPrivateGame;
}

void setQuickCombat(bool isQuickCombat)
{
	s_quickCombat = isQuickCombat;
	SetGameOption(GAMEOPTION_QUICK_COMBAT_KEY, isQuickCombat?1:0);
}

void setQuickCombatDefault(bool isQuickCombat)
{
	s_quickCombatDefault = isQuickCombat;
}

void setQuickHandicap(HandicapTypes h)
{
	s_quickHandicap = h;
}

void setQuickHandicap(const CvString& h)
{
	for(int i = 0; i < GC.getNumHandicapInfos(); i++)
	{
		const HandicapTypes eHandicap = static_cast<HandicapTypes>(i);
		CvHandicapInfo* pkHandicapInfo = GC.getHandicapInfo(eHandicap);
		if(pkHandicapInfo)
		{
			if(_stricmp(h.GetCString(), pkHandicapInfo->GetType()) == 0)
			{
				setQuickHandicap(eHandicap);
			}
		}
	}
}

void setQuickMovement(bool isQuickMovement)
{
	SetGameOption(GAMEOPTION_QUICK_MOVEMENT_KEY, isQuickMovement?1:0);
}

void setQuickstart(bool isQuickstart)
{
	s_quickstart = isQuickstart;
}

void setRandomWorldSize(bool isRandomWorldSize)
{
	s_randomWorldSize = isRandomWorldSize;
}

void setRandomMapScript(bool isRandomMapScript)
{
	if(s_randomMapScript != isRandomMapScript)
	{
		s_randomMapScript = isRandomMapScript;
		ResetMapOptions();
	}
}

void setReady(PlayerTypes p, bool bIsReady)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		if(!bIsReady || p != activePlayer() || canReadyLocalPlayer())
			s_readyPlayers.setAt(p, bIsReady);
	}
}

void setSeaLevel(SeaLevelTypes s)
{
	s_seaLevel = s;
}

void setSeaLevel(const CvString& s)
{
	//Query
	Database::SingleResult kResult;
	DB.SelectAt(kResult, "SeaLevels", "Type", s.c_str());
	s_seaLevelInfo.dirtyGet().CacheResult(kResult);

	s_seaLevel = (SeaLevelTypes)s_seaLevelInfo.get().GetID();
}

void setSlotClaim(PlayerTypes p, SlotClaim s)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_slotClaims.setAt(p, s);
}

void setSlotStatus(PlayerTypes p, SlotStatus eSlotStatus)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_slotStatus.setAt(p, eSlotStatus);
}

void setAllSlotStatus(const std::vector<SlotStatus>& vSlotStatus)
{
	s_slotStatus = vSlotStatus;
}

void setSyncRandomSeed(unsigned int newSeed)
{
	s_syncRandomSeed = newSeed;
}

void setTargetScore(int t)
{
	s_targetScore = t;
}

void setTeamType(PlayerTypes p, TeamTypes t)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_teamTypes.setAt(p, t);
}

void setTransferredMap(bool transferred)
{
	s_transferredMap = transferred;
}

void setTurnTimer(TurnTimerTypes t)
{
	s_turnTimerType = t;
	Database::SingleResult kResult;
	if(!DB.SelectAt(kResult, "TurnTimers", "ID", static_cast<int>(t)))
	{
		CvAssertMsg(false, "Cannot find turn timer info.");
	}
	s_turnTimer.dirtyGet().CacheResult(kResult);
}

void setTurnTimer(const CvString& t)
{
	Database::SingleResult kResult;
	if(!DB.SelectAt(kResult, "TurnTimers", "Type", t.c_str()))
	{
		CvAssertMsg(false, "Cannot find turn timer info.");
	}
	s_turnTimer.dirtyGet().CacheResult(kResult);

	s_turnTimerType = (TurnTimerTypes)s_turnTimer.get().GetID();
}

void SetCityScreenBlocked(bool bCityScreenBlocked)
{
	s_bCityScreenBlocked = bCityScreenBlocked;
}

void setVersionString(const std::string& v)
{
	s_versionString = v;
}

void setVictory(VictoryTypes v, bool isValid)
{
	if(v >= 0 && v < s_numVictoryInfos)
		s_victories.setAt(v, isValid);
}

void setVictories(const std::vector<bool>& v)
{
	s_victories = v;
	s_numVictoryInfos = s_victories.size();
}

void setWhiteFlag(PlayerTypes p, bool flag)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_whiteFlags.setAt(p, flag);
}

void setTurnNotifySteamInvite(PlayerTypes p, bool flag)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_turnNotifySteamInvite.setAt(p, flag);
}

void setTurnNotifyEmail(PlayerTypes p, bool flag)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_turnNotifyEmail.setAt(p, flag);
}

void setTurnNotifyEmailAddress(PlayerTypes p, const CvString& emailAddress)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_turnNotifyEmailAddress.setAt(p, emailAddress);
}

void VerifyHandicap(PlayerTypes p)
{//Verifies that the current handicap is valid for the current player.
	//non-ai players can't use the default ai handicap and ai players MUST use it.
	if(slotStatus(p) == SS_COMPUTER){
		setHandicap(p, (HandicapTypes)GC.getAI_HANDICAP());
	}
	else if(handicap(p) == GC.getAI_HANDICAP()){
		if(lastHumanHandicap(p) != NO_HANDICAP){
			setHandicap(p, lastHumanHandicap(p));
		}
		else if(GC.getGame().isNetworkMultiPlayer()){
			setHandicap(p, (HandicapTypes)GC.getMULTIPLAYER_HANDICAP());
		}
		else{
			setHandicap(p, (HandicapTypes)GC.getSTANDARD_HANDICAP());
		}
	}
}

void setWorldSize(WorldSizeTypes w, bool bResetSlots)
{
	CvAssert(!gameStarted() || isNetworkMultiplayerGame() || isHotSeatGame());
	Database::Connection& db = *GC.GetGameDatabase();

	//Query
	Database::Results kQuery;
	db.Execute(kQuery, "SELECT * from Worlds where ID = ? LIMIT 1");
	kQuery.Bind(1, w);

	if(kQuery.Step())
	{
		s_worldInfo.dirtyGet().CacheResult(kQuery);
		s_worldSize = w;
		if(bResetSlots)
			resetSlots();
	}
	else
	{
		CvAssertMsg(false, "Could not find world size entry.")
	}
}

void setWorldSize(const CvString& w)
{
	CvAssert(!gameStarted() || isNetworkMultiplayerGame() || isHotSeatGame());
	Database::Connection& db = *GC.GetGameDatabase();

	Database::Results kQuery;

	db.Execute(kQuery, "SELECT * from Worlds where Type = ? LIMIT 1");
	kQuery.Bind(1, w.c_str(), w.size(), false);

	if(kQuery.Step())
	{
		s_worldInfo.dirtyGet().CacheResult(kQuery);
		s_worldSize = (WorldSizeTypes)s_worldInfo.get().GetID();
		resetSlots();
	}
	else
	{
		CvAssertMsg(false, "Could not find world size entry.")
	}
}

SlotClaim slotClaim(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_slotClaims[p];
	return SLOTCLAIM_UNASSIGNED;
}

SlotStatus slotStatus(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_slotStatus[p];
	return SS_CLOSED;
}

const std::vector<SlotStatus>& GetSlotStatus()
{
	return s_slotStatus;
}

void StringToBools(const char* szString, int* iNumBools, bool** ppBools)
{
	FAssertMsg(szString, "null string");
	if(szString)
	{
		*iNumBools = strlen(szString);
		*ppBools = FNEW(bool[*iNumBools], c_eCiv5GameplayDLL, 0);
		int i;
		for(i=0; i<*iNumBools; i++)
		{
			(*ppBools)[i] = (szString[i]=='1');
		}
	}
}
unsigned int syncRandomSeed()
{
	return s_syncRandomSeed;
}

int targetScore()
{
	return s_targetScore;
}

bool transferredMap()
{
	return s_transferredMap;
}

TurnTimerTypes turnTimer()
{
	return s_turnTimerType;
}

const CvTurnTimerInfo& turnTimerInfo()
{
	return s_turnTimer;
}

const std::string& versionString()
{
	return s_versionString;
}

const std::vector<bool>& victories()
{
	return s_victories;
}

const CvWorldInfo& worldInfo()
{
	if(s_worldSize != s_worldInfo.get().GetID())
	{
		Database::SingleResult kResult;
		DB.SelectAt(kResult, "Worlds", s_worldSize);
		s_worldInfo.dirtyGet().CacheResult(kResult);
	}
	return s_worldInfo;
}

WorldSizeTypes worldSize()
{
	return s_worldSize;
}

void writeArchive(FDataStream& saveTo)
{
	uint uiVersion = 6;
	saveTo << uiVersion;

	saveTo << s_activePlayer;
	saveTo << s_adminPassword;
	saveTo << s_advancedStartPoints;
	saveTo << s_alias;
	saveTo << s_artStyles;
	saveTo << s_autorun;
	saveTo << s_autorunTurnDelay;
	saveTo << s_autorunTurnLimit;
	saveTo << s_bandwidth;
	saveTo << s_calendar;
	saveTo << s_calendarInfo;
	saveTo << s_civAdjectives;
	saveTo << s_civDescriptions;
	saveTo << s_civPasswords;
	saveTo << s_civShortDescriptions;
	saveTo << s_climate;
	saveTo << s_climateInfo;
	saveTo << s_era;
	saveTo << s_emailAddresses;
	saveTo << s_endTurnTimerLength;
	saveTo << s_flagDecals;
	saveTo << s_DEPRECATEDforceControls;
	saveTo << s_gameMode;
	saveTo << s_gameName;
	saveTo << s_gameSpeed;
	saveTo << s_gameStarted;
	saveTo << s_gameTurn;
	saveTo << s_gameType;
	saveTo << s_gameMapType;
	saveTo << s_gameUpdateTime;
	saveTo << s_handicaps;
	saveTo << s_lastHumanHandicaps;
	saveTo << s_isEarthMap;
	saveTo << s_isInternetGame;
	saveTo << s_leaderNames;
	saveTo << s_loadFileName;
	saveTo << s_localPlayerEmailAddress;
	saveTo << s_mapNoPlayers;
	saveTo << s_mapRandomSeed;
	saveTo << s_loadWBScenario;
	saveTo << s_overrideScenarioHandicap;
	saveTo << s_mapScriptName;
	saveTo << s_maxCityElimination;
	saveTo << s_maxTurns;
	saveTo << s_numMinorCivs;
	CvInfosSerializationHelper::WriteTypeArray<MinorCivTypes>(saveTo, s_minorCivTypes);
	saveTo << s_minorNationCivs;
	saveTo << s_dummyvalue;
	saveTo << s_multiplayerOptions;
	saveTo << s_netIDs;
	writeNicknames(saveTo);
	saveTo << s_numVictoryInfos;
	saveTo << s_pitBossTurnTime;
	saveTo << s_playableCivs;
	CvInfosSerializationHelper::WriteTypeArray<PlayerColorTypes>(saveTo, s_playerColors);
	saveTo << s_privateGame;
	saveTo << s_quickCombat;
	saveTo << s_quickCombatDefault;
	saveTo << s_quickHandicap;
	saveTo << s_quickstart;
	saveTo << s_randomWorldSize;
	saveTo << s_randomMapScript;
	saveTo << s_readyPlayers;
	saveTo << s_seaLevel;
	saveTo << s_seaLevelInfo;
	saveTo << s_dummyvalue2;
	saveTo << s_slotClaims;
	writeSlotStatus(saveTo);
	saveTo << s_smtpHost;
	saveTo << s_syncRandomSeed;
	saveTo << s_targetScore;
	saveTo << s_teamTypes;
	saveTo << s_transferredMap;
	saveTo << s_turnTimer;
	saveTo << s_turnTimerType;
	saveTo << s_bCityScreenBlocked;
	saveTo << s_victories;
	saveTo << s_whiteFlags;
	saveTo << s_worldInfo;
	saveTo << s_worldSize;
	saveTo << s_GameOptions;
	saveTo << s_MapOptions;
	saveTo << s_versionString;
	saveTo << s_turnNotifySteamInvite;
	saveTo << s_turnNotifyEmail;
	saveTo << s_turnNotifyEmailAddress;
}

void write(FDataStream& saveTo)
{
	// header needs to include slot setup
	saveSlotHints(saveTo);
	writeArchive(saveTo);
}

// Get the number of slots that have been taken by humans and AIs
int getActiveSlotCount()
{
	return calcActiveSlotCount(s_slotStatus, s_slotClaims);
}

// Return true if the DLC is allowed in the game
bool isDLCAllowed(const GUID& kDLCID)
{
	for(PackageIDList::const_iterator itr = s_AllowedDLC.begin(); itr != s_AllowedDLC.end(); ++itr)
	{
		if(memcmp(&kDLCID, &(*itr), sizeof(GUID)) == 0)
		{
			return true;
		}
	}

	return false;
}

// Set whether or not the DLC is allowed in the game
void setDLCAllowed(const GUID& kDLCID, bool bState)
{
	if(bState)
	{
		if(!isDLCAllowed(kDLCID))
			s_AllowedDLC.push_back(kDLCID);
	}
	else
	{
		for(PackageIDList::const_iterator itr = s_AllowedDLC.begin(); itr != s_AllowedDLC.end(); ++itr)
		{
			if(memcmp(&kDLCID, &(*itr), sizeof(GUID)) == 0)
			{
				s_AllowedDLC.erase(itr);
				break;
			}
		}
	}
}

// Clear all DLC from being the allowed list
void clearDLCAllowed()
{
	s_AllowedDLC.clear();
}

// Return the number of DLCs allowed
uint getDLCAllowedCount()
{
	return s_AllowedDLC.size();
}

// Return the list of DLCs allowed
const PackageIDList& getDLCAllowed()
{
	return s_AllowedDLC;
}

// Return the list of DLCs available for a player
const PackageIDList& getDLCAvailable(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_DLCPackagesAvailable[p];
	return s_emptyGUIDList;
}

// Set the list of DLCs available to a player
void setDLCAvailable(PlayerTypes p, const PackageIDList& kList)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_DLCPackagesAvailable[p] = kList;
}

bool isDLCAvailable(PlayerTypes p, const GUID& kDLCID)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		const PackageIDList& kList = s_DLCPackagesAvailable[p];
		for(PackageIDList::const_iterator itr = kList.begin(); itr != kList.end(); ++itr)
		{
			if(*itr == kDLCID)
				return true;
		}
	}

	return false;
}

// Clear the list of DLCs available for a player
void clearDLCAvailable(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		s_DLCPackagesAvailable[p].clear();
}

// Get the unique key value for the civilization
const CvString& civilizationKey(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
		return s_civilizationKeys[p];
	static const CvString empty = "";
	return empty;
}

// Set the unique key value for the civilization
void setCivilizationKey(PlayerTypes p, const CvString& szKey)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_civilizationKeys[p] = szKey;

		// Check to see if this is available, if not, set the index to NO_CIVILIZATION and the package ID to invalid
		// The key will stay valid
		bool bFailed = true;
		if(szKey.length() > 0)
		{
			// During the pre-game, we can't be sure the cached *Infos are current, so query the database
			Database::Connection* pDB = GC.GetGameDatabase();
			if(pDB)
			{
				Database::Results kResults;
				if(pDB->Execute(kResults, "SELECT ID, PackageID, Playable from Civilizations where Type = ? LIMIT 1"))
				{
					kResults.Bind(1, szKey.c_str());
					if(kResults.Step())
					{
						s_civilizations.setAt(p, (CivilizationTypes)kResults.GetInt(0));
						if(!ExtractGUID(kResults.GetText(1), s_civilizationPackageID[p]))
							ClearGUID(s_civilizationPackageID[p]);
						s_civilizationKeysAvailable[p] = true;
						s_civilizationKeysPlayable[p] = kResults.GetBool(2);
						bFailed = false;
					}
				}
			}
		}

		if(bFailed)
		{
			s_civilizations.setAt(p, NO_CIVILIZATION);
			ClearGUID(s_civilizationPackageID[p]);
			s_civilizationKeysAvailable[p] = (szKey.length() == 0);	// If the key was empty, then it is the 'random' state so it is available
			s_civilizationKeysPlayable[p] = s_civilizationKeysAvailable[p];
		}
	}
}

// Return true if the key for the players civilization is available on this machine
bool civilizationKeyAvailable(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		return s_civilizationKeysAvailable[p];
	}

	return false;
}

// Get the package ID the player's civilization is from.  If empty, it comes from the base installation.
const GUID& civilizationKeyPackageID(PlayerTypes p)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		return s_civilizationPackageID[p];
	}

	return s_emptyGUID;
}

// Set the unique key value for the civilization's package.
void setCivilizationKeyPackageID(PlayerTypes p, const GUID& kKey)
{
	if(p >= 0 && p < MAX_PLAYERS)
	{
		s_civilizationPackageID[p] = kKey;
	}
}

// Can the player be readied?
bool canReadyLocalPlayer()
{
	for(int itr = 0; itr < MAX_PLAYERS; ++itr)
	{
		if(slotStatus((PlayerTypes)itr) != SS_CLOSED && !CvPreGame::civilizationKeyAvailable((PlayerTypes)itr))
			return false;
	}

	// Also check to see if they have the requested map script
	if (mapScriptName().GetLength() > 0)
	{
		if (!FFILESYSTEM.Exist(mapScriptName()))
			return false;
	}
	return true;
}
}
