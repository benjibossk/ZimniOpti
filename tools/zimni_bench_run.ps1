# Lance un bench Zimnitrita autonome (exe Diag du jeu, monde GM, caméra éditeur), attend la fin, renvoie le CSV.
param([int]$TimeoutMin = 25, [string]$Label = "run")
$game = "D:\SteamLibrary\steamapps\common\Arma Reforger"
$prof = "C:\Users\benbo\Documents\My Games\ZimniBench"
$addons = "C:\Users\benbo\Documents\My Games\ZimniBench\addons,C:\Users\benbo\Documents\My Games\ArmaReforger\addons"
$perf = "$prof\profile\ZimniPerf"
$before = Get-Date
$a = @("-profile", $prof, "-addonsDir", $addons, "-addons", "6ACEAD5654BD416A",
  "-world", "{AE36DA9B85B7512C}World/GameMaster/GM_Zimnitrita.ent",
  "-window", "-screenWidth", "1920", "-screenHeight", "1080", "-forceUpdate", "-noFocus",
  "-noSplash", "-noThrow", "-noBackend", "-noSound", "-disableCrashReporter", "-freezeCheckMode", "kill")
$p = Start-Process -FilePath "$game\ArmaReforgerSteamDiag.exe" -WorkingDirectory $game -ArgumentList (($a | ForEach-Object { if ($_ -match " ") { "`"$_`"" } else { $_ } }) -join " ") -PassThru
if (-not $p.WaitForExit($TimeoutMin * 60000)) { Stop-Process -Id $p.Id -Force; Write-Output "TIMEOUT" }
$csv = Get-ChildItem "$perf\bench_*.csv" | Where-Object { $_.LastWriteTime -gt $before } | Sort-Object LastWriteTime | Select-Object -Last 1
$log = Get-ChildItem "$prof\logs" -Directory -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | Select-Object -Last 1
Write-Output "EXIT=$($p.ExitCode) CSV=$($csv.FullName) LOG=$($log.FullName)"
if ($csv) { Copy-Item $csv.FullName "$perf\$Label.csv" -Force }
