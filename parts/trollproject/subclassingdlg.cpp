/***************************************************************************
 *   Copyright (C) 2002 by Jakob Simon-Gaarde                              *
 *   jsgaarde@tdcspace.dk                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "subclassingdlg.h"
#include <qradiobutton.h>
#include <qstringlist.h>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <kfiledialog.h>
#include <klineedit.h>
#include <qpushbutton.h>
#include <domutil.h>
#include <qdom.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <qfile.h>
#include <qregexp.h>
#include "classparser.h"


#define WIDGET_CAPTION_NAME "widget/property|name=caption/string"
#define WIDGET_CLASS_NAME   "class"
#define WIDGET_SLOTS        "slots"
#define WIDGET_FUNCTIONS    "functions"

// All widgets
#define SLOT_ACCEPT SlotItem(m_slotView,"accept()","virtual","protected","void",false,true)
#define SLOT_REJECT SlotItem(m_slotView,"reject()","virtual","protected","void",false,true)

// Wizards
#define SLOT_BACK SlotItem(m_slotView,"back()","virtual","protected","void",false,true)
#define SLOT_NEXT SlotItem(m_slotView,"next()","virtual","protected","void",false,true)
#define SLOT_HELP SlotItem(m_slotView,"help()","virtual","protected","void",false,true)


SlotItem::SlotItem(QListView *parent,const QString &methodName,
                   const QString &specifier,
                   const QString &access, const QString &returnType,
                   bool isFunc, bool callBaseClass)
: QCheckListItem(parent,methodName,QCheckListItem::CheckBox)
{
  setOn(true);
  m_methodName = methodName;
  m_access = access == "" ? (const QString) "public" : access;
  m_specifier = specifier == "" ? (const QString) "virtual" : specifier;
  m_returnType = returnType == "" ? (const QString) "void" : returnType;
  m_isFunc = isFunc;
  m_callBaseClass = callBaseClass;
  setText(0,m_methodName);
  setText(1,m_access);
  setText(2,m_specifier);
  setText(3,m_returnType);
  setText(4,m_isFunc ? "Function" : "Slot");
  if (m_access=="private" ||
      m_specifier=="non virtual")
  {
    setOn(false);
    setEnabled(false);
  }
  if (m_specifier=="pure virtual")
  {
    setOn(true);
    setEnabled(false);
  }

}


SubclassingDlg::SubclassingDlg(const QString &formFile,QStringList &newFileNames,
                               QWidget* parent, const char* name,bool modal, WFlags fl)
: SubclassingDlgBase(parent,name,modal,fl),
m_newFileNames(newFileNames)
//=================================================
{
  m_formFile = formFile;
  readUiFile();
  m_creatingNewSubclass = true;
}


SubclassingDlg::SubclassingDlg(const QString &formFile,const QString &filename,QStringList &dummy,
                               QWidget* parent, const char* name,bool modal, WFlags fl)
: SubclassingDlgBase(parent,name,modal,fl),
m_newFileNames(dummy)
//=================================================
{
  m_formFile = formFile;
  readUiFile();
  m_creatingNewSubclass = false;

  ClassStore classcontainer;
  CClassParser parser(&classcontainer);
  parser.parse(filename + ".h");
  QStringList classes(classcontainer.getSortedClassNameList());
  typedef QValueList<ParsedMethod*> SlotList;
  for (uint i=0; i<classes.count(); i++)
  {
    QMessageBox::information(0,"Class",classes[i]);
    ParsedClass *cls = classcontainer.getClassByName(classes[i]);
    cls->out();
    SlotList slotlist = cls->getSortedMethodList();
    SlotList::iterator it;
    for ( it = slotlist.begin(); it != slotlist.end(); ++it )
    {
      ParsedMethod *method = *it;
    }
  }
}


void SubclassingDlg::readUiFile()
{
  QStringList splitPath = QStringList::split('/',m_formFile);
  m_formName = QStringList::split('.',splitPath[splitPath.count()-1])[0]; // "somedlg.ui" = "somedlg"
  splitPath.pop_back();
  m_formPath = "/" + splitPath.join("/"); // join path to ui-file

  m_btnOk->setEnabled(false);
  QDomDocument doc;

  DomUtil::openDOMFile(doc,m_formFile);
  m_baseClassName = DomUtil::elementByPathExt(doc,WIDGET_CLASS_NAME).text();

  m_baseCaption = DomUtil::elementByPathExt(doc,WIDGET_CAPTION_NAME).text();
  setCaption(i18n("Create subclass of ")+m_baseClassName);

  // Special widget specific slots
  SlotItem *newSlot;
  m_qtBaseClassName = DomUtil::elementByPathExt(doc,"widget").attribute("class","QDialog");

  if (m_qtBaseClassName=="QMainWindow")
    m_canBeModal = false;
  else
    m_canBeModal = true;
  newSlot = new SLOT_ACCEPT;
  newSlot->setOn(false);
  m_slotView->insertItem(newSlot);
  m_slots << newSlot;
  newSlot = new SLOT_REJECT;
  newSlot->setOn(false);
  m_slotView->insertItem(newSlot);
  m_slots << newSlot;

  if (m_qtBaseClassName == "QWizard")
  {
    newSlot = new SLOT_NEXT;
    m_slotView->insertItem(newSlot);
    m_slots << newSlot;
    newSlot = new SLOT_BACK;
    m_slotView->insertItem(newSlot);
    m_slots << newSlot;
    newSlot = new SLOT_HELP;
    newSlot->setOn(false);
    m_slotView->insertItem(newSlot);
    m_slots << newSlot;
  }

  QDomElement slotsElem = DomUtil::elementByPathExt(doc,WIDGET_SLOTS);
  QDomNodeList slotnodes = slotsElem.childNodes();

  for (unsigned int i=0; i<slotnodes.count();i++)
  {
    QDomElement slotelem = slotnodes.item(i).toElement();
    newSlot = new SlotItem(m_slotView,slotelem.text(),
                           slotelem.attributeNode("specifier").value(),
                           slotelem.attributeNode("access").value(),
                           slotelem.attributeNode("returnType").value(),false);
    m_slotView->insertItem(newSlot);
    m_slots << newSlot;
  }

  QDomElement funcsElem = DomUtil::elementByPathExt(doc,WIDGET_FUNCTIONS);
  QDomNodeList funcnodes = funcsElem.childNodes();
  SlotItem *newFunc;
  for (unsigned int i=0; i<funcnodes.count();i++)
  {
    QDomElement funcelem = funcnodes.item(i).toElement();
    newFunc = new SlotItem(m_slotView,funcelem.text(),
                           funcelem.attributeNode("specifier").value(),
                           funcelem.attributeNode("access").value(),
                           funcelem.attributeNode("returnType").value(),true);
    m_slotView->insertItem(newFunc);
    m_slots << newFunc;
  }


}



SubclassingDlg::~SubclassingDlg()
//===============================
{
}


void SubclassingDlg::updateDlg()
//==============================
{
}

void SubclassingDlg::replace(QString &string, const QString& search, const QString& replace)
//==========================================================================================
{
  int nextPos = string.find(search);
  unsigned int searchLength = search.length();
  while (nextPos>-1)
  {
    string = string.replace(nextPos,searchLength,replace);
    nextPos = string.find(search,nextPos+replace.length());
  }
}

bool SubclassingDlg::loadBuffer(QString &buffer, const QString& filename)
//======================================================================
{
  // open file and buffer it
  QFile dataFile(filename);
  if (!dataFile.open(IO_ReadOnly))
    return false;
  char *temp = new char[dataFile.size()+1];
  dataFile.readBlock(temp,dataFile.size());
  temp[dataFile.size()]='\0';
  buffer = temp;
  delete temp;
  dataFile.close();
  return true;
}

bool SubclassingDlg::replaceKeywords(QString &buffer,bool canBeModal)
//===================================================================
{
  replace(buffer,"$NEWFILENAMEUC$",m_edFileName->text().upper());
  replace(buffer,"$BASEFILENAMELC$",m_formName.lower());
  replace(buffer,"$NEWCLASS$",m_edClassName->text());
  replace(buffer,"$BASECLASS$",m_baseClassName);
  replace(buffer,"$NEWFILENAMELC$",m_edFileName->text().lower());
  if (canBeModal)
  {
    replace(buffer,"$CAN_BE_MODAL_H$",", bool modal = FALSE");
    replace(buffer,"$CAN_BE_MODAL_CPP1$",", bool modal");
    replace(buffer,"$CAN_BE_MODAL_CPP2$",", modal");
  }
  else
  {
    replace(buffer,"$CAN_BE_MODAL_H$","");
    replace(buffer,"$CAN_BE_MODAL_CPP1$","");
    replace(buffer,"$CAN_BE_MODAL_CPP2$","");
  }

  return true;
}

bool SubclassingDlg::saveBuffer(QString &buffer, const QString& filename)
//=======================================================================
{
  // save buffer

  QFile dataFile(filename);
  if (!dataFile.open(IO_WriteOnly))
    return false;
  dataFile.writeBlock((buffer+"\n").ascii(),(buffer+"\n").length());
  dataFile.close();
  return true;
}


void SubclassingDlg::accept()
//===========================
{
  unsigned int i;

  // h - file

  QString public_slot =
    "/*$PUBLIC_SLOTS$*/\n  ";

  QString protected_slot =
    "/*$PROTECTED_SLOTS$*/\n  ";

  QString public_func =
    "/*$PUBLIC_FUNCTIONS$*/\n  ";

  QString protected_func =
    "/*$PROTECTED_FUNCTIONS$*/\n  ";

  QString buffer;
  loadBuffer(buffer,::locate("data", "kdevtrollproject/subclassing/subclass_template.h"));
  replaceKeywords(buffer,m_canBeModal);
  for (i=0; i<m_slots.count(); i++)
  {
    SlotItem *slitem = m_slots[i];
    if (!slitem->isOn())
      continue;
    QString declBuild;
    if (slitem->m_access=="public")
      if (!slitem->m_isFunc)
        declBuild = public_slot;
      else
        declBuild = public_func;
    if (slitem->m_access=="protected")
      if (!slitem->m_isFunc)
        declBuild = protected_slot;
      else
        declBuild = protected_func;
    if (!(slitem->m_specifier=="non virtual"))
      declBuild += "virtual ";
    declBuild += slitem->m_returnType + " ";
    QString spacer;
    if (slitem->m_access=="public")
    {
      if (!slitem->m_isFunc)
      {
        declBuild += spacer.fill(' ',43-declBuild.length()) + slitem->m_methodName + ";";
        replace(buffer,"/*$PUBLIC_SLOTS$*/",declBuild);
      }
      else
      {
        declBuild += spacer.fill(' ',47-declBuild.length()) + slitem->m_methodName + ";";
        replace(buffer,"/*$PUBLIC_FUNCTIONS$*/",declBuild);
      }
    }
    if (slitem->m_access=="protected")
    {
      if (!slitem->m_isFunc)
      {
        declBuild += spacer.fill(' ',46-declBuild.length()) + slitem->m_methodName + ";";
        replace(buffer,"/*$PROTECTED_SLOTS$*/",declBuild);
      }
      else
      {
        declBuild += spacer.fill(' ',50-declBuild.length()) + slitem->m_methodName + ";";
        replace(buffer,"/*$PROTECTED_FUNCTIONS$*/",declBuild);
      }
    }
  }

  saveBuffer(buffer,m_formPath + "/" + m_edFileName->text()+".h");

  // cpp - file

  QString implementation =
    "/*$SPECIALIZATION$*/\n"
    "$RETURNTYPE$ $NEWCLASS$::$METHOD$\n"
    "{\n"
    "}\n";

  QString implementation_callbase =
    "/*$SPECIALIZATION$*/\n"
    "$RETURNTYPE$ $NEWCLASS$::$METHOD$\n"
    "{\n"
    "  $QTBASECLASS$::$METHOD$;\n"
    "}\n";


  loadBuffer(buffer,::locate("data", "kdevtrollproject/subclassing/subclass_template.cpp"));
  replaceKeywords(buffer,m_canBeModal);
  for (i=0; i<m_slots.count(); i++)
  {
    SlotItem *slitem = m_slots[i];
    if (!slitem->isOn())
      continue;
    QString impl = slitem->m_callBaseClass ? implementation_callbase : implementation;
    replace(impl,"$RETURNTYPE$",slitem->m_returnType);
    replace(impl,"$NEWCLASS$",m_edClassName->text());
    replace(impl,"$METHOD$", slitem->m_methodName);
    replace(impl,"$QTBASECLASS$", m_qtBaseClassName);
    replace(buffer,"/*$SPECIALIZATION$*/",impl);
  }
  saveBuffer(buffer,m_formPath + "/" + m_edFileName->text()+".cpp");

  if (m_creatingNewSubclass)
  {
    m_newFileNames.append(m_formPath + "/" + m_edFileName->text()+".cpp");
    m_newFileNames.append(m_formPath + "/" + m_edFileName->text()+".h");
  }
  SubclassingDlgBase::accept();
}

void SubclassingDlg::onChangedClassName()
//=======================================
{
  m_edFileName->setText(m_edClassName->text().lower());
  if (m_edFileName->text().isEmpty() ||
      m_edClassName->text().isEmpty())
    m_btnOk->setEnabled(false);
  else
    m_btnOk->setEnabled(true);
}
