#!/bin/bash
set -e

PREFIX=$(pwd)/build
CC=gcc
CFLAGS="-DDEBUG -DSTOP789"
DRY_RUN=0
NAME=fts5_mecab
SRC=$NAME.c

# 플랫폼 별 확장자
UNAME="$(uname)"
if [[ "$UNAME" == "Darwin" ]]; then
    OUT="lib${NAME}.dylib"
else
    OUT="lib${NAME}.so"
fi


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


usage_exit() {
    echo "Usage: $0 [-h|--help] [--prefix=PREFIX] [--clear-cflags] [-DSYMBOL] [--dry-run]"
    echo "  \$PREFIX=$PREFIX"
    echo "  \$CFLAGS=\"$CFLAGS\""
    echo "  \$DRY_RUN=$DRY_RUN"
    echo "Sample"
    echo "  $0 --prefix=$PREFIX --clear-cflags -DDEBUG -DSTOP789 --dry-run"
    exit 0
}

echo_and_do() {
    echo "$1"
    if [[ "$DRY_RUN" -eq 0 ]]; then
        eval "$1"
    else
        echo "!! DRY RUN !!"
    fi
}

for x in "$@"; do
    case "$x" in
        -h|--help) usage_exit ;;
        --dry-run) DRY_RUN=1 ;;
        --prefix=*) PREFIX="${x#*=}" ;;
        --clear-cflags) CFLAGS="" ;;
        -D*) CFLAGS="$CFLAGS -D${x:2}" ;;
        *) echo "Unknown option: $x"; usage_exit ;;
    esac
done


[ ! -d $PREFIX/lib ] && mkdir -p $PREFIX/lib

echo_and_do "$CC -g -fPIC -shared $SRC -o $OUT -I $PREFIX/include -L $PREFIX/lib -lmecab $CFLAGS"
if [ $? == 0 ]; then
  echo ""
  echo "✅ Compilation succeeded."
else
    echo "❌ Compilation failed."
fi
