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

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}