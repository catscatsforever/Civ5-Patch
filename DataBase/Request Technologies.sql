SELECT Player, Civilization, Standing, Games.GameID, TechnologyKey AS Technology, AVG(Turn)

FROM DataSets
JOIN TechnologiesChanges ON TechnologiesChanges.DataSetID = DataSets.DataSetID
JOIN TechnologyKeys ON TechnologyKeys.TechnologyID = TechnologiesChanges.TechnologyID
JOIN CivKeys ON CivKeys.CivID = TechnologiesChanges.CivID
JOIN GameSeeds ON GameSeeds.GameSeed = TechnologiesChanges.GameSeed
JOIN Games ON Games.Civilization = CivKeys.CivKey AND Games.GameID = GameSeeds.GameID
WHERE Value = 1 AND (TechnologyKey = "Plastics")
ORDER BY Turn
;