#include "mainwindow.h"
#include "graphview.h"

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/layered/FastHierarchyLayout.h>
#include <ogdf/layered/MedianHeuristic.h>
#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/layered/SugiyamaLayout.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontMetricsF>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QInputDialog>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainterPath>
#include <QPainter>
#include <QPixmap>
#include <QBitmap>
#include <QScrollBar>
#include <QStatusBar>
#include <QToolBar>
#include <QStyle>
#include <QSignalBlocker>

#include <cmath>
#include <memory>

namespace {
constexpr qreal LeftRightMargin = 20.0;
constexpr qreal TopBottomMargin = 5.0;
constexpr int ShortHashLength = 8;

enum class RefType { Current, Local, Remote, Tag, Stash, Other };

QString shortRef(QString ref)
{
    ref.remove(QStringLiteral("^{}"));
    if (ref.startsWith(QStringLiteral("refs/heads/"))) return ref.mid(11);
    if (ref.startsWith(QStringLiteral("refs/remotes/"))) return ref.mid(13);
    if (ref.startsWith(QStringLiteral("refs/tags/"))) return ref.mid(10);
    if (ref == QStringLiteral("refs/stash")) return QStringLiteral("stash");
    if (ref.startsWith(QStringLiteral("refs/"))) return ref.mid(5);
    return ref;
}

RefType refType(const QString& ref, const QString& currentBranch)
{
    if (ref == QStringLiteral("refs/heads/") + currentBranch) return RefType::Current;
    if (ref.startsWith(QStringLiteral("refs/heads/"))) return RefType::Local;
    if (ref.startsWith(QStringLiteral("refs/remotes/"))) return RefType::Remote;
    if (ref.startsWith(QStringLiteral("refs/tags/"))) return RefType::Tag;
    if (ref == QStringLiteral("refs/stash")) return RefType::Stash;
    return RefType::Other;
}

QColor refColor(RefType type)
{
    switch (type) {
    case RefType::Current: return QColor(200, 0, 0);
    case RefType::Local: return QColor(0, 195, 0);
    case RefType::Remote: return QColor(255, 221, 170);
    case RefType::Tag: return QColor(255, 255, 0);
    case RefType::Stash: return QColor(128, 128, 255);
    case RefType::Other: return QColor(224, 224, 224);
    }
    return Qt::white;
}

QPointF boundaryPoint(const QRectF& rect, const QPointF& center, const QPointF& toward)
{
    const qreal dx = toward.x() - center.x();
    const qreal dy = toward.y() - center.y();
    if (qFuzzyIsNull(dx) && qFuzzyIsNull(dy)) return center;
    const qreal tx = qFuzzyIsNull(dx) ? 1e100 : (rect.width() / 2.0) / std::abs(dx);
    const qreal ty = qFuzzyIsNull(dy) ? 1e100 : (rect.height() / 2.0) / std::abs(dy);
    const qreal t = qMin(tx, ty);
    return center + (toward - center) * t;
}

QIcon toolbarIcon(int index)
{
    static const QPixmap strip(QStringLiteral(":/revgraphbar.png"));
    return QIcon(strip.copy(index * 20, 0, 20, 20));
}
}

MainWindow::MainWindow(const QString& initialPath, QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("TortoiseGit Revision Graph Cross"));
    resize(1100, 760);
    m_scene = new QGraphicsScene(this);
    m_view = new GraphView(this);
    m_view->setScene(m_scene);
    setCentralWidget(m_view);

    auto* toolbar = addToolBar(QStringLiteral("Граф"));
    toolbar->setMovable(false);

    // Порядок строго соответствует IDR_REVGRAPHBAR оригинального модуля.
    auto* zoomInAction = toolbar->addAction(toolbarIcon(0), QStringLiteral("Увеличить"));
    auto* zoomOutAction = toolbar->addAction(toolbarIcon(1), QStringLiteral("Уменьшить"));
    auto* zoom100Action = toolbar->addAction(toolbarIcon(2), QStringLiteral("Масштаб 100%"));
    auto* fitHeightAction = toolbar->addAction(toolbarIcon(3), QStringLiteral("Вместить по высоте"));
    auto* fitWidthAction = toolbar->addAction(toolbarIcon(4), QStringLiteral("Вместить по ширине"));
    auto* fitAction = toolbar->addAction(toolbarIcon(5), QStringLiteral("Вместить граф"));
    zoomInAction->setToolTip(QStringLiteral("Увеличить"));
    zoomOutAction->setToolTip(QStringLiteral("Уменьшить"));
    zoom100Action->setToolTip(QStringLiteral("Масштаб 100%"));
    fitHeightAction->setToolTip(QStringLiteral("Вместить по высоте"));
    fitWidthAction->setToolTip(QStringLiteral("Вместить по ширине"));
    fitAction->setToolTip(QStringLiteral("Вместить граф"));
    toolbar->addSeparator();
    m_zoomCombo = new QComboBox(this);
    m_zoomCombo->setEditable(true);
    m_zoomCombo->setFixedWidth(72);
    m_zoomCombo->addItems({QStringLiteral("5%"), QStringLiteral("10%"), QStringLiteral("20%"), QStringLiteral("40%"),
                           QStringLiteral("50%"), QStringLiteral("75%"), QStringLiteral("100%"), QStringLiteral("200%")});
    m_zoomCombo->setCurrentText(QStringLiteral("100%"));
    toolbar->addWidget(m_zoomCombo);
    toolbar->addSeparator();
    auto* filterAction = toolbar->addAction(toolbarIcon(6), QStringLiteral("Фильтр"));
    filterAction->setToolTip(QStringLiteral("Фильтр"));
    toolbar->addSeparator();
    auto* overviewAction = toolbar->addAction(toolbarIcon(7), QStringLiteral("Показать обзор"));
    overviewAction->setToolTip(QStringLiteral("Обзор графа"));
    overviewAction->setCheckable(true);
    toolbar->addSeparator();
    auto* findAction = toolbar->addAction(toolbarIcon(8), QStringLiteral("Найти"));
    findAction->setToolTip(QStringLiteral("Найти"));

    m_search = new QLineEdit(this);
    m_search->hide();
    m_status = new QLabel(this);
    statusBar()->addPermanentWidget(m_status);

    connect(zoomInAction, &QAction::triggered, this, [this] { setZoom(qMin<qreal>(2.0, m_view->transform().m11() / 0.9)); });
    connect(zoomOutAction, &QAction::triggered, this, [this] { setZoom(qMax<qreal>(0.01, m_view->transform().m11() * 0.9)); });
    connect(zoom100Action, &QAction::triggered, this, [this] { setZoom(1.0); });
    connect(fitHeightAction, &QAction::triggered, this, [this] { zoomToHeight(); });
    connect(fitWidthAction, &QAction::triggered, this, [this] { zoomToWidth(); });
    connect(fitAction, &QAction::triggered, this, [this] { zoomToFit(); });
    connect(m_zoomCombo, &QComboBox::currentTextChanged, this, [this](QString text) {
        text.remove(QLatin1Char('%'));
        bool ok = false;
        const qreal value = text.toDouble(&ok);
        if (ok && value > 0) setZoom(qBound<qreal>(0.01, value / 100.0, 2.0));
    });
    connect(filterAction, &QAction::triggered, this, [this] {
        QMessageBox::information(this, QStringLiteral("Фильтр"), QStringLiteral("Сейчас показаны все refs (--all)."));
    });
    connect(overviewAction, &QAction::toggled, this, [this](bool checked) { toggleOverview(checked); });
    connect(findAction, &QAction::triggered, this, [this] { findNext(); });

    auto* fileMenu = menuBar()->addMenu(QStringLiteral("Файл"));
    auto* saveAction = fileMenu->addAction(QStringLiteral("Сохранить граф как…"));
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction(QStringLiteral("Выход"));
    connect(saveAction, &QAction::triggered, this, [this] { saveGraph(); });
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto* viewMenu = menuBar()->addMenu(QStringLiteral("Вид"));
    auto* menuZoomIn = viewMenu->addAction(QStringLiteral("Увеличить\tCtrl++"));
    auto* menuZoomOut = viewMenu->addAction(QStringLiteral("Уменьшить\tCtrl+-"));
    auto* menuZoom100 = viewMenu->addAction(QStringLiteral("Масштаб 100%"));
    auto* menuFitHeight = viewMenu->addAction(QStringLiteral("Вместить по высоте"));
    auto* menuFitWidth = viewMenu->addAction(QStringLiteral("Вместить по ширине"));
    auto* menuFit = viewMenu->addAction(QStringLiteral("Вместить граф"));
    connect(menuZoomIn, &QAction::triggered, zoomInAction, &QAction::trigger);
    connect(menuZoomOut, &QAction::triggered, zoomOutAction, &QAction::trigger);
    connect(menuZoom100, &QAction::triggered, zoom100Action, &QAction::trigger);
    connect(menuFitHeight, &QAction::triggered, fitHeightAction, &QAction::trigger);
    connect(menuFitWidth, &QAction::triggered, fitWidthAction, &QAction::trigger);
    connect(menuFit, &QAction::triggered, fitAction, &QAction::trigger);
    viewMenu->addSeparator();
    auto* filterMenuAction = viewMenu->addAction(QStringLiteral("Фильтр"));
    filterMenuAction->setCheckable(true);
    connect(filterMenuAction, &QAction::triggered, filterAction, &QAction::trigger);
    viewMenu->addSeparator();
    auto* overviewMenuAction = viewMenu->addAction(QStringLiteral("Показать обзор"));
    overviewMenuAction->setCheckable(true);
    connect(overviewMenuAction, &QAction::toggled, overviewAction, &QAction::setChecked);
    auto* branchingsAction = viewMenu->addAction(QStringLiteral("Показывать точки ветвления и слияния"));
    branchingsAction->setCheckable(true);
    auto* tagsAction = viewMenu->addAction(QStringLiteral("Показывать все теги"));
    tagsAction->setCheckable(true);
    tagsAction->setChecked(true);
    auto* arrowsAction = viewMenu->addAction(QStringLiteral("Стрелки направлены к слияниям"));
    arrowsAction->setCheckable(true);
    connect(branchingsAction, &QAction::toggled, this, [this](bool checked) {
        m_showBranchingsMerges = checked;
        if (!m_graph.repositoryRoot.isEmpty()) loadRepository(m_graph.repositoryRoot);
    });
    connect(tagsAction, &QAction::toggled, this, [this](bool checked) {
        m_showAllTags = checked;
        if (!m_graph.repositoryRoot.isEmpty()) loadRepository(m_graph.repositoryRoot);
    });
    connect(arrowsAction, &QAction::toggled, this, [this](bool checked) {
        m_arrowsPointToMerges = checked;
        rebuildScene();
    });

    auto* gitMenu = menuBar()->addMenu(QStringLiteral("Git"));
    auto* compareAction = gitMenu->addAction(QStringLiteral("Сравнить ревизии"));
    auto* compareHeadAction = gitMenu->addAction(QStringLiteral("Сравнить ревизии с HEAD"));
    gitMenu->addSeparator();
    auto* unifiedAction = gitMenu->addAction(QStringLiteral("Unified diff ревизий"));
    auto* unifiedHeadAction = gitMenu->addAction(QStringLiteral("Unified diff с HEAD"));
    compareAction->setEnabled(false);
    compareHeadAction->setEnabled(false);
    unifiedAction->setEnabled(false);
    unifiedHeadAction->setEnabled(false);

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("Справка"));
    auto* helpAction = helpMenu->addAction(QStringLiteral("Справка Revision Graph"));
    connect(helpAction, &QAction::triggered, this, [this] {
        QMessageBox::information(this, QStringLiteral("Revision Graph"),
            QStringLiteral("Граф веток Git. Ctrl + колесо — масштаб, перетаскивание мышью — перемещение."));
    });

    m_overviewDock = new QDockWidget(QStringLiteral("Обзор графа"), this);
    auto* overviewView = new GraphView(m_overviewDock);
    overviewView->setScene(m_scene);
    overviewView->setInteractive(false);
    overviewView->setDragMode(QGraphicsView::NoDrag);
    m_overviewDock->setWidget(overviewView);
    addDockWidget(Qt::RightDockWidgetArea, m_overviewDock);
    m_overviewDock->hide();
    connect(m_overviewDock, &QDockWidget::visibilityChanged, overviewAction, &QAction::setChecked);
    connect(m_overviewDock, &QDockWidget::visibilityChanged, overviewMenuAction, &QAction::setChecked);
    if (!initialPath.isEmpty()) loadRepository(initialPath);
}

void MainWindow::chooseRepository()
{
    const QString start = m_graph.repositoryRoot.isEmpty() ? QDir::homePath() : m_graph.repositoryRoot;
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("Выберите Git-репозиторий"), start);
    if (!path.isEmpty()) loadRepository(path);
}

void MainWindow::loadRepository(const QString& path)
{
    statusBar()->showMessage(QStringLiteral("Загрузка истории…"));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    qApp->processEvents();
    GitGraphResult loaded = GitGraphLoader::load(path, m_showBranchingsMerges, m_showAllTags);
    QApplication::restoreOverrideCursor();
    if (!loaded.error.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Не удалось открыть репозиторий"), loaded.error);
        return;
    }
    m_graph = std::move(loaded);
    rebuildScene();
    setWindowTitle(QStringLiteral("%1 — Revision Graph").arg(QDir(m_graph.repositoryRoot).dirName()));
    m_status->setText(QStringLiteral("%1 узлов").arg(m_graph.commits.size()));
    m_view->resetTransform();
    m_view->horizontalScrollBar()->setValue(m_view->horizontalScrollBar()->minimum());
    m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->minimum());
}

void MainWindow::rebuildScene()
{
    m_scene->clear();
    m_nodes.clear();
    m_searchMatches.clear();
    m_searchIndex = -1;

    ogdf::Graph graph;
    ogdf::GraphAttributes attrs(graph, ogdf::GraphAttributes::nodeGraphics | ogdf::GraphAttributes::edgeGraphics);
    QHash<QString, ogdf::node> nodes;
    QHash<int, QString> hashes;
    QHash<QString, QRectF> nodeRects;
    QFont font(QStringLiteral("Monospace"), 9);
    font.setStyleHint(QFont::TypeWriter);
    QFontMetricsF metrics(font);

    auto addNode = [&](const QString& hash) {
        if (nodes.contains(hash)) return nodes.value(hash);
        const ogdf::node node = graph.newNode();
        nodes.insert(hash, node);
        hashes.insert(node->index(), hash);
        const QStringList refs = m_graph.refsByHash.value(hash);
        qreal textWidth = metrics.horizontalAdvance(hash.left(ShortHashLength));
        for (const QString& ref : refs) textWidth = qMax(textWidth, metrics.horizontalAdvance(shortRef(ref)));
        const int lines = qMax(1, refs.size());
        attrs.width(node) = LeftRightMargin * 2.0 + textWidth;
        attrs.height(node) = (TopBottomMargin * 2.0 + metrics.height()) * lines;
        return node;
    };

    for (const GitCommit& commit : m_graph.commits) addNode(commit.hash);
    for (const GitCommit& commit : m_graph.commits) {
        const ogdf::node child = addNode(commit.hash);
        for (const QString& parent : commit.parents) graph.newEdge(child, addNode(parent));
    }

    ogdf::SugiyamaLayout layout;
    layout.setRanking(new ogdf::OptimalRanking());
    layout.setCrossMin(new ogdf::MedianHeuristic());
    auto* hierarchy = new ogdf::FastHierarchyLayout();
    hierarchy->layerDistance(30.0);
    hierarchy->nodeDistance(25.0);
    layout.setLayout(hierarchy);
    layout.call(attrs);

    for (const ogdf::node node : graph.nodes) {
        const QRectF rect(attrs.x(node) - attrs.width(node) / 2.0, attrs.y(node) - attrs.height(node) / 2.0,
                          attrs.width(node), attrs.height(node));
        nodeRects.insert(hashes.value(node->index()), rect);
    }

    // Original DrawConnections: OGDF bends, black two-pixel line and arrow at parent.
    for (const ogdf::edge edge : graph.edges) {
        QVector<QPointF> points;
        const QPointF source(attrs.x(edge->source()), attrs.y(edge->source()));
        const QPointF target(attrs.x(edge->target()), attrs.y(edge->target()));
        points.append(source);
        for (auto it = attrs.bends(edge).begin(); it.valid(); ++it) points.append(QPointF((*it).m_x, (*it).m_y));
        points.append(target);
        if (points.size() >= 2) {
            points[0] = boundaryPoint(nodeRects.value(hashes.value(edge->source()->index())), source, points[1]);
            points.last() = boundaryPoint(nodeRects.value(hashes.value(edge->target()->index())), target, points[points.size() - 2]);
        }
        QPainterPath line(points.front());
        for (int i = 1; i < points.size(); ++i) line.lineTo(points[i]);
        m_scene->addPath(line, QPen(Qt::black, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (points.size() >= 2) {
            const QPointF arrowTip = m_arrowsPointToMerges ? points.front() : points.last();
            const QPointF arrowBack = m_arrowsPointToMerges ? points[1] : points[points.size() - 2];
            QLineF direction(arrowTip, arrowBack);
            direction.setLength(8.0);
            QLineF left = direction; left.setAngle(direction.angle() + 22.5);
            QLineF right = direction; right.setAngle(direction.angle() - 22.5);
            QPainterPath arrow(left.p2()); arrow.lineTo(arrowTip); arrow.lineTo(right.p2());
            m_scene->addPath(arrow, QPen(Qt::black, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        }
    }

    // Original DrawTexts/DrawNode: one colored row per ref, otherwise short hash.
    for (const ogdf::node node : graph.nodes) {
        const QString hash = hashes.value(node->index());
        const QRectF rect = nodeRects.value(hash);
        QStringList refs = m_graph.refsByHash.value(hash);
        if (refs.isEmpty()) refs.append(QString());
        const qreal lineHeight = rect.height() / refs.size();
        QPainterPath hitPath; hitPath.addRoundedRect(rect, 6, 6);
        auto* hitItem = m_scene->addPath(hitPath, Qt::NoPen, Qt::NoBrush);
        hitItem->setData(0, hash);
        hitItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
        hitItem->setToolTip(hash);
        hitItem->setZValue(2.0);
        m_nodes.insert(hash, hitItem);
        for (int line = 0; line < refs.size(); ++line) {
            const QRectF row(rect.left(), rect.top() + lineHeight * line, rect.width(), lineHeight);
            const QString ref = refs[line];
            const QColor color = ref.isEmpty() ? QColor(235, 232, 255) : refColor(refType(ref, m_graph.currentBranch));
            QPainterPath box;
            if (refs.size() == 1) box.addRoundedRect(row, 6, 6); else box.addRect(row);
            auto* shape = m_scene->addPath(box, QPen(color.darker(105), 1.0), QBrush(color));
            shape->setZValue(1.0);
            const QString label = ref.isEmpty() ? hash.left(ShortHashLength) : shortRef(ref);
            auto* text = m_scene->addSimpleText(label, font);
            text->setBrush(Qt::black);
            text->setPos(row.center().x() - text->boundingRect().width() / 2.0,
                         row.center().y() - text->boundingRect().height() / 2.0);
            text->setZValue(1.5);
        }
    }
    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-30, -30, 30, 30));
}

void MainWindow::findNext()
{
    QString query = m_search->text().trimmed();
    if (query.isEmpty()) {
        bool ok = false;
        query = QInputDialog::getText(this, QStringLiteral("Найти"), QStringLiteral("Ветка, тег или hash:"),
                                      QLineEdit::Normal, {}, &ok).trimmed();
        if (!ok || query.isEmpty()) return;
        m_search->setText(query);
        m_searchMatches.clear();
        m_searchIndex = -1;
    }
    if (m_searchMatches.isEmpty()) {
        for (const GitCommit& commit : m_graph.commits) {
            const QString haystack = commit.hash + QLatin1Char(' ') + m_graph.refsByHash.value(commit.hash).join(QLatin1Char(' '));
            if (haystack.contains(query, Qt::CaseInsensitive)) m_searchMatches.append(m_nodes.value(commit.hash));
        }
    }
    if (m_searchMatches.isEmpty()) { statusBar()->showMessage(QStringLiteral("Совпадений нет"), 3000); return; }
    m_searchIndex = (m_searchIndex + 1) % m_searchMatches.size();
    auto* item = m_searchMatches[m_searchIndex];
    m_view->centerOn(item);
    statusBar()->showMessage(QStringLiteral("Совпадение %1 из %2").arg(m_searchIndex + 1).arg(m_searchMatches.size()), 3000);
}

void MainWindow::zoomToFit()
{
    if (!m_scene->items().isEmpty()) {
        m_view->fitInView(m_scene->itemsBoundingRect().adjusted(-4, -4, 4, 4), Qt::KeepAspectRatio);
        const QSignalBlocker blocker(m_zoomCombo);
        m_zoomCombo->setEditText(QStringLiteral("%1%").arg(qRound(m_view->transform().m11() * 100.0)));
    }
}

void MainWindow::zoomToWidth()
{
    const QRectF bounds = m_scene->itemsBoundingRect();
    if (!bounds.isEmpty()) setZoom(qMin<qreal>(2.0, (m_view->viewport()->width() - 20.0) / bounds.width()));
}

void MainWindow::zoomToHeight()
{
    const QRectF bounds = m_scene->itemsBoundingRect();
    if (!bounds.isEmpty()) setZoom(qMin<qreal>(2.0, (m_view->viewport()->height() - 20.0) / bounds.height()));
}

void MainWindow::setZoom(qreal factor)
{
    QTransform transform;
    transform.scale(factor, factor);
    m_view->setTransform(transform);
    const QSignalBlocker blocker(m_zoomCombo);
    m_zoomCombo->setEditText(QStringLiteral("%1%").arg(qRound(factor * 100.0)));
}

void MainWindow::toggleOverview(bool visible)
{
    m_overviewDock->setVisible(visible);
    if (visible) {
        auto* view = static_cast<QGraphicsView*>(m_overviewDock->widget());
        view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

void MainWindow::saveGraph()
{
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Сохранить граф как"),
        QDir(m_graph.repositoryRoot).filePath(QStringLiteral("revision-graph.png")), QStringLiteral("PNG (*.png)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) path += QStringLiteral(".png");
    const QRectF bounds = m_scene->itemsBoundingRect().adjusted(-10, -10, 10, 10);
    QImage image(bounds.size().toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&painter, QRectF(QPointF(0, 0), bounds.size()), bounds);
    painter.end();
    if (!image.save(path)) QMessageBox::critical(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось сохранить граф."));
}
