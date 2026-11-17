// ZimniOpti — charge « guerre » pour le banc : N escouades US à 150 m à l'ouest, N URSS à 150 m à l'est.
// Les IA sont toujours créées sur le SERVEUR (là où elles sont calculées en vrai partie).
// En solo : appel direct. En client connecté à un serveur dédié : demande par RPC du contrôleur joueur.

class ZO_War
{
	protected static const ResourceName WAR_US = "{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et";
	protected static const ResourceName WAR_RU = "{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et";

	protected static ResourceName Veh(int i)
	{
		if (i == 0) return "{C012BB3488BEA0C2}Prefabs/Vehicles/Wheeled/BTR70/BTR70.et";
		if (i == 1) return "{4597626AF36C0858}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320.et";
		if (i == 2) return "{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et";
		return "{259EE7B78C51B624}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469.et";
	}

	protected static ref array<IEntity> s_aGroups = {};
	protected static ref array<IEntity> s_aVeh = {};
	protected static int s_iBoomIdx;
	protected static IEntity s_Building;
	protected static vector s_vCenter;
	protected static int s_iAlive;

	//------------------------------------------------------------------------------------------------
	// squads = 0 : tout supprimer
	// veh = véhicules posés entre les camps ; boomS > 0 : toutes les boomS s, un véhicule explose + un bâtiment s'effondre
	static void Request(vector center, int squads, int veh, int boomS)
	{
		s_vCenter = center;
		if (Replication.IsServer())
		{
			ServerApply(center, squads, veh, boomS);
			return;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (pc)
			pc.ZO_AskWar(center, squads, veh, boomS);
	}

	//------------------------------------------------------------------------------------------------
	static void ServerApply(vector center, int squads, int veh, int boomS)
	{
		Clear();
		s_vCenter = center;
		BaseWorld world = GetGame().GetWorld();
		for (int v = 0; v < veh; v++)
		{
			int col = Math.Mod(v, 6);
			int row = v / 6;
			float vx = center[0] + (col - 2.5) * 25;
			float vz = center[2] + (row - 1) * 30;
			IEntity ve = Spawn(Veh(Math.Mod(v, 4)), Vector(vx, 0, vz), world);
			if (ve)
				s_aVeh.Insert(ve);
		}
		if (boomS > 0)
			GetGame().GetCallqueue().CallLater(Boom, boomS * 1000, true);
		if (squads <= 0 && veh <= 0)
			return;

		for (int i = 0; i < squads; i++)
		{
			float dz = (i - squads * 0.5) * 40;
			SpawnGroup(WAR_US, Vector(center[0] - 150, 0, center[2] + dz), world);
			SpawnGroup(WAR_RU, Vector(center[0] + 150, 0, center[2] + dz), world);
		}
		Print(string.Format("[ZO] GUERRE : %1 escouades/camp, %2 vehicules, boom %3 s a %4", squads, s_aVeh.Count(), boomS, center));
	}

	//------------------------------------------------------------------------------------------------
	protected static void SpawnGroup(ResourceName prefab, vector pos, BaseWorld world)
	{
		IEntity g = Spawn(prefab, pos, world);
		if (g)
			s_aGroups.Insert(g);
	}

	protected static IEntity Spawn(ResourceName prefab, vector pos, BaseWorld world)
	{
		pos[1] = world.GetSurfaceY(pos[0], pos[2]) + 0.5;
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		sp.Transform[3] = pos;
		return GetGame().SpawnEntityPrefab(Resource.Load(prefab), world, sp);
	}

	//------------------------------------------------------------------------------------------------
	// Une « frappe » : le prochain véhicule intact explose, et le bâtiment destructible le plus proche du centre s'effondre.
	protected static void Boom()
	{
		for (int i = 0; i < s_aVeh.Count(); i++)
		{
			IEntity v = s_aVeh[Math.Mod(s_iBoomIdx + i, s_aVeh.Count())];
			SCR_DamageManagerComponent dm = SCR_DamageManagerComponent.GetDamageManager(v);
			if (dm && !dm.GetState() == EDamageState.DESTROYED)
			{
				dm.Kill(Instigator.CreateInstigator(null));
				s_iBoomIdx += i + 1;
				break;
			}
		}

		s_Building = null;
		GetGame().GetWorld().QueryEntitiesBySphere(s_vCenter, 250, FindBuilding, null, EQueryEntitiesFlags.STATIC);
		if (s_Building)
		{
			// l'effondrement peut supprimer l'entité : on lit tout AVANT
			vector bpos = s_Building.GetOrigin();
			SCR_DestructibleBuildingComponent b = SCR_DestructibleBuildingComponent.Cast(s_Building.FindComponent(SCR_DestructibleBuildingComponent));
			s_Building = null;
			if (b)
			{
				b.GoToDestroyedStateLoad(true);
				Print("[ZO] BOOM batiment " + bpos);
			}
		}
	}

	protected static bool FindBuilding(IEntity e)
	{
		SCR_DestructibleBuildingComponent b = SCR_DestructibleBuildingComponent.Cast(e.FindComponent(SCR_DestructibleBuildingComponent));
		if (!b)
			return true;
		SCR_DamageManagerComponent dm = SCR_DamageManagerComponent.GetDamageManager(e);
		if (dm && dm.GetState() == EDamageState.DESTROYED)
			return true;
		s_Building = e;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Clear()
	{
		GetGame().GetCallqueue().Remove(Boom);
		foreach (IEntity v : s_aVeh)
		{
			if (v)
				SCR_EntityHelper.DeleteEntityAndChildren(v);
		}
		s_aVeh.Clear();
		foreach (IEntity e : s_aGroups)
		{
			SCR_AIGroup g = SCR_AIGroup.Cast(e);
			if (!g)
				continue;
			array<AIAgent> agents = {};
			g.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				if (a && a.GetControlledEntity())
					SCR_EntityHelper.DeleteEntityAndChildren(a.GetControlledEntity());
			}
			SCR_EntityHelper.DeleteEntityAndChildren(g);
		}
		s_aGroups.Clear();
	}

	//------------------------------------------------------------------------------------------------
	// Compte les soldats vivants à moins de 300 m du centre (marche aussi côté client : entités répliquées).
	static int CountAlive()
	{
		if (s_vCenter == vector.Zero)
			return 0;
		s_iAlive = 0;
		GetGame().GetWorld().QueryEntitiesBySphere(s_vCenter, 300, CountOne, null, EQueryEntitiesFlags.DYNAMIC);
		return s_iAlive;
	}

	protected static bool CountOne(IEntity e)
	{
		SCR_ChimeraCharacter c = SCR_ChimeraCharacter.Cast(e);
		if (c && c.GetCharacterController() && !c.GetCharacterController().IsDead())
			s_iAlive++;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	void ZO_AskWar(vector center, int squads, int veh, int boomS)
	{
		Rpc(RpcAsk_ZO_War, center, squads, veh, boomS);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ZO_War(vector center, int squads, int veh, int boomS)
	{
		// Sécurité : seul un serveur de TEST (fichier $profile:ZimniPerf/allowwar) accepte. Jamais sur un vrai serveur.
		if (!FileIO.FileExists("$profile:ZimniPerf/allowwar"))
			return;
		ZO_War.ServerApply(center, squads, veh, boomS);
	}
}
