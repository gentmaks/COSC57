#include<stdio.h>

int test(int, int);

int main(){
	int i = test(10, 20);
	printf("In main printing return value of test: %d\n", i);
	return 0;
}
