#include "commonheaders.h"


char *base16encode(const char *inBuffer, int count) {

	char *outBuffer = (char *) mir_alloc(count*2+1);
	char *outBufferPtr = outBuffer;
	BYTE *inBufferPtr = (BYTE *) inBuffer;

	while(count){
		*outBufferPtr++ = encode16(((*inBufferPtr)>>4)&0x0F);
		*outBufferPtr++ = encode16((*inBufferPtr++)&0x0F);
		count--;
	}
	*outBufferPtr = '\0';

	return outBuffer;
}


char *base16decode(const char *inBuffer, int *count) {

	char *outBuffer = (char *) mir_alloc(*count);
	BYTE *outBufferPtr = (BYTE *) outBuffer;
	bool big_endian = false;

	if(*inBuffer == '0' && *(inBuffer+1)=='x') {
		inBuffer += *count;
		big_endian = true;
		*count -= 2;
	}
	while(*count){
		BYTE c0,c1;
		if(big_endian) {
			c1 = decode16(*--inBuffer);
			c0 = decode16(*--inBuffer);
		}
		else {
			c0 = decode16(*inBuffer++);
			c1 = decode16(*inBuffer++);
		}
		if((c0 | c1) == BERR) {
			mir_free(outBuffer);
			return(NULL);
		}
		*outBufferPtr++ = (c0<<4) | c1;
		(*count)-=2;
	}
	*count = (int)(outBufferPtr-(BYTE *)outBuffer);
	outBufferPtr[*count] = '\0';

	return outBuffer;
}


char *base16decode(const char *inBuffer) {

    int count = (int)strlen(inBuffer);
	return base16decode(inBuffer, &count);
}


// EOF
