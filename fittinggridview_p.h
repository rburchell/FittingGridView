/* Copyright (c) 2013 John Brooks <john.brooks@dereferenced.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef FITTINGGRIDVIEW_P_H
#define FITTINGGRIDVIEW_P_H

#include "fittinggridview.h"
#include <QtQml/private/qqmldelegatemodel_p.h>
#include <QtQml/private/qqmlguard_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

namespace {
    class LayoutRow;
}

class FittingGridViewPrivate : public QObject, public QQuickItemChangeListener
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(FittingGridView)

public:
    FittingGridView *q_ptr;

    explicit FittingGridViewPrivate(FittingGridView *q);
    virtual ~FittingGridViewPrivate();

    QVariant modelVariant;
    QQmlGuard<QQmlInstanceModel> model;
    bool ownModel;
    QQmlGuard<QQuickItem> flickable;
    QQuickItem *contentItem;

    int spacing;
    double explicitLayoutWidth;
    double maximumHeight;
    double displayWidth;

    int currentIndex;
    QQuickItem *currentItem;

    QQmlGuard<QQmlComponent> highlight;
    QQuickItem *highlightItem;

    QQmlChangeSet pendingChanges;
    QList<LayoutRow*> rows;

    QMap<int,double> cachedItemAspect;
    QMap<int,QQuickItem*> delegates;

    // Flag set by layout when no expensive operations (e.g. creating delegates) should be done
    bool cachedLayoutOnly;

    double layoutWidth() const;
    void layoutChanged();
    void displayChanged();
    void applyPendingChanges();
    void layout();
    void layoutItems(double minY, double maxY);
    void updateContentSize();

    void createHighlight();
    void updateCurrent(int index);

    void clear();

    int rowOf(int index);
    QQuickItem *createItem(int index, bool asynchronous = false);
    double indexAspectRatio(int index);
    void updateItemSize(int index);
    void applyPositions(LayoutRow *row, double y);

    int maximumLoadingRowItems() const;

    virtual void itemImplicitWidthChanged(QQuickItem *item);
    virtual void itemImplicitHeightChanged(QQuickItem *item);

public slots:
    void createdItem(int index, QObject *object);
    void initItem(int index, QObject *object);
    void destroyingItem(QObject *object);
    void modelUpdated(const QQmlChangeSet &changes, bool reset);
};

#endif // FITTINGGRIDVIEW_P_H
