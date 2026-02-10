extern int read();
int test(int m, int n){
	int a;
	
 	a = read();
	while (m < n){
		m = m + 10;
	}

	return(m);
}
