// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSlider>
#include <QWidget>

/**
 * @brief A widget that combines a QSlider and a QLineEdit to allow users to select a threshold value between 0.0 and 1.0.
 */
class ThresholdSliderWidget : public QWidget {
    Q_OBJECT

  public:
    explicit ThresholdSliderWidget(QWidget *parent = nullptr);

    double value() const;
    void setValue(double v);

  signals:
    void valueChanged(double newValue);

  private:
    void setupUI();
    void setupConnections();

    double sliderValueToReal(int v) const;
    int realToSliderValue(double v) const;
    QString formatThreshold(double v) const;

    QLineEdit *m_thresholdLine;
    QSlider *m_thresholdSlider;
};
