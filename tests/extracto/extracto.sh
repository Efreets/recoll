#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    tmpfile=`mktemp -t rcltsttmpXXXXXX`
    trap "rm -f -- '$tmpfile'" EXIT
    rm -f "$tmpfile"
    recollq --extract-to "$tmpfile" -q HtmlAttachment_uniqueTerm
    md5sum "$tmpfile" | awk '{print $1}'
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
