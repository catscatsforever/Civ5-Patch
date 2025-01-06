-------------------------------------------------
-- FrontEnd
-------------------------------------------------

function ShowHideHandler( bIsHide, bIsInit )

		-- Check for game invites first.  If we have a game invite, we will have flipped 
		-- the Civ5App::eHasShownLegal and not show the legal/touch screens.
		UI:CheckForCommandLineInvitation();
		
    if( not UI:HasShownLegal() ) then
        -- UIManager:QueuePopup( Controls.LegalScreen, PopupPriority.LegalScreen );
    end

    if( not bIsHide ) then
        UIManager:SetUICursor( 0 );
        UIManager:QueuePopup( Controls.MainMenu, PopupPriority.MainMenu );
        --Controls.AtlasLogo:SetTexture( "CivilzationVAtlas.dds" );
        Controls.AtlasLogo:SetTexture( string.format("mod_loading_%d.dds", math.random(14)) );
    else
        Controls.AtlasLogo:UnloadTexture();
    end
end
ContextPtr:SetShowHideHandler( ShowHideHandler );
