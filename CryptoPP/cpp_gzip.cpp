#include "commonheaders.h"

// gzip data
BYTE *cpp_gzip(BYTE *pData, int nLen, int& nCompressedLen) {

	string zipped;
	Gzip gzip(new StringSink(zipped),5);    // 1 is fast, 9 is slow
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
	memcpy(pData,unzipped.c_str(),nLen+1);

	return pData;
}

// zlibc data
string cpp_zlibc(string& pData) {

	string zipped;

	ZlibCompressor zlib(new StringSink(zipped),5);    // 1 is fast, 9 is slow
	zlib.Put((PBYTE)pData.data(), pData.length());
	zlib.MessageEnd();

	return zipped;
}

// zlibd data
string cpp_zlibd(string& pData) {

	string unzipped;

	ZlibDecompressor zlib(new StringSink(unzipped));
	zlib.Put((PBYTE)pData.data(),pData.length());
	zlib.MessageEnd();

	return unzipped;
}

// EOF
