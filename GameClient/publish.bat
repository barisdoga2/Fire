@echo off
setlocal

echo === Fire Publisher (Relative Paths) ===

set PROJECT_DIR=%~dp0..\
set OUTPUT_DIR=%PROJECT_DIR%\x64\Release
set PUBLISH_DIR=%PROJECT_DIR%\publish\Fire
set RES_SRC=%PROJECT_DIR%\res
set RES_DST=%PUBLISH_DIR%\res

echo Project dir: %PROJECT_DIR%
echo Output dir: %OUTPUT_DIR%
echo Publish dir: %PUBLISH_DIR%
echo.

REM --- CLEAN PUBLISH DIR ---
echo Cleaning publish directory...
if exist "%PUBLISH_DIR%" (
    rmdir /S /Q "%PUBLISH_DIR%"
)
mkdir "%PUBLISH_DIR%"
echo Done.
echo.

REM --- COPY EXECUTABLE & DLLs ---
echo Copying EXE and DLL files...
xcopy "%OUTPUT_DIR%\Fire.exe" "%PUBLISH_DIR%\" /Y >nul
xcopy "%OUTPUT_DIR%\*.dll" "%PUBLISH_DIR%\" /Y >nul
echo Done.
echo.

REM --- COPY RES FOLDER ---
echo Copying res folder...
xcopy "%RES_SRC%" "%RES_DST%" /E /I /Y >nul
echo Done.
echo.

echo === PUBLISH COMPLETE ===
pause
