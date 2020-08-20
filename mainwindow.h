#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QTimer>
#include <QBarSeries>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QSettings>

#include "unixclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
        Q_OBJECT
        QString path;
        int timeOut;
        int row;
        QString mac;
        QtCharts::QBarSeries *series;
        QtCharts::QValueAxis *axisY;
        QtCharts::QBarCategoryAxis *axisX;
        QTimer *timer;
        QSettings *settings;

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    private:
        Ui::MainWindow *ui;
        QStandardItemModel *model, *modelLog;
        UnixClient *uc;
        QThread *thr;

        void VisibleScanMenu(bool);
        const QString getLogFile();

    private slots:
        void WifiStart();
        void WifiStop();
        void ScanStart();
        void ScanStop();
        void slotTableViewSelected(const QItemSelection &selected, const QItemSelection &deselected);
        void currentTabChanged(int);
        void refreshTable();


    public slots:
        void handleResults(const QString &s);

    signals:
        void operate(const QString &);
};
#endif // MAINWINDOW_H
