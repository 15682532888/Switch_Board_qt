#include "switch_board.h"
#include "ui_switch_board.h"
#include <QProcess>
#include <qfiledialog.h>
#include <qt_windows.h>
#include <QTimer>
#include <QDateTime>
#include <QToolTip>
#include <QTimer>

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

    // 创建图表
    chart = new QChart();
    chart->setTitle("实时曲线");
    chart->setAnimationOptions(QChart::NoAnimation);

    // 创建4条曲线
    for (int i = 0; i < 4; i++) {
        series[i] = new QLineSeries();
        series[i]->setUseOpenGL(true);
        series[i]->setName(seriesNames[i]);

        QPen pen;
        pen.setColor(colors[i]);
        pen.setWidth(2);
        series[i]->setPen(pen);

        chart->addSeries(series[i]);

        // 连接鼠标悬停信号
        connect(series[i], &QXYSeries::hovered, this, &Switch_Board::tooltipPoint);
        //鼠标离开时隐藏提示
        connect(series[i], &QXYSeries::clicked, this, []() { QToolTip::hideText(); });
    }

    // X 轴（时间）
    axisX = new QValueAxis();
    axisX->setRange(0, 20000);
    axisX->setTitleText("时间 (ms)");
    chart->addAxis(axisX, Qt::AlignmentFlag::AlignBottom);

    // Y 轴（值）
    axisY = new QValueAxis();
    axisY->setRange(0, 200);
    // axisY->setTitleText("A");
    chart->addAxis(axisY, Qt::AlignmentFlag::AlignLeft);

    // 将每条曲线附加到坐标轴
    for (int i = 0; i < 4; i++) {
        series[i]->attachAxis(axisX);
        series[i]->attachAxis(axisY);
    }

    // 创建视图
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing, false);  // 追求性能，不使用抗锯齿
    chartView->setRubberBand(QChartView::RectangleRubberBand);  // 矩形选区缩放
    chartView->setInteractive(true);  // 确保可交互

    QVBoxLayout *chartLayout = new QVBoxLayout(ui->widget);
    ui->widget->setLayout(chartLayout);
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->widget->layout());  // 获取布局
    layout->addWidget(chartView);  // 添加 chartView 到布局（占用全部空间）

    // 数据存储：每条曲线都有自己的数据点
    for (int i = 0; i < 4; i++) {
        dataPointsList.append(QVector<QPointF>());
        dataPointsList[i].reserve(5000);
    }
    // dataPoints.reserve(5000);
    elapsedTimer.start();
    timerChart = new QTimer(this);
    timerChart->setInterval(300);
    connect(timerChart, &QTimer::timeout, this, &Switch_Board::onTimeChart);
    timerChart->start();

ui->textEdit->document()->setMaximumBlockCount(50000); // 只保留最近500行数据


    app_libusbTh = new app_libusb();

    connect(app_libusbTh, app_libusb::libusbSignal, this, Switch_Board::rexda);

    (void)libusb_init(NULL);

    app_libusbTh->start();
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

    if ((0x5a == data[0]) && (0x5a == data[1]))
    {
        // qDebug() << 3.621 * ((0x0f & data[9]) << 8 | (data[8] > 0x3c ? (data[8] - 0x3c) : 0));
        updateData((3.621 * ((0x0f & data[3]) << 8 | (data[2] > 0x3c ? (data[2] - 0x3c) : 0))),
                   (3.621 * ((0x0f & data[5]) << 8 | (data[4] > 0x3c ? (data[4] - 0x3c) : 0))),
                   (3.621 * ((0x0f & data[7]) << 8 | (data[6] > 0x3c ? (data[6] - 0x3c) : 0))),
                   (3.621 * ((0x0f & data[9]) << 8 | (data[8] > 0x3c ? (data[8] - 0x3c) : 0))));
    }
}


void thisCB(uint uTimerID, uint uMsg, HANDLE_PTR dwUser, HANDLE_PTR dw1, HANDLE_PTR dw2)
{

}


void Switch_Board::on_pushButton_OpenDevice_clicked()
{
    app_libusbTh->open(ui->comboBox_DeviceIndex->currentData().toInt());
}

void Switch_Board::on_pushButton_Exit_clicked()
{
    QApplication* app;

    app->quit();
}


void Switch_Board::on_pushButton_Clear_clicked()
{
    ui->textEdit->clear();
}


void Switch_Board::on_pushButton_Save_clicked()
{
    if (ui->textEdit->toPlainText() != "")
    {
        //???
        QFileDialog fileDialog;
        QString str = fileDialog.getSaveFileName(this, tr("Save File"), QCoreApplication::applicationDirPath(), tr("Text File(*.txt)"));

        if (str == "")
        {
            return;
        }

        QFile filename(str);
        if (!filename.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            return;
        }

        QTextStream textStream(&filename);
        QString str1 = ui->textEdit->toPlainText();
        textStream << str1;
        filename.close();

        //????
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
}


void Switch_Board::on_pushButton_SendCmd_clicked()
{
    // QString s = "00 0c 00 06 3b d3 49 0b 7a c4 da 88";
    // QByteArray datas = QByteArray::fromHex(s.toUtf8());
    QByteArray datas = QByteArray::fromHex(ui->lineEdit_SendCmd->text().toUtf8());

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_Find_clicked()
{
    int ret;
    struct libusb_device_descriptor descriptor;
    int deviceNum = libusb_get_device_list(NULL, &devList);

    QMap<QString, int> deviceName;
    int deviceCount = 1;
    for (int deviceIndex = 0; deviceIndex < deviceNum; deviceIndex++)
    {
        ret = libusb_get_device_descriptor(devList[deviceIndex], &descriptor);
        if ((0xcc44 == descriptor.idProduct) && (0xcc84 == descriptor.idVendor))
        {
            deviceName.insert(QString::number(deviceCount), deviceIndex);
            deviceCount++;
        }
    }

    ui->comboBox_DeviceIndex->clear();
    foreach (const QString &str, deviceName.keys()) {
        ui->comboBox_DeviceIndex->addItem(str, deviceName.value(str));
    }
}


void Switch_Board::on_pushButton_POWER_0_clicked()
{
    bool flag;
    QByteArray datas;
    datas.resize(3);

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

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_POWER_1_clicked()
{
    bool flag;
    QByteArray datas;
    datas.resize(3);

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

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_POWER_2_clicked()
{
    bool flag;
    QByteArray datas;
    datas.resize(3);

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

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_POWER_3_clicked()
{
    bool flag;
    QByteArray datas;
    datas.resize(3);

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

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_DEBUG_0_clicked()
{
    QString s = "C1 04";
    QByteArray datas = QByteArray::fromHex(s.toUtf8());

    setButtonColor(ui->pushButton_DEBUG_0, true);
    setButtonColor(ui->pushButton_DEBUG_1, false);
    setButtonColor(ui->pushButton_DEBUG_2, false);
    setButtonColor(ui->pushButton_DEBUG_3, false);

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_DEBUG_1_clicked()
{
    QString s = "C1 03";
    QByteArray datas = QByteArray::fromHex(s.toUtf8());

    setButtonColor(ui->pushButton_DEBUG_0, false);
    setButtonColor(ui->pushButton_DEBUG_1, true);
    setButtonColor(ui->pushButton_DEBUG_2, false);
    setButtonColor(ui->pushButton_DEBUG_3, false);

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_DEBUG_2_clicked()
{
    QString s = "C1 02";
    QByteArray datas = QByteArray::fromHex(s.toUtf8());

    setButtonColor(ui->pushButton_DEBUG_0, false);
    setButtonColor(ui->pushButton_DEBUG_1, false);
    setButtonColor(ui->pushButton_DEBUG_2, true);
    setButtonColor(ui->pushButton_DEBUG_3, false);

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_DEBUG_3_clicked()
{
    QString s = "C1 01";
    QByteArray datas = QByteArray::fromHex(s.toUtf8());

    setButtonColor(ui->pushButton_DEBUG_0, false);
    setButtonColor(ui->pushButton_DEBUG_1, false);
    setButtonColor(ui->pushButton_DEBUG_2, false);
    setButtonColor(ui->pushButton_DEBUG_3, true);

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_DEBUG_NONE_clicked()
{
    QString s = "C1 00";
    QByteArray datas = QByteArray::fromHex(s.toUtf8());

    setButtonColor(ui->pushButton_DEBUG_0, false);
    setButtonColor(ui->pushButton_DEBUG_1, false);
    setButtonColor(ui->pushButton_DEBUG_2, false);
    setButtonColor(ui->pushButton_DEBUG_3, false);

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_POWER_ON_clicked()
{
    QByteArray datas;
    datas.resize(3);

    datas[0] = static_cast<unsigned char>(0xC0);
    datas[1] = static_cast<unsigned char>(0x01);
    datas[2] = static_cast<unsigned char>(0xff);
    BTS7008Flag = 0b1111;

    setButtonColor(ui->pushButton_POWER_0, true);
    setButtonColor(ui->pushButton_POWER_1, true);
    setButtonColor(ui->pushButton_POWER_2, true);
    setButtonColor(ui->pushButton_POWER_3, true);

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_POWER_OFF_clicked()
{
    QByteArray datas;
    datas.resize(3);

    datas[0] = static_cast<unsigned char>(0xC0);
    datas[1] = static_cast<unsigned char>(0x00);
    datas[2] = static_cast<unsigned char>(0xff);
    BTS7008Flag = 0;

    setButtonColor(ui->pushButton_POWER_0, false);
    setButtonColor(ui->pushButton_POWER_1, false);
    setButtonColor(ui->pushButton_POWER_2, false);
    setButtonColor(ui->pushButton_POWER_3, false);

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::tooltipPoint(const QPointF &point)
{
    // 显示精确数值（X: 时间, Y: 值）
    QToolTip::showText(QCursor::pos(), QString("%3 mA").arg(point.y(), 0, 'f', 3), this);
}


void Switch_Board::updateData(double newValue1, double newValue2, double newValue3, double newValue4)
{
    qint64 currentTime = elapsedTimer.elapsed();

    // 更新每条曲线的数据
    dataPointsList[0].append(QPointF(currentTime, newValue1+1.1));
    dataPointsList[1].append(QPointF(currentTime, newValue2+2.1));
    dataPointsList[2].append(QPointF(currentTime, newValue3+3.1));
    dataPointsList[3].append(QPointF(currentTime, newValue4+4.1));

    // 保持最近 5000 个点
    for (int i = 0; i < 4; i++) {
        if (dataPointsList[i].size() > 5000) {
            dataPointsList[i].removeFirst();
        }
    }

    // ui->label_4->setText(QString::number(elapsedTimer.elapsed() - elapsedTimerStart) + " ms");
    elapsedTimerStart = currentTime;
}


void Switch_Board::onTimeChart()
{
    // 批量替换数据，避免 clear() 带来的白屏瞬间
    for (int i = 0; i < 4; i++) {
        if (dataPointsList[i].size() > 0) {
            series[i]->replace(dataPointsList[i]);
        }
    }

    qint64 time = elapsedTimer.elapsed();

    // 只有当时间轴超出范围时才平滑移动窗口
    if (time > 20000) {
        // 使用这种方式会让坐标轴随时间平滑向右平移
        axisX->setRange(time - 20000, time);
    }

    // 3. 优化 Y 轴自动缩放逻辑
    // 只有在必要时才遍历 20000 个点，或者限制遍历频率
    double minY = 0;
    double maxY = 200;
    for (const auto& points : dataPointsList) {
        if (points.isEmpty()) continue;
        for (const auto& point : points) {
            if (point.y() < minY) minY = point.y();
            if (point.y() > maxY) maxY = point.y();
        }
    }
    double margin = (maxY - minY) * 0.1;
    axisY->setRange(minY - margin, maxY + margin);
}


void Switch_Board::setButtonColor(QPushButton* btn, bool active) {
    if (active) {
        // 绿色代表开启
        btn->setStyleSheet("background-color: #2ECC71; color: white; border-radius: 4px;");
    } else {
        // 灰色代表关闭
        btn->setStyleSheet("background-color: #BDC3C7; color: black; border-radius: 4px;");
    }
}


void Switch_Board::on_pushButton_WDG_OFF_clicked()
{
    QByteArray datas;
    datas.resize(3);

    datas[0] = static_cast<unsigned char>(0xC2);
    datas[1] = static_cast<unsigned char>(0x00);
    datas[2] = static_cast<unsigned char>(0xff);
    HCT4066DFlag = 0;

    setButtonColor(ui->pushButton_WDG_0, false);
    setButtonColor(ui->pushButton_WDG_1, false);
    setButtonColor(ui->pushButton_WDG_2, false);
    setButtonColor(ui->pushButton_WDG_3, false);

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_WDG_ON_clicked()
{
    QByteArray datas;
    datas.resize(3);

    datas[0] = static_cast<unsigned char>(0xC2);
    datas[1] = static_cast<unsigned char>(0x01);
    datas[2] = static_cast<unsigned char>(0xff);
    HCT4066DFlag = 0b1111;

    setButtonColor(ui->pushButton_WDG_0, true);
    setButtonColor(ui->pushButton_WDG_1, true);
    setButtonColor(ui->pushButton_WDG_2, true);
    setButtonColor(ui->pushButton_WDG_3, true);

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_WDG_0_clicked()
{
    bool flag;
    QByteArray datas;
    datas.resize(3);

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

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_WDG_1_clicked()
{
    bool flag;
    QByteArray datas;
    datas.resize(3);

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

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_WDG_2_clicked()
{
    bool flag;
    QByteArray datas;
    datas.resize(3);

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

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}


void Switch_Board::on_pushButton_WDG_3_clicked()
{
    bool flag;
    QByteArray datas;
    datas.resize(3);

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

    int dataLength = datas.length();
    unsigned char* data = (unsigned char*)datas.data();

    app_libusbTh->libusb_Tx(data, dataLength);
}

