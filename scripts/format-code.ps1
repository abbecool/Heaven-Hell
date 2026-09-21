[CmdletBinding()]
param(
    [ValidateSet('Check', 'Format')]
    [string]$Mode = 'Check',
    [string]$File,
    [string]$ClangFormat = 'clang-format'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$external = [System.IO.Path]::GetFullPath((Join-Path $root 'src/external'))
$extensions = @('.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.ipp', '.tpp')

# An allowlist keeps generated output and dependencies outside src/tests out.
# Skip reparse points so symlinks cannot lead the formatter outside this tree.
function Get-ProjectSources([string]$Directory) {
    foreach ($entry in Get-ChildItem -LiteralPath $Directory) {
        if ($entry.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
            continue
        }
        if ($entry.FullName -eq $external) {
            continue
        }
        if ($entry.PSIsContainer) {
            Get-ProjectSources $entry.FullName
        }
        elseif ($extensions -contains $entry.Extension) {
            $entry.FullName
        }
    }
}

$files = @(Get-ProjectSources (Join-Path $root 'src'))
$files += @(Get-ProjectSources (Join-Path $root 'tests'))
$files = @($files | Sort-Object)

if ($File) {
    $selected = (Get-Item -LiteralPath $File).FullName
    if ($files -notcontains $selected) {
        throw "File is excluded: only first-party C/C++ files under src/ and tests/ may be formatted: $File"
    }
    $files = @($selected)
}

$tool = (Get-Command $ClangFormat -CommandType Application -ErrorAction Stop).Source
$formatArgs = @("--style=file:$root/.clang-format", '--fallback-style=none')
if ($Mode -eq 'Check') {
    $formatArgs += @('--dry-run', '--Werror', '--ferror-limit=1')
}
else {
    $formatArgs += '-i'
}

$failed = 0
foreach ($source in $files) {
    & $tool @formatArgs $source
    if ($LASTEXITCODE -ne 0) {
        $failed++
    }
}
Write-Output "$Mode completed: $($files.Count) file(s), $failed file(s) with diagnostics."
if ($failed -gt 0) {
    exit 1
}
exit 0
