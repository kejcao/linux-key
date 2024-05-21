#include <vector>
#include <fstream>
#include <map>
#include <filesystem>
#include <future>
#include <mutex>
#include <print>
#include <regex>

#include <libudev.h>
#include "/usr/include/linux/input.h"

using namespace std;
namespace fs = std::filesystem;

ofstream logfile;
mutex writer;

void listen(fs::path path) {
  ifstream ifs(path, ios::binary);

  struct input_event ev;
  while (ifs.read((char *)&ev, sizeof(struct input_event))) {
    uint64_t timestamp = ev.time.tv_sec * 1e6 + ev.time.tv_usec;

    // Key release or press?
    if ((ev.value == 0 || ev.value == 1) && ev.type == 1) {
      lock_guard<mutex> lock(writer);
      println("{} {} {}", timestamp, ev.value, ev.code);
    }
  }
}

// From ChatGPT, will clean this TODO
vector<fs::path> get_keyboard_devices() {
  vector<fs::path> devices;

  for (const auto &d : fs::directory_iterator("/sys/class/input/")) {
    if (d.path().filename().string().starts_with("input")) {
      ifstream ifs(d.path() / "capabilities/key");
      string line;
      getline(ifs, line);

      regex hex_regex("([0-9a-fA-F]+)$");
      smatch match;
      if (regex_search(line, match, hex_regex)) {
        uint64_t keyCapability = stoull(match.str(1), nullptr, 16);
        if ((keyCapability & 0x7f07fc3ff0000) == 0x7f07fc3ff0000) {
          for (const auto &event : fs::directory_iterator(d.path())) {
            if (event.path().filename().string().find("event") == 0) {
              devices.push_back(fs::path("/dev/input") /
                                event.path().filename());
              break;
            }
          }
        }
      }
    }
  }

  return devices;
}

map<fs::path, future<void>> futures;
void refresh() {
  erase_if(futures, [](auto &kv) {
    const auto &[p, future] = kv;
    using namespace chrono_literals;
    if (future.wait_for(0ms) == future_status::ready) {
      println("{} disconnecting {}...", futures.size() - 1, p.string());
      return true;
    }
    return false;
  });

  for (auto p : get_keyboard_devices()) {
    if (!futures.contains(p)) {
      println("{} connecting {}...", futures.size() + 1, p.string());
      futures[p] = async(launch::async, listen, p);
    }
  }
}

int main() {
  logfile = ofstream("/home/kjc/stats/key.data", ios::binary | ios::app);

  auto *udev = udev_new();
  auto *mon = udev_monitor_new_from_netlink(udev, "udev");

  udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL);
  udev_monitor_enable_receiving(mon);

  int fd = udev_monitor_get_fd(mon);
  while (true) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    int ret = select(fd + 1, &fds, NULL, NULL, NULL);
    if (ret > 0 && FD_ISSET(fd, &fds)) {
      if (auto *dev = udev_monitor_receive_device(mon)) {
        // Lazy: for each new device this condition is triggered about a dozen
        // times.
        refresh();
        udev_device_unref(dev);
      }
    }
  }
}
