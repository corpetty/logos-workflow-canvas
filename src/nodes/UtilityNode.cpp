#include "UtilityNode.h"
#include <QQmlComponent>
#include <QQmlEngine>

QQmlComponent* UtilityNode::delegate(QQmlEngine& engine, QObject* parent)
{
    Q_UNUSED(parent)
    static std::unique_ptr<QQmlComponent> delegate;
    if (!delegate) {
        delegate = std::make_unique<QQmlComponent>(&engine, "qrc:/qml/delegates/UtilityNodeDelegate.qml");
    }
    return delegate.get();
}

qan::NodeStyle* UtilityNode::style(QObject* parent)
{
    auto* style = new qan::NodeStyle(parent);
    style->setBackColor(QColor("#263238"));
    style->setBorderColor(QColor("#607d8b"));
    style->setBorderWidth(1);
    style->setBackRadius(4);
    return style;
}
