@ECHO off
pushd "%~dp0"
cd ..
set patchfolder=Tournament Mod V5.1a
ECHO Y | del "%cd%\%patchfolder%\UI\"
ECHO -------------------------------------------------
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.lua" "%cd%\%patchfolder%\UI\CultureOverview.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CultureOverview.xml" "%cd%\%patchfolder%\UI\CultureOverview.xml"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\EnemyUnitPanel.lua" "%cd%\%patchfolder%\UI\EnemyUnitPanel.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\JoiningRoom.lua" "%cd%\%patchfolder%\UI\JoiningRoom.lua"
ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\VictoryProgress.xml" "%cd%\%patchfolder%\UI\VictoryProgress.xml"
ECHO -------------------------------------------------
REM проверяет наличие файла в папке интерфейса, если файл есть ничего не делает, если нету помещает его копию в tm/ui
IF EXIST "%cd%\UI_bc1\UnitFlagManager\UnitFlagManager.lua" (
  ECHO UnitFlagManager.lua exists on EUI, skipping
) ELSE (
  ECHO UnitFlagManager.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\UnitFlagManager.lua" "%cd%\%patchfolder%\UI\UnitFlagManager.lua"
)
ECHO -------------------------------------------------
IF EXIST "%cd%\UI_bc1\PlotHelp\PlotHelpManager.lua" (
  ECHO PlotHelpManager.lua exists on EUI, skipping
) ELSE (
  ECHO PlotHelpManager.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\PlotHelpManager.lua" "%cd%\%patchfolder%\UI\PlotHelpManager.lua"
)
ECHO -------------------------------------------------
IF EXIST "%cd%\UI_bc1\PlotHelp\PlotHelpManager.xml" (
  ECHO PlotHelpManager.xml exists on EUI, skipping
) ELSE (
  ECHO PlotHelpManager.xml does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\PlotHelpManager.xml" "%cd%\%patchfolder%\UI\PlotHelpManager.xml"
)
ECHO -------------------------------------------------
IF EXIST "%cd%\UI_bc1\CityView\ProductionPopup.lua" (
  ECHO ProductionPopup.lua exists on EUI, skipping
) ELSE (
  ECHO ProductionPopup.lua does not exists on EUI, copying to TM
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\ProductionPopup.lua" "%cd%\%patchfolder%\UI\ProductionPopup.lua"
)
ECHO -------------------------------------------------
REM менять текст поиска под каждый файл
set text="-- modified by bc1 from 1.0.3.144 brave new world code"
FIND %text% "%cd%\UI_bc1\CityStatePopup\CityStateDiploPopup.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityStateDiploPopup.lua" "%cd%\%patchfolder%\UI\CityStateDiploPopup.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityStateDiploPopup.lua" "%cd%\%patchfolder%\UI\CityStateDiploPopup.lua"
)
ECHO -------------------------------------------------
set text="-- coded by bc1 from 1.0.3.276 brave new world code"
FIND %text% "%cd%\UI_bc1\CityView\CityView.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\CityView.lua" "%cd%\%patchfolder%\UI\CityView.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\CityView.lua" "%cd%\%patchfolder%\UI\CityView.lua"
)
ECHO -------------------------------------------------
set text="-- coded by bc1 from Civ V 1.0.3.276 code"
FIND %text% "%cd%\UI_bc1\TopPanel\TopPanel.lua" && (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\eui\TopPanel.lua" "%cd%\%patchfolder%\UI\TopPanel.lua"
) || (
  ECHO F | xcopy /s /y "%cd%\%patchfolder%\tmp\ui\TopPanel.lua" "%cd%\%patchfolder%\UI\TopPanel.lua"
)
EXIT