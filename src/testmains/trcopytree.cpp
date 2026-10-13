/* Copyright (C) 2024 J.F.Dockes
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

#include <iostream>
#include <stdlib.h>

#include "copytree.h"
#include "log.h"

void usage() {
    std::cerr << "Usage: copytree src dest\n";
    exit(1);
}  
int main(int argc, const char *argv[])

{
    if (argc != 3) {
        usage();
    }
    std::string srcdir = argv[1];
    std::string dstdir = argv[2];
    Logger::getTheLog()->setLogLevel(Logger::LLDEB);
    if (!copytree(srcdir, dstdir)) {
        std::cerr << "Copy failed\n";
        return 1;
    }
    return 0;
}
