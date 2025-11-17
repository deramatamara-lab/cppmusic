# Disk Space Analyzer
# Identifies what's taking up space on your C: drive

Write-Host "=== Disk Space Analysis ===" -ForegroundColor Cyan
Write-Host ""

$drive = Get-Volume -DriveLetter C
$totalGB = [math]::Round($drive.Size / 1GB, 2)
$freeGB = [math]::Round($drive.FreeSpace / 1GB, 2)
$usedGB = $totalGB - $freeGB
$percentUsed = [math]::Round(($usedGB / $totalGB) * 100, 2)

Write-Host "C: Drive Status:" -ForegroundColor Yellow
Write-Host "  Total: $totalGB GB" -ForegroundColor White
Write-Host "  Used:  $usedGB GB ($percentUsed%)" -ForegroundColor $(if ($percentUsed -gt 90) { "Red" } else { "Yellow" })
Write-Host "  Free:  $freeGB GB" -ForegroundColor $(if ($freeGB -lt 10) { "Red" } else { "Green" })
Write-Host ""

if ($freeGB -lt 10) {
    Write-Host "WARNING: Less than 10GB free! System performance will be severely impacted!" -ForegroundColor Red
    Write-Host ""
}

Write-Host "Analyzing largest folders on C: drive..." -ForegroundColor Green
Write-Host "(This may take a few minutes...)" -ForegroundColor Yellow
Write-Host ""

$largeFolders = @(
    "C:\Users\$env:USERNAME\Downloads",
    "C:\Users\$env:USERNAME\Desktop",
    "C:\Users\$env:USERNAME\Documents",
    "C:\Users\$env:USERNAME\AppData\Local",
    "C:\Program Files",
    "C:\Program Files (x86)",
    "C:\Windows\Temp",
    "C:\Windows\SoftwareDistribution",
    "C:\Windows\WinSxS"
)

$results = @()

foreach ($folder in $largeFolders) {
    if (Test-Path $folder) {
        try {
            $size = (Get-ChildItem -Path $folder -Recurse -ErrorAction SilentlyContinue | 
                     Measure-Object -Property Length -Sum -ErrorAction SilentlyContinue).Sum
            if ($size) {
                $sizeGB = [math]::Round($size / 1GB, 2)
                $results += [PSCustomObject]@{
                    Folder = $folder
                    SizeGB = $sizeGB
                }
                Write-Host "  $folder : $sizeGB GB" -ForegroundColor $(if ($sizeGB -gt 20) { "Red" } else { "White" })
            }
        } catch {
            Write-Host "  $folder : (Access denied or error)" -ForegroundColor Gray
        }
    }
}

Write-Host ""
Write-Host "Top space consumers:" -ForegroundColor Green
$results | Sort-Object SizeGB -Descending | Select-Object -First 10 | ForEach-Object {
    Write-Host "  $($_.SizeGB) GB - $($_.Folder)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Quick Cleanup Suggestions ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Check Downloads folder for old files:" -ForegroundColor Yellow
Write-Host "   C:\Users\$env:USERNAME\Downloads" -ForegroundColor White
Write-Host ""
Write-Host "2. Check Desktop for large files:" -ForegroundColor Yellow
Write-Host "   C:\Users\$env:USERNAME\Desktop" -ForegroundColor White
Write-Host ""
Write-Host "3. Clear browser caches:" -ForegroundColor Yellow
Write-Host "   - Chrome: Settings > Privacy > Clear browsing data" -ForegroundColor White
Write-Host "   - Edge: Settings > Privacy > Clear browsing data" -ForegroundColor White
Write-Host ""
Write-Host "4. Uninstall unused programs:" -ForegroundColor Yellow
Write-Host "   Settings > Apps > Apps & features" -ForegroundColor White
Write-Host ""
Write-Host "5. Use Storage Sense:" -ForegroundColor Yellow
Write-Host "   Settings > System > Storage > Storage Sense" -ForegroundColor White
Write-Host ""
Write-Host "6. Check for duplicate files (use a duplicate finder tool)" -ForegroundColor Yellow
Write-Host ""

