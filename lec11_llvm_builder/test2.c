extern void print(int);
extern int read();

int func(int i){
	int a;
	int	b;
	int c;
	b = i+10;
	c = b;
	
	while (a < i){
		int c; 
		if (b > 0) 
			c = a + b;
		else 
			c = a - b;
		
		a = c;
	}

	return b+c;

}
