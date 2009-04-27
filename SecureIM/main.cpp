#include "commonheaders.h"

extern "C" BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID) {
	g_hInst = hInst;
	if (dwReason == DLL_PROCESS_ATTACH) {
		INITCOMMONCONTROLSEX icce = {
			sizeof(icce), ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES
		};
		InitCommonControlsEx(&icce);
	}
	return TRUE;
}


PLUGININFO *MirandaPluginInfo(DWORD mirandaVersion) {
	return &pluginInfo;
}


PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion) {
	return &pluginInfoEx;
}


MUUID* MirandaPluginInterfaces(void) {
	return interfaces;
}


long __cdecl Service_CreateIM(WPARAM wParam,LPARAM lParam){

	if (!CallService(MS_PROTO_ISPROTOONCONTACT, (WPARAM)wParam, (LPARAM)szModuleName))
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)wParam, (LPARAM)szModuleName);

//	WPARAM flags = 0;
//	HANDLE hMetaContact = getMetaContact((HANDLE)wParam);
//	if( hMetaContact ) {
//		wParam = (WPARAM)hMetaContact;
//		flags = PREF_METANODB;
//	}

	CallContactService((HANDLE)wParam,PSS_MESSAGE,PREF_METANODB,(LPARAM)SIG_INIT);
	return 1;
}


long __cdecl Service_DisableIM(WPARAM wParam,LPARAM lParam) {

//	WPARAM flags = 0;
//	HANDLE hMetaContact = getMetaContact((HANDLE)wParam);
//	if( hMetaContact ) {
//		wParam = (WPARAM)hMetaContact;
//		flags = PREF_METANODB;
//	}

	CallContactService((HANDLE)wParam,PSS_MESSAGE,PREF_METANODB,(LPARAM)SIG_DEIN);
	return 1;
}


long __cdecl Service_IsContactSecured(WPARAM wParam, LPARAM lParam) {

	return isContactSecured((HANDLE)wParam) || isContactPGP((HANDLE)wParam) || isContactGPG((HANDLE)wParam);
}


long __cdecl Service_Status(WPARAM wParam, LPARAM lParam) {

    switch(--lParam) {
    case 0:
    case 1:
    case 2:
		pUinKey ptr = getUinKey((HANDLE)wParam);
		if(ptr) {
			ptr->status=(BYTE)lParam;
			DBWriteContactSettingByte((HANDLE)wParam, szModuleName, "StatusID", (BYTE)lParam);
		}
		break;
    }

	return 1;
}


long __cdecl Service_Status0(WPARAM wParam, LPARAM lParam) {

	return Service_Status(wParam,1);
}


long __cdecl Service_Status1(WPARAM wParam, LPARAM lParam) {

	return Service_Status(wParam,2);
}


long __cdecl Service_Status2(WPARAM wParam, LPARAM lParam) {

	return Service_Status(wParam,3);
}


long __cdecl Service_PGPdelKey(WPARAM wParam, LPARAM lParam) {

	if(bPGPloaded) {
    	    DBDeleteContactSetting((HANDLE)wParam, szModuleName, "pgp");
    	    DBDeleteContactSetting((HANDLE)wParam, szModuleName, "pgp_mode");
    	    DBDeleteContactSetting((HANDLE)wParam, szModuleName, "pgp_abbr");
	}
	{
	    pUinKey ptr = getUinKey((HANDLE)wParam);
	    cpp_delete_context(ptr->cntx); ptr->cntx=0;
	}
	ShowStatusIconNotify((HANDLE)wParam);
	return 1;
}


long __cdecl Service_GPGdelKey(WPARAM wParam, LPARAM lParam) {

	if(bGPGloaded) {
    	    DBDeleteContactSetting((HANDLE)wParam, szModuleName, "gpg");
	}
	{
	    pUinKey ptr = getUinKey((HANDLE)wParam);
	    cpp_delete_context(ptr->cntx); ptr->cntx=0;
	}
	ShowStatusIconNotify((HANDLE)wParam);
	return 1;
}


long __cdecl Service_PGPsetKey(WPARAM wParam, LPARAM lParam) {

	BOOL del = true;
	if(bPGPloaded) {
    	if(bPGPkeyrings) {
	    char szKeyID[128] = {0};
    	    PVOID KeyID = pgp_select_keyid(GetForegroundWindow(),szKeyID);
	    if(szKeyID[0]) {
    		DBDeleteContactSetting((HANDLE)wParam,szModuleName,"pgp");
    		DBCONTACTWRITESETTING cws;
    		cws.szModule = szModuleName;
    		cws.szSetting = "pgp";
    		cws.value.type = DBVT_BLOB;
    		cws.value.pbVal = (LPBYTE)KeyID;
    		cws.value.cpbVal = pgp_size_keyid();
    		CallService(MS_DB_CONTACT_WRITESETTING,wParam,(LPARAM)&cws);
    		DBWriteContactSettingByte((HANDLE)wParam,szModuleName,"pgp_mode",0);
    		DBWriteContactSettingString((HANDLE)wParam,szModuleName,"pgp_abbr",szKeyID);
    	  	del = false;
	    }
    	}
    	else
    	if(bPGPprivkey) {
    		char KeyPath[MAX_PATH] = {0};
    	  	if(ShowSelectKeyDlg(0,KeyPath)){
    	  		char *publ = LoadKeys(KeyPath,false);
    	  		if(publ) {
    				DBDeleteContactSetting((HANDLE)wParam,szModuleName,"pgp");
    		  		DBWriteStringEncode((HANDLE)wParam,szModuleName,"pgp",publ);
    				DBWriteContactSettingByte((HANDLE)wParam,szModuleName,"pgp_mode",1);
    				DBWriteContactSettingString((HANDLE)wParam,szModuleName,"pgp_abbr","(binary)");
    		  		mir_free(publ);
    		  		del = false;
    	  		}
    		}
    	}
	}

	if(del) Service_PGPdelKey(wParam,lParam);
	else {
		pUinKey ptr = getUinKey((HANDLE)wParam);
		cpp_delete_context(ptr->cntx); ptr->cntx=0;
	}
	ShowStatusIconNotify((HANDLE)wParam);
	return 1;
}


long __cdecl Service_GPGsetKey(WPARAM wParam, LPARAM lParam) {

	BOOL del = true;
	if(bGPGloaded && bGPGkeyrings) {
   		char szKeyID[128] = {0};
   	    gpg_select_keyid(GetForegroundWindow(),szKeyID);
   		if(szKeyID[0]) {
   		    DBWriteContactSettingString((HANDLE)wParam,szModuleName,"gpg",szKeyID);
   	  		del = false;
   		}
	}

	if(del) Service_GPGdelKey(wParam,lParam);
	else {
		pUinKey ptr = getUinKey((HANDLE)wParam);
		cpp_delete_context(ptr->cntx); ptr->cntx=0;
	}
	ShowStatusIconNotify((HANDLE)wParam);
	return 1;
}


int onWindowEvent(WPARAM wParam, LPARAM lParam) {

	MessageWindowEventData *mwd = (MessageWindowEventData *)lParam;
	if(mwd->uType == MSG_WINDOW_EVT_OPEN || mwd->uType == MSG_WINDOW_EVT_OPENING) {
		ShowStatusIcon(mwd->hContact);
	}
	return 0;
}


int onIconPressed(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact = (HANDLE)wParam;
	if( isProtoMetaContacts(hContact) )
		hContact = getMostOnline(hContact); // возьмем тот, через который пойдет сообщение

	StatusIconClickData *sicd = (StatusIconClickData *)lParam;
	if( strcmp(sicd->szModule, szModuleName) != 0 ||
		!isSecureProtocol(hContact) ) return 0; // not our event

	BOOL isPGP = isContactPGP(hContact);
	BOOL isGPG = isContactGPG(hContact);
	BOOL isSecured = isContactSecured(hContact)&SECURED;
	BOOL isChat = isChatRoom(hContact);

	if( !isPGP && !isGPG && !isChat ) {
		if(isSecured)	Service_DisableIM(wParam,0);
		else		Service_CreateIM(wParam,0);
	}

	return 0;
}


int Load(PLUGINLINK *link) {

	pluginLink = link;
	DisableThreadLibraryCalls(g_hInst);
	InitializeCriticalSection(&localQueueMutex);

	{
		char temp[MAX_PATH];
		GetTempPath(sizeof(temp),temp);
		GetLongPathName(temp,TEMP,sizeof(TEMP));
		TEMP_SIZE = strlen(TEMP);
		if(TEMP[TEMP_SIZE-1]=='\\') {
			TEMP_SIZE--;
			TEMP[TEMP_SIZE]='\0';
		}
	}

	// get memoryManagerInterface address
	mir_getMMI( &mmi );

	// check for support TrueColor Icons
	BOOL bIsComCtl6 = FALSE;
	HMODULE hComCtlDll = LoadLibrary("comctl32.dll");
	if ( hComCtlDll ) {
		typedef HRESULT (CALLBACK *PFNDLLGETVERSION)(DLLVERSIONINFO*);
		PFNDLLGETVERSION pfnDllGetVersion = (PFNDLLGETVERSION) GetProcAddress(hComCtlDll,"DllGetVersion");
		if ( pfnDllGetVersion ) {
			DLLVERSIONINFO dvi = {0};
			dvi.cbSize = sizeof(dvi);
			HRESULT hRes = (*pfnDllGetVersion)( &dvi );
			if ( SUCCEEDED(hRes) && dvi.dwMajorVersion >= 6 ) {
				bIsComCtl6 = TRUE;
			}
		}
		FreeLibrary(hComCtlDll);
	}
	if (bIsComCtl6)	iBmpDepth = ILC_COLOR32 | ILC_MASK;  // 32-bit images are supported
	else		iBmpDepth = ILC_COLOR24 | ILC_MASK;

//	iBmpDepth = ILC_COLOR32 | ILC_MASK;

	char version[512];
	CallService(MS_SYSTEM_GETVERSIONTEXT, sizeof(version), (LPARAM)&version);
	bCoreUnicode = strstr(version, "Unicode")!=0;
	iCoreVersion = CallService(MS_SYSTEM_GETVERSION,0,0);

	// load crypo++ dll
	if( !loadlib() ) {
		msgbox1(0,sim107,szModuleName,MB_OK|MB_ICONSTOP);
		return 1;
	}

	load_rtfconv();

	// register plugin module
	PROTOCOLDESCRIPTOR pd = {0};
	pd.cbSize = sizeof(pd);
	pd.szName = (char*)szModuleName;
	pd.type = PROTOTYPE_ENCRYPTION;
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM)&pd);

	// hook events
	g_hHook[iHook++] = HookEvent(ME_SYSTEM_MODULESLOADED,onModulesLoaded);
	g_hHook[iHook++] = HookEvent(ME_SYSTEM_OKTOEXIT,onSystemOKToExit); 

/*	g_hEvent[0] = CreateHookableEvent(MODULENAME"/Disabled");
	g_hEvent[1] = CreateHookableEvent(MODULENAME"/Established");*/

	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/IsContactSecured",(MIRANDASERVICE)Service_IsContactSecured);
	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/SIM_EST",(MIRANDASERVICE)Service_CreateIM);
	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/SIM_DIS",(MIRANDASERVICE)Service_DisableIM);
	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/SIM_ST_DIS",(MIRANDASERVICE)Service_Status0);
	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/SIM_ST_ENA",(MIRANDASERVICE)Service_Status1);
	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/SIM_ST_TRY",(MIRANDASERVICE)Service_Status2);
	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/SIM_PGP_SET",(MIRANDASERVICE)Service_PGPsetKey);
	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/SIM_PGP_DEL",(MIRANDASERVICE)Service_PGPdelKey);
	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/SIM_GPG_SET",(MIRANDASERVICE)Service_GPGsetKey);
	g_hService[iService++] = CreateServiceFunction((LPCSTR)MODULENAME"/SIM_GPG_DEL",(MIRANDASERVICE)Service_GPGdelKey);

	return 0;
}


HANDLE AddMenuItem(LPCSTR name,int pos,HICON hicon,LPCSTR service,int flags=0,WPARAM wParam=0) {

	CLISTMENUITEM mi={0};
	mi.cbSize=sizeof(mi);
	mi.flags=flags | CMIF_HIDDEN;
	mi.position=pos;
	mi.hIcon=hicon;
	mi.pszName=Translate(name);
	mi.pszPopupName=(char*)-1;
	mi.pszService=(char*)service;
	return((HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,wParam,(LPARAM)&mi));
}


HANDLE AddSubItem(HANDLE rootid,LPCSTR name,int pos,int poppos,LPCSTR service,WPARAM wParam=0) {

    CLISTMENUITEM mi={0};
    mi.cbSize=sizeof(mi);
    mi.flags=CMIF_CHILDPOPUP | CMIF_HIDDEN;
    mi.position=pos;
    mi.popupPosition=poppos;
    mi.hIcon=NULL;
    mi.pszName=Translate(name);
    mi.pszPopupName=(char*)rootid;
    mi.pszService=(char*)service;
    return((HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,wParam,(LPARAM)&mi));
}


int onModulesLoaded(WPARAM wParam,LPARAM lParam) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
    InitNetlib();
#endif
    bMetaContacts = ServiceExists(MS_MC_GETMETACONTACT)!=0;
    bPopupExists = ServiceExists(MS_POPUP_ADDPOPUPEX)!=0;
    bPopupUnicode = ServiceExists(MS_POPUP_ADDPOPUPW)!=0;

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("onModuleLoaded start");
#endif

    InitIcons();
    GetFlags();

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_init");
#endif
	{ // RSA/AES
		rsa_init(&exp,&imp);

		DBVARIANT dbv1, dbv2;
		dbv1.type = dbv2.type = DBVT_BLOB;

		if( DBGetContactSetting(0,szModuleName,"rsa_priv_2048",&dbv1) == 0 ) {
			if( DBGetContactSetting(0,szModuleName,"rsa_pub_2048",&dbv2) == 0 ) {
				exp->rsa_set_keypair(CPP_MODE_RSA_2048,dbv1.pbVal,dbv1.cpbVal,dbv2.pbVal,dbv2.cpbVal);
				rsa_2048=1;
				DBFreeVariant(&dbv2);
			}
			DBFreeVariant(&dbv1);
		}
		if( DBGetContactSetting(0,szModuleName,"rsa_priv_4096",&dbv1) == 0 ) {
			if( DBGetContactSetting(0,szModuleName,"rsa_pub_4096",&dbv2) == 0 ) {
				exp->rsa_set_keypair(CPP_MODE_RSA_4096,dbv1.pbVal,dbv1.cpbVal,dbv2.pbVal,dbv2.cpbVal);
				rsa_4096=1;
				DBFreeVariant(&dbv2);
			}
			DBFreeVariant(&dbv1);
		}
		if( !rsa_2048 || !rsa_4096 ) {
			forkthread(sttGenerateRSA,0,0);
		}
	}

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("pgp_init");
#endif
	bPGP = DBGetContactSettingByte(0, szModuleName, "pgp", 0);
	if(bPGP) { //PGP
	    bPGPloaded = pgp_init();
   	    bUseKeyrings = DBGetContactSettingByte(0,szModuleName,"ukr",1);
   	    LPSTR priv = DBGetStringDecode(0,szModuleName,"pgpPrivKey");
   	    if(priv) {
	   	    bPGPprivkey = true;
		    if(bPGPloaded)
			pgp_set_key(-1,priv);
	   	    mir_free(priv);
	    }// if(priv)
            if(bPGPloaded && bUseKeyrings) {
    		char PubRingPath[MAX_PATH] = {0}, SecRingPath[MAX_PATH] = {0};
    		if(pgp_get_version()<0x02000000) { // 6xx
    		    bPGPkeyrings = pgp_open_keyrings(PubRingPath,SecRingPath);
		}
        	else {
        		LPSTR tmp;
        		tmp = DBGetString(0,szModuleName,"pgpPubRing");
        		if(tmp) {
        			memcpy(PubRingPath,tmp,strlen(tmp));
        			mir_free(tmp);
        		}
        		tmp = DBGetString(0,szModuleName,"pgpSecRing");
        		if(tmp) {
        			memcpy(SecRingPath,tmp,strlen(tmp));
        			mir_free(tmp);
        		}
        	   	if(PubRingPath[0] && SecRingPath[0]) {
        			bPGPkeyrings = pgp_open_keyrings(PubRingPath,SecRingPath);
        			if(bPGPkeyrings) {
        				DBWriteContactSettingString(0,szModuleName,"pgpPubRing",PubRingPath);
        				DBWriteContactSettingString(0,szModuleName,"pgpSecRing",SecRingPath);
        			}
        			else {
        				DBDeleteContactSetting(0, szModuleName, "pgpPubRing");
        				DBDeleteContactSetting(0, szModuleName, "pgpSecRing");
        			}
        		}
    		}
    	    }// if(bPGPloaded && bUseKeyrings)
   	}// if(bPGP)

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("gpg_init");
#endif
	bGPG = DBGetContactSettingByte(0, szModuleName, "gpg", 0);
	if(bGPG) { //GPG

		LPSTR tmp;

		bGPGloaded = gpg_init();

   		char gpgexec[MAX_PATH] = {0}, gpghome[MAX_PATH] = {0};

		tmp = DBGetString(0,szModuleName,"gpgExec");
		if(tmp) {
			memcpy(gpgexec,tmp,strlen(tmp));
			mir_free(tmp);
		}
		tmp = DBGetString(0,szModuleName,"gpgHome");
		if(tmp) {
			memcpy(gpghome,tmp,strlen(tmp));
			mir_free(tmp);
		}

		if(DBGetContactSettingByte(0, szModuleName, "gpgLogFlag",0)) {
			tmp = DBGetString(0,szModuleName,"gpgLog");
			if(tmp) {
				gpg_set_log(tmp);
				mir_free(tmp);
			}
		}

		bGPGkeyrings = gpg_open_keyrings(gpgexec,gpghome);
		if(bGPGkeyrings) {
			DBWriteContactSettingString(0,szModuleName,"gpgExec",gpgexec);
			DBWriteContactSettingString(0,szModuleName,"gpgHome",gpghome);
		}
		else {
			DBDeleteContactSetting(0, szModuleName, "gpgExec");
			DBDeleteContactSetting(0, szModuleName, "gpgHome");
		}

	    bSavePass = DBGetContactSettingByte(0,szModuleName,"gpgSaveFlag",0);
	    if(bSavePass) {
			tmp = DBGetString(0,szModuleName,"gpgSave");
			if(tmp) {
				gpg_set_passphrases(tmp);
				mir_free(tmp);
			}
	    }
	}

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("loadContactList");
#endif
	loadContactList();

	// add new skin sound
	SkinAddNewSound("IncomingSecureMessage","Incoming Secure Message","Sounds\\iSecureMessage.wav");
	SkinAddNewSound("OutgoingSecureMessage","Outgoing Secure Message","Sounds\\oSecureMessage.wav");

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("init extra icons");
#endif
	// init extra icons
	for(int i=0;i<1+MODE_CNT*IEC_CNT;i++) {
		g_IEC[i].cbSize = sizeof(g_IEC[i]);
		g_IEC[i].ColumnType = bADV;
		g_IEC[i].hImage = (HANDLE)-1;
	}

	// build extra imagelist
	//onExtraImageListRebuilding(0,0);

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("hook events");
#endif
	g_hHook[iHook++] = HookEvent(ME_CLIST_PREBUILDCONTACTMENU, onRebuildContactMenu);
//	g_hMC = HookEvent(ME_MC_SUBCONTACTSCHANGED, onMC);

	if( ServiceExists(MS_EXTRAICON_REGISTER) ) {
		g_hCLIcon = ExtraIcon_Register(szModuleName, Translate("SecureIM status"), "sim_cm_est",
						(MIRANDAHOOK)onExtraImageListRebuilding,
						(MIRANDAHOOK)onExtraImageApplying);
	}
	else {
		g_hHook[iHook++] = HookEvent(ME_CLIST_EXTRA_LIST_REBUILD, onExtraImageListRebuilding);
		g_hHook[iHook++] = HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY, onExtraImageApplying);
	}

	// hook init options
	g_hHook[iHook++] = HookEvent(ME_OPT_INITIALISE, onRegisterOptions);
	if(bPopupExists)
	g_hHook[iHook++] = HookEvent(ME_OPT_INITIALISE, onRegisterPopOptions);
	g_hHook[iHook++] = HookEvent(ME_PROTO_ACK, onProtoAck);
	g_hHook[iHook++] = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, onContactSettingChanged);
	g_hHook[iHook++] = HookEvent(ME_DB_CONTACT_ADDED, onContactAdded);
	g_hHook[iHook++] = HookEvent(ME_DB_CONTACT_DELETED, onContactDeleted);

	// hook message transport
	g_hService[iService++] = CreateProtoServiceFunction(szModuleName, PSR_MESSAGE, (MIRANDASERVICE)onRecvMsg);
	g_hService[iService++] = CreateProtoServiceFunction(szModuleName, PSS_MESSAGE, (MIRANDASERVICE)onSendMsg);
	g_hService[iService++] = CreateProtoServiceFunction(szModuleName, PSS_MESSAGE"W", (MIRANDASERVICE)onSendMsgW);
	g_hService[iService++] = CreateProtoServiceFunction(szModuleName, PSS_FILE, (MIRANDASERVICE)onSendFile);

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("create Native/RSA menu");
#endif
	// create a menu item for creating a secure im connection to the user.
	g_hMenu[0] = AddMenuItem(sim301,110000,g_hICO[ICO_CM_EST],MODULENAME"/SIM_EST",CMIF_NOTOFFLINE);
	g_hMenu[1] = AddMenuItem(sim302,110001,g_hICO[ICO_CM_DIS],MODULENAME"/SIM_DIS",CMIF_NOTOFFLINE);

	if(ServiceExists(MS_CLIST_ADDSUBGROUPMENUITEM)) {
	    g_hMenu[2] = AddMenuItem(sim303,110002,NULL,NULL,CMIF_ROOTPOPUP);
	    g_hMenu[3] = AddSubItem(g_hMenu[2],sim232[0],110003,110002,MODULENAME"/SIM_ST_DIS");
	    g_hMenu[4] = AddSubItem(g_hMenu[2],sim232[1],110004,110002,MODULENAME"/SIM_ST_ENA");
	    g_hMenu[5] = AddSubItem(g_hMenu[2],sim232[2],110005,110002,MODULENAME"/SIM_ST_TRY");
	}
	else {
	    g_hMenu[2] = 0;
	    g_hMenu[3] = AddMenuItem(sim232[0],110003,NULL,MODULENAME"/SIM_ST_DIS");
	    g_hMenu[4] = AddMenuItem(sim232[1],110004,NULL,MODULENAME"/SIM_ST_ENA");
	    g_hMenu[5] = AddMenuItem(sim232[2],110005,NULL,MODULENAME"/SIM_ST_TRY");
	}

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("create PGP/GPG menu");
#endif
	HICON icon;
	if( bPGPloaded ) {
		icon=mode2icon(MODE_PGP|SECURED,2);
		g_hMenu[6] = AddMenuItem(sim306,110006,icon,MODULENAME"/SIM_PGP_SET",0);
		icon=mode2icon(MODE_PGP,2);
		g_hMenu[7] = AddMenuItem(sim307,110007,icon,MODULENAME"/SIM_PGP_DEL",0);
	}

    	if(bGPGloaded) {
		icon=mode2icon(MODE_GPG|SECURED,2);
		g_hMenu[8] = AddMenuItem(sim308,110008,icon,MODULENAME"/SIM_GPG_SET",0);
		icon=mode2icon(MODE_GPG,2);
		g_hMenu[9] = AddMenuItem(sim309,110009,icon,MODULENAME"/SIM_GPG_DEL",0);
    	}


    	// updater plugin support
        if(ServiceExists(MS_UPDATE_REGISTERFL)) {
		CallService(MS_UPDATE_REGISTERFL, (WPARAM)2445, (LPARAM)&pluginInfo);
	}

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("create srmm icons");
#endif
	// add icon to srmm status icons
	if(ServiceExists(MS_MSG_ADDICON)) {

		StatusIconData sid = {0};
		sid.cbSize = sizeof(sid);
		sid.szModule = (char*)szModuleName;
		sid.flags = MBF_DISABLED|MBF_HIDDEN;
		// Native
		sid.dwId = MODE_NATIVE;
		sid.hIcon = mode2icon(MODE_NATIVE|SECURED,3);
		sid.hIconDisabled = mode2icon(MODE_NATIVE,3);
		sid.szTooltip = Translate("SecureIM [Native]");
		CallService(MS_MSG_ADDICON, 0, (LPARAM)&sid);
		// PGP
		sid.dwId = MODE_PGP;
		sid.hIcon = mode2icon(MODE_PGP|SECURED,3);
		sid.hIconDisabled = mode2icon(MODE_PGP,3);
		sid.szTooltip = Translate("SecureIM [PGP]");
		CallService(MS_MSG_ADDICON, 0, (LPARAM)&sid);
		// GPG
		sid.dwId = MODE_GPG;
		sid.hIcon = mode2icon(MODE_GPG|SECURED,3);
		sid.hIconDisabled = mode2icon(MODE_GPG,3);
		sid.szTooltip = Translate("SecureIM [GPG]");
		CallService(MS_MSG_ADDICON, 0, (LPARAM)&sid);
		// RSAAES
		sid.dwId = MODE_RSAAES;
		sid.hIcon = mode2icon(MODE_RSAAES|SECURED,3);
		sid.hIconDisabled = mode2icon(MODE_RSAAES,3);
		sid.szTooltip = Translate("SecureIM [RSA/AES]");
		CallService(MS_MSG_ADDICON, 0, (LPARAM)&sid);

		// hook the window events so that we can can change the status of the icon
		g_hHook[iHook++] = HookEvent(ME_MSG_WINDOWEVENT, onWindowEvent);
		g_hHook[iHook++] = HookEvent(ME_MSG_ICONPRESSED, onIconPressed);
	}

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("onModuleLoaded stop");
#endif
	return 0;
}


int onSystemOKToExit(WPARAM wParam,LPARAM lParam) {

    if(bSavePass) {
	LPSTR tmp = gpg_get_passphrases();
	DBWriteContactSettingString(0,szModuleName,"gpgSave",tmp);
	LocalFree(tmp);
    }
    else {
	DBDeleteContactSetting(0,szModuleName,"gpgSave");
    }
	if(bPGPloaded) pgp_done();
	if(bGPGloaded) gpg_done();
	rsa_done();
	while(iHook--) UnhookEvent(g_hHook[iHook]);
	while(iService--) DestroyServiceFunction(g_hService[iService]);
/*	DestroyHookableEvent(g_hEvent[0]);
	DestroyHookableEvent(g_hEvent[1]);*/
	freeContactList();
	free_rtfconv();
#if defined(_DEBUG) || defined(NETLIB_LOG)
	DeinitNetlib();
#endif
	return 0;
}

int Unload() {
	DeleteCriticalSection(&localQueueMutex);
	return 0;
}

// EOF
