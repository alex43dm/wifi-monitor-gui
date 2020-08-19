#include <QDebug>
#include <QDir>
#include <QBarSet>
#include <QBarSeries>
#include <QChart>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QChartView>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    model = new QStandardItemModel();
    ui->tableView->setModel(model);
    model->sort(0, Qt::AscendingOrder);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &,
            const QItemSelection &)), this, SLOT(slotTableViewSelected(const QItemSelection &,
                    const QItemSelection &)));
    row = -1;

    model->setHorizontalHeaderItem(0, new QStandardItem("Station MAC"));
    model->setHorizontalHeaderItem(1, new QStandardItem("First time seen"));
    model->setHorizontalHeaderItem(2, new QStandardItem("Last time seen"));
    model->setHorizontalHeaderItem(3, new QStandardItem("Power"));
    model->setHorizontalHeaderItem(4, new QStandardItem("# packets"));
    model->setHorizontalHeaderItem(5, new QStandardItem("BSSID"));
    model->setHorizontalHeaderItem(6, new QStandardItem("Probed ESSIDs"));

    ui->tableViewLog->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    modelLog = new QStandardItemModel();
    ui->tableViewLog->setModel(modelLog);

    modelLog->setHorizontalHeaderItem(0, new QStandardItem("LocalTime"));
    modelLog->setHorizontalHeaderItem(1, new QStandardItem("GPSTime"));
    modelLog->setHorizontalHeaderItem(2, new QStandardItem("ESSID"));
    modelLog->setHorizontalHeaderItem(3, new QStandardItem("BSSID"));
    modelLog->setHorizontalHeaderItem(4, new QStandardItem("Power"));
    modelLog->setHorizontalHeaderItem(5, new QStandardItem("Security"));
    modelLog->setHorizontalHeaderItem(6, new QStandardItem("Latitude"));
    modelLog->setHorizontalHeaderItem(7, new QStandardItem("Longitude"));
    modelLog->setHorizontalHeaderItem(8, new QStandardItem("Latitude Error"));
    modelLog->setHorizontalHeaderItem(9, new QStandardItem("Longitude Error"));

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));

    uc = new UnixClient();

    connect(ui->actionStart, &QAction::triggered, this, &MainWindow::WifiStart);
    connect(ui->actionStop, &QAction::triggered, this, &MainWindow::WifiStop);
    connect(ui->actionStart_2, &QAction::triggered, this, &MainWindow::ScanStart);
    connect(ui->actionStop_2, &QAction::triggered, this, &MainWindow::ScanStop);

    //VisibleScanMenu(false);

    worker = new FileTail;
    worker->path = "/var/lib/wifi-monitor";
    thr = new QThread;
    worker->moveToThread(thr);

    connect(thr, &QThread::started, worker, &FileTail::process);
    connect(worker, &FileTail::finished, thr, &QThread::quit);
    connect(worker, &FileTail::finished, worker, &FileTail::deleteLater);
    connect(thr, &QThread::finished, thr, &QThread::deleteLater);
    connect(worker, &FileTail::resultReady, this, &MainWindow::handleResults);

    series = new QtCharts::QBarSeries();

    QtCharts::QChart *chart = new QtCharts::QChart();
    chart->addSeries(series);
    chart->setTitle("Signal");
    //chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);

    QStringList categories;
    categories << "MAC";

    QtCharts::QBarCategoryAxis *axisX = new QtCharts::QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
    axisY->setRange(0, 100);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QtCharts::QChartView *chartView = new QtCharts::QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    ui->tabWidget->addTab(chartView, "Histogram");
}

MainWindow::~MainWindow()
{
    worker->quit = true;

    thr->quit();
    thr->wait();

    delete  worker;
    delete thr;

    delete uc;
    delete modelLog;
    delete model;
    delete ui;
}

void MainWindow::VisibleScanMenu(bool hide)
{
    QList<QMenu *> menus = ui->menubar->findChildren<QMenu *>();
    for (int i = 0; i < menus.size(); ++i)
    {
        if (menus.at(i)->title() == "Scan")
        {
            menus.at(i)->menuAction()->setVisible(hide);
        }
    }
}

void MainWindow::slotTableViewSelected(const QItemSelection &selected,
                                       const QItemSelection &deselected)
{
    (void)deselected;
    if (selected.indexes().size() > 0)
    {
        if ( mac == selected.indexes()[0].data().toString())
            return;

        mac = selected.indexes()[0].data().toString();
        row = selected.indexes()[0].row();

        QString s;
        QString fpath = getLogFile();
        FILE *fp = fopen(fpath.toStdString().c_str(), "r");
        if (fp)
        {
            QTextStream s1(fp, QIODevice::ReadOnly);
            s = s1.readAll();
            fclose(fp);
        }
        else
        {
            ui->statusbar->showMessage("Cann't open file: " + fpath);
            return;
        }

        QHash<float, int> countHash;
        QStringList lines = s.split('\n');
        for (int i = 0; i < lines.size(); ++i)
        {
            if (lines.at(i).contains("Client") && lines.at(i).contains(mac))
            {
                QStringList list = lines[i].split(QLatin1Char(','));
                if (list.size() < 5)
                    continue;
                float a = abs(list.at(4).toFloat());
                if (countHash.contains(a))
                {
                    int val = countHash[a] + 1;
                    countHash[a] = val;
                }
                else
                {
                    countHash.insert(a, 1);
                }
            }
        }

        series->clear();

        qDebug() << "mac : " << mac << Qt::endl;

        QHash<float, int>::const_iterator i = countHash.constBegin();
        while (i != countHash.constEnd())
        {
            qDebug() << i.key() << " : " << i.value() << Qt::endl;
            QString str = QString::number(i.value());
            QtCharts::QBarSet *set = new QtCharts::QBarSet(str);
            *set << i.key();
            series->append(set);

            ++i;
        }
    }
    /*
    else if(deselected.indexes().size() > 0)
        {
            mac.clear();
            row = -1;
        }*/
}

void MainWindow::handleResults(const QString &s)
{
    model->removeRows(0, model->rowCount());

    QStringList lines = s.split('\n');
    int startPos = 0;
    for (int i = 0; i < lines.size(); ++i)
    {
        if (!lines.at(i).contains("Station MAC"))
        {
            continue;
        }
        startPos = ++i;
        break;
    }

    int ind = 0;

    for (int i = startPos; i < lines.size(); ++i)
    {
        QList<QStandardItem *> rowData;
        QStringList list = lines[i].split(QLatin1Char(','));
        if (list.size() == 1)
            continue;
        for (int j = 0; j < list.size(); ++j)
        {
            rowData << new QStandardItem(list.at(j));
        }
        model->appendRow(rowData);
        rowData.clear();

        if (mac.size() && mac == list.at(0))
        {
            ind = model->rowCount();
        }
    }

    if (ind)
    {
        ui->tableView->selectRow(ind - 1);
    }
}

void MainWindow::WifiStart()
{
    if (uc->StartWIFI())
    {
        ui->statusbar->showMessage(uc->err);
    }
    VisibleScanMenu(false);
}

void MainWindow::WifiStop()
{
    if (uc->StoptWIFI())
    {
        ui->statusbar->showMessage(uc->err);
    }
    VisibleScanMenu(true);
}

void MainWindow::ScanStart()
{
    if (uc->StartScan())
    {
        ui->statusbar->showMessage(uc->err);
    }
    else
    {
        worker->quit = false;
        thr->start();
    }
}

void MainWindow::ScanStop()
{
    if (uc->StopScan())
    {
        ui->statusbar->showMessage(uc->err);
    }
    else
    {
        worker->quit = true;
        thr->quit();
    }
}

const QString MainWindow::getLogFile()
{
    QDir dir(worker->path, "dump-*.csv");
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Time);
    QFileInfoList list = dir.entryInfoList();
    QString fpath;
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.fileName().contains("kismet", Qt::CaseInsensitive))
            continue;
        if (fileInfo.fileName().contains("log", Qt::CaseInsensitive))
        {
            return fileInfo.filePath();
        }
    }
    return QString();
}

void MainWindow::currentTabChanged(int tab)
{
    if (tab == 1)
    {
        QString s;
        QString fpath = getLogFile();
        FILE *fp = fopen(fpath.toStdString().c_str(), "r");
        if (fp)
        {
            QTextStream s1(fp, QIODevice::ReadOnly);
            s = s1.readAll();
            fclose(fp);
        }
        else
        {
            ui->statusbar->showMessage("Cann't open file: " + fpath);
            return;
        }

        modelLog->removeRows(0, modelLog->rowCount());

        QStringList lines = s.split('\n');
        for (int i = 0; i < lines.size(); ++i)
        {
            if (lines.at(i).contains("Client"))
            {
                QList<QStandardItem *> rowData;
                QStringList list = lines[i].split(QLatin1Char(','));
                if (list.size() == 1)
                    continue;
                for (int j = 0; j < list.size() - 1; ++j)
                {
                    rowData << new QStandardItem(list.at(j));
                }
                modelLog->appendRow(rowData);
                rowData.clear();
            }
        }
    }
}