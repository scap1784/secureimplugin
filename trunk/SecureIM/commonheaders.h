// Windows API

#define STRICT
#define WIN32_LEAN_AND_MEAN
//#define NETLIB_LOG

#ifdef _MSC_VER
#if _MSC_VER >= 1300 && !defined _DEBUG
//#pragma comment (compiler,"/GS-")
//#pragma comment (linker,"/NODEFAULTLIB:libcmt.lib") 
//#pragma comment (lib,"../../lib/msvcrt.lib")
//#pragma comment (lib,"../../lib/msvcrt71.lib")
#else
#pragma optimize("gsy", on)
#endif 
#endif 

#ifndef M_SIM_COMMONHEADERS_H
#define M_SIM_COMMONHEADERS_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500 
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif

// Windows API
#ifdef _MSC_VER
#if _MSC_VER > 1000
#include <windows.h>
#else
#include <afxwin.h>
#endif
#else
#include <windows.h>
#endif
#include <winsock2.h>
#include <commdlg.h>
#include <commctrl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlwapi.h>
#include <process.h>
#include <time.h>

#ifndef ListView_SetCheckState
#define ListView_SetCheckState(hwndLV, i, fCheck) \
  ListView_SetItemState(hwndLV, i, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), LVIS_STATEIMAGEMASK)
#endif

#ifndef SIZEOF
#define SIZEOF(X) (sizeof(X)/sizeof(X[0]))
#endif

#ifndef M_API_H__
#define M_API_H__

// Miranda API
#include "newpluginapi.h"
#include "m_stdhdr.h"
#include "m_plugins.h"
#include "m_system.h"
#include "m_database.h"
#include "m_protomod.h"
#include "m_protosvc.h"
#include "m_langpack.h"
#include "m_clist.h"
#include "m_options.h"
#include "m_clui.h"
#include "m_clc.h"
#include "m_utils.h"
#include "m_skin.h"
#include "m_popup.h"
#include "m_cluiframes.h"
#include "m_metacontacts.h"
#include "m_updater.h"
#include "m_genmenu.h"
#include "m_icolib.h"
#include "m_message.h"
#include "m_netlib.h"

#endif

// my libs
#include "secureim.h"
#include "version.h"
#include "resource.h"
#include "language.h"
#include "loadlib.h"
#include "mmi.h"
#include "crypt.h"
#include "gettime.h"
#include "language.h"
#include "options.h"
#include "popupoptions.h"
#include "loadicons.h"
#include "forkthread.h"
#include "rtfconv.h"
#include "cryptopp.h"

#define MODULENAME "SecureIM"

extern LPCSTR szModuleName;
extern LPCSTR szVersionStr;
extern char TEMP[MAX_PATH];
extern int  TEMP_SIZE;

// shared vars
extern HINSTANCE g_hInst, g_hIconInst;
extern PLUGINLINK *pluginLink;
extern PLUGININFO pluginInfo;
extern PLUGININFOEX pluginInfoEx;
extern MM_INTERFACE memoryManagerInterface;
extern MUUID interfaces[];

#define MIID_SECUREIM	{0x1B2A39E5, 0xE2F6, 0x494D, { 0x95, 0x8D, 0x18, 0x08, 0xFD, 0x11, 0x0D, 0xD5 }} //1B2A39E5-E2F6-494D-958D-1808FD110DD5

#define PREF_METANODB	0x2000	//!< Flag to indicate message should not be added to db by filter when sending
#define PREF_SIMNOMETA	0x4000	//!< Flag to indicate message should not be inspected by filter on metacontact

#define DLLEXPORT __declspec(dllexport)

extern "C" {

 DLLEXPORT int Load(PLUGINLINK *);
 DLLEXPORT PLUGININFO *MirandaPluginInfo(DWORD);
 DLLEXPORT PLUGININFOEX *MirandaPluginInfoEx(DWORD);
 DLLEXPORT MUUID* MirandaPluginInterfaces(void);
 DLLEXPORT int Unload();

}

extern HANDLE g_hEvent[2], g_hMenu[10], g_hService[14], g_hHook[17];
extern int iService, iHook;
extern HICON g_hIcon[ALL_CNT], g_hICO[ICO_CNT], g_hIEC[IEC_CNT], g_hPOP[POP_CNT];
extern IconExtraColumn g_IEC[IEC_CNT];
extern int iBmpDepth;
extern BOOL bCoreUnicode, bMetaContacts, bPopupExists, bPopupUnicode;
extern BOOL bPGPloaded, bPGPkeyrings, bUseKeyrings, bPGPprivkey;
extern BOOL bGPGloaded, bGPGkeyrings, bSavePass;
extern BOOL bSFT, bSOM, bASI, bMCD, bSCM, bDGP, bAIP, bNOL, bAAK;
extern BYTE bADV, bPGP, bGPG;
extern DWORD iCoreVersion;
extern CRITICAL_SECTION localQueueMutex;

int onModulesLoaded(WPARAM,LPARAM);
int onSystemOKToExit(WPARAM,LPARAM);

char *simDBGetString(HANDLE,const char *,const char *);
char *simDBGetStringDecode(HANDLE,const char *,const char *);
int simDBWriteStringEncode(HANDLE,const char *,const char *,const char *);

#if defined(_DEBUG) || defined(NETLIB_LOG)
extern HANDLE hNetlibUser;
void InitNetlib();
void DeinitNetlib();
int Sent_NetLog(const char *,...);
#endif
/*
int DBWriteString(HANDLE,const char *,const char *,const char *);
int DBGetByte(HANDLE,const char *,const char *,int);
int DBWriteByte(HANDLE,const char *,const char *,BYTE);
int DBGetWord(HANDLE,const char *,const char *,int);
int DBWriteWord(HANDLE,const char *,const char *,WORD);
*/
void GetFlags();
void SetFlags();

LPSTR u2a( LPCWSTR src );
LPWSTR a2u( LPCSTR src );
LPSTR TranslateU( LPCSTR lpText );
int msgbox( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType );
#define msgbox0(a,b,c,d) msgbox(a,b,c,d)
#define msgbox1(a,b,c,d) msgbox(a,b,c,d)

#endif
