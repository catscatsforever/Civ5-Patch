
-------------------------------------------------
-- Choose Pantheon Popup
-------------------------------------------------

include( "IconSupport" );
include( "InstanceManager" );

-- Used for Piano Keys
local ltBlue = {19/255,32/255,46/255,120/255};
local dkBlue = {12/255,22/255,30/255,120/255};

local g_ItemManager = InstanceManager:new( "ItemInstance", "Button", Controls.ItemStack );
local bHidden = true;
local g_bPantheons = true;

local screenSizeX, screenSizeY = UIManager:GetScreenSizeVal()
local spWidth, spHeight = Controls.ItemScrollPanel:GetSizeVal();

-- Original UI designed at 1050px 
local heightOffset = screenSizeY - 1020;

spHeight = spHeight + heightOffset;
Controls.ItemScrollPanel:SetSizeVal(spWidth, spHeight); 
Controls.ItemScrollPanel:CalculateInternalSize();
Controls.ItemScrollPanel:ReprocessAnchoring();

local bpWidth, bpHeight = Controls.BottomPanel:GetSizeVal();
--bpHeight = bpHeight * heightRatio;
print(heightOffset);
print(bpHeight);
bpHeight = bpHeight + heightOffset 
print(bpHeight);

Controls.BottomPanel:SetSizeVal(bpWidth, bpHeight);
Controls.BottomPanel:ReprocessAnchoring();
-------------------------------------------------
-------------------------------------------------
function OnPopupMessage(popupInfo)
	
	local popupType = popupInfo.Type;
	if popupType ~= ButtonPopupTypes.BUTTONPOPUP_FOUND_PANTHEON then
		return;
	end
	
	g_PopupInfo = popupInfo;
	g_bPantheons = popupInfo.Data2;

   	UIManager:QueuePopup( ContextPtr, PopupPriority.SocialPolicy );
end
Events.SerialEventGameMessagePopup.Add( OnPopupMessage );

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
function OnClose()
    UIManager:DequeuePopup(ContextPtr);
end
Controls.CloseButton:RegisterCallback( Mouse.eLClick, OnClose );

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
function InputHandler( uiMsg, wParam, lParam )
    ----------------------------------------------------------------        
    -- Key Down Processing
    ----------------------------------------------------------------        
    if uiMsg == KeyEvents.KeyDown then
        if (wParam == Keys.VK_RETURN or wParam == Keys.VK_ESCAPE) then
			if(Controls.ChooseConfirm:IsHidden())then
	            OnClose();
	        else
				Controls.ChooseConfirm:SetHide(true);
           	end
			return true;
        end
    end
end
ContextPtr:SetInputHandler( InputHandler );

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
function RefreshList()
	g_ItemManager:ResetInstances();
		
	local pPlayer = Players[Game.GetActivePlayer()];
	CivIconHookup( pPlayer:GetID(), 64, Controls.CivIcon, Controls.CivIconBG, Controls.CivIconShadow, false, true );
	
	local availableBeliefs = {};
	
	if (g_bPantheons > 0) then
		Controls.PanelTitle:LocalizeAndSetText("TXT_KEY_CHOOSE_PANTHEON_TITLE");
		for info in GameInfo.Beliefs("Pantheon == 1 ORDER BY ShortDescription") do
			if(info ~= nil) then
				local available = false;
				for i,v in ipairs(Game.GetAvailablePantheonBeliefs()) do
					if (GameInfo.Beliefs[v] == info) then
						available = true;
						break;
					end
				end
				-- Duel Mode
				if (PreGame.GetGameOption("GAMEOPTION_DUEL_STUFF") > 0 and info.DuelDescription ~= nil) then
					if (available == true) then
						table.insert(availableBeliefs, {
							ID = info.ID,
							Name = Locale.Lookup(info.ShortDescription),
							Description = Locale.Lookup(info.DuelDescription),
							Available = true,
						});
					else
						table.insert(availableBeliefs, {
							ID = info.ID,
							Name = Locale.Lookup(info.ShortDescription),
							Description = Locale.Lookup(info.DuelDescription),
							Available = false,
						});
					end
				else
					if (available == true) then
						table.insert(availableBeliefs, {
							ID = info.ID,
							Name = Locale.Lookup(info.ShortDescription),
							Description = Locale.Lookup(info.Description),
							Available = true,
						});
					else
						table.insert(availableBeliefs, {
							ID = info.ID,
							Name = Locale.Lookup(info.ShortDescription),
							Description = Locale.Lookup(info.Description),
							Available = false,
						});
					end
				end
			end
		end

		--[[Controls.PanelTitle:LocalizeAndSetText("TXT_KEY_CHOOSE_PANTHEON_TITLE");
		for i,v in ipairs(Game.GetAvailablePantheonBeliefs()) do
			local belief = GameInfo.Beliefs[v];
			if(belief ~= nil) then
				table.insert(availableBeliefs, {
					ID = belief.ID,
					Name = Locale.Lookup(belief.ShortDescription),
					Description = Locale.Lookup(belief.Description),
				});
			end
		end	]]
	else
		Controls.PanelTitle:LocalizeAndSetText("TXT_KEY_CHOOSE_REFORMATION_BELIEF_TITLE");
		for info in GameInfo.Beliefs("Reformation == 1 ORDER BY ShortDescription") do
			if(info ~= nil) then
				local available = false;
				for i,v in ipairs(Game.GetAvailableReformationBeliefs()) do
					if (GameInfo.Beliefs[v] == info) then
						available = true;
						break;
					end
				end
				-- Duel Mode
				if (PreGame.GetGameOption("GAMEOPTION_DUEL_STUFF") > 0 and info.DuelDescription ~= nil) then
					if (available == true) then
						table.insert(availableBeliefs, {
							ID = info.ID,
							Name = Locale.Lookup(info.ShortDescription),
							Description = Locale.Lookup(info.DuelDescription),
							Available = true,
						});
					else
						table.insert(availableBeliefs, {
							ID = info.ID,
							Name = Locale.Lookup(info.ShortDescription),
							Description = Locale.Lookup(info.DuelDescription),
							Available = false,
						});
					end
				else
					if (available == true) then
						table.insert(availableBeliefs, {
							ID = info.ID,
							Name = Locale.Lookup(info.ShortDescription),
							Description = Locale.Lookup(info.Description),
							Available = true,
						});
					else
						table.insert(availableBeliefs, {
							ID = info.ID,
							Name = Locale.Lookup(info.ShortDescription),
							Description = Locale.Lookup(info.Description),
							Available = false,
						});
					end
				end
					
			end
		end

		--[[Controls.PanelTitle:LocalizeAndSetText("TXT_KEY_CHOOSE_REFORMATION_BELIEF_TITLE");
		for i,v in ipairs(Game.GetAvailableReformationBeliefs()) do
			local belief = GameInfo.Beliefs[v];
			if(belief ~= nil) then
				table.insert(availableBeliefs, {
					ID = belief.ID,
					Name = Locale.Lookup(belief.ShortDescription),
					Description = Locale.Lookup(belief.Description),
				});
			end
		end	]]	
	end

	-- Sort beliefs by their description.
	table.sort(availableBeliefs, function(a,b) return Locale.Compare(a.Name, b.Name) < 0; end);
	
	local bTickTock = false;
	-- for info in GameInfo.Beliefs("Pantheon = 1 ORDER BY ShortDescription") do
	for i, belief in ipairs(availableBeliefs) do
		local itemInstance = g_ItemManager:GetInstance();
		if (belief.Available == true) then
	    	itemInstance.Button:SetDisabled(false);
	    	itemInstance.Name:SetColorByName("Beige_Black");
	    	itemInstance.Description:SetColorByName("Beige_Black");
		else
	    	itemInstance.Button:SetDisabled(true);
	    	itemInstance.Name:SetColorByName("Gray_Black");
	    	itemInstance.Description:SetColorByName("Gray_Black");
	    end
		itemInstance.Name:SetText(belief.Name);
		--itemInstance.Button:SetToolTipString(belief.Description);
		itemInstance.Description:SetText(belief.Description);
		
		itemInstance.Button:RegisterCallback(Mouse.eLClick, function() SelectPantheon(belief.ID); end);
	
		if(bTickTock == false) then
			itemInstance.Box:SetColorVal(unpack(ltBlue));
		else
			itemInstance.Box:SetColorVal(unpack(dkBlue));
		end
		
		local buttonWidth, buttonHeight = itemInstance.Button:GetSizeVal();
		local descWidth, descHeight = itemInstance.Description:GetSizeVal();
		
		local newHeight = descHeight + 40;	
	
		
		itemInstance.Button:SetSizeVal(buttonWidth, newHeight);
		itemInstance.Box:SetSizeVal(buttonWidth + 20, newHeight);
		itemInstance.BounceAnim:SetSizeVal(buttonWidth + 20, newHeight + 5);
		itemInstance.BounceGrid:SetSizeVal(buttonWidth + 20, newHeight + 5);
		
				
		bTickTock = not bTickTock;
	end
	
	Controls.ItemStack:CalculateSize();
	Controls.ItemStack:ReprocessAnchoring();
	Controls.ItemScrollPanel:CalculateInternalSize();
end

function SelectPantheon(beliefID) 
	g_BeliefID = beliefID;
	local belief = GameInfo.Beliefs[beliefID];
	Controls.ConfirmText:LocalizeAndSetText("TXT_KEY_CONFIRM_PANTHEON", belief.ShortDescription);
	Controls.ChooseConfirm:SetHide(false);
end

function OnYes( )
	Controls.ChooseConfirm:SetHide(true);
	
	Network.SendFoundPantheon(Game.GetActivePlayer(), g_BeliefID);
	Events.AudioPlay2DSound("AS2D_INTERFACE_POLICY");	
	
	OnClose();	
end
Controls.Yes:RegisterCallback( Mouse.eLClick, OnYes );

function OnNo( )
	Controls.ChooseConfirm:SetHide(true);
end
Controls.No:RegisterCallback( Mouse.eLClick, OnNo );


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
function ShowHideHandler( bIsHide, bInitState )

	bHidden = bIsHide;
    if( not bInitState ) then
        if( not bIsHide ) then
        	UI.incTurnTimerSemaphore();
        	Events.SerialEventGameMessagePopupShown(g_PopupInfo);
        	
        	RefreshList();
        
			local unitPanel = ContextPtr:LookUpControl( "/InGame/WorldView/UnitPanel/Base" );
			if( unitPanel ~= nil ) then
				unitPanel:SetHide( true );
			end
			
			local infoCorner = ContextPtr:LookUpControl( "/InGame/WorldView/InfoCorner" );
			if( infoCorner ~= nil ) then
				infoCorner:SetHide( true );
			end
               	
        else
      
			local unitPanel = ContextPtr:LookUpControl( "/InGame/WorldView/UnitPanel/Base" );
			if( unitPanel ~= nil ) then
				unitPanel:SetHide(false);
			end
			
			local infoCorner = ContextPtr:LookUpControl( "/InGame/WorldView/InfoCorner" );
			if( infoCorner ~= nil ) then
				infoCorner:SetHide(false);
			end
			
			if(g_PopupInfo ~= nil) then
				Events.SerialEventGameMessagePopupProcessed.CallImmediate(g_PopupInfo.Type, 0);
			end
            UI.decTurnTimerSemaphore();
        end
    end
end
ContextPtr:SetShowHideHandler( ShowHideHandler );

----------------------------------------------------------------
-- 'Active' (local human) player has changed
----------------------------------------------------------------
function OnActivePlayerChanged()
	if (not Controls.ChooseConfirm:IsHidden()) then
		Controls.ChooseConfirm:SetHide(true);
    end
end
Events.GameplaySetActivePlayer.Add(OnActivePlayerChanged);

function OnDirty()
	-- If the user performed any action that changes selection states, just close this UI.
	if not bHidden then
		OnClose();
	end
end
Events.UnitSelectionChanged.Add( OnDirty );
