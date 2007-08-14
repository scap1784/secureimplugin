#include "commonheaders.h"

pSupPro proto=NULL;
pUinKey clist=NULL;
int proto_cnt = 0;
int clist_cnt = 0;


void loadSupportedProtocols() {

    freeSupportedProtocols();

	int numberOfProtocols;
	PROTOCOLDESCRIPTOR **protos;
    char *szNames = DBGetString(0,szModuleName,"protos");

	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&numberOfProtocols, (LPARAM)&protos);

	for (int i=0;i<numberOfProtocols;i++) {
//		if (protos[i]->type == PROTOTYPE_PROTOCOL && CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0) && protos[i]->szName){
		if (protos[i]->type == PROTOTYPE_PROTOCOL && protos[i]->szName && (CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)||strcmp(protos[i]->szName,"MetaContacts")==0)) {
			int j = proto_cnt;
		    proto_cnt++;
			proto = (pSupPro) mir_realloc(proto,sizeof(SupPro)*proto_cnt);
			ZeroMemory(&proto[j],sizeof(SupPro));
       		proto[j].uniqueName = mir_strdup(protos[i]->szName);
       		if( szNames ) {
       			if( proto[j].uniqueName ) {
				    char tmp[64];
					sprintf(tmp,"%s;",proto[j].uniqueName);
       				proto[j].inspecting = (strstr(szNames,tmp)!=NULL);
				}
	       	}
	       	else {
       			proto[j].inspecting = true;
	       	}
		}
	}
	SAFE_FREE(szNames);
}


void freeSupportedProtocols() {

    for(int j=0;j<proto_cnt;j++) {
		mir_free(proto[j].uniqueName);
	}
	SAFE_FREE(proto);
	proto_cnt = 0;
}


pSupPro getSupPro(HANDLE hContact) {

	int j;
	for(j=0;j<proto_cnt && !CallService(MS_PROTO_ISPROTOONCONTACT, (WPARAM)hContact, (LPARAM)proto[j].uniqueName);j++);
	if(j==proto_cnt) return NULL;
	return &proto[j];
}


// add contact in the list of secureIM users
void addinContactList(HANDLE hContact) {

    if (hContact) {
    	LPSTR szProto = (LPSTR) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if ( szProto && isSecureProtocol(hContact) ) {
			if (!CallService(MS_PROTO_ISPROTOONCONTACT, (WPARAM)hContact, (LPARAM)szModuleName))
				CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)szModuleName);
			int j = clist_cnt;
			clist_cnt++;
			clist = (pUinKey) mir_realloc(clist,sizeof(UinKey)*clist_cnt);
			ZeroMemory(&clist[j],sizeof(UinKey));
			clist[j].hContact = hContact;
			clist[j].szProto = szProto;
			clist[j].mode = DBGetContactSettingByte(hContact, szModuleName, "mode", 99);
			if( clist[j].mode == 99 ) {
				if( isContactPGP(hContact) ) clist[j].mode = 1;
				else
				if( isContactGPG(hContact) ) clist[j].mode = 2;
				else
				clist[j].mode = 0;
				DBWriteContactSettingByte(hContact, szModuleName, "mode", clist[j].mode);
			}
			clist[j].status = DBGetContactSettingByte(hContact, szModuleName, "StatusID", 1);
			clist[j].gpgMode = DBGetContactSettingByte(hContact, szModuleName, "gpgANSI", 0);
		}
	}
}


// load contactlist in the list of secureIM users
void loadContactList() {

    freeContactList();

	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
    	addinContactList(hContact);
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
}


// free list of secureIM users
void freeContactList() {

    for(int j=0;j<clist_cnt;j++) {
		cpp_delete_context(clist[j].cntx);
	}
	SAFE_FREE(clist);
	clist_cnt = 0;
}


// find user in the list of secureIM users and add him, if unknow
pUinKey getUinKey(HANDLE hContact) {

    int j;
    for(j=0;j<clist_cnt && clist[j].hContact!=hContact;j++);
	if (j==clist_cnt) {
		LPSTR szProto = (LPSTR) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if ( szProto && isSecureProtocol(hContact) ) {
			clist_cnt++;
			clist = (pUinKey) mir_realloc(clist,sizeof(UinKey)*clist_cnt);
			ZeroMemory(&clist[j],sizeof(UinKey));
			clist[j].hContact = hContact;
			clist[j].szProto = szProto;
	    }
	    else
	    	return NULL;
	}
	return &clist[j];
}


// add message to user queue for send later
void addMsg2Queue(pUinKey ptr,WPARAM wParam,LPSTR szMsg) {
		
		pWM ptrMessage;

		EnterCriticalSection(&localQueueMutex);

		if(ptr->msgQueue==NULL){
			// create new
			ptr->msgQueue = (pWM) mir_alloc(sizeof(struct waitingMessage));
			ptrMessage = ptr->msgQueue;
		}
		else {
			// add to list
			ptrMessage = ptr->msgQueue;
			while (ptrMessage->nextMessage) {
				ptrMessage = ptrMessage->nextMessage;
			}
			ptrMessage->nextMessage = (pWM) mir_alloc(sizeof(struct waitingMessage));
			ptrMessage = ptrMessage->nextMessage;
		}

		ptrMessage->wParam = wParam;
		ptrMessage->nextMessage = NULL;

		if(wParam & PREF_UNICODE) {
			int slen = strlen(szMsg)+1;
			int wlen = wcslen((wchar_t *)(szMsg+slen))+1;
			ptrMessage->Message = (LPSTR) mir_alloc(slen+wlen*sizeof(WCHAR));
			memcpy(ptrMessage->Message,szMsg,slen+wlen*sizeof(WCHAR));
		}
		else{
			ptrMessage->Message = mir_strdup(szMsg);
		}

		LeaveCriticalSection(&localQueueMutex);
}

// EOF
