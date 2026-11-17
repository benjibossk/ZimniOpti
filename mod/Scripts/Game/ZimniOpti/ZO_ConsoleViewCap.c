// ZimniOpti — plafond de distance de vue sur console (fiche F5 de OPTI_MEMOIRE.md).
// Mesure PC 2026-11-09 : 2500 m -> 1000 m = +17 % FPS en moyenne, jusqu'à +50 % sur les pires points.
// Le jeu remet la distance du menu vidéo à plusieurs endroits (caméra, inventaire, menu) :
// on revérifie donc toutes les 2 s et on ne fait que BAISSER, jamais monter.

modded class ArmaReforgerScripted
{
	protected float m_fZO_CapTimer;
	protected bool m_bZO_ShadowsDone;
	protected int m_iZO_OrigDSQ = -1;	// valeur d'origine des ombres lointaines, remise en fin de partie (le jeu sauvegarde les réglages en quittant)

	//------------------------------------------------------------------------------------------------
	protected void ZO_RestoreShadows()
	{
		m_bZO_ShadowsDone = false;
		if (m_iZO_OrigDSQ < 0)
			return;
		BaseContainer vm = GetEngineUserSettings().GetModule("VideoUserSettings");
		if (vm)
		{
			vm.Set("DistantShadowsQuality", m_iZO_OrigDSQ);
			UserSettingsChanged();
			Print("[ZO] OMBRES lointaines remises a " + m_iZO_OrigDSQ);
		}
		m_iZO_OrigDSQ = -1;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnMissionSet(MissionHeader mission)
	{
		ZO_RestoreShadows();
		super.OnMissionSet(mission);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameEnd()
	{
		ZO_RestoreShadows();
		super.OnGameEnd();
	}

	//------------------------------------------------------------------------------------------------
	protected static float ZO_GetViewCap()
	{
		EPlatform p = System.GetPlatform();
		if (p == EPlatform.XBOX_SERIES_S || p == EPlatform.XBOX_ONE_S || p == EPlatform.XBOX_ONE_X || p == EPlatform.PS4)
			return 1200;
		if (p == EPlatform.XBOX_SERIES_X || p == EPlatform.PS5 || p == EPlatform.PS5_PRO)
			return 1600;
		// Test PC : fichier $profile:ZimniPerf/forcecap = simule une Series S
		if (FileIO.FileExists("$profile:ZimniPerf/forcecap"))
		{
			float cv = 1200;
			FileHandle cf = FileIO.OpenFile("$profile:ZimniPerf/capval", FileMode.READ);
			if (cf)
			{
				string cl;
				cf.ReadLine(cl);
				cf.Close();
				if (cl.ToFloat() > 100)
					cv = cl.ToFloat();
			}
			return cv;
		}
		return 0; // PC : pas de plafond
	}

	//------------------------------------------------------------------------------------------------
	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);

		m_fZO_CapTimer += timeslice;
		if (m_fZO_CapTimer < 2)
			return;
		m_fZO_CapTimer = 0;

		if (!world || System.IsConsoleApp())
			return;

		float cap = ZO_GetViewCap();
		if (cap <= 0)
			return;
		if (!GetWorldFile().Contains("Zimnitrita"))
		{
			ZO_RestoreShadows();
			return;
		}

		// Console : ombres lointaines au minimum (F32 : +8,6 % FPS à pied, peu visible). Une fois ; valeur d'origine remise en quittant.
		if (!m_bZO_ShadowsDone)
		{
			m_bZO_ShadowsDone = true;
			BaseContainer vm = GetEngineUserSettings().GetModule("VideoUserSettings");
			int dsq;
			if (vm && vm.Get("DistantShadowsQuality", dsq) && dsq > 0)
			{
				m_iZO_OrigDSQ = dsq;
				vm.Set("DistantShadowsQuality", 0);
				UserSettingsChanged();
				Print("[ZO] OMBRES lointaines " + dsq + " -> 0");
			}
		}

		if (GetViewDistance() <= cap)
			return;

		SetViewDistance(cap);
	}
}
