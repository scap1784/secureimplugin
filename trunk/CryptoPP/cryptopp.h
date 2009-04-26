#ifndef __CRYPTOPP_H__
#define __CRYPTOPP_H__

//#pragma warning(push)
// 4231: nonstandard extension used : 'extern' before template explicit instantiation
// 4250: dominance
// 4251: member needs to have dll-interface
// 4275: base needs to have dll-interface
// 4660: explicitly instantiating a class that's already implicitly instantiated
// 4661: no suitable definition provided for explicit template instantiation request
// 4700: unused variable names...
// 4706: long names...
// 4786: identifer was truncated in debug information
// 4355: 'this' : used in base member initializer list
#pragma warning(disable: 4231 4250 4251 4275 4660 4661 4700 4706 4786 4355)
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "crypto/modes.h"
#include "crypto/osrng.h"
#include "crypto/rsa.h"
#include "crypto/aes.h"
#include "crypto/dh.h"
#include "crypto/crc.h"
#include "crypto/ripemd.h"
#include "crypto/sha.h"
#include "crypto/tiger.h"
#include "crypto/gzip.h"
#include "crypto/zlib.h"
#include "crypto/files.h"
//#pragma warning(pop)

USING_NAMESPACE(CryptoPP);
USING_NAMESPACE(std)

#define KEYSIZE 256
#define DEFMSGS 4096

typedef struct __SIMDATA {
	DH	*dh;		// diffie-hellman
	PBYTE	PubA;		// public keyA 2048 bit
	PBYTE	KeyA;		// private keyA 2048 bit
	PBYTE	KeyB;		// public keyB 2048 bit
	PBYTE	KeyX;		// secret keyX 192 bit
	PBYTE	KeyP;		// pre-shared keyP 192 bit
} SIMDATA;
typedef SIMDATA* pSIMDATA;

typedef struct __PGPDATA {
	PBYTE	pgpKeyID; // PGP KeyID
	PBYTE	pgpKey;   // PGP Key
} PGPDATA;
typedef PGPDATA* pPGPDATA;

typedef struct __GPGDATA {
	BYTE	*gpgKeyID; // GPG KeyID
} GPGDATA;
typedef GPGDATA* pGPGDATA;

#define RSA_KEYSIZE		SHA1::DIGESTSIZE
#define RSA_CalculateDigest	SHA1().CalculateDigest

typedef struct __RSAPRIV {
	string	priv_k;	// private key string
	string	priv_s;	// hash(priv_k)
	string	pub_k;	// public key string
	string	pub_s;	// hash(pub_k)
} RSAPRIV;
typedef RSAPRIV* pRSAPRIV;

typedef struct __RSADATA {
	short			state;	// 0 - нифига нет, 1..6 - keyexchange, 7 - соединение установлено
	u_int			time;	// для прерывания keyexchange, если долго нет ответа
	string			pub_k;	// public key string
	string			pub_s;	// hash(pub_k)
	RSA::PublicKey		pub;	// public key
	string			aes_k;	// aes key
	string			aes_v;	// aes iv
} RSADATA;
typedef RSADATA* pRSADATA;

typedef struct __CNTX {
	int	cntx;		// context id
	PBYTE	pdata;		// data block, for key exchange
	short	features;	// features of client
	short	mode;		// mode of encoding
	short	error;		// error code of last operation
	LPSTR	tmp;		// return string
} CNTX;
typedef CNTX* pCNTX;

#define FEATURES_UTF8			0x01
#define FEATURES_BASE64			0x02
#define FEATURES_GZIP			0x04
#define FEATURES_CRC32			0x08
#define FEATURES_PSK			0x10
#define FEATURES_NEWPG			0x20

#define MODE_BASE16			0x0000
#define MODE_BASE64			0x0001
#define MODE_PGP			0x0002
#define MODE_GPG			0x0004
#define MODE_GPG_ANSI			0x0008
#define MODE_RSA_PRIV			0x0010
#define MODE_RSA_2048			0x0020
#define MODE_RSA_4096			0x0040
#define MODE_RSA			MODE_RSA_4096
#define MODE_RSA_ONLY			0x0080
#define MODE_RSA_ZLIB			0x0100

#define DATA_GZIP			1

#define ERROR_NONE			0
#define ERROR_SEH			1
#define ERROR_NO_KEYA			2
#define ERROR_NO_KEYB			3
#define ERROR_NO_KEYX			4
#define ERROR_BAD_LEN			5
#define ERROR_BAD_CRC			6
#define ERROR_NO_PSK			7
#define ERROR_BAD_PSK			8
#define ERROR_BAD_KEYB			9
#define ERROR_NO_PGP_KEY		10
#define ERROR_NO_PGP_PASS		11
#define ERROR_NO_GPG_KEY		12
#define ERROR_NO_GPG_PASS		13

#define FEATURES (FEATURES_UTF8 | FEATURES_BASE64 | FEATURES_GZIP | FEATURES_CRC32 | FEATURES_NEWPG)
#define DLLEXPORT __declspec(dllexport)

extern LPCSTR szModuleName;
extern LPCSTR szVersionStr;
extern HINSTANCE g_hInst;

pCNTX get_context_on_id(int);
void cpp_free_keys(pCNTX);
BYTE *cpp_gzip(BYTE*,int,int&);
BYTE *cpp_gunzip(BYTE*,int,int&);
string cpp_zlibc(string&);
string cpp_zlibd(string&);

typedef struct {
    int (__cdecl *rsa_gen_keypair)(short);				// генерит RSA-ключи для указанной длины (либо тока 2048, либо 2048 и 4096)
    int (__cdecl *rsa_get_keypair)(short,PBYTE,int*,PBYTE,int*);	// возвращает пару ключей для указанной длины
    int (__cdecl *rsa_get_keyhash)(short,PBYTE,int*,PBYTE,int*);	// возвращает hash пары ключей для указанной длины
    int (__cdecl *rsa_set_keypair)(short,PBYTE,int,PBYTE,int);		// устанавливает ключи, указанной длины
    int (__cdecl *rsa_set_pubkey)(int,PBYTE,int);			// загружает паблик ключ для указанного контекста
    int (__cdecl *rsa_get_state)(int);					// получить статус указанного контекста
    int (__cdecl *rsa_connect)(int);					// запускает процесс установки содинения с указанным контекстом
    int (__cdecl *rsa_disconnect)(int);					// разрывает соединение
    LPSTR (__cdecl *rsa_recv)(int,LPCSTR);				// необходимо передавать сюда все входящие протокольные сообщения
    int   (__cdecl *rsa_send)(int,LPCSTR);				// вызываем для отправки сообщения клиенту
    LPSTR  (__cdecl *utf8encode)(LPCWSTR);
    LPWSTR (__cdecl *utf8decode)(LPCSTR);
    int (__cdecl *is_7bit_string)(LPCSTR);
    int (__cdecl *is_utf8_string)(LPCSTR);
} RSA_EXPORT;
typedef RSA_EXPORT* pRSA_EXPORT;

typedef struct {
    int  (__cdecl *rsa_inject)(int,LPCSTR);			// вставляет сообщение в очередь на отправку
    int  (__cdecl *rsa_check_pub)(int,PBYTE,int,PBYTE,int);	// проверяет интерактивно SHA и сохраняет ключ, если все нормально
    void (__cdecl *rsa_notify)(int,int);			// нотификация о смене состояния
} RSA_IMPORT;
typedef RSA_IMPORT* pRSA_IMPORT;

NAMESPACE_BEGIN(CryptoPP)
typedef RSASS<PKCS1v15, SHA256>::Signer RSASSA_PKCS1v15_SHA256_Signer;
typedef RSASS<PKCS1v15, SHA256>::Verifier RSASSA_PKCS1v15_SHA256_Verifier;
NAMESPACE_END

#endif
