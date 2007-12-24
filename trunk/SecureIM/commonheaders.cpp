#include "commonheaders.h"

HINSTANCE g_hInst, g_hIconInst;
PLUGINLINK *pluginLink;
MM_INTERFACE memoryManagerInterface;
MUUID interfaces[] = {MIID_SECUREIM, MIID_LAST};

const char *szModuleName = MODULENAME;
char TEMP[MAX_PATH];
int  TEMP_SIZE = 0;

HANDLE g_hEvent[2], g_hMenu[10], g_hService[14], g_hHook[13];
int iService=0, iHook=0;
HICON g_hIcon[ALL_CNT], g_hICO[ICO_CNT], g_hIEC[IEC_CNT], g_hPOP[POP_CNT];
IconExtraColumn g_IEC[IEC_CNT];
int iBmpDepth;
BOOL bCoreUnicode = false, bMetaContacts = false, bPopupExists = false, bPopupUnicode = false;
BOOL bPGPloaded = false, bPGPkeyrings = false, bUseKeyrings = false, bPGPprivkey = false;
BOOL bGPGloaded = false, bGPGkeyrings = false, bSavePass = false;
BOOL bSFT, bSOM, bASI, bMCD, bSCM, bDGP, bAIP;
BYTE bADV, bPGP, bGPG;
CRITICAL_SECTION localQueueMutex;
HANDLE hNetlibUser;


PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
		MODULENAME" (2in1)",
		__VERSION_DWORD,
		MODULENAME" plugin for Miranda IM",
		"Johell, Ghost, Nightwish, __alex, Baloo",
		"Johell@ifrance.com, baloo@bk.ru",
		"© 2003 Johell, © 2005-07 Baloo",
		"http://addons.miranda-im.org/details.php?action=viewfile&id=2445",
		0, 0
};


PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
		MODULENAME" (2in1)",
		__VERSION_DWORD,
		MODULENAME" plugin for Miranda IM",
		"Johell, Ghost, Nightwish, __alex, Baloo",
		"Johell@ifrance.com, baloo@bk.ru",
		"© 2003 Johell, © 2005-07 Baloo",
		"http://addons.miranda-im.org/details.php?action=viewfile&id=2445",
		0, 0,
		MIID_SECUREIM
};


char *DBGetString(HANDLE hContact,const char *szModule,const char *szSetting) {
	char *val=NULL;
	DBVARIANT dbv;
	dbv.type = DBVT_ASCIIZ;
	DBGetContactSetting(hContact,szModule,szSetting,&dbv);
	if( dbv.type==DBVT_ASCIIZ || dbv.type==DBVT_UTF8 || dbv.type==DBVT_WCHAR)
		val = mir_strdup(dbv.pszVal);
	DBFreeVariant(&dbv);
	return val;
}


char *DBGetStringDecode(HANDLE hContact,const char *szModule,const char *szSetting) {
	char *val = DBGetString(hContact,szModule,szSetting);
	if(!val) return NULL;
    int len = strlen(val)+64;
	char *buf = (LPSTR)mir_alloc(len);
	strcpy(buf,val); mir_free(val);
	CallService(MS_DB_CRYPT_DECODESTRING,(WPARAM)len,(LPARAM)buf);
	return buf;
}


int DBWriteStringEncode(HANDLE hContact,const char *szModule,const char *szSetting,const char *val) {
    int len = strlen(val)+64;
	char *buf = (LPSTR)mir_alloc(len);
	strcpy(buf,val);
	CallService(MS_DB_CRYPT_ENCODESTRING,(WPARAM)len,(LPARAM)buf);
	int ret = DBWriteContactSettingString(hContact,szModule,szSetting,buf);
	mir_free(buf);
	return ret;
}

/*
int DBWriteString(HANDLE hContact,const char *szModule,const char *szSetting,const char *val) {
	return DBWriteContactSettingString(hContact,szModule,szSetting,val);
}


int DBGetByte(HANDLE hContact,const char *szModule,const char *szSetting,int errorValue) {
	return DBGetContactSettingByte(hContact,szModule,szSetting,errorValue);
}


int DBWriteByte(HANDLE hContact,const char *szModule,const char *szSetting,BYTE val) {
	return DBWriteContactSettingByte(hContact,szModule,szSetting,val);
}


int DBGetWord(HANDLE hContact,const char *szModule,const char *szSetting,int errorValue) {
	return DBGetContactSettingWord(hContact,szModule,szSetting,errorValue);
}


int DBWriteWord(HANDLE hContact,const char *szModule,const char *szSetting,WORD val) {
	return DBWriteContactSettingWord(hContact,szModule,szSetting,val);
}
*/

void GetFlags() {
    bSFT = DBGetContactSettingByte(0,szModuleName,"sft",0);
    bSOM = DBGetContactSettingByte(0,szModuleName,"som",0);
    bASI = DBGetContactSettingByte(0,szModuleName,"asi",0);
    bMCD = DBGetContactSettingByte(0,szModuleName,"mcd",0);
    bSCM = DBGetContactSettingByte(0,szModuleName,"scm",0);
    bDGP = DBGetContactSettingByte(0,szModuleName,"dgp",0);
    bAIP = DBGetContactSettingByte(0,szModuleName,"aip",0);
    bADV = DBGetContactSettingByte(0,szModuleName,"adv",0);
}


void SetFlags() {
    DBWriteContactSettingByte(0,szModuleName,"sft",bSFT);
    DBWriteContactSettingByte(0,szModuleName,"som",bSOM);
    DBWriteContactSettingByte(0,szModuleName,"asi",bASI);
	DBWriteContactSettingByte(0,szModuleName,"mcd",bMCD);
    DBWriteContactSettingByte(0,szModuleName,"scm",bSCM);
    DBWriteContactSettingByte(0,szModuleName,"dgp",bDGP);
    DBWriteContactSettingByte(0,szModuleName,"aip",bAIP);
    DBWriteContactSettingByte(0,szModuleName,"adv",bADV);
}


/*-----------------------------------------------------*/

char* u2a( const wchar_t* src )
{
	int codepage = ServiceExists(MS_LANGPACK_GETCODEPAGE)?CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 ):CP_ACP;

	int cbLen = WideCharToMultiByte( codepage, 0, src, -1, NULL, 0, NULL, NULL );
	char* result = ( char* )mir_alloc( cbLen+1 );
	if ( result == NULL )
		return NULL;

	WideCharToMultiByte( codepage, 0, src, -1, result, cbLen, NULL, NULL );
	result[ cbLen ] = 0;
	return result;
}

wchar_t* a2u( const char* src )
{
	int codepage = ServiceExists(MS_LANGPACK_GETCODEPAGE)?CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 ):CP_ACP;

	int cbLen = MultiByteToWideChar( codepage, 0, src, -1, NULL, 0 );
	wchar_t* result = ( wchar_t* )mir_alloc( sizeof( wchar_t )*(cbLen+1));
	if ( result == NULL )
		return NULL;

	MultiByteToWideChar( codepage, 0, src, -1, result, cbLen );
	result[ cbLen ] = 0;
	return result;
}

void InitNetlib() {
	NETLIBUSER nl_user = {0};
	nl_user.cbSize = sizeof(nl_user);
	nl_user.szSettingsModule = (LPSTR)szModuleName;
	nl_user.flags = NUF_OUTGOING | NUF_HTTPCONNS;
	nl_user.szDescriptiveName = (LPSTR)szModuleName;

	hNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nl_user);
}

void DeinitNetlib() {
	if(hNetlibUser)
		CallService(MS_NETLIB_CLOSEHANDLE, (WPARAM)hNetlibUser, 0);
}

int Sent_NetLog(const char *fmt,...)
{
  va_list va;
  char szText[1024];

  va_start(va,fmt);
  mir_vsnprintf(szText,sizeof(szText),fmt,va);
  va_end(va);
  return CallService(MS_NETLIB_LOG,(WPARAM)hNetlibUser,(LPARAM)szText);
}

// EOF
