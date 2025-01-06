-------------------------------------------------
-- edit: Add Raze button for puppet cities
-------------------------------------------------

-- ANNEX CITY POPUP
-- This popup occurs when a player clicks on a puppeted City
PopupLayouts[ButtonPopupTypes.BUTTONPOPUP_ANNEX_CITY] = function(popupInfo)
	
	local cityID				= popupInfo.Data1;
	
	local activePlayer	= Players[Game.GetActivePlayer()];
	local newCity		= activePlayer:GetCityByID(cityID);
	
	-- Initialize popup text.	
	local cityNameKey = newCity:GetNameKey();
	popupText = Locale.ConvertTextKey("TXT_KEY_POPUP_ANNEX_PUPPET", cityNameKey);
	
	SetPopupText(popupText);
	
	-- Initialize 'Annex City' button.
	local OnAnnexClicked = function()
		Network.SendDoTask(cityID, TaskTypes.TASK_ANNEX_PUPPET, -1, -1, false, false, false, false);
	end
	
	local buttonText = Locale.ConvertTextKey("TXT_KEY_POPUP_ANNEX_CITY");
	AddButton(buttonText, OnAnnexClicked);
	
	-- Initialize 'Leave City as a Puppet' button.
	--local OnNoClicked = function()
		--newCity:ChooseProduction();
	--end
	
	local buttonText = Locale.ConvertTextKey("TXT_KEY_POPUP_DONT_ANNEX_CITY");
	AddButton(buttonText);
	
	-- Initialize 'Raze the City!' button.
	local bRaze = activePlayer:CanRaze(newCity);
	print ("Test CanRaze");
	if (bRaze) then
		print ("CanRaze");
		local OnRazeClicked = function()
			Network.SendDoTask(cityID, TaskTypes.TASK_RAZE, -1, -1, false, false, false, false);
			Network.SendDoTask(cityID, TaskTypes.TASK_ANNEX_PUPPET, -1, -1, false, false, false, false);
		end
		
		buttonText = Locale.ConvertTextKey("TXT_KEY_POPUP_RAZE_CAPTURED_CITY");
		AddButton(buttonText, OnRazeClicked);
	end
	
	-- Initialize 'View City' button.
	local OnViewCityClicked = function()
		UI.SetCityScreenViewingMode(true);
		UI.DoSelectCityAtPlot( newCity:Plot() );
	end
	
	buttonText = Locale.ConvertTextKey("TXT_KEY_POPUP_VIEW_CITY");
	strToolTip = Locale.ConvertTextKey("TXT_KEY_POPUP_VIEW_CITY_DETAILS");
	AddButton(buttonText, OnViewCityClicked, strToolTip);
	
	Controls.CloseButton:SetHide( false );

end

----------------------------------------------------------------        
-- Key Down Processing
----------------------------------------------------------------        
PopupInputHandlers[ButtonPopupTypes.BUTTONPOPUP_ANNEX_CITY] = function( uiMsg, wParam, lParam )
    if uiMsg == KeyEvents.KeyDown then
        if( wParam == Keys.VK_ESCAPE or wParam == Keys.VK_RETURN ) then
			HideWindow();
            return true;
        end
    end
end

----------------------------------------------------------------
-- 'Active' (local human) player has changed
----------------------------------------------------------------
Events.GameplaySetActivePlayer.Add(HideWindow);
