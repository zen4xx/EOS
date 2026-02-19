#include "memory.h"

void memcpy(void* dest, void* src, u32 n){
	char* m_dest = dest;
	char* m_src = src;
	while(n--)
		*m_dest++ = *m_src++;
}

void memset(void* dest, char c, u32 n){
	char* m_dest;
	while(n--)
		*m_dest++ = c;
}
