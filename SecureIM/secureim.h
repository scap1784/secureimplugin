#ifndef __SECURE_IM__
#define __SECURE_IM__

#include "resource.h"

#define STATUS_DISABLED			0
#define STATUS_ENABLED			1
#define STATUS_ALWAYSTRY		2

#define ADV_CNT		8
#define IEC_CNT		9
#define ICO_CNT		5
#define POP_CNT		6
#define ALL_CNT		(IEC_CNT+ICO_CNT+POP_CNT)

#define SiG_KEYR 0
#define SiG_KEY3 SiG_KEYR
#define SiG_KEY4 SiG_KEYR
#define SiG_ENON 1
#define SiG_ENOF 2
#define SiG_RSND 3
#define SiG_INIT 4
#define SiG_DEIN 5
#define SiG_DISA 6
#define SiG_FAKE 7
#define SiG_KEYA 8
#define SiG_KEYB 9
#define SiG_PART 10
#define SiG_SECU 11
#define SiG_SECP 12
#define SiG_PGPM 13
#define SiG_NONE -1
#define SiG_GAME -2

struct SIG {
	char *sig;
	BYTE len;
	char key;
};

const SIG signs[] = {
	{"----Key3@hell----",			17,	SiG_KEYR},
	{"----Key4@hell----",			17,	SiG_KEYR},
	{"----Secured@hell----",		20,	SiG_ENON},
	{"----Offline@hell----",		20,	SiG_ENOF},
	{"----Resend@hell----",			19,	SiG_RSND},
	{"----INIT@HELL----",			17,	SiG_INIT},
	{"----DEINIT@HELL----",			19,	SiG_DEIN},
	{"----DISABLED@HELL----",		21,	SiG_DISA},
	{"----FAKE@HELL----",			17,	SiG_FAKE},
	{"----KeyA@hell----",			17,	SiG_KEYA},
	{"----KeyB@hell----",			17,	SiG_KEYB},
	{"----Part@hell----",			17,	SiG_PART},
	{"[SECURE]",				8,	SiG_SECU},
	{"[$ECURE]",				8,	SiG_SECP},
	{"-----BEGIN PGP MESSAGE-----",		27,	SiG_PGPM},
	{"@@BattleShip",			12,	SiG_GAME},
	{"GoMoku 0.0.2.2:",			15,	SiG_GAME},
	{"@@GoMoku",				8,	SiG_GAME},
	{"pbiChess:",				9,	SiG_GAME},
	{"pbiReverse:",				11,	SiG_GAME},
	{"pbiCorners:",				11,	SiG_GAME},
	{"pbiCheckersInt:",			15,	SiG_GAME},
	{"pbiCheckersRus:",			15,	SiG_GAME},
	{"pbiCheckersPool:",			16,	SiG_GAME},
	{0}
};

#define SIG_KEY3 signs[ 0].sig
#define SIG_KEY4 signs[ 1].sig
#define SIG_ENON signs[ 2].sig
#define SIG_ENOF signs[ 3].sig
#define SIG_RSND signs[ 4].sig
#define SIG_INIT signs[ 5].sig
#define SIG_DEIN signs[ 6].sig
#define SIG_DISA signs[ 7].sig
#define SIG_FAKE signs[ 8].sig
#define SIG_KEYA signs[ 9].sig
#define SIG_KEYB signs[10].sig
#define SIG_PART signs[11].sig
#define SIG_SECU signs[12].sig
#define SIG_SECP signs[13].sig

#define LEN_KEY3 signs[ 0].len
#define LEN_KEY4 signs[ 1].len
#define LEN_ENON signs[ 2].len
#define LEN_ENOF signs[ 3].len
#define LEN_RSND signs[ 4].len
#define LEN_INIT signs[ 5].len
#define LEN_DEIN signs[ 6].len
#define LEN_DISA signs[ 7].len
#define LEN_FAKE signs[ 8].len
#define LEN_KEYA signs[ 9].len
#define LEN_KEYB signs[10].len
#define LEN_PART signs[11].len
#define LEN_SECU signs[12].len
#define LEN_SECP signs[13].len

#define TBL_IEC		0x01
#define TBL_ICO		0x02
#define TBL_POP		0x03

#define IEC_NONE	0x00
#define IEC_ON	 	0x01
#define IEC_ON_OLD	0x02
#define IEC_ON_MID	0x03
#define IEC_OFF		0x04
#define IEC_SHIELD	0x05
#define IEC_QUERY	0x06
#define IEC_PGP  	0x07
#define IEC_GPG  	0x08

#define ICO_DISKEY 	0x00
#define ICO_ENAKEY	0x01
#define ICO_TRYKEY	0x02
#define ICO_PGPKEY	0x03
#define ICO_GPGKEY	0x04

#define POP_RECVKEY	0x00
#define POP_SENDKEY	0x01
#define POP_SECENA 	0x02
#define POP_SECDIS 	0x03
#define POP_SECMSR	0x04
#define POP_SECMSS	0x05

struct ICONS {
	UINT key;
	BYTE tbl;
	BYTE idx;
	char *name;
	char *text;
};

const ICONS icons[] = {
	{IDI_REDKEY,	TBL_IEC, IEC_OFF,		"sim_off", "Connection Disabled"},
	{IDI_GREENKEY,	TBL_IEC, IEC_ON,		"sim_on", "Connection Established (valid prime)"},
	{IDI_COLORKEY,	TBL_IEC, IEC_SHIELD,	"sim_shld", "Establishe Secure Connection"},
	{IDI_BLUEKEY,	TBL_IEC, IEC_QUERY,		"sim_quer", "Key Exchange"},
	{IDI_GREYKEY,	TBL_IEC, IEC_ON_OLD,	"sim_old", "Connection Established (ver <= 1.0.4.4c)"},
	{IDI_YELLOWKEY,	TBL_IEC, IEC_ON_MID,	"sim_mid", "Connection Established (ver >= 1.0.5.0)"},
	{IDI_PGPKEY,	TBL_IEC, IEC_PGP,		"sim_pgp", "PGP Key"},
	{IDI_GPGKEY,	TBL_IEC, IEC_GPG,		"sim_gpg", "GPG Key"},
	{IDI_DISKEY,	TBL_ICO, ICO_DISKEY,	"sim_dis", "flag Disabled"},
	{IDI_ENAKEY,	TBL_ICO, ICO_ENAKEY,	"sim_ena", "flag Enabled"},
	{IDI_TRYKEY,	TBL_ICO, ICO_TRYKEY,	"sim_try", "flag Always Try"},
	{IDI_PGPKEY,	TBL_ICO, ICO_PGPKEY,	"sim_pgpk", "flag PGP Key"},
	{IDI_GPGKEY,	TBL_ICO, ICO_GPGKEY,	"sim_gpgk", "flag GPG Key"},
	{IDI_LOCK,		TBL_POP, POP_SECMSR,	"sim_msr", "Recv Secured MSG (popup)"},
	{IDI_LOCK,		TBL_POP, POP_SECMSS,	"sim_mss", "Send Secured MSG (popup)"},
	{IDI_BLUEKEY,	TBL_POP, POP_RECVKEY,	"sim_rk", "Recv Key (popup)"},
	{IDI_BLUEKEY,	TBL_POP, POP_SENDKEY,	"sim_sk", "Send Key (popup)"},
	{IDI_GREENKEY,	TBL_POP, POP_SECENA,	"sim_sece", "Connection Established (popup)"},
	{IDI_REDKEY,	TBL_POP, POP_SECDIS,	"sim_secd", "Connection Disabled (popup)"},
	{0}
};

#endif
