/*
 * Example receiver program
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

#include <stdio.h>
#include <stdlib.h>
#include <crazyradio.h>

#define VERSION "0.1"

int main(int argc, char *argv[]) {
    cradio_device_t *dev;
    unsigned char buffer[64];
    int res;
    int radio_id = -1;

    if(argc > 1) {
        radio_id = atoi(argv[1]);
    }

    fprintf(stderr, "rx-test: version %s\n", VERSION);

    cradio_init();
    dev = cradio_get(radio_id);

    if(!dev) {
        fprintf(stderr, "could not open device: %s\n", cradio_get_errorstr());
        exit(EXIT_FAILURE);
    }

    printf("Found device: %s\n", dev->model);
    printf("Serial: %s\n", dev->serial);
    printf("Firmware Version: %g\n", dev->firmware);

    if(cradio_set_channel(dev, 100) ||
       cradio_set_data_rate(dev, DATA_RATE_250KBPS) ||
       cradio_set_mode(dev, MODE_PRX)) {
        fprintf(stderr, "error setting up radio: %s\n", cradio_get_errorstr());
        exit(EXIT_FAILURE);
    }

    while(1) {
        res = cradio_read_packet(dev, buffer, 64, 0);
        if(res < 0) {
            fprintf(stderr, "error reading: %s\n", cradio_get_errorstr());
            exit(EXIT_FAILURE);
        }
        printf("received %d bytes of data: %s\n", res, buffer);
    }
}
