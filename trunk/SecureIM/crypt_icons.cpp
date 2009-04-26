#include "commonheaders.h"


// преобразует mode в HICON который нужно разрушить в конце
HICON mode2icon(int mode,int type) {

	int m=mode&0x0f,s=(mode&SECURED)>>4,i; // разобрали на части - режим и сосотояние
	HICON icon;

	switch(type) {
	case 1: i=IEC_CL_DIS+s; break;
	case 2: i=ICO_CM_DIS+s; break;
	case 3: i=ICO_MW_DIS+s; break;
	}

	if( type==1 ) {
/*	    if( m==MODE_NATIVE ) { // просто отдаем копию иконки
		icon = CopyIcon(g_hIEC[i]);
	    }
	    else { // надо наложить овелейные изображения*/
		icon = BindOverlayIcon(g_hIEC[i],g_hICO[ICO_OV_NAT+m]);
/*	    }*/
	}
	else {
/*	    if( m==MODE_NATIVE ) { // просто отдаем копию иконки
		icon = CopyIcon(g_hICO[i]);
	    }
	    else { // надо наложить овелейные изображения*/
		icon = BindOverlayIcon(g_hICO[i],g_hICO[ICO_OV_NAT+m]);
/*	    }*/
	}
	return icon;
}


// преобразует mode в IconExtraColumn который НЕ нужно разрушать в конце
IconExtraColumn mode2iec(int mode) {

	int m=mode&0x0f,s=(mode&SECURED)>>4; // разобрали на части - режим и сосотояние

	if( mode==-1 || (!s && !bASI && m!=MODE_PGP && m!=MODE_GPG) ) {
		return g_IEC[0]; // вернем пустое место
	}

	int i=1+m*IEC_CNT+IEC_CL_DIS+s;
	if( g_IEC[i].hImage==(HANDLE)-1 ) {
/*		g_hIEC[i] = mode2icon(mode,1);
		g_IEC[i].hImage = (HANDLE) CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)g_hIEC[i], (LPARAM)0);*/
		HICON icon = mode2icon(mode,1);
		g_IEC[i].hImage = (HANDLE) CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)icon, (LPARAM)0);
		DestroyIcon(icon);
	}
	return g_IEC[i];
}


// обновляет иконки в clist и в messagew
void ShowStatusIcon(HANDLE hContact,int mode) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("ShowStatusIcon(%02X)",mode);
#endif
	HANDLE hMC = getMetaContact(hContact);
	if( bADV ) { // обновить иконки в clist
		if( mode!= -1 ) {
			IconExtraColumn iec=mode2iec(mode);
			CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
			if( hMC )
			CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hMC, (LPARAM)&iec);
		}
	}
	if( ServiceExists(MS_MSG_MODIFYICON) ) {  // обновить иконки в messagew
		StatusIconData sid = {0};
		sid.cbSize = sizeof(sid);
		sid.szModule = (char*)szModuleName;
		for(int i=MODE_NATIVE; i<MODE_CNT;i++) {
			sid.dwId = i;
			sid.flags = (mode&SECURED)?0:MBF_DISABLED;
			if( mode==-1 || (mode&0x0f)!=i || isChatRoom(hContact) )
				sid.flags |= MBF_HIDDEN;  // отключаем все ненужные иконки
			CallService(MS_MSG_MODIFYICON, (WPARAM)hContact, (LPARAM)&sid);
			if( hMC )
			CallService(MS_MSG_MODIFYICON, (WPARAM)hMC, (LPARAM)&sid);
		}
	}
}


void ShowStatusIcon(HANDLE hContact) {
	ShowStatusIcon(hContact,isContactSecured(hContact));
}


void ShowStatusIconNotify(HANDLE hContact) {
	int mode = isContactSecured(hContact);
//	NotifyEventHooks(g_hEvent[(mode&SECURED)!=0], (WPARAM)hContact, 0);
	ShowStatusIcon(hContact,mode);
}


void RefreshContactListIcons(void) {

//	CallService(MS_CLUI_LISTBEGINREBUILD,0,0);
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) { // сначала все выключаем
		ShowStatusIcon(hContact,-1);
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	// менем местоположение иконки
	for(int i=0;i<1+MODE_CNT*IEC_CNT;i++){
		g_IEC[i].ColumnType = bADV;
	}
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) { // и снова зажигаем иконку
		if( isSecureProtocol(hContact) )
			ShowStatusIcon(hContact);
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
//	CallService(MS_CLUI_LISTENDREBUILD,0,0);
}


// EOF
