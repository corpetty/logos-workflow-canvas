#include "ControlFlowNode.h"
#include <QQmlComponent>
#include <QQmlEngine>

QQmlComponent* ControlFlowNode::delegate(QQmlEngine& engine, QObject* parent)
{
    Q_UNUSED(parent)
    static std::unique_ptr<QQmlComponent> delegate;
    if (!delegate) {
        delegate = std::make_unique<QQmlComponent>(&engine, "qrc:/qml/delegates/ControlFlowDelegate.qml");
    }
    return delegate.get();
}

qan::NodeStyle* ControlFlowNode::style(QObject* parent)
{
    auto* style = new qan::NodeStyle(parent);
    style->setBackColor(QColor("#1a0a2e"));
    style->setBorderColor(QColor("#7e57c2"));
    style->setBorderWidth(2);
    style->setBackRadius(8);
    return style;
}
