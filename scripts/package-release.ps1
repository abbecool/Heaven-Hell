param(
    [string]$MingwBin = "C:/msys64/ucrt64/bin",
    [string]$Configuration = "Release",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $ScriptDir "..")
$Version = (Get-Content (Join-Path $RepoRoot "version.txt") -Raw).Trim()
$PackageName = "HeavenHell-$Version-win64"
$DistDir = Join-Path $RepoRoot "dist"
$StageDir = Join-Path $DistDir $PackageName
$ZipPath = Join-Path $DistDir "$PackageName.zip"
$BuildDir = Join-Path $RepoRoot "build/Windows/Ninja/$Configuration"
$RunDir = Join-Path $RepoRoot "run/$Configuration"
$ExePath = Join-Path $RunDir "heavenhell.exe"
$CMake = Join-Path $MingwBin "cmake.exe"
$Ninja = Join-Path $MingwBin "ninja.exe"
$Gxx = Join-Path $MingwBin "g++.exe"
$Objdump = Join-Path $MingwBin "objdump.exe"

function Assert-ToolExists {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        throw "Required tool not found: $Path"
    }
}

function Assert-PathInside {
    param(
        [string]$Path,
        [string]$Parent
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $fullParent = [System.IO.Path]::GetFullPath($Parent)

    if (-not $fullParent.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $fullParent += [System.IO.Path]::DirectorySeparatorChar
    }

    if (-not $fullPath.StartsWith($fullParent, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to modify path outside package directory: $fullPath"
    }
}

function Get-ImportedDllNames {
    param([string]$BinaryPath)

    & $Objdump -p $BinaryPath |
        Select-String -Pattern "DLL Name:\s+(.+)$" |
        ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() }
}

function Should-SkipDll {
    param([string]$DllName)

    if ($DllName -match "^(api-ms-|ext-ms-)") {
        return $true
    }

    $systemDlls = @(
        "advapi32.dll",
        "comctl32.dll",
        "comdlg32.dll",
        "crypt32.dll",
        "dwrite.dll",
        "dwmapi.dll",
        "gdi32.dll",
        "imm32.dll",
        "kernel32.dll",
        "msvcrt.dll",
        "ntdll.dll",
        "ole32.dll",
        "oleaut32.dll",
        "rpcrt4.dll",
        "setupapi.dll",
        "shell32.dll",
        "shlwapi.dll",
        "user32.dll",
        "usp10.dll",
        "uuid.dll",
        "version.dll",
        "winmm.dll",
        "winspool.drv",
        "ws2_32.dll"
    )

    return $systemDlls -contains $DllName.ToLowerInvariant()
}

function Copy-RuntimeDependencies {
    param(
        [string]$RootBinary,
        [string]$Destination
    )

    $queue = New-Object System.Collections.Queue
    $seen = @{}
    $queue.Enqueue($RootBinary)

    while ($queue.Count -gt 0) {
        $binary = $queue.Dequeue()

        foreach ($dllName in Get-ImportedDllNames $binary) {
            $key = $dllName.ToLowerInvariant()

            if ($seen.ContainsKey($key) -or (Should-SkipDll $dllName)) {
                continue
            }

            $seen[$key] = $true
            $stageDll = Join-Path $Destination $dllName
            $mingwDll = Join-Path $MingwBin $dllName

            if (Test-Path $stageDll) {
                $queue.Enqueue($stageDll)
                continue
            }

            if (Test-Path $mingwDll) {
                Copy-Item -LiteralPath $mingwDll -Destination $Destination -Force
                $queue.Enqueue($stageDll)
                continue
            }

            Write-Warning "Could not find runtime DLL: $dllName"
        }
    }
}

Assert-ToolExists $CMake
Assert-ToolExists $Ninja
Assert-ToolExists $Gxx
Assert-ToolExists $Objdump

$env:PATH = "$MingwBin;$env:PATH"

if (-not $SkipBuild) {
    & $CMake `
        -S $RepoRoot `
        -B $BuildDir `
        "-DCMAKE_BUILD_TYPE=$Configuration" `
        "-DCMAKE_CXX_COMPILER=$Gxx" `
        "-DCMAKE_MAKE_PROGRAM=$Ninja" `
        -G Ninja

    & $CMake --build $BuildDir
}

if (-not (Test-Path $ExePath)) {
    throw "Game executable not found: $ExePath"
}

New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

Assert-PathInside -Path $StageDir -Parent $DistDir
Assert-PathInside -Path $ZipPath -Parent $DistDir

if (Test-Path $StageDir) {
    Remove-Item -LiteralPath $StageDir -Recurse -Force
}

if (Test-Path $ZipPath) {
    Remove-Item -LiteralPath $ZipPath -Force
}

New-Item -ItemType Directory -Path $StageDir | Out-Null

Copy-Item -LiteralPath $ExePath -Destination $StageDir
Copy-Item -LiteralPath (Join-Path $RepoRoot "assets") -Destination $StageDir -Recurse
Copy-Item -LiteralPath (Join-Path $RepoRoot "config_files") -Destination $StageDir -Recurse
Copy-RuntimeDependencies -RootBinary (Join-Path $StageDir "heavenhell.exe") -Destination $StageDir

$ReadmePath = Join-Path $StageDir "README.txt"
@"
HeavenHell $Version

Run heavenhell.exe to start the game.

If Windows SmartScreen warns about this file, choose More info, then Run anyway.
"@ | Set-Content -Path $ReadmePath -Encoding ASCII

Compress-Archive -Path (Join-Path $StageDir "*") -DestinationPath $ZipPath -Force

Write-Host "Created release package:"
Write-Host $ZipPath
