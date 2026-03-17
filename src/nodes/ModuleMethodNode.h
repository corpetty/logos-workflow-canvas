#pragma once

#include "QuickQanava.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QVariant>

/**
 * @brief Graph node representing a Logos module method call
 *
 * Dynamically configured from the registry's node type definition.
 * Ports are created based on the method's parameter and return types.
 */
class ModuleMethodNode : public qan::Node
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString moduleName READ moduleName WRITE setModuleName NOTIFY moduleNameChanged)
    Q_PROPERTY(QString methodName READ methodName WRITE setMethodName NOTIFY methodNameChanged)
    Q_PROPERTY(QString nodeTypeId READ nodeTypeId WRITE setNodeTypeId NOTIFY nodeTypeIdChanged)
    Q_PROPERTY(QString nodeColor READ nodeColor WRITE setNodeColor NOTIFY nodeColorChanged)
    Q_PROPERTY(bool isLive READ isLive WRITE setIsLive NOTIFY isLiveChanged)
    Q_PROPERTY(QString executionStatus READ executionStatus WRITE setExecutionStatus NOTIFY executionStatusChanged)
    Q_PROPERTY(QVariant executionResult READ executionResult WRITE setExecutionResult NOTIFY executionResultChanged)

public:
    explicit ModuleMethodNode(QObject* parent = nullptr) : qan::Node(parent) {}

    static QQmlComponent* delegate(QQmlEngine& engine, QObject* parent = nullptr);
    static qan::NodeStyle* style(QObject* parent = nullptr);

    QString moduleName() const { return m_moduleName; }
    void setModuleName(const QString& v) { if (m_moduleName != v) { m_moduleName = v; emit moduleNameChanged(); } }

    QString methodName() const { return m_methodName; }
    void setMethodName(const QString& v) { if (m_methodName != v) { m_methodName = v; emit methodNameChanged(); } }

    QString nodeTypeId() const { return m_nodeTypeId; }
    void setNodeTypeId(const QString& v) { if (m_nodeTypeId != v) { m_nodeTypeId = v; emit nodeTypeIdChanged(); } }

    QString nodeColor() const { return m_nodeColor; }
    void setNodeColor(const QString& v) { if (m_nodeColor != v) { m_nodeColor = v; emit nodeColorChanged(); } }

    bool isLive() const { return m_isLive; }
    void setIsLive(bool v) { if (m_isLive != v) { m_isLive = v; emit isLiveChanged(); } }

    QString executionStatus() const { return m_executionStatus; }
    void setExecutionStatus(const QString& v) { if (m_executionStatus != v) { m_executionStatus = v; emit executionStatusChanged(); } }

    QVariant executionResult() const { return m_executionResult; }
    void setExecutionResult(const QVariant& v) { m_executionResult = v; emit executionResultChanged(); }

signals:
    void moduleNameChanged();
    void methodNameChanged();
    void nodeTypeIdChanged();
    void nodeColorChanged();
    void isLiveChanged();
    void executionStatusChanged();
    void executionResultChanged();

private:
    QString m_moduleName;
    QString m_methodName;
    QString m_nodeTypeId;
    QString m_nodeColor = "#4a9eff";
    bool m_isLive = false;
    QString m_executionStatus = "idle";
    QVariant m_executionResult;
};
