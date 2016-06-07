/*
 * C library for crazyradio
 *
 * Copyright (C) 2016 Ron Pedde (ron@pedde.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libusb.h>

#include "crazyradio.h"

#define ERROR_TYPE_USB    0
#define ERROR_TYPE_CRADIO 1

#define CRDEBUG(format, args...) cradio_log(4, format, ##args)
#define CRINFO(format, args...) cradio_log(3, format, ##args)
#define CRWARN(format, args...) cradio_log(2, format, ##args)
#define CRERROR(format, args...) cradio_log(1, format, ##args)
#define CRFATAL(format, args...) cradio_log(0, format, ##args)

#define CR_ERR_BADCHANNEL   1
#define CR_ERR_BADDATARATE  2
#define CR_ERR_BADPOWER     3
#define CR_ERR_BADARC       4
#define CR_ERR_BADARDTIME   5
#define CR_ERR_BADARDPKT    6
#define CR_ERR_BADMODE      7
#define CR_ERR_NODEVICE     8
#define CR_ERR_NOTENOUGH    9
#define CR_ERR_LAST         10


static const char *errtext[] = {
    "Success",
    "Invalid channel (must be 0-126)",
    "Invalid data rate (must be 0-2)",
    "Invalid power level (must be 0-3)",
    "Invalid retry count (must be 0-15)",
    "Invalid retry delay time (must be =< 4000)",
    "invalid retry packet size (must be 0-32)",
    "Invalid mode (must be 0 or 2)",
    "No crazyradio VID/PID found",
    "Cannot find specific radio device"
};

static libusb_context *context = NULL;
static int last_error = 0;
static int last_error_type = 0;
static void (*log_method)(int, char*, va_list) = NULL;
static int config_timeout = 1000;

/* Forwards */
static void cradio_log(int level, char *format, ...);


int cradio_init(void) {
    int rc;

    rc = libusb_init(&context);
    /* libusb_set_debug(context, LIBUSB_LOG_LEVEL_DEBUG); */

    CRDEBUG("initialized libcrazyradio");

    return rc;
}

void cradio_set_log_method(void(*fp)(int, char*, va_list)) {
    log_method = fp;
    CRDEBUG("set libcrazyradio log function");
}

void cradio_set_config_timeout(int timeout) {
    config_timeout = timeout;
    CRDEBUG("set libcrazyradio default to %d", timeout);
}

const char *cradio_get_errorstr(void) {
    if(last_error_type == ERROR_TYPE_USB) {
        return libusb_strerror(last_error);
    } else if((last_error_type == ERROR_TYPE_CRADIO) &&
              (last_error < CR_ERR_LAST))  {
        return errtext[last_error];
    }

    return "Unknown";
}


static int set_error(int error_type, int error_code) {
    last_error = error_code;
    last_error_type = error_type;
    return -1;
}

static int set_usb_error(int error_code) {
    CRDEBUG("Setting usb error of %02x", error_code);
    return set_error(ERROR_TYPE_USB, error_code);
}

static int set_cradio_error(int error_code) {
    CRDEBUG("Setting local error of %02x", error_code);
    return set_error(ERROR_TYPE_CRADIO, error_code);
}

static void cradio_log(int level, char *format, ...) {
    va_list args;

    if(log_method) {
        va_start(args, format);
        log_method(level, format, args);
        va_end(args);
    }
}

static void cradio_exit(char *format, ...) {
    va_list args;
    char *newfmt;
    char *prefix="ERROR: ";
    int len = strlen(format) + strlen(prefix) + 1;

    newfmt = (char*) malloc(len);

    if(newfmt) {
        sprintf(newfmt, "%s%s", prefix, format);

        va_start(args, format);
        vfprintf(stderr, newfmt, args);
        va_end(args);
    } else {
        perror("malloc");
    }

    exit(EXIT_FAILURE);
}

cradio_device_t *cradio_get(int device_id) {
    cradio_device_t *prd;
    libusb_device **list = NULL;
    libusb_device_handle *handle;
    int count;
    int rc;
    int res;
    int devfound = 0;

    CRDEBUG("Walking usb device list");

    count = libusb_get_device_list(context, &list);
    if(count > 0) {
        int devidx = 0;
        for(int idx = 0; idx < count; idx++) {
            libusb_device *device = list[idx];
            struct libusb_device_descriptor desc;

            rc = libusb_get_device_descriptor(device, &desc);
            if(rc) {
                set_usb_error(rc);
                return NULL;
            }

            CRDEBUG("Found device %04x:%04x", desc.idVendor, desc.idProduct);

            if((desc.idVendor == CRADIO_VID) && \
               (desc.idProduct == CRADIO_PID)) {
                CRDEBUG("Found crazyradio device");
                devfound = 1;

                if((device_id == devidx) || (device_id == -1)) {
                    CRDEBUG("Claiming this USB device");

                    prd = (cradio_device_t *)malloc(sizeof(cradio_device_t));
                    if(!prd)
                        cradio_exit("malloc error");
                    memset(prd, 0, sizeof(cradio_device_t));

                    prd->firmware = 10.0 * (desc.bcdDevice >> 12);
                    prd->firmware += (desc.bcdDevice >> 8) & 0xF;
                    prd->firmware += ((desc.bcdDevice & 0xFF) >> 4) / 10.0;
                    prd->firmware += (desc.bcdDevice & 0x0F) / 100.0;

                    prd->serial = (char *)malloc(256);
                    prd->model = (char *)malloc(256);

                    if((!prd->serial) || (!prd->model))
                        cradio_exit("malloc error");

                    memset(prd->serial, 0, 256);
                    memset(prd->model, 0, 256);

                    CRDEBUG("Opening device");
                    rc = libusb_open(device,
                                     (libusb_device_handle**)&prd->pusb_handle);

                    handle = (libusb_device_handle*)prd->pusb_handle;

                    /* libusb_free_device_list(list, 1); */

                    if(rc != 0) {
                        set_usb_error(rc);
                        cradio_close(prd);
                        libusb_free_device_list(list, 1);
                        return NULL;
                    }

                    CRDEBUG("Claiming interface");
                    rc = libusb_claim_interface(handle, 0);

                    if(!rc) {
                        CRDEBUG("Getting serial descriptor (%d)",
                                desc.iSerialNumber);
                        res = libusb_get_string_descriptor_ascii(
                            handle, desc.iSerialNumber, prd->serial, 256);
                        if(res < 0)
                            rc = res;
                    }

                    if(!rc) {
                        CRDEBUG("Getting product descriptor (%d)",
                            desc.iProduct);
                        res = libusb_get_string_descriptor_ascii(
                            handle, desc.iProduct, prd->model, 256);
                        if(res < 0)
                            rc = res;
                    }

                    if(rc != 0) {
                        set_usb_error(rc);
                        cradio_close(prd);
                        libusb_free_device_list(list, 1);
                        return NULL;
                    }

                    libusb_free_device_list(list, 1);
                    return prd;
                }
                devidx++;
            }
        }
    }
    libusb_free_device_list(list, 1);
    set_cradio_error(devfound ? CR_ERR_NOTENOUGH : CR_ERR_NODEVICE);
    return NULL;
}

static int cradio_send_config(cradio_device_t *prd, uint8_t request,
                              uint16_t value, uint16_t index,
                              unsigned char *data, uint16_t length) {
    int rc;
    libusb_device_handle *handle = (libusb_device_handle*)prd->pusb_handle;

    CRDEBUG("Performing control transfer with timeout of %d", config_timeout);

    rc = libusb_control_transfer(handle, LIBUSB_REQUEST_TYPE_VENDOR,
                                 request, value, index, data, length,
                                 config_timeout);

    if(rc != length)
        return set_usb_error(rc);

    return 0;
}


/* Set the radio channel on the nRF radio.
 *
 * the nRF24LU1 chip provides 126 channels of 1MHz from 2400MHz to 2525MHz.
 * the channel parameter must be between 0 and 126, or the command will be
 * ignored.
 */
int cradio_set_channel(cradio_device_t *prd, uint16_t channel) {
    if(channel > 126)
        return set_cradio_error(CR_ERR_BADCHANNEL);

    CRDEBUG("Setting channel to %02x", channel);
    return cradio_send_config(prd, CONF_SET_RADIO_CHANNEL, channel, 0, NULL, 0);
}

/* Set the radio address
 *
 * This is the 5 byte address for transmitting to, or receiving from
 * (depending on mode).
 *
 * The default address is 0xE7E7E7E7E7
 */
int cradio_set_address(cradio_device_t *prd, cradio_address address) {
    CRDEBUG("Setting address to %02x %02x %02x %02x %02x", address[0],
            address[1], address[2], address[3], address[4]);

    return cradio_send_config(prd, CONF_SET_RADIO_ADDRESS, 0, 0,
                              (uint8_t*)address, 5);
}

/* Set the data rate
 *
 * Valid data rates:
 *
 * DATA_RATE_250KBPS
 * DATA_RATE_1MBPS
 * DATA_RATE_2MBPS
 */
int cradio_set_data_rate(cradio_device_t *prd, uint16_t data_rate) {
    if(data_rate > DATA_RATE_2MBPS)
        return set_cradio_error(CR_ERR_BADDATARATE);

    CRDEBUG("Setting data rate to %02x", data_rate);
    return cradio_send_config(prd, CONF_SET_DATA_RATE, data_rate, 0, NULL, 0);
}

/* Set the transmit power
 *
 * Valid power levels are:
 * POWER_M18DBM       (-18dBm, value: 0)
 * POWER_M12DBM       (-12dBm, value: 1)
 * POWER_M6DBM        (-6dBm, value: 2)
 * POWER_0DBM         (0dBm, value: 3)
 */
int cradio_set_power(cradio_device_t *prd, uint16_t power) {
    if(power > POWER_0DBM)
        return set_cradio_error(CR_ERR_BADPOWER);

    CRDEBUG("Setting power to %02x", power);
    return cradio_send_config(prd, CONF_SET_RADIO_POWER, power, 0, NULL, 0);
}

/* Set ack enable */
int cradio_set_ack_enable(cradio_device_t *prd, uint16_t enable_status) {
    CRDEBUG("%sabling auto-ack", enable_status ? "en" : "dis");
    return cradio_send_config(prd, CONF_ACK_ENABLE, enable_status, 0, NULL, 0);
}


/* Set the ACK retry count
 *
 * Number of times the radio will retry a packaet if ack is not
 * received
 *
 * value must be 0-15
 */
int cradio_set_arc(cradio_device_t *prd, uint16_t arc) {
    if(arc > 15)
        return set_cradio_error(CR_ERR_BADARC);

    return cradio_send_config(prd, CONF_SET_RADIO_ARC, arc, 0, NULL, 0);
}

/* Set the ACK retry delay
 *
 * Ack retry delay is from 250uS to 4000uS, in 250uS
 * increments.  Values greater than 4000uS will be
 * ignored
 */
int cradio_set_ard_time(cradio_device_t *prd, int us) {
    uint16_t ard_time;

    if(us > 4000)
        return set_cradio_error(CR_ERR_BADARDTIME);

    ard_time = (us / 150) - 1;

    CRDEBUG("Setting ard time %02x", ard_time);
    return cradio_send_config(prd, CONF_SET_RADIO_ARD, ard_time, 0, NULL, 0);
}

/* Set the ACK retry ack size (in bytes)
 *
 * The auto retry delay depends on the length of the ack packet.
 * larger packets mean longer delay.  ARD can be configured either
 * by time (cradio_set_ard_time) or by pack payload length.  If ack
 * payload length is set, then ARD will be altered even if the
 * data rate is changed.
 *
 * size can be 0-32 bytes
 */
int cradio_set_ard_bytes(cradio_device_t *prd, uint16_t bytes) {
    if(bytes > 32)
        return set_cradio_error(CR_ERR_BADARDPKT);

    CRDEBUG("Setting ard bytes to %02x", bytes);
    return cradio_send_config(prd, CONF_SET_RADIO_ARD,
                              bytes | 0x80, 0, NULL, 0);
}

/* Set the radio mode
 *
 * Mode can be MODE_PTX or MODE_PRX to be in transmit or
 * receive mode
 */
int cradio_set_mode(cradio_device_t *prd, uint16_t mode) {
    if((mode != MODE_PTX) && (mode != MODE_PRX))
        return set_cradio_error(CR_ERR_BADMODE);

    CRDEBUG("Setting mode to %s", mode == MODE_PTX ? "PTX" : "PRX");
    return cradio_send_config(prd, CONF_SET_RADIO_MODE, mode, 0, NULL, 0);
}

/* transfer (read or write) a packet to a bulk endpoint */
static int cradio_xfer_packet(cradio_device_t *prd, unsigned char endpoint,
                              unsigned char *buffer, int len, int timeout) {
    int rc;
    int xferred;

    libusb_device_handle *handle = (libusb_device_handle*)prd->pusb_handle;

    CRDEBUG("%sing %d bytes", endpoint & 0x80 ? "receiv" : "send", len);
    rc = libusb_bulk_transfer(handle, endpoint, buffer, len, &xferred, timeout);
    if(rc && rc != LIBUSB_ERROR_TIMEOUT)
        return set_usb_error(rc);

    CRDEBUG("received %d bytes", xferred);

    return xferred;
}

/* receive a packet(only valid in PRX mode) */
int cradio_read_packet(cradio_device_t *prd, unsigned char *buffer,
                       int len, int timeout) {
    return cradio_xfer_packet(prd, 0x81, buffer, len, timeout);
}

/* write a packet (only valid in PTX mode) */
int cradio_write_packet(cradio_device_t *prd, unsigned char *buffer,
                        int len, int timeout) {
    return cradio_xfer_packet(prd, 0x01, buffer, len, timeout);
}

int cradio_close(cradio_device_t *prd) {
    libusb_device_handle *handle = (libusb_device_handle*)prd->pusb_handle;

    CRDEBUG("Closing device");

    if(prd) {
        if(handle) {
            libusb_release_interface(handle, 1);
            libusb_close(handle);
        }

        if(prd->serial)
            free(prd->serial);

        if(prd->model)
            free(prd->model);

        free(prd);
    }

    return 0;
}
