#!/bin/sh

cd `dirname $0`

: ${CC:=clang70}
SOURCE_FILES=src/*.c
OBJ_FILES=
OBJ_MAIN=

if `which ccache > /dev/null 2>&1` ; then
	CC="ccache $CC"
fi

LIBS="
	-L/usr/local/lib
"

BUILD_FLAGS="
	-g
	-Wall -Werror -Wno-unused-function
	-fno-omit-frame-pointer
	-fsanitize=undefined
	-ferror-limit=5
	-std=c11
	-isystem/usr/local/include
	-Iinc
"

mkdir -p bin
mkdir -p obj

for C_FILE in $SOURCE_FILES ; do
	OBJ_FILE=`echo $C_FILE | sed -e 's#\.c$#.o#' | sed -e 's#^src/#obj/#'`
	if [ $C_FILE = 'src/main.c' ] ; then
		OBJ_MAIN="$OBJ_FILE"
	else
		OBJ_FILES="$OBJ_FILES $OBJ_FILE"
	fi

	$CC \
		-c \
		-o $OBJ_FILE \
		$BUILD_FLAGS \
		$C_FILE
done

ar -rc bin/libiduotasm.a $OBJ_FILES

$CC \
	$BUILD_FLAGS \
	$OBJ_FILES \
	$LIBS \
	-lcheck test/*.c \
	-o bin/libiduotasm.test

$CC \
	$BUILD_FLAGS \
	$OBJ_FILES \
	$OBJ_MAIN \
	$LIBS \
	-o bin/iduot-asm

./bin/libiduotasm.test
