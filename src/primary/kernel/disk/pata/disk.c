// Disk driver for Hard Drives that are PATA-only. not very sure outside of that.
// will definitely require a rewrite when the system becomes more complex and we start having more files.
// at that point the PCI subsystem will need to be integrated more deeply + itll use IRQ methods.
#include <modules/modules.h>
#include <display/simple/display.h>
#include <timer/PIT.h>
#include <driver_base/disk/disk_structures.h>
#include <disk/disk.h>
#include <basedevice/subtypes/disk/disk.h>
#include <driver_base/driver_base.h>
#include <modules/strings.h>
/*
The base address register should be gotten from the PCI bus.
When implementing the PCI register, also add this feature.
for now, we can assume that items on the PATA bus, so IDE, 
will use these IO ports: 0x1F0, 0x3F6, 0x170, 0x376, 0x000
    BAR0: Base address of primary channel in PCI native mode (8 ports)
    BAR1: Base address of primary channel control port in PCI native mode (4 ports)
    BAR2: Base address of secondary channel in PCI native mode (8 ports)
    BAR3: Base address of secondary channel control port in PCI native mode (4 ports)
    BAR4: Bus master IDE (16 ports, 8 for each channel)
*/

// Status Register bits
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

// Features/Error register bits
#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

// Command port commands
#define ATA_CMD_READ_PIO            0x20
#define ATA_CMD_READ_PIO_EXT        0x24
#define ATA_CMD_READ_DMA            0xC8
#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_WRITE_PIO           0x30
#define ATA_CMD_WRITE_PIO_EXT       0x34
#define ATA_CMD_WRITE_DMA           0xCA
#define ATA_CMD_WRITE_DMA_EXT       0x35
#define ATA_CMD_CACHE_FLUSH         0xE7
#define ATA_CMD_CACHE_FLUSH_EXT     0xEA
#define ATA_CMD_PACKET              0xA0
#define ATA_CMD_IDENTIFY_PACKET     0xA1
#define ATA_CMD_IDENTIFY            0xEC
// commands specifically for ATAPI
#define ATAPI_CMD_READ              0xA8
#define ATAPI_CMD_EJECT             0x1B

// When sending an Identify packet, these are how we read the packets
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

// When selecting a drive, use bits to select which one
// interface type
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
// master/slave
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01
// Channels
#define ATA_PRIMARY    0x00
#define ATA_SECONDARY  0x01

/*
Task File is a range of 8 ports which are offsets from 
BAR0 (primary channel) and/or BAR2 (secondary channel). 
To exemplify:
    BAR0 + 0 is first port.
    BAR0 + 1 is second port.
    BAR0 + 2 is the third
*/
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
// [(SECCOUNT1 << 8) | SECCOUNT0] expresses
// number of sectors in LBA48 mode
// SECCOUNT0 expresses #sectors in LBA28/CHS
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
/*
The ALTSTATUS/CONTROL port returns the alternate status 
when read and controls a channel when written to.
    For the primary channel, ALTSTATUS/CONTROL port is BAR1 + 2.
    For the secondary channel, ALTSTATUS/CONTROL port is BAR3 + 2.
*/
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

/*
We can now say that each channel has 13 registers. For the primary channel, we use these values:
    Data Register: BAR0 + 0; // Read-Write
    Error Register: BAR0 + 1; // Read Only
    Features Register: BAR0 + 1; // Write Only
    --------- IMPORTANT ----------
    technically the 2, 3, 4, 5 offset also contain alternates when read as word... why? probably to fw us.
    SECCOUNT0: low8(BAR0 + 2); SECCOUNT1: high8(BAR0 + 2) // Read-Write
    LBA0: low8(BAR0 + 3); LBA3: high8(BAR0 + 3) // Read-Write
    LBA1: low8(BAR0 + 4); LBA4: high8(BAR0 + 4) // Read-Write
    LBA2: low8(BAR0 + 5); LBA5: high8(BAR0 + 5) // Read-Write
    HDDEVSEL: BAR0 + 6; // Read-Write, used to select a drive in the channel.
    Command Register: BAR0 + 7; // Write Only.
    Status Register: BAR0 + 7; // Read Only.
    Alternate Status Register: BAR1 + 2; // Read Only.
    Control Register: BAR1 + 2; // Write Only.
    DEVADDRESS: BAR1 + 3; // I don't know what is the benefit from this register.
The map above is the same with the secondary channel, but it uses BAR2 and BAR3 instead of BAR0 and BAR1. 
*/

// Only for PATA systems
/*
    BAR0 is the start of the I/O ports used by the primary channel.
    BAR1 is the start of the I/O ports which control the primary channel.
    BAR2 is the start of the I/O ports used by secondary channel.
    BAR3 is the start of the I/O ports which control secondary channel.
    BAR4 is the start of 8 I/O ports controls the primary channel's Bus Master IDE.
    BAR4 + 8 is the Base of 8 I/O ports controls secondary channel's Bus Master IDE.
*/

struct PCIIDEControllerData {
    IDEDiskDevice* devices[4];
};

// similar idea to read
void ide_write(IDEDiskDevice* device, unsigned char reg, unsigned char data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(device, ATA_REG_CONTROL, 0x80 | device->nIEN);
    if (reg < 0x08)
        outportb(device->port_base  + reg - 0x00, data);
    else if (reg < 0x0C)
        outportb(device->port_base  + reg - 0x06, data);
    else if (reg < 0x0E)
        outportb(device->port_ctrl  + reg - 0x0A, data);
    else if (reg < 0x16)
        outportb(device->port_bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(device, ATA_REG_CONTROL, device->nIEN);
}

u8 ide_read(IDEDiskDevice* device, u8 reg) {
    u8 result;
    // if register is HIGH (4th bit set) then assert bit 7 in CONTROL register before read
    if (reg > 0x07 && reg < 0x0C)
        ide_write(device, ATA_REG_CONTROL, 0x80 | device->nIEN);
    
    if (reg < 0x08)
        // registers 0x00-0x07 map to command block (BAR0/BAR2 + off)
        result = inportb(device->port_base + reg - 0x00);
    else if (reg < 0x0C)
        // same as above but we subtract 0x06 because we use the
        // alternate offset (asserting bit 7 before read)
        result = inportb(device->port_base  + reg - 0x06);
    else if (reg < 0x0E)
        // 0x0C and 0x0D map to the BAR1/BAR3 + off register
        result = inportb(device->port_ctrl  + reg - 0x0A);
    else if (reg < 0x16)
        // these are just the bus master (BAR4)
        result = inportb(device->port_bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        // reset if we were using the alternate offset
        ide_write(device, ATA_REG_CONTROL, device->nIEN);
    return result;
}

// when reading data from the drive just a helper function (reads 4 bytes each time)
void ide_read_buffer(IDEDiskDevice* device, u8 reg, void* buffer, u32 quads) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(device, ATA_REG_CONTROL, 0x80 | device->nIEN);
    if (reg < 0x08)
        insd(device->port_base  + reg - 0x00, buffer, quads);
    else if (reg < 0x0C)
        insd(device->port_base  + reg - 0x06, buffer, quads);
    else if (reg < 0x0E)
        insd(device->port_ctrl  + reg - 0x0C, buffer, quads);
    else if (reg < 0x16)
        insd(device->port_bmide + reg - 0x0E, buffer, quads);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(device, ATA_REG_CONTROL, device->nIEN);
}

// just a general wait function to check if things went well.
u8 ide_polling(IDEDiskDevice* device, u32 advanced_check) {

    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for (int i = 0; i < 15; i++)
        ide_read(device, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.
        // weirdly i read that a usual IO operation takes 30ns, so just in case I modified it to be 15 loops.
        // assuming that a processor runs at 3GHz, I could also just loop like 1200 times on a nop but lowk dtm.

    // (II) Wait for BSY to be cleared:
    // -------------------------------------------------
    while (ide_read(device, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait for BSY to be zero.

    if (advanced_check) {
        u8 state = ide_read(device, ATA_REG_STATUS); // Read Status Register.
        // (III) Check For Errors:
        // -------------------------------------------------
        if (state & ATA_SR_ERR)
            return 2; // Error.

        // (IV) Check If Device fault:
        // -------------------------------------------------
        if (state & ATA_SR_DF)
            return 1; // Device Fault.

        // (V) Check DRQ:
        // -------------------------------------------------
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & ATA_SR_DRQ) == 0)
            return 3; // DRQ should be set

    }

    return 0; // No Error.
}

// recommended for debugging
u8 ide_print_error(IDEDiskDevice* device, u8 err) {
    if (err == 0)
        return err;

    printf("IDE:");
    if (err == 1) {printf("- Device Fault\n     "); err = 19;}
    else if (err == 2) {
        u8 st = ide_read(device, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF)   {printf("- No Address Mark Found\n     ");   err = 7;}
        if (st & ATA_ER_TK0NF)   {printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_ABRT)   {printf("- Command Aborted\n     ");      err = 20;}
        if (st & ATA_ER_MCR)   {printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_IDNF)   {printf("- ID mark not Found\n     ");      err = 21;}
        if (st & ATA_ER_MC)   {printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_UNC)   {printf("- Uncorrectable Data Error\n     ");   err = 22;}
        if (st & ATA_ER_BBK)   {printf("- Bad Sectors\n     ");       err = 13;}
    } else  if (err == 3)           {printf("- Reads Nothing\n     "); err = 23;}
        else  if (err == 4)  {printf("- Write Protected\n     "); err = 8;}
    printf("- [%s %s] %s\n",
        (const char *[]){"Primary", "Secondary"}[device->channel], // Use the channel as an index into the array
        (const char *[]){"Master", "Slave"}[device->drive], // Same as above, using the drive
        device->model);

    return err;
}

// first important function (apparently there are some major errors in this so check that out.)
void ide_initialize(AstatineDriver* ide_driver, u32 BAR0, u32 BAR1, u32 BAR2, u32 BAR3, u32 BAR4) {
    struct PCIIDEControllerData* ide_data = (struct PCIIDEControllerData*)(ide_driver->reserved);
    int i, j, k, count = 0;

    // 1- Detect I/O Ports which interface IDE Controller:
    // this hardcodes example PATA ports here which may not fit in the future, beware.
    // this only happens if bar0-3 is not included
    // temporary data structure btw
    struct {
        u32 base;
        u32 ctrl;
        u32 bmide;
    } channels[2];
    channels[ATA_PRIMARY  ].base  = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
    channels[ATA_PRIMARY  ].ctrl  = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
    channels[ATA_SECONDARY].base  = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
    channels[ATA_SECONDARY].ctrl  = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
    channels[ATA_PRIMARY  ].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
    channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE

    IDEDiskDevice channel_devices[2] = {
        {.port_base = channels[ATA_PRIMARY].base,   .port_ctrl = channels[ATA_PRIMARY].ctrl,   .port_bmide = channels[ATA_PRIMARY].bmide, .nIEN = 0},
        {.port_base = channels[ATA_SECONDARY].base, .port_ctrl = channels[ATA_SECONDARY].ctrl, .port_bmide = channels[ATA_SECONDARY].bmide, .nIEN = 0}
    };

    u8* buf = ide_driver->kfp->kmalloc(2048); // we need a 512 byte buffer for IDENTIFY data

    // 2- Disable IRQs: (irqs are pain just poll esp because this driver is a singletasking driver.)
    ide_write(&channel_devices[ATA_PRIMARY]  , ATA_REG_CONTROL, 2);
    ide_write(&channel_devices[ATA_SECONDARY], ATA_REG_CONTROL, 2);

    IDEDiskDevice device = {0};

    // 3- Detect ATA-ATAPI Devices:
    for (i = 0; i < 2; i++) // for each controller
        for (j = 0; j < 2; j++) { // for each drive
            u8 err = 0, type = IDE_ATA, status;
            u8 drive_num = j + i * 2;

            // (I) Select Drive:
            ide_write(&channel_devices[i], ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
            sleep(1); // Wait 1ms for drive select to work.
                      // drive select takes a WHILE.

            // (II) Send ATA Identify Command:
            ide_write(&channel_devices[i], ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            sleep(1); // This function should be implemented in your OS. which waits for 1 ms.
                      // yea it is thank you :smile:

            // (III) Polling:
            if (ide_read(&channel_devices[i], ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.

            while(1) {
                // check status to see if not busy and if data request is ready (which is a sign it exists)
                status = ide_read(&channel_devices[i], ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
            }

            // (IV) Probe for ATAPI Devices:
            // we only probe here if err (from above) is set because
            // the device won't respond to an ATA identify command;
            // we ignore because we don't want to write a driver for a CD
            if (err != 0) {
                u8 cl = ide_read(&channel_devices[i], ATA_REG_LBA1);
                u8 ch = ide_read(&channel_devices[i], ATA_REG_LBA2);

                if (cl == 0x14 && ch == 0xEB){
                    continue; // we ignore ATAPI
                    type = IDE_ATAPI;
                } else if (cl == 0x69 && ch == 0x96){
                    continue;
                    type = IDE_ATAPI;
                } else continue; // Unknown Type (may not be a device).

                ide_write(&channel_devices[i], ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                sleep(1);
            }

            // (V) Read Identification Space of the Device:
            ide_read_buffer(&channel_devices[i], ATA_REG_DATA, buf, 128);

            // (VI) Read Device Parameters:
            device.base.sector_size = 512; // (almost) every ATA device has a sector size of 512 bytes

            device.type         = type;
            device.channel      = i;
            device.drive        = j;
            device.signature    = *((unsigned short *)(buf+ ATA_IDENT_DEVICETYPE));
            device.capabilities = *((unsigned short *)(buf + ATA_IDENT_CAPABILITIES));
            device.command_sets = *((unsigned int *)(buf + ATA_IDENT_COMMANDSETS));
            
            device.lba48chs     =  device.command_sets & (1 << 26) ? 2 : // Device uses 48-Bit Addressing.
                                  (device.capabilities  & (1 << 9)  ? 1 : 0); // Device uses LBA.
            device.dma_supported=  device.capabilities  & (1 << 8)  ? 1 : 0; // Device supports DMA.
                
            device.port_base    = channels[i].base;
            device.port_ctrl    = channels[i].ctrl;
            device.port_bmide   = channels[i].bmide;
            device.nIEN         = 0;

            // (VII) Get Size:
            if (device.command_sets & (1 << 26))
                // Device uses 48-Bit Addressing:
                device.base.sectors   = *((unsigned int *)(buf + ATA_IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                device.base.sectors   = *((unsigned int *)(buf + ATA_IDENT_MAX_LBA));

            // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
            for(k = 0; k < 40; k += 2) {
                device.model[k] = buf[ATA_IDENT_MODEL + k + 1];
                device.model[k + 1] = buf[ATA_IDENT_MODEL + k];
            }
            device.model[40] = 0; // Terminate String.
            // also get serial
            for(k = 0; k < 20; k += 2) {
                device.serial[k] = buf[ATA_IDENT_SERIAL + k + 1];
                device.serial[k + 1] = buf[ATA_IDENT_SERIAL + k];
            }
            device.serial[20] = 0; // Terminate String.

            device.base.base.name = device.model;
            device.base.base.type = DEVICE_TYPE_DISK;
            // connection of this device is handled by the PCI driver
            // the DISK subsystem doesn't care if it's PATA, SATA, SCSI, NVMe, etc
            device.base.base.conn = CONNECTION_TYPE_OTHER;
            device.base.base.size = sizeof(IDEDiskDevice);

            // This has no driver data right now because
            // we actually have to install the device into
            // the system and the system will manage it from there.
            Device* dev_registered = ide_driver->kfp->register_device(&device.base.base, ide_driver->device->id);
            if (dev_registered != null) {
                ide_data->devices[drive_num] = (IDEDiskDevice*)dev_registered;
                count++;
            }
        }

    (void)count;
    ide_driver->kfp->kfree(buf);
}

// why the fuck was selector listed as a parameter :sobprayskull: it can NOT be that deep
// for the rest:
// this function is used for both directions (read and write) which I don't like but i mean
// beggars can't be choosers ig :sob:
// drive is the which device r u using,
// lba is lba, numsects is numsects, buffer is buffer.
u8 ide_ata_access(IDEDiskDevice* device, u8 direction, u32 lba, u8 numsects, void* buffer) {
    // general variables to start
    u8  lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
    u8  lba_io[6];
    u32 slavebit     = device->drive; // Read the Drive [Master/Slave]
    u32 bus          = device->port_base; // Bus Base, like 0x1F0 which is also data port.
    u32 words      = 256; // Almost every ATA drive has a sector-size of 512-byte.
    u16 cyl, i;
    u8  head, sect, err;

    device->nIEN = 0x02;
    ide_write(device, ATA_REG_CONTROL, device->nIEN);

    // (I) Select one from LBA28, LBA48 or CHS;
    if (device->lba48chs == 2 || lba >= 0x0FFFFFFF) { 
                             // Sure Drive should support LBA in this case, or you are
                             // giving a wrong LBA. (ig this guy's idea is that either the drive
                             // is broken or you're a dumbass if you give a large LBA when the
                             // drive doesn't support which is lowk valid.)
        // LBA48:
        lba_mode  = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
    } else if (device->lba48chs == 1)  { // Drive supports LBA?
        // LBA28:
        lba_mode  = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0; // These Registers are not used here.
        lba_io[4] = 0; // These Registers are not used here.
        lba_io[5] = 0; // These Registers are not used here.
        head      = (lba & 0xF000000) >> 24;
    } else {
        // CHS:
        lba_mode  = 0;
        sect      = (lba % 63) + 1;
        cyl       = (lba + 1  - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head      = (lba + 1  - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
    }

    // (II) See if drive supports DMA or not;
    dma = 0; // We don't support DMA
             // lowk the sigma for this one

    // wait for drive to stop being busy (no timeout...)
    while (ide_read(device, ATA_REG_STATUS) & ATA_SR_BSY) {}

    // (IV) Select Drive from the controller;
    if (lba_mode == 0)
        ide_write(device, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        ide_write(device, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

    // (V) Write Parameters;
    // below is for extended mode
    if (lba_mode == 2) {
        ide_write(device, ATA_REG_SECCOUNT1,   0);
        ide_write(device, ATA_REG_LBA3,   lba_io[3]);
        ide_write(device, ATA_REG_LBA4,   lba_io[4]);
        ide_write(device, ATA_REG_LBA5,   lba_io[5]);
    }
    // we use the _EXT command for reading extended LBA
    // then we write LBA/CHS and which one we use is decided by REG_HDDEVSEL
    ide_write(device, ATA_REG_SECCOUNT0,   numsects);
    ide_write(device, ATA_REG_LBA0,   lba_io[0]);
    ide_write(device, ATA_REG_LBA1,   lba_io[1]);
    ide_write(device, ATA_REG_LBA2,   lba_io[2]);

    // "you're paid by the # of lines of code you write" ahh if statement
    if (lba_mode == 0 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 1 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO;   
    if (lba_mode == 2 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO_EXT;   
    if (lba_mode == 0 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 1 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 2 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA_EXT;
    if (lba_mode == 0 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 1 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 2 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 1 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 2 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA_EXT;
    ide_write(device, ATA_REG_COMMAND, cmd);    

    // After sending the command, we should poll and transfer sector(s).
    if (dma) {
        // DMA path not implemented yet.
        return 0;
    } else if (direction == 0) {
        // PIO Read.
        for (i = 0; i < numsects; i++) {
            err = ide_polling(device, 1);
            if (err)
                return err; // Polling, set error and exit if there is.
            insw(bus, (u16*)buffer, words);
            buffer += (words*2);
        }
    } else {
        // PIO Write.
        for (i = 0; i < numsects; i++) {
            err = ide_polling(device, 0);
            if (err)
                return err; // Polling, set error and exit if there is.
            outsw(bus, (u16*)buffer, words);
            buffer += (words*2);
        }
        ide_write(device, ATA_REG_COMMAND, (u8[]) { ATA_CMD_CACHE_FLUSH,
                ATA_CMD_CACHE_FLUSH,
                ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
        ide_polling(device, 0); // Polling.
    }

    return 0; // Easy, isn't it?
}

// just nice public facing API?
u8 ide_read_sectors(IDEDiskDevice* device, unsigned char numsects, unsigned int lba, void* buffer) {
    u8 package[1];

    // 2: Check if inputs are valid:
    // ==================================
    if (((lba + numsects) > device->base.sectors) && (device->type == IDE_ATA))
        package[0] = 0x2;                     // Seeking to invalid position.

    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        unsigned char err;
        if (device->type == IDE_ATA)
            err = ide_ata_access(device, 0, lba, numsects, buffer);
        package[0] = ide_print_error(device, err);
    }
    return package[0];
}
u8 ide_write_sectors(IDEDiskDevice* device, unsigned char numsects, unsigned int lba, void* buffer) {
    u8 package[1];

    // 2: Check if inputs are valid:
    // ==================================
    if (((lba + numsects) > device->base.sectors) && (device->type == IDE_ATA))
        package[0] = 0x2;                     // Seeking to invalid position.
    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        unsigned char err;
        if (device->type == IDE_ATA)
            err = ide_ata_access(device, /* ATA_WRITE */ 1, lba, numsects, buffer);
        else if (device->type == IDE_ATAPI)
            err = 4; // Write-Protected.
        package[0] = ide_print_error(device, err);
    }
    return package[0];
}

int controller_init(AstatineDriver* self) {
    // The AstatineDriver instance should contain the associated device
    // which is a PCI device in this case.
    PCIDevice* device = (PCIDevice*)self->device;
    memset(self->reserved, 0, sizeof(struct PCIIDEControllerData));
    ide_initialize(self, device->bars[0], device->bars[1], device->bars[2], device->bars[3], device->bars[4]);
    return 0;
}

// We need to unregister the children devices
void controller_deinit(AstatineDriver* self) {
    (void)self;
    // Let's take care of the 4 devices
    // which are taken care of by the driver subsystem.
}

// We can assume that device type and driver type were already checked

bool controller_check(Device* device, struct KernelFunctionPointers* kfp) {
    if (device->conn != CONNECTION_TYPE_PCI) return false;
    PCIDevice* pci_device = (PCIDevice*)device;
    if (pci_device->class_code == 0x01 && pci_device->subclass == 0x01) return true;
    return false;
}

AstatineDriverFile pci_ide_controller_driver = {
    .name = "pci_ide_controller",
    .device_type = DEVICE_TYPE_CONTROLLER,
    .verification = {0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE},
    .driver_type = CONNECTION_TYPE_PCI,
    .init = controller_init,
    .probe = null,
    .check = controller_check,
    .deinit = controller_deinit,
};

bool disk_check(Device* device, struct KernelFunctionPointers* kfp) {
    // we only need to check if the device is an IDE disk device
    if (device->type != DEVICE_TYPE_DISK)
        return false;
    if (!device->parent)
        return false;
    if (device->parent->type != DEVICE_TYPE_CONTROLLER)
        return false;
    if (device->parent->conn != CONNECTION_TYPE_PCI)
        return false;
    if (device->size != sizeof(IDEDiskDevice))
        return false;
    // we can check if the device has the ide disk functions
    // but for now we just assume that if it's a disk device
    // and it's connected to a pci ide controller, it's valid
    return true;
}

u32 disk_read(DiskDriver* driver, u8* buffer, u32 lba) {
    IDEDiskDevice* device = (IDEDiskDevice*)driver->base.device;
    return ide_read_sectors(device, 1, lba, buffer);
}

u32 disk_write(DiskDriver* driver, const u8* buffer, u32 lba) {
    IDEDiskDevice* device = (IDEDiskDevice*)driver->base.device;
    return ide_write_sectors(device, 1, lba, (void*)buffer);
}

int disk_init(AstatineDriver* driver) {
    return 0;
}

DiskDriverFile pci_ide_disk_driver = {
    .base = {
        .name = "pci_ide_disk",
        .device_type = DEVICE_TYPE_DISK,
        .driver_type = CONNECTION_TYPE_OTHER,
        .init = disk_init,
        .probe = null,
        .verification = {0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE},
        .check = disk_check,
    },
    .functions = {
        .read = disk_read,
        .write = disk_write,
    }
};
