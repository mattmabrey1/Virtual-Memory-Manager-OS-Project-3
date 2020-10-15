all:
	gcc -c *.c
	gcc -o main main.o -lpthread
clean:
	rm main *.o
