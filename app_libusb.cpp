#include "app_libusb.h"
#include "qdebug.h"

#include <QDebug>

libusb_device_handle* devh;
struct libusb_device_descriptor descriptor;
libusb_device* device;
libusb_device **devList;
static struct libusb_transfer* in_transfer;
unsigned char inbuf[4096];
int outBufferSize;

unsigned char myBuffer[1024];
unsigned int nextIndex = 0;

QQueue<unsigned char*> rxData;

static void cb_in(struct libusb_transfer* transfer);

app_libusb::app_libusb()
{
    usbThreadIsRun = true;
}

void app_libusb::stop(void)
{
    usbThreadIsRun = false;
}

void app_libusb::open(int deviceIndex)
{
    int ret;

    ret = libusb_open(devList[deviceIndex], &devh);
    ret = libusb_claim_interface(devh, 0);
    // qDebug() << ret;

    in_transfer = libusb_alloc_transfer(0);
    in_transfer->dev_handle = devh;
    in_transfer->endpoint = 0x81;
    in_transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
    in_transfer->timeout = 0;
    in_transfer->buffer = inbuf;
    in_transfer->length = sizeof(inbuf);
    in_transfer->user_data = NULL;
    in_transfer->callback = cb_in;

    ret = libusb_submit_transfer(in_transfer);
    // qDebug() << ret;
}

void app_libusb::libusb_Tx(unsigned char *data, int length)
{
    (void)libusb_bulk_transfer(devh, 0x01, data, length, &outBufferSize, 5000);
}

void app_libusb::run(void)
{
    int ret;
    struct timeval timeval0;

    timeval0.tv_sec = 2;
    timeval0.tv_usec = 0;

    while (usbThreadIsRun)
    {
        ret = libusb_handle_events_timeout_completed(NULL, &timeval0, NULL);

        if (!rxData.isEmpty())
        {
            unsigned char* tempDate = rxData.dequeue();
            QString s;

            for (int i = 0; i < 10; i++)
            {
                s.append(QString("%1").arg(tempDate[i], 2, 16, QChar('0')).toUpper() + " ");
            }

            emit libusbSignal(s);
        }
    }
}

static void cb_in(struct libusb_transfer* transfer)
{
    int ret;

    int dataLength = transfer->actual_length;
    if (1024 < dataLength + nextIndex)
    {
        nextIndex = 0;
    }
    memcpy(&myBuffer[nextIndex], transfer->buffer, dataLength);

    rxData.enqueue(&myBuffer[nextIndex]);
    nextIndex += dataLength;

    ret = libusb_submit_transfer(in_transfer);
}
