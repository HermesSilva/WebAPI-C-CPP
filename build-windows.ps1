#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build script para Windows x64
.DESCRIPTION
    Compila o TootegaWebAPI para Windows x64 e prepara a pasta de distribuicao
.PARAMETER Clean
    Remove a pasta dist antes de compilar
.PARAMETER NoCopy
    Nao copia para bin\x64\Release (apenas dist)
.EXAMPLE
    .\build-windows.ps1
    .\build-windows.ps1 -Clean
#>
param(
    [switch]$Clean,
    [switch]$NoCopy
)

$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
$DistDir = Join-Path $ProjectRoot "dist\windows-x64"
$BinDir = Join-Path $ProjectRoot "bin\x64\Release"

# Kill running server if exists
$runningServer = Get-Process -Name "TootegaWebAPI" -ErrorAction SilentlyContinue
if ($runningServer) {
    Write-Host "Parando servidor em execucao..." -ForegroundColor Yellow
    Stop-Process -Name "TootegaWebAPI" -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
}

function Write-Step { param($msg) Write-Host "`n>> $msg" -ForegroundColor Cyan }
function Write-OK { param($msg) Write-Host "[OK] $msg" -ForegroundColor Green }
function Write-Warn { param($msg) Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-Err { param($msg) Write-Host "[ERROR] $msg" -ForegroundColor Red; exit 1 }

Write-Host "`n============================================" -ForegroundColor Blue
Write-Host "    Tootega WebAPI - Windows x64 Build" -ForegroundColor Blue
Write-Host "============================================`n" -ForegroundColor Blue

# Clean if requested
if ($Clean) {
    Write-Step "Limpando pasta de distribuicao..."
    if (Test-Path $DistDir) {
        Remove-Item -Recurse -Force $DistDir
    }
    Write-OK "Pasta limpa!"
}

# Create dist directory
Write-Step "Preparando diretorios..."
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
Write-OK "Diretorio: $DistDir"

# Compile TypeScript
Write-Step "Compilando TypeScript..."
$WebDir = Join-Path $ProjectRoot "web"
if (Test-Path (Join-Path $WebDir "package.json")) {
    Push-Location $WebDir
    try {
        if (-not (Test-Path "node_modules")) {
            Write-Host "  Instalando dependencias npm..."
            npm install --silent 2>$null
        }
        npm run build 2>$null
        if ($LASTEXITCODE -eq 0) {
            Write-OK "TypeScript compilado!"
        }
        else {
            Write-Warn "Falha ao compilar TypeScript"
        }
    }
    finally {
        Pop-Location
    }
}

# Find MSBuild
Write-Step "Localizando MSBuild..."
$MSBuildPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe"
)

$MSBuildPath = $null
foreach ($path in $MSBuildPaths) {
    if (Test-Path $path) {
        $MSBuildPath = $path
        break
    }
}

# Try vswhere as fallback
if (-not $MSBuildPath) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -property installationPath
        if ($vsPath) {
            $MSBuildPath = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
        }
    }
}

if (-not $MSBuildPath -or -not (Test-Path $MSBuildPath)) {
    Write-Err "MSBuild nao encontrado! Instale o Visual Studio com workload C++."
}

Write-OK "MSBuild: $MSBuildPath"

# Build
Write-Step "Compilando Windows x64 Release..."
& $MSBuildPath "$ProjectRoot\TootegaWebAPI.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Build /verbosity:minimal

if ($LASTEXITCODE -ne 0) {
    Write-Err "Falha na compilacao!"
}

$ExePath = Join-Path $BinDir "TootegaWebAPI.exe"
if (-not (Test-Path $ExePath)) {
    Write-Err "Executavel nao encontrado: $ExePath"
}

Write-OK "Compilacao concluida!"

# Copy to dist
Write-Step "Preparando distribuicao..."

# Copy executable
Copy-Item $ExePath $DistDir -Force
Write-OK "Executavel copiado"

# Copy web resources
$WebSrcDir = Join-Path $ProjectRoot "web"
$WebDestDir = Join-Path $DistDir "web"

if (Test-Path $WebSrcDir) {
    if (Test-Path $WebDestDir) {
        Remove-Item -Recurse -Force $WebDestDir
    }
    Copy-Item -Path $WebSrcDir -Destination $WebDestDir -Recurse -Force
    
    # Remove unnecessary files
    $removeItems = @("node_modules", "ts", "package.json", "package-lock.json", "tsconfig.json")
    foreach ($item in $removeItems) {
        $itemPath = Join-Path $WebDestDir $item
        if (Test-Path $itemPath) {
            Remove-Item -Path $itemPath -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    Write-OK "Web resources copiados"
}

# Also copy to bin folder for development
if (-not $NoCopy) {
    Write-Step "Atualizando pasta bin..."
    $BinWebDir = Join-Path $BinDir "web"
    if (Test-Path $BinWebDir) {
        Remove-Item -Recurse -Force $BinWebDir
    }
    Copy-Item -Path $WebSrcDir -Destination $BinWebDir -Recurse -Force
    
    # Remove unnecessary files from bin too
    foreach ($item in $removeItems) {
        $itemPath = Join-Path $BinWebDir $item
        if (Test-Path $itemPath) {
            Remove-Item -Path $itemPath -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    Write-OK "Pasta bin atualizada"
}

# Summary
Write-Host "`n============================================" -ForegroundColor Green
Write-Host "          BUILD COMPLETO!" -ForegroundColor Green
Write-Host "============================================`n" -ForegroundColor Green

$exeSize = (Get-Item (Join-Path $DistDir "TootegaWebAPI.exe")).Length / 1MB
$fileCount = (Get-ChildItem $DistDir -Recurse -File | Measure-Object).Count

Write-Host "Distribuicao: $DistDir" -ForegroundColor White
Write-Host "Executavel:   $([math]::Round($exeSize, 2)) MB" -ForegroundColor White
Write-Host "Arquivos:     $fileCount" -ForegroundColor White
Write-Host ""

if (-not $NoCopy) {
    Write-Host "Pasta bin tambem atualizada: $BinDir" -ForegroundColor Yellow
    Write-Host ""
}

Write-Host "Para executar:" -ForegroundColor Gray
Write-Host "  cd `"$DistDir`"" -ForegroundColor White
Write-Host "  .\TootegaWebAPI.exe" -ForegroundColor White
Write-Host ""
