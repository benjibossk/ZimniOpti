# Échantillonne toutes les 10 s : heure, VRAM dédiée, mémoire privée (commit), working set du client (Mo) : vram_sampler.ps1 -Out fichier
# S'arrête tout seul 60 s après la fermeture du client (ou après 3 h au plus).
param([string]$Out)
"" | Out-File -Encoding utf8 $Out
$seen = $false; $miss = 0; $t0 = Get-Date; $pid0 = $null
while (((Get-Date) - $t0).TotalHours -lt 3) {
  if ($pid0) { $p = Get-Process -Id $pid0 -ErrorAction SilentlyContinue } else { $p = Get-Process ArmaReforgerSteamDiag -ErrorAction SilentlyContinue | Select-Object -First 1; if ($p) { $pid0 = $p.Id } }
  if ($p) {
    $seen = $true; $miss = 0
    $c = Get-Counter "\GPU Process Memory(pid_$($p.Id)*)\Dedicated Usage" -ErrorAction SilentlyContinue
    $v = ($c.CounterSamples | Measure-Object CookedValue -Sum).Sum / 1MB
    $p.Refresh(); "{0} {1:F0} {2:F0} {3:F0}" -f (Get-Date -Format HH:mm:ss), $v, ($p.PrivateMemorySize64 / 1MB), ($p.WorkingSet64 / 1MB) | Out-File -Append -Encoding utf8 $Out
  } elseif ($seen) {
    $miss++
    if ($miss -ge 6) { break }
  }
  Start-Sleep 10
}
