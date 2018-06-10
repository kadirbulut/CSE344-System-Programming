all: 
	gcc -c -Wall -errors 121044005_main.c
	gcc -o shell 121044005_main.o
	gcc -c -Wall -errors wc.c
	gcc -o wcc wc.o
	gcc -c -Wall -errors cat.c
	gcc -o catt cat.o
	gcc -c -Wall -errors ls.c
	gcc -o lss ls.o
clean:
	rm *.o shell wcc catt lss
