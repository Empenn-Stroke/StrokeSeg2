#pragma once

#include <QLayout>
#include <QRect>
#include <QStyle>
#include <QWidget>
#include <QWidgetItem>

class FlowLayout : public QLayout
{
  public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    explicit FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

    /**
     * @brief Maximum factor by which items can be enlarged to fill a row's available
     * width. 1.0 = never grow beyond natural size, 2.0 = can double in size, etc.
     * Default is 1.6, which lets items breathe on wide windows without becoming huge.
     */
    void setMaxGrowthFactor(qreal factor) { m_maxGrowthFactor = factor; }
    qreal maxGrowthFactor() const { return m_maxGrowthFactor; }

  private:
    int doLayout(const QRect &rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> itemList;
    int m_hSpace;
    int m_vSpace;
    qreal m_maxGrowthFactor = 1.6;
};
