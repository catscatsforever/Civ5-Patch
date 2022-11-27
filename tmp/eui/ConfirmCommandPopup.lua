
-- CONFIRM COMMAND POPUP
-- This popup occurs when an action needs confirmation.
PopupLayouts[ButtonPopupTypes.BUTTONPOPUP_CONFIRMCOMMAND] = function(popupInfo)

	local iAction = popupInfo.Data1;
	
	-- TOURNAMENT MOD - fix MPList freeze on kick button press
	-- when Data1 equals -2, we know its our kick command from MPList
	local popupText, OnYesClicked
	if iAction == -2 then
		local PlayerID = popupInfo.Data2;
		local PlayerName;
		for i = 0, GameDefines.MAX_MAJOR_CIVS - 1 do
			if Players[i]:IsEverAlive() and Players[i]:GetID() == PlayerID then
				PlayerName = Players[i]:GetName();
				break;
			end
		end
		popupText = Locale.ConvertTextKey("TXT_KEY_CONFIRM_KICK_PLAYER_DESC", PlayerName);
			
		-- Initialize 'yes' button.
		OnYesClicked = function()
			Matchmaking.KickPlayer( PlayerID );
		end
	else
		local bAlt = popupInfo.Option1;
		local action = GameInfoActions[iAction];
		popupText = Locale.ConvertTextKey("TXT_KEY_POPUP_ARE_YOU_SURE_ACTION", action.TextKey);
			
		-- Initialize 'yes' button.
		OnYesClicked = function()
			-- Confirm action
			local gameMessageDoCommand = GameMessageTypes.GAMEMESSAGE_DO_COMMAND;
			Game.SelectionListGameNetMessage(gameMessageDoCommand, action.CommandType, action.CommandData, -1, 0, bAlt);
		end
	end

		
	SetPopupText(popupText);
	local buttonText = Locale.ConvertTextKey("TXT_KEY_POPUP_YES");
	AddButton(buttonText, OnYesClicked)
		
	-- Initialize 'no' button.
	local buttonText = Locale.ConvertTextKey("TXT_KEY_POPUP_NO");
	AddButton(buttonText, nil);
	
	Controls.CloseButton:SetHide( true );
end

----------------------------------------------------------------        
-- Key Down Processing
----------------------------------------------------------------        
PopupInputHandlers[ButtonPopupTypes.BUTTONPOPUP_CONFIRMCOMMAND] = function( uiMsg, wParam, lParam )
    if uiMsg == KeyEvents.KeyDown then
        if( wParam == Keys.VK_ESCAPE or wParam == Keys.VK_RETURN ) then
			HideWindow();
            return true;
        end
    end
end