#!/bin/bash


RESULT_FILE_PREFIX=_result_GelEwRIA_
for foo in ./test_scenarios/*; do
    # $foo is full path
    SCENARIO=`basename $foo`
    echo use case $SCENARIO
    if [ -d scenario_$SCENARIO ]; then
	echo scenario_$SCENARIO already exists, skip
	continue
    fi
    mkdir -p scenario_$SCENARIO
    # copy all my code/header/files into new dir
    cp -r * scenario_$SCENARIO
    # goin to new dir
    cd scenario_$SCENARIO
    chmod u+w {unicast.cc,chat.cc,mp1.h}
    \rm -f checker*.py
    # get the files
    \cp -f $foo/{unicast.cc,chat.cc,mp1.h,checker*.py} .
    make clean
    make
    if [ ! $? = 0 ]; then
        echo compile error
        cd ..
        continue
    fi
    # run checkers
    for checker in checker*.py; do
        RESULTFILE=${RESULT_FILE_PREFIX}${checker}
        if [ ! -f $RESULTFILE ] ; then
            python $checker > $RESULTFILE 2>&1
            sleep 1
        fi
    done
    # go back out
    cd ..
done
