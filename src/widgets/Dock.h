#ifndef DOCK_H
#define DOCK_H

#include <QHBoxLayout>
#include <QWidget>

class DockElevator;
class DockWidget;

/**
 * @brief An output-specific dock
 *
 * Basically a transparent LayerShell surface anchored to the Left + Bottom + Right.
 * It has a small height for listening to pointer enter events.
 *
 */
class Dock final : public QWidget
{
public:
    Dock(QScreen *screen) noexcept;
    ~Dock();
    void enterEvent(QEnterEvent *event) noexcept override;

    DockElevator *dockElevator;

    void setDockVisible(bool visible) noexcept;
    bool isDockVisible { false };

    void resizeEvent(QResizeEvent *event) override;
    void updateDimensions() noexcept;
    double dockScale { 1.0 };
    QSize itemSize;
    int itemsSpacing;
    int dockBorderRadius;
    QMargins dockMargins;
    QSize dockSize;
};

#endif // DOCK_H
