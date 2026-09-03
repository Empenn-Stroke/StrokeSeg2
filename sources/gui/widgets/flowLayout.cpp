// SPDX-License-Identifier: AGPL-3.0-or-later
// Adapted from the official Qt "Flow Layout" example (BSD-licensed, Qt Company).

#include "flowLayout.h"

/**
 * @brief Constructs a FlowLayout object with the specified parent widget, margin, horizontal spacing, and vertical spacing.
 * @param parent The parent widget for the layout.
 * @param margin The margin around the layout. If set to -1, the default margin will be used.
 * @param hSpacing The horizontal spacing between items in the layout. If set to -1, the default spacing will be used.
 * @param vSpacing The vertical spacing between items in the layout. If set to -1, the default spacing will be used.
 */
FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing) 
{
    setContentsMargins(margin, margin, margin, margin);
}

/**
 * @brief Constructs a FlowLayout object with the specified margin, horizontal spacing, and vertical spacing.
 * @param margin The margin around the layout. If set to -1, the default margin will be used.
 * @param hSpacing The horizontal spacing between items in the layout. If set to -1, the default spacing will be used.
 * @param vSpacing The vertical spacing between items in the layout. If set to -1, the default spacing will be used.
 */
FlowLayout::FlowLayout(int margin, int hSpacing, int vSpacing)
    : m_hSpace(hSpacing), m_vSpace(vSpacing) 
{
    setContentsMargins(margin, margin, margin, margin);
}

/**
 * @brief Destructor for the FlowLayout class. Cleans up and deletes all layout items managed by this layout.
 */
FlowLayout::~FlowLayout() 
{
    QLayoutItem *item;
    while ((item = takeAt(0))) 
    {
        delete item;
    }
}

/**
 * @brief Adds a new item to the layout. The item is appended to the internal list of items managed by the layout.
 * @param item The QLayoutItem to be added to the layout.
 */
void FlowLayout::addItem(QLayoutItem *item) 
{
    itemList.append(item);
}

/**
 * @brief Returns the horizontal spacing between items in the layout. If a specific horizontal spacing was set during construction, that value is returned. Otherwise, it calculates and returns a smart spacing value based on the style of the parent
 * widget.
 * @return The horizontal spacing between items in the layout.
 */
int FlowLayout::horizontalSpacing() const 
{
    return (m_hSpace >= 0) ? m_hSpace : smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

/**
 * @brief Returns the vertical spacing between items in the layout. If a specific vertical spacing was set during construction, that value is returned. Otherwise, it calculates and returns a smart spacing value based on the style of the parent
 * widget.
 * @return The vertical spacing between items in the layout.
 */
int FlowLayout::verticalSpacing() const 
{
    return (m_vSpace >= 0) ? m_vSpace : smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

/**
 * @brief Returns the number of items currently managed by the layout. This count includes all items that have been added to the layout, regardless of their visibility or position.
 * @return The number of items in the layout.
 */
int FlowLayout::count() const 
{
    return itemList.size();
}

/**
 * @brief Returns the layout item at the specified index. If the index is out of bounds, a null pointer is returned. This function allows access to individual items in the layout for further manipulation or inspection.
 * @param index The index of the item to retrieve. Must be in the range [0, count() - 1].
 * @return A pointer to the QLayoutItem at the specified index, or nullptr if the index is invalid.
 */
QLayoutItem *FlowLayout::itemAt(int index) const 
{
    return itemList.value(index);
}

/**
 * @brief Removes and returns the layout item at the specified index. If the index is out of bounds, a null pointer is returned. The caller is responsible for deleting the returned item if it is no longer needed. This function allows for dynamic
 * @param index The index of the item to remove. Must be in the range [0, count() - 1].
 * @return A pointer to the QLayoutItem that was removed, or nullptr if the index is invalid. The caller is responsible for managing the memory of the returned item.
 */
QLayoutItem *FlowLayout::takeAt(int index) 
{
    QLayoutItem *item = nullptr;

    if (index >= 0 && index < itemList.size()) 
    {
        item = itemList.takeAt(index);
    }

    return item;
}

/**
 * @brief Returns the orientations in which the layout can expand. In this implementation, the layout does not have any specific expanding directions, so an empty set of orientations is returned. This function can be overridden in derived classes to
 * @return An empty set of Qt::Orientations, indicating that the layout does not have any specific expanding directions.
 */
Qt::Orientations FlowLayout::expandingDirections() const 
{
    return {};
}

/**
 * @brief Indicates whether the layout has a height-for-width policy. In this implementation, the layout does have a height-for-width policy, so this function returns true. This means that the layout can adjust its height based on the available
 * width, allowing for more flexible and responsive layouts.
 * @return true, indicating that the layout has a height-for-width policy.
 */
bool FlowLayout::hasHeightForWidth() const 
{
    return true;
}

/**
 * @brief Calculates and returns the preferred height of the layout for a given width. This function is used to determine how tall the layout should be when constrained to a specific width. It calls the doLayout function with the specified width and
 * a testOnly flag set to true, which means that it will only calculate the height without actually performing any layout operations.
 * @param width The width for which to calculate the preferred height.
 * @return The preferred height of the layout for the given width.
 */
int FlowLayout::heightForWidth(int width) const 
{
    return doLayout(QRect(0, 0, width, 0), true);
}

/**
 * @brief Sets the geometry of the layout within the specified rectangle. This function is called by the parent widget to position and size the layout. It first calls the base class implementation to set the geometry, and then it calls the doLayout
 * function to arrange the items within the specified rectangle. The testOnly flag is set to false, indicating that the layout should actually perform the layout operations.
 * @param rect The rectangle within which to set the geometry of the layout. This rectangle defines the area available for the layout to arrange its items. 
 */
void FlowLayout::setGeometry(const QRect &rect) 
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

/**
 * @brief Returns the preferred size of the layout. This function is used to determine the size that the layout would like to have based on its contents and spacing. It calls the minimumSize function to calculate the minimum size required to
 * accommodate all items in the layout, including any margins and spacing. The returned size can be used by the parent widget to determine how much space to allocate for the layout.
 * @return The preferred size of the layout, which is the minimum size required to accommodate all items, including margins and spacing.
 */
QSize FlowLayout::sizeHint() const 
{
    return minimumSize();
}

/**
 * @brief Calculates and returns the minimum size required by the layout to accommodate all its items, including any margins and spacing. This function iterates through all items in the layout, determining the maximum minimum size required for each
 * item, and then combines these sizes to compute the overall minimum size. It also takes into account the contents margins of the layout to ensure that there is enough space around the items.
 * @return The minimum size required by the layout to accommodate all its items, including margins and spacing.
 */
QSize FlowLayout::minimumSize() const 
{
    QSize size;

    for (const QLayoutItem *item : itemList) 
    {
        size = size.expandedTo(item->minimumSize());
    }

    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

/**
 * @brief Performs the layout of items within the specified rectangle. This function is responsible for arranging the items in rows, growing them to fill available space, and centering them within each row. It first calculates the effective rectangle
 * by adjusting for the contents margins, and then it determines the horizontal and vertical spacing to use. The layout is performed in two passes: first, items are grouped into rows based on their natural size, and then each row is processed to grow
 * the items and center them. The function returns the total height used by the layout, including margins.
 * @param rect The rectangle within which to perform the layout. This rectangle defines the area available for arranging the items.
 * @param testOnly A boolean flag indicating whether to only calculate the layout without actually setting the geometry of the items. If true, the function will only compute the layout and return the height used, without modifying the items'
 * positions or sizes.
 * @return The total height used by the layout, including margins, after arranging the items within the specified rectangle.
 */
int FlowLayout::doLayout(const QRect &rect, bool testOnly) const 
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);

    int spaceX = horizontalSpacing();
    int spaceY = verticalSpacing();

    if (spaceX == -1 || spaceY == -1) 
    {
        for (QLayoutItem *item : itemList) 
        {
            const QWidget *wid = item->widget();
            if (wid) 
            {
                if (spaceX == -1)
                {
                    spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
                }
                if (spaceY == -1)
                {
                    spaceY = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
                }
                break;
            }
        }
        if (spaceX == -1) spaceX = 0;
        if (spaceY == -1) spaceY = 0;
    }

    int n = itemList.size();

    QList<QList<QLayoutItem *>> rows;

    if (n > 0) 
    {
        // --- Pass 1: compute a balanced number of columns instead of greedy packing ---
        qreal totalNaturalWidth = 0;

        for (QLayoutItem *item : itemList) 
        {
            QSize baseSize = item->minimumSize().isEmpty() ? item->sizeHint() : item->minimumSize();
            totalNaturalWidth += baseSize.width();
        }

        qreal avgNaturalWidth = totalNaturalWidth / n;

        int maxCols = 1;
        if (avgNaturalWidth + spaceX > 0) 
        {
            maxCols = qMax(1, (int)std::floor((effectiveRect.width() + spaceX) / (avgNaturalWidth + spaceX)));
        }
        maxCols = qMin(maxCols, n);

        int rowsCount = (int)std::ceil((qreal)n / maxCols);
        int balancedCols = (int)std::ceil((qreal)n / rowsCount);

        int idx = 0;
        for (int r = 0; r < rowsCount; ++r) 
        {
            QList<QLayoutItem *> row;
            for (int c = 0; c < balancedCols && idx < n; ++c, ++idx) 
            {
                row.append(itemList.at(idx));
            }
            if (!row.isEmpty()) 
            {
                rows.append(row);
            }
        }
    }

    // --- Pass 2: compute a SINGLE growth factor shared by all rows ---
    qreal growthFactor = m_maxGrowthFactor;

    for (const QList<QLayoutItem *> &row : rows) 
    {
        int naturalWidthSum = 0;

        for (QLayoutItem *item : row) 
        {
            QSize baseSize = item->minimumSize().isEmpty() ? item->sizeHint() : item->minimumSize();
            naturalWidthSum += baseSize.width();
        }

        int spacingTotal = spaceX * (row.size() - 1);
        int availableForItems = effectiveRect.width() - spacingTotal;

        qreal rowFactor = (naturalWidthSum > 0) ? (qreal)availableForItems / (qreal)naturalWidthSum : m_maxGrowthFactor;

        growthFactor = qMin(growthFactor, rowFactor);
    }

    growthFactor = qMax(growthFactor, 1.0);

    // --- Pass 3: place each row's items at the shared growth factor, centered ---
    int y = effectiveRect.y();

    for (const QList<QLayoutItem *> &row : rows) 
    {
        int rowContentWidth = 0;
        int rowContentHeight = 0;
        QList<QSize> finalSizes;

        for (QLayoutItem *item : row) 
        {
            QSize baseSize = item->minimumSize().isEmpty() ? item->sizeHint() : item->minimumSize();
            int finalWidth = qRound(baseSize.width() * growthFactor);
            int finalHeight = qRound(baseSize.height() * growthFactor);

            finalSizes.append(QSize(finalWidth, finalHeight));
            rowContentWidth += finalWidth;
            rowContentHeight = qMax(rowContentHeight, finalHeight);
        }
        rowContentWidth += spaceX * (row.size() - 1);

        int xOffset = effectiveRect.x() + qMax(0, (effectiveRect.width() - rowContentWidth) / 2);
        int x = xOffset;

        for (int i = 0; i < row.size(); ++i) 
        {
            QLayoutItem *item = row.at(i);
            QSize finalSize = finalSizes.at(i);

            if (!testOnly) 
            {
                int itemY = y + (rowContentHeight - finalSize.height()) / 2;
                item->setGeometry(QRect(QPoint(x, itemY), finalSize));
            }

            x += finalSize.width() + spaceX;
        }

        y += rowContentHeight + spaceY;
    }

    int usedHeight = rows.isEmpty() ? 0 : (y - spaceY - effectiveRect.y());
    return usedHeight + top + bottom;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const 
{
    QObject *parent = this->parent();

    int result = -1;

    if (parent) 
    {
        if (parent->isWidgetType()) 
        {
            QWidget *pw = static_cast<QWidget *>(parent);
            result = pw->style()->pixelMetric(pm, nullptr, pw);
        } 
        else 
        {
            result = static_cast<QLayout *>(parent)->spacing();
        }
    }

    return result;
}
