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