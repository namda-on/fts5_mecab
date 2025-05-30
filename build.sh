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

# Mecab 빌드
(
  cd mecab/mecab
  ./configure --prefix=$PREFIX --with-charset=UTF8
  make && make install
)
(
  cd mecab/mecab-ipadic
  ./configure --with-mecab-config=$PREFIX/bin/mecab-config --with-charset=UTF8 -enable-utf8-only --prefix=$PREFIX
  make && make install
)

echo "MeCab and IPADIC built into $PREFIX"