/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2016 Jolla Ltd. All rights reserved.
 *  Contact: Martin Jones <martin.jones@jollamobile.com>
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

/* Voicecall interface
 *
 * Allows dialing via a registered agent
 */

struct voicecall; /* Opaque */

struct voicecall_table {
        int (*dial)(const gchar *number);
};

/*** Functions for voicecall users ***/

/*
 * Initiate a voicecall.
 */
int voicecall_dial(const gchar *name);

/*** Functions for voicecall implementors ***/

/*
 * Register a voicecall implementation. Only one implementation may be
 * registered at a time. In the absence of an implementation, all
 * voicecall functions are no-ops.
 */
int voicecall_plugin_register(const gchar *name, const struct voicecall_table *fns);

int voicecall_plugin_unregister(void);

