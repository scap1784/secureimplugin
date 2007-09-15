#include "commonheaders.h"


BOOL isProtoMetaContacts(HANDLE hContact) {

	if(bMetaContacts)
	for(int j=0;j<clist_cnt;j++)
		if(clist[j].hContact==hContact && clist[j].szProto)
			return strstr(clist[j].szProto,"MetaContacts")!=NULL;
	return false;
}


BOOL isDefaultSubContact(HANDLE hContact) {

	if(bMetaContacts) {
		return (HANDLE)CallService(MS_MC_GETDEFAULTCONTACT,(WPARAM)CallService(MS_MC_GETMETACONTACT,(WPARAM)hContact,0),0)==hContact;
    }
	return false;
}


HANDLE getMetaContact(HANDLE hContact) {

	if(bMetaContacts) {
		return (HANDLE)CallService(MS_MC_GETMETACONTACT,(WPARAM)hContact,0);
    }
    return 0;
}


HANDLE getMostOnline(HANDLE hContact) {

	if(bMetaContacts) {
		return (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT,(WPARAM)hContact,0);
    }
    return 0;
}


// remove all secureim connections on subcontacts
void DeinitMetaContact(HANDLE hContact) {

	HANDLE hMetaContact = isProtoMetaContacts(hContact) ? hContact : getMetaContact(hContact);

	if( hMetaContact ) {
		for(int i=0;i<CallService(MS_MC_GETNUMCONTACTS,(WPARAM)hMetaContact,0);i++) {
			HANDLE hSubContact = (HANDLE)CallService(MS_MC_GETSUBCONTACT,(WPARAM)hMetaContact,(LPARAM)i);
			if(hSubContact && isContactSecured(hSubContact)) {
				CallContactService(hSubContact,PSS_MESSAGE,0,(LPARAM)SIG_DEIN);
			}
		}
	}
}

// EOF