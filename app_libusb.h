#ifndef APP_LIBUSB_H
#define APP_LIBUSB_H

#include <QThread>
#include <QObject>
#include <QQueue>
#include <libusb-1.0/libusb.h>

extern int counttt;
extern QQueue<unsigned char*> rxData;
extern libusb_device **devList;

class app_libusb : public QThread
{
    Q_OBJECT
public:
    app_libusb();
    void run(void) override;
    void stop(void);
    void open(int deviceIndex);
    void libusb_Tx(unsigned char *data, int length);

signals:
    void libusbSignal(unsigned char* data, int length);

private:
    bool usbThreadIsRun = false;
};

#endif // APP_LIBUSB_H
