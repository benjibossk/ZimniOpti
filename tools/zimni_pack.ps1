# Packe ZimniOpti (data.pak) dans le dossier d'addons du banc, via le Workbench en CLI. Le Workbench se ferme seul.
param([int]$TimeoutMin = 40)
$wb = "D:\SteamLibrary\steamapps\common\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$game = "D:\SteamLibrary\steamapps\common\Arma Reforger"
$gproj = "C:\Users\benbo\Documents\My Games\ArmaReforgerWorkbench\addons\ZimniOpti\addon.gproj"
$out = "C:\Users\benbo\Documents\My Games\ZimniBench\addons\ZimniOpti"
$addons = "$game\addons,C:\Users\benbo\Documents\My Games\ArmaReforger\addons"
New-Item -ItemType Directory -Force $out | Out-Null
$a = @("-gproj", $gproj, "-addonsDir", $addons, "-wbModule=ResourceManager", "-packAddon", "-packAddonDir", $out, "-noThrow")
$p = Start-Process -FilePath $wb -WorkingDirectory $game -ArgumentList (($a | ForEach-Object { if ($_ -match " ") { "`"$_`"" } else { $_ } }) -join " ") -PassThru
if (-not $p.WaitForExit($TimeoutMin * 60000)) { Stop-Process -Id $p.Id -Force; Write-Output "TIMEOUT" }
Write-Output "EXIT=$($p.ExitCode)"
Get-ChildItem $out | Select-Object Name, Length | Format-Table -AutoSize | Out-String
