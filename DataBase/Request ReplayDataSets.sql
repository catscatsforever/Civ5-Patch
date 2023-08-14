SELECT Player, Civilization, Standing, Games.GameID, ReplayDataSetKey AS ReplayDataSet, sum(Value) AS SUM_Value, Max(Turn),
(SELECT Value FROM PlayerQuitTurn WHERE PlayerQuitTurn.Player = Games.Player AND PlayerQuitTurn.PlayerGameNumber = Games.PlayerGameNumber) AS QuitTurn
--SELECT *

FROM DataSets
JOIN ReplayDataSetsChanges ON ReplayDataSetsChanges.DataSetID = DataSets.DataSetID
JOIN ReplayDataSetKeys ON ReplayDataSetKeys.ReplayDataSetID = ReplayDataSetsChanges.ReplayDataSetID
JOIN CivKeys ON CivKeys.CivID = ReplayDataSetsChanges.CivID
JOIN GameSeeds ON GameSeeds.GameSeed = ReplayDataSetsChanges.GameSeed
JOIN Games ON Games.Civilization = CivKeys.CivKey AND Games.GameID = GameSeeds.GameID
WHERE ReplayDataSetKeys.ReplayDataSetID = 71
AND Turn < 
CASE WHEN (SELECT Value FROM PlayerQuitTurn WHERE PlayerQuitTurn.Player = Games.Player AND PlayerQuitTurn.PlayerGameNumber = Games.PlayerGameNumber) IS NOT NULL THEN
	(SELECT Value FROM PlayerQuitTurn WHERE PlayerQuitTurn.Player = Games.Player AND PlayerQuitTurn.PlayerGameNumber = Games.PlayerGameNumber)
ELSE
	330
END
--WHERE ReplayDataSetKey = "Born Scientists"-- AND Player = "Edward Gromyako"
--WHERE ReplayDataSetKeys.ReplayDataSetID = 6
GROUP BY Games.GameID, Civilization
HAVING SUM_Value > 0
ORDER BY Games.GameID, Player/*, sum(Value) DESC*/
;