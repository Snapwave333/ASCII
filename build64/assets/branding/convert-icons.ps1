# PowerShell script to convert SVG to ICO files
# This script requires Inkscape to be installed for SVG to PNG conversion
# and ImageMagick for PNG to ICO conversion

param(
    [string]$InkscapePath = "C:\Program Files\Inkscape\bin\inkscape.exe",
    [string]$ImageMagickPath = "C:\Program Files\ImageMagick-7.1.1-Q16-HDRI\magick.exe"
)

$brandingDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$assetsDir = Join-Path $brandingDir ".."

function Convert-SvgToIco {
    param(
        [string]$SvgFile,
        [string]$OutputFile,
        [int[]]$Sizes = @(16, 24, 32, 48, 64, 128, 256)
    )
    
    Write-Host "Converting $SvgFile to $OutputFile..."
    
    # Create temporary directory for PNG files
    $tempDir = [System.IO.Path]::GetTempPath() + [System.Guid]::NewGuid().ToString()
    New-Item -ItemType Directory -Path $tempDir | Out-Null
    
    try {
        $pngFiles = @()
        
        # Convert SVG to PNG at different sizes
        foreach ($size in $Sizes) {
            $pngFile = Join-Path $tempDir "icon_$size.png"
            
            if (Test-Path $InkscapePath) {
                & "$InkscapePath" --export-type=png --export-filename="$pngFile" --export-width=$size --export-height=$size --export-background-opacity=0 "$SvgFile"
            } else {
                Write-Warning "Inkscape not found at $InkscapePath. Creating placeholder PNG file."
                # Create a simple placeholder PNG (this is just a fallback)
                Copy-Item $SvgFile $pngFile
            }
            
            if (Test-Path $pngFile) {
                $pngFiles += $pngFile
            }
        }
        
        # Convert PNG files to ICO
        if (Test-Path $ImageMagickPath -and $pngFiles.Count -gt 0) {
            $pngList = $pngFiles -join " "
            & "$ImageMagickPath" $pngFiles -background transparent $OutputFile
            Write-Host "Successfully created $OutputFile"
        } else {
            Write-Warning "ImageMagick not found or no PNG files created. Using first PNG as fallback."
            if ($pngFiles.Count -gt 0) {
                Copy-Item $pngFiles[0] $OutputFile
            }
        }
        
    } finally {
        # Clean up temporary files
        if (Test-Path $tempDir) {
            Remove-Item -Recurse -Force $tempDir
        }
    }
}

# Convert main icon
$mainSvg = Join-Path $brandingDir "neonglyph-icon.svg"
$mainIco = Join-Path $brandingDir "neonglyph-icon-256.ico"
if (Test-Path $mainSvg) {
    Convert-SvgToIco -SvgFile $mainSvg -OutputFile $mainIco -Sizes @(16, 24, 32, 48, 64, 128, 256)
}

# Convert tray icon
$traySvg = Join-Path $brandingDir "neonglyph-tray-icon.svg"
$trayIco = Join-Path $brandingDir "neonglyph-tray-icon.ico"
if (Test-Path $traySvg) {
    Convert-SvgToIco -SvgFile $traySvg -OutputFile $trayIco -Sizes @(16, 24, 32)
}

Write-Host "Icon conversion complete!"