#!/bin/bash

export LD_LIBRARY_PATH=..:$LD_LIBRARY_PATH

tests=(*.test)

if [ ${#tests[@]} -eq 1 ] ; then
    echo "There are no tests built."
    dname=$(basename `pwd`)
    if [ $dname = "tests" ] ; then
        echo "Please build the tests with the 'make' command."
    else
        echo "Please build the tests with the 'make test' command (from the project root)."
    fi
    exit 1
else
    ret=0
    for t in ${tests[@]} ; do
        ./$t
        if [ $? -eq 1 ] ; then
            ret=1
        fi
    done

    echo "${#tests[@]} tests completed."
    exit $ret
fi
