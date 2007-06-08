#include "commonheaders.h"

LPCSTR szModuleName = MODULENAME;
LPCSTR szVersionStr = MODULENAME" DLL ("__VERSION_STRING")";
HINSTANCE g_hInst;
PLUGINLINK *pluginLink;
MM_INTERFACE memoryManagerInterface={0};
MUUID interfaces[] = {MIID_CRYPTOPP, MIID_LAST};

char TEMP[MAX_PATH];
int  TEMP_SIZE = 0;
BOOL isVista = 0;

PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
		MODULENAME,
		__VERSION_DWORD,
		MODULENAME" library for SecureIM plugin",
		"Baloo",
		"baloo@bk.ru",
		"© 2006-07 Baloo",
		"http://miranda-im.org/download/details.php?action=viewfile&id=2669",
		0, 0
};

PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
		MODULENAME,
		__VERSION_DWORD,
		MODULENAME" library for SecureIM plugin",
		"Baloo",
		"baloo@bk.ru",
		"© 2006-07 Baloo",
		"http://miranda-im.org/download/details.php?action=viewfile&id=2669",
		0, 0,	
		MIID_CRYPTOPP
};


BOOL ExtractFileFromResource( HANDLE FH, int ResType, int ResId, DWORD* Size )
{
    HRSRC	RH;
    PBYTE	RP;
    DWORD	s,x;

    RH = FindResource( g_hInst, MAKEINTRESOURCE( ResId ), MAKEINTRESOURCE( ResType ) );

    if( RH == NULL ) return FALSE;
    RP = (PBYTE) LoadResource( g_hInst, RH );
    if( RP == NULL ) return FALSE;
    s = SizeofResource( g_hInst, RH );
    if( !WriteFile( FH, RP, s, &x, NULL ) ) return FALSE;
    if( x != s ) return FALSE;
    if( Size ) *Size = s;
    return TRUE;
}


void ExtractFile( char *FileName, int ResType, int ResId )
{
    HANDLE FH;
    FH = CreateFile( FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    if( FH == INVALID_HANDLE_VALUE ) return;
    if (!ExtractFileFromResource( FH, ResType, ResId, NULL )) MessageBoxA(0,"Can't extract","!!!",MB_OK);
    CloseHandle( FH );
}

