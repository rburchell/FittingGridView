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

#ifndef FITTINGGRIDVIEW_H
#define FITTINGGRIDVIEW_H

#include <QQuickItem>
#include <QQmlParserStatus>

class FittingGridViewPrivate;

class FittingGridView : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(FittingGridView)
    Q_INTERFACES(QQmlParserStatus)
    
public:
    FittingGridView(QQuickItem *parent = 0);
    ~FittingGridView();

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    QVariant model() const;
    void setModel(const QVariant &model);

    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    Q_PROPERTY(QQuickItem* flickable READ flickable WRITE setFlickable NOTIFY flickableChanged)
    QQuickItem *flickable() const;
    void setFlickable(QQuickItem *flickable);

    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    int spacing() const;
    void setSpacing(int spacing);

    Q_PROPERTY(double layoutWidth READ layoutWidth WRITE setLayoutWidth RESET resetLayoutWidth NOTIFY layoutWidthChanged)
    double layoutWidth() const;
    void setLayoutWidth(double layoutWidth);
    void resetLayoutWidth() { setLayoutWidth(0); }

    Q_PROPERTY(double maximumHeight READ maximumHeight WRITE setMaximumHeight NOTIFY maximumHeightChanged)
    double maximumHeight() const;
    void setMaximumHeight(double maximumHeight);

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    int currentIndex() const;
    void setCurrentIndex(int currentIndex);

    Q_INVOKABLE bool incrementCurrentRow();
    Q_INVOKABLE bool decrementCurrentRow();
    Q_INVOKABLE bool incrementCurrentIndex();
    Q_INVOKABLE bool decrementCurrentIndex();

    Q_PROPERTY(QQuickItem *currentItem READ currentItem NOTIFY currentItemChanged)
    QQuickItem *currentItem() const;

    virtual void classBegin();
    virtual void componentComplete();

signals:
    void modelChanged();
    void delegateChanged();
    void flickableChanged();
    void spacingChanged();
    void layoutWidthChanged();
    void maximumHeightChanged();
    void currentIndexChanged();
    void currentItemChanged();

public slots:
    void polish() { QQuickItem::polish(); }

protected:
    virtual void updatePolish();
    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);

private:
    Q_DECLARE_PRIVATE(FittingGridView)
    FittingGridViewPrivate *d_ptr;
};

QML_DECLARE_TYPE(FittingGridView)

#endif // FITTINGGRIDVIEW_H
