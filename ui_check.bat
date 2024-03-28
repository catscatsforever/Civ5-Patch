@ECHO off
pushd "%~dp0"
cd ..
set patchfolder=Tournament Mod V9.0
ECHO Y | del "%cd%\%patchfolder%\UI\"
REM -------------------------------------------------
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.lua" "%cd%\%patchfolder%\UI\CultureOverview.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.xml" "%cd%\%patchfolder%\UI\CultureOverview.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\EnemyUnitPanel.lua" "%cd%\%patchfolder%\UI\EnemyUnitPanel.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\InGame.lua" "%cd%\%patchfolder%\UI\InGame.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\JoiningRoom.lua" "%cd%\%patchfolder%\UI\JoiningRoom.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\VictoryProgress.xml" "%cd%\%patchfolder%\UI\VictoryProgress.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\VictoryProgress.lua" "%cd%\%patchfolder%\UI\VictoryProgress.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MPList.lua" "%cd%\%patchfolder%\UI\MPList.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MiniMapPanel.lua" "%cd%\%patchfolder%\UI\MiniMapPanel.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MiniMapPanel.xml" "%cd%\%patchfolder%\UI\MiniMapPanel.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ChooseIdeologyPopup.lua" "%cd%\%patchfolder%\UI\ChooseIdeologyPopup.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CCVotePopup.lua" "%cd%\%patchfolder%\UI\CCVotePopup.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CCVotePopup.xml" "%cd%\%patchfolder%\UI\CCVotePopup.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\EndGameMenu.lua" "%cd%\%patchfolder%\UI\EndGameMenu.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ProposalChartPopup.lua" "%cd%\%patchfolder%\UI\ProposalChartPopup.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ProposalChartPopup.xml" "%cd%\%patchfolder%\UI\ProposalChartPopup.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\AdvancedSetup.lua" "%cd%\%patchfolder%\UI\AdvancedSetup.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\StagingRoom.lua" "%cd%\%patchfolder%\UI\StagingRoom.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\StagingRoom.xml" "%cd%\%patchfolder%\UI\StagingRoom.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MPGameDefaults.lua" "%cd%\%patchfolder%\UI\MPGameDefaults.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MPGameOptions.lua" "%cd%\%patchfolder%\UI\MPGameOptions.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MPGameSetupScreen.xml" "%cd%\%patchfolder%\UI\MPGameSetupScreen.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MPTurnPanel.lua" "%cd%\%patchfolder%\UI\MPTurnPanel.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\MPTurnPanel.xml" "%cd%\%patchfolder%\UI\MPTurnPanel.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CivilopediaScreen.lua" "%cd%\%patchfolder%\UI\CivilopediaScreen.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\GameMenu.lua" "%cd%\%patchfolder%\UI\GameMenu.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\Demographics.lua" "%cd%\%patchfolder%\UI\Demographics.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\Bombardment.lua" "%cd%\%patchfolder%\UI\Bombardment.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ChoosePantheonPopup.lua" "%cd%\%patchfolder%\UI\ChoosePantheonPopup.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ChooseReligionPopup.lua" "%cd%\%patchfolder%\UI\ChooseReligionPopup.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ReplayViewer.lua" "%cd%\%patchfolder%\UI\ReplayViewer.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ReplayViewer.xml" "%cd%\%patchfolder%\UI\ReplayViewer.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ReligionOverview.lua" "%cd%\%patchfolder%\UI\ReligionOverview.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ReligionOverview.xml" "%cd%\%patchfolder%\UI\ReligionOverview.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\EspionageOverview.lua" "%cd%\%patchfolder%\UI\EspionageOverview.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityList.lua" "%cd%\%patchfolder%\UI\CityList.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityList.xml" "%cd%\%patchfolder%\UI\CityList.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\GPList.lua" "%cd%\%patchfolder%\UI\GPList.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\GPList.xml" "%cd%\%patchfolder%\UI\GPList.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ResourceList.lua" "%cd%\%patchfolder%\UI\ResourceList.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ResourceList.xml" "%cd%\%patchfolder%\UI\ResourceList.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\UnitList.lua" "%cd%\%patchfolder%\UI\UnitList.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\UnitList.xml" "%cd%\%patchfolder%\UI\UnitList.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\Highlights.xml" "%cd%\%patchfolder%\UI\Highlights.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\NetworkKickedPopup.lua" "%cd%\%patchfolder%\UI\NetworkKickedPopup.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\SimpleDiploTrade.lua" "%cd%\%patchfolder%\UI\SimpleDiploTrade.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\SimpleDiploTrade.xml" "%cd%\%patchfolder%\UI\SimpleDiploTrade.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\TradeRouteHelpers.lua" "%cd%\%patchfolder%\UI\TradeRouteHelpers.lua"

REM -------------------------------------------------
set text="-- destroy: check fix for need to update plot & cargo & airbase"
FIND %text% "%cd%\UI_bc1\UnitFlagManager\UnitFlagManager.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\UnitFlagManager.xml" "%cd%\%patchfolder%\UI\UnitFlagManager.xml"
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\UnitFlagManager.lua" "%cd%\%patchfolder%\UI\UnitFlagManager.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\UnitFlagManager.xml" "%cd%\%patchfolder%\UI\UnitFlagManager.xml"
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\UnitFlagManager.lua" "%cd%\%patchfolder%\UI\UnitFlagManager.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\Improvements\SocialPolicyPopup.lua" (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\SocialPolicyPopup.lua" "%cd%\%patchfolder%\UI\SocialPolicyPopup.lua"
) ELSE (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\SocialPolicyPopup.lua" "%cd%\%patchfolder%\UI\SocialPolicyPopup.lua"
)
REM -------------------------------------------------
IF NOT EXIST "%cd%\UI_bc1\NotificationPanel\DiploList.lua" (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\DiploList.lua" "%cd%\%patchfolder%\UI\DiploList.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\LeaderHead\TradeLogic.lua" (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\TradeLogic.lua" "%cd%\%patchfolder%\UI\TradeLogic.lua"
) ELSE (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\TradeLogic.lua" "%cd%\%patchfolder%\UI\TradeLogic.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\ToolTips\InfoTooltipInclude.lua" (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\EUI_tooltip_library.lua" "%cd%\%patchfolder%\UI\EUI_tooltip_library.lua"
) ELSE (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\InfoTooltipInclude.lua" "%cd%\%patchfolder%\UI\InfoTooltipInclude.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\ToolTips\InfoTooltipInclude.lua" (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\EUI_unit_include.lua" "%cd%\%patchfolder%\UI\EUI_unit_include.lua"
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
IF EXIST "%cd%\UI_bc1\ToolTips\TechButtonInclude.lua" (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\TechButtonInclude.lua" "%cd%\%patchfolder%\UI\TechButtonInclude.lua"
) ELSE (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\TechButtonInclude.lua" "%cd%\%patchfolder%\UI\TechButtonInclude.lua"
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
set text="-- modified by bc1 from Civ V 1.0.3.276 code"
FIND %text% "%cd%\UI_bc1\UnitPanel\UnitPanel.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\UnitPanel.lua" "%cd%\%patchfolder%\UI\UnitPanel.lua"
) || (
  ECHO UnitPanel.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\UnitPanel.lua" "%cd%\%patchfolder%\UI\UnitPanel.lua"
)
REM -------------------------------------------------
set text="-- modified by bc1 from 1.0.3.144 brave new world code"
FIND %text% "%cd%\UI_bc1\CityStatePopup\CityStateDiploPopup.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityStateDiploPopup.lua" "%cd%\%patchfolder%\UI\CityStateDiploPopup.lua"
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityStateDiploPopup.xml" "%cd%\%patchfolder%\UI\CityStateDiploPopup.xml"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityStateDiploPopup.lua" "%cd%\%patchfolder%\UI\CityStateDiploPopup.lua"
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityStateDiploPopup.xml" "%cd%\%patchfolder%\UI\CityStateDiploPopup.xml"
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
IF EXIST "%cd%\UI_bc1\Options\OptionsMenu.lua" (
  ECHO OptionsMenu.lua: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\OptionsMenu.lua.ignore" "%cd%\%patchfolder%\UI\OptionsMenu.lua"
) ELSE (
  ECHO OptionsMenu.lua: no EUI, copying vanilla UI version
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\OptionsMenu.lua.ignore" "%cd%\%patchfolder%\UI\OptionsMenu.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\Options\OptionsMenu.xml" (
  ECHO OptionsMenu.xml: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\OptionsMenu.xml.ignore" "%cd%\%patchfolder%\UI\OptionsMenu.xml"
) ELSE (
  ECHO OptionsMenu.xml: no EUI, copying vanilla UI version
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\OptionsMenu.xml.ignore" "%cd%\%patchfolder%\UI\OptionsMenu.xml"
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
) ELSE (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityBannerManager.lua" "%cd%\%patchfolder%\UI\CityBannerManager.lua"
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityBannerManager.xml" "%cd%\%patchfolder%\UI\CityBannerManager.xml"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\NotificationPanel\DiploCorner.xml" (
  ECHO DiploCorner.lua: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\DiploCorner.lua" "%cd%\%patchfolder%\UI\DiploCorner.lua"
) ELSE (
  ECHO DiploCorner.lua: no EUI, copying vanilla UI version
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\DiploCorner.lua" "%cd%\%patchfolder%\UI\DiploCorner.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\NotificationPanel\DiploCorner.xml" (
  ECHO DiploCorner.xml: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\DiploCorner.xml" "%cd%\%patchfolder%\UI\DiploCorner.xml"
) ELSE (
  ECHO DiploCorner.xml: no EUI, copying vanilla UI version
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\DiploCorner.xml" "%cd%\%patchfolder%\UI\DiploCorner.xml"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\Improvements\WorldView.lua" (
  ECHO WorldView.lua: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\WorldView.lua" "%cd%\%patchfolder%\UI\WorldView.lua"
) ELSE (
  ECHO WorldView.lua: no EUI, copying vanilla UI version
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\WorldView.lua" "%cd%\%patchfolder%\UI\WorldView.lua"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\TopPanel\TopPanel.xml" (
  ECHO TopPanel.xml: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\TopPanel.xml" "%cd%\%patchfolder%\UI\TopPanel.xml"
) ELSE (
  ECHO TopPanel.xml: no EUI, skipping
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\CityView\CityView.xml" (
  ECHO CityView.xml: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityView.xml" "%cd%\%patchfolder%\UI\CityView.xml"
) ELSE (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityView.xml" "%cd%\%patchfolder%\UI\CityView.xml"
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\CityView\CityView_small.xml" (
  ECHO CityView_small.xml: EUI version detected
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityView_small.xml" "%cd%\%patchfolder%\UI\CityView_small.xml"
) ELSE (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityView_small.xml" "%cd%\%patchfolder%\UI\CityView_small.xml"
)
EXIT
