#!/bin/sh

if [ $# -lt 1 ]; then
    echo "What to check?"
    exit 1
fi

libaklisp=$1
if [ ! -e "../$libaklisp" ]; then
# Optional: make a link to it
#    if [ ! -e "$libaklisp" ]; then
#        ln -s "../$libaklisp" $libaklisp
#    fi
#else 
        echo "You must build AkLisp first!" 
fi
