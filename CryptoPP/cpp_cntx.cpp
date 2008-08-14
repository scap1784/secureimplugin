#include "commonheaders.h"

pCNTX cntx = NULL;
int cntx_idx = 0;
int cntx_cnt = 0;
unsigned long thread_timeout = 0;

void __cdecl sttTimeoutThread( LPVOID );

// get context data on context id
pCNTX get_context_on_id(int context) {

    if(	!thread_timeout ) {
	thread_timeout = _beginthread(sttTimeoutThread, 0, 0);
    }
    if( context ) {
	for(int i=0;i<cntx_cnt;i++) {
	    if(cntx[i].cntx == context)
	    		return &(cntx[i]);
	}
	switch( context ) {
		case -1:
		{
			// create context for private pgp keys
			pCNTX tmp = get_context_on_id(cpp_create_context(0));
			tmp->cntx = context; tmp->mode = MODE_PGP;
			tmp->pdata = (PBYTE) mir_alloc(sizeof(PGPDATA));
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

	while(get_context_on_id(++cntx_idx));

	int i;
	for(i=0; i<cntx_cnt && cntx[i].cntx; i++);
	if(i == cntx_cnt) {
		cntx_cnt++; 
		cntx = (pCNTX) mir_realloc(cntx,sizeof(CNTX)*cntx_cnt);
		ZeroMemory(&cntx[i], sizeof(CNTX));
	}
	cntx[i].cntx = cntx_idx;
	cntx[i].mode = mode;
	return cntx_idx;
}

// delete context
void __cdecl cpp_delete_context(int context) {

	pCNTX tmp = get_context_on_id(context);
	if(tmp) {
		cpp_free_keys(tmp);
		tmp->cntx = 0;
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
		ptr->pdata = (PBYTE) mir_alloc(sizeof(PGPDATA));
	    }
	    else
	    if( ptr->mode & MODE_GPG ) {
		ptr->pdata = (PBYTE) mir_alloc(sizeof(GPGDATA));
	    }
	    else
	    if( ptr->mode & MODE_RSA ) {
		pRSADATA p = new RSADATA;
		ptr->pdata = (PBYTE) p;
	    }
	    else {
		ptr->pdata = (PBYTE) mir_alloc(sizeof(SIMDATA));
	    }
	}
	return ptr->pdata;
}

extern void rsa_timeout(int,pRSADATA);

// search not established RSA/AES contexts
void __cdecl sttTimeoutThread( LPVOID ) {

	while(1) {
	    Sleep( 1000 );
	    DWORD time = gettime();
	    for(int i=0;i<cntx_cnt;i++) {
	    	if( cntx[i].cntx>0 && cntx[i].mode&MODE_RSA ) {
			pRSADATA p = (pRSADATA) cntx[i].pdata;
			if( p->time && p->time < time ) {
				rsa_timeout(cntx[i].cntx,p);
			}
	    	}
	    }
	}
}

// EOF 
