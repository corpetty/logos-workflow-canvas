#include "TriggerNode.h"
#include <QQmlComponent>
#include <QQmlEngine>

QQmlComponent* TriggerNode::delegate(QQmlEngine& engine, QObject* parent)
{
    Q_UNUSED(parent)
    static std::unique_ptr<QQmlComponent> delegate;
    if (!delegate) {
        delegate = std::make_unique<QQmlComponent>(&engine, "qrc:/qml/delegates/TriggerNodeDelegate.qml");
    }
    return delegate.get();
}

qan::NodeStyle* TriggerNode::style(QObject* parent)
{
    auto* style = new qan::NodeStyle(parent);
    style->setBackColor(QColor("#2a1a00"));
    style->setBorderColor(QColor("#ffa726"));
    style->setBorderWidth(2);
    style->setBackRadius(6);
    return style;
}
