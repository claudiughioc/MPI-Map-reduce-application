CC = mpicc
EXEC = mpirun

build: master.c
	${CC} -o master master.c -Wall

run: build
	${EXEC} -np 1 ./master

clean:
	rm -fr master

