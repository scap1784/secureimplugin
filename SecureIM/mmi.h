#ifndef __MMI_H__
#define __MMI_H__

#include "commonheaders.h"

void *m_alloc(size_t);
void m_free(void *);
void *m_realloc(void *,size_t);
char *m_strdup(const char *);
char *m_wwstrcat(LPCSTR,LPCSTR);
char *m_awstrcat(LPCSTR,LPCSTR);
char *m_aastrcat(LPCSTR,LPCSTR);

void *operator new(size_t sz);
void operator delete(void *p);
void *operator new[](size_t size);
void operator delete[](void * p);

#undef mir_alloc
#undef mir_free
#undef mir_realloc
#undef mir_strdup

#define mir_alloc(n)			m_alloc(n)
#define mir_free(p)				m_free(p)
#define mir_realloc(p,n)		m_realloc(p,n)
#define mir_strdup(p)			m_strdup(p)

#define SAFE_INIT(t,p)			t p=NULL;
#define SAFE_FREE(p)			if(p) {mir_free((PVOID)p); p=NULL;}
#define SAFE_DELETE(p)			if(p) {delete p; p=NULL;}

#endif
