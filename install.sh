#!/bin/bash
if [ ! -e "bin" ];then
	mkdir bin
fi

if [ ! -e "obj" ];then
	mkdir obj
fi

make
