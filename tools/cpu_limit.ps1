# Bride le client du banc pour approcher une console : limite les cœurs (masque d'affinité) dès qu'il apparaît.
# cpu_limit.ps1 -Mask 15   (15 = 0x0F = 4 threads logiques sur 16)
param([int]$Mask = 15)
$t0 = Get-Date
while (((Get-Date) - $t0).TotalMinutes -lt 15) {
  $p = Get-Process ArmaReforgerSteamDiag -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($p) { $p.ProcessorAffinity = [IntPtr]$Mask; "affinite $Mask pid $($p.Id)"; break }
  Start-Sleep 2
}
