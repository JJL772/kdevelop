/***************************************************************************
 *   Copyright 2006 Alexander Dymo <adymo@kdevelop.org>                    *
 *   Copyright 2006 Andreas Pakulat <apaku@gmx.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QtGui/QWidget>

class KDirOperator;
class KUrlNavigator;
class KFileItem;
class QString;
class KUrl;
class KDevFileManagerPlugin;

class FileManager: public QWidget {
    Q_OBJECT
public:
    FileManager(KDevFileManagerPlugin *plugin, QWidget* parent);
    QList<QAction*> toolBarActions() const;
private slots:
    void gotoUrl(const KUrl&);
    void updateNav( const KUrl& url );
private:
    void setupActions();
    QList<QAction*> tbActions;

    KDirOperator* dirop;
    KUrlNavigator* urlnav;
};

#endif
