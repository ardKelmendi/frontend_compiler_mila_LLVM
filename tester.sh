#!/bin/bash
cd build
echo -e "executed make"
make ;


echo -e "stating testing"
cd ./../tests

for file in *.mila
do
    cd ..
    cd build
    echo -e "executing file $file\n";
    cat ./../tests/$file;
    ./mila < ./../tests/$file 2> /dev/null;
    echo -e "\n=========result========";
    ./output.out

    cd ./../tests

done