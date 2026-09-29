@echo off
set "PATH=C:\Users\hankw\Documents\Codex\ti-nspire-dev\cygwin64\bin;%PATH%"
if not exist "%~dp0build\balatro-desktop.exe" (
  "C:\Users\hankw\Documents\Codex\ti-nspire-dev\cygwin64\bin\bash.exe" --login /cygdrive/c/Users/hankw/Documents/Codex/ti-nspire-dev/example/balatro/build.sh desktop
  if errorlevel 1 (pause & exit /b 1)
)
start "" /D "%~dp0" "%~dp0build\balatro-desktop.exe" "build/desktop-save.tns"
