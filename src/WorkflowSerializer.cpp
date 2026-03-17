#include "WorkflowSerializer.h"
#include "WorkflowGraph.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QDebug>

QJsonObject WorkflowSerializer::serialize(const WorkflowGraph& graph)
{
    // Delegate to WorkflowGraph::serializeToJson() which has full access
    // to the graph internals. Parse it back to a QJsonObject.
    QString json = graph.serializeToJson();
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject workflow = doc.object();

    // Augment with metadata
    workflow["created"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!workflow.contains("name") || workflow["name"].toString().isEmpty()) {
        workflow["name"] = graph.objectName().isEmpty()
                              ? "Untitled Workflow"
                              : graph.objectName();
    }

    return workflow;
}

bool WorkflowSerializer::deserialize(const QJsonObject& json, WorkflowGraph& graph)
{
    if (!json.contains("nodes") || !json.contains("edges")) {
        qWarning() << "[serializer] Invalid workflow JSON — missing nodes or edges";
        return false;
    }

    // Delegate to WorkflowGraph::loadFromJson() which handles node creation,
    // port configuration, position restoration, and edge binding.
    QJsonDocument doc(json);
    return graph.loadFromJson(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}
