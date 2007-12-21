#include "commonheaders.h"


HMODULE hgpg;
HRSRC	hRS_gpg;
PBYTE	pRS_gpg;

extern DLLEXPORT int   __cdecl _gpg_init(void);
extern DLLEXPORT int   __cdecl _gpg_done(void);
extern DLLEXPORT int   __cdecl _gpg_open_keyrings(LPSTR,LPSTR);
extern DLLEXPORT int   __cdecl _gpg_close_keyrings(void);
extern DLLEXPORT void  __cdecl _gpg_set_log(LPCSTR);
extern DLLEXPORT LPSTR __cdecl _gpg_get_error(void);
extern DLLEXPORT int   __cdecl _gpg_size_keyid(void);
extern DLLEXPORT int   __cdecl _gpg_select_keyid(HWND,LPSTR);
extern DLLEXPORT LPSTR __cdecl _gpg_encrypt(LPCSTR,LPCSTR);
extern DLLEXPORT LPSTR __cdecl _gpg_decrypt(LPCSTR);
extern DLLEXPORT LPSTR __cdecl _gpg_get_passphrases();
extern DLLEXPORT void  __cdecl _gpg_set_passphrases(LPCSTR);

int   __cdecl _gpg_init(void);
int   __cdecl _gpg_done(void);
int   __cdecl _gpg_open_keyrings(LPSTR,LPSTR);
int   __cdecl _gpg_close_keyrings(void);
void  __cdecl _gpg_set_log(LPCSTR);
LPSTR __cdecl _gpg_get_error(void);
int   __cdecl _gpg_size_keyid(void);
int   __cdecl _gpg_select_keyid(HWND,LPSTR);
LPSTR __cdecl _gpg_encrypt(LPCSTR,LPCSTR);
LPSTR __cdecl _gpg_decrypt(LPCSTR);
LPSTR __cdecl _gpg_get_passphrases();
void  __cdecl _gpg_set_passphrases(LPCSTR);

int   (__cdecl *p_gpg_init)(void);
int   (__cdecl *p_gpg_done)(void);
int   (__cdecl *p_gpg_open_keyrings)(LPSTR,LPSTR);
int   (__cdecl *p_gpg_close_keyrings)(void);
void  (__cdecl *p_gpg_set_log)(LPCSTR);
LPSTR (__cdecl *p_gpg_get_error)(void);
int   (__cdecl *p_gpg_size_keyid)(void);
int   (__cdecl *p_gpg_select_keyid)(HWND,LPSTR);
LPSTR (__cdecl *p_gpg_encrypt)(LPCSTR,LPCSTR);
LPSTR (__cdecl *p_gpg_decrypt)(LPCSTR);
LPSTR (__cdecl *p_gpg_get_passphrases)();
void  (__cdecl *p_gpg_set_passphrases)(LPCSTR);


#define GPA(x)                                              \
  {                                                         \
    *((PVOID*)&p##x) = (PVOID)GetProcAddress(mod, TEXT(#x)); \
    if (!p##x) {                                            \
      return 0;                                             \
    }                                                       \
  }

int load_gpg_dll(HMODULE mod) {

	GPA(_gpg_init);
	GPA(_gpg_done);
	GPA(_gpg_open_keyrings);
	GPA(_gpg_close_keyrings);
	GPA(_gpg_set_log);
	GPA(_gpg_get_error);
	GPA(_gpg_size_keyid);
	GPA(_gpg_select_keyid);
	GPA(_gpg_encrypt);
	GPA(_gpg_decrypt);
	GPA(_gpg_get_passphrases);
	GPA(_gpg_set_passphrases);

	return 1;
}

#undef GPA


#define GPA(x)                                              \
  {                                                         \
    *((PVOID*)&p##x) = (PVOID)MemGetProcAddress(mod, TEXT(#x)); \
    if (!p##x) {                                            \
      return 0;                                             \
    }                                                       \
  }

int load_gpg_mem(HMODULE mod) {

	GPA(_gpg_init);
	GPA(_gpg_done);
	GPA(_gpg_open_keyrings);
	GPA(_gpg_close_keyrings);
	GPA(_gpg_set_log);
	GPA(_gpg_get_error);
	GPA(_gpg_size_keyid);
	GPA(_gpg_select_keyid);
	GPA(_gpg_encrypt);
	GPA(_gpg_decrypt);
	GPA(_gpg_get_passphrases);
	GPA(_gpg_set_passphrases);

	return 1;
}

#undef GPA


int __cdecl gpg_init()
{
	int r; char t[MAX_PATH];
	if( isVista ){
		sprintf(t,"%s\\gnupgw.dll",TEMP);
		ExtractFile(t,666,1);
		hgpg = LoadLibraryA(t);
	}
	else {
		hRS_gpg = FindResource( g_hInst, MAKEINTRESOURCE(1), MAKEINTRESOURCE(666) );
		pRS_gpg = (PBYTE) LoadResource( g_hInst, hRS_gpg ); LockResource( pRS_gpg );
		hgpg = MemLoadLibrary( pRS_gpg );
	}
	if (hgpg) {
		if( isVista )	load_gpg_dll(hgpg);
		else			load_gpg_mem(hgpg);
		r = p_gpg_init();
		if(r) {
			return r;
		}
		if( isVista ){
			FreeLibrary(hgpg);
		}
		else {
			MemFreeLibrary(hgpg);
	    	UnlockResource( pRS_gpg );
			FreeResource( pRS_gpg );
		}
	}

    hgpg = 0;

	return 0;
}


int __cdecl gpg_done()
{
    int r = 0;
    if(hgpg) {
    	r = p_gpg_done();
		if( isVista ){
			FreeLibrary(hgpg);
		}
		else {
			MemFreeLibrary(hgpg);
		    UnlockResource( pRS_gpg );
			FreeResource( pRS_gpg );
		}
		hgpg = 0;
    }
	return r;
}


int __cdecl gpg_open_keyrings(LPSTR ExecPath, LPSTR HomePath)
{
	return p_gpg_open_keyrings(ExecPath, HomePath);
}


int __cdecl gpg_close_keyrings()
{
	return p_gpg_close_keyrings();
}


void __cdecl gpg_set_log(LPCSTR LogPath)
{
	p_gpg_set_log(LogPath);
}


LPSTR __cdecl gpg_get_error()
{
	return p_gpg_get_error();
}


LPSTR __cdecl gpg_encrypt(pCNTX ptr, LPCSTR szPlainMsg)
{
   	ptr->error = ERROR_NONE;
	pGPGDATA p = (pGPGDATA) ptr->pdata;
	SAFE_FREE(ptr->tmp);

	LPSTR szEncMsg;
	szEncMsg = p_gpg_encrypt(szPlainMsg,(LPCSTR)p->gpgKeyID);
	if(!szEncMsg) return 0;

	DWORD dwEncMsgLen = strlen(szEncMsg);

    ptr->tmp = (LPSTR) mir_alloc(dwEncMsgLen+1);
    memcpy(ptr->tmp, szEncMsg, dwEncMsgLen+1);
    LocalFree((LPVOID)szEncMsg);

    return ptr->tmp;
}


LPSTR __cdecl gpg_decrypt(pCNTX ptr, LPCSTR szEncMsg)
{
  	ptr->error = ERROR_NONE;
	SAFE_FREE(ptr->tmp);

    LPSTR szPlainMsg = p_gpg_decrypt(szEncMsg);
/*	if(!szPlainMsg) {
	    ptr = get_context_on_id(-1); // find private pgp keys
    	if(ptr && ptr->pgpKey)
			szPlainMsg = p_gpg_decrypt_key(szEncMsg,(LPCSTR)ptr->pgpKey);
		if(!szPlainMsg) return NULL;
    }*/

    DWORD dwPlainMsgLen = strlen(szPlainMsg);

    ptr->tmp = (LPSTR) mir_alloc(dwPlainMsgLen+1);
    memcpy(ptr->tmp, szPlainMsg, dwPlainMsgLen+1);
    LocalFree((LPVOID)szPlainMsg);

    return ptr->tmp;
}


LPSTR __cdecl gpg_encodeA(int context, LPCSTR szPlainMsg)
{
    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return NULL;
	cpp_alloc_pdata(ptr); pGPGDATA p = (pGPGDATA) ptr->pdata;
    if(!p->gpgKeyID) { ptr->error = ERROR_NO_GPG_KEY; return NULL; }

	// ansi message: convert to unicode->utf-8 and encrypt.
	LPSTR szUtfMsg;
	if( ptr->mode & MODE_GPG_ANSI || is_utf8_string(szPlainMsg) ) {
		szUtfMsg = mir_strdup(szPlainMsg);
	}
	else {
		int slen = strlen(szPlainMsg)+1;
		LPWSTR wstring = (LPWSTR) alloca(slen*sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, szPlainMsg, -1, wstring, slen*sizeof(WCHAR));
		szUtfMsg = utf8encode(wstring);
	}
	// encrypt
	LPSTR szNewMsg = gpg_encrypt(ptr, szUtfMsg);
	mir_free(szUtfMsg);

	return szNewMsg;
}


LPSTR __cdecl gpg_encodeW(int context, LPCWSTR szPlainMsg)
{
    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return NULL;
	cpp_alloc_pdata(ptr); pGPGDATA p = (pGPGDATA) ptr->pdata;
    if(!p->gpgKeyID) { ptr->error = ERROR_NO_GPG_KEY; return NULL; }

	// unicode message: convert to utf-8 and encrypt.
	LPSTR szUtfMsg;
	if( ptr->mode & MODE_GPG_ANSI ) {
		int wlen = wcslen(szPlainMsg)+1;
		szUtfMsg = (LPSTR) mir_alloc(wlen);
		WideCharToMultiByte(CP_ACP, 0, szPlainMsg, -1, szUtfMsg, wlen, 0, 0);
	}
	else {
		szUtfMsg = utf8encode(szPlainMsg);
	}
	LPSTR szNewMsg = gpg_encrypt(ptr, szUtfMsg);
	mir_free(szUtfMsg);

	return szNewMsg;
}


LPSTR __cdecl gpg_decode(int context, LPCSTR szEncMsg)
{
    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return NULL;

	LPSTR szNewMsg = NULL;
	LPSTR szOldMsg = gpg_decrypt(ptr, szEncMsg);

	if(szOldMsg) {
		if(	is_7bit_string(szOldMsg) || !is_utf8_string(szOldMsg) ) {
			// not UTF8 message
			int slen = strlen(szOldMsg)+1;
			szNewMsg = (LPSTR) mir_alloc(slen*(sizeof(WCHAR)+1));
			strcpy(szNewMsg,szOldMsg);
			WCHAR* wstring = (LPWSTR) alloca(slen*sizeof(WCHAR));
			MultiByteToWideChar(CP_ACP, 0, szOldMsg, -1, wstring, slen*sizeof(WCHAR));
			memcpy(szNewMsg+slen,wstring,slen*sizeof(WCHAR));
		}
		else {
			// utf8 message: convert to unicode -> ansii
			WCHAR *wstring = utf8decode(szOldMsg);
			int wlen = wcslen(wstring)+1;
			szNewMsg = (LPSTR) mir_alloc(wlen*(sizeof(WCHAR)+1));
			WideCharToMultiByte(CP_ACP, 0, wstring, -1, szNewMsg, wlen, 0, 0);
			memcpy(szNewMsg+wlen, wstring, wlen*sizeof(WCHAR));
			mir_free(wstring);
		}
	}
	SAFE_FREE(ptr->tmp);
	ptr->tmp = szNewMsg;
	return szNewMsg;
}


int __cdecl gpg_set_key(int context, LPCSTR RemoteKey)
{
/*    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return 0;
   	ptr->error = ERROR_NONE;

//   	if(!p_gpg_check_key(RemoteKey)) return 0;

   	SAFE_FREE(ptr->pgpKey);
	ptr->pgpKey = (BYTE *) mir_alloc(strlen(RemoteKey)+1);
	strcpy((LPSTR)ptr->pgpKey,RemoteKey);

   	return 1;
*/
	return 0;
}


int __cdecl gpg_set_keyid(int context, LPCSTR RemoteKeyID)
{
    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return 0;
	cpp_alloc_pdata(ptr); pGPGDATA p = (pGPGDATA) ptr->pdata;
   	ptr->error = ERROR_NONE;

   	SAFE_FREE(p->gpgKeyID);
	p->gpgKeyID = (BYTE*) mir_alloc(strlen(RemoteKeyID)+1);
	strcpy((char*)p->gpgKeyID,RemoteKeyID);

   	return 1;
}


int __cdecl gpg_size_keyid()
{
	return p_gpg_size_keyid();
}


int __cdecl gpg_select_keyid(HWND hDlg,LPSTR szKeyID)
{
	return p_gpg_select_keyid(hDlg,szKeyID);
}


LPSTR __cdecl gpg_get_passphrases()
{
	return p_gpg_get_passphrases();
}


void __cdecl gpg_set_passphrases(LPCSTR buffer)
{
	p_gpg_set_passphrases(buffer);
}


// EOF