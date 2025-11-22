foobar : main.o foo.o bar.o
	gcc -o foobar main.o foo.o bar.o
main.o : main.c
	gcc -c main.c
foo.o : foo.c
	gcc -c foo.c
bar.o : bar.c
	gcc -c bar.c
clean :
	rm -f foobar main.o foo.o bar.o

