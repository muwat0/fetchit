#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <cctype>
#include <string_view>
#include <functional>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <ctime>
#include <clocale>
#include <cwchar>

// TOML++ (Config parsing)
#include <toml++/toml.hpp>

using std::string, std::cout, std::vector;
namespace fs = std::filesystem;

string getUser();
string getHost();
string getDistro();
vector<string> distroArt();
string getKernel();
string getUptime();
string getShell();
string getCPU();
string getGPU();
string getRAM();
string getOsDate();

enum class Module {
    Distro,
    Kernel,
    Uptime,
    Shell,
    CPU,
    GPU,
    RAM,
    OsDate,
};

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

struct EnumHash {
    template <class T>
    size_t operator()(T v) const noexcept {
        return static_cast<size_t>(v);
    }
};

struct Config {
    vector<Module> modules;
    bool logoEnabled = true;

    std::unordered_map<Module, string, EnumHash> labels;
    std::unordered_map<Module, Color, EnumHash> colors;
};

struct ModuleSpec {
    Module id;
    std::string_view name;
    string defaultLabel;
    Color defaultColor;
    string (*getter)();
};

static const std::array<ModuleSpec, 8>& moduleSpecs() {
    static const std::array<ModuleSpec, 8> specs = {
        ModuleSpec{Module::Distro, "distro", " distro:", Color::Green, &getDistro},
        ModuleSpec{Module::Kernel, "kernel", " kernel:", Color::Magenta, &getKernel},
        ModuleSpec{Module::Uptime, "uptime", " uptime:", Color::Blue, &getUptime},
        ModuleSpec{Module::Shell, "shell", " shell:", Color::Magenta, &getShell},
        ModuleSpec{Module::CPU, "cpu", "󰍛 CPU:", Color::Yellow, &getCPU},
        ModuleSpec{Module::GPU, "gpu", "󰾲 GPU:", Color::Yellow, &getGPU},
        ModuleSpec{Module::RAM, "ram", " RAM:", Color::Red, &getRAM},
        ModuleSpec{Module::OsDate, "os_date", " OS Date:", Color::Blue, &getOsDate},
    };
    return specs;
}

static const ModuleSpec* findModuleSpec(std::string_view name) {
    for (const auto& spec : moduleSpecs()) {
        if (spec.name == name) return &spec;
    }
    return nullptr;
}

static const ModuleSpec* findModuleSpec(Module m) {
    for (const auto& spec : moduleSpecs()) {
        if (spec.id == m) return &spec;
    }
    return nullptr;
}

enum class FlagValue {
    None,
    Required,
};

struct FlagSpec {
    std::string_view longName;
    char shortName = '\0';
    FlagValue value = FlagValue::None;
    std::string_view valueName;
    std::string_view help;
    std::function<void(std::string_view)> onValue;
    std::function<void()> onFlag;
};

struct ParsedCli {
    bool showHelp = false;
    vector<string> positionals;
    vector<string> warnings;
};

static const FlagSpec* findLongFlag(const vector<FlagSpec>& specs, std::string_view name) {
    for (const auto& s : specs) {
        if (s.longName == name) return &s;
    }
    return nullptr;
}

static const FlagSpec* findShortFlag(const vector<FlagSpec>& specs, char name) {
    for (const auto& s : specs) {
        if (s.shortName == name && name != '\0') return &s;
    }
    return nullptr;
}

static ParsedCli parseCli(int argc, char** argv, const vector<FlagSpec>& specs) {
    ParsedCli out;
    bool onlyPositionals = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i] ? argv[i] : "";
        if (arg.empty()) continue;

        if (!onlyPositionals && arg == "--") {
            onlyPositionals = true;
            continue;
        }

        if (!onlyPositionals && arg.rfind("--", 0) == 0) {
            std::string_view name = arg.substr(2);
            std::string_view value;
            const size_t eq = name.find('=');
            if (eq != std::string_view::npos) {
                value = name.substr(eq + 1);
                name = name.substr(0, eq);
            }

            const FlagSpec* spec = findLongFlag(specs, name);
            if (!spec) {
                out.warnings.push_back("fetchit: warning: unknown option '--" + string(name) + "'; ignoring");
                continue;
            }

            if (spec->value == FlagValue::None) {
                if (!value.empty()) {
                    out.warnings.push_back("fetchit: warning: option '--" + string(name)
                                           + "' does not take a value; ignoring value");
                }
                if (spec->onFlag) spec->onFlag();
            } else {
                if (value.empty()) {
                    if (i + 1 >= argc) {
                        out.warnings.push_back("fetchit: warning: option '--" + string(name)
                                               + "' requires a value; ignoring");
                        continue;
                    }
                    value = argv[++i];
                }
                if (spec->onValue) spec->onValue(value);
            }
            continue;
        }

        if (!onlyPositionals && arg.size() >= 2 && arg[0] == '-' && arg[1] != '-') {
            std::string_view group = arg.substr(1);
            bool consumedValue = false;

            for (size_t j = 0; j < group.size(); ++j) {
                const char ch = group[j];
                const FlagSpec* spec = findShortFlag(specs, ch);
                if (!spec) {
                    out.warnings.push_back(string("fetchit: warning: unknown option '-") + ch + "'; ignoring");
                    continue;
                }

                if (spec->value == FlagValue::None) {
                    if (spec->onFlag) spec->onFlag();
                    continue;
                }

                std::string_view value = group.substr(j + 1);
                if (value.empty()) {
                    if (i + 1 >= argc) {
                        out.warnings.push_back(string("fetchit: warning: option '-") + ch
                                               + "' requires a value; ignoring");
                        break;
                    }
                    value = argv[++i];
                }

                if (spec->onValue) spec->onValue(value);
                consumedValue = true;
                break;
            }

            (void)consumedValue;
            continue;
        }

        out.positionals.push_back(string(arg));
    }

    return out;
}

static void printHelp(const char* argv0, const vector<FlagSpec>& specs) {
    const char* prog = (argv0 && *argv0) ? argv0 : "fetchit";
    std::cout << "Usage: " << prog << " [options]\n\n";
    std::cout << "Options:\n";
    for (const auto& s : specs) {
        std::cout << "  ";
        bool any = false;
        if (s.shortName != '\0') {
            std::cout << "-" << s.shortName;
            any = true;
        }
        if (!s.longName.empty()) {
            if (any) std::cout << ", ";
            std::cout << "--" << s.longName;
            any = true;
        }
        if (s.value == FlagValue::Required) {
            std::cout << " <" << (s.valueName.empty() ? "value" : s.valueName) << ">";
        }
        std::cout << "\n";
        if (!s.help.empty()) {
            std::cout << "      " << s.help << "\n";
        }
    }
}

Config defaultConfig() {
    Config cfg;

    cfg.modules.clear();
    cfg.labels.clear();
    cfg.colors.clear();
    cfg.modules.reserve(moduleSpecs().size());
    for (const auto& spec : moduleSpecs()) {
        cfg.modules.push_back(spec.id);
        cfg.labels[spec.id] = spec.defaultLabel;
        cfg.colors[spec.id] = spec.defaultColor;
    }

    cfg.logoEnabled = true;
    return cfg;
}

static bool getenvNonEmpty(const char* name, string& out) {
    const char* v = std::getenv(name);
    if (!v || !*v) return false;
    out = v;
    return true;
}

static fs::path defaultConfigPath() {
    // XDG base dir spec: $XDG_CONFIG_HOME, fallback to ~/.config.
    string xdg;
    if (getenvNonEmpty("XDG_CONFIG_HOME", xdg)) {
        return fs::path(xdg) / "fetchit" / "config.toml";
    }

    string home;
    if (getenvNonEmpty("HOME", home)) {
        return fs::path(home) / ".config" / "fetchit" / "config.toml";
    }

    return {};
}

static fs::path findExistingConfigPath() {
    const fs::path p = defaultConfigPath();
    if (p.empty()) return {};

    std::error_code ec;
    if (fs::exists(p, ec) && !ec) return p;
    return {};
}

static string toLowerAscii(string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

static bool parseColorName(const string& name, Color& out) {
    const string n = toLowerAscii(name);

    if (n == "red") {
        out = Color::Red;
        return true;
    }
    if (n == "green") {
        out = Color::Green;
        return true;
    }
    if (n == "blue") {
        out = Color::Blue;
        return true;
    }
    if (n == "yellow") {
        out = Color::Yellow;
        return true;
    }
    if (n == "magenta") {
        out = Color::Magenta;
        return true;
    }
    if (n == "purple") {
        out = Color::Purple;
        return true;
    }
    if (n == "cyan") {
        out = Color::Cyan;
        return true;
    }
    if (n == "gray" || n == "grey") {
        out = Color::Gray;
        return true;
    }
    if (n == "dark") {
        out = Color::Dark;
        return true;
    }

    return false;
}

static Config loadConfigOrDefault(vector<string>& warnings) {
    warnings.clear();
    const fs::path configPath = findExistingConfigPath();
    if (configPath.empty()) return defaultConfig();

    Config cfg = defaultConfig();

    toml::table tbl;
    try {
        tbl = toml::parse_file(configPath.string());
    } catch (const toml::parse_error& err) {
        std::ostringstream oss;
        oss << "fetchit: warning: invalid config " << configPath.string() << ": " << err << "; using defaults";
        warnings.push_back(oss.str());
        return defaultConfig();
    }

    if (auto modulesArr = tbl["modules"].as_array()) {
        vector<Module> modules;
        modules.reserve(modulesArr->size());
        for (const auto& node : *modulesArr) {
            const auto* s = node.as_string();
            if (!s) {
                warnings.push_back("fetchit: warning: modules[] must be strings; ignoring non-string entry");
                continue;
            }
            const string name = s->get();
            const ModuleSpec* spec = findModuleSpec(name);
            if (!spec) {
                warnings.push_back("fetchit: warning: unknown module '" + name + "'; ignoring");
                continue;
            }
            modules.push_back(spec->id);
        }

        if (modules.empty()) {
            warnings.push_back("fetchit: warning: modules list is empty after validation; using defaults");
        } else {
            cfg.modules = std::move(modules);
        }
    } else if (tbl.contains("modules")) {
        warnings.push_back("fetchit: warning: modules must be an array of strings; using defaults");
    }

    // [logo]
    if (auto logoTbl = tbl["logo"].as_table()) {
        if (auto enabled = (*logoTbl)["enabled"].value<bool>()) {
            cfg.logoEnabled = *enabled;
        } else if (logoTbl->contains("enabled")) {
            warnings.push_back("fetchit: warning: logo.enabled must be a boolean; using default");
        }
    } else if (tbl.contains("logo")) {
        warnings.push_back("fetchit: warning: logo must be a table; ignoring");
    }

    // [labels]
    if (auto labelsTbl = tbl["labels"].as_table()) {
        for (auto&& [k, v] : *labelsTbl) {
            const string key = string(k.str());
            const ModuleSpec* spec = findModuleSpec(key);
            if (!spec) {
                warnings.push_back("fetchit: warning: unknown labels key '" + key + "'; ignoring");
                continue;
            }
            auto* s = v.as_string();
            if (!s) {
                warnings.push_back("fetchit: warning: labels." + key + " must be a string; ignoring");
                continue;
            }
            cfg.labels[spec->id] = s->get();
        }
    } else if (tbl.contains("labels")) {
        warnings.push_back("fetchit: warning: labels must be a table; ignoring");
    }

    // [colors]
    if (auto colorsTbl = tbl["colors"].as_table()) {
        for (auto&& [k, v] : *colorsTbl) {
            const string key = string(k.str());
            const ModuleSpec* spec = findModuleSpec(key);
            if (!spec) {
                warnings.push_back("fetchit: warning: unknown colors key '" + key + "'; ignoring");
                continue;
            }
            auto* s = v.as_string();
            if (!s) {
                warnings.push_back("fetchit: warning: colors." + key + " must be a string; ignoring");
                continue;
            }

            Color c;
            const string colorName = s->get();
            if (!parseColorName(colorName, c)) {
                warnings.push_back("fetchit: warning: unknown color name '" + colorName + "' for colors." + key
                                   + "; ignoring");
                continue;
            }
            cfg.colors[spec->id] = c;
        }
    } else if (tbl.contains("colors")) {
        warnings.push_back("fetchit: warning: colors must be a table; ignoring");
    }

    return cfg;
}

struct DistroLogo {
    string id;
    Color color;
    std::array<string, 8> art;
};

struct gpuId {
    string vendor;
    string device;
};

vector<gpuId> getGpuIds() {
    vector<gpuId> gpus;
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

int main (int argc, char** argv) {
    std::setlocale(LC_ALL, "");

    ParsedCli cliState;
    const vector<FlagSpec> flagSpecs = {
        FlagSpec{
            .longName = "help",
            .shortName = 'h',
            .value = FlagValue::None,
            .valueName = {},
            .help = "Show this help message",
            .onValue = {},
            .onFlag = [&]() { cliState.showHelp = true; },
        },
    };

    ParsedCli cli = parseCli(argc, argv, flagSpecs);
    cliState.warnings.insert(cliState.warnings.end(), cli.warnings.begin(), cli.warnings.end());
    cliState.positionals = std::move(cli.positionals);

    if (cliState.showHelp) {
        printHelp(argv && argv[0] ? argv[0] : "fetchit", flagSpecs);
        return 0;
    }

    for (const auto& w : cliState.warnings) {
        std::cerr << w << "\n";
    }

    vector<string> configWarnings;
    const Config config = loadConfigOrDefault(configWarnings);
    for (const auto& w : configWarnings) {
        std::cerr << w << "\n";
    }

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

    vector<string> infoLines;
    infoLines.reserve(config.modules.size());
    for (Module m : config.modules) {
        const ModuleSpec* spec = findModuleSpec(m);
        if (!spec) continue;

        auto labelIt = config.labels.find(m);
        const string& label = (labelIt != config.labels.end()) ? labelIt->second : spec->defaultLabel;

        auto colorIt = config.colors.find(m);
        const Color c = (colorIt != config.colors.end()) ? colorIt->second : spec->defaultColor;

        const string value = spec->getter ? spec->getter() : string{};
        infoLines.push_back(formatLine(c, label, value));
    }

    vector<string> logoLines;
    if (config.logoEnabled) {
        logoLines = distroArt();
    }
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

vector<string> distroArt() {
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

    const vector<DistroLogo> logos = {
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
               "       _  ",
               "   ,--(_) ",
               " _/ ;-._\\ ",
               "(_)(   ) )",
               "  \\ ;-'_/ ",
               "   `--(_) "
            }
        },
        {
            "fedora",
            Color::Blue,
            {
                "       _____  ",
                "     /   __)\\ ",
                "     |  /  \\ \\",
                "  ___|  |__/ /",
                " / (_    _)_/ ",
                "/ /  |  |     ",
                "\\ \\__/  |     ",
                " \\(_____/     ",
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

    vector<string> out;
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

    while(shell.find('/') < shell.length()) shell.erase(0, shell.find('/') + 1);

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

    auto joinWith = [](const vector<string>& items, const string& sep) {
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
        vector<string> ids;
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

    vector<string> gpuNames;
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
    ss << std::fixed << std::setprecision(2) << memory << " GB";
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
