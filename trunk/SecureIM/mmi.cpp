#include "commonheaders.h"


void *operator new(size_t sz) {
	return mir_alloc(sz);
}


void operator delete(void *p) {
	mir_free(p);
}


void *operator new[](size_t size) {
	return operator new(size);
}


void operator delete[](void * p) {
	operator delete(p);
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
