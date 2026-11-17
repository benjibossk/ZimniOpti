# Mémoire d'optimisation Zimnitrita (ZimniOpti)

Une fiche par trouvaille : **constat chiffré → cause → piste → test → résultat**.
Statuts : 🔎 constat · 🧪 en test · ✅ gagné · ❌ sans effet / rejeté.
Banc : `zimni_pack.ps1` puis `zimni_bench_run.ps1 -Label <nom>` puis `zimni_bench_compare.py reference.csv <nom>.csv`
(CSV dans `Documents\My Games\ZimniBench\profile\ZimniPerf\`). PC de mesure : Ryzen 7 3800X, RX 9070 XT, plafond ~144 FPS.

---

## 🌙 COMPTE RENDU DE LA NUIT (2026-11-09, ~23h50 → 2h15, tout en automatique)
**Résultat principal : la distance de vue est LE levier.** Plafonnée à 1200 m, grille complète 2 km (64 pts × 4 dir) :
moyenne **131 → 141 FPS**, pire point **73 → 95 FPS**, zones < 90 FPS : **8 → 0**. (heatmaps `heat_g2_ref.png` / `heat_g2_cap.png`)

| Testé | Effet | Fiche |
|---|---|---|
| Distance de vue 2500 → 1200 m | **+13 % moy., jusqu'à +50 % sur les pires points** ✅ | F5 |
| AutoOccluders terrain 0 / 1 | rien ❌ | F6 |
| PixelSizeScale (LOD) rails/palettes/blocs 0,5 et 0,2 | rien ❌ | F8 |
| Ombres, ombres lointaines, détail géométrique, environnement (en jeu) | ≤ +2 % ❌ | F8 |
| Distance objets Low / toute la config en Low (au démarrage) | +1 % / +3 % ❌ | F8 |

- **Pourquoi** : sur ce PC la carte est limitée par le CPU = la quantité d'objets visibles jusqu'à 2,5 km
  (3 millions d'entités, surtout de la végétation générée — F7). Les réglages GPU ne changent rien ; réduire la vue, oui.
- **Ajouté dans ZimniOpti (PAS publié)** : `Scripts/Game/ZimniOpti/ZO_ConsoleViewCap.c` — uniquement sur Zimnitrita,
  plafond 1200 m (Series S, Xbox One, PS4) / 1600 m (Series X, PS5), PC intact. Vérifié qu'il tient (le jeu remet
  la valeur du menu, on rebaisse toutes les 2 s). Banc enrichi (`viewdist`, `engset`, `forcecap`).
- **À faire / décider avec Benji** :
  1. OK pour les valeurs 1200/1600 ? (sur console la distance par défaut est peut-être déjà plus basse : à vérifier en vrai)
  2. Tester en jeu sur console (Series S idéalement) avec ZO_PerfMonitor.
  3. Piste suivante si besoin de plus : alléger la végétation générée autour des villes (forêts/broussailles) — chantier plus lourd.
  4. Contrôle visuel des textures réduites (F1) toujours à faire.



## F1 — Les textures 4096 pèsent ~2,4 Gio de VRAM — ✅ appliqué, −0,7 Go VRAM mesuré (rendu en jeu à confirmer)
- **Constat** : 118 textures 4096 px (Hotel ≈1 Gio, Infrastructure 750 Mio). Series S ≈ 8 Go partagés CPU+GPU.
- **Piste** : retirer le mip du haut (EDDS sans recompression) → 2048 px, −1,8 Gio.
- **Test** : 115 textures remplacées dans ZimniOpti (`zimni_textures.py`).
- **Résultat** : mémoire du banc ≈ 5,0 Go stable (pas de comparaison avant/après faite).
- **Mesure A/B 2026-11-10 (nuit)** : compteur Windows « GPU Process Memory » du client (`vram_sampler.ps1`), avec et sans le dossier `Assets` de ZimniOpti.
  - Trajet des villes (T1, X11Z9…) : VRAM max 4 428 contre 4 492 Mo → aucune différence (ces points ne voient pas les bâtiments texturés).
  - Sites texturés (`textures.txt` : hôtel, prison, centrale) : VRAM **moyenne 4 089 contre 4 805 Mo (−716 Mo)**, max 4 363 contre 5 124 Mo (−760 Mo). La RAM (`memoireMo`) et les FPS ne bougent pas.
  - ✅ **Gain réel ≈ −0,7 Go de VRAM là où ces bâtiments sont visibles.** Sur console la mémoire est partagée, donc ce gain s'ajoute à celui des chunks (F18). Contrôle visuel du rendu toujours à faire.

## F2 — Les villes coûtent cher, la campagne rien — 🔎
- **Constat (référence 2026-11-08)** : zones vides ≈ 144 FPS (plafond). Villes de l'est :
  X12Z10 50 FPS (vers le sud), X11Z9 62, X12Z9 70, X10Z5 Gruwice 78, X11Z11 83.
  Même point : 143 → 50 FPS selon la direction → c'est ce qui est **dans le champ de vue** qui coûte.
- **Cause supposée** : trop d'objets dessinés à distance (trottoirs, bordures en `$grp StaticModelEntity`),
  presque pas de `LODFactors` (24 dans la carte vs ~470 prefabs Bohemia), presque pas d'`Occluder` (39).
- **Pistes** :
  1. distance d'affichage max sur les petits objets des villes ;
  2. LODFactors sur les prefabs de la carte qui reviennent le plus souvent ;
  3. occluders sur les gros bâtiments / murs pour cacher ce qui est derrière.
- **Relevés (analyses\zones\, 2026-11-08)** : dans le cône 500 m de la pire direction, 33-73 % de petits props.
  MAIS le nombre d'objets n'explique pas la direction lente : r = 0,04 sur les 28 couples point×direction
  (X11Z9 : 5 614 objets vers l'est à 143 FPS, 2 065 vers le sud à 62). Densité à 150 m : r = −0,56 (léger).
  → Le coût vient d'AUTRE CHOSE que le nombre : géométrie lourde, ombres/lumières, ou ce qui est vu au-delà de 500 m.
  Prefabs du jeu de base (386/400 les plus fréquents) ont déjà LODFactors (364) ; AUCUN prefab n'a de distance max.
- **Prochain test** : bench à distance de vue réduite / ombres coupées pour isoler la cause (à faire).

## Boîte à outils moteur (recherche 2026-11-08 ; V = vérifié, D = déduit)
- `Occluder` : déjà sur `Building_Base.et` (tous les bâtiments vanilla) (V). `Occluder { Enabled 0 }` pour couper.
  Pas trouvé : WorldOccluderEntity utilisé, occlusion par le terrain réglable (D) → pas de levier « terrain ».
- `MeshObject.LODFactors {5 valeurs}` + `PixelSizeScale` : plus petit = bascule plus tôt en LOD bas (D).
- `MaxDrawDistance` (m) sur entités/prefabs : Bohemia 75-375 m sur petits objets ; Zimnitrita l'utilise 9 fois (550) (V).
- Clutter (herbe) : `Configs/Clutter/c_*.conf`, `MaxDrawDistance` en ratio 0.3-1 (V) ; `ClutterOccluder` retire l'herbe sous un objet.
- Forcer sur console : pas d'ini. Possible par script : `System.GetPlatform()` + `GetGame().SetViewDistance(m)` (V API) ;
  réglages vidéo via `GetGame().GetEngineUserSettings()` (noms de modules à confirmer). `.meta` par plateforme
  (XBOX_SERIES / PS5) pour textures = levier mémoire anti-crash.
- Le `.ent` du monde ne contient aucun réglage distance/ombre (V).

## Ce que font les autres jeux (synthèse de connaissances générales, sources NON vérifiées en ligne)
1. Budget mémoire fixe par console, textures « mip −1 » sur Series S → déjà fait (F1), à passer par plateforme.
2. Distance d'affichage par catégorie (GTA/RDR2, Unreal Cull Distance Volumes) : petits props 50-150 m → `MaxDrawDistance`.
3. Occlusion : bâtiments déclarés occluders + relief (Frostbite, AC Unity) → `Occluder`, `AutoOccluders`.
4. Level design : rues coudées, perspectives fermées (Spider-Man, GT) → arbre/mur/bâtiment en bout des longues rues.
5. HLOD / fusion de props lointains : pas automatique dans Enfusion → plutôt réduire la densité.
6. Peu de modèles différents, beaucoup de copies (instancing) → réduire la variété de props en ville.
7. Herbe/clutter réduits sur console. 8. Presets console forcés → chez nous seulement `SetViewDistance` en script.
- GT tient 60 fps car monde fermé + caméra sur trajectoire connue → visibilité précalculée virage par virage ; non transposable sauf le point 4.

## F5 — Distance de vue — ✅ LEVIER N°1 (mesuré 2026-11-09)
- **Test** : 10 points ciblés (`lod10.txt` : 8 zones rails/chantiers + T1 X12Z10 + T2), 4 dir, pitch −10°, exe du jeu.
  Défaut PC du banc = 2500 m. Pire direction par point (FPS) :

| pt | réf | réf bis | vue 1500 | vue 1000 |
|---|---|---|---|---|
| R4 X11Z8 | 79 | 80 | 110 | 122 |
| R5 X7Z6 | 95 | 95 | 119 | 133 |
| R6 X10Z4 | 79 | 84 | 98 | 110 |
| R7 X11Z4 | 93 | 93 | 116 | 142 |
| T1 X12Z10 (ville) | 60 | 62 | 71 | 79 |
| **moyenne tout** | 108,2 | 109,2 | 119,1 | 126,4 |

- **Conclusion** : ce qu'on voit au loin (au-delà de 1-1,5 km) coûte jusqu'à 35 % des FPS. En ville (T1) le gain est
  moindre (+27 %) : là c'est la densité proche qui domine.
- **Appliqué** : `Scripts/Game/ZimniOpti/ZO_ConsoleViewCap.c` — sur Zimnitrita uniquement, plafond 1200 m (Series S,
  One, PS4) / 1600 m (Series X, PS5), PC inchangé. Revérifié toutes les 2 s, ne fait que baisser (le jeu remet la valeur
  du menu vidéo à l'ouverture inventaire etc.).
- **Validé sur PC** (fichier `ZimniPerf/forcecap` = simule Series S) : en-tête CSV `vue=1200`, moyenne 123,7 (= test manuel 123,1).
  Courbe complète (moy. 10 pts) : 2500 → 108 ; 2000 → 113,5 ; 1600 → 118,3 ; 1500 → 119,1 ; 1200 → 123,1 ; 1000 → 126,4.
- **À confirmer** : la valeur par défaut console (peut-être déjà < 2500) ; test en vrai sur console.

## F4 — Prefabs propres à Zimnitrita : ni LOD, ni occluder, ni distance max — 🔎 cible n°1
- **Constat** : `RailDRoit19m01` 3 460 instances × .xob 390 Ko, `RailDroit09m1` 952 × 420 Ko,
  `Palette_Parpaing_01` 180 × 1 Mo, `Road_PlasticBlock_01` 388, `RoadBlockEnd2m` 324 — tous sans LODFactors.
  (classement complet : analyses\zones\rentables_map.csv)
- **Piste** : override de ces prefabs dans ZimniOpti avec `LODFactors` + distance max ; vérifier que les .xob ont des LOD.
- **Test** : à faire, comparer sur les points avec voies ferrées.

## ⚠️ ERRATUM banc (2026-11-08) : la référence 18 points avait les angles faux
- `Math3D.AnglesToMatrix` = (YAW, PITCH, roll) ; l'entité `SetAngles` = (PITCH, YAW, roll). Le banc passait (0, dir, 0)
  → « 90° » et « 270° » regardaient le CIEL / le SOL (d'où 143 fps), « 0 » = nord, « 180 » = sud à l'envers.
- Conséquence : la conclusion « même nombre d'objets vers l'est à 143 fps » de F2 est FAUSSE. Seuls N et S sont valables.
- Corrigé + caméra propre spawnée (méthode SCR_FPSDiagnosticPlugin) ; nouvelle référence = grille 1 km (256 pts, pitch −10°).

## Outils heatmap (2026-11-08)
- Source du plugin officiel : `tools\PakInspector\base_scripts_extract\scripts\WorkbenchGame\WorldEditor\SCR_FPSDiagnosticPlugin.c`
  (scripts du jeu extraits : `base_scripts_extract`, 5 871 .c).
- `zimni_grid_points.py` (grille, ligne `#cfg settle sample pitch`), `zimni_heatmap.py` (PNG + zones lentes),
  `zimni_series.sh <points> <timeout> nom:AutoOccluders ...` (pack + bench par variante).

## F6 — AutoOccluders du terrain — ❌ sans effet (2026-11-09)
- **Test** : grille 2 km (64 pts × 4 dir, pitch −10°), 3 variantes : original / `AutoOccluders 0` / `AutoOccluders 1`.
- **Résultat** : moyenne 131,4 / 130,6 / 131,6 FPS ; points < 90 FPS : 8 / 9 / 8. Écart = bruit.
  Les 2 creux de la référence (G3000_1000 O 74, G5000_7000 S 76 → 140+ ailleurs) = pics de streaming, pas l'occlusion.
- **Conclusion** : le réglage existe mais ne change rien ici (valeur par défaut probablement déjà active). Ne pas compter dessus.
- Zones lentes confirmées (toutes variantes) : X9Z5 est (~75), X11Z9 est (81), X13Z9 ouest (80), X11Z11 sud (84), X13Z11 sud (85).
  Heatmap : `analyses\heat_g2_ref.png`.

## F7 — Recensement des entités de la carte — 🔎 (2026-11-09)
- **Constat** : 3 072 848 entités posées dans les layers ; l'écrasante majorité = végétation générée (ronces, noisetiers,
  roseaux, arbres : chacun 50-150 k). Hors végétation : bordures béton 10,8 k, cultures (tomate/chou/poivron ~16 k),
  clôtures ~25 k, rails 3,5 k, lignes électriques 4 k.
- **Zone lente X9Z5 est** (carré 1,2 km) : 8 940 entités dont **460 barils de carburant** `BarrelFuel_01_Base`
  (prefab destructible + RplComponent = coûte en CPU/réseau, pas seulement en rendu), 360 tas de déchets, 136 piles de cartons.
- **Pistes** : (1) remplacer les props destructibles décoratifs en masse (barils) par une version statique sans Rpl/destruction ;
  (2) réduire la densité de végétation générée près des villes (forêts/broussailles = gros volume) — à mesurer.
- Outil : `PakTools/zimni_census.py <x> <z> <demi-côté>` (lancer depuis zimnitrita_extract/World).

## F8 — Réglages graphiques moteur (ombres, détail…) — ❌ quasi sans effet sur PC (2026-11-09)
- **Méthode** : le banc applique avant mesure un fichier `ZimniPerf/engset` (« Module Cle type valeur ») via
  `GetEngineUserSettings().GetModule(..).Set(..)` + `UserSettingsChanged()`, puis remet les valeurs d'origine.
  Modules trouvés dans le menu vidéo : `GraphicsQualitySettings.ObjectDrawDistanceScale` (enum Low/…),
  `PipelineUserSettings.ShadowQuality`, `VideoUserSettings.DistantShadowsQuality / EnvironmentQuality / ResolutionScale`,
  `ResourceManagerUserSettings.GeometricDetail`, `GrassMaterialSettings.Lod / Distance`, `TerrainGenMaterialSettings.*`.
- **Résultat** (10 pts, moyenne FPS ; réf 108,6) : ombres 0 → 111,2 (+2 %) ; ombres lointaines 0 → 109,2 ; détail géom. 0 → 109,6 ;
  environnement 0 → 108,9. Tout est dans le bruit. Pour comparaison distance de vue 1200 m → 123,1 (+13 %).
- **Lecture** : sur ce PC (RX 9070 XT) la carte est limitée par le **CPU** (nombre d'objets à envoyer au rendu),
  pas par le GPU. D'où : couper des effets GPU ne sert à rien, réduire ce qui est visible (distance) sert.
  ⚠️ Sur console le GPU est bien plus faible : les ombres pourraient y compter davantage — non mesurable ici.
- Note : le PC de Benji est déjà tout en Low (`ReforgerEngineSettings.conf`).
- **Vérif au démarrage** (réglages écrits dans `ReforgerEngineSettings.conf` du profil banc AVANT lancement, défaut banc =
  ObjectDrawDistance Ultra / ombres High) : objets Low → 109,3 ; **toute la config de Benji en Low** → 111,5 (+3 %).
  Le « Low » du jeu ne règle pas le problème de Zimnitrita ; seule la distance de vue agit.
- **PixelSizeScale** 0,5 et 0,2 sur rails/palettes/blocs (overlay `staging/lod05`, `lod02`) : 109,6 / 109,2 → ❌ rien.

## F3 — Pics à 200 ms (saccades) — 🔎
- **Constat** : X2Z1, X12Z9, X0Z10 ont une frame de 200 ms juste après arrivée → chargement streaming probable.
- **Piste** : à revoir après F1/F2 ; sur console c'est là que les crashs mémoire peuvent arriver.

## F9 — Éjections « REPLICATION FLOODED » + 247k entités statiques répliquées (2026-11-09)
- **Constat** : banc client/serveur (serveur Diag local, ZO_Bench, `#war 8 12 15`) → le client est éjecté `group=1 REPLICATION reason=5 FLOODED` au point R6 (10750,4750), **4 fois sur 4** sans réglage, et **même sans explosions** (`#war 8 12 0`). Serveur : 60 FPS hors pics, gel 2,5 s à la téléportation, Mem 8,6 Go, `Streaming(Static: 246 901)` contre **44 306 sur FFANIZAY**.
- **Cause** : la carte réplique ~5,6× plus d'objets statiques. Recensement (scratchpad/rpl.py) : ≈ 222k répliqués = classes destructibles sans Rpl (`SCR_DestructibleEntity` 128k, `SCR_DestructibleBuildingEntity` 25k, `SCR_PowerPole` 4,7k) + ~89k prefabs avec `RplComponent`. **Murs et clôtures ≈ 100k** (Pole 29,5k, Wooden 19,7k, Net 18,9k, Concrete 17,4k, Stone 10k), routes (bornes et glissières) 14,4k, petits props destructibles, lignes électriques 4,7k, **rails de la map 4 412 avec un Rpl inutile** (RailDRoit19m01, RailDroit09m1). Arbres, rochers et buissons : **non** répliqués.
- **Pistes** : (1) retirer le Rpl des rails de la map (sans impact sur le gameplay) ; (2) passer les clôtures et murs « décor » (hors villes et lignes de front) en variantes non destructibles, ce qui change le gameplay : à décider avec les auteurs ; (3) côté serveur, `-streamsDelta 50` ou `-streamingBudget 300` évitent l'éjection mais retardent fortement l'arrivée des IA (0 visibles en 30 s) ; `-streamingBudget 1000` ne suffit pas.
- **Matrice d'éjection (client Diag + serveur Diag local, 4 points T1→R4→R6→T2)** :
  | variante | éjection | IA vues |
  |---|---|---|
  | guerre 8 + 12 véh + boom (×3) | OUI, au changement de zone | 120 |
  | sans boom / IA seules | OUI | 120 |
  | aucune guerre (séjour 20 s et 2 min) | non | 0 |
  | streamingBudget 1000 | OUI | — |
  | streamingBudget 300 + delta 50 / delta 50 seul | non | **0 en 30 s** (retard massif) |
  | delta 50, séjour 2 min | OUI (à la 1re téléportation) | 120 |
  | 100 vcons | OUI ; serveur 35-45 FPS en bataille | 120 |
  | 100 vcons + delta 50 | non | 0 |
- **Conclusion** : le déclencheur est l'arrivée d'un joueur dans une zone où ~120 IA combattent, sur une carte à 247k objets répliqués. Les réglages serveur évitent l'éjection en différant le flux (les IA n'arrivent pas) : ce n'est pas une solution. Le levier de fond est de réduire les objets répliqués (clôtures et murs, rails).
- **Pièges du banc** : un client Diag exige un serveur Diag (`isDevBinary`) ; `-server` écoute sur **2001** (et ignore `-bindPort`) ; le GM est donné automatiquement par ZO_BenchGM (allowwar) ; les champs de la série sont `label:points:mods:cap:vcons:ping:srvArgs`.

### F9 — suite 2026-11-09 : ce n'est PAS le nombre d'objets statiques
- Testé : rails sans RplComponent (22 prefabs), murs/clôtures en StaticModelEntity (d'abord 615 prefabs surchargés = sans effet car **la classe écrite dans le calque l'emporte sur le prefab** ; puis 17 582 lignes de calques modifiées). Compteur `Static` **inchangé (246 901)** dans tous les cas -> il compte tous les objets fixes, pas les répliqués. Éjection FLOODED **toujours là**. + 131 à 2 000 erreurs `PATHFINDING(E): Invalid custom link settings`. **Tout annulé** (calques restaurés depuis `PakTools\BACKUP_layers_20261109`).
- Vrai message client : `RPL (E): Connection is flooded by data coming from the network and processing is unable to keep up` = **le CLIENT ne traite pas assez vite**, pas un débit serveur.
- Client à R4 avec 120 IA : 50-60 FPS, OK. L'éjection tombe à la **téléportation** R4->R6 : gel ~2,5 s du client (chargement du nouveau secteur) pendant que 120 nouvelles IA + équipement arrivent.
- Pistes réelles : (1) réduire le gel de chargement d'un secteur (textures/maillages : le travail déjà en cours sur ZimniOpti) ; (2) moins d'entités dynamiques par IA ; (3) en vrai jeu le pire cas = réapparition/déploiement près d'un combat, pas la téléportation.
- Prêt mais NON appliqué : `zimni_furn_static.py` (368 meubles sans Rpl/destruction), `zimni_layer_static.py`, `zimni_nodestr.py`. Test à faire en premier : banc client NON perturbé (pas de Minecraft) pour savoir si l'éjection vient du PC de test.
- **RÉSULTAT CLÉ (15:22)** : bataille FIXE à R4 (`#war 8 12 15 1`, option ajoutée à ZO_Benchmark : 5e valeur = point où la bataille tourne tout le banc), client arrivant depuis T1 puis R6/T2 -> **AUCUNE éjection**, 4 points OK (R4 53-64 FPS, pire frame 52 ms ; arrivée de secteur 130 ms). Les FLOODED venaient du banc : suppression + recréation de 132 entités (120 IA + 12 véhicules) d'un coup sur le client. Risque réel = gros spawn/despawn d'IA près d'un joueur (garnisons FF) -> surveiller `Connection is flooded` dans les logs des vrais serveurs. Distance de vue via `viewdist` non appliquée en client/serveur (CSV vue=2500) : à corriger si on veut tester ce levier.

### F4 — résultat 2026-11-09 : rails = ❌ aucun gain
- A/B A→B→B→A sur R6/X10Z4 (zone la plus riche en rails) avec RailDRoit19m01 + RailDroit09m1 (4 412 instances) **cachés** (MeshObject retiré) : FPS identiques à ±1 (R6 96-99, X10Z4 111-114), pires frames 15-36 ms dans les deux cas. Même supprimés, ils ne coûtent rien -> pas la peine d'y toucher. Override retiré, paquet refait.

## F10 — Masquage d'objets (« culling »/chunks) — ❌ sans gain sur PC (2026-11-09)
- Outil `ZO_PropCuller.c` (grille 100 m, retire le drapeau VISIBLE, client seul). **Désactivé partout** ; diag PC via fichiers
  `ZimniPerf/cull` (rayon m) + `ZimniPerf/cullmode` (`trees` | `buildings` | vide = petits objets + intérieurs).
- 4 villes (villes.txt : T1, X10Z5, X11Z9, X12Z9), moyenne FPS :
  réf 67-72 / 82-85 / 84-87 / 75-85. Petits objets rayon 250 : = réf. Rayon 5 m (803k petits + 240k intérieurs) : +2-6 %.
  Bâtiments masqués (84 656) : +0-3 %. Végétation masquée (2,42 M) : **+4-12 %** (seul poste qui bouge).
- **Conclusion** : sur ce PC le rendu des objets n'est pas le goulot (cf. F8 réglages graphiques sans effet) -> PC limité CPU.
  Le banc PC ne peut pas révéler les goulots GPU/VRAM des consoles. À faire : banc console réel, ou PC en 4K forcée.
- Terrain : 64 surfaces (Arland 19), textures sol 344 Mo (Arland 179) dont 320 Mo de détail (BCR/NHO). Retirer le mip haut
  = −240 Mo mais sol flou de près -> pas fait.

## F11 — Zimnitrita vs cartes Bohemia (2026-11-09, sous-agent, parseur EBIN maison)
| | Zimni | Kolguev | Arland | Everon(.ent) |
|---|---|---|---|---|
| instances | 3,13 M | 1,43 M | 173 k | 127 k (+.wst) |
| /km² | 13 300 | 13 600 | 18 400 | — |
| arbres | 2,42 M | 0,85 M | 118 k | (dans .wst) |
| prefabs distincts | **4 891** | 1 665 | 1 013 | 1 771 |
| Forest/WallGenerator vivants | **5 672 / 4 329** | 0 | 0 | 0 |
| LightEntity | **529** | 54 | 3 | 54 |
| WorldOccluderEntity | 0 | 0 | 0 | **128** |
| instances avec LODs (héritage) | 96 % | 98 % | 95 % | 72 % |
- Leviers à refaire, par ordre : (1) lumières 529 -> couper/limiter ; (2) diversité de prefabs (×3) ; (3) aplatir générateurs + découper Casseburne4 (569k inst.) ; (4) occluders villes de l'est.
- Données : `PakInspector\base_worlds\worlds\` (Eden, Arland, Cain .ent), scripts ebin3.py/zlayer.py/summ.py dans le scratchpad de session.

## F12 — Buissons −50 % (471k retirés, `zimni_thin_bushes.py`) — ❌ sans gain PC (2026-11-09)
- Sans Minecraft : réf REF1/REF2 = T1 75/79, X10Z5 89/93, X11Z9 93/97, X12Z9 89/93 ; buissons −50 % (BUSH2) = 80/88/92/89 -> égal ou pire.
- Calques remis d'origine. Conclusion renforcée : sur PC plus rien de la carte ne pèse de façon mesurable ; la suite = mesures CONSOLE.

## F13 — ⭐ La console PLANTE (mémoire), elle ne rame pas — mesure = `memoireMo` du banc (2026-11-09)
- Retours Discord : crash PS5/Series S après 15-30 min, ouverture arsenal = gel, spawn de bots = crash, son coupé ; Series X tient. Serveurs qui s'en sortent : vue 1500 + **vue réseau 1200-1600** (réglage serveur).
- Mémoire client (villes.txt) : réf 4502-4576 Mo ; buissons 1/2 (−471k) 4230-4299 (−270) ; buissons 1/3 (−628k) 4133-4204 (−370) ;
  + rochers Granite_Beach/GraniteStone 1/2 (−147k) **4038-4114 (−465 Mo, −10 %)**. ≈ 0,6 Ko par instance -> 3,13 M instances ≈ 1,8 Go.
- État ZimniOpti : calques = buissons 1/3 + rochers 1/2 (sauvegarde d'origine : `PakTools\BACKUP_layers_20261109`). À valider visuellement.
- Outil : `zimni_thin_bushes.py <World> <N> [regex classe+prefab]`.

## F13 suite — tour de carte (20 zones, ~30 min, buissons 1/3 + rochers 1/2)
- Constat : 4007 → 4123 Mo (+116 Mo), hausse régulière, jamais redescendue en quittant une zone.
- Lecture : le moteur garde en cache ce qu'il a chargé. Pas d'explosion, mais ça s'additionne pendant toute la session. Sur une console déjà au bord, ça peut suffire à faire planter après 15 à 30 min.
- Piste : moins de modèles différents → moins de choses gardées en cache.

## F14 — mémoire détaillée (MemoryStatsSnapshot) + streaming IA de FF
- memstats.csv (tour) : ~4,1 Go dont 2,54 Go en petits blocs (24 M allocations) = données des objets posés. Au fil du tour, les petits blocs bougent à peine (+3,6 Mo sur 7 zones), le total +28 Mo → la croissance = gros blocs (cache maillages/textures), la base = le nombre d'instances.
- FF fait déjà du « chunk » pour les IA : JWK_StreamableAIForce.Update → CheckStreamStateConditions : groupe supprimé (DoStreamOut) quand aucun joueur à < distance+marge, recréé (DoStreamIn) en dessous de distance−marge. Distances par défaut (JWK_WorldEntity, réglables dans les settings FF) : militaires 1200 m, civils ambiants 800 m, véhicules 1400 m.
- Conséquence : baisser ces 3 distances dans les réglages de partie FF sur serveurs console = moins d'IA/véhicules vivants chez le client. Même logique appliquée aux décors = ZO_PropCuller mode "chunk" (en test).
- Comment FF fait apparaître les unités (JWK_SpawnQueueSystem) : une file d'attente traitée toutes les 270 ms. Le groupe est créé (SpawnEntityPrefab), puis chaque soldat est une demande séparée (JWK_AiGroupMemberSpawnRequest) → apparition étalée, jamais 10 soldats dans la même image. Côté joueur, une IA n'existe sur la machine que si elle est dans la portée réseau (networkViewDistance) : au-delà, elle n'est ni chargée ni dessinée.
- À reprendre pour nos décors en mode chunk : recréer une case par petits paquets, pas tout d'un coup (sinon saccade).

## F15 — Mode chunk (suppression réelle des décors loin du joueur) — ❌ gain mémoire faible (2026-11-09)
- 1er essai : client figé (700k suppressions dans la même image) → étalé à 60 cases / 0,5 s.
- Rayon 400 m (villes) : 523 929 objets supprimés, 329 267 ignorés (RplComponent = destructibles), mémoire ≈ 3 944-4 076 Mo vs réf 4 038-4 114. Éjection FLOODED (gel pendant les suppressions).
- Rayon 1 m (= « carte sans petits objets ni intérieurs ») : 541 682 supprimés, 422 979 ignorés (Rpl). Mémoire 4 065 → 3 892 Mo = **≈ −170 Mo**, soit ~0,3 Ko par objet supprimé.
- FPS effondrés pendant le test (~45 au lieu de ~85) : les suppressions (≈7 min) tournaient pendant les mesures → pas de conclusion FPS.
- Conclusion : même en supprimant TOUS les petits objets sans réplication, on ne gagne que ~4 % de mémoire. Le gros des 4 Go est ailleurs (jeu de base, terrain, textures, objets répliqués/destructibles). Prochain test utile : même banc sur une carte officielle (Arland) pour connaître la part propre à Zimnitrita.

## F16 — ⭐ Everon sur le même banc : 1,6-1,9 Go contre 4,0-4,1 Go (2026-11-09)
- Tour 12 points Everon (GM_Eden, même client/serveur/mods) : 1 614 → 1 941 Mo, 144 FPS partout.
- MemoryStatsSnapshot : Everon petits blocs 0,85 Go (8,3 M allocations) ; Zimnitrita 2,52 Go (24-26 M). Gros blocs : 1,1 Go vs 1,6 Go.
- → Zimnitrita coûte ~+2,2 Go, dont ~1,7 Go de petits blocs = données des instances posées (~0,6 Ko × ~3 M). Les petits objets n'en font que 170 Mo (F15) : le gros = la végétation (2,42 M instances).
- Piste : supprimer côté joueur la végétation au-delà de la distance de vue console (invisible de toute façon) = mode « chunktrees » (en test, rayon 1600 m).
- Note : ZO_Benchmark autorise maintenant GM_Eden (test) ; ZO_Bench.conf restauré sur Zimnitrita (sauvegarde PakTools\ZO_Bench.conf.bak).

## F17 — ⭐ Chunk végétation (« chunktrees », rayon 1600 m) — ✅ −500 Mo (2026-11-09)
- 1 796 052 arbres/buissons/rochers recensés ; 1 895 154 suppressions cumulées, 228 293 recréations pendant le parcours.
- Même point (T1) : 4 067 Mo → 3 538 Mo une fois la purge finie (≈ −530 Mo). Parcours villes : 3 540-3 595 Mo contre 4 067-4 109 Mo (≈ −500 Mo, −12 %).
- FPS une fois la purge finie : identiques (T1 97,5 vs 96 ; X12Z9 76,6 vs 78,6 ; X10Z5 87,7 vs 85,1).
- Problème : la purge initiale prend ~3 min et fait chuter les FPS (17 → 56 → 59). Pas d'éjection cette fois.
- ~0,28 Ko par instance supprimée (moins que la moyenne 0,6 Ko : les arbres sont des objets légers).
- À faire avant de garder : purge pendant l'écran de chargement ou beaucoup plus étalée ; actif seulement sur console ; rayon = plafond de vue console + marge (1300 / 1700 m) ; vérifier en jeu qu'aucun arbre ne « pop » à l'horizon.

## F18 — Chunk végétation : purge au chargement + ordre de suppression — ✅ ≈ −800 Mo (2026-11-10, nuit)
- Purge lancée 1 s après le chargement du monde (avant le personnage), budget au temps (100 ms/image sans personnage, 5 ms en jeu), cases proches recréées en priorité ; le banc attend la fin (ZO_PropCuller.IsReady).
- ⚠️ Correction de F17 : la purge ne prenait pas « 3 min » mais **16-18 min** (1,8 M suppressions). Le budget n'y change rien : c'est le `delete` du moteur qui coûte (lecture des données ≈ 0, retirer modèle+physique avant ≈ 0, aucun parent, pas un problème d'objets script).
- Coût d'un `delete` = position de l'entité dans la liste interne (ordre de chargement = EntityID) : 0,12 ms pour les arbres chargés tard (« _aut », ID ~2,8 M), 0,55-0,7 ms pour ceux du milieu (ID ~1,6-2 M). Microtest croisé sur 3 prefabs, même image.
- Solution : balayage `FindEntityByID(EntityID.FromInt(0,i))` de 0 à ~3,56 M (1,1 s) → liste triée par ID → suppression de la fin vers le début. Purge **613 s au lieu de 955-1071 s**.
- Résultat (villes, après purge) : **3 215-3 302 Mo contre 4 067-4 109 Mo (≈ −800 Mo, −20 %)**, FPS identiques (83/105/98/99). Mieux que l'ordre par case (3 430-3 512) : les cases recréées sont aussi moins coûteuses.
- Variante testée « chunkvobj » (retirer modèle 3D + physique sans supprimer, comme le jeu pour un arbre abattu) : purge 12 s mais seulement ≈ −150 Mo → abandonnée.
- Mémoire ≈ 0,5 Ko par arbre supprimé, linéaire : −170 Mo après 1 min de purge, −570 après 5 min, −920 après 10 min.
- Reste à régler : en jeu (5 ms/image) la purge complète prendrait ~30 min sur PC (plus sur console) ; à décider : budget plus gros, ou ne supprimer que la part la moins chère, ou accepter une baisse progressive.
- Production (non publié) : actif sur consoles seulement, rayon 1300 m (Series S/One/PS4) / 1700 m (Series X/PS5) ; PC = désactivé sauf fichiers de diagnostic cull/cullmode.

## Préparation publication chunks (2026-11-10 matin)
- Bug trouvé au banc et corrigé : diagnostic par prefab à moitié remis à null → 16 667 « NULL pointer » dans `InitPurge` (une par image). Diagnostic désormais réservé aux tests (`s_bDiag` = fichier `ZimniPerf/cull` présent), remis à zéro dans `Reset()`.
- Fichiers de test vides (`chunkbudget`, `cull`) : `ToInt("")` levait « Wrong parameter value » → gardé par `IsEmpty()`.
- Journal CHUNK toutes les 5 s : seulement en test. Chez le joueur : 1 ligne en fin de purge.
- `Scripts/WorkbenchGame/EnfusionMCP` (21 handlers recopiés par le MCP) sorti du mod → `PakTools/staging/ZimniOpti_WorkbenchGame_EMCP`.
- Publication : `bash zimni_publish_prep.sh avant` (sort `Missions/ZO_Bench.conf`), puis `apres`.
- Vérifié : chunks ON → 0 erreur, purge 610 s, T1 3 204-3 225 Mo ; chunks OFF → 0 erreur, T1 4 048-4 068 Mo. Restent des erreurs Game Master vanilla (`SCR_MenuLayoutEditorComponent`, `missing RplComponent`), sans lien avec ZimniOpti.

## F19 — VRAM : l'allocateur garde son maximum (relevé de Girbev, overlay DX12 Workbench, 2026-11-10)
- **Constat** : aller-retour Ungru → Ajvan → Ungru → Ajvan. VRAM 1652 → 2019 → 2083 → 2147 Mo. « engine res only » (utilisé par la scène) 1295/1222/1312/1327 : le streaming rend bien. Tas Default 511 → 831 et Buffers 184 → 312, puis figés ; Managed 888/792/856/920. 253 Mo réservés vides au retour. Croissance par blocs de 64 Mo, plafond atteint au 2ᵉ passage.
- **Cause** : l'allocateur ne libère pas sans pression mémoire → la mémoire tend vers la vue la plus lourde vue dans la partie. Le pic compte, pas la moyenne. Explique F13c (montée sur le tour) et F13b (plateau).
- **Total console** : RAM du banc 4,1 Go + VRAM 1,6-2,1 Go ≈ 6 Go sur ~8 Go Series S.
- **Pistes** : alléger les pires vues (urbain île est, vues en hauteur villes/forêts) ; distance de vue plafonnée (F5) ; refaire en client de jeu (l'éditeur peut différer).
- **Fait** : `zimni_server_series.sh` lance `vram_sampler.ps1` à chaque variante → `ZimniPerf/<label>_vram.txt` + ligne « VRAM moyenne/pic ». Le sampler s'arrête seul 60 s après la fermeture du client.

## F19 suite — parcours de Girbev refait en jeu client (banc, 2026-11-10)
Points Ungru (1697, 15080) ↔ Ajvan (15761, 14512), 1 min par étape, VRAM = compteur Windows du client PC (réglages PC, donc plus haut que l'overlay Workbench).
| Étape | VRAM base | VRAM opti | RAM base | RAM opti |
|---|---|---|---|---|
| Ungru | 2 824 | 2 647 | 4 017 | 3 127 |
| Ajvan | 3 253 | 3 067 | 4 023 | 3 152 |
| Ungru (2) | 3 319 | 3 187 | 4 024 | 3 137 |
| Ajvan (2) | 3 319 | 3 187 | 4 025 | 3 154 |
- ✅ **Effet « cliquet » confirmé en jeu** : la VRAM monte de ~500 Mo au 1ᵉʳ passage à Ajvan puis reste figée (plateau dès le retour à Ungru), avec ou sans optimisations.
- Optimisations (chunks + textures réduites) : RAM −880 Mo, VRAM au plateau −132 Mo (Ungru/Ajvan sont loin de l'hôtel/prison, d'où le petit gain VRAM). **Total RAM+VRAM au pic : 7 344 → 6 341 Mo (−1,0 Go).**

## F20 — Chunk étendu « chunkall » : végétation + petits objets extérieurs — ✅ −270 Mo de plus (2026-11-10)
- **Test** : cullmode `chunkall` (rayon 1600) = Tree + objets non-bâtiment, hors intérieurs, diagonale < 6 m. Inventaire 2 523 274 objets ; 2 292 756 supprimés, 230 478 gardés car répliqués (RplComponent). Purge 522 s (plus rapide que chunktrees, 613 s : les petits objets coûtent peu à supprimer). 0 exception.
- **Résultat (villes)** : T1 2 944-2 959 Mo (chunktrees 3 215-3 235), X11Z9 2 990 (3 264), X12Z9 2 988 (3 264), X10Z5 3 008 (3 303) → **−270 Mo de plus**, FPS identiques (85/106/99/101).
- **Cumul villes T1** : carte d'origine 4 502 → 2 950 Mo (−1,55 Go RAM).
- **Appliqué** : production console passée de `chunktrees` à `chunkall` (Series S/One/PS4 rayon 1300, Series X/PS5 1700).
- **Fin500 (contours fins)** : 176 points tous les 500 m ; zone noire = bloc urbain île est (pire 64 FPS à 12500/10000), 2ᵉ = sud île est (74 FPS). Contours dans le dossier (`map/zones_fin500.json`, `zimni_zone_polys.py`).

## F21 — ⚠️ Les chunks libèrent la mémoire DANS le jeu, pas pour le système (2026-11-10 soir)
- **Constat** : memstats (MemoryStatsSnapshot) à T1 : sans chunks petits blocs 2 559 Mo utilisés + 135 Mo de vide (2 631 pages) ; avec chunkall 1 245 Mo utilisés + **1 197 Mo de vide** (2 384 pages). Le compteur du banc (`memoireMo`) ne compte que l'utilisé.
- **Mesure Windows** (mémoire privée du client, `vram_sampler.ps1` colonne 3) : sans chunks ~8 300-8 400 Mo en jeu, avec chunkall ~8 580 Mo. Pendant la purge : 7 175 → 7 023 Mo (−150 Mo seulement pour 2,3 M objets supprimés).
- **Cause** : l'allocateur du moteur garde ses pages ; les objets supprimés laissent des trous dispersés (ils sont entremêlés avec ceux qu'on garde), donc presque aucune page n'est rendue. Les trous resservent ensuite au jeu (≈1,2 Go de marge interne pour les petites allocations), mais le pic atteint au chargement ne baisse pas.
- **Conséquence** : les « −1,1 Go » des chunks (F17/F18/F20) sont surévalués en mémoire réelle (≈ −0,25 Go de pages rendues). Seul ce qui n'est **jamais créé** fait vraiment baisser la mémoire → piste : retirer des objets des layers (comme F12 buissons) ou les faire apparaître à la demande au lieu de les supprimer après.
- **Test layers d'origine vs allégés (F12 : buissons 1/3, rochers 1/2), chunks off, T1** : compteur jeu 4 536 → 4 065 Mo (−470) ; **mémoire privée Windows pic 9 460 → 8 952 Mo (−508)**, working set 5 619 → 5 057 (−560). → Un objet jamais créé = gain réel 1 pour 1. Un objet créé puis supprimé = gain presque nul.
- **Piste n°1** : apparition à la demande. Sortir des layers la végétation et les petits objets (les 2,3 M que chunkall supprime), les stocker par tuiles ; le serveur et les PC les font tous apparaître au chargement (IA, balles, collisions inchangées), les consoles seulement autour du joueur (1 300-1 700 m). Gain attendu sur console ≈ −1 Go réel.

## F22 — ⭐ Prototype « apparition à la demande » (ZO_Streamer) — ✅ ≈ −1 Go RÉEL (2026-11-11, nuit)
- **Chaîne** : (1) relevé en jeu (`$profile:ZimniPerf/streamdump` → `streamdump.bin` : 2 292 794 objets ciblés chunkall, prefab + matrice monde, 119 Mo) ; (2) `zimni_stream_build.py <World> <dump> <prefabs> <tuiles> [--apply]` : retrouve 1 997 801 objets (87 %) dans les calques (position monde à 0,15 m près, même prefab), les retire (calques 274 → 67 Mo), écrit 236 tuiles de 1 km (100 Mo) ; (3) `ZO_Streamer.c` : crée côté joueur les tuiles à moins du rayon, retire celles à plus de rayon + 500 m (réglage test `streamr` = rayon, `streambudget` = ms/image).
- **Banc T1 (même serveur, mémoire privée Windows du client)** :
  | Variante | Compteur jeu | Mémoire réelle (pic) |
  |---|---|---|
  | Calques allégés F12, sans chunks | 4 050 | 8 776 (en jeu ≈ 8 400) |
  | + chunkall (suppression après coup) | 2 950 | ≈ 8 650 |
  | **+ apparition à la demande, rayon 1 600 m** | **2 922-2 940** | **7 607** |
  | Sans aucune de ces 2 M d'objets (borne) | 2 739-2 758 | 7 041 |
- 234 617 objets créés autour de T1 en ≈ 2 s de calcul au total (0,009 ms/objet, 30× moins cher qu'une suppression) ; lecture des tuiles 29 ms. FPS = référence. Aucune erreur script nouvelle.
- **Reste à faire avant production** : (a) serveur : créer TOUT au démarrage (IA, balles, collisions) — aujourd'hui rien ; (b) PC : tout créer au chargement (ou grand rayon) ; (c) tuiles dans le mod (`Stream/`, vérifier qu'elles sont bien empaquetées) ; (d) contrôle visuel (orientation/échelle des arbres recréés) ; (e) procédure à chaque mise à jour de la carte : relevé + script.
- **État** : calques complets remis dans le mod ; version allégée gardée dans `PakTools/staging/World_stream`, calques d'avant dans `staging/World_avant_stream`. ZO_Streamer inactif sans données (aucun effet en l'état).

## Recherche web (2026-11-11, sous-agent) — ce qui existe
- Officiel (wiki FBX Import) : occluders de MODÈLE = boîtes/plans `OCC_` dans le FBX, 3-4 max par bâtiment, ≥ 4-5 m, alignés, enfoncés 1-2 m. Les bâtiments vanilla en ont ; les custom de Zimnitrita peut-être pas → piste FPS est.
- Communautaire (wiki Nabla, non vérifié) : occluders MANUELS (WorldOccluderEntity) cassés depuis ~fin 2024 → à vérifier (test oc_off/oc_on de la nuit). Ticket T175723 : objets spawnés dynamiquement sans occluders → le streaming ne doit jamais toucher aux bâtiments (c'est déjà le cas : végétation + petits objets).
- Aucun budget mémoire console publié par Bohemia ; Series S ≈ 8 Go utilisables sur 10 (devs BF6).
- Mods Workshop : Universal Conflict Performance (plafond IA actives `SetLimitOfActiveAIs`, GarbageSystem.Flush), Lightweight Vegetation (LOD végétation), Dad's Dynamic Performance (vue/herbe selon FPS), Cuzzins Stability Patch (sons d'ambiance bornés), Disable GM Dynamic Simulation (évite REPLICATION FLOODED côté GM).
- Serveur : `networkViewDistance` (rayon de réplication, défaut 1500), `serverMaxViewDistance`, `-maxFPS`, `-nds`, `-streamingBudget` (sens/défauts à vérifier).
- Piste : moins de modèles/textures UNIQUES = moins de mémoire résidente.

## Bancs « console » (2026-11-11 nuit)
- Bridage : client 4 threads (AFF=15) → éjecté **REPLICATION FLOODED à chaque téléportation en combat** (3/3) : symptôme console probable (CPU faible + 230k objets répliqués + IA). 6 threads (AFF=63) pour la suite.
- Outils : `cpu_limit.ps1`, `AFF=` dans zimni_server_series.sh, `zimni_server_series2.sh` (+ `ENGSET="Module Cle type val|..."`), `zimni_stream_ab.sh`, `zimni_nuit_longue.sh` (longue.txt : 3 tours de 8 points en combat), `zimni_nuit2.sh` (occluders + grille fine `fin250.txt`).
- Données streaming dans le mod : le Workbench n'empaquette QUE les ressources enregistrées → tuiles nommées `.json` + `.meta` JSONResourceClass (vérifié : présentes dans data.pak). Lecture via `$ZimniOpti:Stream/` à confirmer.

## F23 — ⚠️ Éjections « REPLICATION FLOODED » : gel du client après un saut de position (2026-11-11, 02:40)
- **Constat** : les 2 sessions longues (console bridée 6 threads, vue Series S, 8 escouades + 4 véhicules + explosions) éjectées au bout de 13 min (ref) et 4 min (streaming), toujours **juste après une téléportation** de point. Client : `RPL (E): Connection is flooded by data coming from the network and processing is unable to keep up`. Juste avant, **aucune ligne de journal pendant 4 s (ref) / 12 s (streaming)** = image gelée.
- **Cause probable** : à l'arrivée dans une nouvelle zone le serveur ouvre d'un coup les flux de toutes les entités répliquées à portée (~230 000 objets Rpl sur la carte : bâtiments destructibles, etc.) + IA/véhicules du combat ; le client lent ne suit pas. Nos scripts sont bornés à ~5 ms/image en jeu (peu probable qu'ils gèlent 12 s), mais à vérifier.
- **Risque console réel** : respawn / déplacement rapide vers une autre base = même saut de position.
- **Piste** : options serveur `-streamingBudget` (déf. 500) et `-streamsDelta` (déf. 100 flux/tick/joueur) pour étaler la rafale ; `networkViewDistance` (prod 1500) ; mod « GM replication off » si des joueurs sont Game Master (60B2AD8092C4C94A).
- **Test** : `zimni_flood_ab.sh` (villes_war, 2×défaut vs 2× `-streamingBudget 200 -streamsDelta 20`) → `analyses/flood_ab.txt`.
- **Résultat** : (à venir)
- Données streaming valides cette nuit grâce au repli profil (`streamprofile`) : client rayon 1300 → 248 070 objets créés près du joueur, lecture 26 ms, création 2,4 s cumulées ; pic RAM privée 8 613 Mo (ref 9 013) mais session trop courte (éjection) pour conclure. Lecture depuis le pak toujours KO (à corriger).

## F22 suite — Plan B des tuiles : ressources .conf au lieu de fichiers lus à la main (2026-11-16)
- **Constat** : `FileIO.OpenFile` ne trouve pas les tuiles `.json` empaquetées (`Stream/…` et `$ZimniOpti:Stream/…` KO), alors que les scripts du jeu lisent des fichiers du pak avec un chemin simple. Cause non établie (le Workbench transforme peut-être les `.json`).
- **Sonde ajoutée** (ZO_Streamer.Init) : si rien n'est trouvé, le journal liste `FileExists` de 5 formes de chemin + `FindFiles` de `Stream/`. Un seul lancement suffira.
- **Plan B écrit** (non compilé : Workbench bloqué par Smart App Control) :
  - `ZO_StreamTile` (config : `m_aP` index prefab, `m_aM` 12 float/objet) chargé par `BaseContainerTools.LoadContainer` + `CreateInstanceFromContainer` ;
  - `ZO_StreamIndex.c` : liste des 1 458 prefabs + table tuile → ResourceName. Version VIDE dans le mod par défaut (streaming inactif) ;
  - `zimni_stream_conf.py` : Stream_data → `staging/Stream_conf` (236 `.conf` + `.meta` CONFResourceClass, 1 997 801 objets, 184 Mo de texte ; axes 4 décimales, position 3) ;
  - `zimni_stream_deploy.sh on|off` : met / retire calques allégés + `Stream/*.conf` + index complet.
- **À tester dès que le Workbench marche** : compilation, puis `deploy on` → banc (message `[ZO] STREAM actif … dossier conf`), temps de chargement d'une tuile (`lectureMs`), contrôle visuel des arbres (orientation / échelle après arrondi).

## F24 — Éclaircissement (buissons 1/3, rochers 1/2) SEULEMENT sur console (préparé hors ligne, 2026-11-16)
- **But** : PC et serveur retrouvent la végétation d'origine (pas de critique « forêt clairsemée », IA/balles inchangées) ; la console garde la version allégée. Seul moyen : via le streaming (un objet supprimé après coup ne rend pas la mémoire, F21) → l'objet éclairci n'est **jamais créé** sur console.
- **Calcul hors ligne écarté** (`zimni_pconly.py check`) : 1,99 M des 2,3 M objets ont un parent, hauteurs relatives au sol, inclinaison posée par le jeu → meilleure convention d'angles avec erreur max 1,0 (inutilisable). Il faut la vraie matrice relevée en jeu.
- **Code prêt (non compilé)** :
  - `ZO_Streamer` : index prefab ≥ `PCONLY` (1 048 576) = « PC/serveur seulement » ; sauté sur consoles (client) ou en test PC avec le fichier `$profile:ZimniPerf/streamconsole`.
  - `zimni_stream_build.py … --thinned <calques_eclaircis>` : lit les calques d'ORIGINE ; tout objet retrouvé dans le relevé mais absent des calques éclaircis (comparaison des lignes `coords` par fichier) est marqué PCONLY.
- **Procédure (Workbench requis)** :
  1. mettre les calques d'origine (`BACKUP_layers_20261109/Zimnitrita`) dans le mod, streaming off, empaqueter ;
  2. relevé en jeu (fichier `streamdump`) → `streamdump.bin` + `streamdump_prefabs.txt` (~2,8 M objets attendus) ;
  3. copie de `BACKUP_layers…` vers un dossier de travail, puis `python -I zimni_stream_build.py <copie> <dump> <prefabs> staging/Stream_data --apply --thinned staging/World_avant_stream` → la copie devient les calques allégés (à mettre dans `staging/World_stream`) ;
  4. `zimni_stream_conf.py` → `.conf`, puis `zimni_stream_deploy.sh on` ;
  5. banc : PC (tout créé) vs `streamconsole` (éclaircis sautés) → la console doit retrouver ≈ −500 Mo de plus ; contrôle visuel PC (forêt d'origine).

## F23 résultat — A/B « flooded » (2026-11-16)
- Réglage serveur `-streamingBudget 200 -streamsDelta 20` (vs défaut 500/100), villes_war, plafond console simulé, 8 consoles virtuelles, 4 coeurs.
- Défaut : KICK FLOODED aux 2 essais (après 2 et 3 points). Bridé : 0 flooded aux 2 essais, les 5 points parcourus (21 mesures).
- **Conclusion** : à mettre sur le serveur de prod (ligne de commande). Coût : le monde arrive un peu plus lentement après téléportation.
- Streaming plan B (.conf) validé le même jour : tuiles lues depuis le pak, pic RAM 7 707 Mo (session courte) vs 9 013 Mo réf.

## F25 — Banc « vol de caméra » et saccades du streamer (2026-11-16)
- Nouveau mode banc `#vol <m/s> <hauteur>` (ZO_Benchmark.FlyFrame) : caméra continue le long des points, mesure toutes les SAMPLE_S s. Parcours `vol.txt` ≈ 12 km à 40 m/s, 30 m, plafond console simulé.
- Vol réf (sans streaming) : 1/97 mesures > 100 ms, RAM privée pic 10 857 Mo, mémoire jeu max 4 098 Mo.
- Streaming v1 : 38/97 > 100 ms. Cause : budget 100 ms/image en caméra libre réactivé à chaque nouvelle tuile (s_bReady repasse à faux) + retraits par paquets de 200 (jusqu'à 55 ms).
- Correctif : drapeau `s_bLoaded` (100 ms seulement au 1er chargement), retraits par 25. Vol 4 : **0/97 > 100 ms, pire image 57 ms**, 97 FPS moyens, RAM privée pic 9 555 Mo, mémoire jeu max 3 053 Mo (−1,0 Go vs réf).

## F24 résultat — éclaircissement console seulement (2026-11-16)
- Relevé carte d'origine : 2 996 503 objets ; 2 700 135 retrouvés dans les calques ; **702 330 marqués PC/serveur seulement**. (Erreur corrigée : `--thinned` doit pointer le dossier PARENT de Zimnitrita_Layers, sinon tout est marqué.)
- Tuiles en place : `staging/World_stream` + `Stream_conf` (anciennes : `*_v1`).
- Vol 12 km : PC (forêt complète) 91 FPS, pire 66 ms, mém. jeu max 3 143 Mo, RAM pic 9 634 Mo ; console simulée 99 FPS, pire 54 ms, 3 095 Mo, RAM 9 489 Mo ; ancienne version éclaircie partout (vol4) 97 FPS, 3 053 Mo.
- **Conclusion** : la console garde le même coût que la version éclaircie ; le PC retrouve la forêt d'origine pour ~+90 Mo et −6 FPS. Les −500 Mo estimés valaient pour une carte entièrement créée ; avec le streaming (rayon 1 300 m) l'écart n'est que local (~50-150 Mo).

## F26 — Overrides de bâtiments dans Zimnitrita : coût (2026-11-16)
- 65 fichiers Zimnitrita au même chemin que le jeu de base : 25 matériaux (.emat : portes, abris avion, garage, panneaux, marquages piste), 36 prefabs (.et, dont 9 générateurs de forêt), 1 texture.
- Ces matériaux pointent presque tous vers des textures partagées du jeu de base ; textures propres à Zimnitrita via eux : 85 références, **≈ 2 Mo de VRAM** (panneaux). Override = une seule version chargée → surcoût négligeable.
- Le vrai poids est ailleurs : **509 textures de bâtiments propres à Zimnitrita = 2 772 Mo si tout résident** (Hotel 981, Infrastructure 751, Military 485, Prison 198 Mio), déjà traité par F1 (4096 → 2048).
- Note : les .emat du jeu de base n'ont pas pu être lus (format compilé) → comparaison base/zimni des textures remplacées non chiffrée, mais le côté Zimnitrita est sûr.
- **Comparaison Bohemia (F26 suite)** : bâtiment BI le plus lourd ≈ 64 Mo (MobileRadar_01, 17 tex) ; MountainHotel_01 34 Mo (10 tex, max 2048) ; Barracks_E_03 40 Mo ; FactoryHallWIP_01 38 Mo. Sur 162 dossiers Structures du jeu de base, seulement 15 textures 4096 (13 dans Roads). Zimnitrita (version d'origine) : Hotel 916 Mo, Power Plant 554 Mo, Bunkers 366 Mo, Prison 198 Mo → 10 à 27× un bâtiment BI. BI : textures uniques ≤ 2048 (souvent 1024) + matériaux partagés `_SharedData` répétés.

## F27 — Lumières (piste F11) — ❌ sans gain (2026-11-16)
- Recensement en jeu (ZO_Lights, `$profile:ZimniPerf/lights`) : **277 LightEntity**, toutes allumées en permanence (même de jour) et toutes avec ombres, aucune dynamique.
- Vol 12 km console simulée : normal 85,9 FPS / sans ombres 87,1 / éteintes 86,8 ; mémoire jeu 3 115-3 121 Mo. Écart dans le bruit → **rien à appliquer**, ni console ni PC.
- Les « pire 200 ms » de ces 3 vols viennent du recensement lui-même (requête carte entière ≈ 550 ms toutes les 60 s), pas des lumières. Module inerte sans le fichier de test.
- Objets cassables : pas de version « console seulement » possible (état répliqué par le serveur, doit être identique partout) ; déjà testés sans gain (F9).

## F28 — Rayon de streaming console (vue plafonnée 1 200 m) (2026-11-16)
- Vol 12 km, console simulée : 1 300 m → 95,9 FPS, pire 62 ms, mém. jeu 3 106 Mo ; 1 000 m → 100,0 FPS, 1 pic 200 ms, 3 060 Mo ; 800 m → **109,3 FPS (+14 %)**, pire 54 ms, 3 026 Mo. RAM Windows identique (~9,7 Go).
- **Mémoire : quasi rien (−80 Mo max). FPS : +14 % à 800 m.** Coût : plus d'arbres au-delà de 800 m alors que la vue va à 1 200 m → collines dégarnies au loin, à juger à l'œil avant d'appliquer.
- Pistes restantes listées (console) : textures par plateforme (.meta XBOX_SERIES/PS5), herbe, MaxDrawDistance petits objets (F4), masques 2048→1024, satmap 8192, terrain 64 surfaces, IA FF.
- **Rayon adaptatif (ZO_Streamer.Adapt, consoles)** : toutes les 5 s, FPS < cible (40) → −100 m (min 600) ; FPS > 1,3 × cible → +100 m ; plafond 75 % de la distance de vue (900 m sur Series S plafonnée 1 200). Retrait des tuiles à rayon + 300 (au lieu de +500). Test PC cible forcée 105 : rayon 1300 → 900 → 600 dans la zone lourde, remonte à 800 puis redescend ; **106,4 FPS, pire 54 ms, 3 008 Mo** (vs 95,9 FPS à 1 300 fixe). Pas encore publié.

## F29 — Herbe et petits objets sur le vol (console simulée, rayon adaptatif) — ❌ (2026-11-16)
- Réf 95,2 FPS, pire 45 ms, 3 059 Mo. Herbe 50 m (défaut 116) : 90,9 FPS, 12 images > 50 ms (bruit) ; herbe 10 m : 100,1 FPS (+5 %, quasi plus d'herbe) ; petits objets masqués > 250 m (521 663 + 240 107 intérieurs) : 98,5 FPS (+3 %), 1 pic 200 ms, 3 125 Mo.
- Écarts dans le bruit du banc (±5 FPS) → rien à appliquer. L'herbe à 10 m enlaidit trop pour +5 %.

## F30 — Au sol vs en l'air (2026-11-16)
- Même trajet au sol (2 m, 15 m/s, regard horizontal), console simulée, rayon fixe : 1 300 m → 115,7 FPS, 3 226 Mo ; 800 m → 121,3 FPS (+5 %), 3 137 Mo. En l'air (30 m) : +14 % (F28).
- **Au sol le rayon compte 3× moins** (relief/bâtiments cachent le lointain) → mode double : grand rayon au sol, réduit en altitude.
- ⚠️ VRAM moyenne au sol 4,73 Go contre ~3,5 Go en vol : de près les textures haute résolution (sol, bâtiments) sont chargées → le risque mémoire console est AU SOL, à pied, en ville. Renforce la piste textures par plateforme.
- ✅ Mode double validé (2026-11-16) : en vol rayon 1300 → 800 m dès le décollage ; au sol il remonte par paliers 800 → 1200 m (= distance de vue). Compile OK (pack EXIT=0, aucune erreur ZO).

## F31 — À pied : distance de vue et intérieurs (2026-11-17) — ❌ faible
- Bruit du banc au sol mesuré (3 runs identiques) : 107,6 / 112,5 / 113,3 FPS → ±5 %.
- Réf (vue 1200, intérieurs 60 m) 119,6 FPS, 3 214 Mo, VRAM moy 4 739 Mo. Vue 900 : 122,3 FPS (+2 %), 3 155 Mo (−60), VRAM 4 696, pire image 47 ms. Intérieurs 30 m : 119,5 FPS, rien.
- Dans le bruit. La mémoire graphique au sol (~4,7 Go) ne bouge pas : elle vient des textures proches (bâtiments, sol) → seule piste réelle = textures par plateforme (F27 liste).

## F32 — Ombres à pied (2026-11-17) — ~ petit gain
- Réf 112,2 / 111,0 FPS (moy 111,6), VRAM moy 4 633/4 712 Mo. ShadowQuality 3→0 : 116,1 (+4 %), VRAM 4 528 (−140), mem −45 Mo. DistantShadowsQuality 2→0 : 121,2 (+8,6 %), VRAM inchangée.
- Réglages appliqués à chaud (log OK). Ombres lointaines = seul gain au-dessus du bruit (±5 %), pas de gain mémoire. Inconnu : valeur par défaut sur console (peut-être déjà basse) → effet console non garanti.
- ✅ Appliqué console (ZO_ConsoleViewCap.c). ⚠️ Le jeu SAUVEGARDE les réglages moteur en quittant → valeur d'origine mémorisée et remise (OnMissionSet / OnGameEnd / sortie de Zimnitrita). Vérifié : « 2 -> 0 » puis « remises a 2 », fichier de réglages = MEDIUM après fermeture.

## F33 — Textures bâtiments 2048 → 1024 (78 textures, ~306 Mo max) — ❌ annulé (2026-11-17)
- Sites (hôtel/prison/centrale) : VRAM moy 2 662 contre 2 694 Mo (−32), pic 4 355 contre 4 290. À pied : 4 635 contre 4 759 (−124), pic identique 5 606/5 609. FPS dans le bruit.
- Gain trop faible pour la perte visuelle → remis à l'état d'avant (staging/tex_backup_avant1024). Le pic mémoire à pied ne vient pas des textures de bâtiments restantes.

## F34 — Journal inondé par les objets cassables créés par le streamer (2026-11-17) — piste
- Chaque run : ~480 000 lignes « DestructibleEntity … was spawned at runtime. It won't take damage and wont replicate » → console.log ~200 Mo + error.log ~184 Mo par session de 15 min.
- Coût probable sur console (écriture disque + formatage de chaîne à chaque création). Piste : ne pas créer en local les prefabs Destructible (les garder dans le monde) ou leur donner un type non destructible. À mesurer (hitches au chargement initial).

## F35 — Tuiles terrain (couleur + relief) 512 → 256 — ❌ (2026-11-17)
- Sol : VRAM moy 4 460 contre 4 512 (−52), pic 5 545 contre 5 607 ; FPS 97,8 contre 112,9. Vol : 3 264 contre 3 424 (−160), pic identique ; FPS 95,1 contre 106,2.
- Gain mémoire faible, FPS même plus bas (bruit ou coût de l'override). Abandonné, tuiles retirées (gardées dans staging/terrain256).

## F36 — ⚠️ Les objets créés par le streamer sont INCASSABLES (version publiée) (2026-11-17)
- **Constat** : message moteur à chaque création locale : « DestructibleEntity … was spawned at runtime. It won't take damage and wont replicate. Change entity type and switch prefab to use SCR_DestructionMultiPhaseComponent ». Par partie : 430 818 arbres/buissons (classe Tree), 21 789 murs/clôtures/panneaux (SCR_DestructibleEntity), 4 poteaux électriques.
- **Conséquence** : arbres non renversables par les véhicules (butent comme un rocher), murs/clôtures/panneaux ne cassent plus. Cohérent sur toutes les machines (serveur et clients créent tout en local) → pas de désynchronisation, mais gameplay changé. À confirmer en jeu (véhicule contre arbre/clôture).
- **Pourquoi** : un objet cassable « posé dans la carte » est synchronisé par son identifiant de carte (identique partout). Créé en cours de partie en local, il n'a pas d'identifiant commun → le moteur coupe ses dégâts.
- **Faire apparaître un objet cassable ?** La voie moteur = prefab à composant `SCR_DestructionMultiPhaseComponent` (dégâts par composant) + `RplComponent`, créé PAR LE SERVEUR et répliqué. Mais c'est justement le coût réseau/CPU qu'on veut éviter (F23 : flood) et il faudrait refaire chaque prefab de mur. Pour 22 k objets éparpillés : pas rentable. Pour les arbres : pas de solution simple (même problème × 430 k).
- **Correctif retenu (A)** : murs, clôtures, panneaux, poteaux (431 prefabs, 63 676 objets = 2 % du streaming) laissés dans la carte → redeviennent cassables. Arbres/buissons restent streamés (incassables, gain perf gardé). Outil : `zimni_stream_build.py --keep analyses/keep_destructibles.txt` → staging/World_stream_v3, Stream_data_v3, Stream_conf_v3.
- ✅ Test banc v3 (2026-11-17) : 0 mur créé en cours de partie (contre ~22 000), arbres inchangés (~470 000), 0 erreur ZO. Sol 103,5 FPS, mem 3 277 Mo (réfs sol 107-120 FPS, ~3 210 Mo) ; vol 110,1 FPS, 3 097 Mo (réf 106,2, 3 042). Coût ≈ +60 Mo, FPS dans le bruit. Reste : contrôle en jeu (un mur casse, rien en double). Version publiable remise dans le mod après le test.
