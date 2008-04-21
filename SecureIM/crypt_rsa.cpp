#include "commonheaders.h"


int rsa_inject(int,LPCSTR);
int rsa_check_pub(int,PBYTE,int,PBYTE,int);
void rsa_notify(int,int);

pRSA_EXPORT exp = NULL;
RSA_IMPORT imp = {
    rsa_inject,
    rsa_check_pub,
    rsa_notify
};

BOOL rsa_2048=0, rsa_4096=0;


int rsa_inject(int context, LPCSTR msg) {
	pUinKey ptr = getUinKey(context);
	if(!ptr) return 0;
	CallContactService(pUinKey->hContact,PSS_MESSAGE,(WPARAM)PREF_METANODB,(LPARAM)msg);
	return 1;
}


int rsa_check_pub(int context, PBYTE pub, int pubLen, PBYTE sig, int sigLen) {
	pUinKey ptr = getUinKey(context);
	if(!ptr) return 0;
	//
	return 1;
}


void rsa_notify(int context, int state) {
	pUinKey ptr = getUinKey(context);
	if(!ptr) return;
	//
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
