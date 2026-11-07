@echo off
chcp 65001 >nul
title PS5 download0.dat Windows Tool
color 0A
cd /d "%~dp0"

:MENU
cls
echo ==========================================
echo        PS5 download0.dat TOOL
echo ==========================================
echo.
echo  1 - Extract download0.dat
echo  2 - Build / Patch patched_download0.dat
echo  3 - Check SleuthKit Tools
echo  4 - Exit
echo.
echo ==========================================
echo Current Folder:
echo %cd%
echo ==========================================
echo.

set /p choice=Choose Option: 

if "%choice%"=="1" goto EXTRACT
if "%choice%"=="2" goto PATCH
if "%choice%"=="3" goto CHECK
if "%choice%"=="4" exit
goto MENU

:CHECK
cls
echo [*] Checking tools...
echo.

if exist "sleuthkit\bin\fls.exe" (
    echo [OK] fls.exe found
) else (
    echo [ERROR] fls.exe not found
)

if exist "sleuthkit\bin\icat.exe" (
    echo [OK] icat.exe found
) else (
    echo [ERROR] icat.exe not found
)

if exist "sleuthkit\bin\istat.exe" (
    echo [OK] istat.exe found
) else (
    echo [ERROR] istat.exe not found
)

if exist "download0.dat" (
    echo [OK] download0.dat found
) else (
    echo [ERROR] download0.dat not found
)

if exist "extract_windows.py" (
    echo [OK] extract_windows.py found
) else (
    echo [ERROR] extract_windows.py not found
)

echo.
pause
goto MENU

:EXTRACT
cls
echo ==========================================
echo Extract download0.dat
echo ==========================================
echo.

if not exist "download0.dat" (
    echo [ERROR] download0.dat not found
    pause
    goto MENU
)

if not exist "extract_windows.py" (
    echo [ERROR] extract_windows.py not found
    pause
    goto MENU
)

echo 1 | python extract_windows.py

echo.
echo [DONE] Extract finished
pause
goto MENU

:PATCH
cls
echo ==========================================
echo Build / Patch download0.dat
echo ==========================================
echo.

if not exist "extracted" (
    echo [ERROR] extracted folder not found
    echo Please run option 1 first.
    pause
    goto MENU
)

if not exist "download0.dat" (
    echo [ERROR] download0.dat not found
    pause
    goto MENU
)

echo 2 | python extract_windows.py

echo.
echo [DONE] Patch finished
echo Output: patched_download0.dat
pause
goto MENU