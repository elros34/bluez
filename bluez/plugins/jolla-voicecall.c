/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2016 Jolla Ltd. All rights reserved.
 *  Contact: Marin Jones <martin.jones@jollamobile.com>
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

/* Plugin that implements voicecall dialing via an agent.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <dlfcn.h>

#include <dbus/dbus.h>
#include <gdbus.h>

#include "plugin.h"
#include "voicecall.h"
#include "log.h"
#include "error.h"

#define VOICECALL_INTERFACE "org.bluez.VoiceCall"
#define VOICECALL_AGENT_INTERFACE "org.bluez.VoiceCallAgent"

struct voicecall_interface {
    struct voicecall_agent *agent;
};

struct voicecall_agent {
    char *interface;
    char *path;
    char *service;
    guint disconnect_watch;
};

static DBusConnection *connection = NULL;
static gboolean initialized = FALSE;
static struct voicecall_interface *voicecall_iface = NULL;

void voicecall_agent_send_release(struct voicecall_agent *agent)
{
    DBusMessage *message = dbus_message_new_method_call(
            agent->service, agent->path,
            VOICECALL_AGENT_INTERFACE, "Release");

    if (message == NULL)
        return;

    dbus_message_set_no_reply(message, TRUE);
    g_dbus_send_message(connection, message);
}

void voicecall_agent_free(struct voicecall_agent *agent)
{
    if (agent == NULL)
        return;

    if (agent->disconnect_watch) {
        voicecall_agent_send_release(agent);
        g_dbus_remove_watch(connection, agent->disconnect_watch);
        agent->disconnect_watch = 0;
    }

    g_free(agent->path);
    g_free(agent->service);
    g_free(agent->interface);
    g_free(agent);
}

static void voicecall_agent_disconnect_cb(DBusConnection *conn, void *data)
{
    DBG("Voicecall agent disconnected");

    struct voicecall_agent *agent = data;
    agent->disconnect_watch = 0;
    voicecall_agent_free(agent);

    voicecall_iface->agent = NULL;
}

struct voicecall_agent *voicecall_agent_new(const char *interface,
                const char *service, const char *path)
{
    struct voicecall_agent *agent = g_new0(struct voicecall_agent, 1);

    if (agent == NULL)
        return NULL;

    agent->interface = g_strdup(interface);
    agent->service = g_strdup(service);
    agent->path = g_strdup(path);

    agent->disconnect_watch = g_dbus_add_disconnect_watch(connection, service,
                            voicecall_agent_disconnect_cb,
                            agent, NULL);

    return agent;
}

static DBusMessage *call_request_register_agent(DBusConnection *conn, DBusMessage *msg, void *data)
{
    struct voicecall_interface *cr = data;
    const char *agent_path;

    DBG("Register voicecall agent");

    if (cr->agent) {
        return btd_error_busy(msg);
    }

    if (dbus_message_get_args(msg, NULL,
                              DBUS_TYPE_OBJECT_PATH, &agent_path,
                              DBUS_TYPE_INVALID) == FALSE)
        return btd_error_invalid_args(msg);

    cr->agent = voicecall_agent_new(VOICECALL_AGENT_INTERFACE,
                              dbus_message_get_sender(msg),
                              agent_path);

    if (cr->agent == NULL)
        return btd_error_failed(msg, "Cannot create agent");

    voicecall_iface = cr;

    return dbus_message_new_method_return(msg);
}

static DBusMessage *call_request_unregister_agent(DBusConnection *conn, DBusMessage *msg, void *data)
{
    DBG("Unregister voicecall agent");

    struct voicecall_interface *cr = data;
    const char *agent_path;
    const char *agent_bus = dbus_message_get_sender(msg);

    if (dbus_message_get_args(msg, NULL,
                              DBUS_TYPE_OBJECT_PATH, &agent_path,
                              DBUS_TYPE_INVALID) == FALSE)
        return btd_error_invalid_args(msg);

    if (cr->agent == NULL)
        return btd_error_failed(msg, "No agent registered");

    if (strcmp(cr->agent->path, agent_path))
        return btd_error_failed(msg, "Attempt to unregister unconnected agent");

    voicecall_agent_free(cr->agent);
    cr->agent = NULL;

    return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable call_request_methods[] = {
    { GDBUS_METHOD("RegisterAgent", GDBUS_ARGS({ "path", "o" }), NULL, call_request_register_agent) },
    { GDBUS_METHOD("UnregisterAgent", GDBUS_ARGS({ "path", "o" }), NULL, call_request_unregister_agent) },
    { }
};

static int register_interface(const char *path)
{
    DBG("REGISTER INTERFACE path %s", path);

    voicecall_iface = g_try_new0(struct voicecall_interface, 1);
    if (!voicecall_iface) {
        error("Failed to alloc memory for voicecall agent");
        return -ENOMEM;
    }

    voicecall_iface->agent = NULL;

    if (g_dbus_register_interface(connection, path, VOICECALL_INTERFACE,
                call_request_methods, NULL, NULL, voicecall_iface,
                        NULL) == FALSE) {
        error("D-Bus failed to register %s interface",
                            VOICECALL_INTERFACE);
        g_free(voicecall_iface);
        voicecall_iface = NULL;
        return -EIO;
    }

    DBG("Registered interface %s on path %s", VOICECALL_INTERFACE, path);

    return 0;
}

static void unregister_interface(const char *path)
{
    DBG("path %s", path);

    g_dbus_unregister_interface(connection, path, VOICECALL_INTERFACE);
    g_free(voicecall_iface);
    voicecall_iface = NULL;
}

static int jolla_voicecall_dial(const gchar *number)
{
    if (voicecall_iface == NULL || voicecall_iface->agent == NULL)
        return -1;

    DBG("Dial number: %s %p", number, voicecall_iface->agent);

    DBusMessageIter iter;
    DBusMessage *msg = dbus_message_new_method_call(voicecall_iface->agent->service, voicecall_iface->agent->path,
                                                    voicecall_iface->agent->interface, "Dial");

    if (msg == NULL)
        return -ENOMEM;

    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &number);

    return g_dbus_send_message(connection, msg) ? 0 : -1;
}

static void jolla_voicecall_exit(void)
{
    DBG("");

    if (!initialized)
        return;

    voicecall_plugin_unregister();
    unregister_interface("/");
    dbus_connection_unref(connection);
    connection = NULL;
    initialized = FALSE;
    DBG("Jolla voicecall uninitialized");
}

static int jolla_voicecall_init(void)
{
    static struct voicecall_table table = {
        .dial = jolla_voicecall_dial
    };
    gboolean enable = FALSE;
    GKeyFile *config;
    int r;

    DBG("");

    if (initialized)
        return -EALREADY;

    r = voicecall_plugin_register("jolla-voicecall", &table);
    if (r < 0) {
        error("Failed to register voicecall plugin");
        return r;
    }

    connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);

    register_interface("/");

    initialized = TRUE;

    DBG("Jolla voicecall initialized");
    return 0;
}

BLUETOOTH_PLUGIN_DEFINE(jolla_voicecall, VERSION,
                        BLUETOOTH_PLUGIN_PRIORITY_DEFAULT,
                        jolla_voicecall_init, jolla_voicecall_exit)
