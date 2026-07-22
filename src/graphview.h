#pragma once

#include <QGraphicsView>

class GraphView final : public QGraphicsView
{
public:
    explicit GraphView(QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;
};
