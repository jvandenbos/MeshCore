// MERLIN sim — replay an input script against the real UI and dump PNGs.
//
//   ./merlin-sim scripts/nodes_sorts.txt out/
//
// Script grammar (one verb per line, '#' comments):
//   start splash|home|chats|nodes|settings
//   nav u|d|l|r|click
//   key <char>|enter|bksp|space
//   text <string>            each character as a KEY event
//   touch <x>,<y>            tap
//   long <x>,<y>             long press
//   swipe l|r
//   tick <n>                 advance the clock n seconds (banner expiry, and
//                            one step of every pending ACK)
//   rx <convo>|<sender>|<text>   incoming traffic, straight into the store
//   banner <who>|<text>|<convo>
//   snap <name>              write <outdir>/<name>.png
//   expect <screen>          assert the current screen, non-zero exit if not

#include "fixtures.h"
#include "sim_gfx.h"

#include "../examples/companion_radio/ui-merlin/router.h"
#include "../examples/companion_radio/ui-merlin/theme.h"
#include "../examples/companion_radio/ui-merlin/screens/channels.h"
#include "../examples/companion_radio/ui-merlin/screens/chats.h"
#include "../examples/companion_radio/ui-merlin/screens/home.h"
#include "../examples/companion_radio/ui-merlin/screens/newchat.h"
#include "../examples/companion_radio/ui-merlin/screens/node_card.h"
#include "../examples/companion_radio/ui-merlin/screens/nodes.h"
#include "../examples/companion_radio/ui-merlin/screens/settings.h"
#include "../examples/companion_radio/ui-merlin/screens/splash.h"
#include "../examples/companion_radio/ui-merlin/screens/thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace merlin;

// One name per ScreenId, in ScreenId order — a gap here is a null the script's
// `expect` verb walks straight into.
static const char *kScreenName[SC_COUNT] = {
    "splash",   "home",  "chats",    "thread",   "nodes",   "node_card",
    "settings", "radio", "newchat",  "channels", "channel", "identity"};

static int screenByName(const std::string &s) {
  for (int i = 0; i < SC_COUNT; ++i)
    if (s == kScreenName[i]) return i;
  return -1;
}

static std::string trim(const std::string &s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <script.txt> <outdir>\n", argv[0]);
    return 2;
  }
  const char *script_path = argv[1];
  std::string outdir = argv[2];
  if (!outdir.empty() && outdir.back() != '/') outdir += '/';

  FILE *fp = fopen(script_path, "r");
  if (!fp) {
    fprintf(stderr, "cannot open script %s\n", script_path);
    return 2;
  }

  FixtureModel model;
  FixtureSink sink;
  sink.model = &model;
  SimGfx gfx(theme::SCREEN_W, theme::SCREEN_H);

  screens::SplashScreen splash;
  screens::HomeScreen home;
  screens::ChatsScreen chats;
  screens::ThreadScreen thread;
  screens::NodesScreen nodes;
  screens::NodeCardScreen card;
  screens::SettingsScreen settings;
  screens::RadioScreen radio;
  screens::IdentityScreen identity;
  screens::NewChatScreen newchat;
  screens::ChannelsScreen channels;
  screens::ChannelScreen channel;

  Router router(model, &sink);
  router.registerScreen(SC_SPLASH, &splash);
  router.registerScreen(SC_HOME, &home);
  router.registerScreen(SC_CHATS, &chats);
  router.registerScreen(SC_THREAD, &thread);
  router.registerScreen(SC_NODES, &nodes);
  router.registerScreen(SC_NODE_CARD, &card);
  router.registerScreen(SC_SETTINGS, &settings);
  router.registerScreen(SC_RADIO, &radio);
  router.registerScreen(SC_IDENTITY, &identity);
  router.registerScreen(SC_NEWCHAT, &newchat);
  router.registerScreen(SC_CHANNELS, &channels);
  router.registerScreen(SC_CHANNEL, &channel);

  uint32_t now = 21 * 3600 + 42 * 60;
  router.tick(now);
  router.start(SC_CHATS);

  int failures = 0, snaps = 0, line_no = 0;
  char line[512];
  printf("== %s\n", script_path);

  while (fgets(line, sizeof(line), fp)) {
    ++line_no;
    std::string s = trim(line);
    if (s.empty() || s[0] == '#') continue;

    std::string verb = s.substr(0, s.find(' '));
    std::string rest = s.size() > verb.size() ? trim(s.substr(verb.size())) : "";

    if (verb == "start") {
      int id = screenByName(rest);
      if (id < 0) { fprintf(stderr, "line %d: unknown screen %s\n", line_no, rest.c_str()); ++failures; continue; }
      router.start((ScreenId)id);
    } else if (verb == "nav") {
      InputEvent::Nav n = InputEvent::U;
      if (rest == "u") n = InputEvent::U;
      else if (rest == "d") n = InputEvent::D;
      else if (rest == "l") n = InputEvent::L;
      else if (rest == "r") n = InputEvent::R;
      else if (rest == "click") n = InputEvent::CLICK;
      else { fprintf(stderr, "line %d: bad nav %s\n", line_no, rest.c_str()); ++failures; continue; }
      router.handle(InputEvent::navi(n));
    } else if (verb == "key") {
      char c;
      if (rest == "enter") c = '\n';
      else if (rest == "bksp") c = '\b';
      else if (rest == "space") c = ' ';
      else if (rest.size() == 1) c = rest[0];
      else { fprintf(stderr, "line %d: bad key %s\n", line_no, rest.c_str()); ++failures; continue; }
      router.handle(InputEvent::key(c));
    } else if (verb == "text") {
      for (char c : rest) router.handle(InputEvent::key(c));
    } else if (verb == "touch" || verb == "long") {
      int x = 0, y = 0;
      if (sscanf(rest.c_str(), "%d,%d", &x, &y) != 2) { fprintf(stderr, "line %d: bad point\n", line_no); ++failures; continue; }
      router.handle(InputEvent::touch(
          verb == "long" ? InputEvent::LONG : InputEvent::TAP, (int16_t)x,
          (int16_t)y));
    } else if (verb == "swipe") {
      router.handle(InputEvent::touch(
          rest == "l" ? InputEvent::SWIPE_L : InputEvent::SWIPE_R, 160, 120));
    } else if (verb == "tick") {
      int n = atoi(rest.c_str());
      now += (uint32_t)(n > 0 ? n : 1);
      model.setClock(now);
      model.advanceAcks();
      router.tick(now);
    } else if (verb == "rx") {
      size_t p1 = rest.find('|'), p2 = rest.find('|', p1 + 1);
      if (p1 == std::string::npos || p2 == std::string::npos) {
        fprintf(stderr, "line %d: rx needs convo|sender|text\n", line_no);
        ++failures;
        continue;
      }
      int c = model.inject(rest.substr(0, p1).c_str(),
                           rest.substr(p1 + 1, p2 - p1 - 1).c_str(),
                           rest.substr(p2 + 1).c_str());
      if (c < 0) {
        fprintf(stderr, "line %d: rx could not store the message\n", line_no);
        ++failures;
      }
      router.markDirty();
    } else if (verb == "banner") {
      size_t p1 = rest.find('|'), p2 = rest.rfind('|');
      if (p1 == std::string::npos || p1 == p2) { fprintf(stderr, "line %d: banner needs who|text|convo\n", line_no); ++failures; continue; }
      static char who[32], txt[96];
      snprintf(who, sizeof(who), "%s", rest.substr(0, p1).c_str());
      snprintf(txt, sizeof(txt), "%s", rest.substr(p1 + 1, p2 - p1 - 1).c_str());
      router.showBanner(who, txt, atoi(rest.substr(p2 + 1).c_str()));
    } else if (verb == "snap") {
      router.render(gfx);
      std::string path = outdir + rest + ".png";
      if (!gfx.writePng(path.c_str())) {
        fprintf(stderr, "line %d: PNG write failed: %s\n", line_no, path.c_str());
        ++failures;
      } else {
        printf("   snap %-28s %s\n", rest.c_str(), path.c_str());
        ++snaps;
      }
    } else if (verb == "expect") {
      const char *cur = kScreenName[router.current()];
      if (rest != cur) {
        fprintf(stderr, "   FAIL line %d: expected screen '%s', on '%s'\n",
                line_no, rest.c_str(), cur);
        ++failures;
      }
    } else {
      fprintf(stderr, "line %d: unknown verb '%s'\n", line_no, verb.c_str());
      ++failures;
    }
  }
  fclose(fp);
  model.persist(); // exercises the StoreBackend seam the device port will use
  printf("   %d snapshot(s), %d failure(s)\n", snaps, failures);
  return failures ? 1 : 0;
}
