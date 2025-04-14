void entry_point(){
}

void main(){
	char* vm = (char*) 0xb8000;
	*vm = 'X';
}
