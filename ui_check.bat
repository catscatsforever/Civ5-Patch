@ECHO off
pushd "%~dp0"
set patchfolder=%cd%
cd ..
del /q "%patchfolder%\UI\"
REM -------------------------------------------------
copy /y "%patchfolder%\tmp\ui\CultureOverview.lua" "%patchfolder%\UI\CultureOverview.lua" > nul
copy /y "%patchfolder%\tmp\ui\CultureOverview.xml" "%patchfolder%\UI\CultureOverview.xml" > nul
copy /y "%patchfolder%\tmp\ui\EnemyUnitPanel.lua" "%patchfolder%\UI\EnemyUnitPanel.lua" > nul
copy /y "%patchfolder%\tmp\ui\InGame.lua" "%patchfolder%\UI\InGame.lua" > nul
copy /y "%patchfolder%\tmp\ui\JoiningRoom.lua" "%patchfolder%\UI\JoiningRoom.lua" > nul
copy /y "%patchfolder%\tmp\ui\VictoryProgress.xml" "%patchfolder%\UI\VictoryProgress.xml" > nul
copy /y "%patchfolder%\tmp\ui\VictoryProgress.lua" "%patchfolder%\UI\VictoryProgress.lua" > nul
copy /y "%patchfolder%\tmp\ui\MPList.lua" "%patchfolder%\UI\MPList.lua" > nul
copy /y "%patchfolder%\tmp\ui\MiniMapPanel.lua" "%patchfolder%\UI\MiniMapPanel.lua" > nul
copy /y "%patchfolder%\tmp\ui\MiniMapPanel.xml" "%patchfolder%\UI\MiniMapPanel.xml" > nul
copy /y "%patchfolder%\tmp\ui\ChooseIdeologyPopup.lua" "%patchfolder%\UI\ChooseIdeologyPopup.lua" > nul
copy /y "%patchfolder%\tmp\ui\CCVotePopup.lua" "%patchfolder%\UI\CCVotePopup.lua" > nul
copy /y "%patchfolder%\tmp\ui\CCVotePopup.xml" "%patchfolder%\UI\CCVotePopup.xml" > nul
copy /y "%patchfolder%\tmp\ui\EndGameMenu.lua" "%patchfolder%\UI\EndGameMenu.lua" > nul
copy /y "%patchfolder%\tmp\ui\ProposalChartPopup.lua" "%patchfolder%\UI\ProposalChartPopup.lua" > nul
copy /y "%patchfolder%\tmp\ui\ProposalChartPopup.xml" "%patchfolder%\UI\ProposalChartPopup.xml" > nul
copy /y "%patchfolder%\tmp\ui\AdvancedSetup.xml" "%patchfolder%\UI\AdvancedSetup.xml" > nul
copy /y "%patchfolder%\tmp\ui\AdvancedSetup.lua" "%patchfolder%\UI\AdvancedSetup.lua" > nul
copy /y "%patchfolder%\tmp\ui\StagingRoom.lua" "%patchfolder%\UI\StagingRoom.lua" > nul
copy /y "%patchfolder%\tmp\ui\StagingRoom.xml" "%patchfolder%\UI\StagingRoom.xml" > nul
copy /y "%patchfolder%\tmp\ui\MPGameDefaults.lua" "%patchfolder%\UI\MPGameDefaults.lua" > nul
copy /y "%patchfolder%\tmp\ui\MPGameOptions.lua" "%patchfolder%\UI\MPGameOptions.lua" > nul
copy /y "%patchfolder%\tmp\ui\MPGameSetupScreen.xml" "%patchfolder%\UI\MPGameSetupScreen.xml" > nul
copy /y "%patchfolder%\tmp\ui\MPTurnPanel.lua" "%patchfolder%\UI\MPTurnPanel.lua" > nul
copy /y "%patchfolder%\tmp\ui\MPTurnPanel.xml" "%patchfolder%\UI\MPTurnPanel.xml" > nul
copy /y "%patchfolder%\tmp\ui\CivilopediaScreen.lua" "%patchfolder%\UI\CivilopediaScreen.lua" > nul
copy /y "%patchfolder%\tmp\ui\GameMenu.lua" "%patchfolder%\UI\GameMenu.lua" > nul
copy /y "%patchfolder%\tmp\ui\Demographics.lua" "%patchfolder%\UI\Demographics.lua" > nul
copy /y "%patchfolder%\tmp\ui\Demographics.xml" "%patchfolder%\UI\Demographics.xml" > nul
copy /y "%patchfolder%\tmp\ui\Bombardment.lua" "%patchfolder%\UI\Bombardment.lua" > nul
copy /y "%patchfolder%\tmp\ui\ChoosePantheonPopup.lua" "%patchfolder%\UI\ChoosePantheonPopup.lua" > nul
copy /y "%patchfolder%\tmp\ui\ChooseReligionPopup.lua" "%patchfolder%\UI\ChooseReligionPopup.lua" > nul
copy /y "%patchfolder%\tmp\ui\ReplayViewer.lua" "%patchfolder%\UI\ReplayViewer.lua" > nul
copy /y "%patchfolder%\tmp\ui\ReplayViewer.xml" "%patchfolder%\UI\ReplayViewer.xml" > nul
copy /y "%patchfolder%\tmp\ui\ReligionOverview.lua" "%patchfolder%\UI\ReligionOverview.lua" > nul
copy /y "%patchfolder%\tmp\ui\ReligionOverview.xml" "%patchfolder%\UI\ReligionOverview.xml" > nul
copy /y "%patchfolder%\tmp\ui\EspionageOverview.lua" "%patchfolder%\UI\EspionageOverview.lua" > nul
copy /y "%patchfolder%\tmp\ui\CityList.lua" "%patchfolder%\UI\CityList.lua" > nul
copy /y "%patchfolder%\tmp\ui\CityList.xml" "%patchfolder%\UI\CityList.xml" > nul
copy /y "%patchfolder%\tmp\ui\GPList.lua" "%patchfolder%\UI\GPList.lua" > nul
copy /y "%patchfolder%\tmp\ui\GPList.xml" "%patchfolder%\UI\GPList.xml" > nul
copy /y "%patchfolder%\tmp\ui\ResourceList.lua" "%patchfolder%\UI\ResourceList.lua" > nul
copy /y "%patchfolder%\tmp\ui\ResourceList.xml" "%patchfolder%\UI\ResourceList.xml" > nul
copy /y "%patchfolder%\tmp\ui\UnitList.lua" "%patchfolder%\UI\UnitList.lua" > nul
copy /y "%patchfolder%\tmp\ui\UnitList.xml" "%patchfolder%\UI\UnitList.xml" > nul
copy /y "%patchfolder%\tmp\ui\Highlights.xml" "%patchfolder%\UI\Highlights.xml" > nul
copy /y "%patchfolder%\tmp\ui\NetworkKickedPopup.lua" "%patchfolder%\UI\NetworkKickedPopup.lua" > nul
copy /y "%patchfolder%\tmp\ui\SimpleDiploTrade.lua" "%patchfolder%\UI\SimpleDiploTrade.lua" > nul
copy /y "%patchfolder%\tmp\ui\SimpleDiploTrade.xml" "%patchfolder%\UI\SimpleDiploTrade.xml" > nul
copy /y "%patchfolder%\tmp\ui\TradeRouteHelpers.lua" "%patchfolder%\UI\TradeRouteHelpers.lua" > nul
copy /y "%patchfolder%\tmp\ui\SocialPolicyPopup.xml" "%patchfolder%\UI\SocialPolicyPopup.xml" > nul
copy /y "%patchfolder%\tmp\ui\TechTree.xml" "%patchfolder%\UI\TechTree.xml" > nul
copy /y "%patchfolder%\tmp\ui\TechHelpInclude.lua" "%patchfolder%\UI\TechHelpInclude.lua" > nul
copy /y "%patchfolder%\tmp\ui\AnnexCityPopup.lua" "%patchfolder%\UI\AnnexCityPopup.lua" > nul
copy /y "%patchfolder%\tmp\ui\ChooseMayaBonus.lua" "%patchfolder%\UI\ChooseMayaBonus.lua" > nul
copy /y "%patchfolder%\tmp\ui\ChooseMayaBonus.xml" "%patchfolder%\UI\ChooseMayaBonus.xml" > nul
copy /y "%patchfolder%\tmp\ui\MainMenu.lua" "%patchfolder%\UI\MainMenu.lua" > nul
copy /y "%patchfolder%\tmp\ui\MainMenu.xml" "%patchfolder%\UI\MainMenu.xml" > nul
copy /y "%patchfolder%\tmp\ui\EconomicGeneralInfo.lua" "%patchfolder%\UI\EconomicGeneralInfo.lua" > nul
copy /y "%patchfolder%\tmp\ui\ChooseInternationalTradeRoutePopup.lua" "%patchfolder%\UI\ChooseInternationalTradeRoutePopup.lua" > nul
copy /y "%patchfolder%\tmp\ui\SelectCivilization.xml" "%patchfolder%\UI\SelectCivilization.xml" > nul
copy /y "%patchfolder%\tmp\ui\SelectCivilization.lua" "%patchfolder%\UI\SelectCivilization.lua" > nul
copy /y "%patchfolder%\tmp\ui\EUI_tooltip_library.lua" "%patchfolder%\UI\EUI_tooltip_library.lua" > nul
copy /y "%patchfolder%\tmp\ui\EUI_unit_include.lua" "%patchfolder%\UI\EUI_unit_include.lua" > nul
copy /y "%patchfolder%\tmp\ui\EUI_context.xml" "%patchfolder%\UI\EUI_context.xml" > nul
copy /y "%patchfolder%\tmp\ui\EUI_context.lua" "%patchfolder%\UI\EUI_context.lua" > nul
copy /y "%patchfolder%\tmp\ui\EUI_core_library.lua" "%patchfolder%\UI\EUI_core_library.lua" > nul
copy /y "%patchfolder%\tmp\ui\EUI_tooltips.lua" "%patchfolder%\UI\EUI_tooltips.lua" > nul
copy /y "%patchfolder%\tmp\ui\EUI_utilities.lua" "%patchfolder%\UI\EUI_utilities.lua" > nul
copy /y "%patchfolder%\tmp\ui\PopulateUniques.lua" "%patchfolder%\UI\PopulateUniques.lua" > nul
copy /y "%patchfolder%\tmp\ui\GameSetupScreen.xml" "%patchfolder%\UI\GameSetupScreen.xml" > nul
copy /y "%patchfolder%\tmp\ui\GameSetupScreen.lua" "%patchfolder%\UI\GameSetupScreen.lua" > nul
copy /y "%patchfolder%\tmp\ui\LoadScreen.xml" "%patchfolder%\UI\LoadScreen.xml" > nul
copy /y "%patchfolder%\tmp\ui\LoadScreen.lua" "%patchfolder%\UI\LoadScreen.lua" > nul
copy /y "%patchfolder%\tmp\ui\FrontEnd.lua" "%patchfolder%\UI\FrontEnd.lua" > nul
copy /y "%patchfolder%\tmp\ui\ActionInfoPanel.lua" "%patchfolder%\UI\ActionInfoPanel.lua" > nul

REM -------------------------------------------------
set text="-- destroy: check fix for need to update plot & cargo & airbase"
FIND %text% "UI_bc1\UnitFlagManager\UnitFlagManager.lua" > nul 2>&1 && (
  copy /y "%patchfolder%\tmp\eui\UnitFlagManager.xml" "%patchfolder%\UI\UnitFlagManager.xml" > nul
  copy /y "%patchfolder%\tmp\eui\UnitFlagManager.lua" "%patchfolder%\UI\UnitFlagManager.lua" > nul
) || (
  copy /y "%patchfolder%\tmp\ui\UnitFlagManager.xml" "%patchfolder%\UI\UnitFlagManager.xml" > nul
  copy /y "%patchfolder%\tmp\ui\UnitFlagManager.lua" "%patchfolder%\UI\UnitFlagManager.lua" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\Improvements\SocialPolicyPopup.lua" (
  copy /y "%patchfolder%\tmp\eui\SocialPolicyPopup.lua" "%patchfolder%\UI\SocialPolicyPopup.lua" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\SocialPolicyPopup.lua" "%patchfolder%\UI\SocialPolicyPopup.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "UI_bc1\NotificationPanel\DiploList.lua" (
  copy /y "%patchfolder%\tmp\ui\DiploList.lua" "%patchfolder%\UI\DiploList.lua" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\LeaderHead\TradeLogic.lua" (
  copy /y "%patchfolder%\tmp\eui\TradeLogic.lua" "%patchfolder%\UI\TradeLogic.lua" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\TradeLogic.lua" "%patchfolder%\UI\TradeLogic.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "UI_bc1\ToolTips\InfoTooltipInclude.lua" (
  copy /y "%patchfolder%\tmp\ui\InfoTooltipInclude.lua" "%patchfolder%\UI\InfoTooltipInclude.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "UI_bc1\PlotHelp\PlotHelpManager.lua" (
  copy /y "%patchfolder%\tmp\ui\PlotHelpManager.lua" "%patchfolder%\UI\PlotHelpManager.lua" > nul
  copy /y "%patchfolder%\tmp\ui\PlotMouseoverInclude.lua" "%patchfolder%\UI\PlotMouseoverInclude.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "UI_bc1\PlotHelp\PlotHelpManager.xml" (
  copy /y "%patchfolder%\tmp\ui\PlotHelpManager.xml" "%patchfolder%\UI\PlotHelpManager.xml" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\ToolTips\TechButtonInclude.lua" (
  copy /y "%patchfolder%\tmp\eui\TechButtonInclude.lua" "%patchfolder%\UI\TechButtonInclude.lua" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\TechButtonInclude.lua" "%patchfolder%\UI\TechButtonInclude.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "UI_bc1\CityView\ProductionPopup.lua" (
  copy /y "%patchfolder%\tmp\ui\ProductionPopup.lua" "%patchfolder%\UI\ProductionPopup.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "UI_bc1\TechTree\TechPopup.lua" (
  copy /y "%patchfolder%\tmp\ui\TechPopup.lua" "%patchfolder%\UI\TechPopup.lua" > nul
)
REM -------------------------------------------------
set text="-- modified by bc1 from Civ V 1.0.3.276 code"
FIND %text% "UI_bc1\UnitPanel\UnitPanel.lua" > nul 2>&1 && (
  copy /y "%patchfolder%\tmp\eui\UnitPanel.lua" "%patchfolder%\UI\UnitPanel.lua" > nul
  copy /y "%patchfolder%\tmp\eui\UnitPanel.xml" "%patchfolder%\UI\UnitPanel.xml" > nul
  copy /y "%patchfolder%\tmp\eui\UnitPanel_small.xml" "%patchfolder%\UI\UnitPanel_small.xml" > nul
) || (
  copy /y "%patchfolder%\tmp\ui\UnitPanel.lua" "%patchfolder%\UI\UnitPanel.lua" > nul
  copy /y "%patchfolder%\tmp\ui\UnitPanel.xml" "%patchfolder%\UI\UnitPanel.xml" > nul
  copy /y "%patchfolder%\tmp\ui\UnitPanel_small.xml" "%patchfolder%\UI\UnitPanel_small.xml" > nul
)
REM -------------------------------------------------
set text="-- modified by bc1 from 1.0.3.144 brave new world code"
FIND %text% "UI_bc1\CityStatePopup\CityStateDiploPopup.lua" > nul 2>&1 && (
  copy /y "%patchfolder%\tmp\eui\CityStateDiploPopup.lua" "%patchfolder%\UI\CityStateDiploPopup.lua" > nul
  copy /y "%patchfolder%\tmp\eui\CityStateDiploPopup.xml" "%patchfolder%\UI\CityStateDiploPopup.xml" > nul
) || (
  copy /y "%patchfolder%\tmp\ui\CityStateDiploPopup.lua" "%patchfolder%\UI\CityStateDiploPopup.lua" > nul
  copy /y "%patchfolder%\tmp\ui\CityStateDiploPopup.xml" "%patchfolder%\UI\CityStateDiploPopup.xml" > nul
)
REM -------------------------------------------------
set text="-- coded by bc1 from 1.0.3.276 brave new world code"
FIND %text% "UI_bc1\CityView\CityView.lua" > nul 2>&1 && (
  copy /y "%patchfolder%\tmp\eui\CityView.lua" "%patchfolder%\UI\CityView.lua" > nul
) || (
  copy /y "%patchfolder%\tmp\ui\CityView.lua" "%patchfolder%\UI\CityView.lua" > nul
)
REM -------------------------------------------------
set text="-- coded by bc1 from Civ V 1.0.3.276 code"
FIND %text% "UI_bc1\TopPanel\TopPanel.lua" > nul 2>&1 && (
  copy /y "%patchfolder%\tmp\eui\TopPanel.lua" "%patchfolder%\UI\TopPanel.lua" > nul
) || (
  copy /y "%patchfolder%\tmp\ui\TopPanel.lua" "%patchfolder%\UI\TopPanel.lua" > nul
)
REM -------------------------------------------------
set text="Game.SelectionListGameNetMessage( GameMessageTypes.GAMEMESSAGE_DO_COMMAND, action.CommandType, action.CommandData, -1, 0, bAlt );"
FIND %text% "UI_bc1\Improvements\ConfirmCommandPopup.lua" > nul 2>&1 && (
  copy /y "%patchfolder%\tmp\eui\ConfirmCommandPopup.lua" "%patchfolder%\UI\ConfirmCommandPopup.lua" > nul
) || (
  copy /y "%patchfolder%\tmp\ui\ConfirmCommandPopup.lua" "%patchfolder%\UI\ConfirmCommandPopup.lua" > nul
)
REM -------------------------------------------------
set text="-- coded by bc1 from Civ V 1.0.3.276 code"
FIND %text% "UI_bc1\TechTree\TechTree.lua" > nul 2>&1 && (
  copy /y "%patchfolder%\tmp\eui\TechTree.lua" "%patchfolder%\UI\TechTree.lua" > nul
) || (
  copy /y "%patchfolder%\tmp\ui\TechTree.lua" "%patchfolder%\UI\TechTree.lua" > nul
)
REM -------------------------------------------------
set text="-- modified by bc1 from 1.0.3.144 brave new world & civ BE code"
FIND %text% "UI_bc1\Core\CityStateStatusHelper.lua" > nul 2>&1 && (
  copy /y "%patchfolder%\tmp\eui\CityStateStatusHelper.lua" "%patchfolder%\UI\CityStateStatusHelper.lua" > nul
) || (
  copy /y "%patchfolder%\tmp\ui\CityStateStatusHelper.lua" "%patchfolder%\UI\CityStateStatusHelper.lua" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\NotificationPanel\NotificationPanel.lua" (
  copy /y "%patchfolder%\tmp\eui\NotificationPanel.lua" "%patchfolder%\UI\NotificationPanel.lua" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\NotificationPanel.lua" "%patchfolder%\UI\NotificationPanel.lua" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\NotificationPanel\NotificationPanel.xml" (
  copy /y "%patchfolder%\tmp\eui\NotificationPanel.xml" "%patchfolder%\UI\NotificationPanel.xml" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\NotificationPanel.xml" "%patchfolder%\UI\NotificationPanel.xml" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\Options\OptionsMenu.lua" (
  copy /y "%patchfolder%\tmp\eui\OptionsMenu.lua.ignore" "%patchfolder%\UI\OptionsMenu.lua" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\OptionsMenu.lua.ignore" "%patchfolder%\UI\OptionsMenu.lua" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\Options\OptionsMenu.xml" (
  copy /y "%patchfolder%\tmp\eui\OptionsMenu.xml.ignore" "%patchfolder%\UI\OptionsMenu.xml" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\OptionsMenu.xml.ignore" "%patchfolder%\UI\OptionsMenu.xml" > nul
)
REM -------------------------------------------------
set text="CityBannerProductionBox = function( city )"
IF EXIST "UI_bc1\CityBanners\CityBannerManager.lua" (
  FIND %text% "UI_bc1\CityBanners\CityBannerManager.lua" > nul 2>&1 && (
    copy /y "%patchfolder%\tmp\eui\CityBannerManager_1.lua" "%patchfolder%\UI\CityBannerManager.lua" > nul
    copy /y "%patchfolder%\tmp\eui\CityBannerManager_1.xml" "%patchfolder%\UI\CityBannerManager.xml" > nul
  ) || (
    copy /y "%patchfolder%\tmp\eui\CityBannerManager_2.lua" "%patchfolder%\UI\CityBannerManager.lua" > nul
    copy /y "%patchfolder%\tmp\eui\CityBannerManager_2.xml" "%patchfolder%\UI\CityBannerManager.xml" > nul
  )
) ELSE (
  copy /y "%patchfolder%\tmp\ui\CityBannerManager.lua" "%patchfolder%\UI\CityBannerManager.lua" > nul
  copy /y "%patchfolder%\tmp\ui\CityBannerManager.xml" "%patchfolder%\UI\CityBannerManager.xml" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\NotificationPanel\DiploCorner.xml" (
  copy /y "%patchfolder%\tmp\eui\DiploCorner.lua" "%patchfolder%\UI\DiploCorner.lua" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\DiploCorner.lua" "%patchfolder%\UI\DiploCorner.lua" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\NotificationPanel\DiploCorner.xml" (
  copy /y "%patchfolder%\tmp\eui\DiploCorner.xml" "%patchfolder%\UI\DiploCorner.xml" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\DiploCorner.xml" "%patchfolder%\UI\DiploCorner.xml" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\Improvements\WorldView.lua" (
  copy /y "%patchfolder%\tmp\eui\WorldView.lua" "%patchfolder%\UI\WorldView.lua" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\WorldView.lua" "%patchfolder%\UI\WorldView.lua" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\TopPanel\TopPanel.xml" (
  copy /y "%patchfolder%\tmp\eui\TopPanel.xml" "%patchfolder%\UI\TopPanel.xml" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\CityView\CityView.xml" (
  copy /y "%patchfolder%\tmp\eui\CityView.xml" "%patchfolder%\UI\CityView.xml" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\CityView.xml" "%patchfolder%\UI\CityView.xml" > nul
)
REM -------------------------------------------------
IF EXIST "UI_bc1\CityView\CityView_small.xml" (
  copy /y "%patchfolder%\tmp\eui\CityView_small.xml" "%patchfolder%\UI\CityView_small.xml" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\CityView_small.xml" "%patchfolder%\UI\CityView_small.xml" > nul
)

REM -------------------------------------------------
IF EXIST "UI_bc1\Improvements\YieldIconManager.lua" (
  copy /y "%patchfolder%\tmp\eui\YieldIconManager.lua" "%patchfolder%\UI\YieldIconManager.lua" > nul
) ELSE (
  copy /y "%patchfolder%\tmp\ui\YieldIconManager.lua" "%patchfolder%\UI\YieldIconManager.lua" > nul
)

EXIT
