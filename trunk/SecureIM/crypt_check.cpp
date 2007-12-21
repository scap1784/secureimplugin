#include "commonheaders.h"


void getContactName(HANDLE hContact, LPSTR szName) {
	if( bCoreUnicode )	wcscpy((LPWSTR)szName,(LPWSTR)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,GCMDF_UNICODE));
	else				getContactNameA(hContact, szName);
}


void getContactNameA(HANDLE hContact, LPSTR szName) {
	strcpy(szName,(LPCSTR)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0));
}


void getContactUin(HANDLE hContact, LPSTR szUIN) {
	getContactUinA(hContact, szUIN);
	if( bCoreUnicode && *szUIN ) {
		LPWSTR tmp = a2u(szUIN);
		wcscpy((LPWSTR)szUIN, tmp);
		mir_free(tmp);
	}
}


void getContactUinA(HANDLE hContact, LPSTR szUIN) {

	*szUIN = 0;

	pSupPro ptr = getSupPro(hContact);
	if(!ptr) return;

	DBVARIANT dbv_uniqueid;
	LPSTR uID = (LPSTR) CallProtoService(ptr->uniqueName, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
	if( uID && DBGetContactSetting(hContact, ptr->uniqueName, uID, &dbv_uniqueid)==0 ) {
		if (dbv_uniqueid.type == DBVT_WORD)
			sprintf(szUIN, "%u [%s]", dbv_uniqueid.wVal, ptr->uniqueName);
		else
		if (dbv_uniqueid.type == DBVT_DWORD)
			sprintf(szUIN, "%u [%s]", (UINT)dbv_uniqueid.dVal, ptr->uniqueName);
		else
		if (dbv_uniqueid.type == DBVT_BLOB)
			sprintf(szUIN, "%s [%s]", dbv_uniqueid.pbVal, ptr->uniqueName);
		else
			sprintf(szUIN, "%s [%s]", dbv_uniqueid.pszVal, ptr->uniqueName);
	}
	else {
		strcpy(szUIN, "===  unknown  ===");
	}
	DBFreeVariant(&dbv_uniqueid);
}


int getContactStatus(HANDLE hContact) {

	pSupPro ptr = getSupPro(hContact);
	if (ptr)
		return DBGetContactSettingWord(hContact, ptr->uniqueName , "Status", ID_STATUS_OFFLINE);

	return -1;
}


BOOL isSecureProtocol(HANDLE hContact) {

	pSupPro ptr = getSupPro(hContact);
	if(!ptr) return false;

	return ptr->inspecting;
}


BYTE isContactSecured(HANDLE hContact) {
	if (!clist_cnt) return 0;

	HANDLE hMetaContact = getMetaContact(hContact);
	if( hMetaContact ) hContact = hMetaContact;

    for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].szProto) {
/*			if(strstr(clist[j].szProto,"MetaContacts")!=NULL) {
				for(int i=0;i<CallService(MS_MC_GETNUMCONTACTS,(WPARAM)hContact,0);i++) {
					HANDLE hSubContact = (HANDLE)CallService(MS_MC_GETSUBCONTACT,(WPARAM)hContact,(LPARAM)i);
					if(hSubContact) {
						BYTE secured = isContactSecured(hSubContact);
						if(secured)	return secured;
					}
				}
			}*/
			if(cpp_keyx(clist[j].cntx)!=0) {
				int features = cpp_get_features(clist[j].cntx);
				if( features == 0)
					return IEC_ON_OLD;
				else
				if(features & FEATURES_NEWPG)
					return IEC_ON;
				return IEC_ON_MID;
			}
			break;
		}
	}
	return 0;
}


BOOL isClientMiranda(HANDLE hContact) {

	if (!bMCD) return true;
	if (!clist_cnt)	return false;
    for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].szProto) {

			BOOL isMiranda = true;

			DBVARIANT dbv;
			DBGetContactSetting(hContact,clist[j].szProto,"MirVer",&dbv);
			if(dbv.type==DBVT_ASCIIZ) {
				isMiranda = strstr(dbv.pszVal,"Miranda")!=NULL;
			}
			DBFreeVariant(&dbv);

			return isMiranda;
		}
	}
	return false;
}


BOOL isProtoSmallPackets(HANDLE hContact) {
	if (!clist_cnt) return false;
    for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].szProto) {
			if (strstr(clist[j].szProto,"MetaContacts")!=NULL) {
				HANDLE hSubContact;
				char *proto = 0;

				hSubContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT,(WPARAM)hContact,0);
				proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hSubContact, 0);
				return  strstr(proto,"IRC")!=NULL ||
						strstr(proto,"WinPopup")!=NULL ||
						strstr(proto,"VyChat")!=NULL;
			}
			else {
				return  strstr(clist[j].szProto,"IRC")!=NULL ||
						strstr(clist[j].szProto,"WinPopup")!=NULL ||
						strstr(clist[j].szProto,"VyChat")!=NULL;
			}
		}
	}
	return false;
}


BOOL isContactInvisible(HANDLE hContact) {
	if(!DBGetContactSettingByte(hContact,"CList","Hidden",0)) {
		if (!clist_cnt) return false;
    	for(int j=0;j<clist_cnt;j++) {
			if (clist[j].hContact == hContact && clist[j].szProto) {
				if(clist[j].waitForExchange) return false;
				switch(DBGetContactSettingWord(hContact,clist[j].szProto,"ApparentMode",0)) {
				case 0:
					return (DBGetContactSettingWord(NULL,clist[j].szProto,"Status",ID_STATUS_OFFLINE)==ID_STATUS_INVISIBLE);
				case ID_STATUS_ONLINE:
					return false;
				case ID_STATUS_OFFLINE:
					return true;
				} //switch
				break;
			}
		}// for
	}
	return true;
}


BOOL isContactNewPG(HANDLE hContact) {
	if (!clist_cnt) return false;
    for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].cntx) {
			return (cpp_get_features(clist[j].cntx) & FEATURES_NEWPG) != 0;
		}
	}
	return false;
}


BOOL isContactPGP(HANDLE hContact) {
	if(!bPGPloaded || (!bPGPkeyrings && !bPGPprivkey)) return false;
	if (!clist_cnt) return false;
    for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact)
			return (clist[j].mode==1);
	}
	return false;
/*	HANDLE hMetaContact = getMetaContact(hContact);
	DBVARIANT dbv;
	DBGetContactSetting(hContact,szModuleName,"pgp",&dbv);
	BOOL r=(dbv.type!=0);
	DBFreeVariant(&dbv);
	if( hMetaContact ) {
		DBGetContactSetting(hMetaContact,szModuleName,"pgp",&dbv);
		r|=(dbv.type!=0);
		DBFreeVariant(&dbv);
	}
	return r;*/
}


BOOL isContactGPG(HANDLE hContact) {
	if(!bGPGloaded || !bGPGkeyrings) return false;
	if (!clist_cnt) return false;
    for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact)
			return (clist[j].mode==2);
	}
	return false;
/*	HANDLE hMetaContact = getMetaContact(hContact);
	DBVARIANT dbv;
	DBGetContactSetting(hContact,szModuleName,"gpg",&dbv);
	BOOL r=(dbv.type!=0);
	DBFreeVariant(&dbv);
	if( hMetaContact ) {
		DBGetContactSetting(hMetaContact,szModuleName,"gpg",&dbv);
		r|=(dbv.type!=0);
		DBFreeVariant(&dbv);
	}
	return r;*/
}


BOOL isChatRoom(HANDLE hContact) {
	if (!clist_cnt) return false;
    for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].szProto)
			return (DBGetContactSettingByte(hContact,clist[j].szProto,"ChatRoom",0)!=0);
	}
	return false;
}


BOOL isFileExist(LPCSTR filename) {
	return (GetFileAttributes(filename)!=(UINT)-1);
}


BOOL isSecureIM(HANDLE hContact) {

	if (!bAIP) return false;
	if (!clist_cnt) return false;
    for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].szProto) {

			BOOL isSecureIM = false;

			DBVARIANT dbv;
			DBGetContactSetting(hContact,clist[j].szProto,"MirVer",&dbv);
			if(dbv.type==DBVT_ASCIIZ) {
				isSecureIM = strstr(dbv.pszVal,"secureim")!=NULL || strstr(dbv.pszVal,"SecureIM")!=NULL;
			}
			DBFreeVariant(&dbv);

			return isSecureIM;
		}
	}
	return false;
}

// EOF
