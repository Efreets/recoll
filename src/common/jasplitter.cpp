/* Copyright (C) 2020 J.F.Dockes
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

// Specialized Japanese text splitter using MeCab running in a Python subprocess.  At the moment, we
// use a single subprocess and mutex-protect its access, so that Japanese text-splitting is
// effectively single-threaded, like the Korean splitter and unlike the Chinese one.

#include "jasplitter.h"

#include <iostream>
#include <string>
#include <cstring>
#include <mutex>

#include "textsplit.h"
#include "log.h"
//#define UTF8ITER_CHECK
#include "utf8iter.h"
#include "smallut.h"
#include "rclconfig.h"
#include "cmdtalk.h"
#include "stemmap.h"

using namespace std;

#define LOGJA LOGDEB0

// Separator char used in the Python word list output.
static const string sepchars(" ");

static CmdTalk *o_talker;
static bool o_starterror{false};
static string o_cmdpath;
static vector<string> o_cmdargs;
static std::mutex o_mutex;
static string o_taggername{"fugashi"};

static const string magicpage{"NEWPPPAGE"};

void jaStaticConfInit(RclConfig *config, const string& tagger)
{
    std::vector<std::string> cmdvec{"jasplitter.py"};
    if (config->processFilterCmd(cmdvec)) {
        auto it = cmdvec.begin();
        o_cmdpath = *it++;
        o_cmdargs.clear();
        o_cmdargs.insert(o_cmdargs.end(), it, cmdvec.end());
    }
    o_taggername = tagger;
}

// Start the Python subprocess
static bool initCmd()
{
    if (o_starterror) {
        // No use retrying
        return false;
    }
    if (o_talker) {
        return true;
    }
    if (o_cmdpath.empty()) {
        return false;
    }
    if (nullptr == (o_talker = new CmdTalk(300))) {
        o_starterror = true;
        return false;
    }
    if (!o_talker->startCmd(o_cmdpath, o_cmdargs)) {
        delete o_talker;
        o_talker = nullptr;
        o_starterror = true;
        return false;
    }
    LOGINF("JASplitter: using tagger mecab with interface " << o_taggername << '\n');
    return true;
}

typedef std::string::size_type strsz_t;

#define ISASCIIPUNCTORCTL(c) (c <= 0x7f && \
                              !((c >= 'A' && c <= 'Z') ||             \
                                (c >= 'a' && c <= 'z') ||             \
                                (c >= '0' && c <= '9')))

bool JASplitter::text_to_words(Utf8Iter& it, unsigned int *cp, int& wordpos)
{
    std::unique_lock<std::mutex> mylock(o_mutex);
    bool first_time = false;
    if (nullptr == o_talker) {
        first_time = true;
    }
    initCmd();
    if (nullptr == o_talker) {
        return false;
    }

    auto stmm = StemMap::getStemMap();
    
    LOGJA("ja_to_words: wordpos " << wordpos << "\n");

    // Current character value
    unsigned int c = 0;

    // Args dict for the tagger cmdtalk input
    unordered_map<string, string> args;
    args.insert(pair<string,string>{"data", string()});
    string& inputdata(args.begin()->second);

    if (first_time) {
        first_time = false;
        // We only use MeCab anyway...
        args.insert(pair<string,string>{"tagger", o_taggername});
    }
    
    // Walk the Japanese characters section, and accumulate tagger input.
    // We also strip some useless chars, and prepare page number computations.
    strsz_t orgbytepos = it.getBpos();
    for (; !it.eof() && !it.error(); it++) {
        c = *it;
        // Are we done with Japanese ? If we only go on with pure Japanese characters
        // (katakana+hiragana), we don't pass the Kanji to mecab because they are identified as
        // Chinese. We can't use isCHINESE() because this only returns true if Jieba is
        // configured. So we test for CJK, which only tests the ranges, and we're in trouble if
        // there is Korean text among the Japanese. Maybe textsplit could export a chinese
        // simple range test.
        if (!TextSplit::isCJK(c) && !ISASCIIPUNCTORCTL(c)) {
            // Non-Japanese: we keep on if encountering space and other ASCII punctuation. Allows
            // sending longer pieces of text to the splitter (perf). Else break, process this piece,
            // and return to the main splitter
            LOGJA("ja_to_words: broke on " << (std::string)it << '\n');
            break;
        } else {
            if (c == '\f') {
                inputdata += magicpage + " ";
            } else {
                // Replace all punctuation and other non-chars with spaces.
                if (ISASCIIPUNCTORCTL(c)) {
                    inputdata += ' ';
                } else {
                    it.appendchartostring(inputdata);
                }
            }
        }
    }
        
    LOGJA("TextSplit::ja_to_words: sending " << inputdata.size() <<
          " bytes to splitter: " << inputdata << '\n');

    // Have the slave analyse the data, check that we get a result,
    unordered_map<string, string> result;
    if (!o_talker->talk(args, result)) {
        LOGERR("Python splitter for Japanese failed for [" << inputdata << "]\n");
        return false;
    }

    // Split the resulting words
    auto resit = result.find("words");
    if (resit == result.end()) {
        LOGERR("TextSplit::ja_to_words: no words in Python splitter output\n");
        return false;
    }        
    string& wordstext = resit->second;
    vector<string> words;
    trimstring(wordstext, " \t\n\r");
    stringToTokens(wordstext, words, sepchars);
    resit = result.find("roots");
    vector<string> roots;
    if (resit != result.end()) {
        string& txt = resit->second;
        trimstring(txt, " \t\n\r");
        stringToTokens(txt, roots, sepchars);
    }

    LOGJA("ja_to_words: splitter returned " << words.size() << " words\n");
#if 0
    for (unsigned int i = 0; i < words.size(); i++) {
        std::cerr << words[i] << "  |  " << (roots.size() > i ? roots[i] : "") << '\n';
    }
#endif
    // bytepos is the position in the local fragment, not in the whole
    // text which is orgbytepos + bytepos
    strsz_t bytepos{0};
    // Adjustment for our page markers
    strsz_t pagefix{0};
    for (unsigned int i = 0; i < words.size(); i++) {
        // The tagger may strip characters from the input (e.g. multiple spaces) so that we can't
        // easily reconstruct the byte position from the concatenated terms. The output seems to be
        // always shorter than the input, so we try to look ahead for the term. Can't be too sure
        // that this works though, depending on exactly what transformation may have been applied
        // from the original input to the term. At worse, the byte positions will be wrong which
        // just impacts later highlighting
        std::string& word = words[i];
        trimstring(word, " \t\n\r");
        if (word == magicpage) {
            LOGDEB1("ja_to_words: NEWPAGE\n");
            m_sink.newpage(wordpos);
            bytepos += word.size() + 1;
            pagefix += word.size();
            continue;
        }
        if (nullptr != stmm && !roots.empty() && roots[i] != "*") {
            const std::string& root = roots[i];
            if (root != word)
                stmm->addMapping(root, word);
        }
        // Find the actual start position of the word in the section.
        strsz_t newpos = inputdata.find(word, bytepos);
        if (newpos != string::npos) {
            bytepos = newpos;
        } else {
            LOGDEB("ja_to_words: word [" << word << "] not found in text\n");
        }
        strsz_t abspos = orgbytepos + bytepos - pagefix;

        LOGJA("TextSplit::ja_to_words: word: [" << word << "]\n");
        if (!m_sink.takeword(word, wordpos, abspos, abspos + word.size())) {
            return false;
        }
        wordpos++;
        bytepos += word.size();
    }

    // Return the found non-japanese Unicode character value. The current input byte offset is kept
    // in the utf8Iter
    *cp = c;
    LOGDEB1("ja_to_words: returning\n");
    return true;
}
