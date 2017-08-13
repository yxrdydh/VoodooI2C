//
//  VoodooNTRGDevice.cpp
//  VoodooI2C
//
//  Created by Nate House on <some future date>
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooNTRGDevice.h"
#include "VoodooI2C.h"
#include "VoodooNTRGWrapper.h"

OSDefineMetaClassAndStructors(VoodooNTRGDevice, VoodooI2CDevice);

bool VoodooNTRGDevice::attach(IOService * provider, IOService* child)
{
    if (!super::attach(provider))
        return false;
    
    assert(_controller == 0);
    _controller = (VoodooI2C*)provider;
    _controller->retain();
    
    
    child->attach(this);
    if (!probe(child))
        return false;
    
    return true;
}

bool VoodooNTRGDevice::probe(IOService* device) {
    
    
    hid_device = (I2CDevice *)IOMalloc(sizeof(I2CDevice));
    
    //hid_device->_dev = _controller->_dev;
    
    if (!super::start(device))
        return false;
    
    
    hid_device->provider = OSDynamicCast(IOACPIPlatformDevice, device);
    hid_device->provider->retain();
    
    int ret = i2c_get_slave_address(hid_device);
    if (ret < 0){
        IOLog("%s::%s::Failed to get a slave address for an I2C device, aborting.\n", getName(), _controller->_dev->name);
        IOFree(hid_device, sizeof(I2CDevice));
        return false;
    }
    
    
    IOLog("%s::%s::HID Probe called for i2c 0x%02x\n", getName(), _controller->_dev->name, hid_device->addr);
    
    initHIDDevice(hid_device);
    
    //super::stop(device);
    return 0;
}

void VoodooNTRGDevice::stop(IOService* device) {
    
    IOLog("I2C HID Device is stopping\n");
    
    destroy_wrapper();
    
    if (hid_device->timerSource){
        hid_device->timerSource->cancelTimeout();
        hid_device->timerSource->release();
        hid_device->timerSource = NULL;
    }
    
    //hid_device->workLoop->removeEventSource(hid_device->interruptSource);
    //hid_device->interruptSource->disable();
    hid_device->interruptSource = NULL;
    
    hid_device->workLoop->release();
    hid_device->workLoop = NULL;
    
    
    
    
    i2c_hid_free_buffers(ihid, HID_MIN_BUFFER_SIZE);
    IOFree(ihid, sizeof(i2c_hid));
    
    IOFree(hid_device, sizeof(I2CDevice));
    
    //hid_device->provider->close(this);
    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VoodooNTRGDevice::detach( IOService * provider )
{
    assert(_controller == provider);
    _controller->release();
    _controller = 0;
    
    super::detach(provider);
}

int VoodooNTRGDevice::initHIDDevice(I2CDevice *hid_device) {
    int ret;
    UInt16 hidRegister;
    
    ihid = (i2c_hid*)IOMalloc(sizeof(i2c_hid));
    
    ihid->client = hid_device;
    
    ret = i2c_hid_acpi_pdata(ihid);
    
    ihid->client = hid_device;
    
    
    hidRegister = ihid->pdata.hid_descriptor_address;
    
    ihid->wHIDDescRegister = (__le16)hidRegister;
    
    ret = i2c_hid_alloc_buffers(ihid, HID_MIN_BUFFER_SIZE);
    if (ret < 0)
        goto err;
    
    //ret = i2c_hid_set_power(ihid, I2C_HID_PWR_ON);
    //if(ret<0)
    //   goto err;
    
    
    ret = i2c_hid_fetch_hid_descriptor(ihid);
    if (ret < 0)
        goto err;
    
    setProperty("HIDDescLength", (UInt32)ihid->hdesc.wHIDDescLength, 32);
    setProperty("bcdVersion", (UInt32)ihid->hdesc.bcdVersion, 32);
    setProperty("ReportDescLength", (UInt32)ihid->hdesc.wReportDescLength, 32);
    setProperty("ReportDescRegister", (UInt32)ihid->hdesc.wReportDescRegister, 32);
    setProperty("InputRegister", (UInt32)ihid->hdesc.wInputRegister, 32);
    setProperty("MaxInputLength", (UInt32)ihid->hdesc.wMaxInputLength, 32);
    setProperty("OutputRegister", (UInt32)ihid->hdesc.wOutputRegister, 32);
    setProperty("MaxOutputLength", (UInt32)ihid->hdesc.wMaxOutputLength, 32);
    setProperty("CommandRegister", (UInt32)ihid->hdesc.wCommandRegister, 32);
    setProperty("DataRegister", (UInt32)ihid->hdesc.wDataRegister, 32);
    setProperty("vendorID", (UInt32)ihid->hdesc.wVendorID, 32);
    setProperty("productID", (UInt32)ihid->hdesc.wProductID, 32);
    setProperty("VersionID", (UInt32)ihid->hdesc.wVersionID, 32);
    
    
    hid_device->workLoop = (IOWorkLoop*)getWorkLoop();
    if(!hid_device->workLoop) {
        IOLog("%s::%s::Failed to get workloop\n", getName(), _controller->_dev->name);
        stop(this);
        return -1;
    }
    
    hid_device->workLoop->retain();
    
    /*
     hid_device->interruptSource = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &VoodooNTRGDevice::InterruptOccured), hid_device->provider);
     
     if (hid_device->workLoop->addEventSource(hid_device->interruptSource) != kIOReturnSuccess) {
     IOLog("%s::%s::Could not add interrupt source to workloop\n", getName(), _controller->_dev->name);
     stop(this);
     return -1;
     }
     
     hid_device->interruptSource->enable();
     */
    
    hid_device->timerSource = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &VoodooNTRGDevice::i2c_hid_get_input));
    if (!hid_device->timerSource){
        goto err;
    }
    
    hid_device->workLoop->addEventSource(hid_device->timerSource);
    hid_device->timerSource->setTimeoutMS(200);
    /*
     
     hid_device->commandGate = IOCommandGate::commandGate(this);
     
     if (!hid_device->commandGate || (_dev->workLoop->addEventSource(hid_device->commandGate) != kIOReturnSuccess)) {
     IOLog("%s::%s::Failed to open HID command gate\n", getName(), _dev->name);
     return -1;
     }
     */
    
    i2c_hid_get_report_descriptor(ihid);
    
    initialize_wrapper();
    registerService();
    
    return 0;
    
err:
    i2c_hid_free_buffers(ihid, HID_MIN_BUFFER_SIZE);
    IOFree(ihid, sizeof(i2c_hid));
    return ret;
}

void VoodooNTRGDevice::initialize_wrapper(void) {
    _wrapper = new CSGesture;
    _wrapper->vendorID = 'GRTN';
    _wrapper->productID = '1000';
    _wrapper->softc = &softc;
    _wrapper->softc->infoSetup = true;
    //_wrapper->softc->panningActive = 0;
    _wrapper->softc->phyx = 2540;
    _wrapper->softc->phyy = 1693;
    _wrapper->softc->resx = 2560 * 10 / 9600;
    _wrapper->softc->resy = 1693 * 10 / 7200;
    _wrapper->softc->disableIntertialScroll = false;
    _wrapper->softc->settings.tapToClickEnabled = true;
    _wrapper->softc->settings.tapDragEnabled = true;
    _wrapper->softc->settings.multiFingerTap = true;
    
    _wrapper->initialize_wrapper(this);
    
    
    _wacwrapper = new VoodooNTRGWrapper;
    if (_wacwrapper->init()) {
        _wacwrapper->attach(this);
        _wacwrapper->start(this);
    }
    else {
        _wacwrapper->release();
        _wacwrapper = NULL;
    }
}

void VoodooNTRGDevice::destroy_wrapper(void) {
    _wrapper->destroy_wrapper();
    _wacwrapper->release();
    _wacwrapper = NULL;
}

int VoodooNTRGDevice::i2c_hid_acpi_pdata(i2c_hid *ihid) {
    
    UInt32 guid_1 = 0x3CDFF6F7;
    UInt32 guid_2 = 0x45554267;
    UInt32 guid_3 = 0x0AB305AD;
    UInt32 guid_4 = 0xDE38893D;
    
    
    OSObject *result = NULL;
    OSObject *params[3];
    char buffer[16];
    
    memcpy(buffer, &guid_1, 4);
    memcpy(buffer + 4, &guid_2, 4);
    memcpy(buffer + 8, &guid_3, 4);
    memcpy(buffer + 12, &guid_4, 4);
    
    
    params[0] = OSData::withBytes(buffer, 16);
    params[1] = OSNumber::withNumber(0x1, 8);
    params[2] = OSNumber::withNumber(0x1, 8);
    
    ihid->client->provider->evaluateObject("_DSM", &result, params, 3);
    if (!result)
        ihid->client->provider->evaluateObject("XDSM", &result, params, 3);
    
    OSNumber* number = OSDynamicCast(OSNumber, result);
    
    if (number)
         ihid->pdata.hid_descriptor_address = number->unsigned32BitValue();
 
    if (result)
         result->release();
 
    params[0]->release();
    params[1]->release();
    params[2]->release();
    
    return 0;
}

int VoodooNTRGDevice::i2c_get_slave_address(I2CDevice* hid_device){
    OSObject* result = NULL;
    
    hid_device->provider->evaluateObject("_CRS", &result);
    
    OSData* data = OSDynamicCast(OSData, result);
    
    hid_device->addr = *(int*)data->getBytesNoCopy(16,1) & 0xFF;
    
    data->release();
    
    return 0;
    
}

int VoodooNTRGDevice::i2c_hid_alloc_buffers(i2c_hid *ihid, UInt report_size) {
    int args_len = sizeof(UInt8) + sizeof(UInt16) + sizeof(UInt16) + report_size;
    
    ihid->inbuf = (char *)IOMalloc(report_size);
    ihid->argsbuf = (char *)IOMalloc(report_size);
    ihid->cmdbuf = (char *)IOMalloc(sizeof(union command) + args_len);
    
    if(!ihid->inbuf || !ihid->argsbuf || !ihid->cmdbuf) {
        i2c_hid_free_buffers(ihid, report_size);
        return -1;
    }
    
    ihid->bufsize = report_size;
    
    return 0;
}

void VoodooNTRGDevice::i2c_hid_free_buffers(i2c_hid *ihid, UInt report_size) {
    IOFree(ihid->inbuf, report_size);
    IOFree(ihid->argsbuf, report_size);
    IOFree(ihid->cmdbuf, sizeof(UInt8) + sizeof(UInt16) + sizeof(UInt16) + report_size);
    ihid->inbuf = NULL;
    ihid->cmdbuf = NULL;
    ihid->argsbuf = NULL;
    ihid->bufsize = 0;
}

int VoodooNTRGDevice::i2c_hid_fetch_hid_descriptor(i2c_hid *ihid) {
    struct i2c_hid_desc *hdesc = &ihid->hdesc;
    UInt dsize;
    int ret;
    
    ret = i2c_hid_command(ihid, &hid_descr_cmd, ihid->hdesc_buffer, sizeof(struct i2c_hid_desc));
    
    if (ret)
    {
        IOLog("%s::%s::hid_descr_cmd failed\n", getName(), hid_device->name);
        return -1;
    }
    
    if((UInt16)(hdesc->bcdVersion) != 0x0100) {
        IOLog("%s::%s::Unexpected HID descriptor bcdVersion %x\n", getName(), hid_device->name, (UInt16)(hdesc->bcdVersion));
        return -1;
    }
    
    //dsize = (UInt16)(hdesc->wHIDDescLength);
    
    //if (dsize != sizeof(struct i2c_hid_desc)) {
    //    IOLog("%s::%s::weird size of HID descriptor\n", getName(), _dev->name);
    //    return -1;
    //}
    
    return 0;
}

int VoodooNTRGDevice::i2c_hid_command(i2c_hid *ihid, struct i2c_hid_cmd *command, unsigned char *buf_recv, int data_len) {
    return __i2c_hid_command(ihid, command, 0, 0, NULL, 0, buf_recv, data_len);
}

int VoodooNTRGDevice::__i2c_hid_command(i2c_hid *ihid, struct i2c_hid_cmd *command, UInt8 reportID, UInt8 reportType, UInt8 *args, int args_len, unsigned char *buf_recv, int data_len) {
    union command *cmd = (union command *)ihid->cmdbuf;
    int ret;
    struct i2c_msg msg[2];
    int msg_num = 1;
    
    int length = command->length;
    bool wait = command->wait;
    UInt registerIndex = command->registerIndex;
    
    if (command == &hid_descr_cmd) {
        cmd->c.reg = ihid->wHIDDescRegister;
    } else {
        cmd->data[0] = ihid->hdesc_buffer[registerIndex];
        cmd->data[1] = ihid->hdesc_buffer[registerIndex + 1];
    }
    
    if (length > 2) {
        cmd->c.opcode = command->opcode;
        cmd->c.reportTypeID = reportID | reportType << 4;
    }
    
    memcpy(cmd->data + length, args, args_len);
    length += args_len;
    
    
    msg[0].addr = ihid->client->addr;
    msg[0].flags = 0; //ihid->client->flags & I2C_M_TEN;
    msg[0].len = length;
    msg[0].buf = cmd->data;
    
    if (data_len > 0) {
        msg[1].addr = ihid->client->addr;
        msg[1].flags = I2C_M_RD;
        msg[1].len = data_len;
        msg[1].buf = buf_recv;
        msg_num = 2;
        hid_device->reading = true;
    }
    
    ret = _controller->i2c_transfer((VoodooI2C::i2c_msg*)msg, msg_num);
    
    if (data_len > 0)
        hid_device->reading = false;
    
    if (ret != msg_num)
        return ret < 0 ? ret : -1;
    
    ret = 0;
    
    return ret;
}

int VoodooNTRGDevice::i2c_hid_set_power(i2c_hid *ihid, int power_state) {
    int ret;
    
    ret = __i2c_hid_command(ihid, &hid_set_power_cmd, power_state, NULL, 0, NULL, 0, NULL);
    if (ret)
        IOLog("failed to change power settings \n");
    
    return ret;
}


void VoodooNTRGDevice::InterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount){
    IOLog("interrupt\n");
    if (hid_device->reading)
        return;
    
    //i2c_hid_get_input(ihid);
    
}

void VoodooNTRGDevice::i2c_hid_get_input(OSObject* owner, IOTimerEventSource* sender) {
    //    IOLog("getting input\n");
    UInt rsize;
    int ret;
    
    rsize = UInt16(ihid->hdesc.wMaxInputLength);
    
    unsigned char* rdesc = (unsigned char *)IOMalloc(rsize);
    
    ret = i2c_hid_command(ihid, &hid_input_cmd, rdesc, rsize);
    
    if ((rdesc[5] & NTRG_TIP_SWITCH) == NTRG_TIP_SWITCH)
        tipSwitch = true;
    if ((rdesc[5] & NTRG_IN_RANGE) == NTRG_IN_RANGE)
        inRange = true;
    if ((rdesc[5] & NTRG_CONFIDENCE) == NTRG_CONFIDENCE)
        validTouch = true; //Always set hw issue???
    int num_fingers = rdesc[24];
    
    
    /*        IOLog("===Input (%d)===\n", rsize);
     for (int i = 0; i < rsize; i++)
     IOLog("0x%02x ", (UInt8) rdesc[i]);
     IOLog("\n");
    */
    
    int return_size = rdesc[0] | rdesc[1] << 8;
    if (return_size == 0) {
        /* host or device initiated RESET completed */
        // test/clear bit?
        hid_device->timerSource->setTimeoutMS(5);
        return;
    }
    
    if (return_size > rsize) {
        IOLog("%s: Incomplete report %d/%d\n", __func__, rsize, return_size);
    }
    
    
    //  This is a routine to accomplish a right click by comparing positional data from report to report.
    //  The length of time to initiate a right click can be adjusted by altering the compareReportCounter check.
    //  TO DO - build in some tolerance for slight movement.
    //  TODO(Nate): Refactor single press right click
    
    
    if (rdesc[2] == NTRG_FINGERTOUCH && num_fingers == 1 && lastTouch < rdesc[3]) {
        UInt16 rtempx = (rdesc[8] + (rdesc[9] << 6));
        UInt16 rtempy = (rdesc[12] + (rdesc[13] << 6));
        UInt16 fingerw = (rdesc[16] + (rdesc[17] << 6));
        UInt16 fingerh = (rdesc[18] + (rdesc[19] << 6));
        
        if (rtempx >= (compareInputx - fingerw / 2) && rtempx <= (compareInputx + fingerw /2) && rtempy >= (compareInputy - fingerh / 2) && rtempy <= (compareInputy + fingerh / 2)) {
            compareReportCounter = compareReportCounter + 1;
            compareInputx = rtempx;
            compareInputy = rtempy;            
            if (compareReportCounter >= 120) {
                rightClick = true;
                compareInputx = 0;
                compareInputy = 0;
                compareReportCounter = 0;
            }
        }
        else {
            compareInputx = rtempx;
            compareInputy = rtempy;
            compareReportCounter = 0;
        }
        
    }
    
    if (rdesc[2] == NTRG_FINGERTOUCH && num_fingers > 1 && tipSwitch && inRange){
        if (!(lastTouch == rdesc[3])){
            for (int i=0;i < NTRG_MAX_INPUT_LEN; i++)
                report [i] = rdesc[i];
            touchscreenRawInput(&softc, report, 1);
            _wrapper->update_relative_mouse(0x0, 0, 0, 0, 0);
        }
    } else {
        if (!(lastTouch == rdesc[3])){
            if (rightClick){
                _wrapper->update_relative_mouse(0x2, 0, 0, 0, 0);
                rightClick = false;
            }
            writeInputReportToBuffer(rdesc, return_size);
            _wrapper->update_relative_mouse(0x0, 0, 0, 0, 0);
        }
    }
    lastTouch = rdesc[03];

    IOFree(rdesc, rsize);
    hid_device->timerSource->setTimeoutMS(5);
}

void VoodooNTRGDevice::writeInputReportToBuffer(unsigned char* rdesc, int return_size){
    
    //  int rsize = NTRG_MAX_INPUT_LEN;
    //  IOLog("===Input (%d)===\n", rsize);
    //  for (int i = 0; i < rsize; i++)
    //      IOLog("0x%02x ", (UInt8) rdesc[i]);
    //  IOLog("\n");
    
    
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, return_size);
    buffer->writeBytes(0, rdesc + 2, return_size - 2);
    
    IOReturn err = _wacwrapper->handleReport(buffer, kIOHIDReportTypeInput);
    if (err != kIOReturnSuccess)
        IOLog("Error handling report: 0x%.8x\n", err);
    
    buffer->release();
    
}

void VoodooNTRGDevice::touchscreenRawInput(struct csgesture_softc *sc, uint8_t * report, int tickinc){
    
    // This function assembles the multitouch data and passes it to csgesture for gesture processing.
    // The NTRG detects up to ten fingers, but the limit is currently set to five and only up to four
    // fingers are currently processed for gestures.
    
    uint8_t *finger_data = &report[NTRG_FINGER_DATA_OFFSET];
    int i;
    
    for (i=0;i < NTRG_MAX_FINGERS; i++) {
        
        sc->x[i] = -1;
        sc->y[i] = -1;
        sc->p[i] = -1;
    }
    
    //unsigned int pos_x, pos_y;
    unsigned int pressure = 255;
    
    //unsigned int tcLink = finger_data[-2];
    int slots = finger_data[19];
    for (int i=0; i < slots && i < NTRG_MAX_FINGERS; i++) {
        
        unsigned int pos_x, pos_y;
        slots = finger_data[19];

        pos_x = (finger_data[3] + (finger_data[4] << 6));
        pos_y = (finger_data[7] + (finger_data[8] << 6));

            sc->x[i] = pos_x;
            sc->y[i] = pos_y;
            sc->p[i] = pressure;
        
        unsigned int tcLink = finger_data[-2];
        if (i < slots){
            //lets find our exact related finger data.
            //they would share a time counter cross input reports at rdesc[4] so -2 index diff
            //so increment and see if we get lucky
            for (int b = 1; b < 111; b++){ //check pointers for itterative desc
                if (finger_data[(NTRG_MAX_INPUT_LEN - 2)] == tcLink && finger_data[NTRG_MAX_INPUT_LEN - 6] == 0x1d ){
                    finger_data += (b + NTRG_MAX_INPUT_LEN);
                    break;
                }
            }
        }
    }
    
    sc->buttondown = 0x0;
    
    _wrapper->ProcessGesture(sc);
    
}


bool VoodooNTRGDevice::i2c_hid_get_report_descriptor(i2c_hid *ihid){
    UInt rsize;
    int ret;
    
    IOLog("reg: 0x%x\n",ihid->hdesc.wReportDescRegister);
    
    rsize = UInt16(ihid->hdesc.wReportDescLength);
    
    unsigned char* rdesc = (unsigned char *)IOMalloc(rsize);
    
    i2c_hid_hwreset(ihid);
    
    if (!rdesc){
        return -1;
    }
    /*
     if (!rsize || rsize > HID_MAX_DESCRIPTOR_SIZE){
     IOLog("%s::%s::Weird size of report descriptor (%u)\n", getName(), hid_device->name, rsize);ƒ
     return 1;
     }
     */
    
    ret = i2c_hid_command(ihid, &hid_report_desc_cmd, rdesc, rsize);
    
    if (!ret){
        IOLog("it worked!\n");
    }
    
    
    
    //_controller->registerDump(_controller->_dev);
    
    /*
     if (ret) {
     IOLog("%s::%s::Reading report descriptor failed", getName(), hid_device->name);
     return -1;
     }
     
     
     IOLog("%s::%s::Report descriptor: %s\n", getName(), hid_device->name, rdesc);
     
     */
    
    IOLog("===Report Descriptor===\n");
    for (int i = 0; i < UInt16(ihid->hdesc.wReportDescLength); i++)
        IOLog("0x%02x\n", (UInt8) rdesc[i]);
    IOLog("===Report Descriptor===\n");
    
    IOFree(rdesc, rsize);
    
    return 0;
    
};

void VoodooNTRGDevice::write_report_descriptor_to_buffer(IOBufferMemoryDescriptor *buffer){
    UInt rsize;
    int ret;
    
    IOLog("Report descriptor register: 0x%x\n",ihid->hdesc.wReportDescRegister);
    
    rsize = UInt16(ihid->hdesc.wReportDescLength);
    
    unsigned char* rdesc = (unsigned char *)IOMalloc(rsize);
    
    i2c_hid_hwreset(ihid);
    
    ret = i2c_hid_command(ihid, &hid_report_desc_cmd, rdesc, rsize);
    
    if (!ret)
        IOLog("Report descriptor was fetched\n");
    
    buffer->writeBytes(0, rdesc, rsize);
    IOLog("Report Descriptor written to buffer (%d)\n", rsize);
    
    IOFree(rdesc, rsize);
}

bool VoodooNTRGDevice::i2c_hid_hwreset(i2c_hid *ihid) {
    int ret;
    unsigned char buf[2];
    
    ret = i2c_hid_set_power(ihid, I2C_HID_PWR_ON);
    
    if (ret)
        return ret;
    
    ret = i2c_hid_command(ihid, &hid_reset_cmd, buf, 2);
    if (ret || (*buf != 0) || (*(buf+1) != 0))
    {
        i2c_hid_set_power(ihid, I2C_HID_PWR_SLEEP);
        return ret;
    }
    
    return 0;
};
