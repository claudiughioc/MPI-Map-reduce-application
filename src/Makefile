CC = mpicc
EXEC = mpirun
MAPSOURCES=master.c hashtable.c hash.c
REDSOURCES=reducer.c hashtable.c hash.c
MAPEXEC=master
REDEXEC=reducer

build: master.c
	${CC} $(MAPSOURCES) -o $(MAPEXEC) -Wall
	${CC} $(REDSOURCES) -o $(REDEXEC) -Wall

run: build
	${EXEC} -np 1 ./master

clean:
	rm -fr master reducer

