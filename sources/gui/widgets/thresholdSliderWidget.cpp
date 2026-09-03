// SPDX-License-Identifier: AGPL-3.0-or-later
#include "thresholdSliderWidget.h"
#include <QLocale>
#include <cmath>

/**
 * @brief Constructs a ThresholdSliderWidget
 * @param parent The parent widget
 */
ThresholdSliderWidget::ThresholdSliderWidget(QWidget *parent) : QWidget(parent)
{
    setupUI();
    setupConnections();
}

/**
 * @brief Sets up the user interface
 */
void ThresholdSliderWidget::setupUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_thresholdLine = new QLineEdit("0.50", this);
    m_thresholdLine->setObjectName("thresholdLine");

    auto *validator = new QDoubleValidator(0.0, 1.0, 2, this);
    validator->setLocale(QLocale::C);
    m_thresholdLine->setValidator(validator);

    m_thresholdSlider = new QSlider(Qt::Horizontal, this);
    m_thresholdSlider->setRange(0, 100);
    m_thresholdSlider->setValue(50);
    m_thresholdSlider->setFixedWidth(146);
    m_thresholdSlider->setObjectName("thresholdSlider");

    layout->addWidget(m_thresholdLine);
    layout->addWidget(m_thresholdSlider);
}

/**
 * @brief Sets up the connections between the slider and the line edit.
 */
void ThresholdSliderWidget::setupConnections()
{
    connect(m_thresholdSlider, &QSlider::valueChanged, this, [this](int v)
    {
        double realVal = sliderValueToReal(v);
        m_thresholdLine->blockSignals(true);
        m_thresholdLine->setText(formatThreshold(realVal));
        m_thresholdLine->blockSignals(false);
        emit valueChanged(realVal);
    });

    connect(m_thresholdLine, &QLineEdit::textChanged, this, [this](const QString &text)
    {
        bool ok;
        double val = 0.0;

        if (text == "1-10\u207B\u2075")
        {
            val = 1.0 - 1e-5;
        }
        else if (text == "10\u207B\u2075")
        {
            val = 1e-5;
        }
        else if (text == "1-10\u207B\u2074")
        {
            val = 1.0 - 1e-4;
        }
        else if (text == "10\u207B\u2074")
        {
            val = 1e-4;
        }
        else
        {
            val = text.toDouble(&ok);
            if (!ok)
            {
                return;
            }
        }
        int sliderVal = realToSliderValue(val);

        if (sliderVal != m_thresholdSlider->value())
        {
            m_thresholdSlider->blockSignals(true);
            m_thresholdSlider->setValue(sliderVal);
            m_thresholdSlider->blockSignals(false);
            emit valueChanged(val);
        }
    });
}

/**
 * @brief Returns the current threshold value
 * @return The threshold value
 */
double ThresholdSliderWidget::value() const
{
    return sliderValueToReal(m_thresholdSlider->value());
}

/**
 * @brief Sets the threshold value
 * @param v The threshold value
 */
void ThresholdSliderWidget::setValue(double v)
{
    m_thresholdSlider->setValue(realToSliderValue(v));
    m_thresholdLine->setText(formatThreshold(v));
}

/**
 * @brief Converts a slider value to a real threshold value
 * @param v The slider value
 * @return The corresponding threshold value
 */
double ThresholdSliderWidget::sliderValueToReal(int v) const
{
    double result = 0.0;

    if (v <= 0)
    {
        result = 1e-5;
    }
    else if (v <= 8)
    {
        result = 1e-4;
    }
    else if (v < 16)
    {
        result = 1e-3;
    }
    else if (v == 16)
    {
        result = 0.01;
    }
    else if (v >= 100)
    {
        result = 1.0 - 1e-5;
    }
    else if (v >= 92)
    {
        result = 1.0 - 1e-4;
    }
    else if (v > 84)
    {
        result = 1.0 - 1e-3;
    }
    else if (v == 84)
    {
        result = 0.99;
    }
    else
    {
        double t = (v - 16) / 80.0;
        result = 0.001 + t * (0.999 - 0.001);
    }

    return result;
}

/**
 * @brief Converts a real threshold value to a slider value
 * @param v The threshold value
 * @return The corresponding slider value
 */
int ThresholdSliderWidget::realToSliderValue(double v) const
{
    int result = 0;

    if (v <= 1e-5)
    {
        result = 0;
    }
    else if (v <= 1e-4)
    {
        result = 8;
    }
    else if (v <= 1e-3)
    {
        result = 16;
    }
    else if (v >= 1.0 - 1e-5)
    {
        result = 100;
    }
    else if (v >= 1.0 - 1e-4)
    {
        result = 92;
    }
    else if (v >= 1.0 - 1e-3)
    {
        result = 84;
    }
    else
    {
        double t = (v - 0.001) / (0.999 - 0.001);
        result = 16 + int(std::round(t * 68));
    }

    return result;
}

/**
 * @brief Formats the threshold value for display in the line edit
 * @param v The threshold value
 * @return The formatted string
 */
QString ThresholdSliderWidget::formatThreshold(double v) const
{
    QString result;

    if (v <= 1e-5)
    {
        result = "10\u207B\u2075";
    }
    else if (v <= 1e-4)
    {
        result = "10\u207B\u2074";
    }
    else if (v <= 1e-3)
    {
        result = "0.001";
    }
    else if (v >= 1.0 - 1e-5)
    {
        result = "1-10\u207B\u2075";
    }
    else if (v >= 1.0 - 1e-4)
    {
        result = "1-10\u207B\u2074";
    }
    else if (v >= 1.0 - 1e-3)
    {
        result = "0.999";
    }
    else
    {
        result = QString::number(v, 'f', 2);
    }

    return result;
}
