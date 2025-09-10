#!/bin/bash

set -e

if test -e out # TODO почему без [ ] (потому что [ ] это и есть test)
then
    rm -rf out
fi

mkdir out
cd out

whoami > me.txt

cp me.txt metoo.txt

man wc > wchelp.txt # TODO почему man и cat
cat wchelp.txt

# TODO почему wc и -f1
wc -l wchelp.txt | cut -d' ' -f1 > wchelp-lines.txt

# TODO почему tac
tac wchelp.txt > wchelp-reversed.txt

cat wchelp.txt wchelp-reversed.txt me.txt metoo.txt wchelp-lines.txt > all.txt

tar -cf result.tar ./*.txt
gzip -k result.tar

cd ..
if test -e result.tar.gz
then
    rm -f result.tar.gz
fi
mv out/result.tar.gz ./result.tar.gz

rm -rf out/
