/***************************************************************************
                             makeview.cpp
                             -------------------                                         

    copyright            : (C) 1999 The KDevelop Team
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#include <qdir.h>
#include <qapplication.h>
#include <klocale.h>
#include <kregexp.h>
#include "makeview.h"


class MakeListBoxItem : public ProcessListBoxItem
{
public:
    MakeListBoxItem(const QString &s,
                    const QString &filename, int linenumber);
    QString filename()
        { return fn; }
    int linenumber()
        { return lineno; }
    virtual bool isCustomItem();

private:
    QString fn;
    int lineno;
};

MakeListBoxItem::MakeListBoxItem(const QString &s,
                                 const QString &filename,
                                 int linenumber)
    : ProcessListBoxItem(s, Error), fn(filename), lineno(linenumber)
{}


bool MakeListBoxItem::isCustomItem()
{
    return true;
}


MakeView::MakeView(QWidget *parent, const char *name)
    : ProcessView(parent, name)
{
    connect( this, SIGNAL(highlighted(int)),
             this, SLOT(lineHighlighted(int)) );
}


MakeView::~MakeView()
{}


void MakeView::startJob()
{
    ProcessView::startJob();
    dirstack.clear();
    dirstack.push(new QString(QDir::currentDirPath()));
}


void MakeView::nextError()
{
    // TODO
    QApplication::beep();
}


void MakeView::prevError()
{
    // TODO
    QApplication::beep();
}


void MakeView::childFinished(bool normal, int status)
{
    QString s;
    ProcessListBoxItem::Type t;
    
    if (normal)
        {
            if (status)
                {
                    s = i18n("*** Exited with status: %1 ***").arg(status);
                    t = ProcessListBoxItem::Error;
                }
            else
                {
                    s = i18n("*** Success ***");
                    t = ProcessListBoxItem::Diagnostic;
                }
        }
    else
        {
            s = i18n("*** Compilation aborted ***");
            t = ProcessListBoxItem::Error;
        }
    insertItem(new ProcessListBoxItem(s, t));
}


void MakeView::lineHighlighted(int line)
{
    ProcessListBoxItem *i = static_cast<ProcessListBoxItem*>(item(line));
    if (i->isCustomItem())
        {
            MakeListBoxItem *gi = static_cast<MakeListBoxItem*>(i);
            emit itemSelected(gi->filename(), gi->linenumber());
        }
}


#include <iostream.h>
void MakeView::insertStdoutLine(const QString &line)
{
    // KRegExp has ERE syntax
    KRegExp enterDirRx("[^\n]*: Entering directory `([^\n]*)'$");
    KRegExp leaveDirRx("[^\n]*: Leaving directory `([^\n]*)'$");

    if (enterDirRx.match(line))
        {
            QString *dir = new QString(enterDirRx.group(1));
            dirstack.push(dir);
            cout << "Entering dir: " << *dir << endl;
        }
    else if (leaveDirRx.match(line))
        {
            cout << "Leaving dir: " << leaveDirRx.group(1) << endl;
            QString *dir = dirstack.pop();
            cout << "Now: " << *dir << endl;
            delete dir;
        }
    ProcessView::insertStdoutLine(line);
}


void MakeView::insertStderrLine(const QString &line)
{
    // KRegExp has ERE syntax
    KRegExp errorGccRx("([^: \t]+):([0-9]+):.*");
    KRegExp errorJadeRx("[a-zA-Z]+:([^: \t]+):([0-9]+):[0-9]+:[a-zA-Z]:.*");
    const int errorGccFileGroup = 1;
    const int errorJadeFileGroup = 1;
    const int errorGccRowGroup = 2;
    const int errorJadeRowGroup = 2;

    // TODO: Consider directories in file name
    if (errorGccRx.match(line))
        {
            QString fn = errorGccRx.group(errorGccFileGroup);
            int row = QString(errorGccRx.group(errorGccRowGroup)).toInt()-1;
            cout << "Gcc error in " << fn << " at line: " << row << endl;
            insertItem(new MakeListBoxItem(line, fn, row));
        }
    else if (errorJadeRx.match(line))
        {
            QString fn = errorGccRx.group(errorJadeFileGroup);
            int row = QString(errorJadeRx.group(errorJadeRowGroup)).toInt()-1;
            cout << "Jade error in " << fn << " at line: " << row << endl;
            insertItem(new MakeListBoxItem(line, fn, row));
        }
    else
        ProcessView::insertStderrLine(line);
}


void MakeView::projectClosed()
{
    clear();
}
