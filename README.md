# ZimniOpti: making Zimnitrita playable on console

ZimniOpti is an Arma Reforger add-on that optimizes the **Zimnitrita** map so it stops crashing and runs smoother on **Xbox Series S/X and PS5**.
🇫🇷 [Version française](README.fr.md)

Workshop GUID: `6ACEAD5654BD416A`.

This repo contains:
- the full mod as it goes to the Workshop (`mod/`);
- the benchmark and data tools used to measure everything (`tools/`);
- the complete research log with every measurement (`research/OPTI_MEMOIRE.md`, in French, one card per finding F1–F36).


---

## TL;DR: the main finding

**Consoles don't run slowly on Zimnitrita. They run out of memory and crash.**

Zimnitrita places **3.13 million objects** (2.42 M trees). For comparison, Kolguev has 1.43 M and Arland 173 k. On the same benchmark, Zimnitrita uses about 4.0–4.1 GB of game memory against 1.6–1.9 GB for Everon. A Series S has about 8 GB shared between CPU and GPU.

The engine does **not give memory back** when an object is deleted after being created (F21). Hiding or deleting objects at runtime therefore barely helps. The only thing that truly saves memory is **never creating the object** on the console in the first place.

## What the mod does

| Feature | How | Measured gain |
|---|---|---|
| **On-demand streaming** (`ZO_Streamer.c`) | Vegetation and small outdoor props (~2.6 M objects) are removed from the map layers and stored in 1 km tiles (`Stream/*.conf`). Consoles only create them around the camera (1,300 m on Series S / PS4, 1,700 m on Series X / PS5). PC and server still create the whole map, so AI, bullets and collisions are unchanged. | **−1.0 GB** game memory, **−1.3 GB** Windows memory on a 12 km flight. 0 / 97 samples with a hitch above 100 ms. |
| **Air / ground modes** | Full radius on foot or in a vehicle. Limited to 800 m when the camera is more than 20 m above the ground (helicopters, planes). | Radius 1,300 → 800 m: +14 % FPS in the air, only +5 % on the ground, where terrain and buildings hide the distance (F30). |
| **Adaptive radius** | Every 5 s, the radius drops by 100 m when FPS falls below the target (40) and rises again when FPS recovers. | Keeps towns smooth. |
| **Console-only thinning** | 702 k objects (one bush in three, half of the rocks) are flagged "PC/server only" and are never created on console. | Lower memory and higher FPS on console, while PC keeps the original forest (F24). |
| **View distance cap** (`ZO_ConsoleViewCap.c`) | 1,200 m on Series S / One / PS4, 1,600 m on Series X / PS5. | +13 % FPS on average, up to +50 % on the worst spots (F5). |
| **Distant shadows at minimum** (console, Zimnitrita only) | The player's own setting is restored on exit, because the game saves engine settings when it quits. | +8.6 % FPS on foot (F32). |
| **4096 textures reduced to 2048** | The top mip level is dropped without recompression, with the same GUID so the mod's version replaces the original. | −716 MB of VRAM where those buildings are visible (hotel, prison, power plant) (F1). |
| **Built-in benchmark scenario** | "ZimniOpti - Benchmark": the camera flies a fixed route and the result shows on screen at the end, so testers can photograph it. | — |

**Server side (not in the mod):** start the dedicated server with `-streamingBudget 200 -streamsDelta 20`. With the defaults, clients got kicked with `REPLICATION FLOODED` after a teleport in 2 runs out of 2; with these values, 0 out of 2 (F23).

## ⚠️ Known limitation: streamed objects can't be destroyed

An object placed in the map has the same ID on every machine, which is how its destruction is synchronised. An object spawned at runtime with `SpawnEntityPrefabLocal` has no shared ID. The engine therefore disables its damage and logs this:

> `DestructibleEntity ... was spawned at runtime. It won't take damage and wont replicate.`

- **Walls, fences, signs and poles** (431 prefabs, 63,676 objects) are now **kept in the map** and are destructible again. This costs about +60 MB.
- **Trees and bushes** stay streamed. Vehicles can't knock them down; that is the price of the memory gain.
- The engine's other route is `SCR_DestructionMultiPhaseComponent` with an `RplComponent`, spawned by the server. It brings back exactly the network and CPU cost we are trying to remove, so it was not used.

## What we tested that did NOT help

All of these stayed within the benchmark noise (about ±5 %) or gave no useful memory gain:

| Test | Result |
|---|---|
| Terrain auto-occluders | Nothing |
| Engine graphics settings, all on Low | +1 to +3 % on PC |
| Rails without LOD | Nothing |
| Hiding objects (culling) far from the player | Hidden ≠ freed memory |
| Bushes −50 % on PC | No gain |
| Lights: 277 lights, shadows off or all lights off | No gain |
| Grass at 50 m or 10 m | +5 % at best, and it looks much worse |
| Small props hidden beyond 250 m | +3 % |
| On foot: view distance 900 m, interiors hidden beyond 30 m | +2 % / nothing |
| 78 building textures 2048 → 1024 | −32 to −124 MB on average, same peak |
| Terrain tiles (colour and normal) 512 → 256 | −52 to −160 MB on average, same peak |

**Where memory still goes:** on foot, VRAM sits around 4.7 GB (3.5 GB in the air) whatever these settings are. It comes from close-range textures and from the base game.

## Advice for mappers (Zimnitrita authors)

- Zimnitrita's own building textures total **2.8 GB** at source size: hotel 916 MB, power plant 554 MB, bunkers 366 MB, prison 198 MB. A big Bohemia building uses 34–64 MB. Prefer 2048 textures, and share materials between buildings.
- A texture override with the same path and the same GUID is loaded only once. Overriding a base-game texture does not double the memory.
- Map-specific prefabs have no LOD and no occluder. A prefab override can add a max draw distance, but we tested it: hiding the most numerous ones (4,412 rails) or all small props beyond 250 m gave 0 to +3 % (F4, F29), and hidden objects still use memory. Real LODs and occluders are made in the 3D model (Blender), so this is mapper-side work, mainly useful for big unique buildings.
- Fewer unique models means less resident memory: 4,891 distinct models, against 1,665 for Kolguev.
- Per-platform texture settings (`.meta` configurations for XBOX_SERIES and PS5) are the clean way to keep full quality on PC and reduced textures on console. They need the source images, so this has to be done on the map side.

## Tools (`tools/`)

The tools are bash, PowerShell and Python 3 scripts. Paths are hard-coded for the author's machine, so adapt them before use.

| Script | Role |
|---|---|
| `zimni_server_series.sh`, `zimni_server_bench.ps1`, `zimni_pack.ps1` | Automated benchmark: packs the mod, starts a dedicated server and a client, flies the camera route, writes CSV, VRAM and RAM logs. Profile switch files in `$profile:ZimniPerf/` (`streamconsole`, `forcecap`, `streamr`, `engset`…) simulate a console on PC. |
| `zimni_stream_build.py`, `zimni_stream_conf.py`, `zimni_stream_deploy.sh` | Build the streaming tiles from an in-game dump, strip the layers, and deploy into the mod. `--thinned` flags console-only objects; `--keep` leaves destructibles in the map. |
| `zimni_textures.py`, `edds_dropmip.py`, `zimni_write_metas.py` | Reduce `.edds` textures by dropping mip levels, and write `.meta` files with the original GUID. |
| `pakextract.py`, `paklist.py` | Read and extract `.pak` files. |
| `*_ab.sh` | A/B tests: radius, grass, lights, shadows, textures, terrain. |

## Benchmark method

- The benchmark runs a dedicated server and a client on one PC. A console is simulated with a capped view distance, the console streaming radius and console-only thinning.
- Routes:
  - air: 12 km at 40 m/s and 30 m above the ground;
  - ground: same route at 15 m/s and 2 m;
  - heavy sites: hotel, prison, power plant.
- Noise: about ±5 % FPS between identical runs. Anything smaller is ignored.
- Not measurable here: real console behaviour. Console testers run the in-game "ZimniOpti - Benchmark" scenario and post a photo of the result.

## Credits

- Mod and research: Benji (FF / Zimnitrita community), with AI assistance (Claude).
- Thanks to Giorbev for the in-game VRAM measurements (F19).
- Zimnitrita map data belongs to its authors. This mod depends on the map and is published with that in mind.
