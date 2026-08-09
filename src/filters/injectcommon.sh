#!/bin/sh
fatal()
{
    echo $*
    exit 1
}

commoncode=recfiltcommon
test -f recfiltcommon || fatal must be executed inside the filters directory

for filter in `grep -rl RECFILTCOMMONCODE` ; do
    test "$filter" = injectcommon.sh -o "$filter" = recfiltcommon && continue
sed -e '/^#RECFILTCOMMONCODE/r recfiltcommon
/^#RECFILTCOMMONCODE/,/^#ENDRECFILTCOMMONCODE/d
' < $filter > filtertmp && mv -f filtertmp $filter && chmod a+x $filter
done
