SELECT Player, Civilization, Standing, Games.GameID, BuildingClassKey AS BuildingClass, Turn
--SELECT *

FROM DataSets
JOIN BuildingClassesChanges ON BuildingClassesChanges.DataSetID = DataSets.DataSetID
JOIN BuildingClassKeys ON BuildingClassKeys.BuildingClassID = BuildingClassesChanges.BuildingClassID
JOIN CivKeys ON CivKeys.CivID = BuildingClassesChanges.CivID
JOIN GameSeeds ON GameSeeds.GameSeed = BuildingClassesChanges.GameSeed
JOIN Games ON Games.Civilization = CivKeys.CivKey AND Games.GameID = GameSeeds.GameID
WHERE BuildingClassKey = "Barn" OR BuildingClassKey = "Granary"
;