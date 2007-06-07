#include "commonheaders.h"

const unsigned char IV[] = "SIMhell@MIRANDA!";

// encrypt string using KeyX, return encoded string as ASCII or NULL
LPSTR __cdecl cpp_encrypt(pCNTX ptr, LPCSTR szPlainMsg) {

   	ptr->error = ERROR_NONE;
	pSIMDATA p = (pSIMDATA) ptr->pdata;

	BYTE dataflag = 0;
	int clen,slen;
	slen = strlen(szPlainMsg);

	LPSTR szMsg;
	if(ptr->features & FEATURES_GZIP) {
		szMsg = (LPSTR) cpp_gzip((BYTE*)szPlainMsg,slen,clen);
		if(clen>=slen) {
			mir_free(szMsg);
		    szMsg = mir_strdup(szPlainMsg);
		}
		else {
			slen = clen;
			dataflag |= DATA_GZIP;
		}
	}
	else
	    szMsg = mir_strdup(szPlainMsg);

	string ciphered;

	CBC_Mode<AES>::Encryption enc(p->KeyX,Tiger::DIGESTSIZE,IV);
	StreamTransformationFilter cbcEncryptor(enc,new StringSink(ciphered));

	cbcEncryptor.Put((BYTE*)szMsg, slen);
	cbcEncryptor.MessageEnd();

	mir_free(szMsg);

	clen = (int) ciphered.length();
	if(ptr->features & FEATURES_CRC32) {
		BYTE crc32[CRC32::DIGESTSIZE];
		CRC32().CalculateDigest(crc32, (BYTE*)ciphered.data(), clen);
		ciphered.insert(0,(char*)&crc32,CRC32::DIGESTSIZE);
		ciphered.insert(0,(char*)&clen,2);
	}
	if(ptr->features & FEATURES_GZIP) {
		ciphered.insert(0,(char*)&dataflag,1);
	}
	clen = (int) ciphered.length();

	SAFE_FREE(ptr->tmp);
	if(ptr->features & FEATURES_BASE64) {
//MessageBoxA(0,"base64","keyB",MB_OK | MB_ICONSTOP);
		ptr->tmp = base64encode(ciphered.data(),clen);
	}
	else {
		ptr->tmp = base16encode(ciphered.data(),clen);
	}

	return ptr->tmp;
}

// decrypt string using KeyX, return decoded string as ASCII or NULL
LPSTR __cdecl cpp_decrypt(pCNTX ptr, LPCSTR szEncMsg) {

	try {
    	ptr->error = ERROR_SEH;
		pSIMDATA p = (pSIMDATA) ptr->pdata;

		int clen = strlen(szEncMsg);

		LPSTR ciphered;
		if(ptr->features & FEATURES_BASE64)
			ciphered = base64decode(szEncMsg,&clen);
		else
			ciphered = base16decode(szEncMsg,&clen);

		BYTE dataflag=0;
		if(ptr->features & FEATURES_GZIP) {
			dataflag = *ciphered;
			clen--;
			memcpy(ciphered,ciphered+1,clen);
		}
		if(ptr->features & FEATURES_CRC32) {
			int len;
			__asm {
				mov esi,[ciphered];
				xor eax,eax;
				mov ax,word ptr [esi];
				mov [len], eax;
			}
			if(len != clen-(2+CRC32::DIGESTSIZE)) {
				// mesage not full
				mir_free(ciphered);
	    		ptr->error = ERROR_BAD_LEN;
				return NULL;
			}
			BYTE crc32[CRC32::DIGESTSIZE];
			CRC32().CalculateDigest(crc32, (BYTE*)(ciphered+2+CRC32::DIGESTSIZE), len);
			if(memcmp(crc32,ciphered+2,CRC32::DIGESTSIZE)) {
				// message is bad
				mir_free(ciphered);
	    		ptr->error = ERROR_BAD_CRC;
				return NULL;
			}
			clen = len;
			memcpy(ciphered,ciphered+2+CRC32::DIGESTSIZE,clen);
		}

		string unciphered;

		CBC_Mode<AES>::Decryption dec(p->KeyX,Tiger::DIGESTSIZE,IV);
		StreamTransformationFilter cbcDecryptor(dec,new StringSink(unciphered));

		cbcDecryptor.Put((BYTE*)ciphered,clen);
		cbcDecryptor.MessageEnd();

		SAFE_FREE(ciphered);
		SAFE_FREE(ptr->tmp);

		if(dataflag & DATA_GZIP) {
			int slen;
			ptr->tmp = (LPSTR) cpp_gunzip((BYTE*)unciphered.data(),unciphered.length(),slen);
		}
		else {
			ptr->tmp = (LPSTR) mir_alloc(unciphered.length()+1);
			memcpy(ptr->tmp,unciphered.data(),unciphered.length()+1);
		}

		ptr->error = ERROR_NONE;
		return ptr->tmp;
	}
	catch (...)	{
		return NULL;
	}
}

// encode message from ANSI into UTF8 if need
LPSTR __cdecl cpp_encodeA(int context, LPCSTR msg) {

    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return NULL;
	cpp_alloc_pdata(ptr); pSIMDATA p = (pSIMDATA) ptr->pdata;
    if(!p->KeyX) { ptr->error = ERROR_NO_KEYX; return NULL; }

	LPSTR szNewMsg = NULL;
	LPSTR szOldMsg = (LPSTR) msg;

	if(ptr->features & FEATURES_UTF8) {
		// ansi message: convert to unicode->utf-8 and encrypt.
		int slen = strlen(szOldMsg)+1;
		LPWSTR wstring = (LPWSTR) alloca(slen*sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, szOldMsg, -1, wstring, slen*sizeof(WCHAR));
		LPSTR szUtfMsg = utf8encode(wstring);
		// encrypt
		szNewMsg = cpp_encrypt(ptr, szUtfMsg);
		mir_free(szUtfMsg);
	}
	else {
		// ansi message: encrypt.
		szNewMsg = cpp_encrypt(ptr, szOldMsg);
	}

	return szNewMsg;
}

// encode message from UNICODE into UTF8 if need
LPSTR __cdecl cpp_encodeW(int context, LPWSTR msg) {

    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return NULL;
	cpp_alloc_pdata(ptr); pSIMDATA p = (pSIMDATA) ptr->pdata;
    if(!p->KeyX) { ptr->error = ERROR_NO_KEYX; return NULL; }

	LPSTR szNewMsg = NULL;
	LPSTR szOldMsg = (LPSTR) msg;

	if(ptr->features & FEATURES_UTF8) {
		// unicode message: convert to utf-8 and encrypt.
		LPSTR szUtfMsg = utf8encode((LPWSTR)szOldMsg);
		szNewMsg = cpp_encrypt(ptr, szUtfMsg);
		mir_free(szUtfMsg);
	}
	else {
		// unicode message: convert to ansi and encrypt.
		int wlen = wcslen((LPWSTR)szOldMsg)+1;
		LPSTR astring = (LPSTR) alloca(wlen);
		WideCharToMultiByte(CP_ACP, 0, (LPWSTR)szOldMsg, -1, astring, wlen, 0, 0);
		szNewMsg = cpp_encrypt(ptr, astring);
	}

	return szNewMsg;
}

// decode message from UTF8 if need, return ANSIzUCS2z
LPSTR __cdecl cpp_decode(int context, LPCSTR szEncMsg) {

    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return NULL;
	cpp_alloc_pdata(ptr); pSIMDATA p = (pSIMDATA) ptr->pdata;
    if(!p->KeyX) { ptr->error = ERROR_NO_KEYX; return NULL; }

	LPSTR szNewMsg = NULL;
	LPSTR szOldMsg = cpp_decrypt(ptr, szEncMsg);

	if(szOldMsg) {
		if(ptr->features & FEATURES_UTF8) {
			// utf8 message: convert to unicode -> ansii
			WCHAR *wstring = utf8decode(szOldMsg);
			int wlen = wcslen(wstring)+1;
			szNewMsg = (LPSTR) mir_alloc(wlen*(sizeof(WCHAR)+2));				// work.zy@gmail.com
			WideCharToMultiByte(CP_ACP, 0, wstring, -1, szNewMsg, wlen, 0, 0);
			memcpy(szNewMsg+strlen(szNewMsg)+1, wstring, wlen*sizeof(WCHAR));	// work.zy@gmail.com
			mir_free(wstring);
		}
		else {
			// ansi message: convert to unicode
			int slen = strlen(szOldMsg)+1;
			szNewMsg = (LPSTR) mir_alloc(slen*(sizeof(WCHAR)+1));
			memcpy(szNewMsg,szOldMsg,slen);
			WCHAR* wstring = (LPWSTR) alloca(slen*sizeof(WCHAR));
			MultiByteToWideChar(CP_ACP, 0, szOldMsg, -1, wstring, slen*sizeof(WCHAR));
			memcpy(szNewMsg+slen,wstring,slen*sizeof(WCHAR));
		}
	}
	SAFE_FREE(ptr->tmp);
	ptr->tmp = szNewMsg;
	return szNewMsg;
}

int __cdecl cpp_encrypt_file(int context,LPCSTR file_in,LPCSTR file_out) {

    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return 0;
	cpp_alloc_pdata(ptr); pSIMDATA p = (pSIMDATA) ptr->pdata;
    if(!p->KeyX) return 0;

	CBC_Mode<AES>::Encryption enc(p->KeyX,Tiger::DIGESTSIZE,IV);
	FileSource *f = new FileSource(file_in,true,new StreamTransformationFilter (enc,new FileSink(file_out)));
	delete f;
	return 1;
}

int __cdecl cpp_decrypt_file(int context,LPCSTR file_in,LPCSTR file_out) {

    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return 0;
	cpp_alloc_pdata(ptr); pSIMDATA p = (pSIMDATA) ptr->pdata;
    if(!p->KeyX) return 0;

	CBC_Mode<AES>::Decryption dec(p->KeyX,Tiger::DIGESTSIZE,IV);
	FileSource *f = new FileSource(file_in,true,new StreamTransformationFilter (dec,new FileSink(file_out)));
	delete f;
	return 1;
}

