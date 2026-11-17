[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [ValidateRange(5,30)][int]$DurationSeconds=20
)
$ErrorActionPreference='Stop'
$principal=New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'This header-only ETW session requires an administrator token.'
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$session='gchd_headers_'+[Guid]::NewGuid().ToString('N')
$etl=Join-Path $OutputDirectory 'usb-headers.etl'
$xml=Join-Path $OutputDirectory 'usb-headers.xml'
$logman='C:\Windows\System32\logman.exe'
foreach ($name in @('usb-headers.etl','usb-headers.xml','ready.txt','complete.txt')) {
    if (Test-Path -LiteralPath (Join-Path $OutputDirectory $name)) { throw "Existing trace output: $name; choose a fresh directory" }
}
$started=$false
try {
    # 0x8041 = Default | HeadersBusTrace | Rundown.
    # Neither PartialDataBusTrace nor FullDataBusTrace is enabled.
    & $logman start $session -p Microsoft-Windows-USB-USBXHCI 0x8041 5 -o $etl -ets -nb 16 128 -bs 64 -f bincirc -max 32
    if ($LASTEXITCODE -ne 0) { throw 'Could not start USB header trace.' }
    $started=$true
    foreach ($provider in @('Microsoft-Windows-USB-UCX','Microsoft-Windows-USB-USBHUB3')) {
        & $logman update $session -p $provider 0x8041 5 -ets
        if ($LASTEXITCODE -ne 0) { throw "Could not enable provider: $provider" }
    }
    [IO.File]::WriteAllText((Join-Path $OutputDirectory 'ready.txt'),$session)
    Start-Sleep -Seconds $DurationSeconds
}
finally {
    if ($started) {
        & $logman stop $session -ets
        if ($LASTEXITCODE -ne 0) { throw "Could not stop ETW session $session; stop it with logman stop $session -ets" }
    }
}
if (-not (Test-Path -LiteralPath $etl)) {
    $captured=Get-ChildItem -LiteralPath $OutputDirectory -Filter 'usb-headers*.etl' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $captured) { throw 'No ETL trace file was produced.' }
    $etl=$captured.FullName
}
& 'C:\Windows\System32\tracerpt.exe' $etl -o $xml -of XML -y
if ($LASTEXITCODE -ne 0) { throw 'Trace stopped, but XML export failed.' }
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'complete.txt'),$xml)
