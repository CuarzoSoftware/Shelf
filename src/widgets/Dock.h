#ifndef DOCK_H
#define DOCK_H

#include <QOpenGLWidget>
#include <QPropertyAnimation>
#include <widgets/DockWidget.h>

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
    Q_OBJECT
    Q_PROPERTY(int offsetY READ offsetY WRITE setOffsetY NOTIFY offsetYChanged)
public:
    Dock(QScreen *screen) noexcept;
    ~Dock();

    void setOffsetY(int offset);
    int offsetY() const;

    void enterEvent(QEnterEvent *event) noexcept override;
    void leaveEvent(QEvent *) noexcept override;

    void paintEvent(QPaintEvent *event) override;

    void hideAllTooltips() noexcept;
    void setDockVisible(bool visible) noexcept;
    void resizeEvent(QResizeEvent *event) override;
    void updateDimensions() noexcept;
    int maxItemSize() const noexcept;

    bool isDockVisible { false };

    // Dimensions
    double dockScale { 1.0 };
    QSize itemSize;
    int itemsSpacing;
    int dockBorderRadius;
    int dotSize;
    int dotMarginTop;
    int dotMarginBottom;
    int tooltipDistY;
    QMargins dockMargins;
    QSize dockSize;
    bool isMouseOver { false };
    int maxHeight { 96 };
    int dockAndMargins{ 32 };

    QVBoxLayout layout { this };
    DockWidget dockWidget { *this };

    QPropertyAnimation slideUpAnim { this, "offsetY", this };
    QPropertyAnimation slideDownAnim { this, "offsetY", this };

Q_SIGNALS:
    void offsetYChanged();
};

#endif // DOCK_H
