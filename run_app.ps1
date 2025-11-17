# Run DAW Project with error capture
Write-Host "=== Starting DAW Project ===" -ForegroundColor Cyan
Write-Host ""

$exePath = "build\src\main\Debug\DAWProject.exe"

if (-not (Test-Path $exePath))
{
    Write-Host "ERROR: Executable not found at: $exePath" -ForegroundColor Red
    Write-Host "Please build the project first:" -ForegroundColor Yellow
    Write-Host "  cd build" -ForegroundColor White
    Write-Host "  cmake --build . --config Debug --target DAWProject" -ForegroundColor White
    exit 1
}

Write-Host "Found executable: $exePath" -ForegroundColor Green
Write-Host "Starting application..." -ForegroundColor Yellow
Write-Host ""

# Run the application
$process = Start-Process -FilePath $exePath -PassThru -Wait -NoNewWindow

if ($process.ExitCode -ne 0)
{
    Write-Host ""
    Write-Host "Application exited with code: $($process.ExitCode)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting:" -ForegroundColor Yellow
    Write-Host "1. Open build\DAWProject.sln in Visual Studio" -ForegroundColor White
    Write-Host "2. Press F5 to run with debugging" -ForegroundColor White
    Write-Host "3. Check the Output window for errors" -ForegroundColor White
}
else
{
    Write-Host ""
    Write-Host "Application closed normally" -ForegroundColor Green
}

