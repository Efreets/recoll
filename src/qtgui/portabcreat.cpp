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
#if defined(_WIN32) || defined(RECOLL_AS_MAC_BUNDLE)
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
#if defined(_WIN32) || defined(RECOLL_AS_MAC_BUNDLE)
    const char *whatstr = QT_TRANSLATE_NOOP("PortabCreatW", "full Recoll installation");
    const char *wherestr = QT_TRANSLATE_NOOP("PortabCreatW", "installation");
#if defined(_WIN32)
    const char *namestr = "recoll-portable/";
#else // Apple
    const char *namestr = "recoll-portable.app/";
#endif
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
static const char *appleinstalldir = "recoll-portable.app";
static const char *appiminstalldir = "recoll-appimage-portable";

void PortabCreatW::createInstall()
{
    PRETEND_USE(appiminstalldir);
#if !defined(_WIN32) && !defined(RECOLL_AS_MAC_BUNDLE)
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
#if defined(_WIN32)
    std::string installdir = path_cat(rootdir, wininstalldir);
    std::vector<std::string> otherinstalldirs = {path_cat(rootdir, appiminstalldir),
						 path_cat(rootdir, appleinstalldir)};
#elif defined(RECOLL_AS_MAC_BUNDLE)
    std::string installdir = path_cat(rootdir, appleinstalldir);
    std::vector<std::string> otherinstalldirs = {path_cat(rootdir, appiminstalldir),
						 path_cat(rootdir, wininstalldir)};
#else
    std::string installdir = path_cat(rootdir, appleinstalldir);
    std::vector<std::string> otherinstalldirs = {path_cat(rootdir, wininstalldir),
						 path_cat(rootdir, appleinstalldir)};
#endif

    auto installfound = path_exists(installdir);
    if (installfound) {
        QMessageBox::warning(0, "Recoll",tr("It seems that an installation already exists here."));
        return;
    }

    // Check available space on target
    int pc;
    long long avmbs;
    if (!fsocc(rootdir, &pc, &avmbs)) {
        QMessageBox::warning(0, "Recoll", tr("Can't determine available space on volume."));
        return;
    }
#if defined(_WIN32)
    std::string recollinstall = path_thisexecdir();
    // Note: fstreebytes is not very accurate on windows, it ignores cluster size adjustments.
    int64_t recollmbs = fsTreeBytes(recollinstall) / 1024 / 1024;
#elif defined(RECOLL_AS_MAC_BUNDLE)
    std::string recollinstall = path_getfather(path_getfather(path_thisexecdir()));
    int64_t recollmbs = fsTreeBytes(recollinstall) / 1024 / 1024;
#else
    PathStat st;
    if (path_fileprops(appimage, &st) != 0) {
        LOGSYSERR("portabcreat", "stat", appimage);
        QMessageBox::warning(0, "Recoll", tr("Error retrieving appimage size"));
        return;
    }
    int64_t recollmbs = st.pst_size / 1024 / 1024;
#endif
    LOGDEB("PortabCreat: space: recoll mbs " << recollmbs << " vol avail mbs " << avmbs << "\n");
    if (recollmbs + 100 > avmbs) {
        QMessageBox::warning(0, "Recoll", tr("Insufficient available space on volume"));
        return;
    }

    std::string reason;

#if defined(_WIN32) || defined(RECOLL_AS_MAC_BUNDLE)
    // Create the recoll installation directory and copy recoll in there
    QProgressDialog progress(tr("Copying installation"), tr("Cancel"), 0, recollmbs, this);
    progress.setValue(0);
    progress.setMinimumDuration(1000);
    TrackProgress progressdata;
    progressdata.progress = &progress;
#ifdef _WIN32
    std::string srcdir = path_thisexecdir();
#else
    std::string srcdir = path_getfather(path_getfather(path_thisexecdir()));
#endif
    if (!copytree(srcdir, installdir, copycb, &progressdata)) {
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
    
    // Create the configuration directory if it does not exist.
    if (!path_makepath(configdir, 0755)) {
        QMessageBox::warning(0, "Recoll", tr("Could not create the configuration directory"));
        return;
    }

    std::string dst = path_cat(configdir, "recoll.conf");
    if (!path_exists(dst)) {
      ConfSimple conf(dst.c_str());
      conf.set("orgidxconfdir", configdir);
      conf.set("topdirs", rootdir);
      std::vector<std::string> dirs{installdir, configdir};
      dirs.insert(dirs.end(), otherinstalldirs.begin(), otherinstalldirs.end());
      conf.set("skippedPaths", stringsToString(dirs));
      if (!conf.ok()) {
        QMessageBox::warning(0, "Recoll", tr("Could not create the configuration file"));
        return;
      }
    }
    dst = path_cat(configdir, "backends");
    if (!path_exists(dst)) {
      if (!stringtofile("", dst, reason)) {
        LOGINF("portabcreat: could not create backends file: " << reason << '\n');
      }
    }
    createCLB->hide();
    mainExplainLBL->setText(tr("A portable Recoll configuration was created on ") +
                               path2qs(rootdir) + ". To use it, start the recoll program in " +
                               path2qs(installdir) + ".");
}

