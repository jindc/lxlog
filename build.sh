mkdir test/log;
gcc -O -g -W -Werror -l pthread -I ../lxlib -I. -o test/test *.c test/test.c ../lxlib/*.c

