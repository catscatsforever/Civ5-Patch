------------------------------------------------------------------------------
--	FILE:	 Pangaea.lua
--	AUTHOR:  Bob Thomas
--	PURPOSE: Global map script - Simulates a Pan-Earth Supercontinent.
--           This is one of Civ5's featured map scripts.
------------------------------------------------------------------------------
--	Copyright (c) 2010 Firaxis Games, Inc. All rights reserved.
------------------------------------------------------------------------------

include("MapGenerator");
include("FractalWorld");
include("FeatureGenerator");
include("TerrainGenerator");

------------------------------------------------------------------------------
function GetMapScriptInfo()
	local world_age, temperature, rainfall, sea_level, resources = GetCoreMapOptions()
	return {
		Name = "Better Pangaea V5.2c",
		Description = "TXT_KEY_MAP_PANGAEA_HELP",
		IsAdvancedMap = false,
		IconIndex = 0,
		SortIndex = 2,
		CustomOptions = {world_age, temperature, rainfall,
			{
				Name = "TXT_KEY_MAP_OPTION_SEA_LEVEL",
				Values = {
					"TXT_KEY_MAP_OPTION_LOW",
					"TXT_KEY_MAP_OPTION_MEDIUM",
					"TXT_KEY_MAP_OPTION_HIGH",
					"TXT_KEY_MAP_OPTION_RANDOM",
				},
				DefaultValue = 1,
				SortPriority = -96,
			},
			{
				Name = "TXT_KEY_MAP_OPTION_RESOURCES",	-- Customizing the Resource setting to Default to Strategic Balance.
				Values = {
					"TXT_KEY_MAP_OPTION_SPARSE",
					"TXT_KEY_MAP_OPTION_STANDARD",
					"TXT_KEY_MAP_OPTION_ABUNDANT",
					"TXT_KEY_MAP_OPTION_LEGENDARY_START",
					"TXT_KEY_MAP_OPTION_STRATEGIC_BALANCE",
					"TXT_KEY_MAP_OPTION_STRATEGIC_BALANCE_WITH_COAL",
					"TXT_KEY_MAP_OPTION_RANDOM",
				},
				DefaultValue = 6,
				SortPriority = -95,
			},
			{
				Name = "TXT_KEY_MAP_OPTION_MORE_RIVERS",
				Values = {
					"TXT_KEY_YES_BUTTON",
					"TXT_KEY_NO_BUTTON",
				},
				DefaultValue = 1,
				SortPriority = -94,
			},
			{
				Name = "TXT_KEY_MAP_OPTION_EXTRA_RESOURCES",
				Values = {
					"1",
					"2",
					"3",
					"4",
					"5",
					"6",
					"7",
					"8",
					"9",
					"10",
					"11",
					"12",
				},
				DefaultValue = 5,
				SortPriority = -93,
			},
			{
				Name = "TXT_KEY_MAP_OPTION_MAX_MOUNTAINS",
				Values = {
					"3",
					"4",
					"5",
					"6",
					"7",
					"8",
					"9",
					"Uncapped",
				},
				DefaultValue = 4,
				SortPriority = -92,
			},
		},
	}
end
------------------------------------------------------------------------------

------------------------------------------------------------------------------
------------------------------------------------------------------------------
PangaeaFractalWorld = {};
-------------------------------------------------------------------------------------------
function FractalWorld:InitFractal(args)
	if(args == nil) then args = {}; end
	
	local continent_grain = args.continent_grain or 2;
	local rift_grain = args.rift_grain or -1; -- Default no rifts. Set grain to between 1 and 3 to add rifts. - Bob
	local invert_heights = args.invert_heights or false;
	local polar = args.polar or true;
	local ridge_flags = args.ridge_flags or self.iFlags;
	
	local fracFlags = {};
	
	if(invert_heights) then
		fracFlags.FRAC_INVERT_HEIGHTS = true;
	end
	
	if(polar) then
		fracFlags.FRAC_POLAR = true;
	end
	
	if(rift_grain > 0 and rift_grain < 4) then
		self.riftsFrac = Fractal.Create(self.iNumPlotsX, self.iNumPlotsY, rift_grain, {}, self.fracXExp, self.fracYExp);
		self.continentsFrac = Fractal.CreateRifts(self.iNumPlotsX, self.iNumPlotsY, continent_grain, fracFlags, self.riftsFrac, self.fracXExp, self.fracYExp);
	else
		self.continentsFrac = Fractal.Create(self.iNumPlotsX, self.iNumPlotsY, continent_grain, fracFlags, self.fracXExp, self.fracYExp);	
	end

	-- Use Brian's tectonics method to weave ridgelines in to the continental fractal.
	-- Without fractal variation, the tectonics come out too regular.
	--
	--[[ "The principle of the RidgeBuilder code is a modified Voronoi diagram. I 
	added some minor randomness and the slope might be a little tricky. It was 
	intended as a 'whole world' modifier to the fractal class. You can modify 
	the number of plates, but that is about it." ]]-- Brian Wade - May 23, 2009
	--
	local WorldSizeTypes = {};
	for row in GameInfo.Worlds() do
		WorldSizeTypes[row.Type] = row.ID;
	end
	local sizekey = Map.GetWorldSize();
	local sizevalues = {
		[WorldSizeTypes.WORLDSIZE_DUEL]     = 4,
		[WorldSizeTypes.WORLDSIZE_TINY]     = 8,
		[WorldSizeTypes.WORLDSIZE_SMALL]    = 16,
		[WorldSizeTypes.WORLDSIZE_STANDARD] = 20,
		[WorldSizeTypes.WORLDSIZE_LARGE]    = 24,
		[WorldSizeTypes.WORLDSIZE_HUGE]		= 32
	}
	--
	local numPlates = sizevalues[sizekey] or 4
	-- Blend a bit of ridge into the fractal.
	-- This will do things like roughen the coastlines and build inland seas. - Brian
	self.continentsFrac:BuildRidges(numPlates, ridge_flags, 1, 100);
end
------------------------------------------------------------------------------
function PangaeaFractalWorld.Create(fracXExp, fracYExp)
	local gridWidth, gridHeight = Map.GetGridSize();
	
	local data = {
		InitFractal = FractalWorld.InitFractal,
		ShiftPlotTypes = FractalWorld.ShiftPlotTypes,
		ShiftPlotTypesBy = FractalWorld.ShiftPlotTypesBy,
		DetermineXShift = FractalWorld.DetermineXShift,
		DetermineYShift = FractalWorld.DetermineYShift,
		GenerateCenterRift = FractalWorld.GenerateCenterRift,
		GeneratePlotTypes = PangaeaFractalWorld.GeneratePlotTypes,	-- Custom method
		
		iFlags = Map.GetFractalFlags(),
		
		fracXExp = fracXExp,
		fracYExp = fracYExp,
		
		iNumPlotsX = gridWidth,
		iNumPlotsY = gridHeight,
		plotTypes = table.fill(PlotTypes.PLOT_OCEAN, gridWidth * gridHeight)
	};
		
	return data;
end	
------------------------------------------------------------------------------
function PangaeaFractalWorld:GeneratePlotTypes(args)
	if(args == nil) then args = {}; end
	
	local sea_level_low = 63;
	local sea_level_normal = 68;
	local sea_level_high = 73;
	local world_age_old = 2;
	local world_age_normal = 3;
	local world_age_new = 5;
	--
	local extra_mountains = 6;
	local grain_amount = 3;
	local adjust_plates = 1.3;
	local shift_plot_types = true;
	local tectonic_islands = true;
	local hills_ridge_flags = self.iFlags;
	local peaks_ridge_flags = self.iFlags;
	local has_center_rift = false;
	
	local sea_level = Map.GetCustomOption(4)
	if sea_level == 4 then
		sea_level = 1 + Map.Rand(3, "Random Sea Level - Lua");
	end
	local world_age = Map.GetCustomOption(1)
	if world_age == 4 then
		world_age = 1 + Map.Rand(3, "Random World Age - Lua");
	end

	-- Set Sea Level according to user selection.
	local water_percent = sea_level_normal;
	if sea_level == 1 then -- Low Sea Level
		water_percent = sea_level_low
	elseif sea_level == 3 then -- High Sea Level
		water_percent = sea_level_high
	else -- Normal Sea Level
	end

	-- Set values for hills and mountains according to World Age chosen by user.
	local adjustment = world_age_normal;
	if world_age == 3 then -- 5 Billion Years
		adjustment = world_age_old;
		adjust_plates = adjust_plates * 0.75;
	elseif world_age == 1 then -- 3 Billion Years
		adjustment = world_age_new;
		adjust_plates = adjust_plates * 1.5;
	else -- 4 Billion Years
	end
	-- Apply adjustment to hills and peaks settings.
	local hillsBottom1 = 28 - adjustment;
	local hillsTop1 = 28 + adjustment;
	local hillsBottom2 = 72 - adjustment;
	local hillsTop2 = 72 + adjustment;
	local hillsClumps = 1 + adjustment;
	local hillsNearMountains = 91 - (adjustment * 2) - extra_mountains;
	local mountains = 97 - adjustment - extra_mountains;

	-- Hills and Mountains handled differently according to map size - Bob
	local WorldSizeTypes = {};
	for row in GameInfo.Worlds() do
		WorldSizeTypes[row.Type] = row.ID;
	end
	local sizekey = Map.GetWorldSize();
	-- Fractal Grains
	local sizevalues = {
		[WorldSizeTypes.WORLDSIZE_DUEL]     = 3,
		[WorldSizeTypes.WORLDSIZE_TINY]     = 3,
		[WorldSizeTypes.WORLDSIZE_SMALL]    = 4,
		[WorldSizeTypes.WORLDSIZE_STANDARD] = 4,
		[WorldSizeTypes.WORLDSIZE_LARGE]    = 5,
		[WorldSizeTypes.WORLDSIZE_HUGE]		= 5
	};
	local grain = sizevalues[sizekey] or 3;
	-- Tectonics Plate Counts
	local platevalues = {
		[WorldSizeTypes.WORLDSIZE_DUEL]		= 6,
		[WorldSizeTypes.WORLDSIZE_TINY]     = 9,
		[WorldSizeTypes.WORLDSIZE_SMALL]    = 12,
		[WorldSizeTypes.WORLDSIZE_STANDARD] = 18,
		[WorldSizeTypes.WORLDSIZE_LARGE]    = 24,
		[WorldSizeTypes.WORLDSIZE_HUGE]     = 30
	};
	local numPlates = platevalues[sizekey] or 5;
	-- Add in any plate count modifications passed in from the map script. - Bob
	numPlates = numPlates * adjust_plates;

	-- Generate continental fractal layer and examine the largest landmass. Reject
	-- the result until the largest landmass occupies 84% or more of the total land.
	local done = false;
	local iAttempts = 0;
	local iWaterThreshold, biggest_area, iNumTotalLandTiles, iNumBiggestAreaTiles, iBiggestID;
	while done == false do
		local grain_dice = Map.Rand(7, "Continental Grain roll - LUA Pangaea");
		if grain_dice < 4 then
			grain_dice = 1;
		else
			grain_dice = 2;
		end
		local rift_dice = Map.Rand(3, "Rift Grain roll - LUA Pangaea");
		if rift_dice < 1 then
			rift_dice = -1;
		end
		
		self.continentsFrac = nil;
		self:InitFractal{continent_grain = grain_dice, rift_grain = rift_dice};
		iWaterThreshold = self.continentsFrac:GetHeight(water_percent);
		
		iNumTotalLandTiles = 0;
		for x = 0, self.iNumPlotsX - 1 do
			for y = 0, self.iNumPlotsY - 1 do
				local i = y * self.iNumPlotsX + x;
				local val = self.continentsFrac:GetHeight(x, y);
				if(val <= iWaterThreshold) then
					self.plotTypes[i] = PlotTypes.PLOT_OCEAN;
				else
					self.plotTypes[i] = PlotTypes.PLOT_LAND;
					iNumTotalLandTiles = iNumTotalLandTiles + 1;
				end
			end
		end

		SetPlotTypes(self.plotTypes);
		Map.RecalculateAreas();
		
		biggest_area = Map.FindBiggestArea(false);
		iNumBiggestAreaTiles = biggest_area:GetNumTiles();
		-- Now test the biggest landmass to see if it is large enough.
		if iNumBiggestAreaTiles >= iNumTotalLandTiles * 0.84 then
			done = true;
			iBiggestID = biggest_area:GetID();
		end
		iAttempts = iAttempts + 1;
		
		--[[ Printout for debug use only
		print("-"); print("--- Pangaea landmass generation, Attempt#", iAttempts, "---");
		print("- This attempt successful: ", done);
		print("- Total Land Plots in world:", iNumTotalLandTiles);
		print("- Land Plots belonging to biggest landmass:", iNumBiggestAreaTiles);
		print("- Percentage of land belonging to Pangaea: ", 100 * iNumBiggestAreaTiles / iNumTotalLandTiles);
		print("- Continent Grain for this attempt: ", grain_dice);
		print("- Rift Grain for this attempt: ", rift_dice);
		print("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
		print(".");
		]]--
	end
	
	-- Generate fractals to govern hills and mountains
	self.hillsFrac = Fractal.Create(self.iNumPlotsX, self.iNumPlotsY, grain, self.iFlags, self.fracXExp, self.fracYExp);
	self.mountainsFrac = Fractal.Create(self.iNumPlotsX, self.iNumPlotsY, grain, self.iFlags, self.fracXExp, self.fracYExp);
	self.hillsFrac:BuildRidges(numPlates, hills_ridge_flags, 1, 2);
	self.mountainsFrac:BuildRidges((numPlates * 2) / 3, peaks_ridge_flags, 6, 1);
	-- Get height values
	local iHillsBottom1 = self.hillsFrac:GetHeight(hillsBottom1);
	local iHillsTop1 = self.hillsFrac:GetHeight(hillsTop1);
	local iHillsBottom2 = self.hillsFrac:GetHeight(hillsBottom2);
	local iHillsTop2 = self.hillsFrac:GetHeight(hillsTop2);
	local iHillsClumps = self.mountainsFrac:GetHeight(hillsClumps);
	local iHillsNearMountains = self.mountainsFrac:GetHeight(hillsNearMountains);
	local iMountainThreshold = self.mountainsFrac:GetHeight(mountains);
	local iPassThreshold = self.hillsFrac:GetHeight(hillsNearMountains);
	-- Get height values for tectonic islands
	local iMountain100 = self.mountainsFrac:GetHeight(100);
	local iMountain99 = self.mountainsFrac:GetHeight(99);
	local iMountain97 = self.mountainsFrac:GetHeight(97);
	local iMountain95 = self.mountainsFrac:GetHeight(95);

	-- Because we haven't yet shifted the plot types, we will not be able to take advantage 
	-- of having water and flatland plots already set. We still have to generate all data
	-- for hills and mountains, too, then shift everything, then set plots one more time.

	local map_plots_data = {}
	for y = 0, self.iNumPlotsY - 1 do	
		for x = 0, self.iNumPlotsX - 1 do
			table.insert(map_plots_data, {x, y});
		end
	end

	local randomized_map_plots_data;

	for iRunCounter = 1, 1 do
		randomized_map_plots_data = GetShuffledCopyOfTable(map_plots_data);
		for loop, plot_data in ipairs(randomized_map_plots_data) do
			local x, y = plot_data[1], plot_data[2];
			local iNumMountains = AreaMountainAnalyzer(self, x, y);
			--print("iNumMountains = ", iNumMountains)
		--end
	--end

	--for x = 0, self.iNumPlotsX - 1 do
		--for y = 0, self.iNumPlotsY - 1 do
		
			local i = y * self.iNumPlotsX + x;
			local val = self.continentsFrac:GetHeight(x, y);
			local mountainVal = self.mountainsFrac:GetHeight(x, y);
			local hillVal = self.hillsFrac:GetHeight(x, y);
			local iMountainsCap = 0;
			if Map.GetCustomOption(8) == 1 then
				iMountainsCap = 3;
			elseif Map.GetCustomOption(8) == 2 then
				iMountainsCap = 4;
			elseif Map.GetCustomOption(8) == 3 then
				iMountainsCap = 5;
			elseif Map.GetCustomOption(8) == 4 then
				iMountainsCap = 6;
			elseif Map.GetCustomOption(8) == 5 then
				iMountainsCap = 7;
			elseif Map.GetCustomOption(8) == 6 then
				iMountainsCap = 8;
			elseif Map.GetCustomOption(8) == 7 then
				iMountainsCap = 9;
			else
				iMountainsCap = 1000;
			end
	
			if(val <= iWaterThreshold) then
				self.plotTypes[i] = PlotTypes.PLOT_OCEAN;
				
				if tectonic_islands then -- Build islands in oceans along tectonic ridge lines - Brian
					if (mountainVal == iMountain100) then -- Isolated peak in the ocean
						self.plotTypes[i] = PlotTypes.PLOT_MOUNTAIN;
					elseif (mountainVal == iMountain99) then
						self.plotTypes[i] = PlotTypes.PLOT_HILLS;
					elseif (mountainVal == iMountain97) or (mountainVal == iMountain95) then
						self.plotTypes[i] = PlotTypes.PLOT_LAND;
					end
				end
					
			else
				if (mountainVal >= iMountainThreshold) then
					if (hillVal >= (iPassThreshold) or iNumMountains >= iMountainsCap) then -- Mountain Pass though the ridgeline - Brian
						if ((hillVal >= iHillsBottom1 and hillVal <= iHillsTop1) or (hillVal >= iHillsBottom2 and hillVal <= iHillsTop2)) then
							self.plotTypes[i] = PlotTypes.PLOT_HILLS;
						else
							self.plotTypes[i] = PlotTypes.PLOT_LAND;
						end
						--self.plotTypes[i] = PlotTypes.PLOT_HILLS;
					else -- Mountain
						self.plotTypes[i] = PlotTypes.PLOT_MOUNTAIN;
					end
				elseif (mountainVal >= iHillsNearMountains) then
					self.plotTypes[i] = PlotTypes.PLOT_HILLS; -- Foot hills - Bob
				else
					if ((hillVal >= iHillsBottom1 and hillVal <= iHillsTop1) or (hillVal >= iHillsBottom2 and hillVal <= iHillsTop2)) then
						self.plotTypes[i] = PlotTypes.PLOT_HILLS;
					else
						self.plotTypes[i] = PlotTypes.PLOT_LAND;
					end
				end
			end
		end
	end

	self:ShiftPlotTypes();
	
	-- Now shift everything toward one of the poles, to reduce how much jungles tend to dominate this script.
	local shift_dice = Map.Rand(2, "Shift direction - LUA Pangaea");
	local iStartRow, iNumRowsToShift;
	local bFoundPangaea, bDoShift = false, false;
	if shift_dice == 1 then
		-- Shift North
		for y = self.iNumPlotsY - 2, 1, -1 do
			for x = 0, self.iNumPlotsX - 1 do
				local i = y * self.iNumPlotsX + x;
				if self.plotTypes[i] == PlotTypes.PLOT_HILLS or self.plotTypes[i] == PlotTypes.PLOT_LAND then
					local plot = Map.GetPlot(x, y);
					local iAreaID = plot:GetArea();
					if iAreaID == iBiggestID then
						bFoundPangaea = true;
						iStartRow = y + 1;
						if iStartRow < self.iNumPlotsY - 4 then -- Enough rows of water space to do a shift.
							bDoShift = true;
						end
						break
					end
				end
			end
			-- Check to see if we've found the Pangaea.
			if bFoundPangaea == true then
				break
			end
		end
	else
		-- Shift South
		for y = 1, self.iNumPlotsY - 2 do
			for x = 0, self.iNumPlotsX - 1 do
				local i = y * self.iNumPlotsX + x;
				if self.plotTypes[i] == PlotTypes.PLOT_HILLS or self.plotTypes[i] == PlotTypes.PLOT_LAND then
					local plot = Map.GetPlot(x, y);
					local iAreaID = plot:GetArea();
					if iAreaID == iBiggestID then
						bFoundPangaea = true;
						iStartRow = y - 1;
						if iStartRow > 3 then -- Enough rows of water space to do a shift.
							bDoShift = true;
						end
						break
					end
				end
			end
			-- Check to see if we've found the Pangaea.
			if bFoundPangaea == true then
				break
			end
		end
	end
	if bDoShift == true then
		if shift_dice == 1 then -- Shift North
			local iRowsDifference = self.iNumPlotsY - iStartRow - 2;
			local iRowsInPlay = math.floor(iRowsDifference * 0.7);
			local iRowsBase = math.ceil(iRowsDifference * 0.3);
			local rows_dice = Map.Rand(iRowsInPlay, "Number of Rows to Shift - LUA Pangaea");
			local iNumRows = math.min(iRowsDifference - 1, iRowsBase + rows_dice);
			local iNumEvenRows = 2 * math.floor(iNumRows / 2); -- MUST be an even number or we risk breaking a 1-tile isthmus and splitting the Pangaea.
			local iNumRowsToShift = math.max(2, iNumEvenRows);
			--print("-"); print("Shifting lands northward by this many plots: ", iNumRowsToShift); print("-");
			-- Process from top down.
			for y = (self.iNumPlotsY - 1) - iNumRowsToShift, 0, -1 do
				for x = 0, self.iNumPlotsX - 1 do
					local sourcePlotIndex = y * self.iNumPlotsX + x + 1;
					local destPlotIndex = (y + iNumRowsToShift) * self.iNumPlotsX + x + 1;
					self.plotTypes[destPlotIndex] = self.plotTypes[sourcePlotIndex]
				end
			end
			for y = 0, iNumRowsToShift - 1 do
				for x = 0, self.iNumPlotsX - 1 do
					local i = y * self.iNumPlotsX + x + 1;
					self.plotTypes[i] = PlotTypes.PLOT_OCEAN;
				end
			end
		else -- Shift South
			local iRowsDifference = iStartRow - 1;
			local iRowsInPlay = math.floor(iRowsDifference * 0.7);
			local iRowsBase = math.ceil(iRowsDifference * 0.3);
			local rows_dice = Map.Rand(iRowsInPlay, "Number of Rows to Shift - LUA Pangaea");
			local iNumRows = math.min(iRowsDifference - 1, iRowsBase + rows_dice);
			local iNumEvenRows = 2 * math.floor(iNumRows / 2); -- MUST be an even number or we risk breaking a 1-tile isthmus and splitting the Pangaea.
			local iNumRowsToShift = math.max(2, iNumEvenRows);
			--print("-"); print("Shifting lands southward by this many plots: ", iNumRowsToShift); print("-");
			-- Process from bottom up.
			for y = 0, (self.iNumPlotsY - 1) - iNumRowsToShift do
				for x = 0, self.iNumPlotsX - 1 do
					local sourcePlotIndex = (y + iNumRowsToShift) * self.iNumPlotsX + x + 1;
					local destPlotIndex = y * self.iNumPlotsX + x + 1;
					self.plotTypes[destPlotIndex] = self.plotTypes[sourcePlotIndex]
				end
			end
			for y = self.iNumPlotsY - iNumRowsToShift, self.iNumPlotsY - 1 do
				for x = 0, self.iNumPlotsX - 1 do
					local i = y * self.iNumPlotsX + x + 1;
					self.plotTypes[i] = PlotTypes.PLOT_OCEAN;
				end
			end
		end
	end

	return self.plotTypes;
end
------------------------------------------------------------------------------

------------------------------------------------------------------------------
function GeneratePlotTypes()
	-- Plot generation customized to ensure enough land belongs to the Pangaea.
	print("Generating Plot Types (Lua Pangaea) ...");
	
	local fractal_world = PangaeaFractalWorld.Create();
	local plotTypes = fractal_world:GeneratePlotTypes();
	
	SetPlotTypes(plotTypes);
	GenerateCoasts();
end
------------------------------------------------------------------------------
function TerrainGenerator.Create(args)
	--[[ Civ4's truncated "Climate" setting has been abandoned. Civ5 has returned to 
	Civ3-style map options for World Age, Temperature, and Rainfall. Control over the 
	terrain has been removed from the XML.  - Bob Thomas, March 2010  ]]--
	--
	-- Sea Level and World Age map options affect only plot generation.
	-- Temperature map options affect only terrain generation.
	-- Rainfall map options affect only feature generation.
	--
	local args = args or {};
	local temperature = args.temperature or 2; -- Default setting is Temperate.
	local fracXExp = args.fracXExp or -1;
	local fracYExp = args.fracYExp or -1;
	local grain_amount = args.grain_amount or 3;
	
	-- These settings offer a limited ability for map scripts to modify terrain.
	-- Where these are inadequate, replace the TerrainGenerator with a custom method.
	local temperature_shift = args.temperature_shift or 0.1;
	local desert_shift = args.desert_shift or 16;
	
	-- Set terrain bands.
	local iDesertPercent = args.iDesertPercent or 29;
	local iPlainsPercent = args.iPlainsPercent or 50; -- Deserts are processed first, so Plains will take this percentage of whatever remains. - Bob
	local fSnowLatitude  = args.fSnowLatitude  or 0.80;
	local fTundraLatitude = args.fTundraLatitude or 0.65;
	local fGrassLatitude = args.fGrassLatitude or 0.1; -- Above this is actually the latitude where it stops being all grass. - Bob
	local fDesertBottomLatitude = args.fDesertBottomLatitude or 0.2;
	local fDesertTopLatitude = args.fDesertTopLatitude or 0.5;
	-- Adjust terrain bands according to user's Temperature selection. (Which must be passed in by the map script.)
	if temperature == 1 then -- World Temperature is Cool.
		iDesertPercent = iDesertPercent - desert_shift;
		fTundraLatitude = fTundraLatitude - (temperature_shift * 1.5);
		fDesertTopLatitude = fDesertTopLatitude - temperature_shift;
		fGrassLatitude = fGrassLatitude - (temperature_shift * 0.5);
	elseif temperature == 3 then -- World Temperature is Hot.
		iDesertPercent = iDesertPercent + desert_shift;
		fSnowLatitude  = fSnowLatitude + (temperature_shift * 0.5);
		fTundraLatitude = fTundraLatitude + temperature_shift;
		fDesertTopLatitude = fDesertTopLatitude + temperature_shift;
		fGrassLatitude = fGrassLatitude - (temperature_shift * 0.5);
	else -- Normal Temperature.
	end
	
	--[[ Activate printout for debugging only
	print("-"); print("- Desert Percentage:", iDesertPercent);
	print("--- Latitude Readout ---");
	print("- All Grass End Latitude:", fGrassLatitude);
	print("- Desert Start Latitude:", fDesertBottomLatitude);
	print("- Desert End Latitude:", fDesertTopLatitude);
	print("- Tundra Start Latitude:", fTundraLatitude);
	print("- Snow Start Latitude:", fSnowLatitude);
	print("- - - - - - - - - - - - - -");
	]]--

	local gridWidth, gridHeight = Map.GetGridSize();
	local world_info = GameInfo.Worlds[Map.GetWorldSize()];

	local data = {
	
		-- member methods
		InitFractals			= TerrainGenerator.InitFractals,
		GetLatitudeAtPlot		= TerrainGenerator.GetLatitudeAtPlot,
		GenerateTerrain			= TerrainGenerator.GenerateTerrain,
		GenerateTerrainAtPlot	= TerrainGenerator.GenerateTerrainAtPlot,
	
		-- member variables
		grain_amount	= grain_amount,
		fractalFlags	= Map.GetFractalFlags(), 
		iWidth			= gridWidth,
		iHeight			= gridHeight,
		
		iDesertPercent	= iDesertPercent,
		iPlainsPercent	= iPlainsPercent,

		iDesertTopPercent		= 100,
		iDesertBottomPercent	= math.max(0, math.floor(100-iDesertPercent)),
		iPlainsTopPercent		= 100,
		iPlainsBottomPercent	= math.max(0, math.floor(100-iPlainsPercent)),
		
		fSnowLatitude			= fSnowLatitude,
		fTundraLatitude			= fTundraLatitude,
		fGrassLatitude			= fGrassLatitude,
		fDesertBottomLatitude	= fDesertBottomLatitude,
		fDesertTopLatitude		= fDesertTopLatitude,
		
		fracXExp		= fracXExp,
		fracYExp		= fracYExp,
		
	}

	data:InitFractals();
	
	return data;
end
----------------------------------------------------------------------------------	
function GenerateTerrain()
	
	-- Get Temperature setting input by user.
	local temp = Map.GetCustomOption(2)
	local iDesertPer = 29; -- 32
	local iPlainsPer = 50; -- Deserts are processed first, so Plains will take this percentage of whatever remains. - Bob
	local fSnowLat = 0.80; -- 0.75
	local fTundraLat = 0.65; -- 0.6
	if temp == 4 then
		temp = 1 + Map.Rand(3, "Random Temperature - Lua");
	end

	local args = {
		temperature = temp,
		iDesertPercent = iDesertPer,
		-- iPlainsPercent = iPlainsPer,
		fSnowLatitude = fSnowLat,
		fTundraLatitude = fTundraLat
		};

	local terraingen = TerrainGenerator.Create(args);

	terrainTypes = terraingen:GenerateTerrain();
	
	SetTerrainTypes(terrainTypes);
end
------------------------------------------------------------------------------
function AddFeatures()
	print("Adding Features (Lua Pangaea) ...");

	-- Get Rainfall setting input by user.
	local rain = Map.GetCustomOption(3)
	if rain == 4 then
		rain = 1 + Map.Rand(3, "Random Rainfall - Lua");
	end
	
	local args = {rainfall = rain}
	local featuregen = FeatureGenerator.Create(args);

	-- False parameter removes mountains from coastlines.
	featuregen:AddFeatures(false);
end
------------------------------------------------------------------------------
function AssignStartingPlots:PlaceStrategicAndBonusResources()
	-- KEY: {Resource ID, Quantity (0 = unquantified), weighting, minimum radius, maximum radius}
	-- KEY: (frequency (1 per n plots in the list), impact list number, plot list, resource data)
	--
	-- The radius creates a zone around the plot that other resources of that
	-- type will avoid if possible. See ProcessResourceList for impact numbers.
	--
	-- Order of placement matters, so changing the order may affect a later dependency.
	
	-- Adjust amounts, if applicable, based on Resource Setting.
	local uran_amt, horse_amt, oil_amt, iron_amt, coal_amt, alum_amt = self:GetMajorStrategicResourceQuantityValues()
	local res = Map.GetCustomOption(5)

	-- Adjust appearance rate per Resource Setting chosen by user.
	local bonus_multiplier = 1;
	if res == 1 then -- Sparse, so increase the number of tiles per bonus.
		bonus_multiplier = 1.5;
	elseif res == 3 then -- Abundant, so reduce the number of tiles per bonus.
		bonus_multiplier = 0.66667;
	end

	-- Place Strategic resources.
	print("Map Generation - Placing Strategics");
	local resources_to_place = {
	{self.oil_ID, oil_amt, 65, 1, 1},
	{self.uranium_ID, uran_amt, 35, 0, 1} };
	self:ProcessResourceList(9, 1, self.marsh_list, resources_to_place)

	local resources_to_place = {
	{self.oil_ID, oil_amt, 40, 1, 2},
	{self.aluminum_ID, alum_amt, 15, 1, 2},
	{self.iron_ID, iron_amt, 45, 1, 2} };
	self:ProcessResourceList(16, 1, self.tundra_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.oil_ID, oil_amt, 60, 1, 1},
	{self.aluminum_ID, alum_amt, 15, 2, 3},
	{self.iron_ID, iron_amt, 25, 2, 3} };
	self:ProcessResourceList(17, 1, self.snow_flat_list, resources_to_place)

	local resources_to_place = {
	{self.oil_ID, oil_amt, 65, 0, 1},
	{self.iron_ID, iron_amt, 35, 1, 1} };
	self:ProcessResourceList(13, 1, self.desert_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.iron_ID, iron_amt, 26, 0, 2},
	{self.coal_ID, coal_amt, 35, 1, 3},
	{self.aluminum_ID, alum_amt, 39, 2, 3} };
	self:ProcessResourceList(22, 1, self.hills_list, resources_to_place)

	local resources_to_place = {
	{self.coal_ID, coal_amt, 30, 1, 2},
	{self.uranium_ID, uran_amt, 70, 1, 2} };
	self:ProcessResourceList(33, 1, self.jungle_flat_list, resources_to_place)
	local resources_to_place = {
	{self.coal_ID, coal_amt, 30, 1, 2},
	{self.uranium_ID, uran_amt, 70, 1, 1} };
	self:ProcessResourceList(39, 1, self.forest_flat_list, resources_to_place)

	local resources_to_place = {
	{self.horse_ID, horse_amt, 100, 2, 5} };
	self:ProcessResourceList(33, 1, self.dry_grass_flat_no_feature, resources_to_place)
	local resources_to_place = {
	{self.horse_ID, horse_amt, 100, 1, 4} };
	self:ProcessResourceList(33, 1, self.plains_flat_no_feature, resources_to_place)

	self:AddModernMinorStrategicsToCityStates() -- Added spring 2011
	
	self:PlaceSmallQuantitiesOfStrategics(23 * bonus_multiplier, self.land_list);
	
	self:PlaceOilInTheSea();

	
	-- Check for low or missing Strategic resources
	if self.amounts_of_resources_placed[self.iron_ID + 1] < 8 then
		--print("Map has very low iron, adding another.");
		local resources_to_place = { {self.iron_ID, iron_amt, 100, 0, 0} };
		self:ProcessResourceList(99999, 1, self.hills_list, resources_to_place) -- 99999 means one per that many tiles: a single instance.
	end
	if self.amounts_of_resources_placed[self.iron_ID + 1] < 4 * self.iNumCivs then
		--print("Map has very low iron, adding another.");
		local resources_to_place = { {self.iron_ID, iron_amt, 100, 0, 0} };
		self:ProcessResourceList(99999, 1, self.land_list, resources_to_place)
	end
	if self.amounts_of_resources_placed[self.horse_ID + 1] < 4 * self.iNumCivs then
		--print("Map has very low horse, adding another.");
		local resources_to_place = { {self.horse_ID, horse_amt, 100, 0, 0} };
		self:ProcessResourceList(99999, 1, self.plains_flat_no_feature, resources_to_place)
	end
	if self.amounts_of_resources_placed[self.horse_ID + 1] < 4 * self.iNumCivs then
		--print("Map has very low horse, adding another.");
		local resources_to_place = { {self.horse_ID, horse_amt, 100, 0, 0} };
		self:ProcessResourceList(99999, 1, self.dry_grass_flat_no_feature, resources_to_place)
	end
	if self.amounts_of_resources_placed[self.coal_ID + 1] < 8 then
		--print("Map has very low coal, adding another.");
		local resources_to_place = { {self.coal_ID, coal_amt, 100, 0, 0} };
		self:ProcessResourceList(99999, 1, self.hills_list, resources_to_place)
	end
	if self.amounts_of_resources_placed[self.coal_ID + 1] < 4 * self.iNumCivs then
		--print("Map has very low coal, adding another.");
		local resources_to_place = { {self.coal_ID, coal_amt, 100, 0, 0} };
		self:ProcessResourceList(99999, 1, self.land_list, resources_to_place)
	end
	if self.amounts_of_resources_placed[self.oil_ID + 1] < 4 * self.iNumCivs then
		--print("Map has very low oil, adding another.");
		local resources_to_place = { {self.oil_ID, oil_amt, 100, 0, 0} };
		self:ProcessResourceList(99999, 1, self.land_list, resources_to_place)
	end
	if self.amounts_of_resources_placed[self.aluminum_ID + 1] < 4 * self.iNumCivs then
		--print("Map has very low aluminum, adding another.");
		local resources_to_place = { {self.aluminum_ID, alum_amt, 100, 0, 0} };
		self:ProcessResourceList(99999, 1, self.hills_list, resources_to_place)
	end
	if self.amounts_of_resources_placed[self.uranium_ID + 1] < 2 * self.iNumCivs then
		--print("Map has very low uranium, adding another.");
		local resources_to_place = { {self.uranium_ID, uran_amt, 100, 0, 0} };
		self:ProcessResourceList(99999, 1, self.land_list, resources_to_place)
	end
	
	
	-- Place Bonus Resources
	print("Map Generation - Placing Bonuses");
	self:PlaceFish(10 * bonus_multiplier, self.coast_list);
	self:PlaceSexyBonusAtCivStarts()
	self:AddExtraBonusesToHillsRegions()
	
	local resources_to_place = {
	{self.deer_ID, 1, 100, 1, 2} };
	self:ProcessResourceList(8 * bonus_multiplier, 3, self.extra_deer_list, resources_to_place)

	local resources_to_place = {
	{self.wheat_ID, 1, 100, 0, 2} };
	self:ProcessResourceList(10 * bonus_multiplier, 3, self.desert_wheat_list, resources_to_place)

	local resources_to_place = {
	{self.deer_ID, 1, 100, 1, 2} };
	self:ProcessResourceList(12 * bonus_multiplier, 3, self.tundra_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.banana_ID, 1, 100, 0, 3} };
	self:ProcessResourceList(14 * bonus_multiplier, 3, self.banana_list, resources_to_place)

	local resources_to_place = {
	{self.wheat_ID, 1, 100, 2, 3} };
	self:ProcessResourceList(50 * bonus_multiplier, 3, self.plains_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.bison_ID, 1, 100, 2, 3} };
	self:ProcessResourceList(60 * bonus_multiplier, 3, self.plains_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.cow_ID, 1, 100, 1, 2} };
	self:ProcessResourceList(18 * bonus_multiplier, 3, self.grass_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.stone_ID, 1, 100, 1, 1} };
	self:ProcessResourceList(30 * bonus_multiplier, 3, self.dry_grass_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.bison_ID, 1, 100, 1, 1} };
	self:ProcessResourceList(50 * bonus_multiplier, 3, self.dry_grass_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.sheep_ID, 1, 100, 1, 1} };
	self:ProcessResourceList(13 * bonus_multiplier, 3, self.hills_open_list, resources_to_place)

	local resources_to_place = {
	{self.stone_ID, 1, 100, 1, 2} };
	self:ProcessResourceList(15 * bonus_multiplier, 3, self.tundra_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.stone_ID, 1, 100, 1, 2} };
	self:ProcessResourceList(19 * bonus_multiplier, 3, self.desert_flat_no_feature, resources_to_place)

	local resources_to_place = {
	{self.deer_ID, 1, 100, 3, 4} };
	self:ProcessResourceList(25 * bonus_multiplier, 3, self.forest_flat_that_are_not_tundra, resources_to_place)
end
------------------------------------------------------------------------------
function AssignStartingPlots:NormalizeStartLocation(region_number)
	--[[ This function measures the value of land in two rings around a given start
	     location, primarily for the purpose of determining how much support the site
	     requires in the form of Bonus Resources. Numerous assumptions are built in 
	     to this operation that would need to be adjusted for any modifications to 
	     terrain or resources types and yields, or to game rules about rivers and 
	     other map elements. Nothing is hardcoded in a way that puts it out of the 
	     reach of modders, but any mods including changes to map elements may have a
	     significant workload involved with rebalancing the start finder and the 
	     resource distribution to fit them properly to a mod's custom needs. I have
	     labored to document every function and method in detail to make it as easy
	     as possible to modify this system.  -- Bob Thomas - April 15, 2010  ]]--
	-- 
	local iW, iH = Map.GetGridSize();
	local start_point_data = self.startingPlots[region_number];
	local x = start_point_data[1];
	local y = start_point_data[2];
	local plot = Map.GetPlot(x, y);
	local plotIndex = y * iW + x + 1;
	local isEvenY = true;
	if y / 2 > math.floor(y / 2) then
		isEvenY = false;
	end
	local wrapX = Map:IsWrapX();
	local wrapY = Map:IsWrapY();
	local innerFourFood, innerThreeFood, innerTwoFood, innerHills, innerForest, innerOneHammer, innerOcean = 0, 0, 0, 0, 0, 0, 0;
	local outerFourFood, outerThreeFood, outerTwoFood, outerHills, outerForest, outerOneHammer, outerOcean = 0, 0, 0, 0, 0, 0, 0;
	local innerCanHaveBonus, outerCanHaveBonus, innerBadTiles, outerBadTiles = 0, 0, 0, 0;
	local iNumFoodBonusNeeded = 0;
	local iNumNativeTwoFoodFirstRing, iNumNativeTwoFoodSecondRing = 0, 0; -- Cities must begin the game with at least three native 2F tiles, one in first ring.
	local search_table = {};
	
	-- Remove any feature Ice from the first ring.
	self:GenerateLuxuryPlotListsAtCitySite(x, y, 1, true)
	
	-- Set up Conditions checks.
	local alongOcean = false;
	local nextToLake = false;
	local isRiver = false;
	local nearRiver = false;
	local nearMountain = false;
	local forestCount, jungleCount = 0, 0;
	local res = Map.GetCustomOption(5);

	-- Check start plot to see if it's adjacent to saltwater.
	if self.plotDataIsCoastal[plotIndex] == true then
		alongOcean = true;
	end
	
	-- Check start plot to see if it's on a river.
	if plot:IsRiver() then
		isRiver = true;
	end

	-- Data Chart for early game tile potentials
	--
	-- 4F:	Flood Plains, Grass on fresh water (includes forest and marsh).
	-- 3F:	Dry Grass, Plains on fresh water (includes forest and jungle), Tundra on fresh water (includes forest), Oasis
	-- 2F:  Dry Plains, Lake, all remaining Jungles.
	--
	-- 1H:	Plains, Jungle on Plains

	-- Adding evaluation of grassland and plains for balance boost of bonus Cows for heavy grass starts. -1/26/2011 BT
	local iNumGrass, iNumPlains = 0, 0;

	-- Evaluate First Ring
	if isEvenY then
		search_table = self.firstRingYIsEven;
	else
		search_table = self.firstRingYIsOdd;
	end

	for loop, plot_adjustments in ipairs(search_table) do
		local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
		--
		if searchX < 0 or searchX >= iW or searchY < 0 or searchY >= iH then
			-- This plot does not exist. It's off the map edge.
			innerBadTiles = innerBadTiles + 1;
		else
			local searchPlot = Map.GetPlot(searchX, searchY)
			local plotType = searchPlot:GetPlotType()
			local terrainType = searchPlot:GetTerrainType()
			local featureType = searchPlot:GetFeatureType()
			--
			if plotType == PlotTypes.PLOT_MOUNTAIN then
				local nearMountain = true;
				innerBadTiles = innerBadTiles + 1;
			elseif plotType == PlotTypes.PLOT_OCEAN then
				if searchPlot:IsLake() then
					nextToLake = true;
					if featureType == FeatureTypes.FEATURE_ICE then
						innerBadTiles = innerBadTiles + 1;
					else
						innerTwoFood = innerTwoFood + 1;
						iNumNativeTwoFoodFirstRing = iNumNativeTwoFoodFirstRing + 1;
					end
				else
					if featureType == FeatureTypes.FEATURE_ICE then
						innerBadTiles = innerBadTiles + 1;
					else
						innerOcean = innerOcean + 1;
						innerCanHaveBonus = innerCanHaveBonus + 1;
					end
				end
			else -- Habitable plot.
				if featureType == FeatureTypes.FEATURE_JUNGLE then
					jungleCount = jungleCount + 1;
					iNumNativeTwoFoodFirstRing = iNumNativeTwoFoodFirstRing + 1;
				elseif featureType == FeatureTypes.FEATURE_FOREST then
					forestCount = forestCount + 1;
				end
				if searchPlot:IsRiver() then
					nearRiver = true;
				end
				if plotType == PlotTypes.PLOT_HILLS then
					innerHills = innerHills + 1;
					if featureType == FeatureTypes.FEATURE_JUNGLE then
						innerTwoFood = innerTwoFood + 1;
						innerCanHaveBonus = innerCanHaveBonus + 1;
					elseif featureType == FeatureTypes.FEATURE_FOREST then
						innerCanHaveBonus = innerCanHaveBonus + 1;
					elseif terrainType == TerrainTypes.TERRAIN_GRASS then
						iNumGrass = iNumGrass + 1;
					elseif terrainType == TerrainTypes.TERRAIN_PLAINS then
						iNumPlains = iNumPlains + 1;
					end
				elseif featureType == FeatureTypes.FEATURE_OASIS then
					innerThreeFood = innerThreeFood + 1;
					iNumNativeTwoFoodFirstRing = iNumNativeTwoFoodFirstRing + 1;
				elseif searchPlot:IsFreshWater() then
					if terrainType == TerrainTypes.TERRAIN_GRASS then
						innerFourFood = innerFourFood + 1;
						iNumGrass = iNumGrass + 1;
						if featureType ~= FeatureTypes.FEATURE_MARSH then
							innerCanHaveBonus = innerCanHaveBonus + 1;
						end
						if featureType == FeatureTypes.FEATURE_FOREST then
							innerForest = innerForest + 1;
						end
						if featureType == FeatureTypes.NO_FEATURE then
							iNumNativeTwoFoodFirstRing = iNumNativeTwoFoodFirstRing + 1;
						end
					elseif featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then
						innerFourFood = innerFourFood + 1;
						innerCanHaveBonus = innerCanHaveBonus + 1;
						iNumNativeTwoFoodFirstRing = iNumNativeTwoFoodFirstRing + 1;
					elseif terrainType == TerrainTypes.TERRAIN_PLAINS then
						innerThreeFood = innerThreeFood + 1;
						innerCanHaveBonus = innerCanHaveBonus + 1;
						iNumPlains = iNumPlains + 1;
						if featureType == FeatureTypes.FEATURE_FOREST then
							innerForest = innerForest + 1;
						else
							innerOneHammer = innerOneHammer + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_TUNDRA then
						innerThreeFood = innerThreeFood + 1;
						innerCanHaveBonus = innerCanHaveBonus + 1;
						if featureType == FeatureTypes.FEATURE_FOREST then
							innerForest = innerForest + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_DESERT then
						innerBadTiles = innerBadTiles + 1;
						innerCanHaveBonus = innerCanHaveBonus + 1; -- Can have Oasis.
					else -- Snow
						innerBadTiles = innerBadTiles + 1;
					end
				else -- Dry Flatlands
					if terrainType == TerrainTypes.TERRAIN_GRASS then
						innerThreeFood = innerThreeFood + 1;
						iNumGrass = iNumGrass + 1;
						if featureType ~= FeatureTypes.FEATURE_MARSH then
							innerCanHaveBonus = innerCanHaveBonus + 1;
						end
						if featureType == FeatureTypes.FEATURE_FOREST then
							innerForest = innerForest + 1;
						end
						if featureType == FeatureTypes.NO_FEATURE then
							iNumNativeTwoFoodFirstRing = iNumNativeTwoFoodFirstRing + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_PLAINS then
						innerTwoFood = innerTwoFood + 1;
						innerCanHaveBonus = innerCanHaveBonus + 1;
						iNumPlains = iNumPlains + 1;
						if featureType == FeatureTypes.FEATURE_FOREST then
							innerForest = innerForest + 1;
						else
							innerOneHammer = innerOneHammer + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_TUNDRA then
						innerCanHaveBonus = innerCanHaveBonus + 1;
						if featureType == FeatureTypes.FEATURE_FOREST then
							innerForest = innerForest + 1;
						else
							innerBadTiles = innerBadTiles + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_DESERT then
						innerBadTiles = innerBadTiles + 1;
						innerCanHaveBonus = innerCanHaveBonus + 1; -- Can have Oasis.
					else -- Snow
						innerBadTiles = innerBadTiles + 1;
					end
				end
			end
		end
	end
				
	-- Evaluate Second Ring
	if isEvenY then
		search_table = self.secondRingYIsEven;
	else
		search_table = self.secondRingYIsOdd;
	end

	for loop, plot_adjustments in ipairs(search_table) do
		local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
		local plot = Map.GetPlot(x, y);
		--
		--
		if searchX < 0 or searchX >= iW or searchY < 0 or searchY >= iH then
			-- This plot does not exist. It's off the map edge.
			outerBadTiles = outerBadTiles + 1;
		else
			local searchPlot = Map.GetPlot(searchX, searchY)
			local plotType = searchPlot:GetPlotType()
			local terrainType = searchPlot:GetTerrainType()
			local featureType = searchPlot:GetFeatureType()
			--
			if plotType == PlotTypes.PLOT_MOUNTAIN then
				local nearMountain = true;
				outerBadTiles = outerBadTiles + 1;
			elseif plotType == PlotTypes.PLOT_OCEAN then
				if searchPlot:IsLake() then
					if featureType == FeatureTypes.FEATURE_ICE then
						outerBadTiles = outerBadTiles + 1;
					else
						outerTwoFood = outerTwoFood + 1;
						iNumNativeTwoFoodSecondRing = iNumNativeTwoFoodSecondRing + 1;
					end
				else
					if featureType == FeatureTypes.FEATURE_ICE then
						outerBadTiles = outerBadTiles + 1;
					elseif terrainType == TerrainTypes.TERRAIN_COAST then
						outerCanHaveBonus = outerCanHaveBonus + 1;
						outerOcean = outerOcean + 1;
					end
				end
			else -- Habitable plot.
				if featureType == FeatureTypes.FEATURE_JUNGLE then
					jungleCount = jungleCount + 1;
					iNumNativeTwoFoodSecondRing = iNumNativeTwoFoodSecondRing + 1;
				elseif featureType == FeatureTypes.FEATURE_FOREST then
					forestCount = forestCount + 1;
				end
				if searchPlot:IsRiver() then
					nearRiver = true;
				end
				if plotType == PlotTypes.PLOT_HILLS then
					outerHills = outerHills + 1;
					if featureType == FeatureTypes.FEATURE_JUNGLE then
						outerTwoFood = outerTwoFood + 1;
						outerCanHaveBonus = outerCanHaveBonus + 1;
					elseif featureType == FeatureTypes.FEATURE_FOREST then
						outerCanHaveBonus = outerCanHaveBonus + 1;
					elseif terrainType == TerrainTypes.TERRAIN_GRASS then
						iNumGrass = iNumGrass + 1;
					elseif terrainType == TerrainTypes.TERRAIN_PLAINS then
						iNumPlains = iNumPlains + 1;
					end
				elseif featureType == FeatureTypes.FEATURE_OASIS then
					innerThreeFood = innerThreeFood + 1;
					iNumNativeTwoFoodSecondRing = iNumNativeTwoFoodSecondRing + 1;
				elseif searchPlot:IsFreshWater() then
					if terrainType == TerrainTypes.TERRAIN_GRASS then
						outerFourFood = outerFourFood + 1;
						iNumGrass = iNumGrass + 1;
						if featureType ~= FeatureTypes.FEATURE_MARSH then
							outerCanHaveBonus = outerCanHaveBonus + 1;
						end
						if featureType == FeatureTypes.FEATURE_FOREST then
							outerForest = outerForest + 1;
						end
						if featureType == FeatureTypes.NO_FEATURE then
							iNumNativeTwoFoodSecondRing = iNumNativeTwoFoodSecondRing + 1;
						end
					elseif featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then
						outerFourFood = outerFourFood + 1;
						outerCanHaveBonus = outerCanHaveBonus + 1;
						iNumNativeTwoFoodSecondRing = iNumNativeTwoFoodSecondRing + 1;
					elseif terrainType == TerrainTypes.TERRAIN_PLAINS then
						outerThreeFood = outerThreeFood + 1;
						outerCanHaveBonus = outerCanHaveBonus + 1;
						iNumPlains = iNumPlains + 1;
						if featureType == FeatureTypes.FEATURE_FOREST then
							outerForest = outerForest + 1;
						else
							outerOneHammer = outerOneHammer + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_TUNDRA then
						outerThreeFood = outerThreeFood + 1;
						outerCanHaveBonus = outerCanHaveBonus + 1;
						if featureType == FeatureTypes.FEATURE_FOREST then
							outerForest = outerForest + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_DESERT then
						outerBadTiles = outerBadTiles + 1;
						outerCanHaveBonus = outerCanHaveBonus + 1; -- Can have Oasis.
					else -- Snow
						outerBadTiles = outerBadTiles + 1;
					end
				else -- Dry Flatlands
					if terrainType == TerrainTypes.TERRAIN_GRASS then
						outerThreeFood = outerThreeFood + 1;
						iNumGrass = iNumGrass + 1;
						if featureType ~= FeatureTypes.FEATURE_MARSH then
							outerCanHaveBonus = outerCanHaveBonus + 1;
						end
						if featureType == FeatureTypes.FEATURE_FOREST then
							outerForest = outerForest + 1;
						end
						if featureType == FeatureTypes.NO_FEATURE then
							iNumNativeTwoFoodSecondRing = iNumNativeTwoFoodSecondRing + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_PLAINS then
						outerTwoFood = outerTwoFood + 1;
						outerCanHaveBonus = outerCanHaveBonus + 1;
						iNumPlains = iNumPlains + 1;
						if featureType == FeatureTypes.FEATURE_FOREST then
							outerForest = outerForest + 1;
						else
							outerOneHammer = outerOneHammer + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_TUNDRA then
						outerCanHaveBonus = outerCanHaveBonus + 1;
						if featureType == FeatureTypes.FEATURE_FOREST then
							outerForest = outerForest + 1;
						else
							outerBadTiles = outerBadTiles + 1;
						end
					elseif terrainType == TerrainTypes.TERRAIN_DESERT then
						outerBadTiles = outerBadTiles + 1;
						outerCanHaveBonus = outerCanHaveBonus + 1; -- Can have Oasis.
					else -- Snow
						outerBadTiles = outerBadTiles + 1;
					end
				end
			end
		end
	end
	
	--[[
	-- Adjust the hammer situation, if needed.
	local innerHammerScore = (4 * innerHills) + (2 * innerForest) + innerOneHammer;
	local outerHammerScore = (2 * outerHills) + outerForest + outerOneHammer;
	local earlyHammerScore = (2 * innerForest) + outerForest + innerOneHammer + outerOneHammer;
	-- If drastic shortage, attempt to add a hill to first ring.
	if (outerHammerScore < 8 and innerHammerScore < 2) or innerHammerScore == 0 then -- Change a first ring plot to Hills.
		if isEvenY then
			randomized_first_ring_adjustments = GetShuffledCopyOfTable(self.firstRingYIsEven);
		else
			randomized_first_ring_adjustments = GetShuffledCopyOfTable(self.firstRingYIsOdd);
		end
		for attempt = 1, 6 do
			local plot_adjustments = randomized_first_ring_adjustments[attempt];
			local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
			-- Attempt to place a Hill at the currently chosen plot.
			local placedHill = self:AttemptToPlaceHillsAtPlot(searchX, searchY);
			if placedHill == true then
				innerHammerScore = innerHammerScore + 4;
				--print("Added hills next to hammer-poor start plot at ", x, y);
				break
			elseif attempt == 6 then
				--print("FAILED to add hills next to hammer-poor start plot at ", x, y);
			end
		end
	end
	]]
	
	-- Add mandatory Iron, Horse, Oil to every start if Strategic Balance option is enabled.
	if res == 5 or res == 6 then
		self:AddStrategicBalanceResources(region_number)
	end
	
	--[[
	-- If early hammers will be too short, attempt to add a small Horse or Iron to second ring.
	if innerHammerScore < 3 and earlyHammerScore < 6 then -- Add a small Horse or Iron to second ring.
		if isEvenY then
			randomized_second_ring_adjustments = GetShuffledCopyOfTable(self.secondRingYIsEven);
		else
			randomized_second_ring_adjustments = GetShuffledCopyOfTable(self.secondRingYIsOdd);
		end
		for attempt = 1, 12 do
			local plot_adjustments = randomized_second_ring_adjustments[attempt];
			local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
			-- Attempt to place a Hill at the currently chosen plot.
			local placedStrategic = self:AttemptToPlaceSmallStrategicAtPlot(searchX, searchY);
			if placedStrategic == true then
				break
			elseif attempt == 12 then
				--print("FAILED to add small strategic resource near hammer-poor start plot at ", x, y);
			end
		end
	end

	]]
	
	-- Rate the food situation.
	local innerFoodScore = (4 * innerFourFood) + (2 * innerThreeFood) + innerTwoFood;
	local outerFoodScore = (4 * outerFourFood) + (2 * outerThreeFood) + outerTwoFood;
	local totalFoodScore = innerFoodScore + outerFoodScore;
	local nativeTwoFoodTiles = iNumNativeTwoFoodFirstRing + iNumNativeTwoFoodSecondRing;

	--[[ Debug printout of food scores.
	print("-");
	print("-- - Start Point in Region #", region_number, " has Food Score of ", totalFoodScore, " with rings of ", innerFoodScore, outerFoodScore);
	]]--	
	
	-- Six levels for Bonus Resource support, from zero to five.
	if totalFoodScore < 4 and innerFoodScore == 0 then
		iNumFoodBonusNeeded = 5;
	elseif totalFoodScore < 6 then
		iNumFoodBonusNeeded = 4;
	elseif totalFoodScore < 8 then
		iNumFoodBonusNeeded = 3;
	elseif totalFoodScore < 12 and innerFoodScore < 5 then
		iNumFoodBonusNeeded = 3;
	elseif totalFoodScore < 17 and innerFoodScore < 9 then
		iNumFoodBonusNeeded = 2;
	elseif nativeTwoFoodTiles <= 1 then
		iNumFoodBonusNeeded = 2;
	elseif totalFoodScore < 24 and innerFoodScore < 11 then
		iNumFoodBonusNeeded = 1;
	elseif nativeTwoFoodTiles == 2 or iNumNativeTwoFoodFirstRing == 0 then
		iNumFoodBonusNeeded = 1;
	elseif totalFoodScore < 20 then
		iNumFoodBonusNeeded = 1;
	end
	
	-- Check for Legendary Start resource option.
	if res == 4 then
		iNumFoodBonusNeeded = iNumFoodBonusNeeded + 2;
	end
	
	-- Check to see if a Grass tile needs to be added at an all-plains site with zero native 2-food tiles in first two rings.
	if nativeTwoFoodTiles == 0 then
		local odd = self.firstRingYIsOdd;
		local even = self.firstRingYIsEven;
		local plot_list = {};
		-- For notes on how the hex-iteration works, refer to PlaceResourceImpact()
		local ripple_radius = 2;
		local currentX = x - ripple_radius;
		local currentY = y;
		for direction_index = 1, 6 do
			for plot_to_handle = 1, ripple_radius do
			 	if currentY / 2 > math.floor(currentY / 2) then
					plot_adjustments = odd[direction_index];
				else
					plot_adjustments = even[direction_index];
				end
				nextX = currentX + plot_adjustments[1];
				nextY = currentY + plot_adjustments[2];
				if wrapX == false and (nextX < 0 or nextX >= iW) then
					-- X is out of bounds.
				elseif wrapY == false and (nextY < 0 or nextY >= iH) then
					-- Y is out of bounds.
				else
					local realX = nextX;
					local realY = nextY;
					if wrapX then
						realX = realX % iW;
					end
					if wrapY then
						realY = realY % iH;
					end
					-- We've arrived at the correct x and y for the current plot.
					local plot = Map.GetPlot(realX, realY);
					if plot:GetResourceType(-1) == -1 then -- No resource here, safe to proceed.
						local plotType = plot:GetPlotType()
						local terrainType = plot:GetTerrainType()
						local featureType = plot:GetFeatureType()
						local plotIndex = realY * iW + realX + 1;
						-- Now check this plot for eligibility to be converted to flat open grassland.
						if plotType == PlotTypes.PLOT_LAND then
							if terrainType == TerrainTypes.TERRAIN_PLAINS then
								if featureType == FeatureTypes.NO_FEATURE then
									table.insert(plot_list, plotIndex);
								end
							end
						end
					end
				end
				currentX, currentY = nextX, nextY;
			end
		end
		local iNumConversionCandidates = table.maxn(plot_list);
		if iNumConversionCandidates == 0 then
			local randomized_first_ring_adjustments, randomized_second_ring_adjustments, randomized_third_ring_adjustments;
			if isEvenY then
				randomized_first_ring_adjustments = GetShuffledCopyOfTable(self.firstRingYIsEven);
			else
				randomized_first_ring_adjustments = GetShuffledCopyOfTable(self.firstRingYIsOdd);
			end
			for attempt = 1, 6 do
				local plot_adjustments = randomized_first_ring_adjustments[attempt];
				local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
				local placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, false, false, false, self);
				if placedBonus == true then
					break;
				end
			end
		else
			--print("-"); print("*** START HAD NO 2-FOOD TILES, YET ONLY QUALIFIED FOR 2 BONUS; CONVERTING A PLAINS TO GRASS! ***"); print("-");
			local diceroll = 1 + Map.Rand(iNumConversionCandidates, "Choosing plot to convert to Grass near food-poor Plains start - LUA");
			local conversionPlotIndex = plot_list[diceroll];
			local conv_x = (conversionPlotIndex - 1) % iW;
			local conv_y = (conversionPlotIndex - conv_x - 1) / iW;
			local plot = Map.GetPlot(conv_x, conv_y);
			plot:SetTerrainType(TerrainTypes.TERRAIN_GRASS, false, false)
			self:PlaceResourceImpact(conv_x, conv_y, 1, 0) -- Disallow strategic resources at this plot, to keep it a farm plot.
		end
	end

	--[[
	-- Add Bonus Resources to food-poor start positions.
	if iNumFoodBonusNeeded > 0 then
		local maxBonusesPossible = innerCanHaveBonus + outerCanHaveBonus;

		--print("-");
		--print("Food-Poor start ", x, y, " needs ", iNumFoodBonusNeeded, " Bonus, with ", maxBonusesPossible, " eligible plots.");
		--print("-");

		local innerPlaced, outerPlaced = 0, 0;
		local randomized_first_ring_adjustments, randomized_second_ring_adjustments, randomized_third_ring_adjustments;
		if isEvenY then
			randomized_first_ring_adjustments = GetShuffledCopyOfTable(self.firstRingYIsEven);
			randomized_second_ring_adjustments = GetShuffledCopyOfTable(self.secondRingYIsEven);
			randomized_third_ring_adjustments = GetShuffledCopyOfTable(self.thirdRingYIsEven);
		else
			randomized_first_ring_adjustments = GetShuffledCopyOfTable(self.firstRingYIsOdd);
			randomized_second_ring_adjustments = GetShuffledCopyOfTable(self.secondRingYIsOdd);
			randomized_third_ring_adjustments = GetShuffledCopyOfTable(self.thirdRingYIsOdd);
		end
		local tried_all_first_ring = false;
		local tried_all_second_ring = false;
		local tried_all_third_ring = false;
		local allow_oasis = true; -- Permanent flag. (We don't want to place more than one Oasis per location).
		local placedOasis; -- Records returning result from each attempt.
		while iNumFoodBonusNeeded > 0 do
			if ((innerPlaced < 2 and innerCanHaveBonus > 0) or (res == 4 and innerPlaced < 3 and innerCanHaveBonus > 0))
			  and tried_all_first_ring == false then
				-- Add bonus to inner ring.
				for attempt = 1, 6 do
					local plot_adjustments = randomized_first_ring_adjustments[attempt];
					local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
					-- Attempt to place a Bonus at the currently chosen plot.
					local placedBonus, placedOasis = self:AttemptToPlaceBonusResourceAtPlot(searchX, searchY, allow_oasis);
					if placedBonus == true then
						if allow_oasis == true and placedOasis == true then -- First oasis was placed on this pass, so change permission.
							allow_oasis = false;
						end
						--print("Placed a Bonus in first ring at ", searchX, searchY);
						innerPlaced = innerPlaced + 1;
						innerCanHaveBonus = innerCanHaveBonus - 1;
						iNumFoodBonusNeeded = iNumFoodBonusNeeded - 1;
						break
					elseif attempt == 6 then
						tried_all_first_ring = true;
					end
				end

			elseif ((innerPlaced + outerPlaced < 5 and outerCanHaveBonus > 0) or (res == 4 and innerPlaced + outerPlaced < 4 and outerCanHaveBonus > 0))
			  and tried_all_second_ring == false then
				-- Add bonus to second ring.
				for attempt = 1, 12 do
					local plot_adjustments = randomized_second_ring_adjustments[attempt];
					local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
					-- Attempt to place a Bonus at the currently chosen plot.
					local placedBonus, placedOasis = self:AttemptToPlaceBonusResourceAtPlot(searchX, searchY, allow_oasis);
					if placedBonus == true then
						if allow_oasis == true and placedOasis == true then -- First oasis was placed on this pass, so change permission.
							allow_oasis = false;
						end
						--print("Placed a Bonus in second ring at ", searchX, searchY);
						outerPlaced = outerPlaced + 1;
						outerCanHaveBonus = outerCanHaveBonus - 1;
						iNumFoodBonusNeeded = iNumFoodBonusNeeded - 1;
						break
					elseif attempt == 12 then
						tried_all_second_ring = true;
					end
				end

			elseif tried_all_third_ring == false then
				-- Add bonus to third ring.
				for attempt = 1, 18 do
					local plot_adjustments = randomized_third_ring_adjustments[attempt];
					local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
					-- Attempt to place a Bonus at the currently chosen plot.
					local placedBonus, placedOasis = self:AttemptToPlaceBonusResourceAtPlot(searchX, searchY, allow_oasis);
					if placedBonus == true then
						if allow_oasis == true and placedOasis == true then -- First oasis was placed on this pass, so change permission.
							allow_oasis = false;
						end
						--print("Placed a Bonus in third ring at ", searchX, searchY);
						iNumFoodBonusNeeded = iNumFoodBonusNeeded - 1;
						break
					elseif attempt == 18 then
						tried_all_third_ring = true;
					end
				end
				
			else -- Tried everywhere, have to give up.
				break				
			end
		end
	end
	]]



	local startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
		iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, self, true);
	local randomized_first_ring_adjustments, randomized_second_ring_adjustments;
	if isEvenY then
		randomized_first_ring_adjustments = GetShuffledCopyOfTable(self.firstRingYIsEven);
		randomized_second_ring_adjustments = GetShuffledCopyOfTable(self.secondRingYIsEven);
	else
		randomized_first_ring_adjustments = GetShuffledCopyOfTable(self.firstRingYIsOdd);
		randomized_second_ring_adjustments = GetShuffledCopyOfTable(self.secondRingYIsOdd);
	end

	-- Check for heavy grass and light plains. Adding Stone if grass count is high and plains count is low. - May 2011, BT
	local iNumStoneNeeded = 0;
	if iNumGrass >= 9 and iNumPlains == 0 and iNumBonuses < 6 then
		iNumStoneNeeded = 2;
	elseif iNumGrass >= 6 and iNumPlains <= 4 and iNumBonuses < 6 then
		iNumStoneNeeded = 1;
	end
	if iNumStoneNeeded > 0 then -- Add Stone to this grass start.
		local stonePlaced, innerPlaced = 0, 0;
		local tried_all_first_ring = false;
		local tried_all_second_ring = false;
		while iNumStoneNeeded > 0 do
			if innerPlaced < 1 and tried_all_first_ring == false then
				-- Add bonus to inner ring.
				for attempt = 1, 6 do
					local plot_adjustments = randomized_first_ring_adjustments[attempt];
					local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
					-- Attempt to place Cows at the currently chosen plot.
					local placedBonus = self:AttemptToPlaceStoneAtGrassPlot(searchX, searchY);
					if placedBonus == true then
						--print("Placed Stone in first ring at ", searchX, searchY);
						innerPlaced = innerPlaced + 1;
						iNumStoneNeeded = iNumStoneNeeded - 1;
						break
					elseif attempt == 6 then
						tried_all_first_ring = true;
					end
				end

			elseif tried_all_second_ring == false then
				-- Add bonus to second ring.
				for attempt = 1, 12 do
					local plot_adjustments = randomized_second_ring_adjustments[attempt];
					local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
					-- Attempt to place Stone at the currently chosen plot.
					local placedBonus = self:AttemptToPlaceStoneAtGrassPlot(searchX, searchY);
					if placedBonus == true then
						--print("Placed Stone in second ring at ", searchX, searchY);
						iNumStoneNeeded = iNumStoneNeeded - 1;
						break
					elseif attempt == 12 then
						tried_all_second_ring = true;
					end
				end

			else -- Tried everywhere, have to give up.
				break				
			end
		end
	end

	local bNeedFirstRingTwoHammers = true;
	for attempt = 1, 6 do
		local plot_adjustments = randomized_first_ring_adjustments[attempt];
		local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
		searchPlot = Map.GetPlot(searchX, searchY);
		local plotType, terrainType, featureType, resourceType, isFreshWater,
		startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore,
		startBadTile, earlyBadTile, midBadTile, lateBadTile, bIsBonus = PlotAnalyzer(searchPlot, alongOcean, self);
		if startHammerScore >= 2 then
			bNeedFirstRingTwoHammers = false;
		end
	end
	if bNeedFirstRingTwoHammers == true then
		local attempt = 1;
		while bNeedFirstRingTwoHammers == true and attempt <= 6 do
			local plot_adjustments = randomized_first_ring_adjustments[attempt];
			local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
			searchPlot = Map.GetPlot(searchX, searchY);
			local placedHill = self:AttemptToPlaceHillsAtPlot(searchX, searchY, true);
			if placedHill then
				bNeedFirstRingTwoHammers = false;
			end
			attempt = attempt + 1;
		end
	end
	if (bNeedFirstRingTwoHammers) then
		print("Where is my hill?! "..tostring(region_number));
	end

	local bNeedFirstRingBonus = true;
	for attempt = 1, 6 do
		local plot_adjustments = randomized_first_ring_adjustments[attempt];
		local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
		searchPlot = Map.GetPlot(searchX, searchY);

		local plotType, terrainType, featureType, resourceType, isFreshWater,
		startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore,
		startBadTile, earlyBadTile, midBadTile, lateBadTile, bIsBonus = PlotAnalyzer(searchPlot, alongOcean, self);
		if (startFoodScore + startHammerScore) > 2 then
			bNeedFirstRingBonus = false;
		end
	end
	if bNeedFirstRingBonus == true then
		local attempt = 1;
		while bNeedFirstRingBonus == true and attempt <= 18 do
			diceroll = 1 + Map.Rand(6, "");
			local plot_adjustments = randomized_first_ring_adjustments[diceroll];
			local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
			searchPlot = Map.GetPlot(searchX, searchY);
			local placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, false, false, true, self);
			--print("searchX = "..tostring(searchX)..", searchY = "..tostring(searchY));
			--print("placedBonus "..tostring(placedBonus));
			local plotType, terrainType, featureType, resourceType, isFreshWater,
			startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore,
			startBadTile, earlyBadTile, midBadTile, lateBadTile, bIsBonus = PlotAnalyzer(searchPlot, alongOcean, self);
			--print("startScore "..tostring(startFoodScore + startHammerScore));
			if (startFoodScore + startHammerScore) > 2 and startFoodScore > 1 then
				bNeedFirstRingBonus = false;
			end
			attempt = attempt + 1;
		end
	end
	--print("bNeedFirstRingBonus "..tostring(bNeedFirstRingBonus));

	startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
		iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, self, true);

	local randomized_search_table;
	if isEvenY then
		search_table = {
						{0, 1}, {1, 0}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1},
						{1, 2}, {1, 1}, {2, 0}, {1, -1}, {1, -2}, {0, -2},
						{-1, -2}, {-2, -1}, {-2, 0}, {-2, 1}, {-1, 2}, {0, 2},
		}
	else
		search_table = {
						{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {0, 1},
						{1, 2}, {2, 1}, {2, 0}, {2, -1}, {1, -2}, {0, -2},
						{-1, -2}, {-1, -1}, {-2, 0}, {-1, 1}, {-1, 2}, {0, 2},
		}
	end
	randomized_search_table = GetShuffledCopyOfTable(search_table);

	local attempt;
	local startScore = startFoodScore + startHammerScore;
	local earlyScore = earlyFoodScore + earlyHammerScore;
	local midScore = midFoodScore + midHammerScore;
	local placedBonus, placedOasis, resource_ID = false, false, -1;

	--[[
	attempt = 1;
	if iNumHabitableTiles > 6 then
		while (earlyScore/iNumHabitableTiles < 3/2 or 6*earlyHammerScore < earlyScore) and attempt < 36
		and (6*iNumHillTiles < iNumHabitableTiles and fastHammerScore >= 9 or 8*iNumHillTiles < iNumHabitableTiles and fastHammerScore < 9) do
			local plot_adjustments = randomized_search_table[attempt];
			local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(x, y, plot_adjustments);
			if AssignStartingPlots:AttemptToPlaceHillsAtPlot(searchX, searchY) then
				startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
					iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
				earlyScore = earlyFoodScore + earlyHammerScore;
			end
			attempt = attempt + 1;
		end
	end
	]]
	
	--if iNumHabitableTiles > 0 then
	for iRunCounter = 1, 7 do
		attempt = 1;
		placedBonus = false;
		while ((startScore + earlyScore)/(18 - iNumStartBadTiles) < 2*(2 + iRunCounter/18)) and
			attempt < 18 and not placedBonus and iNumBonuses < 7 do
			local plot_adjustments = randomized_search_table[attempt];
			local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(x, y, plot_adjustments)
			-- Attempt to place a Bonus at the currently chosen plot.
			placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, bAllowOasis, alongOcean, false, self);

			if placedBonus == true then
				startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
					iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
				earlyScore = earlyFoodScore + earlyHammerScore;
			end
			attempt = attempt + 1;
		end
	end
	
	-- Record conditions at this start location.
	local results_table = {alongOcean, nextToLake, isRiver, nearRiver, nearMountain, forestCount, jungleCount};
	self.startLocationConditions[region_number] = results_table;
end
------------------------------------------------------------------------------
function AssignStartingPlots:AddStrategicBalanceResources(region_number)
	-- This function adds the required Strategic Resources to start plots, for
	-- games that have selected to enable Strategic Resource Balance.
	local iW, iH = Map.GetGridSize();
	local start_point_data = self.startingPlots[region_number];
	local x = start_point_data[1];
	local y = start_point_data[2];
	local plot = Map.GetPlot(x, y);
	local plotIndex = y * iW + x + 1;
	local wrapX = Map:IsWrapX();
	local wrapY = Map:IsWrapY();
	local odd = self.firstRingYIsOdd;
	local even = self.firstRingYIsEven;
	local nextX, nextY, plot_adjustments;
	local iron_list, horse_list, oil_list, coal_list, alum_list = {}, {}, {}, {}, {};
	local iron_fallback, horse_fallback, oil_fallback, coal_fallback, alum_fallback = {}, {}, {}, {}, {};
	local radius = 3;
	local res = Map.GetCustomOption(5);
	
	--print("- Adding Strategic Balance Resources for start location in Region#", region_number);
	
	for ripple_radius = 1, radius do
		local ripple_value = radius - ripple_radius + 1;
		local currentX = x - ripple_radius;
		local currentY = y;
		for direction_index = 1, 6 do
			for plot_to_handle = 1, ripple_radius do
			 	if currentY / 2 > math.floor(currentY / 2) then
					plot_adjustments = odd[direction_index];
				else
					plot_adjustments = even[direction_index];
				end
				nextX = currentX + plot_adjustments[1];
				nextY = currentY + plot_adjustments[2];
				if wrapX == false and (nextX < 0 or nextX >= iW) then
					-- X is out of bounds.
				elseif wrapY == false and (nextY < 0 or nextY >= iH) then
					-- Y is out of bounds.
				else
					local realX = nextX;
					local realY = nextY;
					if wrapX then
						realX = realX % iW;
					end
					if wrapY then
						realY = realY % iH;
					end
					-- We've arrived at the correct x and y for the current plot.
					local plot = Map.GetPlot(realX, realY);
					local plotType = plot:GetPlotType()
					local terrainType = plot:GetTerrainType()
					local featureType = plot:GetFeatureType()
					local plotIndex = realY * iW + realX + 1;
					-- Check this plot for resource placement eligibility.
					if plotType == PlotTypes.PLOT_HILLS then
						if ripple_radius < 3 then
							table.insert(iron_list, plotIndex)

						else
							table.insert(iron_fallback, plotIndex)

						end
						if terrainType ~= TerrainTypes.TERRAIN_SNOW and featureType == FeatureTypes.NO_FEATURE then
							table.insert(horse_fallback, plotIndex)
						end
					elseif plotType == PlotTypes.PLOT_LAND then
						if featureType == FeatureTypes.NO_FEATURE then
							if terrainType == TerrainTypes.TERRAIN_TUNDRA or terrainType == TerrainTypes.TERRAIN_DESERT then
								if ripple_radius < 3 then
									table.insert(oil_list, plotIndex)
								else
									table.insert(oil_fallback, plotIndex)
								end
								table.insert(iron_fallback, plotIndex)
								table.insert(horse_fallback, plotIndex)
							elseif terrainType == TerrainTypes.TERRAIN_PLAINS or terrainType == TerrainTypes.TERRAIN_GRASS then
								if ripple_radius < 3 then
									table.insert(horse_list, plotIndex)
								else
									table.insert(horse_fallback, plotIndex)
								end
								table.insert(iron_fallback, plotIndex)
								table.insert(oil_fallback, plotIndex)
							elseif terrainType == TerrainTypes.TERRAIN_SNOW then
								if ripple_radius < 3 then
									table.insert(oil_list, plotIndex)
								else
									table.insert(oil_fallback, plotIndex)
								end
							end
						elseif featureType == FeatureTypes.FEATURE_MARSH then		
							if ripple_radius < 3 then
								table.insert(oil_list, plotIndex)
							else
								table.insert(oil_fallback, plotIndex)
							end
							table.insert(iron_fallback, plotIndex)
						elseif featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then		
							table.insert(horse_fallback, plotIndex)
							table.insert(oil_fallback, plotIndex)
						elseif featureType == FeatureTypes.FEATURE_JUNGLE or featureType == FeatureTypes.FEATURE_FOREST then		
							table.insert(iron_fallback, plotIndex)
							table.insert(oil_fallback, plotIndex)
						end
					end
					currentX, currentY = nextX, nextY;
				end
			end
		end
	end

	if res == 6 then
		local radius = 6;
		for ripple_radius = 1, radius do
			local ripple_value = radius - ripple_radius + 1;
			local currentX = x - ripple_radius;
			local currentY = y;
			for direction_index = 1, 6 do
				for plot_to_handle = 1, ripple_radius do
			 		if currentY / 2 > math.floor(currentY / 2) then
						plot_adjustments = odd[direction_index];
					else
						plot_adjustments = even[direction_index];
					end
					nextX = currentX + plot_adjustments[1];
					nextY = currentY + plot_adjustments[2];
					if wrapX == false and (nextX < 0 or nextX >= iW) then
						-- X is out of bounds.
					elseif wrapY == false and (nextY < 0 or nextY >= iH) then
						-- Y is out of bounds.
					else
						local realX = nextX;
						local realY = nextY;
						if wrapX then
							realX = realX % iW;
						end
						if wrapY then
							realY = realY % iH;
						end
						-- We've arrived at the correct x and y for the current plot.
						local plot = Map.GetPlot(realX, realY);
						local plotType = plot:GetPlotType()
						local terrainType = plot:GetTerrainType()
						local featureType = plot:GetFeatureType()
						local plotIndex = realY * iW + realX + 1;
						-- Check this plot for resource placement eligibility.
						if plotType == PlotTypes.PLOT_HILLS then
							if ripple_radius < 6 then
								table.insert(coal_list, plotIndex)
							else
								table.insert(coal_fallback, plotIndex)
							end
						elseif plotType == PlotTypes.PLOT_LAND then
							if featureType == FeatureTypes.NO_FEATURE then
								if terrainType == TerrainTypes.TERRAIN_TUNDRA or terrainType == TerrainTypes.TERRAIN_DESERT then
									if ripple_radius < 6 then
										table.insert(coal_list, plotIndex)
										table.insert(alum_list, plotIndex)
									else
										table.insert(coal_fallback, plotIndex)
										table.insert(alum_fallback, plotIndex)
									end
								elseif terrainType == TerrainTypes.TERRAIN_PLAINS or terrainType == TerrainTypes.TERRAIN_GRASS then
									if ripple_radius < 6 then
										table.insert(coal_list, plotIndex)
										table.insert(alum_list, plotIndex)
									else
										table.insert(coal_fallback, plotIndex)
										table.insert(alum_fallback, plotIndex)
									end
								elseif terrainType == TerrainTypes.TERRAIN_SNOW then
									if ripple_radius < 6 then
										table.insert(coal_list, plotIndex)
										table.insert(alum_list, plotIndex)
									else
										table.insert(coal_fallback, plotIndex)
										table.insert(alum_fallback, plotIndex)
									end
								end
							elseif featureType == FeatureTypes.FEATURE_MARSH then		
								if ripple_radius < 4 then
									table.insert(coal_list, plotIndex)
									table.insert(alum_list, plotIndex)
								else
									table.insert(coal_fallback, plotIndex)
									table.insert(alum_fallback, plotIndex)
								end
							elseif featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then
								table.insert(coal_fallback, plotIndex)
								table.insert(alum_fallback, plotIndex)
							elseif featureType == FeatureTypes.FEATURE_JUNGLE or featureType == FeatureTypes.FEATURE_FOREST then
								table.insert(coal_fallback, plotIndex)
								table.insert(alum_fallback, plotIndex)
							end
						end
						currentX, currentY = nextX, nextY;
					end
				end
			end
		end
	end

	local uran_amt, horse_amt, oil_amt, iron_amt, coal_amt, alum_amt = self:GetMajorStrategicResourceQuantityValues()
	coal_amt = 3;
	alum_amt = 3;
	local shuf_list;
	local placed_iron, placed_horse, placed_oil_1, placed_oil_2, placed_coal, placed_alum = false, false, false, false, false, false;

	if table.maxn(iron_list) > 0 then
		shuf_list = GetShuffledCopyOfTable(iron_list)
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.iron_ID, iron_amt, 1, 1, -1, 0, 0, shuf_list);
		if iNumLeftToPlace == 0 then
			placed_iron = true;
		end
	end
	if table.maxn(horse_list) > 0 then
		shuf_list = GetShuffledCopyOfTable(horse_list)
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.horse_ID, horse_amt, 1, 1, -1, 0, 0, shuf_list);
		if iNumLeftToPlace == 0 then
			placed_horse = true;
		end
	end
	if table.maxn(oil_list) > 0 then
		shuf_list = GetShuffledCopyOfTable(oil_list)
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.oil_ID, oil_amt, 1, 1, -1, 0, 0, shuf_list);
		if iNumLeftToPlace == 0 then
			placed_oil_1 = true;
		end
	end
	
	if res == 6 then
		if table.maxn(oil_list) > 0 then
			shuf_list = GetShuffledCopyOfTable(oil_list)
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.oil_ID, 3, 1, 1, -1, 0, 0, shuf_list);
			if iNumLeftToPlace == 0 then
				placed_oil_2 = true;
			end
		end
		if table.maxn(coal_list) > 0 then
			shuf_list = GetShuffledCopyOfTable(coal_list)
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.coal_ID, coal_amt, 1, 1, -1, 0, 0, shuf_list);
			if iNumLeftToPlace == 0 then
				placed_coal = true;
			end
		end
		if table.maxn(alum_list) > 0 then
			shuf_list = GetShuffledCopyOfTable(alum_list)
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.aluminum_ID, alum_amt, 1, 1, -1, 0, 0, shuf_list);
			if iNumLeftToPlace == 0 then
				placed_alum = true;
			end
		end
	end



	if placed_iron == false and table.maxn(iron_fallback) > 0 then
		shuf_list = GetShuffledCopyOfTable(iron_fallback)
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.iron_ID, iron_amt, 1, 1, -1, 0, 0, shuf_list);
	end
	if placed_horse == false and table.maxn(horse_fallback) > 0 then
		shuf_list = GetShuffledCopyOfTable(horse_fallback)
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.horse_ID, horse_amt, 1, 1, -1, 0, 0, shuf_list);
	end
	if placed_oil_1 == false and table.maxn(oil_fallback) > 0 then
		shuf_list = GetShuffledCopyOfTable(oil_fallback)
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.oil_ID, oil_amt, 1, 1, -1, 0, 0, shuf_list);
	end
	if res == 6 then
		if placed_oil_2 == false and table.maxn(oil_fallback) > 0 then
			shuf_list = GetShuffledCopyOfTable(oil_fallback)
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.oil_ID, 3, 1, 1, -1, 0, 0, shuf_list);
		end
		if placed_coal == false and table.maxn(coal_fallback) > 0 then
			shuf_list = GetShuffledCopyOfTable(coal_fallback)
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.coal_ID, coal_amt, 1, 1, -1, 0, 0, shuf_list);
		end
		if placed_alum == false and table.maxn(alum_fallback) > 0 then
			shuf_list = GetShuffledCopyOfTable(alum_fallback)
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.aluminum_ID, alum_amt, 1, 1, -1, 0, 0, shuf_list);
		end
	end
end
------------------------------------------------------------------------------
function NormalizePlotAreas(AssignStartingPlots)
	AssignStartingPlots:FixSugarJungles();

	local iW, iH = Map.GetGridSize();
	local map_plots_data = {}
	for y = 0, iH - 1 do	
		for x = 0, iW - 1 do
			table.insert(map_plots_data, {x, y});
		end
	end

	local randomized_map_plots_data;

	--Placintg Hills to Balance Areas
	for iRunCounter = 1, 9 do
		randomized_map_plots_data = GetShuffledCopyOfTable(map_plots_data);
		for loop, plot_data in ipairs(randomized_map_plots_data) do
			local x, y = plot_data[1], plot_data[2];
			local plot = Map.GetPlot(x, y);
			local plotIndex = y * iW + x + 1;
			local isEvenY = true;
			if y / 2 > math.floor(y / 2) then
				isEvenY = false;
			end
			if plot:GetPlotType() == PlotTypes.PLOT_LAND and plot:GetTerrainType() ~= TerrainTypes.TERRAIN_SNOW or plot:GetPlotType() == PlotTypes.PLOT_HILLS then
				local search_table = {};
				
				-- Set up Conditions checks.
				local alongOcean = false;
				local nextToLake = false;
				local isRiver = false;
				local nearRiver = false;
				local nearMountain = false;
				local res = Map.GetCustomOption(5);
			
				-- Check start plot to see if it's adjacent to saltwater.
				if AssignStartingPlots.plotDataIsCoastal[plotIndex] == true then
					alongOcean = true;
				end
				
				-- Check start plot to see if it's on a river.
				if plot:IsRiver() then
					isRiver = true;
				end

				local startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
					iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);

				local randomized_search_table;
				if isEvenY then
					search_table = {
									{0, 1}, {1, 0}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1},
									{1, 2}, {1, 1}, {2, 0}, {1, -1}, {1, -2}, {0, -2},
									{-1, -2}, {-2, -1}, {-2, 0}, {-2, 1}, {-1, 2}, {0, 2},
									{1, 3}, {2, 2}, {2, 1}, {3, 0}, {2, -1}, {2, -2},
									{1, -3}, {0, -3}, {-1, -3}, {-2, -3}, {-2, -2}, {-3, -1},
									{-3, 0}, {-3, 1}, {-2, 2}, {-2, 3}, {-1, 3}, {0, 3}
					}
				else
					search_table = {
									{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {0, 1},
									{1, 2}, {2, 1}, {2, 0}, {2, -1}, {1, -2}, {0, -2},
									{-1, -2}, {-1, -1}, {-2, 0}, {-1, 1}, {-1, 2}, {0, 2},
									{2, 3}, {2, 2}, {3, 1}, {3, 0}, {3, -1}, {2, -2},
									{2, -3}, {1, -3}, {0, -3}, {-1, -3}, {-2, -2}, {-2, -1},
									{-3, 0}, {-2, 1}, {-2, 2}, {-1, 3}, {0, 3}, {1, 3}
					}
				end
				randomized_search_table = GetShuffledCopyOfTable(search_table);

				local attempt;
				local earlyScore = earlyFoodScore + earlyHammerScore;
				local midScore = midFoodScore + midHammerScore;
				local placedHill = false;

				--print("x = "..tostring(x)..", y = "..tostring(y));
				--print("AvgearlyScore = "..tostring(earlyScore/(36 - iNumEarlyBadTiles))..", AvgMidScore = "..tostring(midScore/(36 - iNumMidBadTiles)))

				--Non-Desert Tiles
				attempt = 1;
				if iNumHabitableTiles > 6 then
					while (midHammerScore/3 < iNumHabitableTiles/4 and iNumHillTiles < iNumHabitableTiles/4)
						and attempt < 36 and not placedHill do
						local plot_adjustments = randomized_search_table[attempt];
						local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(x, y, plot_adjustments);
						searchPlot = Map.GetPlot(searchX, searchY);

						if searchPlot ~= nil then
							startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
								iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(searchPlot, AssignStartingPlots, false);
							earlyScore = earlyFoodScore + earlyHammerScore;
							if (midHammerScore/3 < iNumHabitableTiles/4 and iNumHillTiles < iNumHabitableTiles/4)
								and attempt < 36 and not placedHill then
								placedHill = AssignStartingPlots:AttemptToPlaceHillsAtPlot(searchX, searchY, false);
							end
							startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
								iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
							earlyScore = earlyFoodScore + earlyHammerScore;
							if placedHill then
								--print ("iNumHillTiles, ", iNumHillTiles);
							end
						end
						attempt = attempt + 1;
					end
				end
				--Desert Tiles
				--[[attempt = 1;
				if iNumHabitableTiles > 6 then
					while (earlyScore/iNumHabitableTiles < 3/2 or
					6*earlyHammerScore < earlyScore) and (6*iNumHillTiles < iNumHabitableTiles and fastHammerScore >= 9
					and fastHammerScore < 9) and attempt < 36 and not placedHill do
						local plot_adjustments = randomized_search_table[attempt];
						local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(x, y, plot_adjustments);
						searchPlot = Map.GetPlot(searchX, searchY);
						local pplotType, pterrainType, pfeatureType, presourceType, pisFreshWater,
							pstartFoodScore, pearlyFoodScore, pmidFoodScore, plateFoodScore, pstartHammerScore, pearlyHammerScore, pmidHammerScore, plateHammerScore,
							pstartBadTile, pearlyBadTile, pmidBadTile, plateBadTile, pbIsBonus = PlotAnalyzer(searchPlot, false, AssignStartingPlots);
						if searchPlot ~= nil and (pmidBadTile and searchPlot:GetTerrainType() == TerrainTypes.TERRAIN_DESERT) then
							placedHill = AssignStartingPlots:AttemptToPlaceHillsAtPlot(searchX, searchY);
						end
						if placedHill then
							startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
								iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
							earlyScore = earlyFoodScore + earlyHammerScore;
						end
						attempt = attempt + 1;
					end
				end]]
			end
		end
	end

	--Placintg Bonuses to Balance Areas
	for iRunCounter = 1, 12 do
		randomized_map_plots_data = GetShuffledCopyOfTable(map_plots_data);
		for loop, plot_data in ipairs(randomized_map_plots_data) do
			local x, y = plot_data[1], plot_data[2];
			local plot = Map.GetPlot(x, y);
			local plotIndex = y * iW + x + 1;
			local isEvenY = true;
			if y / 2 > math.floor(y / 2) then
				isEvenY = false;
			end
			if plot:GetPlotType() == PlotTypes.PLOT_LAND and plot:GetTerrainType() ~= TerrainTypes.TERRAIN_SNOW or plot:GetPlotType() == PlotTypes.PLOT_HILLS then
				local search_table = {};
				
				-- Set up Conditions checks.
				local alongOcean = false;
				local nextToLake = false;
				local isRiver = false;
				local nearRiver = false;
				local nearMountain = false;
				local res = Map.GetCustomOption(5);
			
				-- Check start plot to see if it's adjacent to saltwater.
				if AssignStartingPlots.plotDataIsCoastal[plotIndex] == true then
					alongOcean = true;
				end
				
				-- Check start plot to see if it's on a river.
				if plot:IsRiver() then
					isRiver = true;
				end

				local startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
					iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);

				local randomized_search_table;
				if isEvenY then
					search_table = {
									{0, 1}, {1, 0}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1},
									{1, 2}, {1, 1}, {2, 0}, {1, -1}, {1, -2}, {0, -2},
									{-1, -2}, {-2, -1}, {-2, 0}, {-2, 1}, {-1, 2}, {0, 2},
									{1, 3}, {2, 2}, {2, 1}, {3, 0}, {2, -1}, {2, -2},
									{1, -3}, {0, -3}, {-1, -3}, {-2, -3}, {-2, -2}, {-3, -1},
									{-3, 0}, {-3, 1}, {-2, 2}, {-2, 3}, {-1, 3}, {0, 3}
					}
				else
					search_table = {
									{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {0, 1},
									{1, 2}, {2, 1}, {2, 0}, {2, -1}, {1, -2}, {0, -2},
									{-1, -2}, {-1, -1}, {-2, 0}, {-1, 1}, {-1, 2}, {0, 2},
									{2, 3}, {2, 2}, {3, 1}, {3, 0}, {3, -1}, {2, -2},
									{2, -3}, {1, -3}, {0, -3}, {-1, -3}, {-2, -2}, {-2, -1},
									{-3, 0}, {-2, 1}, {-2, 2}, {-1, 3}, {0, 3}, {1, 3}
					}
				end
				randomized_search_table = GetShuffledCopyOfTable(search_table);

				local attempt;
				local earlyScore = earlyFoodScore + earlyHammerScore;
				local midScore = midFoodScore + midHammerScore;
				local placedBonus, placedOasis, resource_ID = false, false, -1;
				local iMaxBonuses = Map.GetCustomOption(7);

				--print("x = "..tostring(x)..", y = "..tostring(y));
				--print("AvgearlyScore = "..tostring(earlyScore/(36 - iNumEarlyBadTiles))..", AvgMidScore = "..tostring(midScore/(36 - iNumMidBadTiles)))
	
				attempt = 1;
				if iNumHabitableTiles > 12 then
					while (earlyScore/(36 - iNumEarlyBadTiles) < (2 + iRunCounter/36)--[[ or midScore/(36 - iNumMidBadTiles) < (3 + 6/18 + iRunCounter/18)]]) and
						attempt < 36 and not placedBonus and iNumBonuses < iMaxBonuses do
						local plot_adjustments = randomized_search_table[attempt];
						local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(x, y, plot_adjustments)
						-- Attempt to place a Bonus at the currently chosen plot.
						searchPlot = Map.GetPlot(searchX, searchY);
						--[[local pplotType, pterrainType, pfeatureType, presourceType, pisFreshWater,
							pstartFoodScore, pearlyFoodScore, pmidFoodScore, plateFoodScore, pstartHammerScore, pearlyHammerScore, pmidHammerScore, plateHammerScore,
							pstartBadTile, pearlyBadTile, pmidBadTile, plateBadTile, pbIsBonus = PlotAnalyzer(searchPlot, false, AssignStartingPlots);]]

						if searchPlot ~= nil--[[ and not (pmidBadTile and searchPlot:GetTerrainType() == TerrainTypes.TERRAIN_DESERT)]] then
							startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
								iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(searchPlot, AssignStartingPlots, false);
							earlyScore = earlyFoodScore + earlyHammerScore;
							if earlyScore/(36 - iNumEarlyBadTiles) < (2 + iRunCounter/36) then
								placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, bAllowOasis and (iRunCounter == 1), alongOcean, false, AssignStartingPlots);
							end
							startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
								iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
							earlyScore = earlyFoodScore + earlyHammerScore;
						end
						attempt = attempt + 1;
					end
				end

				--Desert Tiles
				--[[attempt = 1;
				if iNumHabitableTiles > 12 then
					while (earlyScore/(36 - iNumEarlyBadTiles) < (2.5 + iRunCounter/36)) and
						attempt < 36 and not placedBonus and iNumBonuses < 8 do
						local plot_adjustments = randomized_search_table[attempt];
						local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(x, y, plot_adjustments)
						-- Attempt to place a Bonus at the currently chosen plot.
						searchPlot = Map.GetPlot(searchX, searchY);
						local pplotType, pterrainType, pfeatureType, presourceType, pisFreshWater,
							pstartFoodScore, pearlyFoodScore, pmidFoodScore, plateFoodScore, pstartHammerScore, pearlyHammerScore, pmidHammerScore, plateHammerScore,
							pstartBadTile, pearlyBadTile, pmidBadTile, plateBadTile, pbIsBonus = PlotAnalyzer(searchPlot, false, AssignStartingPlots);
				
						if searchPlot ~= nil and pmidBadTile and searchPlot:GetTerrainType() == TerrainTypes.TERRAIN_DESERT then
							startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
								iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(searchPlot, AssignStartingPlots, false);
							earlyScore = earlyFoodScore + earlyHammerScore;
							if earlyScore/(36 - iNumEarlyBadTiles) < (2 + iRunCounter/36) then
								placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, bAllowOasis and (iRunCounter == 1), alongOcean, false, AssignStartingPlots);
							end
							startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
								iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
							earlyScore = earlyFoodScore + earlyHammerScore;
						end
						attempt = attempt + 1;
					end
				end]]
			end
		end
	end

end
------------------------------------------------------------------------------
function AssignStartingPlots:AttemptToPlaceBonusResourceAtPlot(x, y, bAllowOasis)
	-- Returns two booleans. First is true if something was placed. Second true if Oasis placed.
	--print("-"); print("Attempting to place a Bonus at: ", x, y);
	local plot = Map.GetPlot(x, y);
	if plot == nil then
		--print("Placement failed, plot was nil.");
		return false
	end
	if plot:GetResourceType(-1) ~= -1 then
		--print("Plot already had a resource.");
		return false
	end
	local terrainType = plot:GetTerrainType()
	if terrainType == TerrainTypes.TERRAIN_SNOW then
		--print("Plot was arctic land buried beneath endless snow.");
		return false
	end
	local featureType = plot:GetFeatureType()
	if featureType == FeatureTypes.FEATURE_OASIS then
		--print("Plot already had an Oasis.");
		return false
	end
	local plotType = plot:GetPlotType()
	--
	if featureType == FeatureTypes.FEATURE_JUNGLE then -- Place Banana
		plot:SetResourceType(self.banana_ID, 1);
		--print("Placed Banana.");
		self.amounts_of_resources_placed[self.banana_ID + 1] = self.amounts_of_resources_placed[self.banana_ID + 1] + 1;
		return true, false
	elseif featureType == FeatureTypes.FEATURE_FOREST then -- Place Deer
		plot:SetResourceType(self.deer_ID, 1);
		--print("Placed Deer.");
		self.amounts_of_resources_placed[self.deer_ID + 1] = self.amounts_of_resources_placed[self.deer_ID + 1] + 1;
		return true, false
	elseif plotType == PlotTypes.PLOT_HILLS and featureType == FeatureTypes.NO_FEATURE then
		plot:SetResourceType(self.sheep_ID, 1);
		--print("Placed Sheep.");
		self.amounts_of_resources_placed[self.sheep_ID + 1] = self.amounts_of_resources_placed[self.sheep_ID + 1] + 1;
		return true, false
	elseif plotType == PlotTypes.PLOT_LAND then
		if featureType == FeatureTypes.NO_FEATURE then
			if terrainType == TerrainTypes.TERRAIN_GRASS then -- Place Cows
				local diceroll = Map.Rand(2, "");
				if diceroll == 1 then
					plot:SetResourceType(self.bison_ID, 1);
					--print("Placed Bison.");
					self.amounts_of_resources_placed[self.bison_ID + 1] = self.amounts_of_resources_placed[self.bison_ID + 1] + 1;
					return true, false
				else
					plot:SetResourceType(self.cow_ID, 1);
					--print("Placed Cow.");
					self.amounts_of_resources_placed[self.cow_ID + 1] = self.amounts_of_resources_placed[self.cow_ID + 1] + 1;
					return true, false
				end
			elseif terrainType == TerrainTypes.TERRAIN_PLAINS then -- Place Wheat
				local diceroll = Map.Rand(2, "");
				if diceroll == 1 then
					plot:SetResourceType(self.bison_ID, 1);
					--print("Placed Bison.");
					self.amounts_of_resources_placed[self.bison_ID + 1] = self.amounts_of_resources_placed[self.bison_ID + 1] + 1;
					return true, false
				else
					plot:SetResourceType(self.wheat_ID, 1);
					--print("Placed Wheat.");
					self.amounts_of_resources_placed[self.wheat_ID + 1] = self.amounts_of_resources_placed[self.wheat_ID + 1] + 1;
					return true, false
				end
			elseif terrainType == TerrainTypes.TERRAIN_TUNDRA then -- Place Deer
				plot:SetResourceType(self.deer_ID, 1);
				--print("Placed Deer.");
				self.amounts_of_resources_placed[self.deer_ID + 1] = self.amounts_of_resources_placed[self.deer_ID + 1] + 1;
				return true, false
			elseif terrainType == TerrainTypes.TERRAIN_DESERT then
				if plot:IsFreshWater() then -- Place Wheat
					plot:SetResourceType(self.wheat_ID, 1);
					--print("Placed Wheat.");
					self.amounts_of_resources_placed[self.wheat_ID + 1] = self.amounts_of_resources_placed[self.wheat_ID + 1] + 1;
					return true, false
				elseif bAllowOasis then -- Place Oasis
					plot:SetFeatureType(FeatureTypes.FEATURE_OASIS, -1);
					--print("Placed Oasis.");
					return true, true
				--else
					--print("Not allowed to place any more Oasis help at this site.");
				end
			end
		end
	elseif plotType == PlotTypes.PLOT_OCEAN then
		if terrainType == TerrainTypes.TERRAIN_COAST and featureType == FeatureTypes.NO_FEATURE then
			if plot:IsLake() == false then -- Place Fish
				plot:SetResourceType(self.fish_ID, 1);
				--print("Placed Fish.");
				self.amounts_of_resources_placed[self.fish_ID + 1] = self.amounts_of_resources_placed[self.fish_ID + 1] + 1;
				return true, false
			end
		end
	end	
	-- Nothing placed.
	return false, false
end
------------------------------------------------------------------------------
function AreaMountainAnalyzer(PangaeaFractalWorld, x, y)

	local iNumMountains = 0;

	local search_table;
	local isEvenY = true;
	if x / 2 > math.floor(y / 2) then
		isEvenY = false;
	end
	if isStartLocation then
		if isEvenY then
			search_table = {
							{0, 1}, {1, 0}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1},
							{1, 2}, {1, 1}, {2, 0}, {1, -1}, {1, -2}, {0, -2},
							{-1, -2}, {-2, -1}, {-2, 0}, {-2, 1}, {-1, 2}, {0, 2},
			}
		else
			search_table = {
							{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {0, 1},
							{1, 2}, {2, 1}, {2, 0}, {2, -1}, {1, -2}, {0, -2},
							{-1, -2}, {-1, -1}, {-2, 0}, {-1, 1}, {-1, 2}, {0, 2},
			}
		end
	else
		if isEvenY then
			search_table = {
							{0, 1}, {1, 0}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1},
							{1, 2}, {1, 1}, {2, 0}, {1, -1}, {1, -2}, {0, -2},
							{-1, -2}, {-2, -1}, {-2, 0}, {-2, 1}, {-1, 2}, {0, 2},
							{1, 3}, {2, 2}, {2, 1}, {3, 0}, {2, -1}, {2, -2},
							{1, -3}, {0, -3}, {-1, -3}, {-2, -3}, {-2, -2}, {-3, -1},
							{-3, 0}, {-3, 1}, {-2, 2}, {-2, 3}, {-1, 3}, {0, 3}
			}
		else
			search_table = {
							{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {0, 1},
							{1, 2}, {2, 1}, {2, 0}, {2, -1}, {1, -2}, {0, -2},
							{-1, -2}, {-1, -1}, {-2, 0}, {-1, 1}, {-1, 2}, {0, 2},
							{2, 3}, {2, 2}, {3, 1}, {3, 0}, {3, -1}, {2, -2},
							{2, -3}, {1, -3}, {0, -3}, {-1, -3}, {-2, -2}, {-2, -1},
							{-3, 0}, {-2, 1}, {-2, 2}, {-1, 3}, {0, 3}, {1, 3}
			}
		end
	end

	for loop, plot_adjustments in ipairs(search_table) do
		local iW, iH = Map.GetGridSize();
		local adjusted_x, adjusted_y;
		if Map:IsWrapX() == true then
			adjusted_x = (x + plot_adjustments[1]) % iW;
		else
			adjusted_x = x + plot_adjustments[1];
		end
		if Map:IsWrapY() == true then
			adjusted_y = (y + plot_adjustments[2]) % iH;
		else
			adjusted_y = y + plot_adjustments[2];
		end

		local i = adjusted_y * PangaeaFractalWorld.iNumPlotsX + adjusted_x;
		if PangaeaFractalWorld.plotTypes[i] == PlotTypes.PLOT_MOUNTAIN then
			iNumMountains = iNumMountains + 1;
		end
	end

	return iNumMountains;
end
------------------------------------------------------------------------------
function AreaAnalyzer(plot, AssignStartingPlots, isStartLocation)
	local isRiver, isNearMountain,
	iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles,
	StartFoodScore, EarlyFoodScore, MidFoodScore, LateFoodScore,
	StartHammerScore, EarlyHammerScore, MidHammerScore, LateHammerScore, fastHammerScore = 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;

	local iNumBadDesert = 0;
	local iNumMountains = 0;
	local bAllowOasis = false;

	local search_table;
	local isEvenY = true;
	if plot:GetY() / 2 > math.floor(plot:GetY() / 2) then
		isEvenY = false;
	end
	if isStartLocation then
		if isEvenY then
			search_table = {
							{0, 1}, {1, 0}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1},
							{1, 2}, {1, 1}, {2, 0}, {1, -1}, {1, -2}, {0, -2},
							{-1, -2}, {-2, -1}, {-2, 0}, {-2, 1}, {-1, 2}, {0, 2},
			}
		else
			search_table = {
							{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {0, 1},
							{1, 2}, {2, 1}, {2, 0}, {2, -1}, {1, -2}, {0, -2},
							{-1, -2}, {-1, -1}, {-2, 0}, {-1, 1}, {-1, 2}, {0, 2},
			}
		end
	else
		if isEvenY then
			search_table = {
							{0, 1}, {1, 0}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1},
							{1, 2}, {1, 1}, {2, 0}, {1, -1}, {1, -2}, {0, -2},
							{-1, -2}, {-2, -1}, {-2, 0}, {-2, 1}, {-1, 2}, {0, 2},
							{1, 3}, {2, 2}, {2, 1}, {3, 0}, {2, -1}, {2, -2},
							{1, -3}, {0, -3}, {-1, -3}, {-2, -3}, {-2, -2}, {-3, -1},
							{-3, 0}, {-3, 1}, {-2, 2}, {-2, 3}, {-1, 3}, {0, 3}
			}
		else
			search_table = {
							{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {0, 1},
							{1, 2}, {2, 1}, {2, 0}, {2, -1}, {1, -2}, {0, -2},
							{-1, -2}, {-1, -1}, {-2, 0}, {-1, 1}, {-1, 2}, {0, 2},
							{2, 3}, {2, 2}, {3, 1}, {3, 0}, {3, -1}, {2, -2},
							{2, -3}, {1, -3}, {0, -3}, {-1, -3}, {-2, -2}, {-2, -1},
							{-3, 0}, {-2, 1}, {-2, 2}, {-1, 3}, {0, 3}, {1, 3}
			}
		end
	end

	local PlotType, TerrainType, FeatureType, ResourceType, IsFreshWater, IsCoastal;
	local HaveOasis = false;
	local iNumLandTiles = 0;
	local iNumHillTiles = 0;
	local iNumHabitableTiles = 0;
	local iNumBonuses = 0;

	isRiver = plot:IsRiver();
	isCoastal = plot:IsCoastalLand();
	for loop, plot_adjustments in ipairs(search_table) do
		local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(plot:GetX(), plot:GetY(), plot_adjustments)
		searchPlot = Map.GetPlot(searchX, searchY);

		local plotType, terrainType, featureType, resourceType, isFreshWater,
		startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore,
		startBadTile, earlyBadTile, midBadTile, lateBadTile, bIsBonus = PlotAnalyzer(searchPlot, IsCoastal, AssignStartingPlots);

		PlotType = plotType;
		TerrainType = terrainType;
		FeatureType = featureType;
		ResourceType = resourceType;
		IsFreshWater = isFreshWater;

		if plotType == PlotTypes.PLOT_MOUNTAIN then
			iNumMountains = iNumMountains + 1;
		end
		if plotType == PlotTypes.PLOT_LAND and terrainType ~= TerrainTypes.TERRAIN_SNOW then
			iNumLandTiles = iNumLandTiles + 1;
			iNumHabitableTiles = iNumHabitableTiles + 1;
		end
		if bIsBonus then
			iNumBonuses = iNumBonuses + 1;
		end
		if plotType == PlotTypes.PLOT_HILLS and terrainType ~= TerrainTypes.TERRAIN_SNOW then
			iNumHillTiles = iNumHillTiles + 1;
			iNumHabitableTiles = iNumHabitableTiles + 1;
		end
		if featureType == FeatureTypes.FEATURE_OASIS then
			--print("HaveOasis")
			HaveOasis = true;
		end
		if terrainType == TerrainTypes.TERRAIN_DESERT and not isFreshWater and resourceType == -1 and not searchPlot:IsCoastalLand() then
			--print("bAllowOasis")
			bAllowOasis = true;
		end
		if (not startBadTile) then
			StartFoodScore = StartFoodScore + startFoodScore;
			StartHammerScore = StartHammerScore + startHammerScore;
		else
			iNumStartBadTiles = iNumStartBadTiles + 1;
		end

		if (not earlyBadTile) then
			EarlyFoodScore = EarlyFoodScore + earlyFoodScore;
			EarlyHammerScore = EarlyHammerScore + earlyHammerScore;
		else
			iNumEarlyBadTiles = iNumEarlyBadTiles + 1;
		end

		if (not midBadTile) then
			MidFoodScore = MidFoodScore + midFoodScore;
			MidHammerScore = MidHammerScore + midHammerScore;
		else
			iNumMidBadTiles = iNumMidBadTiles + 1;
			if TerrainType == TerrainTypes.TERRAIN_DESERT then
				iNumBadDesert = iNumBadDesert + 1;
			end
		end

		if (not lateBadTile) then
			LateFoodScore = LateFoodScore + lateFoodScore;
			LateHammerScore = LateHammerScore + lateHammerScore;
		else
			iNumLateBadTiles = iNumLateBadTiles + 1;
		end
		if featureType == FeatureTypes.FEATURE_JUNGLE or featureType == FeatureTypes.FEATURE_MARSH or
			featureType == FeatureTypes.FEATURE_FOREST and terrainType == TerrainTypes.TERRAIN_TUNDRA and plotType == PlotTypes.PLOT_HILLS then
			fastHammerScore = fastHammerScore + 1;
		elseif featureType == FeatureTypes.FEATURE_FOREST and terrainType == TerrainTypes.TERRAIN_TUNDRA and plotType == PlotTypes.PLOT_LAND then
			if resourceType == AssignStartingPlots.dye_ID or resourceType == AssignStartingPlots.silver_ID or resourceType == AssignStartingPlots.iron_ID then
				fastHammerScore = fastHammerScore + 1;
			end
		end
	end

	return StartFoodScore, EarlyFoodScore, MidFoodScore, LateFoodScore, StartHammerScore, EarlyHammerScore, MidHammerScore, LateHammerScore, fastHammerScore,
	iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, (not HaveOasis and bAllowOasis),
	iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses;
end
------------------------------------------------------------------------------
function PlotAnalyzer(plot, isCoastal, AssignStartingPlots)
	local plotType, terrainType, featureType, resourceType, isFreshWater = -1, -1, -1, -1, 0;
	local startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore,
	startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore = 0, 0, 0, 0, 0, 0, 0, 0;
	local startBadTile, earlyBadTile, midBadTile, lateBadTile = false, false, false, false;
	local bIsBonus = false;
	if plot then
		plotType = plot:GetPlotType();
		terrainType = plot:GetTerrainType();
		featureType = plot:GetFeatureType();
		resourceType = plot:GetResourceType(-1);
		isFreshWater = plot:IsFreshWater();
	
		local iW, iH = Map.GetGridSize();
		--local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(plot:GetX(), plot:GetY(), plot_adjustments)
		--
		if plot:GetX() < 0 or plot:GetX() >= iW or plot:GetY() < 0 or plot:GetY() >= iH then
			-- This plot does not exist. It's off the map edge.
			-- iNumBadTiles = iNumBadTiles + 1;
		else
			if plotType == PlotTypes.PLOT_MOUNTAIN or featureType == FeatureTypes.FEATURE_ICE then
				--
			elseif plotType == PlotTypes.PLOT_OCEAN and isCoastal then
			--elseif plotType == PlotTypes.PLOT_OCEAN then
			--[[if terrainType == TerrainTypes.TERRAIN_OCEAN then
				iNumOcean = iNumOcean + 1;
			elseif terrainType == TerrainTypes.TERRAIN_COAST then
				iNumCoast = iNumCoast + 1;
			end]]
				if plot:IsLake() then
					startFoodScore = startFoodScore + 3;
					earlyFoodScore = earlyFoodScore + 3;
					midFoodScore = midFoodScore + 3;
					lateFoodScore = lateFoodScore + 3;
					lateHammerScore = lateHammerScore + 1;
				elseif resourceType == AssignStartingPlots.fish_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 5;
					lateFoodScore = lateFoodScore + 5;
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 2;
				elseif resourceType == AssignStartingPlots.crab_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 3;
					midFoodScore = midFoodScore + 4;
					lateFoodScore = lateFoodScore + 4;
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 2;
				elseif resourceType == AssignStartingPlots.whale_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 3;
					lateFoodScore = lateFoodScore + 3;
					earlyHammerScore = earlyHammerScore + 1;
					midHammerScore = midHammerScore + 2;
					lateHammerScore = lateHammerScore + 3;
				elseif resourceType == AssignStartingPlots.pearls_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 3;
					lateFoodScore = lateFoodScore + 3;
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 2;
				elseif featureType == FeatureTypes.FEATURE_ATOLL then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 3;
					lateFoodScore = lateFoodScore + 3;
					startHammerScore = startHammerScore + 1;
					earlyHammerScore = earlyHammerScore + 1;
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 1;
				end
			elseif plotType == PlotTypes.PLOT_OCEAN and not isCoastal then
				if plot:IsLake() then
					startFoodScore = startFoodScore + 3;
					earlyFoodScore = earlyFoodScore + 3;
					midFoodScore = midFoodScore + 3;
					lateFoodScore = lateFoodScore + 3;
					lateHammerScore = lateHammerScore + 1;
				elseif resourceType == AssignStartingPlots.fish_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 2;
					lateFoodScore = lateFoodScore + 2;
				elseif resourceType == AssignStartingPlots.crab_ID or resourceType == AssignStartingPlots.whale_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 2;
					lateFoodScore = lateFoodScore + 2;
				elseif resourceType == AssignStartingPlots.pearls_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 2;
					lateFoodScore = lateFoodScore + 2;
				elseif featureType == FeatureTypes.FEATURE_ATOLL then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 2;
					lateFoodScore = lateFoodScore + 2;
					startHammerScore = startHammerScore + 1;
					earlyHammerScore = earlyHammerScore + 1;
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 1;
				end
			elseif plotType == PlotTypes.PLOT_HILLS and terrainType ~= TerrainTypes.TERRAIN_SNOW then
				startHammerScore = startHammerScore + 2;
				earlyHammerScore = earlyHammerScore + 2;
				midHammerScore = midHammerScore + 2;
				lateHammerScore = lateHammerScore + 2;
				if featureType == FeatureTypes.FEATURE_FOREST then
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 1;
					startHammerScore = startHammerScore - 1;
					earlyHammerScore = earlyHammerScore - 1;
				elseif featureType == FeatureTypes.FEATURE_JUNGLE then
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					startHammerScore = startHammerScore - 2;
					earlyHammerScore = earlyHammerScore - 2;
				end
			elseif plotType == PlotTypes.PLOT_LAND and terrainType ~= TerrainTypes.TERRAIN_SNOW then
				if terrainType == TerrainTypes.TERRAIN_GRASS then
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 2;
					lateFoodScore = lateFoodScore + 2;
					if featureType == FeatureTypes.FEATURE_FOREST then
						startFoodScore = startFoodScore - 1;
						earlyFoodScore = earlyFoodScore - 1;
						startHammerScore = startHammerScore + 1;
						earlyHammerScore = earlyHammerScore + 1;
					elseif featureType == FeatureTypes.FEATURE_MARSH then
						startFoodScore = startFoodScore - 1;
						earlyFoodScore = earlyFoodScore - 1;
					end
				elseif terrainType == TerrainTypes.TERRAIN_PLAINS then
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 1;
					midFoodScore = midFoodScore + 1;
					lateFoodScore = lateFoodScore + 1;
					startHammerScore = startHammerScore + 1;
					earlyHammerScore = earlyHammerScore + 1;
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 1;
					if featureType == FeatureTypes.FEATURE_JUNGLE then
						startFoodScore = startFoodScore + 1;
						earlyFoodScore = earlyFoodScore + 1;
						startHammerScore = startHammerScore - 1;
						earlyHammerScore = earlyHammerScore - 1;
					end
				elseif terrainType == TerrainTypes.TERRAIN_TUNDRA then
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 1;
					midFoodScore = midFoodScore + 1;
					lateFoodScore = lateFoodScore + 1;
					if featureType == FeatureTypes.FEATURE_FOREST then
						startHammerScore = startHammerScore + 1;
						earlyHammerScore = earlyHammerScore + 1;
						midHammerScore = midHammerScore + 1;
						lateHammerScore = lateHammerScore + 1;
					end
				elseif terrainType == TerrainTypes.TERRAIN_DESERT then
					if featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then
						startFoodScore = startFoodScore + 2;
						earlyFoodScore = earlyFoodScore + 2;
						midFoodScore = midFoodScore + 2;
						lateFoodScore = lateFoodScore + 2;
					elseif featureType == FeatureTypes.FEATURE_OASIS then
						startFoodScore = startFoodScore + 3;
						earlyFoodScore = earlyFoodScore + 3;
						midFoodScore = midFoodScore + 3;
						lateFoodScore = lateFoodScore + 3;
						lateHammerScore = lateHammerScore + 1;
					elseif resourceType == AssignStartingPlots.coal_ID or
						resourceType == AssignStartingPlots.aluminum_ID or
						resourceType == AssignStartingPlots.oil_ID or
						resourceType == AssignStartingPlots.uranium_ID or
						resourceType == -1 then
					end
				end
			elseif terrainType == TerrainTypes.TERRAIN_SNOW then
				if resourceType == AssignStartingPlots.coal_ID or
					resourceType == AssignStartingPlots.aluminum_ID or
					resourceType == AssignStartingPlots.oil_ID or
					resourceType == AssignStartingPlots.uranium_ID or
					resourceType == -1 then
				end
			end
			if plotType == PlotTypes.PLOT_LAND or plotType == PlotTypes.PLOT_HILLS then
				if resourceType == AssignStartingPlots.wheat_ID and isFreshWater then
					bIsBonus = true;
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 4;
					lateFoodScore = lateFoodScore + 4;
				elseif resourceType == AssignStartingPlots.wheat_ID and not isFreshWater then
					bIsBonus = true;
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 3;
					lateFoodScore = lateFoodScore + 4;
				elseif resourceType == AssignStartingPlots.cow_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 1;
					midFoodScore = midFoodScore + 1;
					lateFoodScore = lateFoodScore + 2;
					midHammerScore = midHammerScore + 2;
					lateHammerScore = lateHammerScore + 2;
				elseif resourceType == AssignStartingPlots.sheep_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 1;
					midFoodScore = midFoodScore + 2;
					lateFoodScore = lateFoodScore + 3;
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 1;
				elseif resourceType == AssignStartingPlots.horse_ID then
					bIsBonus = true;
					lateFoodScore = lateFoodScore + 1;
					earlyHammerScore = earlyHammerScore + 1;
					midHammerScore = midHammerScore + 3;
					lateHammerScore = lateHammerScore + 3;
				elseif resourceType == AssignStartingPlots.deer_ID or resourceType == AssignStartingPlots.bison_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 2;
					lateFoodScore = lateFoodScore + 2;
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 1;
				elseif resourceType == AssignStartingPlots.banana_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 3;
					lateFoodScore = lateFoodScore + 4;
				elseif resourceType == AssignStartingPlots.stone_ID then
					bIsBonus = true;
					startHammerScore = startHammerScore + 1;
					earlyHammerScore = earlyHammerScore + 1;
					midHammerScore = midHammerScore + 3;
					lateHammerScore = lateHammerScore + 4;
				elseif resourceType == AssignStartingPlots.iron_ID then
					bIsBonus = true;
					earlyHammerScore = earlyHammerScore + 1;
					midHammerScore = midHammerScore + 3;
					lateHammerScore = lateHammerScore + 4;
				elseif resourceType == AssignStartingPlots.ivory_ID or resourceType == AssignStartingPlots.truffles_ID or resourceType == AssignStartingPlots.fur_ID then
					--
				elseif resourceType == AssignStartingPlots.silk_ID or
					resourceType == AssignStartingPlots.spices_ID or
					resourceType == AssignStartingPlots.dye_ID or
					resourceType == AssignStartingPlots.cotton_ID or 
					resourceType == AssignStartingPlots.sugar_ID or
					resourceType == AssignStartingPlots.wine_ID or
					resourceType == AssignStartingPlots.incense_ID then
					lateFoodScore = lateFoodScore + 1;
				elseif resourceType == AssignStartingPlots.citrus_ID or resourceType == AssignStartingPlots.cocoa_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 1;
					midFoodScore = midFoodScore + 1;
					lateFoodScore = lateFoodScore + 2;
				elseif resourceType == AssignStartingPlots.gold_ID or
					resourceType == AssignStartingPlots.silver_ID or
					resourceType == AssignStartingPlots.copper_ID or
					resourceType == AssignStartingPlots.gems_ID then
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 2;
				elseif resourceType == AssignStartingPlots.salt_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 1;
					midFoodScore = midFoodScore + 1;
					lateFoodScore = lateFoodScore + 1;
					midHammerScore = midHammerScore + 2;
					lateHammerScore = lateHammerScore + 3;
				elseif resourceType == AssignStartingPlots.marble_ID then
					bIsBonus = true;
					midHammerScore = midHammerScore + 2;
					lateHammerScore = lateHammerScore + 3;
				elseif resourceType == AssignStartingPlots.coal_ID or
					resourceType == AssignStartingPlots.aluminum_ID or
					resourceType == AssignStartingPlots.oil_ID or
					resourceType == AssignStartingPlots.uranium_ID or
					resourceType == -1 then
					if isFreshWater then
						if featureType ~= FeatureTypes.FEATURE_OASIS and terrainType ~= TerrainTypes.TERRAIN_SNOW then
							midFoodScore = midFoodScore + 2;
							lateFoodScore = lateFoodScore + 2;
						end
					else
						if plotType == PlotTypes.PLOT_LAND then
							if terrainType ~= TerrainTypes.TERRAIN_SNOW and terrainType ~= TerrainTypes.TERRAIN_TUNDRA then
								midFoodScore = midFoodScore + 1;
								lateFoodScore = lateFoodScore + 2;
							elseif terrainType == TerrainTypes.TERRAIN_TUNDRA and featureType == FeatureTypes.FEATURE_FOREST then
								midHammerScore = midHammerScore + 1;
								lateHammerScore = lateHammerScore + 2;
							end
						elseif plotType == PlotTypes.PLOT_HILLS then
							midHammerScore = midHammerScore + 1;
							lateHammerScore = lateHammerScore + 2;
						end
					end
				end
			end
			startBadTile = (startFoodScore + startHammerScore) < 2;
			earlyBadTile = (earlyFoodScore + earlyHammerScore) < 2;
			midBadTile = (midFoodScore + midHammerScore) < 3;
			lateBadTile = (lateFoodScore + lateHammerScore) < 4;
		end
	end
	return plotType, terrainType, featureType, resourceType, isFreshWater,
	startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore,
	startBadTile, earlyBadTile, midBadTile, lateBadTile, bIsBonus;
end
------------------------------------------------------------------------------
function AttemptToPlaceResource(x, y, bAllowOasis, bIsCoastal, needTwoFood, start_plot_database)
	-- Returns two booleans. First is true if something was placed. Second true if Oasis placed.
	--print("-"); print("Attempting to place a Bonus at: ", x, y);
	--local start_plot_database = AssignStartingPlots.Create();
	local plot = Map.GetPlot(x, y);
	if plot == nil then
		--print("Placement failed, plot was nil.");
		return false
	end
	if plot:IsStartingPlot() then
		return false
	end
	if IsNaturalWonder(plot) then
		return false;
	end
	local iNumStarts = table.maxn(start_plot_database.startingPlots);
	for region_number = 1, iNumStarts do
		local start_point_data = start_plot_database.startingPlots[region_number];
		if x == start_point_data[1] and y == start_point_data[2] then
			return false;
		end
	end
	if plot:GetResourceType(-1) ~= -1 then
		--print("Plot already had a resource.");
		return false
	end
	local terrainType = plot:GetTerrainType()
	if terrainType == TerrainTypes.TERRAIN_SNOW then
		--print("Plot was arctic land buried beneath endless snow.");
		return false
	end
	local featureType = plot:GetFeatureType()
	if featureType == FeatureTypes.FEATURE_OASIS then
		--print("Plot already had an Oasis.");
		return false
	end
	local plotType = plot:GetPlotType()
	--
	if not needTwoFood then
		if bAllowOasis then -- Place Oasis
			if plotType == PlotTypes.PLOT_LAND then
				if featureType == FeatureTypes.NO_FEATURE then
					if terrainType == TerrainTypes.TERRAIN_DESERT then
						if not plot:IsFreshWater() and not plot:IsCoastalLand() then
							plot:SetFeatureType(FeatureTypes.FEATURE_OASIS, -1);
							--print("Placed Oasis.");
							return true, true, -1
						end
					end
				end
			end
		elseif featureType == FeatureTypes.FEATURE_JUNGLE then -- Place Banana
			if plotType == PlotTypes.PLOT_LAND then
				local diceroll = Map.Rand(2, "");
				if diceroll == 1 then
					plot:SetResourceType(start_plot_database.banana_ID, 1);
					--print("Placed Banana.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] + 1;
					return true, false, start_plot_database.banana_ID
				else
					plot:SetPlotType(PlotTypes.PLOT_HILLS, false, true);
					return false;
				end
			else
				plot:SetResourceType(start_plot_database.banana_ID, 1);
				--print("Placed Banana.");
				start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] + 1;
				return true, false, start_plot_database.banana_ID
			end
		elseif featureType == FeatureTypes.FEATURE_FOREST then -- Place Deer
			plot:SetResourceType(start_plot_database.deer_ID, 1);
			--print("Placed Deer.");
			start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] + 1;
			return true, false, start_plot_database.deer_ID
		elseif plotType == PlotTypes.PLOT_HILLS and featureType == FeatureTypes.NO_FEATURE then
			plot:SetResourceType(start_plot_database.sheep_ID, 1);
			--print("Placed Sheep.");
			start_plot_database.amounts_of_resources_placed[start_plot_database.sheep_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.sheep_ID + 1] + 1;
			return true, false, start_plot_database.sheep_ID
		elseif plotType == PlotTypes.PLOT_LAND then
			if featureType == FeatureTypes.NO_FEATURE then
				if terrainType == TerrainTypes.TERRAIN_GRASS then -- Place Bison or Cows
					local diceroll = Map.Rand(3, "");
					if diceroll == 1 then
						plot:SetResourceType(start_plot_database.bison_ID, 1);
						--print("Placed Bison.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] + 1;
						return true, false, start_plot_database.bison_ID
					elseif diceroll == 2 then
						plot:SetResourceType(start_plot_database.cow_ID, 1);
						--print("Placed Cow.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.cow_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.cow_ID + 1] + 1;
						return true, false, start_plot_database.cow_ID
					else
						plot:SetResourceType(start_plot_database.stone_ID, 1);
						--print("Placed Stone.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] + 1;
						return true, false, start_plot_database.stone_ID
					end
				elseif terrainType == TerrainTypes.TERRAIN_PLAINS then -- Place Bison or Wheat
					local diceroll = Map.Rand(4, "");
					if diceroll == 1 then
						plot:SetResourceType(start_plot_database.bison_ID, 1);
						--print("Placed Bison.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] + 1;
						return true, false, start_plot_database.bison_ID
					elseif diceroll == 2 then
						plot:SetResourceType(start_plot_database.wheat_ID, 1);
						--print("Placed Wheat.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] + 1;
						return true, false, start_plot_database.wheat_ID
					else
						plot:SetResourceType(start_plot_database.cow_ID, 1);
						--print("Placed Cow.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.cow_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.cow_ID + 1] + 1;
						return true, false, start_plot_database.cow_ID
					end
				elseif terrainType == TerrainTypes.TERRAIN_TUNDRA then -- Place Deer
					plot:SetResourceType(start_plot_database.deer_ID, 1);
					--print("Placed Deer.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] + 1;
					return true, false, start_plot_database.deer_ID
				elseif terrainType == TerrainTypes.TERRAIN_DESERT then
					local diceroll = Map.Rand(2, "");
					if diceroll == 1 then
						if plot:IsFreshWater() then -- Place Wheat
							plot:SetResourceType(start_plot_database.wheat_ID, 1);
							--print("Placed Wheat.");
							start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] + 1;
							return true, false, start_plot_database.wheat_ID
						else
							plot:SetResourceType(start_plot_database.stone_ID, 1);
							--print("Placed Stone.");
							start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] + 1;
							return true, false, start_plot_database.stone_ID
						end
					else
						start_plot_database:AttemptToPlaceHillsAtPlot(x, y, false);
						return false, false, -1;
					end
				end
			elseif featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then
				plot:SetResourceType(start_plot_database.wheat_ID, 1);
				--print("Placed Wheat.");
				start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] + 1;
				return true, false, start_plot_database.wheat_ID
			end
		--[[elseif plotType == PlotTypes.PLOT_OCEAN and bIsCoastal then
			if terrainType == TerrainTypes.TERRAIN_COAST and featureType == FeatureTypes.NO_FEATURE then
				if plot:IsLake() == false then -- Place Fish
					plot:SetResourceType(start_plot_database.fish_ID, 1);
					--print("Placed Fish.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.fish_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.fish_ID + 1] + 1;
					return true, false, start_plot_database.fish_ID
				end
			end]]
		end	
	else
		if bAllowOasis then -- Place Oasis
			if plotType == PlotTypes.PLOT_LAND then
				if featureType == FeatureTypes.NO_FEATURE then
					if terrainType == TerrainTypes.TERRAIN_DESERT then
						if not plot:IsFreshWater() and not plot:IsCoastalLand() then
							plot:SetFeatureType(FeatureTypes.FEATURE_OASIS, -1);
							--print("Placed Oasis.");
							return true, true, -1
						end
					end
				end
			end
		elseif featureType == FeatureTypes.FEATURE_JUNGLE then -- Place Banana
			if plotType == PlotTypes.PLOT_LAND then
				local diceroll = Map.Rand(2, "");
				if diceroll == 1 then
					plot:SetResourceType(start_plot_database.banana_ID, 1);
					--print("Placed Banana.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] + 1;
					return true, false, start_plot_database.banana_ID
				else
					plot:SetPlotType(PlotTypes.PLOT_HILLS, false, true);
					return false;
				end
			else
				plot:SetResourceType(start_plot_database.banana_ID, 1);
				--print("Placed Banana.");
				start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] + 1;
				return true, false, start_plot_database.banana_ID
			end
		elseif featureType == FeatureTypes.FEATURE_FOREST then -- Place Deer
			plot:SetResourceType(start_plot_database.deer_ID, 1);
			--print("Placed Deer.");
			start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] + 1;
			return true, false, start_plot_database.deer_ID
		elseif plotType == PlotTypes.PLOT_HILLS and featureType == FeatureTypes.NO_FEATURE and
			(terrainType == TerrainTypes.TERRAIN_GRASS or terrainType == TerrainTypes.TERRAIN_PLAINS or terrainType == TerrainTypes.TERRAIN_TUNDRA) then
			plot:SetFeatureType(FeatureTypes.FEATURE_FOREST, -1)
			plot:SetResourceType(start_plot_database.deer_ID, 1);
			--print("Placed Deer.");
			start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] + 1;
			return true, false, start_plot_database.deer_ID
		elseif plotType == PlotTypes.PLOT_LAND then
			if featureType == FeatureTypes.NO_FEATURE then
				if terrainType == TerrainTypes.TERRAIN_GRASS then -- Place Bison or Cows
					local diceroll = Map.Rand(3, "");
					if diceroll == 1 then
						plot:SetResourceType(start_plot_database.bison_ID, 1);
						--print("Placed Bison.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] + 1;
						return true, false, start_plot_database.bison_ID
					elseif diceroll == 2 then
						plot:SetResourceType(start_plot_database.cow_ID, 1);
						--print("Placed Cow.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.cow_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.cow_ID + 1] + 1;
						return true, false, start_plot_database.cow_ID
					else
						plot:SetResourceType(start_plot_database.stone_ID, 1);
						--print("Placed Stone.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] + 1;
						return true, false, start_plot_database.stone_ID
					end
				elseif terrainType == TerrainTypes.TERRAIN_PLAINS then -- Place Bison or Wheat
					local diceroll = Map.Rand(2, "");
					if diceroll == 1 then
						plot:SetResourceType(start_plot_database.bison_ID, 1);
						--print("Placed Bison.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] + 1;
						return true, false, start_plot_database.bison_ID
					else
						plot:SetResourceType(start_plot_database.wheat_ID, 1);
						--print("Placed Wheat.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] + 1;
						return true, false, start_plot_database.wheat_ID
					end
				elseif terrainType == TerrainTypes.TERRAIN_TUNDRA then -- Place Deer
					plot:SetFeatureType(FeatureTypes.FEATURE_FOREST, -1)
					plot:SetResourceType(start_plot_database.deer_ID, 1);
					--print("Placed Deer.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.deer_ID + 1] + 1;
					return true, false, start_plot_database.deer_ID
				elseif terrainType == TerrainTypes.TERRAIN_DESERT then
					if plot:IsFreshWater() then -- Place Wheat
						plot:SetResourceType(start_plot_database.wheat_ID, 1);
						--print("Placed Wheat.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] + 1;
						return true, false, start_plot_database.wheat_ID
					--else
						--print("Not allowed to place any more Oasis help at this site.");
					end
				end
			elseif featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then
				plot:SetResourceType(start_plot_database.wheat_ID, 1);
				--print("Placed Wheat.");
				start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.wheat_ID + 1] + 1;
				return true, false, start_plot_database.wheat_ID
			end
		--[[elseif plotType == PlotTypes.PLOT_OCEAN and bIsCoastal then
			if terrainType == TerrainTypes.TERRAIN_COAST and featureType == FeatureTypes.NO_FEATURE then
				if plot:IsLake() == false then -- Place Fish
					plot:SetResourceType(start_plot_database.fish_ID, 1);
					--print("Placed Fish.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.fish_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.fish_ID + 1] + 1;
					return true, false, start_plot_database.fish_ID
				end
			end]]
		end
	end
	-- Nothing placed.
	return false, false, -1
end
------------------------------------------------------------------------------
function AddRivers()
	
	print("Map Generation - Adding Rivers");

	if Map.GetCustomOption(6) == 1 then
		local passConditions = {
			
			function(plot)
				local startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
							iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, iNumMountains, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots:Create(), false);
				local area = plot:Area();
				local plotsPerRiverEdge = GameDefines["PLOTS_PER_RIVER_EDGE"];
				plotsPerRiverEdge =  4;
				return (iNumBadDesert > 1);
			end,
			function(plot)
				return plot:IsHills() or plot:IsMountain();
			end,
			
			function(plot)
				return (not plot:IsCoastalLand()) and (Map.Rand(8, "MapGenerator AddRivers") == 0);
			end,
			
			function(plot)
				local area = plot:Area();
				local plotsPerRiverEdge = GameDefines["PLOTS_PER_RIVER_EDGE"];
				--plotsPerRiverEdge =  8;
				return (plot:IsHills() or plot:IsMountain()) and (area:GetNumRiverEdges() <	((area:GetNumTiles() / plotsPerRiverEdge) + 1));
			end,
			
			function(plot)
				local area = plot:Area();
				local plotsPerRiverEdge = GameDefines["PLOTS_PER_RIVER_EDGE"];
				--plotsPerRiverEdge =  8;
				return (area:GetNumRiverEdges() < (area:GetNumTiles() / plotsPerRiverEdge) + 1);
			end
		}
		
		for iPass, passCondition in ipairs(passConditions) do
			
			local riverSourceRange;
			local seaWaterRange;

			if (iPass == 2 or iPass == 3) then
				riverSourceRange = GameDefines["RIVER_SOURCE_MIN_RIVER_RANGE"];
				seaWaterRange = GameDefines["RIVER_SOURCE_MIN_SEAWATER_RANGE"];
			elseif (iPsss == 1) then
				riverSourceRange = 0;
				seaWaterRange = 0;
			else
				riverSourceRange = (GameDefines["RIVER_SOURCE_MIN_RIVER_RANGE"] / 2);
				seaWaterRange = (GameDefines["RIVER_SOURCE_MIN_SEAWATER_RANGE"] / 2);
			end
				
			for i, plot in Plots() do 
				if(not plot:IsWater()) then
					if(passCondition(plot)) then
						if (not Map.FindWater(plot, riverSourceRange, true)) then
							if (not Map.FindWater(plot, seaWaterRange, false)) then
								local inlandCorner = plot:GetInlandCorner();
								if(inlandCorner) then
									DoRiver(inlandCorner);
								end
							end
						end
					end			
				end
			end
		end	
	else
		local passConditions = {
			function(plot)
				return plot:IsHills() or plot:IsMountain();
			end,
			
			function(plot)
				return (not plot:IsCoastalLand()) and (Map.Rand(8, "MapGenerator AddRivers") == 0);
			end,
			
			function(plot)
				local area = plot:Area();
				local plotsPerRiverEdge = GameDefines["PLOTS_PER_RIVER_EDGE"];
				return (plot:IsHills() or plot:IsMountain()) and (area:GetNumRiverEdges() <	((area:GetNumTiles() / plotsPerRiverEdge) + 1));
			end,
			
			function(plot)
				local area = plot:Area();
				local plotsPerRiverEdge = GameDefines["PLOTS_PER_RIVER_EDGE"];
				return (area:GetNumRiverEdges() < (area:GetNumTiles() / plotsPerRiverEdge) + 1);
			end
		}
		
		for iPass, passCondition in ipairs(passConditions) do
			
			local riverSourceRange;
			local seaWaterRange;
				
			if (iPass <= 2) then
				riverSourceRange = GameDefines["RIVER_SOURCE_MIN_RIVER_RANGE"];
				seaWaterRange = GameDefines["RIVER_SOURCE_MIN_SEAWATER_RANGE"];
			else
				riverSourceRange = (GameDefines["RIVER_SOURCE_MIN_RIVER_RANGE"] / 2);
				seaWaterRange = (GameDefines["RIVER_SOURCE_MIN_SEAWATER_RANGE"] / 2);
			end
				
			for i, plot in Plots() do 
				if(not plot:IsWater()) then
					if(passCondition(plot)) then
						if (not Map.FindWater(plot, riverSourceRange, true)) then
							if (not Map.FindWater(plot, seaWaterRange, false)) then
								local inlandCorner = plot:GetInlandCorner();
								if(inlandCorner) then
									DoRiver(inlandCorner);
								end
							end
						end
					end			
				end
			end
		end		
	end
end
------------------------------------------------------------------------------
function AssignStartingPlots:FixSugarJungles()
	local iW, iH = Map.GetGridSize()
	for y = 0, iH - 1 do
		for x = 0, iW - 1 do
			local plot = Map.GetPlot(x, y)
			if plot:GetResourceType(-1) == self.gold_ID or plot:GetResourceType(-1) == self.salt_ID or plot:GetResourceType(-1) == self.marble_ID or plot:GetResourceType(-1) == self.incense_ID then
				local terrainType = plot:GetTerrainType();
				local featureType = plot:GetFeatureType();
				if terrainType == TerrainTypes.TERRAIN_DESERT and featureType == FeatureTypes.NO_FEATURE then
					local plotType = plot:GetPlotType()
					if plotType ~= PlotTypes.PLOT_HILLS then
						plot:SetPlotType(PlotTypes.PLOT_HILLS, false, true)
					end
				end
			end
			if plot:GetResourceType(-1) == self.gold_ID then
				if plot:GetPlotType() ~= PlotTypes.PLOT_HILLS then
					plot:SetPlotType(PlotTypes.PLOT_HILLS, false, true)
				end
			end
			if plot:GetResourceType(-1) == self.fur_ID or plot:GetResourceType(-1) == self.dye_ID or plot:GetResourceType(-1) == self.spices_ID or plot:GetResourceType(-1) == self.silk_ID or plot:GetResourceType(-1) == self.citrus_ID or plot:GetResourceType(-1) == self.truffles_ID or plot:GetResourceType(-1) == self.cocoa_ID then
				if plot:GetTerrainType() ~= TerrainTypes.TERRAIN_TUNDRA and plot:GetFeatureType() == FeatureTypes.NO_FEATURE then
					plot:SetFeatureType(FeatureTypes.FEATURE_FOREST, -1)
				end
			end
		end
	end
end
------------------------------------------------------------------------------
function IsPlotInCSRange(AssignStartingPlots, plot_x, plot_y, range)
	for city_state = 1, AssignStartingPlots.iNumCityStates do
		-- First check to see if this city state number received a valid start plot.
		if AssignStartingPlots.city_state_validity_table[city_state] == false then
			-- This one did not! It does not exist on the map nor have valid data, so we will ignore it.
		else
			-- OK, it's a valid city state. Process it.
			local x = AssignStartingPlots.cityStatePlots[city_state][1];
			local y = AssignStartingPlots.cityStatePlots[city_state][2];
			if Map.PlotDistance(x, y, plot_x, plot_y) <= range then
				return true;
			end
		end
	end

	return false;
end
------------------------------------------------------------------------------
function IsPlotInOtherRegionCapRange(AssignStartingPlots, plot_x, plot_y, this_region_number, range)
	for loop, reg_data in ipairs(AssignStartingPlots.regions_sorted_by_type) do
		local region_number = reg_data[1];
		if region_number ~= this_region_number then
			local x = AssignStartingPlots.startingPlots[region_number][1];
			local y = AssignStartingPlots.startingPlots[region_number][2];
			if Map.PlotDistance(x, y, plot_x, plot_y) <= range then
				return true;
			end
		end
	end

	return false;
end
------------------------------------------------------------------------------
function GenerateLuxuryPlotListsInRegionOrRange(AssignStartingPlots, region_number, min_range, max_range)
	local iW, iH = Map.GetGridSize();
	-- This function groups a region's plots in to lists, for Luxury resource assignment.
	local region_data_table = AssignStartingPlots.regionData[region_number];
	local iWestX = region_data_table[1];
	local iSouthY = region_data_table[2];
	local iWidth = region_data_table[3];
	local iHeight = region_data_table[4];
	local iAreaID = region_data_table[5];
	local region_area_object;
	if iAreaID ~= -1 then
		region_area_object = Map.GetArea(iAreaID);
	end

	local region_coast, region_marsh, region_flood_plains, region_tundra_flat_including_forests = {}, {}, {}, {};
	local region_hills_open, region_hills_covered, region_hills_jungle, region_hills_forest = {}, {}, {}, {};
	local region_desert_flat_no_feature, region_plains_flat_no_feature, region_jungle_flat = {}, {}, {};
	local region_forest_flat, region_forest_flat_but_not_tundra = {}, {};
	local region_dry_grass_flat_no_feature, region_fresh_water_grass_flat_no_feature = {}, {};

	-- Iterate through the region's plots, building the fifteen lists defined above.
	for region_loop_y = 0, iHeight - 1 do
		for region_loop_x = 0, iWidth - 1 do
			local x = (region_loop_x + iWestX) % iW;
			local y = (region_loop_y + iSouthY) % iH;
			local isStartingPlot = x == AssignStartingPlots.startingPlots[region_number][1] and y == AssignStartingPlots.startingPlots[region_number][2]
			local isPlotInCSRange = IsPlotInCSRange(AssignStartingPlots, x, y, 3)
			local isPlotInOtherRegionCapRange = IsPlotInOtherRegionCapRange(AssignStartingPlots, x, y, region_number, 4)
			if min_range <= max_range and (max_range == 0 or min_range <= Map.PlotDistance(x, y, AssignStartingPlots.startingPlots[region_number][1], AssignStartingPlots.startingPlots[region_number][2]) and Map.PlotDistance(x, y, AssignStartingPlots.startingPlots[region_number][1], AssignStartingPlots.startingPlots[region_number][2]) <= max_range) and not isStartingPlot and not isPlotInCSRange and not isPlotInOtherRegionCapRange then
				local plotIndex = y * iW + x + 1;
				local plot = Map.GetPlot(x, y);
				local area_of_plot = plot:GetArea();
				-- get plot info
				local plotType = plot:GetPlotType()
				local terrainType = plot:GetTerrainType()
				local featureType = plot:GetFeatureType()
				--
				if plotType == PlotTypes.PLOT_OCEAN then
					if terrainType == TerrainTypes.TERRAIN_COAST then
						if plot:IsLake() == false then
							if featureType ~= AssignStartingPlots.feature_atoll and featureType ~= FeatureTypes.FEATURE_ICE then
								if iAreaID == -1 then
									if plot:IsAdjacentToLand() then
										table.insert(region_coast, plotIndex);
									end
								else
									if plot:IsAdjacentToArea(region_area_object) then
										table.insert(region_coast, plotIndex);
									end
								end
							end
						end
					end
				elseif plotType == PlotTypes.PLOT_HILLS and terrainType ~= TerrainTypes.TERRAIN_SNOW then
					if featureType == FeatureTypes.NO_FEATURE then
						table.insert(region_hills_open, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_JUNGLE then		
						table.insert(region_hills_jungle, plotIndex);
						table.insert(region_hills_covered, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_FOREST then		
						table.insert(region_hills_forest, plotIndex);
						table.insert(region_hills_covered, plotIndex);
					end
				elseif plotType == PlotTypes.PLOT_LAND then
					if featureType == FeatureTypes.NO_FEATURE then
						if terrainType == TerrainTypes.TERRAIN_TUNDRA then
							table.insert(region_tundra_flat_including_forests, plotIndex);
						elseif terrainType == TerrainTypes.TERRAIN_DESERT then
							table.insert(region_desert_flat_no_feature, plotIndex);
						elseif terrainType == TerrainTypes.TERRAIN_PLAINS then
							table.insert(region_plains_flat_no_feature, plotIndex);
						elseif terrainType == TerrainTypes.TERRAIN_GRASS then
							if plot:IsFreshWater() then
								table.insert(region_fresh_water_grass_flat_no_feature, plotIndex);
							else
								table.insert(region_dry_grass_flat_no_feature, plotIndex);
							end
						end
					elseif featureType == FeatureTypes.FEATURE_MARSH then		
						table.insert(region_marsh, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then		
						table.insert(region_flood_plains, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_JUNGLE then		
						table.insert(region_jungle_flat, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_FOREST then		
						table.insert(region_forest_flat, plotIndex);
						if terrainType == TerrainTypes.TERRAIN_TUNDRA then
							table.insert(region_tundra_flat_including_forests, plotIndex);
						else
							table.insert(region_forest_flat_but_not_tundra, plotIndex);
						end
					end
				end
			end
		end
	end
	
	-- Luxury Plot Lists
	local results_table = {
	region_coast, -- (Coast next to land)		-- 1
	region_marsh,								-- 2
	region_flood_plains,						-- 3
	region_hills_open,							-- 4
	region_hills_covered,						-- 5
	region_hills_jungle,						-- 6
	region_hills_forest,						-- 7
	region_jungle_flat,							-- 8
	region_forest_flat,							-- 9
	region_desert_flat_no_feature,				-- 10
	region_plains_flat_no_feature,				-- 11			
	region_dry_grass_flat_no_feature,			-- 12
	region_fresh_water_grass_flat_no_feature,	-- 13
	region_tundra_flat_including_forests,		-- 14
	region_forest_flat_but_not_tundra			-- 15
	};
	return results_table
end
------------------------------------------------------------------------------
function AssignStartingPlots:GetDisabledLuxuriesTargetNumber()
	-- This data was separated out to allow easy replacement in map scripts.
	local worldsizes = {
		[GameInfo.Worlds.WORLDSIZE_DUEL.ID] = 11,
		[GameInfo.Worlds.WORLDSIZE_TINY.ID] = 8,
		[GameInfo.Worlds.WORLDSIZE_SMALL.ID] = 4,
		[GameInfo.Worlds.WORLDSIZE_STANDARD.ID] = 4,
		[GameInfo.Worlds.WORLDSIZE_LARGE.ID] = 2,
		[GameInfo.Worlds.WORLDSIZE_HUGE.ID] = 1
		}
	local maxToDisable = worldsizes[Map.GetWorldSize()];
	return maxToDisable
end
------------------------------------------------------------------------------
function AssignStartingPlots:GetIndicesForLuxuryType(resource_ID)
	-- This function will identify up to four of the fifteen "Luxury Plot Lists"
	-- (visually listed on screen directly above this text) that match terrain 
	-- best suitable for this type of luxury.
	--print("-"); print("Obtaining indices for Luxury#", resource_ID);
	local primary, secondary, tertiary, quaternary, quinternary, sexternary = -1, -1, -1, -1, -1, -1;
	if resource_ID == self.whale_ID then
		primary = 1;
	elseif resource_ID == self.pearls_ID then
		primary = 1;
	elseif resource_ID == self.gold_ID then
		primary, secondary, tertiary, quaternary, quinternary = 4, 10, 5, 11, 12;
	elseif resource_ID == self.silver_ID then
		primary, secondary, tertiary, quaternary = 4, 5, 14, 12;
	elseif resource_ID == self.gems_ID then
		primary, secondary, tertiary, quaternary = 6, 7, 4, 8;
	elseif resource_ID == self.marble_ID then
		primary, secondary, tertiary, quaternary = 12, 10, 11, 4;
	elseif resource_ID == self.ivory_ID then
		primary, secondary, tertiary = 11, 12, 13;
	elseif resource_ID == self.fur_ID then
		primary, secondary, tertiary, quaternary = 14, 15, 11, 12;
	elseif resource_ID == self.dye_ID then
		primary, secondary, tertiary, quaternary, quinternary = 9, 8, 2, 11, 12;
	elseif resource_ID == self.spices_ID then
		primary, secondary, tertiary, quaternary, quinternary = 8, 15, 2, 11, 12;
	elseif resource_ID == self.silk_ID then
		primary, secondary, tertiary, quaternary, quinternary = 15, 8, 11, 12, 13;
	elseif resource_ID == self.sugar_ID then
		primary, secondary, tertiary, quaternary, quinternary = 2, 8, 3, 13, 12;
	elseif resource_ID == self.cotton_ID then
		primary, secondary, tertiary = 3, 13, 12;
	elseif resource_ID == self.wine_ID then
		primary, secondary, tertiary = 11, 12, 13;
	elseif resource_ID == self.incense_ID then
		primary, secondary, tertiary = 10, 3, 11;
	elseif resource_ID == self.copper_ID then
		primary, secondary, tertiary, quaternary = 4, 5, 12, 14;
	elseif resource_ID == self.salt_ID then
		primary, secondary, tertiary, quaternary = 11, 10, 14, 9;
	elseif resource_ID == self.citrus_ID then
		primary, secondary, tertiary, quaternary, quinternary, sexternary = 8, 6, 15, 3, 11, 12;
	elseif resource_ID == self.truffles_ID then
		primary, secondary, tertiary, quaternary, quinternary, sexternary = 15, 8, 2, 5, 11, 12;
	elseif resource_ID == self.crab_ID then
		primary = 1;
	elseif resource_ID == self.cocoa_ID then
		primary, secondary, tertiary, quaternary, quinternary, sexternary = 8, 6, 15, 11, 12, 13;
	end
	--print("Found indices of", primary, secondary, tertiary, quaternary);
	return primary, secondary, tertiary, quaternary, quinternary, sexternary;
end
------------------------------------------------------------------------------
function AssignStartingPlots:PlaceLuxuries()
	-- This function is dependent upon AssignLuxuryRoles() and PlaceCityStates() having been executed first.
	local iW, iH = Map.GetGridSize();
	local res = Map.GetCustomOption(5);
	-- Place Luxuries at civ start locations.
	local placedInCap = {};
	for loop, reg_data in ipairs(self.regions_sorted_by_type) do
		placedInCap[reg_data[1]] = 0;
	end
	for loop, reg_data in ipairs(self.regions_sorted_by_type) do
		local region_number = reg_data[1];
		local this_region_luxury = reg_data[2];
		local x = self.startingPlots[region_number][1];
		local y = self.startingPlots[region_number][2];
		print("-"); print("Attempting to place Luxury#", this_region_luxury, "at start plot", x, y, "in Region#", region_number);
		-- Determine number to place at the start location
		local iNumToPlace = 2;
		if res == 4 then -- Legendary Start
			iNumToPlace = 3;
		end
		-- Obtain plot lists appropriate to this luxury type.
		local primary, secondary, tertiary, quaternary, quinternary, sexternary, luxury_plot_lists, shuf_list;
		primary, secondary, tertiary, quaternary, quinternary, sexternary = self:GetIndicesForLuxuryType(this_region_luxury);
		luxury_plot_lists = self:GenerateLuxuryPlotListsAtCitySite(x, y, 2, false)

		-- First pass, checking only first two rings with a 50% ratio.
		shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
		local iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumToPlace, 0.5, -1, 0, 0, shuf_list);
		if iNumLeftToPlace > 0 and secondary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 0.5, -1, 0, 0, shuf_list);
		end
		if iNumLeftToPlace > 0 and tertiary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 0.5, -1, 0, 0, shuf_list);
		end
		if iNumLeftToPlace > 0 and quaternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 0.5, -1, 0, 0, shuf_list);
		end
		if iNumLeftToPlace > 0 and quinternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 0.5, -1, 0, 0, shuf_list);
		end
		if iNumLeftToPlace > 0 and sexternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 0.5, -1, 0, 0, shuf_list);
		end

		if iNumLeftToPlace > 0 then
			-- Second pass, checking three rings with a 100% ratio.
			luxury_plot_lists = self:GenerateLuxuryPlotListsAtCitySite(x, y, 3, false)
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 1, -1, 0, 0, shuf_list);
			if iNumLeftToPlace > 0 and secondary > 0 then
				shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 1, -1, 0, 0, shuf_list);
			end
			if iNumLeftToPlace > 0 and tertiary > 0 then
				shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 1, -1, 0, 0, shuf_list);
			end
			if iNumLeftToPlace > 0 and quaternary > 0 then
				shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 1, -1, 0, 0, shuf_list);
			end
			if iNumLeftToPlace > 0 and quinternary > 0 then
				shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 1, -1, 0, 0, shuf_list);
			end
			if iNumLeftToPlace > 0 and sexternary > 0 then
				shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(this_region_luxury, 1, iNumLeftToPlace, 1, -1, 0, 0, shuf_list);
			end
		end

		if iNumLeftToPlace > 0 then
			-- If we haven't been able to place all of this lux type at the start, it CAN be placed
			-- in the region somewhere. Subtract remainder from this region's compensation, so that the
			-- regional process, later, will attempt to place this remainder somewhere in the region.
			self.luxury_low_fert_compensation[this_region_luxury] = self.luxury_low_fert_compensation[this_region_luxury] - iNumLeftToPlace;
			self.region_low_fert_compensation[region_number] = self.region_low_fert_compensation[region_number] - iNumLeftToPlace;
			placedInCap[region_number] = iNumLeftToPlace;
		end
		if iNumLeftToPlace > 0 and self.iNumTypesRandom > 0 then
			-- We'll attempt to place one source of a Luxury type assigned to random distribution.
			local randoms_to_place = 1;
			for loop, random_res in ipairs(self.resourceIDs_assigned_to_random) do
		 		primary, secondary, tertiary, quaternary, quinternary, sexternary = self:GetIndicesForLuxuryType(random_res);
		 		if randoms_to_place > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
					randoms_to_place = self:PlaceSpecificNumberOfResources(random_res, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				if randoms_to_place > 0 and secondary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
					randoms_to_place = self:PlaceSpecificNumberOfResources(random_res, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				if randoms_to_place > 0 and tertiary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
					randoms_to_place = self:PlaceSpecificNumberOfResources(random_res, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				if randoms_to_place > 0 and quaternary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
					randoms_to_place = self:PlaceSpecificNumberOfResources(random_res, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				if randoms_to_place > 0 and quinternary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
					randoms_to_place = self:PlaceSpecificNumberOfResources(random_res, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				if randoms_to_place > 0 and sexternary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
					randoms_to_place = self:PlaceSpecificNumberOfResources(random_res, 1, 1, 1, -1, 0, 0, shuf_list);
				end
			end
		end
	end
	
	-- Place Luxuries at City States.
	-- Candidates include luxuries exclusive to CS, the lux assigned to this CS's region (if in a region), and the randoms.
	for city_state = 1, self.iNumCityStates do
		-- First check to see if this city state number received a valid start plot.
		if self.city_state_validity_table[city_state] == false then
			-- This one did not! It does not exist on the map nor have valid data, so we will ignore it.
		else
			-- OK, it's a valid city state. Process it.
			local region_number = self.city_state_region_assignments[city_state];
			local x = self.cityStatePlots[city_state][1];
			local y = self.cityStatePlots[city_state][2];
			local allowed_luxuries = self:GetListOfAllowableLuxuriesAtCitySite(x, y, 2)
			local lux_possible_for_cs = {}; -- Recorded with ID as key, weighting as data entry
			-- Identify Allowable Luxuries assigned to City States.
			-- If any CS-Only types are eligible, then all combined will have a weighting of 75%
			local cs_only_types = {};
			for loop, res_ID in ipairs(self.resourceIDs_assigned_to_cs) do
				if allowed_luxuries[res_ID] == true then
					table.insert(cs_only_types, res_ID);
				end
			end
			local iNumCSAllowed = table.maxn(cs_only_types);
			if iNumCSAllowed > 0 then
				for loop, res_ID in ipairs(cs_only_types) do
					lux_possible_for_cs[res_ID] = 75 / iNumCSAllowed;
				end
			end
			-- Identify Allowable Random Luxuries and the Regional Luxury if any.
			-- If any random types are eligible (plus the regional type if in a region) these combined carry a 25% weighting.
			if self.iNumTypesRandom > 0 or region_number > 0 then
				local random_types_allowed = {};
				for loop, res_ID in ipairs(self.resourceIDs_assigned_to_random) do
					if allowed_luxuries[res_ID] == true then
						table.insert(random_types_allowed, res_ID);
					end
				end
				local iNumRandAllowed = table.maxn(random_types_allowed);
				local iNumAllowed = iNumRandAllowed;
				--[[
				if region_number > 0 then
					iNumAllowed = iNumAllowed + 1; -- Adding the region type in to the mix with the random types.
					local res_ID = self.region_luxury_assignment[region_number];
					if allowed_luxuries[res_ID] == true then
						lux_possible_for_cs[res_ID] = 25 / iNumAllowed;
					end
				end
				]]
				--[[if iNumRandAllowed > 0 then
					for loop, res_ID in ipairs(random_types_allowed) do
						lux_possible_for_cs[res_ID] = 25 / iNumAllowed;
					end
				end]]
			end

			-- If there are no allowable luxury types at this city site, then this city state gets none.
			local iNumAvailableTypes = table.maxn(lux_possible_for_cs);
			if iNumAvailableTypes == 0 then
				--print("City State #", city_state, "has poor land, ineligible to receive a Luxury resource.");
			else
				-- Calculate probability thresholds for each allowable luxury type.
				local res_threshold = {};
				local totalWeight, accumulatedWeight = 0, 0;
				for res_ID, this_weight in pairs(lux_possible_for_cs) do
					totalWeight = totalWeight + this_weight;
				end
				for res_ID, this_weight in pairs(lux_possible_for_cs) do
					local threshold = (this_weight + accumulatedWeight) * 10000 / totalWeight;
					res_threshold[res_ID] = threshold;
					accumulatedWeight = accumulatedWeight + this_weight;
				end
				-- Choose luxury type.
				local use_this_ID;
				local diceroll = Map.Rand(10000, "Choose resource type - Assign Luxury To City State - Lua");
				for res_ID, threshold in pairs(res_threshold) do
					if diceroll < threshold then -- Choose this resource type.
						use_this_ID = res_ID;
						break
					end
				end
				print("-"); print("-"); print("-Assigned Luxury Type", use_this_ID, "to City State#", city_state);
				-- Place luxury.
				local primary, secondary, tertiary, quaternary, quinternary, sexternary, luxury_plot_lists, shuf_list;
				primary, secondary, tertiary, quaternary, quinternary, sexternary = self:GetIndicesForLuxuryType(use_this_ID);
				luxury_plot_lists = self:GenerateLuxuryPlotListsAtCitySite(x, y, 2, false)
				shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
				local iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
				if iNumLeftToPlace > 0 and secondary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
					iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				if iNumLeftToPlace > 0 and tertiary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
					iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				if iNumLeftToPlace > 0 and quaternary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
					iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				if iNumLeftToPlace > 0 and quinternary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
					iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				if iNumLeftToPlace > 0 and sexternary > 0 then
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
					iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
				end
				--if iNumLeftToPlace == 0 then
					--print("-"); print("Placed Luxury ID#", use_this_ID, "at City State#", city_state, "in Region#", region_number, "located at Plot", x, y);
				--end
			end
		end
	end
		
	-- Place Regional Luxuries
	for region_number, res_ID in ipairs(self.region_luxury_assignment) do
		print("-"); print("- - -"); print("Attempting to place regional luxury #", res_ID, "in Region#", region_number);
		local iNumAlreadyPlaced = self.amounts_of_resources_placed[res_ID + 1];
		local assignment_split = self.luxury_assignment_count[res_ID];
		local primary, secondary, tertiary, quaternary, quinternary, sexternary, luxury_plot_lists, shuf_list, iNumLeftToPlace;
		primary, secondary, tertiary, quaternary, quinternary, sexternary = self:GetIndicesForLuxuryType(res_ID);
		luxury_plot_lists = GenerateLuxuryPlotListsInRegionOrRange(self, region_number, 0, 6);

		-- Calibrate number of luxuries per region to world size and number of civs
		-- present. The amount of lux per region should be at its highest when the 
		-- number of civs in the game is closest to "default" for that map size.
		local target_list = self:GetRegionLuxuryTargetNumbers()
		local targetNum = target_list[self.iNumCivs] + placedInCap[region_number] - 1;
		-- Adjust target number according to Resource Setting.
		if res == 1 then
			targetNum = targetNum - 1;
		elseif res == 3 then
			targetNum = targetNum + 1
		end
		local iNumThisLuxToPlace = math.max(1, targetNum); -- Always place at least one.

		--print("-"); print("Target number for Luxury#", res_ID, "with assignment split of", assignment_split, "is", targetNum);
		
		-- Place luxuries.
		shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumThisLuxToPlace, 0.3, 2, 0, 3, shuf_list);
		if iNumLeftToPlace > 0 and secondary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, 2, 0, 3, shuf_list);
		end
		if iNumLeftToPlace > 0 and tertiary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.4, 2, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and quaternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, 2, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and quinternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, 2, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and sexternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, 2, 0, 2, shuf_list);
		end
		luxury_plot_lists = GenerateLuxuryPlotListsInRegionOrRange(self, region_number, 4, 6);
		shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, -1, 0, 3, shuf_list);
		if iNumLeftToPlace > 0 and secondary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, -1, 0, 3, shuf_list);
		end
		if iNumLeftToPlace > 0 and tertiary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.4, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and quaternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and quinternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and sexternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, -1, 0, 2, shuf_list);
		end
		luxury_plot_lists = GenerateLuxuryPlotListsInRegionOrRange(self, region_number, 0, 6);
		shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, -1, 0, 3, shuf_list);
		if iNumLeftToPlace > 0 and secondary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, -1, 0, 3, shuf_list);
		end
		if iNumLeftToPlace > 0 and tertiary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.4, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and quaternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and quinternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and sexternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, -1, 0, 2, shuf_list);
		end
		luxury_plot_lists = GenerateLuxuryPlotListsInRegionOrRange(self, region_number, 0, 0);
		shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, -1, 0, 3, shuf_list);
		if iNumLeftToPlace > 0 and secondary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, -1, 0, 3, shuf_list);
		end
		if iNumLeftToPlace > 0 and tertiary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.4, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and quaternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and quinternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 and sexternary > 0 then
			shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.5, -1, 0, 2, shuf_list);
		end
		if iNumLeftToPlace > 0 then
			print("-"); print("-"); print("Number of LuxuryID", res_ID, "left to place in Region#", region_number, "is", iNumLeftToPlace);
		end
	end

	-- Place Random Luxuries
	if self.iNumTypesRandom > 0 then
		print("* *"); print("* iNumTypesRandom = ", self.iNumTypesRandom); print("* *");
		-- This table governs targets for total number of luxuries placed in the world, not
		-- including the "extra types" of Luxuries placed at start locations. These targets
		-- are approximate. An additional random factor is added in based on number of civs.
		-- Any difference between regional and city state luxuries placed, and the target, is
		-- made up for with the number of randomly placed luxuries that get distributed.
		local world_size_data = self:GetWorldLuxuryTargetNumbers()
		local targetLuxForThisWorldSize = world_size_data[1];
		local loopTarget = world_size_data[2];
		local extraLux = Map.Rand(self.iNumCivs, "Luxury Resource Variance - Place Resources LUA");
		local iNumRandomLuxTarget = targetLuxForThisWorldSize + extraLux - self.totalLuxPlacedSoFar;
		
		if self.iNumTypesRandom * 3 > iNumRandomLuxTarget then
			print ("iNumRandomLuxTarget = " .. tostring(iNumRandomLuxTarget) .. ". Just putting in 3 of each random.");
		end
		
		local iNumRandomLuxPlaced, iNumThisLuxToPlace = 0, 0;
		-- This table weights the amount of random luxuries to place, with first-selected getting heavier weighting.
		local random_lux_ratios_table = {
		{1},
		{0.55, 0.45},
		{0.40, 0.33, 0.27},
		{0.35, 0.25, 0.25, 0.15},
		{0.25, 0.25, 0.20, 0.15, 0.15},
		{0.20, 0.20, 0.20, 0.15, 0.15, 0.10},
		{0.20, 0.20, 0.15, 0.15, 0.10, 0.10, 0.10},
		{0.20, 0.15, 0.15, 0.10, 0.10, 0.10, 0.10, 0.10} };

		for loop, res_ID in ipairs(self.resourceIDs_assigned_to_random) do
			local primary, secondary, tertiary, quaternary, quinternary, sexternary, luxury_plot_lists, current_list, iNumLeftToPlace;
			primary, secondary, tertiary, quaternary, quinternary, sexternary = self:GetIndicesForLuxuryType(res_ID);
			
			-- If calculated number of randoms is low, just place 3 of each
			if self.iNumTypesRandom * 3 > iNumRandomLuxTarget then
				iNumThisLuxToPlace = 3;
				
			elseif self.iNumTypesRandom > 8 then
				iNumThisLuxToPlace = math.max(3, math.ceil(iNumRandomLuxTarget / 10));
				
			else
				local lux_minimum = math.max(3, loopTarget - loop);
				local lux_share_of_remaining = math.ceil(iNumRandomLuxTarget * random_lux_ratios_table[self.iNumTypesRandom][loop]);
				iNumThisLuxToPlace = math.max(lux_minimum, lux_share_of_remaining);
			end
			-- Place this luxury type.
			current_list = self.global_luxury_plot_lists[primary];
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumThisLuxToPlace, 0.25, 2, 4, 6, current_list);
			if iNumLeftToPlace > 0 and secondary > 0 then
				current_list = self.global_luxury_plot_lists[secondary];
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.25, 2, 4, 6, current_list);
			end
			if iNumLeftToPlace > 0 and tertiary > 0 then
				current_list = self.global_luxury_plot_lists[tertiary];
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.25, 2, 4, 6, current_list);
			end
			if iNumLeftToPlace > 0 and quaternary > 0 then
				current_list = self.global_luxury_plot_lists[quaternary];
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, 2, 4, 6, current_list);
			end
			if iNumLeftToPlace > 0 and quinternary > 0 then
				current_list = self.global_luxury_plot_lists[quinternary];
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, 2, 4, 6, current_list);
			end
			if iNumLeftToPlace > 0 and sexternary > 0 then
				current_list = self.global_luxury_plot_lists[sexternary];
				iNumLeftToPlace = self:PlaceSpecificNumberOfResources(res_ID, 1, iNumLeftToPlace, 0.3, 2, 4, 6, current_list);
			end
			iNumRandomLuxPlaced = iNumRandomLuxPlaced + iNumThisLuxToPlace - iNumLeftToPlace;
			print("-"); print("Random Luxury Target Number:", iNumThisLuxToPlace);
			print("Random Luxury Target Placed:", iNumThisLuxToPlace - iNumLeftToPlace); print("-");
		end

		--[[
		print("-"); print("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
		print("+ Random Luxuries Target Number:", iNumRandomLuxTarget);
		print("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
		print("+ Random Luxuries Number Placed:", iNumRandomLuxPlaced);
		print("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+"); print("-");
		]]--

	end

	-- For Resource settings other than Sparse, add a second luxury type at start locations.
	-- This second type will be selected from Random types if possible, CS types if necessary, and other regions' types as a final fallback.
	if res ~= 1 then
		for region_number = 1, self.iNumCivs do
			--local attempt = 1
			--while attempt <= 6 do
				local x = self.startingPlots[region_number][1];
				local y = self.startingPlots[region_number][2];
				local use_this_ID;
				local candidate_types, iNumTypesAllowed = {}, 0;
				local allowed_luxuries = self:GetListOfAllowableLuxuriesAtCitySite(x, y, 2)
				print("-"); print("--- Eligible Types List for Second Luxury in Region#", region_number, "---");
				-- See if any Random types are eligible.
				for loop, res_ID in ipairs(self.resourceIDs_assigned_to_random) do
					if allowed_luxuries[res_ID] == true then
						print("- Found eligible luxury type:", res_ID);
						iNumTypesAllowed = iNumTypesAllowed + 1;
						table.insert(candidate_types, res_ID);
					end
				end
				-- Check to see if any Special Case luxuries are eligible. Disallow if Strategic Balance resource setting.
				if res ~= 5 and res ~= 6 then
					for loop, res_ID in ipairs(self.resourceIDs_assigned_to_special_case) do
						if allowed_luxuries[res_ID] == true then
							print("- Found eligible luxury type:", res_ID);
							iNumTypesAllowed = iNumTypesAllowed + 1;
							table.insert(candidate_types, res_ID);
						end
					end
				end

				local placed = false;
				if iNumTypesAllowed > 0 then
					print("- iNumTypesAllowed:", iNumTypesAllowed);
					local shuf_candidate_types = GetShuffledCopyOfTable(candidate_types)
					for attempt = 1, iNumTypesAllowed do
						use_this_ID = shuf_candidate_types[attempt];

						if use_this_ID ~= nil then -- Place this luxury type at this start.
							local primary, secondary, tertiary, quaternary, quinternary, sexternary, luxury_plot_lists, shuf_list;
							primary, secondary, tertiary, quaternary, quinternary, sexternary = self:GetIndicesForLuxuryType(use_this_ID);
							luxury_plot_lists = self:GenerateLuxuryPlotListsAtCitySite(x, y, 2, false)
							shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
							local iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							if iNumLeftToPlace > 0 and secondary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace > 0 and tertiary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace > 0 and quaternary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace > 0 and quinternary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace > 0 and sexternary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							luxury_plot_lists = self:GenerateLuxuryPlotListsAtCitySite(x, y, 3, false)
							if iNumLeftToPlace > 0 and primary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace > 0 and secondary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace > 0 and tertiary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace > 0 and quaternary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace > 0 and quinternary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace > 0 and sexternary > 0 then
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
								iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
							end
							if iNumLeftToPlace == 0 then
								print("-"); print("Placed Second Luxury type of ID#", use_this_ID, "for start located at Plot", x, y, " in Region#", region_number);
								placed = true;
								break;
							else
								print("-"); print("Failed to Place Second Luxury type of ID#", use_this_ID, "for start located at Plot", x, y, " in Region#", region_number);
							end
						end
					end
				end

				if not placed or iNumTypesAllowed == 0 then
					-- See if any City State types are eligible.
					for loop, res_ID in ipairs(self.resourceIDs_assigned_to_cs) do
						if allowed_luxuries[res_ID] == true then
							print("- Found eligible luxury type:", res_ID);
							iNumTypesAllowed = iNumTypesAllowed + 1;
							table.insert(candidate_types, res_ID);
						end
					end

					if iNumTypesAllowed > 0 then
						local shuf_candidate_types = GetShuffledCopyOfTable(candidate_types);
						for attempt = 1, iNumTypesAllowed do
							use_this_ID = shuf_candidate_types[attempt];

							if use_this_ID ~= nil then -- Place this luxury type at this start.
								local primary, secondary, tertiary, quaternary, quinternary, sexternary, luxury_plot_lists, shuf_list;
								primary, secondary, tertiary, quaternary, quinternary, sexternary = self:GetIndicesForLuxuryType(use_this_ID);
								luxury_plot_lists = self:GenerateLuxuryPlotListsAtCitySite(x, y, 2, false)
								shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
								local iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								if iNumLeftToPlace > 0 and secondary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace > 0 and tertiary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace > 0 and quaternary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace > 0 and quinternary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace > 0 and sexternary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								luxury_plot_lists = self:GenerateLuxuryPlotListsAtCitySite(x, y, 3, false)
								if iNumLeftToPlace > 0 and primary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace > 0 and secondary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace > 0 and tertiary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace > 0 and quaternary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace > 0 and quinternary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quinternary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace > 0 and sexternary > 0 then
									shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[sexternary])
									iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
								end
								if iNumLeftToPlace == 0 then
									print("-"); print("Placed Second Luxury type of ID#", use_this_ID, "for start located at Plot", x, y, " in Region#", region_number);
									placed = true;
									break;
								else
									print("-"); print("Failed to Place Second Luxury type of ID#", use_this_ID, "for start located at Plot", x, y, " in Region#", region_number);
								end
							end
						end
					end
				end

				--[[if iNumTypesAllowed > 0 then
					local diceroll = 1 + Map.Rand(iNumTypesAllowed, "Choosing second luxury type at a start location - LUA");
					use_this_ID = candidate_types[diceroll];
				else
					-- See if any City State types are eligible.
					for loop, res_ID in ipairs(self.resourceIDs_assigned_to_cs) do
						if allowed_luxuries[res_ID] == true then
							print("- Found eligible luxury type:", res_ID);
							iNumTypesAllowed = iNumTypesAllowed + 1;
							table.insert(candidate_types, res_ID);
						end
					end
					if iNumTypesAllowed > 0 then
						local diceroll = 1 + Map.Rand(iNumTypesAllowed, "Choosing second luxury type at a start location - LUA");
						use_this_ID = candidate_types[diceroll];
					else
						print("-"); print("Failed to place second Luxury type at start in Region#", region_number, "-- no eligible types!"); print("-");
					end
				end
				print("--- End of Eligible Types list for Second Luxury in Region#", region_number, "---");
				if use_this_ID ~= nil then -- Place this luxury type at this start.
					local primary, secondary, tertiary, quaternary, luxury_plot_lists, shuf_list;
					primary, secondary, tertiary, quaternary = self:GetIndicesForLuxuryType(use_this_ID);
					luxury_plot_lists = self:GenerateLuxuryPlotListsAtCitySite(x, y, 2, false)
					shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[primary])
					local iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
					if iNumLeftToPlace > 0 and secondary > 0 then
						shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[secondary])
						iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
					end
					if iNumLeftToPlace > 0 and tertiary > 0 then
						shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[tertiary])
						iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
					end
					if iNumLeftToPlace > 0 and quaternary > 0 then
						shuf_list = GetShuffledCopyOfTable(luxury_plot_lists[quaternary])
						iNumLeftToPlace = self:PlaceSpecificNumberOfResources(use_this_ID, 1, 1, 1, -1, 0, 0, shuf_list);
					end
					if iNumLeftToPlace == 0 then
						print("-"); print("Placed Second Luxury type of ID#", use_this_ID, "for start located at Plot", x, y, " in Region#", region_number);
						attempt = 7;
					else
						print("-"); print("Failed to Place Second Luxury type of ID#", use_this_ID, "for start located at Plot", x, y, " in Region#", region_number);
						attempt = attempt + 1;
					end
				end]]
			--end
		end
	end

	-- Handle Special Case Luxuries
	if self.iNumTypesSpecialCase > 0 then
		-- Add a special case function for each luxury to be handled as a special case.
		self:PlaceMarble()
	end

end
------------------------------------------------------------------------------
function AssignStartingPlots:PlaceMarble()
	local marble_already_placed = self.amounts_of_resources_placed[self.marble_ID + 1];
	local marble_target = math.ceil(self.iNumCivs * 0.75);
	local res = Map.GetCustomOption(5);
	if res == 1 then
		marble_target = math.ceil(self.iNumCivs * 0.5);
	elseif res == 3 then
		marble_target = math.ceil(self.iNumCivs * 0.9);
	end
	local iNumMarbleToPlace = math.max(2, marble_target - marble_already_placed);
	local iW, iH = Map.GetGridSize();
	local iNumLeftToPlace = iNumMarbleToPlace;
	local iNumPlots = table.maxn(self.marble_list);
	if iNumPlots < 1 then
		--print("No eligible plots available to place Marble!");
		return
	end
	-- Main loop
	for place_resource = 1, iNumMarbleToPlace do
		for loop, plotIndex in ipairs(self.marble_list) do
			if self.marbleData[plotIndex] == 0 and self.luxuryData[plotIndex] == 0 then
				local x = (plotIndex - 1) % iW;
				local y = (plotIndex - x - 1) / iW;
				local res_plot = Map.GetPlot(x, y)
				if res_plot:GetResourceType(-1) == -1 then -- Placing this resource in this plot.
					res_plot:SetResourceType(self.marble_ID, 1);
					self.amounts_of_resources_placed[self.marble_ID + 1] = self.amounts_of_resources_placed[self.marble_ID + 1] + 1;
					--print("-"); print("Placed Marble randomly at Plot", x, y);
					self.totalLuxPlacedSoFar = self.totalLuxPlacedSoFar + 1;
					iNumLeftToPlace = iNumLeftToPlace - 1;
					--print("Still need to place", iNumLeftToPlace, "more units of Marble.");
					self:PlaceResourceImpact(x, y, 2, 1)
					self:PlaceResourceImpact(x, y, 7, 6)
					break
				end
			end
		end
	end
	if iNumLeftToPlace > 0 then
		print("Failed to place", iNumLeftToPlace, "units of Marble.");
	end
end
------------------------------------------------------------------------------
function AssignStartingPlots:AttemptToPlaceHillsAtPlot(x, y, isStartLocation)
	-- This function will add hills at a specified plot, if able.
	--print("-"); print("Attempting to add Hills at: ", x, y);
	local plot = Map.GetPlot(x, y);
	if plot == nil then
		--print("Placement failed, plot was nil.");
		return false
	end
	if plot:GetResourceType(-1) ~= -1 then
		--print("Placement failed, plot had a resource.");
		return false
	end
	if IsNaturalWonder(plot) then
		return false;
	end
	local plotType = plot:GetPlotType()
	local featureType = plot:GetFeatureType();
	if plotType == PlotTypes.PLOT_OCEAN then
		--print("Placement failed, plot was water.");
		return false
	--elseif plot:IsRiverSide() then
		--print("Placement failed, plot was next to river.");
		--return false
	elseif featureType == FeatureTypes.FEATURE_FOREST and not isStartLocation then
		--print("Placement failed, plot had a forest already.");
		return false
	end	
	-- Change the plot type from flatlands to hills and clear any features.
	plot:SetPlotType(PlotTypes.PLOT_HILLS, false, true);
	plot:SetFeatureType(FeatureTypes.NO_FEATURE, -1);
	return true
end
------------------------------------------------------------------------------
function CreateRegionBoundaries(AssignStartingPlots)
	local iW, iH = Map.GetGridSize();
	for y = 0, iH - 1 do
		for x = 0, iW - 1 do
			local plot = Map.GetPlot(x, y);
			if plot:GetFeatureType() == FeatureTypes.FEATURE_ICE then
				plot:SetFeatureType(FeatureTypes.NO_FEATURE, -1);
			end
		end
	end
	for region_number = 1, table.maxn(AssignStartingPlots.regionData) do
		for y = 0, iH - 1 do
			for x = 0, iW - 1 do
				if (x == AssignStartingPlots.regionData[region_number][1] or x == AssignStartingPlots.regionData[region_number][1] + AssignStartingPlots.regionData[region_number][3] - 1) and
					y >= AssignStartingPlots.regionData[region_number][2] and y <= AssignStartingPlots.regionData[region_number][2] + AssignStartingPlots.regionData[region_number][4] - 1 or
					(y == AssignStartingPlots.regionData[region_number][2] or y == AssignStartingPlots.regionData[region_number][2] + AssignStartingPlots.regionData[region_number][4] - 1) and
					x >= AssignStartingPlots.regionData[region_number][1] and x <= AssignStartingPlots.regionData[region_number][1] + AssignStartingPlots.regionData[region_number][3] - 1 then
					local plot = Map.GetPlot(x, y);
					--plot:SetPlotType(PlotTypes.PLOT_MOUNTAIN, false, true);
					--plot:SetPlotType(PlotTypes.PLOT_OCEAN, false, true);
					plot:SetFeatureType(PlotTypes.FEATURE_ICE, -1);
				end
			end
		end
	end
end
------------------------------------------------------------------------------
function AssignStartingPlots:FindStart(region_number)
	-- This function attempts to choose a start position for a single region.
	-- This function returns two boolean flags, indicating the success level of the operation.
	local bSuccessFlag = false; -- Returns true when a start is placed, false when process fails.
	local bForcedPlacementFlag = false; -- Returns true if this region had no eligible starts and one was forced to occur.
	
	-- Obtain data needed to process this region.
	local iW, iH = Map.GetGridSize();
	local region_data_table = self.regionData[region_number];
	local iWestX = region_data_table[1];
	local iSouthY = region_data_table[2];
	local iWidth = region_data_table[3];
	local iHeight = region_data_table[4];
	local iAreaID = region_data_table[5];
	local iMembershipEastX = iWestX + iWidth - 1;
	local iMembershipNorthY = iSouthY + iHeight - 1;
	--
	local terrainCounts = self.regionTerrainCounts[region_number];
	--
	local region_type = self.regionTypes[region_number];
	-- Done setting up region data.
	-- Set up contingency.
	local fallback_plots = {};
	
	-- Establish scope of center bias.
	local fCenterWidth = (self.centerBias / 100) * iWidth;
	local iNonCenterWidth = math.floor((iWidth - fCenterWidth) / 2)
	local iCenterWidth = iWidth - (iNonCenterWidth * 2);
	local iCenterWestX = (iWestX + iNonCenterWidth) % iW; -- Modulo math to synch coordinate to actual map in case of world wrap.
	local iCenterTestWestX = (iWestX + iNonCenterWidth); -- "Test" values ignore world wrap for easier membership testing.
	local iCenterTestEastX = (iCenterWestX + iCenterWidth - 1);

	local fCenterHeight = (self.centerBias / 100) * iHeight;
	local iNonCenterHeight = math.floor((iHeight - fCenterHeight) / 2)
	local iCenterHeight = iHeight - (iNonCenterHeight * 2);
	local iCenterSouthY = (iSouthY + iNonCenterHeight) % iH;
	local iCenterTestSouthY = (iSouthY + iNonCenterHeight);
	local iCenterTestNorthY = (iCenterTestSouthY + iCenterHeight - 1);

	-- Establish scope of "middle donut", outside the center but inside the outer.
	local fMiddleWidth = (self.middleBias / 100) * iWidth;
	local iOuterWidth = math.floor((iWidth - fMiddleWidth) / 2)
	local iMiddleWidth = iWidth - (iOuterWidth * 2);
	local iMiddleWestX = (iWestX + iOuterWidth) % iW;
	local iMiddleTestWestX = (iWestX + iOuterWidth);
	local iMiddleTestEastX = (iMiddleTestWestX + iMiddleWidth - 1);

	local fMiddleHeight = (self.middleBias / 100) * iHeight;
	local iOuterHeight = math.floor((iHeight - fMiddleHeight) / 2)
	local iMiddleHeight = iHeight - (iOuterHeight * 2);
	local iMiddleSouthY = (iSouthY + iOuterHeight) % iH;
	local iMiddleTestSouthY = (iSouthY + iOuterHeight);
	local iMiddleTestNorthY = (iMiddleTestSouthY + iMiddleHeight - 1); 

	-- Assemble candidates lists.
	local two_plots_from_ocean = {};
	local center_candidates = {};
	local center_river = {};
	local center_coastal = {};
	local center_inland_dry = {};
	local middle_candidates = {};
	local middle_river = {};
	local middle_coastal = {};
	local middle_inland_dry = {};
	local outer_plots = {};
	
	-- Identify candidate plots.
	for region_y = 0, iHeight - 1 do -- When handling global plot indices, process Y first.
		for region_x = 0, iWidth - 1 do
			local x = (region_x + iWestX) % iW; -- Actual coords, adjusted for world wrap, if any.
			local y = (region_y + iSouthY) % iH; --
			local plotIndex = y * iW + x + 1;
			local isTooCloseToOthers = false;
			for region_num = 1, table.maxn(self.regionData) do
				if region_number ~= region_num and self.startingPlots[region_num] ~= nil then
					if Map.PlotDistance(x, y, self.startingPlots[region_num][1], self.startingPlots[region_num][2]) < 11 then
						isTooCloseToOthers = true;
					end
				end
			end
			local plot = Map.GetPlot(x, y);
			local plotType = plot:GetPlotType()
			if plotType == PlotTypes.PLOT_HILLS or plotType == PlotTypes.PLOT_LAND and not isTooCloseToOthers then -- Could host a city.
				-- Check if plot is two away from salt water.
				if self.plotDataIsNextToCoast[plotIndex] == true then
					table.insert(two_plots_from_ocean, plotIndex);
				else
					local area_of_plot = plot:GetArea();
					if area_of_plot == iAreaID or iAreaID == -1 then -- This plot is a member, so it goes on at least one candidate list.
						--
						-- Test whether plot is in center bias, middle donut, or outer donut.
						--
						local test_x = region_x + iWestX; -- "Test" coords, ignoring any world wrap and
						local test_y = region_y + iSouthY; -- reaching in to virtual space if necessary.
						if (test_x >= iCenterTestWestX and test_x <= iCenterTestEastX) and 
						   (test_y >= iCenterTestSouthY and test_y <= iCenterTestNorthY) then -- Center Bias.
							table.insert(center_candidates, plotIndex);
							if plot:IsRiverSide() then
								table.insert(center_river, plotIndex);
							elseif plot:IsFreshWater() or self.plotDataIsCoastal[plotIndex] == true then
								table.insert(center_coastal, plotIndex);
							else
								table.insert(center_inland_dry, plotIndex);
							end
						elseif (test_x >= iMiddleTestWestX and test_x <= iMiddleTestEastX) and 
						       (test_y >= iMiddleTestSouthY and test_y <= iMiddleTestNorthY) then
							table.insert(middle_candidates, plotIndex);
							if plot:IsRiverSide() then
								table.insert(middle_river, plotIndex);
							elseif plot:IsFreshWater() or self.plotDataIsCoastal[plotIndex] == true then
								table.insert(middle_coastal, plotIndex);
							else
								table.insert(middle_inland_dry, plotIndex);
							end
						else
							table.insert(outer_plots, plotIndex);
						end
					end
				end
			end
		end
	end

	-- Check how many plots landed on each list.
	local iNumDisqualified = table.maxn(two_plots_from_ocean);
	local iNumCenter = table.maxn(center_candidates);
	local iNumCenterRiver = table.maxn(center_river);
	local iNumCenterCoastLake = table.maxn(center_coastal);
	local iNumCenterInlandDry = table.maxn(center_inland_dry);
	local iNumMiddle = table.maxn(middle_candidates);
	local iNumMiddleRiver = table.maxn(middle_river);
	local iNumMiddleCoastLake = table.maxn(middle_coastal);
	local iNumMiddleInlandDry = table.maxn(middle_inland_dry);
	local iNumOuter = table.maxn(outer_plots);
	
	--[[ Debug printout.
	print("-");
	print("--- Number of Candidate Plots in Region #", region_number, " - Region Type:", region_type, " ---");
	print("-");
	print("Candidates in Center Bias area: ", iNumCenter);
	print("Which are next to river: ", iNumCenterRiver);
	print("Which are next to lake or sea: ", iNumCenterCoastLake);
	print("Which are inland and dry: ", iNumCenterInlandDry);
	print("-");
	print("Candidates in Middle Donut area: ", iNumMiddle);
	print("Which are next to river: ", iNumMiddleRiver);
	print("Which are next to lake or sea: ", iNumMiddleCoastLake);
	print("Which are inland and dry: ", iNumMiddleInlandDry);
	print("-");
	print("Candidate Plots in Outer area: ", iNumOuter);
	print("-");
	print("Disqualified, two plots away from salt water: ", iNumDisqualified);
	print("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
	]]--
	
	-- Process lists of candidate plots.
	if iNumCenter + iNumMiddle > 0 then
		local candidate_lists = {};
		if iNumCenterRiver > 0 then -- Process center bias river plots.
			table.insert(candidate_lists, center_river);
		end
		if iNumCenterCoastLake > 0 then -- Process center bias lake or coastal plots.
			table.insert(candidate_lists, center_coastal);
		end
		if iNumCenterInlandDry > 0 then -- Process center bias inland dry plots.
			table.insert(candidate_lists, center_inland_dry);
		end
		if iNumMiddleRiver > 0 then -- Process middle donut river plots.
			table.insert(candidate_lists, middle_river);
		end
		if iNumMiddleCoastLake > 0 then -- Process middle donut lake or coastal plots.
			table.insert(candidate_lists, middle_coastal);
		end
		if iNumMiddleInlandDry > 0 then -- Process middle donut inland dry plots.
			table.insert(candidate_lists, middle_inland_dry);
		end
		--
		for loop, plot_list in ipairs(candidate_lists) do -- Up to six plot lists, processed by priority.
			local election_returns = self:IterateThroughCandidatePlotList(plot_list, region_type)
			-- If any candidates are eligible, choose one.
			local found_eligible = election_returns[1];
			if found_eligible then
				local bestPlotScore = election_returns[2]; 
				local bestPlotIndex = election_returns[3];
				local x = (bestPlotIndex - 1) % iW;
				local y = (bestPlotIndex - x - 1) / iW;
				self.startingPlots[region_number] = {x, y, bestPlotScore};
				self:PlaceImpactAndRipples(x, y)
				return true, false
			end
			-- If none eligible, check for fallback plot.
			local found_fallback = election_returns[4];
			if found_fallback then
				local bestFallbackScore = election_returns[5];
				local bestFallbackIndex = election_returns[6];
				local x = (bestFallbackIndex - 1) % iW;
				local y = (bestFallbackIndex - x - 1) / iW;
				table.insert(fallback_plots, {x, y, bestFallbackScore});
			end
		end
	end
	-- Reaching this point means no eligible sites in center bias or middle donut subregions!
	
	-- Process candidates from Outer subregion, if any.
	if iNumOuter > 0 then
		local outer_eligible_list = {};
		local found_eligible = false;
		local found_fallback = false;
		local bestFallbackScore = -50;
		local bestFallbackIndex;
		-- Process list of candidate plots.
		for loop, plotIndex in ipairs(outer_plots) do
			local score, meets_minimums = self:EvaluateCandidatePlot(plotIndex, region_type)
			-- Test current plot against best known plot.
			if meets_minimums == true then
				found_eligible = true;
				table.insert(outer_eligible_list, plotIndex);
			else
				found_fallback = true;
				if score > bestFallbackScore then
					bestFallbackScore = score;
					bestFallbackIndex = plotIndex;
				end
			end
		end
		if found_eligible then -- Iterate through eligible plots and choose the one closest to the center of the region.
			local closestPlot;
			local closestDistance = math.max(iW, iH);
			local bullseyeX = iWestX + (iWidth / 2);
			if bullseyeX < iWestX then -- wrapped around: un-wrap it for test purposes.
				bullseyeX = bullseyeX + iW;
			end
			local bullseyeY = iSouthY + (iHeight / 2);
			if bullseyeY < iSouthY then -- wrapped around: un-wrap it for test purposes.
				bullseyeY = bullseyeY + iH;
			end
			if bullseyeY / 2 ~= math.floor(bullseyeY / 2) then -- Y coord is odd, add .5 to X coord for hex-shift.
				bullseyeX = bullseyeX + 0.5;
			end
			
			for loop, plotIndex in ipairs(outer_eligible_list) do
				local x = (plotIndex - 1) % iW;
				local y = (plotIndex - x - 1) / iW;
				local adjusted_x = x;
				local adjusted_y = y;
				if y / 2 ~= math.floor(y / 2) then -- Y coord is odd, add .5 to X coord for hex-shift.
					adjusted_x = x + 0.5;
				end
				
				if x < iWestX then -- wrapped around: un-wrap it for test purposes.
					adjusted_x = adjusted_x + iW;
				end
				if y < iSouthY then -- wrapped around: un-wrap it for test purposes.
					adjusted_y = y + iH;
				end
				local fDistance = math.sqrt( (adjusted_x - bullseyeX)^2 + (adjusted_y - bullseyeY)^2 );
				if fDistance < closestDistance then -- Found new "closer" plot.
					closestPlot = plotIndex;
					closestDistance = fDistance;
				end
			end
			-- Assign the closest eligible plot as the start point.
			local x = (closestPlot - 1) % iW;
			local y = (closestPlot - x - 1) / iW;
			-- Re-get plot score for inclusion in start plot data.
			local score, meets_minimums = self:EvaluateCandidatePlot(closestPlot, region_type)
			-- Assign this plot as the start for this region.
			self.startingPlots[region_number] = {x, y, score};
			self:PlaceImpactAndRipples(x, y)
			return true, false
		end
		-- Add the fallback plot (best scored plot) from the Outer region to the fallback list.
		if found_fallback then
			local x = (bestFallbackIndex - 1) % iW;
			local y = (bestFallbackIndex - x - 1) / iW;
			table.insert(fallback_plots, {x, y, bestFallbackScore});
		end
	end
	-- Reaching here means no plot in the entire region met the minimum standards for selection.
	
	-- The fallback plot contains the best-scored plots from each test area in this region.
	-- We will compare all the fallback plots and choose the best to be the start plot.
	local iNumFallbacks = table.maxn(fallback_plots);
	if iNumFallbacks > 0 then
		local best_fallback_score = 0
		local best_fallback_x;
		local best_fallback_y;
		for loop, plotData in ipairs(fallback_plots) do
			local score = plotData[3];
			if score > best_fallback_score then
				best_fallback_score = score;
				best_fallback_x = plotData[1];
				best_fallback_y = plotData[2];
			end
		end
		-- Assign the start for this region.
		self.startingPlots[region_number] = {best_fallback_x, best_fallback_y, best_fallback_score};
		self:PlaceImpactAndRipples(best_fallback_x, best_fallback_y)
		bSuccessFlag = true;
	else
		-- This region cannot have a start and something has gone way wrong.
		-- We'll force a one tile grass island in the SW corner of the region and put the start there.
		local forcePlot = Map.GetPlot(iWestX, iSouthY);
		bSuccessFlag = true;
		bForcedPlacementFlag = true;
		forcePlot:SetPlotType(PlotTypes.PLOT_LAND, false, true);
		forcePlot:SetTerrainType(TerrainTypes.TERRAIN_GRASS, false, true);
		forcePlot:SetFeatureType(FeatureTypes.NO_FEATURE, -1);
		self.startingPlots[region_number] = {iWestX, iSouthY, 0};
		self:PlaceImpactAndRipples(iWestX, iSouthY)
	end

	return bSuccessFlag, bForcedPlacementFlag
end
------------------------------------------------------------------------------
function GenerateLuxuryPlotListsAtCapitalSite(AssignStartingPlots, x, y, innerradius, outerradius)
	-- bRemoveFeatureIce is piggybacked on to this function to reduce redundant code.
	-- If ice is being removed from around a plot, ONLY that will occur. If both ice 
	-- removal and plot list generation are desired, call this function twice.
	--print("GenerateLuxuryPlotListsAtCitySite called. RemoveIce:", bRemoveFeatureIce, "Plot:", x, y, "Radius:", radius);
	local iW, iH = Map.GetGridSize();
	local wrapX = Map:IsWrapX();
	local wrapY = Map:IsWrapY();
	local odd = AssignStartingPlots.firstRingYIsOdd;
	local even = AssignStartingPlots.firstRingYIsEven;
	local nextX, nextY, plot_adjustments;

	local region_coast, region_marsh, region_flood_plains, region_tundra_flat_including_forests = {}, {}, {}, {};
	local region_hills_open, region_hills_covered, region_hills_jungle, region_hills_forest = {}, {}, {}, {};
	local region_desert_flat_no_feature, region_plains_flat_no_feature, region_jungle_flat = {}, {}, {};
	local region_forest_flat, region_forest_flat_but_not_tundra = {}, {};
	local region_dry_grass_flat_no_feature, region_fresh_water_grass_flat_no_feature = {}, {};

	--if innerradius > 0 and innerradius <= outerradius and outerradius < 8 then
	--if radius > 0 and radius < 6 then
		for plotIndex = 0, iW*iH do
			local searchX = (plotIndex - 1) % iW;
			local searchY = (plotIndex - searchX - 1) / iW;
			if Map.PlotDistance(x, y, searchX, searchY) >= innerradius and Map.PlotDistance(x, y, searchX, searchY) <= outerradius then
				-- We've arrived at the correct x and y for the current plot.
				local plot = Map.GetPlot(searchX, searchY);
				
				local plotType = plot:GetPlotType()
				local terrainType = plot:GetTerrainType()
				local featureType = plot:GetFeatureType()
				--local plotIndex = realY * iW + realX + 1;
				
				if plotType == PlotTypes.PLOT_OCEAN then
					if terrainType == TerrainTypes.TERRAIN_COAST then
						if plot:IsLake() == false then
							if featureType ~= AssignStartingPlots.feature_atoll and featureType ~= FeatureTypes.FEATURE_ICE then
								table.insert(region_coast, plotIndex);
							end
						end
					end
				elseif plotType == PlotTypes.PLOT_HILLS and terrainType ~= TerrainTypes.TERRAIN_SNOW then
					if featureType == FeatureTypes.NO_FEATURE then
						table.insert(region_hills_open, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_JUNGLE then		
						table.insert(region_hills_jungle, plotIndex);
						table.insert(region_hills_covered, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_FOREST then		
						table.insert(region_hills_forest, plotIndex);
						table.insert(region_hills_covered, plotIndex);
					end
				elseif plotType == PlotTypes.PLOT_LAND then
					if featureType == FeatureTypes.NO_FEATURE then
						if terrainType == TerrainTypes.TERRAIN_TUNDRA then
							table.insert(region_tundra_flat_including_forests, plotIndex);
						elseif terrainType == TerrainTypes.TERRAIN_DESERT then
							table.insert(region_desert_flat_no_feature, plotIndex);
						elseif terrainType == TerrainTypes.TERRAIN_PLAINS then
							table.insert(region_plains_flat_no_feature, plotIndex);
						elseif terrainType == TerrainTypes.TERRAIN_GRASS then
							if plot:IsFreshWater() then
								table.insert(region_fresh_water_grass_flat_no_feature, plotIndex);
							else
								table.insert(region_dry_grass_flat_no_feature, plotIndex);
							end
						end
					elseif featureType == FeatureTypes.FEATURE_MARSH then		
						table.insert(region_marsh, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then		
						table.insert(region_flood_plains, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_JUNGLE then		
						table.insert(region_jungle_flat, plotIndex);
					elseif featureType == FeatureTypes.FEATURE_FOREST then		
						table.insert(region_forest_flat, plotIndex);
						if terrainType == TerrainTypes.TERRAIN_TUNDRA then
							table.insert(region_tundra_flat_including_forests, plotIndex);
						else
							table.insert(region_forest_flat_but_not_tundra, plotIndex);
						end
					end
				end
			end
		end
	--end
			
	local results_table = {
	region_coast, -- (Coast next to land)		-- 1
	region_marsh,								-- 2
	region_flood_plains,						-- 3
	region_hills_open,							-- 4
	region_hills_covered,						-- 5
	region_hills_jungle,						-- 6
	region_hills_forest,						-- 7
	region_jungle_flat,							-- 8
	region_forest_flat,							-- 9
	region_desert_flat_no_feature,				-- 10
	region_plains_flat_no_feature,				-- 11			
	region_dry_grass_flat_no_feature,			-- 12
	region_fresh_water_grass_flat_no_feature,	-- 13
	region_tundra_flat_including_forests,		-- 14
	region_forest_flat_but_not_tundra			-- 15
	};
	return results_table
end
------------------------------------------------------------------------------
function IsNaturalWonder(plot)
	for row in GameInfo.Features() do
		if (row.NaturalWonder == true) then
			if (plot:GetFeatureType() == row.ID) then
				return true;
			end
		end
	end

	return false;
end
------------------------------------------------------------------------------
function AssignStartingPlots:AssignLuxuryRoles()
	-- Each region gets an individual Luxury type assigned to it.
	-- Each Luxury type can be assigned to no more than three regions.
	-- No more than nine total Luxury types will be assigned to regions.
	-- Between two and four Luxury types will be assigned to City States.
	-- Remaining Luxury types will be distributed at random or left out.
	--
	-- Luxury roles must be assigned before City States can be placed.
	-- This is because civs who are forced to share their luxury type with other 
	-- civs may get extra city states placed in their region to compensate.

	self:SortRegionsByType() -- creates self.regions_sorted_by_type, which will be expanded to store all data regarding regional luxuries.

	-- Assign a luxury to each region.
	for index, region_info in ipairs(self.regions_sorted_by_type) do
		local region_number = region_info[1];
		local resource_ID = self:AssignLuxuryToRegion(region_number)
		self.regions_sorted_by_type[index][2] = resource_ID; -- This line applies the assignment.
		self.region_luxury_assignment[region_number] = resource_ID;
		self.luxury_assignment_count[resource_ID] = self.luxury_assignment_count[resource_ID] + 1; -- Track assignments
		--
		print("-"); print("Region#", region_number, " of type ", self.regionTypes[region_number], " has been assigned Luxury ID#", resource_ID);
		--
		local already_assigned = TestMembership(self.resourceIDs_assigned_to_regions, resource_ID)
		if not already_assigned then
			table.insert(self.resourceIDs_assigned_to_regions, resource_ID);
			self.iNumTypesAssignedToRegions = self.iNumTypesAssignedToRegions + 1;
			self.iNumTypesUnassigned = self.iNumTypesUnassigned - 1;
		end
	end
	
	-- Assign three of the remaining types to be exclusive to City States.
	-- Build options list.
	local iNumAvailableTypes = 0;
	local resource_IDs, resource_weights = {}, {};
	for index, resource_options in ipairs(self.luxury_city_state_weights) do
		local res_ID = resource_options[1];
		local test = TestMembership(self.resourceIDs_assigned_to_regions, res_ID)
		if test == false then
			table.insert(resource_IDs, res_ID);
			table.insert(resource_weights, resource_options[2]);
			iNumAvailableTypes = iNumAvailableTypes + 1;
		else
			print("Luxury ID#", res_ID, "rejected by City States as already belonging to Regions.");
		end
	end
	if iNumAvailableTypes < 3 then
		print("---------------------------------------------------------------------------------------");
		print("- Luxuries have been modified in ways disruptive to the City State Assignment Process -");
		print("---------------------------------------------------------------------------------------");
	end
	-- Choose luxuries.
	local cs_lux = 0
	for attempt = 1, 20 do
		local totalWeight = 0;
		local res_threshold = {};
		for i, this_weight in ipairs(resource_weights) do
			totalWeight = totalWeight + this_weight;
		end
		local accumulatedWeight = 0;
		for index, weight in ipairs(resource_weights) do
			local threshold = (weight + accumulatedWeight) * 10000 / totalWeight;
			table.insert(res_threshold, threshold);
			accumulatedWeight = accumulatedWeight + resource_weights[index];
		end
		local use_this_ID;
		local diceroll = Map.Rand(10000, "Choose resource type - City State Luxuries - Lua");
		for index, threshold in ipairs(res_threshold) do
			if diceroll < threshold then -- Choose this resource type.
				use_this_ID = resource_IDs[index];
				table.insert(self.resourceIDs_assigned_to_cs, use_this_ID);
				table.remove(resource_IDs, index);
				table.remove(resource_weights, index);
				self.iNumTypesUnassigned = self.iNumTypesUnassigned - 1;
				cs_lux = cs_lux + 1
				--print("-"); print("City States have been assigned Luxury ID#", use_this_ID);
				break
			end
		end
		if cs_lux > 3 then
			break
		end
	end
	
	-- Assign Marble to special casing.
	table.insert(self.resourceIDs_assigned_to_special_case, self.marble_ID);
	self.iNumTypesUnassigned = self.iNumTypesUnassigned - 1;

	-- Assign appropriate amount to be Disabled, then assign the rest to be Random.
	local maxToDisable = self:GetDisabledLuxuriesTargetNumber()
	self.iNumTypesDisabled = math.min(self.iNumTypesUnassigned, maxToDisable);
	self.iNumTypesRandom = self.iNumTypesUnassigned - self.iNumTypesDisabled;
	local remaining_resource_IDs = {};
	for index, resource_options in ipairs(self.luxury_fallback_weights) do
		local res_ID = resource_options[1];
		local test1 = TestMembership(self.resourceIDs_assigned_to_regions, res_ID)
		local test2 = TestMembership(self.resourceIDs_assigned_to_cs, res_ID)
		if test1 == false and test2 == false then
			table.insert(remaining_resource_IDs, res_ID);
		end
	end
	local randomized_version = GetShuffledCopyOfTable(remaining_resource_IDs)
	local countdown = math.min(self.iNumTypesUnassigned, maxToDisable);
	for loop, resID in ipairs(randomized_version) do
		if countdown > 0 then
			table.insert(self.resourceIDs_not_being_used, resID);
			countdown = countdown - 1;
		else
			table.insert(self.resourceIDs_assigned_to_random, resID);
		end
	end
	
	-- Debug printout of luxury assignments.
	print("--- Luxury Assignment Table ---");
	print("-"); print("- - Assigned to Regions - -");
	for index, data in ipairs(self.regions_sorted_by_type) do
		print("Region#", data[1], "has Luxury type", data[2]);
	end
	print("-"); print("- - Assigned to City States - -");
	for index, type in ipairs(self.resourceIDs_assigned_to_cs) do
		print("Luxury type", type);
	end
	print("-"); print("- - Assigned to Random - -");
	for index, type in ipairs(self.resourceIDs_assigned_to_random) do
		print("Luxury type", type);
	end
	print("-"); print("- - Luxuries handled via Special Case - -");
	for index, type in ipairs(self.resourceIDs_assigned_to_special_case) do
		print("Luxury type", type);
	end
	print("-"); print("- - Disabled - -");
	for index, type in ipairs(self.resourceIDs_not_being_used) do
		print("Luxury type", type);
	end
	print("- - - - - - - - - - - - - - - -");
	--	
end
------------------------------------------------------------------------------
function AssignStartingPlots:AttemptToPlaceNaturalWonder(wonder_number, row_number)
	-- Attempts to place a specific natural wonder. The "wonder_number" is a Lua index while "row_number" is an XML index.
	local iW, iH = Map.GetGridSize();
	local feature_type_to_place;
	for thisFeature in GameInfo.Features() do
		if thisFeature.Type == self.wonder_list[wonder_number] then
			feature_type_to_place = thisFeature.ID;
			break
		end
	end
	local temp_table = self.eligibility_lists[wonder_number];
	local candidate_plot_list = GetShuffledCopyOfTable(temp_table)
	for loop, plotIndex in ipairs(candidate_plot_list) do
		if self.naturalWondersData[plotIndex] == 0 then -- No collision with civ start or other NW, so place wonder here!
			local x = (plotIndex - 1) % iW;
			local y = (plotIndex - x - 1) / iW;
			local plot = Map.GetPlot(x, y);
			-- If called for, force the local terrain to conform to what the wonder needs.
			local method_number = GameInfo.Natural_Wonder_Placement[row_number].TileChangesMethodNumber;
			if method_number ~= -1 then
				-- Custom method for tile changes needed by this wonder.
				NWCustomPlacement(x, y, row_number, method_number, self)
			else
				-- Check the XML data for any standard type tile changes, execute any that are indicated.
				if GameInfo.Natural_Wonder_Placement[row_number].ChangeCoreTileToMountain == true then
					if not plot:IsMountain() then
						plot:SetPlotType(PlotTypes.PLOT_MOUNTAIN, false, false);
					end
				elseif GameInfo.Natural_Wonder_Placement[row_number].ChangeCoreTileToFlatland == true then
					if plot:GetPlotType() ~= PlotTypes.PLOT_LAND then
						plot:SetPlotType(PlotTypes.PLOT_LAND, false, false);
					end
				end
				if GameInfo.Natural_Wonder_Placement[row_number].ChangeCoreTileTerrainToGrass == true then
					if plot:GetTerrainType() ~= TerrainTypes.TERRAIN_GRASS then
						plot:SetTerrainType(TerrainTypes.TERRAIN_GRASS, false, false);
					end
				elseif GameInfo.Natural_Wonder_Placement[row_number].ChangeCoreTileTerrainToPlains == true then
					if plot:GetTerrainType() ~= TerrainTypes.TERRAIN_PLAINS then
						plot:SetTerrainType(TerrainTypes.TERRAIN_PLAINS, false, false);
					end
				end
				if GameInfo.Natural_Wonder_Placement[row_number].SetAdjacentTilesToShallowWater == true then
					for loop, direction in ipairs(self.direction_types) do
						local adjPlot = Map.PlotDirection(x, y, direction)
						if adjPlot:GetTerrainType() ~= TerrainTypes.TERRAIN_COAST then
							adjPlot:SetTerrainType(TerrainTypes.TERRAIN_COAST, false, false)
						end
					end
				end
			end
			-- Now place this wonder and record the placement.
			plot:SetFeatureType(feature_type_to_place)
			table.insert(self.placed_natural_wonder, wonder_number);
			self:PlaceResourceImpact(x, y, 6, math.floor(iH / 5))	-- Natural Wonders layer
			self:PlaceResourceImpact(x, y, 1, 1)					-- Strategic layer
			self:PlaceResourceImpact(x, y, 2, 1)					-- Luxury layer
			self:PlaceResourceImpact(x, y, 3, 1)					-- Bonus layer
			self:PlaceResourceImpact(x, y, 5, 1)					-- City State layer
			self:PlaceResourceImpact(x, y, 7, 1)					-- Marble layer
			local plotIndex = y * iW + x + 1;
			self.playerCollisionData[plotIndex] = true;				-- Record exact plot of wonder in the collision list.
			--
			--print("- Placed ".. self.wonder_list[wonder_number].. " in Plot", x, y);
			--
			return true
		end
	end
	-- If reached here, this wonder was unable to be placed because all candidates are too close to an already-placed NW.
	return false
end
------------------------------------------------------------------------------
function NWCustomPlacement(x, y, row_number, method_number, AssignStartingPlots)
	local iW, iH = Map.GetGridSize();
	if method_number == 1 then
		-- This method handles tile changes for the Great Barrier Reef.
		local plot = Map.GetPlot(x, y);
		if not plot:IsWater() then
			plot:SetPlotType(PlotTypes.PLOT_OCEAN, false, false);
		end
		if plot:GetTerrainType() ~= TerrainTypes.TERRAIN_COAST then
			plot:SetTerrainType(TerrainTypes.TERRAIN_COAST, false, false)
		end
		-- The Reef has a longer shape and demands unique handling. Process the extra plots.
		local extra_direction_types = {
			DirectionTypes.DIRECTION_EAST,
			DirectionTypes.DIRECTION_SOUTHEAST,
			DirectionTypes.DIRECTION_SOUTHWEST};
		local SEPlot = Map.PlotDirection(x, y, DirectionTypes.DIRECTION_SOUTHEAST)
		if not SEPlot:IsWater() then
			SEPlot:SetPlotType(PlotTypes.PLOT_OCEAN, false, false);
		end
		if SEPlot:GetTerrainType() ~= TerrainTypes.TERRAIN_COAST then
			SEPlot:SetTerrainType(TerrainTypes.TERRAIN_COAST, false, false)
		end
		if SEPlot:GetFeatureType() ~= FeatureTypes.NO_FEATURE then
			SEPlot:SetFeatureType(FeatureTypes.NO_FEATURE, -1)
		end
		if SEPlot:GetResourceType(-1) ~= -1 then
			AssignStartingPlots.amounts_of_resources_placed[SEPlot:GetResourceType(-1) + 1] = AssignStartingPlots.amounts_of_resources_placed[SEPlot:GetResourceType(-1) + 1] - 1;
			SEPlot:SetResourceType(SEPlot:GetResourceType(-1), 1);
		end
		local southeastX = SEPlot:GetX();
		local southeastY = SEPlot:GetY();
		for loop, direction in ipairs(extra_direction_types) do -- The three plots extending another plot past the SE plot.
			local adjPlot = Map.PlotDirection(southeastX, southeastY, direction)
			if adjPlot:GetTerrainType() ~= TerrainTypes.TERRAIN_COAST then
				adjPlot:SetTerrainType(TerrainTypes.TERRAIN_COAST, false, false)
			end
			local adjX = adjPlot:GetX();
			local adjY = adjPlot:GetY();
			local adjPlotIndex = adjY * iW + adjX + 1;
		end
		-- Now check the rest of the adjacent plots.
		local direction_types = { -- Not checking to southeast.
			DirectionTypes.DIRECTION_NORTHEAST,
			DirectionTypes.DIRECTION_EAST,
			DirectionTypes.DIRECTION_SOUTHWEST,
			DirectionTypes.DIRECTION_WEST,
			DirectionTypes.DIRECTION_NORTHWEST
			};
		for loop, direction in ipairs(direction_types) do
			local adjPlot = Map.PlotDirection(x, y, direction)
			if adjPlot:GetTerrainType() ~= TerrainTypes.TERRAIN_COAST then
				adjPlot:SetTerrainType(TerrainTypes.TERRAIN_COAST, false, false)
			end
		end
		-- Now place the Reef's second wonder plot. (The core method will place the main plot).
		local feature_type_to_place;
		for thisFeature in GameInfo.Features() do
			if thisFeature.Type == "FEATURE_REEF" then
				feature_type_to_place = thisFeature.ID;
				break
			end
		end
		SEPlot:SetFeatureType(feature_type_to_place);
	
	elseif method_number == 2 then
		-- This method handles tile changes for the Rock of Gibraltar.
		local plot = Map.GetPlot(x, y);
		plot:SetPlotType(PlotTypes.PLOT_LAND, false, false);
		plot:SetTerrainType(TerrainTypes.TERRAIN_GRASS, false, false)
		local direction_types = {
			DirectionTypes.DIRECTION_NORTHEAST,
			DirectionTypes.DIRECTION_EAST,
			DirectionTypes.DIRECTION_SOUTHEAST,
			DirectionTypes.DIRECTION_SOUTHWEST,
			DirectionTypes.DIRECTION_WEST,
			DirectionTypes.DIRECTION_NORTHWEST};
		for loop, direction in ipairs(direction_types) do
			local adjPlot = Map.PlotDirection(x, y, direction)
			if adjPlot:GetPlotType() == PlotTypes.PLOT_OCEAN then
				if adjPlot:GetTerrainType() ~= TerrainTypes.TERRAIN_COAST then
					adjPlot:SetTerrainType(TerrainTypes.TERRAIN_COAST, false, false)
				end
			else
				if adjPlot:GetPlotType() ~= PlotTypes.PLOT_MOUNTAIN then
					adjPlot:SetPlotType(PlotTypes.PLOT_MOUNTAIN, false, false);
				end
				if adjPlot:GetFeatureType() ~= FeatureTypes.NO_FEATURE then
					adjPlot:SetFeatureType(FeatureTypes.NO_FEATURE, -1)
				end
				if adjPlot:GetResourceType(-1) ~= -1 then
					AssignStartingPlots.amounts_of_resources_placed[adjPlot:GetResourceType(-1) + 1] = AssignStartingPlots.amounts_of_resources_placed[adjPlot:GetResourceType(-1) + 1] - 1;
					adjPlot:SetResourceType(adjPlot:GetResourceType(-1), 1);
				end
			end
		end

	-- These method numbers are not needed for the core game's natural wonders;
	-- however, this is where a modder could insert more custom methods, as needed.
	-- Any new methods added must be called from Natural_Wonder_Placement in Civ5Features.xml - Sirian, June 2011
	--
	--elseif method_number == 3 then
	--elseif method_number == 4 then
	--elseif method_number == 5 then

	end
end
------------------------------------------------------------------------------
function StartPlotSystem()
	-- Get Resources setting input by user.
	local res = Map.GetCustomOption(5)
	if res == 7 then
		res = 1 + Map.Rand(3, "Random Resources Option - Lua");
	end

	print("Creating start plot database.");
	local start_plot_database = AssignStartingPlots.Create()
	
	print("Dividing the map in to Regions.");
	-- Regional Division Method 1: Biggest Landmass
	local args = {
		method = 1,
		resources = res,
		};
	start_plot_database:GenerateRegions(args)

	print("Choosing start locations for civilizations.");
	start_plot_database:ChooseLocations()
	
	print("Balancing areas around central plots.");
	start_plot_database:BalanceAndAssign()

	print("Placing Natural Wonders.");
	start_plot_database:PlaceNaturalWonders()

	print("Placing Resources and City States.");
	start_plot_database:PlaceResourcesAndCityStates()

	NormalizePlotAreas(start_plot_database);

	--CreateRegionBoundaries(start_plot_database);

end
------------------------------------------------------------------------------
