#include "commonheaders.h"


int __cdecl rsa_inject(int,LPCSTR);
int __cdecl rsa_check_pub(int,PBYTE,int,PBYTE,int);
void __cdecl rsa_notify(int,int);

pRSA_EXPORT exp = NULL;
RSA_IMPORT imp = {
    rsa_inject,
    rsa_check_pub,
    rsa_notify
};

BOOL rsa_2048=0, rsa_4096=0;


int __cdecl rsa_inject(int context, LPCSTR msg) {
	pUinKey ptr = getUinCtx(context); if(!ptr) return 0;
	LPSTR buf = (LPSTR) mir_alloc(strlen(msg)+LEN_SECU+1);
	memcpy(buf,SIG_SECU,LEN_SECU); memcpy(buf+LEN_SECU,msg,strlen(msg)+1);
#ifdef _DEBUG
	Sent_NetLog("rsa_inject: '%s'", msg);
#endif
	// отправляем сообщение
	sendSplitMessage(ptr,buf);
	mir_free(buf);
	return 1;
}


int __cdecl rsa_check_pub(int context, PBYTE pub, int pubLen, PBYTE sig, int sigLen) {
	pUinKey ptr = getUinCtx(context); if(!ptr) return 0;
	LPSTR sha = mir_strdup(to_hex(sig,sigLen));
	LPSTR cnm = (LPSTR) alloca(128); getContactNameA(ptr->hContact,cnm);
	LPSTR msg = (LPSTR) alloca(512); sprintf(msg,Translate(sim404),cnm,sha);
	int v=(msgbox(0,msg,szModuleName,MB_YESNO|MB_ICONQUESTION)==IDYES);
	if( v ) {
            DBCONTACTWRITESETTING cws;
            cws.szModule = szModuleName;
            cws.szSetting = "rsa_pub";
            cws.value.type = DBVT_BLOB;
            cws.value.pbVal = pub;
            cws.value.cpbVal = pubLen;
            CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)ptr->hContact, (LPARAM)&cws);
            ptr->keyLoaded = true;
	}
	mir_free(sha);
	return v;
}


void __cdecl rsa_notify(int context, int state) {
	pUinKey ptr = getUinCtx(context); if(!ptr) return;
	LPCSTR msg=NULL;
	switch( state ) {
	case 1: {
		showPopUpEC(ptr->hContact);
		ShowStatusIconNotify(ptr->hContact);
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
        	return;
        }
	case -3: // соединение разорвано вручную
	case -4: { // соединение разорвано вручную другой стороной
		showPopUpDC(ptr->hContact);
		ShowStatusIconNotify(ptr->hContact);
		return;
	}
	default:
		return;
	}
	showPopUpDCmsg(ptr->hContact,msg);
	ShowStatusIconNotify(ptr->hContact);
}


void __cdecl sttGenerateRSA( LPVOID param ) {

	char priv_key[4096]; int priv_len;
	char pub_key[4096]; int pub_len;

	exp->rsa_gen_keypair(MODE_RSA_2048|MODE_RSA_4096);

	DBCONTACTWRITESETTING cws;
	cws.szModule = szModuleName;
	cws.value.type = DBVT_BLOB;

	exp->rsa_get_keypair(MODE_RSA_2048,(PBYTE)&priv_key,&priv_len,(PBYTE)&pub_key,&pub_len);
	cws.szSetting = "rsa_priv_2048";
	cws.value.pbVal = (PBYTE)&priv_key;
	cws.value.cpbVal = priv_len;
	CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)0, (LPARAM)&cws);
	cws.szSetting = "rsa_pub_2048";
	cws.value.pbVal = (PBYTE)&pub_key;
	cws.value.cpbVal = pub_len;
	CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)0, (LPARAM)&cws);
	rsa_2048=1;

	exp->rsa_get_keypair(MODE_RSA_4096,(PBYTE)&priv_key,&priv_len,(PBYTE)&pub_key,&pub_len);
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
