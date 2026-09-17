// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSlider>
#include <QWidget>

/**
 * @brief The ThresholdSliderWidget class combines a QSlider and a QLineEdit to allow users
 * to select a threshold value between 0.0 and 1.0. The widget emits a signal when the value changes.
 */
class ThresholdSliderWidget : public QWidget {
    Q_OBJECT

  public:
    /**
     * @brief Constructor for ThresholdSliderWidget.
     *
     * Initializes the threshold slider widget with a specified parent widget.
     *
     * @param parent The parent widget for this widget. Defaults to nullptr.
     */
    explicit ThresholdSliderWidget(QWidget *parent = nullptr);

    /**
     * @brief Gets the current threshold value.
     *
     * @return The current threshold value as a double.
     */
    double value() const;

    /**
     * @brief Sets the threshold value.
     *
     * @param v The new threshold value to set.
     */
    void setValue(double v);

  signals:
    /**
     * @brief Signal emitted when the threshold value changes.
     *
     * @param newValue The new threshold value.
     */
    void valueChanged(double newValue);

  private:
    /**
     * @brief Sets up the user interface components.
     */
    void setupUI();

    /**
     * @brief Sets up the connections between UI components and signals.
     */
    void setupConnections();

    /**
     * @brief Converts a slider value to a real threshold value.
     *
     * @param v The slider value.
     * @return The corresponding real threshold value.
     */
    double sliderValueToReal(int v) const;

    /**
     * @brief Converts a real threshold value to a slider value.
     *
     * @param v The real threshold value.
     * @return The corresponding slider value.
     */
    int realToSliderValue(double v) const;

    /**
     * @brief Formats the threshold value as a string.
     *
     * @param v The threshold value.
     * @return The formatted threshold value as a string.
     */
    QString formatThreshold(double v) const;

private:
    QLineEdit *m_thresholdLine; /**< The line edit for entering the threshold value. */
    QSlider *m_thresholdSlider; /**< The slider for selecting the threshold value. */
};
