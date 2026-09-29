@echo off
"C:\Users\hankw\Documents\Codex\ti-nspire-dev\cygwin64\bin\bash.exe" --login /cygdrive/c/Users/hankw/Documents/Codex/ti-nspire-dev/example/balatro/build.sh
if errorlevel 1 (echo Build failed.) else (echo Ready: balatro.tns)
pause
