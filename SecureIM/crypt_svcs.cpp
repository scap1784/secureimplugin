#include "commonheaders.h"

//#define _DEBUG

// return SignID
int getSecureSig(LPCSTR szMsg, LPSTR *szPlainMsg=NULL) {
	if(szPlainMsg) *szPlainMsg=(LPSTR)szMsg;
	for(int i=0;signs[i].len;i++) {
		if (memcmp(szMsg,signs[i].sig,signs[i].len)==0) {
/*			for(int i=strlen(szMsg)-1;i;i--) {
				if( szMsg[i] == '\x0D' || szMsg[i] == '\x0A' )
					((LPSTR)szMsg)[i] = '\0';
				else
					break;
			}*/
			if(szPlainMsg) *szPlainMsg = (LPSTR)(szMsg+signs[i].len);
			if(signs[i].key==SiG_GAME && !bDGP)
				return SiG_NONE;
			return signs[i].key;
		}
	}
	return SiG_NONE;
}


// set wiat flag and run thread
void waitForExchange(HANDLE hContact, pUinKey ptr) {
	ptr->waitForExchange = true;
	forkthread(sttWaitForExchange, 0, new TWaitForExchange( hContact ));
}


int returnNoError(HANDLE hContact) {
	HANDLE hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	CloseHandle( CreateThread( NULL, 0, sttFakeAck, new TFakeAckParams( hEvent, hContact, 666, 0 ), 0, NULL ));
	SetEvent( hEvent );
	return 666;
}


int returnError(HANDLE hContact, LPCSTR err) {
	HANDLE hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	CloseHandle( CreateThread( NULL, 0, sttFakeAck, new TFakeAckParams( hEvent, hContact, 777, err ), 0, NULL ));
	SetEvent( hEvent );
	return 777;
}


LPSTR szUnrtfMsg = NULL;

// RecvMsg handler
int onRecvMsg(WPARAM wParam, LPARAM lParam) {

	CCSDATA *pccsd = (CCSDATA *)lParam;
	PROTORECVEVENT *ppre = (PROTORECVEVENT *)pccsd->lParam;
	pUinKey ptr = getUinKey(pccsd->hContact);
	LPSTR szEncMsg = ppre->szMessage, szPlainMsg = NULL;

#ifdef _DEBUG
	Sent_NetLog("onRecv: %s", szEncMsg);
#endif

	// cut rtf tags
	if( pRtfconvString && memcmp(szEncMsg,"{\\rtf1",6)==0 ) {
		SAFE_FREE(szUnrtfMsg);
		int len = strlen(szEncMsg)+1;
		LPWSTR szTemp = (LPWSTR)mir_alloc(len*sizeof(WCHAR));
	   	if(ppre->flags & PREF_UNICODE)
	   		rtfconvW((LPWSTR)(szEncMsg+len),szTemp);
	   	else
	   		rtfconvA(szEncMsg,szTemp);
	   	len = wcslen(szTemp)-1;
	   	while(len) {
	   		if( szTemp[len] == 0x0D || szTemp[len] == 0x0A )
	   			szTemp[len] = 0;
	   		else
	   			break;
			len--;
	   	}
	   	len = wcslen(&szTemp[1])+1;
	   	szUnrtfMsg = (LPSTR)mir_alloc(len*(sizeof(WCHAR)+1));
		WideCharToMultiByte(CP_ACP, 0, &szTemp[1], -1, szUnrtfMsg, len*(sizeof(WCHAR)+1), NULL, NULL);
		memcpy(szUnrtfMsg+len,&szTemp[1],len*sizeof(WCHAR));
	   	ppre->szMessage = szEncMsg = szUnrtfMsg;
	   	ppre->flags |= PREF_UNICODE;
	   	mir_free(szTemp);
	}

	int ssig = getSecureSig(ppre->szMessage,&szEncMsg);
	BOOL bSecured = isContactSecured(pccsd->hContact);
	BOOL bPGP = isContactPGP(pccsd->hContact);
	BOOL bGPG = isContactGPG(pccsd->hContact);
	HANDLE hMetaContact = getMetaContact(pccsd->hContact);
	if( hMetaContact ) {
		ptr = getUinKey(hMetaContact);
	}

	// pass any unchanged message
	if (!ptr ||
		ssig==SiG_GAME ||
		!isSecureProtocol(pccsd->hContact) ||
		(isProtoMetaContacts(pccsd->hContact) && (pccsd->wParam & PREF_SIMNOMETA)) ||
		isChatRoom(pccsd->hContact) ||
		(ssig==SiG_NONE && !ptr->msgSplitted && !bSecured && !bPGP && !bGPG))
		return CallService(MS_PROTO_CHAINRECV, wParam, lParam);

	// drop message: fake, unsigned or from invisible contacts
	if (isContactInvisible(pccsd->hContact) ||
		ssig==SiG_FAKE)
		return 1;

	// receive non-secure message in secure mode
	if (ssig==SiG_NONE && !ptr->msgSplitted) {
		if(ppre->flags & PREF_UNICODE) {
			szPlainMsg = m_awstrcat(Translate(sim402),szEncMsg);
		}
		else {
			szPlainMsg = m_aastrcat(Translate(sim402),szEncMsg);
		}
       	ppre->szMessage = szPlainMsg;
     	pccsd->wParam |= PREF_SIMNOMETA;
		int ret = CallService(MS_PROTO_CHAINRECV, wParam, lParam);
		mir_free(szPlainMsg);
		return ret;
	}

	// received non-pgp secure message from disabled contact
	if (!bPGP && !bGPG && DBGetContactSettingByte(pccsd->hContact, szModuleName, "StatusID", 1)==0) {
		// tell to the other side that we have the plugin disabled with him
		pccsd->lParam = (LPARAM) SIG_DISA;
		pccsd->szProtoService = PSS_MESSAGE;
		CallService(MS_PROTO_CHAINSEND, wParam, lParam);

		showPopUp(sim003,pccsd->hContact,g_hPOP[POP_SECDIS],0);
		SAFE_FREE(ptr->msgSplitted);
		return 1;
	}

	// combine splitted message
	if(ssig==SiG_NONE && ptr->msgSplitted) {
		LPSTR tmp = (LPSTR) mir_alloc(strlen(ptr->msgSplitted)+strlen(szEncMsg)+1);
		strcpy(tmp,ptr->msgSplitted);
		strcat(tmp,szEncMsg);
		mir_free(ptr->msgSplitted);
		ptr->msgSplitted = szEncMsg = ppre->szMessage = tmp;
		ssig = getSecureSig(tmp,&szEncMsg);
	}
	else {
		SAFE_FREE(ptr->msgSplitted);
	}
	//
	if(ssig==SiG_PART) {
		int msg_id,part_num,part_all;
		sscanf(szEncMsg,"%4X%2X%2X",&msg_id,&part_num,&part_all);
		//
		pPM ppm = NULL, pm = ptr->msgPart;
        if( !ptr->msgPart ) {
			pm = ptr->msgPart = new partitionMessage;
			pm->id = msg_id;
			pm->message = new LPSTR[part_all];
		}
		else
			while(pm) {
				if( pm->id == msg_id ) break;
				ppm = pm; pm = pm->nextMessage;
			}
		if(!pm) { // nothing to found
			pm = ppm->nextMessage = new partitionMessage;
			pm->id = msg_id;
			pm->message = new LPSTR[part_all];
		}
		pm->message[part_num] = new char[strlen(szEncMsg)];
		strcpy(pm->message[part_num],szEncMsg+8);
		int len=0,i;
		for( i=0; i<part_all; i++ ){
			if(pm->message[i]==NULL) break;
			len+=strlen(pm->message[i]);
		}
		if( i==part_all ) { // combine message
			LPSTR tmp = (LPSTR)mir_alloc(len+1);
			for( i=0; i<part_all; i++ ){
				strcat(tmp,pm->message[i]);
				delete pm->message[i];
			}
			szEncMsg = ppre->szMessage = tmp;
			ssig = getSecureSig(tmp,&szEncMsg);
			if(ppm) ppm->nextMessage = pm->nextMessage;
			else 	ptr->msgPart = pm->nextMessage;
			delete pm;
		}
		else
			return 1;
	}

	// decrypt PGP/GPG message
	if(ssig==SiG_PGPM &&
	   ((bPGPloaded && (bPGPkeyrings || bPGPprivkey))||
	   (bGPGloaded && bGPGkeyrings))) {
		szEncMsg = ppre->szMessage;
	    if(!ptr->cntx) {
			ptr->cntx = cpp_create_context(((bGPGloaded && bGPGkeyrings)?MODE_GPG:MODE_PGP) | ((DBGetContactSettingByte(pccsd->hContact,szModuleName,"gpgANSI",0))?MODE_GPG_ANSI:0));
			ptr->keyLoaded = 0;
		}

        if(!strstr(szEncMsg,"-----END PGP MESSAGE-----"))
			return 1; // no end tag, don't display it ...

     	LPSTR szNewMsg = NULL;
    	LPSTR szOldMsg = NULL;

		if(!ptr->keyLoaded && bPGPloaded) ptr->keyLoaded = LoadKeyPGP(ptr);
		if(!ptr->keyLoaded && bGPGloaded) ptr->keyLoaded = LoadKeyGPG(ptr);

        if(ptr->keyLoaded==1) szOldMsg = pgp_decode(ptr->cntx, szEncMsg);
    	else
    	if(ptr->keyLoaded==2) szOldMsg = gpg_decode(ptr->cntx, szEncMsg);

		if(!szOldMsg) { // error while decrypting message, send error
			SAFE_FREE(ptr->msgSplitted);
			ppre->flags &= ~PREF_UNICODE;
			pccsd->wParam &= ~PREF_UNICODE;
	     	ppre->szMessage = Translate(sim401);
			return CallService(MS_PROTO_CHAINRECV, wParam, lParam);
		}

   	    int olen = (strlen(szOldMsg)+1)*(sizeof(WCHAR)+1);
		szNewMsg = (LPSTR) mir_alloc(olen);
		memcpy(szNewMsg,szOldMsg,olen);

   		ppre->flags |= PREF_UNICODE;
   		pccsd->wParam |= PREF_UNICODE;

		if(!isContactPGP(pccsd->hContact) && !isContactGPG(pccsd->hContact)) {
			mir_free(szNewMsg);
			szNewMsg = m_awstrcat(Translate(sim403),szNewMsg);
		}

     	ppre->szMessage = szNewMsg;

		// show decoded message
		showPopUpRM(ptr->hContact);
		SkinPlaySound("IncomingSecureMessage");
		SAFE_FREE(ptr->msgSplitted);
     	pccsd->wParam |= PREF_SIMNOMETA;
		int ret = CallService(MS_PROTO_CHAINRECV, wParam, lParam);
		mir_free(szNewMsg);
		return ret;
	}

	switch(ssig) {

	case SiG_SECU: { // new secured msg, pass to rsa_recv
	    if(!ptr->cntx) {
			ptr->cntx = cpp_create_context(MODE_RSA);
			ptr->keyLoaded = 0;
		}

     	LPSTR szNewMsg = NULL;
    	LPSTR szOldMsg = NULL;

		szOldMsg = exp->rsa_recv(ptr->cntx,szEncMsg);
		if(!szOldMsg) { // don't handle it ...
			SAFE_FREE(ptr->msgSplitted);
			return 1;
		}
	} break;

	case SiG_ENON: { // online message
		if (cpp_keyx(ptr->cntx)) {
			// decrypting message
			szPlainMsg = decodeMsg(ptr,lParam,szEncMsg);
			if(!ptr->decoded) {
				mir_free(szPlainMsg);
				SAFE_FREE(ptr->msgSplitted);
				ptr->msgSplitted=mir_strdup(szEncMsg);
				return 1; // don't display it ...
			}
			showPopUpRM(ptr->hContact);
		}
		else {
			// reinit key exchange user has send an encrypted message and i have no key
			cpp_reset_context(ptr->cntx);

			LPSTR reSend = (LPSTR) mir_alloc(strlen(szEncMsg)+LEN_RSND);
			strcpy(reSend,SIG_RSND); // copy resend sig
			strcat(reSend,szEncMsg); // add mess

			pccsd->lParam = (LPARAM) reSend; // reSend Message to reemit
			pccsd->szProtoService = PSS_MESSAGE;
			CallService(MS_PROTO_CHAINSEND, wParam, lParam); // send back cipher message
			mir_free(reSend);

			LPSTR keyToSend = InitKeyA(ptr,0); // calculate public and private key

			pccsd->lParam = (LPARAM) keyToSend;
			CallService(MS_PROTO_CHAINSEND, wParam, lParam); // send new key
			mir_free(keyToSend);

			showPopUp(sim005,NULL,g_hPOP[POP_SECDIS],0);
			showPopUpKS(ptr->hContact);

			return 1;
		}
	} break;

	case SiG_ENOF: { // offline message
		// if offline key is set and we have not an offline message unset key
		if (ptr->offlineKey && cpp_keyx(ptr->cntx)) {
			cpp_reset_context(ptr->cntx);
			ptr->offlineKey = false;
		}
		// decrypting message with last offline key
		DBVARIANT dbv;
		dbv.type = DBVT_BLOB;

		if(	DBGetContactSetting(ptr->hContact,szModuleName,"offlineKey",&dbv) == 0 ) {
			// if valid key is succefully retrieved
			ptr->offlineKey = true;
			InitKeyX(ptr,dbv.pbVal);
			DBFreeVariant(&dbv);

			// decrypting message
			szPlainMsg = decodeMsg(ptr,lParam,szEncMsg);

			showPopUpRM(ptr->hContact);
			ShowStatusIconNotify(ptr->hContact);
		}
		else {
			// exit and show messsage
			return CallService(MS_PROTO_CHAINRECV, wParam, lParam);
		}
	} break;

	case SiG_RSND: { // resend message
		if (cpp_keyx(ptr->cntx)) {
			// decrypt sended back message and save message for future sending with a new secret key
			szPlainMsg = decodeMsg(ptr,(LPARAM)pccsd,szEncMsg);
			addMsg2Queue(ptr,pccsd->wParam,szPlainMsg);
			mir_free(szPlainMsg);

			showPopUpRM(ptr->hContact);
			showPopUp(sim004,NULL,g_hPOP[POP_SECDIS],0);
		}
		return 1; // don't display it ...
	} break;

	case SiG_DEIN:   // deinit message
	case SiG_DISA: { // disabled message
		// other user has disabled SecureIM with you
		ptr->waitForExchange=false;
		cpp_delete_context(ptr->cntx); ptr->cntx=0;

		showPopUpDC(ptr->hContact);
		ShowStatusIconNotify(ptr->hContact);
		return 1;
	} break;

	case SiG_KEYR:   // key3 message
	case SiG_KEYA:   // keyA message
	case SiG_KEYB: { // keyB message
		switch(ssig) {
		case SiG_KEYR: { // key3 message
			// receive KeyB from user;
			showPopUpKR(ptr->hContact);

			// reinit key exchange if an old key from user is found
			if (cpp_keyb(ptr->cntx)) {
				cpp_reset_context(ptr->cntx);
			}
			if(InitKeyB(ptr,szEncMsg)!=ERROR_NONE) {
				// tell to the other side that we have the plugin disabled with him
				ptr->waitForExchange=false;

				pccsd->lParam = (LPARAM) SIG_DISA;
				pccsd->szProtoService = PSS_MESSAGE;
				CallService(MS_PROTO_CHAINSEND, wParam, lParam);

				showPopUp(sim013,ptr->hContact,g_hPOP[POP_SECDIS],0);
				ShowStatusIconNotify(ptr->hContact);
				return 1;
			}

			// other side support new key format ?
			if( cpp_get_features(ptr->cntx) & FEATURES_NEWPG ) {
				cpp_reset_context(ptr->cntx);

				LPSTR keyToSend = InitKeyA(ptr,FEATURES_NEWPG|KEY_A_SIG); // calculate NEW public and private key
#ifdef _DEBUG
				Sent_NetLog("Sending KEYA: %s", keyToSend);
#endif
				pccsd->lParam = (LPARAM)keyToSend;
				pccsd->szProtoService = PSS_MESSAGE;
				CallService(MS_PROTO_CHAINSEND, wParam, lParam);
				mir_free(keyToSend);

				waitForExchange(ptr->hContact,ptr);
				showPopUpKS(ptr->hContact);
				return 1;
			}

			// auto send my public key to keyB user if not done before
			if (!cpp_keya(ptr->cntx)) {
				LPSTR keyToSend = InitKeyA(ptr,0); // calculate public and private key
#ifdef _DEBUG
				Sent_NetLog("Sending KEYA: %s", keyToSend);
#endif
				pccsd->lParam = (LPARAM)keyToSend;
				pccsd->szProtoService = PSS_MESSAGE;
				CallService(MS_PROTO_CHAINSEND, wParam, lParam);
				mir_free(keyToSend);

				showPopUpKS(ptr->hContact);
			}
		} break;

		case SiG_KEYA: { // keyA message
			// receive KeyA from user;
			showPopUpKR(ptr->hContact);

			cpp_reset_context(ptr->cntx);
			if(InitKeyB(ptr,szEncMsg)!=ERROR_NONE) {
				// tell to the other side that we have the plugin disabled with him
				ptr->waitForExchange=false;

				pccsd->lParam = (LPARAM) SIG_DISA;
				pccsd->szProtoService = PSS_MESSAGE;
				CallService(MS_PROTO_CHAINSEND, wParam, lParam);

				showPopUp(sim013,ptr->hContact,g_hPOP[POP_SECDIS],0);
				ShowStatusIconNotify(ptr->hContact);
				return 1;
			}

			LPSTR keyToSend = InitKeyA(ptr,FEATURES_NEWPG|KEY_B_SIG); // calculate NEW public and private key
#ifdef _DEBUG
			Sent_NetLog("Sending KEYB: %s", keyToSend);
#endif
			pccsd->lParam = (LPARAM)keyToSend;
			pccsd->szProtoService = PSS_MESSAGE;
			CallService(MS_PROTO_CHAINSEND, wParam, lParam);
			mir_free(keyToSend);
		} break;

		case SiG_KEYB: { // keyB message
			// receive KeyB from user;
			showPopUpKR(ptr->hContact);

			// clear all and send DISA if received KeyB, and not exist KeyA or error on InitKeyB
			if(!cpp_keya(ptr->cntx) || InitKeyB(ptr,szEncMsg)!=ERROR_NONE) {
				// tell to the other side that we have the plugin disabled with him
				ptr->waitForExchange=false;

				pccsd->lParam = (LPARAM) SIG_DISA;
				pccsd->szProtoService = PSS_MESSAGE;
				CallService(MS_PROTO_CHAINSEND, wParam, lParam);

				showPopUp(sim013,ptr->hContact,g_hPOP[POP_SECDIS],0);
				ShowStatusIconNotify(ptr->hContact);

				cpp_reset_context(ptr->cntx);
				return 1;
			}
		} break;

		} // switch

		/* common part (CalcKeyX & SendQueue) */
		//  calculate KeyX
		if (cpp_keya(ptr->cntx) && cpp_keyb(ptr->cntx) && !cpp_keyx(ptr->cntx)) CalculateKeyX(ptr,ptr->hContact);
		ShowStatusIconNotify(ptr->hContact);
#ifdef _DEBUG
		Sent_NetLog("Session established");
#endif

		ptr->waitForExchange = false;
		EnterCriticalSection(&localQueueMutex);
		// we need to resend last send back message with new crypto Key
		pWM ptrMessage = ptr->msgQueue;
		while (ptrMessage) {
			pccsd->wParam = ptrMessage->wParam;
			pccsd->lParam = (LPARAM)ptrMessage->Message;
#ifdef _DEBUG
			Sent_NetLog("Sent message from queue: %s",ptrMessage->Message);
#endif
			LPSTR RsMsg = encodeMsg(ptr,(LPARAM)pccsd);

			pccsd->lParam = (LPARAM)RsMsg;
			pccsd->szProtoService = PSS_MESSAGE;
			CallService(MS_PROTO_CHAINSEND, wParam, lParam);

			mir_free(RsMsg);
			mir_free(ptrMessage->Message);

			pWM tmp = ptrMessage;
			ptrMessage = ptrMessage->nextMessage;
			mir_free(tmp);
		}
		ptr->msgQueue = NULL;
		LeaveCriticalSection(&localQueueMutex);
		showPopUpSM(ptr->hContact);
		SkinPlaySound("OutgoingSecureMessage");
		return 1;
		/* common part (CalcKeyX & SendQueue) */
	} break;

	} //switch

	// receive message
	if (cpp_keyx(ptr->cntx) && (ssig==SiG_ENON||ssig==SiG_ENOF)) {
		SkinPlaySound("IncomingSecureMessage");
	}
   	pccsd->wParam |= PREF_SIMNOMETA;
	int ret = CallService(MS_PROTO_CHAINRECV, wParam, lParam);
	SAFE_FREE(szPlainMsg);
	return ret;
}


// SendMsgW handler
int onSendMsgW(WPARAM wParam, LPARAM lParam) {
	if(!lParam) return 0;

    CCSDATA *ccs = (CCSDATA *) lParam;
	ccs->wParam |= PREF_UNICODE;
	
	return onSendMsg(wParam, lParam);
}


// SendMsg handler
int onSendMsg(WPARAM wParam, LPARAM lParam) {

	CCSDATA *pccsd = (CCSDATA *)lParam;
	pUinKey ptr = getUinKey(pccsd->hContact);
	int ssig = getSecureSig((LPCSTR)pccsd->lParam);
	int stat = getContactStatus(pccsd->hContact);
	HANDLE hMetaContact = getMetaContact(pccsd->hContact);

	if( hMetaContact ) {
		ptr = getUinKey(hMetaContact);
	}
#ifdef _DEBUG
	Sent_NetLog("onSend: %s",(LPSTR)pccsd->lParam);
#endif

	// pass unhandled messages
	if (!ptr ||
		ssig==SiG_GAME ||
		isChatRoom(pccsd->hContact) ||
		(hMetaContact && (pccsd->wParam & PREF_METANODB)) ||
		stat == -1 ||
		(ssig==-1 && ptr->sendQueue)
		)
		return CallService(MS_PROTO_CHAINSEND, wParam, lParam);

	// encrypt PGP/GPG message
	if(isContactPGP(ptr->hContact) || isContactGPG(ptr->hContact)) {
/*
		if(stat==ID_STATUS_OFFLINE) {
			if (MessageBox(0,Translate(sim110),Translate(szModuleName),MB_YESNO|MB_ICONQUESTION)==IDNO) {
				return returnNoError(pccsd->hContact);
			}
			// exit and send unencrypted message
			return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
		}
*/
		if(!ptr->cntx) {
			ptr->cntx = cpp_create_context((isContactGPG(ptr->hContact)?MODE_GPG:MODE_PGP) | ((DBGetContactSettingByte(ptr->hContact,szModuleName,"gpgANSI",0))?MODE_GPG_ANSI:0));
			ptr->keyLoaded = 0;
		}
		if(!ptr->keyLoaded && bPGPloaded) ptr->keyLoaded = LoadKeyPGP(ptr);
		if(!ptr->keyLoaded && bGPGloaded) ptr->keyLoaded = LoadKeyGPG(ptr);
		if(!ptr->keyLoaded) return returnError(pccsd->hContact,Translate(sim108));

		LPSTR szOldMsg = (LPSTR) pccsd->lParam;
		LPSTR szNewMsg = NULL;
		if(ptr->keyLoaded == 1) { // PGP
    		if(pccsd->wParam & PREF_UNICODE)
    			szNewMsg = pgp_encodeW(ptr->cntx,(LPCWSTR)(szOldMsg+strlen(szOldMsg)+1));
    		else
    			szNewMsg = pgp_encodeA(ptr->cntx,szOldMsg);
    	}
    	else
		if(ptr->keyLoaded == 2) { // GPG
    		if(pccsd->wParam & PREF_UNICODE)
    			szNewMsg = gpg_encodeW(ptr->cntx,(LPCWSTR)(szOldMsg+strlen(szOldMsg)+1));
    		else
    			szNewMsg = gpg_encodeA(ptr->cntx,szOldMsg);
		}
		if(!szNewMsg) {
			return returnError(pccsd->hContact,Translate(sim109));
		}

		pccsd->wParam &= ~PREF_UNICODE;
		int ret;

		if(stat==ID_STATUS_OFFLINE) {
			char buffer[500];
			int len = strlen(szNewMsg);
			pccsd->lParam = (LPARAM) &buffer;
			pccsd->szProtoService = PSS_MESSAGE;
			szOldMsg = szNewMsg;
			WORD msg_id = DBGetContactSettingWord(pccsd->hContact, szModuleName, "msgid", 0) + 1;
			DBWriteContactSettingWord(pccsd->hContact, szModuleName, "msgid", msg_id);
#define OFFLINE_PART_SIZE	400
			int part_all = (len+OFFLINE_PART_SIZE-1)/OFFLINE_PART_SIZE;
			for(int part_num=0;part_num<part_all;part_num++) {
				int sz = (len>OFFLINE_PART_SIZE)?OFFLINE_PART_SIZE:len;
				sprintf(buffer,"%s%04X%02X%02X",SIG_PART,msg_id,part_num,part_all);
				memcpy(&buffer[LEN_PART+8],szOldMsg,sz);
				buffer[LEN_PART+8+sz] = 0;
#ifdef _DEBUG
				Sent_NetLog("Part: %s",buffer);
#endif
				ret = CallService(MS_PROTO_CHAINSEND, wParam, lParam);
				szOldMsg += sz;
				len -= sz;
			}
		}
		else {
			pccsd->lParam = (LPARAM) szNewMsg;
			pccsd->szProtoService = PSS_MESSAGE;
			ret = CallService(MS_PROTO_CHAINSEND, wParam, lParam);
		}

		showPopUpSM(ptr->hContact);
		SkinPlaySound("OutgoingSecureMessage");

		return ret;
	}

	// get contact SecureIM status
	int stid = DBGetContactSettingByte(ptr->hContact, szModuleName, "StatusID", 1);

	// SecureIM connection with this contact is disabled
	if (stid==STATUS_DISABLED) {
		if (ssig==SiG_INIT) { // if user try initialize connection
			// secure IM is disabled ...
			return returnError(pccsd->hContact,Translate(sim105));
		}
		if (ptr->cntx) { // if exist secure context
			cpp_delete_context(ptr->cntx); ptr->cntx=0;

			CCSDATA ccsd;
			memcpy(&ccsd, (HLOCAL)lParam, sizeof(CCSDATA));

			ccsd.lParam = (LPARAM) SIG_DEIN;
			ccsd.szProtoService = PSS_MESSAGE;
			CallService(MS_PROTO_CHAINSEND, wParam, (LPARAM)&ccsd);

			showPopUpDC(pccsd->hContact);
			ShowStatusIconNotify(pccsd->hContact);
		}
		return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
	}

	// contact is offline
	if (stat==ID_STATUS_OFFLINE) {

		if (ssig==SiG_INIT && cpp_keyx(ptr->cntx)) {
			// reinit key exchange
			cpp_reset_context(ptr->cntx);
		}

		if (!bSOM) {
		    if(ssig!=-1) {
				return returnNoError(pccsd->hContact);
		    }
			// exit and send unencrypted message
			return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
		}
		BOOL isMiranda = isClientMiranda(ptr->hContact);

		if (stid==STATUS_ALWAYSTRY && isMiranda) {  // always try && Miranda
			// set key for offline user
			DBVARIANT dbv; dbv.type = DBVT_BLOB;
			if(	DBGetContactSettingDword(ptr->hContact, szModuleName, "offlineKeyTimeout", 0) > gettime() &&
				DBGetContactSetting(ptr->hContact, szModuleName, "offlineKey", &dbv) == 0
			  ) {
				// if valid key is succefully retrieved
				ptr->offlineKey = true;
				InitKeyX(ptr,dbv.pbVal);
				DBFreeVariant(&dbv);
			}
			else {
				DBDeleteContactSetting(ptr->hContact,szModuleName,"offlineKey");
				DBDeleteContactSetting(ptr->hContact,szModuleName,"offlineKeyTimeout");
				if (MessageBox(0,Translate(sim106),Translate(szModuleName),MB_YESNO|MB_ICONQUESTION)==IDNO) {
					return returnNoError(pccsd->hContact);
				}
				// exit and send unencrypted message
				return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
			}
		}
		else {
/*			if (stid==STATUS_ALWAYSTRY && !isMiranda || stid!=STATUS_ALWAYSTRY && isMiranda) {
				int res=MessageBox(0,Translate("User is offline now, Do you want to send your message ?\nIt will be unencrypted !"),Translate("Can't Send Encrypted Message !"),MB_YESNO);
				if (res==IDNO) return 1;
			}*/
		    if(ssig!=-1) {
				return returnNoError(pccsd->hContact);
		    }
			// exit and send unencrypted message
			return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
		}

	}
	else {
		// contact is online and we use an offline key -> reset offline key
		if (ptr->offlineKey) {
			cpp_reset_context(ptr->cntx);
			ptr->offlineKey = false;
			ShowStatusIconNotify(ptr->hContact);
		}
	}

	// if init is called from contact menu list reinit secure im connection
	if (ssig==SiG_INIT) {
		cpp_reset_context(ptr->cntx);
	}

	// if deinit is called from contact menu list deinit secure im connection
	if (ssig==SiG_DEIN) {
		// disable SecureIM only if it was enabled
		if (ptr->cntx) {
			cpp_delete_context(ptr->cntx); ptr->cntx=0;

			CallService(MS_PROTO_CHAINSEND, wParam, lParam);

			showPopUpDC(pccsd->hContact);
			ShowStatusIconNotify(pccsd->hContact);
		}
		return 1;
	}

	if (cpp_keya(ptr->cntx) && cpp_keyb(ptr->cntx) && !cpp_keyx(ptr->cntx)) CalculateKeyX(ptr,ptr->hContact);
	ShowStatusIconNotify(pccsd->hContact);

	// if cryptokey exist
	if (cpp_keyx(ptr->cntx)) {

		LPSTR szNewMsg = encodeMsg(ptr,(LPARAM)pccsd);

		pccsd->lParam = (LPARAM) szNewMsg;
		pccsd->szProtoService = PSS_MESSAGE;
		int ret = CallService(MS_PROTO_CHAINSEND, wParam, lParam);

		mir_free(szNewMsg);

		showPopUpSM(ptr->hContact);
		SkinPlaySound("OutgoingSecureMessage");

		return ret;
	}
	else {
	  	// send KeyA if init || always_try || waitkey || always_if_possible
  		if (ssig==SiG_INIT || (stid==STATUS_ALWAYSTRY && isClientMiranda(ptr->hContact)) || isSecureIM(ptr->hContact) || ptr->waitForExchange) {
			if (ssig==-1) {
				addMsg2Queue(ptr, pccsd->wParam, (LPSTR)pccsd->lParam);
			}
			if (!ptr->waitForExchange) {
				// init || always_try || always_if_possible
				LPSTR keyToSend = InitKeyA(ptr,0);	// calculate public and private key & fill KeyA

#ifdef _DEBUG
				Sent_NetLog("Sending KEY3: %s", keyToSend);
#endif

	  			pccsd->wParam &= ~PREF_UNICODE;
	  			pccsd->lParam = (LPARAM) keyToSend;
	  			pccsd->szProtoService = PSS_MESSAGE;
	  			CallService(MS_PROTO_CHAINSEND, wParam, lParam);
	  			mir_free(keyToSend);

				waitForExchange(ptr->hContact,ptr);

	  			showPopUpKS(pccsd->hContact);
	  			ShowStatusIconNotify(pccsd->hContact);
	  		}
			return returnNoError(pccsd->hContact);
	  	}
 		return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
	}
}


int file_idx = 0;

int onSendFile(WPARAM wParam, LPARAM lParam) {

	CCSDATA *pccsd=(CCSDATA*)lParam;

	pUinKey ptr = getUinKey(pccsd->hContact);
	if (!ptr || !bSFT) return CallService(PSS_FILE, wParam, lParam);

	if (isContactSecured(pccsd->hContact)) {

		char **file=(char **)pccsd->lParam;
		if(file_idx==100) file_idx=0;
		int i;
		for(i=0;file[i];i++) {

		    char *name = strrchr(file[i],'\\');
		    if( !name ) name = file[i];
		    else name++;

			char *file_out = (char*) mir_alloc(TEMP_SIZE+strlen(name)+20);
			sprintf(file_out,"%s\\%s.AESHELL(%d)",TEMP,name,file_idx++);

			char buf[MAX_PATH];
			sprintf(buf,"%s\n%s",Translate(sim011),file[i]);
            showPopUp(buf,NULL,g_hPOP[POP_SECMSS],2);

			cpp_encrypt_file(ptr->cntx,file[i],file_out);

			mir_free(file[i]);
			file[i]=file_out;
		}
		SAFE_FREE(ptr->fileSend);
		if(i) {
		    ptr->fileSend = (char **)mir_alloc(sizeof(char*)*(i+1));
			for(i=0;file[i];i++) {
				ptr->fileSend[i] = mir_strdup(file[i]);
			}
		}
	}
	return CallService(PSS_FILE, wParam, lParam);
}


int onProtoAck(WPARAM wParam,LPARAM lParam) {

	ACKDATA *ack=(ACKDATA*)lParam;
	if (ack->type!=ACKTYPE_FILE) return 0; //quit if not file transfer event
	PROTOFILETRANSFERSTATUS *f = (PROTOFILETRANSFERSTATUS*) ack->lParam;

	pUinKey ptr = getUinKey(ack->hContact);
	if (!ptr || (f && f->sending && !bSFT)) return 0;

	if (isContactSecured(ack->hContact)) {
		switch(ack->result) {
//		case ACKRESULT_FILERESUME:
		case ACKRESULT_DATA: {
			if (!f->sending) {
				ptr->finFileRecv = (f->currentFileSize == f->currentFileProgress);
				if(!ptr->lastFileRecv) ptr->lastFileRecv = mir_strdup(f->currentFile);
			}
			else if (f->sending) {
				ptr->finFileSend = (f->currentFileSize == f->currentFileProgress);
				if(!ptr->lastFileSend) ptr->lastFileSend = mir_strdup(f->currentFile);
			}
		} break;
//		case ACKRESULT_INITIALISING:
		case ACKRESULT_DENIED:
		case ACKRESULT_FAILED: {
			if (ptr->lastFileRecv) {
				if (strstr(ptr->lastFileRecv,".AESHELL")) unlink(ptr->lastFileRecv);
				SAFE_FREE(ptr->lastFileRecv);
			}
			if (ptr->lastFileSend) {
				if (strstr(ptr->lastFileSend,".AESHELL")) unlink(ptr->lastFileSend);
				SAFE_FREE(ptr->lastFileSend);
			}
			if (ptr->fileSend) {
				char **file=ptr->fileSend;
        		for(int i=0;file[i];i++) {
					if (strstr(file[i],".AESHELL")) unlink(file[i]);
					mir_free(file[i]);
				}
				SAFE_FREE(ptr->fileSend);
			}
			return 0;
		} break;
		case ACKRESULT_NEXTFILE:
		case ACKRESULT_SUCCESS: {
			if (ptr->finFileRecv && ptr->lastFileRecv) {
				if (strstr(ptr->lastFileRecv,".AESHELL")) {
					LPSTR file_out=mir_strdup(ptr->lastFileRecv);
					LPSTR pos=strrchr(file_out,'.'); //find last .
					if (pos) *pos='\0'; //remove aes from name
					if(isFileExist(file_out)) {
						char ext[32]={0};
						LPSTR p=strrchr(file_out,'.');
						LPSTR x=strrchr(file_out,'\\');
						if(p>x) {
							strcpy(ext,p);
							pos=p;
						}
						for(int i=1;i<10000;i++) {
							sprintf(pos," (%d)%s",i,ext);
							if(!isFileExist(file_out)) break;
						}
					}

					char buf[MAX_PATH];
					sprintf(buf,"%s\n%s",Translate(sim012),file_out);
					showPopUp(buf,NULL,g_hPOP[POP_SECMSR],2);

					cpp_decrypt_file(ptr->cntx,ptr->lastFileRecv,file_out);
					mir_free(file_out);
					unlink(ptr->lastFileRecv);
				}
				SAFE_FREE(ptr->lastFileRecv);
				ptr->finFileRecv = false;
			}
			if (ptr->finFileSend && ptr->lastFileSend) {
				if (strstr(ptr->lastFileSend,".AESHELL")) unlink(ptr->lastFileSend);
				SAFE_FREE(ptr->lastFileSend);
				ptr->finFileSend = false;
			}
		} break;
		} // switch
	}
	return 0;
}


int onContactSettingChanged(WPARAM wParam,LPARAM lParam) {

	HANDLE hContact = (HANDLE)wParam;
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;
	pUinKey ptr = getUinKey(hContact);
	int stat = getContactStatus(hContact);

	if(!hContact || !ptr || stat==-1 || strcmp(cws->szSetting,"Status")) return 0;

	HANDLE hMetaContact = getMetaContact(hContact);
	if(hMetaContact) ptr = getUinKey(hMetaContact);

	if (stat==ID_STATUS_OFFLINE) { // go offline
		if (cpp_keyx(ptr->cntx)) { // have active context
			cpp_delete_context(ptr->cntx); ptr->cntx=0; // reset context
			if(hMetaContact) { // is subcontact of metacontact
				if ( !isContactPGP((HANDLE)wParam) && !isContactGPG((HANDLE)wParam) ) { // only for native SecureIM
					showPopUpDC(hMetaContact);
					ShowStatusIconNotify(hMetaContact);
					if(getMostOnline(hMetaContact)) { // make handover
						CallContactService(hMetaContact,PSS_MESSAGE,0,(LPARAM)SIG_INIT);
					}
				}
			}
			else { // is contact or metacontact (not subcontact)
				showPopUpDC(hContact);	// show popup "Disabled"
				ShowStatusIconNotify(hContact); // change icon in CL
			}
		}
	}
	else { // go not offline
		if(!hMetaContact) { // is contact or metacontact (not subcontact)
			if (ptr->offlineKey) {
				cpp_reset_context(ptr->cntx);
				ptr->offlineKey = false;
			}
			ShowStatusIconNotify(hContact); // change icon in CL
		}
	}
	if(bADV) {
		if(isContactPGP((HANDLE)wParam))
			CallService(MS_CLIST_EXTRA_SET_ICON, wParam, (LPARAM)&g_IEC[IEC_PGP]);
		if(isContactGPG((HANDLE)wParam)) 
			CallService(MS_CLIST_EXTRA_SET_ICON, wParam, (LPARAM)&g_IEC[IEC_GPG]);
	}
	return 0;
}


int onContactAdded(WPARAM wParam,LPARAM lParam) {

	CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)wParam, (LPARAM)szModuleName);
	return 0;
}


static LPCSTR states[] = {sim303,sim304,sim305};


int onRebuildContactMenu(WPARAM wParam,LPARAM lParam) {

	HANDLE hContact = (HANDLE)wParam;
	UINT i;

	CLISTMENUITEM mi = {0};
	mi.cbSize = sizeof(CLISTMENUITEM);

	ShowStatusIconNotify(hContact);

	// check offline/online
	if(getUinKey(hContact)==NULL) {
		// hide menu bars
		mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIF_HIDDEN;
		for(i=0;i<(sizeof(g_hMenu)/sizeof(g_hMenu[0]));i++) {
			if( g_hMenu[i] )
				CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[i],(LPARAM)&mi);
		}
		return 0;
	}

//	char *szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
//	if (szProto==NULL) // || DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
//		return 0;

	BOOL isSecureProto = isSecureProtocol(hContact);
	BOOL isPGP = isContactPGP(hContact);
	BOOL isGPG = isContactGPG(hContact);
	BOOL isSecured = isContactSecured(hContact);
	BOOL isChat = isChatRoom(hContact);

	if ( !isSecureProto || isPGP || isGPG || isChat || !isClientMiranda(hContact) || getMetaContact(hContact) ) {
		// hide menu bars
		mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIF_HIDDEN;
		for(i=0;i<(sizeof(g_hMenu)/sizeof(g_hMenu[0]));i++) {
			if( g_hMenu[i] )
				CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[i],(LPARAM)&mi);
		}
	}
	else {
		// create secureim connection
		mi.flags = CMIM_FLAGS | (CMIF_NOTOFFLINE | ((!isSecured) ? (0) : (CMIF_HIDDEN)));
		CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[0],(LPARAM)&mi);
		// disable secureim connection
		mi.flags = CMIM_FLAGS | (CMIF_NOTOFFLINE | ((!isSecured) ? (CMIF_HIDDEN) : (0)));
		CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[1],(LPARAM)&mi);
		// set status menu
		if(!bSCM) {
			mi.flags = CMIM_FLAGS | CMIF_HIDDEN;
			for(i=2;i<=5;i++) {
				if(g_hMenu[i])
					CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[i],(LPARAM)&mi);
			}
		}
		else
		if(!isPGP && !isGPG && !isChat && bSCM && isClientMiranda(hContact)) {
			UINT status = DBGetContactSettingByte(hContact, szModuleName, "StatusID", 1);

			mi.flags = CMIM_FLAGS | CMIM_NAME | CMIM_ICON;
			mi.hIcon = g_hICO[status];
			mi.pszName = (char*)states[status];
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[2],(LPARAM)&mi);

			mi.flags = CMIM_FLAGS | CMIM_ICON;
			for(i=0;i<3;i++) {
				mi.hIcon = (i == status) ? g_hICO[status] : NULL;
				CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[3+i],(LPARAM)&mi);
			}
		}
	}

	if( isSecureProto && !isChat ) {
		if( bPGPloaded ) {
			mi.flags = CMIM_FLAGS | CMIF_HIDDEN;
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[6],(LPARAM)&mi);
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[7],(LPARAM)&mi);
			if((bPGPkeyrings || bPGPprivkey) && !isGPG) {
				mi.flags = CMIM_FLAGS;
				CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[isPGP+6],(LPARAM)&mi);
			}
		}
		if( bGPGloaded ) {
			mi.flags = CMIM_FLAGS | CMIF_HIDDEN;
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[8],(LPARAM)&mi);
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[9],(LPARAM)&mi);
			if(bGPGkeyrings && !isPGP) {
				mi.flags = CMIM_FLAGS;
				CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[isGPG+8],(LPARAM)&mi);
			}
		}
	}

	return 0;
}


int onExtraImageListRebuilding(WPARAM, LPARAM) {
	if(bADV && ServiceExists(MS_CLIST_EXTRA_ADD_ICON)) {
		for(int i=1;i<IEC_CNT;i++)
			g_IEC[i].hImage = (HANDLE) CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)g_hIEC[i], (LPARAM)0);
		RefreshContactListIcons();
	}
	return 0;
}


int onExtraImageApplying(WPARAM wParam, LPARAM) {
	if(bADV && ServiceExists(MS_CLIST_EXTRA_SET_ICON) && isSecureProtocol((HANDLE)wParam)) {
		if(isContactPGP((HANDLE)wParam)) {
			CallService(MS_CLIST_EXTRA_SET_ICON, wParam, (LPARAM)&g_IEC[IEC_PGP]);
			return 0;
		}
		if(isContactGPG((HANDLE)wParam)) {
			CallService(MS_CLIST_EXTRA_SET_ICON, wParam, (LPARAM)&g_IEC[IEC_GPG]);
			return 0;
		}
		int mode = isContactSecured((HANDLE)wParam);
		if(bASI && !mode) mode=IEC_OFF;
		CallService(MS_CLIST_EXTRA_SET_ICON, wParam, (LPARAM)&g_IEC[mode]);
	}
	return 0;
}

//#undef _DEBUG

// EOF
