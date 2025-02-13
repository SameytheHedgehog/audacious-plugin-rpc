#include <iostream>
#include <string.h>

#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/hook.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/tuple.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "discord_rpc.h"

#define EXPORT __attribute__((visibility("default")))
#define APPLICATION_ID "484736379171897344"

static const char *SETTING_EXTRA_TEXT = "extra_text";

class RPCPlugin : public GeneralPlugin {

public:
    static const char about[];
    static const PreferencesWidget widgets[];
    static const PluginPreferences prefs;

    static constexpr PluginInfo info = {
        N_("Discord RPC"),
        "audacious-plugin-rpc",
        about,
        &prefs
    };

    constexpr RPCPlugin() : GeneralPlugin (info, false) {}

    bool init();
    void cleanup();
};

EXPORT RPCPlugin aud_plugin_instance;

DiscordEventHandlers handlers;
DiscordRichPresence presence;
std::string fullTitle;
std::string playingStatus;

void init_discord() {
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}

void update_presence() {
    Discord_UpdatePresence(&presence);
}

void init_presence() {
    memset(&presence, 0, sizeof(presence));
    presence.state = "Initialized";
    presence.details = "Waiting...";
    presence.largeImageKey = "logo";
    presence.largeImageText = "audacious-plugin-rpc v1.2.1 (Samey's Edit)";
    presence.smallImageKey = "stop";
    update_presence();
}

void cleanup_discord() {
    Discord_ClearPresence();
    Discord_Shutdown();
}

int numDigits(int number)
{
    int digits = 0;
    while (number) {
        number /= 10;
        digits++;
    }
    return digits;
}

void title_changed() {
    if (!aud_drct_get_ready()) {
        return;
    }

    if (aud_drct_get_playing()) {
        bool paused = aud_drct_get_paused();
        Tuple tuple = aud_drct_get_tuple();
        String album = tuple.get_str(Tuple::Album);
        std::string title(tuple.get_str(Tuple::Title));
        std::string format(tuple.get_str(Tuple::Codec));
        int length(tuple.get_int(Tuple::Length));
	    int min = length / 60000;
	    length = length - 60000 * min;
	    int sec = length / 1000;
	    length = length - 1000 * sec;
	    int digits = numDigits(sec);
	    std::string preSec = "0" + std::to_string(sec);
	    std::string secString(digits > 1 ? std::to_string(sec) : preSec);

        if (album) {
            fullTitle = (title + " - " + std::string(album)).substr(0, 127);
        } else {
            fullTitle = title.substr(0, 127);
        }

        playingStatus = "Length: " + std::to_string(min) + ":" + secString + ", Type: " + format;
        presence.details = fullTitle.c_str();
        presence.smallImageKey = paused ? "pause" : "play";
        presence.smallImageText = paused ? "Paused" : "Playing";
    } else {
        playingStatus = "Stopped";
        presence.state = "Stopped";
        presence.smallImageKey = "stop";
    }

    std::string extraText(aud_get_str("audacious-plugin-rpc", SETTING_EXTRA_TEXT));
    playingStatus = (playingStatus + " " + extraText).substr(0, 127);

    presence.state = playingStatus.c_str();
    update_presence();
}

void update_title_presence(void*, void*) {
    title_changed();
}

void open_github() {
   system("xdg-open https://github.com/SameytheHedgehog/audacious-plugin-rpc");
}

bool RPCPlugin::init() {
    init_discord();
    init_presence();
    hook_associate("playback ready", update_title_presence, nullptr);
    hook_associate("playback end", update_title_presence, nullptr);
    hook_associate("playback stop", update_title_presence, nullptr);
    hook_associate("playback pause", update_title_presence, nullptr);
    hook_associate("playback unpause", update_title_presence, nullptr);
    hook_associate("title change", update_title_presence, nullptr);
    return true;
}

void RPCPlugin::cleanup() {
    hook_dissociate("playback ready", update_title_presence);
    hook_dissociate("playback end", update_title_presence);
    hook_dissociate("playback stop", update_title_presence);
    hook_dissociate("playback pause", update_title_presence);
    hook_dissociate("playback unpause", update_title_presence);
    hook_dissociate("title change", update_title_presence);
    cleanup_discord();
}

const char RPCPlugin::about[] = N_("Discord RPC music status plugin\n\nWritten by Derzsi Daniel <daniel@tohka.us>\nForked by Essem <essem@essem.space>\nEditted by SameytheHedgie");

const PreferencesWidget RPCPlugin::widgets[] =
{
  WidgetEntry(
      N_("Extra status text:"),
      WidgetString("audacious-plugin-rpc", SETTING_EXTRA_TEXT, title_changed)
  ),
  WidgetButton(
      N_("Fork on GitHub"),
      {open_github}
  )
};

const PluginPreferences RPCPlugin::prefs = {{ widgets }};
