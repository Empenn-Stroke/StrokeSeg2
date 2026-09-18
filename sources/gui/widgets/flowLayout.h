#pragma once

#include <QLayout>
#include <QRect>
#include <QStyle>
#include <QWidget>
#include <QWidgetItem>

/**
 * @brief The FlowLayout class provides a layout manager that arranges child widgets in a flow-like manner.
 *
 * This class extends QLayout and arranges widgets in a flow that dynamically adjusts based on available
 * space. It supports setting horizontal and vertical spacing, as well as a maximum growth factor for
 * widgets to fill available space without becoming excessively large.
 */
class FlowLayout : public QLayout
{
  public:
    /**
     * @brief Constructor for FlowLayout with a parent widget.
     *
     * Initializes the flow layout with a specified parent widget and spacing.
     *
     * @param parent The parent widget for this layout. Defaults to nullptr.
     * @param margin The margin around the layout. Defaults to -1 (default layout margin).
     * @param hSpacing The horizontal spacing between widgets. Defaults to -1 (default layout spacing).
     * @param vSpacing The vertical spacing between widgets. Defaults to -1 (default layout spacing).
     */
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);

    /**
     * @brief Constructor for FlowLayout without a parent widget.
     *
     * Initializes the flow layout with specified spacing.
     *
     * @param margin The margin around the layout. Defaults to -1 (default layout margin).
     * @param hSpacing The horizontal spacing between widgets. Defaults to -1 (default layout spacing).
     * @param vSpacing The vertical spacing between widgets. Defaults to -1 (default layout spacing).
     */
    explicit FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~FlowLayout() override;

    /**
     * @brief Adds an item to the layout.
     *
     * @param item The item to add.
     */
    void addItem(QLayoutItem *item) override;

    /**
     * @brief Gets the horizontal spacing between widgets.
     *
     * @return The horizontal spacing.
     */
    int horizontalSpacing() const;

    /**
     * @brief Gets the vertical spacing between widgets.
     *
     * @return The vertical spacing.
     */
    int verticalSpacing() const;

    /**
     * @brief Returns the directions in which the layout can expand.
     *
     * @return The expanding directions.
     */
    Qt::Orientations expandingDirections() const override;

    /**
     * @brief Checks if the layout has a height for a given width.
     *
     * @return True if the layout has a height for width, false otherwise.
     */
    bool hasHeightForWidth() const override;

    /**
     * @brief Calculates the height for a given width.
     *
     * @param width The width for which to calculate the height.
     * @return The calculated height.
     */
    int heightForWidth(int) const override;

    /**
     * @brief Gets the number of items in the layout.
     *
     * @return The number of items.
     */
    int count() const override;

    /**
     * @brief Gets an item at a specific index.
     *
     * @param index The index of the item to retrieve.
     * @return The item at the specified index.
     */
    QLayoutItem *itemAt(int index) const override;

    /**
     * @brief Gets the minimum size of the layout.
     *
     * @return The minimum size.
     */
    QSize minimumSize() const override;

    /**
     * @brief Sets the geometry of the layout.
     *
     * @param rect The rectangle defining the layout's geometry.
     */
    void setGeometry(const QRect &rect) override;

    /**
     * @brief Gets the size hint for the layout.
     *
     * @return The size hint.
     */
    QSize sizeHint() const override;

    /**
     * @brief Removes and returns an item at a specific index.
     *
     * @param index The index of the item to remove.
     * @return The removed item.
     */
    QLayoutItem *takeAt(int index) override;

    /**
     * @brief Sets the maximum factor by which items can grow to fill a row's available width.
     *
     * @param factor The growth factor.
     */
    void setMaxGrowthFactor(qreal factor) { m_maxGrowthFactor = factor; }

    /**
     * @brief Gets the maximum factor by which items can grow.
     *
     * @return The growth factor.
     */
    qreal maxGrowthFactor() const { return m_maxGrowthFactor; }

  private:
    /**
     * @brief Arranges items in the layout.
     *
     * @param rect The rectangle defining the layout's geometry.
     * @param testOnly If true, the method only calculates the layout and does not apply it.
     * @return The height of the layout.
     */
    int doLayout(const QRect &rect, bool testOnly) const;

    /**
     * @brief Determines the spacing based on the style metrics.
     *
     * @param pm The pixel metric to use for spacing.
     * @return The spacing.
     */
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> itemList; /**< The list of items in the layout. */
    int m_hSpace; /**< The horizontal spacing between widgets. */
    int m_vSpace; /**< The vertical spacing between widgets. */
    qreal m_maxGrowthFactor = 1.6; /**< The maximum growth factor for widgets. */
};
