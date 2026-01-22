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

# Compile TypeScript
Write-Step "Compilando TypeScript..."
$WebDir = Join-Path $ProjectRoot "web"
if (Test-Path (Join-Path $WebDir "package.json")) {
    Push-Location $WebDir
    if (-not (Test-Path "node_modules")) {
        Write-Host "Instalando dependencias npm..."
        npm install --silent 2>$null
    }
    npm run build 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-OK "TypeScript compilado!"
    }
    else {
        Write-Warn "Falha ao compilar TypeScript (verifique se npm esta instalado)"
    }
    Pop-Location
}
else {
    Write-Warn "package.json nao encontrado em web/"
}

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
                # Create Windows distribution folder
                $WinDistDir = Join-Path $DistDir "windows-x64"
                New-Item -ItemType Directory -Force -Path $WinDistDir | Out-Null
                
                # Copy executable
                Copy-Item $WinExe $WinDistDir
                
                # Copy web resources folder
                $WebSrcDir = Join-Path $ProjectRoot "web"
                if (Test-Path $WebSrcDir) {
                    $WebDestDir = Join-Path $WinDistDir "web"
                    Copy-Item -Path $WebSrcDir -Destination $WebDestDir -Recurse -Force
                    # Remove unnecessary files from web folder
                    Remove-Item -Path (Join-Path $WebDestDir "node_modules") -Recurse -Force -ErrorAction SilentlyContinue
                    Remove-Item -Path (Join-Path $WebDestDir "ts") -Recurse -Force -ErrorAction SilentlyContinue
                    Remove-Item -Path (Join-Path $WebDestDir "package*.json") -Force -ErrorAction SilentlyContinue
                    Remove-Item -Path (Join-Path $WebDestDir "tsconfig.json") -Force -ErrorAction SilentlyContinue
                    Write-OK "Web resources copiados!"
                }
                
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
    
    try {
        $dockerVersion = docker --version 2>$null
    }
    catch {
        $dockerVersion = $null
    }
    
    if (-not $dockerVersion) {
        Write-Warn "Docker nao encontrado! Pulando builds Linux."
        Write-Warn "Para compilar para Linux, instale o Docker Desktop: https://www.docker.com/products/docker-desktop"
        $SkipLinux = $true
    }
    else {
        Write-OK "Docker encontrado: $dockerVersion"
    }
}

if (-not $SkipLinux) {
    
    Write-Step "Configurando Docker Buildx para multi-arquitetura..."
    $builderExists = docker buildx ls 2>&1 | Select-String "tootega-builder"
    if (-not $builderExists) {
        $null = docker buildx create --name tootega-builder --driver docker-container --bootstrap 2>&1
    }
    $null = docker buildx use tootega-builder 2>&1
    
    Write-Step "Configurando emulacao QEMU para ARM64..."
    $null = docker run --rm --privileged multiarch/qemu-user-static --reset -p yes 2>&1
    
    Write-Step "Compilando Linux x64 (amd64)..."
    $LinuxX64Dir = Join-Path $DistDir "linux-x64"
    docker buildx build --platform linux/amd64 --file "$ProjectRoot\docker\Dockerfile.linux" --target export --output "type=local,dest=$LinuxX64Dir" $ProjectRoot 2>&1 | ForEach-Object { Write-Host $_ }
    
    if ($LASTEXITCODE -eq 0) {
        $LinuxX64 = Join-Path $LinuxX64Dir "TootegaWebAPI"
        if (Test-Path $LinuxX64) {
            # Copy web resources for Linux x64
            $WebSrcDir = Join-Path $ProjectRoot "web"
            if (Test-Path $WebSrcDir) {
                $WebDestDir = Join-Path $LinuxX64Dir "web"
                Copy-Item -Path $WebSrcDir -Destination $WebDestDir -Recurse -Force
                # Remove unnecessary files
                Remove-Item -Path (Join-Path $WebDestDir "node_modules") -Recurse -Force -ErrorAction SilentlyContinue
                Remove-Item -Path (Join-Path $WebDestDir "ts") -Recurse -Force -ErrorAction SilentlyContinue
                Remove-Item -Path (Join-Path $WebDestDir "package*.json") -Force -ErrorAction SilentlyContinue
                Remove-Item -Path (Join-Path $WebDestDir "tsconfig.json") -Force -ErrorAction SilentlyContinue
            }
            Write-OK "Linux x64 build completo!"
        }
    }
    else {
        Write-Err "Falha no build Linux x64"
    }
    
    Write-Step "Compilando Linux ARM64 (aarch64)..."
    Write-Warn "Este build usa emulacao QEMU e pode demorar alguns minutos..."
    
    $LinuxARM64Dir = Join-Path $DistDir "linux-arm64"
    docker buildx build --platform linux/arm64 --file "$ProjectRoot\docker\Dockerfile.linux" --target export --output "type=local,dest=$LinuxARM64Dir" $ProjectRoot 2>&1 | ForEach-Object { Write-Host $_ }
    
    if ($LASTEXITCODE -eq 0) {
        $LinuxARM64 = Join-Path $LinuxARM64Dir "TootegaWebAPI"
        if (Test-Path $LinuxARM64) {
            # Copy web resources for Linux ARM64
            $WebSrcDir = Join-Path $ProjectRoot "web"
            if (Test-Path $WebSrcDir) {
                $WebDestDir = Join-Path $LinuxARM64Dir "web"
                Copy-Item -Path $WebSrcDir -Destination $WebDestDir -Recurse -Force
                # Remove unnecessary files
                Remove-Item -Path (Join-Path $WebDestDir "node_modules") -Recurse -Force -ErrorAction SilentlyContinue
                Remove-Item -Path (Join-Path $WebDestDir "ts") -Recurse -Force -ErrorAction SilentlyContinue
                Remove-Item -Path (Join-Path $WebDestDir "package*.json") -Force -ErrorAction SilentlyContinue
                Remove-Item -Path (Join-Path $WebDestDir "tsconfig.json") -Force -ErrorAction SilentlyContinue
            }
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

# List distribution folders
$folders = Get-ChildItem $DistDir -Directory 2>$null
if ($folders) {
    Write-Host "Plataforma".PadRight(20) "Pasta".PadRight(25) "Conteudo" -ForegroundColor Gray
    Write-Host ("-" * 70) -ForegroundColor Gray
    
    foreach ($folder in $folders) {
        $platform = switch -Wildcard ($folder.Name) {
            "*windows*" { "Windows x64" }
            "*linux-x64*" { "Linux x64" }
            "*linux-arm64*" { "Linux ARM64" }
            default { "Unknown" }
        }
        $contents = (Get-ChildItem $folder.FullName -Recurse | Measure-Object).Count
        Write-Host $platform.PadRight(20) $folder.Name.PadRight(25) "$contents arquivos"
    }
    Write-Host ""
    Write-Host "Cada pasta contem o executavel + pasta 'web' com os recursos estaticos." -ForegroundColor Yellow
    Write-Host ""
}
else {
    Write-Warn "Nenhuma pasta encontrada no diretorio de distribuicao."
}
