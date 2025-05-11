------------------------------------------------- 
-- Staging Room Screen
-------------------------------------------------
-- edit:
--     Duel Mode
--     Community Remarks
--     Ingame Civ Drafter
--     Emote Picker Menu
-- for EUI and vanilla UI
-------------------------------------------------
include( "IconSupport" );
include( "SupportFunctions"  );
include( "InstanceManager" );
include( "UniqueBonuses" );
include( "MPGameOptions" );
include( "TurnStatusBehavior" ); -- for turn status button behavior

-------------------------------------------------
-- Globals
-------------------------------------------------
local m_SlotInstances = {};
local g_ChatInstances = {};
-- Community Remarks START
local g_CommunityRemarks = {};
local g_MMRoleTypes = {};
-- Community Remarks END

-- Ingame Civ Drafter START
g_DraftProgress = 'DRAFT_PROGRESS_DEFAULT';
g_DraftBansTable = {}
g_DraftResultInstances = { Bans = {}, BansD = {}, BansT = {}, Picks = {}, PicksD = {}, PicksB = {}, PicksBD = {}, PicksT = {} };
g_DraftCivInstances = {};
g_SelectedCivs = {};
g_BannedCivs = {};
g_PlayerEntries = {};
g_DummyInstancies = {};
g_CivEntries = {};
g_DraftSettings = {
	TournamentMode = PreGame.GetGameOption('GAMEOPTION_TOURNAMENT_MODE'),
	WorldSize = PreGame.GetWorldSize(),
}

numBans = PreGame.GetGameOption("GAMEOPTION_TOURNAMENT_MODE") > 0 and 1 or 2;
minBans = PreGame.GetGameOption("GAMEOPTION_TOURNAMENT_MODE") > 0 and 1 or 0;
numPicks = 3;
-- Ingame Civ Drafter END

local g_AdvancedOptionIM = InstanceManager:new( "GameOption", "Text", Controls.AdvancedOptions );
local g_AdvancedOptionsList = {};

local m_HostID;
local m_bIsHost;

local m_PlayerNames = {};
local m_bLaunchReady = false;
local m_bTeamsValid = false;

local m_bEditOptions = false;

local g_fCountdownTimer = -1;  --Start game countdown timer.  Set to -1 when not in use.

-- slot type pulldown options for the local player
local g_localSlotTypeOptions = { "TXT_KEY_SLOTTYPE_OPEN", 
																	"TXT_KEY_SLOTTYPE_OBSERVER" }

-- slot type pulldown options for non-local players
local g_slotTypeOptions = { "TXT_KEY_SLOTTYPE_OPEN", 
														"TXT_KEY_SLOTTYPE_HUMANREQ", 
														"TXT_KEY_SLOTTYPE_AI",
														"TXT_KEY_SLOTTYPE_OBSERVER",
														"TXT_KEY_SLOTTYPE_CLOSED" }
														
-- Associates an int value with our slotTypes so that we can index them in different pulldowns  											
local g_slotTypeData = {};
g_slotTypeData["TXT_KEY_SLOTTYPE_OPEN"]			= { tooltip = "TXT_KEY_SLOTTYPE_OPEN_TT",			index=0 };
g_slotTypeData["TXT_KEY_SLOTTYPE_HUMANREQ"]	= { tooltip = "TXT_KEY_SLOTTYPE_HUMANREQ_TT", index=1 };
g_slotTypeData["TXT_KEY_PLAYER_TYPE_HUMAN"]	= { tooltip = "TXT_KEY_SLOTTYPE_HUMAN_TT",		index=2 };
g_slotTypeData["TXT_KEY_SLOTTYPE_AI"]				= { tooltip = "TXT_KEY_SLOTTYPE_AI_TT",				index=3 };
g_slotTypeData["TXT_KEY_SLOTTYPE_OBSERVER"]	= { tooltip = "TXT_KEY_SLOTTYPE_OBSERVER_TT", index=4 };
g_slotTypeData["TXT_KEY_SLOTTYPE_CLOSED"]		= { tooltip = "TXT_KEY_SLOTTYPE_CLOSED_TT",		index=5 };		

local hoursStr = Locale.ConvertTextKey( "TXT_KEY_HOURS" );
local secondsStr = Locale.ConvertTextKey( "TXT_KEY_SECONDS" );	
local PlayerConnectedStr = Locale.ConvertTextKey( "TXT_KEY_MP_PLAYER_CONNECTED" );
local PlayerConnectingStr = Locale.ConvertTextKey( "TXT_KEY_MP_PLAYER_CONNECTING" );
local PlayerNotConnectedStr = Locale.ConvertTextKey( "TXT_KEY_MP_PLAYER_NOTCONNECTED" );							
														
-------------------------------------------------
-- Determine if the screen is for the dedicated server ingame screen										
-------------------------------------------------
function IsInGameScreen()
	if(	PreGame.GameStarted() 
			and Matchmaking.IsHost() 
			and PreGame.GetSlotStatus(Matchmaking.GetLocalID()) == SlotStatus.SS_OBSERVER ) then
			return true;
	end
	
	return false;
end

-------------------------------------------------
-- retrieve player names
-------------------------------------------------
function BuildPlayerNames()
    local playerList = Matchmaking.GetPlayerList();
    
    if( playerList ~= nil ) then
        m_PlayerNames = {};
        
        for i = 1, #playerList do
            m_PlayerNames[ playerList[i].playerID ] = playerList[i].playerName;
        end
    end
end


-------------------------------------------------
-------------------------------------------------
function OnEditHost()
    UIManager:PushModal( Controls.SetCivNames );
	LuaEvents.SetCivNameEditSlot(0);
end
Controls.LocalEditButton:RegisterCallback( Mouse.eLClick, OnEditHost );

-------------------------------------------------
-------------------------------------------------
function ShowHideExitButton()
	local bShow = IsInGameScreen();
	Controls.ExitButton:SetHide( not bShow );
end

-------------------------------------------------
-------------------------------------------------
function OnExitGame()
	Events.UserRequestClose();
end
Controls.ExitButton:RegisterCallback( Mouse.eLClick, OnExitGame );

-------------------------------------------------
-------------------------------------------------
function ShowHideBackButton()
	local bShow = not IsInGameScreen();
	Controls.BackButton:SetHide( not bShow );
end

-------------------------------------------------
-------------------------------------------------
function ShowHideInviteButton()
	local bShow = PreGame.IsInternetGame() and not Network.IsDedicatedServer();
	Controls.InviteButton:SetHide( not bShow );
end
-------------------------------------------------
-------------------------------------------------
function OnInviteButton()
    Steam.ActivateInviteOverlay();
end
Controls.InviteButton:RegisterCallback( Mouse.eLClick, OnInviteButton );

-------------------------------------------------
-------------------------------------------------
function ShowHideSaveButton()
	local bDisable = g_fCountdownTimer ~= -1; -- Disable the save game button while the countdown is active.
	local bShow = Matchmaking.IsHost();
	Controls.SaveButton:SetHide(not bShow);
  if( bDisable ) then
		Controls.SaveButton:SetDisabled( true );
		Controls.SaveButton:SetAlpha( 0.5 );
	else
		Controls.SaveButton:SetDisabled( false );
		Controls.SaveButton:SetAlpha( 1.0 );
  end
  
  -- Only show the game configuration tooltip if we'd be saving the game configuration vs. an actual game save.
  if(PreGame.GameStarted()) then 
		Controls.SaveButton:LocalizeAndSetToolTip( "" );
  else
		Controls.SaveButton:LocalizeAndSetToolTip( "TXT_KEY_SAVE_GAME_CONFIGURATION_TT" );
	end
end
-------------------------------------------------
-------------------------------------------------
function OnSaveButton()
    UIManager:QueuePopup( Controls.SaveMenu, PopupPriority.SaveMenu );
end
Controls.SaveButton:RegisterCallback( Mouse.eLClick, OnSaveButton );

-------------------------------------------------
-------------------------------------------------
function ShowHideStrategicViewButton()
	local bShow = IsInGameScreen();
	Controls.StrategicViewButton:SetHide( not bShow );
end
-------------------------------------------------
-------------------------------------------------      
function OnStrategicView()
	local bIsObserver = PreGame.GetSlotStatus( Game.GetActivePlayer() ) == SlotStatus.SS_OBSERVER;
	if (bIsObserver) then
		-- Observer gets to toggle the world view completely off.
		local eViewType = GetGameViewRenderType();
		if (eViewType == GameViewTypes.GAMEVIEW_NONE) then
			SetGameViewRenderType(GameViewTypes.GAMEVIEW_STANDARD);			
		else
			if (eViewType == GameViewTypes.GAMEVIEW_STANDARD) then
				SetGameViewRenderType(GameViewTypes.GAMEVIEW_STRATEGIC);
			else
				SetGameViewRenderType(GameViewTypes.GAMEVIEW_NONE);
			end
		end
	else
		ToggleStrategicView();
	end		
end
Controls.StrategicViewButton:RegisterCallback( Mouse.eLClick, OnStrategicView );

-------------------------------------------------
-------------------------------------------------
function OnCancel()
	Controls.RemoveButton:SetHide(true);

	PreGame.SetLeaderName( Matchmaking.GetLocalID(), "" );
	PreGame.SetCivilizationDescription( Matchmaking.GetLocalID(), "" );
	PreGame.SetCivilizationShortDescription( Matchmaking.GetLocalID(), "" );
	PreGame.SetCivilizationAdjective( Matchmaking.GetLocalID(), "" );
	
	local civIndex = PreGame.GetCivilization( Matchmaking.GetLocalID() );
    if( civIndex ~= -1 ) then
        civ = GameInfo.Civilizations[ civIndex ];

        -- Use the Civilization_Leaders table to cross reference from this civ to the Leaders table
        local leader = GameInfo.Leaders[GameInfo.Civilization_Leaders( "CivilizationType = '" .. civ.Type .. "'" )().LeaderheadType];
		local leaderDescription = leader.Description;
		
		PlayerLeader = Locale.ConvertTextKey( leaderDescription );
		PlayerCiv = Locale.ConvertTextKey( civ.ShortDescription );
		Controls.Title:SetText( Locale.ConvertTextKey( "TXT_KEY_RANDOM_LEADER_CIV", Locale.ConvertTextKey( leaderDescription ), Locale.ConvertTextKey( civ.ShortDescription ) ) );
	else
		PlayerLeader = Locale.ConvertTextKey( "TXT_KEY_RANDOM_LEADER" );
        PlayerCiv = Locale.ConvertTextKey( "TXT_KEY_RANDOM_CIV" );
        Controls.Title:SetText( Locale.ConvertTextKey( "TXT_KEY_RANDOM_LEADER_CIV", PlayerLeader, PlayerCiv ) );

	end
end
Controls.RemoveButton:RegisterCallback( Mouse.eLClick, OnCancel );


-------------------------------------------------
-- Context OnUpdate
-------------------------------------------------
function OnUpdate( fDTime )
	-- OnUpdate only runs when the game start countdown is ticking down.
	g_fCountdownTimer = g_fCountdownTimer - fDTime;
	if( not Network.IsEveryoneConnected() ) then
		-- not all players are connected anymore.  This is probably due to a player join in progress.
		StopCountdown();
	elseif( g_fCountdownTimer <= 0 ) then
			-- Timer elapsed, launch the game if we're the host.
	    if(m_bIsHost) then
				LaunchGame();
			end
			
			StopCountdown();
	else
		local intTime = math.floor(g_fCountdownTimer);
		local countdownString = Locale.ConvertTextKey("TXT_KEY_GAMESTART_COUNTDOWN_FORMAT", intTime );
		Controls.CountdownButton:SetHide(false);
		Controls.CountdownButton:SetText( countdownString );
	end
end

-------------------------------------------------
-- Start Game Launch Countdown
-------------------------------------------------
function StartCountdown()
	g_fCountdownTimer = 10;
	ContextPtr:SetUpdate( OnUpdate );
end

-------------------------------------------------
-- Stop Game Launch Countdown
-------------------------------------------------
function StopCountdown()
	Controls.CountdownButton:SetHide(true);
	g_fCountdownTimer = -1;
	ContextPtr:ClearUpdate();
end


-------------------------------------------------
-- Launch Game
-------------------------------------------------
function LaunchGame()

	if (PreGame.IsHotSeatGame()) then
		-- In case they changed the DLC.  This won't do anything if it is already setup properly.
		local prevCursor = UIManager:SetUICursor( 1 );
		Modding.ActivateAllowedDLC();
		UIManager:SetUICursor( prevCursor );
	end
	Matchmaking.LaunchMultiplayerGame();

end
Controls.LaunchButton:RegisterCallback( Mouse.eLClick, LaunchGame );

-------------------------------------------------
-- Event Handler:  Hot Join Events
-------------------------------------------------
function OnHotJoinStarted()
	-- Display hot joining popup if we're a hot joiner.
	if(Network.IsPlayerHotJoining(Matchmaking.GetLocalID())) then
		Controls.HotJoinPopup:SetHide(false);
	end
end
Events.MultiplayerHotJoinStarted.Add(OnHotJoinStarted);

function OnHotJoinCompleted()
	-- Remove hot join popup on completion.
	Controls.HotJoinPopup:SetHide(true);
	
	-- The hot joiner's network connection status needs to be updated.
	RefreshPlayerList();
end
Events.MultiplayerHotJoinCompleted.Add(OnHotJoinCompleted);

-------------------------------------------------
-- Event Handler: MultiplayerGameLaunched
-------------------------------------------------
function OnGameLaunched()

	UIManager:DequeuePopup( ContextPtr );

end
Events.MultiplayerGameLaunched.Add( OnGameLaunched );


-------------------------------------------------
-------------------------------------------------
function GetPlayerIDBySelectionIndex(selectionIndex)
	if(selectionIndex == 0) then
		return Matchmaking.GetLocalID();
	end

	return m_SlotInstances[selectionIndex].playerID;
end


-------------------------------------------------
-------------------------------------------------
function CivSelected( selectionIndex, civID )
	local playerID = GetPlayerIDBySelectionIndex(selectionIndex);
	if( playerID >= 0 ) then
		PreGame.SetCivilization( playerID, civID );
		Network.BroadcastPlayerInfo();
		UpdateDisplay();
	end
end


-------------------------------------------------
-------------------------------------------------
function InviteSelected( selectionIndex, playerChoiceID )

	local slotInstance = m_SlotInstances[ selectionIndex ];

	if slotInstance then

		if ( playerChoiceID == -1 ) then -- AI

    		slotInstance.InvitePulldown:GetButton():LocalizeAndSetText( "TXT_KEY_AI_NICKNAME" );

		else -- TODO: Send Invite and Lock Slot

			slotInstance.InvitePulldown:GetButton():SetText( Locale.ConvertTextKey("TXT_KEY_WAITING_FOR_INVITE_RESPONSE", "TEMP" ) );
--			slotInstance.InvitePulldown:GetButton():SetText( "Waiting for Player..." );

		end

	end
end


-------------------------------------------------
-------------------------------------------------
function OnKickPlayer( selectionIndex )
	local playerID = GetPlayerIDBySelectionIndex(selectionIndex);
	UIManager:PushModal(Controls.ConfirmKick, true);	
	local playerName = m_SlotInstances[selectionIndex].PlayerNameLabel:GetText();
	LuaEvents.SetKickPlayer(playerID, playerName);
end

-------------------------------------------------
-------------------------------------------------
function OnEditPlayer( selectionIndex )
	local playerID = GetPlayerIDBySelectionIndex(selectionIndex);
	UIManager:PushModal(Controls.SetCivNames);
	LuaEvents.SetCivNameEditSlot(playerID);
	UpdateDisplay();
end

-- NEW
-------------------------------------------------
-------------------------------------------------

function IsHasRemapToken( playerID )
	if ( PreGame.GetGameOption("GAMEOPTION_ENABLE_REMAP_VOTE") > 0 ) then
		local tokens = PreGame.GetGameOption("GAMEOPTION_REMAP_VOTE_TOKENS");
		return math.floor(tokens / 2 ^ playerID) % 2 > 0
	end
	return false
end

function OnRemapVotePlayerToken( v1, v2, cb )
	if Matchmaking.IsHost() then
		print(v1,v2,cb:GetVoid1())
		local playerID = GetPlayerIDBySelectionIndex(cb:GetVoid1())
		if (playerID < 0 or playerID > 134217727) then
    	    print('WARNING OnRemapVotePlayerToken: id out of bounds');
    	else
			local bValue = cb:IsChecked();
			local product = (2 ^ 29) + (playerID * 2) + (bValue and 1 or 0);
			PreGame.SetLeaderKey( product, '' );
		end
		Network.BroadcastGameSettings();
	end	
end
Controls.RemapTokenCheck:RegisterCallback( Mouse.eLClick, OnRemapVotePlayerToken );
-------------------------------------------------
-------------------------------------------------
function OnSwapPlayer( selectionIndex )
	local playerID = GetPlayerIDBySelectionIndex(selectionIndex);
	Network.SetPlayerDesiredSlot(playerID);
end

-------------------------------------------------
-------------------------------------------------
function OnReadyCheck( bChecked )
	PreGame.SetReady( Matchmaking.GetLocalID(), bChecked );
	Network.BroadcastPlayerInfo();
	UpdateDisplay();
	CheckGameAutoStart();	
	ShowHideSaveButton();	
end
Controls.LocalReadyCheck:RegisterCheckHandler( OnReadyCheck );


-------------------------------------------------
-------------------------------------------------
function OnSelectTeam( selectionIndex, playerChoiceID )
	local playerID = GetPlayerIDBySelectionIndex(selectionIndex);
	PreGame.SetTeam(playerID, playerChoiceID);
	local slotInstance = m_SlotInstances[selectionIndex];
	local teamID = playerChoiceID + 1; -- Real programmers count from zero.
	if( slotInstance ~= nil ) then
		slotInstance.TeamLabel:LocalizeAndSetText( "TXT_KEY_MULTIPLAYER_DEFAULT_TEAM_NAME", teamID );
	else
		Controls.TeamLabel:LocalizeAndSetText( "TXT_KEY_MULTIPLAYER_DEFAULT_TEAM_NAME", teamID );
	end
	Network.BroadcastPlayerInfo();
	
	DoCheckTeams();
end


--------------------------------------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------------------------------------
function DoCheckTeams()

    m_bTeamsValid = false;
    local teamTest = -1;
    
    -- Find the team of the first valid player.  We can't simply use the host's team because they could be an observer.
    for i = 0, GameDefines.MAX_MAJOR_CIVS do
			if( PreGame.GetSlotStatus( i ) == SlotStatus.SS_COMPUTER or PreGame.GetSlotStatus( i ) == SlotStatus.SS_TAKEN ) then
				teamTest = PreGame.GetTeam( i );
        break;
    	end
    end
    
    for i = 0, GameDefines.MAX_MAJOR_CIVS do
        if( PreGame.GetSlotStatus( i ) == SlotStatus.SS_COMPUTER or PreGame.GetSlotStatus( i ) == SlotStatus.SS_TAKEN ) then
        	if( PreGame.GetTeam( i ) ~= teamTest ) then
        	    m_bTeamsValid = true;
        	    break;
        	end
    	end
    end
    
end


-------------------------------------------------
-------------------------------------------------
function OnHandicapTeam( selectionIndex, handicap )
	local listingEntry = m_SlotInstances[selectionIndex];

	if(listingEntry ~= nil) then
		listingEntry.HandicapLabel:LocalizeAndSetText( GameInfo.HandicapInfos[handicap].Description );
	else
		Controls.HandicapLabel:LocalizeAndSetText( GameInfo.HandicapInfos[handicap].Description );
	end
	local playerID = GetPlayerIDBySelectionIndex(selectionIndex);
	PreGame.SetHandicap(playerID, handicap);
	Network.BroadcastPlayerInfo();
end

-------------------------------------------------
-------------------------------------------------
function OnPlayerName( selectionIndex, id )
	local playerID = GetPlayerIDBySelectionIndex(selectionIndex);
	if (id == 0) then 
		PreGame.SetSlotStatus( playerID, SlotStatus.SS_COMPUTER );
		PreGame.SetHandicap( playerID, 1 );
		PreGame.SetNickName( playerID, "" );
	else
		PreGame.SetSlotStatus( playerID, SlotStatus.SS_TAKEN );
		if (PreGame.GetNickName(playerID) == "") then
			PreGame.SetNickName( playerID, Locale.ConvertTextKey( "TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME", playerID + 1 ) );
		end
	end
	Network.BroadcastPlayerInfo();

end

-------------------------------------------------
-------------------------------------------------
function SetSlotToHuman(playerID)
	-- Sets the given playerID slot to be human. Assumes that slot hasn't already been done so.
	PreGame.SetSlotStatus( playerID, SlotStatus.SS_TAKEN );
	PreGame.SetHandicap( playerID, 3 );
	if (PreGame.GetNickName(playerID) == "") then
		PreGame.SetNickName( playerID, Locale.ConvertTextKey( "TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME", playerID + 1 ) );
	end
end

function OnSlotType( playerID, id )
	-- NOTE: Slot type pulldowns store the slot's playerID rather than the selection Index in their voids.
	--print("OnSlotType ID:" .. id);
	--print("	Player ID:" .. playerID);
	local resetReadyStatus = false; -- In most cases, changing a slottype resets the player's ready status.

	if(id == g_slotTypeData["TXT_KEY_SLOTTYPE_OPEN"].index) then
		-- TXT_KEY_SLOTTYPE_OPEN - Open human player slot.
		if(not Network.IsPlayerConnected(playerID)) then
			-- Can't open a slot occupied by a human.
			PreGame.SetSlotStatus( playerID, SlotStatus.SS_OPEN );
			PreGame.SetSlotClaim( playerID, SlotClaim.SLOTCLAIM_ASSIGNED );		
			resetReadyStatus = true;
		end
	elseif(id == g_slotTypeData["TXT_KEY_PLAYER_TYPE_HUMAN"].index) then
		-- TXT_KEY_PLAYER_TYPE_HUMAN
		if(PreGame.GetSlotStatus( playerID ) ~= SlotStatus.SS_TAKEN) then
			SetSlotToHuman(playerID);
			resetReadyStatus = true;
		end
	elseif(id == g_slotTypeData["TXT_KEY_SLOTTYPE_HUMANREQ"].index) then
		-- TXT_KEY_SLOTTYPE_HUMANREQ 
		if(not Network.IsPlayerConnected(playerID)) then
			-- Don't open the slot if someone is already occupying it.
			PreGame.SetSlotStatus( playerID, SlotStatus.SS_OPEN );	
			resetReadyStatus = true;
		elseif(Network.IsPlayerConnected(playerID) and PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_OBSERVER) then
			-- Setting human required on an human occupied observer slot switches them to normal player mode.
			SetSlotToHuman(playerID);
			resetReadyStatus = true;
		end
		PreGame.SetSlotClaim( playerID, SlotClaim.SLOTCLAIM_RESERVED );
	elseif(id == g_slotTypeData["TXT_KEY_SLOTTYPE_AI"].index) then
		-- TXT_KEY_SLOTTYPE_AI
		local bCanEnableAISlots = PreGame.GetMultiplayerAIEnabled();
		if(bCanEnableAISlots and not Network.IsPlayerConnected(playerID)) then
			-- only switch to AI if AI are enabled in multiplayer.
			PreGame.SetSlotStatus( playerID, SlotStatus.SS_COMPUTER );
    	PreGame.SetHandicap( playerID, 1 );
    	resetReadyStatus = true;
			if ( PreGame.IsHotSeatGame() ) then
				-- Reset so the player can force a rebuild
				PreGame.SetNickName( playerID, "" );
				PreGame.SetLeaderName( playerID, "" );
				PreGame.SetCivilizationDescription( playerID, "" );
				PreGame.SetCivilizationShortDescription( playerID, "" );
				PreGame.SetCivilizationAdjective( playerID, "" );
			end
		end
	elseif(id == g_slotTypeData["TXT_KEY_SLOTTYPE_OBSERVER"].index) then
		-- TXT_KEY_SLOTTYPE_OBSERVER
		PreGame.SetSlotStatus( playerID, SlotStatus.SS_OBSERVER );
		resetReadyStatus = true;
	elseif(id == g_slotTypeData["TXT_KEY_SLOTTYPE_CLOSED"].index) then
		-- TXT_KEY_SLOTTYPE_CLOSED
		if(not Network.IsPlayerConnected(playerID)) then
			PreGame.SetSlotStatus( playerID, SlotStatus.SS_CLOSED );
			PreGame.SetSlotClaim( playerID, SlotClaim.SLOTCLAIM_ASSIGNED );
			resetReadyStatus = true;
		end
	end
	
	if(resetReadyStatus) then
		PreGame.SetReady(playerID, false);
	end	

	UpdateDisplay();
	Network.BroadcastPlayerInfo();
end


-------------------------------------------------
-- Refresh Player List
-------------------------------------------------
function RefreshPlayerList()
    m_HostID  = Matchmaking.GetHostID();
    m_bIsHost = Matchmaking.IsHost();
	m_bLaunchReady = true;
	
	-- Get the Current Player List
	local playerTable = Matchmaking.GetPlayerList();
	
	for i = 1, #m_SlotInstances do
		m_SlotInstances[i].Root:SetHide( true );
	end
	-- Ingame Civ Drafter START
	for i in next, g_PlayerEntries do
		g_PlayerEntries[i].UpdatePending = true
	end
	-- Ingame Civ Drafter END

	-- Display Each Player
	if( playerTable ) then
		for i, playerInfo in ipairs( playerTable ) do
		  local playerID = playerInfo.playerID;
		    
		  m_SlotInstances[i].playerID = playerTable[i].playerID;

			if( playerInfo.playerID == Matchmaking.GetLocalID() ) then
				UpdateLocalPlayer( playerInfo );
			else	
				UpdatePlayer( m_SlotInstances[ i ], playerInfo );
			end
			-- Ingame Civ Drafter START
			local bIsHuman  = (PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_TAKEN ) or (PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_OPEN );
			g_PlayerEntries[playerID].statusInstance.PlayerName:LocalizeAndSetText( GetSlotTypeString(playerID) );
			--print('pid', playerID, playerInfo.playerName)
			if PreGame.GetSlotClaim( playerID ) ~= SlotClaim.SLOTCLAIM_UNASSIGNED and bIsHuman then
				g_PlayerEntries[playerID].statusInstance.PlayerName:SetText( playerInfo.playerName );
				if g_PlayerEntries[playerID].statusInstance.Root:IsHidden() then
					g_PlayerEntries[playerID].statusInstance.Root:SetHide( false );
					g_DummyInstancies[playerID].Root:SetHide( false );
				end
			else
				if not g_PlayerEntries[playerID].statusInstance.Root:IsHidden() then
					g_PlayerEntries[playerID].statusInstance.Root:SetHide( true );
					g_DummyInstancies[playerID].Root:SetHide( true );
				end
			end
			Controls.DraftPlayersStatusStack:CalculateSize();
			Controls.DraftPlayersStatusStack:ReprocessAnchoring();
			Controls.DraftPlayersStatusScrollPanel:CalculateInternalSize();
			local product = 5 * 2 ^ 28;  -- send draft update (local)
			PreGame.SetLeaderKey( product, '' );
			g_PlayerEntries[playerID].UpdatePending = false;
			-- Ingame Civ Drafter END
		end
	end
	for i, en in next, g_PlayerEntries do
		if en.UpdatePending == true and not en.statusInstance.Root:IsHidden() then
			g_PlayerEntries[i].statusInstance.Root:SetHide( true );
			g_DummyInstancies[i].Root:SetHide( true );
			g_PlayerEntries[i].UpdatePending = false;
		end
	end

	Controls.SlotStack:CalculateSize();
	Controls.SlotStack:ReprocessAnchoring();
	Controls.ListingScrollPanel:CalculateInternalSize();
end


-------------------------------------------------
-------------------------------------------------
function UpdatePlayer( slotInstance, playerInfo )

	local bIsPitboss = PreGame.GetGameOption("GAMEOPTION_PITBOSS");
	
	-- Player Listing Entry
	if(slotInstance ~= nil) then
		slotInstance.Root:SetHide( false );

		local playerName = playerInfo.playerName or Locale.ConvertTextKey( "TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME", playerID + 1);
		local playerID   = playerInfo.playerID;

		--------------------------------------------------------------
		-- Handle Civ Info
		
        --slotInstance.Portrait
        
		local civIndex = PreGame.GetCivilization( playerID );
		local activeCivSlot = (PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_COMPUTER 
													or PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_TAKEN);
		if( civIndex ~= -1 and activeCivSlot ) then
			civ = GameInfo.Civilizations[ civIndex ];

			-- Use the Civilization_Leaders table to cross reference from this civ to the Leaders table
			local leader = GameInfo.Leaders[GameInfo.Civilization_Leaders( "CivilizationType = '" .. civ.Type .. "'" )().LeaderheadType];
			local leaderDescription = leader.Description;
			
			slotInstance.CivLabel:LocalizeAndSetText( "TXT_KEY_RANDOM_LEADER_CIV", Locale.ConvertTextKey( leaderDescription ), Locale.ConvertTextKey( civ.ShortDescription ) );
			slotInstance.CivLabel:LocalizeAndSetToolTip( "" );
        
			IconHookup( leader.PortraitIndex, 64, leader.IconAtlas, slotInstance.Portrait );
			-- Ingame Civ Drafter START
			--SimpleCivIconHookup( playerID, 64, slotInstance.Icon);  -- weird civ icon glitch fix
			local thisCivType = PreGame.GetCivilization( playerID );
    		local thisCiv = GameInfo.Civilizations[thisCivType];
			IconHookup( thisCiv.PortraitIndex, 64, thisCiv.IconAtlas, slotInstance.Icon )
			-- Ingame Civ Drafter END
		else   
			if ( not activeCivSlot or PreGame.IsCivilizationKeyAvailable( playerID ) ) then
				--------------------------------------------------------------
				-- Random Civ             
				slotInstance.CivLabel:LocalizeAndSetText( "TXT_KEY_RANDOM_LEADER" );
				slotInstance.CivLabel:LocalizeAndSetToolTip( "" );

				IconHookup( 22, 64, "LEADER_ATLAS", slotInstance.Portrait );
				SimpleCivIconHookup(-1, 64, slotInstance.Icon);
			else
				--------------------------------------------------------------
				-- A player has chosen a civilization we don't have access to
				slotInstance.CivLabel:LocalizeAndSetText( "TXT_KEY_UNAVAILABLE_LEADER" );
				slotInstance.CivLabel:LocalizeAndSetToolTip( "TXT_KEY_UNAVAILABLE_LEADER_HELP", Locale.ConvertTextKey( PreGame.GetCivilizationPackageTextKey( playerID ) ) );

				IconHookup( 22, 64, "LEADER_ATLAS", slotInstance.Portrait );
				SimpleCivIconHookup(-1, 64, slotInstance.Icon);
			end				
		end

		if (PreGame.CanReadyLocalPlayer()) then
			Controls.LocalReadyCheck:LocalizeAndSetToolTip( "TXT_KEY_MP_READY_CHECK" );				
		else
			Controls.LocalReadyCheck:LocalizeAndSetToolTip( "TXT_KEY_MP_READY_CHECK_UNAVAILABLE_DATA_HELP" );				
		end

		--------------------------------------------------------------
		-- Set Team
		local teamID = PreGame.GetTeam(playerID) + 1; -- Real programmers count from zero.
		slotInstance.TeamLabel:LocalizeAndSetText( "TXT_KEY_MULTIPLAYER_DEFAULT_TEAM_NAME", teamID );
		--------------------------------------------------------------
		
		--------------------------------------------------------------
		-- Set Handicap
		local tHandicap = GameInfo.HandicapInfos( "ID = '" .. PreGame.GetHandicap( playerID ) .. "'" )();
		if (tHandicap ~= nil) then
			slotInstance.HandicapLabel:LocalizeAndSetText( tHandicap.Description );
		end
		--------------------------------------------------------------
		
		--------------------------------------------------------------
		-- Update Turn Status
		UpdateTurnStatusForPlayerID(playerID);
		--------------------------------------------------------------

		--------------------------------------------------------------
		-- Set Slot Type
		local slotTypeStr = GetSlotTypeString(playerID);
		slotInstance.SlotTypeLabel:LocalizeAndSetText( slotTypeStr );
		slotInstance.SlotTypeLabel:LocalizeAndSetToolTip( g_slotTypeData[slotTypeStr].tooltip );		
		--------------------------------------------------------------
		
		-------------------------------------------------------------
		-- Refresh slottype pulldown
		PopulateSlotTypePulldown( slotInstance.SlotTypePulldown, playerID, g_slotTypeOptions );
		-------------------------------------------------------------
	
        local bIsHuman  = (PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_TAKEN);
        local bIsLocked = (PreGame.GetSlotClaim( playerID ) == SlotClaim.SLOTCLAIM_RESERVED) or
                          (PreGame.GetSlotClaim( playerID ) == SlotClaim.SLOTCLAIM_ASSIGNED);
        local bIsClosed = (PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_CLOSED);
        local bIsObserver = PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_OBSERVER;

		-- You can't change this slot's options.
		local bIsDisabled = ( (PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_CLOSED) or 
															(PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_OBSERVER) or
															((PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_OPEN)
																and not (PreGame.GetSlotClaim( playerID ) == SlotClaim.SLOTCLAIM_RESERVED)) );
														
		local bIsEmptyHumanRequiredSlot = (PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_OPEN 
																			 and PreGame.GetSlotClaim( playerID ) == SlotClaim.SLOTCLAIM_RESERVED);
        local bIsHotSeat = PreGame.IsHotSeatGame();
        local bIsReady;
        local bCantChangeCiv;		-- Can't change civilization
        local bCantChangeTeam;	-- can't change teams
        
		local bSlotTypeDisabled;
		local bCantChangeRemapToken = bIsObserver or PreGame.GameStarted() or (PreGame.GetLoadFileName() ~= "");

		-- Community Remarks START
		-- NEW: check if player has any community remarks
		local tstrTooltip = {};
		local netID = GetNetID(playerID);
		if ( not bIsHotSeat and bIsHuman ) then
			if g_CommunityRemarks[netID] then
				table.insert(tstrTooltip, Locale.ConvertTextKey('TXT_KEY_COMMUNITY_REMARKS_HEADLINE'));
				for _,i in ipairs(g_CommunityRemarks[netID]) do
					table.insert(tstrTooltip, i.Tooltip);
				end
				slotInstance.CommunityRemarkSign:SetHide(false);
				slotInstance.CommunityRemarkSign:SetToolTipString(table.concat(tstrTooltip, '[ENDCOLOR][NEWLINE]'));
			end
		else
			slotInstance.CommunityRemarkSign:SetHide(true);
		end
		-- Community Remarks END
		-----------------------------------------------------------

        if( bIsHuman ) then	
			TruncateString(slotInstance.PlayerNameLabel, slotInstance.PlayerNameBox:GetSizeX() - 
										 slotInstance.PlayerNameLabel:GetOffsetX(), playerName); 
			slotInstance.InvitePulldown:GetButton():SetText( playerName );
			
			-- You can only change the slot's civ/team if you're in hotseat mode.
			if( bIsHotSeat ) then 
				bIsReady = PreGame.IsReady( m_HostID ); -- same ready status as host (local player)
				bCantChangeCiv = PreGame.IsReady( m_HostID );
				bCantChangeTeam = PreGame.IsReady( m_HostID );
			else
				bIsReady = PreGame.IsReady( playerID );
				bCantChangeCiv = true;
				bCantChangeTeam = not m_bIsHost or PreGame.IsReady( m_HostID ); -- The host can override human's team selection
			end
			
			bSlotTypeDisabled = not m_bIsHost or PreGame.IsReady( m_HostID ); -- The host can override slot types
        else
			bIsReady = not bIsEmptyHumanRequiredSlot and -- Empty human required slots block game readiness.
									(bIsObserver and (PreGame.IsReady( playerID ) or (not Network.IsPlayerConnected( playerID ) and PreGame.IsReady( m_HostID ))) or -- human observers manually ready up if occupied or ready up with the host if empty.
									(not bIsObserver and PreGame.IsReady( m_HostID )) or -- non-observers share ready status with the host.
									(PreGame.GetLoadFileName() ~= "" and PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_CLOSED)); -- prevent closed player slots from blocking game startup when loading save games -tsmith
            
			if( m_bIsHost ) then
				bSlotTypeDisabled = PreGame.IsReady( m_HostID );
				
				-- Host can't change the team/civ of open/closed slots.
				bCantChangeCiv = bIsDisabled or PreGame.IsReady( m_HostID ) or bIsEmptyHumanRequiredSlot;
				bCantChangeTeam = bIsDisabled or PreGame.IsReady( m_HostID );
			else
				bSlotTypeDisabled = true;
				bCantChangeCiv = true;
				bCantChangeTeam = true;
			end
			
			if(Network.IsPlayerConnected(playerID)) then 
				-- Use the player name if a player is network connected to the game (observers only)
				TruncateString(slotInstance.PlayerNameLabel, slotInstance.PlayerNameBox:GetSizeX() - 
							slotInstance.PlayerNameLabel:GetOffsetX(), playerName); 
				slotInstance.InvitePulldown:GetButton():SetText( playerName );
			else
				-- The default for non-humans is to display the slot type as the player name.
				local playerNameTextKey = GetSlotTypeString(playerID);
				TruncateString(slotInstance.PlayerNameLabel, slotInstance.PlayerNameBox:GetSizeX() - 
												 slotInstance.PlayerNameLabel:GetOffsetX(), Locale.ConvertTextKey(playerNameTextKey)); 
				slotInstance.InvitePulldown:GetButton():LocalizeAndSetText(playerNameTextKey);
			end      
		end
        
		if(PreGame.GameStarted()) then 
			-- Can't change anything after the game has been started.
			bCantChangeCiv = true;
			bCantChangeTeam = true;
			bSlotTypeDisabled = true;
			bCantChangeRemapToken = true;
		end
        
		-- You can't auto launch the game if someone isn't ready.
		if( not bIsReady ) then
			m_bLaunchReady = false;
        end
        
        if( bIsDisabled ) then
        	slotInstance.EnableCheck:SetCheck( false );
        	slotInstance.EnableCheck:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_MP_ENABLE_SLOT" ) );	
    	else
        	slotInstance.EnableCheck:SetCheck( true );
        	slotInstance.EnableCheck:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_MP_DISABLE_SLOT" ) );
    	end

    	if( bIsLocked ) then
        	slotInstance.LockCheck:SetCheck( true );
        	slotInstance.LockCheck:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_MP_UNLOCK_SLOT" ) );
    	else
        	slotInstance.LockCheck:SetCheck( false );
            slotInstance.LockCheck:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_MP_LOCK_SLOT" ) );
        end

        slotInstance.ReadyHighlight:SetHide( not (bIsReady and (bIsObserver or not bIsDisabled)) );


		slotInstance.PlayerNameLabel:SetHide( false );
		slotInstance.InvitePulldown:SetHide( true );
		   
		-- Note: You can't change slot types, civs, or teams after the game has started.   
		slotInstance.CivPulldown:SetHide( bCantChangeCiv or (PreGame.GetLoadFileName() ~= "") or PreGame.GameStarted() );
		slotInstance.TeamPulldown:SetHide( bCantChangeTeam or (PreGame.GetLoadFileName() ~= "") or PreGame.GameStarted() );
		slotInstance.SlotTypePulldown:SetHide( bSlotTypeDisabled or (PreGame.GetLoadFileName() ~= "") or PreGame.GameStarted() );
    
		-- Hide Civ/Team labels for open/closed slots
		slotInstance.TeamLabel:SetHide(bIsDisabled);
		slotInstance.CivLabel:SetHide(bIsDisabled); 

		if ( bIsHuman ) then
			slotInstance.HandicapLabel:SetHide( false );
			slotInstance.HandicapPulldown:SetHide( bCantChangeCiv );
			slotInstance.PingTimeLabel:SetHide( false );
		else
			slotInstance.HandicapLabel:SetHide( true );
			slotInstance.HandicapPulldown:SetHide( true );
			slotInstance.PingTimeLabel:SetHide( true );
		end

        --slotInstance.DisableBlock:SetHide( not bIsDisabled );
        
		if( m_bIsHost ) then
			if( bIsHotSeat ) then
				slotInstance.KickButton:SetHide( true );
				slotInstance.EditButton:SetHide( bIsDisabled or not bIsHuman );				
				slotInstance.RemapTokenCheck:SetHide( PreGame.GetGameOption("GAMEOPTION_ENABLE_REMAP_VOTE") <= 0 or not bIsHuman );
				slotInstance.RemapTokenCheck:SetDisabled( bCantChangeRemapToken );
				slotInstance.RemapTokenCheck:SetCheck( IsHasRemapToken(playerID) );
				--slotInstance.EnableCheck:SetHide( false );
				slotInstance.PingTimeLabel:SetHide( true );
				slotInstance.LockCheck:SetHide( true );				
			else
				--slotInstance.EnableCheck:SetHide( bIsHuman or bCantChangeCiv );
				slotInstance.LockCheck:SetHide( bIsHuman or bCantChangeCiv );
				slotInstance.KickButton:SetHide( not bIsHuman and not Network.IsPlayerConnected(playerID));
				slotInstance.EditButton:SetHide( true );
				slotInstance.RemapTokenCheck:SetHide( PreGame.GetGameOption("GAMEOPTION_ENABLE_REMAP_VOTE") <= 0 or not bIsHuman and not Network.IsPlayerConnected(playerID));
				slotInstance.RemapTokenCheck:SetDisabled( bCantChangeRemapToken );
				slotInstance.RemapTokenCheck:SetCheck( IsHasRemapToken(playerID) );
			end
		else
			--slotInstance.EnableCheck:SetHide( true );
			slotInstance.LockCheck:SetHide( true );
			slotInstance.KickButton:SetHide( true );
			slotInstance.EditButton:SetHide( true );
			slotInstance.RemapTokenCheck:SetHide( PreGame.GetGameOption("GAMEOPTION_ENABLE_REMAP_VOTE") <= 0 or not bIsHuman );
			slotInstance.RemapTokenCheck:SetDisabled( true );
			slotInstance.RemapTokenCheck:SetCheck( IsHasRemapToken(playerID) );
		end
		
		-- Handle swap button highlight
		local highlightSwap = (Network.GetPlayerDesiredSlot(Matchmaking.GetLocalID()) == playerID) -- We want to switch to this slot
			or (Network.GetPlayerDesiredSlot(playerID) == Matchmaking.GetLocalID()); -- They want to switch with us.
		slotInstance.SwapButtonHighAlpha:SetHide( not highlightSwap );
		slotInstance.SwapButton:SetHide( bIsHotSeat or PreGame.IsReady(Matchmaking.GetLocalID()) or (PreGame.GetSlotStatus(playerID) == SlotStatus.SS_CLOSED));  
        
		--TEMP: S.S, hiding lock since it doesn't do anything and too many people believe it's the cause of MP probs.
		slotInstance.LockCheck:SetHide( true );
        
		slotInstance.HostIcon:SetHide( playerID ~= m_HostID );		
        
		-- Player connected indicator
		if(bIsHuman or bIsObserver) then
			slotInstance.ConnectionStatus:SetHide(false);
			if(Network.IsPlayerHotJoining(playerID)) then
				-- Player is hot joining.
				slotInstance.ConnectionStatus:SetTextureOffsetVal(0,32);
				slotInstance.ConnectionStatus:SetToolTipString( PlayerConnectingStr );
			elseif(Network.IsPlayerConnected(playerID)) then
				-- fully connected
				slotInstance.ConnectionStatus:SetTextureOffsetVal(0,0);
				slotInstance.ConnectionStatus:SetToolTipString( PlayerConnectedStr );
			else
				-- Not connected
				slotInstance.ConnectionStatus:SetTextureOffsetVal(0,96);
				slotInstance.ConnectionStatus:SetToolTipString( PlayerNotConnectedStr );		
			end		
		else
			slotInstance.ConnectionStatus:SetHide(true);
		end
        
		if ( not bIsHotSeat and bIsHuman ) then
			UpdatePingTimeLabel( slotInstance.PingTimeLabel, playerID );
		end
	end
end
--print(SlotStatus.SS_OPEN, SlotStatus.SS_COMPUTER, SlotStatus.SS_CLOSED, SlotStatus.SS_TAKEN, SlotStatus.SS_OBSERVER)

----------------------------------------------
function UpdateTurnStatusForPlayerID( iPlayerID )
	if(Players ~= nil) then
		local pPlayer = Players[iPlayerID];
		if(iPlayerID == Matchmaking.GetLocalID()) then
			UpdateTurnStatus(pPlayer, Controls.Icon, Controls.ActiveTurnAnim, m_SlotInstances);
		else
			local slotInstance = nil;
			for iSlot, slotElement in pairs( m_SlotInstances ) do
				local slotPlayerID = GetPlayerIDBySelectionIndex(iSlot);
				if(slotPlayerID == iPlayerID) then
					slotInstance = slotElement;
					break;
				end
			end
			if(slotInstance ~= nil) then -- minor civs don't have slot instances.
				UpdateTurnStatus(pPlayer, slotInstance.Icon, slotInstance.ActiveTurnAnim, m_SlotInstances);
			end
		end
	end
end

-- Here are all the events for which we need to update a given player's turn status.
Events.AIProcessingStartedForPlayer.Add( UpdateTurnStatusForPlayerID );
Events.AIProcessingEndedForPlayer.Add( UpdateTurnStatusForPlayerID );

----------------------------------------------
function UpdateTurnStatusForAll()
	UpdateTurnStatusForPlayerID(Matchmaking.GetLocalID());
	for iSlot, slotInstance in pairs( m_SlotInstances ) do
		UpdateTurnStatusForPlayerID(slotInstance.playerID);
	end
end
Events.NewGameTurn.Add( UpdateTurnStatusForAll );
Events.RemotePlayerTurnEnd.Add( UpdateTurnStatusForAll );

----------------------------------------------



-------------------------------------------------
-------------------------------------------------
function UpdateLocalPlayer( playerInfo )
	--------------------------------------------------------------
	-- Fill In Slot Info
	local playerName = playerInfo.playerName or Locale.ConvertTextKey( "TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME", Matchmaking.GetLocalID() + 1);
	
	Controls.RemoveButton:SetHide( true );
    --------------------------------------------------------------
    -- Get player civ
	local civIndex = PreGame.GetCivilization( Matchmaking.GetLocalID() );
	local activeCivSlot = (PreGame.GetSlotStatus( Matchmaking.GetLocalID() ) == SlotStatus.SS_COMPUTER 
													or PreGame.GetSlotStatus( Matchmaking.GetLocalID() ) == SlotStatus.SS_TAKEN);
	if( civIndex ~= -1 and activeCivSlot ) then
		--------------------------------------------------------------
		-- Selected a Civ
		civ = GameInfo.Civilizations[ civIndex ];

		-- Use the Civilization_Leaders table to cross reference from this civ to the Leaders table
		local leader = GameInfo.Leaders( "Type = '" .. GameInfo.Civilization_Leaders( "CivilizationType = '" .. civ.Type .. "'" )().LeaderheadType .. "'" )();
		local leaderDescription = leader.Description;
		
		Controls.CivLabel:LocalizeAndSetText( "TXT_KEY_RANDOM_LEADER_CIV", Locale.ConvertTextKey( leaderDescription ), Locale.ConvertTextKey( civ.ShortDescription ) );
		
		IconHookup( leader.PortraitIndex, 64, leader.IconAtlas, Controls.Portrait );
		-- Ingame Civ Drafter START
		--SimpleCivIconHookup( Matchmaking.GetLocalID(), 64, Controls.Icon);  -- weird civ icon glitch fix
		local thisCivType = PreGame.GetCivilization( Matchmaking.GetLocalID() );
    	local thisCiv = GameInfo.Civilizations[thisCivType];
		IconHookup( thisCiv.PortraitIndex, 64, thisCiv.IconAtlas, Controls.Icon )
		-- Ingame Civ Drafter END
	else   
		--------------------------------------------------------------
		-- Random Civ             
        
		Controls.CivLabel:LocalizeAndSetText( "TXT_KEY_RANDOM_LEADER" );
		IconHookup( 22, 64, "LEADER_ATLAS", Controls.Portrait );
		SimpleCivIconHookup(-1, 64, Controls.Icon);
	end
	--------------------------------------------------------------
	
	--------------------------------------------------------------
	-- Determine turn completed status.
	UpdateTurnStatusForPlayerID(Matchmaking.GetLocalID());
  --------------------------------------------------------------
	
	--------------------------------------------------------------
	-- Set Team
	local teamID = PreGame.GetTeam(Matchmaking.GetLocalID()) + 1; -- Real programmers count from zero.
	Controls.TeamLabel:LocalizeAndSetText( "TXT_KEY_MULTIPLAYER_DEFAULT_TEAM_NAME", teamID );
	--------------------------------------------------------------
	
	--------------------------------------------------------------
	-- Set Handicap
	Controls.HandicapLabel:LocalizeAndSetText( GameInfo.HandicapInfos( "ID = '" .. PreGame.GetHandicap( Matchmaking.GetLocalID() ) .. "'" )().Description );
	--------------------------------------------------------------
	
	-------------------------------------------------------------
	-- Set Slot Type
	local slotTypeStr = GetSlotTypeString(Matchmaking.GetLocalID());
	Controls.SlotTypeLabel:LocalizeAndSetText( slotTypeStr );
	Controls.SlotTypeLabel:LocalizeAndSetToolTip( g_slotTypeData[slotTypeStr].tooltip );		
	--------------------------------------------------------------
	
	-------------------------------------------------------------
	-- Refresh slottype pulldown
	PopulateSlotTypePulldown( Controls.SlotTypePulldown, Matchmaking.GetLocalID(), g_localSlotTypeOptions );
	-------------------------------------------------------------
	
	local bIsHotSeat = PreGame.IsHotSeatGame();
	local bIsObserver = ( (PreGame.GetSlotStatus( Matchmaking.GetLocalID() ) == SlotStatus.SS_OBSERVER) );
	-----------------------------------------------------------
	-- Ready Status
	local bIsReady = PreGame.IsReady( Matchmaking.GetLocalID() );
	Controls.LocalReadyCheck:SetHide(false);
	if( not bIsReady ) then
		m_bLaunchReady = false;
	end

	-----------------------------------------------------------
	-- Community Remarks START
	-- NEW: check if local player has any community remarks
	local tstrTooltip = {};
	local netID = GetNetID(Matchmaking.GetLocalID());
	if g_CommunityRemarks[netID] then
		table.insert(tstrTooltip, Locale.ConvertTextKey('TXT_KEY_COMMUNITY_REMARKS_HEADLINE'));
		for _,i in ipairs(g_CommunityRemarks[netID]) do
			table.insert(tstrTooltip, i.Tooltip);
		end
		Controls.LocalCommunityRemarkSign:SetHide(false);
		Controls.LocalCommunityRemarkSign:SetToolTipString(table.concat(tstrTooltip, '[ENDCOLOR][NEWLINE]'));
	else
		Controls.LocalCommunityRemarkSign:SetHide(true);
	end
	-- Community Remarks END
	-----------------------------------------------------------
	
	local bCantChangeCiv = bIsReady or (PreGame.GetLoadFileName() ~= "") or bIsObserver or PreGame.GameStarted();
	local bCantChangeTeam = bIsReady or (PreGame.GetLoadFileName() ~= "") or bIsObserver or PreGame.GameStarted();
	local bSlotTypeDisabled = bIsReady or (PreGame.GetLoadFileName() ~= "") or bIsHotSeat or Network.IsDedicatedServer() or PreGame.GameStarted();
	local bCantChangeRemapToken = (PreGame.GetLoadFileName() ~= "") or bIsObserver or PreGame.GameStarted();

	-- Hide Civ/Team/Handicap labels for observers
  Controls.TeamLabel:SetHide(bIsObserver);
	Controls.CivLabel:SetHide(bIsObserver); 
	Controls.HandicapLabel:SetHide(bIsObserver);

	Controls.LocalEditButton:SetHide( not bIsHotSeat );
  Controls.LocalReadyCheck:SetCheck( bIsReady );
  Controls.ReadyHighlight:SetHide( not bIsReady );

	TruncateString(Controls.PlayerNameLabel, Controls.PlayerNameBox:GetSizeX() - 
				   Controls.PlayerNameLabel:GetOffsetX(), playerName); 
	Controls.CivPulldown:SetHide( bCantChangeCiv );
	Controls.TeamPulldown:SetHide( bCantChangeTeam );
	Controls.SlotTypePulldown:SetHide( bSlotTypeDisabled );
	Controls.HandicapPulldown:SetHide( bCantChangeCiv );
  Controls.HostIcon:SetHide( Matchmaking.GetLocalID() ~= m_HostID );
	Controls.RemapTokenCheck:SetHide( PreGame.GetGameOption("GAMEOPTION_ENABLE_REMAP_VOTE") <= 0 );
	Controls.RemapTokenCheck:SetDisabled( bCantChangeRemapToken or Matchmaking.GetLocalID() ~= m_HostID );
	Controls.RemapTokenCheck:SetCheck( IsHasRemapToken(Matchmaking.GetLocalID()) );
	---------------------------------------------------------
	
end

-------------------------------------------------
-- Back Button Handler
-------------------------------------------------
function BackButtonClick()
	if (Matchmaking.IsLaunchingGame()) then
		return;	-- Can't exit while launching.
	end
	HandleExitRequest();
end
Controls.BackButton:RegisterCallback( Mouse.eLClick, BackButtonClick );
Controls.HotJoinCancelButton:RegisterCallback( Mouse.eLClick, BackButtonClick );


-------------------------------------------------
-- Leave the Game
-------------------------------------------------
function HandleExitRequest()
	
	StopCountdown(); -- Make sure there is no countdown going.
	
	local isHost = Matchmaking.IsHost();
	Matchmaking.LeaveMultiplayerGame();
	if ( not PreGame.IsHotSeatGame() ) then
		-- Refresh the lobby as we enter it
		if ( PreGame.IsInternetGame() and not isHost ) then
			Matchmaking.RefreshInternetGameList();
		elseif ( not PreGame.IsInternetGame() and not isHost ) then
			Matchmaking.RefreshLANGameList();
		end
	end
	if PreGame.GetLoadFileName() ~= "" then
		-- If there is a load file name, then we may have went in to the staging room with a loaded game
		-- This can set a lot of things we don't want on at this time.
		
		local bIsInternet = PreGame.IsInternetGame();
		local eGameType = PreGame.GetGameType();
		
		PreGame.Reset();
			
		PreGame.SetInternetGame( bIsInternet );
		PreGame.SetGameType(eGameType);
		PreGame.ResetSlots();		
	end
	
	-- Ingame Civ Drafter START
	HandleExitDrafts();
	-- Ingame Civ Drafter END
	UIManager:DequeuePopup( ContextPtr );
end


----------------------------------------------------------------
-- Connection Established
----------------------------------------------------------------
function OnConnect( playerID )
    if( ContextPtr:IsHidden() == false ) then

    	UpdateDisplay();
      BuildPlayerNames();
    	OnChat( playerID, -1, Locale.ConvertTextKey( "TXT_KEY_CONNECTED" ) );

    end
end
Events.ConnectedToNetworkHost.Add( OnConnect );


----------------------------------------------------------------
----------------------------------------------------------------
function OnDisconnectOrPossiblyUpdate()
	if( ContextPtr:IsHidden() == false ) then
		BuildPlayerNames(); -- Player names need to be rebuilt because this event could have been for a player ID swap.
		UpdateDisplay();
		UpdatePageTabView(true);
		ShowHideInviteButton();
		ShowHideSaveButton();
		CheckGameAutoStart(); -- Player disconnects can affect the game start countdown.
	end
end
Events.MultiplayerGamePlayerUpdated.Add( OnDisconnectOrPossiblyUpdate );


----------------------------------------------------------------
----------------------------------------------------------------
function OnDisconnect( playerID )
	if( ContextPtr:IsHidden() == false ) then
		if(Network.IsPlayerKicked(playerID)) then
			OnChat( playerID, -1, Locale.ConvertTextKey( "TXT_KEY_KICKED" ) );
		else
			OnChat( playerID, -1, Locale.ConvertTextKey( "TXT_KEY_DISCONNECTED" ) );
		end
		ShowHideInviteButton();
		-- Ingame Civ Drafter START
		if g_DraftProgress == 'DRAFT_PROGRESS_INIT' or g_DraftProgress == 'DRAFT_PROGRESS_DEFAULT' then
			print('disconnect: unready', playerID)
			local product = 6 * 2 ^ 28 + playerID;  -- player disconnected (INIT stage)
			local data = PreGame.SetLeaderKey( product, '' );
		elseif g_DraftProgress ~= 'DRAFT_PROGRESS_OVER' then
			print('disconnect: reset drafts', playerID)
			local product = 4 * 2 ^ 28;  -- reset draft data (local)
			local data = PreGame.SetLeaderKey( product, 'TXT_KEY_DRAFTS_RESET_DISC' );
		end
		-- Ingame Civ Drafter END
	end
end
Events.MultiplayerGamePlayerDisconnected.Add( OnDisconnect );

----------------------------------------------------------------
----------------------------------------------------------------
function OnHostMigration( playerID )
    if( ContextPtr:IsHidden() == false ) then
    	OnChat( playerID, -1, Locale.ConvertTextKey( "TXT_KEY_HOST_MIGRATION" ) );
	end
end
Events.MultiplayerGameHostMigration.Add( OnHostMigration );


----------------------------------------------------------------
-- UPDATE CURRENT SETTINGS
----------------------------------------------------------------
function UpdateDisplay()
	if( ContextPtr:IsHidden() == false ) then
		UpdateOptions();
		RefreshPlayerList();
		DoCheckTeams();
		-- Ingame Civ Drafter START
		UpdateDraftScreen();
		-- Ingame Civ Drafter END

		-- Allow the host to manually start the game when loading a game.
		if(m_bIsHost and PreGame.GetLoadFileName() ~= "" and not IsInGameScreen()) then
			Controls.LaunchButton:SetHide( false );	
			Controls.LaunchButton:SetDisabled( not Network.IsEveryoneConnected() );
			
			-- Set launch button tool tip.
			if ( not Network.IsEveryoneConnected() ) then
				-- Launch button is blocked by a player joining the game.
				Controls.LaunchButton:LocalizeAndSetToolTip( "TXT_KEY_LAUNCH_GAME_BLOCKED_PLAYER_JOINING" );
			else
				-- Launch button is functional.  No tooltip needed.
				Controls.LaunchButton:SetToolTipString();
			end	        
		else
			Controls.LaunchButton:SetHide( true );
		end
	end
end


-------------------------------------------------
-- CHECK FOR GAME AUTO START
-------------------------------------------------
function CheckGameAutoStart()
-- Check to see if we should start/stop the multiplayer game.

	-- Check to make sure we don't have too many players for this map.
	local totalPlayers = 0;
	for i = 0, GameDefines.MAX_MAJOR_CIVS do
		if( PreGame.GetSlotStatus( i ) == SlotStatus.SS_COMPUTER or PreGame.GetSlotStatus( i ) == SlotStatus.SS_TAKEN ) then
			if( PreGame.GetSlotClaim( i ) == SlotClaim.SLOTCLAIM_ASSIGNED ) then
				totalPlayers = totalPlayers + 1;
			end
		end
	end
	local maxPlayers = GetMaxPlayersForCurrentMap();
	local bPlayerCountValid = (totalPlayers <= maxPlayers);
		
	if(m_bLaunchReady and m_bTeamsValid and bPlayerCountValid and not PreGame.GameStarted() and Network.IsEveryoneConnected()) then
		-- Everyone has readied up and we can start.
		if(PreGame.IsHotSeatGame()) then
			-- Hotseat skips the countdown.
			LaunchGame();
		else
			StartCountdown();
		end
	else
		-- We can't autostart now, stop the countdown incase we started it earlier.
		StopCountdown();
	end
end


-------------------------------------------------
-------------------------------------------------
function OnPreGameDirty()
    if( ContextPtr:IsHidden() == false ) then
	    UpdateDisplay();
		UpdatePageTabView(true);
		CheckGameAutoStart();  -- Check for autostart because this event could to due to a ready status changing.
	end
end
Events.PreGameDirty.Add( OnPreGameDirty );

-------------------------------------------------
-------------------------------------------------
function UpdatePingTimeLabel( kLabel, playerID )
	local iPingTime = Network.GetPingTime( playerID );
	if (iPingTime >= 0) then
		iPingTime = math.clamp(iPingTime, 0, 99999); --limit value to something that can be displayed in 5 chars.
		kLabel:SetHide( false );
		if (iPingTime == 0) then
			kLabel:LocalizeAndSetText("TXT_KEY_STAGING_ROOM_UNDER_1_MS");
		else
			if (iPingTime < 1000) then
				local ms = Locale.Lookup("TXT_KEY_STAGING_ROOM_TIME_MS");
				kLabel:SetText( tostring(iPingTime) .. ms );
			else
				local s = Locale.Lookup("TXT_KEY_STAGING_ROOM_TIME_S");
				kLabel:SetText( string.format("%.2fs" , (iPingTime / 1000)) );
			end
		end
	else
		kLabel:SetHide( true );
	end			
end
-------------------------------------------------
-------------------------------------------------
function OnPingTimesChanged()
    if( ContextPtr:IsHidden() == false and not PreGame.IsHotSeatGame()) then

		-- Get the Current Player List
		local playerTable = Matchmaking.GetPlayerList();
		
		-- Display Each Player
		if( playerTable ) then
			for i, playerInfo in ipairs( playerTable ) do
				local playerID = playerInfo.playerID;

				if( playerID ~= Matchmaking.GetLocalID() ) then
					local slotInstance = m_SlotInstances[ i ];				
					if (slotInstance ~= nil and not slotInstance.Root:IsHidden()) then

						local bIsHuman  = (PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_TAKEN ) or
										  (PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_OPEN );
				        
						if ( bIsHuman ) then
							UpdatePingTimeLabel( slotInstance.PingTimeLabel, playerID );
						end
					end
				end
			end	
		end
    end
end
Events.MultiplayerPingTimesChanged.Add( OnPingTimesChanged );

----------------------------------------------------------------        
-- Input Handler
----------------------------------------------------------------        
function InputHandler( uiMsg, wParam, lParam )
	if uiMsg == KeyEvents.KeyDown then
		if wParam == Keys.VK_ESCAPE then
			-- Ingame Civ Drafter START
			if not Controls.DraftBGBlock:IsHidden() then
				OnDraftsClose();
			else
			-- Ingame Civ Drafter END
				if (not Matchmaking.IsLaunchingGame()) then
					HandleExitRequest();
				end
				return true;
			-- Ingame Civ Drafter START
			end
			-- Ingame Civ Drafter END
		end
		-- TODO: REMOVE
		--[[
		if wParam == Keys.V then
    		AddPlayerBans (1,'User','4')
		end
		if wParam == Keys.B then
			UIManager:SetDrawControlBounds(true)
		end
		if wParam == Keys.N then
			UIManager:SetDrawControlBounds(false)
		end
		]]--
	end
	
    if( uiMsg == KeyEvents.KeyUp
        and (wParam == Keys.VK_TAB or wParam == Keys.VK_RETURN)) then
        Controls.ChatEntry:TakeFocus();
        return true;
    end
end
ContextPtr:SetInputHandler( InputHandler );

-------------------------------------------------
-------------------------------------------------
function UpdatePageTitle()
	local bIsModding = (ContextPtr:LookUpControl("../.."):GetID() == "ModMultiplayerSelectScreen");
	if(IsInGameScreen()) then
		Controls.TitleLabel:LocalizeAndSetText( "{TXT_KEY_MULTIPLAYER_DEDICATED_SERVER_ROOM:upper}" );	
	elseif(m_bEditOptions) then
		if(bIsModding) then
			Controls.TitleLabel:LocalizeAndSetText( "TXT_KEY_MOD_MP_GAME_SETUP_HEADER" );
		else
			Controls.TitleLabel:LocalizeAndSetText( "TXT_KEY_MULTIPLAYER_GAME_SETUP_HEADER" );	
		end
	else
		Controls.TitleLabel:LocalizeAndSetText( "{TXT_KEY_MULTIPLAYER_STAGING_ROOM:upper}" );
	end
end
-------------------------------------------------
-------------------------------------------------
function UpdatePageTabView(bUpdateOnly)
	Controls.Host:SetHide( m_bEditOptions );
	Controls.ListingScrollPanel:SetHide( m_bEditOptions );
	Controls.OptionsScrollPanel:SetHide( not m_bEditOptions );
	Controls.OptionsPageTabHighlight:SetHide( not m_bEditOptions );
	Controls.PlayersPageTabHighlight:SetHide( m_bEditOptions );
	Controls.GameOptionsSummary:SetHide( m_bEditOptions );
	Controls.GameOptionsSummaryTitle:SetHide( m_bEditOptions );
	Controls.VerticalTrim:SetHide( m_bEditOptions );
	
	UpdatePageTitle();
	if (m_bEditOptions) then
		Controls.OptionsScrollPanel:SetSizeY( Controls.VerticalTrim:GetSizeY() - 2);
		Controls.OptionsScrollPanel:CalculateInternalSize();
		
		bIsModding = (ContextPtr:LookUpControl("../.."):GetID() == "ModMultiplayerSelectScreen");
		if(bIsModding) then
			Controls.ModsButton:SetHide( false );
		else
			Controls.ModsButton:SetHide( true );
		end
		
		PopulateMapSizePulldown();

		RefreshMapScripts();	
		PreGame.SetRandomMapScript(false);	-- Random map scripts is not supported in multiplayer
		UpdateGameOptionsDisplay(bUpdateOnly);	
	else
		if (PreGame.IsHotSeatGame()) then
			-- In case they changed the DLC
			local prevCursor = UIManager:SetUICursor( 1 );
			local bChanged = Modding.ActivateAllowedDLC();																		
			UIManager:SetUICursor( prevCursor );
			
			Events.SystemUpdateUI( SystemUpdateUIType.RestoreUI, "StagingRoom" );
		end
	end
end
-------------------------------------------------
-------------------------------------------------
function OnPlayersPageTab()
	if (m_bEditOptions) then
		m_bEditOptions = false;
		UpdatePageTabView(true);
	end
end
Controls.PlayersPageTab:RegisterCallback( Mouse.eLClick, OnPlayersPageTab );

-------------------------------------------------
-------------------------------------------------
function OnOptionsPageTab()
	if (not m_bEditOptions) then
		m_bEditOptions = true;
		UpdatePageTabView(true);
	end
end
Controls.OptionsPageTab:RegisterCallback( Mouse.eLClick, OnOptionsPageTab );

-------------------------------------------------
-------------------------------------------------
function ShowHideHandler( bIsHide, bIsInit )

		--print("ShowHideHandler Hide: " .. tostring(bIsHide) .. " Init: " .. tostring(bIsInit));
    if( bIsHide ) then
			Controls.ChatStack:DestroyAllChildren();
    end
    
    -- Create slot instances.
    if ( bIsInit ) then
			CreateSlots();
		end
    
    -- Show background image if this is the ingame dedicated server screen.
    if( not bIsHide 
			and IsInGameScreen() ) then
			Controls.AtlasLogo:SetHide(false);
    else
			Controls.AtlasLogo:SetHide(true);
    end
    
    if( not bIsHide ) then
		-- Ingame Civ Drafter START
		print('--- StagingRoom.lua ---')
		print((PreGame.GetLoadFileName() ~= ""), PreGame.GameStarted())
		Controls.DraftButton:SetHide( (PreGame.GetLoadFileName() ~= "") or PreGame.GameStarted() );
		RebuildDrafts();
		PopulateDrafts();
		-- Ingame Civ Drafter END
		if( Matchmaking.IsHost() and
			not IsInGameScreen() and
			PreGame.GetGameOption("GAMEOPTION_PITBOSS") == 1 and
			OptionsManager.GetTurnNotifySmtpHost_Cached() ~= "" and
			OptionsManager.GetTurnNotifySmtpPassword_Cached() == "" ) then
			-- Display email smtp password prompt if we're hosting a pitboss game and the password is blank.
			UIManager:PushModal(Controls.ChangeSmtpPassword, true);		
		end
    
		-- Activate only the DLC allowed for this MP game.  Mods will also deactivated/activate too.
		if (not ContextPtr:IsHotLoad()) then
			local prevCursor = UIManager:SetUICursor( 1 );
			local bChanged = Modding.ActivateAllowedDLC();		
			UIManager:SetUICursor( prevCursor );
		
			-- Send out an event to continue on, as the ActivateDLC may have swapped out the UI	
			Events.SystemUpdateUI( SystemUpdateUIType.RestoreUI, "StagingRoom" );
		end
				
		if ( PreGame.IsHotSeatGame() ) then		
			-- If Hot Seat, just 'ready' the host
			Controls.LocalReadyCheck:SetHide(true);
			PreGame.SetReady( Matchmaking.GetLocalID() );
			-- Hide the chat panel		
			Controls.ChatBox:SetHide( true );
		else
			Controls.ChatBox:SetHide( false );
		end
		
		SetInLobby( true );
		m_bEditOptions = false;
		ShowHideExitButton();
		ShowHideBackButton();
		ShowHideInviteButton();		
		ShowHideStrategicViewButton();
		UpdatePageTabView(true);
		StopCountdown(); -- Make sure the countdown is stopped if we're toggling the hide status of the screen.
			
		AdjustScreenSize();
	
		ValidateCivSelections();
   		BuildSlots();		-- Populate the civs for the slots.  This can change so it must be done every time.
        BuildPlayerNames();
		UpdateDisplay();
		ShowHideSaveButton();
		UIManager:SetUICursor( 0 );
	end
end
ContextPtr:SetShowHideHandler( ShowHideHandler );


-------------------------------------------------
-------------------------------------------------
function UpdateOptions()

	-- Set Game Name
	local strGameName = Matchmaking.GetCurrentGameName();
	Controls.NameLabel:SetText( strGameName );
	
	-- Game State Indicator
	if( PreGame.GameStarted() ) then
		Controls.HotJoinBox:SetHide( false );
		Controls.LoadingBox:SetHide( true );	
	elseif( PreGame.GetLoadFileName() ~= "" ) then
		Controls.HotJoinBox:SetHide( true );
	  Controls.LoadingBox:SetHide( false );
  else
		Controls.HotJoinBox:SetHide( true );
	  Controls.LoadingBox:SetHide( true );
	end
	
	-- Set Max Turns if set
	local maxTurns = PreGame.GetMaxTurns();
	if(maxTurns == 0) then
		Controls.MaxTurns:SetHide(true);
	else
		Controls.MaxTurns:SetHide(false);
		Controls.MaxTurns:SetText(Locale.ConvertTextKey("TXT_KEY_MAX_TURNS", maxTurns));
	end
	
	-- Show turn timer value if set.
	if(PreGame.GetGameOption("GAMEOPTION_END_TURN_TIMER_ENABLED") == 1) then
		Controls.TurnTimer:SetHide(false);
		local turnTimerStr = Locale.ConvertTextKey("TXT_KEY_MP_OPTION_TURN_TIMER");
		local pitBossTurnTime = PreGame.GetPitbossTurnTime();
		if(pitBossTurnTime > 0) then
			turnTimerStr = turnTimerStr .. ": " .. pitBossTurnTime;
			if(PreGame.GetGameOption("GAMEOPTION_PITBOSS") == 1) then
				turnTimerStr = turnTimerStr .. " " .. hoursStr;
			else
				turnTimerStr = turnTimerStr .. " " .. secondsStr;
			end
		end
		Controls.TurnTimer:SetText(turnTimerStr);
	else
		Controls.TurnTimer:SetHide(true);
	end
			
	-- Set Start Era
--	Controls.StartEraLabel:SetText( Locale.ConvertTextKey("TXT_KEY_START_ERA", Locale.ConvertTextKey(GameInfo.Eras[ PreGame.GetEra() ].Description)));
	local eraInfo = GameInfo.Eras[PreGame.GetEra()];
	if(eraInfo == nil) then
		Controls.StartEraLabel:SetText( Locale.ConvertTextKey("TXT_KEY_MISC_UNKNOWN") );
	else
		Controls.StartEraLabel:SetText( Locale.ConvertTextKey( GameInfo.Eras[ PreGame.GetEra() ].Description ) );		
	end
	
	-- Set Turn Mode
	local turnModeStr = GetTurnModeStr();
	Controls.TurnModeLabel:LocalizeAndSetText(turnModeStr);
	Controls.TurnModeLabel:LocalizeAndSetToolTip(GetTurnModeToolTipStr(turnModeStr));

	-- Set Game Map Type
	if( not PreGame.IsRandomMapScript() ) then   
		local mapScriptFileName = Locale.ToLower(PreGame.GetMapScript());
		local mapScript = nil;
        
		for row in GameInfo.MapScripts{FileName = mapScriptFileName} do
			mapScript = row;
			break;
		end
		
		if(mapScript ~= nil) then
			Controls.MapTypeLabel:LocalizeAndSetText( mapScript.Name );
			Controls.MapTypeLabel:LocalizeAndSetToolTip( mapScript.Description );
		else
			local mapInfo = UI.GetMapPreview(mapScriptFileName);
			if(mapInfo ~= nil) then
				Controls.MapTypeLabel:LocalizeAndSetText(mapInfo.Name);
				Controls.MapTypeLabel:LocalizeAndSetToolTip(mapInfo.Description);
			else
				-- print("Cannot get info for map or map script - " .. tostring(mapScriptFileName));
				local fileTitle = Path.GetFileName(PreGame.GetMapScript());
				if (fileTitle ~= nil) then
					Controls.MapTypeLabel:SetText("[COLOR_RED]" .. fileTitle .. "[ENDCOLOR]");
					Controls.MapTypeLabel:LocalizeAndSetToolTip("TXT_KEY_FILE_INFO_NOT_FOUND");
				end
			end	
		end
	end
    
	if(PreGame.IsRandomMapScript()) then
		Controls.MapTypeLabel:LocalizeAndSetText( "TXT_KEY_RANDOM_MAP_SCRIPT" );
		Controls.MapTypeLabel:LocalizeAndSetToolTip( "TXT_KEY_RANDOM_MAP_SCRIPT_HELP" );
	end
	
	-- Set Map Size
	if(not PreGame.IsRandomWorldSize() ) then
		info = GameInfo.Worlds[ PreGame.GetWorldSize() ];
		Controls.MapSizeLabel:LocalizeAndSetText( info.Description );
		Controls.MapSizeLabel:LocalizeAndSetToolTip( info.Help );
	else
		Controls.MapSizeLabel:LocalizeAndSetText( "TXT_KEY_RANDOM_MAP_SIZE" );
		Controls.MapSizeLabel:LocalizeAndSetToolTip( "TXT_KEY_RANDOM_MAP_SIZE_HELP" );
	end
	
    -- Set Game Pace Slot
    info = GameInfo.GameSpeeds[ PreGame.GetGameSpeed() ];
	Controls.GameSpeedLabel:LocalizeAndSetText( info.Description );
	Controls.GameSpeedLabel:LocalizeAndSetToolTip( info.Help );
	
	-- Game Options
	g_AdvancedOptionIM:ResetInstances();
	g_AdvancedOptionsList = {};
	
    local count = 1;
    -- When there's 8 or more players connected to us (9 player game), warn that it's an unsupported game.
    
    local totalPlayers = 0;
    for i = 0, GameDefines.MAX_MAJOR_CIVS do
        if( PreGame.GetSlotStatus( i ) == SlotStatus.SS_COMPUTER or PreGame.GetSlotStatus( i ) == SlotStatus.SS_TAKEN ) then
			if( PreGame.GetSlotClaim( i ) == SlotClaim.SLOTCLAIM_ASSIGNED ) then
				totalPlayers = totalPlayers + 1;
			end
		end
	end

	-- Is it a private game?
    if(PreGame.IsPrivateGame()) then
		local controlTable = g_AdvancedOptionIM:GetInstance();
		g_AdvancedOptionsList[count] = controlTable;
		controlTable.Text:LocalizeAndSetText("TXT_KEY_MULTIPLAYER_PRIVATE_GAME");
		count = count + 1;
    end
	
    if(totalPlayers > 8) then
		local controlTable = g_AdvancedOptionIM:GetInstance();
		g_AdvancedOptionsList[count] = controlTable;
		controlTable.Text:LocalizeAndSetText("TXT_KEY_MULTIPLAYER_UNSUPPORTED_NUMBER_OF_PLAYERS");
		count = count + 1;
    end
    
    for option in GameInfo.GameOptions{Visible = 1} do
    	if option.SupportsMultiplayer then
			if( option.Type ~= "GAMEOPTION_END_TURN_TIMER_ENABLED" 
					and option.Type ~= "GAMEOPTION_SIMULTANEOUS_TURNS"
					and option.Type ~= "GAMEOPTION_DYNAMIC_TURNS") then 
				local savedValue = PreGame.GetGameOption(option.Type);
				if(savedValue ~= nil and savedValue == 1) then
					local controlTable = g_AdvancedOptionIM:GetInstance();
					g_AdvancedOptionsList[count] = controlTable;
					controlTable.Text:LocalizeAndSetText(option.Description);
					controlTable.Text:LocalizeAndSetToolTip(option.Help);
					count = count + 1;
				end
			end
		end
	end

-- Duel Mode START
	for dueloption in GameInfo.GameOptions{Type = "GAMEOPTION_DUEL_STUFF"} do
		local savedValue = PreGame.GetGameOption(dueloption.Type);
		if(savedValue ~= nil and savedValue == 1) then
			local controlTable = g_AdvancedOptionIM:GetInstance();
			g_AdvancedOptionsList[count] = controlTable;
			controlTable.Text:LocalizeAndSetText(dueloption.Description);
			controlTable.Text:SetAlpha(0.8);
			controlTable.Text:LocalizeAndSetToolTip(dueloption.Help);
			count = count + 1;
		end

		if PreGame.GetGameOption("GAMEOPTION_DUEL_STUFF") > 0 then
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_DISABLE_OXFORD_UNIVERSITY"} do
				local savedValue = PreGame.GetGameOption(option.Type);
				if(savedValue ~= nil and savedValue == 1) then
					local controlTable = g_AdvancedOptionIM:GetInstance();
					g_AdvancedOptionsList[count] = controlTable;
					controlTable.Text:LocalizeAndSetText(option.Description);
					controlTable.Text:LocalizeAndSetToolTip(option.Help);
					count = count + 1;
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_NO_LEAGUES"} do
				local savedValue = PreGame.GetGameOption(option.Type);
				if(savedValue ~= nil and savedValue == 1) then
					local controlTable = g_AdvancedOptionIM:GetInstance();
					g_AdvancedOptionsList[count] = controlTable;
					controlTable.Text:LocalizeAndSetText(option.Description);
					controlTable.Text:LocalizeAndSetToolTip(option.Help);
					count = count + 1;
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_NO_GOODY_HUTS"} do
				local savedValue = PreGame.GetGameOption(option.Type);
				if(savedValue ~= nil and savedValue == 1) then
					local controlTable = g_AdvancedOptionIM:GetInstance();
					g_AdvancedOptionsList[count] = controlTable;
					controlTable.Text:LocalizeAndSetText(option.Description);
					controlTable.Text:LocalizeAndSetToolTip(option.Help);
					count = count + 1;
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_NO_ESPIONAGE"} do
				local savedValue = PreGame.GetGameOption(option.Type);
				if(savedValue ~= nil and savedValue == 1) then
					local controlTable = g_AdvancedOptionIM:GetInstance();
					g_AdvancedOptionsList[count] = controlTable;
					controlTable.Text:LocalizeAndSetText(option.Description);
					controlTable.Text:LocalizeAndSetToolTip(option.Help);
					count = count + 1;
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_NO_BARBARIANS"} do
				local savedValue = PreGame.GetGameOption(option.Type);
				if(savedValue ~= nil and savedValue == 1) then
					local controlTable = g_AdvancedOptionIM:GetInstance();
					g_AdvancedOptionsList[count] = controlTable;
					controlTable.Text:LocalizeAndSetText(option.Description);
					controlTable.Text:LocalizeAndSetToolTip(option.Help);
					count = count + 1;
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_RAGING_BARBARIANS"} do
				if PreGame.GetGameOption("GAMEOPTION_NO_BARBARIANS") <= 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue == 1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:LocalizeAndSetText(option.Description);
						controlTable.Text:LocalizeAndSetToolTip(option.Help);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_BAN_WONDER1"} do
				if PreGame.GetGameOption("GAMEOPTION_BAN_WORLD_WONDERS") > 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue > -1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:SetText("[COLOR_NEGATIVE_TEXT]" .. Locale.ConvertTextKey(GameInfo.Buildings[savedValue].Description) .. "[ENDCOLOR]");
						controlTable.Text:LocalizeAndSetToolTip(GameInfo.Buildings[savedValue].Help);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_BAN_WONDER2"} do
				if PreGame.GetGameOption("GAMEOPTION_BAN_WORLD_WONDERS") > 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue > -1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:SetText("[COLOR_NEGATIVE_TEXT]" .. Locale.ConvertTextKey(GameInfo.Buildings[savedValue].Description) .. "[ENDCOLOR]");
						controlTable.Text:LocalizeAndSetToolTip(GameInfo.Buildings[savedValue].Help);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_BAN_WONDER3"} do
				if PreGame.GetGameOption("GAMEOPTION_BAN_WORLD_WONDERS") > 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue > -1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:SetText("[COLOR_NEGATIVE_TEXT]" .. Locale.ConvertTextKey(GameInfo.Buildings[savedValue].Description) .. "[ENDCOLOR]");
						controlTable.Text:LocalizeAndSetToolTip(GameInfo.Buildings[savedValue].Help);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_BAN_PANTHEON1"} do
				if PreGame.GetGameOption("GAMEOPTION_BAN_PANTHEONS") > 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue > -1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:SetText("[COLOR_NEGATIVE_TEXT]" .. Locale.ConvertTextKey(GameInfo.Beliefs[savedValue].ShortDescription) .. "[ENDCOLOR]");
						controlTable.Text:LocalizeAndSetToolTip(GameInfo.Beliefs[savedValue].Description);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_BAN_PANTHEON2"} do
				if PreGame.GetGameOption("GAMEOPTION_BAN_PANTHEONS") > 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue > -1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:SetText("[COLOR_NEGATIVE_TEXT]" .. Locale.ConvertTextKey(GameInfo.Beliefs[savedValue].ShortDescription) .. "[ENDCOLOR]");
						controlTable.Text:LocalizeAndSetToolTip(GameInfo.Beliefs[savedValue].Description);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_BAN_PANTHEON3"} do
				if PreGame.GetGameOption("GAMEOPTION_BAN_PANTHEONS") > 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue > -1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:SetText("[COLOR_NEGATIVE_TEXT]" .. Locale.ConvertTextKey(GameInfo.Beliefs[savedValue].ShortDescription) .. "[ENDCOLOR]");
						controlTable.Text:LocalizeAndSetToolTip(GameInfo.Beliefs[savedValue].Description);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_BAN_BELIEF1"} do
				if PreGame.GetGameOption("GAMEOPTION_BAN_RELIGION_BELIEFS") > 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue > -1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:SetText("[COLOR_NEGATIVE_TEXT]" .. Locale.ConvertTextKey(GameInfo.Beliefs[savedValue].ShortDescription) .. "[ENDCOLOR]");
						controlTable.Text:LocalizeAndSetToolTip(GameInfo.Beliefs[savedValue].Description);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_BAN_BELIEF2"} do
				if PreGame.GetGameOption("GAMEOPTION_BAN_RELIGION_BELIEFS") > 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue > -1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:SetText("[COLOR_NEGATIVE_TEXT]" .. Locale.ConvertTextKey(GameInfo.Beliefs[savedValue].ShortDescription) .. "[ENDCOLOR]");
						controlTable.Text:LocalizeAndSetToolTip(GameInfo.Beliefs[savedValue].Description);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_BAN_BELIEF3"} do
				if PreGame.GetGameOption("GAMEOPTION_BAN_RELIGION_BELIEFS") > 0 then
					local savedValue = PreGame.GetGameOption(option.Type);
					if(savedValue ~= nil and savedValue > -1) then
						local controlTable = g_AdvancedOptionIM:GetInstance();
						g_AdvancedOptionsList[count] = controlTable;
						controlTable.Text:SetText("[COLOR_NEGATIVE_TEXT]" .. Locale.ConvertTextKey(GameInfo.Beliefs[savedValue].ShortDescription) .. "[ENDCOLOR]");
						controlTable.Text:LocalizeAndSetToolTip(GameInfo.Beliefs[savedValue].Description);
						count = count + 1;
					end
				end
			end
			for option in GameInfo.GameOptions{Type = "GAMEOPTION_ENABLE_REMAP_VOTE"} do
				local savedValue = PreGame.GetGameOption(option.Type);
				if(savedValue ~= nil and savedValue == 1) then
					local controlTable = g_AdvancedOptionIM:GetInstance();
					g_AdvancedOptionsList[count] = controlTable;
					controlTable.Text:LocalizeAndSetText(option.Description);
					controlTable.Text:LocalizeAndSetToolTip(option.Help);
					count = count + 1;
				end
			end
		end
	end
	-- Duel Mode END
	-- Ingame Civ Drafter START
	local op1 = PreGame.GetGameOption('GAMEOPTION_TOURNAMENT_MODE');
	local op2 = PreGame.GetWorldSize();
	if g_DraftProgress == 'DRAFT_PROGRESS_INIT' or g_DraftProgress == 'DRAFT_PROGRESS_DEFAULT' then
		if g_DraftSettings.TournamentMode ~= op1 then
			RebuildDrafts()
		end
	else
		if g_DraftSettings.TournamentMode ~= op1 or g_DraftSettings.WorldSize ~= op2 then
			local product = 4 * 2 ^ 28;  -- reset draft data (local)
			local data = PreGame.SetLeaderKey( product, 'TXT_KEY_DRAFTS_RESET_GAMEOPTION' );
			RebuildDrafts()
			Controls.DraftBGBlock:SetHide(true)
		end
	end
	g_DraftSettings.TournamentMode = op1;
	g_DraftSettings.WorldSize = op2;
	-- Ingame Civ Drafter END
	-- Add empty text to padd the bottom.
	local controlTable = g_AdvancedOptionIM:GetInstance();
	controlTable.Text:SetText(" ");
		
		-- Update scrollable panel
		Controls.AdvancedOptions:CalculateSize();
		Controls.GameOptionsSummary:CalculateInternalSize();
end


-------------------------------------------------
-------------------------------------------------
function PopulateCivPulldown( pullDown, playerID )

    local controlTable = {};

	pullDown:ClearEntries();

    ------------------------------------------------------------------------------------------------
    -- set up the random slot
    ------------------------------------------------------------------------------------------------
    pullDown:BuildEntry( "InstanceOne", controlTable );
    controlTable.Button:SetVoids( playerID, -1 );

    controlTable.Button:SetText( Locale.Lookup( "TXT_KEY_RANDOM_LEADER" ) );
    controlTable.Button:SetToolTipString( Locale.Lookup( "TXT_KEY_RANDOM_LEADER_HELP" ) );


	local civEntries = {};
	for row in DB.Query([[SELECT 
							Civilizations.ID as CivID, 
							Civilizations.Description as CivDescription, 
							Civilizations.ShortDescription as CivShortDescription,
							Leaders.Description as LeaderDescription
							FROM Civilizations, Leaders, Civilization_Leaders
							WHERE
								Civilizations.Playable = 1 AND
								Civilizations.Type = Civilization_Leaders.CivilizationType AND
								Leaders.Type = Civilization_Leaders.LeaderheadType
						]]) do
						
		local title = Locale.Lookup("TXT_KEY_RANDOM_LEADER_CIV", Locale.Lookup(row.LeaderDescription), Locale.Lookup(row.CivShortDescription)); 

		table.insert(civEntries, {ID = row.CivID, Title = title, Description = Locale.Lookup(row.CivDescription), ShortDescription = Locale.Lookup(row.CivShortDescription)});						
	end
	
-- Sorting Civs by Short Description
	table.sort(civEntries, function(a,b)
		local astr = Locale.ConvertTextKey(a.ShortDescription);
		local bstr = Locale.ConvertTextKey(b.ShortDescription);
		local a0, b0
		if astr:match("The ") then
			a0 = astr:sub(5)
		else
			a0 = astr
		end
		if bstr:match("The ") then
			b0 = bstr:sub(5)
		else
			b0 = bstr
		end
		return a0 < b0
	end);
-- Sorting Civs by Short Description END

	for i,v in ipairs(civEntries) do
		
		local controlTable = {};
        pullDown:BuildEntry( "InstanceOne", controlTable );
        
        controlTable.Button:SetVoids( playerID, v.ID );
        controlTable.Button:SetText(v.Title);
        
        local bw,bh = (controlTable.Button:GetSizeVal() - 20);
        
        local textControl = controlTable.Button:GetTextControl();
        textControl:SetTruncateWidth(bw);
        
        controlTable.Button:SetToolTipString(v.Description);
	end

    pullDown:CalculateInternals();
    pullDown:RegisterSelectionCallback( CivSelected );
end


-------------------------------------------------
-- TODO: This only gets called once.  We need something more dynamic.
-------------------------------------------------
function PopulateInvitePulldown( pullDown, playerID )

	local controlTable = {};
	
	pullDown:ClearEntries();
	
	pullDown:BuildEntry( "InstanceOne", controlTable );

	controlTable.Button:SetText( Locale.ConvertTextKey("TXT_KEY_AI_NICKNAME") );
	controlTable.Button:SetVoids( playerID, -1 );

	local friendList = Matchmaking.GetFriendList();

	if friendList then

		-- Populate with Steam friends.
		for i = 1, #friendList do

			local controlTable = {};
			pullDown:BuildEntry( "InstanceOne", controlTable );

			controlTable.Button:SetText( "Invite " .. friendList[i].playerName );
			controlTable.Button:SetVoids( playerID, friendList[i].steamID );

		end

	end

	pullDown:CalculateInternals();
	pullDown:RegisterSelectionCallback( InviteSelected );

end


-------------------------------------------------
-------------------------------------------------
function PopulateTeamPulldown( pullDown, playerID )

	local playerTable = Matchmaking.GetPlayerList();
	local count = 0;

	pullDown:ClearEntries();

	-- Display Each Player
	if playerTable then
    	count = #playerTable;
		
		for i = 1, count, 1 do

			local controlTable = {};
			pullDown:BuildEntry( "InstanceOne", controlTable );
			
			controlTable.Button:SetText( Locale.ConvertTextKey("TXT_KEY_MULTIPLAYER_DEFAULT_TEAM_NAME", i) );
			controlTable.Button:SetVoids( playerID, i-1 ); -- TODO: playerID is really more like the slot position.

		end

		pullDown:CalculateInternals();
		pullDown:RegisterSelectionCallback( OnSelectTeam );
	end

end


-------------------------------------------------
-------------------------------------------------
function PopulateHandicapPulldown( pullDown, playerID )

	pullDown:ClearEntries();

	for info in GameInfo.HandicapInfos() do
		if ( info.Type ~= "HANDICAP_AI_DEFAULT" ) then
			local controlTable = {};
			pullDown:BuildEntry( "InstanceOne", controlTable );

			controlTable.Button:SetText( Locale.ConvertTextKey(info.Description) );
			controlTable.Button:LocalizeAndSetToolTip(info.Help);
			controlTable.Button:SetVoids( playerID, info.ID );
		end
	end    

	pullDown:CalculateInternals();
	pullDown:RegisterSelectionCallback( OnHandicapTeam );

end

-------------------------------------------------
-------------------------------------------------
function PopulateSlotTypePulldown( pullDown, playerID, slotTypeOptions )

	pullDown:ClearEntries();
	
	local controlTable = {};
	local createEntry;
	for i, typeName in ipairs(slotTypeOptions) do
		controlTable = {};
		createEntry = true;
		
		if(PreGame.IsHotSeatGame()) then
			if(typeName == "TXT_KEY_SLOTTYPE_HUMANREQ") then
				-- "Human Required" slot type is the "Human" slot type in hotseat mode.
				typeName = "TXT_KEY_PLAYER_TYPE_HUMAN";
			elseif(typeName == "TXT_KEY_SLOTTYPE_OBSERVER") then
				-- observer mode not allowed in hotseat mode.
				createEntry = false;
			elseif(typeName == "TXT_KEY_SLOTTYPE_OPEN") then
				-- open mode is redundent in hotseat mode.
				createEntry = false;
			end
		elseif(Network.IsPlayerConnected(playerID)) then
			-- player is actively occupying this slot
			if(typeName == "TXT_KEY_SLOTTYPE_OPEN") then
				-- transform open slottype position to human (for changing from an observer to normal human player)
				typeName = "TXT_KEY_PLAYER_TYPE_HUMAN";
			elseif(typeName == "TXT_KEY_SLOTTYPE_CLOSED" or typeName == "TXT_KEY_SLOTTYPE_AI") then
				-- Don't allow those types on human occupied slots.
				createEntry = false;
			end
		end
				
		if(createEntry == true) then
			pullDown:BuildEntry( "InstanceOne", controlTable );
					
			controlTable.Button:SetText( Locale.ConvertTextKey(typeName) );
			controlTable.Button:LocalizeAndSetToolTip( g_slotTypeData[typeName].tooltip );
			controlTable.Button:SetVoids( playerID, g_slotTypeData[typeName].index );	
		end
	end

	pullDown:CalculateInternals();
	pullDown:RegisterSelectionCallback( OnSlotType);

end


--------------------------------------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------------------------------------
-- Slot Type 
--------------------------------------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------------------------------------

-------------------------------------------------
-------------------------------------------------
function GetSlotTypeString( playerID )
	-- print("GetSlotTypeString playerID: " .. playerID .. " GetSlotStatus: " .. PreGame.GetSlotStatus(playerID) .. " GetSlotClaim: " .. PreGame.GetSlotClaim(playerID));
	if(PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_COMPUTER) then
		return "TXT_KEY_SLOTTYPE_AI";
	elseif(PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_OBSERVER) then
		return "TXT_KEY_SLOTTYPE_OBSERVER";
	elseif(PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_CLOSED) then
		return "TXT_KEY_SLOTTYPE_CLOSED";
	elseif(PreGame.GetSlotStatus( playerID ) == SlotStatus.SS_TAKEN) then
		return "TXT_KEY_PLAYER_TYPE_HUMAN";	
	elseif(PreGame.GetSlotClaim( playerID ) == SlotClaim.SLOTCLAIM_RESERVED) then
		if ( PreGame.IsHotSeatGame() ) then
			-- reserved slots are human players in hotseat mode.
			return "TXT_KEY_PLAYER_TYPE_HUMAN";
		else
			-- In normal multiplayer, reserved slots in the staging room indicate that this slot must be occupied by a human player for the game to start.                   
			return "TXT_KEY_SLOTTYPE_HUMANREQ";
    end
	end
		
	return "TXT_KEY_SLOTTYPE_OPEN";
end


--------------------------------------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------------------------------------
-- Chat
--------------------------------------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------------------------------------

-------------------------------------------------
-------------------------------------------------
local bFlipper = false;
function OnChat( fromPlayer, toPlayer, text, eTargetType )
	if( ContextPtr:IsHidden() == false and m_PlayerNames[ fromPlayer ] ~= nil ) then
		local controlTable = {};
		ContextPtr:BuildInstanceForControl( "ChatEntry", controlTable, Controls.ChatStack );
	    
		table.insert( g_ChatInstances, controlTable );
		if( #g_ChatInstances > 100 ) then
	    
			Controls.ChatStack:ReleaseChild( g_ChatInstances[ 1 ].Box );
			table.remove( g_ChatInstances, 1 );
		end
	    
		controlTable.String:SetText( m_PlayerNames[ fromPlayer ] .. ": " .. text );
		controlTable.Box:SetSizeY( controlTable.String:GetSizeY() + 15 );
		controlTable.Box:ReprocessAnchoring();

		if( bFlipper ) then
			controlTable.Box:SetColorChannel( 3, 0.4 );
		end
		bFlipper = not bFlipper;

		Events.AudioPlay2DSound( "AS2D_IF_MP_CHAT_DING" );		

		Controls.ChatStack:CalculateSize();
		Controls.ChatScroll:CalculateInternalSize();
		Controls.ChatScroll:SetScrollValue( 1 );
	end
end
Events.GameMessageChat.Add( OnChat );


-------------------------------------------------
-------------------------------------------------
function SendChat( text )
    if( string.len( text ) > 0 ) then
        Network.SendChat( text );
    end
    Controls.ChatEntry:ClearString();
end
Controls.ChatEntry:RegisterCallback( SendChat );


----------------------------------------------------------------
----------------------------------------------------------------
function OnEnable( bIsCheck, slotNumber )
	local playerID = GetPlayerIDBySelectionIndex( slotNumber );

	local bCanEnableAISlots = PreGame.GetMultiplayerAIEnabled();
	
	if(bCanEnableAISlots) then
		if( bIsCheck ) then
    		PreGame.SetSlotStatus( playerID, SlotStatus.SS_COMPUTER );
    		PreGame.SetHandicap( playerID, 1 );
			if ( PreGame.IsHotSeatGame() ) then
				-- Reset so the player can force a rebuild
				PreGame.SetNickName( playerID, "" );
				PreGame.SetLeaderName( playerID, "" );
				PreGame.SetCivilizationDescription( playerID, "" );
				PreGame.SetCivilizationShortDescription( playerID, "" );
				PreGame.SetCivilizationAdjective( playerID, "" );
			end
		else
    		PreGame.SetSlotStatus( playerID, SlotStatus.SS_CLOSED );
		end
	end
    UpdateDisplay();
    Network.BroadcastPlayerInfo();
end


----------------------------------------------------------------
----------------------------------------------------------------
function OnLock( bIsCheck, slotNumber )

	local playerID = GetPlayerIDBySelectionIndex( slotNumber );
	
    if( bIsCheck ) then
    	PreGame.SetSlotClaim( playerID, SlotClaim.SLOTCLAIM_RESERVED );
	else
    	PreGame.SetSlotClaim( playerID, SlotClaim.SLOTCLAIM_UNASSIGNED );
	end

    UpdateDisplay();
end

----------------------------------------------------------------
-- create the slots
----------------------------------------------------------------
function CreateSlots()
	for i = 1, GameDefines.MAX_MAJOR_CIVS, 1 do

		local instance = {};
		ContextPtr:BuildInstanceForControl( "PlayerSlot", instance, Controls.SlotStack );
		instance.Root:SetHide( true );
	    
		instance.LockCheck:RegisterCheckHandler( OnLock );
		instance.EnableCheck:RegisterCheckHandler( OnEnable );
		instance.EnableCheck:SetVoid1( i );
		instance.KickButton:RegisterCallback( Mouse.eLClick, OnKickPlayer );
		instance.KickButton:SetVoid1( i );
		instance.EditButton:RegisterCallback( Mouse.eLClick, OnEditPlayer );
		instance.EditButton:SetVoid1( i );
		instance.RemapTokenCheck:RegisterCallback( Mouse.eLClick, OnRemapVotePlayerToken );
		instance.RemapTokenCheck:SetVoid1( i );
		instance.SwapButton:RegisterCallback( Mouse.eLClick, OnSwapPlayer );
		instance.SwapButton:SetVoid1( i );

	    
		m_SlotInstances[i] = instance;
	end
end

----------------------------------------------------------------
-- build the slots
----------------------------------------------------------------
function BuildSlots()
	for i = 1, GameDefines.MAX_MAJOR_CIVS, 1 do

		local instance = m_SlotInstances[i];
		instance.Root:SetHide( true );
	    
		PopulateCivPulldown( instance.CivPulldown, i );
		PopulateInvitePulldown( instance.InvitePulldown, i );
		PopulateTeamPulldown( instance.TeamPulldown, i );
		PopulateHandicapPulldown( instance.HandicapPulldown, i );

		instance.EnableCheck:SetVoid1( i );
		instance.KickButton:SetVoid1( i );
		instance.EditButton:SetVoid1( i );
		instance.RemapTokenCheck:SetVoid1( i );
	    
	end

	-- Setup Local Slot
	PopulateCivPulldown( Controls.CivPulldown, 0 );
	PopulateTeamPulldown( Controls.TeamPulldown, 0 );
	PopulateHandicapPulldown( Controls.HandicapPulldown, 0 );
end

-------------------------------------------------
-------------------------------------------------
function OnDLCChanged()
    if( ContextPtr:IsHidden() == false ) then
        ValidateCivSelections();
	   	BuildSlots();		-- Populate the civs for the slots.  This can change so it must be done every time.
		UpdateDisplay();
	end
	-- Ingame Civ Drafter START
	RebuildDrafts();
	print('OnDLCChanged')
	PopulateDrafts();
	-- Ingame Civ Drafter END
end
Events.AfterModsDeactivate.Add( OnDLCChanged );

-------------------------------------------------
-------------------------------------------------
function OnVersionMismatch( iPlayerID, playerName, bIsHost )
    if( bIsHost ) then
        Events.FrontEndPopup.CallImmediate( Locale.ConvertTextKey( "TXT_KEY_MP_VERSION_MISMATCH_FOR_HOST", playerName ) );
    	Matchmaking.KickPlayer( iPlayerID );
    else
        Events.FrontEndPopup.CallImmediate( Locale.ConvertTextKey( "TXT_KEY_MP_VERSION_MISMATCH_FOR_PLAYER" ) );
        HandleExitRequest();
    end
end
Events.PlayerVersionMismatchEvent.Add( OnVersionMismatch );


-----------------------------------------------------------------
-- Adjust for resolution
-----------------------------------------------------------------
function AdjustScreenSize()
    local screenX, screenY = UIManager:GetScreenSizeVal();
    
    local TOP_COMPENSATION = 52 + ((screenY - 768) * 0.3 );
    local TOP_FRAME = 108;
    local BOTTOM_COMPENSATION = 240;
    local LOCAL_SLOT_COMPENSATION = 113;

	if ( Controls.ChatBox:IsHidden() ) then
		BOTTOM_COMPENSATION = 80;
	end
	
    Controls.MainGrid:SetSizeY( screenY - TOP_COMPENSATION );
    
    Controls.ListingScrollPanel:SetSizeY( screenY - TOP_COMPENSATION - LOCAL_SLOT_COMPENSATION - BOTTOM_COMPENSATION - TOP_FRAME );
    Controls.ListingScrollPanel:CalculateInternalSize();
    
    Controls.GameOptionsSummary:SetSizeY( screenY - TOP_COMPENSATION - BOTTOM_COMPENSATION - TOP_FRAME - 8 );
    Controls.GameOptionsSummary:CalculateInternalSize();
    
    Controls.VerticalTrim:SetSizeY( screenY - TOP_COMPENSATION - BOTTOM_COMPENSATION - TOP_FRAME );
    -- Ingame Civ Drafter START
    local screenXcap = math.min(screenX, 1920)
    local screenYcap = math.min(screenY, screenY)
    local width = math.max(1000, screenXcap * 0.77)
    Controls.DraftPopup:SetSizeY( screenYcap - TOP_COMPENSATION + 50 )
    Controls.DraftPopup:SetSizeX( width )
    Controls.DraftCivPicker:SetSizeY( screenYcap - TOP_COMPENSATION - 334 )
    Controls.DraftCivPicker:SetSizeX( width - 56 )
    Controls.DraftCivPickerFrame:SetSizeY( screenYcap - TOP_COMPENSATION - 330 )
    Controls.DraftCivPickerFrame:SetSizeX( width - 52 )
    Controls.DraftCivScrollPanel:SetSizeY( screenYcap - TOP_COMPENSATION - 336 )
    Controls.DraftCivScrollPanel:SetSizeX( width - 52 )
    Controls.DraftCivStack:SetWrapWidth( width - 52 )
    Controls.DraftCivScrollPanel:CalculateInternalSize();
    Controls.DraftCivScrollBar:SetSizeY(screenYcap - TOP_COMPENSATION - 381)
	Controls.DraftCivScrollPanel:SetScrollValue( 0 );
    Controls.DraftPlayersStatus:SetSizeY( screenYcap - TOP_COMPENSATION - 334 )
    Controls.DraftPlayersStatus:SetSizeX( width - 56 )
    Controls.DraftPlayersStatusFrame:SetSizeY( screenYcap - TOP_COMPENSATION - 330 )
    Controls.DraftPlayersStatusFrame:SetSizeX( width - 52 )
    Controls.DraftPlayersStatusScrollPanel:SetSizeY( screenYcap - TOP_COMPENSATION - 350 )
    Controls.DraftPlayersStatusScrollPanel:SetSizeX( width - 52 )
    Controls.DraftPlayersStatusStack:SetWrapWidth( width - 52 )
    Controls.DraftPlayersStatusScrollPanel:CalculateInternalSize();
    Controls.DraftPlayersStatusScrollBar:SetSizeY(screenYcap - TOP_COMPENSATION - 386)
	Controls.DraftPlayersStatusScrollPanel:SetScrollValue( 0 );
    Controls.DraftCivPicker:SetSizeY( screenYcap - TOP_COMPENSATION - 384 )
    Controls.DraftStatusBox:SetSizeX( width - 56 )
    Controls.DraftYourBansScrollPanel:SetSizeX( width - 52 )
    Controls.DraftYourBansScrollPanel:CalculateInternalSize();
    -- Ingame Civ Drafter END
end


-------------------------------------------------
-------------------------------------------------
function OnUpdateUI( type, tag, iData1, iData2, strData1 )
    if( type == SystemUpdateUIType.ScreenResize ) then
        AdjustScreenSize();
    end
end
Events.SystemUpdateUI.Add( OnUpdateUI );

-------------------------------------------------
function OnAbandoned(eReason)
	if( UI.GetCurrentGameState() == GameStateTypes.CIV5_GS_MAIN_MENU ) then
		-- Still in the front end
		if (eReason == NetKicked.BY_HOST) then
			Events.FrontEndPopup.CallImmediate( "TXT_KEY_MP_KICKED" );
		elseif (eReason == NetKicked.NO_HOST) then
			Events.FrontEndPopup.CallImmediate( "TXT_KEY_MP_HOST_LOST" );
		else
			Events.FrontEndPopup.CallImmediate( "TXT_KEY_MP_NETWORK_CONNECTION_LOST");
		end
		HandleExitRequest();
	end
end
Events.MultiplayerGameAbandoned.Add( OnAbandoned );

-------------------------------------------------
AdjustScreenSize();


-- Community Remarks START
-- populate tables from the database
for i in GameInfo.PlayerMMRoleTypes() do
	g_MMRoleTypes[i.ID] =
		{
			Type = i.Type,
			Description = Locale.ConvertTextKey(i.Description),
			Tooltip = Locale.ConvertTextKey(i.TooltipText),
		};
end
for i in GameInfo.CommunityPlayerRemarks() do
	local netID = i.PlayerNetID:sub(2);
	local date = os.date('%d %b %Y ', i.EpochTimestamp);
	if g_CommunityRemarks[netID] == nil then
		g_CommunityRemarks[netID] = {};
	end
	table.insert(g_CommunityRemarks[netID],
		{
			NetID = netID,
			Name = i.PlayerLastObservedName,
			Role = g_MMRoleTypes[i.PlayerMMRoleType] and g_MMRoleTypes[i.PlayerMMRoleType].Description or '??',
			Tooltip = '[ICON_BULLET]' .. date .. Locale.ConvertTextKey('TXT_KEY_COMMUNITY_REMARKS_PLAYERNAME', i.PlayerLastObservedName) .. ': ' .. (g_MMRoleTypes[i.PlayerMMRoleType] and g_MMRoleTypes[i.PlayerMMRoleType].Tooltip or '??'),
		});
end
-- real netID from the dll (only for non-local MP games)
function GetNetID( iPlayerID )
	if iPlayerID >= 0 and iPlayerID < 2 ^ 28 then
		local product = (2 ^ 28) + iPlayerID;
    	return PreGame.GetNickName(product);
    end
end
-- Community Remarks END

-- Emote Picker Menu START
Controls.EmotePickerButton:RegisterCallback( Mouse.eLClick, function() Controls.ChatEntry:TakeFocus(); Controls.EmotePicker:SetHide(not Controls.EmotePicker:IsHidden()) end );
for i in GameInfo.IconFontMapping('1 ORDER BY LENGTH(IconFontTexture), rowid') do
    local ins = {};
    ContextPtr:BuildInstanceForControl('EmoteInstance', ins, Controls.EmoteStack);
    local emoteText = string.format('[%s]', i.IconName);
    ins.EmoteButton:SetText(emoteText);
    ins.EmoteButton:RegisterCallback( Mouse.eLClick, function() Controls.ChatEntry:TakeFocus(); Controls.ChatEntry:SetText(Controls.ChatEntry:GetText() .. emoteText); UI.PostKeyMessage(35); return true; end );
end
Controls.EmotesScrollPanel:ReprocessAnchoring()
Controls.EmotesScrollPanel:CalculateInternalSize()
-- Emote Picker Menu END

-------------------------------------------------
-- INGAME CIV DRAFTER
-------------------------------------------------

function RebuildDrafts()
	print('rebuild drafts')
	numBans = PreGame.GetGameOption("GAMEOPTION_TOURNAMENT_MODE") > 0 and 1 or 2;
	minBans = PreGame.GetGameOption("GAMEOPTION_TOURNAMENT_MODE") > 0 and 1 or 0;
	g_DraftResultInstances = { Bans = {}, BansD = {}, BansT = {}, Picks = {}, PicksD = {}, PicksB = {}, PicksBD = {}, PicksT = {} };
	g_DraftCivInstances = {};
	g_BannedCivs = {};
	g_SelectedCivs = {};
	g_CivEntries = {};
	for row in DB.Query([[SELECT 
							Civilizations.ID as CivID,
							Leaders.ID as LeaderID,
							Civilizations.ShortDescription as CivShortDescription,
							Leaders.Description as LeaderDescription
							FROM Civilizations, Leaders, Civilization_Leaders
							WHERE
								Civilizations.Playable = 1 AND
								Civilizations.Type = Civilization_Leaders.CivilizationType AND
								Leaders.Type = Civilization_Leaders.LeaderheadType
						]]) do
		table.insert(g_CivEntries, {ID = row.CivID, LeaderID = row.LeaderID, ShortDescription = Locale.Lookup(row.CivShortDescription)});						
	end
	-- Sorting Civs by Short Description
	table.sort(g_CivEntries, function(a,b)
		local astr = Locale.ConvertTextKey(a.ShortDescription);
		local bstr = Locale.ConvertTextKey(b.ShortDescription);
		local a0, b0
		if astr:match("The ") then
			a0 = astr:sub(5)
		else
			a0 = astr
		end
		if bstr:match("The ") then
			b0 = bstr:sub(5)
		else
			b0 = bstr
		end
		return a0 < b0
	end);
	-- Sorting Civs by Short Description END

	Controls.DraftCivStack:DestroyAllChildren();
	--Controls.DraftStatusStack:DestroyAllChildren();
	for i,v in ipairs(g_CivEntries) do
		local instance = {};
		ContextPtr:BuildInstanceForControl( "DraftCivEntry", instance, Controls.DraftCivStack );
		g_DraftCivInstances[v.ID] = instance;
		instance.CivName:SetText(v.ShortDescription);
		instance.DraftCivEntryButton:SetVoids(v.ID, v.LeaderID);
		instance.DraftCivEntryButton:RegisterCallback( Mouse.eLClick, doCivSelectionHighlight );
	
		local leader = GameInfo.Leaders[v.LeaderID];
		local civ = GameInfo.Civilizations[v.ID];
		IconHookup(leader.PortraitIndex, 128, leader.IconAtlas, instance.LeaderIcon); 
		IconHookup(civ.PortraitIndex, 45, civ.IconAtlas, instance.LeaderSubIcon);
		local trait = GameInfo.Leader_Traits("LeaderType ='" .. leader.Type .. "'")().TraitType;
		local bonusDescTbl = {};
		for row in DB.Query([[SELECT ID, Description, PortraitIndex, IconAtlas from Units INNER JOIN 
							Civilization_UnitClassOverrides ON Units.Type = Civilization_UnitClassOverrides.UnitType 
							WHERE Civilization_UnitClassOverrides.CivilizationType = ? AND
							Civilization_UnitClassOverrides.UnitType IS NOT NULL]], civ.Type) do
			table.insert(bonusDescTbl, Locale.Lookup(row.Description));
		end
		
		for row in DB.Query([[SELECT ID, Description, PortraitIndex, IconAtlas from Buildings INNER JOIN 
							Civilization_BuildingClassOverrides ON Buildings.Type = Civilization_BuildingClassOverrides.BuildingType 
							WHERE Civilization_BuildingClassOverrides.CivilizationType = ? AND
							Civilization_BuildingClassOverrides.BuildingType IS NOT NULL]], civ.Type) do
			table.insert(bonusDescTbl, Locale.Lookup(row.Description));
		end
			
		for row in DB.Query([[SELECT ID, Description, PortraitIndex, IconAtlas from Improvements where CivilizationType = ?]], civ.Type) do
			table.insert(bonusDescTbl, Locale.Lookup(row.Description));
		end
		local bonusText = table.concat(bonusDescTbl, ', ');
		local strCivBonus = string.format("[COLOR_POSITIVE_TEXT]%s:[ENDCOLOR] %s[NEWLINE][COLOR_POSITIVE_TEXT]%s[ENDCOLOR]",
			Locale.ConvertTextKey( GameInfo.Traits[trait].ShortDescription ), Locale.ConvertTextKey( GameInfo.Traits[trait].Description ), bonusText or "")
		g_CivEntries[i].bonusText = strCivBonus;
		instance.DraftCivEntryButton:LocalizeAndSetToolTip( strCivBonus );
	end
	Controls.DraftCivStack:CalculateSize();
	Controls.DraftCivScrollPanel:CalculateInternalSize();
	-- draft results

	Controls.DraftPlayersStatusStack:DestroyAllChildren();
	for i = 0, GameDefines.MAX_MAJOR_CIVS - 1 do

		local instance = {};

		local playerEntry = g_PlayerEntries[iPlayer];
		if (playerEntry == nil) then
			playerEntry = {}
			g_PlayerEntries[i] = playerEntry
			g_DummyInstancies[i] = {}
		
			playerEntry.statusInstance = {}
			ContextPtr:BuildInstanceForControl( 'DraftPlayerStatusEntry', playerEntry.statusInstance, Controls.DraftPlayersStatusStack );
			ContextPtr:BuildInstanceForControl( 'DummyInstance', g_DummyInstancies[i], Controls.DraftPlayersStatusStack );
			--g_DummyInstancies[i].Root:SetSizeX(0)
			playerEntry.statusInstance.PlayerName:LocalizeAndSetText('TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME', i)

			playerEntry.bansIM = InstanceManager:new("DraftCivEntrySmall", "DraftCivEntryButton", playerEntry.statusInstance.DraftPlayerBansStack)
			playerEntry.bansIMD = InstanceManager:new("Dummy2Instance", "DummyButton", playerEntry.statusInstance.DraftPlayerBansDummy)
			playerEntry.picksIM = InstanceManager:new("DraftCivEntrySmall", "DraftCivEntryButton", playerEntry.statusInstance.DraftPlayerPicksStack)
			playerEntry.picksIMD = InstanceManager:new("Dummy2Instance", "DummyButton", playerEntry.statusInstance.DraftPlayerPicksDummy)
			playerEntry.picksBigIM = InstanceManager:new("DraftCivEntry", "DraftCivEntryButton", playerEntry.statusInstance.DraftPlayerPicksBigStack)
			playerEntry.picksBigIMD = InstanceManager:new("Dummy3Instance", "DummyButton", playerEntry.statusInstance.DraftPlayerPicksBigDummy)
		else
			playerEntry.bansIM:ResetInstances()
			playerEntry.bansIMD:ResetInstances()
			playerEntry.picksIM:ResetInstances()
			playerEntry.picksIMD:ResetInstances()
			playerEntry.picksBigIM:ResetInstances()
			playerEntry.picksBigIMD:ResetInstances()
			playerEntry.statusInstance.Root:SetHide(false)
			g_DummyInstancies[i].Root:SetHide(false)
		end

		Controls.DraftButton:LocalizeAndSetToolTip('TXT_KEY_DRAFTS_HELP', numBans, numPicks)
		local DraftCivEntrySmall_WIDTH = 70
		if g_DraftResultInstances.BansT[i] == nil then
			g_DraftResultInstances.BansT[i] = {};
			g_DraftResultInstances.PicksT[i] = {};
		end
		for _ = 1, numBans do
			local inst = playerEntry.bansIM:GetInstance()
			table.insert(g_DraftResultInstances.BansT[i], inst);
			IconHookup( 22, 64, "LEADER_ATLAS", inst.LeaderIcon );
			SimpleCivIconHookup(-1, 32, inst.LeaderSubIcon);
			inst.LeaderFrame:SetColor( {x=1, y=50/255, z=50/255, w=210/255 } )
			inst.LeaderIcon:SetColor( {x=50/255, y=50/255, z=50/255, w=1 } )
			inst.LeaderSubIconFrame:SetColor( {x=1, y=50/255, z=50/255, w=210/255 } )
			inst.LeaderSubIcon:SetColor( {x=150/255, y=150/255, z=150/255, w=1 } )
			local w = math.min( DraftCivEntrySmall_WIDTH, playerEntry.statusInstance.DraftPlayerStatusFrame:GetSizeX() / numBans )
			inst.Root:SetSizeX(w)
		end
		for _ = 1, numPicks do
			local inst = playerEntry.picksIM:GetInstance()
			table.insert(g_DraftResultInstances.PicksT[i], inst);
			IconHookup( 22, 64, "LEADER_ATLAS", inst.LeaderIcon );
			SimpleCivIconHookup(-1, 32, inst.LeaderSubIcon);
			local w = math.min( DraftCivEntrySmall_WIDTH, playerEntry.statusInstance.DraftPlayerStatusFrame:GetSizeX() / numPicks )
			inst.Root:SetSizeX(w)
		end
		playerEntry.statusInstance.DraftPlayerBansStack:CalculateSize();
		playerEntry.statusInstance.DraftPlayerBansStack:ReprocessAnchoring();
		playerEntry.statusInstance.DraftPlayerPicksStack:CalculateSize();
		playerEntry.statusInstance.DraftPlayerPicksStack:ReprocessAnchoring();
	end
	Controls.DraftPlayersStatusStack:CalculateSize();
	Controls.DraftPlayersStatusScrollPanel:CalculateInternalSize();
	if not Controls.DraftConfirmBansButton:IsDisabled() then
		Controls.DraftConfirmBansButton:SetDisabled(true);
	end
	Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_HELP')
end

function ResetDrafts( message )
	print('reset drafts', message)
	g_DraftProgress = 'DRAFT_PROGRESS_INIT'
	ClearSelection();
	ClearBans();
	UpdateDraftScreen();
	Controls.DraftBGBlock:SetHide(true)
	if ContextPtr:IsHidden() == false and message then
		AddDraftStatus(Locale.Lookup(message));
		AddDraftStatus('------------------------------[NEWLINE][NEWLINE]');
		OnChat( 0, -1, Locale.Lookup(message) );
	end
end

function PopulateDrafts()
	print('populate drafts')
	local product = 2 * 2 ^ 28;  -- request draft data
	local data = PreGame.GetNickName(product);
	print('req data', data)
	if data then
		local control, text = string.match(data, '(.-)|(.*)');
		if control == 'DRAFT_PROGRESS_DEFAULT' then
			print('--- DRAFT_PROGRESS_DEFAULT');
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_DEFAULT');
			Controls.DraftPlayersStatus:SetHide(false);
			Controls.DraftCivPicker:SetHide(true);
			if not Controls.DraftConfirmBansButton:IsDisabled() then
				Controls.DraftConfirmBansButton:SetDisabled(true);
			end
			Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_HELP')
		elseif control == 'DRAFT_PROGRESS_INIT' then
			print('--- DRAFT_PROGRESS_INIT');
			AddDraftStatus('------------------------------');
			AddDraftStatus(Locale.Lookup('TXT_KEY_DRAFT_STATUS_PROGRESS_INIT'));
			g_DraftProgress = control;
			g_DraftBansTable = {}
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_INIT');
			Controls.DraftPlayersStatus:SetHide(false);
			Controls.DraftCivPicker:SetHide(true);
			if not Controls.DraftConfirmBansButton:IsDisabled() then
				Controls.DraftConfirmBansButton:SetDisabled(true);
			end
			Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_HELP')
			local p = 0;
			for bReady in string.gmatch(text, '([^,]+)') do
				if bReady == '1' then
					if g_PlayerEntries[p] then
						g_PlayerEntries[p].readyForDrafts = true;
						g_PlayerEntries[p].statusInstance.DraftPlayerStatusBlackFade:SetHide( true );
					end
				else
				end
				p = p + 1;
			end
		elseif control == 'DRAFT_PROGRESS_BANS' then
			print('--- DRAFT_PROGRESS_BANS');
			AddDraftStatus(Locale.Lookup('TXT_KEY_DRAFT_STATUS_PROGRESS_BANS'));
			g_DraftProgress = control;
			if len(g_DraftBansTable[Matchmaking.GetLocalID()] or {}) == 0 then
				Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS', numBans, len(g_SelectedCivs));
			else
				Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS2');
			end
			Controls.DraftPlayersStatus:SetHide(true);
			Controls.DraftCivPicker:SetHide(false);
			Controls.DraftConfirmBansButton:SetHide(false);
			for p, ban in string.gmatch(text, '(.-):(.-);') do
				print('p', p, 'ban', ban)
				local pName = Matchmaking.GetPlayerList()[tonumber(p) + 1].playerName or Locale.Lookup('TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME', tonumber(p) + 1);
				AddPlayerBans(tonumber(p), pName, ban);
				if Matchmaking.GetLocalID() == tonumber(p) and not Matchmaking.IsHost() then
					if len(g_SelectedCivs) < minBans or (not Matchmaking.IsHost() and len(g_SelectedCivs) > numBans) then
						if not Controls.DraftConfirmBansButton:IsDisabled() then
							Controls.DraftConfirmBansButton:SetDisabled(true);
						end
						Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_HELP')
					elseif not Matchmaking.IsHost() and g_DraftBansTable[Matchmaking.GetLocalID()] ~= nil then
						if not Controls.DraftConfirmBansButton:IsDisabled() then
							Controls.DraftConfirmBansButton:SetDisabled(true);
						end
						Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_TT2')
					else
						Controls.DraftConfirmBansButton:SetDisabled(false);
						local civtbl = {}
						for k,v in pairs(g_SelectedCivs) do
							table.insert(civtbl, GameInfo.Civilizations[k] and Locale.Lookup(GameInfo.Civilizations[k].ShortDescription))
						end
						table.sort(civtbl, function(astr,bstr)
							local a0, b0
							if astr:match("The ") then a0 = astr:sub(5) else a0 = astr end
							if bstr:match("The ") then b0 = bstr:sub(5) else b0 = bstr end
							return a0 < b0
						end);
						Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_TT', next(civtbl) and table.concat(civtbl, ', ') or '[ENDCOLOR]{TXT_KEY_MP_NO_PROPOSAL}')
					end
				end
			end
		elseif control == 'DRAFT_PROGRESS_BUSY' then
			print('--- DRAFT_PROGRESS_BUSY');
			AddDraftStatus(Locale.Lookup('TXT_KEY_DRAFT_STATUS_PROGRESS_BUSY'));
			g_DraftProgress = control;
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BUSY');
			Controls.DraftPlayersStatus:SetHide(true);
			Controls.DraftCivPicker:SetHide(false);
			if not Controls.DraftConfirmBansButton:IsDisabled() then
				Controls.DraftConfirmBansButton:SetDisabled(true);
			end
			Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_HELP')
			for p, ban in string.gmatch(text, '(.-):(.-);') do
				print('p', p, 'ban', ban)
				local pName = Matchmaking.GetPlayerList()[tonumber(p) + 1].playerName or Locale.Lookup('TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME', tonumber(p) + 1);
				AddPlayerBans(tonumber(p), pName, ban);
			end
		elseif control == 'DRAFT_PROGRESS_OVER' then
			print('--- DRAFT_PROGRESS_OVER');
			g_DraftProgress = control;
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_OVER');
			Controls.DraftPlayersStatus:SetHide(false);
			Controls.DraftCivPicker:SetHide(true);
			if not Controls.DraftConfirmBansButton:IsDisabled() then
				Controls.DraftConfirmBansButton:SetDisabled(true);
			end
			Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_HELP')
			local bans, text = string.match(text, '(.-)|(.*)');
			for p, ban in string.gmatch(bans, '(.-):(.-);') do
				print('p', p, 'ban', ban);
				local pName = Matchmaking.GetPlayerList()[tonumber(p) + 1].playerName or Locale.Lookup('TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME', tonumber(p) + 1);
				AddPlayerBans(tonumber(p), pName, ban);
			end
			AddDraftStatus(Locale.Lookup('TXT_KEY_DRAFT_STATUS_RESULT'));
			AssignDraftedCivs( text );
		end
	end
	UpdateDraftCivButtons();
	UpdateDraftScreen();
end

function IsReadyForDraft( playerID )
	if playerID >= 0 and playerID < GameDefines.MAX_MAJOR_CIVS then
		local product = 3 * 2 ^ 28;  -- request s_draftPlayersReady
		local data = PreGame.GetNickName(product);
		local bitmap = tonumber(data)
		if bitmap < 0 then bitmap = bitmap + 2 ^ 32 end
		--print('bitmap', bitmap, playerID, math.floor(bitmap / 2 ^ playerID) % 2)
		return math.floor(bitmap / 2 ^ playerID) % 2 > 0
	end
	return false
end

function LaunchDrafts()
	if ( Controls.DraftBGBlock:IsHidden() ) then
		Controls.DraftBGBlock:SetHide(false);
		if g_DraftProgress == 'DRAFT_PROGRESS_DEFAULT' or g_DraftProgress == 'DRAFT_PROGRESS_INIT' then
			local playerID = Matchmaking.GetLocalID();
			local product = 3 * 2 ^ 28 + playerID;  -- send player ready (local)
			PreGame.SetLeaderKey( product, '' );
			--Network.BroadcastGameSettings();
		end
	else
		Controls.DraftBGBlock:SetHide(true);
	end
end
Controls.DraftButton:RegisterCallback( Mouse.eLClick, LaunchDrafts );

function OnDraftsClose()
	Controls.DraftBGBlock:SetHide(true);
end
Controls.DraftCloseButton:RegisterCallback( Mouse.eLClick, OnDraftsClose )


function sendSelectedCivs()
	if not Controls.DraftConfirmBansButton:IsDisabled() then
		Controls.DraftConfirmBansButton:SetDisabled(true);
	end
	Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_TT2')
	local list = {};
	for k,v in pairs(g_SelectedCivs) do
		table.insert(list, k);
	end
	local civs = table.concat(list, ', ') or '';
	ClearSelection();
	print('send bans', civs)
	Network.SendRenameCity(-2, civs);  -- send bans
end
Controls.DraftConfirmBansButton:RegisterCallback( Mouse.eLClick, sendSelectedCivs );

function len(T)
  local k = 0
  for _ in next, T do k = k + 1 end
  return k
end

function append(tbl, child)
  tbl[#tbl] = child;
end

function doCivBan(civId, leaderId)
	local instance = g_DraftCivInstances[civId];
	if instance then
		if g_SelectedCivs[civId] then
			doCivSelectionHighlight(civId, nil, nil, nil, nil, true);
		end
		if g_BannedCivs[civId] then
			g_BannedCivs[civId] = nil;
			instance.LeaderFrame:SetColor( {x=1, y=1, z=1, w=1 } )
			instance.LeaderIcon:SetColor( {x=200/255, y=200/255, z=200/255, w=1 } )
			instance.LeaderSubIconFrame:SetColor( {x=1, y=1, z=1, w=1 } )
			instance.LeaderSubIcon:SetColor( {x=1, y=1, z=1, w=1 } )
			instance.CivName:SetColor( {x=1, y=1, z=200/255, w=1 }, 0 )
		else
			g_BannedCivs[civId] = true;
			instance.LeaderFrame:SetColor( {x=1, y=50/255, z=50/255, w=210/255 } )
			instance.LeaderIcon:SetColor( {x=50/255, y=50/255, z=50/255, w=1 } )
			instance.LeaderSubIconFrame:SetColor( {x=1, y=50/255, z=50/255, w=210/255 } )
			instance.LeaderSubIcon:SetColor( {x=150/255, y=150/255, z=150/255, w=1 } )
			instance.CivName:SetColor( {x=1, y=50/255, z=50/255, w=180/255 }, 0 )
		end
	end
end

function doCivSelectionHighlight(civId, leaderId, v3,v4,v5, force)
	local instance = g_DraftCivInstances[civId];
	if g_SelectedCivs[civId] then
		g_SelectedCivs[civId] = nil;
		instance.LeaderFrame:SetColor( {x=1, y=1, z=1, w=1 } )
		instance.LeaderFrameSelectedAnim:SetHide(true)
		instance.LeaderIcon:SetColor( {x=200/255, y=200/255, z=200/255, w=1 } )
		instance.LeaderSubIconFrameSelectedAnim:SetHide(true)
		instance.LeaderSubIcon:SetColor( {x=1, y=1, z=1, w=1 } )
		instance.CivName:SetColor( {x=1, y=1, z=200/255, w=1 }, 0 )
	else
		if force or Matchmaking.IsHost() or len(g_SelectedCivs) < numBans then
			g_SelectedCivs[civId] = true;
			instance.LeaderFrame:SetColor( {x=200/255, y=200/255, z=50/255, w=1 } )
			instance.LeaderFrameSelectedAnim:SetHide(false)
			instance.LeaderIcon:SetColor( {x=1, y=225/255, z=50/255, w=1 } )
			instance.CivName:SetColor( {x=1, y=225/255, z=50/255, w=1 }, 0 )
			instance.LeaderSubIconFrameSelectedAnim:SetHide(false)
		end
	end
	if g_DraftProgress == 'DRAFT_PROGRESS_BANS' then
		if len(g_SelectedCivs) >= minBans and (Matchmaking.IsHost() or len(g_SelectedCivs) <= numBans) then
				Controls.DraftConfirmBansButton:SetDisabled(false);
				local civtbl = {}
				for k,v in pairs(g_SelectedCivs) do
					table.insert(civtbl, GameInfo.Civilizations[k] and Locale.Lookup(GameInfo.Civilizations[k].ShortDescription))
				end
				table.sort(civtbl, function(astr,bstr)
					local a0, b0
					if astr:match("The ") then a0 = astr:sub(5) else a0 = astr end
					if bstr:match("The ") then b0 = bstr:sub(5) else b0 = bstr end
					return a0 < b0
				end);
				Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_TT', next(civtbl) and table.concat(civtbl, ', ') or '[ENDCOLOR]{TXT_KEY_MP_NO_PROPOSAL}')
		else
			if not Controls.DraftConfirmBansButton:IsDisabled() then
				Controls.DraftConfirmBansButton:SetDisabled(true);
			end
			Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_HELP')
		end
		if len(g_DraftBansTable[Matchmaking.GetLocalID()] or {}) == 0 then
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS', numBans, len(g_SelectedCivs));
		else
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS2');
		end
	end
	UpdateDraftCivButtons()
end

function AddDraftStatus( text )
	local instance = {};
	ContextPtr:BuildInstanceForControl( "ChatEntry", instance, Controls.DraftStatusStack );
	instance.String:SetText( text );
	instance.Box:SetSizeY( instance.String:GetSizeY() + 15 );
	instance.Box:ReprocessAnchoring();

	Controls.DraftStatusStack:CalculateSize();
	Controls.DraftYourBansScrollPanel:CalculateInternalSize();
	Controls.DraftYourBansScrollPanel:SetScrollValue( 1 );
end

bb = {}
pp = {}
ppB = {}
function AddPlayerBans(playerID, playerName, strCivs)
	print('AddPlayerBans', playerID, playerName,strCivs)
	local bansTbl = {};
	local banIdsTbl = {};
	local playerEntry = g_PlayerEntries[playerID];
	local DraftCivEntrySmall_WIDTH = 60;
	if g_DraftResultInstances.Bans[playerID] == nil then
		g_DraftResultInstances.Bans[playerID] = {};
		g_DraftResultInstances.BansD[playerID] = {};
	end
	if len(g_DraftResultInstances.BansT[playerID] or {}) > 0 then
		g_DraftResultInstances.BansT[playerID] = {}
		playerEntry.bansIM:ResetInstances();
		playerEntry.bansIMD:ResetInstances();
	end
	if g_DraftBansTable[playerID] == nil then
		g_DraftBansTable[playerID] = {}
	end
	for civId in string.gmatch(strCivs, '([^,]+)') do
		local civ = GameInfo.Civilizations[tonumber(civId)];
		if civ then
			civId = tonumber(civId);
			--print(civ.ShortDescription);
			doCivBan(civId);
			g_DraftBansTable[playerID][civId] = g_BannedCivs[civId]
			table.insert(bansTbl, Locale.Lookup(civ.ShortDescription))
			table.insert(banIdsTbl, civId)
			local leader, bonusText
			if playerEntry then
				for i,ce in ipairs(g_CivEntries) do
					if civId == ce.ID then
						leader = GameInfo.Leaders[ce.LeaderID];
						bonusText = string.format("%s[NEWLINE]%s", ce.ShortDescription, ce.bonusText);  -- include civ name
					end
				end

				local inst = playerEntry.bansIM:GetInstance();
				table.insert(g_DraftResultInstances.Bans[playerID], inst);
				IconHookup(leader.PortraitIndex, 64, leader.IconAtlas, inst.LeaderIcon); 
				IconHookup(civ.PortraitIndex, 32, civ.IconAtlas, inst.LeaderSubIcon);
				inst.LeaderFrame:SetColor( {x=1, y=50/255, z=50/255, w=210/255 } )
				inst.LeaderIcon:SetColor( {x=50/255, y=50/255, z=50/255, w=1 } )
				inst.LeaderSubIconFrame:SetColor( {x=1, y=50/255, z=50/255, w=210/255 } )
				inst.LeaderSubIcon:SetColor( {x=150/255, y=150/255, z=150/255, w=1 } )

				local dummy = playerEntry.bansIMD:GetInstance();
				dummy.DummyButton:LocalizeAndSetToolTip( bonusText );
				table.insert(g_DraftResultInstances.BansD[playerID], dummy);
			end
		end
	end
	-- recalculate width for all items to make them fit in the stack
	for i, inst in next, g_DraftResultInstances.Bans[playerID] do
		local tsize = len(g_DraftResultInstances.Bans[playerID])
		local width = playerEntry.statusInstance.DraftPlayerStatusFrame:GetSizeX()
		local w1 = math.min( DraftCivEntrySmall_WIDTH, width / tsize )
		local w2 = (width - DraftCivEntrySmall_WIDTH) / (tsize - 1)
		local d1 = (width - DraftCivEntrySmall_WIDTH * 1.5) / (tsize - 2)
		
		local dummy = g_DraftResultInstances.BansD[playerID][i]
		if w1 < DraftCivEntrySmall_WIDTH then
			if i == 1 then  -- first item: half the original width
				dummy.Root:SetSizeX(DraftCivEntrySmall_WIDTH / 2)
				dummy.DummyButton:SetSizeX(DraftCivEntrySmall_WIDTH / 2 + 1)
			elseif i == tsize then  -- last item: always full width
				dummy.Root:SetSizeX(DraftCivEntrySmall_WIDTH)
				dummy.DummyButton:SetSizeX(DraftCivEntrySmall_WIDTH + 1)
			else
				dummy.Root:SetSizeX(d1)
				dummy.DummyButton:SetSizeX(d1 + 1)
			end
			dummy.DummyButton:RegisterCallback(Mouse.eMouseEnter, function()
				if i < tsize then
					bb[i] = true
					for j, inst2 in next, g_DraftResultInstances.Bans[playerID] do
						if j == i then
							inst2.Root:SetSizeX(math.max(w2, DraftCivEntrySmall_WIDTH / 1.5));
						else
							inst2.Root:SetSizeX(w2 - (math.max(w2, DraftCivEntrySmall_WIDTH / 1.5) - w2) / (tsize - 2));
						end
					end
				end
			end);
			dummy.DummyButton:RegisterCallback(Mouse.eMouseExit, function()
				bb[i] = false
				local bOut = true  -- check if cursor is outside for all dummies
				for j in next, bb do
					if bb[j] then
						bOut = false
					end
				end
				if bOut then  -- if so, resize icons evenly
					for j, inst2 in next, g_DraftResultInstances.Bans[playerID] do
						inst2.Root:SetSizeX(w2);
					end
				end
			end);
			playerEntry.statusInstance.DraftPlayerBansStack:SetAnchor('L,T');
			inst.Root:SetSizeX(w2)
		else
			dummy.Root:SetSizeX(w1)
			dummy.DummyButton:SetSizeX(w1 + 1)
			inst.Root:SetSizeX(w1)
		end
	end
	playerEntry.statusInstance.DraftPlayerBansStack:CalculateSize();
	playerEntry.statusInstance.DraftPlayerBansStack:ReprocessAnchoring();
	playerEntry.statusInstance.DraftPlayerBansDummy:CalculateSize();
	playerEntry.statusInstance.DraftPlayerBansDummy:ReprocessAnchoring();

	local s = Locale.Lookup('TXT_KEY_DRAFT_STATUS_PLAYER_BAN', playerName, next(bansTbl) and table.concat(bansTbl, ', ') or '[ENDCOLOR]{TXT_KEY_MP_NO_PROPOSAL}');
	AddDraftStatus(s);
end

function AssignDraftedCivs( civsArr )
	print('AssignDraftedCivs')
	local totalPlayers = 0;
	local playerSlotsTbl = {};
	for i = 0, GameDefines.MAX_MAJOR_CIVS - 1 do
		if( PreGame.GetSlotStatus( i ) == SlotStatus.SS_TAKEN and PreGame.GetSlotClaim( i ) == SlotClaim.SLOTCLAIM_ASSIGNED) then
			table.insert(playerSlotsTbl, i);
			totalPlayers = totalPlayers + 1;
		end
	end
	-- assign X civs for each player
	local playerToCivsTbl = {};
	local iSlot = 0;
	for civId in string.gmatch(civsArr, '([^,]+)') do
		local civ = GameInfo.Civilizations[tonumber(civId)];
		if civ then
			if playerToCivsTbl[iSlot + 1] == nil then
				playerToCivsTbl[iSlot + 1] = {};
			end
			if #playerToCivsTbl[iSlot + 1] < numPicks then
				table.insert(playerToCivsTbl[iSlot + 1], civId);
			end
			iSlot = (iSlot + 1) % totalPlayers;
		end
	end
	for i, civs in next, playerToCivsTbl do
		print('player', playerSlotsTbl[i], 'civs', table.concat(civs, ', '))
		local t = '';
		local playerID = playerSlotsTbl[i];
		local playerEntry = g_PlayerEntries[playerID];
		local DraftCivEntrySmall_WIDTH = 60;
		local DraftCivEntryBig_WIDTH = 120
		if g_DraftResultInstances.Picks[playerID] == nil then
			g_DraftResultInstances.Picks[playerID] = {};
			g_DraftResultInstances.PicksD[playerID] = {};
			g_DraftResultInstances.PicksB[playerID] = {};  -- Bigger Pick buttons for local player
			g_DraftResultInstances.PicksBD[playerID] = {};
		end
		if len(g_DraftResultInstances.PicksT[playerID]) > 0 then
			g_DraftResultInstances.PicksT[playerID] = {}
			playerEntry.picksIM:ResetInstances();
			playerEntry.picksIMD:ResetInstances();
			playerEntry.picksBigIM:ResetInstances();
			playerEntry.picksBigIMD:ResetInstances();
		end
		for k, v in next, civs do
			local leader, civ, bonusText
			if playerEntry then
				for i,ce in ipairs(g_CivEntries) do
					if tonumber(v) == ce.ID then
						leader = GameInfo.Leaders[ce.LeaderID];
						civ = GameInfo.Civilizations[ce.ID];
						bonusText = string.format("%s[NEWLINE]%s", ce.ShortDescription, ce.bonusText);  -- include civ name
					end
				end
				local inst = playerEntry.picksIM:GetInstance()
				inst.civID = tonumber(v)
				table.insert(g_DraftResultInstances.Picks[playerID], inst);
				IconHookup(leader.PortraitIndex, 64, leader.IconAtlas, inst.LeaderIcon); 
				IconHookup(civ.PortraitIndex, 32, civ.IconAtlas, inst.LeaderSubIcon);

				local dummy = playerEntry.picksIMD:GetInstance();
				dummy.DummyButton:LocalizeAndSetToolTip( bonusText );
				table.insert(g_DraftResultInstances.PicksD[playerID], dummy);

				local inst2 = playerEntry.picksBigIM:GetInstance()
				table.insert(g_DraftResultInstances.PicksB[playerID], inst2);
				IconHookup(leader.PortraitIndex, 128, leader.IconAtlas, inst2.LeaderIcon); 
				IconHookup(civ.PortraitIndex, 45, civ.IconAtlas, inst2.LeaderSubIcon);
				inst2.CivName:SetText( Locale.Lookup(civ.ShortDescription) )

				local dummy2 = playerEntry.picksBigIMD:GetInstance();
				dummy2.DummyButton:LocalizeAndSetToolTip( bonusText );
				dummy2.DummyButton:RegisterCallback(Mouse.eLClick, function()
					if Matchmaking.GetLocalID() == playerID then
						PreGame.SetCivilization( playerID, tonumber(v) );
						Network.BroadcastPlayerInfo();
						--UpdateDisplay();
						Controls.DraftBGBlock:SetHide(true);
					end
				end);
				table.insert(g_DraftResultInstances.PicksBD[playerID], dummy2);
			end
			civs[k] = GameInfo.Civilizations[tonumber(v)] and Locale.Lookup(GameInfo.Civilizations[tonumber(v)].ShortDescription) or '??';
		end
		for i, inst in next, g_DraftResultInstances.Picks[playerID] do
			local dummy = g_DraftResultInstances.PicksD[playerID][i]
			local tsize = len(g_DraftResultInstances.Picks[playerID])
			local width = playerEntry.statusInstance.DraftPlayerStatusFrame:GetSizeX()
			local w1 = math.min(DraftCivEntrySmall_WIDTH, width / tsize )
			local w2 = (width - DraftCivEntrySmall_WIDTH) / (tsize - 1)
			local d1 = (width - DraftCivEntrySmall_WIDTH * 1.5) / (tsize - 2)
			
			local instB = g_DraftResultInstances.PicksB[playerID][i]
			local dummyB = g_DraftResultInstances.PicksBD[playerID][i]
			local extraPad = 50
			local widthB = playerEntry.statusInstance.DraftPlayerStatusFrame:GetSizeX() + extraPad
			local w1B = math.min(DraftCivEntryBig_WIDTH, widthB / tsize )
			local w2B = (widthB - DraftCivEntryBig_WIDTH) / (tsize - 1)
			local d1B = (widthB - DraftCivEntryBig_WIDTH * 1.5) / (tsize - 2)
			if w1 < DraftCivEntrySmall_WIDTH then
				if i == 1 then
					dummy.Root:SetSizeX(DraftCivEntrySmall_WIDTH / 2)
					dummy.DummyButton:SetSizeX(DraftCivEntrySmall_WIDTH / 2 + 1)
				elseif i == tsize then
					dummy.Root:SetSizeX(DraftCivEntrySmall_WIDTH)
					dummy.DummyButton:SetSizeX(DraftCivEntrySmall_WIDTH + 1)
				else
					dummy.Root:SetSizeX(d1)
					dummy.DummyButton:SetSizeX(d1 + 1)
				end
				dummy.DummyButton:RegisterCallback(Mouse.eMouseEnter, function()
					if i < tsize then
						pp[i] = true
						for j, inst2 in next, g_DraftResultInstances.Picks[playerID] do
							if j == i then
								inst2.Root:SetSizeX(math.max(w2, DraftCivEntrySmall_WIDTH / 1.5));
							else
								inst2.Root:SetSizeX(w2 - (math.max(w2, DraftCivEntrySmall_WIDTH / 1.5) - w2) / (tsize - 2));
							end
						end
					end
				end);
				dummy.DummyButton:RegisterCallback(Mouse.eMouseExit, function()
					pp[i] = false
					local bOut = true
					for j in next, pp do
						if pp[j] then
							bOut = false
						end
					end
					if bOut then
						for j, inst2 in next, g_DraftResultInstances.Picks[playerID] do
							inst2.Root:SetSizeX(w2);
						end
					end
				end);
				playerEntry.statusInstance.DraftPlayerPicksStack:SetAnchor('L,T');
				inst.Root:SetSizeX(w2)
			else
				dummy.Root:SetSizeX(w1)
				dummy.DummyButton:SetSizeX(w1 + 1)
				inst.Root:SetSizeX(w1)
			end
			if w1B < DraftCivEntryBig_WIDTH then
				if i == 1 then
					dummyB.Root:SetSizeX(DraftCivEntryBig_WIDTH / 2)
					dummyB.DummyButton:SetSizeX(DraftCivEntryBig_WIDTH / 2 + 1)
				elseif i == tsize then
					dummyB.Root:SetSizeX(DraftCivEntryBig_WIDTH)
					dummyB.DummyButton:SetSizeX(DraftCivEntryBig_WIDTH + 1)
				else
					dummyB.Root:SetSizeX(d1B)
					dummyB.DummyButton:SetSizeX(d1B + 1)
				end
				dummyB.DummyButton:RegisterCallback(Mouse.eMouseEnter, function()
					if i < tsize then
						ppB[i] = true
						for j, inst2B in next, g_DraftResultInstances.PicksB[playerID] do
							if j == i then
								inst2B.Root:SetSizeX(math.max(w2B, DraftCivEntryBig_WIDTH / 1.5));
							else
								inst2B.Root:SetSizeX(w2B - (math.max(w2B, DraftCivEntryBig_WIDTH / 1.5) - w2B) / (tsize - 2));
							end
						end
					end
					if Matchmaking.GetLocalID() == playerID then
						g_DraftResultInstances.PicksB[playerID][i].LeaderFrameSelectedAnim:SetHide(false)
						g_DraftResultInstances.PicksB[playerID][i].LeaderSubIconFrameSelectedAnim:SetHide(false)
					end
				end);
				dummyB.DummyButton:RegisterCallback(Mouse.eMouseExit, function()
					ppB[i] = false
					local bOut = true
					for j in next, ppB do
						if ppB[j] then
							bOut = false
						end
					end
					if bOut then
						for j, inst2B in next, g_DraftResultInstances.PicksB[playerID] do
							inst2B.Root:SetSizeX(w2B);
						end
					end
					if Matchmaking.GetLocalID() == playerID then
						g_DraftResultInstances.PicksB[playerID][i].LeaderFrameSelectedAnim:SetHide(true)
						g_DraftResultInstances.PicksB[playerID][i].LeaderSubIconFrameSelectedAnim:SetHide(true)
					end
				end);
				playerEntry.statusInstance.DraftPlayerPicksBigStack:SetAnchor('L,T');
				playerEntry.statusInstance.DraftPlayerPicksBigStack:SetOffsetX(-extraPad / 2);
				instB.Root:SetSizeX(w2B)
			else
				dummyB.Root:SetSizeX(w1B)
				dummyB.DummyButton:SetSizeX(w1B + 1)
				instB.Root:SetSizeX(w1B)
				dummyB.DummyButton:RegisterCallback(Mouse.eMouseEnter, function()
					if Matchmaking.GetLocalID() == playerID then
						g_DraftResultInstances.PicksB[playerID][i].LeaderFrameSelectedAnim:SetHide(false)
						g_DraftResultInstances.PicksB[playerID][i].LeaderSubIconFrameSelectedAnim:SetHide(false)
					end
				end);
				dummyB.DummyButton:RegisterCallback(Mouse.eMouseExit, function()
					if Matchmaking.GetLocalID() == playerID then
						g_DraftResultInstances.PicksB[playerID][i].LeaderFrameSelectedAnim:SetHide(true)
						g_DraftResultInstances.PicksB[playerID][i].LeaderSubIconFrameSelectedAnim:SetHide(true)
					end
				end);
			end
		end
		playerEntry.statusInstance.DraftPlayerPicksStack:CalculateSize();
		playerEntry.statusInstance.DraftPlayerPicksStack:ReprocessAnchoring();
		playerEntry.statusInstance.DraftPlayerPicksBigStack:CalculateSize();
		playerEntry.statusInstance.DraftPlayerPicksBigStack:ReprocessAnchoring();
		playerEntry.statusInstance.DraftPlayerPicksDummy:CalculateSize();
		playerEntry.statusInstance.DraftPlayerPicksDummy:ReprocessAnchoring();
		playerEntry.statusInstance.DraftPlayerPicksBigDummy:CalculateSize();
		playerEntry.statusInstance.DraftPlayerPicksBigDummy:ReprocessAnchoring();

		local s = Locale.Lookup('TXT_KEY_DRAFT_STATUS_PLAYER_PICK', Matchmaking.GetPlayerList()[playerID + 1].playerName, next(civs) and table.concat(civs, ', ') or '[ENDCOLOR]{TXT_KEY_MP_NO_PROPOSAL}');
		AddDraftStatus(s);
	end
end

function ClearSelection()
	for it in next, g_SelectedCivs do
		doCivSelectionHighlight(it, nil, nil, nil, nil, true);
	end

	g_SelectedCivs = {};
end

function ClearBans()
	for it in next, g_BannedCivs do
		doCivBan(it);
	end

	g_DraftBansTable = {};
	g_BannedCivs = {};
end

function UpdateDraftCivButtons()
	for id, inst in next, g_DraftCivInstances do
		if g_DraftProgress == 'DRAFT_PROGRESS_BANS' and (Matchmaking.IsHost() or g_DraftBansTable[Matchmaking.GetLocalID()] == nil) then
			if g_BannedCivs[id] then
				inst.DraftCivEntryButton:SetDisabled(true);
			else
				inst.DraftCivEntryButton:SetDisabled(false);
			end
		else
			inst.DraftCivEntryButton:SetDisabled(true);
		end

		if g_SelectedCivs[id] or g_BannedCivs[id] or (not Matchmaking.IsHost() and g_DraftBansTable[Matchmaking.GetLocalID()] ~= nil) then
			inst.HoverAnim:SetHide(true);
			inst.SubHoverAnim:SetHide(true);
		else
			inst.HoverAnim:SetHide(false);
			inst.SubHoverAnim:SetHide(false);
		end
	end
end

function UpdateDraftScreen()
	print('upd drafts')
	if PreGame.IsMultiplayerGame() and Controls.DraftButton:IsHidden() and (PreGame.GetLoadFileName() == "") and not PreGame.GameStarted() then
		Controls.DraftButton:SetHide(false);
	elseif not PreGame.IsMultiplayerGame() and not Controls.DraftButton:IsHidden() then
		Controls.DraftButton:SetHide(true);
	end
	for i, fr in next, g_PlayerEntries do
		local bReady = IsReadyForDraft(i) or g_DraftProgress == 'DRAFT_PROGRESS_OVER';
		if bReady then
			if not fr.readyForDrafts then
				g_PlayerEntries[i].readyForDrafts = true;
				g_PlayerEntries[i].statusInstance.DraftPlayerStatusBlackFade:SetHide( true );
			end
		else
			if fr.readyForDrafts then
				g_PlayerEntries[i].readyForDrafts = false;
				g_PlayerEntries[i].statusInstance.DraftPlayerStatusBlackFade:SetHide( false );
			end
		end

		if g_DraftProgress == 'DRAFT_PROGRESS_OVER' then
			local playerID = i - 1
			if g_DraftResultInstances.Picks[playerID] ~= nil then
				for ind, it in next, g_DraftResultInstances.Picks[playerID] do
					if it.civID == PreGame.GetCivilization(playerID) then
						print('player civ', playerID, it.civID)
						it.LeaderFrame:SetColor( {x=94/255, y=220/255, z=31/255, w=1 } )
						it.LeaderSubIconFrame:SetColor( {x=94/255, y=220/255, z=31/255, w=1 } )
						g_DraftResultInstances.PicksB[playerID][ind].LeaderFrame:SetColor( {x=94/255, y=220/255, z=31/255, w=1 } )
						g_DraftResultInstances.PicksB[playerID][ind].LeaderSubIconFrame:SetColor( {x=94/255, y=220/255, z=31/255, w=1 } )
					else
						it.LeaderFrame:SetColor( {x=1, y=1, z=1, w=1 } )
						it.LeaderSubIconFrame:SetColor( {x=1, y=1, z=1, w=1 } )
						g_DraftResultInstances.PicksB[playerID][ind].LeaderFrame:SetColor( {x=1, y=1, z=1, w=1 } )
						g_DraftResultInstances.PicksB[playerID][ind].LeaderSubIconFrame:SetColor( {x=1, y=1, z=1, w=1 } )
					end
				end
			end
			if Matchmaking.GetLocalID() == i then
				g_PlayerEntries[i].statusInstance.FocusFrame:SetHide(false)
				g_PlayerEntries[i].statusInstance.DraftPlayerBansStack:SetHide(true)
				g_PlayerEntries[i].statusInstance.DraftPlayerBansDummy:SetHide(true)
				g_PlayerEntries[i].statusInstance.DraftPlayerPicksStack:SetHide(true)
				g_PlayerEntries[i].statusInstance.DraftPlayerPicksDummy:SetHide(true)
				g_PlayerEntries[i].statusInstance.DraftPlayerPicksBigStack:SetHide(false)
				g_PlayerEntries[i].statusInstance.DraftPlayerPicksBigDummy:SetHide(false)
				g_PlayerEntries[i].statusInstance.BansLabel:SetHide(true)
				g_PlayerEntries[i].statusInstance.PicksLabel:SetOffsetY(30)
			else
				g_PlayerEntries[i].statusInstance.FocusFrame:SetHide(true)
				g_PlayerEntries[i].statusInstance.DraftPlayerBansStack:SetHide(false)
				g_PlayerEntries[i].statusInstance.DraftPlayerBansDummy:SetHide(false)
				g_PlayerEntries[i].statusInstance.DraftPlayerPicksStack:SetHide(false)
				g_PlayerEntries[i].statusInstance.DraftPlayerPicksDummy:SetHide(false)
				g_PlayerEntries[i].statusInstance.DraftPlayerPicksBigStack:SetHide(true)
				g_PlayerEntries[i].statusInstance.DraftPlayerPicksBigDummy:SetHide(true)
				g_PlayerEntries[i].statusInstance.BansLabel:SetHide(false)
				g_PlayerEntries[i].statusInstance.PicksLabel:SetOffsetY(115)
			end
		else
			g_PlayerEntries[i].statusInstance.FocusFrame:SetHide(true)
			g_PlayerEntries[i].statusInstance.DraftPlayerBansStack:SetHide(false)
			g_PlayerEntries[i].statusInstance.DraftPlayerBansDummy:SetHide(false)
			g_PlayerEntries[i].statusInstance.DraftPlayerPicksStack:SetHide(false)
			g_PlayerEntries[i].statusInstance.DraftPlayerPicksDummy:SetHide(false)
			g_PlayerEntries[i].statusInstance.DraftPlayerPicksBigStack:SetHide(true)
			g_PlayerEntries[i].statusInstance.DraftPlayerPicksBigDummy:SetHide(true)
			g_PlayerEntries[i].statusInstance.BansLabel:SetHide(false)
			g_PlayerEntries[i].statusInstance.PicksLabel:SetOffsetY(115)
		end
	end
	if g_DraftProgress == 'DRAFT_PROGRESS_BANS' then
		if len(g_DraftBansTable[Matchmaking.GetLocalID()] or {}) == 0 then
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS', numBans, len(g_SelectedCivs));
		else
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS2');
		end
		Controls.DraftConfirmBansButton:SetHide(false);
	else
		Controls.DraftConfirmBansButton:SetHide(true);
	end
	UpdateDraftCivButtons()
end

-- 'events' from dll
function OnGameplayAlertMessage( text )
	print('alert', text)
	if (PreGame.GetLoadFileName() ~= "") or PreGame.GameStarted() then
		print('draft message during ongoing game - ignored')
		return;
	end
	local control, text = string.match(text, '(.-)|(.*)')
	if control == 'ban-success' then
		local splayerID, bans = string.match(text, '(.*)%|(.*)');
		local pName = Matchmaking.GetPlayerList()[tonumber(splayerID) + 1].playerName or Locale.Lookup('TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME', tonumber(splayerID) + 1);
		AddPlayerBans(tonumber(splayerID), pName, bans);
		if Matchmaking.GetLocalID() == tonumber(splayerID) and not Matchmaking.IsHost() then
			if len(g_DraftBansTable[Matchmaking.GetLocalID()] or {}) == 0 then
				Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS', numBans, len(g_SelectedCivs));
			else
				Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS2');
			end
			if not Controls.DraftConfirmBansButton:IsDisabled() then
				Controls.DraftConfirmBansButton:SetDisabled(true);
			end
			Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_TT2')
		end
	elseif control == 'ban-failure' then
		local playerID = tonumber(text) or -1;
		local pName = Matchmaking.GetPlayerList()[playerID + 1].playerName or Locale.Lookup('TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME', playerID + 1);
		if Matchmaking.GetLocalID() == playerID then
			AddDraftStatus('--- ban again');
			if Controls.DraftConfirmBansButton:IsDisabled() then
				Controls.DraftConfirmBansButton:SetDisabled(false);
				local civtbl = {}
				for k,v in pairs(g_SelectedCivs) do
					table.insert(civtbl, GameInfo.Civilizations[k] and Locale.Lookup(GameInfo.Civilizations[k].ShortDescription))
				end
				table.sort(civtbl, function(astr,bstr)
					local a0, b0
					if astr:match("The ") then a0 = astr:sub(5) else a0 = astr end
					if bstr:match("The ") then b0 = bstr:sub(5) else b0 = bstr end
					return a0 < b0
				end);
				Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_TT', next(civtbl) and table.concat(civtbl, ', ') or '[ENDCOLOR]{TXT_KEY_MP_NO_PROPOSAL}')
			end
		end
	elseif control == 'ready' then
		local playerID = tonumber(text) or -1;
		if g_PlayerEntries[playerID] then
			if not g_PlayerEntries[playerID].readyForDrafts then
				g_PlayerEntries[playerID].readyForDrafts = true;
				g_PlayerEntries[playerID].statusInstance.DraftPlayerStatusBlackFade:SetHide( true );
			end
		end
	elseif control == 'reset' then
		ResetDrafts(text);
		RebuildDrafts();
	elseif control == 'upd' then
		local playerID = tonumber(text) or -1;
		if Matchmaking.GetLocalID() == playerID then
			Controls.DraftBGBlock:SetHide(true)
		end
		UpdateDraftScreen();
	elseif control == 'DRAFT_PROGRESS_INIT' then
		print('--- DRAFT_PROGRESS_INIT');
		g_DraftProgress = control;
		g_DraftBansTable = {}
		Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_INIT');
		Controls.DraftPlayersStatus:SetHide(false);
		Controls.DraftCivPicker:SetHide(true);
		print('send local secret hash', text)
		Network.SendRenameCity(-1, text);  -- send secret hash
	elseif control == 'DRAFT_PROGRESS_BANS' then
		print('--- DRAFT_PROGRESS_BANS');
		AddDraftStatus(Locale.Lookup('TXT_KEY_DRAFT_STATUS_PROGRESS_BANS'));
		g_DraftProgress = control;
		if len(g_DraftBansTable[Matchmaking.GetLocalID()] or {}) == 0 then
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS', numBans, len(g_SelectedCivs));
		else
			Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BANS2');
		end
		Controls.DraftPlayersStatus:SetHide(true);
		Controls.DraftCivPicker:SetHide(false);
		Controls.DraftConfirmBansButton:SetHide(false);
		if len(g_SelectedCivs) < minBans or (not Matchmaking.IsHost() and len(g_SelectedCivs) > numBans) then
			if not Controls.DraftConfirmBansButton:IsDisabled() then
				Controls.DraftConfirmBansButton:SetDisabled(true);
			end
			Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_HELP')
		elseif not Matchmaking.IsHost() and g_DraftBansTable[Matchmaking.GetLocalID()] ~= nil then
			if not Controls.DraftConfirmBansButton:IsDisabled() then
				Controls.DraftConfirmBansButton:SetDisabled(true);
			end
			Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_TT2')
		else
			Controls.DraftConfirmBansButton:SetDisabled(false);
			local civtbl = {}
			for k,v in pairs(g_SelectedCivs) do
				table.insert(civtbl, GameInfo.Civilizations[k] and Locale.Lookup(GameInfo.Civilizations[k].ShortDescription))
			end
			table.sort(civtbl, function(astr,bstr)
				local a0, b0
				if astr:match("The ") then a0 = astr:sub(5) else a0 = astr end
				if bstr:match("The ") then b0 = bstr:sub(5) else b0 = bstr end
				return a0 < b0
			end);
			Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_TT', next(civtbl) and table.concat(civtbl, ', ') or '[ENDCOLOR]{TXT_KEY_MP_NO_PROPOSAL}')
		end

	elseif control == 'DRAFT_PROGRESS_BUSY' then
		print('--- DRAFT_PROGRESS_BUSY');
		AddDraftStatus(Locale.Lookup('TXT_KEY_DRAFT_STATUS_PROGRESS_BUSY'));
		g_DraftProgress = control;
		Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_BUSY');
		if not Controls.DraftConfirmBansButton:IsDisabled() then
			Controls.DraftConfirmBansButton:SetDisabled(true);
		end
		Controls.DraftConfirmBansButton:LocalizeAndSetToolTip('TXT_KEY_DRAFT_BANS_CONFIRM_HELP')
		print('send local secret', text)
		Network.SendRenameCity(-3, text);  -- send secret
	elseif control == 'DRAFT_PROGRESS_OVER' then
		print('--- DRAFT_PROGRESS_OVER');
		g_DraftProgress = control;
		Controls.DraftProgressBar:LocalizeAndSetText('TXT_KEY_DRAFT_PROGRESS_OVER');
		Controls.DraftPlayersStatus:SetHide(false);
		Controls.DraftCivPicker:SetHide(true);
		AddDraftStatus(Locale.Lookup('TXT_KEY_DRAFT_STATUS_RESULT'));
		AssignDraftedCivs( text );
	end
	UpdateDraftCivButtons();
	UpdateDraftScreen();
end
Events.GameplayAlertMessage.Add( OnGameplayAlertMessage );

function HandleExitDrafts()
	print('exit drafts')
	local product = 4 * 2 ^ 28;  -- reset draft data (local)
	local data = PreGame.SetLeaderKey( product, 'TXT_KEY_DRAFTS_RESET_DISC' );
	Controls.DraftStatusStack:DestroyAllChildren();
	--RebuildDrafts();
	--ResetDrafts();
	--PopulateDrafts();
end

-------------------------------------------------
-- END OF INGAME CIV DRAFTER
-------------------------------------------------
