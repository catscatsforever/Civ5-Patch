-------------------------------------------------
-- Include file that has handy stuff for the tech tree and other screens that need to show a tech button
-------------------------------------------------
-- edit: Duel Mode for EUI and vanilla UI
-------------------------------------------------
include( "IconSupport" );
include( "InfoTooltipInclude" );

-- List the textures that we will need here
local defaultErrorTextureSheet;

techPediaSearchStrings = {};

if g_UseSmallIcons then
	defaultErrorTextureSheet = "UnitActions360.dds";
else
	defaultErrorTextureSheet = "UnitActions.dds";
end

local validUnitBuilds = nil;
local validBuildingBuilds = nil;
local validImprovementBuilds = nil;

turnsString = Locale.ConvertTextKey("TXT_KEY_TURNS");
freeString = Locale.ConvertTextKey("TXT_KEY_FREE");
lockedString = "[ICON_LOCKED]"; --Locale.ConvertTextKey("TXT_KEY_LOCKED");

function GetTechPedia( void1, void2, button )
	local searchString = techPediaSearchStrings[tostring(button)];
	Events.SearchForPediaEntry( searchString );		
end

function GatherInfoAboutUniqueStuff( civType )

	validUnitBuilds = {};
	validBuildingBuilds = {};
	validImprovementBuilds = {};

	-- put in the default units for any civ
	for thisUnitClass in GameInfo.UnitClasses() do
		validUnitBuilds[thisUnitClass.Type]	= thisUnitClass.DefaultUnit;	
	end

	-- put in my overrides
	for thisOverride in GameInfo.Civilization_UnitClassOverrides() do
 		if thisOverride.CivilizationType == civType then
			validUnitBuilds[thisOverride.UnitClassType]	= thisOverride.UnitType;
 		end
	end

	-- put in the default buildings for any civ
	for thisBuildingClass in GameInfo.BuildingClasses() do
		validBuildingBuilds[thisBuildingClass.Type]	= thisBuildingClass.DefaultBuilding;	
	end

	-- put in my overrides
	for thisOverride in GameInfo.Civilization_BuildingClassOverrides() do
 		if thisOverride.CivilizationType == civType then
			validBuildingBuilds[thisOverride.BuildingClassType]	= thisOverride.BuildingType;	
 		end
	end
	
	-- add in support for unique improvements
	for thisImprovement in GameInfo.Improvements() do
		if thisImprovement.CivilizationType == civType or thisImprovement.CivilizationType == nil then
			validImprovementBuilds[thisImprovement.Type] = thisImprovement.Type;	
		else
			validImprovementBuilds[thisImprovement.Type] = nil;	
		end
	end
	
end


function AddSmallButtonsToTechButton( thisTechButtonInstance, tech, maxSmallButtons, textureSize )
	-- This has a few assumptions, the main one being that the small buttons are named "B1", "B2", "B3"... and that GatherInfoAboutUniqueStuff() has been called before this

	-- first, hide the ones we aren't using
	for buttonNum = 1, maxSmallButtons, 1 do
		local buttonName = "B"..tostring(buttonNum);
		thisTechButtonInstance[buttonName]:SetHide(true);
	end
	
	if tech == nil then
		return;
	end

	local buttonNum = 1;

	local techType = tech.Type;

	-- add the stuff granted by this tech here
  	for thisUnitInfo in GameInfo.Units(string.format("PreReqTech = '%s'", techType)) do
 		-- if this tech grants this player the ability to make this unit
		if validUnitBuilds[thisUnitInfo.Class] == thisUnitInfo.Type then
			local buttonName = "B"..tostring(buttonNum);
			local thisButton = thisTechButtonInstance[buttonName];
			if thisButton then
				AdjustArtOnGrantedUnitButton( thisButton, thisUnitInfo, textureSize );
				buttonNum = buttonNum + 1;
			end
		end
 	end
 	
	playerID = Game.GetActivePlayer();
	player = Players[playerID];
	civID = GameInfo.Civilizations[player:GetCivilizationType()].ID;
 	for thisBuildingInfo in GameInfo.Buildings(string.format("PreReqTech = '%s'", techType)) do
 		-- if this tech grants this player the ability to construct this building
 -- Duel Mode
		if validBuildingBuilds[thisBuildingInfo.BuildingClass] == thisBuildingInfo.Type then
			if not (civID == 21 and thisBuildingInfo.ID == 162 or PreGame.GetGameOption("GAMEOPTION_DUEL_STUFF") > 0 and (PreGame.GetGameOption("GAMEOPTION_BAN_WORLD_WONDERS") > 0 
				and (PreGame.GetGameOption("GAMEOPTION_BAN_WONDER1") == thisBuildingInfo.ID or PreGame.GetGameOption("GAMEOPTION_BAN_WONDER2") == thisBuildingInfo.ID or PreGame.GetGameOption("GAMEOPTION_BAN_WONDER3") == thisBuildingInfo.ID) or 70 == thisBuildingInfo.ID or PreGame.GetGameOption("GAMEOPTION_DISABLE_OXFORD_UNIVERSITY") > 0 and 61 == thisBuildingInfo.ID)) then
				local buttonName = "B"..tostring(buttonNum);
				local thisButton = thisTechButtonInstance[buttonName];
				if thisButton then
					AdjustArtOnGrantedBuildingButton( thisButton, thisBuildingInfo, textureSize );
					buttonNum = buttonNum + 1;
				end
			end
		end
 	end

 	for thisResourceInfo in GameInfo.Resources(string.format("TechReveal = '%s'", techType)) do
 		if (thisResourceInfo.ID ~= 39) then
	 		-- if this tech grants this player the ability to reveal this resource
			local buttonName = "B"..tostring(buttonNum);
			local thisButton = thisTechButtonInstance[buttonName];
			if thisButton then
				AdjustArtOnGrantedResourceButton( thisButton, thisResourceInfo, textureSize );
				buttonNum = buttonNum + 1;
			end
		end
 	end
 
 	for thisProjectInfo in GameInfo.Projects(string.format("TechPrereq = '%s'", techType)) do
 		-- if this tech grants this player the ability to build this project
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
 		if thisButton then
			AdjustArtOnGrantedProjectButton( thisButton, thisProjectInfo, textureSize );
 			buttonNum = buttonNum + 1;
 		end
	end

	-- if this tech grants this player the ability to perform this action (usually only workers can do these)
	playerID = Game.GetActivePlayer();
	player = Players[playerID];
	civID = GameInfo.Civilizations[player:GetCivilizationType()].ID;
	for thisBuildInfo in GameInfo.Builds{PrereqTech = techType, ShowInTechTree  = 1} do
		if not (civID == 10 and thisBuildInfo.ID == 5) then
			if thisBuildInfo.ImprovementType then
				if validImprovementBuilds[thisBuildInfo.ImprovementType] == thisBuildInfo.ImprovementType then
					local buttonName = "B"..tostring(buttonNum);
					local thisButton = thisTechButtonInstance[buttonName];
					if thisButton then
						AdjustArtOnGrantedActionButton( thisButton, thisBuildInfo, textureSize );
	 					buttonNum = buttonNum + 1;
	 				end
	 			end
			else
				local buttonName = "B"..tostring(buttonNum);
				local thisButton = thisTechButtonInstance[buttonName];
				if thisButton then
					AdjustArtOnGrantedActionButton( thisButton, thisBuildInfo, textureSize );
	 				buttonNum = buttonNum + 1;
	 			end
			end
		end
	end
	if civID == 10 and tech.ID == 0 then
		local thisBuildInfo = GameInfo.Builds[5];
		if thisBuildInfo.ImprovementType then
			if validImprovementBuilds[thisBuildInfo.ImprovementType] == thisBuildInfo.ImprovementType then
				local buttonName = "B"..tostring(buttonNum);
				local thisButton = thisTechButtonInstance[buttonName];
				if thisButton then
					AdjustArtOnGrantedActionButton( thisButton, thisBuildInfo, textureSize );
 					buttonNum = buttonNum + 1;
 				end
 			end
		else
			local buttonName = "B"..tostring(buttonNum);
			local thisButton = thisTechButtonInstance[buttonName];
			if thisButton then
				AdjustArtOnGrantedActionButton( thisButton, thisBuildInfo, textureSize );
 				buttonNum = buttonNum + 1;
 			end
		end
	end
	
	-- show processes
	local processCondition = "TechPrereq = '" .. techType .. "'";
	for row in GameInfo.Processes(processCondition) do
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( row.PortraitIndex, textureSize, row.IconAtlas, thisButton );
			thisButton:SetHide( false );
			local strPText = Locale.ConvertTextKey( row.Description );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ENABLE_PRODUCITON_CONVERSION", strPText) );
		end
		buttonNum = buttonNum + 1;
	end	
		
 	-- todo: need to add abilities, etc.
	local condition = "TechType = '" .. techType .. "'";
		
	for row in GameInfo.Route_TechMovementChanges(condition) do
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey("TXT_KEY_FASTER_MOVEMENT", GameInfo.Routes[row.RouteType].Description ) );
			buttonNum = buttonNum + 1;
		else
			break
		end
	end	
	
	-- Some improvements can have multiple yield changes, group them and THEN add buttons.
	local yieldChanges = {};
	for row in GameInfo.Improvement_TechYieldChanges(condition) do
		local improvementType = row.ImprovementType;
		
		if(yieldChanges[improvementType] == nil) then
			yieldChanges[improvementType] = {};
		end
		
		local improvement = GameInfo.Improvements[row.ImprovementType];
		local yield = GameInfo.Yields[row.YieldType];
		
		table.insert(yieldChanges[improvementType], Locale.Lookup( "TXT_KEY_YIELD_IMPROVED", improvement.Description , yield.Description, row.Yield));
	end
	
	-- Let's sort the yield change butons!
	local sortedYieldChanges = {};
	for k,v in pairs(yieldChanges) do
	
		
		table.insert(sortedYieldChanges, {k,v});
	end
	table.sort(sortedYieldChanges, function(a,b) return Locale.Compare(a[1], b[1]) == -1 end); 
	
	for i,v in pairs(sortedYieldChanges) do
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if(thisButton ~= nil) then
			table.sort(v[2], function(a,b) return Locale.Compare(a,b) == -1 end);
		
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString(table.concat(v[2], "[NEWLINE]"));
			buttonNum = buttonNum + 1;
		else
			break;
		end
	end	
	
	for row in GameInfo.Improvement_TechNoFreshWaterYieldChanges(condition) do
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey("TXT_KEY_NO_FRESH_WATER", GameInfo.Improvements[row.ImprovementType].Description , GameInfo.Yields[row.YieldType].Description, row.Yield));
			buttonNum = buttonNum + 1;
		else
			break;
		end
	end	

	for row in GameInfo.Improvement_TechFreshWaterYieldChanges(condition) do
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey("TXT_KEY_FRESH_WATER", GameInfo.Improvements[row.ImprovementType].Description , GameInfo.Yields[row.YieldType].Description, row.Yield));
			buttonNum = buttonNum + 1;
		else
			break;
		end
	end	

	if tech.EmbarkedMoveChange > 0 then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_FASTER_EMBARKED_MOVEMENT" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.AllowsEmbarking then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_EMBARKING" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.AllowsDefensiveEmbarking then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ABLTY_DEFENSIVE_EMBARK_STRING" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.EmbarkedAllWaterPassage then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_CROSSING_OCEANS" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.UnitFortificationModifier > 0 then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_UNIT_FORTIFICATION_MOD", tech.UnitFortificationModifier ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.UnitBaseHealModifier > 0 then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_UNIT_BASE_HEAL_MOD", tech.UnitBaseHealModifier ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.AllowEmbassyTradingAllowed then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_EMBASSY" ) );
			buttonNum = buttonNum + 1;
		end	
	end
	
	if tech.OpenBordersTradingAllowed then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_OPEN_BORDERS" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.DefensivePactTradingAllowed then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_DEFENSIVE_PACTS" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.ResearchAgreementTradingAllowed then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_RESEARCH_AGREEMENTS" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.TradeAgreementTradingAllowed then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_TRADE_AGREEMENTS" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.BridgeBuilding then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_BRIDGES" ) );
			buttonNum = buttonNum + 1;
		end
	end

	if tech.MapVisible then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_REVEALS_ENTIRE_MAP" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.InternationalTradeRoutesChange > 0 then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ADDITIONAL_INTERNATIONAL_TRADE_ROUTE" ) );
			buttonNum = buttonNum + 1;
		end	
	end

	playerID = Game.GetActivePlayer();
	player = Players[playerID];
	civID = GameInfo.Civilizations[player:GetCivilizationType()].ID;
	if civID == 43 and (tech.ID == 12 or tech.ID == 31) then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ADDITIONAL_INTERNATIONAL_TRADE_ROUTE" ) );
			buttonNum = buttonNum + 1;
		end	
	end
	
	
	if (tech.ScenarioTechButton == 1) then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_SCENARIO_TECH_BUTTON_1" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if (tech.ScenarioTechButton == 2) then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_SCENARIO_TECH_BUTTON_2" ) );
			buttonNum = buttonNum + 1;
		end
	end


	if (tech.ScenarioTechButton == 3) then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_SCENARIO_TECH_BUTTON_3" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if (tech.ScenarioTechButton == 4) then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_SCENARIO_TECH_BUTTON_4" ) );
			buttonNum = buttonNum + 1;
		end
	end

	if (tech.ScenarioTechButton == 3) then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "Allows infantry to pillage" ) );
			buttonNum = buttonNum + 1;
		end
	end

	if (tech.ScenarioTechButton == 4) then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "Allows infantry to pillage" ) );
			buttonNum = buttonNum + 1;
		end
	end

	for row in GameInfo.Technology_TradeRouteDomainExtraRange(condition) do
		if (row.TechType == techType and row.Range > 0) then
			local buttonName = "B"..tostring(buttonNum);
			local thisButton = thisTechButtonInstance[buttonName];
			if thisButton then
				IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
				thisButton:SetHide( false );
				if (GameInfo.Domains[row.DomainType].ID == DomainTypes.DOMAIN_LAND) then
					thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_EXTENDS_LAND_TRADE_ROUTE_RANGE" ) );
				elseif (GameInfo.Domains[row.DomainType].ID == DomainTypes.DOMAIN_SEA) then
					thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_EXTENDS_SEA_TRADE_ROUTE_RANGE" ) );
				end
				buttonNum = buttonNum + 1;
			end	
		end
	end
	
	if tech.InfluenceSpreadModifier > 0 then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_DOUBLE_TOURISM" ) );
			buttonNum = buttonNum + 1;
		end	
	end
	
	if tech.AllowsWorldCongress then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_WORLD_CONGRESS" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.TriggersIdeology then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_TRIGGERS_IDEOLOGY" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.AllowsNuking then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_ALLOWS_NUKING" ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	if tech.ExtraVotesPerDiplomat > 0 then
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton then
			IconHookup( 0, textureSize, "GENERIC_FUNC_ATLAS", thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey( "TXT_KEY_EXTRA_VOTES_FROM_DIPLOMATS", tech.ExtraVotesPerDiplomat ) );
			buttonNum = buttonNum + 1;
		end
	end
	
	for row in GameInfo.Technology_FreePromotions(condition) do
		local promotion = GameInfo.UnitPromotions[row.PromotionType];
		local buttonName = "B"..tostring(buttonNum);
		local thisButton = thisTechButtonInstance[buttonName];
		if thisButton and promotion ~= nil then
			IconHookup( promotion.PortraitIndex, textureSize, promotion.IconAtlas, thisButton );
			thisButton:SetHide( false );
			thisButton:SetToolTipString( Locale.ConvertTextKey("TXT_KEY_FREE_PROMOTION_FROM_TECH", promotion.Description, promotion.Help) );
			buttonNum = buttonNum + 1;
		else
			break;
		end
	end
	
	return buttonNum;
	
end


function AddCallbackToSmallButtons( thisTechButtonInstance, maxSmallButtons, void1, void2, thisEvent, thisCallback )
	for buttonNum = 1, maxSmallButtons, 1 do
		local buttonName = "B"..tostring(buttonNum);
		thisTechButtonInstance[buttonName]:SetVoids(void1, void2);
		thisTechButtonInstance[buttonName]:RegisterCallback(thisEvent, thisCallback);
	end
end


function AdjustArtOnGrantedUnitButton( thisButton, thisUnitInfo, textureSize )
	-- if we have one, update the unit picture
	if thisButton then
		
		-- Tooltip
		local bIncludeRequirementsInfo = true;
		thisButton:SetToolTipString( GetHelpTextForUnit(thisUnitInfo.ID, bIncludeRequirementsInfo) );
		local portraitOffset, portraitAtlas = UI.GetUnitPortraitIcon(thisUnitInfo.ID);
		local textureOffset, textureSheet = IconLookup( portraitOffset, textureSize, portraitAtlas );				
		if textureOffset == nil then
			textureSheet = defaultErrorTextureSheet;
			textureOffset = nullOffset;
		end				
		thisButton:SetTexture( textureSheet );
		thisButton:SetTextureOffset( textureOffset );
		thisButton:SetHide( false );
		techPediaSearchStrings[tostring(thisButton)] = Locale.ConvertTextKey(thisUnitInfo.Description);
		thisButton:RegisterCallback( Mouse.eRClick, GetTechPedia );
	end
end


function AdjustArtOnGrantedBuildingButton( thisButton, thisBuildingInfo, textureSize )
	-- if we have one, update the building (or wonder) picture
	if thisButton then
		
		-- Tooltip
		local bExcludeName = false;
		local bExcludeHeader = false;
		thisButton:SetToolTipString( GetHelpTextForBuilding(thisBuildingInfo.ID, bExcludeName, bExcludeHeader, false, nil) );
		
		local textureOffset, textureSheet = IconLookup( thisBuildingInfo.PortraitIndex, textureSize, thisBuildingInfo.IconAtlas );				
		if textureOffset == nil then
			textureSheet = defaultErrorTextureSheet;
			textureOffset = nullOffset;
		end				
		thisButton:SetTexture( textureSheet );
		thisButton:SetTextureOffset( textureOffset );
		thisButton:SetHide( false );
		techPediaSearchStrings[tostring(thisButton)] = Locale.ConvertTextKey(thisBuildingInfo.Description);
		thisButton:RegisterCallback( Mouse.eRClick, GetTechPedia );
	end
end


function AdjustArtOnGrantedProjectButton( thisButton, thisProjectInfo, textureSize )
	-- if we have one, update the project picture
	if thisButton then
		
		-- Tooltip
		local bIncludeRequirementsInfo = true;
		thisButton:SetToolTipString( GetHelpTextForProject(thisProjectInfo.ID, bIncludeRequirementsInfo) );

		local textureOffset, textureSheet = IconLookup( thisProjectInfo.PortraitIndex, textureSize, thisProjectInfo.IconAtlas );				
		if textureOffset == nil then
			textureSheet = defaultErrorTextureSheet;
			textureOffset = nullOffset;
		end				
		thisButton:SetTexture( textureSheet );
		thisButton:SetTextureOffset( textureOffset );
		thisButton:SetHide( false );
		techPediaSearchStrings[tostring(thisButton)] = Locale.ConvertTextKey(thisProjectInfo.Description);
		thisButton:RegisterCallback( Mouse.eRClick, GetTechPedia );
	end
end


function AdjustArtOnGrantedResourceButton( thisButton, thisResourceInfo, textureSize )
	if thisButton then
		thisButton:SetToolTipString( Locale.ConvertTextKey("TXT_KEY_REVEALS_RESOURCE_ON_MAP", thisResourceInfo.Description)); 

		local textureOffset, textureSheet = IconLookup( thisResourceInfo.PortraitIndex, textureSize, thisResourceInfo.IconAtlas );				
		if textureOffset == nil then
			textureSheet = defaultErrorTextureSheet;
			textureOffset = nullOffset;
		end				
		thisButton:SetTexture( textureSheet );
		thisButton:SetTextureOffset( textureOffset );
		thisButton:SetHide( false );
		techPediaSearchStrings[tostring(thisButton)] =  Locale.Lookup(thisResourceInfo.Description);
		thisButton:RegisterCallback( Mouse.eRClick, GetTechPedia );
	end
end

function AdjustArtOnGrantedActionButton( thisButton, thisBuildInfo, textureSize )
	if thisButton then
		thisButton:SetToolTipString( Locale.ConvertTextKey( thisBuildInfo.Description ) );
		local textureOffset, textureSheet = IconLookup( thisBuildInfo.IconIndex, textureSize, thisBuildInfo.IconAtlas );				
		if textureOffset == nil then
			textureSheet = defaultErrorTextureSheet;
			textureOffset = nullOffset;
		end				
		thisButton:SetTexture( textureSheet );
		thisButton:SetTextureOffset( textureOffset );
		thisButton:SetHide(false);

		if thisBuildInfo.RouteType then
			techPediaSearchStrings[tostring(thisButton)] = Locale.ConvertTextKey( GameInfo.Routes[thisBuildInfo.RouteType].Description );
		elseif thisBuildInfo.ImprovementType then
			techPediaSearchStrings[tostring(thisButton)] = Locale.ConvertTextKey( GameInfo.Improvements[thisBuildInfo.ImprovementType].Description );
		else -- we are a choppy thing
			techPediaSearchStrings[tostring(thisButton)] = Locale.ConvertTextKey( GameInfo.Concepts["CONCEPT_WORKERS_CLEARINGLAND"].Description );
		end
		thisButton:RegisterCallback( Mouse.eRClick, GetTechPedia );
		--techPediaSearchStrings[tostring(thisButton)] = Locale.ConvertTextKey( thisBuildInfo.Description );
		--thisButton:RegisterCallback( Mouse.eRClick, GetTechPedia );
	end
end
