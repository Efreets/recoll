/* Copyright (C) 2020-2021 J.F.Dockes
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

#include "portabcreat.h"

#include <QShortcut>
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>
#include <QApplication>

#include "log.h"
#include "recoll.h"
#include "pathut.h"
#ifdef _WIN32
#include "fstreewalk.h"
#include "copytree.h"
#endif
#include "conftree.h"
#include "smallut.h"
#include "copyfile.h"

static const char *toolExplain =
    QT_TRANSLATE_NOOP("PortabCreatW", R"(
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd">
<html><head>
<meta name="qrichtext" content="1" />
<style type="text/css">
  p, li { white-space: pre-wrap; }
  p { margin-top:0px; margin-bottom:0px;margin-left:0px; margin-right:0px;
    -qt-block-indent:0; text-indent:0px;}
</style>
</head>
<body style="font-family:'Sans Serif'; font-size:10pt; font-weight:400; font-style:normal;">
<p>This tool creates a portable Recoll configuration on a movable volume (e.g. thumbdrive). 
It will copy the %1 to the volume and create an initial configuration there. 
The directory names for the %2 (<em>%3</em>) and configuration (<em>recoll-config</em>) are 
used to detect a portable installation and should not be changed.
</p>
</body></html>
                                      )");

static const char *buttonExplain =
    QT_TRANSLATE_NOOP("PortabCreatW", R"(
This will let you chose the location, and initialize it. 
You will then need to start a new Recoll instance from the installation on the volume to run 
indexing and queries there.
                                      )");

void PortabCreatW::init()
{
#ifdef _WIN32
    const char *whatstr = QT_TRANSLATE_NOOP("PortabCreatW", "full Recoll installation");
    const char *wherestr = QT_TRANSLATE_NOOP("PortabCreatW", "installation");
    const char *namestr = "recoll-portable/";
#else
    const char *whatstr = QT_TRANSLATE_NOOP("PortabCreatW", "AppImage executable");
    const char *wherestr = QT_TRANSLATE_NOOP("PortabCreatW", "executable location");
    const char *namestr = "recoll-appimage-portable/";
#endif
    mainExplainLBL->setText(tr(toolExplain).arg(tr(whatstr)).arg(tr(wherestr)).arg(tr(namestr)));
    createCLB->setDescription(tr(buttonExplain));
}

struct TrackProgress {
    int64_t bytescopied{0};
    QProgressDialog* progress{nullptr};
};
bool copycb(void *ctx, int64_t bytes)
{
    auto tp = static_cast<TrackProgress*>(ctx);
    tp->bytescopied += bytes;
    tp->progress->setValue(tp->bytescopied/(1024*1024));
    qApp->processEvents();
    return !tp->progress->wasCanceled();
}

static const char *wininstalldir = "recoll-portable";
static const char *appiminstalldir = "recoll-appimage-portable";

void PortabCreatW::createInstall()
{
#ifndef _WIN32
    const char *envapp = getenv("APPIMAGE");
    if (nullptr == envapp)
        return;
    std::string appimage(envapp);
#endif
    QFileDialog fdialog(this, tr("Choose location for portable installation"));
    fdialog.setAcceptMode(QFileDialog::AcceptOpen);
    fdialog.setFileMode(QFileDialog::Directory);
    fdialog.setOption(QFileDialog::ShowDirsOnly);
    if (fdialog.exec() == 0) 
        return;
    QStringList dirl = fdialog.selectedFiles();
    if (dirl.size() != 1) {
        // Can't happen ?
        QMessageBox::warning(0, "Recoll", tr("Choose exactly one directory"));
        return;
    }
    std::string rootdir(qs2path(dirl[0]));
    std::string configdir = path_cat(rootdir, "recoll-config");
#ifdef _WIN32
    std::string installdir = path_cat(rootdir, wininstalldir);
    std::string otherinstalldir = path_cat(rootdir, appiminstalldir);
#else
    std::string installdir = path_cat(rootdir, appiminstalldir);
    std::string otherinstalldir = path_cat(rootdir, wininstalldir);
#endif
    auto configfound = path_exists(configdir);
    auto installfound = path_exists(installdir);
    if (configfound || installfound) {
        QMessageBox::warning(0, "Recoll",tr("It seems that an installation already exists here."));
        return;
    }
    int pc;
    long long avmbs;
    if (!fsocc(rootdir, &pc, &avmbs)) {
        QMessageBox::warning(0, "Recoll", tr("Can't determine available space on volume."));
        return;
    }
#ifdef _WIN32
    std::string recollinstall = path_thisexecdir();
    // Note: fstreebytes is not very accurate on windows, it ignores cluster size adjustments.
    int64_t recollmbs = fsTreeBytes(recollinstall) / 1024 / 1024;
    LOGDEB("PorabCreat: space: recoll mbs " << recollmbs << " vol avail mbs " << avmbs << "\n");
#else
    PathStat st;
    if (path_fileprops(appimage, &st) != 0) {
        LOGSYSERR("portabcreat", "stat", appimage);
        QMessageBox::warning(0, "Recoll", tr("Error retrieving appimage size"));
        return;
    }
    int64_t recollmbs = st.pst_size / 1024 / 1024;
#endif
    if (recollmbs + 100 > avmbs) {
        QMessageBox::warning(0, "Recoll", tr("Insufficient available space on volume"));
        return;
    }

    std::string reason;

#ifdef _WIN32
    // Create the recoll installation directory and copy recoll in there
    QProgressDialog progress(tr("Copying installation"), tr("Cancel"), 0, recollmbs, this);
    progress.setValue(0);
    progress.setMinimumDuration(1000);
    TrackProgress progressdata;
    progressdata.progress = &progress;
    if (!copytree(path_thisexecdir(), installdir, copycb, &progressdata)) {
        QMessageBox::warning(0, "Recoll", tr("Could not copy recoll installation"));
        return;
    }
#else
    path_makepath(installdir, 0755);
    std::string appdst = path_cat(installdir, path_getsimple(appimage));
    if (!copyfile(appimage, appdst, reason)) {
        LOGERR("Could not copy recoll appimage: " << reason  << "\n");
        QMessageBox::warning(0, "Recoll", tr("Could not copy recoll appimage"));
        return;
    }
#endif
    
    // Create the configuration directory
    if (!path_makepath(configdir, 0755)) {
        QMessageBox::warning(0, "Recoll", tr("Could not create the configuration directory"));
        return;
    }

    std::string dst = path_cat(configdir, "recoll.conf");
    ConfSimple conf(dst.c_str());
    conf.set("orgidxconfdir", configdir);
    conf.set("topdirs", rootdir);
    std::vector<std::string> dirs{installdir, otherinstalldir, configdir};
    conf.set("skippedPaths", stringsToString(dirs));
    if (!conf.ok()) {
        QMessageBox::warning(0, "Recoll", tr("Could not create the configuration file"));
        return;
    }
    dst = path_cat(configdir, "backends");
    if (!stringtofile("", dst, reason)) {
        LOGINF("portabcreat: could not create backends file: " << reason << '\n');
    }
    createCLB->hide();
    mainExplainLBL->setText(tr("A portable Recoll configuration was created on ") +
                               path2qs(rootdir) + ". To use it, start the recoll program in " +
                               path2qs(installdir) + ".");
}

