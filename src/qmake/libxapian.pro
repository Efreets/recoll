QT       -= core gui

TARGET = libxapian
TEMPLATE = lib

DEFINES += HAVE_CONFIG_H

windows {
    XAPIANDIR = ../../../recolldeps/msvc/xapian-core/
    DEFINES += __WIN32__
    DEFINES += UNICODE

  contains(QMAKE_CC, cl){
    # msvc
    CONFIG += staticlib
    DEFINES += USING_STATIC_LIBICONV
    INCLUDEPATH += \
      ../../../recolldeps/msvc/zlib-1.2.11/ \
      ../../../recolldeps/msvc/wlibiconv/include
   }
}

mac {
    XAPIANDIR = ../../../xapian-core-2.1.0/
    CONFIG += staticlib
    QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64
}

# Note: not including honey backend source for now: needs: configure --disable-backend-honey
SOURCES += \
$$XAPIANDIR/api/compactor.cc \
$$XAPIANDIR/api/constinfo.cc \
$$XAPIANDIR/api/database.cc \
$$XAPIANDIR/api/decvalwtsource.cc \
$$XAPIANDIR/api/document.cc \
$$XAPIANDIR/api/documenttermlist.cc \
$$XAPIANDIR/api/documentvaluelist.cc \
$$XAPIANDIR/api/editdistance.cc \
$$XAPIANDIR/api/enquire.cc \
$$XAPIANDIR/api/error.cc \
$$XAPIANDIR/api/expanddecider.cc \
$$XAPIANDIR/api/keymaker.cc \
$$XAPIANDIR/api/matchspy.cc \
$$XAPIANDIR/api/mset.cc \
$$XAPIANDIR/api/msetiterator.cc \
$$XAPIANDIR/api/positioniterator.cc \
$$XAPIANDIR/api/postingiterator.cc \
$$XAPIANDIR/api/postingsource.cc \
$$XAPIANDIR/api/query.cc \
$$XAPIANDIR/api/queryinternal.cc \
$$XAPIANDIR/api/rangeproc.cc \
$$XAPIANDIR/api/registry.cc \
$$XAPIANDIR/api/replication.cc \
$$XAPIANDIR/api/result.cc \
$$XAPIANDIR/api/rset.cc \
$$XAPIANDIR/api/smallvector.cc \
$$XAPIANDIR/api/sortable-serialise.cc \
$$XAPIANDIR/api/terminfo.cc \
$$XAPIANDIR/api/termiterator.cc \
$$XAPIANDIR/api/termlist.cc \
$$XAPIANDIR/api/valueiterator.cc \
$$XAPIANDIR/api/valuesetmatchdecider.cc \
$$XAPIANDIR/api/vectortermlist.cc \
$$XAPIANDIR/backends/alltermslist.cc \
$$XAPIANDIR/backends/contiguousalldocspostlist.cc \
$$XAPIANDIR/backends/databasehelpers.cc \
$$XAPIANDIR/backends/databaseinternal.cc \
$$XAPIANDIR/backends/databasereplicator.cc \
$$XAPIANDIR/backends/dbcheck.cc \
$$XAPIANDIR/backends/dbfactory.cc \
$$XAPIANDIR/backends/dbfactory_remote.cc \
$$XAPIANDIR/backends/documentinternal.cc \
$$XAPIANDIR/backends/empty_database.cc \
$$XAPIANDIR/backends/flint_lock.cc \
$$XAPIANDIR/backends/glass/glass_alldocspostlist.cc \
$$XAPIANDIR/backends/glass/glass_alltermslist.cc \
$$XAPIANDIR/backends/glass/glass_changes.cc \
$$XAPIANDIR/backends/glass/glass_check.cc \
$$XAPIANDIR/backends/glass/glass_compact.cc \
$$XAPIANDIR/backends/glass/glass_cursor.cc \
$$XAPIANDIR/backends/glass/glass_database.cc \
$$XAPIANDIR/backends/glass/glass_dbcheck.cc \
$$XAPIANDIR/backends/glass/glass_document.cc \
$$XAPIANDIR/backends/glass/glass_freelist.cc \
$$XAPIANDIR/backends/glass/glass_inverter.cc \
$$XAPIANDIR/backends/glass/glass_metadata.cc \
$$XAPIANDIR/backends/glass/glass_positionlist.cc \
$$XAPIANDIR/backends/glass/glass_postlist.cc \
$$XAPIANDIR/backends/glass/glass_spelling.cc \
$$XAPIANDIR/backends/glass/glass_spellingwordslist.cc \
$$XAPIANDIR/backends/glass/glass_synonym.cc \
$$XAPIANDIR/backends/glass/glass_table.cc \
$$XAPIANDIR/backends/glass/glass_termlist.cc \
$$XAPIANDIR/backends/glass/glass_termlisttable.cc \
$$XAPIANDIR/backends/glass/glass_valuelist.cc \
$$XAPIANDIR/backends/glass/glass_values.cc \
$$XAPIANDIR/backends/glass/glass_version.cc \
$$XAPIANDIR/backends/inmemory/inmemory_alltermslist.cc \
$$XAPIANDIR/backends/inmemory/inmemory_database.cc \
$$XAPIANDIR/backends/inmemory/inmemory_document.cc \
$$XAPIANDIR/backends/inmemory/inmemory_positionlist.cc \
$$XAPIANDIR/backends/leafpostlist.cc \
$$XAPIANDIR/backends/multi/multi_alltermslist.cc \
$$XAPIANDIR/backends/multi/multi_database.cc \
$$XAPIANDIR/backends/multi/multi_postlist.cc \
$$XAPIANDIR/backends/multi/multi_termlist.cc \
$$XAPIANDIR/backends/multi/multi_valuelist.cc \
$$XAPIANDIR/backends/postlist.cc \
$$XAPIANDIR/backends/remote/net_postlist.cc \
$$XAPIANDIR/backends/remote/remote-database.cc \
$$XAPIANDIR/backends/remote/remote-document.cc \
$$XAPIANDIR/backends/remote/remote_alltermslist.cc \
$$XAPIANDIR/backends/remote/remote_keylist.cc \
$$XAPIANDIR/backends/remote/remote_termlist.cc \
$$XAPIANDIR/backends/slowvaluelist.cc \
$$XAPIANDIR/backends/uuids.cc \
$$XAPIANDIR/backends/valuelist.cc \
$$XAPIANDIR/common/bitstream.cc \
$$XAPIANDIR/common/closefrom.cc \
$$XAPIANDIR/common/compression_stream.cc \
$$XAPIANDIR/common/debuglog.cc \
$$XAPIANDIR/common/errno_to_string.cc \
$$XAPIANDIR/common/fileutils.cc \
$$XAPIANDIR/common/io_utils.cc \
$$XAPIANDIR/common/keyword.cc \
$$XAPIANDIR/common/msvc_dirent.cc \
$$XAPIANDIR/common/omassert.cc \
$$XAPIANDIR/common/pack.cc \
$$XAPIANDIR/common/posixy_wrapper.cc \
$$XAPIANDIR/common/replicate_utils.cc \
$$XAPIANDIR/common/safe.cc \
$$XAPIANDIR/common/serialise-double.cc \
$$XAPIANDIR/common/socket_utils.cc \
$$XAPIANDIR/common/str.cc \
$$XAPIANDIR/expand/bo1eweight.cc \
$$XAPIANDIR/expand/esetinternal.cc \
$$XAPIANDIR/expand/expandweight.cc \
$$XAPIANDIR/expand/ortermlist.cc \
$$XAPIANDIR/expand/probeweight.cc \
$$XAPIANDIR/geospatial/geoencode.cc  \
$$XAPIANDIR/geospatial/latlong_distance_keymaker.cc  \
$$XAPIANDIR/geospatial/latlong_metrics.cc  \
$$XAPIANDIR/geospatial/latlong_posting_source.cc \
$$XAPIANDIR/geospatial/latlongcoord.cc  \
$$XAPIANDIR/languages/algorithms/arabic.cc \
$$XAPIANDIR/languages/algorithms/armenian.cc \
$$XAPIANDIR/languages/algorithms/basque.cc \
$$XAPIANDIR/languages/algorithms/catalan.cc \
$$XAPIANDIR/languages/algorithms/czech.cc \
$$XAPIANDIR/languages/algorithms/danish.cc \
$$XAPIANDIR/languages/algorithms/dutch.cc \
$$XAPIANDIR/languages/algorithms/dutch_porter.cc \
$$XAPIANDIR/languages/algorithms/earlymodernenglish.cc \
$$XAPIANDIR/languages/algorithms/english.cc \
$$XAPIANDIR/languages/algorithms/esperanto.cc \
$$XAPIANDIR/languages/algorithms/estonian.cc \
$$XAPIANDIR/languages/algorithms/finnish.cc \
$$XAPIANDIR/languages/algorithms/french.cc \
$$XAPIANDIR/languages/algorithms/german.cc \
$$XAPIANDIR/languages/algorithms/greek.cc \
$$XAPIANDIR/languages/algorithms/hindi.cc \
$$XAPIANDIR/languages/algorithms/hungarian.cc \
$$XAPIANDIR/languages/algorithms/indonesian.cc \
$$XAPIANDIR/languages/algorithms/irish.cc \
$$XAPIANDIR/languages/algorithms/italian.cc \
$$XAPIANDIR/languages/algorithms/lithuanian.cc \
$$XAPIANDIR/languages/algorithms/lovins.cc \
$$XAPIANDIR/languages/algorithms/nepali.cc \
$$XAPIANDIR/languages/algorithms/norwegian.cc \
$$XAPIANDIR/languages/algorithms/persian.cc \
$$XAPIANDIR/languages/algorithms/polish.cc \
$$XAPIANDIR/languages/algorithms/porter.cc \
$$XAPIANDIR/languages/algorithms/portuguese.cc \
$$XAPIANDIR/languages/algorithms/romanian.cc \
$$XAPIANDIR/languages/algorithms/russian.cc \
$$XAPIANDIR/languages/algorithms/serbian.cc \
$$XAPIANDIR/languages/algorithms/sesotho.cc \
$$XAPIANDIR/languages/algorithms/spanish.cc \
$$XAPIANDIR/languages/algorithms/swedish.cc \
$$XAPIANDIR/languages/algorithms/tamil.cc \
$$XAPIANDIR/languages/algorithms/turkish.cc \
$$XAPIANDIR/languages/algorithms/yiddish.cc \
$$XAPIANDIR/languages/snowball_runtime.cc \
$$XAPIANDIR/languages/stem.cc  \
$$XAPIANDIR/matcher/andmaybepostlist.cc \
$$XAPIANDIR/matcher/andnotpostlist.cc \
$$XAPIANDIR/matcher/andpostlist.cc \
$$XAPIANDIR/matcher/boolorpostlist.cc \
$$XAPIANDIR/matcher/collapser.cc \
$$XAPIANDIR/matcher/deciderpostlist.cc \
$$XAPIANDIR/matcher/estimateop.cc \
$$XAPIANDIR/matcher/exactphrasepostlist.cc \
$$XAPIANDIR/matcher/externalpostlist.cc \
$$XAPIANDIR/matcher/extraweightpostlist.cc \
$$XAPIANDIR/matcher/localsubmatch.cc \
$$XAPIANDIR/matcher/matcher.cc \
$$XAPIANDIR/matcher/maxpostlist.cc \
$$XAPIANDIR/matcher/msetcmp.cc \
$$XAPIANDIR/matcher/nearpostlist.cc \
$$XAPIANDIR/matcher/orpositionlist.cc \
$$XAPIANDIR/matcher/orpospostlist.cc \
$$XAPIANDIR/matcher/orpostlist.cc \
$$XAPIANDIR/matcher/phrasepostlist.cc \
$$XAPIANDIR/matcher/selectpostlist.cc \
$$XAPIANDIR/matcher/synonympostlist.cc \
$$XAPIANDIR/matcher/valuegepostlist.cc \
$$XAPIANDIR/matcher/valuerangepostlist.cc \
$$XAPIANDIR/matcher/valuestreamdocument.cc \
$$XAPIANDIR/matcher/wrapperpostlist.cc \
$$XAPIANDIR/matcher/xorpostlist.cc \
$$XAPIANDIR/net/progclient.cc \
$$XAPIANDIR/net/remoteconnection.cc \
$$XAPIANDIR/net/remoteserver.cc \
$$XAPIANDIR/net/remotetcpclient.cc \
$$XAPIANDIR/net/replicatetcpclient.cc \
$$XAPIANDIR/net/replicatetcpserver.cc \
$$XAPIANDIR/net/serialise-error.cc \
$$XAPIANDIR/net/serialise.cc \
$$XAPIANDIR/net/tcpclient.cc \
$$XAPIANDIR/net/tcpserver.cc \
$$XAPIANDIR/queryparser/queryparser.cc \
$$XAPIANDIR/queryparser/queryparser_internal.cc \
$$XAPIANDIR/queryparser/termgenerator.cc \
$$XAPIANDIR/queryparser/termgenerator_internal.cc \
$$XAPIANDIR/queryparser/word-breaker.cc \
$$XAPIANDIR/unicode/description_append.cc \
$$XAPIANDIR/unicode/unicode-data.cc \
$$XAPIANDIR/unicode/utf8itor.cc \
$$XAPIANDIR/weight/bb2weight.cc \
$$XAPIANDIR/weight/bm25plusweight.cc \
$$XAPIANDIR/weight/bm25weight.cc \
$$XAPIANDIR/weight/boolweight.cc \
$$XAPIANDIR/weight/coordweight.cc \
$$XAPIANDIR/weight/diceweight.cc \
$$XAPIANDIR/weight/dlhweight.cc \
$$XAPIANDIR/weight/dphweight.cc \
$$XAPIANDIR/weight/ifb2weight.cc \
$$XAPIANDIR/weight/ineb2weight.cc \
$$XAPIANDIR/weight/inl2weight.cc \
$$XAPIANDIR/weight/lmweight.cc \
$$XAPIANDIR/weight/pl2plusweight.cc \
$$XAPIANDIR/weight/pl2weight.cc \
$$XAPIANDIR/weight/tfidfweight.cc \
$$XAPIANDIR/weight/weight.cc \
$$XAPIANDIR/weight/weightinternal.cc

INCLUDEPATH += $$XAPIANDIR $$XAPIANDIR/include \
           $$XAPIANDIR/common
