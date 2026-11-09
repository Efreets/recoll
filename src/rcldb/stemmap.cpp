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
#include "stemmap.h"

#include <mutex>
#include <map>
#include <string>

#include "log.h"

class StemMap::Internal {
public:
    using Map = std::map<std::string, std::string>;
    Map allstems;
};

static bool enabled{false};
static std::mutex theMapMutex;
static StemMap *theStemMap;

StemMap* StemMap::getStemMap()
{
    std::lock_guard<std::mutex> lck(theMapMutex);
    if (!enabled) {
        return nullptr;
    }
    if (nullptr == theStemMap) {
        theStemMap = new StemMap;
    }
    return theStemMap;
}

StemMap::StemMap()
    : m(new Internal)
{
}

void StemMap::enable(bool onoff)
{
    enabled = onoff;
}

bool StemMap::addMappings(const std::vector<std::string>& roots,
                          const std::vector<std::string>& words)
{
    std::lock_guard<std::mutex> lck(theMapMutex);
    if (words.size() != roots.size()) {
        LOGERR("StemMap::addMappings: differing input sizes: " <<
               words.size() << " " << roots.size() << '\n');
        return false;
    }
    for (unsigned int i = 0; i < words.size(); i++) {
        m->allstems[roots[i]] += words[i] + " ";
    }
    return true;    
}
bool StemMap::addMapping(const std::string& root, const std::string& word)
{
    std::lock_guard<std::mutex> lck(theMapMutex);
    m->allstems[root] += word + " ";
    return true;    
}

void StemMap::clear()
{
    std::lock_guard<std::mutex> lck(theMapMutex);
    m->allstems.clear();
}
    
// Hidden state of the public iterator. Defined here so that the map type
// stays private to this implementation file.
struct StemMap::const_iterator::Impl {
    explicit Impl(Internal::Map::const_iterator it) : it(it) {}
    Internal::Map::const_iterator it;
};

StemMap::const_iterator::const_iterator() = default;
StemMap::const_iterator::~const_iterator() = default;
StemMap::const_iterator::const_iterator(const const_iterator& other)
    : m_impl(other.m_impl ? std::unique_ptr<Impl>(new Impl(*other.m_impl))
                          : nullptr)
{
}

StemMap::const_iterator&
StemMap::const_iterator::operator=(const const_iterator& other)
{
    if (this != &other) {
        m_impl = other.m_impl ? std::unique_ptr<Impl>(new Impl(*other.m_impl))
                              : nullptr;
    }
    return *this;
}

StemMap::const_iterator::const_iterator(const_iterator&&) noexcept = default;
StemMap::const_iterator&
StemMap::const_iterator::operator=(const_iterator&&) noexcept = default;

StemMap::const_iterator::const_iterator(std::unique_ptr<Impl>&& impl)
    : m_impl(std::move(impl))
{
}

StemMap::const_iterator::reference
StemMap::const_iterator::operator*() const
{
    return *m_impl->it;
}

StemMap::const_iterator::pointer
StemMap::const_iterator::operator->() const
{
    return &*m_impl->it;
}

StemMap::const_iterator& StemMap::const_iterator::operator++()
{
    ++m_impl->it;
    return *this;
}

StemMap::const_iterator StemMap::const_iterator::operator++(int)
{
    const_iterator tmp(*this);
    ++m_impl->it;
    return tmp;
}

bool StemMap::const_iterator::operator==(const const_iterator& other) const
{
    if (!m_impl && !other.m_impl) {
        return true;
    }
    if (!m_impl || !other.m_impl) {
        return false;
    }
    return m_impl->it == other.m_impl->it;
}

StemMap::const_iterator StemMap::begin() const
{
    return const_iterator(
        std::unique_ptr<const_iterator::Impl>(
            new const_iterator::Impl(m->allstems.cbegin())));
}

StemMap::const_iterator StemMap::end() const
{
    return const_iterator(
        std::unique_ptr<const_iterator::Impl>(
            new const_iterator::Impl(m->allstems.cend())));
}

