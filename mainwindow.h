#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QThread>
#include <QBarSeries>
#include <QValueAxis>
#include <QBarCategoryAxis>

#include "filetail.h"
#include "unixclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
        Q_OBJECT
        int row;
        QString mac;
        QtCharts::QBarSeries *series;
        QtCharts::QValueAxis *axisY;
        QtCharts::QBarCategoryAxis *axisX;

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    private:
        Ui::MainWindow *ui;
        QStandardItemModel *model, *modelLog;
        UnixClient *uc;
        QThread *thr;
        FileTail *worker;

        void VisibleScanMenu(bool);
        const QString getLogFile();

    private slots:
        void WifiStart();
        void WifiStop();
        void ScanStart();
        void ScanStop();
        void slotTableViewSelected(const QItemSelection &selected, const QItemSelection &deselected);
        void currentTabChanged(int);


    public slots:
        void handleResults(const QString &s);

    signals:
        void operate(const QString &);
};
#endif // MAINWINDOW_H
