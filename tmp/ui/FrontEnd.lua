-------------------------------------------------
-- FrontEnd
-------------------------------------------------
-- edit:
--     Random background image & scaling
-- for EUI & vanilla UI
-------------------------------------------------

-- Random background image & scaling START
g_LoadingPaths = {
	-- Upgrade1Textures.fpk
	'loadingbasegame_1.dds',  -- mod 1
	'loadingbasegame_3.dds',  -- mod 2
	'loadingbasegame_7.dds',  -- mod 3
	'loadingbasegame_9.dds',  -- mod 4
	'loadingbasegame_10.dds',  -- mod 5
	'loadingbasegame_13.dds',  -- mod 6
	'loadingbasegame_14.dds',  -- mod 7
	'loadingbasegame_15.dds',  -- mod 8
	'loadingbasegame_18.dds',  -- mod 9
	-- patch /Art/Loadings
	'mod_loading_10.dds',
	'mod_loading_11.dds',
	-- Expansion2UITextures.fpk
	'loading_2.dds',  -- mod 12
	'loading_3.dds',  -- mod 13
	'loading_6.dds',  -- mod 14
}
-- Random background image & scaling END

function ShowHideHandler( bIsHide, bIsInit )

		-- Check for game invites first.  If we have a game invite, we will have flipped 
		-- the Civ5App::eHasShownLegal and not show the legal/touch screens.
		UI:CheckForCommandLineInvitation();

---------- Temudjin START
--	if not UI:HasShownLegal() then
--		UIManager:QueuePopup( Controls.LegalScreen, PopupPriority.LegalScreen );
--	end
---------- Temudjin END

	if not bIsHide then
		--Controls.AtlasLogo:SetTexture( "CivilzationVAtlas.dds" );
-- Random Random background image & scaling START
		Controls.AtlasLogo:SetTexture( g_LoadingPaths[math.random(#g_LoadingPaths)] );
		OnScreenResize()
-- Random Random background image & scaling END
		UIManager:SetUICursor( 0 );
		UIManager:QueuePopup( Controls.MainMenu, PopupPriority.MainMenu );
	else
		Controls.AtlasLogo:UnloadTexture();
	end
end
ContextPtr:SetShowHideHandler( ShowHideHandler );

-- Random background image & scaling START
function OnScreenResize()
	local rx, ry = UIManager.GetScreenSizeVal();
	Controls.AtlasLogo:Resize(rx, rx * 1200 / 1920);
end

function SystemUpdateUIHandler( type )
	if type == SystemUpdateUIType.ScreenResize then
		OnScreenResize()
	end
end
Events.SystemUpdateUI.Add( SystemUpdateUIHandler )
-- Random background image & scaling END
