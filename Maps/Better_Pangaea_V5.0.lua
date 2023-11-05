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
		Name = "Better Pangaea V5.0",
		Description = "TXT_KEY_MAP_PANGAEA_HELP",
		IsAdvancedMap = false,
		IconIndex = 0,
		SortIndex = 2,
		CustomOptions = {world_age, temperature, rainfall, sea_level,
			{
				Name = "TXT_KEY_MAP_OPTION_RESOURCES",	-- Customizing the Resource setting to Default to Strategic Balance.
				Values = {
					"TXT_KEY_MAP_OPTION_SPARSE",
					"TXT_KEY_MAP_OPTION_STANDARD",
					"TXT_KEY_MAP_OPTION_ABUNDANT",
					"TXT_KEY_MAP_OPTION_LEGENDARY_START",
					"TXT_KEY_MAP_OPTION_STRATEGIC_BALANCE",
					"Strategic Balance With Coal",
					"TXT_KEY_MAP_OPTION_RANDOM",
				},
				DefaultValue = 6,
				SortPriority = -95,
			},},
	}
end
------------------------------------------------------------------------------

------------------------------------------------------------------------------
------------------------------------------------------------------------------
PangaeaFractalWorld = {};
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
	for x = 0, self.iNumPlotsX - 1 do
		for y = 0, self.iNumPlotsY - 1 do
		
			local i = y * self.iNumPlotsX + x;
			local val = self.continentsFrac:GetHeight(x, y);
			local mountainVal = self.mountainsFrac:GetHeight(x, y);
			local hillVal = self.hillsFrac:GetHeight(x, y);
	
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
					if (hillVal >= (iPassThreshold)*6/10) then -- Mountain Pass though the ridgeline - Brian
						self.plotTypes[i] = PlotTypes.PLOT_HILLS;
					else -- Mountain
						self.plotTypes[i] = PlotTypes.PLOT_MOUNTAIN;
						-- self.plotTypes[i] = PlotTypes.PLOT_HILLS;
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
				local placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, false, false, YieldTypes.YIELD_FOOD, self);
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
		iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, self, true);
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
		while bNeedFirstRingBonus == true and attempt <= 6 do
			diceroll = 1 + Map.Rand(6, "");
			local plot_adjustments = randomized_first_ring_adjustments[diceroll];
			local searchX, searchY = self:ApplyHexAdjustment(x, y, plot_adjustments)
			searchPlot = Map.GetPlot(searchX, searchY);
			local placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, false, false, YieldTypes.YIELD_FOOD, self);
			print("searchX = "..tostring(searchX)..", searchY = "..tostring(searchY));
			print("placedBonus "..tostring(placedBonus));
			local plotType, terrainType, featureType, resourceType, isFreshWater,
			startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore,
			startBadTile, earlyBadTile, midBadTile, lateBadTile, bIsBonus = PlotAnalyzer(searchPlot, alongOcean, self);
			print("startScore "..tostring(startFoodScore + startHammerScore));
			if (startFoodScore + startHammerScore) > 2 then
				bNeedFirstRingBonus = false;
			end
			attempt = attempt + 1;
		end
	end
	print("bNeedFirstRingBonus "..tostring(bNeedFirstRingBonus));

	startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
		iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, self, true);

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
					iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
				earlyScore = earlyFoodScore + earlyHammerScore;
			end
			attempt = attempt + 1;
		end
	end
	]]
	
	--if iNumHabitableTiles > 0 then
	for iRunCounter = 1, 8 do
		attempt = 1;
		placedBonus = false;
		while (startScore/(18 - iNumStartBadTiles) < (2 + iRunCounter/18)) and
			attempt < 18 and not placedBonus and iNumBonuses < 6 do
			local plot_adjustments = randomized_search_table[attempt];
			local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(x, y, plot_adjustments)
			-- Attempt to place a Bonus at the currently chosen plot.
			placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, bAllowOasis, alongOcean, YieldTypes.YIELD_FOOD, self);
			-- local placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, alongOcean, allow_oasis, YieldTypes.YIELD_PRODUCTION, self);

			if placedBonus == true then
				startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
					iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
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
	local iron_list, horse_list, oil_list, coal_list = {}, {}, {}, {};
	local iron_fallback, horse_fallback, oil_fallback, coal_fallback = {}, {}, {}, {};
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
							if terrainType ~= TerrainTypes.TERRAIN_SNOW and featureType == FeatureTypes.NO_FEATURE then
								table.insert(horse_fallback, plotIndex)
							end
						elseif plotType == PlotTypes.PLOT_LAND then
							if featureType == FeatureTypes.NO_FEATURE then
								if terrainType == TerrainTypes.TERRAIN_TUNDRA or terrainType == TerrainTypes.TERRAIN_DESERT then
									if ripple_radius < 6 then
										table.insert(coal_list, plotIndex)
										table.insert(oil_fallback, plotIndex)
									else
										table.insert(coal_fallback, plotIndex)
									end
								elseif terrainType == TerrainTypes.TERRAIN_PLAINS or terrainType == TerrainTypes.TERRAIN_GRASS then
									if ripple_radius < 6 then
										table.insert(coal_list, plotIndex)
										table.insert(oil_fallback, plotIndex)
									else
										table.insert(coal_fallback, plotIndex)
									end
								elseif terrainType == TerrainTypes.TERRAIN_SNOW then
									if ripple_radius < 6 then
										table.insert(coal_list, plotIndex)
										table.insert(oil_fallback, plotIndex)
									else
										table.insert(coal_fallback, plotIndex)
									end
								end
							elseif featureType == FeatureTypes.FEATURE_MARSH then		
								if ripple_radius < 4 then
									table.insert(coal_list, plotIndex)
									table.insert(oil_fallback, plotIndex)
								else
									table.insert(coal_fallback, plotIndex)
								end
							elseif featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then
								table.insert(coal_fallback, plotIndex)
							elseif featureType == FeatureTypes.FEATURE_JUNGLE or featureType == FeatureTypes.FEATURE_FOREST then
								table.insert(coal_fallback, plotIndex)
								table.insert(oil_fallback, plotIndex)
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
	local shuf_list;
	local placed_iron, placed_horse, placed_oil, placed_coal = false, false, false, false;

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
			placed_oil = true;
		end
	end
	
	if res == 6 then
		if table.maxn(coal_list) > 0 then
			shuf_list = GetShuffledCopyOfTable(coal_list)
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.coal_ID, coal_amt, 1, 1, 1, 0, 0, shuf_list);
			if iNumLeftToPlace == 0 then
				placed_coal = true;
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
	if placed_oil == false and table.maxn(oil_fallback) > 0 then
		shuf_list = GetShuffledCopyOfTable(oil_fallback)
		iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.oil_ID, oil_amt, 1, 1, -1, 0, 0, shuf_list);
		print("Fallback Used");
		if iNumLeftToPlace == 0 then
			print("All Oil Placed 2nd Attempt");
		else
			--print("Not All Oil Placed");
		end
	end
	if res == 6 then
		if placed_coal == false and table.maxn(coal_fallback) > 0 then
			shuf_list = GetShuffledCopyOfTable(coal_fallback)
			iNumLeftToPlace = self:PlaceSpecificNumberOfResources(self.coal_ID, coal_amt, 1, 1, 1, 0, 0, shuf_list);
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

	for iRunCounter = 1, 8 do
		randomized_map_plots_data = GetShuffledCopyOfTable(map_plots_data);
		for loop, plot_data in ipairs(randomized_map_plots_data) do
			local x, y = plot_data[1], plot_data[2];
			local plot = Map.GetPlot(x, y);
			local plotIndex = y * iW + x + 1;
			local isEvenY = true;
			if y / 2 > math.floor(y / 2) then
				isEvenY = false;
			end
			if plot:GetPlotType() == PlotTypes.PLOT_LAND and plot:GetTerrainType() ~= TerrainTypes.TERRAIN_TUNDRA or plot:GetPlotType() == PlotTypes.PLOT_HILLS then
				local search_table = {};
				
				-- Set up Conditions checks.
				local alongOcean = false;
				local nextToLake = false;
				local isRiver = false;
				local nearRiver = false;
				local nearMountain = false;
				local res = Map.GetCustomOption(5);
				-- local start = Map.GetCustomOption(7);
			
				-- Check start plot to see if it's adjacent to saltwater.
				if AssignStartingPlots.plotDataIsCoastal[plotIndex] == true then
					alongOcean = true;
				end
				
				-- Check start plot to see if it's on a river.
				if plot:IsRiver() then
					isRiver = true;
				end

				local startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
					iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);

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

				--print("x = "..tostring(x)..", y = "..tostring(y));
				--print("AvgearlyScore = "..tostring(earlyScore/(36 - iNumEarlyBadTiles))..", AvgMidScore = "..tostring(midScore/(36 - iNumMidBadTiles)))

				attempt = 1;
				if iNumHabitableTiles > 6 then
					while (earlyScore/iNumHabitableTiles < 3/2 or 6*earlyHammerScore < earlyScore--[[ or 4*iNumBadDesert > iNumHabitableTiles]]) and attempt < 36
					and (6*iNumHillTiles < iNumHabitableTiles and fastHammerScore >= 9 or 8*iNumHillTiles < iNumHabitableTiles and fastHammerScore < 9) do
						local plot_adjustments = randomized_search_table[attempt];
						local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(x, y, plot_adjustments);
						if AssignStartingPlots:AttemptToPlaceHillsAtPlot(searchX, searchY) then
							startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
								iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
							earlyScore = earlyFoodScore + earlyHammerScore;
						end
						attempt = attempt + 1;
					end
				end
	
				attempt = 1;
				if iNumHabitableTiles > 12 then
					while (earlyScore/(36 - iNumEarlyBadTiles) < (2 + iRunCounter/36)--[[ or midScore/(36 - iNumMidBadTiles) < (3 + 6/18 + iRunCounter/18)]]) and
						attempt < 36 and not placedBonus and iNumBonuses < 9 do
						local plot_adjustments = randomized_search_table[attempt];
						local searchX, searchY = AssignStartingPlots:ApplyHexAdjustment(x, y, plot_adjustments)
						-- Attempt to place a Bonus at the currently chosen plot.
						placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, bAllowOasis, alongOcean, YieldTypes.YIELD_FOOD, AssignStartingPlots);
						-- local placedBonus, placedOasis, resource_ID = AttemptToPlaceResource(searchX, searchY, alongOcean, allow_oasis, YieldTypes.YIELD_PRODUCTION, AssignStartingPlots);

						if placedBonus == true then
							startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
								iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots, false);
							earlyScore = earlyFoodScore + earlyHammerScore;
						end
						attempt = attempt + 1;
					end
				end
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
function AreaAnalyzer(plot, AssignStartingPlots, isStartLocation)
	local isRiver, isNearMountain,
	iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles,
	StartFoodScore, EarlyFoodScore, MidFoodScore, LateFoodScore,
	StartHammerScore, EarlyHammerScore, MidHammerScore, LateHammerScore, fastHammerScore = 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;

	local iNumBadDesert = 0;
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
	iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, (not HaveOasis and bAllowOasis),
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
		resourceType = plot:GetResourceType();
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
				elseif resourceType == AssignStartingPlots.crab_ID or resourceType == AssignStartingPlots.whale_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 2;
					earlyFoodScore = earlyFoodScore + 2;
					midFoodScore = midFoodScore + 4;
					lateFoodScore = lateFoodScore + 4;
					midHammerScore = midHammerScore + 1;
					lateHammerScore = lateHammerScore + 2;
				elseif resourceType == AssignStartingPlots.pearls_ID then
					bIsBonus = true;
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 1;
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
					startFoodScore = startFoodScore + 1;
					earlyFoodScore = earlyFoodScore + 1;
					midFoodScore = midFoodScore + 1;
					lateFoodScore = lateFoodScore + 1;
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
				-- iNumHills = iNumHills + 1;
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
					-- iNumFlatGrass = iNumFlatGrass + 1;
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
					-- iNumFlatPlains = iNumFlatPlains + 1;
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
					-- iNumFlatTundra = iNumFlatTundra + 1;
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
					-- iNumFlatDesert = iNumFlatDesert + 1;
					if featureType == FeatureTypes.FEATURE_FLOOD_PLAINS then
						startFoodScore = startFoodScore + 2;
						earlyFoodScore = earlyFoodScore + 2;
						midFoodScore = midFoodScore + 2;
						lateFoodScore = lateFoodScore + 2;
					elseif featureType == FeatureTypes.FEATURE_OASIS then
						bIsBonus = true;
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
				-- iNumSnow = iNumSnow + 1;
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
					midFoodScore = midFoodScore + 2;
					lateFoodScore = lateFoodScore + 3;
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
					startHammerScore = startHammerScore + 1;
					earlyHammerScore = earlyHammerScore + 1;
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
								if resourceType == AssignStartingPlots.coal_ID or
									resourceType == AssignStartingPlots.aluminum_ID or
									resourceType == AssignStartingPlots.oil_ID or
									resourceType == AssignStartingPlots.uranium_ID or
									resourceType == -1 then
									midHammerScore = midHammerScore + 1;
									lateHammerScore = lateHammerScore + 2;
								end
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
function AttemptToPlaceResource(x, y, bAllowOasis, bIsCoastal, yieldType, start_plot_database)
	-- Returns two booleans. First is true if something was placed. Second true if Oasis placed.
	--print("-"); print("Attempting to place a Bonus at: ", x, y);
	--local start_plot_database = AssignStartingPlots.Create();
	local plot = Map.GetPlot(x, y);
	if plot == nil then
		--print("Placement failed, plot was nil.");
		return false
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
	if yieldType == YieldTypes.YIELD_FOOD then
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
			plot:SetResourceType(start_plot_database.banana_ID, 1);
			--print("Placed Banana.");
			start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.banana_ID + 1] + 1;
			return true, false, start_plot_database.banana_ID
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
					local diceroll = Map.Rand(2, "");
					if diceroll == 1 then
						plot:SetResourceType(start_plot_database.bison_ID, 1);
						--print("Placed Bison.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.bison_ID + 1] + 1;
						return true, false, start_plot_database.bison_ID
					else
						plot:SetResourceType(start_plot_database.cow_ID, 1);
						--print("Placed Cow.");
						start_plot_database.amounts_of_resources_placed[start_plot_database.cow_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.cow_ID + 1] + 1;
						return true, false, start_plot_database.cow_ID
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
		elseif plotType == PlotTypes.PLOT_OCEAN and bIsCoastal then
			if terrainType == TerrainTypes.TERRAIN_COAST and featureType == FeatureTypes.NO_FEATURE then
				if plot:IsLake() == false then -- Place Fish
					plot:SetResourceType(start_plot_database.fish_ID, 1);
					--print("Placed Fish.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.fish_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.fish_ID + 1] + 1;
					return true, false, start_plot_database.fish_ID
				end
			end
		end	
	elseif yieldType == YieldTypes.YIELD_PRODUCTION then
		if featureType == FeatureTypes.FEATURE_JUNGLE then -- Place Iron
			plot:SetResourceType(start_plot_database.iron_ID, 2);
			--print("Placed Iron.");
			start_plot_database.amounts_of_resources_placed[start_plot_database.iron_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.iron_ID + 1] + 2;
			return true, false, start_plot_database.iron_ID
		elseif featureType == FeatureTypes.FEATURE_FOREST then -- Place Iron
			plot:SetResourceType(start_plot_database.iron_ID, 2);
			--print("Placed Iron.");
			start_plot_database.amounts_of_resources_placed[start_plot_database.iron_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.iron_ID + 1] + 2;
			return true, false, start_plot_database.iron_ID
		elseif plotType == PlotTypes.PLOT_HILLS and featureType == FeatureTypes.NO_FEATURE then
			plot:SetResourceType(start_plot_database.iron_ID, 2);
			--print("Placed Iron.");
			start_plot_database.amounts_of_resources_placed[start_plot_database.iron_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.iron_ID + 1] + 2;
			return true, false, start_plot_database.iron_ID
		elseif plotType == PlotTypes.PLOT_LAND then
			if featureType == FeatureTypes.NO_FEATURE then
				if terrainType == TerrainTypes.TERRAIN_GRASS then -- Place Stone
					plot:SetResourceType(start_plot_database.stone_ID, 1);
					--print("Placed Stone.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] + 1;
					return true, false, start_plot_database.stone_ID
				elseif terrainType == TerrainTypes.TERRAIN_PLAINS then -- Place Horse
					plot:SetResourceType(start_plot_database.horse_ID, 2);
					--print("Placed Horse.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.horse_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.horse_ID + 1] + 2;
					return true, false, start_plot_database.horse_ID
				elseif terrainType == TerrainTypes.TERRAIN_TUNDRA then -- Place Stone
					plot:SetResourceType(start_plot_database.stone_ID, 1);
					--print("Placed Stone.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] + 1;
					return true, false, start_plot_database.stone_ID
				elseif terrainType == TerrainTypes.TERRAIN_DESERT then
					plot:SetResourceType(start_plot_database.stone_ID, 1);
					--print("Placed Stone.");
					start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] = start_plot_database.amounts_of_resources_placed[start_plot_database.stone_ID + 1] + 1;
					return true, false, start_plot_database.stone_ID
				end
			end
		end
	end
	-- Nothing placed.
	return false, false, -1
end
------------------------------------------------------------------------------
function AddRivers()
	
	print("Map Generation - Adding Rivers");

	local passConditions = {
		
		function(plot)
			local startFoodScore, earlyFoodScore, midFoodScore, lateFoodScore, startHammerScore, earlyHammerScore, midHammerScore, lateHammerScore, fastHammerScore,
						iNumStartBadTiles, iNumEarlyBadTiles, iNumMidBadTiles, iNumLateBadTiles, iNumBadDesert, bAllowOasis, iNumLandTiles, iNumHillTiles, iNumHabitableTiles, iNumBonuses = AreaAnalyzer(plot, AssignStartingPlots:Create(), false);
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
		end
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

end
------------------------------------------------------------------------------
