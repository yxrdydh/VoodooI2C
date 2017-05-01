//
//  VoodooNTRGWrapper.cpp
//  VoodooI2C
//
//  Created by Nate on 4/26/17.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

//
//  VoodooNTRGWrapper.cpp
//  VoodooI2C
//
//  Created by Joe Litko on 10/20/16.
//  Copyright © 2016 Alexandre Daoud. All rights reserved.
//

#include "VoodooNTRGWrapper.h"
#include "VoodooNTRGDevice.h"

OSDefineMetaClassAndStructors(VoodooNTRGWrapper, IOHIDDevice)

static VoodooNTRGDevice* GetOwner(const IOService *us)
{
    IOService *prov = us->getProvider();
    
    if (prov == NULL)
        return NULL;
    return OSDynamicCast(VoodooNTRGDevice, prov);
}

bool VoodooNTRGWrapper::start(IOService *provider) {
    if (OSDynamicCast(VoodooNTRGDevice, provider) == NULL)
        return false;
    
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    setProperty("HIDDefaultBehavior", OSString::withCString("TouchScreen"));
    return IOHIDDevice::start(provider);
}

IOReturn VoodooNTRGWrapper::setProperties(OSObject *properties) {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return kIOReturnUnsupported;
}

IOReturn VoodooNTRGWrapper::newReportDescriptor(IOMemoryDescriptor **descriptor) const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, GetOwner(this)->ihid->hdesc.wReportDescLength);
    
    if (buffer == NULL) return kIOReturnNoResources;
    GetOwner(this)->write_report_descriptor_to_buffer(buffer);
    *descriptor = buffer;
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return kIOReturnSuccess;
}

IOReturn VoodooNTRGWrapper::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return kIOReturnUnsupported;
}

IOReturn VoodooNTRGWrapper::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return kIOReturnUnsupported;
}

IOReturn VoodooNTRGWrapper::handleReport(
                                          IOMemoryDescriptor * report,
                                          IOHIDReportType      reportType,
                                          IOOptionBits         options  ) {
    return IOHIDDevice::handleReport(report, reportType, options);
}

OSString* VoodooNTRGWrapper::newManufacturerString() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSString::withCString("Sunrise Point");
}

OSNumber* VoodooNTRGWrapper::newPrimaryUsageNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(kHIDUsage_Dig_TouchScreen, 32);
}

OSNumber* VoodooNTRGWrapper::newPrimaryUsagePageNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(kHIDPage_Digitizer, 32);
}

OSNumber* VoodooNTRGWrapper::newProductIDNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(GetOwner(this)->ihid->hdesc.wProductID, 16);
}

OSString* VoodooNTRGWrapper::newProductString() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSString::withCString("NTRG 4814");
}

OSString* VoodooNTRGWrapper::newSerialNumberString() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSString::withCString("1234");
}

OSString* VoodooNTRGWrapper::newTransportString() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSString::withCString("I2C");
}

OSNumber* VoodooNTRGWrapper::newVendorIDNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(GetOwner(this)->ihid->hdesc.wVendorID, 16);
}

OSNumber* VoodooNTRGWrapper::newLocationIDNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(123, 32);
}
