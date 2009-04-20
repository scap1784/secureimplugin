#include "commonheaders.h"

pSupPro proto=NULL;
pUinKey clist=NULL;
int proto_cnt = 0;
int clist_cnt = 0;


void loadSupportedProtocols() {
    int numberOfProtocols;
    PROTOCOLDESCRIPTOR **protos;
    char *szNames = simDBGetString(0,szModuleName,"protos");
    if( szNames && strchr(szNames,':') == NULL ) {
	LPSTR tmp = (LPSTR) mir_alloc(2048); int j=0;
	for(int i=0; szNames[i]; i++) {
    		if( szNames[i] == ';' ) {
    			memcpy((PVOID)&tmp[j],(PVOID)":1:0:0",6); j+=6;
    		}
    		tmp[j++] = szNames[i];
	}
	tmp[j] = '\0';
    	SAFE_FREE(szNames); szNames = tmp;
	DBWriteContactSettingString(0,szModuleName,"protos",szNames);
    }

    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&numberOfProtocols, (LPARAM)&protos);

	for (int i=0;i<numberOfProtocols;i++) {
//		if (protos[i]->type == PROTOTYPE_PROTOCOL && protos[i]->szName && (CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)||strcmp(protos[i]->szName,"MetaContacts")==0)) {
		if (protos[i]->type == PROTOTYPE_PROTOCOL && protos[i]->szName && CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)) {
		    int j = proto_cnt; proto_cnt++;
		    proto = (pSupPro) mir_realloc(proto,sizeof(SupPro)*proto_cnt);
		    ZeroMemory(&proto[j],sizeof(SupPro));
		    proto[j].name = mir_strdup(protos[i]->szName);
		    if( szNames ) {
       			if( proto[j].name ) {
			    char tmp[128]; strcpy(tmp,proto[j].name); strcat(tmp,":");
       			    LPSTR szName = strstr(szNames,tmp);
       			    if( szName ) {
			    	szName = strchr(szName,':');
				if( szName ) {
				    proto[j].inspecting = (*++szName == '1');
				    szName = strchr(szName,':');
				    if( szName ) {
				    	proto[j].split_on = atoi(++szName); proto[j].tsplit_on = proto[j].split_on;
					szName = strchr(szName,':');
					if( szName ) {
					    proto[j].split_off = atoi(++szName); proto[j].tsplit_off = proto[j].split_off;
				    	}
				    }
			    	}
       			    }
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
	for (int j=0;j<proto_cnt;j++) {
	    mir_free(proto[j].name);
	}
	SAFE_FREE(proto);
	proto_cnt = 0;
}


pSupPro getSupPro(HANDLE hContact) {
	int j;
	for(j=0;j<proto_cnt && !CallService(MS_PROTO_ISPROTOONCONTACT, (WPARAM)hContact, (LPARAM)proto[j].name);j++);
	if(j==proto_cnt) return NULL;
	return &proto[j];
}


// add contact in the list of secureIM users
pUinKey addContact(HANDLE hContact) {
	int j;
	if (hContact) {
//    		LPSTR szProto = (LPSTR) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
    		pSupPro proto = getSupPro(hContact);
		if ( proto ) {
			if ( !CallService(MS_PROTO_ISPROTOONCONTACT, (WPARAM)hContact, (LPARAM)szModuleName) )
				CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)szModuleName);
			for(j=0;j<clist_cnt;j++) {
				if(!clist[j].hContact) break;
			}
			if(j==clist_cnt) {
				clist_cnt++;
				clist = (pUinKey) mir_realloc(clist,sizeof(UinKey)*clist_cnt);
			}
			ZeroMemory(&clist[j],sizeof(UinKey));
			clist[j].hContact = hContact;
			clist[j].proto = proto;
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
			return &clist[j];
		}
	}
	return NULL;
}


// delete contact from the list of secureIM users
void delContact(HANDLE hContact) {
	if (hContact) {
		int j;
		for(j=0;j<clist_cnt;j++) {
			if(clist[j].hContact == hContact) {
				cpp_delete_context(clist[j].cntx); clist[j].cntx = 0;
				clist[j].hContact = 0;
				SAFE_FREE(clist[j].tmp);
				SAFE_FREE(clist[j].msgSplitted);
				return;
			}
		}
	}
}


// load contactlist in the list of secureIM users
void loadContactList() {

    freeContactList();
    loadSupportedProtocols();

    HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
    	  addContact(hContact);
	  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
    }
}


// free list of secureIM users
void freeContactList() {

	for(int j=0;j<clist_cnt;j++) {
		cpp_delete_context(clist[j].cntx);
		SAFE_FREE(clist[j].tmp);
		SAFE_FREE(clist[j].msgSplitted);
	}
	SAFE_FREE(clist);
	clist_cnt = 0;

	freeSupportedProtocols();
}


// find user in the list of secureIM users and add him, if unknow
pUinKey getUinKey(HANDLE hContact) {
    int j;
    for(j=0;j<clist_cnt && clist[j].hContact!=hContact;j++);
    if (j==clist_cnt) return addContact(hContact);
    return &clist[j];
}


pUinKey getUinCtx(int cntx) {
    int j;
    for(j=0;j<clist_cnt && clist[j].cntx!=cntx;j++);
    if (j==clist_cnt) return NULL;
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
