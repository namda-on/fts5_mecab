#!/bin/bash
set -e

PREFIX=$(pwd)/build
CC=gcc
CFLAGS="-DDEBUG -DSTOP789"
DRY_RUN=0
NAME=fts5_mecab
SRC=$NAME.c

mkdir -p $PREFIX/include $PREFIX/lib
cp sqlite/sqlite3*.h $PREFIX/include/

BUILD_ARG=""
if [ -n "$BUILD_TRIPLE" ]; then
  BUILD_ARG="--build=${BUILD_TRIPLE}"
fi

# Mecab 빌드
echo "---Build mecab lib---"
(
  cd mecab/mecab
  ./configure $BUILD_ARG --prefix=$PREFIX --with-charset=UTF8
  make && make install
)

echo "---Build mecab ipadic---"
(
  cd mecab/mecab-ipadic
  ./configure $BUILD_ARG --with-mecab-config=$PREFIX/bin/mecab-config --with-charset=UTF8 -enable-utf8-only --prefix=$PREFIX
  make && make install
)

echo "MeCab and IPADIC built into $PREFIX"