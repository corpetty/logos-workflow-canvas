#include "CanvasWidget.h"
#include "WorkflowGraph.h"
#include "logos_api.h"
#include "logos_api_client.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QCoreApplication>
#include <QtQml/qqml.h>

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
    QStringList qmlSearchPaths = {
        appDataDir + "Nix/plugins/workflow_canvas/qt-6/qml",   // Non-portable (Nix dev)
        appDataDir + "/plugins/workflow_canvas/qt-6/qml",       // Portable
        QCoreApplication::applicationDirPath() + "/../lib/qt-6/qml", // App output
    };
    for (const QString& path : qmlSearchPaths) {
        if (QDir(path + "/QuickQanava").exists()) {
            m_quickWidget->engine()->addImportPath(path);
            qDebug() << "[canvas] Added QML import path:" << path;
            break;
        }
    }

    // Register canvas-specific QML types
    qmlRegisterType<WorkflowGraph>("WorkflowCanvas", 1, 0, "WorkflowGraph");

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

    QVariant result = engineClient->invokeRemoteMethod(
        QString("workflow_engine"), QString("executeWorkflow"), QVariant(workflowJson));

    QString resultJson = result.toString();
    QJsonDocument doc = QJsonDocument::fromJson(resultJson.toUtf8());
    QJsonObject resultObj = doc.object();

    // Add to history
    m_executionHistory.prepend(QJsonValue(resultObj));
    if (m_executionHistory.size() > 100) {
        m_executionHistory.removeLast();
    }
    emit executionHistoryChanged();

    emit executionCompleted(resultObj["executionId"].toString(), resultObj);
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

QString CanvasWidget::workflowDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/workflows";
    QDir().mkpath(dir);
    return dir;
}

void CanvasWidget::saveWorkflow(const QString& name, const QString& workflowJson)
{
    QString path = workflowDir() + "/" + name + ".json";
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
    QString path = workflowDir() + "/" + name + ".json";
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
    QString path = workflowDir() + "/" + name + ".json";
    QFile::remove(path);
    qDebug() << "[canvas] Deleted workflow:" << path;
}
