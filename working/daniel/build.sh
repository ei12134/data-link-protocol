#!/bin/sh

BIN="bin"
SRC="src"

mkdir -v -p $BIN;
cp makefile $BIN

# RUN MAKE
make -C $BIN $1

