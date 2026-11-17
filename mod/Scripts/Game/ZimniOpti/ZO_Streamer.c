// ZimniOpti — PROTOTYPE « apparition à la demande » (F21) : la végétation et les petits objets extérieurs
// sont retirés des calques de la carte et rangés par tuiles de 1 km (fichiers binaires).
// Ils sont créés côté joueur seulement autour de la caméra : un objet jamais créé ne coûte rien en mémoire,
// alors qu'un objet créé puis supprimé laisse un trou que le moteur ne rend pas.
//
// Fichiers (dossier $profile:ZimniPerf/stream/ en test, sinon Stream/ dans le mod) :
//   prefabs.json : une ligne par prefab (index = numéro de ligne)
//   t_<tx>_<tz>.json (binaire malgré le nom) : int n, puis n int (index prefab), puis n*12 float (matrice monde : 3 axes + position)
// Test PC : $profile:ZimniPerf/streamr = rayon en m ; streambudget = ms par image en jeu ;
//           $profile:ZimniPerf/streamdump = relève tous les objets ciblés (avant retrait des calques).
// Serveur et PC : TOUT est créé au chargement (IA, balles, collisions inchangées) ; consoles : rayon 1300/1700 m.
// Sécurité : si les objets sont déjà dans la carte (calques complets), le système reste inactif.
// Plan B (mod publié) : une tuile = un .conf (Stream/t_<tx>_<tz>.conf) chargé comme ressource, liste des prefabs et
// des tuiles dans ZO_StreamIndex.c (généré par zimni_stream_build.py --conf). Pas de lecture de fichier « à la main ».
[BaseContainerProps(configRoot: true)]
class ZO_StreamTile
{
	[Attribute()]
	ref array<int> m_aP;	// index prefab
	[Attribute()]
	ref array<float> m_aM;	// 12 float par objet (3 axes + position)
}

class ZO_Streamer
{
	protected static const float TILE = 1000;
	protected static const int NT = 17;
	protected static const int DIST_MUL = 1000;
	protected static const float ALL_RADIUS = 100000;
	protected static const int PCONLY = 1048576;	// index prefab + PCONLY = objet « PC et serveur seulement » (éclaircissement console, F24)
	protected static bool s_bSkipPcOnly;	// consoles (ou test PC : fichier streamconsole) : ces objets ne sont jamais créés
	protected static bool s_bAll;		// serveur et PC : toute la carte est créée au chargement

	protected static float s_fAge;
	protected static int s_iState;			// 0 = à initialiser, 1 = actif, 2 = inactif
	protected static float s_fRadius;
	// Rayon adaptatif (consoles) : baisse si les FPS tombent sous la cible (villes...), remonte quand ça va mieux.
	// Plafonné aussi à 75 % de la distance de vue du joueur. Test PC : $profile:ZimniPerf/streamfps = FPS cible.
	protected static const float RADIUS_MIN = 600;
	protected static float s_fRadiusMax;
	protected static float s_fFpsTarget = 40;
	protected static float s_fFpsTime;
	protected static int s_iFpsFrames;
	protected static int s_iPlayBudget = 5;
	protected static bool s_bLoaded;
	protected static bool s_bDiag;	// journal des pires durées : $profile:ZimniPerf/streamdiag	// premier chargement fini (s_bReady repasse à faux à chaque nouvelle tuile)
	protected static string s_sDir;
	protected static ref map<int, ResourceName> s_mConf = new map<int, ResourceName>();	// plan B : clé tuile -> .conf
	protected static ref array<ref Resource> s_aRes;
	protected static ref array<ResourceName> s_aNames;
	protected static ref map<int, ref array<IEntity>> s_mTiles;	// tuiles créées
	protected static ref array<int> s_aQueue;		// tuiles à créer (code distance*DIST_MUL + clé), triées
	protected static ref array<int> s_aUnload;		// tuiles à retirer
	protected static int s_iLastCam = -1;
	protected static bool s_bReady;
	protected static bool s_bDumped;
	protected static int s_iSpawned, s_iDeleted, s_iSpawnMs, s_iReadMs;
	protected static float s_fLog;

	// tuile en cours de création
	protected static int s_iCurKey = -1;
	protected static ref array<int> s_aCurPref;
	protected static ref array<float> s_aCurMat;
	protected static int s_iCurPos;
	protected static ref array<IEntity> s_aCurEnts;
	// tuile en cours de retrait
	protected static ref array<IEntity> s_aDelEnts;
	protected static int s_iDelPos;

	//------------------------------------------------------------------------------------------------
	static bool IsReady()
	{
		return s_iState == 2 || s_bReady;
	}

	// true dès qu'on sait que le système est actif (le culler se coupe alors : rien à purger)
	static bool IsActive()
	{
		return s_iState == 1;
	}

	static void Reset()
	{
		s_fAge = 0;
		s_iState = 0;
		s_aRes = null;
		s_aNames = null;
		s_mTiles = null;
		s_aQueue = null;
		s_aUnload = null;
		s_iLastCam = -1;
		s_bReady = false;
		s_bLoaded = false;
		s_bDumped = false;
		s_iCurKey = -1;
		s_aCurPref = null;
		s_aCurMat = null;
		s_aCurEnts = null;
		s_aDelEnts = null;
		s_iSpawned = 0;
		s_iDeleted = 0;
		s_iSpawnMs = 0;
		s_iReadMs = 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ReadFirstLine(string path)
	{
		FileHandle f = FileIO.OpenFile(path, FileMode.READ);
		if (!f)
			return "";
		string s;
		f.ReadLine(s);
		f.Close();
		s.Trim();
		return s;
	}

	protected static void Init()
	{
		s_iState = 2;
		// Rayon : fichier de test PC (streamr) > consoles 1300/1700 > tout le reste (serveur, PC) = TOUTE la carte
		s_fRadius = ALL_RADIUS;
		EPlatform p = System.GetPlatform();
		if (p == EPlatform.XBOX_SERIES_S || p == EPlatform.XBOX_ONE_S || p == EPlatform.XBOX_ONE_X || p == EPlatform.PS4)
			s_fRadius = 1300;
		else if (p == EPlatform.XBOX_SERIES_X || p == EPlatform.PS5 || p == EPlatform.PS5_PRO)
			s_fRadius = 1700;
		if (FileIO.FileExists("$profile:ZimniPerf/streamr"))
		{
			string rs = ReadFirstLine("$profile:ZimniPerf/streamr");
			if (!rs.IsEmpty() && rs.ToFloat() > 0)
				s_fRadius = rs.ToFloat();
		}
		string bs = ReadFirstLine("$profile:ZimniPerf/streambudget");
		if (!bs.IsEmpty() && bs.ToInt() > 0)
			s_iPlayBudget = bs.ToInt();
		s_bAll = s_fRadius >= ALL_RADIUS;
		s_fRadiusMax = s_fRadius;
		string fs = ReadFirstLine("$profile:ZimniPerf/streamfps");
		if (!fs.IsEmpty() && fs.ToFloat() > 0)
			s_fFpsTarget = fs.ToFloat();
		if (fs == "off")
			s_fFpsTarget = 0;
		s_bDiag = FileIO.FileExists("$profile:ZimniPerf/streamdiag");
		s_bSkipPcOnly = !System.IsConsoleApp() && (p == EPlatform.XBOX_SERIES_S || p == EPlatform.XBOX_ONE_S || p == EPlatform.XBOX_ONE_X || p == EPlatform.PS4 || p == EPlatform.XBOX_SERIES_X || p == EPlatform.PS5 || p == EPlatform.PS5_PRO || FileIO.FileExists("$profile:ZimniPerf/streamconsole"));

		// table des prefabs
		// données dans le mod (fichiers « .json » pour que le Workbench les empaquette ; contenu binaire)
		// ou, en test PC avec le fichier streamprofile, dans $profile:ZimniPerf/stream/
		array<string> dirs = {"$ZimniOpti:Stream/", "Stream/"};
		if (FileIO.FileExists("$profile:ZimniPerf/streamprofile"))
			dirs.InsertAt("$profile:ZimniPerf/stream/", 0);
		FileHandle f;
		foreach (string d : dirs)
		{
			s_sDir = d;
			f = FileIO.OpenFile(s_sDir + "prefabs.json", FileMode.READ);
			if (f)
				break;
		}
		// plan B : données en .conf + index généré
		if (!f)
		{
			array<string> idxNames = {};
			ZO_StreamIndex.Fill(idxNames, s_mConf);
			if (!idxNames.IsEmpty())
			{
				s_sDir = "conf";
				s_aNames = {};
				foreach (string nm : idxNames)
					s_aNames.Insert(nm);
			}
		}
		if (!f && !s_aNames)
		{
			// diagnostic (lecture depuis le pak) : quelles formes de chemin le jeu voit-il ?
			array<string> probes = {"Stream/prefabs.json", "$ZimniOpti:Stream/prefabs.json", "$ZimniOpti:/Stream/prefabs.json", "{AB2912B29B018146}Stream/prefabs.json", "Stream/prefabs.json.meta"};
			foreach (string pr : probes)
				PrintFormat("[ZO] STREAM sonde %1 : existe=%2", pr, FileIO.FileExists(pr));
			array<string> found = {};
			FileIO.FindFiles(found.Insert, "Stream/", "");
			FileIO.FindFiles(found.Insert, "$ZimniOpti:Stream/", "");
			string ex;
			if (!found.IsEmpty())
				ex = found[0];
			PrintFormat("[ZO] STREAM sonde FindFiles : %1 fichiers, ex. %2", found.Count(), ex);
			Print("[ZO] STREAM pas de donnees (Stream/prefabs.json introuvable) : inactif", LogLevel.WARNING);
			return;
		}
		if (f)
		{
			s_aNames = {};
			string line;
			// ordre exact (index = numéro de ligne), lignes vides comprises
			while (f.ReadLine(line) >= 0)
			{
				line.Trim();
				s_aNames.Insert(line);
			}
			f.Close();
		}
		s_aRes = {};
		s_aRes.Resize(s_aNames.Count());
		s_mTiles = new map<int, ref array<IEntity>>();
		s_aQueue = {};
		s_aUnload = {};
		if (!LayersStripped())
		{
			Print("[ZO] STREAM donnees presentes mais calques complets (objets deja dans la carte) : inactif", LogLevel.WARNING);
			return;
		}
		s_iState = 1;
		PrintFormat("[ZO] STREAM actif : rayon %1 m (tout=%4), %2 prefabs, dossier %3, serveur=%5", s_fRadius, s_aNames.Count(), s_sDir, s_bAll, System.IsConsoleApp());
	}

	//------------------------------------------------------------------------------------------------
	// Sécurité anti-doublon : si les 5 premiers objets d'une tuile existent déjà dans le monde, la carte n'a pas été
	// allégée (calques d'origine + données) -> on ne crée rien.
	protected static string s_sProbeName;
	protected static int s_iProbeHits;

	protected static bool ProbeOne(IEntity e)
	{
		EntityPrefabData pd = e.GetPrefabData();
		if (pd && pd.GetPrefabName() == s_sProbeName)
			s_iProbeHits++;
		return true;
	}

	protected static bool LayersStripped()
	{
		BaseWorld w = GetGame().GetWorld();
		for (int k = 0; k < NT * NT; k++)
		{
			OpenTile(k);
			int n = s_aCurPref.Count();
			if (n < 5)
				continue;
			int hits = 0;
			for (int i = 0; i < 5; i++)
			{
				int m = i * 12;
				s_sProbeName = s_aNames[s_aCurPref[i] % PCONLY];
				s_iProbeHits = 0;
				w.QueryEntitiesBySphere(Vector(s_aCurMat[m + 9], s_aCurMat[m + 10], s_aCurMat[m + 11]), 0.2, ProbeOne, null, EQueryEntitiesFlags.STATIC);
				if (s_iProbeHits > 0)
					hits++;
			}
			s_iCurKey = -1;
			s_aCurPref = null;
			s_aCurMat = null;
			s_aCurEnts = null;
			s_iReadMs = 0;
			return hits < 3;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// distance de la caméra au point le plus proche de la tuile
	protected static float TileDist(int tx, int tz, vector c)
	{
		float x0 = tx * TILE;
		float z0 = tz * TILE;
		float dx = 0;
		float dz = 0;
		if (c[0] < x0)
			dx = x0 - c[0];
		else if (c[0] > x0 + TILE)
			dx = c[0] - x0 - TILE;
		if (c[2] < z0)
			dz = z0 - c[2];
		else if (c[2] > z0 + TILE)
			dz = c[2] - z0 - TILE;
		return Math.Sqrt(dx * dx + dz * dz);
	}

	protected static void Plan(vector c)
	{
		s_aQueue.Clear();
		s_aUnload.Clear();
		for (int tx = 0; tx < NT; tx++)
		{
			for (int tz = 0; tz < NT; tz++)
			{
				int k = tx * NT + tz;
				float d = TileDist(tx, tz, c);
				bool has = s_mTiles.Contains(k) || k == s_iCurKey;
				if (d < s_fRadius && !has)
					s_aQueue.Insert((int)d * DIST_MUL + k);
				else if (d > s_fRadius + 300 && s_mTiles.Contains(k))
					s_aUnload.Insert(k);
			}
		}
		s_aQueue.Sort();
		if (!s_aQueue.IsEmpty() || !s_aUnload.IsEmpty())
			s_bReady = false;
	}

	protected static void Adapt(float timeslice, vector c, BaseWorld world)
	{
		if (s_fFpsTarget <= 0)
			return;	// test : rayon fixe ($profile:ZimniPerf/streamfps = "off")
		s_fFpsTime += timeslice;
		s_iFpsFrames++;
		if (s_fFpsTime < 5)
			return;
		float fps = s_iFpsFrames / s_fFpsTime;
		s_fFpsTime = 0;
		s_iFpsFrames = 0;
		// Au sol le relief cache le lointain (F30 : rayon 3× moins coûteux) -> rayon plein ; en l'air (> 20 m) : 800 m max
		float top = Math.Min(s_fRadiusMax, GetGame().GetViewDistance());
		if (c[1] - world.GetSurfaceY(c[0], c[2]) > 20)
			top = Math.Min(top, 800);
		top = Math.Max(RADIUS_MIN, top);
		float r = s_fRadius;
		if (r > top)
			r = top;
		else if (fps < s_fFpsTarget && r > RADIUS_MIN)
			r = Math.Max(RADIUS_MIN, r - 100);
		else if (fps > s_fFpsTarget * 1.3 && r < top)
			r = Math.Min(top, r + 100);
		if (r == s_fRadius)
			return;
		PrintFormat("[ZO] STREAM rayon %1 -> %2 m (%3 FPS, cible %4)", s_fRadius, r, fps.ToString(-1, 0), s_fFpsTarget);
		s_fRadius = r;
		s_iLastCam = -2;	// force un nouveau plan à l'image suivante
	}

	protected static bool OpenTile(int k)
	{
		int tx = k / NT;
		int tz = k % NT;
		s_iCurKey = k;
		s_iCurPos = 0;
		s_aCurEnts = {};
		s_aCurPref = {};
		s_aCurMat = {};
		int t0 = System.GetTickCount();
		if (s_sDir == "conf")
		{
			ResourceName rn = s_mConf.Get(k);
			if (!rn.IsEmpty())
			{
				Resource hold = BaseContainerTools.LoadContainer(rn);
				if (hold && hold.IsValid())
				{
					ZO_StreamTile t = ZO_StreamTile.Cast(BaseContainerTools.CreateInstanceFromContainer(hold.GetResource().ToBaseContainer()));
					if (t && t.m_aP && t.m_aM && t.m_aM.Count() == t.m_aP.Count() * 12)
					{
						s_aCurPref = t.m_aP;
						s_aCurMat = t.m_aM;
					}
				}
			}
			s_iReadMs += System.GetTickCount() - t0;
			return true;
		}
		FileHandle f = FileIO.OpenFile(string.Format("%1t_%2_%3.json", s_sDir, tx, tz), FileMode.READ);
		if (f)
		{
			int n;
			f.Read(n, 4);
			if (n > 0)
			{
				s_aCurPref.Resize(n);
				s_aCurMat.Resize(n * 12);
				f.ReadArray(s_aCurPref, 4, n);
				f.ReadArray(s_aCurMat, 4, n * 12);
			}
			f.Close();
		}
		s_iReadMs += System.GetTickCount() - t0;
		return true;
	}

	protected static void FinishTile()
	{
		s_mTiles.Set(s_iCurKey, s_aCurEnts);
		s_iCurKey = -1;
		s_aCurPref = null;
		s_aCurMat = null;
		s_aCurEnts = null;
	}

	// Diagnostic saccades : pire durée (ms) d'une opération unique, par type
	protected static int s_iMaxDel, s_iMaxSpawn, s_iMaxOpen, s_iMaxLoad, s_iMaxFrame;

	protected static void Process(int budgetMs, BaseWorld world)
	{
		int t0 = System.GetTickCount();
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		vector mat[4];
		while (System.GetTickCount() - t0 < budgetMs)
		{
			// 1) retrait d'une tuile éloignée
			if (s_aDelEnts)
			{
				int dn = 0;
				int td = System.GetTickCount();
				while (s_iDelPos < s_aDelEnts.Count() && dn < 25)
				{
					IEntity de = s_aDelEnts[s_iDelPos];
					s_iDelPos++;
					dn++;
					if (de)
					{
						delete de;
						s_iDeleted++;
					}
				}
				s_iMaxDel = Math.Max(s_iMaxDel, System.GetTickCount() - td);
				if (s_iDelPos >= s_aDelEnts.Count())
					s_aDelEnts = null;
				continue;
			}
			// 2) création de la tuile en cours, par paquets
			if (s_iCurKey >= 0)
			{
				int ts = System.GetTickCount();
				int n = s_aCurPref.Count();
				int stop = Math.Min(n, s_iCurPos + 100);
				for (int i = s_iCurPos; i < stop; i++)
				{
					int pi = s_aCurPref[i];
					if (pi >= PCONLY)
					{
						if (s_bSkipPcOnly)
							continue;
						pi -= PCONLY;
					}
					if (pi < 0 || pi >= s_aNames.Count())
						continue;
					Resource r = s_aRes[pi];
					if (!r)
					{
						int tl = System.GetTickCount();
						r = Resource.Load(s_aNames[pi]);
						s_aRes[pi] = r;
						s_iMaxLoad = Math.Max(s_iMaxLoad, System.GetTickCount() - tl);
					}
					if (!r || !r.IsValid())
						continue;
					int m = i * 12;
					mat[0] = Vector(s_aCurMat[m], s_aCurMat[m + 1], s_aCurMat[m + 2]);
					mat[1] = Vector(s_aCurMat[m + 3], s_aCurMat[m + 4], s_aCurMat[m + 5]);
					mat[2] = Vector(s_aCurMat[m + 6], s_aCurMat[m + 7], s_aCurMat[m + 8]);
					mat[3] = Vector(s_aCurMat[m + 9], s_aCurMat[m + 10], s_aCurMat[m + 11]);
					sp.Transform = mat;
					IEntity ne = GetGame().SpawnEntityPrefabLocal(r, world, sp);
					if (ne)
					{
						s_aCurEnts.Insert(ne);
						s_iSpawned++;
					}
				}
				s_iCurPos = stop;
				s_iSpawnMs += System.GetTickCount() - ts;
				s_iMaxSpawn = Math.Max(s_iMaxSpawn, System.GetTickCount() - ts);
				if (s_iCurPos >= n)
					FinishTile();
				continue;
			}
			if (!s_aUnload.IsEmpty())
			{
				int uk = s_aUnload[0];
				s_aUnload.RemoveOrdered(0);
				array<IEntity> ua = s_mTiles.Get(uk);
				s_mTiles.Remove(uk);
				if (ua && !ua.IsEmpty())
				{
					s_aDelEnts = ua;
					s_iDelPos = 0;
				}
				continue;
			}
			if (!s_aQueue.IsEmpty())
			{
				int code = s_aQueue[0];
				s_aQueue.RemoveOrdered(0);
				int to = System.GetTickCount();
				OpenTile(code % DIST_MUL);
				s_iMaxOpen = Math.Max(s_iMaxOpen, System.GetTickCount() - to);
				continue;
			}
			if (!s_bReady)
				PrintFormat("[ZO] STREAM pret : %1 tuiles, crees=%2 retires=%3 lectureMs=%4 creationMs=%5 memMo=%6", s_mTiles.Count(), s_iSpawned, s_iDeleted, s_iReadMs, s_iSpawnMs, System.MemoryAllocationKB() / 1024);
			s_bReady = true;
			s_bLoaded = true;
			break;
		}
	}

	//------------------------------------------------------------------------------------------------
	static void Frame(BaseWorld world, float timeslice)
	{
		s_fAge += timeslice;
		if (s_fAge < 1)
			return;
		if (!s_bDumped)
		{
			s_bDumped = true;
			if (FileIO.FileExists("$profile:ZimniPerf/streamdump"))
				Dump(world);
		}
		if (s_iState == 0)
			Init();
		if (s_iState != 1)
			return;
		IEntity pl = null;
		int budget = s_iPlayBudget;
		if (s_bAll)
		{
			// serveur / PC : toute la carte, sans attendre de caméra (IA, balles et collisions comme avant)
			if (s_iLastCam == -1)
			{
				s_iLastCam = 0;
				Plan(vector.Zero);
			}
			if (System.IsConsoleApp())
				budget = 200;
			else
			{
				pl = SCR_PlayerController.GetLocalControlledEntity();
				if (!pl)
					budget = 100;
				else
					budget = Math.Max(budget, 20);
			}
		}
		else
		{
			pl = SCR_PlayerController.GetLocalControlledEntity();
			CameraManager cm = GetGame().GetCameraManager();
			if (!pl && (!cm || !cm.CurrentCamera()))
				return;
			vector cam[4];
			world.GetCurrentCamera(cam);
			vector c = cam[3];
			int ck = (int)(c[0] / 100) * 1000 + (int)(c[2] / 100);
			if (s_bLoaded)
				Adapt(timeslice, c, world);
			if (ck != s_iLastCam)
			{
				s_iLastCam = ck;
				Plan(c);
			}
			// caméra libre (GM / banc) : 100 ms seulement au premier chargement, ensuite budget de jeu
			if (!pl && !s_bLoaded)
				budget = 100;
		}
		int tf = System.GetTickCount();
		Process(budget, world);
		s_iMaxFrame = Math.Max(s_iMaxFrame, System.GetTickCount() - tf);
		s_fLog += timeslice;
		if (s_fLog >= 5)
		{
			s_fLog = 0;
			if (s_bReady && s_bDiag)
			{
				PrintFormat("[ZO] STREAM pires ms : image=%1 retrait=%2 creation=%3 lecture=%4 chargePrefab=%5 (tuiles=%6 file=%7)", s_iMaxFrame, s_iMaxDel, s_iMaxSpawn, s_iMaxOpen, s_iMaxLoad, s_mTiles.Count(), s_aQueue.Count());
				s_iMaxFrame = 0; s_iMaxDel = 0; s_iMaxSpawn = 0; s_iMaxOpen = 0; s_iMaxLoad = 0;
			}
			if (!s_bReady)
				PrintFormat("[ZO] STREAM tuiles=%1 file=%2 crees=%3 retires=%4 lectureMs=%5 creationMs=%6 memMo=%7", s_mTiles.Count(), s_aQueue.Count(), s_iSpawned, s_iDeleted, s_iReadMs, s_iSpawnMs, System.MemoryAllocationKB() / 1024);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Relevé : tous les objets ciblés (mêmes règles que chunkall) -> $profile:ZimniPerf/streamdump.bin + _prefabs.txt
	// Format : par objet int prefab + 12 float (matrice monde). L'outil zimni_stream_build.py en fait les tuiles.
	protected static ref array<int> s_aDumpPref;
	protected static ref array<float> s_aDumpMat;
	protected static ref map<ResourceName, int> s_mDumpIdx;
	protected static ref array<ResourceName> s_aDumpNames;

	protected static bool DumpOne(IEntity e)
	{
		if (!ZO_PropCuller.IsStreamTarget(e))
			return true;
		ResourceName rn = e.GetPrefabData().GetPrefabName();
		int idx;
		if (!s_mDumpIdx.Find(rn, idx))
		{
			idx = s_aDumpNames.Count();
			s_aDumpNames.Insert(rn);
			s_mDumpIdx.Insert(rn, idx);
		}
		s_aDumpPref.Insert(idx);
		vector mat[4];
		e.GetWorldTransform(mat);
		for (int a = 0; a < 4; a++)
		{
			vector v = mat[a];
			s_aDumpMat.Insert(v[0]);
			s_aDumpMat.Insert(v[1]);
			s_aDumpMat.Insert(v[2]);
		}
		return true;
	}

	protected static void Dump(BaseWorld world)
	{
		int t0 = System.GetTickCount();
		s_aDumpPref = {};
		s_aDumpMat = {};
		s_mDumpIdx = new map<ResourceName, int>();
		s_aDumpNames = {};
		world.QueryEntitiesByAABB(Vector(-1000, -500, -1000), Vector(30000, 3000, 30000), DumpOne, null, EQueryEntitiesFlags.STATIC);
		FileHandle f = FileIO.OpenFile("$profile:ZimniPerf/streamdump.bin", FileMode.WRITE);
		if (f)
		{
			int n = s_aDumpPref.Count();
			f.Write(n, 4);
			f.WriteArray(s_aDumpPref, 4, n);
			f.WriteArray(s_aDumpMat, 4, n * 12);
			f.Close();
		}
		FileHandle g = FileIO.OpenFile("$profile:ZimniPerf/streamdump_prefabs.txt", FileMode.WRITE);
		if (g)
		{
			foreach (ResourceName rn : s_aDumpNames)
				g.WriteLine(rn);
			g.Close();
		}
		PrintFormat("[ZO] STREAM releve : %1 objets, %2 prefabs, %3 ms", s_aDumpPref.Count(), s_aDumpNames.Count(), System.GetTickCount() - t0);
		s_aDumpPref = null;
		s_aDumpMat = null;
		s_mDumpIdx = null;
		s_aDumpNames = null;
	}
}
