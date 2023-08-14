SELECT Player, Civilization, Games.GameID, Standing, PolicyKey AS Policy, Games.GameID,
AVG(Turn)

FROM DataSets
JOIN PoliciesChanges ON PoliciesChanges.DataSetID = DataSets.DataSetID
JOIN PolicyKeys ON PolicyKeys.PolicyID = PoliciesChanges.PolicyID
JOIN CivKeys ON CivKeys.CivID = PoliciesChanges.CivID
JOIN GameSeeds ON GameSeeds.GameSeed = PoliciesChanges.GameSeed
JOIN Games ON Games.Civilization = CivKeys.CivKey AND Games.GameID = GameSeeds.GameID
--WHERE Value = 1 AND PolicyKey IN ("Tradition Finisher", "Liberty Finisher")
WHERE Value = 1 AND PolicyKey IN ("Secularism", "Humanism", "Free Thought")
GROUP BY Policy
;