/*
   Copyright 2009 David Nolden <david.nolden.kdevelop@art-master.de>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "iassistant.h"

using namespace KDevelop;

KDevelop::IAssistant::~IAssistant() {

}

KDevelop::IAssistantAction::~IAssistantAction() {

}

QIcon KDevelop::IAssistantAction::icon() const {
    return QIcon();
}

QString KDevelop::IAssistantAction::toolTip() const {
    return QString();
}

unsigned int KDevelop::IAssistantAction::flags() const {
    return NoFlag;
}

QIcon KDevelop::IAssistant::icon() {
    return QIcon();
}

QString KDevelop::IAssistant::title() {
    return QString();
}

QList< KSharedPtr< KDevelop::IAssistantAction > > KDevelop::StandardAssistant::actions() {
    return m_actions;
}

KDevelop::StandardAssistant::StandardAssistant(const QList< KSharedPtr< KDevelop::IAssistantAction > >& actions) : m_actions(actions) {
}

#include "iassistant.moc"
