#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "serial_dev.h"
#include "minichlink.h"

const uint8_t UARTBOOT_MAGIC[2] = { 0x57, 0xab };
const uint8_t UARTBOOT_CMD_ERASE = 0x81;
const uint8_t UARTBOOT_CMD_PROGRAM  = 0x80;
const uint8_t UARTBOOT_CMD_VERIFY = 0x82;
const uint8_t UARTBOOT_CMD_END = 0x83;

// It is different for X033/X035 series
const uint8_t UARTBOOT_MAX_DATA_SIZE = 60;

static void wch_uartboot_progressbar(uint32_t progress, uint32_t total);
static int wch_uartboot_send(void *dev, uint8_t *data, uint32_t len);
static int wch_uartboot_erase(void *dev);
static int wch_uartboot_send_data(void *dev, uint8_t cmd, uint8_t * data, uint32_t len);
static int wch_uartboot_program(void *dev, uint8_t * data, uint32_t len);
static int wch_uartboot_verify(void *dev, uint8_t * data, uint32_t len);
static int wch_uartboot_end(void *dev);

typedef struct {
	struct ProgrammerStructBase psb;
	serial_dev_t serial;
} uartboot_ctx_t;

void wch_uartboot_progressbar(uint32_t progress, uint32_t total)
{
    printf("\rProgress: [");
    for(int i = 0; i < 20; i++) {
        if(i * 5 <= (progress / (double)total * 100)) {
            printf("=");
        } else {
            printf(" ");
        }
    }
    printf("] %d%%", (int)(progress / (double)total * 100));
    fflush(stdout);
}

int wch_uartboot_send(void *dev, uint8_t *data, uint32_t len) {
    uint8_t buf[len + 3];
    uint8_t sum = 0;

    // Magic bytes
    buf[0] = UARTBOOT_MAGIC[0];
    buf[1] = UARTBOOT_MAGIC[1];

    // Data
    for(uint32_t i = 0; i < len; i++) {
        buf[i + 2] = data[i];
        sum += data[i];
    }

    // Checksum
    buf[len + 2] = sum;

    // Send data
    if (serial_dev_write(&((uartboot_ctx_t*)dev)->serial, buf, len + 3) == -1)
		return -2;

    // Receive status
    if (serial_dev_read(&((uartboot_ctx_t*)dev)->serial, buf, 2) == -1)
		return -2;

    return buf[0] == 0x00 && buf[1] == 0x00 ? 0 : -1;
}

int wch_uartboot_erase(void *dev) {
    // Rev 0 and 1 are empty
    uint8_t data[4] = { UARTBOOT_CMD_ERASE, 0x02, 0x00, 0x00 };
    return wch_uartboot_send(dev, data, 4);
}

int wch_uartboot_send_data(void *dev, uint8_t cmd, uint8_t * data, uint32_t len) {
    uint8_t buf[64];

    buf[0] = cmd;
    
    uint32_t index = 0;
    uint32_t to_send = len;

    while(to_send > 0)
    {
        uint8_t sector_size = to_send > 60 ? 60 : to_send;

        // Copy data
        memcpy(buf + 4, data + index, sector_size);

        to_send -= sector_size;
        index += sector_size;

        // Create a progress bar with frame based on the current progress (index / len * 100, 15 width) using printf
        wch_uartboot_progressbar(index, len);

        // Set length
        buf[1] = sector_size;

        // Send
        if(wch_uartboot_send(dev, buf, sector_size + 4) == -1) {
            return -1;
        }
    }

    printf("\n");

    return 0;
}

static int wch_uartboot_program(void *dev, uint8_t * data, uint32_t len)
{
    return wch_uartboot_send_data(dev, UARTBOOT_CMD_PROGRAM, data, len);
}

static int wch_uartboot_verify(void *dev, uint8_t * data, uint32_t len)
{
    return wch_uartboot_send_data(dev, UARTBOOT_CMD_VERIFY, data, len);
}

static int wch_uartboot_end(void *dev)
{
    // Data length, rev 0 and 1 are empty
    uint8_t data[4] = { UARTBOOT_CMD_END, 0x02, 0x00, 0x00 };
    return wch_uartboot_send(dev, data, 4);
}

int UARTBootExit(void *dev)
{
    serial_dev_close(&((uartboot_ctx_t*)dev)->serial);
	free(dev);
	return 0;
}

int UARTBootSetupInterface(void *dev)
{
    return 0;
}

int UARTBootWriteBinaryBlob(void *dev, uint32_t address_to_write, uint32_t blob_size, uint8_t *blob)
{
    if(address_to_write != 0x8000000) {
        perror("Address is fixed to flash section");
        return -1;
    }

    printf("Erasing flash\n");
    if(wch_uartboot_erase(dev) == -1) {
        fprintf(stderr, "Error while erasing flash.\n");
        return -1;
    }

    printf("Writing flash\n");
    if(wch_uartboot_program(dev, blob, blob_size) == -1)
    {
        fprintf(stderr, "Error while writing flash.\n");
        return -1;
    }

    printf("Verifying flash\n");
    if(wch_uartboot_verify(dev, blob, blob_size) == -1)
    {
        fprintf(stderr, "Verification of the program failed.\n");
        return -1;
    }

    printf("Jumping to application...\n");
    if(wch_uartboot_end(dev) == -1)
    {
        fprintf(stderr, "Error while jumping to application.\n");
        return -1;
    }

    return 0;
}

int UARTBootReadBinaryBlob(void * dev, uint32_t address_to_read_from, uint32_t read_size, uint8_t * blob)
{
    return -1;
}

int UARTBootErase(void * dev, uint32_t address, uint32_t length, int type)
{
    return wch_uartboot_erase(dev);
}

int UARTBootWriteReg32(void * dev, uint8_t reg_7_bit, uint32_t command)
{
	return -1; // Not supported
}

int UARTBootReadReg32(void * dev, uint8_t reg_7_bit, uint32_t * commandresp)
{
	return -1; // Not supported
}

int UARTBootFlushLLCommands(void * dev)
{
	return 0;
}

int UARTBootHaltMode(void * dev, int mode)
{
    return 0;
}

int UARTBootDelayUS(void * dev, int microseconds) {
	return 0;
}

void * TryInit_UARTBoot(const init_hints_t *hints)
{
    uartboot_ctx_t *ctx;

    if (!(ctx = calloc(sizeof(uartboot_ctx_t), 1))) {
		perror("calloc");
		return NULL;
	}

	const char* serial_to_open = NULL;

	// Get the serial port that shall be opened.
	// First, if we have a directly set serial port hint, use that.
	// Otherwise, use the environment variable MINICHLINK_SERIAL.
	// If that also doesn't exist, fall back to the default serial.
	if (hints && hints->serial_port != NULL) {
		serial_to_open = hints->serial_port;
	}
	else if ((serial_to_open = getenv("MINICHLINK_SERIAL")) == NULL) {
		// fallback
		serial_to_open = DEFAULT_SERIAL_NAME;
	}

    if (serial_dev_create(&ctx->serial, serial_to_open, 460800) == -1) {
		perror("create");
		return NULL;
	}

	if (serial_dev_open(&ctx->serial) == -1) {
		perror("open");
		return NULL;
	}

    MCF.WriteReg32 = UARTBootWriteReg32;
	MCF.ReadReg32 = UARTBootReadReg32;
	MCF.FlushLLCommands = UARTBootFlushLLCommands;
    MCF.WriteBinaryBlob = UARTBootWriteBinaryBlob;
    MCF.ReadBinaryBlob = UARTBootWriteBinaryBlob;
    MCF.DelayUS = UARTBootDelayUS;
    MCF.HaltMode = UARTBootHaltMode;
    MCF.Exit = UARTBootExit;
    MCF.SetupInterface = UARTBootSetupInterface;

    return ctx;
}