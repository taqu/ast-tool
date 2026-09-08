#Requires -Version 5.1
<#
.SYNOPSIS
    Build and package the ast-tool Windows x64 release artifact.

.DESCRIPTION
    Performs a clean Release build, runs smoke tests, packages the binary
    archive, and generates SHA256SUMS.

    Prerequisites: MSVC 2022+, CMake 3.11+ on PATH.

.PARAMETER Version
    Version string to embed in artifact names (e.g. "0.1.0").
    Defaults to the version reported by ast-tool --version after build.

.PARAMETER OutDir
    Directory to write artifacts to. Defaults to "release-artifacts".

.EXAMPLE
    .\scripts\package-windows.ps1
    .\scripts\package-windows.ps1 -Version 0.1.0 -OutDir out\artifacts
#>
param(
    [string]$Version = "",
    [string]$OutDir = "release-artifacts"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path $PSScriptRoot -Parent
Push-Location $RepoRoot

try {
    $BuildDir = "build-release"
    Write-Host "==> Cleaning release build directory ($BuildDir)..."
    if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }

    Write-Host "==> Configuring (Release)..."
    cmake -B $BuildDir -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    Write-Host "==> Building (Release)..."
    cmake --build $BuildDir --config Release
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed" }

    $Exe = Join-Path $RepoRoot "bin\ast-tool.exe"
    if (-not (Test-Path $Exe)) { throw "Expected executable not found: $Exe" }

    Write-Host "==> Smoke tests..."
    & $Exe --version
    if ($LASTEXITCODE -ne 0) { throw "--version failed" }
    & $Exe --help | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "--help failed" }

    if ($Version -eq "") {
        $VerOutput = & $Exe --version 2>&1
        $Version = ($VerOutput -replace '^ast-tool\s+', '').Trim()
        Write-Host "    Detected version: $Version"
    }

    $ArtifactName = "ast-tool-$Version-windows-x64"
    $StagingDir = Join-Path $RepoRoot "staging\$ArtifactName"

    Write-Host "==> Staging artifact: $ArtifactName..."
    if (Test-Path $StagingDir) { Remove-Item -Recurse -Force $StagingDir }
    New-Item -ItemType Directory -Force -Path $StagingDir | Out-Null

    Copy-Item "$RepoRoot\bin\ast-tool.exe" $StagingDir\
    $Dlls = Get-ChildItem "$RepoRoot\bin\*.dll" -ErrorAction SilentlyContinue
    foreach ($dll in $Dlls) { Copy-Item $dll.FullName $StagingDir\ }
    Copy-Item "$RepoRoot\README.md"    $StagingDir\
    Copy-Item "$RepoRoot\LICENSE"      $StagingDir\
    Copy-Item "$RepoRoot\CHANGELOG.md" $StagingDir\
    Copy-Item "$RepoRoot\NOTICE"       $StagingDir\

    if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Force -Path $OutDir | Out-Null }
    $ZipPath = Join-Path $OutDir "$ArtifactName.zip"
    if (Test-Path $ZipPath) { Remove-Item -Force $ZipPath }

    Write-Host "==> Creating archive: $ZipPath..."
    Compress-Archive -Path $StagingDir -DestinationPath $ZipPath

    Write-Host "==> Generating SHA256SUMS..."
    $Hash = (Get-FileHash $ZipPath -Algorithm SHA256).Hash.ToLower()
    $ChecksumFile = Join-Path $OutDir "SHA256SUMS"
    "$Hash  $ArtifactName.zip" | Out-File -Encoding utf8 $ChecksumFile -Append

    Write-Host "==> Verifying archive..."
    $TestDir = Join-Path $env:TEMP "ast-tool-verify-$([System.Guid]::NewGuid().ToString('N').Substring(0,8))"
    Expand-Archive -Path $ZipPath -DestinationPath $TestDir
    $TestExe = Join-Path $TestDir "$ArtifactName\ast-tool.exe"
    & $TestExe --version
    if ($LASTEXITCODE -ne 0) { throw "Post-packaging smoke test failed (--version)" }
    & $TestExe --help | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Post-packaging smoke test failed (--help)" }
    Remove-Item -Recurse -Force $TestDir

    Write-Host ""
    Write-Host "==> Done."
    Write-Host "    Artifact : $ZipPath"
    Write-Host "    Checksums: $ChecksumFile"
    Write-Host "    Size     : $([math]::Round((Get-Item $ZipPath).Length / 1MB, 2)) MB"

} finally {
    Pop-Location
    if (Test-Path "staging") { Remove-Item -Recurse -Force "staging" }
}
