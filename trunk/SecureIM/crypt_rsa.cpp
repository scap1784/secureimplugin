#include "commonheaders.h"


int __cdecl rsa_inject(int,LPCSTR);
int __cdecl rsa_check_pub(int,PBYTE,int,PBYTE,int);
void __cdecl rsa_notify(int,int);

extern void deleteRSAcntx(pUinKey);


pRSA_EXPORT exp = NULL;
RSA_IMPORT imp = {
    rsa_inject,
    rsa_check_pub,
    rsa_notify
};

BOOL rsa_2048=0, rsa_4096=0;


int __cdecl rsa_inject(int context, LPCSTR msg) {
	pUinKey ptr = getUinCtx(context); if(!ptr) return 0;
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_inject: '%s'", msg);
#endif
	int len = strlen(msg)+1;
	LPSTR buf = (LPSTR) mir_alloc(LEN_SECU+len);
	memcpy(buf,SIG_SECU,LEN_SECU);
	memcpy(buf+LEN_SECU,msg,len);
	// отправляем сообщение
	sendSplitMessage(ptr,buf);
	mir_free(buf);
	return 1;
}


#define MSGSIZE 1024

int __cdecl rsa_check_pub(int context, PBYTE pub, int pubLen, PBYTE sig, int sigLen) {
	int v=0;
	pUinKey ptr = getUinCtx(context); if(!ptr) return 0;
	LPSTR cnm = (LPSTR) alloca(NAMSIZE); getContactNameA(ptr->hContact,cnm);
	LPSTR uin = (LPSTR) alloca(KEYSIZE); getContactUinA(ptr->hContact,uin);
	LPSTR msg = (LPSTR) alloca(MSGSIZE);
	if( bAAK ) {
		mir_snprintf(msg,MSGSIZE,Translate(sim521),cnm,uin);
		showPopUpKRmsg(ptr->hContact,msg);
		HistoryLog(ptr->hContact,msg);
		v=1;
	}
	else {
		LPSTR sha = mir_strdup(to_hex(sig,sigLen));
		mir_snprintf(msg,MSGSIZE,Translate(sim520),cnm,sha);
		v=(msgbox(0,msg,szModuleName,MB_YESNO|MB_ICONQUESTION)==IDYES);
		mir_free(sha);
	}
	if(v) {
		DBCONTACTWRITESETTING cws;
		cws.szModule = szModuleName;
		cws.szSetting = "rsa_pub";
		cws.value.type = DBVT_BLOB;
		cws.value.pbVal = pub;
		cws.value.cpbVal = pubLen;
		CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)ptr->hContact, (LPARAM)&cws);
		ptr->keyLoaded = true;
	}
	return v;
}


void __cdecl rsa_notify(int context, int state) {
	pUinKey ptr = getUinCtx(context); if(!ptr) return;
	LPCSTR msg=NULL;
	switch( state ) {
	case 1: {
		showPopUpEC(ptr->hContact);
		ShowStatusIconNotify(ptr->hContact);
		waitForExchange(ptr,2); // досылаем сообщения из очереди
		return;
	}
	case -1: // сессия разорвана по ошибке, неверный тип сообщения
		msg=sim501; break;
	case -2: // сессия разорвана по ошибке другой стороной
		msg=sim502; break;
	case -5: // ошибка декодирования AES сообщения
		msg=sim505; break;
	case -6: // ошибка декодирования RSA сообщения
		msg=sim506; break;
	case -7: // таймаут установки соединения (10 секунд)
		msg=sim507; break;
	case -8: { // сессия разорвана по причине "disabled"
		msg=sim508; 
//		ptr->status=ptr->tstatus=STATUS_DISABLED;
//		DBWriteContactSettingByte(ptr->hContact, szModuleName, "StatusID", ptr->status);
	} break;
	case -0x10: // сессия разорвана по ошибке
	case -0x21:
	case -0x22:
	case -0x23:
	case -0x24:
	case -0x32:
	case -0x33:
	case -0x34:
	case -0x40:
	case -0x50:
	case -0x60: {
		char buf[1024];
		sprintf(buf,sim510,-state);
		showPopUpDCmsg(ptr->hContact,buf);
		ShowStatusIconNotify(ptr->hContact);
       		if(ptr->cntx) deleteRSAcntx(ptr);
		waitForExchange(ptr,3); // досылаем нешифровано
        	return;
        }
	case -3: // соединение разорвано вручную
	case -4: { // соединение разорвано вручную другой стороной
		showPopUpDC(ptr->hContact);
		ShowStatusIconNotify(ptr->hContact);
       		if(ptr->cntx) deleteRSAcntx(ptr);
		waitForExchange(ptr,3); // досылаем нешифровано
		return;
	}
	default:
		return;
	}
	showPopUpDCmsg(ptr->hContact,msg);
	ShowStatusIconNotify(ptr->hContact);
	if(ptr->cntx) deleteRSAcntx(ptr);
	waitForExchange(ptr,3); // досылаем нешифровано
}


void __cdecl sttGenerateRSA( LPVOID param ) {

	char priv_key[4096]; int priv_len;
	char pub_key[4096]; int pub_len;

	exp->rsa_gen_keypair(CPP_MODE_RSA_2048|CPP_MODE_RSA_4096);

	DBCONTACTWRITESETTING cws;
	cws.szModule = szModuleName;
	cws.value.type = DBVT_BLOB;

	exp->rsa_get_keypair(CPP_MODE_RSA_2048,(PBYTE)&priv_key,&priv_len,(PBYTE)&pub_key,&pub_len);
	cws.szSetting = "rsa_priv_2048";
	cws.value.pbVal = (PBYTE)&priv_key;
	cws.value.cpbVal = priv_len;
	CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)0, (LPARAM)&cws);
	cws.szSetting = "rsa_pub_2048";
	cws.value.pbVal = (PBYTE)&pub_key;
	cws.value.cpbVal = pub_len;
	CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)0, (LPARAM)&cws);
	rsa_2048=1;

	exp->rsa_get_keypair(CPP_MODE_RSA_4096,(PBYTE)&priv_key,&priv_len,(PBYTE)&pub_key,&pub_len);
	cws.szSetting = "rsa_priv_4096";
	cws.value.pbVal = (PBYTE)&priv_key;
	cws.value.cpbVal = priv_len;
	CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)0, (LPARAM)&cws);
	cws.szSetting = "rsa_pub_4096";
	cws.value.pbVal = (PBYTE)&pub_key;
	cws.value.cpbVal = pub_len;
	CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)0, (LPARAM)&cws);
	rsa_4096=1;
}

// EOF
