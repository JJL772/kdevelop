/***************************************************************************
 *   Copyright (C) 2001-2002 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
 *   Copyright (C) 2003 by Mario Scalas (VCS Support)                      *
 *   mario.scalas@libero.it                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _FILETREEWIDGET_H_
#define _FILETREEWIDGET_H_

#include <qstringlist.h>
#include <kfiletreeview.h>
// VCS Support
#include <kdevvcsfileinfoprovider.h>

class FileViewPart;
class KToggleAction;

class FileTreeWidget : public KFileTreeView
{
    Q_OBJECT
public:
    FileTreeWidget( FileViewPart *part, QWidget *parent, const char *name=0 );
    virtual ~FileTreeWidget();

    void openDirectory(const QString &dirName);
    bool shouldBeShown( KFileTreeViewItem* item );

    bool showVCSFields() const;
    bool showNonProjectFiles() const;

    QString projectDirectory();
    QStringList projectFiles();

    FileViewPart *part() const { return m_part; }

    KURL::List selectedPathUrls();
    KDevVCSFileInfoProvider *vcsFileInfoProvider() const;
    void applyHidePatterns( const QString &hidePatterns );
    QString hidePatterns() const;

public slots:
    void hideOrShow();

private slots:
    void slotItemExecuted(QListViewItem *item);
    void slotContextMenu(KListView *, QListViewItem *item, const QPoint &p);
    void slotSelectionChanged();

    void slotReloadTree();
    void slotToggleShowNonProjectFiles();
    void slotToggleShowVCSFields( bool checked );

    void addProjectFiles( QStringList const & fileList, bool constructing = false );
    void removeProjectFiles( QStringList const & fileList );
    void slotSyncWithRepository();
    void vcsDirStatusReady( const VCSFileInfoMap &modifiedFiles, void *callerData );

private:
    bool matchesHidePattern(const QString &fileName);

    FileViewPart *m_part;
    KFileTreeBranch * m_rootBranch;

    //! @fixme Is this guard really useful?
    bool m_isReloadingTree;
    //! We use this guard to avoid enabling the "reload tree" action when performing
    // syncing with remote repositories.
    bool m_isSyncingWithRepository;
    QListViewItem *m_vcsStatusRequestedItem;

    QStringList m_hidePatterns;
    QStringList m_projectFiles;

    QPtrList<KFileTreeViewItem> m_selectedItems;

    KToggleAction *m_actionToggleShowVCSFields,
        *m_actionToggleShowNonProjectFiles;
};

#endif
