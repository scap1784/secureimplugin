#include "commonheaders.h"


void m_check(void *ptr, const char *module) {
    if(ptr==NULL) {
    	char buffer[128];
    	strcpy(buffer,module);
    	strcat(buffer,": NULL pointer detected !");
		MessageBox(0,buffer,szModuleName,MB_OK|MB_ICONSTOP);
#ifdef _MSC_VER
		__asm{ int 3 };
#endif		
		exit(1);
    }
}


void *m_alloc(size_t size) {
    void *ptr = memoryManagerInterface.mmi_malloc(size);
    m_check(ptr,"m_alloc");
   	ZeroMemory(ptr,size);
	return ptr;
}


void m_free(void *ptr) {
//    m_check(ptr,"m_free");
    if(ptr) memoryManagerInterface.mmi_free(ptr);
}


void *m_realloc(void *ptr,size_t size) {
    ptr = memoryManagerInterface.mmi_realloc(ptr,size);
    m_check(ptr,"m_realloc");
	return ptr;
}


void *operator new(size_t sz) {
	return m_alloc(sz);
}


void operator delete(void *p) {
	m_free(p);
}


void *operator new[](size_t size) {
	return operator new(size);
}


void operator delete[](void * p) {
	operator delete(p);
}


char *m_strdup(const char *str) {
    if(str==NULL) return NULL;
//    m_check((void*)str,"m_strdup");
    int len = strlen(str)+1;
	char *dup = (char*) m_alloc(len);
	memcpy((void*)dup,(void*)str,len);
	return dup;
}


// ANSIzUCS2z + ANSIzUCS2z = ANSIzUCS2z
char *m_wwstrcat(LPCSTR strA, LPCSTR strB) {
	int lenA = strlen(strA);
	int lenB = strlen(strB);
	LPSTR str = (LPSTR)mir_alloc((lenA+lenB+1)*(sizeof(WCHAR)+1));
	memcpy(str,									strA,			lenA);
	memcpy(str+lenA,							strB,			lenB+1);
	memcpy(str+lenA+lenB+1,						strA+lenA+1,	lenA*sizeof(WCHAR));
	memcpy(str+lenA+lenB+1+lenA*sizeof(WCHAR),	strB+lenB+1,	(lenB+1)*sizeof(WCHAR));
	return str;
}


// ANSIz + ANSIzUCS2z = ANSIzUCS2z
char *m_awstrcat(LPCSTR strA, LPCSTR strB) {
	int lenA = strlen(strA);
	LPSTR tmpA = (LPSTR)mir_alloc((lenA+1)*(sizeof(WCHAR)+1));
	strcpy(tmpA, strA);
	MultiByteToWideChar(CP_ACP, 0, strA, -1, (LPWSTR)(tmpA+lenA+1), (lenA+1)*sizeof(WCHAR));
	LPSTR str = m_wwstrcat(tmpA, strB);
	mir_free(tmpA);
	return str;
}


// ANSIz + ANSIz = ANSIzUCS2z
char *m_aastrcat(LPCSTR strA, LPCSTR strB) {
	int lenA = strlen(strA);
	int lenB = strlen(strB);
	LPSTR str = (LPSTR)mir_alloc((lenA+lenB+1)*(sizeof(WCHAR)+1));
	strcpy(str,strA);
	strcat(str,strB);
	MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)(str+lenA+lenB+1), (lenA+lenB+1)*sizeof(WCHAR));
	return str;
}

// EOF
