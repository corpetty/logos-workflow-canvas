#pragma once

#include "rep_workflow_canvas_source.h"
#include "logos_ui_plugin_context.h"

#include <QJsonArray>
#include <QString>

/**
 * @brief The Workflow Canvas UI backend (universal authoring model).
 *
 * Replaces the old CanvasWidget. Everything that talked to the three workflow
 * modules lives here; everything that draws the graph stayed in QML.
 *
 * The split is not cosmetic: a ui_qml module's backend runs in a separate
 * ui-host process from the QML view, so this class cannot touch the graph.
 * Where CanvasWidget reached into the live qan::Node objects to paint per-node
 * execution results, this backend only publishes the engine's result JSON on
 * `lastExecutionResult` and QML applies it to its own nodes.
 *
 * workflow_registry / workflow_engine / workflow_scheduler are all DECLARED
 * dependencies, so calls go through the generated typed `modules()` accessors
 * rather than a runtime client.
 */
class WorkflowCanvasBackend : public WorkflowCanvasSimpleSource,
                              public LogosUiPluginContext
{
public:
    void refreshNodeTypes() override;

    void executeWorkflow(const QString& workflowJson) override;
    void clearLastExecutionResult() override;

    void deployWorkflow(const QString& workflowId, const QString& workflowJson) override;

    void saveWorkflow(const QString& name, const QString& workflowJson) override;
    QString loadWorkflow(const QString& name) override;
    QStringList listSavedWorkflows() override;
    void deleteWorkflow(const QString& name) override;

protected:
    /// The palette is fetched once the module is wired, not in the
    /// constructor — the registry can only be called through modules().
    void onContextReady() override;

private:
    /// Where saved workflows live (AppDataLocation/workflows).
    QString workflowDir() const;

    QJsonArray m_history;
};
