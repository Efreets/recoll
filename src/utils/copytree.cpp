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
#include <sys/stat.h>

#include "copytree.h"
#include "copyfile.h"
#include "fstreewalk.h"
#include "log.h"

class CopyWalker : public FsTreeWalkerCB {
public:
    virtual FsTreeWalker::Status
      processone(const std::string &, FsTreeWalker::CbFlag, const struct PathStat & = PathStat());
    std::string srcroot;
    std::string dstroot;
    bool (*ucb)(void*, int64_t){nullptr};
    void *ctx{nullptr};
};

#ifndef _WIN32
#include <unistd.h>
bool copylink(const std::string& src, const std::string& dst)
{
    char pathbuf[PATH_MAX+1];
    ssize_t buff_len = readlink(src.c_str(), pathbuf, PATH_MAX);
    if (buff_len < 0 || buff_len == PATH_MAX) {
        LOGSYSERR("copylink", "readlink", src);
        return false;
    }
    pathbuf[buff_len] = 0;
    if (symlink(pathbuf, dst.c_str()) < 0) {
        LOGSYSERR("copylink", "symlink", dst);
        return false;
    }
    return true;
}
#endif

FsTreeWalker::Status
CopyWalker::processone(const std::string& path, FsTreeWalker::CbFlag flag,
                       const struct PathStat& pst)
{
    std::string reason;

    // Compute dest by substituting src root with dst root
    std::string dst{path};
    dst.replace(0, srcroot.size(), dstroot);

    switch (flag) {
    case FsTreeWalker::FtwDirEnter:
        LOGDEB0("Directory [" << path <<"] -> [" << dst << "]\n");
        if (path_exists(dst)) {
            if (!path_isdir(dst)) {
                LOGERR("copytree: " << dst << " exists and is not a directory\n");
                return FsTreeWalker::FtwError;
            }
            return FsTreeWalker::FtwOk;
        }
        if (!path_makepath(dst, pst.pst_mode) != 0) {
            LOGSYSERR("copytree", "mkdir", dst);
            return FsTreeWalker::FtwError;
        }
        if (ucb) {
            if (!ucb(ctx, pst.pst_size)) {
                return FsTreeWalker::FtwStop;
            }
        }
        return FsTreeWalker::FtwOk;
        break;
    case FsTreeWalker::FtwRegular:
#ifndef _WIN32
        if (pst.pst_type == PathStat::PST_SYMLINK) {
            if (!copylink(path, dst)) {
                return FsTreeWalker::FtwError;
            }
            return FsTreeWalker::FtwOk;
        }
#endif
        if (path_exists(dst)) {
            if (!path_isfile(dst)) {
                LOGERR("copytree: " << dst << " exists and is not a file\n");
                return FsTreeWalker::FtwError;
            }
        }
        if (!copyfile(path, dst, reason)) {
            LOGERR("copytree: copyfile: " << reason << "\n");
            return FsTreeWalker::FtwError;
        }
        if (ucb) {
            if (!ucb(ctx, pst.pst_size)) {
                return FsTreeWalker::FtwStop;
            }
        }
        return FsTreeWalker::FtwOk;
        break;
    default:
        return FsTreeWalker::FtwOk;
    }
    return FsTreeWalker::FtwOk;
}

bool copytree(const std::string &src, const std::string &dst,
              bool (*ucb)(void*, int64_t), void *ctx)
{
    LOGDEB("copytree: " << src << " -> " << dst << '\n');
    CopyWalker cb;
    cb.srcroot = src;
    cb.dstroot = dst;
    cb.ucb = ucb;
    cb.ctx = ctx;
    path_catslash(cb.srcroot);
    path_catslash(cb.dstroot);
    FsTreeWalker walker;
    return walker.walk(src, cb) == FsTreeWalker::FtwOk;
}
