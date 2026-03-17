#pragma once

#include <IComponent.h>
#include <QObject>

/**
 * @brief IComponent factory for the Workflow Canvas
 *
 * Loaded by logos-app as a UI plugin. Creates the main
 * CanvasWidget which hosts the QuickQanava graph editor.
 */
class CanvasComponent : public QObject, public IComponent {
    Q_OBJECT
    Q_INTERFACES(IComponent)
    Q_PLUGIN_METADATA(IID IComponent_iid FILE "metadata.json")

public:
    QWidget* createWidget(LogosAPI* logosAPI = nullptr) override;
    void destroyWidget(QWidget* widget) override;
};
