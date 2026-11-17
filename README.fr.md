# ZimniOpti : rendre Zimnitrita jouable sur console

🇬🇧 [English version](README.md)

ZimniOpti est un mod Arma Reforger qui optimise la carte **Zimnitrita**. Le but : qu'elle ne plante plus et tourne mieux sur **Xbox Series S/X et PS5**.
GUID Workshop : `6ACEAD5654BD416A`.

Ce dépôt contient :
- le mod complet, tel qu'il part sur le Workshop (`mod/`) ;
- les outils du banc de mesure et de préparation des données (`tools/`) ;
- le journal de recherche complet, avec toutes les mesures (`research/OPTI_MEMOIRE.md`, une fiche par trouvaille, de F1 à F36).

---

## En bref : la découverte principale

**Sur Zimnitrita, la console ne rame pas : elle manque de mémoire et plante.**

Zimnitrita pose **3,13 millions d'objets**, dont 2,42 millions d'arbres. À titre de comparaison, Kolguev en a 1,43 million et Arland 173 000. Sur le même banc, Zimnitrita occupe environ 4,0 à 4,1 Go de mémoire de jeu, contre 1,6 à 1,9 Go pour Everon. Une Series S a environ 8 Go, partagés entre le processeur et la carte graphique.

Le moteur **ne rend pas la mémoire** d'un objet supprimé après sa création (F21). Cacher ou supprimer des objets en cours de partie ne sert donc presque à rien. La seule chose qui libère vraiment de la mémoire, c'est de **ne jamais créer l'objet** sur la console.

## Ce que fait le mod

| Fonction | Comment | Gain mesuré |
|---|---|---|
| **Apparition à la demande** (`ZO_Streamer.c`) | La végétation et les petits objets extérieurs (environ 2,6 millions) sont retirés des calques de la carte et rangés en tuiles de 1 km (`Stream/*.conf`). Les consoles ne les créent qu'autour de la caméra : 1 300 m sur Series S et PS4, 1 700 m sur Series X et PS5. Le PC et le serveur créent toute la carte, donc l'IA, les balles et les collisions ne changent pas. | **−1,0 Go** de mémoire de jeu et **−1,3 Go** de mémoire Windows sur un vol de 12 km. 0 mesure sur 97 avec une saccade de plus de 100 ms. |
| **Deux modes : sol et hélico** | Rayon plein à pied ou en véhicule. Limité à 800 m quand la caméra est à plus de 20 m au-dessus du sol (hélico, avion). | Rayon 1 300 → 800 m : +14 % de FPS en vol, seulement +5 % au sol, où le relief et les bâtiments cachent le lointain (F30). |
| **Rayon adaptatif** | Toutes les 5 s, le rayon baisse de 100 m si les FPS passent sous la cible de 40, et remonte quand ils reviennent. | Garde les villes fluides. |
| **Éclaircissement sur console seulement** | 702 000 objets (un buisson sur trois, la moitié des rochers) sont marqués « PC et serveur seulement » et ne sont jamais créés sur console. | Moins de mémoire et plus de FPS sur console. Le PC garde la forêt d'origine (F24). |
| **Distance de vue plafonnée** (`ZO_ConsoleViewCap.c`) | 1 200 m sur Series S, One et PS4 ; 1 600 m sur Series X et PS5. | +13 % de FPS en moyenne, jusqu'à +50 % sur les pires points (F5). |
| **Ombres lointaines au minimum** (console, sur Zimnitrita seulement) | Le réglage du joueur est remis en quittant, car le jeu enregistre les réglages à la sortie. | +8,6 % de FPS à pied (F32). |
| **Textures 4096 réduites en 2048** | Le plus grand niveau de détail de la texture est retiré, sans recompression, avec le même identifiant que l'original, qui est donc remplacé. | −716 Mo de mémoire vidéo là où ces bâtiments sont visibles : hôtel, prison, centrale (F1). |
| **Scénario de benchmark intégré** | « ZimniOpti - Benchmark » : la caméra suit un parcours fixe et le résultat s'affiche à la fin, pour que les testeurs le prennent en photo. | — |

**Côté serveur (pas dans le mod) :** lancer le serveur dédié avec `-streamingBudget 200 -streamsDelta 20`. Avec les valeurs par défaut, les joueurs étaient expulsés (« REPLICATION FLOODED ») après une téléportation, 2 fois sur 2. Avec ces valeurs, 0 fois sur 2 (F23).

## ⚠️ Limite connue : les objets créés par le mod sont incassables

Un objet posé dans la carte a le même numéro sur toutes les machines, et c'est ce numéro qui permet de le casser chez tout le monde. Un objet créé en cours de partie avec `SpawnEntityPrefabLocal` n'a pas de numéro commun. Le moteur coupe donc ses dégâts et l'écrit dans le journal :

> `DestructibleEntity ... was spawned at runtime. It won't take damage and wont replicate.`

- Les **murs, clôtures, panneaux et poteaux** (431 modèles, 63 676 objets) **restent dans la carte** et sont de nouveau cassables. Ça coûte environ 60 Mo.
- Les **arbres et buissons** restent gérés par le mod. Les véhicules ne peuvent plus les coucher : c'est le prix du gain de mémoire.
- L'autre méthode du moteur (`SCR_DestructionMultiPhaseComponent` avec un `RplComponent`, objet créé par le serveur) ramène justement le coût réseau et processeur qu'on cherche à éviter. Elle n'a donc pas été retenue.

## Ce qu'on a testé et qui n'a PAS marché

Tous ces tests sont restés dans la variation normale du banc (environ ±5 %) ou n'ont pas fait gagner de mémoire utile :

| Test | Résultat |
|---|---|
| Occlusion automatique du terrain | Rien |
| Réglages graphiques du moteur, tout au minimum | +1 à +3 % sur PC |
| Rails sans niveaux de détail | Rien |
| Cacher les objets loin du joueur | Un objet caché n'est pas de la mémoire libérée |
| Buissons −50 % sur PC | Pas de gain |
| Lumières : 277 lumières, ombres coupées ou lumières éteintes | Pas de gain |
| Herbe à 50 m ou 10 m | +5 % au mieux, et c'est beaucoup plus laid |
| Petits objets cachés au-delà de 250 m | +3 % |
| À pied : distance de vue 900 m, intérieurs cachés au-delà de 30 m | +2 % / rien |
| 78 textures de bâtiments 2048 → 1024 | −32 à −124 Mo en moyenne, pic identique |
| Tuiles du terrain (couleur et relief) 512 → 256 | −52 à −160 Mo en moyenne, pic identique |

**Où part encore la mémoire :** à pied, la mémoire vidéo reste vers 4,7 Go (3,5 Go en vol), quels que soient ces réglages. Elle vient des textures vues de près et du jeu de base.

## Conseils pour les mappeurs (auteurs de Zimnitrita)

- Les textures des bâtiments propres à Zimnitrita pèsent **2,8 Go** en taille d'origine : hôtel 916 Mo, centrale 554 Mo, bunkers 366 Mo, prison 198 Mo. Un gros bâtiment Bohemia, c'est 34 à 64 Mo. Préférer des textures en 2048 et partager les matériaux entre bâtiments.
- Une texture remplacée avec le même chemin et le même identifiant n'est chargée qu'une fois. Remplacer une texture du jeu de base ne double pas la mémoire.
- Les modèles propres à la carte n'ont ni niveaux de détail, ni objet d'occlusion, ni distance d'affichage maximale. Les ajouter est le plus gros gain qui reste côté carte (F4).
- Moins de modèles différents, c'est moins de mémoire occupée en permanence : Zimnitrita en a 4 891, contre 1 665 pour Kolguev.
- Le réglage de texture par plateforme (configurations XBOX_SERIES et PS5 dans les `.meta`) est la façon propre de garder la pleine qualité sur PC et des textures réduites sur console. Il faut les images sources, donc ça se fait côté carte.

## Outils (`tools/`)

Ce sont des scripts bash, PowerShell et Python 3. Les chemins sont écrits pour la machine de l'auteur, il faut les adapter avant de s'en servir.

| Script | Rôle |
|---|---|
| `zimni_server_series.sh`, `zimni_server_bench.ps1`, `zimni_pack.ps1` | Banc automatique. Il empaquette le mod, lance un serveur dédié et un client, fait parcourir le trajet à la caméra, puis écrit les mesures (fichiers CSV, mémoire vidéo, RAM). Des fichiers interrupteurs dans `$profile:ZimniPerf/` (`streamconsole`, `forcecap`, `streamr`, `engset`…) simulent une console sur PC. |
| `zimni_stream_build.py`, `zimni_stream_conf.py`, `zimni_stream_deploy.sh` | Fabriquent les tuiles du streaming à partir d'un relevé fait en jeu, allègent les calques, puis mettent le tout dans le mod. `--thinned` marque les objets réservés au PC ; `--keep` laisse les objets cassables dans la carte. |
| `zimni_textures.py`, `edds_dropmip.py`, `zimni_write_metas.py` | Réduisent les textures `.edds` en retirant des niveaux de détail, et écrivent les `.meta` avec l'identifiant d'origine. |
| `pakextract.py`, `paklist.py` | Lisent et extraient les fichiers `.pak`. |
| `*_ab.sh` | Tests A/B : rayon, herbe, lumières, ombres, textures, terrain. |

## Méthode du banc

- Le banc fait tourner un serveur dédié et un client sur le même PC. Pour simuler une console, il plafonne la distance de vue, applique le rayon de streaming console et l'éclaircissement console.
- Trajets :
  - en vol : 12 km à 40 m/s, à 30 m du sol ;
  - au sol : le même trajet à 15 m/s, à 2 m du sol ;
  - sites lourds : hôtel, prison, centrale.
- Variation normale : environ ±5 % de FPS entre deux trajets identiques. Un écart plus petit est ignoré.
- Ce qu'on ne peut pas mesurer ici : le comportement réel d'une console. Les testeurs console lancent le scénario « ZimniOpti - Benchmark » en jeu et envoient la photo du résultat.

## Remerciements

- Mod et recherches : Benji (communauté FF / Zimnitrita), avec l'aide d'une IA (Claude).
- Merci à Giorbev pour les relevés de mémoire vidéo en jeu (F19).
- Les données de la carte Zimnitrita appartiennent à ses auteurs. Ce mod dépend de la carte et est publié dans cet esprit.
