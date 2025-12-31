#include "switch_board.h"
#include "./ui_switch_board.h"
#include <QProcess>
#include <qfiledialog.h>
#include <qt_windows.h>
#include <QTimer>
#include <QDateTime>

int timerId;
unsigned char dddate[30] = {0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x08};
unsigned int txLength;
unsigned char BTS7008Flag = 0;

Switch_Board::Switch_Board(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Switch_Board)
{
    ui->setupUi(this);

    app_libusbTh = new app_libusb();

    connect(app_libusbTh, SIGNAL(libusbSignal(QString)), this, SLOT(rexda(QString)));

    (void)libusb_init(NULL);

    app_libusbTh->start();
}

Switch_Board::~Switch_Board()
{
    delete ui;
    app_libusbTh->stop();
    app_libusbTh->quit();
    (void)app_libusbTh->wait();
    delete app_libusbTh;
}

void Switch_Board::rexda(QString s)
{
    ui->textEdit->append("Rx: " + s);
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


void Switch_Board::setButtonColor(QPushButton* btn, bool active) {
    if (active) {
        // 绿色代表开启
        btn->setStyleSheet("background-color: #2ECC71; color: white; border-radius: 4px;");
    } else {
        // 灰色代表关闭
        btn->setStyleSheet("background-color: #BDC3C7; color: black; border-radius: 4px;");
    }
}

