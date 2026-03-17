#pragma once

#include <QJsonObject>
#include <QString>

class WorkflowGraph;

/**
 * @brief Serializes/deserializes QuickQanava graph to/from workflow JSON format
 */
class WorkflowSerializer
{
public:
    /**
     * @brief Serialize a WorkflowGraph to the v2 JSON format
     */
    static QJsonObject serialize(const WorkflowGraph& graph);

    /**
     * @brief Deserialize v2 JSON into a WorkflowGraph
     * @return true on success
     */
    static bool deserialize(const QJsonObject& json, WorkflowGraph& graph);
};
