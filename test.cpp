#include <stdio.h>
#include <glib.h>
#include <NetworkManager.h>

#include <memory>

typedef struct {
    GMainLoop *         loop;
    GError *            error;
    NMRemoteConnection *rconn;
} RequestData;

static void
add_cb(GObject *source, GAsyncResult *result, gpointer user_data)
{
    RequestData *rdata = (RequestData *) user_data;

    rdata->rconn = nm_client_add_connection_finish(NM_CLIENT(source), result, &rdata->error);
    g_main_loop_quit(rdata->loop);
}

int main(void) {
    std::shared_ptr<GSList> plugins
    {
        nm_vpn_plugin_info_list_load(),
        [](auto list)
        {
            g_slist_free_full(list,
                              g_object_unref);
        }
    };

    auto plugin = nm_vpn_plugin_info_list_find_by_name(
        plugins.get(),
        "openvpn");

    GError* error = nullptr;

    // editor is [[transfer:none]] from libnm so will be freed with  %plugins
    auto editor = nm_vpn_plugin_info_load_editor_plugin(plugin, &error);
    if (error)
    {
        std::string msg{error->message};
        g_clear_error(&error);
        throw std::runtime_error(msg);
    }

    // Wrap the connection with shared_ptr to ensure clearing on dereference
    std::shared_ptr<NMConnection> conn
    {
        nm_vpn_editor_plugin_import(editor,
                                    "sample.ovpn",
                                    &error),
        [](auto connection)
        {
            g_clear_object(&connection);
        }
    };
    if (error)
    {
        std::string msg{error->message};
        g_clear_error(&error);
        throw std::runtime_error(msg);
    }

    if (!nm_connection_normalize(conn.get(), NULL, NULL, &error))
    {
        std::string msg{error->message};
        g_clear_error(&error);
        throw std::runtime_error(msg);
    }

    std::shared_ptr<NMClient> cli
    {
        nm_client_new(nullptr, &error),
        [](auto client)
        {
            g_clear_object(&client);
        }
    };
    if (!cli)
    {
        std::string msg{error->message};
        throw std::runtime_error(msg);
    }

    RequestData rdata;
    rdata.loop  = g_main_loop_new(nullptr, false);
    rdata.rconn = nullptr;
    rdata.error = nullptr;

    nm_client_add_connection_async(cli.get(), conn.get(), true, nullptr, add_cb, &rdata);
    g_main_loop_run(rdata.loop);
    g_clear_pointer(&rdata.loop, g_main_loop_unref);
    g_clear_object(&rdata.rconn);

    if (rdata.error) {
        std::string msg{rdata.error->message};
        g_clear_error(&rdata.error);
        throw std::runtime_error(msg);
    }

    return 0;
}
