#!/usr/bin/env python3
#################################
# Copyright (C) 2026 J.F.Dockes
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the
#   Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
########################################################

# Interface to the Mecab Japanese text analyser: we receive text from our parent process and
# have it segmented by the analyser, then return the results.

import sys
import cmdtalk

_TAGGER_NONE = 0
_TAGGER_MECAB = 1
_TAGGER_FUGASHI = 2

class Processor(object):
    def __init__(self, proto):
        self.proto = proto
        self.init = False
        self.tkind = _TAGGER_NONE
    def _init_tagger(self, taggername):
        # Nothing to do for now, really
        if taggername.lower() == "mecab":
            global MeCab
            import MeCab
            self.tagger = MeCab.Tagger("-Owakati")
            self.tkind = _TAGGER_MECAB
        elif taggername.lower() == "fugashi":
            global fugashi
            import fugashi
            self.tagger = fugashi.Tagger("-Owakati")
            self.tkind = _TAGGER_FUGASHI
        else:
            raise Exception(f"Bad analyser name {taggername}")
        self.init = True

    def realroot(self, word, root):
        # For translitterations of e.g. english words, mecab returns something like
        # "japaneseword - original" (e.g. プレビュー-preview). For now we discard these.
        if root.find('-') != -1:
            return "*"
        if root == word:
            return "*"
        return root
    
    def process(self, params):
        if "data" not in params:
            return {"error": "No data field in parameters"}
        # proto.log(f"DATA: {params['data']}")
        if not self.init:
            self._init_tagger(params["tagger"])
        words = ""
        roots = ""
        if self.tkind == _TAGGER_MECAB:
            node = self.tagger.parseToNode(params["data"])

            while node:
                # Skip BOS (Beginning of sentence) and EOS (End of sentence) nodes
                if node.stat in (MeCab.MECAB_BOS_NODE, MeCab.MECAB_EOS_NODE):
                    node = node.next
                    continue
    
                # Split the comma-separated feature string
                features = node.feature.split(',')
                # Surface form (what appeared in the text)
                surface = node.surface
                # In IPAdic, features[6] holds the dictionary/base form (原形)
                # Check length in case unknown words return fewer elements
                rootpos = 7
                root_form = features[rootpos] if \
                    len(features) > rootpos and features[rootpos] != '*' else surface
                #print(f"{surface} | {root_form}")
                words += surface + " "
                roots += self.realroot(surface, root_form) + " "
                node = node.next
        elif self.tkind == _TAGGER_FUGASHI:
            for word in self.tagger(params["data"]):
                if not word.feature.lemma:
                    continue
                #print(f"{word.surface} | {word.feature.lemma}")
                words += word.surface + " "
                roots += self.realroot(word.surface, word.feature.lemma) + " "
        else:
            raise Exception(f"Bad analyser kind {self.tkind}")
        return {"words" : words, "roots" : roots}


proto = cmdtalk.CmdTalk()
processor = Processor(proto)
if len(sys.argv) == 1:
    cmdtalk.main(proto, processor)
elif len(sys.argv) > 1:
    data = ""
    for w in sys.argv[1:]:
        data += w + " "
#    taggername = "MeCab"
    taggername = "fugashi"
    result = processor.process({"tagger" : taggername, "data" : data})
    for w,r in zip(result["words"].split(), result["roots"].split()):
        print(f"{w} | {r}")
    
