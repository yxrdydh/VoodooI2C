//
//  VoodooNTRGWrapper.h
//  VoodooI2C
//
//  Created by Nate on 4/26/17.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooNTRGWrapper_h
#define VoodooNTRGWrapper_h

#include <IOKit/hid/IOHIDDevice.h>

class VoodooI2CHIDDevice;

class VoodooNTRGWrapper : public IOHIDDevice
{
    OSDeclareDefaultStructors(VoodooNTRGWrapper)
    
public:
    virtual bool start(IOService *provider) override;
    
    virtual IOReturn setProperties(OSObject *properties) override;
    
    virtual IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const override;
    
    virtual IOReturn setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options=0) override;
    virtual IOReturn getReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options) override;
    virtual IOReturn handleReport(
                                  IOMemoryDescriptor * report,
                                  IOHIDReportType      reportType = kIOHIDReportTypeInput,
                                  IOOptionBits         options    = 0 ) override;
    
    virtual OSString* newManufacturerString() const override;
    virtual OSNumber* newPrimaryUsageNumber() const override;
    virtual OSNumber* newPrimaryUsagePageNumber() const override;
    virtual OSNumber* newProductIDNumber() const override;
    virtual OSString* newProductString() const override;
    virtual OSString* newSerialNumberString() const override;
    virtual OSString* newTransportString() const override;
    virtual OSNumber* newVendorIDNumber() const override;
    
    virtual OSNumber* newLocationIDNumber() const override;
};

#endif /* VoodooNTRGWrapper_h */
