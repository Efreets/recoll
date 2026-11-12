#!/bin/sh

# Updating the index with Python and misc Python exercises

thisdir=`dirname $0`
topdir=$thisdir/..
. $topdir/shared.sh

initvariables $0

### NOTE: needs work under Windows. Excluded for now
# Python interpreter. Find it relative to recollindex
sys=`uname -o`
if test "$sys" = Msys; then
    rindex=`which recollindex`
    test -n "$rindex" || fatal recollindex not found
    toprecoll=`dirname "$rindex"`
    python3="$toprecoll/share/filters/python/python.exe"
else
    python3=`which python3`
fi


xrun()
{
    echo $*
    $*
}

(
    export RECOLL_TESTDATA=$tstdata
    export AUTHORNAME=somestrangeauthorname$$
    "$python3" updates.py
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout


checkresult
