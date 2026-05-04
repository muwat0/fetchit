#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cctype>
#include <sys/utsname.h>

using std::string, std::cout;
namespace fs = std::filesystem;

string getUser();
string getHost();
string getDistro();
string getKernel();
string getUptime();
string getShell();
string getCPU();
string getGPU();
string getRAM();

struct gpuId {
    string vendor;
    string device;
};

std::vector<gpuId> getGpuIds() {
    std::vector<gpuId> gpus;
    std::string pciPath = "/sys/bus/pci/devices/";

    for (const auto& entry : fs::directory_iterator(pciPath)) {
        std::ifstream classFile(entry.path().string() + "/class");
        std::string classId;
        classFile >> classId;

        if (classId.compare(0, 4, "0x03") == 0) {
            std::ifstream vFile(entry.path().string() + "/vendor");
            std::ifstream dFile(entry.path().string() + "/device");
            std::string v, d;
            vFile >> v; dFile >> d;
            gpus.push_back({v, d});
        }
    }
    return gpus;
}

int main (int argc, const char *argv[]) {
    cout << "\t\t\t--- " << getUser() << "@" << getHost() << " ---\n";
    cout << "\t\t distro:\t" << getDistro() << "\n";
    cout << "\t\t kernel:\t" << getKernel() << "\n";
    cout << "\t\t uptime:\t" << getUptime() << "\n";
    cout << "\t\t shell:\t" << getShell() << "\n";
    cout << "\t\t󰍛 CPU:  \t" << getCPU() << "\n";
    cout << "\t\t GPU:  \t" << getGPU() << "\n";
    cout << "\t\t RAM:  \t" << getRAM() << "\n";

    return 0;
}

string getUser() {
    string username;
    username = std::getenv("USER");
    if (!username.empty()) return username;

    return "";
}

string getHost() {
    std::ifstream readHostname("/etc/hostname");

    if (!readHostname.is_open()) {
        std::cerr << "Error: couldn't read /etc/hostname\n";
        return "";
    }

    string hostname;
    getline(readHostname, hostname);

    readHostname.close();

    if (!hostname.empty()) return hostname;
    else return "";
}

string getDistro() {
    std::ifstream readOsRelease("/etc/os-release");

    if(!readOsRelease.is_open()) {
        std::cerr << "Error: Couldn't read /etc/os-release\n";
        return "";
    }

    string distroName;
    string line;
    while (std::getline(readOsRelease, line)) {
        if(line.empty() || line[0] == '#') continue;

        std::size_t delimiter = line.find("=");
        if(delimiter != string::npos) {
            string key = line.substr(0, delimiter);

            if (key == "NAME"){ 
                distroName = line.substr(delimiter + 1);
                distroName = distroName.substr(1, distroName.length() - 2);
            }
        }
    }

    readOsRelease.close();

    if (!distroName.empty()) return distroName;
    else return "";
}

string getKernel() {
    struct utsname kernelInfo;

    if (uname(&kernelInfo) != 0) {
        std::cerr << "Error: uname call failed\n";
        return "";
    }

    return kernelInfo.release;
}

string getUptime() {
    std::ifstream readUptime("/proc/uptime");

    if(!readUptime.is_open()) {
        std::cerr << "Error: Couldn't read /proc/uptime\n";
        return "";
    }

    string uptime;
    std::getline(readUptime, uptime);

    uptime = uptime.substr(0, uptime.find(' '));

    readUptime.close();

    int uptimeInt = std::stoi(uptime);
    uptime = "";
    for (int i = uptimeInt; i > 0; i /= 60) {
        if (i > 86400) uptime = uptime + std::to_string(i / 86400) + " days ";
        else if (i > 3600) uptime = uptime + std::to_string(i / 3600) + " hours ";
        else if (i > 60) uptime = uptime + std::to_string(i / 60) + " minutes";
    }

    if (!uptime.empty()) return uptime + "";

    return "";
}

string getShell() {
    string shell;
    shell = std::getenv("SHELL");
    if (!shell.empty()) return shell;

    return "";
}

string getCPU() {
    std::ifstream readCPU("/proc/cpuinfo");

    if(!readCPU.is_open()) {
        std::cerr << "Error: Couldn't read /proc/cpuinfo\n";
        return "";
    }

    string cpuName;
    string processor;
    string line;
    while (std::getline(readCPU, line)) {
        if(line.empty() || line[0] == '#') continue;

        std::size_t delimiter = line.find(":");
        if(delimiter != string::npos) {
            string key = line.substr(0, delimiter);

            if (key == "model name	"){ 
                cpuName = line.substr(delimiter);
                cpuName = cpuName.substr(2, cpuName.length() - 1);
            }
            else if (key == "processor	"){ 
                processor = line.substr(delimiter + 1);
                processor = processor.substr(1, processor.length() - 1);
            }
        }
    }

    readCPU.close();

    cpuName += " (" + std::to_string(std::stoi(processor) + 1) + ")";

    std::ifstream readFreq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");

    if(!readFreq.is_open()) {
        std::cerr << "Error: Couldn't read /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq\n";
        return "";
    }

    string freq;
    std::getline(readFreq, freq);
    float clock = std::stoi(freq) / 1000000.0f;
    std::stringstream ss;
    ss << clock;

    cpuName += " @ " + ss.str() + " GHz";

    readFreq.close();

    if (!cpuName.empty()) return cpuName;
    else return "";
}

string getGPU() {
    auto gpus = getGpuIds();
    if (gpus.empty()) return "";

    auto normalizeHex = [](string hex) {
        if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0) {
            hex = hex.substr(2);
        }
        for (char& c : hex) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return hex;
    };

    auto joinWith = [](const std::vector<string>& items, const string& sep) {
        string out;
        for (const auto& item : items) {
            if (item.empty()) continue;
            if (!out.empty()) out += sep;
            out += item;
        }
        return out;
    };

    std::ifstream readPciIds("/usr/share/hwdata/pci.ids");

    if(!readPciIds.is_open()) {
        std::cerr << "Error: Couldn't read /usr/share/hwdata/pci.ids\n";
        std::vector<string> ids;
        for (const auto& gpu : gpus) {
            string vendor = normalizeHex(gpu.vendor);
            string device = normalizeHex(gpu.device);
            if (vendor.empty() || device.empty()) continue;
            ids.push_back("0x" + vendor + ":0x" + device);
        }
        return joinWith(ids, " / ");
    }

    std::unordered_map<string, std::unordered_map<string, string>> pciMap;
    string line;
    string currentVendor;
    while (std::getline(readPciIds, line)) {
        if(line.empty() || line[0] == '#') continue;

        if (line[0] != '\t') {
            std::istringstream vendorLine(line);
            if (vendorLine >> currentVendor) {
                currentVendor = normalizeHex(currentVendor);
            } else {
                currentVendor.clear();
            }
            continue;
        }

        if (line.size() < 2 || line[1] == '\t') continue;
        if (currentVendor.empty()) continue;

        std::istringstream deviceLine(line.substr(1));
        string deviceId;
        if (!(deviceLine >> deviceId)) continue;

        string deviceName;
        std::getline(deviceLine, deviceName);
        if (!deviceName.empty()) {
            size_t first = deviceName.find_first_not_of(' ');
            if (first != string::npos) deviceName = deviceName.substr(first);
            else deviceName.clear();
        }

        if (!deviceName.empty()) {
            pciMap[currentVendor][normalizeHex(deviceId)] = deviceName;
        }
    }

    readPciIds.close();

    std::vector<string> gpuNames;
    for (const auto& gpu : gpus) {
        string vendor = normalizeHex(gpu.vendor);
        string device = normalizeHex(gpu.device);
        if (vendor.empty() || device.empty()) continue;

        string name;
        auto vendorIt = pciMap.find(vendor);
        if (vendorIt != pciMap.end()) {
            auto deviceIt = vendorIt->second.find(device);
            if (deviceIt != vendorIt->second.end()) {
                name = deviceIt->second;
            }
        }

        if (name.empty()) {
            name = "0x" + vendor + ":0x" + device;
        }

        gpuNames.push_back(name);
    }

    string result = joinWith(gpuNames, " / ");
    if (!result.empty()) return result;

    return "";
}

string getRAM() {
    std::ifstream readRAM("/proc/meminfo");

    if(!readRAM.is_open()) {
        std::cerr << "Error: Couldn't read /proc/meminfo\n";
        return "";
    }

    //

    int memkbs = 0;
    string line;
    while (std::getline(readRAM, line)) {
        if(line.empty() || line[0] == '#') continue;

        int delim = line.find(':');
        if (line.substr(0, delim) == "MemTotal") {
            line = line.substr(16);
            delim = line.find(' ');
            memkbs = std::stoi(line.substr(0, delim));
            break;
        }
    }

    readRAM.close();

    memkbs /= 1024;
    float memory = memkbs / 1024.0f;
    std::stringstream ss;
    ss << memory << " GB";
    string memGigs = ss.str();

    if (!memGigs.empty()) return memGigs;

    return "";
}
