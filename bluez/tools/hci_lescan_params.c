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
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define RFKILL_EVENT_SIZE_V1	8

enum rfkill_type {
	RFKILL_TYPE_ALL = 0,
	RFKILL_TYPE_WLAN,
	RFKILL_TYPE_BLUETOOTH,
	RFKILL_TYPE_UWB,
	RFKILL_TYPE_WWAN,
	RFKILL_TYPE_GPS,
	RFKILL_TYPE_FM,
	NUM_RFKILL_TYPES,
};

enum rfkill_operation {
	RFKILL_OP_ADD = 0,
	RFKILL_OP_DEL,
	RFKILL_OP_CHANGE,
	RFKILL_OP_CHANGE_ALL,
};

struct rfkill_event {
	uint32_t idx;
	uint8_t  type;
	uint8_t  op;
	uint8_t  soft;
	uint8_t  hard;
};

static bool blocked = true;

static int set_leparams()
{
	int err, dd, dev_id = 0;
	uint8_t scan_type = 0x01;
	uint8_t own_type = 0x00;
	uint16_t interval = htobs(0x0010);
	uint16_t window = htobs(0x0010);

	printf("Set LE scan parameters\n");
	dev_id = hci_get_route(NULL);
	if (dev_id < 0) {
		perror("Device is not available");
		err = -1;
		goto out;
	}

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		perror("HCI device open failed");
		err = -1;
		goto out;
	}

	err = hci_le_set_scan_parameters(dd, scan_type, interval, window,
							own_type, 0x00, 1000);
	if (err < 0) {
		perror("Set scan parameters failed");
	}

out:
	if (dd)
		hci_close_dev(dd);

	return err;
}

static int hcidevup(bool up)
{
	struct hci_dev_info dev_info;
	int fd = -1;
	int err = 0;
	uint16_t bt_device= 0;

	fd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (fd < 0) {
		printf("Cannot open BT socket: %s(%d)\n", strerror(errno), errno);
		err = -errno;
		goto out;
	}

	memset(&dev_info, 0, sizeof(dev_info));
	dev_info.dev_id = bt_device;
	if (ioctl(fd, HCIGETDEVINFO, &dev_info) < 0) {
		printf("Cannot get BT info: %s(%d)\n", strerror(errno), errno);
		err = -errno;
		goto out;
	}

	if (up) {
		if (ioctl(fd, HCIDEVUP, dev_info.dev_id) < 0) {
			printf("Cannot raise BT dev %d: %s(%d)\n", dev_info.dev_id,
						strerror(errno), errno);
			if (errno != ERFKILL && errno != EALREADY) {
				err = -errno;
				goto out;
			}
		}
		printf("HCI DEV UP\n");
	} else {
		if (ioctl(fd, HCIDEVDOWN, dev_info.dev_id) < 0) {
			printf("Cannot put BT dev %d down: %s(%d)\n",
				dev_info.dev_id, strerror(errno), errno);
			if (errno != ERFKILL && errno != EALREADY) {
				err = -errno;
				goto out;
			}
		}
		printf("HCI DEV DOWN\n");
	}
out:
	if (fd >= 0)
		close(fd);

	return err;
}

static int rfkill_block(bool block)
{
	struct rfkill_event event;
	ssize_t len;
	int fd, err = 0;
	fd = open("/dev/rfkill", O_RDWR | O_CLOEXEC);
	if (fd < 0)
		return -errno;

	memset(&event, 0, sizeof(event));
	event.op = RFKILL_OP_CHANGE_ALL;
	event.type = RFKILL_TYPE_BLUETOOTH;
	event.soft = block;

	len = write(fd, &event, sizeof(event));
	if (len < 0) {
		err = -errno;
		perror("Failed to change RFKILL state");
	} else {
		printf("BT device %s\n", (block? "blocked" : "unblocked"));
	}

	close(fd);

	return err;
}

struct rfkill_id {
	union {
		enum rfkill_type type;
		uint32_t index;
	};
	enum {
		RFKILL_IS_INVALID,
		RFKILL_IS_TYPE,
		RFKILL_IS_INDEX,
	} result;
};

static const char *get_name(uint32_t idx)
{
	static char name[128] = {};
	char *pos, filename[64];
	int fd;

	snprintf(filename, sizeof(filename) - 1,
				"/sys/class/rfkill/rfkill%u/name", idx);

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return NULL;

	memset(name, 0, sizeof(name));
	read(fd, name, sizeof(name) - 1);

	pos = strchr(name, '\n');
	if (pos)
		*pos = '\0';

	close(fd);

	return name;
}

static int rfkill_list()
{
	struct rfkill_id id;
	struct rfkill_event event;
	const char *name;
	ssize_t len;
	int fd;

	id.type = RFKILL_TYPE_BLUETOOTH;
	id.result = RFKILL_IS_TYPE;

	fd = open("/dev/rfkill", O_RDONLY);
	if (fd < 0) {
		perror("Can't open RFKILL control device");
		return 1;
	}

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("Can't set RFKILL control device to non-blocking");
		close(fd);
		return 1;
	}

	while (1) {
		len = read(fd, &event, sizeof(event));
		if (len < 0) {
			if (errno == EAGAIN)
				break;
			perror("Reading of RFKILL events failed");
			break;
		}

		if (len != RFKILL_EVENT_SIZE_V1) {
			fprintf(stderr, "Wrong size of RFKILL event\n");
			continue;
		}

		if (event.op != RFKILL_OP_ADD)
			continue;

		/* filter out unwanted results */
		switch (id.result)
		{
		case RFKILL_IS_TYPE:
			if (event.type != id.type)
				continue;
			break;
		case RFKILL_IS_INDEX:
			if (event.idx != id.index)
				continue;
			break;
		case RFKILL_IS_INVALID:; /* must be last */
		}

		name = get_name(event.idx);

		printf("%u: %s: %d\n", event.idx, name, event.type);
		printf("\tSoft blocked: %s\n", event.soft ? "yes" : "no");
		printf("\tHard blocked: %s\n", event.hard ? "yes" : "no");
		if (event.hard)
			return 1;

		blocked = event.soft ? true : false;
	}

	close(fd);
	return 0;
}

int main()
{
	if (rfkill_list()) {
		printf("failed to read current BT block state");
		goto out;
	}

	if (blocked) {
		if (rfkill_block(false)) {
			printf("Failed to unblock BT device\n");
			goto out;
		}
	}

	if (hcidevup(true)) {
		printf("Failed to bring BT device up\n");
		goto out;
	}

	if (set_leparams())
		printf("Failed to set le parameters\n");

	if (hcidevup(false)) {
		printf("Failed to bring BT device down\n");
		goto out;
	}

	if (blocked) {
		if (rfkill_block(true)) {
			printf("Failed to block BT device\n");
			goto out;
		}
	}

out:
	return 0;
}
