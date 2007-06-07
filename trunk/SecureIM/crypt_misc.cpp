#include "commonheaders.h"


int __stdcall SendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam ) {
	ACKDATA ack = {0};
	ack.cbSize = sizeof( ACKDATA );
	ack.szModule = 	(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	ack.hContact = hContact;
	ack.type = type;
	ack.result = result;
	ack.hProcess = hProcess;
	ack.lParam = lParam;
	return CallService( MS_PROTO_BROADCASTACK, 0, ( LPARAM )&ack );
}


DWORD CALLBACK sttFakeAck( LPVOID param ) {
	TFakeAckParams* tParam = ( TFakeAckParams* )param;
	WaitForSingleObject( tParam->hEvent, INFINITE );

	Sleep( 100 );
	if ( tParam->msg == NULL )
		SendBroadcast( tParam->hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )tParam->id, 0 );
	else
		SendBroadcast( tParam->hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, ( HANDLE )tParam->id, LPARAM( tParam->msg ));

	CloseHandle( tParam->hEvent );
	delete tParam;
	return 0;
}


void __cdecl sttWaitForExchange( LPVOID param ) {

	TWaitForExchange* tParam = ( TWaitForExchange* )param;

	pUinKey ptr = getUinKey(tParam->hContact);
	delete tParam;
	if(!ptr) return;

	for(int i=0;i<DBGetContactSettingWord(0,szModuleName,"ket",10)*10; i++) {
		Sleep( 100 );
	   	if(!ptr->waitForExchange) return;
	} // for

   	// if keyexchange failed or timeout
   	if(ptr->waitForExchange) {
   		if (ptr->msgQueue && MessageBox(0,Translate(sim104),Translate(szModuleName),MB_YESNO|MB_ICONQUESTION)==IDYES) {
	   		EnterCriticalSection(&localQueueMutex);
	   		ptr->sendQueue = true;
	   		pWM ptrMessage = ptr->msgQueue;
   			while (ptrMessage) {
   				// send unencrypted messages
   				CallContactService(ptr->hContact,PSS_MESSAGE,(WPARAM)ptrMessage->wParam,(LPARAM)ptrMessage->Message);
   				mir_free(ptrMessage->Message);
   				pWM tmp = ptrMessage;
   				ptrMessage = ptrMessage->nextMessage;
   				mir_free(tmp);
   			}
   			ptr->msgQueue = NULL;
	   		ptr->sendQueue = false;
	   		LeaveCriticalSection(&localQueueMutex);
   		}
		ptr->waitForExchange = false;
   		ShowStatusIconNotify(ptr->hContact);
   	} // if
}

// EOF
