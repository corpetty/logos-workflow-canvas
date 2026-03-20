#include "WorkflowGraph.h"
#include "nodes/ModuleMethodNode.h"
#include "nodes/UtilityNode.h"
#include "nodes/ControlFlowNode.h"
#include "nodes/TransformNode.h"
#include "nodes/TriggerNode.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>

WorkflowGraph::WorkflowGraph(QQuickItem* parent)
    : qan::Graph(parent)
{
}

qan::Node* WorkflowGraph::insertWorkflowNode(const QJsonObject& nodeTypeDef)
{
    QString category = nodeTypeDef["category"].toString();

    qan::Node* node = nullptr;

    if (category == "module_method")  node = createModuleMethodNode(nodeTypeDef);
    else if (category == "utility")   node = createUtilityNode(nodeTypeDef);
    else if (category == "control_flow") node = createControlFlowNode(nodeTypeDef);
    else if (category == "transform") node = createTransformNode(nodeTypeDef);
    else if (category == "trigger")   node = createTriggerNode(nodeTypeDef);
    else {
        qWarning() << "[workflow_graph] Unknown category:" << category;
        return nullptr;
    }

    if (node) {
        configurePorts(node, nodeTypeDef);
    }

    return node;
}

qan::Node* WorkflowGraph::createModuleMethodNode(const QJsonObject& def)
{
    auto* node = insertNode<ModuleMethodNode>(nullptr);
    if (node) {
        node->setLabel(def["methodDisplayName"].toString());
        node->setModuleName(def["module"].toString());
        node->setMethodName(def["method"].toString());
        node->setNodeTypeId(def["nodeTypeId"].toString());
        node->setNodeColor(def["color"].toString("#4a9eff"));
        node->setIsLive(def["isLive"].toBool(false));
    }
    return node;
}

qan::Node* WorkflowGraph::createUtilityNode(const QJsonObject& def)
{
    auto* node = insertNode<UtilityNode>(nullptr);
    if (node) {
        node->setLabel(def["methodDisplayName"].toString());
        node->setNodeTypeId(def["nodeTypeId"].toString());
        node->setSubtype(def["nodeTypeId"].toString().section("/", 1));

        QJsonObject defaults = def["defaultProperties"].toObject();
        for (auto it = defaults.begin(); it != defaults.end(); ++it) {
            node->setPropertyValue(it.key(), it.value().toVariant());
        }
    }
    return node;
}

qan::Node* WorkflowGraph::createControlFlowNode(const QJsonObject& def)
{
    auto* node = insertNode<ControlFlowNode>(nullptr);
    if (node) {
        node->setLabel(def["methodDisplayName"].toString());
        node->setNodeTypeId(def["nodeTypeId"].toString());
        node->setSubtype(def["nodeTypeId"].toString().section("/", 1));
        node->setIsErrorCatch(def["isErrorCatch"].toBool(false));
    }
    return node;
}

qan::Node* WorkflowGraph::createTransformNode(const QJsonObject& def)
{
    auto* node = insertNode<TransformNode>(nullptr);
    if (node) {
        node->setLabel(def["methodDisplayName"].toString());
        node->setNodeTypeId(def["nodeTypeId"].toString());
        node->setSubtype(def["nodeTypeId"].toString().section("/", 1));
    }
    return node;
}

qan::Node* WorkflowGraph::createTriggerNode(const QJsonObject& def)
{
    auto* node = insertNode<TriggerNode>(nullptr);
    if (node) {
        node->setLabel(def["methodDisplayName"].toString());
        node->setNodeTypeId(def["nodeTypeId"].toString());
        node->setSubtype(def["nodeTypeId"].toString().section("/", 1));
    }
    return node;
}

void WorkflowGraph::configurePorts(qan::Node* node, const QJsonObject& def)
{
    QJsonObject ports = def["ports"].toObject();

    for (const auto& portVal : ports["inputs"].toArray()) {
        QJsonObject portDef = portVal.toObject();
        insertPort(node,
                   qan::NodeItem::Dock::Left,
                   qan::PortItem::Type::In,
                   portDef["label"].toString(),
                   portDef["id"].toString());
    }

    for (const auto& portVal : ports["outputs"].toArray()) {
        QJsonObject portDef = portVal.toObject();
        insertPort(node,
                   qan::NodeItem::Dock::Right,
                   qan::PortItem::Type::Out,
                   portDef["label"].toString(),
                   portDef["id"].toString());
    }
}

QString WorkflowGraph::serializeToJson() const
{
    QJsonObject workflow;
    workflow["name"] = "Untitled Workflow";
    workflow["version"] = "2.0";

    // Serialize nodes — iterate via gtpo's begin()/end() on the graph
    QJsonArray nodesArr;
    const auto& nodes = get_nodes();
    for (const auto* node : nodes) {
        if (!node) continue;

        QJsonObject nodeObj;
        nodeObj["id"] = QString::number(reinterpret_cast<quintptr>(node));

        // Get position from the NodeItem (QQuickItem subclass)
        auto* nodeItem = const_cast<qan::Node*>(node)->getItem();
        if (nodeItem) {
            QJsonObject pos;
            pos["x"] = nodeItem->x();
            pos["y"] = nodeItem->y();
            nodeObj["position"] = pos;
        }

        // Serialize by dynamic type
        if (auto* mmNode = dynamic_cast<const ModuleMethodNode*>(node)) {
            nodeObj["type"] = "module_method";
            nodeObj["nodeTypeId"] = mmNode->nodeTypeId();
            nodeObj["module"] = mmNode->moduleName();
            nodeObj["method"] = mmNode->methodName();
            nodeObj["color"] = mmNode->nodeColor();
            nodeObj["isLive"] = mmNode->isLive();
        } else if (auto* uNode = dynamic_cast<const UtilityNode*>(node)) {
            nodeObj["type"] = "utility";
            nodeObj["nodeTypeId"] = uNode->nodeTypeId();
            nodeObj["subtype"] = uNode->subtype();
            QJsonObject props;
            QVariant val = uNode->getProperty("value");
            qDebug() << "[canvas] Serialize UtilityNode" << uNode->subtype()
                     << "value valid:" << val.isValid() << "value:" << val;
            if (val.isValid()) props["value"] = QJsonValue::fromVariant(val);
            QVariant tmpl = uNode->getProperty("template");
            if (tmpl.isValid()) props["template"] = tmpl.toString();
            nodeObj["properties"] = props;
        } else if (auto* cfNode = dynamic_cast<const ControlFlowNode*>(node)) {
            nodeObj["type"] = "control_flow";
            nodeObj["nodeTypeId"] = cfNode->nodeTypeId();
            nodeObj["subtype"] = cfNode->subtype();
            nodeObj["isErrorCatch"] = cfNode->isErrorCatch();
        } else if (auto* tNode = dynamic_cast<const TransformNode*>(node)) {
            nodeObj["type"] = "transform";
            nodeObj["nodeTypeId"] = tNode->nodeTypeId();
            nodeObj["subtype"] = tNode->subtype();
        } else if (auto* trNode = dynamic_cast<const TriggerNode*>(node)) {
            nodeObj["type"] = "trigger";
            nodeObj["nodeTypeId"] = trNode->nodeTypeId();
            nodeObj["subtype"] = trNode->subtype();
        } else {
            nodeObj["type"] = "unknown";
        }

        nodeObj["label"] = node->getLabel();

        // Serialize port definitions — iterate all ports, filter by type
        QJsonObject portsObj;
        QJsonArray inputPorts, outputPorts;
        if (nodeItem) {
            const auto& allPorts = nodeItem->getPorts();
            for (const auto* item : allPorts) {
                auto* port = qobject_cast<const qan::PortItem*>(item);
                if (!port) continue;
                QJsonObject portObj;
                portObj["id"] = port->getId();
                portObj["label"] = port->getLabel();
                if (port->getType() == qan::PortItem::Type::In ||
                    port->getType() == qan::PortItem::Type::InOut) {
                    inputPorts.append(portObj);
                }
                if (port->getType() == qan::PortItem::Type::Out ||
                    port->getType() == qan::PortItem::Type::InOut) {
                    outputPorts.append(portObj);
                }
            }
        }
        portsObj["inputs"] = inputPorts;
        portsObj["outputs"] = outputPorts;
        nodeObj["ports"] = portsObj;

        nodesArr.append(nodeObj);
    }
    workflow["nodes"] = nodesArr;

    // Serialize edges — iterate nodes and collect outgoing edges
    QJsonArray edgesArr;
    int edgeCounter = 0;
    for (const auto* node : nodes) {
        if (!node) continue;
        auto adjacentEdges = const_cast<qan::Node*>(node)->collectAdjacentEdges();
        for (auto* edge : adjacentEdges) {
            if (!edge) continue;
            auto* src = edge->getSource();
            if (src != node) continue;  // Only serialize from source side

            QJsonObject edgeObj;
            edgeObj["id"] = QString("edge_%1").arg(++edgeCounter);
            edgeObj["sourceNode"] = QString::number(reinterpret_cast<quintptr>(src));
            edgeObj["targetNode"] = QString::number(
                reinterpret_cast<quintptr>(edge->getDestination()));

            // Get port binding from the EdgeItem's source/destination items.
            // When an edge is bound to a port via bindEdge(), the EdgeItem's
            // sourceItem/destinationItem are set to the PortItem instances.
            auto* edgeItem = edge->getItem();
            QString srcPortId, dstPortId;
            if (edgeItem) {
                auto* srcPort = qobject_cast<qan::PortItem*>(edgeItem->getSourceItem());
                auto* dstPort = qobject_cast<qan::PortItem*>(edgeItem->getDestinationItem());
                if (srcPort) srcPortId = srcPort->getId();
                if (dstPort) dstPortId = dstPort->getId();
            }
            edgeObj["sourcePort"] = srcPortId;
            edgeObj["targetPort"] = dstPortId;

            edgesArr.append(edgeObj);
        }
    }
    workflow["edges"] = edgesArr;

    return QString::fromUtf8(
        QJsonDocument(workflow).toJson(QJsonDocument::Indented));
}

bool WorkflowGraph::loadFromJson(const QString& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull()) return false;

    clearGraph();

    QJsonObject workflow = doc.object();
    QMap<QString, qan::Node*> nodeMap;

    // Recreate nodes
    QJsonArray jsonNodes = workflow["nodes"].toArray();
    for (const auto& nodeVal : jsonNodes) {
        QJsonObject nodeObj = nodeVal.toObject();
        QString oldId = nodeObj["id"].toString();

        QJsonObject def;
        def["category"] = nodeObj["type"].toString();
        def["nodeTypeId"] = nodeObj["nodeTypeId"].toString();
        def["module"] = nodeObj["module"].toString();
        def["method"] = nodeObj["method"].toString();
        def["methodDisplayName"] = nodeObj["label"].toString();
        def["subtype"] = nodeObj["subtype"].toString();
        def["color"] = nodeObj["color"].toString("#4a9eff");
        def["isLive"] = nodeObj["isLive"].toBool(false);
        def["isErrorCatch"] = nodeObj["isErrorCatch"].toBool(false);
        def["ports"] = nodeObj["ports"].toObject();

        if (nodeObj.contains("properties")) {
            def["defaultProperties"] = nodeObj["properties"].toObject();
        }

        qan::Node* node = insertWorkflowNode(def);
        if (node) {
            nodeMap[oldId] = node;

            // Restore position
            QJsonObject pos = nodeObj["position"].toObject();
            auto* nodeItem = node->getItem();
            if (nodeItem) {
                nodeItem->setX(pos["x"].toDouble());
                nodeItem->setY(pos["y"].toDouble());
            }
        }
    }

    // Recreate edges by binding ports
    QJsonArray jsonEdges = workflow["edges"].toArray();
    for (const auto& edgeVal : jsonEdges) {
        QJsonObject edgeObj = edgeVal.toObject();
        QString srcNodeId = edgeObj["sourceNode"].toString();
        QString dstNodeId = edgeObj["targetNode"].toString();
        QString srcPortId = edgeObj["sourcePort"].toString();
        QString dstPortId = edgeObj["targetPort"].toString();

        auto* srcNode = nodeMap.value(srcNodeId, nullptr);
        auto* dstNode = nodeMap.value(dstNodeId, nullptr);
        if (!srcNode || !dstNode) continue;

        // Find port items by ID using NodeItem::findPort()
        auto* srcNodeItem = srcNode->getItem();
        auto* dstNodeItem = dstNode->getItem();
        if (!srcNodeItem || !dstNodeItem) continue;

        qan::PortItem* srcPort = srcPortId.isEmpty() ? nullptr : srcNodeItem->findPort(srcPortId);
        qan::PortItem* dstPort = dstPortId.isEmpty() ? nullptr : dstNodeItem->findPort(dstPortId);

        if (srcPort && dstPort) {
            auto* edge = insertEdge(srcNode, dstNode);
            if (edge) {
                bindEdge(edge, srcPort, dstPort);
            }
        }
    }

    return true;
}

void WorkflowGraph::clearGraph() noexcept
{
    clearSelection();
    // Collect all nodes into a list then remove them (edges auto-removed)
    QList<qan::Node*> toRemove;
    for (auto* node : get_nodes()) {
        toRemove.append(node);
    }
    for (auto* node : toRemove) {
        removeNode(node);
    }
    m_nodeCounter = 0;
}
