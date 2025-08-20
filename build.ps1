# Simple PowerShell build script for the WinTaskBarAutoHideFix application.
# Assumes that the Developer Command Prompt for Visual Studio is available.

$ErrorActionPreference = 'Stop'

# --- Configuration ---
$executableName = "WinTaskBarAutoHideFix.exe"
$sourceFiles = @("src/main.cpp")
$resourceFile = "src/WinTaskBarAutoHideFix.rc"
$manifestFile = "src/WinTaskBarAutoHideFix.manifest"
$outputDir = "build"

# --- Script Body ---
Write-Host "Starting build for $executableName..."

# Check if cl.exe is available
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Host "Error: cl.exe not found. Please run this script from a Developer Command Prompt for Visual Studio."
    exit 1
}

# Create output directory if it doesn't exist
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# Compile the resource file
$resourceOutputFile = "$outputDir/resource.res"
Write-Host "Compiling resource file..."
rc.exe /fo $resourceOutputFile $resourceFile

# Compile the C++ source code
$objectFiles = @()
foreach ($file in $sourceFiles) {
    $objectFile = "$outputDir/" + [System.IO.Path]::GetFileNameWithoutExtension($file) + ".obj"
    $objectFiles += $objectFile
    Write-Host "Compiling C++ source file: $file..."
    cl.exe /c /EHsc /nologo /W3 /D "UNICODE" /D "_UNICODE" /Fo$objectFile $file
}

# Link everything together
$executablePath = "$outputDir/$executableName"
Write-Host "Linking..."
link.exe /NOLOGO /SUBSYSTEM:WINDOWS /OUT:$executablePath $objectFiles $resourceOutputFile /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /MANIFESTFILE:"$outputDir/app.manifest" user32.lib shell32.lib

# Embed the manifest
mt.exe -nologo -manifest "$outputDir/app.manifest" -outputresource:"$executablePath;#1"


Write-Host "Build successful! Executable is at: $executablePath"
