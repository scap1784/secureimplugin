#ifndef __UTF_8__
#define __UTF_8__

LPSTR utf8encode(LPCWSTR str);
LPWSTR utf8decode(LPCSTR str);
BOOL is_7bit_string(LPCSTR);
BOOL is_utf8_string(LPCSTR);

#endif
