#ifndef __MMI_H__
#define __MMI_H__

#include "commonheaders.h"

char *m_wwstrcat(LPCSTR,LPCSTR);
char *m_awstrcat(LPCSTR,LPCSTR);
char *m_aastrcat(LPCSTR,LPCSTR);

void *operator new(size_t sz);
void operator delete(void *p);
void *operator new[](size_t size);
void operator delete[](void * p);

#define SAFE_INIT(t,p)			t p=NULL;
#define SAFE_FREE(p)			if(p) {mir_free((PVOID)p); p=NULL;}
#define SAFE_DELETE(p)			if(p) {delete p; p=NULL;}

#endif
