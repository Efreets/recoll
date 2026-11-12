#!/bin/sh

# test conftree python module

thisdir=`dirname $0`
topdir=$thisdir/..
. $topdir/shared.sh

initvariables $0

# Python interpreter. Find it relative to recollindex
sys=`uname -o`
if test "$sys" = Msys; then
    rindex=`which recollindex`
    test -n "$rindex" || fatal recollindex not found
    toprecoll=`dirname "$rindex"`
    python3="$toprecoll/share/filters/python/python.exe"
    cp conftree-nocase.txt conftree.txt
else
    python3=`which python3`
    cp conftree-case.txt conftree.txt
fi

trap "rm -f -- conftree.txt" EXIT

xrun()
{
    echo $*
    $*
}

(
    "$python3" conftree.py
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
rm -f conftree.txt
