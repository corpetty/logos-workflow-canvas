#pragma once

#include "QuickQanava.h"
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

/**
 * @brief Graph node for utility operations (String, Number, Boolean, JSON, Display, Template)
 */
class UtilityNode : public qan::Node
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString nodeTypeId READ nodeTypeId WRITE setNodeTypeId NOTIFY nodeTypeIdChanged)
    Q_PROPERTY(QString subtype READ subtype WRITE setSubtype NOTIFY subtypeChanged)
    Q_PROPERTY(QVariant propertyValue READ propertyValue NOTIFY propertyValueChanged)

public:
    explicit UtilityNode(QObject* parent = nullptr) : qan::Node(parent) {}

    static QQmlComponent* delegate(QQmlEngine& engine, QObject* parent = nullptr);
    static qan::NodeStyle* style(QObject* parent = nullptr);

    QString nodeTypeId() const { return m_nodeTypeId; }
    void setNodeTypeId(const QString& v) { if (m_nodeTypeId != v) { m_nodeTypeId = v; emit nodeTypeIdChanged(); } }

    QString subtype() const { return m_subtype; }
    void setSubtype(const QString& v) { if (m_subtype != v) { m_subtype = v; emit subtypeChanged(); } }

    QVariant propertyValue() const { return m_properties.value("value"); }

    Q_INVOKABLE void setPropertyValue(const QString& key, const QVariant& value) {
        m_properties[key] = value;
        emit propertyValueChanged();
    }

    Q_INVOKABLE QVariant getProperty(const QString& key) const {
        return m_properties.value(key);
    }

signals:
    void nodeTypeIdChanged();
    void subtypeChanged();
    void propertyValueChanged();

private:
    QString m_nodeTypeId;
    QString m_subtype;
    QVariantMap m_properties;
};
