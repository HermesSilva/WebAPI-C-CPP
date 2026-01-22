#!/usr/bin/env pwsh
param(
    [switch]$SkipWindows,
    [switch]$SkipLinux,
    [switch]$Clean
)

$ErrorActionPreference = "Continue"
$ProjectRoot = $PSScriptRoot
$DistDir = Join-Path $ProjectRoot "dist"

function Write-Step { param($msg) Write-Host "`n>> $msg" -ForegroundColor Cyan }
function Write-OK { param($msg) Write-Host "[OK] $msg" -ForegroundColor Green }
function Write-Warn { param($msg) Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-Err { param($msg) Write-Host "[ERROR] $msg" -ForegroundColor Red }

Write-Host "`n================================================================" -ForegroundColor Blue
Write-Host "         Tootega WebAPI - Multi-Platform Build" -ForegroundColor Blue
Write-Host "  Plataformas: Windows x64 | Linux x64 | Linux ARM64" -ForegroundColor Blue
Write-Host "================================================================`n" -ForegroundColor Blue

Write-Step "Preparando diretorio de distribuicao..."
if ($Clean -and (Test-Path $DistDir)) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
Write-OK "Diretorio: $DistDir"

if (-not $SkipWindows) {
    Write-Step "Compilando Windows x64..."
    
    $MSBuildPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    if (-not (Test-Path $MSBuildPath)) {
        $MSBuildPath = "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    }
    if (-not (Test-Path $MSBuildPath)) {
        $MSBuildPath = "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    }
    
    if (Test-Path $MSBuildPath) {
        & $MSBuildPath "$ProjectRoot\TootegaWebAPI.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Build /verbosity:minimal
        
        if ($LASTEXITCODE -eq 0) {
            $WinExe = Join-Path $ProjectRoot "bin\x64\Release\TootegaWebAPI.exe"
            if (Test-Path $WinExe) {
                Copy-Item $WinExe (Join-Path $DistDir "TootegaWebAPI-windows-x64.exe")
                Write-OK "Windows x64 build completo!"
            }
        }
        else {
            Write-Err "Falha no build Windows x64"
        }
    }
    else {
        Write-Warn "MSBuild nao encontrado. Pulando build Windows."
    }
}

if (-not $SkipLinux) {
    Write-Step "Verificando Docker..."
    
    $dockerVersion = docker --version 2>$null
    if (-not $dockerVersion) {
        Write-Err "Docker nao encontrado! Instale o Docker Desktop."
        exit 1
    }
    Write-OK "Docker encontrado: $dockerVersion"
    
    Write-Step "Configurando Docker Buildx para multi-arquitetura..."
    $builderExists = docker buildx ls 2>&1 | Select-String "tootega-builder"
    if (-not $builderExists) {
        $null = docker buildx create --name tootega-builder --driver docker-container --bootstrap 2>&1
    }
    $null = docker buildx use tootega-builder 2>&1
    
    Write-Step "Configurando emulacao QEMU para ARM64..."
    $null = docker run --rm --privileged multiarch/qemu-user-static --reset -p yes 2>&1
    
    Write-Step "Compilando Linux x64 (amd64)..."
    docker buildx build --platform linux/amd64 --file "$ProjectRoot\docker\Dockerfile.linux" --target export --output "type=local,dest=$DistDir\linux-x64" $ProjectRoot 2>&1 | ForEach-Object { Write-Host $_ }
    
    if ($LASTEXITCODE -eq 0) {
        $LinuxX64 = Join-Path $DistDir "linux-x64\TootegaWebAPI"
        if (Test-Path $LinuxX64) {
            Move-Item $LinuxX64 (Join-Path $DistDir "TootegaWebAPI-linux-x64") -Force
            Remove-Item (Join-Path $DistDir "linux-x64") -Recurse -Force -ErrorAction SilentlyContinue
            Write-OK "Linux x64 build completo!"
        }
    }
    else {
        Write-Err "Falha no build Linux x64"
    }
    
    Write-Step "Compilando Linux ARM64 (aarch64)..."
    Write-Warn "Este build usa emulacao QEMU e pode demorar alguns minutos..."
    
    docker buildx build --platform linux/arm64 --file "$ProjectRoot\docker\Dockerfile.linux" --target export --output "type=local,dest=$DistDir\linux-arm64" $ProjectRoot 2>&1 | ForEach-Object { Write-Host $_ }
    
    if ($LASTEXITCODE -eq 0) {
        $LinuxARM64 = Join-Path $DistDir "linux-arm64\TootegaWebAPI"
        if (Test-Path $LinuxARM64) {
            Move-Item $LinuxARM64 (Join-Path $DistDir "TootegaWebAPI-linux-arm64") -Force
            Remove-Item (Join-Path $DistDir "linux-arm64") -Recurse -Force -ErrorAction SilentlyContinue
            Write-OK "Linux ARM64 build completo!"
        }
    }
    else {
        Write-Err "Falha no build Linux ARM64"
    }
}

Write-Host "`n================================================================" -ForegroundColor Green
Write-Host "                    BUILD COMPLETO!" -ForegroundColor Green
Write-Host "================================================================`n" -ForegroundColor Green

Write-Host "Arquivos gerados em: $DistDir`n" -ForegroundColor White

$files = Get-ChildItem $DistDir -File 2>$null
if ($files) {
    Write-Host "Plataforma".PadRight(25) "Tamanho".PadRight(15) "Arquivo" -ForegroundColor Gray
    Write-Host ("-" * 70) -ForegroundColor Gray
    
    foreach ($file in $files) {
        $platform = switch -Wildcard ($file.Name) {
            "*windows*" { "Windows x64" }
            "*linux-x64*" { "Linux x64" }
            "*linux-arm64*" { "Linux ARM64" }
            default { "Unknown" }
        }
        $size = "{0:N2} MB" -f ($file.Length / 1MB)
        Write-Host $platform.PadRight(25) $size.PadRight(15) $file.Name
    }
    Write-Host ""
}
else {
    Write-Warn "Nenhum arquivo encontrado no diretorio de distribuicao."
}
