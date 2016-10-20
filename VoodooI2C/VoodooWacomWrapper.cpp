//
//  VoodooWacomWrapper.cpp
//  VoodooI2C
//
//  Created by Joe Litko on 10/20/16.
//  Copyright © 2016 Alexandre Daoud. All rights reserved.
//

#include "VoodooWacomWrapper.h"
#include "VoodooWacomDevice.h"

OSDefineMetaClassAndStructors(VoodooWacomWrapper, IOHIDDevice)

static VoodooWacomDevice* GetOwner(const IOService *us)
{
    IOService *prov = us->getProvider();
    
    if (prov == NULL)
        return NULL;
    return OSDynamicCast(VoodooWacomDevice, prov);
}

bool VoodooWacomWrapper::start(IOService *provider) {
    if (OSDynamicCast(VoodooWacomDevice, provider) == NULL)
        return false;
    
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    setProperty("HIDDefaultBehavior", OSString::withCString("Trackpad"));
    return IOHIDDevice::start(provider);
}

IOReturn VoodooWacomWrapper::setProperties(OSObject *properties) {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return kIOReturnUnsupported;
}

IOReturn VoodooWacomWrapper::newReportDescriptor(IOMemoryDescriptor **descriptor) const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, GetOwner(this)->ihid->hdesc.wReportDescLength);
    
    if (buffer == NULL) return kIOReturnNoResources;
    GetOwner(this)->write_report_descriptor_to_buffer(buffer);
    *descriptor = buffer;
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return kIOReturnSuccess;
}

IOReturn VoodooWacomWrapper::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return kIOReturnUnsupported;
}

IOReturn VoodooWacomWrapper::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return kIOReturnUnsupported;
}

IOReturn VoodooWacomWrapper::handleReport(
                                        IOMemoryDescriptor * report,
                                        IOHIDReportType      reportType,
                                        IOOptionBits         options  ) {
    return IOHIDDevice::handleReport(report, reportType, options);
}

OSString* VoodooWacomWrapper::newManufacturerString() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSString::withCString("Lynx Point");
}

OSNumber* VoodooWacomWrapper::newPrimaryUsageNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(kHIDUsage_Dig_TouchScreen, 32);
}

OSNumber* VoodooWacomWrapper::newPrimaryUsagePageNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(kHIDPage_Digitizer, 32);
}

OSNumber* VoodooWacomWrapper::newProductIDNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(GetOwner(this)->ihid->hdesc.wProductID, 16);
}

OSString* VoodooWacomWrapper::newProductString() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSString::withCString("Wacom 4814");
}

OSString* VoodooWacomWrapper::newSerialNumberString() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSString::withCString("1234");
}

OSString* VoodooWacomWrapper::newTransportString() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSString::withCString("I2C");
}

OSNumber* VoodooWacomWrapper::newVendorIDNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(GetOwner(this)->ihid->hdesc.wVendorID, 16);
}

OSNumber* VoodooWacomWrapper::newLocationIDNumber() const {
    IOLog("VoodooI2C: %s, line %d\n", __FILE__, __LINE__);
    return OSNumber::withNumber(123, 32);
}
