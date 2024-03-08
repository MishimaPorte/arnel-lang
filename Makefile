main: main.c
	gcc -o main -DARNEL_DEBUG_BUILD -ggdb main.c lexer.c parser.c compiler.c types.c

test.ssa: main
	./main 1> test.ssa

test.s: test.ssa
	qbe < test.ssa > test.s

test: test.s
	gcc -o test test.s 

kek: kek.c hashmap.c
	gcc -o kek kek.c hashmap.c 
