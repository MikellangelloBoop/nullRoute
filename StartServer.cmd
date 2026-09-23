@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\StartServer.ps1" %*
pause
