#include "memory.h"

void memcpy(void* dest, void* src, u32 n){
	char* m_dest = (char*)dest;
	const char* m_src = (const char*)src;
	while(n--)
		*m_dest++ = *m_src++;
}

void memset(void* dest, char c, u32 n){
	char* m_dest = (char*)dest;
	while(n--)
		*m_dest++ = c;
}

void memmove(void* dest, void* src, u32 n){
	char* d = (char*)dest;
	char* s = (char*)src;
	if (d == s || n == 0) return;
	if (d < s) {
		while(n--) *d++ = *s++;
	} else {
		d += n; s += n;
		while(n--) *--d = *--s;
	}
}
