#!/bin/bash

set -e

tar -czf proj.tar.gz proj

if [ -d "out" ]
then
  read -p "Directory 'out' already exists. Rewrite it? (y/N): " answer
  if [ "$answer" = "y" ] || [ "$answer" = "Y" ]
  then
    rm -rf out
    mkdir out
  else
    echo "Canceled"
    exit
  fi
else
  mkdir out
fi

cp proj.tar.gz out/proj.tar.gz
cd out/
tar -xzf proj.tar.gz && rm -f proj.tar.gz

mkdir proj/include
mkdir proj/src
mkdir proj/build

mv proj/*.h proj/include/
mv proj/*.cpp proj/src/

g++ -I./proj/include proj/src/lib.cpp proj/src/main.cpp -o proj/build/app

echo "10 20" | ./proj/build/app > stdout.txt
