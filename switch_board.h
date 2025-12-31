#ifndef SWITCH_BOARD_H
#define SWITCH_BOARD_H

#include <QMainWindow>
#include <QPushButton>
#include "app_libusb.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Switch_Board; }
QT_END_NAMESPACE

class Switch_Board : public QMainWindow
{
    Q_OBJECT

public:
    Switch_Board(QWidget *parent = nullptr);
    ~Switch_Board();
	
signals:
    void stopUpdateTemp(void);

private slots:
    void rexda(QString);

    void on_pushButton_OpenDevice_clicked();

    void on_pushButton_Exit_clicked();

    void on_pushButton_Clear_clicked();

    void on_pushButton_Save_clicked();

    void on_pushButton_SendCmd_clicked();

    void on_pushButton_Find_clicked();
    void on_pushButton_POWER_0_clicked();

    void on_pushButton_POWER_1_clicked();

    void on_pushButton_POWER_2_clicked();

    void on_pushButton_POWER_3_clicked();

    void on_pushButton_DEBUG_0_clicked();

    void on_pushButton_DEBUG_1_clicked();

    void on_pushButton_DEBUG_2_clicked();

    void on_pushButton_DEBUG_3_clicked();

    void on_pushButton_DEBUG_NONE_clicked();

    void on_pushButton_POWER_ON_clicked();

    void on_pushButton_POWER_OFF_clicked();

private:
    Ui::Switch_Board *ui;
    app_libusb* app_libusbTh;
    void setButtonColor(QPushButton* btn, bool active);
};
#endif // SWITCH_BOARD_H
