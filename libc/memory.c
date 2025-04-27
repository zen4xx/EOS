
void memory_copy(void* dest, void* src, unsigned int n){
	char* m_dest = dest;
	char* m_src = src;
	while(n--)
		*m_dest++ = *m_src++;
}

void int_to_ascii(int n, char str[]){
	int i, sign;
	if((sign = n) < 0) n = -n;
	i = 0;
	do {
		str[i++] = n % 10 + '0';
	} while((n /= 10) > 0);

	if(sign < 0) str[i++] = '-';
	str[i] = '\0';
}
