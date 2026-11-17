d=r"C:/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti/Scripts/Game/ZimniOpti/"
def rep(f,a,b):
    s=open(d+f,encoding='utf-8',newline='').read()
    if chr(13) in s: a=a.replace(chr(10),chr(13)+chr(10)); b=b.replace(chr(10),chr(13)+chr(10))
    assert s.count(a)==1,(f,a); open(d+f,'w',encoding='utf-8',newline='').write(s.replace(a,b))
rep('ZO_ConsoleViewCap.c','''		if (FileIO.FileExists("$profile:ZimniPerf/forcecap"))
			return 1200;''','''		if (FileIO.FileExists("$profile:ZimniPerf/forcecap"))
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
		}''')
rep('ZO_PropCuller.c','''			s_fInRadius = 90;
''','''			s_fInRadius = 90;
		FileHandle irf = FileIO.OpenFile("$profile:ZimniPerf/inradius", FileMode.READ);
		if (irf)
		{
			string irl;
			irf.ReadLine(irl);
			irf.Close();
			if (irl.ToFloat() > 0)
				s_fInRadius = irl.ToFloat();
		}
''')
print('patch ok')
