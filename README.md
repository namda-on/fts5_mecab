# fts5_mecab
fts5_mecab.so provides mecab tokenizer for sqlite3 fts5.

## build environment
1. run build.sh
```
./build.sh
```

## compile and execute

Case without macro.

```
$ cd $HOME/usr/src/fts5_mecab
$ gcc -g -fPIC -shared fts5_mecab.c -o fts5_mecab.so -I$HOME/usr/include -L$HOME/usr/lib -lmecab
$ LD_LIBRARY_PATH=$HOME/usr/lib $HOME/usr/bin/sqlite3
sqlite> .load ./fts5_mecab
sqlite> CREATE VIRTUAL TABLE t1 USING fts5(x, tokenize = 'mecab');
```

Case with parameters 'v' or 'vv'.
This requires compile option '-DDEBUG'.

```
$ cd $HOME/usr/src/fts5_mecab
$ gcc -g -fPIC -shared fts5_mecab.c -o fts5_mecab.so -I$HOME/usr/include -L$HOME/usr/lib -lmecab -DDEBUG
$ LD_LIBRARY_PATH=$HOME/usr/lib $HOME/usr/bin/sqlite3
sqlite> .load /PATH/TO/fts5_mecab
sqlite> CREATE VIRTUAL TABLE t1 USING fts5(x, tokenize = 'mecab vv');
```

Case with parameters 'stop789'.
This requires compile option '-DSTOP789'.
If you use stop789 option, the token which has posid 7,8,9 will be discarded.

```
$ cd $HOME/usr/src/fts5_mecab
$ gcc -g -fPIC -shared fts5_mecab.c -o fts5_mecab.so -I$HOME/usr/include -L$HOME/usr/lib -lmecab -DSTOP789
$ LD_LIBRARY_PATH=$HOME/usr/lib $HOME/usr/bin/sqlite3
sqlite> .load /PATH/TO/fts5_mecab
sqlite> CREATE VIRTUAL TABLE t1 USING fts5(x, tokenize = 'mecab stop789');
```

## bash alias (Optional)

Add a line into .bashrc.

```
alias sqlite3='LD_LIBRARY_PATH=$HOME/usr/lib $HOME/usr/bin/sqlite3 -cmd ".load $HOME/usr/lib/fts5_mecab"'
```
