//
//  VoodooWacom4814.h
//  VoodooI2C
//
//  Created by Joe Litko on 10/20/16.
//  Copyright © 2016 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooWacomDevice_h
#define VoodooWacomDevice_h


#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOLocks.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOTimerEventSource.h>
#include "VoodooI2CDevice.h"
#include "csgesture.h"

#define __le16 UInt16
#define __le32 UInt32

#define HID_MAX_DESCRIPTOR_SIZE 4096

#define I2C_HID_PWR_ON 0x00
#define I2C_HID_PWR_SLEEP 0x01

#define WACOM_PEN_DOWN 0x21
#define WACOM_FINGERTOUCH 0x0c
#define WACOM_PENINPUT 0x06
#define WACOM_SINGLETOUCH 0x01
#define WACOM_TWOTOUCH 0x02
#define WACOM_THREETOUCH 0x03
#define WACOM_FOURTOUCH 0x04

#define WACOM_FINGER_DATA_OFFSET 6
// #define WACOM_TOUCH_INFO_OFFSET 6
#define WACOM_HOVER_INFO_OFFSET 5
#define WACOM_MAX_FINGERS 5
#define WACOM_FINGER_DATA_LEN 7
#define WACOM_MAX_INPUT_LEN 42

#define KBD_LCONTROL_BIT 1

class VoodooI2C;
class VoodooWacomWrapper;
class IOBufferMemoryDescriptor;

class VoodooWacomDevice : public VoodooI2CDevice
{
    typedef IOService super;
    OSDeclareDefaultStructors(VoodooWacomDevice);
    
private:
    CSGesture* _wrapper;
    VoodooWacomWrapper* _wacwrapper;
    
    void initialize_wrapper(void);
    void destroy_wrapper(void);
    
protected:
    VoodooI2C* _controller;
    
public:
    virtual bool attach(IOService * provider, IOService* child) override;
    virtual void detach(IOService * provider) override;
    void stop(IOService* device) override;
    
    
    typedef struct {
        
        unsigned short addr;
        
        void* _dev;
        
        IOWorkLoop* workLoop;
        //IOCommandGate* commandGate;
        
        IOInterruptEventSource *interruptSource;
        
        IOACPIPlatformDevice* provider;
        
        IOTimerEventSource* timerSource;
        
        char* name;
        
        bool reading;
        
    } I2CDevice;
    
    I2CDevice* hid_device;
    
    union command {
        UInt8 data[0];
        struct cmd {
            __le16 reg;
            UInt8 reportTypeID;
            UInt8 opcode;
        } c;
    };
    
    struct i2c_hid_desc {
        __le16 wHIDDescLength;
        __le16 bcdVersion;
        __le16 wReportDescLength;
        __le16 wReportDescRegister;
        __le16 wInputRegister;
        __le16 wMaxInputLength;
        __le16 wOutputRegister;
        __le16 wMaxOutputLength;
        __le16 wCommandRegister;
        __le16 wDataRegister;
        __le16 wVendorID;
        __le16 wProductID;
        __le16 wVersionID;
        __le32 reserved;
    } __packed;
    
    struct i2c_hid_platform_data {
        UInt16 hid_descriptor_address;
    };
    
    typedef struct {
        I2CDevice *client;
        
        
        union {
            UInt8 hdesc_buffer[sizeof(struct i2c_hid_desc)];
            struct i2c_hid_desc hdesc;
        };
        
        __le16 wHIDDescRegister;
        
        UInt bufsize;
        char *inbuf;
        char *rawbuf;
        char *cmdbuf;
        char *argsbuf;
        
        unsigned long flags;
        
        struct i2c_hid_platform_data pdata;
    } i2c_hid;
    
    i2c_hid *ihid;
    
    struct i2c_hid_cmd {
        UInt registerIndex;
        UInt8 opcode;
        UInt length;
        bool wait;
    };
    
    struct i2c_msg {
        UInt16 addr;
        UInt16 flags;
        UInt16 len;
        UInt8 *buf;
        
#define I2C_M_TEN 0x0010
#define I2C_M_RD 0x0001
#define I2C_M_RECV_LEN 0x0400
        
#define I2C_HID_READ_PENDING (1 << 2);
        
#define I2C_HID_CMD(opcode_) \
.opcode = opcode_, .length = 4,\
.registerIndex = offsetof(struct i2c_hid_desc, wCommandRegister)
    };
    struct i2c_hid_cmd hid_reset_cmd = { I2C_HID_CMD(0x01),
        .wait = true
    };
    
    struct i2c_hid_cmd hid_descr_cmd = { .length = 2};
    
    struct i2c_hid_cmd hid_input_cmd = {
        .registerIndex = offsetof(struct i2c_hid_desc, wInputRegister),
        .length = 2
    };
    
    struct i2c_hid_cmd hid_report_desc_cmd = {
        .registerIndex = offsetof(struct i2c_hid_desc, wReportDescRegister),
        .opcode = 0x00,
        .length =2
    };
    
    struct i2c_hid_cmd hid_set_power_cmd = { I2C_HID_CMD(0x08) };
    
    uint8_t report[WACOM_MAX_INPUT_LEN];
    
    
    int initHIDDevice(I2CDevice *hid_device);
    
    int i2c_hid_acpi_pdata(i2c_hid *ihid);
    
    struct csgesture_softc softc;
    
    int i2c_hid_alloc_buffers(i2c_hid *ihid, UInt report_size);
    
    void i2c_hid_free_buffers(i2c_hid *ihid, UInt report_size);
    
    int i2c_hid_fetch_hid_descriptor(i2c_hid *ihid);
    
    int i2c_hid_command(i2c_hid *ihid, struct i2c_hid_cmd *command, unsigned char *buf_recv, int data_len);
    
    int __i2c_hid_command(i2c_hid *ihid, struct i2c_hid_cmd *command, UInt8 reportID, UInt8 reportType, UInt8 *args, int args_len, unsigned char *buf_recv, int data_len);
    
    int i2c_hid_set_power(i2c_hid *ihid, int power_state);
    
    bool probe(IOService* device);
    
    void InterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount);
    
    void i2c_hid_get_input(OSObject* owner, IOTimerEventSource* sender);
    
    bool i2c_hid_get_report_descriptor(i2c_hid *ihid);
    
    void write_report_descriptor_to_buffer(IOBufferMemoryDescriptor *buffer);
    
    int i2c_get_slave_address(I2CDevice* hid_device);
    
    bool i2c_hid_hwreset(i2c_hid *ihid);
    
//    void writeInputReportToBuffer(unsigned char* rdesc, int return_size);
    
    void touchscreenRawInput(struct csgesture_softc *sc, uint8_t report[42], int tickinc);
    
    UInt16 compareInputx;
    UInt16 compareInputy;
    int compareReportCounter = 0;
    bool exitsingletouch = false;
    bool exitmultitouch = false;
};

#endif /* VoodooWacomDevice_h */
