#include "memory.h"

void memcpy(void* dest, void* src, u32 n){
	char* m_dest = (char*)dest;
	const char* m_src = (const char*)src;
	while(n--)
		*m_dest++ = *m_src++;
}

/* This used to be:  char* m_dest;  while(n--) *m_dest++ = c;
 * i.e. it wrote c to a completely uninitialised pointer. Under QEMU the
 * garbage in that register happened to be harmless; on real hardware it
 * scribbles over whatever it points at. */
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
