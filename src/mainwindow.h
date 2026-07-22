#pragma once

#include "gitgraph.h"

#include <QHash>
#include <QMainWindow>

class GraphView;
class QGraphicsPathItem;
class QGraphicsScene;
class QLineEdit;
class QLabel;
class QComboBox;
class QDockWidget;

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(const QString& initialPath = {}, QWidget* parent = nullptr);

private:
    void chooseRepository();
    void loadRepository(const QString& path);
    void rebuildScene();
    void findNext();
    void zoomToFit();
    void zoomToWidth();
    void zoomToHeight();
    void setZoom(qreal factor);
    void toggleOverview(bool visible);
    void saveGraph();

    GraphView* m_view = nullptr;
    QGraphicsScene* m_scene = nullptr;
    QLineEdit* m_search = nullptr;
    QComboBox* m_zoomCombo = nullptr;
    QDockWidget* m_overviewDock = nullptr;
    QLabel* m_status = nullptr;
    GitGraphResult m_graph;
    QHash<QString, QGraphicsPathItem*> m_nodes;
    QVector<QGraphicsPathItem*> m_searchMatches;
    int m_searchIndex = -1;
    bool m_showBranchingsMerges = false;
    bool m_showAllTags = true;
    bool m_arrowsPointToMerges = false;
};
