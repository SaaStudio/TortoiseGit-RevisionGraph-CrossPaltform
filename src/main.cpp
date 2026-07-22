#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TortoiseGit Revision Graph Cross"));
    QApplication::setOrganizationName(QStringLiteral("TortoiseGit Revision Graph Cross"));

    const QString initialPath = app.arguments().size() > 1 ? app.arguments().at(1) : QDir::currentPath();
    MainWindow window(initialPath);
    if (qEnvironmentVariableIsSet("TGIT_GRAPH_TEST_BRANCHINGS")) {
        for (QAction* action : window.findChildren<QAction*>()) {
            if (action->text() == QStringLiteral("Показывать точки ветвления и слияния")) {
                action->setChecked(true);
                break;
            }
        }
    }
    window.show();
    const QString screenshotPath = qEnvironmentVariable("TGIT_GRAPH_SCREENSHOT");
    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(1000, &window, [&window, screenshotPath] {
            const bool menuShot = qEnvironmentVariableIsSet("TGIT_GRAPH_SCREENSHOT_MENU");
            QMenu* menu = menuShot ? window.menuBar()->actions().value(1)->menu() : nullptr;
            if (menu) {
                menu->popup(window.mapToGlobal(QPoint(80, 25)));
                QTimer::singleShot(200, menu, [menu, screenshotPath] {
                    menu->grab().save(screenshotPath);
                    QApplication::quit();
                });
                return;
            }
            window.grab().save(screenshotPath);
            QApplication::quit();
        });
    }
    return app.exec();
}
