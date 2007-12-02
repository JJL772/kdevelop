/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ANNOTATEVIEW_H
#define ANNOTATEVIEW_H

#include <QDialog>
#include <KJob>

#include <vcsannotation.h>

#include "ui_annotateview.h"

class CvsPlugin;
class CvsJob;

/**
 * Shows the output from @code cvs annotate @endcode in a nice way.
 * Create a CvsJob by calling CvsProxy::editors() and connect the job's
 * result(KJob*) signal to AnnotateView::slotJobFinished(KJob* job)
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 */
class AnnotateView : public QWidget, private Ui::AnnotateViewBase
{
Q_OBJECT
public:
    explicit AnnotateView(CvsPlugin* plugin, CvsJob* job=0, QWidget *parent = 0);
    virtual ~AnnotateView();

    /**
     * Parses the output generated by a @code cvs annotate @endcode command and
     * fills the given @p annotateInfo with all the information about each line of the file.
     * @param jobOutput Pass in the plain output of a @code cvs editors @endcode job
     * @param annotateInfo This will be filled with the annotation info of the file
     *                     based uppon @p jobOutput.
     */
    static void parseOutput(const QString& jobOutput, const QString& workingDirectory,
                            KDevelop::VcsAnnotation& annotateInfo);

private slots:
    /**
     * Connect a job's result() signal to this slot. When called, the output from the job
     * will be passed to the parseOutput() method and any found locker information will be
     * displayed.
     * @note If you pass a CvsJob object to the ctor, it's result() signal
     *       will automatically be connected to this slot.
     */
    void slotJobFinished(KJob* job);

private:
    CvsPlugin* m_plugin;
    QString m_output;
};

#endif
