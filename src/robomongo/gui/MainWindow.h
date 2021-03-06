#pragma once

#include <QMainWindow>
QT_BEGIN_NAMESPACE
class QLabel;
class QToolBar;
class QDockWidget;
QT_END_NAMESPACE

#include "robomongo/gui/ViewMode.h"

namespace Robomongo
{
    class LogWidget;
    class ExplorerWidget;
    class EventBus;
    class ConnectionFailedEvent;
    class ScriptExecutingEvent;
    class ScriptExecutedEvent;
    class QueryWidgetUpdatedEvent;
    class AllTabsClosedEvent;
    class WorkAreaWidget;
    class ConnectionMenu;
    class App;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        typedef QMainWindow baseClass;
        MainWindow();
        void keyPressEvent(QKeyEvent *event);

    public Q_SLOTS:
        void manageConnections();
        void toggleOrientation();
        void enterTextMode();
        void enterTreeMode();
        void enterTableMode();
        void enterCustomMode();
        void saveViewMode();
        void executeScript();
        void stopScript();
        void toggleFullScreen2();
        void refreshConnections();
        void aboutRobomongo();
        void open();
        void save();
        void saveAs();

        void setDefaultUuidEncoding();
        void setJavaUuidEncoding();
        void setCSharpUuidEncoding();
        void setPythonUuidEncoding();
        void setLoadMongoRcJs();
        void setDisableConnectionShortcuts();

        void toggleLogs(bool show);
        void connectToServer(QAction *action);
        void handle(ConnectionFailedEvent *event);
        void handle(ScriptExecutingEvent *event);
        void handle(ScriptExecutedEvent *event);
        void handle(AllTabsClosedEvent *event);
        void handle(QueryWidgetUpdatedEvent *event);
    private Q_SLOTS:
        void updateMenus();
        void setUtcTimeZone();
        void setLocalTimeZone();
    private:
        QLabel *_status;
        ViewMode _viewMode;
        LogWidget *_log;
        QDockWidget *_logDock;

        WorkAreaWidget *_workArea;

        /*
        ** The only Explorer in the window
        */
        ExplorerWidget *_explorer;

        App *_app;
        EventBus *_bus;

        ConnectionMenu *_connectionsMenu;
        QAction *_connectAction;
        QAction *_openAction;
        QAction *_saveAction;
        QAction *_saveAsAction;
        QAction *_executeAction;
        QAction *_stopAction;
        QAction *_orientationAction;
        QToolBar *_execToolBar;

        void updateConnectionsMenu();
        void createDatabaseExplorer();
        void createTabs();
    };

}
