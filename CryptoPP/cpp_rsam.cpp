#include "commonheaders.h"

///////////////////////////////////////////////////////////////////////////

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

int rsa_gen_keypair(short);
int rsa_get_keypair(short,PBYTE,int*,PBYTE,int*);
int rsa_set_keypair(short,PBYTE,int,PBYTE,int);
int rsa_connect(int,PBYTE,int);
int rsa_disconnect(int);
LPSTR rsa_recv(int,LPCSTR);
int rsa_send(int,LPCSTR);

void inject_msg(int,int,const string&);
string encode_msg(pRSADATA,string&);
string encode_msg(pRSADATA,LPCSTR);
string encode_msg(pRSADATA,LPCSTR,int);
string decode_msg(pRSADATA,string&);
string gen_aes_key_iv(short,pRSADATA,pRSAPRIV);
void init_pub(pRSADATA,string&);
void null_msg(int,int,int);

///////////////////////////////////////////////////////////////////////////

RSA_EXPORT exp = {
    rsa_gen_keypair,
    rsa_get_keypair,
    rsa_set_keypair,
    rsa_connect,
    rsa_disconnect,
    rsa_recv,
    rsa_send
};

pRSA_IMPORT imp;

string null;

///////////////////////////////////////////////////////////////////////////


int __cdecl rsa_init(pRSA_EXPORT* e, pRSA_IMPORT i) {
	*e = &exp;
	imp = i;
	return 1;
}


int __cdecl rsa_done(void) {
	return 1;
}


///////////////////////////////////////////////////////////////////////////


pRSAPRIV rsa_gen_keys(int context) {

    if( context != -2 && context != -3) return 0;

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


int rsa_gen_keypair(short mode) {

    if( mode&MODE_RSA_2048 )	rsa_gen_keys(-2); // 2048
	if( mode&MODE_RSA_4096 )	rsa_gen_keys(-3); // 4096

	return 1;
}


int rsa_get_keypair(short mode, PBYTE privKey, int* privKeyLen, PBYTE pubKey, int* pubKeyLen) {

    pCNTX ptr = get_context_on_id((mode&MODE_RSA_4096)?-3:-2);
	pRSAPRIV r = (pRSAPRIV) ptr->pdata;

	*privKeyLen = r->priv_k.length(); if(privKey) r->priv_k.copy((char*)privKey, *privKeyLen);
	*pubKeyLen = r->pub_k.length(); if(pubKey) r->pub_k.copy((char*)pubKey, *pubKeyLen);

	return 1;
}


int rsa_set_keypair(short mode, PBYTE privKey, int privKeyLen, PBYTE pubKey, int pubKeyLen) {

    pCNTX ptr = get_context_on_id((mode&MODE_RSA_4096)?-3:-2);
	pRSAPRIV r = (pRSAPRIV) ptr->pdata;

	r->priv_k.assign((char*)privKey, privKeyLen);
	r->pub_k.assign((char*)pubKey, pubKeyLen);

	r->priv_s = sign(r->priv_k);
	r->pub_s = sign(r->pub_k);

	return 1;
}


int rsa_connect(int context, PBYTE pubKey, int pubKeyLen) {

    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return 0;
	cpp_alloc_pdata(ptr); pRSADATA p = (pRSADATA) ptr->pdata;
	if(p->state) return p->state;

	string pub;	pub.assign((char*)pubKey, pubKeyLen);
	init_pub(p,pub);

	pRSAPRIV r = (pRSAPRIV) (get_context_on_id((ptr->mode&MODE_RSA_4096)?-3:-2))->pdata;

	inject_msg(context,0x10,tlv(0,0)+tlv(1,r->pub_s)+tlv(2,p->pub_s));
	p->state = 2;

	return p->state;
}


int rsa_disconnect(int context) {

    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return 0;
	cpp_alloc_pdata(ptr); pRSADATA p = (pRSADATA) ptr->pdata;
	if(!p->state) return 1;

	null_msg(context,0xF0,-3); // соединение разорвано вручную
	p->state = 0;

	return 1;
}


LPSTR rsa_recv(int context, LPCSTR msg) {

    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return 0;
	cpp_alloc_pdata(ptr); pRSADATA p = (pRSADATA) ptr->pdata;
	pRSAPRIV r = (pRSAPRIV) (get_context_on_id((ptr->mode&MODE_RSA_4096)?-3:-2))->pdata;

	int len = strlen(msg); PBYTE buf = (PBYTE)base64decode(msg,&len);
	string data; int type; un_tlv(buf,len,type,data);

	if( p->state && type>0x00 && type<0xF0 && p->state!=(type>>8) ) {
		// шлем перерывание сессии
		null_msg(context,0x00,-1); // сессия разорвана по ошибке
		p->state=0;
		return 0;
	}

	int t[3];

	switch( type ) {

	case 0x00:
	{
		imp->rsa_notify(context,-2); // сессия разорвана по ошибке другой стороной
		p->state=0;
	} break;

	case 0x10:
	{
		int features; string sha1,sha2;
		un_tlv(un_tlv(un_tlv(data,t[0],features),t[1],sha1),t[2],sha2);
		BOOL lr = (p->pub_s==sha1); BOOL ll = (r->pub_s==sha2);
		switch( (ll<<8)|lr ){
		case 0x11: {
			inject_msg(context,0x21,gen_aes_key_iv(ptr->mode,p,r));
			p->state=5;
		} break;
		case 0x10: {
			inject_msg(context,0x22,tlv(0,features)+tlv(1,r->pub_k)+tlv(2,r->pub_s));
			p->state=3;
		} break;
		case 0x01: {
			inject_msg(context,0x23,tlv(0,features)+tlv(1,p->pub_s));
			p->state=3;
		} break;
		default: {
			inject_msg(context,0x24,tlv(0,features)+tlv(1,r->pub_k)+tlv(2,r->pub_s));
			p->state=3;
		} break;
		}
	} break;

	case 0x21:
	case 0x32:
	case 0x40:
	{
		string enc,sig;	un_tlv(un_tlv(data,t[0],enc),t[1],sig);
		string key = RSADecryptString(r->priv_k,enc);
		if( !key.length() || !RSAVerifyString(p->pub,key,sig) ) {
			null_msg(context,0x00,-1); // сессия разорвана по ошибке
			p->state=0;
			return 0;
		}
		un_tlv(un_tlv(key,t[0],p->aes_k),t[1],p->aes_v);
		#undef BUFSIZE
		#define BUFSIZE 512
		PBYTE buffer=(PBYTE) mir_alloc(BUFSIZE);
		GlobalRNG().GenerateBlock(buffer, BUFSIZE);
        inject_msg(context,0x50,encode_msg(p,sign(buffer,BUFSIZE)));
		mir_free(buffer);
		p->state=6;
	} break;

	case 0x22:
	case 0x33:
	case 0x34:
	{
		int features; string pub;
		un_tlv(un_tlv(data,t[0],features),t[1],pub);
		string sig = sign(pub);
		if( !imp->rsa_check_pub(context,(PBYTE)pub.data(),pub.length(),(PBYTE)sig.data(),sig.length()) ) {
			null_msg(context,0x00,-1); // сессия разорвана по ошибке
			p->state=0;
			return 0;
		}
		init_pub(p,pub);
		inject_msg(context,0x32,gen_aes_key_iv(ptr->mode,p,r));
		p->state=5;
	} break;

	case 0x23: {
		int features;
		un_tlv(data,t[0],features);
		inject_msg(context,0x33,tlv(1,r->pub_k)+tlv(2,r->pub_s));
		p->state=4;
	} break;

	case 0x24: {
		int features; string pub;
		un_tlv(un_tlv(data,t[0],features),t[1],pub);
		string sig = sign(pub);
		if( !imp->rsa_check_pub(context,(PBYTE)pub.data(),pub.length(),(PBYTE)sig.data(),sig.length()) ) {
			null_msg(context,0x00,-1); // сессия разорвана по ошибке
			p->state=0;
			return 0;
		}
		init_pub(p,pub);
		inject_msg(context,0x34,tlv(1,r->pub_k)+tlv(2,r->pub_s));
		p->state=4;
	} break;

	case 0x50:
	{
		string msg = decode_msg(p,data);
		if( msg.length() == 0 ) {
			null_msg(context,0x00,-1); // сессия разорвана по ошибке
			p->state=0;
			return 0;
		}
		#undef BUFSIZE
		#define BUFSIZE 512
		PBYTE buffer=(PBYTE) mir_alloc(BUFSIZE);
		GlobalRNG().GenerateBlock(buffer, BUFSIZE);
        inject_msg(context,0x60,encode_msg(p,sign(buffer,BUFSIZE)));
		mir_free(buffer);
		imp->rsa_notify(context,1);	// заебися, криптосессия установлена
		p->state=7;
	} break;

	case 0x60:
	{
		string msg = decode_msg(p,data);
		if( msg.length() == 0 ) {
			null_msg(context,0x00,-1); // сессия разорвана по ошибке
			p->state=0;
			return 0;
		}
		imp->rsa_notify(context,1);	// заебися, криптосессия установлена
		p->state=7;
	} break;

	case 0x70:
	{
		SAFE_FREE(ptr->tmp);
		string msg = decode_msg(p,data);
		if( msg.length() ) {
			ptr->tmp = (LPSTR) mir_alloc(msg.length()+1);
			memcpy(ptr->tmp,msg.c_str(),msg.length()+1);
		}
		else {
			imp->rsa_notify(context,-5); // ошибка дэкодирования сообщения
		}
		return ptr->tmp;
	} break;

	case 0xF0:
	{
		imp->rsa_notify(context,-4); // соединение разорвано вручную другой стороной
		p->state=0;
	} break;
	}
	p->time = gettime()+10;
	return 0;
}


int rsa_send(int context, LPCSTR msg) {

    pCNTX ptr = get_context_on_id(context);
    if(!ptr) return 0;
	cpp_alloc_pdata(ptr); pRSADATA p = (pRSADATA) ptr->pdata;
	if(p->state!=7) return 0;

	inject_msg(context,0x70,encode_msg(p,msg));

	return 1;
}


void inject_msg(int context, int type, const string& msg) {
	string txt=tlv(type,msg);
	char* base64=base64encode(txt.data(),txt.length());
	imp->rsa_inject(context,(LPCSTR)base64);
	mir_free(base64);
}


string encode_msg(pRSADATA p, string& msg) {
	return encode_msg(p,msg.data(),msg.length());
}


string encode_msg(pRSADATA p, LPCSTR msg) {
	return encode_msg(p,msg,strlen(msg));
}


string encode_msg(pRSADATA p, LPCSTR msg, int len) {

	string ciphered;
	string sig = sign((PBYTE)msg,len);

	CBC_Mode<AES>::Encryption enc((PBYTE)p->aes_k.data(),p->aes_k.length(),(PBYTE)p->aes_v.data());

	StreamTransformationFilter cbcEncryptor(enc,new StringSink(ciphered));
	cbcEncryptor.Put((PBYTE)msg,len);
	cbcEncryptor.MessageEnd();

	return tlv(1,ciphered)+tlv(2,sig);
}


string decode_msg(pRSADATA p, string& msg) {

	int t; string ciphered,sig; un_tlv(un_tlv(msg,t,ciphered),t,sig);

	string unciphered;
	try {
		CBC_Mode<AES>::Decryption dec((PBYTE)p->aes_k.data(),p->aes_k.length(),(PBYTE)p->aes_v.data());
		StreamTransformationFilter cbcDecryptor(dec,new StringSink(unciphered));

		cbcDecryptor.Put((PBYTE)ciphered.data(),ciphered.length());
		cbcDecryptor.MessageEnd();

		if( sig != sign(unciphered) ) {
			unciphered.erase();
		}
	}
	catch (...)	{
		;
	}
	return unciphered;
}


string gen_aes_key_iv(short m,pRSADATA p,pRSAPRIV r) {

	#undef BUFSIZE
	#define BUFSIZE 512
	PBYTE buffer=(PBYTE) mir_alloc(BUFSIZE);

	GlobalRNG().GenerateBlock(buffer, BUFSIZE);
	p->aes_k = ( m&MODE_RSA_4096 ) ? sign256(buffer,BUFSIZE) : sign128(buffer,BUFSIZE);
	
	GlobalRNG().GenerateBlock(buffer, BUFSIZE);
	p->aes_v = ( m&MODE_RSA_4096 ) ? sign256(buffer,BUFSIZE) : sign128(buffer,BUFSIZE);

	mir_free(buffer);

	string buf = tlv(10,p->aes_k)+tlv(11,p->aes_v);
	string enc = RSAEncryptString(p->pub,buf);
	string sig = RSASignString(r->priv_k,buf);

	return tlv(1,enc)+tlv(2,sig);
}


void init_pub(pRSADATA p, string& pub) {

	p->pub_k = pub;
	p->pub_s = sign(pub);

	int t; string mod, exp;	un_tlv(un_tlv(pub,t,mod),t,exp);
	p->pub.Initialize(BinaryToInteger(mod),BinaryToInteger(exp));
}


void null_msg(int context, int type, int status) {
	inject_msg(context,type,null);
	imp->rsa_notify(context,status);
}


// EOF
