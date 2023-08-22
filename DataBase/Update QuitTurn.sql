CREATE TABLE If not EXISTS PlayerQuitTurn (
	"Player"	TEXT,
	"PlayerGameNumber"	INTEGER,
	"Value"	INTEGER NOT NULL
)
;

DELETE FROM PlayerQuitTurn
;

REPLACE INTO PlayerQuitTurn
	SELECT Player, PlayerGameNumber, Turn

	FROM ReplayDataSetsChanges
	JOIN CivKeys ON CivKeys.CivID = ReplayDataSetsChanges.CivID
	JOIN GameSeeds ON GameSeeds.GameSeed = ReplayDataSetsChanges.GameSeed
	JOIN Games ON Games.Civilization = CivKeys.CivKey AND Games.GameID = GameSeeds.GameID
	WHERE ReplayDataSetID = 6 AND Value < 0
;
/*SELECT Player, PlayerGameNumber, Turn, Value

FROM ReplayDataSetsChanges
JOIN CivKeys ON CivKeys.CivID = ReplayDataSetsChanges.CivID
JOIN GameSeeds ON GameSeeds.GameSeed = ReplayDataSetsChanges.GameSeed
JOIN Games ON Games.Civilization = CivKeys.CivKey AND Games.GameID = GameSeeds.GameID
WHERE ReplayDataSetID = 6 AND Value < 0*/