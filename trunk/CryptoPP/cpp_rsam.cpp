#include "commonheaders.h"

///////////////////////////////////////////////////////////////////////////

#define RAND_SIZE 32

string tlv(int,const string&);
string tlv(int,const char*);
string tlv(int,int);
string& un_tlv(string&,int&,string&);
string& un_tlv(string&,int&,int&);
void 	un_tlv(PBYTE,int,int&,string&);

string sign(PBYTE,int);
string sign(string&);

string sign128(PBYTE,int);
string sign256(PBYTE,int);

Integer BinaryToInteger(const string&);
string	IntegerToBinary(const Integer&);

AutoSeededRandomPool& GlobalRNG();

void	GenerateRSAKey(unsigned int,string&,string&);
string	RSAEncryptString(const RSA::PublicKey&,const string&);
string	RSADecryptString(const string&,const string&);
string	RSASignString(const string&,const string&);
BOOL	RSAVerifyString(const RSA::PublicKey&,const string&,const string&);

///////////////////////////////////////////////////////////////////////////

int __cdecl rsa_gen_keypair(short);
int __cdecl rsa_get_keypair(short,PBYTE,int*,PBYTE,int*);
int __cdecl rsa_get_keyhash(short,PBYTE,int*,PBYTE,int*);
int __cdecl rsa_set_keypair(short,PBYTE,int,PBYTE,int);
int __cdecl rsa_set_pubkey(int,PBYTE,int);
int __cdecl rsa_get_state(int);
int __cdecl rsa_connect(int);
int __cdecl rsa_disconnect(int);
LPSTR __cdecl rsa_recv(int,LPCSTR);
int __cdecl rsa_send(int,LPCSTR);
int __cdecl rsa_encrypt_file(int,LPCSTR,LPCSTR);
int __cdecl rsa_decrypt_file(int,LPCSTR,LPCSTR);

void inject_msg(int,int,const string&);
string encode_msg(short,pRSADATA,string&);
string decode_msg(pRSADATA,string&);
string encode_rsa(short,pRSADATA,pRSAPRIV,string&);
string decode_rsa(pRSADATA,pRSAPRIV,string&);
string gen_aes_key_iv(short,pRSADATA,pRSAPRIV);
void init_pub(pRSADATA,string&);
void null_msg(int,int,int);

///////////////////////////////////////////////////////////////////////////

RSA_EXPORT exp = {
    rsa_gen_keypair,
    rsa_get_keypair,
    rsa_get_keyhash,
    rsa_set_keypair,
    rsa_set_pubkey,
    rsa_get_state,
    rsa_connect,
    rsa_disconnect,
    rsa_recv,
    rsa_send,
	rsa_encrypt_file,
	rsa_decrypt_file,
    utf8encode,
    utf8decode,
    is_7bit_string,
    is_utf8_string
};

pRSA_IMPORT imp;

string null;

///////////////////////////////////////////////////////////////////////////


int __cdecl rsa_init(pRSA_EXPORT* e, pRSA_IMPORT i) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_init");
#endif
	*e = &exp;
	imp = i;
	return 1;
}


int __cdecl rsa_done(void) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_done");
#endif
	return 1;
}


///////////////////////////////////////////////////////////////////////////


pRSAPRIV rsa_gen_keys(int context) {

	if( context != -2 && context != -3) return 0;

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_gen_keys: %d", context);
#endif
	pCNTX ptr = get_context_on_id(context);
	pRSAPRIV r = (pRSAPRIV) ptr->pdata;

	string priv, pub;
	GenerateRSAKey((context==-3)?4096:2048, priv, pub);

	StringSource pubsrc(pub, true, NULL);
	RSAES_PKCS1v15_Encryptor Encryptor(pubsrc);

	r->priv_k = priv;
	r->priv_s = sign(priv);
	
	pub = tlv(1, IntegerToBinary(Encryptor.GetTrapdoorFunction().GetModulus())) +
	      tlv(2, IntegerToBinary(Encryptor.GetTrapdoorFunction().GetPublicExponent()));
	r->pub_k = pub;
	r->pub_s = sign(pub);

	return r;
}


int __cdecl rsa_gen_keypair(short mode) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_gen_keypair: %d", mode);
#endif
	if( mode&MODE_RSA_2048 )	rsa_gen_keys(-2); // 2048
	if( mode&MODE_RSA_4096 )	rsa_gen_keys(-3); // 4096

	return 1;
}


int __cdecl rsa_get_keypair(short mode, PBYTE privKey, int* privKeyLen, PBYTE pubKey, int* pubKeyLen) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_get_keypair: %d", mode);
#endif
	pCNTX ptr = get_context_on_id((mode&MODE_RSA_4096)?-3:-2);
	pRSAPRIV r = (pRSAPRIV) ptr->pdata;

	*privKeyLen = r->priv_k.length(); r->priv_k.copy((char*)privKey, *privKeyLen);
	*pubKeyLen = r->pub_k.length(); r->pub_k.copy((char*)pubKey, *pubKeyLen);

	return 1;
}


int __cdecl rsa_get_keyhash(short mode, PBYTE privKey, int* privKeyLen, PBYTE pubKey, int* pubKeyLen) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_get_keyhash: %d", mode);
#endif
	pCNTX ptr = get_context_on_id((mode&MODE_RSA_4096)?-3:-2);
	pRSAPRIV r = (pRSAPRIV) ptr->pdata;

	if( privKey ) { *privKeyLen = r->priv_s.length(); r->priv_s.copy((char*)privKey, *privKeyLen); }
	if( pubKey ) { *pubKeyLen = r->pub_s.length(); r->pub_s.copy((char*)pubKey, *pubKeyLen); }

	return 1;
}


int __cdecl rsa_set_keypair(short mode, PBYTE privKey, int privKeyLen, PBYTE pubKey, int pubKeyLen) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_set_keypair: '%s'", privKey);
#endif
	pCNTX ptr = get_context_on_id((mode&MODE_RSA_4096)?-3:-2);
	pRSAPRIV r = (pRSAPRIV) ptr->pdata;

	r->priv_k.assign((char*)privKey, privKeyLen);
	r->pub_k.assign((char*)pubKey, pubKeyLen);

	r->priv_s = sign(r->priv_k);
	r->pub_s = sign(r->pub_k);

	return 1;
}


int __cdecl rsa_set_pubkey(int context, PBYTE pubKey, int pubKeyLen) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_set_pubkey: '%s'", pubKey);
#endif
	pCNTX ptr = get_context_on_id(context);	if(!ptr) return 0;
	pRSADATA p = (pRSADATA) cpp_alloc_pdata(ptr);

	if( pubKey && pubKeyLen ) {
	    string pub;	pub.assign((char*)pubKey, pubKeyLen);
	    init_pub(p,pub);
	}

	return 1;
}


int __cdecl rsa_connect(int context) {

	pCNTX ptr = get_context_on_id(context); if(!ptr) return 0;
	pRSADATA p = (pRSADATA) cpp_alloc_pdata(ptr); if(p->state) return p->state;
	pRSAPRIV r = (pRSAPRIV) (get_context_on_id((ptr->mode&MODE_RSA_4096)?-3:-2))->pdata;

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_connect: '%s' '%s'", r->pub_s.c_str(), p->pub_s.c_str());
#endif
        if(ptr->mode&MODE_RSA_ONLY) {
		inject_msg(context,0x0D,tlv(0,0)+tlv(1,r->pub_k)+tlv(2,p->pub_s));
		p->state = 0x0D;
        }
        else {
		inject_msg(context,0x10,tlv(0,0)+tlv(1,r->pub_s)+tlv(2,p->pub_s));
		p->state = 2;
	}
	p->time = gettime()+10;

	return p->state;
}


int __cdecl rsa_disconnect(int context) {

	pCNTX ptr = get_context_on_id(abs(context)); if(!ptr) return 0;
	pRSADATA p = (pRSADATA) cpp_alloc_pdata(ptr); 
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_disconnect: '%s'", p->pub_s.c_str());
#endif
        if( context<0 ) { // просто послать разрыв по причине "disabled"
		p->state = 0;
		inject_msg(-context,0xFF,null);
//		imp->rsa_notify(-context,-8); // соединение разорвано по причине "disabled"
		return 1;
        }

	if(!p->state) return 1;

	PBYTE buffer=(PBYTE) alloca(RAND_SIZE);
	GlobalRNG().GenerateBlock(buffer,RAND_SIZE);
	inject_msg(context,0xF0,encode_msg(0,p,sign(buffer,RAND_SIZE)));

	p->state = 0;
	imp->rsa_notify(context,-3); // соединение разорвано вручную

	return 1;
}


int __cdecl rsa_get_state(int context) {

	pCNTX ptr = get_context_on_id(context);	if(!ptr) return 0;
	pRSADATA p = (pRSADATA) cpp_alloc_pdata(ptr);

	return p->state;
}


LPSTR __cdecl rsa_recv(int context, LPCSTR msg) {

	pCNTX ptr = get_context_on_id(context);	if(!ptr) return 0;
	pRSADATA p = (pRSADATA) cpp_alloc_pdata(ptr);
	pRSAPRIV r = (pRSAPRIV) (get_context_on_id((ptr->mode&MODE_RSA_4096)?-3:-2))->pdata;

	int len = strlen(msg); PBYTE buf = (PBYTE)base64decode(msg,&len);
	string data; int type; un_tlv(buf,len,type,data);

#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_recv(%02x): [%d] '%s'", type, p->state, data.c_str());
#endif
        if( type>0x10 && type<0xE0 ) 
	    if( p->state==0 || p->state!=(type>>4) ) {
		// шлем перерывание сессии
		p->state=0; p->time=0;
		null_msg(context,0x00,-1); // сессия разорвана по ошибке, неверный тип сообщения
		return 0;
	    }

	int t[4];

	switch( type ) {

	case 0x00:
	{
		if( p->state == 0 || p->state == 7 ) return 0;
		p->state=0; p->time=0;
		imp->rsa_notify(context,-2); // сессия разорвана по ошибке другой стороной
	} break;

	case 0x10:
	{
		int features; string sha1,sha2;
		un_tlv(un_tlv(un_tlv(data,t[0],features),t[1],sha1),t[2],sha2);
		BOOL lr = (p->pub_s==sha1); BOOL ll = (r->pub_s==sha2);
		switch( (lr<<4)|ll ){
		case 0x11: { // оба паблика совпали
			inject_msg(context,0x21,gen_aes_key_iv(ptr->mode,p,r));
			p->state=5;
		} break;
		case 0x10: { // совпал удаленный паблик
			inject_msg(context,0x22,tlv(0,features)+tlv(1,r->pub_k)+tlv(2,r->pub_s));
			p->state=3;
		} break;
		case 0x01: { // совпал локальный паблик
			inject_msg(context,0x23,tlv(0,features));
			p->state=3;
		} break;
		case 0x00: { // не совпали оба паблика
			inject_msg(context,0x24,tlv(0,features)+tlv(1,r->pub_k)+tlv(2,r->pub_s));
			p->state=3;
		} break;
		}
	} break;

	case 0x22: // получили удаленный паблик, отправляем уже криптоключ
	{
		int features; string pub;
		un_tlv(un_tlv(data,t[0],features),t[1],pub);
		string sig = sign(pub);
		if( !imp->rsa_check_pub(context,(PBYTE)pub.data(),pub.length(),(PBYTE)sig.data(),sig.length()) ) {
			p->state=0; p->time=0;
			null_msg(context,0x00,-type); // сессия разорвана по ошибке
			return 0;
		}
		init_pub(p,pub);
		if( p->state==0 ) { // timeout
			rsa_connect(context);
			return 0;
		}
		inject_msg(context,0x32,gen_aes_key_iv(ptr->mode,p,r));
		p->state=5;
	} break;

	case 0x23: // отправляем локальный паблик
	{
		int features;
		un_tlv(data,t[0],features);
		inject_msg(context,0x33,tlv(1,r->pub_k)+tlv(2,r->pub_s));
		p->state=4;
	} break;

	case 0x24: // получили удаленный паблик, отправим локальный паблик
	{
		int features; string pub;
		un_tlv(un_tlv(data,t[0],features),t[1],pub);
		string sig = sign(pub);
		if( !imp->rsa_check_pub(context,(PBYTE)pub.data(),pub.length(),(PBYTE)sig.data(),sig.length()) ) {
			p->state=0; p->time=0;
			null_msg(context,0x00,-type); // сессия разорвана по ошибке
			return 0;
		}
		init_pub(p,pub);
		if( p->state==0 ) { // timeout
			rsa_connect(context);
			return 0;
		}
		inject_msg(context,0x34,tlv(1,r->pub_k)+tlv(2,r->pub_s));
		p->state=4;
	} break;

	case 0x33: // получили удаленный паблик, отправляем криптоключ
	case 0x34:
	{
		string pub;
		un_tlv(data,t[0],pub);
		string sig = sign(pub);
		if( !imp->rsa_check_pub(context,(PBYTE)pub.data(),pub.length(),(PBYTE)sig.data(),sig.length()) ) {
			p->state=0; p->time=0;
			null_msg(context,0x00,-type); // сессия разорвана по ошибке
			return 0;
		}
		init_pub(p,pub);
		if( p->state==0 ) { // timeout
			rsa_connect(context);
			return 0;
		}
		inject_msg(context,0x40,gen_aes_key_iv(ptr->mode,p,r));
		p->state=5;
	} break;

	case 0x21: // получили криптоключ, отправляем криптотест
	case 0x32:
	case 0x40:
	{
		string key = decode_rsa(p,r,data);
		if( !key.length() ) {
			p->state=0; p->time=0;
			null_msg(context,0x00,-type); // сессия разорвана по ошибке
			return 0;
		}
		un_tlv(un_tlv(key,t[0],p->aes_k),t[1],p->aes_v);
		PBYTE buffer=(PBYTE) alloca(RAND_SIZE);
		GlobalRNG().GenerateBlock(buffer,RAND_SIZE);
		inject_msg(context,0x50,encode_msg(0,p,sign(buffer,RAND_SIZE)));
		p->state=6;
	} break;

	case 0x50: // получили криптотест, отправляем свой криптотест
	{
		string msg = decode_msg(p,data);
		if( msg.length() == 0 ) {
			p->state=0; p->time=0;
			null_msg(context,0x00,-type); // сессия разорвана по ошибке
			return 0;
		}
		PBYTE buffer=(PBYTE) alloca(RAND_SIZE);
		GlobalRNG().GenerateBlock(buffer,RAND_SIZE);
        	inject_msg(context,0x60,encode_msg(0,p,sign(buffer,RAND_SIZE)));
		p->state=7; p->time=0;
		imp->rsa_notify(context,1);	// заебися, криптосессия установлена
	} break;

	case 0x60: // получили криптотест, сессия установлена
	{
		string msg = decode_msg(p,data);
		if( msg.length() == 0 ) {
			p->state=0; p->time=0;
			null_msg(context,0x00,-type); // сессия разорвана по ошибке
			return 0;
		}
		p->state=7; p->time=0;
		imp->rsa_notify(context,1);	// заебися, криптосессия установлена
	} break;

	case 0x70: // получили AES сообщение, декодируем
	{
		SAFE_FREE(ptr->tmp);
		string msg = decode_msg(p,data);
		if( msg.length() ) {
			ptr->tmp = (LPSTR) mir_alloc(msg.length()+1);
			memcpy(ptr->tmp,msg.c_str(),msg.length()+1);
		}
		else {
			imp->rsa_notify(context,-5); // ошибка декодирования AES сообщения
		}
		return ptr->tmp;
	} break;

	case 0x0D: // запрос паблика
	case 0xD0: // ответ пабликом
	{
		int features; string pub,sha;
		un_tlv(un_tlv(un_tlv(data,t[0],features),t[1],pub),t[2],sha);
		if( p->pub_k!=pub ) { // пришел новый паблик
			string sig = sign(pub);
			if( !imp->rsa_check_pub(context,(PBYTE)pub.data(),pub.length(),(PBYTE)sig.data(),sig.length()) ) {
				p->state=0; p->time=0;
				null_msg(context,0x00,-type); // сессия разорвана по ошибке
				return 0;
			}
			init_pub(p,pub);
			if( p->state==0 ) { // timeout
				rsa_connect(context);
				return 0;
			}
		}
		if( type == 0x0D ) { // нужно отправить мой паблик
			inject_msg(context,0xD0,tlv(0,features)+tlv(1,r->pub_k)+tlv(2,p->pub_s));
		}
		p->state=0; p->time=0;
	} break;

	case 0xE0: // получили RSA сообщение, декодируем
	{
		SAFE_FREE(ptr->tmp);
		string msg = decode_rsa(p,r,data);
		if( msg.length() ) {
			ptr->tmp = (LPSTR) mir_alloc(msg.length()+1);
			memcpy(ptr->tmp,msg.c_str(),msg.length()+1);
		}
		else {
			imp->rsa_notify(context,-6); // ошибка декодирования RSA сообщения
		}
		return ptr->tmp;
	} break;

	case 0xF0: // разрыв соединения
	{
		if( p->state != 7 ) return 0;
		string msg = decode_msg(p,data);
		if( msg.length() == 0 ) return 0;
		p->state=0;
		imp->rsa_notify(context,-4); // соединение разорвано вручную другой стороной
	} break;

	case 0xFF: // разрыв соединения по причине "disabled"
	{
		p->state=0;
		imp->rsa_notify(context,-8); // соединение разорвано по причине "disabled"
	} break;
	}
	if( p->state != 0 && p->state != 7 ) p->time = gettime()+10;
	return 0;
}


int __cdecl rsa_send(int context, LPCSTR msg) {

	pCNTX ptr = get_context_on_id(context);	if(!ptr) return 0;
	pRSADATA p = (pRSADATA) cpp_alloc_pdata(ptr); if(p->state!=0 && p->state!=7) return 0;

	if( p->state == 7 ) // сессия установлена, шифруем AES и отправляем
		inject_msg(context,0x70,encode_msg(1,p,string(msg)));
	else
	if( p->state == 0 ) { // сессия  установлена, отправляем RSA сообщение
		if( !p->pub_k.length() ) return 0;
		// есть паблик ключ - отправим сообщение
		pRSAPRIV r = (pRSAPRIV) (get_context_on_id((ptr->mode&MODE_RSA_4096)?-3:-2))->pdata;
		inject_msg(context,0xE0,encode_rsa(1,p,r,string(msg)));
	}

	return 1;
}


void inject_msg(int context, int type, const string& msg) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("inject_msg(%02x): '%s'", type, msg.c_str());
#endif
	string txt=tlv(type,msg);
	char* base64=base64encode(txt.data(),txt.length());
	imp->rsa_inject(context,(LPCSTR)base64);
	mir_free(base64);
}


string encode_msg(short z,pRSADATA p, string& msg) {

	string zlib = (z) ? cpp_zlibc(msg) : msg;
	string sig = sign(zlib);

	string ciphered;
	try {
		CBC_Mode<AES>::Encryption enc((PBYTE)p->aes_k.data(),p->aes_k.length(),(PBYTE)p->aes_v.data());
		StreamTransformationFilter cbcEncryptor(enc,new StringSink(ciphered));
		cbcEncryptor.Put((PBYTE)zlib.data(),zlib.length());
		cbcEncryptor.MessageEnd();
	}
	catch (...) {
		;
	}
	return tlv(z,ciphered)+tlv(2,sig);
}


string decode_msg(pRSADATA p, string& msg) {

	string ciphered,sig; int t1,t2;
	un_tlv(un_tlv(msg,t1,ciphered),t2,sig);

	string unciphered,zlib;
	try {
		CBC_Mode<AES>::Decryption dec((PBYTE)p->aes_k.data(),p->aes_k.length(),(PBYTE)p->aes_v.data());
		StreamTransformationFilter cbcDecryptor(dec,new StringSink(zlib));
		cbcDecryptor.Put((PBYTE)ciphered.data(),ciphered.length());
		cbcDecryptor.MessageEnd();

		if( sig == sign(zlib) ) {
			unciphered = (t1==1) ? cpp_zlibd(zlib) : zlib;
		}
	}
	catch (...) {
		;
	}
	return unciphered;
}


string encode_rsa(short z,pRSADATA p,pRSAPRIV r,string& msg) {

	string zlib = (z) ? cpp_zlibc(msg) : msg;

	string enc = RSAEncryptString(p->pub,zlib);
	string sig = RSASignString(r->priv_k,zlib);

	return tlv(z,enc)+tlv(2,sig);
}


string decode_rsa(pRSADATA p,pRSAPRIV r,string& msg) {

	string ciphered,sig; int t1,t2;
	un_tlv(un_tlv(msg,t1,ciphered),t2,sig);

	string unciphered,zlib;
	zlib = RSADecryptString(r->priv_k,ciphered);
	if( zlib.length() && RSAVerifyString(p->pub,zlib,sig) ) {
		unciphered = (t1==1) ? cpp_zlibd(zlib) : zlib;
	}
	return unciphered;
}


string gen_aes_key_iv(short m,pRSADATA p,pRSAPRIV r) {

	PBYTE buffer=(PBYTE) alloca(RAND_SIZE);

	GlobalRNG().GenerateBlock(buffer, RAND_SIZE);
	p->aes_k = ( m&MODE_RSA_4096 ) ? sign256(buffer,RAND_SIZE) : sign128(buffer,RAND_SIZE);
	
	GlobalRNG().GenerateBlock(buffer, RAND_SIZE);
	p->aes_v = ( m&MODE_RSA_4096 ) ? sign256(buffer,RAND_SIZE) : sign128(buffer,RAND_SIZE);

	string buf = tlv(10,p->aes_k)+tlv(11,p->aes_v);
//	string enc = RSAEncryptString(p->pub,buf);
//	string sig = RSASignString(r->priv_k,buf);
//	return tlv(1,enc)+tlv(2,sig);

	return encode_rsa(0,p,r,buf);
}


void init_pub(pRSADATA p, string& pub) {

	p->pub_k = pub;
	p->pub_s = sign(pub);

	int t; string mod, exp;
	un_tlv(un_tlv(pub,t,mod),t,exp);
	p->pub.Initialize(BinaryToInteger(mod),BinaryToInteger(exp));
}


void null_msg(int context, int type, int status) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("null_msg: '%02x'", status);
#endif
	inject_msg(context,type,null);
	imp->rsa_notify(context,status);
}


void rsa_timeout(int context, pRSADATA p) {
#if defined(_DEBUG) || defined(NETLIB_LOG)
	Sent_NetLog("rsa_timeout");
#endif
	p->state=0; p->time=0;
//	null_msg(context,0x00,-7);
	imp->rsa_notify(context,-7); // сессия разорвана по таймауту
}


int __cdecl rsa_encrypt_file(int context,LPCSTR file_in,LPCSTR file_out) {

	pCNTX ptr = get_context_on_id(context);	if(!ptr) return 0;
	pRSADATA p = (pRSADATA) cpp_alloc_pdata(ptr); if(p->state!=7) return 0;

	try {
		CBC_Mode<AES>::Encryption enc((PBYTE)p->aes_k.data(),p->aes_k.length(),(PBYTE)p->aes_v.data());
		FileSource *f = new FileSource(file_in,true,new StreamTransformationFilter (enc,new FileSink(file_out)));
		delete f;
	}
	catch (...) {
		return 0;
	}
	return 1;
}


int __cdecl rsa_decrypt_file(int context,LPCSTR file_in,LPCSTR file_out) {

	pCNTX ptr = get_context_on_id(context);	if(!ptr) return 0;
	pRSADATA p = (pRSADATA) cpp_alloc_pdata(ptr); if(p->state!=7) return 0;

	try {
		CBC_Mode<AES>::Decryption dec((PBYTE)p->aes_k.data(),p->aes_k.length(),(PBYTE)p->aes_v.data());
		FileSource *f = new FileSource(file_in,true,new StreamTransformationFilter (dec,new FileSink(file_out)));
		delete f;
	}
	catch (...) {
		return 0;
	}
	return 1;
}


// EOF
