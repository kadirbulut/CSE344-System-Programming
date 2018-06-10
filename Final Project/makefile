all: 
	gcc -g -c -Wall -errors homeworkServer.c -lpthread
	gcc -o homeworkServer homeworkServer.o -lpthread -lm
	gcc -g -c -Wall -errors clientApp.c 
	gcc -o clientApp clientApp.o -lm
clean:
	rm *.o homeworkServer clientApp
