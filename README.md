# Tootega WebAPI

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue.svg" alt="C++17"/>
  <img src="https://img.shields.io/badge/TypeScript-5.x-3178C6.svg" alt="TypeScript"/>
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg" alt="Platform"/>
  <img src="https://img.shields.io/badge/Architecture-x64%20%7C%20ARM64-green.svg" alt="Architecture"/>
  <img src="https://img.shields.io/badge/License-Proprietary-red.svg" alt="License"/>
</p>

Servidor WebAPI RESTful cross-platform desenvolvido em C++17, com interface web em TypeScript e suporte a múltiplas plataformas e arquiteturas.

## 📋 Índice

- [Características](#-características)
- [Requisitos](#-requisitos)
- [Estrutura do Projeto](#-estrutura-do-projeto)
- [Compilação](#-compilação)
  - [Windows com Visual Studio 2022](#windows-com-visual-studio-2022)
  - [Windows com CMake (Linha de Comando)](#windows-com-cmake-linha-de-comando)
  - [Linux](#linux)
  - [WSL (Windows Subsystem for Linux)](#wsl-windows-subsystem-for-linux)
- [Frontend Web](#-frontend-web)
- [Cross-Compilation](#-cross-compilation-compilação-cruzada)
  - [Usando WSL](#1-usando-wsl-recomendado)
  - [Usando Docker](#2-usando-docker)
  - [Usando GitHub Actions](#3-usando-github-actions-cicd)
  - [Toolchain Files](#4-toolchain-file-para-cross-compilation)
- [Execução](#-execução)
- [API Endpoints](#-api-endpoints)
  - [Endpoints Públicos](#endpoints-públicos)
  - [Autenticação (JWT)](#autenticação-jwt)
  - [Browseroso](#browseroso-navegador-de-tabelas)
- [Documentação da API](#-documentação-da-api)
- [Configuração](#-configuração)

---

## ✨ Características

- **Cross-Platform**: Suporte a Windows e Linux
- **Multi-Arquitetura**: Compatível com x64 e ARM64
- **RESTful API**: Endpoints JSON com suporte a CORS
- **Autenticação JWT**: Login seguro com tokens JWT
- **Browseroso**: Interface web para navegação de tabelas SQL
- **Banco de Dados**: Acesso via ODBC (SQL Server, PostgreSQL, etc.)
- **Header-Only HTTP**: Utiliza cpp-httplib (sem dependências externas)
- **Frontend TypeScript**: Interface web moderna com TypeScript
- **C++17**: Código moderno e eficiente
- **Zero Configuração**: Pronto para executar após compilação
- **Documentação Interativa**: Scalar, ReDoc e OpenAPI 3.0

---

## 📦 Requisitos

### Windows

| Requisito | Versão Mínima |
|-----------|---------------|
| Visual Studio | 2022 (v17.0+) |
| MSVC Toolset | v143 |
| Windows SDK | 10.0.19041.0+ |
| CMake | 3.20+ (incluído no VS) |

**Workloads necessários no Visual Studio:**

- "Desenvolvimento para desktop com C++"
- "Ferramentas CMake do C++ para Windows" (opcional)

### Linux / WSL

| Requisito | Versão Mínima |
|-----------|---------------|
| GCC | 9.0+ |
| CMake | 3.20+ |
| Ninja | 1.10+ (recomendado) |

**Instalação das dependências (Ubuntu/Debian):**

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build
```

**Instalação das dependências (Fedora/RHEL):**

```bash
sudo dnf install -y gcc-c++ cmake ninja-build
```

**Instalação das dependências (Arch Linux):**

```bash
sudo pacman -S base-devel cmake ninja
```

---

## 📁 Estrutura do Projeto

```
WebAPI-C++/
├── 📄 CMakeLists.txt           # Configuração CMake
├── 📄 CMakePresets.json        # Presets para CMake
├── 📄 TootegaWebAPI.sln        # Solução Visual Studio 2022
├── 📄 TootegaWebAPI.vcxproj    # Projeto Visual Studio 2022
├── 📄 README.md                # Este arquivo
├── 📄 build-all.ps1            # Script de build multiplataforma
│
├── 📁 .vscode/                 # Configurações VS Code
│   ├── launch.json
│   ├── settings.json
│   └── tasks.json
│
├── 📁 docker/                  # Arquivos Docker
│   ├── docker-compose.build.yml
│   └── Dockerfile.linux
│
├── 📁 include/                 # Headers externos
│   ├── httplib.h               # cpp-httplib (HTTP server)
│   └── 📁 nanodbc/             # ODBC wrapper
│       ├── nanodbc.h
│       └── nanodbc.cpp
│
├── 📁 web/                     # Frontend Web
│   ├── package.json
│   ├── tsconfig.json
│   ├── 📁 css/                 # Estilos CSS
│   │   ├── variables.css       # Variáveis CSS
│   │   ├── reset.css           # Reset de estilos
│   │   ├── layout.css          # Layout geral
│   │   ├── components.css      # Componentes
│   │   └── 📁 pages/           # CSS específico por página
│   ├── 📁 js/                  # JavaScript compilado
│   │   ├── auth.js
│   │   ├── common.js
│   │   └── 📁 pages/
│   ├── 📁 ts/                  # TypeScript fonte
│   │   ├── auth.ts
│   │   ├── common.ts
│   │   └── 📁 pages/
│   └── 📁 pages/               # Páginas HTML
│       ├── home.html
│       ├── login.html
│       └── browseroso.html
│
└── 📁 src/                     # Código fonte C++
    ├── main.cpp                # Ponto de entrada
    │
    ├── 📁 api/                 # Controllers da API
    │   ├── version_controller.h/cpp    # Versão e info do sistema
    │   ├── docs_controller.h/cpp       # Documentação (Scalar/ReDoc/OpenAPI)
    │   ├── auth_controller.h/cpp       # Autenticação JWT
    │   ├── browseroso_controller.h/cpp # Interface de browse de tabelas
    │   └── static_controller.h/cpp     # Arquivos estáticos (CSS/JS/HTML)
    │
    ├── 📁 core/                # Componentes principais
    │   ├── server.h/cpp        # Servidor HTTP
    │   ├── system_info.h/cpp   # Informações do sistema
    │   └── jwt.h/cpp           # Geração e validação JWT
    │
    └── 📁 data/                # Acesso a dados
        └── database.h/cpp      # Conexão ODBC e queries
```

---

## 🔨 Compilação

### Windows com Visual Studio 2022

#### Método 1: Usando o arquivo de solução (.sln)

1. **Abra a solução** `TootegaWebAPI.sln` no Visual Studio 2022

2. **Selecione a configuração** na barra de ferramentas:
   - `Debug` ou `Release`
   - `x64` ou `ARM64`

3. **Compile** usando uma das opções:
   - Menu: `Build → Build Solution`
   - Atalho: `Ctrl+Shift+B`
   - Tecla: `F7`

4. **O executável** será gerado em:
   - Debug: `bin\x64\Debug\TootegaWebAPI.exe`
   - Release: `bin\x64\Release\TootegaWebAPI.exe`

#### Método 2: Abrindo a pasta (CMake)

1. **Abra a pasta** do projeto: `File → Open → Folder`

2. **Selecione o preset** no dropdown da barra de ferramentas:
   - `Windows x64 Debug`
   - `Windows x64 Release`
   - `Windows ARM64 Debug`
   - `Windows ARM64 Release`

3. **Compile** com `Ctrl+Shift+B`

### Windows com CMake (Linha de Comando)

Abra o **Developer Command Prompt for VS 2022** ou **Developer PowerShell for VS 2022**:

```powershell
# Navegue até a pasta do projeto
cd D:\Tootega\Source\WebAPI-C++

# Configure o projeto (x64)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Compile em Release
cmake --build build --config Release

# Compile em Debug
cmake --build build --config Debug
```

**Para ARM64:**

```powershell
cmake -B build-arm64 -G "Visual Studio 17 2022" -A ARM64
cmake --build build-arm64 --config Release
```

**Usando Ninja (mais rápido):**

```powershell
# Configure
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build
```

### Linux

```bash
# Clone ou navegue até o projeto
cd /path/to/WebAPI-C++

# Crie o diretório de build
mkdir -p build && cd build

# Configure (Release)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Compile (use -j para paralelismo)
cmake --build . -j$(nproc)
```

**Usando presets (se Ninja estiver instalado):**

```bash
# Configure
cmake --preset linux-x64-release

# Build
cmake --build --preset linux-x64-release
```

**Para ARM64 (cross-compilation ou nativo):**

```bash
cmake --preset linux-arm64-release
cmake --build --preset linux-arm64-release
```

### WSL (Windows Subsystem for Linux)

1. **Instale o WSL** (se ainda não tiver):

   ```powershell
   wsl --install -d Ubuntu
   ```

2. **Instale as dependências no WSL:**

   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake ninja-build gdb
   ```

3. **Compile usando o Visual Studio 2022:**
   - Abra a pasta do projeto no VS 2022
   - Selecione o preset `WSL Debug` ou `WSL Release`
   - Compile com `Ctrl+Shift+B`

4. **Ou compile diretamente no terminal WSL:**

   ```bash
   cd /mnt/d/Tootega/Source/WebAPI-C++
   cmake --preset wsl-release
   cmake --build --preset wsl-release
   ```

---

## 🌐 Frontend Web

O projeto inclui uma interface web moderna desenvolvida com TypeScript e CSS modular.

### Estrutura do Frontend

```
web/
├── css/
│   ├── variables.css     # Variáveis CSS (cores, espaçamentos, etc.)
│   ├── reset.css         # Reset de estilos padrão
│   ├── layout.css        # Layout principal e grid
│   ├── components.css    # Componentes reutilizáveis
│   └── pages/            # Estilos específicos por página
├── ts/
│   ├── common.ts         # Utilitários compartilhados
│   ├── auth.ts           # Funções de autenticação
│   └── pages/            # Scripts específicos por página
├── js/                   # JavaScript compilado (gerado)
└── pages/
    ├── home.html         # Página inicial (requer auth)
    ├── login.html        # Página de login
    └── browseroso.html   # Navegador de tabelas
```

### Compilando o TypeScript

```bash
cd web

# Instalar dependências (primeira vez)
npm install

# Compilar TypeScript
npm run build

# Ou em modo watch (desenvolvimento)
npm run watch
```

### Tecnologias do Frontend

| Tecnologia | Descrição |
|------------|-----------|
| TypeScript | Linguagem tipada para JavaScript |
| CSS Custom Properties | Variáveis CSS para temas |
| Fetch API | Requisições HTTP modernas |
| JWT | Autenticação via token |

### Páginas Disponíveis

| Página | URL | Descrição |
|--------|-----|-----------|
| Login | `/login` | Autenticação do usuário |
| Home | `/home` | Dashboard principal |
| Browseroso | `/browseroso` | Navegador de tabelas SQL |

---

## 🚀 Execução

### Uso Básico

```bash
# Windows
.\bin\x64\Release\TootegaWebAPI.exe

# Linux
./bin/TootegaWebAPI
```

### Opções de Linha de Comando

| Opção | Descrição | Padrão |
|-------|-----------|--------|
| `-h, --host <address>` | Endereço de bind | `0.0.0.0` |
| `-p, --port <port>` | Porta do servidor | `8080` |
| `--help` | Exibe ajuda | - |

### Exemplos

```bash
# Iniciar na porta padrão (8080)
./TootegaWebAPI

# Iniciar em porta específica
./TootegaWebAPI --port 3000

# Bind apenas em localhost
./TootegaWebAPI --host 127.0.0.1 --port 8080

# Bind em interface específica
./TootegaWebAPI --host 192.168.1.100 --port 80
```

### Execução como Serviço

**Windows (usando NSSM):**

```powershell
nssm install TootegaWebAPI "C:\path\to\TootegaWebAPI.exe"
nssm set TootegaWebAPI AppParameters "--port 8080"
nssm start TootegaWebAPI
```

**Linux (systemd):**

```bash
sudo nano /etc/systemd/system/tootega-webapi.service
```

```ini
[Unit]
Description=Tootega WebAPI Server
After=network.target

[Service]
Type=simple
User=www-data
ExecStart=/opt/tootega/TootegaWebAPI --port 8080
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable tootega-webapi
sudo systemctl start tootega-webapi
```

---

## 📡 API Endpoints

### Endpoints Públicos

#### GET /

Mensagem de boas-vindas com links para endpoints disponíveis.

**Resposta:**

```json
{
    "message": "Welcome to Tootega WebAPI",
    "documentation": "/api/docs",
    "version": "/api/version"
}
```

#### GET /health

Health check simples.

**Resposta:**

```json
{
    "status": "healthy"
}
```

#### GET /api/version

Retorna informações básicas de versão do sistema.

**Resposta:**

```json
{
    "api_version": "1.0.0",
    "os": "Windows",
    "os_version": "10.0.22631",
    "architecture": "x64",
    "hostname": "SERVIDOR01"
}
```

#### GET /api/version/detailed

Retorna informações detalhadas do sistema, build e runtime.

**Resposta:**

```json
{
    "api": {
        "name": "Tootega WebAPI",
        "version": "1.0.0",
        "build_timestamp": "Jan 21 2026 10:30:00"
    },
    "system": {
        "os": "Windows",
        "os_version": "10.0.22631",
        "architecture": "x64",
        "hostname": "SERVIDOR01"
    },
    "build": {
        "compiler": "MSVC 1939 (VS 2022)",
        "configuration": "Release",
        "target_arch": "x64"
    },
    "runtime": {
        "uptime_seconds": 3600,
        "uptime_formatted": "01:00:00",
        "current_time": "2026-01-21T10:30:00Z"
    }
}
```

#### GET /api/health

Health check detalhado com informações de uptime.

**Resposta:**

```json
{
    "status": "healthy",
    "version": "1.0.0",
    "uptime_seconds": 3600,
    "timestamp": "2026-01-21T10:30:00Z"
}
```

### Autenticação (JWT)

#### POST /api/auth/login

Realiza login e retorna token JWT.

**Request Body:**

```json
{
    "username": "admin",
    "password": "senha123"
}
```

**Resposta (200):**

```json
{
    "success": true,
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "expires_in": 3600,
    "token_type": "Bearer"
}
```

**Resposta (401):**

```json
{
    "success": false,
    "error": "Invalid credentials"
}
```

#### POST /api/auth/logout

Realiza logout (invalida token no cliente).

**Headers:**

```
Authorization: Bearer <token>
```

**Resposta:**

```json
{
    "success": true,
    "message": "Logged out successfully"
}
```

#### GET /api/auth/verify

Verifica se o token JWT é válido.

**Headers:**

```
Authorization: Bearer <token>
```

**Resposta (200):**

```json
{
    "valid": true,
    "user": "admin"
}
```

#### POST /api/auth/refresh

Renova o token JWT antes de expirar.

**Headers:**

```
Authorization: Bearer <token>
```

**Resposta:**

```json
{
    "success": true,
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "expires_in": 3600
}
```

### Browseroso (Navegador de Tabelas)

> ⚠️ Requer autenticação JWT

#### GET /browseroso

Interface web para navegação de tabelas SQL.

#### POST /api/browseroso/connect

Conecta ao banco de dados.

**Request Body:**

```json
{
    "connectionString": "Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=MyDB;Uid=sa;Pwd=password;"
}
```

**Resposta:**

```json
{
    "success": true,
    "message": "Connected successfully"
}
```

#### POST /api/browseroso/disconnect

Desconecta do banco de dados.

**Resposta:**

```json
{
    "success": true,
    "message": "Disconnected"
}
```

#### GET /api/browseroso/status

Retorna status da conexão.

**Resposta:**

```json
{
    "connected": true,
    "database": "MyDB"
}
```

#### GET /api/browseroso/tables

Lista todas as tabelas do banco conectado.

**Resposta:**

```json
{
    "tables": [
        {"schema": "dbo", "name": "Customers"},
        {"schema": "dbo", "name": "Orders"}
    ]
}
```

#### GET /api/browseroso/tables/:schema/:table/columns

Retorna metadados das colunas de uma tabela.

**Resposta:**

```json
{
    "columns": [
        {"name": "Id", "type": "int", "nullable": false, "isPrimaryKey": true},
        {"name": "Name", "type": "varchar(100)", "nullable": true, "isPrimaryKey": false}
    ]
}
```

#### GET /api/browseroso/tables/:schema/:table/data

Retorna dados de uma tabela com paginação.

**Query Parameters:**

- `page` (default: 1)
- `pageSize` (default: 50)
- `filter` (opcional)
- `filterColumn` (opcional)

**Resposta:**

```json
{
    "columns": ["Id", "Name"],
    "rows": [
        {"Id": "1", "Name": "John"},
        {"Id": "2", "Name": "Jane"}
    ],
    "totalRows": 100,
    "page": 1,
    "pageSize": 50
}
```

### Páginas Web

| Endpoint | Descrição | Autenticação |
|----------|-----------|--------------|
| GET /home | Página inicial | ✅ Requer |
| GET /login | Página de login | ❌ Público |
| GET /browseroso | Navegador de tabelas | ✅ Requer |

### Arquivos Estáticos

| Endpoint | Descrição |
|----------|-----------|
| GET /css/* | Arquivos CSS |
| GET /js/* | Arquivos JavaScript |
| GET /pages/* | Páginas HTML |

---

## 📚 Documentação da API

O Tootega WebAPI oferece documentação interativa completa através de múltiplas interfaces:

### GET /api/docs - Scalar UI (Recomendado)

Interface de documentação **moderna e interativa** com tema escuro e layout limpo.

- ✨ Design moderno e responsivo
- 🎨 Tema escuro elegante
- 📖 Navegação por sidebar
- 🔄 Try-it-out integrado
- 📝 Exemplos de código

Acesse: **<http://localhost:8080/api/docs>**

### GET /api/redoc - ReDoc

Documentação **limpa e profissional**, ideal para documentação pública.

- 📄 Layout de três colunas
- 🎯 Foco em legibilidade
- 📱 Totalmente responsivo
- 🖨️ Otimizado para impressão

Acesse: **<http://localhost:8080/api/redoc>**

### GET /api/openapi.json

Especificação **OpenAPI 3.0** em formato JSON.

```bash
curl http://localhost:8080/api/openapi.json
```

### GET /api/openapi.yaml

Especificação **OpenAPI 3.0** em formato YAML (simplificado).

```bash
curl http://localhost:8080/api/openapi.yaml
```

---

## 🌍 Cross-Compilation (Compilação Cruzada)

Cross-compilation permite gerar executáveis para outras plataformas a partir do Windows.

### Opções Disponíveis

| Método | Plataforma Alvo | Complexidade | Recomendado |
|--------|-----------------|--------------|-------------|
| WSL | Linux x64/ARM64 | ⭐ Fácil | ✅ Sim |
| Docker | Linux x64/ARM64 | ⭐⭐ Médio | ✅ Sim |
| Cross-Toolchain | Linux x64 | ⭐⭐⭐ Difícil | ❌ Não |
| CI/CD (GitHub Actions) | Todas | ⭐⭐ Médio | ✅ Sim |

### 1. Usando WSL (Recomendado)

O **Windows Subsystem for Linux** é a forma mais simples de compilar para Linux no Windows.

```powershell
# Instalar WSL com Ubuntu (se ainda não tiver)
wsl --install -d Ubuntu

# Ou instalar uma distro específica
wsl --install -d Debian
```

**Compilando no WSL:**

```bash
# Entrar no WSL
wsl

# Instalar dependências
sudo apt update
sudo apt install -y build-essential cmake ninja-build unixodbc-dev

# Navegar até o projeto (acessando o disco do Windows)
cd /mnt/d/Tootega/Source/WebAPI-C++

# Compilar
cmake -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j$(nproc)

# O executável estará em build-linux/TootegaWebAPI
```

**Para ARM64 no WSL (requer Ubuntu ARM64 ou cross-compiler):**

```bash
sudo apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

cmake -B build-linux-arm64 \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build-linux-arm64 -j$(nproc)
```

### 2. Usando Docker

Docker permite compilar em containers Linux isolados.

**Dockerfile para compilação:**

```dockerfile
# Dockerfile.build
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential cmake ninja-build unixodbc-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j$(nproc)

# O executável estará em /src/build/TootegaWebAPI
```

**Comandos para compilar:**

```powershell
# Build da imagem (compila o projeto)
docker build -f Dockerfile.build -t tootega-build .

# Copiar o executável para o Windows
docker create --name temp-container tootega-build
docker cp temp-container:/src/build/TootegaWebAPI ./TootegaWebAPI-linux
docker rm temp-container
```

**Multi-arquitetura com Docker Buildx:**

```powershell
# Habilitar buildx para multi-plataforma
docker buildx create --use

# Compilar para ARM64
docker buildx build --platform linux/arm64 -f Dockerfile.build -t tootega-arm64 --load .
```

### 3. Usando GitHub Actions (CI/CD)

Crie `.github/workflows/build.yml`:

```yaml
name: Build

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Configure CMake
        run: cmake -B build -G "Visual Studio 17 2022" -A x64
      
      - name: Build
        run: cmake --build build --config Release
      
      - name: Upload Artifact
        uses: actions/upload-artifact@v4
        with:
          name: TootegaWebAPI-windows-x64
          path: build/Release/TootegaWebAPI.exe

  build-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Install Dependencies
        run: sudo apt-get update && sudo apt-get install -y unixodbc-dev
      
      - name: Configure CMake
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release
      
      - name: Build
        run: cmake --build build -j$(nproc)
      
      - name: Upload Artifact
        uses: actions/upload-artifact@v4
        with:
          name: TootegaWebAPI-linux-x64
          path: build/TootegaWebAPI

  build-linux-arm64:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Install Cross-Compiler
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
      
      - name: Configure CMake
        run: |
          cmake -B build \
            -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
            -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
            -DCMAKE_BUILD_TYPE=Release
      
      - name: Build
        run: cmake --build build -j$(nproc)
      
      - name: Upload Artifact
        uses: actions/upload-artifact@v4
        with:
          name: TootegaWebAPI-linux-arm64
          path: build/TootegaWebAPI
```

### 4. Toolchain File para Cross-Compilation

Para compilação cruzada avançada, crie um arquivo de toolchain:

**`cmake/toolchains/linux-x64.cmake`:**

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Ajuste os caminhos conforme seu ambiente
set(CMAKE_C_COMPILER /usr/bin/x86_64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/x86_64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

**Uso:**

```bash
cmake -B build-cross -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-x64.cmake
cmake --build build-cross
```

### Considerações sobre ODBC

⚠️ **Importante**: Este projeto usa ODBC para acesso a banco de dados.

| Plataforma | Driver ODBC | Notas |
|------------|-------------|-------|
| Windows | SQL Server Native Client | Incluído no Windows |
| Linux x64 | unixODBC + msodbcsql17 | Requer instalação |
| Linux ARM64 | unixODBC + FreeTDS | Microsoft não suporta ARM64 |

**Instalando driver ODBC no Linux:**

```bash
# Ubuntu/Debian x64
curl https://packages.microsoft.com/keys/microsoft.asc | sudo apt-key add -
sudo add-apt-repository "$(curl https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/prod.list)"
sudo apt-get update
sudo apt-get install -y msodbcsql17 unixodbc-dev

# Para ARM64 (use FreeTDS)
sudo apt-get install -y freetds-dev freetds-bin unixodbc-dev
```

---

## ⚙️ Configuração

### Variáveis de Build (CMake)

| Variável | Descrição | Padrão |
|----------|-----------|--------|
| `CMAKE_BUILD_TYPE` | Tipo de build (Debug/Release) | - |
| `CMAKE_INSTALL_PREFIX` | Diretório de instalação | `/usr/local` |

### Portas e Firewall

**Windows Firewall:**

```powershell
netsh advfirewall firewall add rule name="Tootega WebAPI" dir=in action=allow protocol=TCP localport=8080
```

**Linux (UFW):**

```bash
sudo ufw allow 8080/tcp
```

**Linux (firewalld):**

```bash
sudo firewall-cmd --permanent --add-port=8080/tcp
sudo firewall-cmd --reload
```

---

## 📄 Licença

Copyright © 2026 Tootega. Todos os direitos reservados.

---

## 📞 Suporte

Para suporte técnico, entre em contato com a equipe de desenvolvimento.
