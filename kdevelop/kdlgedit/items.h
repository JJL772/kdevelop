/***************************************************************************
                           item.h  -  description
                             -------------------
    begin                : Thu Apr 08 1999
    copyright            : (C) 1999 by Pascal Krahmer
    email                : pascal@beast.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef KDLG_ITEMS_H
#define KDLG_ITEMS_H

#include <qframe.h>
#include "itemsglobal.h"
#include "kdlgpropwidget.h"
#include <kcursor.h>
#include "kdlgeditwidget.h"
#include <kruler.h>
#include <qlineedit.h>
#include <qpushbutton.h>

class KDlgEditWidget;

/**
  * @short Base item inherited by each complexer item.
  *
  * This is the base item inherited by each other item. It defines
  * the default methods and variables needed for each and every item.
  */
class KDlgItem_Base : public QObject
{
  Q_OBJECT
  public:
    /**
     * @param editwid The editwidget which creates the item.
     * @param parent The parent widget (used to create the child). Get it through the KDlgItem_Base::getItem() method of the parent items' class.
     * @param ismainwidget Is TRUE the ites cannot be moved and if it is resized, the rulers are also resized.
     * @param name Just passed to the items' widgets' constructor
    */
    KDlgItem_Base( KDlgEditWidget* editwid = 0, QWidget *parent = 0, bool ismainwidget = false, const char* name = 0 );
    virtual ~KDlgItem_Base() {}

    /**
     * Returns the type of the items class. Has to be overloaded in order to return the right type.
     * (i.e. returns "QPushButton" for a PushButton item.
    */
    virtual QString itemClass() { return QString("[Base]"); }

    /**
     * Returns a pointer to the items widget.
    */
    virtual QWidget *getItem() { return item; }

    /**
     * Rebuilds the item from its properties. If <i>it</i> is 0 the
     * idget stored in this class (<i>item</i>), otherwise
     * the one <i>it</i> points to is changed.
     * If you reimplement this method (and you should do so) you
     * can call repaintItem(item) in order to let your item be handled.
     * This makes sense since every widget in QT inherites QWidget so
     * you won't need to set the properties of the QWidget in your code.
    */
    virtual void repaintItem(QWidget *it = 0);

    /**
     * Returns a pointer to the properties of this item. See KDlgPropertyBase for
     * more details.
    */
    KDlgPropertyBase *getProps() { return props; }

    /**
     * Returns a pointer to the child database. If you call this method for
     * a KDlgItem_Widget you�ll get a pointer otherwise 0 because only a QWidget
     * may contain childs.
    */
    KDlgItemDatabase *getChildDb() { return childs; }

    /**
     * Returns the number child items if called for a KDlgItem_Widget otherwise 0.
    */
    int getNrOfChilds() { if (childs) return childs->numItems(); else return 0; }

    /**
     * Adds a child item to the children database.(Use only for KDlgItem_Widget's!!)<br><br>
     * Returns true if successful, otherwise false.
    */
    bool addChild(KDlgItem_Base *itm) { if (childs) return childs->addItem(itm); else return false; }

    /**
     * Returns a pointer to the KDlgEditWidget class which created this item.
    */
    KDlgEditWidget* getEditWidget() { return editWidget; }

    /**
     * Has to be overloaded ! Sets the state if this item to selected. (That means the border and the rectangles are painted)
    */
    virtual void select() { }

    /**
     * Has to be overloaded ! Sets the state if this item to not selected. (That means the border and the rectangles are NOT painted)
    */
    virtual void deselect() { }

    /**
     * Removes this item including all children (if a KDlgItem_Widget) from the dialog.
    */
    void deleteMyself();

    bool isMainWidget;
  protected:
    int Prop2Bool(QString name);
    int Prop2Int(QString name, int defaultval=0);
    QString Prop2Str(QString name);


    KDlgItemDatabase *childs;
    QWidget *item;
    KDlgPropertyBase *props;
    KDlgEditWidget* editWidget;
};


/**
 * This class is inherited from KDlgItem_Base. It defines an item using some #defines and
 * and including the item_class.cpp.inc file which contains the definition.
*/
class KDlgItem_Widget : public KDlgItem_Base
{
  Q_OBJECT

  public:
    KDlgItem_Widget( KDlgEditWidget* editwid , QWidget *parent, bool ismainwidget, const char* name = 0 );

  #define classname KDlgItem_Widget       // the classname
  #define widgettype QFrame               // type of the items widget
  #define classdesc "QWidget"             // string returned by itemClass()
  // use this macro to add lines to the MyWidget class of the class (see item_class.cpp.inc)
  #define MyWidgetAdd  public: MyWidget(KDlgItem_Widget* wid, QWidget* parent = 0, bool isMainWidget = false, const char* name = 0);
  #include "item_class.cpp.inc"           // includes the stuff
};


class KDlgItem_LineEdit : public KDlgItem_Base
{
  Q_OBJECT

  #define classname KDlgItem_LineEdit
  #define widgettype QLineEdit
  #define classdesc "QLineEdit"
  #define MyWidgetAdd virtual void keyPressEvent ( QKeyEvent * ) {}
  #include "item_class.cpp.inc"
};


class KDlgItem_PushButton : public KDlgItem_Base
{
  Q_OBJECT

  #define classname KDlgItem_PushButton
  #define widgettype QPushButton
  #define classdesc "QPushButton"
  #include "item_class.cpp.inc"
};


#endif