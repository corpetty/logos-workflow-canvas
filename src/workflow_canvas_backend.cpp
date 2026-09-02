#include "workflow_canvas_backend.h"

// Generated umbrella: LogosModules (behind modules()) from
// metadata.json#dependencies — typed wrappers for the three workflow modules.
#include "logos_sdk.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

constexpr int kMaxHistory = 100;

QString compact(const QJsonArray& arr)
{
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

// A workflow name arrives from the view and becomes a path component. Keep it
// to a single harmless filename so a name like "../../config" cannot walk out
// of the workflow directory.
QString sanitizeName(const QString& name)
{
    QString safe;
    safe.reserve(name.size());
    for (const QChar c : name) {
        if (c.isLetterOrNumber() || c == '-' || c == '_' || c == ' ')
            safe.append(c);
    }
    return safe.trimmed();
}

} // namespace

void WorkflowCanvasBackend::onContextReady()
{
    refreshNodeTypes();
}

void WorkflowCanvasBackend::refreshNodeTypes()
{
    const QString json = modules().workflow_registry.getNodeTypeDefinitions();

    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) {
        qWarning() << "[canvas] workflow_registry returned no palette";
        setConnectionStatus(QStringLiteral("no_registry"));
        return;
    }

    qDebug() << "[canvas] palette:" << doc.array().size() << "node types";
    setNodeTypeDefinitions(json);
    setConnectionStatus(QStringLiteral("connected"));
}

void WorkflowCanvasBackend::executeWorkflow(const QString& workflowJson)
{
    const QString resultJson = modules().workflow_engine.executeWorkflow(workflowJson);

    // Published as-is. The view reads `nodeResults` out of it and paints its
    // own nodes — the backend is in a different process and has no graph to
    // reach into, which is what CanvasWidget used to do here.
    setLastExecutionResult(resultJson);

    const QJsonObject resultObj = QJsonDocument::fromJson(resultJson.toUtf8()).object();
    m_history.prepend(resultObj);
    while (m_history.size() > kMaxHistory)
        m_history.removeLast();
    setExecutionHistory(compact(m_history));
}

void WorkflowCanvasBackend::clearLastExecutionResult()
{
    setLastExecutionResult(QString());
}

void WorkflowCanvasBackend::deployWorkflow(const QString& workflowId,
                                           const QString& workflowJson)
{
    modules().workflow_scheduler.deployWorkflow(workflowId, workflowJson);
    qDebug() << "[canvas] deployed workflow:" << workflowId;
}

QString WorkflowCanvasBackend::workflowDir() const
{
    // AppDataLocation, as before. A UI plugin gets no host-provided
    // per-instance path: LogosUiPluginContext deliberately carries only
    // modules() — "a UI plugin is a view, not a module" — so there is no
    // instancePersistencePath() to prefer here the way a core module would.
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/workflows");
    QDir().mkpath(dir);
    return dir;
}

void WorkflowCanvasBackend::saveWorkflow(const QString& name, const QString& workflowJson)
{
    const QString safe = sanitizeName(name);
    if (safe.isEmpty()) {
        qWarning() << "[canvas] refusing to save workflow under empty name:" << name;
        return;
    }

    const QString path = workflowDir() + "/" + safe + ".json";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(workflowJson.toUtf8());
        qDebug() << "[canvas] saved workflow:" << path;
    } else {
        qWarning() << "[canvas] failed to save workflow:" << path;
    }
}

QString WorkflowCanvasBackend::loadWorkflow(const QString& name)
{
    const QString safe = sanitizeName(name);
    if (safe.isEmpty())
        return {};

    const QString path = workflowDir() + "/" + safe + ".json";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString::fromUtf8(file.readAll());

    qWarning() << "[canvas] failed to load workflow:" << path;
    return {};
}

QStringList WorkflowCanvasBackend::listSavedWorkflows()
{
    QDir dir(workflowDir());
    const QStringList files =
        dir.entryList({ QStringLiteral("*.json") }, QDir::Files, QDir::Time);

    QStringList names;
    names.reserve(files.size());
    for (const QString& file : files)
        names.append(file.chopped(5));   // drop ".json"
    return names;
}

void WorkflowCanvasBackend::deleteWorkflow(const QString& name)
{
    const QString safe = sanitizeName(name);
    if (safe.isEmpty())
        return;

    const QString path = workflowDir() + "/" + safe + ".json";
    QFile::remove(path);
    qDebug() << "[canvas] deleted workflow:" << path;
}
