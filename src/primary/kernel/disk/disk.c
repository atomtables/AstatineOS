// Disk driver for Hard Drives that are PATA-only. not very sure outside of that.
// will definitely require a rewrite when the system becomes more complex and we start having more files.
// at that point the PCI subsystem will need to be integrated more deeply + itll use IRQ methods.
#include <modules/modules.h>
#include <display/simple/display.h>
#include <timer/PIT.h>
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
struct IDEChannelRegisters {
   u16 base;  // I/O Base. (BAR0/BAR2)
   u16 ctrl;  // Control Base (BAR1/BAR3)
   u16 bmide; // Bus Master IDE (BAR4/BAR4+8)
   u8  nIEN;  // nIEN (No Interrupt) (not sure what this is)
} channels[2];

u8 ide_buf[2048] = {0};
volatile static bool ide_irq_invoked = 0;

// up to 4 devices on the IDE bus
struct IDEDevice {
   u8  reserved;    // 0 (Empty) or 1 (Has disk).
   u8  channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
   u8  drive;       // 0 (Master Drive) or 1 (Slave Drive).
   u16 type;        // 0: ATA, 1:ATAPI.
   u16 signature;   // Drive Signature
   u16 capabilities;// Features.
   u32 command_sets;// Command Sets Supported.
   u32 size;        // Size in Sectors.
   u8  model[41];   // Model in string.
} ide_devices[4];

// similar idea to read
void ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   if (reg < 0x08)
      outportb(channels[channel].base  + reg - 0x00, data);
   else if (reg < 0x0C)
      outportb(channels[channel].base  + reg - 0x06, data);
   else if (reg < 0x0E)
      outportb(channels[channel].ctrl  + reg - 0x0A, data);
   else if (reg < 0x16)
      outportb(channels[channel].bmide + reg - 0x0E, data);
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

u8 ide_read(u8 channel, u8 reg) {
    u8 result;
    // if register is HIGH (4th bit set) then assert bit 7 in CONTROL register before read
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    
    if (reg < 0x08)
        // registers 0x00-0x07 map to command block (BAR0/BAR2 + off)
        result = inportb(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        // same as above but we subtract 0x06 because we use the
        // alternate offset (asserting bit 7 before read)
        result = inportb(channels[channel].base  + reg - 0x06);
    else if (reg < 0x0E)
        // 0x0C and 0x0D map to the BAR1/BAR3 + off register
        result = inportb(channels[channel].ctrl  + reg - 0x0A);
    else if (reg < 0x16)
        // these are just the bus master (BAR4)
        result = inportb(channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        // reset if we were using the alternate offset
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    return result;
}

// when reading data from the drive just a helper function (reads 4 bytes each time)
void ide_read_buffer(u8 channel, u8 reg, void* buffer, u32 quads) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        insd(channels[channel].base  + reg - 0x00, buffer, quads);
    else if (reg < 0x0C)
        insd(channels[channel].base  + reg - 0x06, buffer, quads);
    else if (reg < 0x0E)
        insd(channels[channel].ctrl  + reg - 0x0C, buffer, quads);
    else if (reg < 0x16)
        insd(channels[channel].bmide + reg - 0x0E, buffer, quads);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

// just a general wait function to check if things went well.
u8 ide_polling(u8 channel, u32 advanced_check) {

    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for (int i = 0; i < 15; i++)
        ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.
        // weirdly i read that a usual IO operation takes 30ns, so just in case I modified it to be 15 loops.
        // assuming that a processor runs at 3GHz, I could also just loop like 1200 times on a nop but lowk dtm.

    // (II) Wait for BSY to be cleared:
    // -------------------------------------------------
    bool timeout = false;
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait for BSY to be zero.

    if (advanced_check) {
        u8 state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.

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
u8 ide_print_error(u32 drive, u8 err) {
    if (err == 0)
        return err;

    display.printf("IDE:");
    if (err == 1) {display.printf("- Device Fault\n     "); err = 19;}
    else if (err == 2) {
        u8 st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF)   {display.printf("- No Address Mark Found\n     ");   err = 7;}
        if (st & ATA_ER_TK0NF)   {display.printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_ABRT)   {display.printf("- Command Aborted\n     ");      err = 20;}
        if (st & ATA_ER_MCR)   {display.printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_IDNF)   {display.printf("- ID mark not Found\n     ");      err = 21;}
        if (st & ATA_ER_MC)   {display.printf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_UNC)   {display.printf("- Uncorrectable Data Error\n     ");   err = 22;}
        if (st & ATA_ER_BBK)   {display.printf("- Bad Sectors\n     ");       err = 13;}
    } else  if (err == 3)           {display.printf("- Reads Nothing\n     "); err = 23;}
        else  if (err == 4)  {display.printf("- Write Protected\n     "); err = 8;}
    display.printf("- [%s %s] %s\n",
        (const char *[]){"Primary", "Secondary"}[ide_devices[drive].channel], // Use the channel as an index into the array
        (const char *[]){"Master", "Slave"}[ide_devices[drive].drive], // Same as above, using the drive
        ide_devices[drive].model);

    return err;
}

// first important function (apparently there are some major errors in this so check that out.)
void ide_initialize(u32 BAR0, u32 BAR1, u32 BAR2, u32 BAR3, u32 BAR4) {
    int i, j, k, count = 0;

    // 1- Detect I/O Ports which interface IDE Controller:
    // this hardcodes example PATA ports here which may not fit in the future, beware.
    channels[ATA_PRIMARY  ].base  = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
    channels[ATA_PRIMARY  ].ctrl  = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
    channels[ATA_SECONDARY].base  = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
    channels[ATA_SECONDARY].ctrl  = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
    channels[ATA_PRIMARY  ].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
    channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE
    // 2- Disable IRQs: (irqs are pain just poll esp because this driver is a singletasking driver.)
    ide_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);
    // 3- Detect ATA-ATAPI Devices:
    for (i = 0; i < 2; i++) // for each controller
        for (j = 0; j < 2; j++) { // for each drive
            u8 err = 0, type = IDE_ATA, status;
            ide_devices[count].reserved = 0; // Assuming that no drive here.

            // (I) Select Drive:
            ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
            sleep(1); // Wait 1ms for drive select to work.
                      // drive select takes a WHILE.

            // (II) Send ATA Identify Command:
            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            sleep(1); // This function should be implemented in your OS. which waits for 1 ms.
                      // yea it is thank you :smile:

            // (III) Polling:
            if (ide_read(i, ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.

            while(1) {
                // check status to see if not busy and if data request is ready (which is a sign it exists)
                status = ide_read(i, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
            }

            // (IV) Probe for ATAPI Devices:
            // we only probe here if err (from above) is set because
            // the device won't respond to an ATA identify command;
            // we ignore because we don't want to write a driver for a CD
            if (err != 0) {
                u8 cl = ide_read(i, ATA_REG_LBA1);
                u8 ch = ide_read(i, ATA_REG_LBA2);

                if (cl == 0x14 && ch == 0xEB){
                    continue; // we ignore ATAPI
                    type = IDE_ATAPI;
                } else if (cl == 0x69 && ch == 0x96){
                    continue;
                    type = IDE_ATAPI;
                } else
                    continue; // Unknown Type (may not be a device).

                ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                sleep(1);
            }

            // (V) Read Identification Space of the Device:
            ide_read_buffer(i, ATA_REG_DATA, ide_buf, 128);

            // (VI) Read Device Parameters:
            ide_devices[count].reserved     = 1;
            ide_devices[count].type         = type;
            ide_devices[count].channel      = i;
            ide_devices[count].drive        = j;
            ide_devices[count].signature    = *((unsigned short *)(ide_buf + ATA_IDENT_DEVICETYPE));
            ide_devices[count].capabilities = *((unsigned short *)(ide_buf + ATA_IDENT_CAPABILITIES));
            ide_devices[count].command_sets  = *((unsigned int *)(ide_buf + ATA_IDENT_COMMANDSETS));

            // (VII) Get Size:
            if (ide_devices[count].command_sets & (1 << 26))
                // Device uses 48-Bit Addressing:
                ide_devices[count].size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                ide_devices[count].size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA));

            // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
            for(k = 0; k < 40; k += 2) {
                ide_devices[count].model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                ide_devices[count].model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];}
            ide_devices[count].model[40] = 0; // Terminate String.

            count++;
        }

    // 4- Print Summary:
    for (i = 0; i < 4; i++)
        if (ide_devices[i].reserved == 1) {
            display.printf(" Found %s Drive %dGB - %s\n",
                (const char *[]){"ATA", "ATAPI"}[ide_devices[i].type],         /* Type */
                ide_devices[i].size / 1024 / 1024 / 2,               /* Size */
                ide_devices[i].model);
        }
}

// why the fuck was selector listed as a parameter :sobprayskull: it can NOT be that deep
// for the rest:
// this function is used for both directions (read and write) which I don't like but i mean
// beggars can't be choosers ig :sob:
// drive is the which device r u using,
// lba is lba, numsects is numsects, buffer is buffer.
u8 ide_ata_access(u8 direction, u8 drive, u32 lba, u8 numsects, void* buffer) {
    // general variables to start
    u8  lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
    u8  lba_io[6];
    u32 channel      = ide_devices[drive].channel; // Read the Channel.
    u32 slavebit     = ide_devices[drive].drive; // Read the Drive [Master/Slave]
    u32 bus = channels[channel].base; // Bus Base, like 0x1F0 which is also data port.
    u32 words      = 256; // Almost every ATA drive has a sector-size of 512-byte.
    u16 cyl, i;
    u8  head, sect, err;

    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);

    // (I) Select one from LBA28, LBA48 or CHS;
    if (lba >= 0x10000000) { // Sure Drive should support LBA in this case, or you are
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
    } else if (ide_devices[drive].capabilities & 0x200)  { // Drive supports LBA?
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
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY) {}

    // (IV) Select Drive from the controller;
    if (lba_mode == 0)
        ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

    // (V) Write Parameters;
    // below is for extended mode
    if (lba_mode == 2) {
        ide_write(channel, ATA_REG_SECCOUNT1,   0);
        ide_write(channel, ATA_REG_LBA3,   lba_io[3]);
        ide_write(channel, ATA_REG_LBA4,   lba_io[4]);
        ide_write(channel, ATA_REG_LBA5,   lba_io[5]);
    }
    // we use the _EXT command for reading extended LBA
    // then we write LBA/CHS and which one we use is decided by REG_HDDEVSEL
    ide_write(channel, ATA_REG_SECCOUNT0,   numsects);
    ide_write(channel, ATA_REG_LBA0,   lba_io[0]);
    ide_write(channel, ATA_REG_LBA1,   lba_io[1]);
    ide_write(channel, ATA_REG_LBA2,   lba_io[2]);

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
    ide_write(channel, ATA_REG_COMMAND, cmd);    

    // After sending the command, we should poll, then we read/write a sector, 
    // then we should poll, then we read/write a sector, until we read/write all 
    // sectors needed, if an error has happened, the function will return a specific error code. 
    if (dma)
        if (direction == 0);
            // DMA Read.
        else;
            // DMA Write.
    else
        if (direction == 0)
            // PIO Read.
        for (i = 0; i < numsects; i++) {
            if (err = ide_polling(channel, 1))
                return err; // Polling, set error and exit if there is.
            insw(bus, (u16*)buffer, words);
            buffer += (words*2);
        } else {
            // PIO Write.
            for (i = 0; i < numsects; i++) {
                if (err = ide_polling(channel, 0))
                    return err; // Polling, set error and exit if there is.
                outsw(bus, (u16*)buffer, words);
                buffer += (words*2);
            }
            ide_write(channel, ATA_REG_COMMAND, (char []) {   ATA_CMD_CACHE_FLUSH,
                            ATA_CMD_CACHE_FLUSH,
                            ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
            ide_polling(channel, 0); // Polling.
        }

    return 0; // Easy, isn't it?
}

// why tf does this even exist :pray:
u8 package[1];

// just nice public facing API?
u8 ide_read_sectors(unsigned char drive, unsigned char numsects, unsigned int lba, void* buffer) {
    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].reserved == 0) package[0] = 0x1;      // Drive Not Found!

    // 2: Check if inputs are valid:
    // ==================================
    else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
        package[0] = 0x2;                     // Seeking to invalid position.

    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        unsigned char err;
        if (ide_devices[drive].type == IDE_ATA)
            err = ide_ata_access(/* ATA_READ */0, drive, lba, numsects, buffer);
        package[0] = ide_print_error(drive, err);
    }
    return package[0];
}
u8 ide_write_sectors(unsigned char drive, unsigned char numsects, unsigned int lba, void* buffer) {

    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].reserved == 0)
        package[0] = 0x1;      // Drive Not Found!
    // 2: Check if inputs are valid:
    // ==================================
    else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
        package[0] = 0x2;                     // Seeking to invalid position.
    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        unsigned char err;
        if (ide_devices[drive].type == IDE_ATA)
            err = ide_ata_access(/* ATA_WRITE */ 1, drive, lba, numsects, buffer);
        else if (ide_devices[drive].type == IDE_ATAPI)
            err = 4; // Write-Protected.
        package[0] = ide_print_error(drive, err);
    }
    return package[0];
}