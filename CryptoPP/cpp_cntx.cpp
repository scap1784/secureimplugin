#include "commonheaders.h"


pCNTX cntx = NULL;
int cntx_idx = 0;
int cntx_cnt = 0;
int cntx_inc = 10; // выделять по 10 контекстов за раз
HANDLE thread_timeout = 0;

void __cdecl sttTimeoutThread(LPVOID);


// get context data on context id
pCNTX get_context_on_id(int context) {

    if(	!thread_timeout ) {
	thread_timeout = (HANDLE) _beginthread(sttTimeoutThread,0,0); // -1 при ошибке
	if( thread_timeout == (HANDLE)-1L ) thread_timeout = 0;
    }

    if( context ) {
		for(int i=0;i<cntx_cnt;i++) {
			if(cntx[i].cntx == context)
				return &cntx[i];
	}
	switch( context ) {
	case -1:
	{
		// create context for private pgp keys
		pCNTX tmp = get_context_on_id(cpp_create_context(0));
		tmp->cntx = context; tmp->mode = MODE_PGP;
		tmp->pdata = (PBYTE) malloc(sizeof(PGPDATA));
		memset(tmp->pdata,0,sizeof(PGPDATA));
		return tmp;
	}
	case -2:
	case -3:
	{
		// create context for private rsa keys
		pCNTX tmp = get_context_on_id(cpp_create_context(0));
		tmp->cntx = context; tmp->mode = (context==-3) ? MODE_RSA_4096 : MODE_RSA_2048;
		pRSAPRIV p = new RSAPRIV;
		tmp->pdata = (PBYTE) p;
		return tmp;
	}
	} // switch
    }
    return NULL;
}


// create context, return context id
int __cdecl cpp_create_context(int mode) {

	int i;
//	while(get_context_on_id(++cntx_idx));
	cntx_idx++;

    	EnterCriticalSection(&localContextMutex);
	for(i=0; i<cntx_cnt && cntx[i].cntx; i++);
	if(i == cntx_cnt) { // надо добавить новый блок
		cntx_cnt += cntx_inc; 
		cntx = (pCNTX) mir_realloc(cntx,sizeof(CNTX)*cntx_cnt);
		memset(&cntx[i],0,sizeof(CNTX)*cntx_inc); // очищаем выделенный блок
	}
	else
		memset(&cntx[i],0,sizeof(CNTX)); // очищаем конкретный контекст
	cntx[i].cntx = cntx_idx;
	cntx[i].mode = mode;
	LeaveCriticalSection(&localContextMutex);

	return cntx_idx;
}


// delete context
void __cdecl cpp_delete_context(int context) {

	pCNTX tmp = get_context_on_id(context);
	if(tmp) { // помечаем на удаление
		tmp->deleted = gettime()+10; // будет удален через 10 секунд
	}
}


// reset context
void __cdecl cpp_reset_context(int context) {

	pCNTX tmp = get_context_on_id(context);
	if(tmp)	cpp_free_keys(tmp);
}


// allocate pdata
PBYTE cpp_alloc_pdata(pCNTX ptr) {
	if( !ptr->pdata ) {
	    if( ptr->mode & MODE_PGP ) {
			ptr->pdata = (PBYTE) malloc(sizeof(PGPDATA));
			memset(ptr->pdata,0,sizeof(PGPDATA));
	    }
	    else
	    if( ptr->mode & MODE_GPG ) {
			ptr->pdata = (PBYTE) malloc(sizeof(GPGDATA));
			memset(ptr->pdata,0,sizeof(GPGDATA));
	    }
	    else
	    if( ptr->mode & MODE_RSA ) {
			pRSADATA p = new RSADATA;
			p->state = 0;
			p->time = 0;
			p->thread = p->event = 0;
			p->thread_exit = 0;
			p->queue = new STRINGQUEUE;
/*			p->pub_k.assign("");
			p->pub_s.assign("");
			p->aes_k.assign("");
			p->aes_v.assign("");*/
			ptr->pdata = (PBYTE) p;
	    }
	    else {
			ptr->pdata = (PBYTE) malloc(sizeof(SIMDATA));
			memset(ptr->pdata,0,sizeof(SIMDATA));
	    }
	}
	return ptr->pdata;
}


// search not established RSA/AES contexts && clear deleted contexts
void __cdecl sttTimeoutThread( LPVOID ) {

	while(1) {
	    Sleep( 1000 ); // раз в секунду
	    DWORD time = gettime();
	    for(int i=0;i<cntx_cnt;i++) {
		EnterCriticalSection(&localContextMutex);
	    	pCNTX tmp = &cntx[i];
	    	if( tmp->cntx>0 && tmp->mode&MODE_RSA && tmp->pdata ) {
	    		// проверяем не протухло ли соединение
			pRSADATA p = (pRSADATA) tmp->pdata;
			if( p->time && p->time < time ) {
				rsa_timeout(tmp->cntx,p);
			}
	    	}
	    	if( tmp->cntx>0 && tmp->deleted && tmp->deleted < time ) {
			// удалить помеченный для удаления контекст
			cpp_free_keys(tmp);
			tmp->cntx = 0;
			tmp->deleted = 0;
	    	}
		LeaveCriticalSection(&localContextMutex);
	    }
	}
}


// EOF
