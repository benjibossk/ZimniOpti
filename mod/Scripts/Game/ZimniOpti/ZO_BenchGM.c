// ZimniOpti — banc client/serveur : sur le serveur de TEST uniquement ($profile:ZimniPerf/allowwar),
// chaque joueur qui arrive reçoit le Game Master et l'éditeur s'ouvre (sinon le client reste bloqué au menu de déploiement).
modded class SCR_BaseGameMode
{
	protected override void OnPlayerRegistered(int playerId)
	{
		super.OnPlayerRegistered(playerId);
		if (Replication.IsServer() && FileIO.FileExists("$profile:ZimniPerf/allowwar"))
			GetGame().GetCallqueue().CallLater(ZO_GiveGM, 3000, false, playerId);
	}

	protected void ZO_GiveGM(int playerId)
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return;
		SCR_EditorManagerEntity em = core.GetEditorManager(playerId);
		if (!em)
		{
			GetGame().GetCallqueue().CallLater(ZO_GiveGM, 2000, false, playerId);	// pas encore créé : on réessaie
			return;
		}
		em.AddEditorModes(EEditorModeAccess.BASE, EEditorMode.EDIT);
		em.SetCurrentMode(EEditorMode.EDIT);
		em.Open(false);
		Print("[ZO] GM donne au joueur " + playerId);
	}
}
