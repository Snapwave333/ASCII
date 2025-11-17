param(
    [string]$BuildConfig = "Release"
)

Write-Host "=== NeonGlyph Build Environment Setup ==="

# Detect vswhere
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
  Write-Warning "vswhere.exe not found. Install Visual Studio or Visual Studio Build Tools."
} else {
  Write-Host "vswhere found at: $vswhere"
}

# Detect Visual Studio BuildTools installation path
$vsPath = $null
if (Test-Path $vswhere) {
  $vsPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath 2>$null
}
if ([string]::IsNullOrWhiteSpace($vsPath)) {
  Write-Warning "Visual Studio Build Tools not detected. Install VS 2022 Build Tools with C++ workload."
} else {
  Write-Host "VS installation path: $vsPath"
}

# Detect VsDevCmd.bat
$vsDevCmd = $null
if ($vsPath) {
  $vsDevCmdCandidate = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"
  if (Test-Path $vsDevCmdCandidate) { $vsDevCmd = $vsDevCmdCandidate }
}
if (-not $vsDevCmd) {
  Write-Warning "VsDevCmd.bat not found. Ensure VS 2022 Build Tools installed."
} else {
  Write-Host "VsDevCmd: $vsDevCmd"
}

# Check CMake
$cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
$cmakePath = $null
if ($cmakeCmd) { $cmakePath = $cmakeCmd.Path }
if (-not $cmakePath) {
  Write-Warning "CMake not found in PATH. Attempting to locate Python-provided CMake..."
  $py = Get-Command python -ErrorAction SilentlyContinue
  if ($py) {
    $cmakeBin = & python -c "import cmake, os; print(os.path.join(os.path.dirname(cmake.__file__), 'data','bin'))" 2>$null
    if ($cmakeBin -and (Test-Path $cmakeBin)) {
      Write-Host "Found Python CMake bin at: $cmakeBin"
      $env:PATH = "$env:PATH;$cmakeBin"
      $cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
      if ($cmakeCmd) { $cmakePath = $cmakeCmd.Path }
    }
  }
  if (-not $cmakePath) {
    Write-Warning "CMake still not available. Install Kitware CMake and add to PATH."
  }
} else {
  Write-Host "CMake: $cmakePath"
}

# Check Vulkan SDK
if (-not $env:VULKAN_SDK) {
  Write-Warning "VULKAN_SDK not set. Install Vulkan SDK and set VULKAN_SDK environment variable (e.g., C:\VulkanSDK\<version>)."
} else {
  Write-Host "VULKAN_SDK: $env:VULKAN_SDK"
}

# Summarize readiness
$ready = $true
if (-not $vsDevCmd) { $ready = $false }
if (-not $cmakePath) { $ready = $false }
if (-not $env:VULKAN_SDK) { $ready = $false }

if (-not $ready) {
  Write-Error "Build environment incomplete. Install missing components and re-run."
  exit 1
}

Write-Host "Environment checks passed. Initializing VS developer environment..."

# Run within VS dev environment and build
$buildDir = "build64"
cmd /c "call `"$vsDevCmd`" && cmake -S . -B $buildDir -A x64 -DWINDOWS_INTEGRATION=OFF -DBUILD_STORY_DEMO=OFF && cmake --build $buildDir --config $BuildConfig" | Write-Host

if ($LASTEXITCODE -ne 0) {
  Write-Error "CMake build failed with exit code $LASTEXITCODE"
  exit $LASTEXITCODE
}

Write-Host "Build completed successfully"
exit 0
