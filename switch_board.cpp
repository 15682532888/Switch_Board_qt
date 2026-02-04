#include "switch_board.h"
#include "ui_switch_board.h"
#include <QProcess>
#include <QFileDialog>
#include <qt_windows.h>
#include <QTimer>
#include <QDateTime>
#include <QToolTip>
#include "qcustomplot.h"

int timerId;
unsigned char dddate[30] = {0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x08};
unsigned int txLength;
unsigned char BTS7008Flag = 0;
unsigned char HCT4066DFlag = 0;
quint64 elapsedTimerStart;

Switch_Board::Switch_Board(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Switch_Board)
{
    ui->setupUi(this);
	
	customPlot = new QCustomPlot(this);

    // 颜色数组
    QColor colors[4] = {
        Qt::blue,    // 曲线1
        Qt::red,     // 曲线2
        Qt::green,   // 曲线3
        Qt::magenta  // 曲线4
    };

    QString seriesNames[4] = {
        "TAS 电流(mA)",
        "曲线2",
        "曲线3",
        "曲线4"
    };

    // 建立 4 條曲線
    for (int i = 0; i < 4; ++i) {
        graph[i] = customPlot->addGraph();
        graph[i]->setName(seriesNames[i]);
        graph[i]->setPen(QPen(colors[i], 2));
        graph[i]->setAntialiased(true);
        graphDataContainers[i] = QSharedPointer<QCPGraphDataContainer>(new QCPGraphDataContainer);
        graph[i]->setData(graphDataContainers[i]);
        // 可選擇是否顯示散點：graph[i]->setScatterStyle(QCPScatterStyle::ssCircle);
    }

    // X 軸設定（時間 ms）
    customPlot->xAxis->setLabel("时间 (ms)");
    customPlot->xAxis->setRange(0, 20000);

    // Y 軸設定（電流 mA）
    customPlot->yAxis->setLabel("电流 (mA)");
    customPlot->yAxis->setRange(0, 200);

    // 啟用互動：拖曳、縮放、選取
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    // 圖例顯示在右上角
    customPlot->legend->setVisible(true);
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    // 放入 ui->widget
    QVBoxLayout *chartLayout = new QVBoxLayout(ui->widget);
    ui->widget->setLayout(chartLayout);
    chartLayout->addWidget(customPlot);

    // 用來追蹤 Y 軸極值（增量更新，避免每次掃描全部點）
    minY = 0.0;
    maxY = 200.0;

    elapsedTimer.start();

    // 圖表刷新定時器（建議 400~600ms）
    timerChart = new QTimer(this);
    timerChart->setInterval(300);
    connect(timerChart, &QTimer::timeout, this, &Switch_Board::onTimeChart);
    timerChart->start();

ui->textEdit->document()->setMaximumBlockCount(50000); // 只保留最近500行数据


    app_libusbTh = new app_libusb();

    connect(app_libusbTh, app_libusb::libusbSignal, this, &Switch_Board::rexda);

    (void)libusb_init(NULL);

    app_libusbTh->start();

    // 滑鼠移動時顯示提示
    connect(customPlot, &QCustomPlot::mouseMove, this, &Switch_Board::onMouseMoveShowTooltip);
}

Switch_Board::~Switch_Board()
{
    if (timerChart->isActive())
    {
        timerChart->stop();
    }

    app_libusbTh->stop();
    app_libusbTh->quit();
    (void)app_libusbTh->wait();
    delete app_libusbTh;

    delete ui;
}


void Switch_Board::rexda(unsigned char* data, int length)
{
    QString s;

    for (int i = 0; i < length; i++)
    {
        s.append(QString("%1").arg(data[i], 2, 16, QChar('0')).toUpper() + " ");
    }
    ui->textEdit->append("Rx: " + s);

    if (length >= 10 && (0x5a == data[0]) && (0x5a == data[1]))
    {
        double v1 = 3.621 * (((0x0f & data[3]) << 8) | (data[2] > 0x3c ? (data[2] - 0x3c) : 0));
        double v2 = 3.621 * (((0x0f & data[5]) << 8) | (data[4] > 0x3c ? (data[4] - 0x3c) : 0));
        double v3 = 3.621 * (((0x0f & data[7]) << 8) | (data[6] > 0x3c ? (data[6] - 0x3c) : 0));
        double v4 = 3.621 * (((0x0f & data[9]) << 8) | (data[8] > 0x3c ? (data[8] - 0x3c) : 0));

        updateData(v1, v2, v3, v4);
    }
}

void thisCB(uint uTimerID, uint uMsg, HANDLE_PTR dwUser, HANDLE_PTR dw1, HANDLE_PTR dw2)
{

}

void Switch_Board::updateData(double newValue1, double newValue2, double newValue3, double newValue4)
{
    qint64 currentTime = elapsedTimer.elapsed();

    double values[4] = {newValue1 + 1.1, newValue2 + 2.1, newValue3 + 3.1, newValue4 + 4.1};

    constexpr int MAX_POINTS = 2500;

    for (int i = 0; i < 4; ++i)
    {
        // 加入新資料點（key = time, value = y）
        graphDataContainers[i]->add(QCPGraphData(currentTime, values[i]));

        // 更新極值
        if (values[i] < minY) minY = values[i];
        if (values[i] > maxY) maxY = values[i];

        // 移除過舊的點（保持最多 MAX_POINTS 個）
        while (graphDataContainers[i]->size() > MAX_POINTS)
        {
            // 移除最小的 key（最舊的）
            graphDataContainers[i]->remove(graphDataContainers[i]->begin()->key);
        }
    }
}

void Switch_Board::onTimeChart()
{
    qint64 now = elapsedTimer.elapsed();

    // X 軸滑動
    if (now > 20000)
    {
        customPlot->xAxis->setRange(now - 20000, now);
    }

    // Y 軸自動範圍
    double margin = (maxY - minY) * 0.12;
    if (margin < 5.0) margin = 5.0;
    customPlot->yAxis->setRange(minY - margin, maxY + margin);

    customPlot->replot(QCustomPlot::rpQueuedReplot);
}

void Switch_Board::onMouseMoveShowTooltip(QMouseEvent *event)
{
    QString tip;
    double mouseKey = customPlot->xAxis->pixelToCoord(event->pos().x());

    for (int i = 0; i < 4; ++i)
    {
        // 先檢查是否靠近曲線（距離 < 20 像素）
        if (graph[i]->selectTest(event->pos(), false) < 20)
        {
            // 找到最接近 mouseKey 的資料迭代器
            auto it = graph[i]->data()->findBegin(mouseKey);

            double closestKey = 0;
            double closestValue = 0;

            // 比較前後兩個點，選最近的
            if (it != graph[i]->data()->constEnd())
            {
                closestKey = it->key;
                closestValue = it->value;

                // 如果不是第一個點，比較前一個
                if (it != graph[i]->data()->constBegin())
                {
                    auto prev = it - 1;
                    double distPrev = qAbs(prev->key - mouseKey);
                    double distCurr = qAbs(closestKey - mouseKey);

                    if (distPrev < distCurr)
                    {
                        closestKey = prev->key;
                        closestValue = prev->value;
                    }
                }

                tip += QString("%1: %2 mA (at %3 ms)\n")
                           .arg(graph[i]->name())
                           .arg(closestValue, 0, 'f', 3)
                           .arg(closestKey, 0, 'f', 0);
            }
        }
    }

    if (!tip.isEmpty())
    {
        QToolTip::showText(event->globalPos(), tip.trimmed(), this);
    }
    else
    {
        QToolTip::hideText();
    }
}

void Switch_Board::setButtonColor(QPushButton* btn, bool active)
{
    if (active) {
        btn->setStyleSheet("background-color: #2ECC71; color: white; border-radius: 4px;");
    } else {
        btn->setStyleSheet("background-color: #BDC3C7; color: black; border-radius: 4px;");
    }
}

void Switch_Board::on_pushButton_OpenDevice_clicked()
{
    app_libusbTh->open(ui->comboBox_DeviceIndex->currentData().toInt());
}

void Switch_Board::on_pushButton_Exit_clicked()
{
    QApplication::quit();
}

void Switch_Board::on_pushButton_Clear_clicked()
{
    ui->textEdit->clear();
}

void Switch_Board::on_pushButton_Save_clicked()
{
    if (ui->textEdit->toPlainText().isEmpty())
        return;

    QString str = QFileDialog::getSaveFileName(this, tr("Save File"),
                                               QCoreApplication::applicationDirPath(),
                                               tr("Text File(*.txt)"));
    if (str.isEmpty())
        return;

    QFile file(str);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream textStream(&file);
    textStream << ui->textEdit->toPlainText();
    file.close();
	
	//        QString filePath = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss") + ".dat";
//        QFile file(filePath);
//        if (!file.open(QIODevice::WriteOnly))
//        {
//            return;
//        }

//        QDataStream data(&file);
//        data.setByteOrder(QDataStream::BigEndian);
//        data.setVersion(QDataStream::Qt_6_2);
//        unsigned char outbuf333[122] = { 0x44,0x0c,0x00,0x06,0x3b,0xd3,0x49,0x0b,0x7a,0xc4,0xda,0x88 };
//        data.writeBytes((char *)outbuf333, sizeof(outbuf333));

        //????
//        QString filePath = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss") + ".dat";
//        QFile file(filePath);
//        if (!file.open(QIODevice::ReadOnly))
//        {
//            return;
//        }

//        QDataStream data(&file);
//        data.setByteOrder(QDataStream::BigEndian);
//        data.setVersion(QDataStream::Qt_6_2);
//        char *rbytes;
//        uint len;
//        data.readBytes(rbytes, len);
//        delete rbytes;
}

void Switch_Board::on_pushButton_SendCmd_clicked()
{
    // QString s = "00 0c 00 06 3b d3 49 0b 7a c4 da 88";
    // QByteArray datas = QByteArray::fromHex(s.toUtf8());
    QByteArray datas = QByteArray::fromHex(ui->lineEdit_SendCmd->text().toUtf8());

    int dataLength = datas.length();
    if (dataLength == 0)
        return;

    unsigned char* data = reinterpret_cast<unsigned char*>(datas.data());

    app_libusbTh->libusb_Tx(data, dataLength);
}

void Switch_Board::on_pushButton_Find_clicked()
{
    int ret;
    struct libusb_device_descriptor descriptor;
    int deviceNum = libusb_get_device_list(NULL, &devList);

    QMap<QString, int> deviceName;
    int deviceCount = 1;

    for (int deviceIndex = 0; deviceIndex < deviceNum; ++deviceIndex)
    {
        ret = libusb_get_device_descriptor(devList[deviceIndex], &descriptor);
        if (ret < 0)
            continue;

        if ((0xcc44 == descriptor.idProduct) && (0xcc84 == descriptor.idVendor))
        {
            deviceName.insert(QString::number(deviceCount), deviceIndex);
            deviceCount++;
        }
    }

    ui->comboBox_DeviceIndex->clear();
    for (const QString &key : deviceName.keys())
    {
        ui->comboBox_DeviceIndex->addItem(key, deviceName.value(key));
    }
}

void Switch_Board::on_pushButton_POWER_0_clicked()
{
    bool flag;
    QByteArray datas(3, 0);

    datas[0] = static_cast<unsigned char>(0xC0);

    if (0b1 & BTS7008Flag)
    {
        BTS7008Flag &= 0b11111110;
        datas[1] = static_cast<unsigned char>(0x00);
        flag = false;
    }
    else
    {
        BTS7008Flag |= 0b1;
        datas[1] = static_cast<unsigned char>(0x01);
        flag = true;
    }

    datas[2] = static_cast<unsigned char>(0x02);

    setButtonColor(ui->pushButton_POWER_0, flag);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_POWER_1_clicked()
{
    bool flag;
    QByteArray datas(3, 0);

    datas[0] = static_cast<unsigned char>(0xC0);

    if (0b10 & BTS7008Flag)
    {
        BTS7008Flag &= 0b11111101;
        datas[1] = static_cast<unsigned char>(0x00);
        flag = false;
    }
    else
    {
        BTS7008Flag |= 0b10;
        datas[1] = static_cast<unsigned char>(0x01);
        flag = true;
    }

    datas[2] = static_cast<unsigned char>(0x03);

    setButtonColor(ui->pushButton_POWER_1, flag);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_POWER_2_clicked()
{
    bool flag;
    QByteArray datas(3, 0);

    datas[0] = static_cast<unsigned char>(0xC0);

    if (0b100 & BTS7008Flag)
    {
        BTS7008Flag &= 0b11111011;
        datas[1] = static_cast<unsigned char>(0x00);
        flag = false;
    }
    else
    {
        BTS7008Flag |= 0b100;
        datas[1] = static_cast<unsigned char>(0x01);
        flag = true;
    }

    datas[2] = static_cast<unsigned char>(0x01);

    setButtonColor(ui->pushButton_POWER_2, flag);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_POWER_3_clicked()
{
    bool flag;
    QByteArray datas(3, 0);

    datas[0] = static_cast<unsigned char>(0xC0);

    if (0b1000 & BTS7008Flag)
    {
        BTS7008Flag &= 0b11110111;
        datas[1] = static_cast<unsigned char>(0x00);
        flag = false;
    }
    else
    {
        BTS7008Flag |= 0b1000;
        datas[1] = static_cast<unsigned char>(0x01);
        flag = true;
    }

    datas[2] = static_cast<unsigned char>(0x00);

    setButtonColor(ui->pushButton_POWER_3, flag);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_DEBUG_0_clicked()
{
    QByteArray datas = QByteArray::fromHex("C1 04");

    setButtonColor(ui->pushButton_DEBUG_0, true);
    setButtonColor(ui->pushButton_DEBUG_1, false);
    setButtonColor(ui->pushButton_DEBUG_2, false);
    setButtonColor(ui->pushButton_DEBUG_3, false);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), datas.size());
}

void Switch_Board::on_pushButton_DEBUG_1_clicked()
{
    QByteArray datas = QByteArray::fromHex("C1 03");

    setButtonColor(ui->pushButton_DEBUG_0, false);
    setButtonColor(ui->pushButton_DEBUG_1, true);
    setButtonColor(ui->pushButton_DEBUG_2, false);
    setButtonColor(ui->pushButton_DEBUG_3, false);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), datas.size());
}

void Switch_Board::on_pushButton_DEBUG_2_clicked()
{
    QByteArray datas = QByteArray::fromHex("C1 02");

    setButtonColor(ui->pushButton_DEBUG_0, false);
    setButtonColor(ui->pushButton_DEBUG_1, false);
    setButtonColor(ui->pushButton_DEBUG_2, true);
    setButtonColor(ui->pushButton_DEBUG_3, false);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), datas.size());
}

void Switch_Board::on_pushButton_DEBUG_3_clicked()
{
    QByteArray datas = QByteArray::fromHex("C1 01");

    setButtonColor(ui->pushButton_DEBUG_0, false);
    setButtonColor(ui->pushButton_DEBUG_1, false);
    setButtonColor(ui->pushButton_DEBUG_2, false);
    setButtonColor(ui->pushButton_DEBUG_3, true);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), datas.size());
}

void Switch_Board::on_pushButton_DEBUG_NONE_clicked()
{
    QByteArray datas = QByteArray::fromHex("C1 00");

    setButtonColor(ui->pushButton_DEBUG_0, false);
    setButtonColor(ui->pushButton_DEBUG_1, false);
    setButtonColor(ui->pushButton_DEBUG_2, false);
    setButtonColor(ui->pushButton_DEBUG_3, false);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), datas.size());
}

void Switch_Board::on_pushButton_POWER_ON_clicked()
{
    QByteArray datas(3, 0);
    datas[0] = 0xC0;
    datas[1] = 0x01;
    datas[2] = 0xff;

    BTS7008Flag = 0b1111;

    setButtonColor(ui->pushButton_POWER_0, true);
    setButtonColor(ui->pushButton_POWER_1, true);
    setButtonColor(ui->pushButton_POWER_2, true);
    setButtonColor(ui->pushButton_POWER_3, true);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_POWER_OFF_clicked()
{
    QByteArray datas(3, 0);
    datas[0] = 0xC0;
    datas[1] = 0x00;
    datas[2] = 0xff;

    BTS7008Flag = 0;

    setButtonColor(ui->pushButton_POWER_0, false);
    setButtonColor(ui->pushButton_POWER_1, false);
    setButtonColor(ui->pushButton_POWER_2, false);
    setButtonColor(ui->pushButton_POWER_3, false);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_WDG_OFF_clicked()
{
    QByteArray datas(3, 0);
    datas[0] = 0xC2;
    datas[1] = 0x00;
    datas[2] = 0xff;

    HCT4066DFlag = 0;

    setButtonColor(ui->pushButton_WDG_0, false);
    setButtonColor(ui->pushButton_WDG_1, false);
    setButtonColor(ui->pushButton_WDG_2, false);
    setButtonColor(ui->pushButton_WDG_3, false);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_WDG_ON_clicked()
{
    QByteArray datas(3, 0);
    datas[0] = 0xC2;
    datas[1] = 0x01;
    datas[2] = 0xff;

    HCT4066DFlag = 0b1111;

    setButtonColor(ui->pushButton_WDG_0, true);
    setButtonColor(ui->pushButton_WDG_1, true);
    setButtonColor(ui->pushButton_WDG_2, true);
    setButtonColor(ui->pushButton_WDG_3, true);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_WDG_0_clicked()
{
    bool flag;
    QByteArray datas(3, 0);

    datas[0] = static_cast<unsigned char>(0xC2);

    if (0b1 & HCT4066DFlag)
    {
        HCT4066DFlag &= 0b11111110;
        datas[1] = static_cast<unsigned char>(0x00);
        flag = false;
    }
    else
    {
        HCT4066DFlag |= 0b1;
        datas[1] = static_cast<unsigned char>(0x01);
        flag = true;
    }

    datas[2] = static_cast<unsigned char>(0x02);

    setButtonColor(ui->pushButton_WDG_0, flag);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_WDG_1_clicked()
{
    bool flag;
    QByteArray datas(3, 0);

    datas[0] = static_cast<unsigned char>(0xC2);

    if (0b10 & HCT4066DFlag)
    {
        HCT4066DFlag &= 0b11111101;
        datas[1] = static_cast<unsigned char>(0x00);
        flag = false;
    }
    else
    {
        HCT4066DFlag |= 0b10;
        datas[1] = static_cast<unsigned char>(0x01);
        flag = true;
    }

    datas[2] = static_cast<unsigned char>(0x03);

    setButtonColor(ui->pushButton_WDG_1, flag);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_WDG_2_clicked()
{
    bool flag;
    QByteArray datas(3, 0);

    datas[0] = static_cast<unsigned char>(0xC2);

    if (0b100 & HCT4066DFlag)
    {
        HCT4066DFlag &= 0b11111011;
        datas[1] = static_cast<unsigned char>(0x00);
        flag = false;
    }
    else
    {
        HCT4066DFlag |= 0b100;
        datas[1] = static_cast<unsigned char>(0x01);
        flag = true;
    }

    datas[2] = static_cast<unsigned char>(0x01);

    setButtonColor(ui->pushButton_WDG_2, flag);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}

void Switch_Board::on_pushButton_WDG_3_clicked()
{
    bool flag;
    QByteArray datas(3, 0);

    datas[0] = static_cast<unsigned char>(0xC2);

    if (0b1000 & HCT4066DFlag)
    {
        HCT4066DFlag &= 0b11110111;
        datas[1] = static_cast<unsigned char>(0x00);
        flag = false;
    }
    else
    {
        HCT4066DFlag |= 0b1000;
        datas[1] = static_cast<unsigned char>(0x01);
        flag = true;
    }

    datas[2] = static_cast<unsigned char>(0x00);

    setButtonColor(ui->pushButton_WDG_3, flag);

    app_libusbTh->libusb_Tx(reinterpret_cast<unsigned char*>(datas.data()), 3);
}
