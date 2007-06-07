#include "commonheaders.h"

// gzip data
BYTE *cpp_gzip(BYTE *pData, int nLen, int& nCompressedLen) {

	string zipped;
	Gzip gzip(new StringSink(zipped),9);    // 1 is fast, 9 is slow
	gzip.Put(pData, nLen);
	gzip.MessageEnd();

	nCompressedLen = (int) zipped.length();
	BYTE* pCompressed = (BYTE*) mir_alloc(nCompressedLen+1);
	memcpy(pCompressed,zipped.data(),nCompressedLen);

	return pCompressed;
}

// gunzip data
BYTE *cpp_gunzip(BYTE *pCompressedData, int nCompressedLen, int& nLen) {

	string unzipped;
	Gunzip gunzip(new StringSink(unzipped));
	gunzip.Put((BYTE*)pCompressedData,nCompressedLen);
	gunzip.MessageEnd();

	nLen = (int) unzipped.length();
	BYTE* pData = (BYTE*) mir_alloc(nLen+1);
	memcpy(pData,unzipped.data(),nLen);

	return pData;
}
