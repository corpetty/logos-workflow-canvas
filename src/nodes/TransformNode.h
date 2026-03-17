#pragma once

#include "QuickQanava.h"
#include <QObject>
#include <QString>

/**
 * @brief Graph node for data transform operations (Map, Filter, Pick, Merge, Expression, HTTP)
 */
class TransformNode : public qan::Node
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString nodeTypeId READ nodeTypeId WRITE setNodeTypeId NOTIFY nodeTypeIdChanged)
    Q_PROPERTY(QString subtype READ subtype WRITE setSubtype NOTIFY subtypeChanged)
    Q_PROPERTY(QString executionStatus READ executionStatus WRITE setExecutionStatus NOTIFY executionStatusChanged)

public:
    explicit TransformNode(QObject* parent = nullptr) : qan::Node(parent) {}

    static QQmlComponent* delegate(QQmlEngine& engine, QObject* parent = nullptr);
    static qan::NodeStyle* style(QObject* parent = nullptr);

    QString nodeTypeId() const { return m_nodeTypeId; }
    void setNodeTypeId(const QString& v) { if (m_nodeTypeId != v) { m_nodeTypeId = v; emit nodeTypeIdChanged(); } }

    QString subtype() const { return m_subtype; }
    void setSubtype(const QString& v) { if (m_subtype != v) { m_subtype = v; emit subtypeChanged(); } }

    QString executionStatus() const { return m_executionStatus; }
    void setExecutionStatus(const QString& v) { if (m_executionStatus != v) { m_executionStatus = v; emit executionStatusChanged(); } }

signals:
    void nodeTypeIdChanged();
    void subtypeChanged();
    void executionStatusChanged();

private:
    QString m_nodeTypeId;
    QString m_subtype;
    QString m_executionStatus = "idle";
};
