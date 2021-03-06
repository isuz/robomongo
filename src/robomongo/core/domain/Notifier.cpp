#include "robomongo/core/domain/Notifier.h"

#include <QAction>
#include <QClipboard>
#include <QApplication>
#include <QWidget>
#include <QMenu>

#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/settings/SettingsManager.h"

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/utils/DialogUtils.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/domain/MongoServer.h"

namespace Robomongo
{
    namespace detail
    {
        bool isSimpleType(Robomongo::BsonTreeItem *item)
        {
            return Robomongo::BsonUtils::isSimpleType(item->type()) || Robomongo::BsonUtils::isUuidType(item->type(),item->binType());
        }
    }

    Notifier::Notifier(INotifierObserver *const observer, MongoShell *shell, const MongoQueryInfo &queryInfo, QObject *parent)
        :BaseClass(parent),_observer(observer),_shell(shell),_queryInfo(queryInfo)
    {
        QWidget *wid = dynamic_cast<QWidget*>(_observer);

        _deleteDocumentAction = new QAction("Delete Document", wid);
        VERIFY(connect(_deleteDocumentAction, SIGNAL(triggered()), SLOT(onDeleteDocument())));

        _editDocumentAction = new QAction("Edit Document", wid);
        VERIFY(connect(_editDocumentAction, SIGNAL(triggered()), SLOT(onEditDocument())));

        _viewDocumentAction = new QAction("View Document", wid);
        VERIFY(connect(_viewDocumentAction, SIGNAL(triggered()), SLOT(onViewDocument())));

        _insertDocumentAction = new QAction("Insert Document", wid);
        VERIFY(connect(_insertDocumentAction, SIGNAL(triggered()), SLOT(onInsertDocument())));

        _copyValueAction = new QAction("Copy Value", wid);
        VERIFY(connect(_copyValueAction, SIGNAL(triggered()), SLOT(onCopyDocument())));
    }

    void Notifier::query()
    {
        _shell->query(0, _queryInfo);
    }

    void Notifier::initMenu(QMenu *const menu, BsonTreeItem *const item)
    {
        bool isEditable = _queryInfo.isNull ? false : true;
        bool onItem = item ? true : false;
        
        bool isSimple = false;
        if(item){
            isSimple = detail::isSimpleType(item);
        }

        if (onItem && isEditable) menu->addAction(_editDocumentAction);
        if (onItem)               menu->addAction(_viewDocumentAction);
        if (isEditable)           menu->addAction(_insertDocumentAction);
        if (onItem && isSimple)   menu->addSeparator();
        if (onItem && isSimple)   menu->addAction(_copyValueAction);
        if (onItem && isEditable) menu->addSeparator();
        if (onItem && isEditable) menu->addAction(_deleteDocumentAction);
    }

    void Notifier::onDeleteDocument()
    {
        if (_queryInfo.isNull)
            return;

        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if(!documentItem)
            return;

        mongo::BSONObj obj = documentItem->root();
        mongo::BSONElement id = obj.getField("_id");

        if (id.eoo()) {
            QMessageBox::warning(dynamic_cast<QWidget*>(_observer), "Cannot delete", "Selected document doesn't have _id field. \n"
                "Maybe this is a system document that should be managed in a special way?");
            return;
        }

        mongo::BSONObjBuilder builder;
        builder.append(id);
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        // Ask user
        int answer = utils::questionDialog(dynamic_cast<QWidget*>(_observer),"Delete","Document","%1 %2 with id:<br><b>%3</b>?",QtUtils::toQString(id.toString(false)));

        if (answer != QMessageBox::Yes)
            return ;

        _shell->server()->removeDocuments(query, _queryInfo.databaseName, _queryInfo.collectionName);
        _shell->query(0, _queryInfo);
    }

    void Notifier::onEditDocument()
    {
        if (_queryInfo.isNull)
            return;

        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if(!documentItem)
            return;

        mongo::BSONObj obj = documentItem->root();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, AppRegistry::instance().settingsManager()->uuidEncoding(), AppRegistry::instance().settingsManager()->timeZone() );
        const QString &json = QtUtils::toQString(str);

        DocumentTextEditor editor(QtUtils::toQString(_queryInfo.serverAddress),
            QtUtils::toQString(_queryInfo.databaseName),
            QtUtils::toQString(_queryInfo.collectionName),
            json);

        editor.setWindowTitle("Edit Document");
        int result = editor.exec();

        if (result == QDialog::Accepted) {
            mongo::BSONObj obj = editor.bsonObj();
            AppRegistry::instance().bus()->subscribe(this, InsertDocumentResponse::Type);
            _shell->server()->saveDocument(obj, _queryInfo.databaseName, _queryInfo.collectionName);
        }
    }

    void Notifier::onViewDocument()
    {
        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if(!documentItem)
            return;

        mongo::BSONObj obj = documentItem->root();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, AppRegistry::instance().settingsManager()->uuidEncoding(), AppRegistry::instance().settingsManager()->timeZone());
        const QString &json = QtUtils::toQString(str);

        std::string server = _queryInfo.isNull ? "" : _queryInfo.serverAddress;
        std::string database = _queryInfo.isNull ? "" : _queryInfo.databaseName;
        std::string collection = _queryInfo.isNull ? "" : _queryInfo.collectionName;

        DocumentTextEditor *editor = new DocumentTextEditor(QtUtils::toQString(server),QtUtils::toQString(database), QtUtils::toQString(collection), json, true, dynamic_cast<QWidget*>(_observer));

        editor->setWindowTitle("View Document");
        editor->show();
    }

    void Notifier::onInsertDocument()
    {
        if (_queryInfo.isNull)
            return;

        DocumentTextEditor editor(QtUtils::toQString(_queryInfo.serverAddress),
            QtUtils::toQString(_queryInfo.databaseName),
            QtUtils::toQString(_queryInfo.collectionName),
            "{\n    \n}");

        editor.setCursorPosition(1, 4);
        editor.setWindowTitle("Insert Document");
        int result = editor.exec();

        if (result == QDialog::Accepted) {
            mongo::BSONObj obj = editor.bsonObj();
            _shell->server()->insertDocument(obj, _queryInfo.databaseName, _queryInfo.collectionName);
            _shell->query(0, _queryInfo);
        }
    }

    void Notifier::onCopyDocument()
    {
        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if(!documentItem)
            return;

        if (!detail::isSimpleType(documentItem))
            return;

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(documentItem->value());
    }
}