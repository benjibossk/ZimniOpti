// Lumières de la carte (piste F11 : 529 lumières contre ~54 sur les cartes Bohemia).
// Test PC : $profile:ZimniPerf/lights = "count" (recense seulement), "noshadow" (coupe les ombres), "off" (éteint tout).
// Le recensement se refait toutes les 60 s (des lampes s'allument à la tombée de la nuit).
class ZO_Lights
{
	protected static float s_fAge;
	protected static float s_fNext = 15;
	protected static int s_iMode = -1;	// -1 = pas lu, 0 = inactif, 1 = count, 2 = noshadow, 3 = off
	protected static int s_iTotal, s_iOn, s_iShadow, s_iDyn, s_iChanged;

	static void Reset()
	{
		s_fAge = 0;
		s_fNext = 15;
		s_iMode = -1;
	}

	static void Frame(BaseWorld world, float timeslice)
	{
		if (s_iMode == 0)
			return;
		if (s_iMode < 0)
		{
			s_iMode = 0;
			FileHandle h = FileIO.OpenFile("$profile:ZimniPerf/lights", FileMode.READ);
			if (!h)
				return;
			string m;
			h.ReadLine(m);
			h.Close();
			m.Trim();
			if (m == "count")
				s_iMode = 1;
			else if (m == "noshadow")
				s_iMode = 2;
			else if (m == "off")
				s_iMode = 3;
			PrintFormat("[ZO] LIGHTS mode %1", m);
		}
		s_fAge += timeslice;
		if (s_fAge < s_fNext)
			return;
		s_fNext = s_fAge + 60;
		s_iTotal = 0;
		s_iOn = 0;
		s_iShadow = 0;
		s_iDyn = 0;
		s_iChanged = 0;
		int t0 = System.GetTickCount();
		world.QueryEntitiesByAABB(Vector(-1000, -500, -1000), Vector(30000, 3000, 30000), OnLight, FilterLight, EQueryEntitiesFlags.ALL);
		PrintFormat("[ZO] LIGHTS total=%1 allumees=%2 ombres=%3 dynamiques=%4 modifiees=%5 (%6 ms)", s_iTotal, s_iOn, s_iShadow, s_iDyn, s_iChanged, System.GetTickCount() - t0);
	}

	protected static bool FilterLight(IEntity e)
	{
		return LightEntity.Cast(e) != null;
	}

	protected static bool OnLight(IEntity e)
	{
		LightEntity l = LightEntity.Cast(e);
		if (!l)
			return true;
		s_iTotal++;
		if (l.IsEnabled())
			s_iOn++;
		if (l.IsCastShadow())
			s_iShadow++;
		if (l.IsDynamic())
			s_iDyn++;
		if (s_iMode == 2 && l.IsCastShadow())
		{
			l.SetCastShadow(false);
			s_iChanged++;
		}
		else if (s_iMode == 3 && l.IsEnabled())
		{
			l.SetEnabled(false);
			s_iChanged++;
		}
		return true;
	}
}
