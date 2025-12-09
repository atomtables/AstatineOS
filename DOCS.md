# driver stuff for info

For the actual drivers, we do still have the "check" and "probe" items that will be used
for checking if devices are compatible or if probing needs to happen. For something like a
ISA driver for PCSPK that'll still be the case.

But for items like PCI IDE controller that have to enumerate items after init, the check function
checks for the actual PCI IDE controller and the init function registers new devices that can then
be used individually!!!

Those devices register with the magistrate... no they fucking don't what the hell is ts (they register with the
basedevice list which they will then connect with the disk-level ide driver!!!)