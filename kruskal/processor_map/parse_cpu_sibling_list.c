#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void process(int c);
void normal(int c);
void comma(int c);
void hyphen (int c);
void number (int c);
void white_symb(int c);

enum states {
	NORMAL,
	NUMBER,
	COMMA,
	HYPHEN,
	WHITE_SYMB
};

int state = NORMAL;
int *core_list;
int hypbef, hypaft;
bool hypon=false;
int prev=0;

int main() {

	int curr,c,i;
	int num_cores = 88;
	char *sib_list = "0-6 7 10-12,45-50,70-72,80,83,84-87";
	//char *sib_list = "20-22";
	hypbef=hypaft=0;


	core_list = (int *)malloc(num_cores * sizeof(int));

	for(i=0;i<num_cores;i++)
		core_list[i] = 0;

	c=0;

	while(sib_list[c]!='\0') {
		process(sib_list[c]);
		c++;
		}
		if(hypon) {
			for(i=hypbef; i<=hypaft; i++) {
				printf("%d ", i);
				core_list[i] = 1;
				}
	}
		else
			printf("%d ",hypbef);
	
	putchar('\n');

	for(i=0;i<num_cores;i++)
		printf("core_list[%d] = %d\n", i, core_list[i]);

	putchar('\n');
	
    return 0;
}

void
process(int c) {
	
	switch(state) {
		case NORMAL:
			normal(c);
			break;
		case NUMBER:
			number(c);
			break;
		case COMMA:
			comma(c);
			break;
		case HYPHEN:
			hyphen(c);
			break;
		case WHITE_SYMB:
			white_symb(c);
			break;
	}

}

void
normal(int c) {
	
	if (c>='0' && c<='9') 
		number(c);
	else if (c == ',')
		comma(c);
	else if (c == '-')
		hyphen(c);
	else if (c == ' ')
		white_symb(c);
	
}

void
white_symb(int c) {
	int i;
	if(c==' '){
		prev = 0;
		if(hypon) {
			for(i=hypbef; i<=hypaft; i++) {
				printf("%d ", i);
				core_list[i] = 1;
				}
			hypon=0;
		}
		else
		printf("%d ",hypbef);
	}
	
	if (c=='\0')
		return;

	state=NORMAL;
}

void
hyphen(int c) {
	if(c=='-') {
		prev = 0;
		hypon=1;
	}
	state=NORMAL;
}

void
comma(int c) {
	int i;
	if (c==',') {
		prev = 0;
	}
	if(hypon) {
			for(i=hypbef; i<=hypaft; i++){
				printf("%d ", i);
				core_list[i] = 1;
				}
			hypon=0;
		}
	else
		printf("%d ",hypbef);
	
	state = NORMAL;
}

void
number(int c) {
	
	char tmp=(char)c;

	if (c >= '0' && c<= '9') {
		prev = prev*10 + atoi(&tmp);	
	}
	if(hypon==0)
		hypbef=prev;
	else {
		hypaft=prev;
	}

	state = NORMAL;
}

