@echo off
setlocal EnableDelayedExpansion

:: Set the solution filename (must match the project name in CMakeLists.txt)
set "SLN_FILE=Celestials.sln"

:: ********************************************************************************
:: * This batch file must be run in the same folder as CMakeLists.txt. It is      *
:: * recommended to run it in a Developer PowerShell or Developer Command Prompt  *
:: * for Visual Studio to ensure all necessary Visual Studio files are found.     *
:: * Additionally, running this batch file with administrator privileges is       *
:: * strongly advised. This allows the script to create a symbolic link to the    *
:: * Visual Studio solution (.sln) file in the same folder as CMakeLists.txt.     *
:: *                                                                              *
:: * The script uses 'mklink' to create a symbolic link to the solution file      *
:: * because simply copying the .sln file to another folder will prevent Visual   *
:: * Studio from locating the project files referenced in the solution. If you do *
:: * not need a symbolic link to the solution file in the source directory, you   *
:: * can safely ignore the warning at the end of this script.                     *
:: *                                                                              *
:: * The only modification typically required is to update the solution filename  *
:: * near the top of this file to match the project name defined in your          *
:: * CMakeLists.txt file.                                                         *
:: *                                                                              *
:: * For example, if your CMakeLists.txt contains:                                *
:: *     project(ArtilleryGame CXX)                                               *
:: * then set the solution filename at the top of this batch file to:             *
:: *     @set "SLN_FILE=ArtilleryGame.sln"                                        *
:: *                                                                              *
:: * The solution filename will match the project name specified in               *
:: * CMakeLists.txt.                                                              *
:: *                                                                              *
:: * -----------------------------------------------------------------------------*
:: *                                                                              *
:: *       This batch file was created by TEK Nemesis for your convenience.       *
:: *               You may distribute, modify, and use it freely.                 *
:: *                                                                              *
:: ********************************************************************************

:: Set the working directory to the location of the batch file
cd /d "%~dp0"

:: Set the source directory to the current directory
set "SOURCE_DIR=%CD%"

:: Set the build directory as a subdirectory of the source directory
set "BUILD_DIR=%SOURCE_DIR%\build"

:: Set full path to the solution file created by CMake (in the build folder)
set "CMAKE_SLN_FILE=%BUILD_DIR%\%SLN_FILE%"

:: Set full path to the symbolic link solution file (in the source folder)
set "MLINK_SLN_FILE=%SOURCE_DIR%\%SLN_FILE%"

:: Create build directory if it doesn't exist
echo.
echo Creating build directory...
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
    if !ERRORLEVEL! neq 0 (
        echo ERROR: Failed to create build directory: %BUILD_DIR%
        goto :end
    )
)

:: Create output directories if they don't exist
echo.
echo Creating output directories...
if not exist "%SOURCE_DIR%\out\x64-Debug" (
    mkdir "%SOURCE_DIR%\out\x64-Debug"
    if !ERRORLEVEL! neq 0 (
        echo ERROR: Failed to create output directory: %SOURCE_DIR%\out\x64-Debug
        goto :end
    )
)
if not exist "%SOURCE_DIR%\out\x64-Release" (
    mkdir "%SOURCE_DIR%\out\x64-Release"
    if !ERRORLEVEL! neq 0 (
        echo ERROR: Failed to create output directory: %SOURCE_DIR%\out\x64-Release
        goto :end
    )
)

:: Generate solution in the build directory
echo.
echo Generating solution in build directory...
cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if !ERRORLEVEL! neq 0 goto :cmake_error

:: Check if the solution file exists in the build directory
if not exist "%CMAKE_SLN_FILE%" goto :cmake_error

echo.
echo Solution generated successfully in build directory!
echo Solution: %CMAKE_SLN_FILE%

:: Create a symbolic link to the solution file in the source directory
:: (Requires administrator privileges)
echo.
echo Creating symbolic link to solution file in source directory...
:: Remove existing symbolic link if it exists
if exist "%MLINK_SLN_FILE%" (
    del "%MLINK_SLN_FILE%"
)
mklink "%MLINK_SLN_FILE%" "%CMAKE_SLN_FILE%" || goto :mklink_error
if exist "%MLINK_SLN_FILE%" (
    echo Solution linked successfully!
    echo Solution: %MLINK_SLN_FILE%
) else (
    goto :mklink_error
)

:: Launch the symbolic link solution in Visual Studio
echo.
echo Launching Visual Studio with the solution in the source directory...
start "" "%MLINK_SLN_FILE%"
goto :end

:cmake_error
:: Cannot continue because CMake failed to create the solution file
echo.
echo ERROR: The solution file (%SLN_FILE%) was not found in the build directory!
echo.
echo Note: You may need to run this batch file in a Developer PowerShell or Developer Command Prompt.
echo       Additionally, administrator privileges are required to create a symbolic link to the solution file.
goto :end

:mklink_error
:: Failed to create symbolic link, but the original solution file exists
echo.
echo WARNING: Failed to create a symbolic link to the solution file in the source directory!
echo.
echo This step is optional but convenient for having a solution file in the source directory.
echo Re-run this batch file with administrator privileges to create the symbolic link.
echo.
echo Launching Visual Studio with the solution file in the build directory...
start "" "%CMAKE_SLN_FILE%"

:end
echo.
pause
endlocal