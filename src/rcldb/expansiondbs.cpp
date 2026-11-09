/* Copyright (C) 2005 J.F.Dockes 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "autoconfig.h"

#include "expansiondbs.h"

#include <memory>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>

#include "log.h"
#include "utf8iter.h"
#include "smallut.h"
#include "chrono.h"
#include "textsplit.h"
#include "xmacros.h"
#include "rcldb.h"
#include "rcldb_p.h"
#include "stemdb.h"
#include "stemmap.h"

using namespace std;

namespace Rcl {

static bool processStemMap(Xapian::WritableDatabase& wdb);

/**
 * Create all expansion dbs used to transform user input term to widen a query
 * We use Xapian synonyms subsets to store the expansions.
 */
bool createExpansionDbs(Xapian::WritableDatabase& wdb, const vector<string>& langs)
{
    LOGDEB("StemDb::createExpansionDbs: languages: " <<stringsToString(langs) << "\n");
    Chrono cron;

    // Maybe peruse the stemmap. This is a map of word/stem assocs created while indexing as opposed
    // to the normal mechanism which is to process the term list after indexing. The stemmap is used
    // for Japanese at the moment, because it seems that context is important for Japanese stemming
    // so that working on isolated words would produce inferior results.
    // If the stemmap is not enabled or empty, this does nothing.
    if (!processStemMap(wdb)) {
        LOGERR("createExpansionDbs: fatal error while processing stemmap\n");
        return false;
    }
    
    // Erase and recreate all the expansion groups

    // If langs is empty and we don't need casediac expansion, then no need to
    // walk the big list
    if (langs.empty()) {
        if (o_index_stripchars)
            return true;
    }

    // Walk the list of all terms, and stem/unac each.
    string ermsg;
    try {
        // Stem dbs
        vector<XapWritableComputableSynFamMember> stemdbs;
        // Note: tried to make this to work with stack-allocated objects, couldn't.
        // Looks like a bug in copy constructors somewhere, can't guess where
        vector<std::shared_ptr<SynTermTransStem> > stemmers;
        for (unsigned int i = 0; i < langs.size(); i++) {
            stemmers.push_back(std::shared_ptr<SynTermTransStem>
                               (new SynTermTransStem(langs[i])));
            stemdbs.push_back(
                XapWritableComputableSynFamMember(wdb, synFamStem, langs[i], 
                                                  stemmers.back().get()));
            stemdbs.back().recreate();
        }

        // Unaccented stem dbs
        vector<XapWritableComputableSynFamMember> unacstemdbs;
        // We can reuse the same stemmer pointers, the objects are stateless.
        if (!o_index_stripchars) {
            for (unsigned int i = 0; i < langs.size(); i++) {
                unacstemdbs.push_back(
                    XapWritableComputableSynFamMember(
                        wdb, synFamStemUnac, langs[i], stemmers.back().get()));
                unacstemdbs.back().recreate();
            }
        }
        SynTermTransUnac transunac(UNACOP_UNACFOLD);
        XapWritableComputableSynFamMember 
            diacasedb(wdb, synFamDiCa, "all", &transunac);
        if (!o_index_stripchars)
            diacasedb.recreate();

        Xapian::TermIterator it = wdb.allterms_begin();
        // We'd want to skip to the first non-prefixed term, but this is a bit
        // complicated, so we just jump over most of the prefixed term and then 
        // skip the rest one by one.
        it.skip_to(wrap_prefix("Z"));
        for ( ;it != wdb.allterms_end(); it++) {
            const string term{*it};
            if (has_prefix(term))
                continue;

            // Detect and skip CJK and other ngrammed terms.
            Utf8Iter utfit(term);
            if (utfit.eof()) // Empty term?? Seems to happen.
                continue;
            if (TextSplit::noStemming(*utfit)) {
                // LOGDEB("stemskipped: Skipping CJK\n");
                continue;
            }

            string lower = term;
            // If the index is raw, compute the case-folded term which
            // is the input to the stem db, and add a synonym from the
            // stripped term to the cased and accented one, for accent
            // and case expansion at query time
            if (!o_index_stripchars) {
                unacmaybefold(term, lower, UNACOP_FOLD);
                diacasedb.addSynonym(term);
            }

            // Dont' apply stemming to terms which don't look like
            // natural language words.
            if (!Db::isSpellingCandidate(term)) {
                LOGDEB1("createExpansionDbs: skipped: [" << term << "]\n");
                continue;
            }

            // Create stemming synonym for every language. The input is the 
            // lowercase accented term
            for (unsigned int i = 0; i < langs.size(); i++) {
                stemdbs[i].addSynonym(lower);
            }

            // For a raw index, also maybe create a stem expansion for
            // the unaccented term. While this may be incorrect, it is
            // also necessary for searching in a diacritic-unsensitive
            // way on a raw index
            if (!o_index_stripchars) {
                string unac;
                unacmaybefold(lower, unac, UNACOP_UNAC);
                if (unac != lower) {
                    for (unsigned int i = 0; i < langs.size(); i++) {
                        unacstemdbs[i].addSynonym(unac);
                    }
                }
            }
        }
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
        LOGERR("Db::createStemDb: map build failed: " << ermsg << "\n");
        return false;
    }

    LOGDEB("StemDb::createExpansionDbs: done: " << cron.secs() << " S\n");
    return true;
}

static bool processStemMap(Xapian::WritableDatabase& wdb)
{
    auto stmmp = StemMap::getStemMap();
    if (nullptr == stmmp) {
        LOGDEB("processStemMap: not enabled\n");
        return true;
    }
    XapWritableSynFamMember syns(wdb, synFamCJK, "japanese");
    // We do NOT reset the existing transformations, as they're only ever recreated by a full
    // indexing (unlike western stemming where we walk the full term list to recreate the db from
    // scratch). This means that some spurious data will stay in there if documents are, e.g.,
    // deleted.
    for (const auto& [stem, words] : *stmmp) {
        // words is a space-separated list of surface forms.
        std::vector<std::string> uwords;
        stringToTokens(words, uwords);
        std::sort(uwords.begin(), uwords.end());
        auto uit = std::unique(uwords.begin(), uwords.end());
        uwords.resize(uit - uwords.begin());
        for (const auto& word : uwords) {
            syns.addSynonym(word, stem);
        }
    }        
    return true;
}

}    
