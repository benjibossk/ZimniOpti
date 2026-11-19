# Règles chiffrées · Map budget rules (Phase 0, 2026-11-19)

Repères mesurés sur PC, avec une console simulée et la vue à 1 200 m. Chaque carte fait le même scénario : survol de 12 km, puis combat d'environ 90 IA dans sa ville la plus dense. Détail : fiche F37 de [OPTI_MEMOIRE.md](OPTI_MEMOIRE.md).
*Measured on PC with a simulated console and a 1,200 m view distance. Every map runs the same scenario: a 12 km flight, then about 90 AI fighting in its densest town. Details: card F37.*

## Niveaux · Levels

| Niveau | Mémoire du jeu (pic, combat) | RAM du jeu (pic, combat) | Saccades > 100 ms (combat) | Référence |
|---|---|---|---|---|
| 🟢 **Sûr** · Safe | ≤ 1,8 Go | ≤ 7 Go | 0 | Everon (1,77 Go), Arland (1,14 Go) |
| 🟠 **Limite** · Borderline | 1,8 – 3,5 Go | 7 – 10 Go | quelques-unes | Zimnitrita + ZimniOpti (3,44 Go) |
| 🔴 **Danger** | ≥ 4 Go | ≥ 11 Go | plus de 10 | Zimnitrita d'origine (4,36 Go) |

Les seuils console restent **provisoires** : ils seront recalés avec des photos de résultat Series S, au moins une sur Everon et une sur Zimnitrita.
*Console thresholds are **provisional** until we get Series S result photos, at least one on Everon and one on Zimnitrita.*

## Ce qui fait monter la mémoire · What drives memory

| Facteur | Arland | Everon | Zimnitrita | Effet mesuré |
|---|---|---|---|---|
| Objets posés | 173 k | ~1 M (est.) | 3,13 M (2,42 M arbres) | Principal facteur (F21, F24) |
| Modèles différents | — | — | 4 891 (Kolguev : 1 665) | Mémoire occupée en permanence |
| Textures des bâtiments propres à la carte | — | — | 2,8 Go à la source | VRAM de près (F1) |
| Combat de 90 IA | +0,13 Go | +0,11 Go | +0,23 à +0,34 Go | Faible sur la mémoire, −15 à −25 % de FPS |

## Règles pour les mappeurs · Rules for mappers

1. **Viser moins de 1,5 M d'objets posés**, dont moins de 1 M d'arbres. Remplacer une partie des forêts denses par des coupes, des plantations ou des pépinières.
   *Aim for fewer than 1.5 M placed objects, fewer than 1 M of them trees. Replace part of the dense forests with clearings, plantations or nurseries.*
2. **Moins de 2 000 modèles différents** : réutiliser les modèles du jeu de base.
   *Fewer than 2,000 distinct models: reuse base-game assets.*
3. **Textures en 2048 au maximum** pour les bâtiments propres à la carte, avec des matériaux partagés et un réglage de texture réduit sur console dans les `.meta`.
   *Textures 2048 or smaller on map-specific buildings, shared materials, and reduced console texture settings in the `.meta` files.*
4. **Tester avec le scénario Combat** : le survol seul ne montre pas les saccades.
   *Test with the Combat scenario: a flight alone hides the hitches.*
5. **Comparer à Everon** sur le même banc. Plus de 2 fois la mémoire d'Everon, c'est la zone de plantage sur Series S.
   *Compare against Everon on the same bench. More than 2× Everon's memory is the Series S crash zone.*

Le nombre d'objets d'Everon est une estimation, à mesurer avec le même relevé que Zimnitrita.
*Everon's object count is an estimate, still to be measured with the same dump as Zimnitrita.*
