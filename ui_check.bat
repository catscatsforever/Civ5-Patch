@ECHO off
pushd "%~dp0"
cd ..
set patchfolder=Tournament Mod V10.5g
del /q "%cd%\%patchfolder%\UI\"
REM -------------------------------------------------
copy /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.lua" "%cd%\%patchfolder%\UI\CultureOverview.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.xml" "%cd%\%patchfolder%\UI\CultureOverview.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\EnemyUnitPanel.lua" "%cd%\%patchfolder%\UI\EnemyUnitPanel.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\InGame.lua" "%cd%\%patchfolder%\UI\InGame.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\JoiningRoom.lua" "%cd%\%patchfolder%\UI\JoiningRoom.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\VictoryProgress.xml" "%cd%\%patchfolder%\UI\VictoryProgress.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\VictoryProgress.lua" "%cd%\%patchfolder%\UI\VictoryProgress.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\MPList.lua" "%cd%\%patchfolder%\UI\MPList.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\MiniMapPanel.lua" "%cd%\%patchfolder%\UI\MiniMapPanel.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\MiniMapPanel.xml" "%cd%\%patchfolder%\UI\MiniMapPanel.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ChooseIdeologyPopup.lua" "%cd%\%patchfolder%\UI\ChooseIdeologyPopup.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\CCVotePopup.lua" "%cd%\%patchfolder%\UI\CCVotePopup.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\CCVotePopup.xml" "%cd%\%patchfolder%\UI\CCVotePopup.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\EndGameMenu.lua" "%cd%\%patchfolder%\UI\EndGameMenu.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ProposalChartPopup.lua" "%cd%\%patchfolder%\UI\ProposalChartPopup.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ProposalChartPopup.xml" "%cd%\%patchfolder%\UI\ProposalChartPopup.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\AdvancedSetup.lua" "%cd%\%patchfolder%\UI\AdvancedSetup.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\StagingRoom.lua" "%cd%\%patchfolder%\UI\StagingRoom.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\StagingRoom.xml" "%cd%\%patchfolder%\UI\StagingRoom.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\MPGameDefaults.lua" "%cd%\%patchfolder%\UI\MPGameDefaults.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\MPGameOptions.lua" "%cd%\%patchfolder%\UI\MPGameOptions.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\MPGameSetupScreen.xml" "%cd%\%patchfolder%\UI\MPGameSetupScreen.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\MPTurnPanel.lua" "%cd%\%patchfolder%\UI\MPTurnPanel.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\MPTurnPanel.xml" "%cd%\%patchfolder%\UI\MPTurnPanel.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\CivilopediaScreen.lua" "%cd%\%patchfolder%\UI\CivilopediaScreen.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\GameMenu.lua" "%cd%\%patchfolder%\UI\GameMenu.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\Demographics.lua" "%cd%\%patchfolder%\UI\Demographics.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\Demographics.xml" "%cd%\%patchfolder%\UI\Demographics.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\Bombardment.lua" "%cd%\%patchfolder%\UI\Bombardment.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ChoosePantheonPopup.lua" "%cd%\%patchfolder%\UI\ChoosePantheonPopup.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ChooseReligionPopup.lua" "%cd%\%patchfolder%\UI\ChooseReligionPopup.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ReplayViewer.lua" "%cd%\%patchfolder%\UI\ReplayViewer.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ReplayViewer.xml" "%cd%\%patchfolder%\UI\ReplayViewer.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ReligionOverview.lua" "%cd%\%patchfolder%\UI\ReligionOverview.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ReligionOverview.xml" "%cd%\%patchfolder%\UI\ReligionOverview.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\EspionageOverview.lua" "%cd%\%patchfolder%\UI\EspionageOverview.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\CityList.lua" "%cd%\%patchfolder%\UI\CityList.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\CityList.xml" "%cd%\%patchfolder%\UI\CityList.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\GPList.lua" "%cd%\%patchfolder%\UI\GPList.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\GPList.xml" "%cd%\%patchfolder%\UI\GPList.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ResourceList.lua" "%cd%\%patchfolder%\UI\ResourceList.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ResourceList.xml" "%cd%\%patchfolder%\UI\ResourceList.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\UnitList.lua" "%cd%\%patchfolder%\UI\UnitList.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\UnitList.xml" "%cd%\%patchfolder%\UI\UnitList.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\Highlights.xml" "%cd%\%patchfolder%\UI\Highlights.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\NetworkKickedPopup.lua" "%cd%\%patchfolder%\UI\NetworkKickedPopup.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\SimpleDiploTrade.lua" "%cd%\%patchfolder%\UI\SimpleDiploTrade.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\SimpleDiploTrade.xml" "%cd%\%patchfolder%\UI\SimpleDiploTrade.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\TradeRouteHelpers.lua" "%cd%\%patchfolder%\UI\TradeRouteHelpers.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\SocialPolicyPopup.xml" "%cd%\%patchfolder%\UI\SocialPolicyPopup.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\TechTree.xml" "%cd%\%patchfolder%\UI\TechTree.xml" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\TechHelpInclude.lua" "%cd%\%patchfolder%\UI\TechHelpInclude.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\AnnexCityPopup.lua" "%cd%\%patchfolder%\UI\AnnexCityPopup.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ChooseMayaBonus.lua" "%cd%\%patchfolder%\UI\ChooseMayaBonus.lua" > nul
copy /y "%cd%\%patchfolder%\tmp\ui\ChooseMayaBonus.xml" "%cd%\%patchfolder%\UI\ChooseMayaBonus.xml" > nul

REM -------------------------------------------------
set text="-- destroy: check fix for need to update plot & cargo & airbase"
FIND %text% "%cd%\UI_bc1\UnitFlagManager\UnitFlagManager.lua" > nul 2>&1 && (
  copy /y "%cd%\%patchfolder%\tmp\eui\UnitFlagManager.xml" "%cd%\%patchfolder%\UI\UnitFlagManager.xml" > nul
  copy /y "%cd%\%patchfolder%\tmp\eui\UnitFlagManager.lua" "%cd%\%patchfolder%\UI\UnitFlagManager.lua" > nul
) || (
  copy /y "%cd%\%patchfolder%\tmp\ui\UnitFlagManager.xml" "%cd%\%patchfolder%\UI\UnitFlagManager.xml" > nul
  copy /y "%cd%\%patchfolder%\tmp\ui\UnitFlagManager.lua" "%cd%\%patchfolder%\UI\UnitFlagManager.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\GameSetup\SelectCivilization.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\SelectCivilization.lua" "%cd%\%patchfolder%\UI\SelectCivilization.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\SelectCivilization.lua" "%cd%\%patchfolder%\UI\SelectCivilization.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\FrontEnd\FrontEnd.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\FrontEnd.lua" "%cd%\%patchfolder%\UI\FrontEnd.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\FrontEnd.lua" "%cd%\%patchfolder%\UI\FrontEnd.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\Improvements\SocialPolicyPopup.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\SocialPolicyPopup.lua" "%cd%\%patchfolder%\UI\SocialPolicyPopup.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\SocialPolicyPopup.lua" "%cd%\%patchfolder%\UI\SocialPolicyPopup.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "%cd%\UI_bc1\NotificationPanel\DiploList.lua" (
  copy /y "%cd%\%patchfolder%\tmp\ui\DiploList.lua" "%cd%\%patchfolder%\UI\DiploList.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\LeaderHead\TradeLogic.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\TradeLogic.lua" "%cd%\%patchfolder%\UI\TradeLogic.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\TradeLogic.lua" "%cd%\%patchfolder%\UI\TradeLogic.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\ToolTips\InfoTooltipInclude.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\EUI_tooltip_library.lua" "%cd%\%patchfolder%\UI\EUI_tooltip_library.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\InfoTooltipInclude.lua" "%cd%\%patchfolder%\UI\InfoTooltipInclude.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\ToolTips\InfoTooltipInclude.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\EUI_unit_include.lua" "%cd%\%patchfolder%\UI\EUI_unit_include.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "%cd%\UI_bc1\PlotHelp\PlotHelpManager.lua" (
  copy /y "%cd%\%patchfolder%\tmp\ui\PlotHelpManager.lua" "%cd%\%patchfolder%\UI\PlotHelpManager.lua" > nul
  copy /y "%cd%\%patchfolder%\tmp\ui\PlotMouseoverInclude.lua" "%cd%\%patchfolder%\UI\PlotMouseoverInclude.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "%cd%\UI_bc1\PlotHelp\PlotHelpManager.xml" (
  copy /y "%cd%\%patchfolder%\tmp\ui\PlotHelpManager.xml" "%cd%\%patchfolder%\UI\PlotHelpManager.xml" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\ToolTips\TechButtonInclude.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\TechButtonInclude.lua" "%cd%\%patchfolder%\UI\TechButtonInclude.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\TechButtonInclude.lua" "%cd%\%patchfolder%\UI\TechButtonInclude.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "%cd%\UI_bc1\CityView\ProductionPopup.lua" (
  copy /y "%cd%\%patchfolder%\tmp\ui\ProductionPopup.lua" "%cd%\%patchfolder%\UI\ProductionPopup.lua" > nul
)
REM -------------------------------------------------
IF NOT EXIST "%cd%\UI_bc1\TechTree\TechPopup.lua" (
  copy /y "%cd%\%patchfolder%\tmp\ui\TechPopup.lua" "%cd%\%patchfolder%\UI\TechPopup.lua" > nul
)
REM -------------------------------------------------
set text="-- modified by bc1 from Civ V 1.0.3.276 code"
FIND %text% "%cd%\UI_bc1\UnitPanel\UnitPanel.lua" > nul 2>&1 && (
  copy /y "%cd%\%patchfolder%\tmp\eui\UnitPanel.lua" "%cd%\%patchfolder%\UI\UnitPanel.lua" > nul
) || (
  copy /y "%cd%\%patchfolder%\tmp\ui\UnitPanel.lua" "%cd%\%patchfolder%\UI\UnitPanel.lua" > nul
)
REM -------------------------------------------------
set text="-- modified by bc1 from 1.0.3.144 brave new world code"
FIND %text% "%cd%\UI_bc1\CityStatePopup\CityStateDiploPopup.lua" > nul 2>&1 && (
  copy /y "%cd%\%patchfolder%\tmp\eui\CityStateDiploPopup.lua" "%cd%\%patchfolder%\UI\CityStateDiploPopup.lua" > nul
  copy /y "%cd%\%patchfolder%\tmp\eui\CityStateDiploPopup.xml" "%cd%\%patchfolder%\UI\CityStateDiploPopup.xml" > nul
) || (
  copy /y "%cd%\%patchfolder%\tmp\ui\CityStateDiploPopup.lua" "%cd%\%patchfolder%\UI\CityStateDiploPopup.lua" > nul
  copy /y "%cd%\%patchfolder%\tmp\ui\CityStateDiploPopup.xml" "%cd%\%patchfolder%\UI\CityStateDiploPopup.xml" > nul
)
REM -------------------------------------------------
set text="-- coded by bc1 from 1.0.3.276 brave new world code"
FIND %text% "%cd%\UI_bc1\CityView\CityView.lua" > nul 2>&1 && (
  copy /y "%cd%\%patchfolder%\tmp\eui\CityView.lua" "%cd%\%patchfolder%\UI\CityView.lua" > nul
) || (
  copy /y "%cd%\%patchfolder%\tmp\ui\CityView.lua" "%cd%\%patchfolder%\UI\CityView.lua" > nul
)
REM -------------------------------------------------
set text="-- coded by bc1 from Civ V 1.0.3.276 code"
FIND %text% "%cd%\UI_bc1\TopPanel\TopPanel.lua" > nul 2>&1 && (
  copy /y "%cd%\%patchfolder%\tmp\eui\TopPanel.lua" "%cd%\%patchfolder%\UI\TopPanel.lua" > nul
) || (
  copy /y "%cd%\%patchfolder%\tmp\ui\TopPanel.lua" "%cd%\%patchfolder%\UI\TopPanel.lua" > nul
)
REM -------------------------------------------------
set text="Game.SelectionListGameNetMessage( GameMessageTypes.GAMEMESSAGE_DO_COMMAND, action.CommandType, action.CommandData, -1, 0, bAlt );"
FIND %text% "%cd%\UI_bc1\Improvements\ConfirmCommandPopup.lua" > nul 2>&1 && (
  copy /y "%cd%\%patchfolder%\tmp\eui\ConfirmCommandPopup.lua" "%cd%\%patchfolder%\UI\ConfirmCommandPopup.lua" > nul
) || (
  copy /y "%cd%\%patchfolder%\tmp\ui\ConfirmCommandPopup.lua" "%cd%\%patchfolder%\UI\ConfirmCommandPopup.lua" > nul
)
REM -------------------------------------------------
set text="-- coded by bc1 from Civ V 1.0.3.276 code"
FIND %text% "%cd%\UI_bc1\TechTree\TechTree.lua" > nul 2>&1 && (
  copy /y "%cd%\%patchfolder%\tmp\eui\TechTree.lua" "%cd%\%patchfolder%\UI\TechTree.lua" > nul
) || (
  copy /y "%cd%\%patchfolder%\tmp\ui\TechTree.lua" "%cd%\%patchfolder%\UI\TechTree.lua" > nul
)
REM -------------------------------------------------
set text="-- modified by bc1 from 1.0.3.144 brave new world & civ BE code"
FIND %text% "%cd%\UI_bc1\Core\CityStateStatusHelper.lua" > nul 2>&1 && (
  copy /y "%cd%\%patchfolder%\tmp\eui\CityStateStatusHelper.lua" "%cd%\%patchfolder%\UI\CityStateStatusHelper.lua" > nul
) || (
  copy /y "%cd%\%patchfolder%\tmp\ui\CityStateStatusHelper.lua" "%cd%\%patchfolder%\UI\CityStateStatusHelper.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\NotificationPanel\NotificationPanel.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\NotificationPanel.lua" "%cd%\%patchfolder%\UI\NotificationPanel.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\NotificationPanel.lua" "%cd%\%patchfolder%\UI\NotificationPanel.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\NotificationPanel\NotificationPanel.xml" (
  copy /y "%cd%\%patchfolder%\tmp\eui\NotificationPanel.xml" "%cd%\%patchfolder%\UI\NotificationPanel.xml" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\NotificationPanel.xml" "%cd%\%patchfolder%\UI\NotificationPanel.xml" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\Options\OptionsMenu.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\OptionsMenu.lua.ignore" "%cd%\%patchfolder%\UI\OptionsMenu.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\OptionsMenu.lua.ignore" "%cd%\%patchfolder%\UI\OptionsMenu.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\Options\OptionsMenu.xml" (
  copy /y "%cd%\%patchfolder%\tmp\eui\OptionsMenu.xml.ignore" "%cd%\%patchfolder%\UI\OptionsMenu.xml" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\OptionsMenu.xml.ignore" "%cd%\%patchfolder%\UI\OptionsMenu.xml" > nul
)
REM -------------------------------------------------
set text="CityBannerProductionBox = function( city )"
IF EXIST "%cd%\UI_bc1\CityBanners\CityBannerManager.lua" (
  FIND %text% "%cd%\UI_bc1\CityBanners\CityBannerManager.lua" > nul 2>&1 && (
    copy /y "%cd%\%patchfolder%\tmp\eui\CityBannerManager_1.lua" "%cd%\%patchfolder%\UI\CityBannerManager.lua" > nul
    copy /y "%cd%\%patchfolder%\tmp\eui\CityBannerManager_1.xml" "%cd%\%patchfolder%\UI\CityBannerManager.xml" > nul
  ) || (
    copy /y "%cd%\%patchfolder%\tmp\eui\CityBannerManager_2.lua" "%cd%\%patchfolder%\UI\CityBannerManager.lua" > nul
    copy /y "%cd%\%patchfolder%\tmp\eui\CityBannerManager_2.xml" "%cd%\%patchfolder%\UI\CityBannerManager.xml" > nul
  )
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\CityBannerManager.lua" "%cd%\%patchfolder%\UI\CityBannerManager.lua" > nul
  copy /y "%cd%\%patchfolder%\tmp\ui\CityBannerManager.xml" "%cd%\%patchfolder%\UI\CityBannerManager.xml" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\NotificationPanel\DiploCorner.xml" (
  copy /y "%cd%\%patchfolder%\tmp\eui\DiploCorner.lua" "%cd%\%patchfolder%\UI\DiploCorner.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\DiploCorner.lua" "%cd%\%patchfolder%\UI\DiploCorner.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\NotificationPanel\DiploCorner.xml" (
  copy /y "%cd%\%patchfolder%\tmp\eui\DiploCorner.xml" "%cd%\%patchfolder%\UI\DiploCorner.xml" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\DiploCorner.xml" "%cd%\%patchfolder%\UI\DiploCorner.xml" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\Improvements\WorldView.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\WorldView.lua" "%cd%\%patchfolder%\UI\WorldView.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\WorldView.lua" "%cd%\%patchfolder%\UI\WorldView.lua" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\TopPanel\TopPanel.xml" (
  copy /y "%cd%\%patchfolder%\tmp\eui\TopPanel.xml" "%cd%\%patchfolder%\UI\TopPanel.xml" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\CityView\CityView.xml" (
  copy /y "%cd%\%patchfolder%\tmp\eui\CityView.xml" "%cd%\%patchfolder%\UI\CityView.xml" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\CityView.xml" "%cd%\%patchfolder%\UI\CityView.xml" > nul
)
REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\CityView\CityView_small.xml" (
  copy /y "%cd%\%patchfolder%\tmp\eui\CityView_small.xml" "%cd%\%patchfolder%\UI\CityView_small.xml" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\CityView_small.xml" "%cd%\%patchfolder%\UI\CityView_small.xml" > nul
)

REM -------------------------------------------------
IF EXIST "%cd%\UI_bc1\Improvements\YieldIconManager.lua" (
  copy /y "%cd%\%patchfolder%\tmp\eui\YieldIconManager.lua" "%cd%\%patchfolder%\UI\YieldIconManager.lua" > nul
) ELSE (
  copy /y "%cd%\%patchfolder%\tmp\ui\YieldIconManager.lua" "%cd%\%patchfolder%\UI\YieldIconManager.lua" > nul
)

EXIT
