// ZimniOpti — banc de test automatique (Workbench Play ou partie solo).
// Se lance tout seul si $profile:ZimniPerf/bench_points.txt existe (une ligne par point : "label x z").
// Pour chaque point : téléportation, 4 s de stabilisation, puis 4 directions x 3 s de mesure.
// Résultat : $profile:ZimniPerf/bench_<unix>.csv ; fin signalée dans le log par "[ZO] BENCH TERMINE".
// Sans personnage (monde Game Master lancé en ligne de commande) : pilote la caméra courante.
// Si $profile:ZimniPerf/autoquit existe : ferme le jeu à la fin (bench de nuit autonome).

class ZO_BenchPoint
{
	string m_sLabel;
	vector m_vPos;
}

//------------------------------------------------------------------------------------------------
// Scénario « ZimniOpti - Benchmark » (joueurs, consoles comprises) : en-tête de mission dédié -> banc vol intégré.
class ZO_BenchMissionHeader : SCR_MissionHeader
{
}

class ZO_Benchmark
{
	protected static const string POINTS_FILE = "$profile:ZimniPerf/bench_points.txt";
	// Réglables par une ligne "#cfg <settleMs> <sampleS> <pitchDeg>" dans bench_points.txt (mode grille = rapide)
	protected int SETTLE_MS = 4000;
	protected float SAMPLE_S = 3.0;
	protected float m_fPitch = 0;
	protected static const float COUNT_RADIUS = 150;
	protected static const string AUTOQUIT_FILE = "$profile:ZimniPerf/autoquit";
	protected static const float CAM_HEIGHT = 2.0;

	protected static bool s_bDone;

	protected ref array<ref ZO_BenchPoint> m_aPoints = {};
	protected string m_sOut;
	protected int m_iPoint = -1;
	protected int m_iDir;
	protected bool m_bSampling;
	protected float m_fAcc;
	protected int m_iFrames;
	protected float m_fWorst;
	protected int m_iCount;
	protected vector m_vAngles;
	// Mode vol "#vol <vitesse m/s> <hauteur m>" : la caméra parcourt les points en continu (pas de téléportation)
	protected float m_fFlySpeed;
	protected float m_fFlyH = 40;
	protected float m_fFlyDist;
	protected int m_iSeg;
	protected float m_fFlyWait;	// secondes d'attente au départ (chargement) avant de voler
	// Récapitulatif du vol (écran final)
	protected float m_fTotTime;
	protected int m_iTotFrames;
	protected int m_iHitch50;
	protected int m_iHitch100;
	protected float m_fTotWorst;
	protected float m_fMinFps = 1000;
	protected int m_iMemMax;

	//------------------------------------------------------------------------------------------------
	static bool ShouldRun()
	{
		return !s_bDone && (FileIO.FileExists(POINTS_FILE) || IsBenchMission());
	}

	//------------------------------------------------------------------------------------------------
	static bool IsBenchMission()
	{
		return ZO_BenchMissionHeader.Cast(GetGame().GetMissionHeader()) != null;
	}

	// Parcours intégré du scénario joueurs : ~12 km à 40 m/s, 30 m au-dessus du sol (même trajet que vol.txt)
	protected void LoadBuiltin()
	{
		SAMPLE_S = 3.0;
		m_fPitch = -10;
		m_fFlySpeed = 40;
		m_fFlyH = 30;
		m_fFlyWait = 25;
		AddPoint("Ville", 12329, 10275);
		AddPoint("Nord-Ouest", 11500, 9500);
		AddPoint("Nord-Est", 12500, 9500);
		AddPoint("Sud", 10500, 5500);
		AddPoint("Retour", 12329, 10275);
	}

	protected void AddPoint(string label, float x, float z)
	{
		ZO_BenchPoint p = new ZO_BenchPoint();
		p.m_sLabel = label;
		p.m_vPos = Vector(x, 0, z);
		m_aPoints.Insert(p);
	}

	bool Start()
	{
		s_bDone = true;
		FileHandle h = FileIO.OpenFile(POINTS_FILE, FileMode.READ);
		if (!h)
		{
			if (!IsBenchMission())
				return false;
			LoadBuiltin();
		}

		string line;
		while (h && h.ReadLine(line) >= 0)
		{
			line.Trim();
			if (line.StartsWith("#cfg"))
			{
				array<string> c = {};
				line.Split(" ", c, true);
				if (c.Count() >= 4)
				{
					SETTLE_MS = c[1].ToInt();
					SAMPLE_S = c[2].ToFloat();
					m_fPitch = c[3].ToFloat();
				}
				continue;
			}
			if (line.StartsWith("#vol"))
			{
				array<string> fv = {};
				line.Split(" ", fv, true);
				if (fv.Count() >= 2)
					m_fFlySpeed = fv[1].ToFloat();
				if (fv.Count() >= 3)
					m_fFlyH = fv[2].ToFloat();
				continue;
			}
			if (line.StartsWith("#war"))
			{
				// "#war <escouadesParCamp>" : simule un combat US contre URSS à chaque point
				array<string> w = {};
				line.Split(" ", w, true);
				// "#war <escouades> [vehicules] [boomSec]" : + véhicules et explosions/bâtiments détruits toutes les boomSec
				if (w.Count() >= 2)
					m_iWarSquads = w[1].ToInt();
				if (w.Count() >= 3)
					m_iWarVeh = w[2].ToInt();
				if (w.Count() >= 4)
					m_iWarBoom = w[3].ToInt();
				// 5e valeur = n° du point (0..) où UNE bataille fixe tourne tout le banc (le client y arrive depuis loin)
				if (w.Count() >= 5)
					m_iWarAt = w[4].ToInt();
				continue;
			}
			if (line.IsEmpty() || line.StartsWith("#"))
				continue;

			array<string> parts = {};
			line.Split(" ", parts, true);
			if (parts.Count() < 3)
				continue;

			ZO_BenchPoint p = new ZO_BenchPoint();
			p.m_sLabel = parts[0];
			p.m_vPos = Vector(parts[1].ToFloat(), 0, parts[2].ToFloat());
			m_aPoints.Insert(p);
		}
		if (h)
			h.Close();

		if (m_aPoints.IsEmpty())
			return false;

		// Test F5 : distance de vue forcée si $profile:ZimniPerf/viewdist contient un nombre (m)
		FileHandle vdf = FileIO.OpenFile("$profile:ZimniPerf/viewdist", FileMode.READ);
		if (vdf)
		{
			string vds;
			vdf.ReadLine(vds);
			vdf.Close();
			if (vds.ToFloat() > 0)
				GetGame().SetViewDistance(vds.ToFloat());
		}

		string engInfo = ApplyEngineSettings();
		// Test herbe : $profile:ZimniPerf/grass = distance d'herbe en m
		FileHandle gf = FileIO.OpenFile("$profile:ZimniPerf/grass", FileMode.READ);
		if (gf)
		{
			string gs;
			gf.ReadLine(gs);
			gf.Close();
			PrintFormat("[ZO] HERBE %1 -> %2 m", GetGame().GetGrassDistance(), gs.ToInt());
			GetGame().SetGrassDistance(gs.ToInt());
			engInfo += " herbe=" + GetGame().GetGrassDistance();
		}

		m_sOut = string.Format("$profile:ZimniPerf/bench_%1.csv", System.GetUnixTime());
		FileHandle o = FileIO.OpenFile(m_sOut, FileMode.WRITE);
		if (o)
		{
			o.WriteLine(string.Format("# plateforme=%1 ; points=%2 ; vue=%3 ; moteur=%4", typename.EnumToString(EPlatform, System.GetPlatform()), m_aPoints.Count(), GetGame().GetViewDistance(), engInfo));
			o.WriteLine("label;x;z;direction;fpsMoyen;pireFrameMs;memoireMo;entites150m;camX;camZ;iaVivantes");
			o.Close();
		}

		Print(string.Format("[ZO] BENCH DEMARRE : %1 points -> %2", m_aPoints.Count(), m_sOut));
		if (m_fFlySpeed > 0 && m_aPoints.Count() >= 2)
		{
			m_iPoint = 0;
			m_iSeg = 0;
			m_fFlyDist = 0;
			m_fAcc = 0;
			m_iFrames = 0;
			m_fWorst = 0;
			if (m_iWarAt >= 0 && m_iWarAt < m_aPoints.Count())
			{
				vector wp = m_aPoints[m_iWarAt].m_vPos;
				wp[1] = GetGame().GetWorld().GetSurfaceY(wp[0], wp[2]) + 0.5;
				SpawnWar(wp);
			}
			Print(string.Format("[ZO] BENCH VOL : %1 m/s, %2 m de haut", m_fFlySpeed, m_fFlyH));
		}
		else
		{
			m_fFlySpeed = 0;
			NextPoint();
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void NextPoint()
	{
		m_iPoint++;
		if (m_iPoint >= m_aPoints.Count())
		{
			Print("[ZO] BENCH TERMINE -> " + m_sOut);
			if (FileIO.FileExists(AUTOQUIT_FILE))
				GetGame().GetCallqueue().CallLater(Quit, 2000);
			return;
		}

		ZO_BenchPoint p = m_aPoints[m_iPoint];
		Print("[ZO] BENCH POINT " + p.m_sLabel);	// repère horaire (VRAM échantillonnée à part)
		BaseWorld world = GetGame().GetWorld();
		p.m_vPos[1] = world.GetSurfaceY(p.m_vPos[0], p.m_vPos[2]) + 0.5;
		if (m_iWarAt < 0)
		{
			ClearWar();
			SpawnWar(p.m_vPos);
		}
		else if (m_iPoint == 0 && m_iWarAt < m_aPoints.Count())
		{
			vector wp = m_aPoints[m_iWarAt].m_vPos;
			wp[1] = world.GetSurfaceY(wp[0], wp[2]) + 0.5;
			ClearWar();
			SpawnWar(wp);
		}
		if (SCR_PlayerController.GetLocalControlledEntity())
			SCR_Global.TeleportPlayer(SCR_PlayerController.GetLocalPlayerId(), p.m_vPos);
		m_vAngles = vector.Zero;
		ApplyCamera();

		m_iCount = 0;
		world.QueryEntitiesBySphere(p.m_vPos, COUNT_RADIUS, CountEntity, null, EQueryEntitiesFlags.ALL);

		m_iDir = 0;
		GetGame().GetCallqueue().CallLater(StartDirection, SETTLE_MS);
	}

	//------------------------------------------------------------------------------------------------
	protected bool CountEntity(IEntity e)
	{
		m_iCount++;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void StartDirection()
	{
		m_vAngles = Vector(m_iDir * 90, m_fPitch, 0);
		IEntity ent = SCR_PlayerController.GetLocalControlledEntity();
		if (ent)
			ent.SetAngles(Vector(0, m_iDir * 90, 0)); // entité = (pitch, yaw, roll) ; AnglesToMatrix = (yaw, pitch, roll)
		ApplyCamera();

		m_fAcc = 0;
		m_iFrames = 0;
		m_fWorst = 0;
		m_bSampling = true;
	}

	//------------------------------------------------------------------------------------------------
	// Sans personnage : place la caméra courante (caméra d'éditeur GM) au point, à hauteur d'homme.
	protected void ApplyCamera()
	{
		if (SCR_PlayerController.GetLocalControlledEntity() || m_iPoint < 0 || m_iPoint >= m_aPoints.Count())
			return;

		CameraManager cm = GetGame().GetCameraManager();
		if (!cm)
			return;

		// On déplace la caméra ACTIVE (caméra éditeur GM). Une caméra spawnée (méthode du plugin officiel)
		// est ignorée en mode GM : l'éditeur garde la sienne -> mesures figées (vu 2026-11-08).
		CameraBase cam = cm.CurrentCamera();
		if (!cam)
			return;

		vector pos = m_aPoints[m_iPoint].m_vPos;
		pos[1] = pos[1] + CAM_HEIGHT;
		vector mat[4];
		Math3D.AnglesToMatrix(m_vAngles, mat);
		mat[3] = pos;
		cam.SetWorldTransform(mat);
	}

	//------------------------------------------------------------------------------------------------
	// Test A/B des réglages moteur : $profile:ZimniPerf/engset, lignes "Module Cle f|i|b valeur"
	// (ex. "GraphicsQualitySettings ObjectDrawDistanceScale f 0.5"). Les valeurs d'origine sont remises à la fin.
	protected ref array<string> m_aEngRestore = {};

	protected string ApplyEngineSettings()
	{
		string info;
		FileHandle h = FileIO.OpenFile("$profile:ZimniPerf/engset", FileMode.READ);
		if (!h)
			return info;

		UserSettings us = GetGame().GetEngineUserSettings();
		string line;
		while (h.ReadLine(line) >= 0)
		{
			line.Trim();
			array<string> a = {};
			line.Split(" ", a, true);
			if (a.Count() < 4 || !us)
				continue;

			BaseContainer m = us.GetModule(a[0]);
			if (!m)
			{
				info += a[0] + "?absent ";
				continue;
			}

			if (a[2] == "f")
			{
				float of;
				m.Get(a[1], of);
				m_aEngRestore.Insert(string.Format("%1 %2 f %3", a[0], a[1], of));
				m.Set(a[1], a[3].ToFloat());
				m.Get(a[1], of);
				info += string.Format("%1=%2 ", a[1], of);
			}
			else if (a[2] == "b")
			{
				bool ob;
				m.Get(a[1], ob);
				m_aEngRestore.Insert(string.Format("%1 %2 b %3", a[0], a[1], ob.ToString()));
				m.Set(a[1], a[3] == "1" || a[3] == "true");
				m.Get(a[1], ob);
				info += string.Format("%1=%2 ", a[1], ob);
			}
			else
			{
				int oi;
				m.Get(a[1], oi);
				m_aEngRestore.Insert(string.Format("%1 %2 i %3", a[0], a[1], oi));
				int was = oi;
				m.Set(a[1], a[3].ToInt());
				m.Get(a[1], oi);
				info += string.Format("%1=%2->%3 ", a[1], was, oi);
			}
		}
		h.Close();
		GetGame().UserSettingsChanged();
		Print("[ZO] REGLAGES MOTEUR : " + info);
		return info;
	}

	protected void RestoreEngineSettings()
	{
		UserSettings us = GetGame().GetEngineUserSettings();
		if (!us || m_aEngRestore.IsEmpty())
			return;

		foreach (string line : m_aEngRestore)
		{
			array<string> a = {};
			line.Split(" ", a, true);
			BaseContainer m = us.GetModule(a[0]);
			if (!m)
				continue;
			if (a[2] == "f")
				m.Set(a[1], a[3].ToFloat());
			else if (a[2] == "b")
				m.Set(a[1], a[3] == "1" || a[3] == "true");
			else
				m.Set(a[1], a[3].ToInt());
		}
		GetGame().UserSettingsChanged();
	}

	//------------------------------------------------------------------------------------------------
	// Charge « guerre » : voir ZO_War (spawn côté serveur ; en client connecté, demande par RPC).
	protected int m_iWarSquads;
	protected int m_iWarVeh;
	protected int m_iWarBoom;
	protected int m_iWarAt = -1;

	protected void SpawnWar(vector center)
	{
		if (m_iWarSquads > 0 || m_iWarVeh > 0)
			ZO_War.Request(center, m_iWarSquads, m_iWarVeh, m_iWarBoom);
	}

	protected int CountWarAlive()
	{
		return ZO_War.CountAlive();
	}

	protected void ClearWar()
	{
		if (m_iWarSquads > 0 || m_iWarVeh > 0)
			ZO_War.Request(vector.Zero, 0, 0, 0);
	}


	//------------------------------------------------------------------------------------------------
	protected void Quit()
	{
		RestoreEngineSettings();
		Print("[ZO] AUTOQUIT");
		GetGame().RequestClose();
	}

	//------------------------------------------------------------------------------------------------
	// Appelé à chaque frame par le contrôleur du joueur local.
	// Vol continu : avance la caméra le long des segments, une mesure toutes les SAMPLE_S secondes.
	protected void FlyFrame(float timeSlice)
	{
		if (m_fFlyWait > 0)
		{
			m_fFlyWait -= timeSlice;
			vector sp = m_aPoints[0].m_vPos;
			sp[1] = GetGame().GetWorld().GetSurfaceY(sp[0], sp[2]) + m_fFlyH;
			CameraManager wcm = GetGame().GetCameraManager();
			if (wcm && wcm.CurrentCamera())
			{
				vector wm[4];
				Math3D.AnglesToMatrix(Vector(0, m_fPitch, 0), wm);
				wm[3] = sp;
				wcm.CurrentCamera().SetWorldTransform(wm);
			}
			return;
		}
		if (m_iSeg >= m_aPoints.Count() - 1)
		{
			m_fFlySpeed = -1;
			Print("[ZO] BENCH TERMINE -> " + m_sOut);
			ShowResult();
			if (FileIO.FileExists(AUTOQUIT_FILE))
				GetGame().GetCallqueue().CallLater(Quit, 2000);
			return;
		}

		vector a = m_aPoints[m_iSeg].m_vPos;
		vector b = m_aPoints[m_iSeg + 1].m_vPos;
		float dx = b[0] - a[0];
		float dz = b[2] - a[2];
		float len = Math.Sqrt(dx * dx + dz * dz);
		m_fFlyDist += m_fFlySpeed * timeSlice;
		if (len < 1 || m_fFlyDist >= len)
		{
			m_fFlyDist = 0;
			m_iSeg++;
			Print("[ZO] BENCH POINT " + m_aPoints[m_iSeg].m_sLabel);
			return;
		}

		float t = m_fFlyDist / len;
		vector pos = Vector(a[0] + dx * t, 0, a[2] + dz * t);
		BaseWorld world = GetGame().GetWorld();
		pos[1] = world.GetSurfaceY(pos[0], pos[2]) + m_fFlyH;
		float yaw = Math.Atan2(dx, dz) * Math.RAD2DEG;

		CameraManager cm = GetGame().GetCameraManager();
		if (cm && cm.CurrentCamera())
		{
			vector mat[4];
			Math3D.AnglesToMatrix(Vector(yaw, m_fPitch, 0), mat);
			mat[3] = pos;
			cm.CurrentCamera().SetWorldTransform(mat);
		}

		m_fAcc += timeSlice;
		m_iFrames++;
		if (timeSlice > m_fWorst)
			m_fWorst = timeSlice;
		m_fTotTime += timeSlice;
		m_iTotFrames++;
		if (timeSlice > 0.05)
			m_iHitch50++;
		if (timeSlice > 0.1)
			m_iHitch100++;
		if (timeSlice > m_fTotWorst)
			m_fTotWorst = timeSlice;
		if (m_fAcc < SAMPLE_S)
			return;

		float segFps = m_iFrames / m_fAcc;
		if (segFps < m_fMinFps)
			m_fMinFps = segFps;
		m_iMemMax = Math.Max(m_iMemMax, System.MemoryAllocationKB() / 1024);

		FileHandle o = FileIO.OpenFile(m_sOut, FileMode.APPEND);
		if (o)
		{
			o.WriteLine(string.Format("%1;%2;%3;%4;%5;%6;%7;%8;%9",
				m_aPoints[m_iSeg].m_sLabel + ">" + m_aPoints[m_iSeg + 1].m_sLabel, pos[0].ToString(-1, 0), pos[2].ToString(-1, 0), yaw.ToString(-1, 0),
				(m_iFrames / m_fAcc).ToString(-1, 1), (m_fWorst * 1000).ToString(-1, 1),
				System.MemoryAllocationKB() / 1024, 0, pos[0].ToString(-1, 0) + ";" + pos[2].ToString(-1, 0) + ";" + CountWarAlive()));
			o.Close();
		}
		m_fAcc = 0;
		m_iFrames = 0;
		m_fWorst = 0;
	}

	// Écran final à prendre en photo et poster dans le fil Discord
	protected void ShowResult()
	{
		float avg = 0;
		if (m_fTotTime > 0)
			avg = m_iTotFrames / m_fTotTime;
		string txt = string.Format("Plateforme : %1\nFPS moyens : %2 | FPS mini (3 s) : %3\nPire image : %4 ms\nSaccades > 50 ms : %5 | > 100 ms : %6\nMemoire jeu max : %7 Mo\nDistance de vue : %8 m\nPhoto -> fil Discord ZimniOpti",
			typename.EnumToString(EPlatform, System.GetPlatform()), avg.ToString(-1, 1), m_fMinFps.ToString(-1, 1), (m_fTotWorst * 1000).ToString(-1, 0),
			m_iHitch50, m_iHitch100, m_iMemMax, GetGame().GetViewDistance());
		Print("[ZO] BENCH RESULTAT : " + txt);
		SCR_HintManagerComponent.ShowCustomHint(txt, "ZimniOpti - Benchmark termine", 3600);
	}

	void OnFrame(float timeSlice)
	{
		if (m_fFlySpeed > 0)
		{
			FlyFrame(timeSlice);
			return;
		}
		if (m_fFlySpeed < 0)
			return;
		ApplyCamera();
		if (!m_bSampling)
			return;

		m_fAcc += timeSlice;
		m_iFrames++;
		if (timeSlice > m_fWorst)
			m_fWorst = timeSlice;

		if (m_fAcc < SAMPLE_S)
			return;

		m_bSampling = false;
		ZO_BenchPoint p = m_aPoints[m_iPoint];
		FileHandle o = FileIO.OpenFile(m_sOut, FileMode.APPEND);
		if (o)
		{
			// Contrôle : position réelle de la caméra rendue (doit = x;z du point)
			vector camMat[4];
			vector camPos;
			CameraManager cm = GetGame().GetCameraManager();
			if (cm && cm.CurrentCamera())
			{
				cm.CurrentCamera().GetWorldCameraTransform(camMat);
				camPos = camMat[3];
			}
			o.WriteLine(string.Format("%1;%2;%3;%4;%5;%6;%7;%8;%9",
				p.m_sLabel, p.m_vPos[0].ToString(-1, 0), p.m_vPos[2].ToString(-1, 0), m_iDir * 90,
				(m_iFrames / m_fAcc).ToString(-1, 1), (m_fWorst * 1000).ToString(-1, 1),
				System.MemoryAllocationKB() / 1024, m_iCount, camPos[0].ToString(-1, 0) + ";" + camPos[2].ToString(-1, 0) + ";" + CountWarAlive()));
			o.Close();
		}
		// Diagnostic cache : mémoire détaillée par catégorie, une ligne par point (direction 0)
		if (m_iDir == 0)
		{
			FileHandle ms = FileIO.OpenFile("$profile:ZimniPerf/memstats.csv", FileMode.APPEND);
			if (ms)
			{
				MemoryStatsSnapshot snap = new MemoryStatsSnapshot();
				string line = p.m_sLabel;
				for (int si = 0; si < MemoryStatsSnapshot.GetStatsCount(); si++)
					line += ";" + MemoryStatsSnapshot.GetStatName(si) + "=" + snap.GetStatValue(si);
				ms.WriteLine(line);
				ms.Close();
			}
		}

		m_iDir++;
		if (m_iDir < 4)
			GetGame().GetCallqueue().CallLater(StartDirection, 200);
		else
			NextPoint();
	}
}

//------------------------------------------------------------------------------------------------
// Pilote : la boucle du jeu (marche avec ou sans personnage, Workbench Play ou exe du jeu).
modded class ArmaReforgerScripted
{
	protected ref ZO_Benchmark m_ZO_Bench;
	protected float m_fZO_Wait;

	//------------------------------------------------------------------------------------------------
	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);

		// Solo/Workbench (serveur) ou client connecté ; jamais sur un serveur dédié
		if (!world || System.IsConsoleApp())
			return;

		ZO_Lights.Frame(world, timeslice);

		if (!m_ZO_Bench)
		{
			if (!ZO_Benchmark.ShouldRun() || (!GetWorldFile().Contains("Zimnitrita") && !GetWorldFile().Contains("GM_Eden")))
				return;

			// Attend un monde chargé + une caméra ou un personnage, puis 20 s de streaming
			CameraManager cm = GetCameraManager();
			if (!SCR_PlayerController.GetLocalControlledEntity() && (!cm || !cm.CurrentCamera()))
				return;

			// attend la fin de la purge initiale des chunks (sinon les mesures la comptent)
			if (GetWorldFile().Contains("Zimnitrita") && (!ZO_PropCuller.IsReady() || !ZO_Streamer.IsReady()))
				return;
			m_fZO_Wait += timeslice;
			if (m_fZO_Wait < 20)
				return;

			m_ZO_Bench = new ZO_Benchmark();
			if (!m_ZO_Bench.Start())
				m_ZO_Bench = null;
			return;
		}

		m_ZO_Bench.OnFrame(timeslice);
	}
}
