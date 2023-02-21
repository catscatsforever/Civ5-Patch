@ECHO off
pushd "%~dp0"
cd ..
set patchfolder=Tournament Mod V5.4
ECHO Y | del "%cd%\%patchfolder%\UI\"
REM -------------------------------------------------
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.lua" "%cd%\%patchfolder%\UI\CultureOverview.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.xml" "%cd%\%patchfolder%\UI\CultureOverview.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\EnemyUnitPanel.lua" "%cd%\%patchfolder%\UI\EnemyUnitPanel.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\JoiningRoom.lua" "%cd%\%patchfolder%\UI\JoiningRoom.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\VictoryProgress.xml" "%cd%\%patchfolder%\UI\VictoryProgress.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\VictoryProgress.lua" "%cd%\%patchfolder%\UI\VictoryProgress.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MPList.lua" "%cd%\%patchfolder%\UI\MPList.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MiniMapPanel.lua" "%cd%\%patchfolder%\UI\MiniMapPanel.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MiniMapPanel.xml" "%cd%\%patchfolder%\UI\MiniMapPanel.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ChooseIdeologyPopup.lua" "%cd%\%patchfolder%\UI\ChooseIdeologyPopup.lua"

REM -------------------------------------------------
set text="-- destroy: check fix for need to update plot & cargo & airbase"
FIND %text% "%cd%\UI_bc1\UnitFlagManager\UnitFlagManager.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\UnitFlagManager.lua" "%cd%\%patchfolder%\UI\UnitFlagManager.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\UnitFlagManager.lua" "%cd%\%patchfolder%\UI\UnitFlagManager.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\PlotHelp\PlotHelpManager.lua" (
  ECHO PlotHelpManager.lua exists on EUI, skipping
) ELSE (
  ECHO PlotHelpManager.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\PlotHelpManager.lua" "%cd%\%patchfolder%\UI\PlotHelpManager.lua"
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\PlotMouseoverInclude.lua" "%cd%\%patchfolder%\UI\PlotMouseoverInclude.lua"
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
IF EXIST "%cd%\UI_bc1\UnitPanel\UnitPanel.lua" (
  ECHO UnitPanel.lua exists on EUI, skipping
) ELSE (
  ECHO UnitPanel.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\UnitPanel.lua" "%cd%\%patchfolder%\UI\UnitPanel.lua"
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
REM -------------------------------------------------
set text="-- modified by bc1 from 1.0.3.144 brave new world & civ BE code"
FIND %text% "%cd%\UI_bc1\Core\CityStateStatusHelper.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityStateStatusHelper.lua" "%cd%\%patchfolder%\UI\CityStateStatusHelper.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityStateStatusHelper.lua" "%cd%\%patchfolder%\UI\CityStateStatusHelper.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\NotificationPanel\NotificationPanel.lua" (
  ECHO NotificationPanel.lua: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\NotificationPanel.lua" "%cd%\%patchfolder%\UI\NotificationPanel.lua"
) ELSE (
  ECHO NotificationPanel.lua: no EUI, copying vanilla UI version
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\NotificationPanel.lua" "%cd%\%patchfolder%\UI\NotificationPanel.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\NotificationPanel\NotificationPanel.xml" (
  ECHO NotificationPanel.xml: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\NotificationPanel.xml" "%cd%\%patchfolder%\UI\NotificationPanel.xml"
) ELSE (
  ECHO NotificationPanel.xml: no EUI, copying vanilla UI version
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\NotificationPanel.xml" "%cd%\%patchfolder%\UI\NotificationPanel.xml"
)
REM -------------------------------------------------
set text="CityBannerProductionBox = function( city )"
IF EXIST "%cd%\UI_bc1\CityBanners\CityBannerManager.lua" (
  FIND %text% "%cd%\UI_bc1\CityBanners\CityBannerManager.lua" && (
    ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityBannerManager_1.lua" "%cd%\%patchfolder%\UI\CityBannerManager.lua"
    ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityBannerManager_1.xml" "%cd%\%patchfolder%\UI\CityBannerManager.xml"
  ) || (
    ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityBannerManager_2.lua" "%cd%\%patchfolder%\UI\CityBannerManager.lua"
    ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityBannerManager_2.xml" "%cd%\%patchfolder%\UI\CityBannerManager.xml"
  )
)
EXIT