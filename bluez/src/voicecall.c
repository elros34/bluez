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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <string.h>
#include <glib.h>

#include "voicecall.h"

static gchar *impl = NULL;
static struct voicecall_table table;

int voicecall_dial(const gchar *number)
{
    return impl ? (table.dial)(number) : -ENOENT;
}

int voicecall_plugin_register(const gchar *name, const struct voicecall_table *fns)
{
    if (impl)
        return -EALREADY;

    impl = g_strdup(name);
    memcpy(&table, fns, sizeof(struct voicecall_table));
    return 0;
}

int voicecall_plugin_unregister(void)
{
    if (!impl)
        return -ENOENT;

    memset(&table, 0, sizeof(struct voicecall_table));
    g_free(impl);
    impl = NULL;

    return 0;
}
