$ErrorActionPreference = "Stop"
$PSNativeCommandUseErrorActionPreference = $false

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Command
    )

    & $Command[0] $Command[1..($Command.Length - 1)]
    if ($LASTEXITCODE -ne 0) {
        throw ("Command failed: " + ($Command -join " "))
    }
}

function Invoke-Quiet {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Command
    )

    $commandLine = ($Command | ForEach-Object {
        if ($_ -match "\s") {
            '"' + $_ + '"'
        } else {
            $_
        }
    }) -join " "

    cmd /c "$commandLine >nul 2>nul" | Out-Null
}

function Invoke-DockerBuild {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Dockerfile,
        [Parameter(Mandatory = $true)]
        [string]$Image,
        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    cmd /c "set DOCKER_BUILDKIT=0&& docker build -f $Dockerfile -t $Image $Context"
    if ($LASTEXITCODE -ne 0) {
        throw ("Command failed: docker build -f " + $Dockerfile + " -t " + $Image + " " + $Context)
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\\..")).Path
$imageName = "favorite-widget-pages-audit"
$containerName = "favorite-widget-pages-audit-run"
$buildRoot = Join-Path $repoRoot "build"
$artifactRoot = Join-Path $buildRoot "playwright"

if (Test-Path $artifactRoot) {
    Remove-Item -Recurse -Force $artifactRoot
}

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null

Invoke-DockerBuild -Dockerfile (Join-Path $repoRoot "tools/test/Dockerfile.playwright") -Image $imageName -Context $repoRoot

$runExitCode = 0

try {
    Invoke-Quiet @("docker", "rm", "-f", $containerName)

    Invoke-Checked @("docker", "create", "--name", $containerName, $imageName)

    & docker start -a $containerName
    $runExitCode = $LASTEXITCODE

    Invoke-Checked @("docker", "cp", "${containerName}:/work/build/playwright", $buildRoot)
}
finally {
    Invoke-Quiet @("docker", "rm", "-f", $containerName)
    Invoke-Quiet @("docker", "image", "rm", $imageName)
}

if ($runExitCode -ne 0) {
    exit $runExitCode
}
