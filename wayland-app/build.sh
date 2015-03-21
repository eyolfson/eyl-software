#!/bin/bash

echo -e '\e[1;36m[Compile]\e[m \e[36mmain.o\e[m'
gcc -std=c11 -O3 -D_GNU_SOURCE -Wall -c main.c -o main.o
echo -e '\e[1;36m[Compile]\e[m \e[36mxdg_shell.o\e[m'
gcc -std=c11 -O3 -Wall -c xdg_shell.c -o xdg_shell.o
echo -e '\e[1;34m[Bin]\e[m \e[34mwayland-app\e[m'
gcc main.o xdg_shell.o -lwayland-client -o wayland-app
