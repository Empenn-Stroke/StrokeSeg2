#include "parametersFormWidget.h"
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "parametersFormWidget.h"
#include "thresholdSliderWidget.h"
#include <QJsonParseError>
#include <QPainter>
#include<QHBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <utils/env_path.h>
#include <utils/modelManifest.h>
/**
 * @brief Constructs a ParametersFormWidget object with the given parent widget.
 * @param parent The parent widget.
 */
ParametersFormWidget::ParametersFormWidget(QWidget *parent) : QWidget(parent)
{
    setupUI();
    setupConnections();
}

/**
 * @brief Sets up the user interface for the ParametersFormWidget.
 */
void ParametersFormWidget::setupUI()
{
    m_formLayout = new QFormLayout(this);
    m_formLayout->setContentsMargins(0, 0, 0, 0);
    m_formLayout->setSpacing(8);

    // Suffix
    m_suffix = new QLineEdit(this);
    m_suffix->setObjectName("suffix");
    m_suffix->setPlaceholderText("Enter the suffix name");

    // Destination Container
    QWidget *destinationContainer = new QWidget(this);
    destinationContainer->setObjectName("destinationContainer");
    destinationContainer->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *destinationLayout = new QHBoxLayout(destinationContainer);
    destinationLayout->setSpacing(8);
    destinationLayout->setContentsMargins(0, 0, 0, 0);

    m_destination = new QLineEdit(destinationContainer);
    m_destination->setObjectName("destination");
    m_destination->setPlaceholderText("Select output folder");

    m_destinationButton = new QPushButton(destinationContainer);
    m_destinationButton->setText("...");
    m_destinationButton->setObjectName("destinationBtn");

    destinationLayout->addWidget(m_destination);
    destinationLayout->addWidget(m_destinationButton);

    // Model
    m_model = new QComboBox(this);
    QDir modelsDir = Paths::modelDir();
    QStringList entries = modelsDir.entryList(QStringList() << "*.onnx", QDir::Files);
    for (const QString &entry : entries) {
        m_model->addItem(QFileInfo(modelsDir.filePath(entry)).baseName());
    }

    // Toggles & Checkboxes
    m_toggleView = new QCheckBox("", this);
    m_toggleOpenFolder = new QCheckBox("", this);
    m_toggleOutput = new QCheckBox("", this);
    m_skipPreProcessing = new QCheckBox("", this);
    m_skipInference = new QCheckBox("", this);
    m_skipPostProcessing = new QCheckBox("", this);
    m_savePMap = new QCheckBox("", this);
    m_saveInterSteps = new QCheckBox("", this);
    m_brainExtractionCheckBox = new QCheckBox("", this);

    m_brainExtractionCheckBox->setToolTip("Turn off if your image is already skull-stripped");
    m_brainExtractionCheckBox->setChecked(true);

    // Execution Mode
    m_mode = new QComboBox(this);
    m_mode->addItems({"Prediction", "Brain Extraction Only"});

    // Threshold
    m_thresholdWidget = new ThresholdSliderWidget(this);

    // Assembly
    m_formLayout->addRow("Suffix :", m_suffix);
    m_formLayout->addRow("Destination :", destinationContainer);
    m_formLayout->addRow("Model :", m_model);
    m_formLayout->addRow("Open viewer :", m_toggleView);
    m_formLayout->addRow("Open destination folder :", m_toggleOpenFolder);
    m_formLayout->addRow("Output MNI space :", m_toggleOutput);
    m_formLayout->addRow("Skip pre-processing:", m_skipPreProcessing);
    m_formLayout->addRow("Skip inference:", m_skipInference);
    m_formLayout->addRow("Skip post-processing:", m_skipPostProcessing);
    m_formLayout->addRow("Save probability map :", m_savePMap);
    m_formLayout->addRow("Save intermediary steps :", m_saveInterSteps);
    m_formLayout->addRow("Brain extraction (BET) :", m_brainExtractionCheckBox);
    m_formLayout->addRow("Execution mode :", m_mode);
    m_formLayout->addRow("Threshold :", m_thresholdWidget);
}

/**
 * @brief Sets up the connections for the ParametersFormWidget.
 */
void ParametersFormWidget::setupConnections() 
{
    connect(m_destinationButton, &QPushButton::clicked, this, [this]() 
    {
        QSettings settings;
        QString lastDest = settings.value("lastDestPath", QDir::homePath()).toString();
        QString folderPath = QFileDialog::getExistingDirectory(nullptr, "Choose output folder", lastDest, QFileDialog::ShowDirsOnly);

        if (!folderPath.isEmpty()) {
            m_destination->setText(folderPath);
            settings.setValue("lastDestPath", folderPath);
            emit destinationChanged(folderPath);
        }
    });

    connect(m_mode, &QComboBox::currentIndexChanged, this, [this](int index) 
    {
        bool isBetOnly = (index == 1);

        QWidget *thresholdLabel = m_formLayout->labelForField(m_thresholdWidget);
        if (thresholdLabel)
            thresholdLabel->setVisible(!isBetOnly);
        m_thresholdWidget->setVisible(!isBetOnly);

        QList<QCheckBox *> conditionalBoxes = {m_skipPreProcessing, m_skipInference, m_skipPostProcessing, m_savePMap, m_saveInterSteps, m_brainExtractionCheckBox};

        for (auto *cb : conditionalBoxes) {
            cb->setChecked(false);
            cb->setEnabled(!isBetOnly);
        }
    });

    connect(m_destination, &QLineEdit::textChanged, this, &ParametersFormWidget::parametersChanged);

    connect(m_model, &QComboBox::currentTextChanged, this, &ParametersFormWidget::parametersChanged);

    connect(m_brainExtractionCheckBox, &QCheckBox::toggled, this, &ParametersFormWidget::parametersChanged);

    connect(m_model, &QComboBox::currentTextChanged, this, [this]() { emit modelChanged(getCurrentModelInputs()); });
}

/**
 * @brief Retrieves the current parameters from the form widget.
 * @return A PipelineParams object containing the current parameters.
 */
PipelineParams ParametersFormWidget::getParams() const 
{
    PipelineParams p;
    p.openViewer = m_toggleView->isChecked();
    p.openFolder = m_toggleOpenFolder->isChecked();
    p.outputDir = m_destination->text();
    p.modelPath = Paths::modelDir().filePath(m_model->currentText() + ".onnx");
    p.suffix = m_suffix->text();
    p.savePMap = m_savePMap->isChecked();
    p.saveInterSteps = m_saveInterSteps->isChecked();
    p.skipPreProcessing = m_skipPreProcessing->isChecked();
    p.skipInference = m_skipInference->isChecked();
    p.skipPostProcessing = m_skipPostProcessing->isChecked();
    p.mni = m_toggleOutput->isChecked();
    p.brainExtraction = m_brainExtractionCheckBox->isChecked();
    p.betOnly = (m_mode->currentText() == "Brain Extraction Only");
    p.threshold = static_cast<float>(m_thresholdWidget->value());
    return p;
}

/**
 * @brief Sets the parameters of the form widget based on the provided PipelineParams object.
 * @param params The PipelineParams object containing the parameters to set.
 */
void ParametersFormWidget::setParams(const PipelineParams &params)
{
    if (!params.outputDir.isEmpty())
    {
        m_destination->setText(params.outputDir);
        m_destination->setCursorPosition(m_destination->text().length());
    }
    if (!params.suffix.isEmpty())
    {
        m_suffix->setText(params.suffix);
    }
    if (params.threshold > 0 && params.threshold != 0.5)
    {
        m_thresholdWidget->setValue(params.threshold);
    }
    m_savePMap->setChecked(params.savePMap);
    m_saveInterSteps->setChecked(params.saveInterSteps);
    m_brainExtractionCheckBox->setChecked(params.brainExtraction);
    m_skipPreProcessing->setChecked(params.skipPreProcessing);
    m_skipInference->setChecked(params.skipInference);
    m_skipPostProcessing->setChecked(params.skipPostProcessing);

    if (!params.modelPath.isEmpty()) 
    {
        QString modelName = QFileInfo(params.modelPath).baseName();
        int index = m_model->findText(modelName);
        if (index != -1)
            m_model->setCurrentIndex(index);
    }
}

/**
 * @brief Loads the saved settings for the form widget from QSettings.
 */
void ParametersFormWidget::loadSettings()
{
    QSettings settings;
    m_suffix->setText(settings.value("suffix", "").toString());
    m_destination->setText(settings.value("destination", "").toString());

    int modelIdx = settings.value("modelIndex", 0).toInt();
    if (modelIdx < m_model->count())
    {
        m_model->setCurrentIndex(modelIdx);
    }

    m_mode->setCurrentIndex(settings.value("executionMode", 0).toInt());
    m_toggleView->setChecked(settings.value("toggleView", false).toBool());
    m_toggleOpenFolder->setChecked(settings.value("toggleOpenFolder", false).toBool());
    m_toggleOutput->setChecked(settings.value("toggleOutput", false).toBool());
    m_skipPreProcessing->setChecked(settings.value("skipPreProcessing", false).toBool());
    m_skipInference->setChecked(settings.value("skipInference", false).toBool());
    m_skipPostProcessing->setChecked(settings.value("skipPostProcessing", false).toBool());
    m_savePMap->setChecked(settings.value("savePMap", false).toBool());
    m_saveInterSteps->setChecked(settings.value("saveInterSteps", false).toBool());
    m_brainExtractionCheckBox->setChecked(settings.value("brainExtraction", true).toBool());
    m_thresholdWidget->setValue(settings.value("threshold", 0.50).toDouble());
}

/**
 * @brief Saves the current settings of the form widget to QSettings.
 */
void ParametersFormWidget::saveSettings() 
{
    QSettings settings;
    settings.setValue("suffix", m_suffix->text());
    settings.setValue("destination", m_destination->text());
    settings.setValue("modelIndex", m_model->currentIndex());
    settings.setValue("executionMode", m_mode->currentIndex());
    settings.setValue("toggleView", m_toggleView->isChecked());
    settings.setValue("toggleOpenFolder", m_toggleOpenFolder->isChecked());
    settings.setValue("toggleOutput", m_toggleOutput->isChecked());
    settings.setValue("skipPreProcessing", m_skipPreProcessing->isChecked());
    settings.setValue("skipInference", m_skipInference->isChecked());
    settings.setValue("skipPostProcessing", m_skipPostProcessing->isChecked());
    settings.setValue("savePMap", m_savePMap->isChecked());
    settings.setValue("saveInterSteps", m_saveInterSteps->isChecked());
    settings.setValue("threshold", m_thresholdWidget->value());
    settings.setValue("brainExtraction", m_brainExtractionCheckBox->isChecked());
}

/**
 * @brief Resets all fields in the form widget to their default values.
 */
void ParametersFormWidget::resetFields() 
{
    m_suffix->setText("");
    m_destination->setText("");
    m_toggleView->setChecked(false);
    m_toggleOpenFolder->setChecked(false);
    m_toggleOutput->setChecked(false);
    m_savePMap->setChecked(false);
    m_saveInterSteps->setChecked(false);
    m_brainExtractionCheckBox->setChecked(true);
    m_thresholdWidget->setValue(0.50);
    m_model->setCurrentIndex(1);
    m_mode->setCurrentIndex(0);
    m_skipPreProcessing->setChecked(false);
    m_skipInference->setChecked(false);
    m_skipPostProcessing->setChecked(false);
}

/**
 * @brief Enables or disables the input fields in the form widget based on the provided boolean value.
 * @param enabled If true, enables the input fields; if false, disables them.
 */
void ParametersFormWidget::setInputsEnabled(bool enabled)
{
    m_suffix->setEnabled(enabled);
    m_destination->setEnabled(enabled);
    m_destinationButton->setEnabled(enabled);
    m_model->setEnabled(enabled);
    m_toggleView->setEnabled(enabled);
    m_toggleOpenFolder->setEnabled(enabled);
    m_toggleOutput->setEnabled(enabled);
    m_mode->setEnabled(enabled);

    bool isBetOnly = (m_mode->currentIndex() == 1);
    m_thresholdWidget->setEnabled(enabled && !isBetOnly);
    m_skipPreProcessing->setEnabled(enabled && !isBetOnly);
    m_brainExtractionCheckBox->setEnabled(enabled && !isBetOnly);
    m_skipInference->setEnabled(enabled && !isBetOnly);
    m_skipPostProcessing->setEnabled(enabled && !isBetOnly);
    m_savePMap->setEnabled(enabled && !isBetOnly);
    m_saveInterSteps->setEnabled(enabled && !isBetOnly);
}



/**
 * @brief Retrieves the current model modalities based on the selected model in the combo box. It constructs the path to the ONNX model file and loads the modalities using the ModelManifest class.
 * @return A QList of Modality objects representing the current model modalities.
 */
QList<Modality> ParametersFormWidget::getCurrentModelModalities() const 
{
    QString onnxPath = Paths::modelDir().filePath(m_model->currentText() + ".onnx");
    return ModelManifest::loadModalities(onnxPath);
}

/**
 * @brief Retrieves the current model inputs based on the selected model in the combo box. If the selected model contains "multimodal" or "flair" (case-insensitive), it returns a list containing "t1" and "flair". Otherwise, it returns a list
 * containing only "t1".
 * @return A QStringList containing the current model inputs.
 */
QStringList ParametersFormWidget::getCurrentModelInputs() const
{
    QStringList names;
    for (const Modality &m : getCurrentModelModalities()) 
    {
        names << m.name;
    }
    return names;
}

/**
 * @brief Refreshes the model selection combo box with the provided list of entries. It retains the currently selected model if it exists in the new list.
 * @param entries A QStringList containing the new model entries to populate the combo box.
 */
void ParametersFormWidget::refreshModels(const QStringList &entries) 
{
    QString currentModel = m_model->currentText();
    m_model->clear();
    for (const QString &entry : entries) 
    {
        m_model->addItem(QFileInfo(entry).baseName());
    }

    int index = m_model->findText(currentModel);
    if (index != -1)
    {
        m_model->setCurrentIndex(index);
    }
}

/**
 * @brief Overrides the paint event to ensure that the widget is painted with the correct style. This is particularly useful for custom widgets that need to maintain a consistent appearance with the rest of the application.
 * @param event The QPaintEvent that triggered the paint operation.
 */
void ParametersFormWidget::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
