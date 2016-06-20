/*
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2016 Jolla Ltd. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int main(/*int argc, char *argv[]*/)
{
	int err, dd, dev_id;
	uint8_t scan_type = 0x01;
	uint8_t own_type = 0x00;
	uint16_t interval = htobs(0x0010);
	uint16_t window = htobs(0x0010);

	printf("Set LE scan parameters\n");
//	if (dev_id < 0) {
	dev_id = hci_get_route(NULL);
	if (dev_id < 0) {
		perror("Device is not available");
		exit(1);
	}
//	}

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		perror("HCI device open failed");
		exit(1);
	}

	err = hci_le_set_scan_parameters(dd, scan_type, interval, window,
							own_type, 0x00, 1000);
	if (err < 0) {
		perror("Set scan parameters failed");
		exit(1);
	}

	hci_close_dev(dd);
	return 0;
}
