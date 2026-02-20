[CmdletBinding()]
param(
    [string]$SpecPath = "affixes/affixes.json",
    [string]$DefaultOutputFileName = "CalamityAffixes_UserPatch.esp"
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

function Decode-Mo2IniValue([string]$value) {
    if ([string]::IsNullOrWhiteSpace($value)) {
        return $null
    }

    $trimmed = $value.Trim()
    if (
        ($trimmed.StartsWith("'") -and $trimmed.EndsWith("'")) -or
        ($trimmed.StartsWith('"') -and $trimmed.EndsWith('"'))
    ) {
        $trimmed = $trimmed.Substring(1, $trimmed.Length - 2)
    }

    $byteArrayMatch = [regex]::Match(
        $trimmed,
        '^@bytearray\((?<value>.*)\)$',
        [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
    )
    if ($byteArrayMatch.Success) {
        return $byteArrayMatch.Groups["value"].Value.Trim()
    }

    return $trimmed
}

function Resolve-Mo2Path([string]$rawValue, [string]$mo2Root) {
    $decoded = Decode-Mo2IniValue $rawValue
    if ([string]::IsNullOrWhiteSpace($decoded)) {
        return $null
    }

    $withBase = $decoded.Replace("%BASE_DIR%", $mo2Root).Replace("%base_dir%", $mo2Root)
    $expanded = [Environment]::ExpandEnvironmentVariables($withBase).Replace("/", "\")

    if ([System.IO.Path]::IsPathRooted($expanded)) {
        return $expanded
    }

    return Join-Path $mo2Root $expanded
}

function Get-IniValue([string]$iniPath, [string]$section, [string]$key) {
    if (-not (Test-Path $iniPath)) {
        return $null
    }

    $currentSection = ""
    foreach ($rawLine in Get-Content -Path $iniPath -ErrorAction SilentlyContinue) {
        $line = $rawLine.Trim()
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        if ($line.StartsWith(";") -or $line.StartsWith("#")) {
            continue
        }

        $sectionMatch = [regex]::Match($line, '^\[(?<section>[^\]]+)\]$')
        if ($sectionMatch.Success) {
            $currentSection = $sectionMatch.Groups["section"].Value.Trim()
            continue
        }

        if (-not $currentSection.Equals($section, [System.StringComparison]::OrdinalIgnoreCase)) {
            continue
        }

        $entryMatch = [regex]::Match($line, '^(?<name>[^=]+?)\s*=\s*(?<value>.*)$')
        if (-not $entryMatch.Success) {
            continue
        }

        if ($entryMatch.Groups["name"].Value.Trim().Equals($key, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $entryMatch.Groups["value"].Value
        }
    }

    return $null
}

function Get-Mo2RootCandidates {
    return @(
        "$env:LOCALAPPDATA\ModOrganizer",
        "$env:LOCALAPPDATA\ModOrganizer2",
        "$env:USERPROFILE\AppData\Local\ModOrganizer",
        "$env:USERPROFILE\AppData\Local\ModOrganizer2"
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique
}

function Get-Mo2Context {
    $roots = Get-Mo2RootCandidates
    $iniCandidates = @()
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) {
            continue
        }

        $iniPath = Join-Path $root "ModOrganizer.ini"
        if (Test-Path $iniPath) {
            $iniCandidates += $iniPath
        }

        $nestedInis = Get-ChildItem -Path $root -Filter "ModOrganizer.ini" -Recurse -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty FullName
        if ($null -ne $nestedInis) {
            $iniCandidates += $nestedInis
        }
    }

    $iniCandidates = $iniCandidates | Select-Object -Unique
    $bestContext = $null
    $bestScore = -1
    foreach ($iniPath in $iniCandidates) {
        $root = Split-Path -Parent $iniPath
        $selectedProfile = Decode-Mo2IniValue (Get-IniValue -iniPath $iniPath -section "General" -key "selected_profile")
        $gamePath = Resolve-Mo2Path -rawValue (Get-IniValue -iniPath $iniPath -section "General" -key "gamePath") -mo2Root $root

        $loadOrderPath = $null
        if (-not [string]::IsNullOrWhiteSpace($selectedProfile)) {
            $profileRoot = Join-Path $root (Join-Path "profiles" $selectedProfile)
            $profileCandidates = @(
                (Join-Path $profileRoot "loadorder.txt"),
                (Join-Path $profileRoot "plugins.txt")
            )

            foreach ($candidate in $profileCandidates) {
                if (Test-Path $candidate) {
                    $loadOrderPath = $candidate
                    break
                }
            }
        }

        $score = 0
        if (-not [string]::IsNullOrWhiteSpace($loadOrderPath)) {
            $score += 2
        }
        if (-not [string]::IsNullOrWhiteSpace($gamePath)) {
            $score += 1
        }
        if (-not [string]::IsNullOrWhiteSpace($selectedProfile)) {
            $score += 1
        }

        if ($score -gt $bestScore) {
            $bestScore = $score
            $bestContext = [PSCustomObject]@{
                RootPath = $root
                IniPath = $iniPath
                SelectedProfile = $selectedProfile
                GamePath = $gamePath
                LoadOrderPath = $loadOrderPath
            }
        }
    }

    if ($bestScore -gt 0) {
        return $bestContext
    }

    return $null
}

function Find-DefaultDataPath([object]$mo2Context) {
    if ($null -ne $mo2Context -and -not [string]::IsNullOrWhiteSpace($mo2Context.GamePath)) {
        $mo2Candidates = @(
            (Join-Path $mo2Context.GamePath "Data"),
            $mo2Context.GamePath
        )

        foreach ($path in $mo2Candidates) {
            if ([string]::IsNullOrWhiteSpace($path)) {
                continue
            }

            if (Test-Path (Join-Path $path "Skyrim.esm")) {
                return $path
            }
        }
    }

    $candidates = @(
        "$env:ProgramFiles(x86)\Steam\steamapps\common\Skyrim Special Edition\Data",
        "$env:ProgramFiles(x86)\Steam\steamapps\common\The Elder Scrolls V Skyrim Special Edition\Data",
        "C:\SteamLibrary\steamapps\common\Skyrim Special Edition\Data",
        "D:\SteamLibrary\steamapps\common\Skyrim Special Edition\Data",
        "E:\SteamLibrary\steamapps\common\Skyrim Special Edition\Data",
        "F:\SteamLibrary\steamapps\common\Skyrim Special Edition\Data",
        "G:\SteamLibrary\steamapps\common\Skyrim Special Edition\Data"
    )

    foreach ($path in $candidates) {
        if ([string]::IsNullOrWhiteSpace($path)) {
            continue
        }

        if (Test-Path (Join-Path $path "Skyrim.esm")) {
            return $path
        }
    }

    return $null
}

function Find-DefaultLoadOrderPath([object]$mo2Context) {
    $candidates = @(
        $mo2Context.LoadOrderPath,
        "$env:LOCALAPPDATA\Skyrim Special Edition\plugins.txt",
        "$env:LOCALAPPDATA\Skyrim Special Edition\loadorder.txt"
    )

    foreach ($path in $candidates) {
        if ([string]::IsNullOrWhiteSpace($path)) {
            continue
        }

        if (Test-Path $path) {
            return $path
        }
    }

    $mo2Roots = Get-Mo2RootCandidates
    $searchPatterns = @("loadorder.txt", "plugins.txt")

    foreach ($root in $mo2Roots) {
        if (-not (Test-Path $root)) {
            continue
        }

        foreach ($pattern in $searchPatterns) {
            $found = Get-ChildItem -Path $root -Filter $pattern -Recurse -ErrorAction SilentlyContinue |
                Select-Object -First 1
            if ($null -ne $found) {
                return $found.FullName
            }
        }
    }

    return $null
}

function Select-FolderPath([string]$title, [string]$description, [string]$initialPath) {
    $dialog = New-Object System.Windows.Forms.FolderBrowserDialog
    $dialog.Description = $description
    $dialog.ShowNewFolderButton = $false
    if (-not [string]::IsNullOrWhiteSpace($initialPath) -and (Test-Path $initialPath)) {
        $dialog.SelectedPath = $initialPath
    }

    if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
        throw "Cancelled by user."
    }

    return $dialog.SelectedPath
}

function Select-FilePath([string]$title, [string]$filter, [string]$initialDirectory, [string]$defaultFileName) {
    $dialog = New-Object System.Windows.Forms.OpenFileDialog
    $dialog.Title = $title
    $dialog.Filter = $filter
    $dialog.CheckFileExists = $true
    $dialog.Multiselect = $false

    if (-not [string]::IsNullOrWhiteSpace($initialDirectory) -and (Test-Path $initialDirectory)) {
        $dialog.InitialDirectory = $initialDirectory
    }

    if (-not [string]::IsNullOrWhiteSpace($defaultFileName)) {
        $dialog.FileName = $defaultFileName
    }

    if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
        throw "Cancelled by user."
    }

    return $dialog.FileName
}

function Select-SaveFilePath([string]$title, [string]$initialDirectory, [string]$defaultFileName) {
    $dialog = New-Object System.Windows.Forms.SaveFileDialog
    $dialog.Title = $title
    $dialog.Filter = "ESP files (*.esp)|*.esp|All files (*.*)|*.*"
    $dialog.OverwritePrompt = $true
    $dialog.AddExtension = $true
    $dialog.DefaultExt = "esp"

    if (-not [string]::IsNullOrWhiteSpace($initialDirectory) -and (Test-Path $initialDirectory)) {
        $dialog.InitialDirectory = $initialDirectory
    }

    if (-not [string]::IsNullOrWhiteSpace($defaultFileName)) {
        $dialog.FileName = $defaultFileName
    }

    if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
        throw "Cancelled by user."
    }

    return $dialog.FileName
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$dotnet = Get-Command dotnet -ErrorAction SilentlyContinue
if ($null -eq $dotnet) {
    [System.Windows.Forms.MessageBox]::Show(
        "dotnet runtime/sdk was not found. Install .NET 8+ and try again.",
        "Calamity UserPatch Wizard",
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Error
    ) | Out-Null
    exit 1
}

try {
    $mo2Context = Get-Mo2Context
    $defaultDataPath = Find-DefaultDataPath -mo2Context $mo2Context
    $defaultLoadOrder = Find-DefaultLoadOrderPath -mo2Context $mo2Context

    $dataPath = Select-FolderPath `
        "Select Skyrim Data Folder" `
        "Select Skyrim Data folder that contains Skyrim.esm." `
        $defaultDataPath

    if (-not (Test-Path (Join-Path $dataPath "Skyrim.esm"))) {
        $warn = [System.Windows.Forms.MessageBox]::Show(
            "Selected folder does not contain Skyrim.esm.`nContinue anyway?",
            "Calamity UserPatch Wizard",
            [System.Windows.Forms.MessageBoxButtons]::YesNo,
            [System.Windows.Forms.MessageBoxIcon]::Warning
        )
        if ($warn -ne [System.Windows.Forms.DialogResult]::Yes) {
            throw "Cancelled by user."
        }
    }

    $loadOrderDirectory = $null
    $loadOrderName = "plugins.txt"
    if (-not [string]::IsNullOrWhiteSpace($defaultLoadOrder)) {
        $loadOrderDirectory = Split-Path -Parent $defaultLoadOrder
        $loadOrderName = Split-Path -Leaf $defaultLoadOrder
    }

    $loadOrderPath = Select-FilePath `
        "Select Load Order File (plugins.txt or loadorder.txt)" `
        "Load order (*.txt)|plugins.txt;loadorder.txt|Text files (*.txt)|*.txt|All files (*.*)|*.*" `
        $loadOrderDirectory `
        $loadOrderName

    $outputDirectory = Split-Path -Parent $loadOrderPath
    $outputPath = Select-SaveFilePath `
        "Save User Patch ESP" `
        $outputDirectory `
        $DefaultOutputFileName

    Push-Location $repoRoot
    try {
        & dotnet run --project "tools/CalamityAffixes.UserPatch" -- `
            --spec $SpecPath `
            --masters $dataPath `
            --loadorder $loadOrderPath `
            --output $outputPath
        if ($LASTEXITCODE -ne 0) {
            throw "UserPatch generation failed. Check console output."
        }
    }
    finally {
        Pop-Location
    }

    [System.Windows.Forms.MessageBox]::Show(
        "UserPatch generated successfully.`n`n$outputPath",
        "Calamity UserPatch Wizard",
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Information
    ) | Out-Null
}
catch {
    $message = $_.Exception.Message
    [System.Windows.Forms.MessageBox]::Show(
        "UserPatch generation was cancelled or failed.`n`n$message",
        "Calamity UserPatch Wizard",
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Error
    ) | Out-Null
    exit 1
}

exit 0
