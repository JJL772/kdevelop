/***************************************************************************
 *   Copyright (C) 2001 by Daniel Engelschalt                              *
 *   daniel.engelschalt@gmx.net                                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _CCCONFIGWIDGET_H_
#define _CCCONFIGWIDGET_H_

#include "ccconfigwidgetbase.h"

class CppSupportPart;
class QCheckListItem;
class CCConfigWidget : public CCConfigWidgetBase
{
    Q_OBJECT
    
public:
    CCConfigWidget( CppSupportPart* part, QWidget* parent = 0, const char* name = 0 );
    virtual ~CCConfigWidget( );

public slots:
    void slotSelectTemplateGroup( const QString & );
    void interfaceFile();
    void implementationFile();
    void slotEnableChooseFiles( bool c );

    void accept( );

private:
    void initFileTemplatesTab();
    void saveFileTemplatesTab();
    
    void initCodeCompletionTab();
    void saveCodeCompletionTab();
    
private:
    CppSupportPart* m_pPart;
    QCheckListItem* m_includeGlobalFunctions;
    QCheckListItem* m_includeTypes;
    QCheckListItem* m_includeEnums;
    QCheckListItem* m_includeTypedefs;
};

#endif
