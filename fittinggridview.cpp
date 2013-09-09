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

    if (d->flickable) {
        disconnect(d->flickable, 0, this, 0);
    }

    d->clear();
    d->flickable = flickable;
    d->contentItem = flickable->property("contentItem").value<QQuickItem*>();

    if (d->highlightItem)
        d->highlightItem->setParentItem(d->contentItem);

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

int FittingGridView::currentIndex() const
{
    Q_D(const FittingGridView);
    return d->currentIndex;
}

void FittingGridView::setCurrentIndex(int index)
{
    Q_D(FittingGridView);
    d->applyPendingChanges();
    if (d->currentIndex == index)
        return;

    if (isComponentComplete()) {
        d->updateCurrent(index);
    } else {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}

QQuickItem *FittingGridView::currentItem() const
{
    Q_D(const FittingGridView);
    return d->currentItem;
}

bool FittingGridView::incrementCurrentIndex()
{
    Q_D(FittingGridView);
    if (currentIndex() < d->model->count() - 1) {
        setCurrentIndex(currentIndex() + 1);
        return true;
    }
    return false;
}

bool FittingGridView::decrementCurrentIndex()
{
    if (currentIndex() > 0) {
        setCurrentIndex(currentIndex() - 1);
        return true;
    }
    return false;
}

QQmlComponent *FittingGridView::highlight() const
{
    Q_D(const FittingGridView);
    return d->highlight;
}

QQuickItem *FittingGridView::highlightItem() const
{
    Q_D(const FittingGridView);
    return d->highlightItem;
}

void FittingGridView::setHighlight(QQmlComponent *component)
{
    Q_D(FittingGridView);
    if (d->highlight == component)
        return;

    d->highlight = component;
    if (isComponentComplete())
        d->createHighlight();
    polish();

    emit highlightChanged();
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
    d->applyPendingChanges();
    d->updateCurrent(d->currentIndex);
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
    if (!m_displayHeight) {
        if (isPresentable())
            m_displayHeight = calculateHeight(count(), view->displayWidth, aspect());
        else
            m_displayHeight = view->maximumHeight;
    }
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

bool FittingGridView::incrementCurrentRow()
{
    Q_D(FittingGridView);
    int rowIndex = d->rowOf(currentIndex());
    if (rowIndex >= 0 && rowIndex < d->rows.size() - 1)
        setCurrentIndex(d->rows[rowIndex + 1]->first);
    else
        return incrementCurrentIndex();
    return true;
}

bool FittingGridView::decrementCurrentRow()
{
    Q_D(FittingGridView);
    int rowIndex = d->rowOf(currentIndex());
    if (rowIndex < 0)
        return decrementCurrentIndex();
    else if (rowIndex)
        setCurrentIndex(d->rows[rowIndex - 1]->first);
    else
        return false;
    return true;
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
    , currentIndex(-1)
    , currentItem(0)
    , highlightItem(0)
    , cachedLayoutOnly(false)
{
}

FittingGridViewPrivate::~FittingGridViewPrivate()
{
    clear();
    if (ownModel)
        delete model;
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

int FittingGridViewPrivate::rowOf(int index)
{
    for (int i = 0; i < rows.size(); i++) {
        if (rows[i]->first <= index && rows[i]->last >= index)
            return i;
        else if (rows[i]->first > index)
            break;
    }
    return -1;
}

QQuickItem *FittingGridViewPrivate::createItem(int index, bool asynchronous)
{
    if (!contentItem)
        return 0;

    QQuickItem *item = delegates.value(index);
    if (item)
        return item;

    if (cachedLayoutOnly)
        return 0;

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

    QQuickItem *item = qobject_cast<QQuickItem*>(object);
    if (!item)
        return;

    item->setParentItem(contentItem);
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

    applyPendingChanges();
    layoutItems(contentY, contentY + viewportHeight);
    updateContentSize();

    if (highlight && !highlightItem)
        createHighlight();

    if (highlightItem) {
        highlightItem->setVisible(currentItem != 0);
        DEBUG() << "layout: highlight" << highlightItem << currentItem << (currentItem ? currentItem->position() : QPointF());
        if (currentItem) {
            highlightItem->setPosition(currentItem->position());
            highlightItem->setSize(QSizeF(currentItem->width(), currentItem->height()));
        }
    }
}

void FittingGridViewPrivate::layoutItems(double minY, double maxY)
{
    double y = 0;
    int firstRow = -1, lastRow = -1, currentRow = -1;

    DEBUG() << "layout: position" << minY << "to" << maxY << "total" << model->count()
            << "layoutWidth" << layoutWidth() << "displayWidth" << displayWidth;

    // Find existing rows within the range, and ensure positions of all existing rows up to there
    // XXX This means all rows below lastRow have completely inconsistent data
    for (int ri = 0; lastRow < 0 || (currentRow < 0 && currentIndex >= 0); ri++) {
        int rowFirst = ri ? (rows[ri-1]->last + 1) : 0;
        if (rowFirst >= model->count()) {
            if (lastRow < 0)
                lastRow = ri - 1;
            while (rows.size() > ri)
                delete rows.takeLast();
            break;
        }

        if (ri == rows.size()) {
            LayoutRow *row = new LayoutRow(this);
            rows.append(row);
        }

        LayoutRow *row = rows[ri];

        // Use maximumHeight when calculating if the current row is within minY to stay consistent
        // with cachedLayoutOnly and avoid flipping delegates
        if (firstRow < 0 && (y + maximumHeight) >= minY)
            firstRow = ri;

        // Do a cached-only layout for items we're not interested in displaying.
        cachedLayoutOnly = (firstRow < 0 || lastRow >= 0);

        row->updateRow(rowFirst, model->count() - 1);
        row->displayY = y;

        if (row->first <= currentIndex && row->last >= currentIndex) {
            // If the row was laid out with cachedLayoutOnly and isn't presentable,
            // reset cachedLayoutOnly and lay out again as if data changed, because
            // invalidation may not happen naturally when delegates are created via
            // applyPositions.
            if (cachedLayoutOnly) {
                cachedLayoutOnly = false;
                row->dataChanged();
                row->updateRow(rowFirst, model->count() - 1);
            }

            // If it still contains currentIndex, set as currentRow
            if (row->first <= currentIndex && row->last >= currentIndex)
                currentRow = ri;
        }

        if (!row->isPresentable()) {
            if (!cachedLayoutOnly) {
                DEBUG() << "layout: row" << ri << "for" << row->first << "to" << row->last << "still loading"
                        << row->itemsLoading();
            } else {
                DEBUG() << "layout: row" << ri << "for" << row->first << "to" << row->last << "unpresentable at"
                        << y;
            }
        } else {
            DEBUG() << "layout: row" << ri << "for" << row->first << "to" << row->last << "y" << y
                    << "height" << row->displayHeight();
        }

        // The height of an unpresentable row is maximumHeight.
        y += row->displayHeight() + spacing;

        if (y > maxY && lastRow < 0)
            lastRow = ri;
    }

    cachedLayoutOnly = false;

    if (firstRow >= 0 && lastRow >= 0) {
        for (int i = firstRow; i >= 0 && i <= lastRow; i++) {
            applyPositions(rows[i], rows[i]->displayY);
        }

        if (currentRow >= 0 && (currentRow < firstRow || currentRow > lastRow)) {
            applyPositions(rows[currentRow], rows[currentRow]->displayY);
        }

        int firstIndex = rows[firstRow]->first;
        int lastIndex = rows[lastRow]->last;
        int firstCurrent = currentRow >= 0 ? rows[currentRow]->first : -1;
        int lastCurrent = currentRow >= 0 ? rows[currentRow]->last : -1;
        for (auto it = delegates.begin(); it != delegates.end(); ) {
            if ((it.key() < firstIndex || it.key() > lastIndex) &&
                (it.key() < firstCurrent || it.key() > lastCurrent))
            {
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

void FittingGridViewPrivate::updateCurrent(int index)
{
    Q_Q(FittingGridView);

    int oldIndex = currentIndex;
    QQuickItem *oldItem = currentItem;
    currentIndex = index;

    if (currentIndex < 0 || currentIndex >= model->count()) {
        if (currentItem) {
            model->release(currentItem);
            currentItem = 0;
        }
    } else {
        currentItem = createItem(currentIndex);
        // Hold an extra reference to the current item
        model->object(currentIndex);
        if (oldItem && currentItem != oldItem)
            model->release(oldItem);
    }

    q->polish();

    if (oldIndex != currentIndex)
        emit q->currentIndexChanged();
    if (oldItem != currentItem)
        emit q->currentItemChanged();
}

void FittingGridViewPrivate::createHighlight()
{
    Q_Q(FittingGridView);

    if (highlightItem) {
        highlightItem->deleteLater();
        highlightItem = 0;
    }

    if (!highlight || !contentItem) {
        emit q->highlightItemChanged();
        return;
    }

    QQmlContext *creationContext = highlight->creationContext();
    QQmlContext *context = new QQmlContext(creationContext ? creationContext : qmlContext(flickable));
    QObject *nobj = highlight->beginCreate(context);
    if (nobj) {
        QQml_setParent_noEvent(context, nobj);
        highlightItem = qobject_cast<QQuickItem*>(nobj);
        if (!highlightItem)
            delete nobj;
    } else
        delete context;

    if (highlightItem) {
        QQml_setParent_noEvent(highlightItem, contentItem);
        highlightItem->setParentItem(contentItem);
    }

    highlight->completeCreate();
    emit q->highlightItemChanged();
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
            // Set Y to allow for approximate positioning of the current item
            item->setY(y);
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

void FittingGridViewPrivate::applyPendingChanges()
{
    Q_Q(FittingGridView);
    if (pendingChanges.isEmpty())
        return;

    DEBUG() << "layout: model changes:" << pendingChanges;

    // Process changes in the data and update existing rows. It's okay if this process
    // leaves gaps; they will be closed while recalculating row layouts
    bool currentChanged = false;
    int newCurrentIndex = currentIndex;
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

        if (newCurrentIndex >= remove.index) {
            if (newCurrentIndex < remove.end())
                newCurrentIndex = qMin(remove.index, model->count() - 1);
            else
                newCurrentIndex -= remove.count;
            currentChanged  = true;
        }
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

        if (newCurrentIndex >= insert.index) {
            newCurrentIndex += insert.count;
            currentChanged = true;
        }
    }

    pendingChanges.clear();
    if (currentChanged && q->isComponentComplete()) {
        // Avoid changing indexes before they're evaluated for the first time
        if (!currentItem)
            newCurrentIndex = currentIndex;
        updateCurrent(newCurrentIndex);
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
    if (currentItem) {
        model->release(currentItem);
        currentItem = 0;
    }
    if (highlightItem) {
        QQmlGuard<QQmlComponent> tmp = highlight;
        highlight = 0;
        createHighlight();
        highlight = tmp;
    }
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

