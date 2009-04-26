#include "commonheaders.h"


HINSTANCE LoadIconsPack(const char* szIconsPack)
{
	HINSTANCE hNewIconInst = NULL;
	WORD i;

	hNewIconInst = LoadLibrary(szIconsPack);

	if (hNewIconInst != NULL)
	{
		for(i=ID_FIRSTICON; i<=ID_LASTICON; i++)
			if (LoadIcon(hNewIconInst, MAKEINTRESOURCE(i)) == NULL)
			{
				FreeLibrary(hNewIconInst);
				hNewIconInst = NULL;
				break;
			}
	}
	return hNewIconInst;
}



int ReloadIcons(WPARAM wParam, LPARAM lParam)
{
	HICON hIcon[ALL_CNT] = {0};
	for (int i=0; icons[i].key; i++) {
		if(!hIcon[i]) {
			hIcon[i] = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)icons[i].name);
		}
		if(icons[i].tbl == TBL_IEC)
			g_hIEC[icons[i].idx]=hIcon[i];
		else
		if(icons[i].tbl == TBL_ICO)
			g_hICO[icons[i].idx]=hIcon[i];
		else
		if(icons[i].tbl == TBL_POP)
			g_hPOP[icons[i].idx]=hIcon[i];
	}

	return 0;
}


void InitIcons(void)
{
	HINSTANCE hNewIconInst = NULL;

	if (hNewIconInst == NULL)
		hNewIconInst = LoadIconsPack("icons\\secureim_icons.dll");

	if (hNewIconInst == NULL)
		hNewIconInst = LoadIconsPack("plugins\\secureim_icons.dll");

	if (hNewIconInst == NULL)
		g_hIconInst = g_hInst;
	else
		g_hIconInst = hNewIconInst;


	SKINICONDESC sid={0};

	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszSection = "SecureIM";

	HICON hIcon[ALL_CNT] = {0};
	for (int i=0; icons[i].key; i++) {
		if(ServiceExists(MS_SKIN2_ADDICON)) {
			sid.pszDescription = Translate(icons[i].text);
			sid.pszName = icons[i].name;
			sid.pszDefaultFile = "secureim_icons.dll";
			sid.iDefaultIndex = icons[i].key;
			sid.hDefaultIcon = (HICON)LoadImage(g_hIconInst, MAKEINTRESOURCE(icons[i].key), IMAGE_ICON, 16, 16, LR_SHARED);
			CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
			hIcon[i] = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)icons[i].name);
		}
		else {
			for (int j=i-1; j>=0; j--)
				if(icons[i].key==icons[j].key) {
					hIcon[i]=hIcon[j];
					break;
				}
			if(!hIcon[i]) {
				hIcon[i] = (HICON)LoadImage(g_hIconInst, MAKEINTRESOURCE(icons[i].key), IMAGE_ICON, 16, 16, LR_SHARED);
			}
		}
		if(icons[i].tbl == TBL_IEC)
			g_hIEC[icons[i].idx]=hIcon[i];
		else
		if(icons[i].tbl == TBL_ICO)
			g_hICO[icons[i].idx]=hIcon[i];
		else
		if(icons[i].tbl == TBL_POP)
			g_hPOP[icons[i].idx]=hIcon[i];
	}

	if(ServiceExists(MS_SKIN2_ADDICON)) {
		hIcoLibIconsChanged = HookEvent(ME_SKIN2_ICONSCHANGED, ReloadIcons);
	}
}

// EOF