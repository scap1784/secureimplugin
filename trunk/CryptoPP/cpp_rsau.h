#ifndef __CPP_RSAU_H__
#define __CPP_RSAU_H__

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

#endif
