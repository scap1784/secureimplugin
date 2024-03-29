#include "commonheaders.h"


pRSA_EXPORT exp = NULL;
RSA_IMPORT imp = {
    rsa_inject,
    rsa_check_pub,
    rsa_notify
};

BOOL rsa_4096=0;


int __cdecl rsa_inject(HANDLE context, LPCSTR msg) {
	pUinKey ptr = getUinCtx(context); if(!ptr) return 0;
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_inject: '%s'", msg);
#endif
	int len = strlen(msg)+1;
	LPSTR buf = (LPSTR) mir_alloc(LEN_SECU+len);
	memcpy(buf,SIG_SECU,LEN_SECU);
	memcpy(buf+LEN_SECU,msg,len);
	// ���������� ���������
	splitMessageSend(ptr,buf);
	mir_free(buf);
	return 1;
}


#define MSGSIZE 1024

int __cdecl rsa_check_pub(HANDLE context, PBYTE pub, int pubLen, PBYTE sig, int sigLen) {
	int v=0, k=0;
	pUinKey ptr = getUinCtx(context); if(!ptr) return 0;
	LPSTR cnm = (LPSTR) mir_alloc(NAMSIZE); getContactNameA(ptr->hContact,cnm);
	LPSTR uin = (LPSTR) mir_alloc(KEYSIZE); getContactUinA(ptr->hContact,uin);
	LPSTR msg = (LPSTR) mir_alloc(MSGSIZE);
	LPSTR sha = mir_strdup(to_hex(sig,sigLen));
	LPSTR sha_old = NULL;
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_check_pub: %s %s %s", cnm, uin, sha);
#endif
	DBVARIANT dbv;
	dbv.type = DBVT_BLOB;
	if( DBGetContactSetting(ptr->hContact,szModuleName,"rsa_pub",&dbv) == 0 ) {
		k = 1;
		PBYTE buf = (PBYTE) alloca(sigLen); int len;
		exp->rsa_get_hash((PBYTE)dbv.pbVal,dbv.cpbVal,(PBYTE)buf,&len);
		sha_old = mir_strdup(to_hex(buf,len));
		DBFreeVariant(&dbv);
	}
	if( bAAK ) {
		if( k )	mir_snprintf(msg,MSGSIZE,Translate(sim523),cnm,uin,sha,sha_old);
		else	mir_snprintf(msg,MSGSIZE,Translate(sim521),cnm,uin,sha);
		showPopUpKRmsg(ptr->hContact,msg);
		HistoryLog(ptr->hContact,msg);
		v = 1;
#if defined(_DEBUG) || defined(NETLIB_LOG)
		Sent_NetLog("rsa_check_pub: auto accepted");
#endif
	}
	else {
		if( k ) mir_snprintf(msg,MSGSIZE,Translate(sim522),cnm,sha,sha_old);
		else	mir_snprintf(msg,MSGSIZE,Translate(sim520),cnm,sha);
		v = (msgbox(0,msg,szModuleName,MB_YESNO|MB_ICONQUESTION)==IDYES);
#if defined(_DEBUG) || defined(NETLIB_LOG)
		Sent_NetLog("rsa_check_pub: manual accepted %d",v);
#endif
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
	mir_free(cnm);
	mir_free(uin);
	mir_free(msg);
	mir_free(sha);
	SAFE_FREE(sha_old);
	return v;
}


void __cdecl rsa_notify(HANDLE context, int state) {
	pUinKey ptr = getUinCtx(context); if(!ptr) return;
	LPCSTR msg=NULL;
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_notify: 0x%x", state);
#endif
	switch( state ) {
	case 1: {
		showPopUpEC(ptr->hContact);
		ShowStatusIconNotify(ptr->hContact);
		waitForExchange(ptr,2); // �������� ��������� �� �������
		return;
	}
	case -1: // ������ ��������� �� ������, �������� ��� ���������
		msg=sim501; break;
	case -2: // ������ ��������� �� ������ ������ ��������
		msg=sim502; break;
	case -5: // ������ ������������� AES ���������
		msg=sim505; break;
	case -6: // ������ ������������� RSA ���������
		msg=sim506; break;
	case -7: // ������� ��������� ���������� (10 ������)
		msg=sim507; break;
	case -8: { // ������ ��������� �� ������� "disabled"
		msg=sim508; 
//		ptr->status=ptr->tstatus=STATUS_DISABLED;
//		DBWriteContactSettingByte(ptr->hContact, szModuleName, "StatusID", ptr->status);
	} break;
	case -0x10: // ������ ��������� �� ������
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
		waitForExchange(ptr,3); // �������� �����������
        	return;
        }
	case -3: // ���������� ��������� �������
	case -4: { // ���������� ��������� ������� ������ ��������
		showPopUpDC(ptr->hContact);
		ShowStatusIconNotify(ptr->hContact);
       		if(ptr->cntx) deleteRSAcntx(ptr);
		waitForExchange(ptr,3); // �������� �����������
		return;
	}
	default:
		return;
	}
	showPopUpDCmsg(ptr->hContact,msg);
	ShowStatusIconNotify(ptr->hContact);
	if(ptr->cntx) deleteRSAcntx(ptr);
	waitForExchange(ptr,3); // �������� �����������
}


unsigned __stdcall sttGenerateRSA( LPVOID param ) {

	char priv_key[4096]; int priv_len;
	char pub_key[4096]; int pub_len;

	exp->rsa_gen_keypair(CPP_MODE_RSA_4096);

	DBCONTACTWRITESETTING cws;
	cws.szModule = szModuleName;
	cws.value.type = DBVT_BLOB;

	exp->rsa_get_keypair(CPP_MODE_RSA_4096,(PBYTE)&priv_key,&priv_len,(PBYTE)&pub_key,&pub_len);

	cws.szSetting = "rsa_priv";
	cws.value.pbVal = (PBYTE)&priv_key;
	cws.value.cpbVal = priv_len;
	CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)0, (LPARAM)&cws);

	cws.szSetting = "rsa_pub";
	cws.value.pbVal = (PBYTE)&pub_key;
	cws.value.cpbVal = pub_len;
	CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)0, (LPARAM)&cws);

	rsa_4096=1;

	return 0;
}


// ��������� ������-���� � RSA ��������
BYTE loadRSAkey(pUinKey ptr) {
       	if( !ptr->keyLoaded ) {
       	    DBVARIANT dbv;
       	    dbv.type = DBVT_BLOB;
       	    if(	DBGetContactSetting(ptr->hContact,szModuleName,"rsa_pub",&dbv) == 0 ) {
       		ptr->keyLoaded = exp->rsa_set_pubkey(ptr->cntx,dbv.pbVal,dbv.cpbVal);
#if defined(_DEBUG) || defined(NETLIB_LOG)
		Sent_NetLog("loadRSAkey %d", ptr->keyLoaded);
#endif
       		DBFreeVariant(&dbv);
       	    }
       	}
       	return ptr->keyLoaded;
}

// ������� RSA ��������
void createRSAcntx(pUinKey ptr) {
	if( !ptr->cntx ) {
		ptr->cntx = cpp_create_context(CPP_MODE_RSA);
		ptr->keyLoaded = 0;
	}
}


// ����������� RSA ��������
void resetRSAcntx(pUinKey ptr) {
	if( ptr->cntx ) {
		cpp_delete_context(ptr->cntx);
		ptr->cntx = cpp_create_context(CPP_MODE_RSA);
		ptr->keyLoaded = 0;
	}			
}


// ������� RSA ��������
void deleteRSAcntx(pUinKey ptr) {
	cpp_delete_context(ptr->cntx);
	ptr->cntx = 0;
	ptr->keyLoaded = 0;
}


// EOF
