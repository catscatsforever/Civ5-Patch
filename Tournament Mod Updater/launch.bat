cmd /c start /min "" powershell.exe -executionpolicy bypass -WindowStyle Hidden -File .\update.ps1
Set _seconds=0
:waitloop
  :: Wait for 5 seconds
  Set /a "_seconds=_seconds+5">nul
  PING -n 11 127.0.0.1>nul
  :: If 30 seconds have elapsed exit the loop
  if %_seconds%==30 goto nextstep
  if exist .\update.ps1 goto waitloop
:nextstep
(goto) 2>nul & del "%~f0"