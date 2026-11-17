// ZimniOpti — mesure de performance par joueur (PC + consoles).
// Chaque client agrège ses frames sur ZO_WINDOW secondes (FPS moyen, pire frame, mémoire, position)
// et envoie un résumé au serveur, qui l'écrit dans $profile:ZimniPerf/perf_<date>.csv.
// Le serveur ajoute aussi sa propre ligne (playerId 0) : ses FPS = charge CPU de simulation (IA, physique, réseau).

//------------------------------------------------------------------------------------------------
class ZO_PerfLog
{
	protected static string s_sPath;

	//------------------------------------------------------------------------------------------------
	protected static string GetPath()
	{
		if (!s_sPath.IsEmpty())
			return s_sPath;

		int y, m, d;
		System.GetYearMonthDay(y, m, d);
		FileIO.MakeDirectory("$profile:ZimniPerf");
		s_sPath = string.Format("$profile:ZimniPerf/perf_%1-%2-%3.csv", y, m.ToString(2), d.ToString(2));
		if (!FileIO.FileExists(s_sPath))
		{
			FileHandle h = FileIO.OpenFile(s_sPath, FileMode.WRITE);
			if (h)
			{
				h.WriteLine("unix;playerId;plateforme;fpsMoyen;pireFrameMs;memoireMo;x;y;z");
				h.Close();
			}
		}
		return s_sPath;
	}

	//------------------------------------------------------------------------------------------------
	static void Write(int playerId, string platform, float fps, float worstMs, int memMB, vector pos)
	{
		FileHandle h = FileIO.OpenFile(GetPath(), FileMode.APPEND);
		if (!h)
			return;

		h.WriteLine(string.Format("%1;%2;%3;%4;%5;%6;%7;%8;%9",
			System.GetUnixTime(), playerId, platform,
			fps.ToString(-1, 1), worstMs.ToString(-1, 1), memMB,
			pos[0].ToString(-1, 0), pos[1].ToString(-1, 0), pos[2].ToString(-1, 0)));
		h.Close();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	protected static const float ZO_WINDOW = 5.0;

	protected float m_fZO_Acc;
	protected int m_iZO_Frames;
	protected float m_fZO_Worst;

	//------------------------------------------------------------------------------------------------
	override void OnUpdate(float timeSlice)
	{
		super.OnUpdate(timeSlice);

		if (!m_bIsLocalPlayerController)
			return;

		m_fZO_Acc += timeSlice;
		m_iZO_Frames++;
		if (timeSlice > m_fZO_Worst)
			m_fZO_Worst = timeSlice;

		if (m_fZO_Acc < ZO_WINDOW)
			return;

		vector pos = vector.Zero;
		IEntity ent = GetControlledEntity();
		if (ent)
			pos = ent.GetOrigin();

		float fps = m_iZO_Frames / m_fZO_Acc;
		float worstMs = m_fZO_Worst * 1000;
		int memMB = System.MemoryAllocationKB() / 1024;

		m_fZO_Acc = 0;
		m_iZO_Frames = 0;
		m_fZO_Worst = 0;

		if (Replication.IsServer())
			ZO_Record(fps, worstMs, memMB, pos);
		else
			Rpc(RpcAsk_ZO_Perf, fps, worstMs, memMB, pos);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void RpcAsk_ZO_Perf(float fps, float worstMs, int memMB, vector pos)
	{
		ZO_Record(fps, worstMs, memMB, pos);
	}

	//------------------------------------------------------------------------------------------------
	protected void ZO_Record(float fps, float worstMs, int memMB, vector pos)
	{
		int playerId = GetPlayerId();
		string platform = typename.EnumToString(PlatformKind, GetGame().GetPlayerManager().GetPlatformKind(playerId));
		ZO_PerfLog.Write(playerId, platform, fps, worstMs, memMB, pos);
	}
}

//------------------------------------------------------------------------------------------------
// Ligne serveur (playerId 0) : FPS du serveur dédié = charge CPU de la simulation.
modded class SCR_BaseGameMode
{
	//------------------------------------------------------------------------------------------------
	override void OnGameStart()
	{
		super.OnGameStart();

		// Serveur dédié uniquement (IsConsoleApp = pas de rendu, rien à voir avec les consoles de jeu)
		if (Replication.IsServer() && System.IsConsoleApp())
			GetGame().GetCallqueue().CallLater(ZO_ServerSample, 5000, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void ZO_ServerSample()
	{
		// GetFrameTimeS = moyenne des 10 dernières frames (pas de pire frame côté serveur)
		ZO_PerfLog.Write(0, "SERVEUR", System.GetFPS(), System.GetFrameTimeS() * 1000, System.MemoryAllocationKB() / 1024, vector.Zero);
	}
}
