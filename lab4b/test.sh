#!/bin/bash

./lab4b --log=out --scale=F --period=2 <<EOF
LOG JIBBERISH
SCALE=C
STOP
START
OFF
EOF

if [ ! -s out ]; then
    echo "test 1 failed"
else
    echo "test 1 passed"
fi

./lab4b --scale=F --period=2 >std_out <<EOF
LOG JIBBERISH
OFF
EOF

grep "LOG JIBBERISH" std_out
if [ $? -eq 0 ]; then
    echo "test 2 failed"
else
    echo "test 2 passed"
fi

./lab4b --scale=F --period=2 >/dev/null 2>std_err <<EOF
JIBBERISH
OFF
EOF

if [ ! -s std_err ]; then
    echo "test 3 failed"
else
    echo "test 3 passed"
fi

rm -f out std_out std_err
