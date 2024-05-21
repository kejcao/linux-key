#include "/usr/include/linux/input.h"
#include <bits/stdc++.h>
#include <libudev.h>
#include <thread>

using namespace std;
namespace fs = std::filesystem;

// https://stackoverflow.com/questions/16695432/input-event-structure-description-from-linux-input-h

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

  // // vector<double> v = {
  // //     0.007859230041503906,  0.0076792240142822266, 0.008411884307861328,
  // //     0.00831747055053711,   0.008234977722167969,  0.008572578430175781,
  // //     0.009616851806640625,  0.009622573852539063,  0.00926518440246582,
  // //     0.009131669998168945,  0.008625507354736328,  0.008272171020507813,
  // //     0.008666276931762695,  0.009252548217773438,  0.009163856506347656,
  // //     0.008917093276977539,  0.009442806243896484,  0.009443044662475586,
  // //     0.009201765060424805,  0.009229660034179688,  0.008151531219482422,
  // //     0.008004188537597656,  0.007958173751831055,  0.0069658756256103516,
  // //     0.0044634342193603516, 0.004178524017333984,  0.00484156608581543,
  // //     0.003814220428466797,
  // // };
  // //
  // // for (auto &&e : v | views::slide(2)) {
  // //   uint64_t x = std::bit_cast<uint64_t>(e[0]);
  // //   uint64_t y = std::bit_cast<uint64_t>(e[1]);
  // //   std::println("{:0>64b}", x ^ y);
  // // }
  //
  // // vector<uint64_t> v = {
  // //     1716240550758646, 1716240550858651, 1716240550904640,
  // 1716240550975632,
  // //     1716240551264644, 1716240551337685, 1716240551353652,
  // 1716240551430713,
  // //     1716240551439646, 1716240551495679, 1716240551534682,
  // 1716240551586786,
  // //     1716240551586786, 1716240551654674, 1716240551706657,
  // 1716240551726646,
  // //     1716240552167817, 1716240552222658, 1716240552250660,
  // 1716240552289660,
  // //     1716240552314660, 1716240552370659, 1716240552370659,
  // 1716240552441648,
  // //     1716240552471639, 1716240552551635, 1716240558920638,
  // 1716240558974614,
  // //     1716240558998640, 1716240559038626, 1716240559114640,
  // 1716240559198619,
  // //     1716240559215624, 1716240559246614, 1716240559271624,
  // 1716240559335622,
  // //     1716240559345614, 1716240559430627, 1716240559430627,
  // 1716240559494637,
  // //     1716240559512638, 1716240559538617, 1716240559561619,
  // 1716240559584621,
  // //     1716240559649610, 1716240559649610, 1716240559684641,
  // 1716240559713629,
  // //     1716240559778641, 1716240559798638, 1716240559862642,
  // 1716240559926617,
  // //     1716240559959617, 1716240559974616, 1716240560032639,
  // 1716240560046635,
  // //     1716240560081615, 1716240560097618, 1716240560128635,
  // 1716240560137622,
  // //     1716240560183618, 1716240560192636, 1716240560238628,
  // 1716240560247627,
  // //     1716240560297638, 1716240560356612, 1716240560398640,
  // 1716240560429637,
  // //     1716240560505624, 1716240560505624, 1716240560590618,
  // 1716240560631645,
  // //     1716240560646627, 1716240560718636, 1716240560768633,
  // 1716240560782634,
  // //     1716240560833614, 1716240560846636, 1716240560891631,
  // 1716240560902619,
  // //     1716240560962637, 1716240561006601, 1716240561058609,
  // 1716240561078614,
  // //     1716240561095611, 1716240561159613, 1716240561205626,
  // 1716240561224608,
  // //     1716240561262613, 1716240561286635, 1716240561382638,
  // 1716240561438635,
  // //     1716240561454616, 1716240561510609, 1716240561510609,
  // 1716240561561611,
  // //     1716240561620675, 1716240561630614, 1716240561630614,
  // 1716240561692613,
  // //     1716240561703633, 1716240561734637, 1716240561750609,
  // 1716240561806600,
  // //     1716240563241632, 1716240563288630, 1716240563361600,
  // 1716240563390610,
  // //     1716240563934638, 1716240563970608,
  // // };
  // //
  // // // vector<uint64_t> d = {};
  // // for (auto &&e : v | views::slide(2)) {
  // //   // d.push_back(e[1]-e[0]);
  // //   // std::println("{:0>64b}", e[1]-e[0]);
  // //   std::println("{:b}", e[1]-e[0]);
  // //   // std::println("{}", e[1]-e[0]);
  // // }
}
