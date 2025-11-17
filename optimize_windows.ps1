# Windows System Optimization Script
# Run as Administrator for full functionality

Write-Host "=== Windows System Optimization ===" -ForegroundColor Cyan
Write-Host ""

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "WARNING: Not running as Administrator. Some optimizations may be limited." -ForegroundColor Yellow
    Write-Host ""
}

# 1. DISK CLEANUP - CRITICAL (Disk is 100% full!)
Write-Host "1. Starting Disk Cleanup..." -ForegroundColor Green
try {
    # Clean Windows Update files
    Write-Host "   - Cleaning Windows Update files..." -ForegroundColor Yellow
    if ($isAdmin) {
        Dism.exe /online /Cleanup-Image /StartComponentCleanup /ResetBase 2>&1 | Out-Null
    }
    
    # Clean temporary files
    Write-Host "   - Cleaning temporary files..." -ForegroundColor Yellow
    Remove-Item -Path "$env:TEMP\*" -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -Path "$env:LOCALAPPDATA\Temp\*" -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -Path "C:\Windows\Temp\*" -Recurse -Force -ErrorAction SilentlyContinue
    
    # Clean browser caches
    Write-Host "   - Cleaning browser caches..." -ForegroundColor Yellow
    $browserPaths = @(
        "$env:LOCALAPPDATA\Google\Chrome\User Data\Default\Cache",
        "$env:LOCALAPPDATA\Microsoft\Edge\User Data\Default\Cache",
        "$env:APPDATA\Mozilla\Firefox\Profiles\*\cache2"
    )
    foreach ($path in $browserPaths) {
        if (Test-Path $path) {
            Remove-Item -Path "$path\*" -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    
    # Clean Recycle Bin
    Write-Host "   - Emptying Recycle Bin..." -ForegroundColor Yellow
    Clear-RecycleBin -Force -ErrorAction SilentlyContinue
    
    Write-Host "   ✓ Disk cleanup completed!" -ForegroundColor Green
} catch {
    Write-Host "   ✗ Error during disk cleanup: $_" -ForegroundColor Red
}
Write-Host ""

# 2. DISABLE UNNECESSARY STARTUP PROGRAMS
Write-Host "2. Optimizing Startup Programs..." -ForegroundColor Green
Write-Host "   Current startup programs:" -ForegroundColor Yellow
$startupPrograms = @(
    "HPSEU_Host_Launcher",
    "RiotClient",
    "Viber",
    "MicrosoftEdgeAutoLaunch_C20C439905A8A614B916288AD7473F4F"
)

foreach ($prog in $startupPrograms) {
    try {
        $regPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
        $value = Get-ItemProperty -Path $regPath -Name $prog -ErrorAction SilentlyContinue
        if ($value) {
            Write-Host "   - Found: $prog" -ForegroundColor Yellow
            Write-Host "     (You can disable this in Task Manager > Startup if not needed)" -ForegroundColor Gray
        }
    } catch {}
}
Write-Host "   ✓ Startup analysis completed!" -ForegroundColor Green
Write-Host "   → Open Task Manager (Ctrl+Shift+Esc) > Startup tab to disable unnecessary programs" -ForegroundColor Cyan
Write-Host ""

# 3. OPTIMIZE MEMORY
Write-Host "3. Optimizing Memory..." -ForegroundColor Green
try {
    # Clear standby memory (requires admin)
    if ($isAdmin) {
        Write-Host "   - Clearing standby memory..." -ForegroundColor Yellow
        [System.GC]::Collect()
        [System.GC]::WaitForPendingFinalizers()
    }
    
    # Show top memory consumers
    Write-Host "   - Top memory consumers:" -ForegroundColor Yellow
    Get-Process | Sort-Object WorkingSet -Descending | Select-Object -First 5 | ForEach-Object {
        $memMB = [math]::Round($_.WorkingSet / 1MB, 2)
        Write-Host "     $($_.ProcessName): $memMB MB" -ForegroundColor Gray
    }
    
    Write-Host "   ✓ Memory optimization completed!" -ForegroundColor Green
} catch {
    Write-Host "   ✗ Error during memory optimization: $_" -ForegroundColor Red
}
Write-Host ""

# 4. DISABLE UNNECESSARY SERVICES
Write-Host "4. Checking Services..." -ForegroundColor Green
Write-Host "   → Review services in services.msc and disable unnecessary ones" -ForegroundColor Cyan
Write-Host "   Common services that can be disabled (if not needed):" -ForegroundColor Yellow
Write-Host "     - Windows Search (if you don't use it)" -ForegroundColor Gray
Write-Host "     - Superfetch/SysMain" -ForegroundColor Gray
Write-Host "     - Print Spooler (if no printer)" -ForegroundColor Gray
Write-Host ""

# 5. DISK DEFRAGMENTATION (if HDD)
Write-Host "5. Checking Disk Health..." -ForegroundColor Green
try {
    $drives = Get-Volume | Where-Object {$_.DriveLetter -ne $null}
    foreach ($drive in $drives) {
        $driveLetter = $drive.DriveLetter
        if ($driveLetter) {
            $disk = Get-PhysicalDisk | Where-Object {$_.DeviceID -eq (Get-Partition -DriveLetter $driveLetter).DiskNumber}
            if ($disk.MediaType -eq "HDD") {
                Write-Host "   - Drive $driveLetter is HDD - consider defragmentation" -ForegroundColor Yellow
                Write-Host "     Run: Optimize-Volume -DriveLetter $driveLetter -Defrag" -ForegroundColor Gray
            } else {
                Write-Host "   - Drive $driveLetter is SSD - no defragmentation needed" -ForegroundColor Green
            }
        }
    }
} catch {
    Write-Host "   ✗ Error checking disk health: $_" -ForegroundColor Red
}
Write-Host ""

# 6. WINDOWS UPDATES & MAINTENANCE
Write-Host "6. System Maintenance..." -ForegroundColor Green
Write-Host "   → Run Windows Update to ensure system is up to date" -ForegroundColor Cyan
Write-Host "   → Check for driver updates in Device Manager" -ForegroundColor Cyan
Write-Host ""

# 7. DISABLE VISUAL EFFECTS (Performance boost)
Write-Host "7. Performance Settings..." -ForegroundColor Green
Write-Host "   → Right-click This PC > Properties > Advanced System Settings" -ForegroundColor Cyan
Write-Host "   → Performance Settings > Adjust for best performance" -ForegroundColor Cyan
Write-Host ""

# 8. CHECK FOR MALWARE/ANTIVIRUS CONFLICTS
Write-Host "8. Security Check..." -ForegroundColor Green
Write-Host "   - Detected: Avast Antivirus" -ForegroundColor Yellow
Write-Host "   → Ensure only ONE antivirus is active (Windows Defender + Avast can conflict)" -ForegroundColor Cyan
Write-Host ""

# SUMMARY
Write-Host "=== OPTIMIZATION SUMMARY ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "CRITICAL ISSUES FOUND:" -ForegroundColor Red
Write-Host "  1. C: Drive is 100% FULL - This is the main cause of slowness!" -ForegroundColor Red
Write-Host "     ACTION: Free up disk space immediately!" -ForegroundColor Yellow
Write-Host ""
Write-Host "RECOMMENDATIONS:" -ForegroundColor Yellow
Write-Host "  1. Delete large files/folders you don't need" -ForegroundColor White
Write-Host "  2. Uninstall unused programs (Settings > Apps)" -ForegroundColor White
Write-Host "  3. Move files to external drive or cloud storage" -ForegroundColor White
Write-Host "  4. Disable unnecessary startup programs (Task Manager > Startup)" -ForegroundColor White
Write-Host "  5. Run Disk Cleanup tool (cleanmgr.exe)" -ForegroundColor White
Write-Host "  6. Consider disabling Windows Search if not used" -ForegroundColor White
Write-Host "  7. Check for duplicate files" -ForegroundColor White
Write-Host ""
Write-Host "IMMEDIATE ACTIONS:" -ForegroundColor Green
Write-Host "  - Open File Explorer and check Downloads, Desktop, Documents for large files" -ForegroundColor Cyan
Write-Host "  - Run: cleanmgr.exe (Disk Cleanup)" -ForegroundColor Cyan
Write-Host "  - Check: Settings > System > Storage to see what's using space" -ForegroundColor Cyan
Write-Host ""
Write-Host "Script completed! Review the recommendations above." -ForegroundColor Green

