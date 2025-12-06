# update waow

so I just worked on the device and drivers thing, the idea was that the kernel discovers devices by looking on
the PCI bus or like the PnP bus for ISA, and then drivers can identify what devices they want to connect to by
specifying if that driver connects on the PCI bus, or if it's a TTY driver or Network driver.

It's a decent system, and it's really similar to Linux (apparently). But one thing is that there also needs to
be sibling support and some better management for removable or hot-pluggable devices. For removable, the driver
needs to have a two-way attachment (like the device.driver and driver.device) so that if the kernel detects that
the device no longer exists, it can automatically deinit the driver (shouldn't be too hard since the generic
already exists.) And hot-plugging is really easy it's just the current system but instead of looking for a device
that supports our driver, we look for a driver that supports our device.

I think what's a good thing to work on next is some dynamic global way to select a function through a string,
like a symbol map or ObjC's selector features. I really want to flesh out the driver system before I start working
deeper on stuff like the UI or a user-mode infinite-process or even a scheduler (although multitasking is very
high up my list)

# update new 06/12/25
what the bloody cinnamon toast hell
```
if (device->type != driver->device_type && device->type != DEVICE_TYPE_UNKNOWN) {
    continue;
}
if (device->conn != driver->driver_type && device->conn != CONNECTION_TYPE_UNKNOWN) {
    continue;
}
```
i had problems with the code because I used || instead of && so it would skip. I ONLY ADDED THIS AFTER
COMMITTING AND THIS IS THE SOURCE OF ALL MY PROBLEMS WHAT THE ACTUAL BLEEPING CRAP BRO I LOST SLEEP OVER
THIS

luckily I kinda made the code a lot more better and reusable so now I think 1. it works better, 2. the relocation
is functional, and 3. we shouldn't have any issues with user mode programs now.

for the future: make sure that drivers are linked with the driver file, so if the driver file moves in memory
or the functions get moved to a different page, the drivers can still work with no issue (provided that none of
them are currently being used at that moment.) Which shouldn't be an issue since the drivers only rely on
ObjC-like C code (self as first argument) so they all only work on a given instance.