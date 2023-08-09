#include<stdio.h>
#include<stdlib.h>


void print(char *p){

	printf("%d\n",sizeof(p));
}
int main(){

	char *c=malloc(4);
	print(c);
	char y[8];
	print(y);
	return 0;
}
