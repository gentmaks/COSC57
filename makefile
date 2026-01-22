filename = example
(filename).out: $(filename).l
	yacc -d $(filename).y
	lex $(filename).l
	gcc -g lex.yy.c y.tab.c -o $(filename).out

clean:
	rm -rf $(filename).out lex.yy.c y.tab.c y.tab.h
