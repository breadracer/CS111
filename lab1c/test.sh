#!/bin/bash

# NAME: Shawn Ma
# EMAIL: breadracer@outlook.com
# ID: 204996814

chmod 755 simpsh

echo "hello" > input.txt
touch output.txt
touch error.txt
touch verbose.txt
touch profile.txt
./simpsh --pipe --rdwr input.txt --rdwr output.txt --command 2 1 1 cat \
	 --command 0 3 3 wc --close 0 --close 1 --wait > /dev/null
if [ $? -eq 0 ]; then
    echo "test case 1 passed"
else
    echo "test case 1 failed"
    rm -f input.txt output.txt error.txt profile.txt verbose.txt
    exit 1
fi
echo "hello" > input.txt
printf "" > output.txt
printf "" > error.txt
./simpsh --rdonly input.txt --creat --rdwr output.txt --wronly error.txt \
	 --command 0 1 2 cat --default 12 --ignore 12 > /dev/null
if  [ $? -eq 0 ] && [ -e output.txt ]; then
    echo "test case 2 passed"
else
    echo "test case 2 failed"
    rm -f input.txt output.txt error.txt profile.txt verbose.txt
    exit 1
fi
printf "" > output.txt
printf "" > error.txt
./simpsh --verbose --rdonly input.txt --creat --wronly output.txt --wronly error.txt \
	 --command 0 1 2 cat > verbose.txt
if [ $? -eq 0 ] && [ -s output.txt ] && [ -s verbose.txt ]; then
    echo "test case 3 passed"
else
    echo "test case 3 failed"
    rm -f input.txt output.txt error.txt profile.txt verbose.txt
    exit 1
fi
printf "" > output.txt
printf "" > error.txt
./simpsh --pipe --pipe --pipe --rdonly input.txt --creat --wronly output.txt --creat \
	 --wronly error.txt --command 6 1 8 cat --command 0 3 8 sort \
	 --command 2 5 8 grep "h" --command 4 7 8 wc -l --close 0 --close 1 \
	 --close 2 --close 3 --close 4 --close 5 --profile --wait > profile.txt
if [ $? -eq 0 ] && [ -s output.txt ] && [ ! -s error.txt ] && [ -s profile.txt ]; then
    echo "test case 4 passed"
else
    echo "test case 4 failed"
    rm -f input.txt output.txt error.txt profile.txt verbose.txt
    exit 1
fi
printf "HSAFASDLKasdfklhdsafhljsae" > input.txt
printf "" > output.txt
printf "" > error.txt
./simpsh --verbose --pipe --pipe --rdonly input.txt --creat --wronly output.txt --creat \
	 --wronly error.txt --command 4 1 6 cat --command 0 3 6 tr A-Z a-z \
	 --command 2 5 6 cat --close 0 --close 1 --close 2 \
	 --close 3 --wait > verbose.txt
if [ $? -eq 0 ] && [ -s output.txt ] && [ ! -s error.txt ] && [ -s verbose.txt ]; then
    echo "test case 5 passed"
else
    echo "test case 5 failed"
    rm -f input.txt output.txt error.txt profile.txt verbose.txt
    exit 1
fi
echo "All tests passed"
rm -f input.txt output.txt error.txt profile.txt verbose.txt
