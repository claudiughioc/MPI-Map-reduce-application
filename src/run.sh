#!/bin/bash
echo "Test SPRC"
echo "Building the project"
make clean
make

for i in 2 3 4
do
    for j in 4 6 8 10
    do
        echo "Running test with $i mappers and $j reducers:"
        echo "$i" > config
        echo "$j" >> config
        echo "2600.txt" >> config
        echo "2600.out" >> config

        time mpirun -np 1 ./master
    done
done
make clean

echo "Finish SPRC test"
