CC = mpic++
EXEC = mpirun
MAPSOURCES=master.cpp
REDSOURCES=reducer.cpp
MAPEXEC=master
REDEXEC=reducer

build: master.cpp
	${CC} $(MAPSOURCES) -o $(MAPEXEC) -Wall
	${CC} $(REDSOURCES) -o $(REDEXEC) -Wall

run: build
	${EXEC} -np 1 ./master

clean:
	rm -fr $(MAPEXEC) $(REDEXEC)

