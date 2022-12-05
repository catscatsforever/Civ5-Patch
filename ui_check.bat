@ECHO off
pushd "%~dp0"
cd ..
set patchfolder=Tournament Mod V5.2b
ECHO Y | del "%cd%\%patchfolder%\UI\"
REM -------------------------------------------------
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.lua" "%cd%\%patchfolder%\UI\CultureOverview.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.xml" "%cd%\%patchfolder%\UI\CultureOverview.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\EnemyUnitPanel.lua" "%cd%\%patchfolder%\UI\EnemyUnitPanel.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\JoiningRoom.lua" "%cd%\%patchfolder%\UI\JoiningRoom.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\VictoryProgress.xml" "%cd%\%patchfolder%\UI\VictoryProgress.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MPList.lua" "%cd%\%patchfolder%\UI\MPList.lua"
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\UnitFlagManager\UnitFlagManager.lua" (
  ECHO UnitFlagManager.lua exists on EUI, skipping
) ELSE (
  ECHO UnitFlagManager.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\UnitFlagManager.lua" "%cd%\%patchfolder%\UI\UnitFlagManager.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\PlotHelp\PlotHelpManager.lua" (
  ECHO PlotHelpManager.lua exists on EUI, skipping
) ELSE (
  ECHO PlotHelpManager.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\PlotHelpManager.lua" "%cd%\%patchfolder%\UI\PlotHelpManager.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\PlotHelp\PlotHelpManager.xml" (
  ECHO PlotHelpManager.xml exists on EUI, skipping
) ELSE (
  ECHO PlotHelpManager.xml does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\PlotHelpManager.xml" "%cd%\%patchfolder%\UI\PlotHelpManager.xml"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\CityView\ProductionPopup.lua" (
  ECHO ProductionPopup.lua exists on EUI, skipping
) ELSE (
  ECHO ProductionPopup.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ProductionPopup.lua" "%cd%\%patchfolder%\UI\ProductionPopup.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\TechTree\TechPopup.lua" (
  ECHO TechPopup.lua exists on EUI, skipping
) ELSE (
  ECHO TechPopup.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\TechPopup.lua" "%cd%\%patchfolder%\UI\TechPopup.lua"
)
REM -------------------------------------------------
set text="-- modified by bc1 from 1.0.3.144 brave new world code"
FIND %text% "%cd%\UI_bc1\CityStatePopup\CityStateDiploPopup.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityStateDiploPopup.lua" "%cd%\%patchfolder%\UI\CityStateDiploPopup.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityStateDiploPopup.lua" "%cd%\%patchfolder%\UI\CityStateDiploPopup.lua"
)
REM -------------------------------------------------
set text="-- coded by bc1 from 1.0.3.276 brave new world code"
FIND %text% "%cd%\UI_bc1\CityView\CityView.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityView.lua" "%cd%\%patchfolder%\UI\CityView.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityView.lua" "%cd%\%patchfolder%\UI\CityView.lua"
)
REM -------------------------------------------------
set text="-- coded by bc1 from Civ V 1.0.3.276 code"
FIND %text% "%cd%\UI_bc1\TopPanel\TopPanel.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\TopPanel.lua" "%cd%\%patchfolder%\UI\TopPanel.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\TopPanel.lua" "%cd%\%patchfolder%\UI\TopPanel.lua"
)
REM -------------------------------------------------
set text="Game.SelectionListGameNetMessage( GameMessageTypes.GAMEMESSAGE_DO_COMMAND, action.CommandType, action.CommandData, -1, 0, bAlt );"
FIND %text% "%cd%\UI_bc1\Improvements\ConfirmCommandPopup.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\ConfirmCommandPopup.lua" "%cd%\%patchfolder%\UI\ConfirmCommandPopup.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ConfirmCommandPopup.lua" "%cd%\%patchfolder%\UI\ConfirmCommandPopup.lua"
)
REM -------------------------------------------------
set text="-- coded by bc1 from Civ V 1.0.3.276 code"
FIND %text% "%cd%\UI_bc1\TechTree\TechTree.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\TechTree.lua" "%cd%\%patchfolder%\UI\TechTree.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\TechTree.lua" "%cd%\%patchfolder%\UI\TechTree.lua"
)
EXIT