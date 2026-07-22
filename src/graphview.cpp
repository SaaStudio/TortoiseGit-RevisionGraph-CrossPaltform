#include "graphview.h"

#include <QWheelEvent>

GraphView::GraphView(QWidget* parent) : QGraphicsView(parent)
{
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setBackgroundBrush(QColor(245, 246, 248));
}

void GraphView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const qreal factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        if ((factor > 1.0 && transform().m11() < 4.0) || (factor < 1.0 && transform().m11() > 0.08))
            scale(factor, factor);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}
