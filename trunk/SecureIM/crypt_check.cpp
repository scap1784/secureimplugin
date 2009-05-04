#include "commonheaders.h"


int getContactStatus(HANDLE hContact) {

	pSupPro ptr = getSupPro(hContact);
	if (ptr)
		return DBGetContactSettingWord(hContact, ptr->name, "Status", ID_STATUS_OFFLINE);

	return -1;
}


BOOL isSecureProtocol(HANDLE hContact) {

	pSupPro ptr = getSupPro(hContact);
	if(!ptr) return false;

	return ptr->inspecting;
}


BYTE isContactSecured(HANDLE hContact) {

	if (!clist_cnt) return 0;

	BYTE r=0;

	if( isProtoMetaContacts(hContact) )
		hContact = getMostOnline(hContact); // возьмем тот, через который пойдет сообщение

//	HANDLE hMetaContact = getMetaContact(hContact);
//	if( hMetaContact ) hContact = hMetaContact;

	for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].proto->inspecting) {
/*			if(strstr(clist[j].proto->name,"MetaContacts")!=NULL) {
				for(int i=0;i<CallService(MS_MC_GETNUMCONTACTS,(WPARAM)hContact,0);i++) {
					HANDLE hSubContact = (HANDLE)CallService(MS_MC_GETSUBCONTACT,(WPARAM)hContact,(LPARAM)i);
					if(hSubContact) {
						BYTE secured = isContactSecured(hSubContact);
						if(secured)	return secured;
					}
				}
			}*/
       			DBVARIANT dbv;
			r=clist[j].mode;
			switch(r) {
			case MODE_NATIVE:
				if(cpp_keyx(clist[j].cntx)!=0) r|=SECURED;
				break;
			case MODE_PGP:
        		DBGetContactSetting(hContact,szModuleName,"pgp",&dbv);
        		if( dbv.type!=0 ) r|=SECURED;
        		DBFreeVariant(&dbv);
        		break;
			case MODE_GPG:
        		DBGetContactSetting(hContact,szModuleName,"gpg",&dbv);
        		if( dbv.type!=0 ) r|=SECURED;
        		DBFreeVariant(&dbv);
        		break;
			case MODE_RSAAES:
				if(exp->rsa_get_state(clist[j].cntx)==7) r|=SECURED;
				break;
			case MODE_RSA:
				if(clist[j].cntx) r|=SECURED;
				break;
			}
			break;
		}
	}
	return r; // (mode&SECURED) - проверка на EST/DIS
}


BOOL isClientMiranda(HANDLE hContact) {

	if (!bMCD) return true;
	if (!clist_cnt)	return false;
	for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].proto->inspecting) {

			BOOL isMiranda = true;

			DBVARIANT dbv;
			DBGetContactSetting(hContact,clist[j].proto->name,"MirVer",&dbv);
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
		if (clist[j].hContact == hContact && clist[j].proto->inspecting) {
//			if (strstr(clist[j].proto->name,"MetaContacts")!=NULL) {
//				HANDLE hSubContact;
//				char *proto = 0;
//
//				hSubContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT,(WPARAM)hContact,0);
//				proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hSubContact, 0);
//				return  strstr(proto,"IRC")!=NULL ||
//						strstr(proto,"WinPopup")!=NULL ||
//						strstr(proto,"VyChat")!=NULL;
//			}
//			else {
				return  strstr(clist[j].proto->name,"IRC")!=NULL ||
						strstr(clist[j].proto->name,"WinPopup")!=NULL ||
						strstr(clist[j].proto->name,"VyChat")!=NULL;
//			}
		}
	}
	return false;
}


BOOL isContactInvisible(HANDLE hContact) {
	if( !DBGetContactSettingByte(hContact,"CList","Hidden",0) ) {
		if( !clist_cnt ) return false;
		for(int j=0;j<clist_cnt;j++) {
			if( clist[j].hContact == hContact && clist[j].proto->inspecting ) {
				if( clist[j].waitForExchange ) return false;
				switch( (int)DBGetContactSettingWord(hContact,clist[j].proto->name,"ApparentMode",0) ) {
				case 0:
					return (CallProtoService(clist[j].proto->name,PS_GETSTATUS,0,0)==ID_STATUS_INVISIBLE);
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


BOOL isNotOnList(HANDLE hContact) {
	return DBGetContactSettingByte(hContact, "CList", "NotOnList", 0);
}


BOOL isContactNewPG(HANDLE hContact) {
	if (!clist_cnt) return false;
	for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].cntx) {
			return (clist[j].features & CPP_FEATURES_NEWPG) != 0;
		}
	}
	return false;
}


BOOL isContactPGP(HANDLE hContact) {
	if(!bPGPloaded || (!bPGPkeyrings && !bPGPprivkey)) return false;
	if (!clist_cnt) return false;
//	HANDLE hMetaContact = getMetaContact(hContact);
//   	if( hMetaContact ) hContact = hMetaContact;
	for(int j=0;j<clist_cnt;j++) {
	    if (clist[j].hContact == hContact && clist[j].mode==MODE_PGP) {
//        	HANDLE hMetaContact = getMetaContact(hContact);
        	DBVARIANT dbv;
        	DBGetContactSetting(hContact,szModuleName,"pgp",&dbv);
        	BOOL r=(dbv.type!=0);
        	DBFreeVariant(&dbv);
//        	if( hMetaContact ) {
//        		DBGetContactSetting(hMetaContact,szModuleName,"pgp",&dbv);
//        		r|=(dbv.type!=0);
//        		DBFreeVariant(&dbv);
//        	}
        	return r;
	    }
	}
	return false;
}


BOOL isContactGPG(HANDLE hContact) {
	if(!bGPGloaded || !bGPGkeyrings) return false;
	if (!clist_cnt) return false;
//	HANDLE hMetaContact = getMetaContact(hContact);
//   	if( hMetaContact ) hContact = hMetaContact;
	for(int j=0;j<clist_cnt;j++) {
	    if (clist[j].hContact == hContact && clist[j].mode==MODE_GPG) {
//        	HANDLE hMetaContact = getMetaContact(hContact);
        	DBVARIANT dbv;
        	DBGetContactSetting(hContact,szModuleName,"gpg",&dbv);
        	BOOL r=(dbv.type!=0);
        	DBFreeVariant(&dbv);
//        	if( hMetaContact ) {
//       		DBGetContactSetting(hMetaContact,szModuleName,"gpg",&dbv);
//        		r|=(dbv.type!=0);
//        		DBFreeVariant(&dbv);
//        	}
        	return r;
	    }
	}
	return false;
}


BOOL isContactRSAAES(HANDLE hContact) {
	if (!clist_cnt) return false;
//	HANDLE hMetaContact = getMetaContact(hContact);
//   	if( hMetaContact ) hContact = hMetaContact;
        for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].mode==MODE_RSAAES) {
        		return true;
		}
	}
	return false;
}


BOOL isContactRSA(HANDLE hContact) {
	if (!clist_cnt) return false;
//	HANDLE hMetaContact = getMetaContact(hContact);
//   	if( hMetaContact ) hContact = hMetaContact;
        for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].mode==MODE_RSA) {
        		return true;
		}
	}
	return false;
}


BOOL isChatRoom(HANDLE hContact) {
	if (!clist_cnt) return false;
	for(int j=0;j<clist_cnt;j++) {
		if (clist[j].hContact == hContact && clist[j].proto->inspecting)
			return (DBGetContactSettingByte(hContact,clist[j].proto->name,"ChatRoom",0)!=0);
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
		if (clist[j].hContact == hContact && clist[j].proto->inspecting) {

			BOOL isSecureIM = false;

			if( bNOL && DBGetContactSettingByte(hContact,"CList","NotOnList",0) ) {
				return false;
			}

			DBVARIANT dbv;
			DBGetContactSetting(hContact,clist[j].proto->name,"MirVer",&dbv);
			if(dbv.type==DBVT_ASCIIZ) {
				isSecureIM = (strstr(dbv.pszVal,"SecureIM")!=NULL) || (strstr(dbv.pszVal,"secureim")!=NULL);
			}
			DBFreeVariant(&dbv);

			return isSecureIM;
		}
	}
	return false;
}


// EOF
