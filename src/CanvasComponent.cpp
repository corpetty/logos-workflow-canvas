#include "CanvasComponent.h"
#include "CanvasWidget.h"

#include <QQuickStyle>

QWidget* CanvasComponent::createWidget(LogosAPI* logosAPI)
{
    QQuickStyle::setStyle("Basic");
    return new CanvasWidget(logosAPI);
}

void CanvasComponent::destroyWidget(QWidget* widget)
{
    delete widget;
}
