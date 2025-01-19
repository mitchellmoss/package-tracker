#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSettings>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addPackage();
    void removePackage();
    void refreshPackages();
    void showPackageDetails(QListWidgetItem* item);

private:
    void setupUI();
    void loadPackages();
    void savePackages();
    
    QListWidget *packageList;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *refreshButton;
    QLineEdit *trackingInput;
    QTextEdit *detailsView;
    
    QSettings settings;
};

#endif // MAINWINDOW_H
