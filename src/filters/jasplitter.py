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

import MeCab

class Processor(object):
    def __init__(self, proto):
        self.proto = proto
        self.init = False

    def _init_tagger(self, taggername):
        # Nothing to do for now, really
        if taggername != "MeCab":
            raise Exception(f"Bad analyser name {taggername}")
        self.tagger = MeCab.Tagger("-Owakati")
        self.init = True

    def process(self, params):
        if "data" not in params:
            return {"error": "No data field in parameters"}
        # proto.log(f"DATA: {params['data']}")
        if not self.init:
            self._init_tagger(params["tagger"])

        wordstring = self.tagger.parse(params["data"])
        return {"words" : wordstring}


proto = cmdtalk.CmdTalk()
processor = Processor(proto)
if len(sys.argv) == 1:
    cmdtalk.main(proto, processor)
elif len(sys.argv) > 1:    
    wordstring = processor.process({"tagger" : "MeCab", "data" : sys.argv[1]})
    print(f"{wordstring}")
    
