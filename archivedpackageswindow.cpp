#include "archivedpackageswindow.h"
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QMap>
#include <QVariant>
#include <QListWidgetItem>

ArchivedPackagesWindow::ArchivedPackagesWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Archived Packages");
    layout = new QVBoxLayout(this);
    
    archivedList = new QListWidget(this);
    archivedList->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(archivedList);
    
    connect(archivedList, &QListWidget::customContextMenuRequested,
            this, &ArchivedPackagesWindow::onItemContextMenuRequested);
    
    refreshList();
}

ArchivedPackagesWindow::~ArchivedPackagesWindow() {}

void ArchivedPackagesWindow::refreshList()
{
    archivedList->clear();
    
    QSettings settings;
    QStringList savedPackages = settings.value("trackingNumbers").toStringList();
    QMap<QString, QVariant> archivedMap = settings.value("packageArchived").toMap();
    
    for (const QString &trackingNumber : savedPackages) {
        bool isArchived = archivedMap.contains(trackingNumber) ? archivedMap[trackingNumber].toBool() : false;
        if (isArchived) {
            auto item = new QListWidgetItem(trackingNumber);
            archivedList->addItem(item);
        }
    }
}

void ArchivedPackagesWindow::onItemContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* item = archivedList->itemAt(pos);
    if (!item)
        return;
    
    QMenu contextMenu;
    QAction *unarchiveAction = contextMenu.addAction("Unarchive");
    connect(unarchiveAction, &QAction::triggered, this, &ArchivedPackagesWindow::unarchiveSelected);
    contextMenu.exec(archivedList->viewport()->mapToGlobal(pos));
}

void ArchivedPackagesWindow::unarchiveSelected()
{
    QListWidgetItem* item = archivedList->currentItem();
    if (!item)
        return;
    
    QString trackingNumber = item->text();
    emit requestUnarchive(trackingNumber);
    
    delete archivedList->takeItem(archivedList->row(item));
} 