param(
    [string]$HexFile = "C:\Users\Damjan\source\repos\reStrikeCTR\restrike_ctr_vial.hex"
)

$avrdude = "$env:LOCALAPPDATA\QMK\Toolbox\avrdude.exe"
$conf    = "$env:LOCALAPPDATA\QMK\Toolbox\avrdude.conf"

if (-not (Test-Path $HexFile)) {
    Write-Error "Hex file not found at $HexFile"
    exit 1
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   ReStrike CTR - Automated Pro Micro Flasher" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Target HEX: $HexFile" -ForegroundColor Yellow
Write-Host "Waiting for Pro Micro bootloader COM port..." -ForegroundColor Yellow
Write-Host ">> PLEASE DOUBLE-TAP 'RST' TO 'GND' ON THE BOARD NOW <<" -ForegroundColor Green
Write-Host ""

$knownPorts = [System.IO.Ports.SerialPort]::GetPortNames()

$foundPort = $null
$startTime = Get-Date

while ($null -eq $foundPort) {
    $currentPorts = [System.IO.Ports.SerialPort]::GetPortNames()
    foreach ($p in $currentPorts) {
        if ($p -notin $knownPorts -and $p -match "COM[0-9]+") {
            $foundPort = $p
            break
        }
    }
    Start-Sleep -Milliseconds 50
    if ((Get-Date) - $startTime -gt (New-TimeSpan -Minutes 2)) {
        Write-Warning "Timed out waiting for new COM port."
        exit 1
    }
}

Write-Host "Detected bootloader on $foundPort! Connecting avrdude..." -ForegroundColor Cyan

# Retry avrdude for up to 3 attempts with 200ms backoff
$flashed = $false
for ($attempt = 1; $attempt -le 4; $attempt++) {
    Start-Sleep -Milliseconds 250
    Write-Host "Flashing attempt $attempt on $foundPort..." -ForegroundColor Green
    & $avrdude -C $conf -p atmega32u4 -c avr109 -b 57600 -P $foundPort -U "flash:w:${HexFile}:i"
    if ($LASTEXITCODE -eq 0) {
        $flashed = $true
        break
    }
}

if ($flashed) {
    Write-Host ""
    Write-Host "SUCCESS: Controller flashed and verified!" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Warning "Flashing failed on $foundPort."
}
