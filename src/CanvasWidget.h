#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QQuickWidget>
#include <QQmlContext>
#include <QJsonArray>
#include <QJsonObject>

class LogosAPI;
class WorkflowGraphModel;

/**
 * @brief Main widget for the Workflow Canvas
 *
 * Hosts a QQuickWidget running the QuickQanava-based graph editor.
 * Communicates with the workflow_registry module to populate the
 * node palette, and with the workflow_engine module to execute
 * workflows. All inter-module calls go through LogosAPI.
 */
class CanvasWidget : public QWidget
{
    Q_OBJECT

    // Properties exposed to QML
    Q_PROPERTY(QJsonArray nodeTypeDefinitions READ nodeTypeDefinitions NOTIFY nodeTypesChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QJsonArray executionHistory READ executionHistory NOTIFY executionHistoryChanged)

public:
    explicit CanvasWidget(LogosAPI* logosAPI, QWidget* parent = nullptr);
    ~CanvasWidget() override;

    QJsonArray nodeTypeDefinitions() const { return m_nodeTypes; }
    QString connectionStatus() const { return m_connectionStatus; }
    QJsonArray executionHistory() const { return m_executionHistory; }

public slots:
    /**
     * @brief Execute the current workflow graph
     * @param workflowJson Serialized graph from QML
     */
    void executeWorkflow(const QString& workflowJson);

    /**
     * @brief Refresh the node palette from the registry
     */
    void refreshNodeTypes();

    /**
     * @brief Save a workflow to disk
     */
    void saveWorkflow(const QString& name, const QString& workflowJson);

    /**
     * @brief Load a workflow from disk
     * @return Workflow JSON string, or empty on failure
     */
    QString loadWorkflow(const QString& name);

    /**
     * @brief List saved workflow names
     */
    QStringList listSavedWorkflows();

    /**
     * @brief Delete a saved workflow
     */
    void deleteWorkflow(const QString& name);

    /**
     * @brief Deploy a workflow to the scheduler
     */
    void deployWorkflow(const QString& workflowId, const QString& workflowJson);

signals:
    void nodeTypesChanged();
    void connectionStatusChanged();
    void executionHistoryChanged();

    // Execution events forwarded to QML for visualization
    void executionStarted(const QString& executionId);
    void nodeExecuting(const QString& executionId, const QString& nodeId);
    void nodeCompleted(const QString& executionId, const QString& nodeId, const QJsonObject& result);
    void nodeFailed(const QString& executionId, const QString& nodeId, const QString& error);
    void executionCompleted(const QString& executionId, const QJsonObject& result);

private:
    void setupUI();
    void connectToModules();
    QString workflowDir() const;

    LogosAPI* m_logosAPI = nullptr;
    QQuickWidget* m_quickWidget = nullptr;

    QJsonArray m_nodeTypes;
    QString m_connectionStatus = "disconnected";
    QJsonArray m_executionHistory;
};
