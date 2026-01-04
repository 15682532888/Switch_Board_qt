#ifndef SWITCH_BOARD_H
#define SWITCH_BOARD_H

#include <QMainWindow>
#include <QPushButton>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QPen>
#include <QVector>
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
    void rexda(unsigned char* data, int length);

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

    void tooltipPoint(const QPointF &point);  // 工具提示槽

    void updateData(double newValue1, double newValue2, double newValue3, double newValue4);

    void onTimeChart();

private:
    Ui::Switch_Board *ui;
    app_libusb* app_libusbTh;
    void setButtonColor(QPushButton* btn, bool active);

    QSplineSeries *series[4];
    QChart *chart;
    QChartView *chartView;
    QValueAxis *axisX;
    QValueAxis *axisY;
    // QVector<std::pair<double, double>> dataPoints;
    QVector<QVector<QPointF>> dataPointsList;  // 存储多条曲线的数据
    QElapsedTimer elapsedTimer;
    QTimer *timerChart;
};
#endif // SWITCH_BOARD_H
