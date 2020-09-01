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
#include <QDateTime>
#include <QThread>
#include <QInputDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#define MAX_ROW_COUNT_TW1 7

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    settings = new QSettings("wifi-monitor-gui");
    path = settings->value("main/Path").toString();
    if (path.size() == 0)
    {
        path = "/var/lib/wifi-monitor";
    }

    timeOut = settings->value("main/RefreshTimeout").toInt();
    if (timeOut == 0)
    {
        timeOut = 1000;
    }

    kickTimeout = settings->value("main/KickTimeout").toInt();
    if (kickTimeout == 0)
    {
        kickTimeout = 120;
    }


    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    model = new QStandardItemModel();
    ui->tableView->setModel(model);
    model->sort(0, Qt::AscendingOrder);

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MainWindow::slotTableViewSelected);
    row = -1;

    model->setHorizontalHeaderItem(0, new QStandardItem("Station MAC"));
    model->setHorizontalHeaderItem(1, new QStandardItem("First time seen"));
    model->setHorizontalHeaderItem(2, new QStandardItem("Time in range"));
    model->setHorizontalHeaderItem(3, new QStandardItem("Power"));
    model->setHorizontalHeaderItem(4, new QStandardItem("Associated"));

    ui->tableViewLog->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    modelLog = new QStandardItemModel();
    ui->tableViewLog->setModel(modelLog);
    modelLog->sort(0, Qt::AscendingOrder);

    modelLog->setHorizontalHeaderItem(0, new QStandardItem("Station MAC"));
    modelLog->setHorizontalHeaderItem(1, new QStandardItem("First time seen"));
    modelLog->setHorizontalHeaderItem(2, new QStandardItem("Last time seen"));
    modelLog->setHorizontalHeaderItem(3, new QStandardItem("Average Power"));
    modelLog->setHorizontalHeaderItem(4, new QStandardItem("Number of times in range"));

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::currentTabChanged);

    uc = new UnixClient();

    connect(ui->actionStart, &QAction::triggered, this, &MainWindow::WifiStart);
    connect(ui->actionStop, &QAction::triggered, this, &MainWindow::WifiStop);
    connect(ui->actionStart_2, &QAction::triggered, this, &MainWindow::ScanStart);
    connect(ui->actionStop_2, &QAction::triggered, this, &MainWindow::ScanStop);
    connect(ui->actionSet_Kick_TimeOut, &QAction::triggered, this, &MainWindow::SetKickTimeOut);

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

    WifiGetStatus();

    stateTimer = new QTimer;
    stateTimer->setInterval(3000);
    connect(stateTimer, &QTimer::timeout, this, &MainWindow::WifiGetStatus);
    stateTimer->start();
}

MainWindow::~MainWindow()
{
    delete settings;
    delete stateTimer;
    delete timer;

    delete axisY;
    delete axisX;
    delete series;

    delete uc;
    delete modelLog;
    delete model;
    delete ui;
}

void MainWindow::SetKickTimeOut()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("QInputDialog::getInteger()"),
                                         tr("Kick TimeOut:"), QLineEdit::Normal,
                                         QString::number(kickTimeout), &ok);
    if (ok && !text.isEmpty())
    {
        kickTimeout = text.toInt();
        uc->SetKickTimeOut(kickTimeout);
    }
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

static QStandardItem *getStandartItem(const QString &str)
{
    QStandardItem *sit = new QStandardItem(str);
    sit->setTextAlignment(Qt::AlignCenter);
    return sit;
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
            if (j == 4 || j == 6)
                continue;

            if (j == 2)
            {
                QTime a(0, 0, 0);
                rowData << getStandartItem(a.addSecs(dateTimeDiff(list.at(j - 1), list.at(j))).toString());
            }
            else
            {
                rowData << getStandartItem(list.at(j));
            }
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

void MainWindow::WifiGetStatus()
{
    if (!uc->Status())
    {
        if (uc->wifi_mode == 0)
        {
            VisibleScanMenu(true);
        }
        else
        {
            VisibleScanMenu(false);
        }

        if (uc->scanning == 1)
        {
            timer->start();
        }


        if (uc->kick_timeout != kickTimeout)
        {
            uc->SetKickTimeOut(kickTimeout);
        }

        ui->statusbar->showMessage("wifi interface: " + uc->iface);
    }
    else
    {
        VisibleScanMenu(false);
        ui->statusbar->showMessage("get status error: " + uc->err);
    }
}

void MainWindow::WifiStart()
{
    if (uc->StartWIFI())
    {
        ui->statusbar->showMessage(uc->err);
    }
}

void MainWindow::WifiStop()
{
    if (uc->StoptWIFI())
    {
        ui->statusbar->showMessage(uc->err);
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

void MainWindow::fillLogTable(MACresult *res)
{
    QList<QStandardItem *> rowData;
    rowData << getStandartItem(res->mac);
    rowData << getStandartItem(res->FirstTimeSeen);
    rowData << getStandartItem(res->LastTimeSeen);
    rowData << getStandartItem("-" + QString::number(res->powerSum, 'g', 3));
    rowData << getStandartItem(QString::number(res->numberInRange));
    modelLog->appendRow(rowData);

    delete  res;
}

void MainWindow::runThread(std::shared_ptr<QStringList> s, const QString &mac)
{
    MACStat *worker = new MACStat(s, mac, kickTimeout);
    QThread *thread = new QThread;
    worker->moveToThread(thread);
    connect(thread, &QThread::started, worker, &MACStat::process);
    connect(worker, &MACStat::sendResult, this, &MainWindow::fillLogTable);
    thread->start();
}

void MainWindow::currentTabChanged(int tab)
{
    if (tab == 1)
    {
        QStringList log;
        QSet<QString> *MACs = new QSet<QString>();

        QString fpath = getLogFile();
        FILE *fp = fopen(fpath.toStdString().c_str(), "r");
        if (fp)
        {
            QTextStream s1(fp, QIODevice::ReadOnly);
            while (!s1.atEnd())
            {
                QString line = s1.readLine();
                if (!line.contains("Client"))
                    continue;
                log << line;

                QStringList list = line.split(QLatin1Char(','));
                if (list.size() < MAX_ROW_COUNT_TW2 || MACs->contains(list.at(3)))
                    continue;

                MACs->insert(list.at(3));
            }
            fclose(fp);
        }
        else
        {
            ui->statusbar->showMessage("Cann't open file: " + fpath);
            return;
        }

        std::shared_ptr<QStringList> ptr = std::make_shared<QStringList>(log);

        modelLog->removeRows(0, modelLog->rowCount());

        foreach (const QString &mac, *MACs)
        {
            runThread(ptr, mac);
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
