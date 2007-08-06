#include "commonheaders.h"

/*
static int CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_COMMAND:
			if (wParam == STN_CLICKED) { // It was a click on the Popup.
				PUDeletePopUp(hWnd);
				return TRUE;
			}
			break;
		case UM_FREEPLUGINDATA: {
			return TRUE; //TRUE or FALSE is the same, it gets ignored.
			}
		default:
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
*/

void showPopUp(LPCSTR lpzText,HANDLE hContact,HICON hIcon, UINT type) {
	//type=0 key colors
	//type=1 session colors
	//type=2 SR colors

	if(!bPopupExists) return;

	//hContact = A_VALID_HANDLE_YOU_GOT_FROM_SOMEWHERE;
	COLORREF colorBackKey = RGB(230,230,255);
	COLORREF colorTextKey = RGB(0,0,0);
	COLORREF colorBackSec = RGB(255,255,200);
	COLORREF colorTextSec = RGB(0,0,0);
	COLORREF colorBackSR = RGB(200,255,200);
	COLORREF colorTextSR = RGB(0,0,0);
	COLORREF colorBack = 0;
	COLORREF colorText = 0;
	int timeout=0;
	int res;

	DBVARIANT dbv_timeout;

	if (type==0) {
		colorBack=DBGetContactSettingDword(0,szModuleName,"colorKeyb",(UINT)-1);
		colorText=DBGetContactSettingDword(0,szModuleName,"colorKeyt",(UINT)-1);
		if (colorBack==(UINT)-1) colorBack=colorBackKey;
		if (colorText==(UINT)-1) colorText=colorTextKey;

		res=DBGetContactSetting(0,szModuleName,"timeoutKey",&dbv_timeout);
		if (res==0) timeout=atoi(dbv_timeout.pszVal);
		DBFreeVariant(&dbv_timeout);
	}
	else if (type==1) {
		colorBack=DBGetContactSettingDword(0,szModuleName,"colorSecb",(UINT)-1);
		colorText=DBGetContactSettingDword(0,szModuleName,"colorSect",(UINT)-1);
		if (colorBack==(UINT)-1) colorBack=colorBackSec;
		if (colorText==(UINT)-1) colorText=colorTextSec;

		res=DBGetContactSetting(0,szModuleName,"timeoutSec",&dbv_timeout);
		if (res==0) timeout=atoi(dbv_timeout.pszVal);
		DBFreeVariant(&dbv_timeout);
	}
	else if (type>=2) {
		colorBack=DBGetContactSettingDword(0, szModuleName, "colorSRb", (UINT)-1);
		colorText=DBGetContactSettingDword(0, szModuleName, "colorSRt", (UINT)-1);
		if (colorBack==(UINT)-1) colorBack=colorBackSR;
		if (colorText==(UINT)-1) colorText=colorTextSR;

		res=DBGetContactSetting(0,szModuleName,"timeoutSR",&dbv_timeout);
		if (res==0) timeout=atoi(dbv_timeout.pszVal);
		DBFreeVariant(&dbv_timeout);
	}

	if( bCoreUnicode && bPopupUnicode ) {
		POPUPDATAW ppd = {0};

		ppd.lchContact = hContact; //Be sure to use a GOOD handle, since this will not be checked.
		ppd.lchIcon = hIcon;
		LPWSTR lpwzContactName = (LPWSTR)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,GCMDF_UNICODE);
		wcscpy(ppd.lpwzContactName, lpwzContactName);
		LPWSTR lpwzText = a2u(lpzText);
		wcscpy(ppd.lpwzText, TranslateW(lpwzText));
		mir_free(lpwzText);
		ppd.colorBack = colorBack;
		ppd.colorText = colorText;
		ppd.iSeconds = timeout;
//		ppd.PluginWindowProc = (WNDPROC)PopupDlgProc;

		//Now that the plugin data has been filled, we add it to the PopUpData.
//		ppd.PluginData = NULL;

		//Now that every field has been filled, we want to see the popup.
		CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&ppd, 0);
	}
	else {
		POPUPDATAEX ppd = {0};

		ppd.lchContact = hContact; //Be sure to use a GOOD handle, since this will not be checked.
		ppd.lchIcon = hIcon;
		LPSTR lpzContactName = (LPSTR)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0);
		strcpy(ppd.lpzContactName, lpzContactName);
		strcpy(ppd.lpzText, Translate(lpzText));
		ppd.colorBack = colorBack;
		ppd.colorText = colorText;
		ppd.iSeconds = timeout;
//		ppd.PluginWindowProc = (WNDPROC)PopupDlgProc;

		//Now that the plugin data has been filled, we add it to the PopUpData.
//		ppd.PluginData = NULL;

		//Now that every field has been filled, we want to see the popup.
		CallService(MS_POPUP_ADDPOPUPEX, (WPARAM)&ppd, 0);
	}
}


void showPopUpDC(HANDLE hContact) {
	int indic=DBGetContactSettingByte(0, szModuleName, "dc",1);
	if (indic==1) showPopUp(sim006,hContact,g_hPOP[POP_SECDIS],1);
}
void showPopUpEC(HANDLE hContact) {
	int indic=DBGetContactSettingByte(0, szModuleName, "ec",1);
	if (indic==1) showPopUp(sim001,hContact,g_hPOP[POP_SECENA],1);
}
void showPopUpKS(HANDLE hContact) {
	int indic=DBGetContactSettingByte(0, szModuleName, "ks",1);
	if (indic==1) showPopUp(sim007,hContact,g_hPOP[POP_SENDKEY],0);
}
void showPopUpKR(HANDLE hContact) {
	int indic=DBGetContactSettingByte(0, szModuleName, "kr",1);
	if (indic==1) showPopUp(sim008,hContact,g_hPOP[POP_RECVKEY],0);
}
void showPopUpSM(HANDLE hContact) {
	int indic=DBGetContactSettingByte(0, szModuleName, "ss",0);
	if (indic==1) showPopUp(sim009,hContact,g_hPOP[POP_SECMSS],2);
}
void showPopUpRM(HANDLE hContact) {
	int indic=DBGetContactSettingByte(0, szModuleName, "sr",0);
	if (indic==1) showPopUp(sim010,hContact,g_hPOP[POP_SECMSR],2);
}


void ShowStatusIcon(HANDLE hContact,UINT mode) {
	HANDLE hMC = getMetaContact(hContact);
	if( bADV ) {
		CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&g_IEC[mode]);
		if( hMC )
		CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hMC, (LPARAM)&g_IEC[mode]);
	}
	if( ServiceExists(MS_MSG_MODIFYICON) ) {
		StatusIconData sid = {0};
		sid.cbSize = sizeof(sid);
		sid.szModule = (char*)szModuleName;
		if( mode ); //sid.hIcon = CopyIcon(g_hIEC[mode]);
		else		sid.flags = MBF_DISABLED;
		if( isChatRoom(hContact) )
			sid.flags |= MBF_HIDDEN;
//		sid.hIconDisabled = g_hIEC[IEC_OFF];
//		sid.szTooltip = Translate("SecureIM");
		CallService(MS_MSG_MODIFYICON, (WPARAM)hContact, (LPARAM)&sid);
		if( hMC )
		CallService(MS_MSG_MODIFYICON, (WPARAM)hMC, (LPARAM)&sid);
	}
}


void ShowStatusIcon(HANDLE hContact) {
	int mode = isContactSecured(hContact);
	if(isContactPGP(hContact)) mode = IEC_PGP;
	else
	if(isContactGPG(hContact)) mode = IEC_GPG;
	ShowStatusIcon(hContact,mode);
}


void ShowStatusIconNotify(HANDLE hContact) {
	int mode = isContactSecured(hContact);
	NotifyEventHooks(g_hEvent[mode!=0], (WPARAM)hContact, 0);
	if(bASI && !mode) mode = IEC_OFF;
	if(isContactPGP(hContact)) mode = IEC_PGP;
	else
	if(isContactGPG(hContact)) mode = IEC_GPG;
	ShowStatusIcon(hContact,mode);
}


void RefreshContactListIcons(void) {

//	CallService(MS_CLUI_LISTBEGINREBUILD,0,0);
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) {
		ShowStatusIcon(hContact,0);
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	g_IEC[0].ColumnType = EXTRA_ICON_ADV1 + bADV - 1;
	for(int i=1;i<IEC_CNT;i++){
		g_IEC[i].ColumnType = g_IEC[0].ColumnType;
	}
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) {
		if (isSecureProtocol(hContact))
			ShowStatusIcon(hContact);
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
//	CallService(MS_CLUI_LISTENDREBUILD,0,0);
}

// EOF
