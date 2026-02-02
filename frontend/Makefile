minic: minic.y minic.l
	bison -d minic.y
	flex minic.l
	g++ -Wall -Wextra -g -o minic minic.tab.c lex.yy.c ast.c semantic.cpp

clean:
	rm -f minic minic.tab.c minic.tab.h lex.yy.c
