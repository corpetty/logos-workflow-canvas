#include "ModuleMethodNode.h"
#include <QQmlComponent>
#include <QQmlEngine>

QQmlComponent* ModuleMethodNode::delegate(QQmlEngine& engine, QObject* parent)
{
    Q_UNUSED(parent)
    static std::unique_ptr<QQmlComponent> delegate;
    if (!delegate) {
        delegate = std::make_unique<QQmlComponent>(&engine, "qrc:/qml/delegates/ModuleNodeDelegate.qml");
    }
    return delegate.get();
}

qan::NodeStyle* ModuleMethodNode::style(QObject* parent)
{
    auto* style = new qan::NodeStyle(parent);
    style->setBackColor(QColor("#1a1a2e"));
    style->setBorderColor(QColor("#4a9eff"));
    style->setBorderWidth(2);
    style->setBackRadius(6);
    return style;
}
