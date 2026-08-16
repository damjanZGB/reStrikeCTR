param(
    [switch]$LaunchObs
)

$srcDll = "C:\Users\Damjan\source\repos\reStrikeOBS\build_x64\RelWithDebInfo\restrike.dll"
$destDll = "C:\Program Files\obs-studio\obs-plugins\64bit\restrike.dll"
$srcData = "C:\Users\Damjan\source\repos\reStrikeOBS\data"
$destData = "C:\Program Files\obs-studio\data\obs-plugins\restrike"

if (!(Test-Path $srcDll)) {
    Write-Error "Source DLL not found: $srcDll"
    exit 1
}

Write-Host "Checking for running OBS instances..."
$obsProcs = Get-Process -Name "obs64", "obs" -ErrorAction SilentlyContinue
if ($obsProcs) {
    Write-Host "Stopping OBS Studio..."
    $obsProcs | Stop-Process -Force
    Start-Sleep -Seconds 2
}

Write-Host "Copying fresh restrike.dll with Administrator elevation..."
$copyCmd = "Copy-Item -Path '$srcDll' -Destination '$destDll' -Force; Copy-Item -Path '$srcData\*' -Destination '$destData' -Recurse -Force -ErrorAction SilentlyContinue"
Start-Process powershell.exe -Verb RunAs -ArgumentList "-NoProfile -Command `"$copyCmd`"" -Wait

Start-Sleep -Seconds 1
$ver = (Get-Item $destDll).LastWriteTime
$len = (Get-Item $destDll).Length
Write-Host "Destination restrike.dll ($len bytes, timestamp: $ver)" -ForegroundColor Green

if ($LaunchObs) {
    Write-Host "Launching OBS Studio..."
    Start-Process "C:\Program Files\obs-studio\bin\64bit\obs64.exe" -WorkingDirectory "C:\Program Files\obs-studio\bin\64bit"
}
