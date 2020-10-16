# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2020-21
# Andre Martins Esgalhado - 95533
# Bruno Miguel Da Silva Mendes - 95544
# Grupo - 16

SHELL := /bin/bash
CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../ -pthread
LDFLAGS=-lm

build: tecnicofs

tecnicofs: fs/state.o fs/operations.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs fs/state.o fs/operations.o main.o
	rm -f fs/*.o *.o *.out *.last

fs/state.o: fs/state.c fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/state.o -c fs/state.c

fs/operations.o: fs/operations.c fs/operations.h fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/operations.o -c fs/operations.c

main.o: main.c fs/operations.h fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o main.o -c main.c

clean:
	@echo Cleaning...
	rm -f fs/*.o *.o *.out *.last tecnicofs

%.txt: outputs/%.stdin
	./tecnicofs inputs/$@ output.out 1 nosync > stdin.last
	#valgrind --leak-check=full ./tecnicofs inputs/$@ output.out 1 nosync > stdin.last
	#diff <(sort <(head -n -1 stdin.last)) <(sort <(head -n -1 $<))
	#diff <(sort output.out) <(sort outputs/$@)
	./tecnicofs inputs/$@ output.out 3 mutex > stdin.last
	#valgrind --leak-check=full ./tecnicofs inputs/$@ output.out 3 mutex > stdin.last
	#diff <(sort <(head -n -1 stdin.last)) <(sort <(head -n -1 $<))
	#diff <(sort output.out) <(sort outputs/$@)
	./tecnicofs inputs/$@ output.out 3 mutex > stdin.last
	#valgrind --leak-check=full ./tecnicofs inputs/$@ output.out 3 mutex > stdin.last
	#diff <(sort <(head -n -1 stdin.last)) <(sort <(head -n -1 $<))
	#diff <(sort output.out) <(sort outputs/$@)

test: test1.txt test2.txt test3.txt test4.txt

test%: inputs/test%.txt
	./tecnicofs $< output.out 1 nosync
	read -n 1
	./tecnicofs $< output.out 3 mutex
	read -n 1
	./tecnicofs $< output.out 3 rwlock
	
