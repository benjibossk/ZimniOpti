# Banc CLIENT/SERVEUR automatique pour ZimniOpti.
#  1) télécharge (une fois) les mods Workshop demandés via le serveur dédié (mode -config)
#  2) lance le serveur dédié local (exe D:\Serveur, port 2302, profil séparé) sur le scénario ZO_Bench + mods
#  3) lance le jeu qui se connecte (réessaie tant que le serveur charge) et fait le banc (bench_points.txt)
#  4) ferme tout, copie le CSV client (<Label>.csv) et le CSV perf serveur (<Label>_serveur.csv)
# usage: zimni_server_bench.ps1 -Label x [-Points war8.txt] [-Mods "GUID1,GUID2"] [-TimeoutMin 25] [-DownloadOnly] [-Vcons 100]
#  -Vcons N : serveur DIAG avec N connexions virtuelles (-rpl-vcons) = charge réplication de N joueurs
param([string]$Scenario = "{5A0B7E5C0FFEE001}Missions/ZO_Bench.conf", [string]$Label = "srv", [string]$Points = "", [string]$Mods = "", [int]$TimeoutMin = 25, [switch]$DownloadOnly, [int]$Vcons = 0, [int]$PingMs = 0, [string]$SrvArgs = "")
$prof = "C:\Users\benbo\Documents\My Games\ZimniBench"
$srvAddons = "$prof\srvaddons"                      # mods téléchargés par le serveur
$addons = "$prof\addons,C:\Users\benbo\Documents\My Games\ArmaReforger\addons,$srvAddons\addons"
$srvDir = "D:\Serveur\arma-reforger\server"
$game = "D:\SteamLibrary\steamapps\common\Arma Reforger"
$perf = "$prof\profile\ZimniPerf"
$srvPerf = "$prof\srvprofile\profile\ZimniPerf"
$base = "597697D81A1EA202,672166DE5926F11F,6ACEAD5654BD416A"   # Zimnitrita, MinusBuildingPack, ZimniOpti
$q = { param($x) ($x | ForEach-Object { if ($_ -match " ") { "`"$_`"" } else { $_ } }) -join " " }
New-Item -ItemType Directory -Force $srvAddons, $srvPerf | Out-Null
"" | Out-File -Encoding ascii "$srvPerf\allowwar"     # autorise le RPC « guerre » sur CE serveur de test uniquement
if ($Points) { Copy-Item "$perf\$Points" "$perf\bench_points.txt" -Force }
$modList = @(); if ($Mods) { $modList = $Mods.Split(",") | ForEach-Object { $_.Trim() } | Where-Object { $_ } }

function Get-SrvLog { Get-ChildItem "$prof\srvprofile\logs" -Directory -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | Select-Object -Last 1 }
function Test-Downloaded($g) { [bool](Get-ChildItem "$srvAddons\addons", "C:\Users\benbo\Documents\My Games\ArmaReforger\addons" -Directory -Filter "*_$g" -ErrorAction SilentlyContinue) }

# ---- 1) téléchargement des mods manquants (mode -config, scénario vanilla, on coupe dès que c'est là)
$missing = $modList | Where-Object { -not (Test-Downloaded $_) }
if ($missing) {
  Write-Output "TELECHARGEMENT : $($missing -join ',')"
  $cfg = @{ bindAddress = "127.0.0.1"; bindPort = 2303; publicAddress = "127.0.0.1"; publicPort = 2303
    a2s = @{ address = "127.0.0.1"; port = 17791 }
    game = @{ name = "ZimniBench download"; scenarioId = "{ECC61978EDCC2B5A}Missions/23_Campaign.conf"; maxPlayers = 1; visible = $false
      crossPlatform = $false; gameProperties = @{ battlEye = $false }
      mods = @($missing | ForEach-Object { @{ modId = $_ } }) } }
  $cfg | ConvertTo-Json -Depth 6 | Out-File -Encoding ascii "$prof\server_download.json"
  $da = @("-config", "$prof\server_download.json", "-profile", "$prof\srvprofile", "-addonsDir", $srvAddons, "-addonDownloadDir", $srvAddons)
  $d = Start-Process -FilePath "$srvDir\ArmaReforgerServer.exe" -WorkingDirectory $srvDir -ArgumentList (& $q $da) -PassThru -WindowStyle Minimized
  for ($i = 0; $i -lt 180; $i++) {
    Start-Sleep 5
    if (-not ($modList | Where-Object { -not (Test-Downloaded $_) })) { break }
    if ($d.HasExited) { break }
  }
  Start-Sleep 5; if (-not $d.HasExited) { Stop-Process -Id $d.Id -Force }
  $still = $modList | Where-Object { -not (Test-Downloaded $_) }
  if ($still) { Write-Output "TELECHARGEMENT KO : $($still -join ',') (voir log srvprofile)"; exit 1 }
  Write-Output "TELECHARGEMENT OK"
}
if ($DownloadOnly) { exit 0 }

# ---- 2) serveur dédié de test (mode ligne de commande : seul moyen de charger ZimniOpti non publié)
$all = (@($base.Split(",")) + $modList) -join ","
$before = Get-Date
$sa = @("-server", $Scenario, "-bindIP", "127.0.0.1", "-bindPort", "2302",
  "-profile", "$prof\srvprofile", "-addonsDir", $addons, "-addons", $all, "-maxFPS", "60", "-logStats", "10000", "-noThrow")
$exe = "ArmaReforgerServerDiag.exe"   # le client Diag (dev) est refusé par un serveur non-Diag (isDevBinary)
if ($Vcons -gt 0) { $sa += @("-rpl-vcons", "$Vcons") }
if ($SrvArgs) { $sa += $SrvArgs.Split(" ") }   # ex: "-streamingBudget 300 -streamsDelta 50"
$s = Start-Process -FilePath "$srvDir\$exe" -WorkingDirectory $srvDir -ArgumentList (& $q $sa) -PassThru -WindowStyle Minimized
# surveille le log : un mod qui ne compile pas ouvre une fenêtre d'erreur bloquante -> on arrête CE serveur
$broken = $false
for ($i = 0; $i -lt 6 -and -not $broken; $i++) {
  Start-Sleep 5
  $l = Get-SrvLog
  if ($l -and $l.LastWriteTime -gt $before) {
    $bad = Select-String -Path "$($l.FullName)\console.log" -Pattern "Can't compile" -ErrorAction SilentlyContinue | Select-Object -First 3
    if ($bad) { $broken = $true; Write-Output "MOD INCOMPATIBLE :"; $bad | ForEach-Object { Write-Output "  $($_.Line)" } }
  }
}
if ($broken) { if (-not $s.HasExited) { Stop-Process -Id $s.Id -Force }; exit 1 }
if ($s.HasExited) { Write-Output "SERVEUR KO (log $((Get-SrvLog).FullName))"; exit 1 }
Write-Output "SERVEUR LANCE (addons $all)"

# ---- ping simulé : clumsy (WinDivert, droits admin -> fenêtre UAC) sur le trafic du port 2302
$cl = $null
if ($PingMs -gt 0) {
  $half = [int]($PingMs / 2)   # appliqué dans les 2 sens -> aller-retour = PingMs
  $cla = "--filter `"udp.DstPort == 2001 or udp.SrcPort == 2001`" --lag on --lag-time $half --drop on --drop-chance 1"
  $cl = Start-Process -FilePath "C:\Users\benbo\tools\clumsy\clumsy.exe" -ArgumentList $cla -Verb RunAs -PassThru -WindowStyle Minimized
  Write-Output "PING simule ${PingMs} ms + 1% pertes"
}

# ---- 3) client : se connecte, réessaie tant que le serveur charge la carte
$ca = @("-profile", $prof, "-addonsDir", $addons, "-client", "127.0.0.1:2001", "-addons", $all,
  "-window", "-screenWidth", "1920", "-screenHeight", "1080", "-forceUpdate", "-noFocus",
  "-noSplash", "-noThrow", "-noSound", "-disableCrashReporter", "-freezeCheckMode", "kill")
$done = $false
for ($t = 1; $t -le 10; $t++) {
  if ($s.HasExited) { Write-Output "SERVEUR MORT pendant l'attente"; break }
  $c = Start-Process -FilePath "$game\ArmaReforgerSteamDiag.exe" -WorkingDirectory $game -ArgumentList (& $q $ca) -PassThru
  if ($c.WaitForExit(120000)) { Write-Output "client sorti vite (essai $t) : serveur pas pret"; Start-Sleep 20; continue }
  # attente de fin ; si le serveur éjecte le client (FLOODED…), on le note et on coupe tout de suite
  $deadline = (Get-Date).AddMinutes($TimeoutMin)
  while (-not $c.HasExited) {
    Start-Sleep 5
    $clog = Get-ChildItem "$prof\logs" -Directory | Sort-Object CreationTime | Select-Object -Last 1
    $k = Select-String -Path "$($clog.FullName)\console.log" -Pattern "Kick cause code: (.*)" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($k) { Write-Output "KICK $($k.Matches[0].Groups[1].Value) a $($k.Line.Substring(0,12))"; Stop-Process -Id $c.Id -Force; break }
    if ((Get-Date) -gt $deadline) { Stop-Process -Id $c.Id -Force; Write-Output "TIMEOUT client"; break }
  }
  $done = $true; break
}

# ---- 4) fin
if ($cl) { Get-Process clumsy -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue }
if (-not $s.HasExited) { Stop-Process -Id $s.Id -Force }
$csv = Get-ChildItem "$perf\bench_*.csv" -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt $before } | Sort-Object LastWriteTime | Select-Object -Last 1
if ($csv) { Copy-Item $csv.FullName "$perf\$Label.csv" -Force }
$sp = Get-ChildItem "$srvPerf\perf_*.csv" -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | Select-Object -Last 1
if ($sp) { Copy-Item $sp.FullName "$perf\${Label}_serveur.csv" -Force }
Write-Output "FIN connecte=$done CSV=$($csv.Name) SERVEUR_CSV=$($sp.Name) LOGSRV=$((Get-SrvLog).FullName)"
