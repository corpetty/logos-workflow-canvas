#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

// QuickQanava headers
#include "QuickQanava.h"

class ModuleMethodNode;
class UtilityNode;
class ControlFlowNode;
class TransformNode;
class TriggerNode;

/**
 * @brief Extended qan::Graph for workflow semantics
 *
 * Adds methods for inserting typed workflow nodes, serializing
 * to/from the workflow JSON format, and managing port connections.
 */
class WorkflowGraph : public qan::Graph
{
    Q_OBJECT

public:
    explicit WorkflowGraph(QQuickItem* parent = nullptr);

    /**
     * @brief Insert a node from a node type definition
     * @param nodeTypeDef JSON definition from the registry
     * @return The created node
     */
    Q_INVOKABLE qan::Node* insertWorkflowNode(const QJsonObject& nodeTypeDef);

    /**
     * @brief Serialize the entire graph to workflow JSON
     */
    Q_INVOKABLE QString serializeToJson() const;

    /**
     * @brief Load a workflow from JSON, replacing the current graph
     */
    Q_INVOKABLE bool loadFromJson(const QString& json);

    /**
     * @brief Clear all nodes and edges
     */
    Q_INVOKABLE void clearGraph() noexcept override;

private:
    qan::Node* createModuleMethodNode(const QJsonObject& def);
    qan::Node* createUtilityNode(const QJsonObject& def);
    qan::Node* createControlFlowNode(const QJsonObject& def);
    qan::Node* createTransformNode(const QJsonObject& def);
    qan::Node* createTriggerNode(const QJsonObject& def);

    void configurePorts(qan::Node* node, const QJsonObject& def);

    int m_nodeCounter = 0;
};
