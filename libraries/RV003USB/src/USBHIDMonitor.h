#ifndef _RV003USB_USB_HID_MONITOR_H
#define _RV003USB_USB_HID_MONITOR_H

#include "USBHID.h"
#include "Print.h"

extern "C" {
#include "rv003usb.h"
}

#include <string.h>

#define RV003USB_MONITOR_RX_SIZE 32
#define RV003USB_MONITOR_TX_SIZE 64
#define RV003USB_MONITOR_REPORT_SIZE 64
#define RV003USB_MONITOR_REPORT_ID 0xA0
#define RV003USB_MONITOR_INPUT_REPORT_SIZE 8
#define RV003USB_MONITOR_INPUT_REPORT_ID 0xA1
#define RV003USB_MONITOR_NOTIFY_TX_READY 0x01
#define RV003USB_MONITOR_PROTOCOL_VERSION 0x01
#define RV003USB_MONITOR_PAYLOAD_OFFSET 8
#define RV003USB_MONITOR_PAYLOAD_SIZE (RV003USB_MONITOR_REPORT_SIZE - RV003USB_MONITOR_PAYLOAD_OFFSET)
#define RV003USB_MONITOR_WRITE_TIMEOUT_MS 20
#define RV003USB_MONITOR_CMD_PING 0x01
#define RV003USB_MONITOR_CMD_WRITE 0x10
#define RV003USB_MONITOR_CMD_READ 0x11
#define RV003USB_MONITOR_CMD_STATUS 0x12
#define RV003USB_MONITOR_STATUS_OK 0x00
#define RV003USB_MONITOR_STATUS_EMPTY 0x01
#define RV003USB_MONITOR_STATUS_BAD_COMMAND 0x80
#define RV003USB_MONITOR_STATUS_BAD_LENGTH 0x81
#define RV003USB_MONITOR_STATUS_BAD_VERSION 0x82

static volatile uint8_t _rv003usbMonitorRxHead = 0;
static volatile uint8_t _rv003usbMonitorRxTail = 0;
static volatile uint8_t _rv003usbMonitorTxHead = 0;
static volatile uint8_t _rv003usbMonitorTxTail = 0;
static uint8_t _rv003usbMonitorRxBuffer[RV003USB_MONITOR_RX_SIZE];
static uint8_t _rv003usbMonitorTxBuffer[RV003USB_MONITOR_TX_SIZE];
static uint8_t _rv003usbMonitorSetHeader[RV003USB_MONITOR_PAYLOAD_OFFSET];
static uint8_t _rv003usbMonitorReportTxFrame[RV003USB_MONITOR_REPORT_SIZE];

class USBHIDMonitor_ : public Print {
private:
#if defined(CORE_LIGHTWEIGHT_PRINT)
    static size_t write_callback(void *ctx, uint8_t c)
    {
        return ((USBHIDMonitor_ *)ctx)->write(c);
    }
#endif

public:
#if defined(CORE_LIGHTWEIGHT_PRINT)
    USBHIDMonitor_() : Print(write_callback, this) {}
#else
    USBHIDMonitor_() : Print() {}
#endif

    void begin();
    void end();
    int available();
    int availableForWrite();
    int read();
    int peek();
    void flush();
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    using Print::write;
};

static USBHIDMonitor_ USBHIDMonitor;

static uint8_t rv003usbMonitorRingCount(uint8_t head, uint8_t tail)
{
    return (uint8_t)(head - tail);
}

static bool rv003usbMonitorRingPush(volatile uint8_t *head, volatile uint8_t *tail, uint8_t *buffer, uint8_t size, uint8_t value)
{
    uint8_t next = (uint8_t)(*head + 1);
    if ((uint8_t)(next - *tail) > (uint8_t)(size - 1)) {
        return false;
    }
    buffer[*head & (uint8_t)(size - 1)] = value;
    *head = next;
    return true;
}

static int rv003usbMonitorRingPop(volatile uint8_t *head, volatile uint8_t *tail, uint8_t *buffer, uint8_t size)
{
    if (*head == *tail) {
        return -1;
    }
    uint8_t value = buffer[*tail & (uint8_t)(size - 1)];
    *tail = (uint8_t)(*tail + 1);
    return value;
}

static int rv003usbMonitorRingPeek(volatile uint8_t *head, volatile uint8_t *tail, uint8_t *buffer, uint8_t size)
{
    if (*head == *tail) {
        return -1;
    }
    return buffer[*tail & (uint8_t)(size - 1)];
}

static void rv003usbMonitorPrepareResponse(uint8_t command, uint8_t sequence, uint8_t status, const uint8_t *payload, uint8_t length)
{
    memset(_rv003usbMonitorReportTxFrame, 0, sizeof(_rv003usbMonitorReportTxFrame));
    _rv003usbMonitorReportTxFrame[0] = RV003USB_MONITOR_REPORT_ID;
    _rv003usbMonitorReportTxFrame[1] = RV003USB_MONITOR_PROTOCOL_VERSION;
    _rv003usbMonitorReportTxFrame[2] = command;
    _rv003usbMonitorReportTxFrame[3] = sequence;
    _rv003usbMonitorReportTxFrame[4] = length;
    _rv003usbMonitorReportTxFrame[5] = status;

    if ((payload != nullptr) && (length > 0)) {
        memcpy(_rv003usbMonitorReportTxFrame + RV003USB_MONITOR_PAYLOAD_OFFSET, payload, length);
    }
}

static void rv003usbMonitorPrepareSimpleResponse(uint8_t command, uint8_t sequence, uint8_t status)
{
    rv003usbMonitorPrepareResponse(command, sequence, status, nullptr, 0);
}

static void rv003usbMonitorPreparePingResponse(uint8_t sequence)
{
    memset(_rv003usbMonitorReportTxFrame, 0, sizeof(_rv003usbMonitorReportTxFrame));
    _rv003usbMonitorReportTxFrame[0] = RV003USB_MONITOR_REPORT_ID;
    _rv003usbMonitorReportTxFrame[1] = RV003USB_MONITOR_PROTOCOL_VERSION;
    _rv003usbMonitorReportTxFrame[2] = RV003USB_MONITOR_CMD_PING;
    _rv003usbMonitorReportTxFrame[3] = sequence;
    _rv003usbMonitorReportTxFrame[4] = 4;
    _rv003usbMonitorReportTxFrame[5] = RV003USB_MONITOR_STATUS_OK;
    _rv003usbMonitorReportTxFrame[8] = 'P';
    _rv003usbMonitorReportTxFrame[9] = 'O';
    _rv003usbMonitorReportTxFrame[10] = 'N';
    _rv003usbMonitorReportTxFrame[11] = 'G';
}

static bool rv003usbMonitorHeaderIsValid()
{
    return (_rv003usbMonitorSetHeader[0] == RV003USB_MONITOR_REPORT_ID)
        && (_rv003usbMonitorSetHeader[1] == RV003USB_MONITOR_PROTOCOL_VERSION)
        && (_rv003usbMonitorSetHeader[4] <= RV003USB_MONITOR_PAYLOAD_SIZE);
}

static void rv003usbMonitorAcceptSetData(int offset, const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
        int frameOffset = offset + i;
        if (frameOffset < RV003USB_MONITOR_PAYLOAD_OFFSET) {
            _rv003usbMonitorSetHeader[frameOffset] = data[i];
            continue;
        }

        if (!rv003usbMonitorHeaderIsValid() || _rv003usbMonitorSetHeader[2] != RV003USB_MONITOR_CMD_WRITE) {
            continue;
        }

        uint8_t payloadIndex = (uint8_t)(frameOffset - RV003USB_MONITOR_PAYLOAD_OFFSET);
        if (payloadIndex >= _rv003usbMonitorSetHeader[4]) {
            continue;
        }
        rv003usbMonitorRingPush(&_rv003usbMonitorRxHead, &_rv003usbMonitorRxTail, _rv003usbMonitorRxBuffer, RV003USB_MONITOR_RX_SIZE, data[i]);
    }
}

extern "C" uint8_t rv003usbMonitorWriteFromHost(const uint8_t *data, uint8_t len)
{
    uint8_t written = 0;
    while (written < len) {
        if (!rv003usbMonitorRingPush(&_rv003usbMonitorRxHead, &_rv003usbMonitorRxTail, _rv003usbMonitorRxBuffer, RV003USB_MONITOR_RX_SIZE, data[written])) {
            break;
        }
        written++;
    }
    return written;
}

extern "C" uint8_t rv003usbMonitorReadForHost(uint8_t *data, uint8_t maxLen)
{
    uint8_t read = 0;
    while (read < maxLen) {
        int value = rv003usbMonitorRingPop(&_rv003usbMonitorTxHead, &_rv003usbMonitorTxTail, _rv003usbMonitorTxBuffer, RV003USB_MONITOR_TX_SIZE);
        if (value < 0) {
            break;
        }
        data[read++] = (uint8_t)value;
    }
    return read;
}

extern "C" uint8_t rv003usbMonitorAvailableForHostWrite(void)
{
    return (uint8_t)((RV003USB_MONITOR_RX_SIZE - 1) - rv003usbMonitorRingCount(_rv003usbMonitorRxHead, _rv003usbMonitorRxTail));
}

extern "C" uint8_t rv003usbMonitorHasTxData(void)
{
    return rv003usbMonitorRingCount(_rv003usbMonitorTxHead, _rv003usbMonitorTxTail) > 0;
}

static void rv003usbMonitorProcessFrame()
{
    uint8_t command = _rv003usbMonitorSetHeader[2];
    uint8_t sequence = _rv003usbMonitorSetHeader[3];
    uint8_t payloadLength = _rv003usbMonitorSetHeader[4];

    if (_rv003usbMonitorSetHeader[0] != RV003USB_MONITOR_REPORT_ID) {
        rv003usbMonitorPrepareSimpleResponse(command, sequence, RV003USB_MONITOR_STATUS_BAD_COMMAND);
        return;
    }

    if (_rv003usbMonitorSetHeader[1] != RV003USB_MONITOR_PROTOCOL_VERSION) {
        rv003usbMonitorPrepareSimpleResponse(command, sequence, RV003USB_MONITOR_STATUS_BAD_VERSION);
        return;
    }

    if (payloadLength > RV003USB_MONITOR_PAYLOAD_SIZE) {
        rv003usbMonitorPrepareSimpleResponse(command, sequence, RV003USB_MONITOR_STATUS_BAD_LENGTH);
        return;
    }

    switch (command) {
    case RV003USB_MONITOR_CMD_PING:
        rv003usbMonitorPreparePingResponse(sequence);
        break;
    case RV003USB_MONITOR_CMD_WRITE:
        rv003usbMonitorPrepareSimpleResponse(command, sequence, RV003USB_MONITOR_STATUS_OK);
        break;
    case RV003USB_MONITOR_CMD_READ: {
        uint8_t payload[RV003USB_MONITOR_PAYLOAD_SIZE];
        uint8_t read = rv003usbMonitorReadForHost(payload, sizeof(payload));
        if (read == 0) {
            rv003usbMonitorPrepareSimpleResponse(command, sequence, RV003USB_MONITOR_STATUS_EMPTY);
        } else {
            rv003usbMonitorPrepareResponse(command, sequence, RV003USB_MONITOR_STATUS_OK, payload, read);
        }
        break;
    }
    case RV003USB_MONITOR_CMD_STATUS: {
        uint8_t statusPayload[2] = {0, rv003usbMonitorAvailableForHostWrite()};
        rv003usbMonitorPrepareResponse(command, sequence, RV003USB_MONITOR_STATUS_OK, statusPayload, sizeof(statusPayload));
        break;
    }
    default:
        rv003usbMonitorPrepareSimpleResponse(command, sequence, RV003USB_MONITOR_STATUS_BAD_COMMAND);
        break;
    }
}

inline void USBHIDMonitor_::begin()
{
    USBHID.begin();
}

inline void USBHIDMonitor_::end()
{
}

inline int USBHIDMonitor_::available()
{
    return rv003usbMonitorRingCount(_rv003usbMonitorRxHead, _rv003usbMonitorRxTail);
}

inline int USBHIDMonitor_::availableForWrite()
{
    return (RV003USB_MONITOR_TX_SIZE - 1) - rv003usbMonitorRingCount(_rv003usbMonitorTxHead, _rv003usbMonitorTxTail);
}

inline int USBHIDMonitor_::read()
{
    return rv003usbMonitorRingPop(&_rv003usbMonitorRxHead, &_rv003usbMonitorRxTail, _rv003usbMonitorRxBuffer, RV003USB_MONITOR_RX_SIZE);
}

inline int USBHIDMonitor_::peek()
{
    return rv003usbMonitorRingPeek(&_rv003usbMonitorRxHead, &_rv003usbMonitorRxTail, _rv003usbMonitorRxBuffer, RV003USB_MONITOR_RX_SIZE);
}

inline void USBHIDMonitor_::flush()
{
}

inline size_t USBHIDMonitor_::write(uint8_t c)
{
    uint32_t start = millis();
    do {
        if (rv003usbMonitorRingPush(&_rv003usbMonitorTxHead, &_rv003usbMonitorTxTail, _rv003usbMonitorTxBuffer, RV003USB_MONITOR_TX_SIZE, c)) {
            return 1;
        }
    } while ((uint32_t)(millis() - start) < RV003USB_MONITOR_WRITE_TIMEOUT_MS);

    return 0;
}

inline size_t USBHIDMonitor_::write(const uint8_t *buffer, size_t size)
{
    size_t written = 0;
    while (written < size) {
        if (!write(buffer[written])) {
            break;
        }
        written++;
    }
    return written;
}

extern "C" void usb_handle_hid_get_report_start(struct usb_endpoint *e, int reqLen, uint32_t lValueLSBIndexMSB)
{
    (void)lValueLSBIndexMSB;
    if (reqLen > (int)sizeof(_rv003usbMonitorReportTxFrame)) {
        reqLen = sizeof(_rv003usbMonitorReportTxFrame);
    }
    e->opaque = _rv003usbMonitorReportTxFrame;
    e->max_len = reqLen;
}

extern "C" void usb_handle_hid_set_report_start(struct usb_endpoint *e, int reqLen, uint32_t lValueLSBIndexMSB)
{
    (void)lValueLSBIndexMSB;
    memset(_rv003usbMonitorSetHeader, 0, sizeof(_rv003usbMonitorSetHeader));
    if (reqLen > RV003USB_MONITOR_REPORT_SIZE) {
        reqLen = RV003USB_MONITOR_REPORT_SIZE;
    }
    e->count = 0;
    e->max_len = reqLen;
}

extern "C" void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint, uint8_t *data, int len, struct rv003usb_internal *ist)
{
    (void)current_endpoint;
    (void)ist;
    int offset = e->count << 3;
    int toCopy = e->max_len - offset;
    if (toCopy > len) {
        toCopy = len;
    }
    if (toCopy <= 0) {
        return;
    }

    rv003usbMonitorAcceptSetData(offset, data, toCopy);
    e->count++;
    if ((e->count << 3) >= e->max_len) {
        rv003usbMonitorProcessFrame();
    }
}

#endif // _RV003USB_USB_HID_MONITOR_H
