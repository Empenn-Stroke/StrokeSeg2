// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QWidget>
#include <utils/pipelineParams.h>

#include <utils/modality.h>


class ThresholdSliderWidget;

class ParametersFormWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit ParametersFormWidget(QWidget *parent = nullptr);

    PipelineParams getParams() const;
    void setParams(const PipelineParams &params);

    void loadSettings();
    void saveSettings();
    void resetFields();
    void setInputsEnabled(bool enabled);

    QList<Modality> getCurrentModelModalities() const;

    QStringList getCurrentModelInputs() const;

    void refreshModels(const QStringList &entries);

    void paintEvent(QPaintEvent *event);

  signals:
    void destinationChanged(const QString &folderPath);
    void parametersChanged();
    void modelChanged(const QStringList &inputs);

  private:
    void setupUI();
    void setupConnections();
    QStringList loadModelManifestInputs(const QString &modelName) const;

  private:

    QFormLayout *m_formLayout;
    QLineEdit *m_suffix;
    QLineEdit *m_destination;
    QPushButton *m_destinationButton;
    QComboBox *m_model;
    QComboBox *m_mode;

    QCheckBox *m_toggleView;
    QCheckBox *m_toggleOpenFolder;
    QCheckBox *m_toggleOutput;
    QCheckBox *m_skipPreProcessing;
    QCheckBox *m_skipInference;
    QCheckBox *m_skipPostProcessing;
    QCheckBox *m_savePMap;
    QCheckBox *m_saveInterSteps;
    QCheckBox *m_brainExtractionCheckBox;

    ThresholdSliderWidget *m_thresholdWidget;
};
