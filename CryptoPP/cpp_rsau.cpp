#include "commonheaders.h"


string tlv(int t, const string& v) {
	string b;
	t |= v.length()<<8;
	b.assign((const char*)&t,3);
	b += v;
	return b;
}


string tlv(int t, const char* v) {
	return tlv(t, string(v));
}


string tlv(int t, int v) {
	string s;
	u_int l=(v<=0xFF)?1:((v<=0xFFFF)?2:((v<=0xFFFFFF)?3:4));
	s.assign((char*)&v,l);
	return tlv(t,s);
}


string& un_tlv(string& b, int& t, string& v) {
	string r; v = r;
	if( b.length() > 3 ) {
		t = 0;
		b.copy((char*)&t,3);
		u_int l = t>>8;
		t &= 0xFF;
		if( b.length() >= 3+l ) {
		  v = b.substr(3,l);
		  r = b.substr(3+l);
		}
	}
	if( !v.length() ) {
		t = -1;
	}
	b = r;
	return b;
}


string& un_tlv(string& b, int& t, int& v) {
	string s;
	un_tlv(b,t,s);
	v = 0;
	s.copy((char*)&v,s.length());
	return b;
}


void un_tlv(PBYTE b, int l, int& t, string& v) {
	string s;
	s.assign((char*)b,l);
	un_tlv(s,t,v);
}


string sign(PBYTE b, int l)
{
	BYTE h[RSA_KEYSIZE];
	RSA_CalculateDigest(h, b, l);
	string s; s.assign((char*)&h,RSA_KEYSIZE);
	return s;
}


string sign(string& b)
{
	return sign((PBYTE)b.data(),b.length());
}

/*
string sign128(PBYTE b, int l)
{
	Weak::MD5 md5;
	SecByteBlock h(md5.DigestSize());
	md5.CalculateDigest(h, b, l);
	string s; s.assign((char*)h.data(),h.size());
	return s;
}
*/

string sign128(PBYTE b, int l)
{
	BYTE h[RIPEMD128::DIGESTSIZE];
	RIPEMD128().CalculateDigest(h, b, l);
	string s; s.assign((char*)&h,sizeof(h));
	return s;
}


string sign256(PBYTE b, int l)
{
	BYTE h[SHA256::DIGESTSIZE];
	SHA256().CalculateDigest(h, b, l);
	string s; s.assign((char*)&h,sizeof(h));
	return s;
}


Integer BinaryToInteger(const string& data)
{
  StringSource ss(data, true, NULL);
  SecByteBlock result(ss.MaxRetrievable());
  ss.Get(result, result.size());
  return Integer(result, result.size());
}


string IntegerToBinary(const Integer& value)
{
  SecByteBlock sbb(value.MinEncodedSize());
  value.Encode(sbb, sbb.size());
  string data;
  StringSource(sbb, sbb.size(), true, new StringSink(data)); 
  return data;
}

/*
RandomPool& GlobalRNG()
{
	static RandomPool randomPool;
	return randomPool;
}
*/

AutoSeededRandomPool& GlobalRNG()
{
	static AutoSeededRandomPool randomPool;
	return randomPool;
}


void GenerateRSAKey(unsigned int keyLength, string& privKey, string& pubKey)
{
	RSAES_PKCS1v15_Decryptor priv(GlobalRNG(), keyLength);
	StringSink privFile(privKey);
	priv.DEREncode(privFile);
	privFile.MessageEnd();

	RSAES_PKCS1v15_Encryptor pub(priv);
	StringSink pubFile(pubKey);
	pub.DEREncode(pubFile);
	pubFile.MessageEnd();
}


string RSAEncryptString(const RSA::PublicKey& pubkey, const string& plaintext)
{
	RSAES_PKCS1v15_Encryptor pub(pubkey);

	string result;
	StringSource(plaintext, true, new PK_EncryptorFilter(GlobalRNG(), pub, new StringSink(result)));
	return result;
}


string RSADecryptString(const string& privkey, const string& ciphertext)
{
	StringSource privsrc(privkey, true, NULL);
	RSAES_PKCS1v15_Decryptor priv(privsrc);

	string result;
	try {
		StringSource(ciphertext, true, new PK_DecryptorFilter(GlobalRNG(), priv, new StringSink(result)));
	}
	catch (...)	{
		;
	}
	return result;
}


string RSASignString(const string& privkey, const string& plaintext)
{
	StringSource privsrc(privkey, true, NULL);
	RSASSA_PKCS1v15_SHA_Signer priv(privsrc);

	string result;
	try {
		StringSource(plaintext, true, new SignerFilter(GlobalRNG(), priv, new StringSink(result)));
	}
	catch (...)	{
		;
	}
	return result;
}


BOOL RSAVerifyString(const RSA::PublicKey& pubkey, const string& plaintext, const string& sig)
{
  RSASSA_PKCS1v15_SHA_Verifier ver(pubkey);

  return ver.VerifyMessage((PBYTE)plaintext.data(), plaintext.length(), (PBYTE)sig.data(), sig.length());
}


// EOF
