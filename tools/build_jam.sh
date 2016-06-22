#!/bin/sh
echo "Building jam executable ..."
tar zxf jam-2.5.tar.gz

cd jam-2.5
if [ `uname -s` = MINGW32_NT-5.0 ]; then
    echo "apply patch for mingw environment"
    patch -p1 < ../mingw.diff
fi  

make

find . -name "jam" -exec cp {} ../jam \;
find . -name "jam.exe" -exec cp {} ../jam.exe \;

if [ ! -x ../jam -a ! -x ../jam.exe ]; then 
    # fail to build jam, fallback to jam0
    find . -name "jam0" -exec cp {} ../jam \;
    find . -name "jam0.exe" -exec cp {} ../jam.exe \;
fi

cd ..
rm -rf jam-2.5
echo "Done!"
