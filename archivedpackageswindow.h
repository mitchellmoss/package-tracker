#ifndef ARCHIVEDPACKAGESWINDOW_H
#define ARCHIVEDPACKAGESWINDOW_H

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>

class ArchivedPackagesWindow : public QDialog {
    Q_OBJECT
public:
    explicit ArchivedPackagesWindow(QWidget *parent = nullptr);
    ~ArchivedPackagesWindow();
    void refreshList();

signals:
    // Emitted when the user wishes to unarchive a package.
    void requestUnarchive(const QString &trackingNumber);

private slots:
    void onItemContextMenuRequested(const QPoint &pos);
    void unarchiveSelected();

private:
    QListWidget *archivedList;
    QVBoxLayout *layout;
};

#endif // ARCHIVEDPACKAGESWINDOW_H 