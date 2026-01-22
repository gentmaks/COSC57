extern void print(int);
extern int read();

int func(int i){
	int c;
	
	if (i < 10){
		c = i + 10;
	}
	else{
		int b;
		c = i + 20;
		b = c + 40;
	}
	return c;
}
