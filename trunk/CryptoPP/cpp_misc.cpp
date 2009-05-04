#include "commonheaders.h"


int __cdecl cpp_get_features(int context) {
    pCNTX ptr = get_context_on_id(context); if(!ptr) return 0;
    return ptr->features;
}


int __cdecl cpp_get_error(int context) {
    pCNTX ptr = get_context_on_id(context); if(!ptr) return 0;
    return ptr->error;
}


int __cdecl cpp_get_version(void) {
    return __VERSION_DWORD;
}


BOOL cpp_get_simdata(int context, pCNTX *ptr, pSIMDATA *p) {
    *ptr = get_context_on_id(context);
//    if(!*ptr || !(*ptr)->pdata || (*ptr)->mode&(MODE_PGP|MODE_GPG|MODE_RSA)) return FALSE;
    if(!*ptr || (*ptr)->mode&(MODE_PGP|MODE_GPG|MODE_RSA)) return FALSE;
    *p = (pSIMDATA) cpp_alloc_pdata(*ptr);
    return TRUE;
}


int __cdecl cpp_size_keyx(void) {
    return(Tiger::DIGESTSIZE+2);
}


void __cdecl cpp_get_keyx(int context, BYTE *key) {
    pCNTX ptr; pSIMDATA p; if(!cpp_get_simdata(context,&ptr,&p)) return;
    memcpy(key,p->KeyX,Tiger::DIGESTSIZE);
    memcpy(key+Tiger::DIGESTSIZE,&ptr->features,2);
}


void __cdecl cpp_set_keyx(int context, BYTE *key) {
    pCNTX ptr; pSIMDATA p; if(!cpp_get_simdata(context,&ptr,&p)) return;
    SAFE_FREE(p->PubA);
    SAFE_FREE(p->KeyA);
    SAFE_FREE(p->KeyB);
    SAFE_FREE(p->KeyX);
    p->KeyX = (PBYTE) malloc(Tiger::DIGESTSIZE+2);
    memcpy(p->KeyX,key,Tiger::DIGESTSIZE);
    memcpy(&ptr->features,key+Tiger::DIGESTSIZE,2);
}


void __cdecl cpp_get_keyp(int context, BYTE *key) {
    pCNTX ptr; pSIMDATA p; if(!cpp_get_simdata(context,&ptr,&p)) return;
    memcpy(key,p->KeyP,Tiger::DIGESTSIZE);
}


int __cdecl cpp_size_keyp(void) {
    return(Tiger::DIGESTSIZE);
}


void __cdecl cpp_set_keyp(int context, BYTE *key) {
    pCNTX ptr; pSIMDATA p; if(!cpp_get_simdata(context,&ptr,&p)) return;
    SAFE_FREE(p->KeyP);
    p->KeyP = (PBYTE) malloc(Tiger::DIGESTSIZE);
    memcpy(p->KeyP,key,Tiger::DIGESTSIZE);
}


int __cdecl cpp_keya(int context) {
    pCNTX ptr; pSIMDATA p; if(!cpp_get_simdata(context,&ptr,&p)) return 0;
    return p->KeyA!=NULL;
}


int __cdecl cpp_keyb(int context) {
    pCNTX ptr; pSIMDATA p; if(!cpp_get_simdata(context,&ptr,&p)) return 0;
    return p->KeyB!=NULL;
}


int __cdecl cpp_keyx(int context) {
    pCNTX ptr; pSIMDATA p; if(!cpp_get_simdata(context,&ptr,&p)) return 0;
    return p->KeyX!=NULL;
}


int __cdecl cpp_keyp(int context) {
    pCNTX ptr; pSIMDATA p; if(!cpp_get_simdata(context,&ptr,&p)) return 0;
    return p->KeyP!=NULL;
}


// EOF
