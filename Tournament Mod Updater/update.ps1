# Set variables
$response = Invoke-RestMethod -Uri "https://api.github.com/repos/catscatsforever/Civ5-Patch/releases/latest"
$url = $response.assets[0].browser_download_url
$version = $response.tag_name
$fileName = "Tournament_Mod_V${version}.zip"
$path = $PSScriptRoot
$dest = "$path\${fileName}"
$folder = "$path\Tournament Mod V${version}"

# Set working directory to the directory where installer puts the script
Set-Location -Path "$path"

# Delete any existing mod folders in the current directory
Get-ChildItem $path -Recurse -Force -Directory "Tournament*" |
Remove-Item -Recurse -Force

# Start downloading latest release from public Github
$WebClient = New-Object System.Net.WebClient
$WebClient.DownloadFile($url, $dest)
$WebClient.Dispose()

# Extract the archive
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory($fileName, $path)

# Run ui_check.bat
Start-Process -FilePath "$folder\ui_check.bat" -WindowStyle Hidden -Wait

# Remove the archive
Remove-Item $fileName -Force

# Remove the script itself
Remove-Item $MyInvocation.MyCommand.Source -Force