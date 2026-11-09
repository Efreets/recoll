#ifndef _STEMMAP_H_INCLUDED_
#define _STEMMAP_H_INCLUDED_
/* Copyright (C) 2026 J.F.Dockes
 *
 * License: GPL 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// The global word derivation map used for languages where we want stem computation to happen in
// text context (ONLY Japanese at the moment). This is updated while indexing, and used in place of
// the term list walk to update the stemdb at the end.
// Singleton class, lock-protected.
//
// This will probably not need changes if we use it for other languages, because the CJK languages
// we could use it for are anyway discriminated on Unicode range, so there is no possible collision?
// Possible issue with Kanji ?

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class StemMap {
public:
    // Return the singleton pointer. A nullptr return is normal and means that stemmap has not been
    // enabled.
    static StemMap *getStemMap();
    static void enable(bool on);
    
    bool addMappings(const std::vector<std::string>& roots, const std::vector<std::string>& words);
    bool addMapping(const std::string& root, const std::string& word);
    void clear();
    
    // Exposed value type: root -> space-separated words. This is the logical
    // content of the map, not a reference to its internal container.
    using value_type = std::pair<const std::string, std::string>;

    // Read-only forward iterator over the mappings. The actual state
    // (a std::map const_iterator) is hidden behind Impl, defined in the
    // implementation file, so no internal container type leaks into this
    // header.
    //
    // Iterators are invalidated by clear(). addMappings() does not
    // invalidate existing std::map iterators, but it must not run
    // concurrently with iteration: callers must ensure that no thread
    // modifies the map while it is being iterated over.
    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = StemMap::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        const_iterator();
        ~const_iterator();
        const_iterator(const const_iterator&);
        const_iterator& operator=(const const_iterator&);
        const_iterator(const_iterator&&) noexcept;
        const_iterator& operator=(const_iterator&&) noexcept;

        reference operator*() const;
        pointer operator->() const;

        const_iterator& operator++();
        const_iterator operator++(int);

        bool operator==(const const_iterator& other) const;
        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }
    private:
        friend class StemMap;
        struct Impl;
        explicit const_iterator(std::unique_ptr<Impl>&& impl);

        std::unique_ptr<Impl> m_impl;
    };
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    StemMap(const StemMap&) = delete; 
    StemMap& operator=(const StemMap&) = delete;
    class Internal;
private:
    Internal *m;
    StemMap();
};

#endif /* _STEMMAP_H_INCLUDED_ */
