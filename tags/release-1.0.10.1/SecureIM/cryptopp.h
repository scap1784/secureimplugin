#ifndef __CRYPTOPP_H__
#define __CRYPTOPP_H__

#define FEATURES_UTF8			0x01
#define FEATURES_BASE64			0x02
#define FEATURES_GZIP			0x04
#define FEATURES_CRC32			0x08
#define FEATURES_PSK			0x10
#define FEATURES_NEWPG			0x20

#define MODE_BASE16				0x00
#define MODE_BASE64				0x01
#define MODE_PGP				0x02
#define MODE_GPG				0x04
#define MODE_GPG_ANSI			0x08
#define MODE_RSA_PRIV			0x10
#define MODE_RSA				0x20
#define MODE_RSA_2048			MODE_RSA
#define MODE_RSA_4096			0x40
#define MODE_RSA_ONLY			0x80

#define ERROR_NONE				0
#define ERROR_SEH				1
#define ERROR_NO_KEYA			2
#define ERROR_NO_KEYB			3
#define ERROR_NO_KEYX			4
#define ERROR_BAD_LEN			5
#define ERROR_BAD_CRC			6
#define ERROR_NO_PSK			7
#define ERROR_BAD_PSK			8
#define ERROR_BAD_KEYB			9
#define ERROR_NO_PGP_KEY		10

typedef struct {
    int (*rsa_gen_keypair)(short);							// генерит RSA-ключи для указанной длины (либо тока 2048, либо 2048 и 4096)
    int (*rsa_get_keypair)(short,PBYTE,int*,PBYTE,int*);	// возвращает пару ключей для указанной длины
    int (*rsa_set_keypair)(short,PBYTE,int,PBYTE,int);		// устанавливает ключи, указанной длины
    int (*rsa_connect)(int,PBYTE,int);						// запускает процесс установки содинения с указанным контекстом
    int (*rsa_disconnect)(int);								// разрывает соединение
    LPSTR (*rsa_recv)(int,LPCSTR);							// необходимо передавать сюда все входящие протокольные сообщения
    int (*rsa_send)(int,LPCSTR);							// вызываем для отправки сообщения клиенту
} RSA_EXPORT;
typedef RSA_EXPORT* pRSA_EXPORT;

typedef struct {
    int (*rsa_inject)(int,LPCSTR);					// вставляет сообщение в очередь на отправку
    int (*rsa_check_pub)(int,PBYTE,int,PBYTE,int);	// проверяет интерактивно SHA и сохраняет ключ, если все нормально
    void (*rsa_notify)(int,int);					// нотификация о смене статуса
} RSA_IMPORT;
typedef RSA_IMPORT* pRSA_IMPORT;

#endif
