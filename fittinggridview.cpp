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

#include "fittinggridview.h"
#include "fittinggridview_p.h"
#include <QtQml/private/qqmldelegatemodel_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QQmlContext>
#include <QDebug>
#include <functional>

#define LAYOUT_DEBUG

#ifdef LAYOUT_DEBUG
#define DEBUG() qDebug()
#else
#define DEBUG() if (0) qDebug()
#endif

/* Avoid layout logic during display-only updates
 * Items with 0 size can permanently stop layouts
 * Asynchronous delegate creation?
 * Content height / >maxY row updates?
 */

FittingGridView::FittingGridView(QQuickItem *parent)
    : QQuickItem(parent)
    , d_ptr(new FittingGridViewPrivate(this))
{
}

FittingGridView::~FittingGridView()
{
}

QVariant FittingGridView::model() const
{
    Q_D(const FittingGridView);
    return d->modelVariant;
}

void FittingGridView::setModel(const QVariant &model)
{
    Q_D(FittingGridView);
    if (d->modelVariant == model)
        return;

    if (d->model) {
        disconnect(d->model, SIGNAL(modelUpdated(QQmlChangeSet,bool)),
                d, SLOT(modelUpdated(QQmlChangeSet,bool)));
        disconnect(d->model, SIGNAL(initItem(int,QObject*)), d, SLOT(initItem(int,QObject*)));
        disconnect(d->model, SIGNAL(createdItem(int,QObject*)), d, SLOT(createdItem(int,QObject*)));
        disconnect(d->model, SIGNAL(destroyingItem(QObject*)), d, SLOT(destroyingItem(QObject*)));
    }

    QQmlInstanceModel *oldModel = d->model;

    d->clear();
    d->model = 0;
    d->modelVariant = model;

    QObject *object = qvariant_cast<QObject*>(model);
    QQmlInstanceModel *vim = 0;
    if (object && (vim = qobject_cast<QQmlInstanceModel *>(object))) {
        if (d->ownModel) {
            delete oldModel;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QQmlDelegateModel(qmlContext(this), this);
            d->ownModel = true;
            if (isComponentComplete())
                static_cast<QQmlDelegateModel *>(d->model.data())->componentComplete();
        } else {
            d->model = oldModel;
        }
        if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(d->model)) {
            dataModel->setModel(model);
        }
    }

    if (d->model) {
        connect(d->model, SIGNAL(createdItem(int,QObject*)), d, SLOT(createdItem(int,QObject*)));
        connect(d->model, SIGNAL(initItem(int,QObject*)), d, SLOT(initItem(int,QObject*)));
        connect(d->model, SIGNAL(destroyingItem(QObject*)), d, SLOT(destroyingItem(QObject*)));
        if (isComponentComplete()) {
            polish();
        }

        connect(d->model, SIGNAL(modelUpdated(QQmlChangeSet,bool)),
                d, SLOT(modelUpdated(QQmlChangeSet,bool)));
    }
    emit modelChanged();
}

QQmlComponent *FittingGridView::delegate() const
{
    Q_D(const FittingGridView);
    if (d->model) {
        if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(d->model))
            return dataModel->delegate();
    }

    return 0;
}

void FittingGridView::setDelegate(QQmlComponent *delegate)
{
    Q_D(FittingGridView);
    if (delegate == this->delegate())
        return;
    if (!d->ownModel) {
        d->model = new QQmlDelegateModel(qmlContext(this), this);
        d->ownModel = true;
    }
    if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(d->model)) {
        dataModel->setDelegate(delegate);
        d->clear();
        polish();
    }
    emit delegateChanged();
}

QQuickItem *FittingGridView::flickable() const
{
    Q_D(const FittingGridView);
    return d->flickable;
}

void FittingGridView::setFlickable(QQuickItem *flickable)
{
    Q_D(FittingGridView);
    if (flickable == d->flickable)
        return;

    d->flickable = flickable;
    d->contentItem = flickable->property("contentItem").value<QQuickItem*>();
    flickable->setParentItem(this);

    connect(flickable, SIGNAL(contentYChanged()), SLOT(polish()));

    emit flickableChanged();
}

int FittingGridView::spacing() const
{
    Q_D(const FittingGridView);
    return d->spacing;
}

void FittingGridView::setSpacing(int spacing)
{
    Q_D(FittingGridView);
    if (d->spacing == spacing)
        return;

    d->spacing = spacing;
    d->layoutChanged();
    emit spacingChanged();
}

double FittingGridView::layoutWidth() const
{
    Q_D(const FittingGridView);
    return d->layoutWidth();
}

void FittingGridView::setLayoutWidth(double layoutWidth)
{
    Q_D(FittingGridView);
    if (d->explicitLayoutWidth == layoutWidth)
        return;

    d->explicitLayoutWidth = layoutWidth;
    d->layoutChanged();
    emit layoutWidthChanged();
}

double FittingGridView::maximumHeight() const
{
    Q_D(const FittingGridView);
    return d->maximumHeight;
}

void FittingGridView::setMaximumHeight(double maximumHeight)
{
    Q_D(FittingGridView);
    if (d->maximumHeight == maximumHeight)
        return;

    d->maximumHeight = maximumHeight;
    d->layoutChanged();
    emit maximumHeightChanged();
}

void FittingGridView::classBegin()
{
    QQuickItem::classBegin();
}

void FittingGridView::componentComplete()
{
    Q_D(FittingGridView);
    QQuickItem::componentComplete();

    if (d->model && d->ownModel)
        static_cast<QQmlDelegateModel *>(d->model.data())->componentComplete();

    if (!d->flickable) {
        QQmlComponent component(qmlContext(this)->engine());
        component.setData("import QtQuick 2.0\nFlickable { anchors.fill: parent }", QUrl());
        QQuickItem *f = static_cast<QQuickItem*>(component.create(qmlContext(this)));
        f->setParentItem(this);
        setFlickable(f);
    }

    d->layoutChanged();
    d->displayChanged();
}

void FittingGridView::updatePolish()
{
    Q_D(FittingGridView);
    d->layout();
}

void FittingGridView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(FittingGridView);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    if (newGeometry.width() != oldGeometry.width()) {
        d->displayChanged();
        if (!d->explicitLayoutWidth) {
            d->layoutChanged();
            emit layoutWidthChanged();
        }
    }
}

namespace {

class LayoutRow
{
public:
    FittingGridViewPrivate *view;
    int first;
    int last;

    double displayY;

    LayoutRow(FittingGridViewPrivate *v);

    bool isEmpty() const { return first < 0 || last < 0; }
    int count() const { return isEmpty() ? 0 : (last - first + 1); }
    bool isPresentable() { return !isEmpty() && !itemsLoading(); }

    double aspect();
    double layoutHeight();
    double displayHeight();
    int itemsLoading();

    double calculateHeight(int count, double width, double aspect);

    // Update the row by setting the first index if applicable, and adding or
    // removing items from the end to meet layout requirements.
    bool updateRow(int first, int maxLast);

    void dataChanged();
    void layoutChanged();
    void displayChanged();

private:
    double m_aspect;
    double m_layoutHeight;
    double m_displayHeight;
    int m_itemsLoading;
};

LayoutRow::LayoutRow(FittingGridViewPrivate *v)
    : view(v)
    , first(-1)
    , last(-1)
    , displayY(-1)
    , m_aspect(0)
    , m_layoutHeight(0)
    , m_displayHeight(0)
    , m_itemsLoading(-1)
{
}

double LayoutRow::calculateHeight(int count, double width, double aspect)
{
    if (!aspect || !count)
        return 0;
    return qRound((width - ((count - 1) * view->spacing)) / aspect);
}

bool LayoutRow::updateRow(int newFirst, int maxLast)
{
    bool added = false;
    bool removed = false;

    Q_ASSERT(newFirst >= 0);
    Q_ASSERT(maxLast >= newFirst);

    if (first != newFirst) {
        first = last = newFirst;
        dataChanged();

        added = true;
    } else if (last > maxLast) {
        last = maxLast;
        dataChanged();
    } else if (m_layoutHeight) {
        // Layout is cached, no changes to data are possible or necessary
        return false;
    } else if (itemsLoading()) {
        // Truncate at the first not-loaded item and refill; this is an easy way to
        // ensure that we fill up with the right number of sequential loaded items.
        // The normal remove-from-end logic wouldn't work, because it'd take all loading
        // items at the end off.
        for (int i = first; i < last; i++) {
            if (!view->indexAspectRatio(i)) {
                last = i;
                dataChanged();
                break;
            }
        }
    }

    while (last < maxLast
           && (!aspect() || layoutHeight() > view->maximumHeight)
           && (!itemsLoading() || count() < view->maximumLoadingRowItems()))
    {
        // Add an item to the end
        double newAspect = aspect() + view->indexAspectRatio(last + 1);
        last++;
        dataChanged();
        m_aspect = newAspect;

        added = true;
    }

    while (!added && last > first) {
        // Calculate the layout height without the last item
        double newAspect = aspect() - view->indexAspectRatio(last);
        double newLayoutHeight = calculateHeight(count() - 1, view->layoutWidth(), newAspect);
        if (newLayoutHeight > view->maximumHeight) {
            // Do not remove any more items
            break;
        }

        last--;
        dataChanged();
        m_aspect = newAspect;
        m_layoutHeight = newLayoutHeight;

        removed = true;
    }

    return added || removed;
}

double LayoutRow::aspect()
{
    if (!m_aspect) {
        for (int i = first; i <= last; i++)
            m_aspect += view->indexAspectRatio(i);
    }
    return m_aspect;
}

double LayoutRow::layoutHeight()
{
    if (!m_layoutHeight)
        m_layoutHeight = calculateHeight(count(), view->layoutWidth(), aspect());
    return m_layoutHeight;
}

double LayoutRow::displayHeight()
{
    if (!m_displayHeight)
        m_displayHeight = calculateHeight(count(), view->displayWidth, aspect());
    return m_displayHeight;
}

int LayoutRow::itemsLoading()
{
    if (m_itemsLoading < 0) {
        m_itemsLoading = 0;
        for (int i = first; i <= last; i++) {
            if (!view->indexAspectRatio(i))
                m_itemsLoading++;
        }
    }
    return m_itemsLoading;
}

void LayoutRow::dataChanged()
{
    m_aspect = 0;
    m_itemsLoading = -1;
    layoutChanged();
}

void LayoutRow::layoutChanged()
{
    m_layoutHeight = 0;
    displayChanged();
}

void LayoutRow::displayChanged()
{
    m_displayHeight = 0;
}

}

FittingGridViewPrivate::FittingGridViewPrivate(FittingGridView *q)
    : QObject(q)
    , q_ptr(q)
    , ownModel(false)
    , contentItem(0)
    , spacing(2)
    , explicitLayoutWidth(0)
    , maximumHeight(300)
    , displayWidth(0)
{
}

FittingGridViewPrivate::~FittingGridViewPrivate()
{
    qDeleteAll(rows);

    foreach (QQuickItem *item, delegates) {
        model->release(item);
    }
}

double FittingGridViewPrivate::layoutWidth() const
{
    if (explicitLayoutWidth)
        return explicitLayoutWidth;
    return displayWidth;
}

void FittingGridViewPrivate::layoutChanged()
{
    Q_Q(FittingGridView);

    foreach (LayoutRow *row, rows) {
        row->layoutChanged();
        row->displayY = -1;
    }
    q->polish();
}

void FittingGridViewPrivate::displayChanged()
{
    Q_Q(FittingGridView);
    displayWidth = contentItem ? contentItem->width() : 0;
    foreach (LayoutRow *row, rows) {
        row->displayChanged();
        row->displayY = -1;
    }
    q->polish();
}

QQuickItem *FittingGridViewPrivate::createItem(int index, bool asynchronous)
{
    if (!contentItem)
        return 0;

    QQuickItem *item = delegates.value(index);
    if (item)
        return item;

    QObject *object = model->object(index, asynchronous);
    item = qmlobject_cast<QQuickItem*>(object);
    if (!item) {
        if (object)
            model->release(object);
        return 0;
    }

    item->setParentItem(contentItem);
    DEBUG() << "create delegate:" << index << item << item->implicitWidth() << item->implicitHeight();
    return item;
}

void FittingGridViewPrivate::createdItem(int index, QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem*>(object);
    if (!item)
        return;

    item->setVisible(false);
    delegates.insert(index, item);

    QQuickItemPrivate::get(item)->addItemChangeListener(this,
                                                        QQuickItemPrivate::ImplicitWidth |
                                                        QQuickItemPrivate::ImplicitHeight);
}

void FittingGridViewPrivate::initItem(int index, QObject *object)
{
    Q_UNUSED(index);
    Q_UNUSED(object);
}

void FittingGridViewPrivate::destroyingItem(QObject *object)
{
    Q_UNUSED(object);
}

void FittingGridViewPrivate::modelUpdated(const QQmlChangeSet &changes, bool reset)
{
    Q_Q(FittingGridView);

    if (reset)
        clear();
    pendingChanges.apply(changes);
    if (!pendingChanges.isEmpty())
        q->polish();
}

template<typename T> T updateIndexMap(const T &map, int index, int delta,
                                      std::function<void(typename T::const_iterator)> removeFunc = nullptr)
{
    T updated;
    for (auto it = map.begin(); it != map.end(); it++) {
        if (it.key() < index) {
            updated.insert(it.key(), it.value());
        } else if (delta < 0 && it.key() >= index && it.key() < (index - delta)) {
            // Removed
            if (removeFunc)
                removeFunc(it);
        } else
            updated.insert(it.key() + delta, it.value());
    }
    return updated;
}

void FittingGridViewPrivate::layout()
{
    double contentY = flickable->property("contentY").toDouble();
    double viewportHeight = flickable->height();

    if (layoutWidth() < 1 || displayWidth < 1 || viewportHeight < 1)
        return;

    if (!pendingChanges.isEmpty())
        DEBUG() << "layout: model changes:" << pendingChanges;

    // Process changes in the data and update existing rows. It's okay if this process
    // leaves gaps; they will be closed while recalculating row layouts
    foreach (const QQmlChangeSet::Remove &remove, pendingChanges.removes()) {
        // Delete all rows after the removed index; recalculation is relatively cheap
        // and would almost always happen anyway.
        while (!rows.isEmpty()) {
            if (rows.last()->last < remove.index)
                break;
            delete rows.takeLast();
        }

        cachedItemAspect = updateIndexMap(cachedItemAspect, remove.index, -remove.count);
        delegates = updateIndexMap(delegates, remove.index, -remove.count,
            [this](decltype(delegates.constBegin()) it) {
                model->release(it.value());
            }
        );
    }

    foreach (const QQmlChangeSet::Insert &insert, pendingChanges.inserts()) {
        foreach (LayoutRow *row, rows) {
            // Row intersects with the insertion; all other rows are technically unchanged
            if (row->first < insert.end() && row->last >= insert.index)
                row->dataChanged();

            // Layout will take care of fixing the row
            if (row->first >= insert.index)
                row->first += insert.count;
            if (row->last >= insert.index)
                row->last += insert.count;
        }

        cachedItemAspect = updateIndexMap(cachedItemAspect, insert.index, insert.count);
        delegates = updateIndexMap(delegates, insert.index, insert.count);
    }

    pendingChanges.clear();

    layoutItems(contentY, contentY + viewportHeight);
    updateContentSize();
}

void FittingGridViewPrivate::layoutItems(double minY, double maxY)
{
    double y = 0;
    int firstRow = -1, lastRow = -1;

    DEBUG() << "layout: position" << minY << "to" << maxY << "total" << model->count()
            << "layoutWidth" << layoutWidth() << "displayWidth" << displayWidth;

    // Find existing rows within the range, and ensure positions of all existing rows up to there
    for (int ri = 0; lastRow < 0; ri++) {
        int rowFirst = ri ? (rows[ri-1]->last + 1) : 0;
        if (rowFirst >= model->count()) {
            lastRow = ri - 1;
            while (rows.size() - 1 > lastRow)
                delete rows.takeLast();
            break;
        }

        if (ri == rows.size()) {
            LayoutRow *row = new LayoutRow(this);
            rows.append(row);
        }

        LayoutRow *row = rows[ri];
        row->updateRow(rowFirst, model->count() - 1);
        row->displayY = y;

        if (!row->isPresentable()) {
            DEBUG() << "layout: row" << ri << "for" << row->first << "to" << row->last << "still loading"
                    << row->itemsLoading();
            // An unpresentable row must be between first and last, or it won't ever become presentable
            if (firstRow < 0)
                firstRow = ri;
            lastRow = ri;
            break;
        }

        if (firstRow < 0 && (y + row->displayHeight()) >= minY)
            firstRow = ri;

        DEBUG() << "layout: row" << ri << "for" << row->first << "to" << row->last << "y" << y
                << "height" << row->displayHeight();

        y += row->displayHeight() + spacing;

        if (y > maxY) {
            lastRow = ri;
            // XXX This means all rows below lastRow have completely inconsistent data
            break;
        }
    }

    if (firstRow >= 0 && lastRow >= 0) {
        for (int i = firstRow; i >= 0 && i <= lastRow; i++) {
            applyPositions(rows[i], rows[i]->displayY);
        }

        int firstIndex = rows[firstRow]->first;
        int lastIndex = rows[lastRow]->last;
        for (auto it = delegates.begin(); it != delegates.end(); ) {
            if (it.key() < firstIndex || it.key() > lastIndex) {
                model->release(it.value());
                it = delegates.erase(it);
            } else
                it++;
        }
    } else {
        for (auto it = delegates.begin(); it != delegates.end(); it++)
            model->release(it.value());
        delegates.clear();
    }
}

void FittingGridViewPrivate::updateContentSize()
{
    double avg;
    if (!rows.isEmpty()) {
        if (rows.last()->last == model->count() - 1 && rows.last()->displayY >= 0) {
            flickable->setProperty("contentHeight", rows.last()->displayY + rows.last()->displayHeight());
            return;
        }

        avg = (rows.last()->last + 1) / rows.size();
    } else {
        avg = 1;
    }

    flickable->setProperty("contentHeight", (model->count() / avg) * maximumHeight);
}

int FittingGridViewPrivate::maximumLoadingRowItems() const
{
    return (layoutWidth() && maximumHeight) ? int(ceil(layoutWidth() / ((3.0/4.0) * maximumHeight))) : 6;
}

double FittingGridViewPrivate::indexAspectRatio(int index)
{
    QMap<int,double>::iterator it = cachedItemAspect.find(index);
    if (it != cachedItemAspect.end())
        return it.value();

    QQuickItem *item = createItem(index);
    if (item) {
        double w = item->implicitWidth();
        double h = item->implicitHeight();
        double v = (w && h) ? (item->implicitWidth() / item->implicitHeight()) : 0;
        cachedItemAspect.insert(index, v);
        return v;
    } else
        return 0;
}

void FittingGridViewPrivate::updateItemSize(int index)
{
    Q_Q(FittingGridView);

    cachedItemAspect.remove(index);
    foreach (LayoutRow *row, rows) {
        if (row->first <= index && row->last >= index) {
            row->dataChanged();
            break;
        } else if (row->first > index) {
            break;
        }
    }

    q->polish();
}

void FittingGridViewPrivate::applyPositions(LayoutRow *row, double y)
{
    if (!row->isPresentable()) {
        // Don't show anything in an unpresentable row
        for (int index = row->first; index <= row->last; index++) {
            QQuickItem *item = createItem(index);
            if (!item)
                continue;

            item->setVisible(false);
        }
        return;
    }

    double x = 0;
    double availableWidth = displayWidth - ((row->count() - 1) * spacing);
    double rAspect = row->aspect();

    for (int index = row->first; index <= row->last; index++) {
        QQuickItem *item = createItem(index);
        if (!item)
            continue;

        double aspect = indexAspectRatio(index);

        item->setPosition(QPointF(x, y));
        item->setSize(QSizeF(qRound(availableWidth / (rAspect / aspect)), row->displayHeight()));
        item->setVisible(true);

        availableWidth -= item->width();
        rAspect -= aspect;
        x += item->width() + spacing;
    }
}

void FittingGridViewPrivate::clear()
{
    qDeleteAll(rows);
    rows.clear();
    pendingChanges.clear();
    cachedItemAspect.clear();
    foreach (QQuickItem *item, delegates)
        model->release(item);
    delegates.clear();
}

void FittingGridViewPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    int index = model->indexOf(item, 0);
    if (index >= 0)
        updateItemSize(index);
}

void FittingGridViewPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    int index = model->indexOf(item, 0);
    if (index >= 0)
        updateItemSize(index);
}
