#include "TransformNode.h"
#include <QQmlComponent>
#include <QQmlEngine>

QQmlComponent* TransformNode::delegate(QQmlEngine& engine, QObject* parent)
{
    Q_UNUSED(parent)
    static std::unique_ptr<QQmlComponent> delegate;
    if (!delegate) {
        delegate = std::make_unique<QQmlComponent>(&engine, "qrc:/qml/delegates/TransformNodeDelegate.qml");
    }
    return delegate.get();
}

qan::NodeStyle* TransformNode::style(QObject* parent)
{
    auto* style = new qan::NodeStyle(parent);
    style->setBackColor(QColor("#0a2a2a"));
    style->setBorderColor(QColor("#26a69a"));
    style->setBorderWidth(2);
    style->setBackRadius(6);
    return style;
}
