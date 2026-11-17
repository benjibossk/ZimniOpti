// ZimniOpti — déchargement des objets fixes loin de la caméra (côté client uniquement).
// Mode "chunkall" (PRODUCTION consoles, végétation + petits objets extérieurs) / "chunktrees" (végétation seule) : la végétation au-delà du rayon est SUPPRIMÉE (prefab + position gardés)
// et recréée quand on se rapproche -> ~ -500 Mo de RAM sur Zimnitrita (test F17).
//   - purge initiale dès le chargement du monde, avant l'arrivée du joueur (budget large tant qu'il n'a pas de personnage) ;
//   - ensuite, file d'attente au temps (quelques ms par image), cases les plus proches recréées en premier.
// Modes de diagnostic sur PC : fichiers $profile:ZimniPerf/cull (rayon m) et cullmode (""/trees/buildings/chunk/chunktrees).
// Serveur, physique serveur, réseau : inchangés (les entités répliquées ne sont jamais touchées).

class ZO_PropCuller
{
	protected static const float CELL = 100;
	protected static const float MAX_DIAG = 6;		// « petit objet » : diagonale de la boîte < 6 m
	protected static const int KEY_OFS = 20020;		// décale les clés de case pour qu'elles soient positives
	protected static const int DIST_MUL = 2000000;	// code de file = distance(cases) * DIST_MUL + (clé+KEY_OFS)*2 + grille
	protected static int s_iLoadBudgetMs = 100;	// purge pendant le chargement / menu (pas de personnage) ; fichier chunkbudget
	protected static int s_iFrames, s_iSpentMs;
	protected static bool s_bPrefDone;
	protected static bool s_bDiag;	// journal détaillé : seulement quand un fichier de test ZimniPerf/cull existe (PC)
	protected static const int PLAY_BUDGET_MS = 5;	// en jeu

	protected static ref map<int, ref array<IEntity>> s_mCells;
	protected static ref set<int> s_sHidden;
	protected static float s_fRadius = -1;
	protected static int s_iLastCell = -999999;
	protected static int s_iCount;
	// intérieurs des bâtiments : grille à part, rayon court
	protected static ref map<int, ref array<IEntity>> s_mInCells;
	protected static ref set<int> s_sInHidden;
	protected static float s_fInRadius;
	protected static int s_iInCount;
	protected static string s_sMode;
	protected static int s_iDelMs, s_iMetaMs;
	protected static ref array<IEntity> s_aOrder;	// ordre de la requête, pour la purge initiale
	protected static int s_iOrderPos;
	protected static ref array<int> s_aPrefMs, s_aPrefN;
	protected static int s_iPhysN, s_iPhysMs;

	//------------------------------------------------------------------------------------------------
	static bool IsChunk()
	{
		return s_sMode.StartsWith("chunk");
	}

	//------------------------------------------------------------------------------------------------
	// true quand la purge initiale est finie (ou que le système est inactif) — le banc attend ce signal
	static bool IsReady()
	{
		if (s_fRadius < 0)
			return false;
		if (s_fRadius == 0 || !IsChunk())
			return true;
		return s_bInitDone;
	}

	//------------------------------------------------------------------------------------------------
	static float GetRadius()
	{
		s_sMode = "";
		s_bDiag = FileIO.FileExists("$profile:ZimniPerf/cull");
		FileHandle mf = FileIO.OpenFile("$profile:ZimniPerf/cullmode", FileMode.READ);
		if (mf)
		{
			mf.ReadLine(s_sMode);
			mf.Close();
			s_sMode.Trim();
		}
		FileHandle bf = FileIO.OpenFile("$profile:ZimniPerf/chunkbudget", FileMode.READ);
		if (bf)
		{
			string bs;
			bf.ReadLine(bs);
			bf.Close();
			bs.Trim();
			if (!bs.IsEmpty() && bs.ToInt() > 0)
				s_iLoadBudgetMs = bs.ToInt();
		}
		// Fichier de diagnostic prioritaire (PC)
		FileHandle f = FileIO.OpenFile("$profile:ZimniPerf/cull", FileMode.READ);
		if (f)
		{
			string s;
			f.ReadLine(s);
			f.Close();
			s.Trim();
			if (s.IsEmpty())
				return 0;
			return s.ToFloat();
		}
		// Apparition à la demande active : la végétation lointaine n'est jamais créée, rien à purger
		if (ZO_Streamer.IsActive())
			return 0;
		// Production : consoles seulement, rayon = plafond de vue console + marge
		EPlatform p = System.GetPlatform();
		if (p == EPlatform.XBOX_SERIES_S || p == EPlatform.XBOX_ONE_S || p == EPlatform.XBOX_ONE_X || p == EPlatform.PS4)
		{
			s_sMode = "chunkall";
			return 1300;
		}
		if (p == EPlatform.XBOX_SERIES_X || p == EPlatform.PS5 || p == EPlatform.PS5_PRO)
		{
			s_sMode = "chunkall";
			return 1700;
		}
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static int Key(float x, float z)
	{
		return Math.Floor(x / CELL) * 1000 + Math.Floor(z / CELL);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Put(map<int, ref array<IEntity>> cells, IEntity e)
	{
		vector o = e.GetOrigin();
		int k = Key(o[0], o[2]);
		array<IEntity> a = cells.Get(k);
		if (!a)
		{
			a = {};
			cells.Insert(k, a);
		}
		a.Insert(e);
	}

	// Intérieur = enfant (direct ou non) d'un bâtiment
	protected static bool IsInterior(IEntity e)
	{
		IEntity p = e.GetParent();
		while (p)
		{
			if (Building.Cast(p) || SCR_DestructibleBuildingEntity.Cast(p))
				return true;
			p = p.GetParent();
		}
		return false;
	}

	// Mode "chunkall" : végétation + petits objets extérieurs (ni bâtiment, ni intérieur, diagonale < MAX_DIAG)
	protected static bool IsSmallOutdoorProp(IEntity e)
	{
		if (Tree.Cast(e) || Building.Cast(e) || SCR_DestructibleBuildingEntity.Cast(e))
			return false;
		if (IsInterior(e))
			return false;
		vector mins, maxs;
		e.GetWorldBounds(mins, maxs);
		return vector.Distance(mins, maxs) <= MAX_DIAG;
	}

	// Cible des chunks (même filtre pour l'inventaire et pour le tri par ID)
	// Filtre « chunkall » indépendant du mode (relevé du streaming ZO_Streamer)
	static bool IsStreamTarget(IEntity e)
	{
		if (!e.GetVObject() || !e.GetPrefabData() || e.FindComponent(RplComponent))
			return false;
		return Tree.Cast(e) || IsSmallOutdoorProp(e);
	}

	protected static bool IsChunkTarget(IEntity e)
	{
		if (Tree.Cast(e))
			return true;
		return s_sMode == "chunkall" && IsSmallOutdoorProp(e);
	}

	protected static bool AddOne(IEntity e)
	{
		if (!e.GetVObject())
			return true;
		if (s_sMode == "chunkall")
		{
			if (IsChunkTarget(e))
			{
				Put(s_mCells, e);
				s_iCount++;
				if (s_aOrder)
					s_aOrder.Insert(e);
			}
			return true;
		}
		bool isTree = Tree.Cast(e) != null;
		bool isBld = Building.Cast(e) || SCR_DestructibleBuildingEntity.Cast(e);
		// cullmode "trees"/"chunktrees" = seulement la végétation, "buildings" = seulement les bâtiments
		if (s_sMode == "trees" || s_sMode == "buildings" || s_sMode == "chunktrees")
		{
			if (((s_sMode == "trees" || s_sMode == "chunktrees") && !isTree) || (s_sMode == "buildings" && !isBld))
				return true;
			Put(s_mCells, e);
			s_iCount++;
			if (s_aOrder)
				s_aOrder.Insert(e);
			return true;
		}
		if (isTree || isBld)
			return true;
		bool interior = IsInterior(e);
		if (!interior)
		{
			vector mins, maxs;
			e.GetWorldBounds(mins, maxs);
			if (vector.Distance(mins, maxs) > MAX_DIAG)
				return true;
		}
		if (interior)
		{
			Put(s_mInCells, e);
			s_iInCount++;
		}
		else
		{
			Put(s_mCells, e);
			s_iCount++;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Build(BaseWorld world)
	{
		s_mCells = new map<int, ref array<IEntity>>();
		s_sHidden = new set<int>();
		s_mInCells = new map<int, ref array<IEntity>>();
		s_sInHidden = new set<int>();
		s_iCount = 0;
		s_iInCount = 0;
		s_fInRadius = 60;
		EPlatform p = System.GetPlatform();
		if (p == EPlatform.XBOX_SERIES_X || p == EPlatform.PS5 || p == EPlatform.PS5_PRO)
			s_fInRadius = 90;
		FileHandle irf = FileIO.OpenFile("$profile:ZimniPerf/inradius", FileMode.READ);
		if (irf)
		{
			string irl;
			irf.ReadLine(irl);
			irf.Close();
			if (irl.ToFloat() > 0)
				s_fInRadius = irl.ToFloat();
		}
		// en mode chunk : on garde l'ordre de la requête (= ordre de rangement du moteur) pour la purge initiale
		s_aOrder = null;
		if (IsChunk())
			s_aOrder = {};
		s_iOrderPos = 0;
		int t0 = System.GetTickCount();
		world.QueryEntitiesByAABB(Vector(-1000, -500, -1000), Vector(30000, 3000, 30000), AddOne, null, EQueryEntitiesFlags.STATIC);
		PrintFormat("[ZO] CULL mode=%6 : %1 objets (%2 cases, rayon %3 m) + %4 objets interieurs (rayon %5 m) — inventaire %7 ms", s_iCount, s_mCells.Count(), s_fRadius, s_iInCount, s_fInRadius, s_sMode, System.GetTickCount() - t0);
		if (s_aOrder)
		{
			SortOrderById(world);
			// au départ, toutes les cases sont « à purger » ; la file remettra en « visibles » celles proches de la caméra
			foreach (int hk, array<IEntity> ha : s_mCells)
				s_sHidden.Insert(hk);
		}
		FileHandle tf = FileIO.OpenFile("$profile:ZimniPerf/delorder", FileMode.READ);
		if (tf && s_aOrder)
		{
			string tl;
			tf.ReadLine(tl);
			tf.Close();
			if (tl.Length() > 0)
				DelMicroTest();
		}
	}

	//------------------------------------------------------------------------------------------------
	// Le moteur range les entités dans l'ordre de chargement (EntityID croissant) et une suppression coûte
	// d'autant plus cher que l'entité est loin de la FIN de cette liste (0,12 ms près de la fin, 0,7 ms au milieu).
	// On reconstruit donc la liste dans l'ordre des ID (balayage FindEntityByID) ; InitPurge la vide par la fin.
	protected static void SortOrderById(BaseWorld world)
	{
		int t0 = System.GetTickCount();
		int want = s_aOrder.Count();
		array<IEntity> sorted = {};
		int miss = 0;
		int i = 0;
		while (miss < 300000 && i < 20000000)
		{
			IEntity e = world.FindEntityByID(EntityID.FromInt(0, i));
			i++;
			if (!e)
			{
				miss++;
				continue;
			}
			miss = 0;
			if (!e.GetVObject() || !IsChunkTarget(e))
				continue;
			if (!(e.GetFlags() & EntityFlags.STATIC))
				continue;
			sorted.Insert(e);
		}
		PrintFormat("[ZO] CULL tri par ID : %1 vegetaux trouves (inventaire %2), ID max balaye %3, %4 ms", sorted.Count(), want, i, System.GetTickCount() - t0);
		// sécurité : si le balayage n'a pas tout retrouvé (ID non séquentiels), on garde l'ordre de la requête
		if (sorted.Count() >= want * 0.95)
			s_aOrder = sorted;
	}

	//------------------------------------------------------------------------------------------------
	// microtest : 2000 « betula_pendula_3f » (DefaultLibrary, lent ?) et 2000 « _aut » (CainLibrary, rapide ?) en alternance
	protected static void DelMicroTest()
	{
		array<IEntity> A = {};
		array<IEntity> B = {};
		array<IEntity> C = {};
		foreach (IEntity e : s_aOrder)
		{
			if (!e || !e.GetPrefabData())
				continue;
			string pn = e.GetPrefabData().GetPrefabName();
			if (A.Count() < 2000 && pn.Contains("DefaultLibrary/Vegetation/Trees/t_carpinus_betulus_0"))
				A.Insert(e);
			else if (B.Count() < 2000 && pn.Contains("_aut.et"))
				B.Insert(e);
			else if (C.Count() < 2000 && pn.Contains("DefaultLibrary/Vegetation/Trees/t_picea_abies_3s"))
				C.Insert(e);
		}
		for (int q = 0; q < 3; q++)
			PrintFormat("[ZO] ID A=%1 B=%2 C=%3 debut=%4 fin=%5", A[q].GetID().ToString(), B[q].GetID().ToString(), C[q].GetID().ToString(), s_aOrder[q].GetID().ToString(), s_aOrder[s_aOrder.Count() - 1 - q].GetID().ToString());
		for (int q2 = 0; q2 < 10; q2++)
		{
			IEntity qe = s_aOrder[q2 * (s_aOrder.Count() / 10)];
			PrintFormat("[ZO] ID echantillon %1 = %2 %3", q2, qe.GetID().ToString(), qe.GetPrefabData().GetPrefabName());
		}
		int ta, tb, tc;
		for (int i = 0; i < 2000; i++)
		{
			int t = System.GetTickCount();
			if (i < A.Count())
				delete A[i];
			int t2 = System.GetTickCount();
			if (i < B.Count())
				delete B[i];
			int t3 = System.GetTickCount();
			if (i < C.Count())
				delete C[i];
			ta += t2 - t;
			tb += t3 - t2;
			tc += System.GetTickCount() - t3;
		}
		PrintFormat("[ZO] MICRO carpinus_0 n=%1 ms=%2 | _aut n=%3 ms=%4 | picea_3s n=%5 ms=%6", A.Count(), ta, B.Count(), tb, C.Count(), tc);
	}

	//------------------------------------------------------------------------------------------------
	// Purge initiale : TOUTE la végétation est supprimée dans l'ordre de rangement du moteur
	// (12x plus rapide que case par case : 0,05 ms au lieu de 0,6-1,3 ms par objet, mesuré 2026-11-10).
	// Les cases proches de la caméra sont ensuite recréées par la file normale (les plus proches d'abord).
	// true = terminé.
	protected static bool InitPurge(int budgetMs)
	{
		if (!s_aOrder)
			return true;
		if (!s_mStored)
			s_mStored = new map<int, ref ZO_CellStore>();
		int t0 = System.GetTickCount();
		int n = s_aOrder.Count();
		while (s_iOrderPos < n)
		{
			// contrôle du temps toutes les 16 suppressions (une suppression peut coûter jusqu à 0,7 ms)
			if ((s_iOrderPos & 15) == 0 && System.GetTickCount() - t0 >= budgetMs)
				return false;
			// on part de la fin de la liste
			IEntity e = s_aOrder[n - 1 - s_iOrderPos];
			s_iOrderPos++;
			if (!e)
				continue;
			if (!e.GetPrefabData())
			{
				s_iSkipPrefab++;
				continue;
			}
			if (e.FindComponent(RplComponent))
			{
				s_iSkipRpl++;
				continue;
			}
			vector o = e.GetOrigin();
			int k = Key(o[0], o[2]);
			// case proche de la caméra (déjà « montrée » par la file) : on n'y touche pas
			if (!s_sHidden.Contains(k))
				continue;
			ZO_CellStore st = s_mStored.Get(k);
			if (!st)
			{
				st = new ZO_CellStore();
				s_mStored.Insert(k, st);
			}
			int pidx = st.Add(e);
			bool hp = e.GetPhysics() != null;
			int tm1 = System.GetTickCount();
			delete e;
			int dm = System.GetTickCount() - tm1;
			s_iDelMs += dm;
			s_iDel++;
			// diagnostic (tests PC seulement) : coût par prefab, une seule fois
			if (s_bDiag && !s_bPrefDone)
			{
				if (!s_aPrefMs || !s_aPrefN)
				{
					s_aPrefMs = {};
					s_aPrefN = {};
				}
				while (s_aPrefMs.Count() <= pidx)
				{
					s_aPrefMs.Insert(0);
					s_aPrefN.Insert(0);
				}
				s_aPrefMs[pidx] = s_aPrefMs[pidx] + dm;
				s_aPrefN[pidx] = s_aPrefN[pidx] + 1;
			}
			if (hp)
			{
				s_iPhysN++;
				s_iPhysMs += dm;
			}
		}
		// fin : toutes les cases sont « cachées » ; on retire des cases les pointeurs morts
		foreach (int k2, array<IEntity> a : s_mCells)
		{
			for (int i = a.Count() - 1; i >= 0; i--)
			{
				if (!a[i])
					a.Remove(i);
			}
		}
		s_aOrder = null;
		s_aShow = null;	// force le recalcul de la file (recréation autour de la caméra)
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Mode "chunk" : on SUPPRIME vraiment les objets loin (prefab + position gardés), on les recrée en approchant
	protected static ref map<int, ref ZO_CellStore> s_mStored;
	protected static int s_iDel, s_iSkipRpl, s_iSkipPrefab, s_iRespawn;
	protected static ref array<int> s_aShow;		// cases à recréer, triées : les plus proches d'abord
	protected static ref array<int> s_aHide;		// cases à purger
	protected static int s_iShowPos, s_iHidePos;
	protected static bool s_bInitDone;
	protected static int s_iInitStart;
	protected static float s_fLogTimer;

	protected static void ChunkCell(int k, array<IEntity> a, bool visible)
	{
		if (!s_mStored)
			s_mStored = new map<int, ref ZO_CellStore>();
		if (!visible)
		{
			// la case a pu être en partie purgée par InitPurge : on complète son stock
			ZO_CellStore st = s_mStored.Get(k);
			if (!st)
				st = new ZO_CellStore();
			array<IEntity> keep = {};
			foreach (IEntity e : a)
			{
				if (!e)
					continue;
				if (!e.GetPrefabData())
				{
					s_iSkipPrefab++;
					keep.Insert(e);
					continue;
				}
				if (e.FindComponent(RplComponent))
				{
					s_iSkipRpl++;
					keep.Insert(e);
					continue;
				}
				s_iDel++;
				st.Add(e);
				int tm1 = System.GetTickCount();
				delete e;
				s_iDelMs += System.GetTickCount() - tm1;
			}
			// la case ne garde que ce qui n'a pas été supprimé
			a.Clear();
			a.InsertAll(keep);
			s_mStored.Set(k, st);
			return;
		}
		ZO_CellStore st2 = s_mStored.Get(k);
		if (!st2)
			return;
		BaseWorld w = GetGame().GetWorld();
		vector mat[4];
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		int cnt = st2.m_aPrefab.Count();
		for (int i = 0; i < cnt; i++)
		{
			Resource r = ZO_CellStore.GetPrefab(st2.m_aPrefab[i]);
			if (!r || !r.IsValid())
				continue;
			mat[0] = st2.m_aMat[i * 4];
			mat[1] = st2.m_aMat[i * 4 + 1];
			mat[2] = st2.m_aMat[i * 4 + 2];
			mat[3] = st2.m_aMat[i * 4 + 3];
			sp.Transform = mat;
			IEntity ne = GetGame().SpawnEntityPrefabLocal(r, w, sp);
			if (ne)
			{
				a.Insert(ne);
				s_iRespawn++;
			}
		}
		s_mStored.Remove(k);
	}


	//------------------------------------------------------------------------------------------------
	// Liste les cases qui doivent changer d'état. hasCam=false (chargement, pas encore de caméra) : tout est « loin ».
	protected static void QueueGrid(map<int, ref array<IEntity>> cells, set<int> hiddenSet, float radius, vector c, bool hasCam, int grid)
	{
		float r2 = (radius + CELL) * (radius + CELL);	// marge d'une case : rien ne disparaît sous le nez
		foreach (int k, array<IEntity> a : cells)
		{
			bool far = true;
			float d2 = 0;
			if (hasCam)
			{
				float cx = (Math.Floor(k / 1000) + 0.5) * CELL;
				float cz = (Math.Mod(k, 1000) + 0.5) * CELL;
				float dx = cx - c[0];
				float dz = cz - c[2];
				d2 = dx * dx + dz * dz;
				far = d2 > r2;
			}
			if (far == hiddenSet.Contains(k))
				continue;
			int code = (k + KEY_OFS) * 2 + grid;
			if (far)
				s_aHide.Insert(code);
			else
				s_aShow.Insert((int)(Math.Sqrt(d2) / CELL) * DIST_MUL + code);
		}
	}

	protected static void BuildQueue(vector c, bool hasCam)
	{
		s_aShow = {};
		s_aHide = {};
		QueueGrid(s_mCells, s_sHidden, s_fRadius, c, hasCam, 0);
		QueueGrid(s_mInCells, s_sInHidden, s_fInRadius, c, hasCam, 1);
		s_aShow.Sort();
		s_iShowPos = 0;
		s_iHidePos = 0;
	}

	protected static void DoCell(int code, bool visible)
	{
		code = code % DIST_MUL;
		int grid = code % 2;
		int k = code / 2 - KEY_OFS;
		map<int, ref array<IEntity>> cells = s_mCells;
		set<int> hiddenSet = s_sHidden;
		if (grid == 1)
		{
			cells = s_mInCells;
			hiddenSet = s_sInHidden;
		}
		array<IEntity> a = cells.Get(k);
		if (!a)
			return;
		bool hidden = hiddenSet.Contains(k);
		if (visible && hidden)
		{
			ChunkCell(k + grid * 10000000, a, true);
			hiddenSet.RemoveItem(k);
		}
		else if (!visible && !hidden)
		{
			ChunkCell(k + grid * 10000000, a, false);
			hiddenSet.Insert(k);
		}
	}

	protected static int QueueLeft()
	{
		if (s_aOrder)
			return s_aOrder.Count() - s_iOrderPos;
		if (!s_aShow)
			return 0;
		return (s_aShow.Count() - s_iShowPos) + (s_aHide.Count() - s_iHidePos);
	}

	// Traite la file jusqu'à épuisement du budget. true = file vide.
	protected static bool Process(int budgetMs)
	{
		if (!s_aShow)
			return true;
		int t0 = System.GetTickCount();
		while (s_iShowPos < s_aShow.Count())
		{
			if (System.GetTickCount() - t0 >= budgetMs)
				return false;
			DoCell(s_aShow[s_iShowPos], true);
			s_iShowPos++;
		}
		while (s_iHidePos < s_aHide.Count())
		{
			if (System.GetTickCount() - t0 >= budgetMs)
				return false;
			DoCell(s_aHide[s_iHidePos], false);
			s_iHidePos++;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Mode chunk : appelé à CHAQUE image dès que le monde existe
	protected static void ChunkFrame(BaseWorld world, float timeslice)
	{
		IEntity pl = SCR_PlayerController.GetLocalControlledEntity();
		CameraManager cm = GetGame().GetCameraManager();
		bool hasCam = false;
		if (pl)
			hasCam = true;
		else if (cm && cm.CurrentCamera())
			hasCam = true;
		vector c;
		int ck = -999998;
		if (hasCam)
		{
			vector cam[4];
			world.GetCurrentCamera(cam);
			c = cam[3];
			ck = Key(c[0], c[2]);
		}
		int budget = PLAY_BUDGET_MS;
		if (!pl)
			budget = s_iLoadBudgetMs;
		int tp = System.GetTickCount();
		// 1) la file (recréation des cases proches en priorité, purge des cases qui s'éloignent)
		if (ck != s_iLastCell || !s_aShow)
		{
			s_iLastCell = ck;
			BuildQueue(c, hasCam);
		}
		bool empty = Process(budget);
		// 2) le temps qui reste : purge globale dans l'ordre des ID
		if (empty && s_aOrder)
		{
			empty = false;
			int left = budget - (System.GetTickCount() - tp);
			if (left > 0 && InitPurge(left))
				PrintFormat("[ZO] CHUNK suppression globale finie en %1 s (supprimes=%2) memMo=%3", (System.GetTickCount() - s_iInitStart) / 1000.0, s_iDel, System.MemoryAllocationKB() / 1024);
		}
		s_iSpentMs += System.GetTickCount() - tp;
		s_iFrames++;
		if (empty && !s_bInitDone)
		{
			s_bInitDone = true;
			PrintFormat("[ZO] CHUNK purge initiale terminee en %1 s (supprimes=%2, ignores Rpl=%3 sansPrefab=%4, joueur=%5) memMo=%6", (System.GetTickCount() - s_iInitStart) / 1000.0, s_iDel, s_iSkipRpl, s_iSkipPrefab, pl != null, System.MemoryAllocationKB() / 1024);
		}
		s_fLogTimer += timeslice;
		if (s_fLogTimer >= 5)
		{
			s_fLogTimer = 0;
			if (s_bDiag)
			{
				PrintFormat("[ZO] CHUNK supprimes=%1 recrees=%2 ignores(Rpl)=%3 ignores(sansPrefab)=%4 file=%5 memMo=%6 images=%7 scriptMs=%8 budget=%9", s_iDel, s_iRespawn, s_iSkipRpl, s_iSkipPrefab, QueueLeft(), System.MemoryAllocationKB() / 1024, s_iFrames, s_iSpentMs, budget);
				PrintFormat("[ZO] CHUNK detail : suppression=%1 ms, lecture=%2 ms ; avec physique n=%3 ms=%4", s_iDelMs, s_iMetaMs, s_iPhysN, s_iPhysMs);
				if (s_aPrefN && s_iDel > 300000)
				{
					for (int pi = 0; pi < s_aPrefN.Count(); pi++)
					{
						if (s_aPrefN[pi] >= 2000)
							PrintFormat("[ZO] PREF %1 n=%2 ms=%3", ZO_CellStore.GetPrefabName(pi), s_aPrefN[pi], s_aPrefMs[pi]);
					}
					s_aPrefN = null;
					s_aPrefMs = null;
					s_bPrefDone = true;
				}
			}
			s_iDelMs = 0;
			s_iMetaMs = 0;
			s_iFrames = 0;
			s_iSpentMs = 0;
		}
	}

	//------------------------------------------------------------------------------------------------
	// Ancien mode diagnostic : masquage du drapeau VISIBLE (aucun gain mémoire)
	protected static void SetCell(array<IEntity> a, bool visible)
	{
		foreach (IEntity e : a)
		{
			if (!e)
				continue;
			if (visible)
				e.SetFlags(EntityFlags.VISIBLE, false);
			else
				e.ClearFlags(EntityFlags.VISIBLE, false);
		}
	}

	protected static void ApplyVisibility(map<int, ref array<IEntity>> cells, set<int> hiddenSet, float radius, vector c)
	{
		float r2 = (radius + CELL) * (radius + CELL);
		foreach (int k, array<IEntity> a : cells)
		{
			float cx = (Math.Floor(k / 1000) + 0.5) * CELL;
			float cz = (Math.Mod(k, 1000) + 0.5) * CELL;
			float dx = cx - c[0];
			float dz = cz - c[2];
			bool far = (dx * dx + dz * dz) > r2;
			bool hidden = hiddenSet.Contains(k);
			if (far && !hidden)
			{
				SetCell(a, false);
				hiddenSet.Insert(k);
			}
			else if (!far && hidden)
			{
				SetCell(a, true);
				hiddenSet.RemoveItem(k);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static float s_fAge, s_fVisTimer;

	static void Reset()
	{
		s_mCells = null;
		s_sHidden = null;
		s_mInCells = null;
		s_sInHidden = null;
		s_mStored = null;
		ZO_CellStore.ResetTable();
		s_aOrder = null;
		s_iOrderPos = 0;
		s_aShow = null;
		s_aHide = null;
		s_fRadius = -1;
		s_iLastCell = -999999;
		s_bInitDone = false;
		s_fAge = 0;
		s_fVisTimer = 0;
		s_fLogTimer = 0;
		s_iDel = 0;
		s_bPrefDone = false;
		s_aPrefMs = null;
		s_aPrefN = null;
		s_iRespawn = 0;
		s_iSkipRpl = 0;
		s_iSkipPrefab = 0;
	}

	static void Frame(BaseWorld world, float timeslice)
	{
		s_fAge += timeslice;
		if (s_fRadius < 0)
		{
			if (s_fAge < 1)		// une seconde : le monde statique est chargé
				return;
			s_fRadius = GetRadius();
			if (s_fRadius > 0)
			{
				s_iInitStart = System.GetTickCount();
				Build(world);
			}
		}
		if (s_fRadius <= 0 || !s_mCells)
			return;

		if (IsChunk())
		{
			ChunkFrame(world, timeslice);
			return;
		}

		// mode visibilité : 15 s d'attente puis 2 fois par seconde
		if (s_fAge < 15)
			return;
		s_fVisTimer += timeslice;
		if (s_fVisTimer < 0.5)
			return;
		s_fVisTimer = 0;
		vector cam[4];
		world.GetCurrentCamera(cam);
		vector c = cam[3];
		int ck = Key(c[0], c[2]);
		if (ck == s_iLastCell)
			return;
		s_iLastCell = ck;
		ApplyVisibility(s_mCells, s_sHidden, s_fRadius, c);
		ApplyVisibility(s_mInCells, s_sInHidden, s_fInRadius, c);
	}
}

//------------------------------------------------------------------------------------------------
// Stockage d'une case purgée. IMPORTANT : AUCUN objet script par arbre (seulement des tableaux de valeurs) :
// avec 1,8 M petits objets script, chaque « delete » d'entité devenait de plus en plus lent (0,1 -> 0,9 ms).
class ZO_CellStore
{
	ref array<int> m_aPrefab = {};		// index dans la table des prefabs
	ref array<vector> m_aMat = {};		// 4 vecteurs par objet

	protected static ref array<ref Resource> s_aRes;
	protected static ref map<ResourceName, int> s_mIdx;

	static void ResetTable()
	{
		s_aRes = null;
		s_mIdx = null;
	}

	static Resource GetPrefab(int idx)
	{
		if (!s_aRes || idx < 0 || idx >= s_aRes.Count())
			return null;
		return s_aRes[idx];
	}

	static string GetPrefabName(int idx)
	{
		foreach (ResourceName rn, int i : s_mIdx)
		{
			if (i == idx)
				return rn;
		}
		return "";
	}

	int Add(IEntity e)
	{
		if (!s_aRes)
		{
			s_aRes = {};
			s_mIdx = new map<ResourceName, int>();
		}
		ResourceName rn = e.GetPrefabData().GetPrefabName();
		int idx;
		if (!s_mIdx.Find(rn, idx))
		{
			idx = s_aRes.Count();
			s_aRes.Insert(Resource.Load(rn));
			s_mIdx.Insert(rn, idx);
		}
		m_aPrefab.Insert(idx);
		vector mat[4];
		e.GetWorldTransform(mat);
		m_aMat.Insert(mat[0]);
		m_aMat.Insert(mat[1]);
		m_aMat.Insert(mat[2]);
		m_aMat.Insert(mat[3]);
		return idx;
	}
}

//------------------------------------------------------------------------------------------------
modded class ArmaReforgerScripted
{
	protected int m_iZO_IsZimni;		// 0 = pas encore testé, 1 = oui, 2 = non
	protected BaseWorld m_ZO_World;

	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		if (!world)
			return;
		// changement de monde (menu -> partie, reconnexion) : tout repart de zéro
		if (world != m_ZO_World)
		{
			m_ZO_World = world;
			m_iZO_IsZimni = 0;
			ZO_PropCuller.Reset();
			ZO_Streamer.Reset();
		}
		if (m_iZO_IsZimni == 0)
		{
			m_iZO_IsZimni = 2;
			if (GetWorldFile().Contains("Zimnitrita"))
				m_iZO_IsZimni = 1;
		}
		if (m_iZO_IsZimni != 1)
			return;
		// le streaming tourne aussi sur le serveur dédié (il y crée toute la carte) ; le culler, jamais
		ZO_Streamer.Frame(world, timeslice);
		if (System.IsConsoleApp())
			return;
		ZO_PropCuller.Frame(world, timeslice);
	}
}
