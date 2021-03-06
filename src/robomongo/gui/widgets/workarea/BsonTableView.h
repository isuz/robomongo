#pragma once
#include <QTableView>

#include "robomongo/core/domain/Notifier.h"

namespace Robomongo
{
    class BsonTableView : public QTableView , public INotifierObserver
    {
        Q_OBJECT
    public:
        typedef QTableView BaseClass;
        explicit BsonTableView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent = 0);     

    public Q_SLOTS:
        void showContextMenu(const QPoint &point);

    private:
        virtual QModelIndex selectedIndex() const;
        Notifier _notifier;
    };
}
