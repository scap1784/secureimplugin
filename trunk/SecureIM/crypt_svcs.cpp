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


// set wait flag and run thread
void waitForExchange(pUinKey ptr) {
	if(ptr->waitForExchange) return;
	ptr->waitForExchange = true;
	forkthread(sttWaitForExchange, 0, new TWaitForExchange( ptr->hContact ));
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


// преобразуем текст из чистого UTF8 в формат миранды
LPSTR utf8_to_miranda(LPCSTR szUtfMsg, DWORD& flags) {
	LPSTR szNewMsg;
	if( iCoreVersion < 0x00060000 ) {
		flags &= ~(PREF_UTF|PREF_UNICODE);
		LPWSTR wszMsg = exp->utf8decode(szUtfMsg);
		LPSTR szMsg = mir_u2a(wszMsg);
		if( bCoreUnicode ) {
		    flags |= PREF_UNICODE;
		    int olen = wcslen((LPWSTR)wszMsg)+1;
		    int nlen = olen*(sizeof(WCHAR)+1);
		    szNewMsg = (LPSTR) mir_alloc(nlen);
		    memcpy(szNewMsg,szMsg,olen);
		    memcpy(szNewMsg+olen,wszMsg,olen*sizeof(WCHAR));
		    mir_free(szMsg);
		}
		else {
		    szNewMsg = szMsg;	
                }
	}
	else {
		flags &= ~PREF_UNICODE;	flags |= PREF_UTF;
		szNewMsg = (LPSTR) mir_strdup(szUtfMsg);
	}
	return szNewMsg;
}


// преобразуем текст из формата миранды в чистый UTF8
LPSTR miranda_to_utf8(LPCSTR szMirMsg, DWORD flags) {
	LPSTR szNewMsg;
	if(flags & PREF_UTF) {
		szNewMsg = (LPSTR) szMirMsg;
	}
	else
	if(flags & PREF_UNICODE) {
		szNewMsg = exp->utf8encode((LPCWSTR)(szMirMsg+strlen(szMirMsg)+1));
	}
	else {
		LPWSTR wszMirMsg = mir_a2u(szMirMsg);
		szNewMsg = exp->utf8encode((LPCWSTR)wszMirMsg);
		mir_free(wszMirMsg);
	}
	return mir_strdup(szNewMsg);
}


// разбивает сообщение szMsg на части длиной iLen, возвращает строку вида PARTzPARTzz
LPSTR splitMessage(LPSTR szMsg, int iLen) {

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("split: msg: -----\n%s\n-----\n",szMsg);
#endif
	int len = strlen(szMsg);
	LPSTR out = (LPSTR) mir_alloc(len<<1);
	LPSTR buf = out;

	WORD msg_id = DBGetContactSettingWord(0, szModuleName, "msgid", 0) + 1;
	DBWriteContactSettingWord(0, szModuleName, "msgid", msg_id);

	int part_all = (len+iLen-1)/iLen;
	for(int part_num=0; part_num<part_all; part_num++) {
		int sz = (len>iLen)?iLen:len;
		sprintf(buf,"%s%04X%02X%02X",SIG_SECP,msg_id,part_num,part_all);
		memcpy(buf+LEN_SECP+8,szMsg,sz);
		*(buf+LEN_SECP+8+sz) = '\0';
#if defined(_DEBUG) || defined(NETLIB_LOG)
		Sent_NetLog("split: part: %s",buf);
#endif
		buf += LEN_SECP+8+sz+1;
		szMsg += sz;
		len -= sz;
	}
	*buf = '\0';
	return out;
}


// собираем сообщение из частей, части храним в структуре у контакта
LPSTR combineMessage(pUinKey ptr, LPSTR szMsg) {

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("combine: part: %s",szMsg);
#endif
	int msg_id,part_num,part_all;
	sscanf(szMsg,"%4X%2X%2X",&msg_id,&part_num,&part_all);
	//
	pPM ppm = NULL, pm = ptr->msgPart;
	if( !ptr->msgPart ) {
		pm = ptr->msgPart = new partitionMessage;
		ZeroMemory(pm,sizeof(partitionMessage));
		pm->id = msg_id;
		pm->message = new LPSTR[part_all];
		ZeroMemory(pm->message,sizeof(LPSTR)*part_all);
	}
	else
	while(pm) {
		if( pm->id == msg_id ) break;
		ppm = pm; pm = pm->nextMessage;
	}
	if(!pm) { // nothing to found
		pm = ppm->nextMessage = new partitionMessage;
		ZeroMemory(pm,sizeof(partitionMessage));
		pm->id = msg_id;
		pm->message = new LPSTR[part_all];
		ZeroMemory(pm->message,sizeof(LPSTR)*part_all);
	}
	pm->message[part_num] = new char[strlen(szMsg)];
	strcpy(pm->message[part_num],szMsg+8);
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("combine: save part: %s",pm->message[part_num]);
#endif
	int len=0,i;
	for( i=0; i<part_all; i++ ){
		if(pm->message[i]==NULL) break;
		len+=strlen(pm->message[i]);
	}
	if( i==part_all ) { // combine message
       		SAFE_FREE(ptr->tmp);
		ptr->tmp = (LPSTR) mir_alloc(len+1); *(ptr->tmp)='\0';
		for( i=0; i<part_all; i++ ){
			strcat(ptr->tmp,pm->message[i]);
			delete pm->message[i];
		}
		delete pm->message;
		if(ppm) ppm->nextMessage = pm->nextMessage;
		else 	ptr->msgPart = pm->nextMessage;
		delete pm;
#if defined(_DEBUG) || defined(NETLIB_LOG)
		Sent_NetLog("combine: all parts: -----\n%s\n-----\n", ptr->tmp);
#endif
		// собрали одно сообщение
		return ptr->tmp;
	}
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("combine: not all parts");
#endif
	// еще не собрали
	return NULL;
}


// отправляет сообщение, если надо то разбивает на части
int sendSplitMessage(pUinKey ptr, LPSTR szMsg) {

	int ret;
	int len = strlen(szMsg);
	int par = (getContactStatus(ptr->hContact)==ID_STATUS_OFFLINE)?ptr->proto->split_off:ptr->proto->split_on;
	if( par && len>par ) {
		LPSTR msg = splitMessage(szMsg,par);
		LPSTR buf = msg;
		while( *buf ) {
			len = strlen(buf);
			ret = CallContactService(ptr->hContact,PSS_MESSAGE,(WPARAM)PREF_METANODB,(LPARAM)buf);
			buf += len+1;
		}
		SAFE_FREE(msg);
	}
	else {
		ret = CallContactService(ptr->hContact,PSS_MESSAGE,(WPARAM)PREF_METANODB,(LPARAM)szMsg);
	}
	return ret;
}


// загружает паблик-ключ в RSA контекст
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

// удаляет RSA контекст
void deleteRSAcntx(pUinKey ptr) {
	cpp_delete_context(ptr->cntx);
	ptr->cntx = 0;
	ptr->keyLoaded = 0;
}


LPSTR szUnrtfMsg = NULL;


// RecvMsg handler
long __cdecl onRecvMsg(WPARAM wParam, LPARAM lParam) {

	CCSDATA *pccsd = (CCSDATA *)lParam;
	PROTORECVEVENT *ppre = (PROTORECVEVENT *)pccsd->lParam;
	pUinKey ptr = getUinKey(pccsd->hContact);
	LPSTR szEncMsg = ppre->szMessage, szPlainMsg = NULL;

#if defined(_DEBUG) || defined(NETLIB_LOG)
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
	BOOL bSecured = isContactSecured(pccsd->hContact)&SECURED;
	BOOL bPGP = isContactPGP(pccsd->hContact);
	BOOL bGPG = isContactGPG(pccsd->hContact);

//	HANDLE hMetaContact = getMetaContact(pccsd->hContact);
//	if( hMetaContact ) {
//		ptr = getUinKey(hMetaContact);
//	}

	// pass any unchanged message
	if( !ptr ||
		ssig==SiG_GAME ||
		!isSecureProtocol(pccsd->hContact) ||
		(isProtoMetaContacts(pccsd->hContact) && (pccsd->wParam & PREF_SIMNOMETA)) ||
		isChatRoom(pccsd->hContact) ||
		(ssig==SiG_NONE && !ptr->msgSplitted && !bSecured && !bPGP && !bGPG)
	  )
		return CallService(MS_PROTO_CHAINRECV, wParam, lParam);

	// drop message: fake, unsigned or from invisible contacts
	if( isContactInvisible(pccsd->hContact) || ssig==SiG_FAKE )
		return 1;

	// receive non-secure message in secure mode
	if( ssig==SiG_NONE && !ptr->msgSplitted ) {
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
	if( ssig!=SiG_PGPM && !bPGP && !bGPG && ptr->status==STATUS_DISABLED ) {
	    if( ptr->mode==MODE_NATIVE ) {
		// tell to the other side that we have the plugin disabled with him
		pccsd->lParam = (LPARAM) SIG_DISA;
		pccsd->szProtoService = PSS_MESSAGE;
		CallService(MS_PROTO_CHAINSEND, wParam, lParam);

		showPopUp(sim003,pccsd->hContact,g_hPOP[POP_PU_DIS],0);
	    }
	    else {
		if( !ptr->cntx ) {
		    ptr->cntx = cpp_create_context(CPP_MODE_RSA);
		    ptr->keyLoaded = 0;
		}
		exp->rsa_disconnect(-ptr->cntx);
		deleteRSAcntx(ptr);
	    }
	    SAFE_FREE(ptr->msgSplitted);
	    return 1;
	}

	// combine message splitted by protocol (no tags)
	if( ssig==SiG_NONE && ptr->msgSplitted ) {
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

	// combine message splitted by secureim (with tags)
	if( ssig==SiG_SECP || ssig==SiG_PART ) {
		LPSTR msg = combineMessage(ptr,szEncMsg);
		if( !msg ) return 1;
		szEncMsg = ppre->szMessage = msg;
		ssig = getSecureSig(msg,&szEncMsg);
	}

	// decrypt PGP/GPG message
	if( ssig==SiG_PGPM &&
	   ((bPGPloaded && (bPGPkeyrings || bPGPprivkey))||
	   (bGPGloaded && bGPGkeyrings))
	  ) {
		szEncMsg = ppre->szMessage;
		if( !ptr->cntx ) {
			ptr->cntx = cpp_create_context(((bGPGloaded && bGPGkeyrings)?CPP_MODE_GPG:CPP_MODE_PGP) | ((DBGetContactSettingByte(pccsd->hContact,szModuleName,"gpgANSI",0))?CPP_MODE_GPG_ANSI:0));
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
			ppre->flags &= ~(PREF_UNICODE|PREF_UTF);
			pccsd->wParam &= ~(PREF_UNICODE|PREF_UTF);
			ppre->szMessage = Translate(sim401);
			return CallService(MS_PROTO_CHAINRECV, wParam, lParam);
		}

		// receive encrypted message in non-encrypted mode
		if(!isContactPGP(pccsd->hContact) && !isContactGPG(pccsd->hContact)) {
			szNewMsg = m_ustrcat(TranslateU(sim403),szOldMsg);
			szOldMsg = szNewMsg;
		}
		szNewMsg = utf8_to_miranda(szOldMsg,ppre->flags); pccsd->wParam = ppre->flags;
		ppre->szMessage = szNewMsg;

		// show decoded message
		showPopUpRM(ptr->hContact);
		SAFE_FREE(ptr->msgSplitted);
		pccsd->wParam |= PREF_SIMNOMETA;
		int ret = CallService(MS_PROTO_CHAINRECV, wParam, lParam);
		mir_free(szNewMsg);
		return ret;
	}

	switch(ssig) {

	case SiG_PGPM:
		return CallService(MS_PROTO_CHAINRECV, wParam, lParam);

	case SiG_SECU: { // new secured msg, pass to rsa_recv
		if( ptr->mode==MODE_NATIVE ) {
		    ptr->mode = MODE_RSAAES;
		    cpp_delete_context(ptr->cntx);
		    ptr->cntx = 0;
		    ptr->keyLoaded = 0;
		    DBWriteContactSettingByte(ptr->hContact, szModuleName, "mode", ptr->mode);
		}
		if( !ptr->cntx ) {
		    ptr->cntx = cpp_create_context(CPP_MODE_RSA);
		    ptr->keyLoaded = 0;
		}
		loadRSAkey(ptr);
		if( exp->rsa_get_state(ptr->cntx)==0 )
		    showPopUpKR(ptr->hContact);

		LPSTR szOldMsg = exp->rsa_recv(ptr->cntx,szEncMsg);
		if( !szOldMsg )	return 1; // don't display it ...

		LPSTR szNewMsg = utf8_to_miranda(szOldMsg,ppre->flags); pccsd->wParam = ppre->flags;
		ppre->szMessage = szNewMsg;

		// show decoded message
		showPopUpRM(ptr->hContact);
		SAFE_FREE(ptr->msgSplitted);
		pccsd->wParam |= PREF_SIMNOMETA;
		int ret = CallService(MS_PROTO_CHAINRECV, wParam, lParam);
		mir_free(szNewMsg);
		return ret;
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
//			showPopUpRM(ptr->hContact);
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

			showPopUp(sim005,NULL,g_hPOP[POP_PU_DIS],0);
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

//			showPopUpRM(ptr->hContact);
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
			showPopUp(sim004,NULL,g_hPOP[POP_PU_DIS],0);
		}
		return 1; // don't display it ...
	} break;

	case SiG_DISA: { // disabled message
		ptr->status=ptr->tstatus=STATUS_DISABLED;
		DBWriteContactSettingByte(ptr->hContact, szModuleName, "StatusID", ptr->status);
	}
	case SiG_DEIN: { // deinit message
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
		if( ptr->mode==MODE_RSAAES ) {
		    ptr->mode = MODE_NATIVE;
		    cpp_delete_context(ptr->cntx);
		    ptr->cntx = 0;
		    ptr->keyLoaded = 0;
		    DBWriteContactSettingByte(ptr->hContact, szModuleName, "mode", ptr->mode);
		}
		switch(ssig) {
		case SiG_KEYR: { // key3 message

			// receive KeyB from user;
			showPopUpKR(ptr->hContact);

			// reinit key exchange if an old key from user is found
			if( cpp_keyb(ptr->cntx) ) {
				cpp_reset_context(ptr->cntx);
			}
			if( InitKeyB(ptr,szEncMsg)!=CPP_ERROR_NONE ) {
				// tell to the other side that we have the plugin disabled with him
				ptr->waitForExchange=false;

				pccsd->lParam = (LPARAM) SIG_DISA;
				pccsd->szProtoService = PSS_MESSAGE;
				CallService(MS_PROTO_CHAINSEND, wParam, lParam);

				showPopUp(sim013,ptr->hContact,g_hPOP[POP_PU_DIS],0);
				ShowStatusIconNotify(ptr->hContact);
				return 1;
			}

			// other side support new key format ?
			if( cpp_get_features(ptr->cntx) & CPP_FEATURES_NEWPG ) {
				cpp_reset_context(ptr->cntx);

				LPSTR keyToSend = InitKeyA(ptr,CPP_FEATURES_NEWPG|KEY_A_SIG); // calculate NEW public and private key
#if defined(_DEBUG) || defined(NETLIB_LOG)
				Sent_NetLog("Sending KEYA: %s", keyToSend);
#endif
				pccsd->lParam = (LPARAM)keyToSend;
				pccsd->szProtoService = PSS_MESSAGE;
				CallService(MS_PROTO_CHAINSEND, wParam, lParam);
				mir_free(keyToSend);

				waitForExchange(ptr);
				showPopUpKS(ptr->hContact);
				return 1;
			}

			// auto send my public key to keyB user if not done before
			if( !cpp_keya(ptr->cntx) ) {
				LPSTR keyToSend = InitKeyA(ptr,0); // calculate public and private key
#if defined(_DEBUG) || defined(NETLIB_LOG)
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
			if(InitKeyB(ptr,szEncMsg)!=CPP_ERROR_NONE) {
				// tell to the other side that we have the plugin disabled with him
				ptr->waitForExchange=false;

				pccsd->lParam = (LPARAM) SIG_DISA;
				pccsd->szProtoService = PSS_MESSAGE;
				CallService(MS_PROTO_CHAINSEND, wParam, lParam);

				showPopUp(sim013,ptr->hContact,g_hPOP[POP_PU_DIS],0);
				ShowStatusIconNotify(ptr->hContact);
				return 1;
			}

			LPSTR keyToSend = InitKeyA(ptr,CPP_FEATURES_NEWPG|KEY_B_SIG); // calculate NEW public and private key
#if defined(_DEBUG) || defined(NETLIB_LOG)
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
			if(!cpp_keya(ptr->cntx) || InitKeyB(ptr,szEncMsg)!=CPP_ERROR_NONE) {
				// tell to the other side that we have the plugin disabled with him
				ptr->waitForExchange=false;

				pccsd->lParam = (LPARAM) SIG_DISA;
				pccsd->szProtoService = PSS_MESSAGE;
				CallService(MS_PROTO_CHAINSEND, wParam, lParam);

				showPopUp(sim013,ptr->hContact,g_hPOP[POP_PU_DIS],0);
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
#if defined(_DEBUG) || defined(NETLIB_LOG)
		Sent_NetLog("Session established");
#endif

		ptr->waitForExchange = false; // дошлем сообщения из очереди
/*		EnterCriticalSection(&localQueueMutex);
		// we need to resend last send back message with new crypto Key
		pWM ptrMessage = ptr->msgQueue;
		while (ptrMessage) {
			pccsd->wParam = ptrMessage->wParam;
			pccsd->lParam = (LPARAM)ptrMessage->Message;
#if defined(_DEBUG) || defined(NETLIB_LOG)
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
		showPopUpSM(ptr->hContact); */
		return 1;
		/* common part (CalcKeyX & SendQueue) */
	} break;

	} //switch

	// receive message
	if (cpp_keyx(ptr->cntx) && (ssig==SiG_ENON||ssig==SiG_ENOF)) {
		showPopUpRM(ptr->hContact);
	}
   	pccsd->wParam |= PREF_SIMNOMETA;
	int ret = CallService(MS_PROTO_CHAINRECV, wParam, lParam);
	SAFE_FREE(szPlainMsg);
	return ret;
}


// SendMsgW handler
long __cdecl onSendMsgW(WPARAM wParam, LPARAM lParam) {
	if(!lParam) return 0;

	CCSDATA *ccs = (CCSDATA *) lParam;
	if( !(ccs->wParam & PREF_UTF) )
		ccs->wParam |= PREF_UNICODE;
	
	return onSendMsg(wParam, lParam);
}


// SendMsg handler
long __cdecl onSendMsg(WPARAM wParam, LPARAM lParam) {

	CCSDATA *pccsd = (CCSDATA *)lParam;
	pUinKey ptr = getUinKey(pccsd->hContact);
	int ssig = getSecureSig((LPCSTR)pccsd->lParam);
	int stat = getContactStatus(pccsd->hContact);

//	HANDLE hMetaContact = getMetaContact(pccsd->hContact);
//	if( hMetaContact ) {
//		ptr = getUinKey(hMetaContact);
//	}
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("onSend: %s",(LPSTR)pccsd->lParam);
#endif
	// pass unhandled messages
	if ( !ptr ||
		ssig==SiG_GAME || ssig==SiG_PGPM || ssig==SiG_SECU || ssig==SiG_SECP ||
		isChatRoom(pccsd->hContact) ||
/*		(ssig!=SiG_NONE && hMetaContact && (pccsd->wParam & PREF_METANODB)) || */
		stat==-1 ||
		(ssig==SiG_NONE && ptr->sendQueue) ||
		(ssig==SiG_NONE && ptr->status==STATUS_DISABLED) // Disabled - pass unhandled
	   )
		return CallService(MS_PROTO_CHAINSEND, wParam, lParam);

	// encrypt PGP/GPG message
	if( isContactPGP(ptr->hContact) || isContactGPG(ptr->hContact) ) {
/*
		if(stat==ID_STATUS_OFFLINE) {
			if (msgbox1(0,sim110,szModuleName,MB_YESNO|MB_ICONQUESTION)==IDNO) {
				return returnNoError(pccsd->hContact);
			}
			// exit and send unencrypted message
			return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
		}
*/
		if( !ptr->cntx ) {
			ptr->cntx = cpp_create_context((isContactGPG(ptr->hContact)?CPP_MODE_GPG:CPP_MODE_PGP) | ((DBGetContactSettingByte(ptr->hContact,szModuleName,"gpgANSI",0))?CPP_MODE_GPG_ANSI:0));
			ptr->keyLoaded = 0;
		}
		if( !ptr->keyLoaded && bPGPloaded ) ptr->keyLoaded = LoadKeyPGP(ptr);
		if( !ptr->keyLoaded && bGPGloaded ) ptr->keyLoaded = LoadKeyGPG(ptr);
		if( !ptr->keyLoaded ) return returnError(pccsd->hContact,Translate(sim108));

		LPSTR szNewMsg = NULL;
		LPSTR szUtfMsg = miranda_to_utf8((LPCSTR)pccsd->lParam,pccsd->wParam);
		if( ptr->keyLoaded == 1 ) { // PGP
    			szNewMsg = pgp_encode(ptr->cntx,szUtfMsg);
    		}
    		else
		if( ptr->keyLoaded == 2 ) { // GPG
    			szNewMsg = gpg_encode(ptr->cntx,szUtfMsg);
		}
		mir_free(szUtfMsg);
		if( !szNewMsg ) {
			return returnError(pccsd->hContact,Translate(sim109));
		}

		// отправляем зашифрованное сообщение
		sendSplitMessage(ptr,szNewMsg);

		showPopUpSM(ptr->hContact);

		return returnNoError(pccsd->hContact);
	}

	// get contact SecureIM status
	int stid = ptr->status;

	// RSA/AES
	if( ptr->mode==MODE_RSAAES ) {
		// contact is offline
		if ( stat==ID_STATUS_OFFLINE ) {
        		if( ptr->cntx ) {
        			if( exp->rsa_get_state(ptr->cntx)!=0 ) {
					cpp_reset_context(ptr->cntx);
					ptr->keyLoaded = 0;
				}
        		}
        		else {
        			ptr->cntx = cpp_create_context(CPP_MODE_RSA);
        			ptr->keyLoaded = 0;
        		}
			if( !loadRSAkey(ptr) || !bSOM ) {
        			// просто шлем незашифрованное в оффлайн
				return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
			}
			LPSTR szUtfMsg = miranda_to_utf8((LPCSTR)pccsd->lParam,pccsd->wParam);
			exp->rsa_send(ptr->cntx,szUtfMsg);
			mir_free(szUtfMsg);
			showPopUpSM(ptr->hContact);
			return returnNoError(pccsd->hContact);
		}
		// SecureIM connection with this contact is disabled
		if( stid==STATUS_DISABLED ) {
			if( ptr->cntx ) {
				exp->rsa_disconnect(-ptr->cntx);
				deleteRSAcntx(ptr);
			}
			if( ssig==SiG_NONE ) {
			    // просто шлем незашифрованное (не знаю даже когда такое случится)
			    return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
			}
			// ничего не шлем дальше - это служебное сообщение
			return returnNoError(pccsd->hContact);
		}
		// разорвать соединение
		if( ssig==SiG_DEIN ) {
			ptr->waitForExchange=false;
			if( ptr->cntx ) {
				exp->rsa_disconnect(ptr->cntx);
				deleteRSAcntx(ptr);
			}
			return returnNoError(pccsd->hContact);
		}
		// соединение установлено
		if( ptr->cntx && exp->rsa_get_state(ptr->cntx)==7 ) {
			LPSTR szUtfMsg = miranda_to_utf8((LPCSTR)pccsd->lParam,pccsd->wParam);
			exp->rsa_send(ptr->cntx,szUtfMsg);
			mir_free(szUtfMsg);
			showPopUpSM(ptr->hContact);
			return returnNoError(pccsd->hContact);
		}
		// просто сообщение (без тэгов, нет контекста и работают AIP & NOL)
		if( ssig==SiG_NONE && isSecureIM(ptr->hContact) ) {
			// добавим его в очередь
		    addMsg2Queue(ptr, pccsd->wParam, (LPSTR)pccsd->lParam);
			// запускаем процесс установки соединения
		    ssig=SiG_INIT;
			// запускаем трэд ожидания и досылки
		    if (!ptr->waitForExchange) waitForExchange(ptr);
		}
		// установить соединение
		if( ssig==SiG_INIT ) {
			if( !ptr->cntx ) {
				ptr->cntx = cpp_create_context(CPP_MODE_RSA);
				ptr->keyLoaded = 0;
			}
			loadRSAkey(ptr);
			exp->rsa_connect(ptr->cntx);
			showPopUpKS(pccsd->hContact);
			ShowStatusIconNotify(pccsd->hContact);
			return returnNoError(pccsd->hContact);
		}
		// просто шлем незашифрованное (не знаю даже когда такое случится)
		return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
	}

	// SecureIM connection with this contact is disabled
	if( stid==STATUS_DISABLED ) {
		// if user try initialize connection
		if( ssig==SiG_INIT ) {
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
	if( stat==ID_STATUS_OFFLINE ) {

		if( ssig==SiG_INIT && cpp_keyx(ptr->cntx) ) {
			// reinit key exchange
			cpp_reset_context(ptr->cntx);
		}

		if( !bSOM ) {
		    if( ssig!=SiG_NONE ) {
				return returnNoError(pccsd->hContact);
		    }
			// exit and send unencrypted message
			return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
		}
		BOOL isMiranda = isClientMiranda(ptr->hContact);

		if( stid==STATUS_ALWAYSTRY && isMiranda ) {  // always try && Miranda
			// set key for offline user
			DBVARIANT dbv; dbv.type = DBVT_BLOB;
			if( DBGetContactSettingDword(ptr->hContact, szModuleName, "offlineKeyTimeout", 0) > gettime() &&
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
				if (msgbox1(0,sim106,szModuleName,MB_YESNO|MB_ICONQUESTION)==IDNO) {
					return returnNoError(pccsd->hContact);
				}
				// exit and send unencrypted message
				return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
			}
		}
		else {
/*			if (stid==STATUS_ALWAYSTRY && !isMiranda || stid!=STATUS_ALWAYSTRY && isMiranda) {
				int res=msgbox1(0,"User is offline now, Do you want to send your message ?\nIt will be unencrypted !","Can't Send Encrypted Message !",MB_YESNO);
				if (res==IDNO) return 1;
			}*/
		    if( ssig!=SiG_NONE ) {
			return returnNoError(pccsd->hContact);
		    }
			// exit and send unencrypted message
			return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
		}

	}
	else {
		// contact is online and we use an offline key -> reset offline key
		if( ptr->offlineKey ) {
			cpp_reset_context(ptr->cntx);
			ptr->offlineKey = false;
			ShowStatusIconNotify(ptr->hContact);
		}
	}

	// if init is called from contact menu list reinit secure im connection
	if( ssig==SiG_INIT ) {
		cpp_reset_context(ptr->cntx);
	}

	// if deinit is called from contact menu list deinit secure im connection
	if( ssig==SiG_DEIN ) {
		// disable SecureIM only if it was enabled
		if (ptr->cntx) {
			cpp_delete_context(ptr->cntx); ptr->cntx=0;

			pccsd->wParam |= PREF_METANODB;
			CallService(MS_PROTO_CHAINSEND, wParam, lParam);

			showPopUpDC(pccsd->hContact);
			ShowStatusIconNotify(pccsd->hContact);
		}
		return returnNoError(pccsd->hContact);
	}

	if( cpp_keya(ptr->cntx) && cpp_keyb(ptr->cntx) && !cpp_keyx(ptr->cntx) ) CalculateKeyX(ptr,ptr->hContact);
	ShowStatusIconNotify(pccsd->hContact);

	// if cryptokey exist
	if( cpp_keyx(ptr->cntx) ) {

/*	    if( !hMetaContact && isProtoMetaContacts(pccsd->hContact) && (DBGetContactSettingByte(NULL, "MetaContacts", "SubcontactHistory", 1) == 1)) {
		// add sent event to subcontact
    		DBEVENTINFO dbei; HANDLE hC = getMostOnline(pccsd->hContact);
		ZeroMemory(&dbei, sizeof(dbei));
		dbei.cbSize = sizeof(dbei);
		dbei.szModule = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hC, 0);
		dbei.flags = DBEF_SENT;
		dbei.timestamp = time(NULL);
		dbei.eventType = EVENTTYPE_MESSAGE;
		if(pccsd->wParam & PREF_RTL) dbei.flags |= DBEF_RTL;
		if(pccsd->wParam & PREF_UTF) dbei.flags |= DBEF_UTF;
		dbei.cbBlob = strlen((char *)pccsd->lParam) + 1;
		if ( pccsd->wParam & PREF_UNICODE )
			dbei.cbBlob *= ( sizeof( wchar_t )+1 );
		dbei.pBlob = (PBYTE)pccsd->lParam;

		CallService(MS_DB_EVENT_ADD, (WPARAM)hC, (LPARAM)&dbei);
	    } */

	    LPSTR szNewMsg = encodeMsg(ptr,(LPARAM)pccsd);

	    pccsd->lParam = (LPARAM) szNewMsg;
	    pccsd->wParam |= PREF_METANODB;
            pccsd->szProtoService = PSS_MESSAGE;
            int ret = CallService(MS_PROTO_CHAINSEND, wParam, lParam);

	    mir_free(szNewMsg);

	    showPopUpSM(ptr->hContact);

	    return ret;
	}
	else {
	  	// send KeyA if init || always_try || waitkey || always_if_possible
  		if( ssig==SiG_INIT || (stid==STATUS_ALWAYSTRY && isClientMiranda(ptr->hContact)) || isSecureIM(ptr->hContact) || ptr->waitForExchange ) {
			if (ssig==SiG_NONE) {
				addMsg2Queue(ptr, pccsd->wParam, (LPSTR)pccsd->lParam);
			}
			if (!ptr->waitForExchange) {
				// init || always_try || always_if_possible
				LPSTR keyToSend = InitKeyA(ptr,0);	// calculate public and private key & fill KeyA

#if defined(_DEBUG) || defined(NETLIB_LOG)
				Sent_NetLog("Sending KEY3: %s", keyToSend);
#endif

				pccsd->wParam &= ~PREF_UNICODE; pccsd->wParam |= PREF_METANODB;
				pccsd->lParam = (LPARAM) keyToSend;
	  			pccsd->szProtoService = PSS_MESSAGE;
	  			CallService(MS_PROTO_CHAINSEND, wParam, lParam);
	  			mir_free(keyToSend);

				waitForExchange(ptr);

	  			showPopUpKS(pccsd->hContact);
	  			ShowStatusIconNotify(pccsd->hContact);
	  		}
			return returnNoError(pccsd->hContact);
	  	}
 		return CallService(MS_PROTO_CHAINSEND, wParam, lParam);
	}
}


int file_idx = 0;

long __cdecl onSendFile(WPARAM wParam, LPARAM lParam) {

	CCSDATA *pccsd=(CCSDATA*)lParam;

	pUinKey ptr = getUinKey(pccsd->hContact);
	if (!ptr || !bSFT) return CallService(PSS_FILE, wParam, lParam);

	if( isContactSecured(pccsd->hContact)&SECURED ) {

		char **file=(char **)pccsd->lParam;
		if(file_idx==100) file_idx=0;
		int i;
		for(i=0;file[i];i++) {

			if (strstr(file[i],".AESHELL")) continue;

		    char *name = strrchr(file[i],'\\');
		    if( !name ) name = file[i];
		    else name++;

			char *file_out = (char*) mir_alloc(TEMP_SIZE+strlen(name)+20);
			sprintf(file_out,"%s\\%s.AESHELL(%d)",TEMP,name,file_idx++);

			char buf[MAX_PATH];
			sprintf(buf,"%s\n%s",Translate(sim011),file[i]);
			showPopUp(buf,NULL,g_hPOP[POP_PU_MSS],2);

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


int __cdecl onProtoAck(WPARAM wParam,LPARAM lParam) {

	ACKDATA *ack=(ACKDATA*)lParam;
	if (ack->type!=ACKTYPE_FILE) return 0; //quit if not file transfer event
	PROTOFILETRANSFERSTATUS *f = (PROTOFILETRANSFERSTATUS*) ack->lParam;

	pUinKey ptr = getUinKey(ack->hContact);
	if (!ptr || (f && f->sending && !bSFT)) return 0;

	if( isContactSecured(ack->hContact)&SECURED ) {
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
				if (strstr(ptr->lastFileRecv,".AESHELL")) mir_unlink(ptr->lastFileRecv);
				SAFE_FREE(ptr->lastFileRecv);
			}
			if (ptr->lastFileSend) {
				if (strstr(ptr->lastFileSend,".AESHELL")) mir_unlink(ptr->lastFileSend);
				SAFE_FREE(ptr->lastFileSend);
			}
			if (ptr->fileSend) {
				char **file=ptr->fileSend;
        		for(int i=0;file[i];i++) {
					if (strstr(file[i],".AESHELL")) mir_unlink(file[i]);
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
					showPopUp(buf,NULL,g_hPOP[POP_PU_MSR],2);

					cpp_decrypt_file(ptr->cntx,ptr->lastFileRecv,file_out);
					mir_free(file_out);
					mir_unlink(ptr->lastFileRecv);
				}
				SAFE_FREE(ptr->lastFileRecv);
				ptr->finFileRecv = false;
			}
			if (ptr->finFileSend && ptr->lastFileSend) {
				if (strstr(ptr->lastFileSend,".AESHELL")) mir_unlink(ptr->lastFileSend);
				SAFE_FREE(ptr->lastFileSend);
				ptr->finFileSend = false;
			}
		} break;
		} // switch
	}
	return 0;
}


int __cdecl onContactSettingChanged(WPARAM wParam,LPARAM lParam) {

	HANDLE hContact = (HANDLE)wParam;
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;
	if(!hContact || strcmp(cws->szSetting,"Status")) return 0;

	pUinKey ptr = getUinKey(hContact);
	int stat = getContactStatus(hContact);
	if(!ptr || stat==-1) return 0;

//	HANDLE hMetaContact = getMetaContact(hContact);
//	if(hMetaContact) {
//		ptr = getUinKey(hMetaContact);
//		if(!ptr) return 0;
//	}

	if (stat==ID_STATUS_OFFLINE) { // go offline
		if (ptr->mode==MODE_NATIVE && cpp_keyx(ptr->cntx)) { // have active context
			cpp_delete_context(ptr->cntx); ptr->cntx=0; // reset context
//			if(hMetaContact) { // is subcontact of metacontact
//				showPopUpDC(hMetaContact);
//				ShowStatusIconNotify(hMetaContact);
//				if(getMostOnline(hMetaContact)) { // make handover
//					CallContactService(hMetaContact,PSS_MESSAGE,0,(LPARAM)SIG_INIT);
//				}
//			}
//			else { // is contact or metacontact (not subcontact)
				showPopUpDC(hContact);	// show popup "Disabled"
				ShowStatusIconNotify(hContact); // change icon in CL
//			}
		}
		else
		if (ptr->mode==MODE_RSAAES && exp->rsa_get_state(ptr->cntx)==7) {
			deleteRSAcntx(ptr);
//			if(hMetaContact) { // is subcontact of metacontact
//				showPopUpDC(hMetaContact);
//				ShowStatusIconNotify(hMetaContact);
//				if(getMostOnline(hMetaContact)) { // make handover
//					CallContactService(hMetaContact,PSS_MESSAGE,0,(LPARAM)SIG_INIT);
//				}
//			}
//			else { // is contact or metacontact (not subcontact)
				showPopUpDC(hContact);	// show popup "Disabled"
				ShowStatusIconNotify(hContact); // change icon in CL
//			}
		}
	}
	else { // go not offline
//		if(!hMetaContact) { // is contact or metacontact (not subcontact)
			if (ptr->offlineKey) {
				cpp_reset_context(ptr->cntx);
				ptr->offlineKey = false;
			}
			ShowStatusIconNotify(hContact); // change icon in CL
//		}
	}
	return 0;
}


static LPCSTR states[] = {sim303,sim304,sim305};


int __cdecl onRebuildContactMenu(WPARAM wParam,LPARAM lParam) {

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("onRebuildContactMenu start");
#endif
	HANDLE hContact = (HANDLE)wParam;
	BOOL bMC = isProtoMetaContacts(hContact);
	if( bMC ) hContact = getMostOnline(hContact); // возьмем тот, через который пойдет сообщение
	pUinKey ptr = getUinKey(hContact);
	int i;

	CLISTMENUITEM mi = {0};
	mi.cbSize = sizeof(CLISTMENUITEM);

	ShowStatusIconNotify(hContact);

	// check offline/online
	if(!ptr) {
		// hide menu bars
		mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIF_HIDDEN;
		for(i=0;i<(sizeof(g_hMenu)/sizeof(g_hMenu[0]));i++) {
			if( g_hMenu[i] )
				CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[i],(LPARAM)&mi);
		}
		return 0;
	}

//	char *szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
//	if (szProto==NULL) // || DBGetContactSettingDword(hContact, szProto, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
//		return 0;

	BOOL isSecureProto = isSecureProtocol(hContact);
	BOOL isPGP = isContactPGP(hContact);
	BOOL isGPG = isContactGPG(hContact);
	BOOL isRSAAES = isContactRSAAES(hContact);
	BOOL isSecured = isContactSecured(hContact)&SECURED;
	BOOL isChat = isChatRoom(hContact);

	// hide all menu bars
	mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIF_HIDDEN;
	for(i=0;i<(sizeof(g_hMenu)/sizeof(g_hMenu[0]));i++) {
		if( g_hMenu[i] )
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[i],(LPARAM)&mi);
	}

	if ( isSecureProto && !isChat && (ptr->mode==MODE_NATIVE || ptr->mode==MODE_RSAAES) && isClientMiranda(hContact) /*&& !getMetaContact(hContact)*/ ) {
		// Native/RSAAES
		mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIM_ICON;
		if( !isSecured ) {
			// create secureim connection
			mi.hIcon = mode2icon(ptr->mode|SECURED,2);
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[0],(LPARAM)&mi);
		}
		else {
			// disable secureim connection
			mi.hIcon = mode2icon(ptr->mode,2);
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[1],(LPARAM)&mi);
		}
		// set status menu
		if(bSCM && !bMC) {
			mi.flags = CMIM_FLAGS;
			for(i=2;i<=(ptr->mode==MODE_RSAAES?4:5);i++) {
				if(g_hMenu[i])
					CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[i],(LPARAM)&mi);
			}

			mi.flags = CMIM_FLAGS | CMIM_NAME | CMIM_ICON;
			mi.hIcon = g_hICO[ICO_ST_DIS+ptr->status];
			mi.pszName = (char*)states[ptr->status];
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[2],(LPARAM)&mi);

			mi.flags = CMIM_FLAGS | CMIM_ICON;
			for(i=0;i<=(ptr->mode==MODE_RSAAES?1:2);i++) {
				mi.hIcon = (i == ptr->status) ? g_hICO[ICO_ST_DIS+ptr->status] : NULL;
				CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[3+i],(LPARAM)&mi);
			}
		}
	}
	else
	if( isSecureProto && !isChat && (ptr->mode==MODE_PGP || ptr->mode==MODE_GPG) ) {
		// PGP, GPG
		if( ptr->mode==MODE_PGP && bPGPloaded ) {
			if((bPGPkeyrings || bPGPprivkey) && !isGPG) {
				mi.flags = CMIM_FLAGS;
				CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[isPGP+6],(LPARAM)&mi);
			}
		}
		if( ptr->mode==MODE_GPG && bGPGloaded ) {
			if(bGPGkeyrings && !isPGP) {
				mi.flags = CMIM_FLAGS;
				CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)g_hMenu[isGPG+8],(LPARAM)&mi);
			}
		}
	}

	return 0;
}


int __cdecl onExtraImageListRebuilding(WPARAM, LPARAM) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("onExtraImageListRebuilding");
#endif
	if( bADV && ServiceExists(MS_CLIST_EXTRA_ADD_ICON) ) {
		RefreshContactListIcons();
	}
	return 0;
}


int __cdecl onExtraImageApplying(WPARAM wParam, LPARAM) {
	if( bADV && ServiceExists(MS_CLIST_EXTRA_SET_ICON) && isSecureProtocol((HANDLE)wParam) ) {
		IconExtraColumn iec = mode2iec(isContactSecured((HANDLE)wParam));
		CallService(MS_CLIST_EXTRA_SET_ICON, wParam, (LPARAM)&iec);
	}
	return 0;
}


//  wParam=(WPARAM)(HANDLE)hContact
//  lParam=0
int __cdecl onContactAdded(WPARAM wParam,LPARAM lParam) {
	addContact((HANDLE)wParam);
	return 0;
}


//  wParam=(WPARAM)(HANDLE)hContact
//  lParam=0
int __cdecl onContactDeleted(WPARAM wParam,LPARAM lParam) {
	delContact((HANDLE)wParam);
	return 0;
}


// EOF
