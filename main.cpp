#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cctype>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <ctime>
#include <clocale>
#include <cwchar>

using std::string, std::cout;
namespace fs = std::filesystem;

string getUser();
string getHost();
string getDistro();
std::vector<string> distroArt();
string getKernel();
string getUptime();
string getShell();
string getCPU();
string getGPU();
string getRAM();
string getOsDate();

enum class Color {
    Red,
    RedLight,
    RedDeep,
    Green,
    GreenLight,
    GreenDeep,
    Blue,
    BlueLight,
    BlueDeep,
    Yellow,
    YellowLight,
    Magenta,
    Purple,
    PurpleLight,
    PurpleDeep,
    Orange,
    OrangeLight,
    OrangeDeep,
    Cyan,
    Gray,
    Dark,
    Reset
};

string color(Color c);

struct DistroLogo {
    string id;
    Color color;
    std::array<string, 8> art;
};

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

int main () {
    std::setlocale(LC_ALL, "");

    auto displayWidth = [&](const string& text) {
        std::mbstate_t state{};
        const char* src = text.c_str();
        size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);
        if (len == static_cast<size_t>(-1)) {
            return static_cast<int>(text.size());
        }
        std::wstring wide(len, L'\0');
        state = std::mbstate_t{};
        src = text.c_str();
        std::mbsrtowcs(wide.data(), &src, len, &state);

        int width = 0;
        for (wchar_t ch : wide) {
            int w = wcwidth(ch);
            if (w > 0) width += w;
        }
        return width;
    };

    auto formatLine = [&](Color c, const string& label, const string& value) {
        const size_t labelWidth = 12;
        const size_t valueGap = 2;
        string padded = label;
        int currentWidth = displayWidth(padded);
        if (currentWidth < static_cast<int>(labelWidth)) {
            padded += string(labelWidth - currentWidth, ' ');
        }
        padded += string(valueGap, ' ');
        return color(c) + padded + color(Color::Reset) + value;
    };

    std::vector<string> infoLines = {
        formatLine(Color::Green, " distro:", getDistro()),
        formatLine(Color::Magenta, " kernel:", getKernel()),
        formatLine(Color::Blue, " uptime:", getUptime()),
        formatLine(Color::Magenta, " shell:", getShell()),
        formatLine(Color::Yellow, "󰍛 CPU:", getCPU()),
        formatLine(Color::Yellow, "󰾲 GPU:", getGPU()),
        formatLine(Color::Red, " RAM:", getRAM()),
        formatLine(Color::Blue, " OS Date:", getOsDate())
    };

    std::vector<string> logoLines = distroArt();
    size_t logoWidth = 0;
    for (const auto& line : logoLines) {
        if (line.size() > logoWidth) logoWidth = line.size();
    }

    const string gap = "   ";
    cout << string(logoWidth, ' ') << gap
         << "--- " << color(Color::Green) << getUser() << color(Color::Reset)
         << "@" << color(Color::Red) << getHost() << color(Color::Reset) << " ---\n";
    cout << "\n";

    size_t totalLines = infoLines.size();
    if (logoLines.size() > totalLines) totalLines = logoLines.size();

    for (size_t i = 0; i < totalLines; ++i) {
        if (i < logoLines.size()) {
            cout << logoLines[i];
            if (logoLines[i].size() < logoWidth) {
                cout << string(logoWidth - logoLines[i].size(), ' ');
            }
        } else {
            cout << string(logoWidth, ' ');
        }

        cout << gap;

        if (i < infoLines.size()) {
            cout << infoLines[i];
        }

        cout << "\n";
    }

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

            if (key == "PRETTY_NAME"){
                distroName = line.substr(delimiter + 1);
                distroName = distroName.substr(1, distroName.length() - 2);
            }
        }
    }

    readOsRelease.close();

    if (!distroName.empty()) return distroName;
    else return "";
}

string color(Color c) {
    switch (c) {
        case Color::Red:
        case Color::RedLight:
        case Color::RedDeep:
            return "\033[1;31m";
        case Color::Green:
        case Color::GreenLight:
        case Color::GreenDeep:
            return "\033[1;32m";
        case Color::Blue:
        case Color::BlueLight:
        case Color::BlueDeep:
            return "\033[1;34m";
        case Color::Yellow:
        case Color::YellowLight:
        case Color::Orange:
        case Color::OrangeLight:
        case Color::OrangeDeep:
            return "\033[1;33m";
        case Color::Magenta:
        case Color::Purple:
        case Color::PurpleLight:
        case Color::PurpleDeep:
            return "\033[1;35m";
        case Color::Cyan:
            return "\033[1;36m";
        case Color::Gray:
        case Color::Dark:
            return "\033[1;37m";
        case Color::Reset:
            return "\033[0m";
    }

    return "\033[0m";
}

std::vector<string> distroArt() {
    std::ifstream readOsRelease("/etc/os-release");

    if(!readOsRelease.is_open()) {
        std::cerr << "Error: Couldn't read /etc/os-release\n";
    }

    string distroID;
    string line;
    while (std::getline(readOsRelease, line)) {
        if(line.empty() || line[0] == '#') continue;

        std::size_t delimiter = line.find("=");
        if(delimiter != string::npos) {
            string key = line.substr(0, delimiter);

            if (key == "ID"){
                distroID = line.substr(delimiter + 1);
                if (distroID[0] == '"' && distroID[distroID.length() - 1] == '"') distroID = distroID.substr(1, distroID.length() - 2);
            }
        }
    }

    readOsRelease.close();

    const std::vector<DistroLogo> logos = {
        {
            "arch",
            Color::Blue,
            {
                "      /\\      ",
                "     /  \\     ",
                "    /    \\    ",
                "   /      \\   ",
                "  /   ,,   \\  ",
                " /   |  |   \\ ",
                "/_-''    ''-_\\",
                "               "
            }
        },
        {
            "cachyos",
            Color::Green,
            {
                "   /''''''''''''/   ",
                "  /''''''''''''/    ",
                " /''''''/           ",
                "/''''''/            ",
                "\\......\\          ",
                " \\......\\         ",
                "  \\.............../",
                "   \\............./ "
            }
        },
        {
            "debian",
            Color::Red,
            {
                "  _____  ",
                " /  __ \\",
                "|  /    |",
                "|  \\___-",
                "-_       ",
                "  --_    ",
                "         ",
                "         "
            }
        },
        {
            "ubuntu",
            Color::Orange,
            {
                "------    |\\   ",     // TODO:
                "   |      | \\  ",
                "   |      |  / ",
                "   |      |/   ",
                "               ",
                "               ",
                "               ",
                "               "
            }
        },
        {
            "fedora",
            Color::Blue,
            {
                "------    |\\   ",     // TODO:
                "   |      | \\  ",
                "   |      |  / ",
                "   |      |/   ",
                "               ",
                "               ",
                "               ",
                "               "
            }
        },
        {
            "manjaro",
            Color::Green,
            {
                "||||||||| ||||",
                "||||||||| ||||",
                "||||      ||||",
                "|||| |||| ||||",
                "|||| |||| ||||",
                "|||| |||| ||||",
                "|||| |||| ||||",
                "              "
            }
        },
        {
            "opensuse",
            Color::Green,
            {
                "  _______  ",
                "__|   __ \\ ",
                "     / .\\ \\",
                "     \\__/ |",
                "   _______|",
                "   \\_______",
                "__________/",
                "           "
            }
        },
        {
            "pop",
            Color::Cyan,
            {
                "______           ",
                "\\   _ \\        __",
                " \\ \\ \\ \\      / /",
                "  \\ \\_\\ \\    / / ",
                "   \\  ___\\  /_/  ",
                "    \\ \\    _     ",
                "   __\\_\\__(_)_   ",
                "  (___________)` "
            }
        },
        {
            "linuxmint",
            Color::Green,
            {
                " __________  ",
                "|_          \\",
                "  | | _____ |",
                "  | | | | | |",
                "  | | | | | |",
                "  | \\_____/ |",
                "  \\_________/",
            }
        },
        {
            "endeavouros",
            Color::Purple,
            {
                "          /o.       ",
                "        /sssso-     ",
                "      /ossssssso:   ",
                "    /ssssssssssso+  ",
                "  /ssssssssssssssso+",
                "//osssssssssssssso+-",
                " `+++++++++++++++-` "
            }
        },
        {
            "void",
            Color::Green,
            {
                "    ____   ",
                "  'pfPfp.% ",
                "//  _._  \\\\",
                "UU |===| UU",
                "\\\\  ^~^  //",
                " `0PpppP'  ",
                "   `````   "
            }
        },
        {
            "alpine",
            Color::Blue,
            {
                "   /\\ /\\    ",
                "  // \\  \\   ",
                " //   \\  \\  ",
                "///    \\  \\ ",
                "//      \\  \\",
                "         \\  "
            }
        },
    };

    // Ubuntu: Orange (+ Light/Deep Orange)
    // Fedora: Blue + Gray/White

    std::vector<string> out;
    for (const auto& logo : logos) {
        if (logo.id == distroID) {
            const string prefix = color(logo.color);
            const string suffix = color(Color::Reset);
            for (const auto& line : logo.art) {
                out.push_back(prefix + line + suffix);
            }
            return out;
        }
    }

    return out;
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

string getOsDate() {
    struct stat info;
    if (stat("/etc/hostname", &info) != 0) {
        std::cerr << "Error: couldn't read /etc/hostname metadata\n";
        return "";
    }

    std::time_t now = std::time(nullptr);
    if (now < info.st_ctime) {
        return "";
    }

    std::tm startTime{};
    std::tm endTime{};
    if (localtime_r(&info.st_ctime, &startTime) == nullptr) {
        std::cerr << "Error: couldn't convert /etc/hostname ctime\n";
        return "";
    }
    if (localtime_r(&now, &endTime) == nullptr) {
        std::cerr << "Error: couldn't convert current time\n";
        return "";
    }

    auto isLeapYear = [](int year) {
        if (year % 400 == 0) return true;
        if (year % 100 == 0) return false;
        return year % 4 == 0;
    };

    auto daysInMonth = [&](int year, int month) {
        if (month == 2) return isLeapYear(year) ? 29 : 28;
        return 30 + ((month + (month > 7)) % 2);
    };

    int startYear = startTime.tm_year + 1900;
    int startMonth = startTime.tm_mon + 1;
    int startDay = startTime.tm_mday;
    int startHour = startTime.tm_hour;
    int startMinute = startTime.tm_min;

    int endYear = endTime.tm_year + 1900;
    int endMonth = endTime.tm_mon + 1;
    int endDay = endTime.tm_mday;
    int endHour = endTime.tm_hour;
    int endMinute = endTime.tm_min;

    if (endMinute < startMinute) {
        endMinute += 60;
        endHour -= 1;
    }

    if (endHour < startHour) {
        endHour += 24;
        endDay -= 1;
    }

    if (endDay < startDay) {
        endMonth -= 1;
        if (endMonth == 0) {
            endMonth = 12;
            endYear -= 1;
        }
        endDay += daysInMonth(endYear, endMonth);
    }

    if (endMonth < startMonth) {
        endMonth += 12;
        endYear -= 1;
    }

    int years = endYear - startYear;
    int months = endMonth - startMonth;
    int days = endDay - startDay;
    int hours = endHour - startHour;
    int minutes = endMinute - startMinute;

    if (years < 0 || months < 0 || days < 0 || hours < 0 || minutes < 0) {
        return "";
    }

    string out;
    if (years > 0) out += std::to_string(years) + (years == 1 ? " year" : " years");
    if (months > 0) {
        if (!out.empty()) out += " ";
        out += std::to_string(months) + (months == 1 ? " month" : " months");
    }
    if (days > 0) {
        if (!out.empty()) out += " ";
        out += std::to_string(days) + (days == 1 ? " day" : " days");
    }
    if (hours > 0) {
        if (!out.empty()) out += " ";
        out += std::to_string(hours) + (hours == 1 ? " hour" : " hours");
    }
    if (minutes > 0) {
        if (!out.empty()) out += " ";
        out += std::to_string(minutes) + (minutes == 1 ? " minute" : " minutes");
    }

    if (!out.empty()) return out;

    return "0 minutes";
}
