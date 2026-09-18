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

/**
 * @brief The ParametersFormWidget class provides a widget for configuring pipeline parameters.
 *
 * This class creates a form widget that allows users to configure various pipeline parameters,
 * such as destination folder, model selection, and processing options. It also supports
 * loading and saving settings, as well as refreshing the list of available models.
 */
class ParametersFormWidget : public QWidget
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor for ParametersFormWidget.
     *
     * Initializes the parameters form widget with a specified parent widget.
     *
     * @param parent The parent widget for this widget. Defaults to nullptr.
     */
    explicit ParametersFormWidget(QWidget *parent = nullptr);

    /**
     * @brief Gets the current pipeline parameters.
     *
     * @return The current pipeline parameters.
     */
    PipelineParams getParams() const;

    /**
     * @brief Sets the pipeline parameters.
     *
     * @param params The pipeline parameters to set.
     */
    void setParams(const PipelineParams &params);

    /**
     * @brief Loads the settings from QSettings.
     */
    void loadSettings();

    /**
     * @brief Saves the current settings to QSettings.
     */
    void saveSettings();

    /**
     * @brief Resets all fields to their default values.
     */
    void resetFields();

    /**
     * @brief Enables or disables input fields based on the provided flag.
     *
     * @param enabled True to enable inputs, false to disable them.
     */
    void setInputsEnabled(bool enabled);

    /**
     * @brief Gets the current model modalities.
     *
     * @return A list of current model modalities.
     */
    QList<Modality> getCurrentModelModalities() const;

    /**
     * @brief Gets the current model inputs.
     *
     * @return A list of current model inputs.
     */
    QStringList getCurrentModelInputs() const;

    /**
     * @brief Refreshes the list of available models.
     *
     * @param entries A list of model entries to display.
     */
    void refreshModels(const QStringList &entries);

    /**
     * @brief Handles the paint event for the widget.
     *
     * @param event The paint event that occurred.
     */
    void paintEvent(QPaintEvent *event) override;

  signals:
    /**
     * @brief Signal emitted when the destination folder changes.
     *
     * @param folderPath The new destination folder path.
     */
    void destinationChanged(const QString &folderPath);

    /**
     * @brief Signal emitted when the pipeline parameters change.
     */
    void parametersChanged();

    /**
     * @brief Signal emitted when the model changes.
     *
     * @param inputs A list of new model inputs.
     */
    void modelChanged(const QStringList &inputs);

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
     * @brief Loads the model manifest inputs for a given model name.
     *
     * @param modelName The name of the model.
     * @return A list of model inputs.
     */
    QStringList loadModelManifestInputs(const QString &modelName) const;

  private:
    QFormLayout *m_formLayout; /**< The form layout for the widget. */
    QLineEdit *m_suffix; /**< The line edit for the file suffix. */
    QLineEdit *m_destination; /**< The line edit for the destination folder. */
    QPushButton *m_destinationButton; /**< The button for selecting the destination folder. */
    QComboBox *m_model; /**< The combo box for selecting the model. */
    QComboBox *m_mode; /**< The combo box for selecting the mode. */

    QCheckBox *m_toggleView; /**< The checkbox for toggling view. */
    QCheckBox *m_toggleOpenFolder; /**< The checkbox for toggling folder opening. */
    QCheckBox *m_toggleOutput; /**< The checkbox for toggling output. */
    QCheckBox *m_skipPreProcessing; /**< The checkbox for skipping pre-processing. */
    QCheckBox *m_skipInference; /**< The checkbox for skipping inference. */
    QCheckBox *m_skipPostProcessing; /**< The checkbox for skipping post-processing. */
    QCheckBox *m_savePMap; /**< The checkbox for saving PMap. */
    QCheckBox *m_saveInterSteps; /**< The checkbox for saving intermediate steps. */
    QCheckBox *m_brainExtractionCheckBox; /**< The checkbox for brain extraction. */

    ThresholdSliderWidget *m_thresholdWidget; /**< The threshold slider widget. */
};
