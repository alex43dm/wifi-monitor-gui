#ifdef DEBUG
#include <QDebug>
#endif
#include <QDir>
#include <QTextStream>
#include <QBarSet>
#include <QBarSeries>
#include <QChart>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QChartView>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#define MAX_ROW_COUNT_TW1 7
#define MAX_ROW_COUNT_TW2 10

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    settings = new QSettings("wifi-monitor-gui");
    path = settings->value("main/Path").toString();
    timeOut = settings->value("main/RefreshTimeout").toInt();

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    model = new QStandardItemModel();
    ui->tableView->setModel(model);
    model->sort(0, Qt::AscendingOrder);

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MainWindow::slotTableViewSelected);
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

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::currentTabChanged);

    uc = new UnixClient();

    connect(ui->actionStart, &QAction::triggered, this, &MainWindow::WifiStart);
    connect(ui->actionStop, &QAction::triggered, this, &MainWindow::WifiStop);
    connect(ui->actionStart_2, &QAction::triggered, this, &MainWindow::ScanStart);
    connect(ui->actionStop_2, &QAction::triggered, this, &MainWindow::ScanStop);

    VisibleScanMenu(false);

    timer = new QTimer;
    timer->setInterval(timeOut);
    connect(timer, &QTimer::timeout, this, &MainWindow::refreshTable);

    series = new QtCharts::QBarSeries();

    QtCharts::QChart *chart = new QtCharts::QChart();
    chart->addSeries(series);
    chart->setTitle("Signal");
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);

    axisX = new QtCharts::QBarCategoryAxis();
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    axisY = new QtCharts::QValueAxis();
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
    delete settings;
    delete timer;

    delete axisY;
    delete axisX;
    delete series;

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

        QHash<QString, int> countHash;
        QStringList lines = s.split('\n');
        for (const QString &line : lines)
        {
            if (line.contains("Client") && line.contains(mac))
            {
                QStringList cell = line.split(QLatin1Char(','));
                if (cell.size() < 5)
                {
#ifdef DEBUG
                    qDebug() << "Error parsing: " << list << Qt::endl;
#endif
                    continue;
                }
                if (countHash.contains(cell.at(4)))
                {
                    countHash[cell.at(4)] = countHash[cell.at(4)] + 1;
                }
                else
                {
                    countHash.insert(cell.at(4), 1);
                }
            }
        }

        series->clear();
#ifdef DEBUG
        qDebug() << "mac : " << mac << Qt::endl;
#endif
        int max = -1;
        QHash<QString, int>::const_iterator i = countHash.constBegin();
        while (i != countHash.constEnd())
        {
#ifdef DEBUG
            qDebug() << i.key() << " : " << i.value() << Qt::endl;
#endif
            QtCharts::QBarSet *set = new QtCharts::QBarSet(i.key());
            *set << i.value();
            series->append(set);
            if (i.value() > max) max = i.value();
            ++i;
        }

        axisX->clear();
        QStringList categories;
        categories << mac;
        axisX->append(categories);

        axisY->setRange(0, max + max / 4);
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
        for (int j = 0; j < MAX_ROW_COUNT_TW1; ++j)
        {
            if (j == MAX_ROW_COUNT_TW1 - 1 && list.size() > MAX_ROW_COUNT_TW1)
            {
                QString str = list.at(j);
                for (int l = j + 1; l < list.size(); ++l)
                {
                    str += "," + list.at(l);
                }
                rowData << new QStandardItem(str);
                continue;
            }
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
    else
    {
        QStringList l = uc->StatustWIFI().split(' ');
        if (l.size() == 2)
        {
            if (l[1] == "0")
            {
                VisibleScanMenu(true);
            }
            ui->statusbar->showMessage("wifi interface: " + l[0]);
        }
    }
}

void MainWindow::ScanStart()
{
    if (uc->StartScan())
    {
        ui->statusbar->showMessage(uc->err);
    }
    else
    {
        timer->start();
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
        timer->stop();
    }
}

void MainWindow::WifiState()
{

}

const QString MainWindow::getLogFile()
{
    QDir dir(path, "dump-*.csv");
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Time);

    for (const QFileInfo &fileInfo : dir.entryInfoList())
    {
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

        for (const QString &line : s.split('\n'))
        {
            if (!line.contains("Client"))
                continue;

            QList<QStandardItem *> rowData;
            QStringList list = line.split(QLatin1Char(','));
            if (list.size() == 1)
                continue;
            for (int j = 0; j < MAX_ROW_COUNT_TW2; ++j)
            {
                rowData << new QStandardItem(list.at(j));
            }
            modelLog->appendRow(rowData);
            rowData.clear();
        }
    }
}

void MainWindow::refreshTable()
{
    QDir dir(path, "dump-*.csv");
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Time);
    QString fpath;
    for (const QFileInfo &file : dir.entryInfoList())
    {
        if (file.fileName().contains("kismet", Qt::CaseInsensitive))
            continue;
        if (file.fileName().contains("log", Qt::CaseInsensitive))
            continue;
        fpath = file.filePath();
        break;
    }

    try
    {
        FILE *fp = fopen(fpath.toStdString().c_str(), "r");
        if (fp)
        {
            QTextStream s1(fp, QIODevice::ReadOnly);
            QString s = s1.readAll();
            fclose(fp);
            handleResults(s);
        }
        else
        {
            ui->statusbar->showMessage("Cann't open file: " + fpath);
        }
    }
    catch (const std::exception &ex)
    {
        ui->statusbar->showMessage("Error: " + QString(ex.what()));
    }
}
