/***************************************************************************
 *   Copyright 2006-2009 Alexander Dymo  <adymo@kdevelop.org>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
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
#include "container.h"

#include <QMap>
#include <QBoxLayout>
#include <QLabel>
#include <QStylePainter>
#include <QStackedWidget>
#include <QStyleOptionTabBarBase>

#include <kdebug.h>
#include <klocale.h>
#include <ktabbar.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <kglobal.h>
#include <kdeversion.h>
#include <kacceleratormanager.h>

#include "view.h"
#include "document.h"
#include <QStackedWidget>
#include <qstyle.h>

namespace Sublime {

// struct ContainerPrivate

struct ContainerPrivate {
    QMap<QWidget*, View*> viewForWidget;

    KTabBar *tabBar;
    QStackedWidget *stack;
    QLabel *fileNameCorner;
    QLabel *statusCorner;
};

class UnderlinedLabel: public QLabel {
public:
    UnderlinedLabel(KTabBar *tabBar, QWidget* parent = 0, Qt::WindowFlags f = 0)
        :QLabel(parent, f), m_tabBar(tabBar)
    {
    }

protected:
    virtual void paintEvent(QPaintEvent *ev)
    {
        QLabel::paintEvent(ev);

        if (m_tabBar->isVisible() && m_tabBar->count() > 0)
        {
            QStylePainter p(this);
            QStyleOptionTabBarBase optTabBase;
            optTabBase.init(m_tabBar);
            optTabBase.shape = m_tabBar->shape();

            QStyleOptionTab tabOverlap;
            tabOverlap.shape = m_tabBar->shape();
            int overlap = style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, &tabOverlap, m_tabBar);
            QRect rect;
            rect.setRect(0, height()-overlap, width(), overlap);
            optTabBase.rect = rect;
            p.drawPrimitive(QStyle::PE_FrameTabBarBase, optTabBase);
        }
    }

    KTabBar *m_tabBar;
};


class StatusLabel: public UnderlinedLabel {
public:
    StatusLabel(KTabBar *tabBar, QWidget* parent = 0, Qt::WindowFlags f = 0):
        UnderlinedLabel(tabBar, parent, f)
    {
        setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }

    virtual QSize minimumSizeHint() const
    {
        QRect rect = style()->itemTextRect(fontMetrics(), QRect(), Qt::AlignRight, true, i18n("Line: 00000 Col: 000"));
        QStyleOptionTab tabOverlap;
        tabOverlap.shape = m_tabBar->shape();
        int overlap = style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, &tabOverlap, m_tabBar);
        rect.setHeight(rect.height()+overlap);
        return rect.size();
    }
};


// class Container

Container::Container(QWidget *parent)
    :QWidget(parent), d(new ContainerPrivate())
{
    KAcceleratorManager::setNoAccel(this);

    QBoxLayout *l = new QBoxLayout(QBoxLayout::TopToBottom, this);
    l->setMargin(0);
    l->setSpacing(0);

    QBoxLayout *h = new QBoxLayout(QBoxLayout::LeftToRight);
    h->setMargin(0);
    h->setSpacing(0);

    d->tabBar = new KTabBar(this);
    h->addWidget(d->tabBar);
    d->fileNameCorner = new UnderlinedLabel(d->tabBar, this);
    h->addWidget(d->fileNameCorner);
    d->statusCorner = new StatusLabel(d->tabBar, this);
    h->addWidget(d->statusCorner);
    l->addLayout(h);

    d->stack = new QStackedWidget(this);
    l->addWidget(d->stack);

    connect(d->tabBar, SIGNAL(currentChanged(int)), this, SLOT(widgetActivated(int)));
    connect(d->tabBar, SIGNAL(closeRequest(int)), this, SLOT(closeRequest(int)));
    connect(d->tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(tabMoved(int, int)));

    KConfigGroup group = KGlobal::config()->group("UiSettings");
    setTabBarHidden(group.readEntry("TabBarVisibility", 1) == 0);
#if KDE_VERSION < KDE_MAKE_VERSION(4,2,60)
    d->tabBar->setHoverCloseButton(true);
    d->tabBar->setCloseButtonEnabled(true);
    d->tabBar->setTabReorderingEnabled(true);
#else
    d->tabBar->setTabsClosable(true);
    d->tabBar->setMovable(true);
#endif
}

Container::~Container()
{
    delete d;
}

void Container::closeRequest(int idx)
{
    emit closeRequest((widget(idx)));
}

void Container::widgetActivated(int idx)
{
    if (idx < 0)
        return;
    if (QWidget* w = d->stack->widget(idx)) {
        w->setFocus();
        if(d->viewForWidget.contains(w))
            emit activateView(d->viewForWidget[w]);
    }
}

void Container::addWidget(View *view)
{
    QWidget *w = view->widget(this);
    int idx = d->stack->addWidget(w);
    d->tabBar->insertTab(idx, view->document()->title());
    d->viewForWidget[w] = view;
    connect(view, SIGNAL(statusChanged(Sublime::View*)), this, SLOT(statusChanged(Sublime::View*)));
    connect(view->document(), SIGNAL(titleChanged(Sublime::Document*)), this, SLOT(documentTitleChanged(Sublime::Document*)));
}

void Container::statusChanged(Sublime::View* view)
{
    d->statusCorner->setText(view->viewStatus());
}

void Container::documentTitleChanged(Sublime::Document* doc)
{
    QMapIterator<QWidget*, View*> it = d->viewForWidget;
    while (it.hasNext()) {
        if (it.next().value()->document() == doc) {
            int tabIndex = d->stack->indexOf(it.key());
            if (tabIndex != -1) {
                d->tabBar->setTabText(tabIndex, doc->title());
            }
            break;
        }
    }
}

int Container::count() const
{
    return d->stack->count();
}

QWidget* Container::currentWidget() const
{
    return d->stack->currentWidget();
}

void Container::setCurrentWidget(QWidget* w)
{
    d->stack->setCurrentWidget(w);
    d->tabBar->setCurrentIndex(d->stack->indexOf(w));
    if (View *view = d->viewForWidget[w])
    {
        statusChanged(view);
        d->fileNameCorner->setText(view->document()->title());
    }
}

QWidget* Container::widget(int i) const
{
    return d->stack->widget(i);
}

int Container::indexOf(QWidget* w) const
{
    return d->stack->indexOf(w);
}

void Sublime::Container::removeWidget(QWidget *w)
{
    if (w) {
        d->tabBar->removeTab(d->stack->indexOf(w));
        d->stack->removeWidget(w);
        if (d->tabBar->currentIndex() != -1)
            d->fileNameCorner->setText(d->tabBar->tabText(d->tabBar->currentIndex()));
        View* view = d->viewForWidget.take(w);
        if (view)
        {
            disconnect(view->document(), SIGNAL(titleChanged(Sublime::Document*)), this, SLOT(documentTitleChanged(Sublime::Document*)));
            disconnect(view, SIGNAL(statusChanged(Sublime::View*)), this, SLOT(statusChanged(Sublime::View*)));
        }
    }
}

bool Container::hasWidget(QWidget *w)
{
    return d->stack->indexOf(w) != -1;
}

View *Container::viewForWidget(QWidget *w) const
{
    return d->viewForWidget[w];
}

void Container::setTabIcon(int index, const QIcon& icon) const
{
    d->tabBar->setTabIcon(index, icon);
}

void Container::setTabBarHidden(bool hide)
{
    if (hide)
    {
        d->tabBar->hide();
        d->fileNameCorner->show();
    }
    else
    {
        d->fileNameCorner->hide();
        d->tabBar->show();
    }
}

void Container::tabMoved(int from, int to)
{
    QWidget *w = d->stack->widget(from);
    d->stack->removeWidget(w);
    d->stack->insertWidget(to, w);
}

}

#include "container.moc"

