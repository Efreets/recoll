/* Copyright (C) 2012 J.F.Dockes
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
#ifndef _SYNFAMILY_H_INCLUDED_
#define _SYNFAMILY_H_INCLUDED_

/**
 * The Xapian synonyms mechanism can be used for many things beyond actual synonyms,
 * anything that would turn a string into a group of equivalents.
 * The table only has one keyspace, but this keyspace can be partially traversed
 * by specifying a key prefix with Database::synonym_keys_begin/end(prefix)
 * For a given key, the synonyms can be listed with synonyms_begin/end(key)
 *
 * The SynFamily classes defined here partition the Xapian synonyms keyspace by using
 * prefixes, providing different applications each with a family of keyspaces.
 * Two characters are reserved by the class and should not be used inside 
 * either family or member names: ':' and ';'
 * A synonym key for family "stemdb", member "french", key "somestem"  looks like:
 *    :Stm:french:somestem  -> somestem expansions
 * For a given family, a special entry, ";members" is used to list all the members, e.g.:
 *    :Stm;members  -> french
 *    :Stm;members  -> english
 *      ...
 *
 * For example a piece of the english stem expansion section would look like the following:
 *   :Stm:english:sign    -> signed
 *   :Stm:english:sign    -> signs 
 *   :Stm:english:solicit -> solicitation 
 *
 * At the moment, Recoll uses the Xapian synonyms table for stem and case/diac expansion,
 * but not at all for actual synonyms groups (see common/syngroups.h for this).
 */

#include <string>
#include <vector>

#include <xapian.h>

#include "log.h"
#include "xmacros.h"
#include "strmatcher.h"

namespace Rcl {

class XapSynFamily  {
public:
    /** 
     * Construct from readable xapian database and family name (ie: Stm)
     */
    XapSynFamily(Xapian::Database xdb, const std::string& familyname)
        : m_rdb(xdb) {
        m_prefix1 = std::string(":") + familyname;
    }
    virtual ~XapSynFamily() = default;
    
    /** Retrieve all members of this family (e.g: french english german...) */
    virtual bool getMembers(std::vector<std::string>&);

    /** Debug: list complete map for one member to stdout */
    virtual bool listMap(const std::string& membername); 

    /** Debug: expand root to list of synonyms for given member. The real implementations receive a
     * derived term and either compute or find the root before doing something like this */
    bool synExpand(const std::string& membername, 
                   const std::string& root, std::vector<std::string>& result);

    // The prefix shared by all synonym entries inside a family member
    virtual std::string memberprefix(const std::string& member) {
        return m_prefix1 + ":" + member + ":";
    }

    // The key for the "list of members" entry
    virtual std::string memberskey() {
        return m_prefix1 + ";" + "members";
    }

    Xapian::Database& getdb() {
        return m_rdb;
    }

protected:
    Xapian::Database m_rdb;
    std::string m_prefix1;
};

/** Modify ops for a synonyms family 
 * 
 * A method to add a synonym entry inside a given member would make sense, 
 * but would not be used presently as all these ops go through 
 * ComputableSynFamMember objects
 */
class XapWritableSynFamily : public XapSynFamily {
public:
    /** Construct with Xapian db open for r/w */
    XapWritableSynFamily(Xapian::WritableDatabase db, const std::string& familyname)
        : XapSynFamily(db, familyname),  m_wdb(db) {}
    virtual ~XapWritableSynFamily() = default;
    
    /** Delete all entries for one member (e.g. french), and remove from list
     * of members */
    virtual bool deleteMember(const std::string& membername);

    /** Add to list of members. Idempotent, does not affect actual expansions */
    virtual bool createMember(const std::string& membername);

    Xapian::WritableDatabase getdb() {return m_wdb;}

protected:
    Xapian::WritableDatabase m_wdb;
};

/** A functor which transforms a string */
class SynTermTrans {
public:
    virtual ~SynTermTrans() = default;
    virtual std::string operator()(const std::string&) = 0;
    virtual std::string name() {return "SynTermTrans: unknown";}
};

/** Common ancester for the expand-time classes: pure virtual */
class XapSynFamMemberCommon {
public:
    /** Expand a term to its list of synonyms. If filtertrans is set we 
     * keep only the results which transform to the same value as the input 
     * This is used for example for filtering the result of case+diac
     * expansion when only either case or diac expansion is desired.
     */
    virtual bool synExpand(const std::string& term, std::vector<std::string>& result,
                           SynTermTrans *filtertrans = nullptr) = 0;
    
    /** Same with also wildcard/regexp expansion of entry against the keys.
     * The input matcher will be modified to fit our key format. */
    virtual bool synKeyExpand(StrMatcher* in, std::vector<std::string>& result,
                              SynTermTrans *filtertrans = nullptr) = 0;
};


/** A member (set of root-synonyms associations) of a SynFamily for
 * which the root is computable from the input term.
 * The objects use a functor member to compute the term root on input
 * (e.g. compute the term sterm or casefold it
 */
class XapComputableSynFamMember : public XapSynFamMemberCommon {
public:
    XapComputableSynFamMember(Xapian::Database xdb, std::string familyname, 
                              std::string membername, SynTermTrans* trans)
        : m_family(xdb, familyname), m_membername(membername), 
          m_trans(trans), m_prefix(m_family.memberprefix(m_membername)) {}

    virtual ~XapComputableSynFamMember() = default;
    
    /** Expand a term to its list of synonyms. If filtertrans is set we 
     * keep only the results which transform to the same value as the input 
     * This is used for example for filtering the result of case+diac
     * expansion when only either case or diac expansion is desired.
     */
    virtual bool synExpand(const std::string& term, std::vector<std::string>& result,
                           SynTermTrans *filtertrans = nullptr) override;
    
    /** Same with also wildcard/regexp expansion of entry against the keys.
     * The input matcher will be modified to fit our key format. */
    virtual bool synKeyExpand(StrMatcher* in, std::vector<std::string>& result,
                              SynTermTrans *filtertrans = nullptr) override;

private:
    XapSynFamily m_family;
    std::string  m_membername;
    SynTermTrans *m_trans;
    std::string m_prefix;
};

/** Computable term root SynFamily member, modify ops */
class XapWritableComputableSynFamMember {
public:
    XapWritableComputableSynFamMember(
        Xapian::WritableDatabase xdb, std::string familyname, 
        std::string membername, SynTermTrans* trans)
        : m_family(xdb, familyname), m_membername(membername), 
          m_trans(trans), m_prefix(m_family.memberprefix(m_membername)) {
    }
    virtual ~XapWritableComputableSynFamMember() = default;
    
    virtual bool addSynonym(const std::string& term) {
        LOGDEB2("addSynonym:me " << this << " term [" << term << "] m_trans " << m_trans << "\n");
        std::string transformed = (*m_trans)(term);
        LOGDEB2("addSynonym: transformed [" << transformed << "]\n");
        if (transformed == term)
            return true;

        std::string ermsg;
        try {
            m_family.getdb().add_synonym(m_prefix + transformed, term);
        } XCATCHERROR(ermsg);
        if (!ermsg.empty()) {
            LOGERR("XapWritableComputableSynFamMember::addSynonym: xapian error " << ermsg << "\n");
            return false;
        }
        return true;
    }

    void clear() {
        m_family.deleteMember(m_membername);
    }

    void recreate() {
        clear();
        m_family.createMember(m_membername);
    }

private:
    XapWritableSynFamily m_family;
    std::string  m_membername;
    SynTermTrans *m_trans;
    std::string m_prefix;
};


/** A member (set of root-synonyms associations) of a SynFamily, where the root forms can't be
 * computed.
 * We need a reverse map for expansion term->(reverse map)->root->(direct map)->expansion.
 * This is used for Japanese, maybe for other CJK languages in the future. 
 */
class XapSynFamMember : public XapSynFamMemberCommon {
public:
    XapSynFamMember(Xapian::Database xdb, std::string familyname, 
                    std::string membername)
        : m_family(xdb, familyname), m_membername(membername), 
          m_prefix(m_family.memberprefix(m_membername)),
          m_revprefix(m_family.memberprefix(revmembername(membername))) {}

    virtual ~XapSynFamMember() = default;
    
    static std::string revmembername(const std::string& membername) {
        return membername + "-rev";
    }

    /** Expand a term to its list of synonyms. If filtertrans is set we 
     * keep only the results which transform to the same value as the input 
     * This is used for example for filtering the result of case+diac
     * expansion when only either case or diac expansion is desired.
     */
    virtual bool synExpand(const std::string& term, std::vector<std::string>& result,
                           SynTermTrans *filtertrans = nullptr) override;
    
    /** Same with also wildcard/regexp expansion of entry against the keys.
     * The input matcher will be modified to fit our key format. */
    virtual bool synKeyExpand(StrMatcher *in, std::vector<std::string>& result,
                      SynTermTrans *filtertrans = nullptr) override;

private:
    XapSynFamily m_family;
    std::string m_membername;
    std::string m_prefix;
    std::string m_revprefix;
};

/** SynFamily member (non-computable roots), modify ops */
class XapWritableSynFamMember {
public:
    XapWritableSynFamMember(
        Xapian::WritableDatabase xdb, std::string familyname, std::string membername)
        : m_family(xdb, familyname), m_membername(membername), 
          m_prefix(m_family.memberprefix(m_membername)),
          m_revprefix(m_family.memberprefix(revmembername(membername)))  {
        // For now there is only one member: japanese. Create every time, this is idempotent.
        m_family.createMember("japanese");
    }
    virtual ~XapWritableSynFamMember() = default;
    
    virtual bool addSynonym(const std::string& term, const std::string& root) {
        LOGDEB2("addSynonym:me " << this << " term [" << term << "] root [" << root << "]\n");
        if (root == term)
            return true;

        std::string ermsg;
        try {
            m_family.getdb().add_synonym(m_revprefix + term, root);
        } XCATCHERROR(ermsg);
        if (!ermsg.empty()) {
            LOGERR("XapWritableSynFamMember::addSynonym: xapian error " << ermsg << "\n");
            return false;
        }
        try {
            m_family.getdb().add_synonym(m_prefix + root, term);
        } XCATCHERROR(ermsg);
        if (!ermsg.empty()) {
            LOGERR("XapWritableSynFamMember::addSynonym: xapian error " << ermsg << "\n");
            return false;
        }
        return true;
    }

    static std::string revmembername(const std::string& membername) {
        return membername + "-rev";
    }

    void clear() {
        m_family.deleteMember(m_membername);
        m_family.deleteMember(revmembername(m_membername));
    }

    void recreate() {
        clear();
        m_family.createMember(m_membername);
    }

private:
    XapWritableSynFamily m_family;
    std::string m_membername;
    std::string m_prefix;
    std::string m_revprefix;
};


//
// Prefixes are centrally defined here to avoid collisions
//

// Lowercase accented stem to expansion. Family member name: language
static const std::string synFamStem("Stm");

// Japanese (and maybe in the future others) dual way term<->root mappings
static const std::string synFamCJK("CJK");

// Lowercase unaccented stem to expansion. Family member name: language
static const std::string synFamStemUnac("StU");

// Lowercase unaccented term to case and accent variations. Only one
// member, named "all". This set is used for separate case/diac
// expansion by post-filtering the results of dual expansion.
static const std::string synFamDiCa("DCa");

} // end namespace Rcl

#endif /* _SYNFAMILY_H_INCLUDED_ */
