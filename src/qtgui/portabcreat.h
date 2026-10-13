/* Copyright (C) 2005-2026 J.F.Dockes
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
#ifndef _PORTABCREAT_H_INCLUDED_
#define _PORTABCREAT_H_INCLUDED_

#include "ui_portabcreat.h"

class PortabCreatW : public QDialog, public Ui::PortabCreatW {
    Q_OBJECT
public:
  PortabCreatW(QWidget *parent = 0)
      : QDialog(parent) {
        setupUi(this);
        init();
  }
public slots:
    void createInstall();

private:
    void init();
};


#endif /* _PORTABCREAT_H_INCLUDED_ */
