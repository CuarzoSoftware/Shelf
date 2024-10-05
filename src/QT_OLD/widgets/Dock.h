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
    double maxItemSize() noexcept;

    std::list<QWidget*> items;

    bool isDockVisible { false };

    // Dimensions
    double dockScale { 0.75 };
    double n { 0.0 };
    QSizeF itemSize;
    QSizeF dockSize;
    double itemsSpacing;
    double dockBorderRadius;
    double dockMargin;
    double dotSize;
    double dotMarginTop;
    double dotMarginBottom;
    double tooltipDistY;
    bool isMouseOver { false };
    double maxHeight { 96 };
    double dockAndMargins{ 32 };

    DockWidget dockWidget { *this };

    QPropertyAnimation slideUpAnim { this, "offsetY", this };
    QPropertyAnimation slideDownAnim { this, "offsetY", this };

Q_SIGNALS:
    void offsetYChanged();
};

#endif // DOCK_H
