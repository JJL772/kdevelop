/* $Id$
 *
 *  Copyright (C) 2001 Roberto Raggi (raggi@cli.di.unipi.it)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 */

#include "qeditor_view.h"
#include "qeditor_part.h"
#include "qeditor_search.h"
#include "qeditor.h"
#include "qeditorcodecompletion_iface_impl.h"
#include "linenumberwidget.h"
#include "markerwidget.h"
#include "levelwidget.h"
#include "gotolinedialog.h"
#include "koFind.h"
#include "koReplace.h"

#include <qregexp.h>
#include <qlayout.h>
#include <qpopupmenu.h>
#include <private/qrichtext_p.h>

#include <kdebug.h>
#include <kapplication.h>


QEditorView::QEditorView( QEditorPart* document, QWidget* parent, const char* name )
	: KTextEditor::View( document, parent, name ),
	  m_document( document ),
	  m_popupMenu( 0 )
{
    m_markerWidgetVisible = false;
    m_lineNumberWidgetVisible = false;
    m_levelWidgetVisible = true;
	    
    m_findDialog = new KoFindDialog();
    m_findDialog->setOptions( KoFindDialog::FromCursor );
    
    m_replaceDialog = new KoReplaceDialog();
    m_replaceDialog->setOptions( KoReplaceDialog::FromCursor );
    
    QHBoxLayout* lay = new QHBoxLayout( this );
    
    m_editor = new QEditor( this );
    m_lineNumberWidget = new LineNumberWidget( m_editor, this );
    m_markerWidget = new MarkerWidget( m_editor, this );
    m_levelWidget = new LevelWidget( m_editor, this );
    connect( m_levelWidget, SIGNAL(expandBlock(QTextParag*)),
	     m_editor, SLOT(expandBlock(QTextParag*)) );
    connect( m_levelWidget, SIGNAL(collapseBlock(QTextParag*)),
	     m_editor, SLOT(collapseBlock(QTextParag*)) );
    
    lay->addWidget( m_markerWidget );
    lay->addWidget( m_lineNumberWidget );
    lay->addWidget( m_levelWidget );
    lay->addWidget( m_editor );
    
    if( !isMarkerWidgetVisible() ){
	m_markerWidget->hide();
    }
    
    if( !isLineNumberWidgetVisible() ){
	m_lineNumberWidget->hide();
    }
    
    if( !isLevelWidgetVisible() ){	
	m_levelWidget->hide();
    }
    
    setFocusProxy( m_editor );
    connect( m_editor, SIGNAL(cursorPositionChanged(int, int)),
	     this, SIGNAL(cursorPositionChanged()) );
    
    m_pCodeCompletion = new CodeCompletion_Impl( this );
    connect(m_pCodeCompletion,SIGNAL(completionAborted()),
	    this,SIGNAL(completionAborted()));
    connect(m_pCodeCompletion,SIGNAL(completionDone()),
	    this,SIGNAL(completionDone()));
    connect(m_pCodeCompletion,SIGNAL(argHintHidden()),
	    this,SIGNAL(argHintHidden()));
    connect(m_pCodeCompletion,SIGNAL(completionDone(KTextEditor::CompletionEntry)),
	    this,SIGNAL(completionDone(KTextEditor::CompletionEntry)));
    connect(m_pCodeCompletion,SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString *)),
	    this,SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString *)) );
}

QEditorView::~QEditorView()
{
    delete( m_pCodeCompletion );
    m_pCodeCompletion = 0;
}


bool QEditorView::isMarkerWidgetVisible() const
{
    return m_markerWidgetVisible;
}

void QEditorView::setMarkerWidgetVisible( bool b )
{
    m_markerWidgetVisible = b;
    if( m_markerWidgetVisible ){
	m_markerWidget->show();
    } else {
	m_markerWidget->hide();
    }
}

bool QEditorView::isLineNumberWidgetVisible() const
{
    return m_lineNumberWidgetVisible;
}

void QEditorView::setLineNumberWidgetVisible( bool b )
{
    m_lineNumberWidgetVisible = b;
    if( m_lineNumberWidgetVisible ){
	m_lineNumberWidget->show();
    } else {
	m_lineNumberWidget->hide();
    }
}

bool QEditorView::isLevelWidgetVisible() const
{
    return m_levelWidgetVisible;
}

void QEditorView::setLevelWidgetVisible( bool b )
{
    m_levelWidgetVisible = b;
    if( m_levelWidgetVisible ){
	m_levelWidget->show();
    } else {
	m_levelWidget->hide();
    }
}

KTextEditor::Document* QEditorView::document() const
{
	return m_document;
}

QPoint QEditorView::cursorCoordinates()
{
	QTextCursor *cursor = m_editor->textCursor();
	QTextStringChar *chr = cursor->parag()->at( cursor->index() );
	int h = cursor->parag()->lineHeightOfChar( cursor->index() );
	int x = cursor->parag()->rect().x() + chr->x;
	int y, dummy;
	cursor->parag()->lineHeightOfChar( cursor->index(), &dummy, &y );
	y += cursor->parag()->rect().y();
	return m_editor->contentsToViewport( QPoint( x, y+h ) );
}

void QEditorView::cursorPosition(unsigned int *line, unsigned int *col)
{
	*line = cursorLine();
	*col = cursorColumn();
}

void QEditorView::cursorPositionReal(unsigned int *line, unsigned int *col)
{
	*line = cursorLine();
	*col = cursorColumnReal();
}

bool QEditorView::setCursorPosition(unsigned int line, unsigned int col)
{
#warning "TODO: implement QEditorView::setCursorPosition"
	kdDebug() << "TODO: implement QEditorView::setCursorPosition" << endl;

	m_editor->setCursorPosition( line, col );
	m_editor->ensureCursorVisible();

	return true;
}

bool QEditorView::setCursorPositionReal(unsigned int line, unsigned int col)
{
	m_editor->setCursorPosition( line, col );
	m_editor->ensureCursorVisible();
	return true;
}

unsigned int QEditorView::cursorLine()
{
	int line, col;
	m_editor->getCursorPosition( &line, &col );
	return line;
}

unsigned int QEditorView::cursorColumn()
{
	const int tabwidth = 4;
	int line, col;

	m_editor->getCursorPosition( &line, &col );
	QString text = m_editor->text( line ).left( col );
	col = 0;

	for( int i=0; i<text.length(); ++i ){
		if( text[ i ] == QChar('\t') ){
			col += tabwidth - (col % tabwidth);
		} else {
			++col;
		}
	}
	return col;
}

unsigned int QEditorView::cursorColumnReal()
{
	int line, col;
	m_editor->getCursorPosition( &line, &col );
	return col;
}

void QEditorView::copy( ) const
{
	m_editor->copy();
}

void QEditorView::cut( )
{
	m_editor->cut();
}

void QEditorView::paste( )
{
	m_editor->paste();
}

void QEditorView::installPopup( class QPopupMenu *rmb_Menu )
{
	kdDebug() << "QEditorView::installPopup()" << endl;
		m_popupMenu = rmb_Menu;
}

void QEditorView::showArgHint(QStringList functionList,
							  const QString& strWrapping,
							  const QString& strDelimiter)
{
	m_pCodeCompletion->showArgHint( functionList, strDelimiter, strDelimiter );
}

void QEditorView::showCompletionBox(QValueList<KTextEditor::CompletionEntry> complList,
									int offset,
									bool casesensitive )
{
	m_pCodeCompletion->showCompletionBox( complList, offset, casesensitive );
}

QString QEditorView::currentTextLine() const
{
	int line, col;
	m_editor->getCursorPosition( &line, &col );
	return m_editor->text( line );
}

void QEditorView::insertText( const QString& text )
{
	m_editor->insert( text );
}

void QEditorView::setLanguage( const QString& language )
{
	m_editor->setLanguage( language );
}

QString QEditorView::language() const
{
	return m_editor->language();
}

void QEditorView::contextMenuEvent( QContextMenuEvent* e )
{
	e->accept();
}

void QEditorView::gotoLine()
{
	GotoLineDialog dlg;
	dlg.setEditor( m_editor );
	dlg.exec();
}

void QEditorView::doFind()
{
#if 0
	int startLine = 0;
	int startCol = 0;
	int endLine = -1;
	int endCol = -1;

	if( m_findDialog->options() & KoReplaceDialog::FromCursor ){
		m_editor->getCursorPosition( &startLine, &startCol );
	}

	if( m_findDialog->options() & KoFindDialog::SelectedText ){
		m_editor->getSelection( &startLine, &startCol, &endLine, &endCol );
	}

	QRegExp regexp( m_findDialog->pattern() );
	unsigned int foundAtLine = 0;
	unsigned int foundAtCol = 0;
	unsigned int matchLen = 0;
	bool backwards = m_findDialog->options() & KoFindDialog::FindBackwards ? true : 0;

	bool found = m_document->searchText( startLine, startCol, regexp,
										 &foundAtLine, &foundAtCol,
										 &matchLen, backwards );
	if( found && (endLine == -1 ||
				  (!backwards && foundAtLine<=endLine && foundAtCol<=endCol) ||
				  (backwards && startLine>=foundAtLine && startCol>=foundAtLine) ) ){
		if( backwards ){
			m_document->setSelection( foundAtLine, foundAtCol+matchLen, foundAtLine, foundAtCol );
		} else {
			m_document->setSelection( foundAtLine, foundAtCol, foundAtLine, foundAtCol+matchLen );
		}
	} else {
		kapp->beep();
	}
	}
#endif
}

void QEditorView::doReplace()
{
#if 0
	m_replaceDialog->exec();
#endif
}
