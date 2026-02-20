@echo off
setlocal
set "SCRIPT_DIR=%~dp0"

where powershell >nul 2>nul
if errorlevel 1 (
  echo [ERROR] Windows PowerShell was not found.
  echo Try running CalamityAffixes.UserPatch.exe from a terminal with arguments.
  pause
  exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build_user_patch_wizard.ps1" %*
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" (
  echo.
  echo UserPatch wizard failed. Read the message above and try again.
  pause
)
exit /b %EXIT_CODE%
