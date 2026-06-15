#define _CRT_SECURE_NO_WARNINGS 1
#define __IDP__                 1
#define __NT__                  1
#define __X64__                 1

#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <funcs.hpp>
#include <name.hpp>
#include <nalt.hpp>
#include <bytes.hpp>
#include <segment.hpp>
#include <auto.hpp>
#include <diskio.hpp>
#include <fpro.h>

#include "discord-rpc.h"

#include <ctime>
#include <cstring>

static const char* APP_ID      = "1408193592039706694";
static const char* PLUGIN_NAME = "IDA Pro RPC";
static int64_t     g_start_time = 0;
static bool        g_rpc_running = false;

static bool g_show_filename    = true;
static bool g_show_function    = true;
static bool g_show_elapsed     = true;
static bool g_show_ida_version = true;

static int64_t     g_last_activity = 0;
static bool        g_was_idle      = false;
static qtimer_t    g_idle_timer    = NULL;

static const char* get_current_filename()
{
    static char filename[MAXSTR];
    ssize_t n = get_root_filename(filename, sizeof(filename));
    if (n > 0) return filename;
    return "no file";
}

static const char* get_current_function_name()
{
    ea_t ea = get_screen_ea();
    func_t* fn = get_func(ea);
    if (fn != NULL)
    {
        static qstring func_name;
        if (get_func_name(&func_name, fn->start_ea) > 0)
            return func_name.c_str();
    }
    return NULL;
}

static void handle_ready(void)
{
    msg("[%s] discord connected!\n", PLUGIN_NAME);
}

static void handle_disconnected(int errcode, const char* message)
{
    msg("[%s] discord disconnected (%d: %s)\n", PLUGIN_NAME, errcode, message);
}

static void handle_error(int errcode, const char* message)
{
    msg("[%s] discord error (%d: %s)\n", PLUGIN_NAME, errcode, message);
}

static void update_presence();

static void discord_init()
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready        = handle_ready;
    handlers.disconnected = handle_disconnected;
    handlers.errored      = handle_error;

    Discord_Initialize(APP_ID, &handlers, 1, NULL);
    g_rpc_running = true;
    g_start_time  = (int64_t)time(0);
    g_last_activity = g_start_time;
    g_was_idle = false;
    msg("[%s] discord rpc initialized with app id: %s\n", PLUGIN_NAME, APP_ID);
}

static void discord_shutdown()
{
    if (g_rpc_running)
    {
        Discord_ClearPresence();
        Discord_Shutdown();
        g_rpc_running = false;
        msg("[%s] discord rpc shutdown\n", PLUGIN_NAME);
    }
}

static void update_presence()
{
    if (!g_rpc_running) return;

    DiscordRichPresence rpc;
    memset(&rpc, 0, sizeof(rpc));

    if (g_was_idle)
    {
        rpc.details = "No activity for 5 minutes";
        rpc.state = "Idle (AFK)";
    }
    else
    {
        static char details[MAXSTR];
        if (g_show_filename)
        {
            qsnprintf(details, sizeof(details) - 1, "%s", get_current_filename());
            rpc.details = details;
        }

        static char state[MAXSTR];
        const char* func_name = get_current_function_name();

        if (func_name != NULL && g_show_function)
        {
            qsnprintf(state, sizeof(state) - 1, "Reversing %s", func_name);
            rpc.state = state;
        }
        else
        {
            rpc.state = "Idle";
        }

        if (g_show_elapsed)
            rpc.startTimestamp = g_start_time;
    }

    rpc.largeImageKey  = "ida_logo";
    if (g_show_ida_version)
        rpc.largeImageText = "IDA Pro 9.4";
    else
        rpc.largeImageText = "IDA Pro";

    Discord_UpdatePresence(&rpc);
}

static int idaapi timer_callback(void*)
{
    if (g_rpc_running)
    {
        bool is_idle = ((int64_t)time(0) - g_last_activity) >= 300; // 300 seconds = 5 mins
        if (is_idle != g_was_idle)
        {
            g_was_idle = is_idle;
            update_presence();
        }
    }
    return 1000;
}

static void wake_up()
{
    g_last_activity = (int64_t)time(0);
    if (g_was_idle)
    {
        g_was_idle = false;
        update_presence();
    }
}

static ssize_t idaapi ui_callback(void*, int code, va_list)
{
    wake_up();
    switch (code)
    {
    case ui_ready_to_run:
    case ui_updated_actions:
        update_presence();
        break;
    }
    return 0;
}

static ssize_t idaapi idp_callback(void*, int code, va_list)
{
    wake_up();
    switch (code)
    {
    case processor_t::ev_newfile:
    case processor_t::ev_oldfile:
        update_presence();
        break;
    }
    return 0;
}

static ssize_t idaapi idb_callback(void*, int code, va_list)
{
    wake_up();
    switch (code)
    {
    case idb_event::savebase:
    case idb_event::func_updated:
    case idb_event::func_added:
    case idb_event::renamed:
        update_presence();
        break;
    }
    return 0;
}

static ssize_t idaapi view_callback(void*, int code, va_list)
{
    wake_up();
    switch (code)
    {
    case view_loc_changed:
    case view_curpos:
        update_presence();
        break;
    }
    return 0;
}

static void show_options_dialog()
{
    ushort flags = 0;
    if (g_show_filename)    flags |= (1 << 0);
    if (g_show_function)    flags |= (1 << 1);
    if (g_show_elapsed)     flags |= (1 << 2);
    if (g_show_ida_version) flags |= (1 << 3);
    if (g_rpc_running)      flags |= (1 << 4);

    int result = ask_form(
        "ida pro rpc - settings\n\n"
        "<show filename:C>\n"
        "<show function name:C>\n"
        "<show elapsed time:C>\n"
        "<show ida version:C>\n"
        "<rpc enabled:C>>\n\n",
        &flags
    );

    if (result == 1)
    {
        g_show_filename    = (flags & (1 << 0)) != 0;
        g_show_function    = (flags & (1 << 1)) != 0;
        g_show_elapsed     = (flags & (1 << 2)) != 0;
        g_show_ida_version = (flags & (1 << 3)) != 0;
        bool rpc_on        = (flags & (1 << 4)) != 0;

        if (rpc_on && !g_rpc_running)
        {
            discord_init();
        }
        else if (!rpc_on && g_rpc_running)
        {
            discord_shutdown();
        }

        update_presence();
        msg("[%s] settings saved\n", PLUGIN_NAME);
    }
}

static plugmod_t* idaapi plugin_init(void)
{
    msg("[%s] loading...\n", PLUGIN_NAME);

    discord_init();

    hook_to_notification_point(HT_UI,   (hook_cb_t*)ui_callback);
    hook_to_notification_point(HT_IDP,  (hook_cb_t*)idp_callback);
    hook_to_notification_point(HT_IDB,  (hook_cb_t*)idb_callback);
    hook_to_notification_point(HT_VIEW, (hook_cb_t*)view_callback);

    g_idle_timer = register_timer(1000, timer_callback, NULL);

    update_presence();

    msg("[%s] loaded successfully!\n", PLUGIN_NAME);
    return PLUGIN_KEEP;
}

static void idaapi plugin_term(void)
{
    if (g_idle_timer != NULL)
    {
        unregister_timer(g_idle_timer);
        g_idle_timer = NULL;
    }

    unhook_from_notification_point(HT_UI,   (hook_cb_t*)ui_callback);
    unhook_from_notification_point(HT_IDP,  (hook_cb_t*)idp_callback);
    unhook_from_notification_point(HT_IDB,  (hook_cb_t*)idb_callback);
    unhook_from_notification_point(HT_VIEW, (hook_cb_t*)view_callback);

    discord_shutdown();
    msg("[%s] unloaded\n", PLUGIN_NAME);
}

static bool idaapi plugin_run(size_t)
{
    show_options_dialog();
    return true;
}

plugin_t PLUGIN =
{
    IDP_INTERFACE_VERSION,
    PLUGIN_FIX,
    plugin_init,
    plugin_term,
    plugin_run,
    "discord rich presence for ida pro 9.4",
    "discord rpc plugin",
    "IDA Pro RPC",
    "Ctrl-Alt-D"
};
