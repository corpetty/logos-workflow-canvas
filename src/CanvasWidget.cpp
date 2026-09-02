#include "CanvasWidget.h"
#include "WorkflowGraph.h"
#include "nodes/ModuleMethodNode.h"
#include "nodes/UtilityNode.h"
#include "nodes/ControlFlowNode.h"
#include "nodes/TransformNode.h"
#include "nodes/TriggerNode.h"
#include "logos_api.h"
#include "logos_api_client.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QCoreApplication>
#include <QtQml/qqml.h>
#include "QuickQanava.h"

#if !defined(_WIN32)
#  include <dlfcn.h>
#endif

namespace {

// The directory this plugin's own shared object was loaded from.
//
// The QuickQanava QML module ships INSIDE the plugin directory, so the import
// path is always "<my own dir>/qt-6/qml" — but where that directory IS depends
// on how the plugin was installed: the app bundle's plugins/, the user data
// directory, or a portable tree. Guessing at those locations meant the canvas
// silently rendered nothing whenever it was installed somewhere the list did
// not anticipate — which is what an app-bundle install did.
//
// Asking the loader removes the guess entirely.
QString pluginDirectory()
{
#if !defined(_WIN32)
    Dl_info info{};
    // Any symbol defined in this library identifies it; the address of a
    // function in this translation unit is the cheapest one to hand.
    if (dladdr(reinterpret_cast<const void*>(&pluginDirectory), &info) && info.dli_fname)
        return QFileInfo(QString::fromUtf8(info.dli_fname)).absolutePath();
#endif
    return QString();
}

} // namespace

CanvasWidget::CanvasWidget(LogosAPI* logosAPI, QWidget* parent)
    : QWidget(parent)
    , m_logosAPI(logosAPI)
{
    setupUI();
    connectToModules();
}

CanvasWidget::~CanvasWidget()
{
}

void CanvasWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setMinimumSize(800, 600);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // QuickQanava is installed as a shared QML module alongside this plugin.
    // The LGX installer places bundled files next to the plugin .so:
    //   <plugins>/workflow_canvas/workflow_canvas.so
    //   <plugins>/workflow_canvas/qt-6/qml/QuickQanava/qmldir
    //
    // Try both portable and non-portable (Nix dev) plugin paths, plus the
    // app's own lib directory.
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QStringList qmlSearchPaths;
    // First choice: beside this very .so. Exact for every install layout.
    const QString ownDir = pluginDirectory();
    if (!ownDir.isEmpty())
        qmlSearchPaths << ownDir + "/qt-6/qml";
    // Then the historical guesses, kept as a fallback.
    const QString userDir = qEnvironmentVariable("LOGOS_USER_DIR");
    if (!userDir.isEmpty())
        qmlSearchPaths << userDir + "/plugins/workflow_canvas/qt-6/qml";
    qmlSearchPaths
        << appDataDir + "Nix/plugins/workflow_canvas/qt-6/qml"    // Non-portable (Nix dev)
        << appDataDir + "/plugins/workflow_canvas/qt-6/qml"       // Portable
        << QCoreApplication::applicationDirPath() + "/../plugins/workflow_canvas/qt-6/qml"
        << QCoreApplication::applicationDirPath() + "/../lib/qt-6/qml";  // App output
    bool foundQanava = false;
    for (const QString& path : qmlSearchPaths) {
        if (QDir(path + "/QuickQanava").exists()) {
            m_quickWidget->engine()->addImportPath(path);
            qDebug() << "[canvas] Added QML import path:" << path;
            foundQanava = true;
            break;
        }
    }
    if (!foundQanava) {
        // Say so loudly. Without QuickQanava the view fails to compile and the
        // canvas comes up blank with nothing in the log explaining why.
        qWarning() << "[canvas] QuickQanava QML module not found; the graph "
                      "will not render. Looked in:" << qmlSearchPaths;
    }

    // Initialize QuickQanava: registers default styles and edge path components
    // as QML context properties (required for edge rendering).
    QuickQanava::initialize(m_quickWidget->engine());

    // Set default edge style to be visible on dark backgrounds
    auto* edgeStyle = qan::Edge::style();
    if (edgeStyle) {
        edgeStyle->setLineColor(QColor("#58a6ff"));
        edgeStyle->setLineWidth(2.0);
    }

    // Register canvas-specific QML types so they're visible to the QML engine.
    // (QML_ELEMENT in headers only works with qt_add_qml_module; this plugin
    // is a plain shared library, so we register manually.)
    qmlRegisterType<WorkflowGraph>("WorkflowCanvas", 1, 0, "WorkflowGraph");
    qmlRegisterType<ModuleMethodNode>("WorkflowCanvas", 1, 0, "ModuleMethodNode");
    qmlRegisterType<UtilityNode>("WorkflowCanvas", 1, 0, "UtilityNode");
    qmlRegisterType<ControlFlowNode>("WorkflowCanvas", 1, 0, "ControlFlowNode");
    qmlRegisterType<TransformNode>("WorkflowCanvas", 1, 0, "TransformNode");
    qmlRegisterType<TriggerNode>("WorkflowCanvas", 1, 0, "TriggerNode");

    // Expose this widget to QML so it can call our slots
    m_quickWidget->rootContext()->setContextProperty("canvasWidget", this);

    m_quickWidget->setSource(QUrl("qrc:/qml/WorkflowCanvas.qml"));

    if (m_quickWidget->status() == QQuickWidget::Error) {
        qWarning() << "[canvas] QML load errors:";
        for (const auto& err : m_quickWidget->errors()) {
            qWarning() << "[canvas]  " << err.toString();
        }
    }

    layout->addWidget(m_quickWidget);
    setLayout(layout);
}

void CanvasWidget::connectToModules()
{
    if (!m_logosAPI) {
        m_connectionStatus = "no_api";
        emit connectionStatusChanged();
        qDebug() << "[canvas] No LogosAPI — running in standalone mode";
        return;
    }

    // Query the registry for node types
    refreshNodeTypes();

    m_connectionStatus = "connected";
    emit connectionStatusChanged();
}

void CanvasWidget::refreshNodeTypes()
{
    if (!m_logosAPI) return;

    auto* registryClient = m_logosAPI->getClient("workflow_registry");
    if (!registryClient) {
        qWarning() << "[canvas] workflow_registry module not available";
        return;
    }

    QVariant result = registryClient->invokeRemoteMethod(
        QString("workflow_registry"), QString("getNodeTypeDefinitions"), QVariantList());

    QString json = result.toString();
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    m_nodeTypes = doc.array();

    qDebug() << "[canvas] Loaded" << m_nodeTypes.size() << "node types from registry";
    emit nodeTypesChanged();
}

void CanvasWidget::executeWorkflow(const QString& workflowJson)
{
    if (!m_logosAPI) {
        qWarning() << "[canvas] Cannot execute — no LogosAPI";
        return;
    }

    auto* engineClient = m_logosAPI->getClient("workflow_engine");
    if (!engineClient) {
        qWarning() << "[canvas] workflow_engine module not available";
        return;
    }

    qDebug() << "[canvas] Executing workflow, JSON:" << workflowJson;

    QVariant result = engineClient->invokeRemoteMethod(
        QString("workflow_engine"), QString("executeWorkflow"), QVariant(workflowJson));

    QString resultJson = result.toString();
    qDebug() << "[canvas] Execution result:" << resultJson.left(500);

    QJsonDocument doc = QJsonDocument::fromJson(resultJson.toUtf8());
    QJsonObject resultObj = doc.object();

    // Update the last execution result (visible to QML)
    m_lastExecutionResult = resultJson;
    emit lastExecutionResultChanged();

    // Paint per-node results onto the graph. The matching is by the id
    // WorkflowGraph::serializeToJson wrote, so the graph owns it — this used
    // to be an inline dynamic_cast loop over get_nodes() right here.
    if (auto* root = m_quickWidget->rootObject()) {
        if (auto* graphObj = root->findChild<WorkflowGraph*>("WorkflowGraph"))
            graphObj->applyNodeResults(resultJson);
    }

    // Add to history
    m_executionHistory.prepend(QJsonValue(resultObj));
    if (m_executionHistory.size() > 100) {
        m_executionHistory.removeLast();
    }
    emit executionHistoryChanged();

    emit executionCompleted(resultObj["executionId"].toString(), resultObj);
}

void CanvasWidget::clearLastExecutionResult()
{
    m_lastExecutionResult.clear();
    emit lastExecutionResultChanged();
}

void CanvasWidget::deployWorkflow(const QString& workflowId, const QString& workflowJson)
{
    if (!m_logosAPI) return;

    auto* schedulerClient = m_logosAPI->getClient("workflow_scheduler");
    if (!schedulerClient) {
        qWarning() << "[canvas] workflow_scheduler module not available";
        return;
    }

    schedulerClient->invokeRemoteMethod(
        QString("workflow_scheduler"), QString("deployWorkflow"),
        QVariant(workflowId), QVariant(workflowJson));

    qDebug() << "[canvas] Deployed workflow:" << workflowId;
}

// ── Persistence ──────────────────────────────────────────────────────

namespace {

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

QString CanvasWidget::workflowDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/workflows";
    QDir().mkpath(dir);
    return dir;
}

void CanvasWidget::saveWorkflow(const QString& name, const QString& workflowJson)
{
    const QString safe = sanitizeName(name);
    if (safe.isEmpty()) {
        qWarning() << "[canvas] refusing to save workflow under empty name:" << name;
        return;
    }
    QString path = workflowDir() + "/" + safe + ".json";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(workflowJson.toUtf8());
        qDebug() << "[canvas] Saved workflow:" << path;
    } else {
        qWarning() << "[canvas] Failed to save workflow:" << path;
    }
}

QString CanvasWidget::loadWorkflow(const QString& name)
{
    const QString safe = sanitizeName(name);
    if (safe.isEmpty())
        return QString();
    QString path = workflowDir() + "/" + safe + ".json";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    qWarning() << "[canvas] Failed to load workflow:" << path;
    return QString();
}

QStringList CanvasWidget::listSavedWorkflows()
{
    QDir dir(workflowDir());
    QStringList filters;
    filters << "*.json";
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Time);

    QStringList names;
    for (const auto& file : files) {
        names.append(file.chopped(5)); // Remove .json
    }
    return names;
}

void CanvasWidget::deleteWorkflow(const QString& name)
{
    const QString safe = sanitizeName(name);
    if (safe.isEmpty())
        return;
    QString path = workflowDir() + "/" + safe + ".json";
    QFile::remove(path);
    qDebug() << "[canvas] Deleted workflow:" << path;
}
