// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include <QFile>
#include <QTextEdit>

/**
 * @brief The ModelManager class provides a GUI window for managing machine learning models.
 *
 * This class creates a window with a title bar and content area that can be used to
 * import, delete, and list machine learning models. The window supports dragging and
 * includes functionality to handle model imports and deletions, as well as writing
 * model manifests.
 */
class ModelManager : public QWidget
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor for ModelManager.
     *
     * Initializes the model manager window with a specified parent widget.
     *
     * @param parent The parent widget for this window. Defaults to nullptr.
     */
    explicit ModelManager(QWidget *parent = nullptr);

  protected:
    /**
     * @brief Handles custom events for the window.
     *
     * This method is overridden to provide custom behavior for handling events.
     * It supports dragging the window by clicking and dragging the title bar.
     *
     * @param obj The object that received the event.
     * @param event The event that occurred.
     * @return True if the event was handled, false otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * @brief Handles native events for the window.
     *
     * This method is overridden to provide custom behavior for handling native events.
     *
     * @param eventType The type of the event.
     * @param message The message associated with the event.
     * @param result A pointer to the result of the event.
     * @return True if the event was handled, false otherwise.
     */
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

    bool m_dragging = false; /**< A boolean indicating whether the window is being dragged. */
    QPoint m_dragPosition; /**< The position where the mouse was clicked for dragging. */

private:
    QWidget *m_titleBar; /**< The title bar widget for window dragging. */
    QWidget *m_mainArea; /**< The main area widget containing model entries. */
    QVBoxLayout *m_mainAreaLayout; /**< The layout for the main area. */

  private:
    /**
     * @brief Adds a model entry to the main area.
     *
     * @param modelPath The file path of the model to be added.
     */
    void addModelEntry(const QString &modelPath);

    /**
     * @brief Prompts the user for channel names.
     *
     * @param ok A boolean reference indicating whether the user accepted the input.
     * @return A list of channel names entered by the user.
     */
    QStringList promptForChannelNames(bool &ok);

    /**
     * @brief Writes a model manifest file.
     *
     * @param onnxPath The file path of the ONNX model.
     * @param inputs A list of input names for the model.
     * @param errorMessage A reference to a string for storing error messages.
     * @return True if the manifest was written successfully, false otherwise.
     */
    bool writeModelManifest(const QString &onnxPath, const QStringList &inputs, QString &errorMessage);
    
  private slots:
    /**
     * @brief Imports a model.
     */
    void importModel();

    /**
     * @brief Deletes a model.
     *
     * @param filePath The file path of the model to be deleted.
     * @param container The container widget containing the model entry.
     */
    void deleteModel(const QString &filePath, QWidget *container);
  
  signals:
    /**
     * @brief Signal emitted when the list of models has changed.
     */
    void modelsChanged();
};