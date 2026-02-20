[CmdletBinding()]
param(
    [string]$SpecPath = "affixes/affixes.json",
    [string]$DefaultOutputFileName = "CalamityAffixes_UserPatch.esp"
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

function Find-DefaultDataPath {
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

function Find-DefaultLoadOrderPath {
    $candidates = @(
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

    $mo2Roots = @(
        "$env:LOCALAPPDATA\ModOrganizer",
        "$env:LOCALAPPDATA\ModOrganizer2",
        "$env:USERPROFILE\AppData\Local\ModOrganizer",
        "$env:USERPROFILE\AppData\Local\ModOrganizer2"
    )

    foreach ($root in $mo2Roots) {
        if (-not (Test-Path $root)) {
            continue
        }

        $found = Get-ChildItem -Path $root -Filter "loadorder.txt" -Recurse -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($null -ne $found) {
            return $found.FullName
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
    $defaultDataPath = Find-DefaultDataPath
    $defaultLoadOrder = Find-DefaultLoadOrderPath

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
