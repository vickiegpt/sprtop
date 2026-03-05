#define SPRTOP_NUM_CHA_BOXES 60
#include "MeshUp/MeshReverseEngineering/cha_coremapping.cpp"
#include "MeshUp/MeshReverseEngineering/disabled_core.cpp"
#include "MeshUp/MeshReverseEngineering/logging.cpp"
#include <cstdint>
#include <iostream>
#include <map>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
// ------------------
// regular C++ header
// ------------------
enum tile_type { XCC, LCC, MCC, UNKNOWN };

// XCC: 7x7 grid, 4 IMCs, 33 core positions (2 positions pre-disabled as type 5)
std::vector<std::vector<int>> XCCTemplate = {
    {3, 4, 4, 3, 4, 0, 0}, //
    {1, 1, 1, 1, 5, 1, 5}, // 1 is a core location
    {2, 1, 1, 1, 1, 1, 2}, // 2 is IMC (internal memory controller)
    {1, 1, 1, 1, 1, 1, 1}, // 3 UPI
    {1, 1, 1, 1, 1, 1, 1}, // 4 PCIE/CXL
    {2, 1, 1, 1, 1, 1, 2}, // 5 Disabled Cores
    {3, 4, 4, 1, 4, 1, 1}, //
};

// LCC: 6x5 grid, 2 IMCs, ~20 core positions
std::vector<std::vector<int>> LCCTemplate = {
    {3, 4, 3, 4, 0},    // UPI, PCIe, UPI, PCIe, empty
    {1, 1, 1, 1, 1},    // 5 core positions
    {2, 1, 1, 1, 2},    // IMC, 3 cores, IMC
    {1, 1, 1, 1, 1},    // 5 core positions
    {1, 1, 1, 1, 1},    // 5 core positions
    {3, 4, 1, 4, 1},    // UPI, PCIe, core, PCIe, core
};

// MCC: 7x7 grid, 4 IMCs, 34 core positions
std::vector<std::vector<int>> MCCTemplate = {
    {3, 4, 4, 3, 4, 0, 0}, //
    {1, 1, 1, 1, 1, 1, 1}, //
    {2, 1, 1, 1, 1, 1, 2}, //
    {1, 1, 1, 1, 1, 1, 1}, //
    {1, 1, 1, 1, 1, 1, 1}, //
    {2, 1, 1, 1, 1, 1, 2}, //
    {3, 4, 4, 1, 4, 1, 1}, //
};
std::map<std::string, enum tile_type> cpu_map = {
    {"Bronze 3408U", MCC},    {"Silver 4410T", MCC},       {"Silver 4410Y", MCC},    {"Silver 4416+", MCC},
    {"Gold 5411N", MCC},      {"Gold 5412U", MCC},         {"Gold 5415+", MCC},      {"Gold 5416S", MCC},
    {"Gold 5418N", MCC},      {"Gold 5418Y", MCC},         {"Gold 5420+", MCC},      {"Gold 5423N", LCC},
    {"Gold 5433N", LCC},      {"Gold 6403N", MCC},         {"Gold 6414U", XCC},      {"Gold 6416H", MCC},
    {"Gold 6418H", MCC},      {"Gold 6421N", MCC},         {"Gold 6423N", MCC},      {"Gold 6426Y", MCC},
    {"Gold 6428N", MCC},      {"Gold 6430", XCC},          {"Gold 6433N", MCC},      {"Gold 6433NE", MCC},
    {"Gold 6434", MCC},       {"Gold 6434H", MCC},         {"Gold 6438M", MCC},      {"Gold 6438N", MCC},
    {"Gold 6438Y+", MCC},     {"Gold 6442Y", MCC},         {"Gold 6443N", MCC},      {"Gold 6444Y", MCC},
    {"Gold 6448H", MCC},      {"Gold 6448Y", MCC},         {"Gold 6454S", XCC},      {"Gold 6458Q", MCC},
    {"Platinum 8444H", XCC},  {"Platinum 8450H", XCC},     {"Platinum 8452Y", XCC},  {"Platinum 8454H", XCC},
    {"Platinum 8458P", XCC},  {"Platinum 8460H", XCC},     {"Platinum 8460Y+", XCC}, {"Platinum 8461V", XCC},
    {"Platinum 8462Y+", XCC}, {"Platinum 8468", XCC},      {"Platinum 8468H", XCC},  {"Platinum 8468V", XCC},
    {"Platinum 8470", XCC},   {"Platinum 8470N", XCC},     {"Platinum 8470Q", XCC},  {"Platinum 8471N", XCC},
    {"Platinum 8480+", XCC},  {"Platinum 8480C", UNKNOWN}, {"Platinum 8490H", XCC},  {"Max 9460", XCC},
    {"Max 9462", XCC},        {"Max 9468", XCC},           {"Max 9470", XCC},        {"Max 9480", XCC}};

static const char* tile_type_to_string(enum tile_type t) {
    switch (t) {
    case XCC: return "XCC";
    case LCC: return "LCC";
    case MCC: return "MCC";
    case UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

// Count positions with value 1 or 5 in a template (core positions including disabled)
static int count_core_positions(const std::vector<std::vector<int>>& tmpl) {
    int count = 0;
    for (const auto& row : tmpl) {
        for (int cell : row) {
            if (cell == 1 || cell == 5) {
                count++;
            }
        }
    }
    return count;
}

// Count only active core positions (value == 1) in a template
static int count_active_cores(const std::vector<std::vector<int>>& tmpl) {
    int count = 0;
    for (const auto& row : tmpl) {
        for (int cell : row) {
            if (cell == 1) {
                count++;
            }
        }
    }
    return count;
}

// Detect number of sockets by parsing lscpu output
static int detect_num_sockets() {
    std::string output = exec("lscpu");
    std::string line = parseLscpuOutput(output, "Socket(s):");
    if (!line.empty()) {
        // Extract the number after the colon
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string num_str = line.substr(pos + 1);
            // Trim whitespace
            num_str.erase(0, num_str.find_first_not_of(" \t"));
            num_str.erase(num_str.find_last_not_of(" \t\n\r") + 1);
            int n = std::atoi(num_str.c_str());
            if (n > 0) return n;
        }
    }
    return 1; // fallback
}

namespace sprtop {

class SPRCoreCHA {
public:
    SPRCoreCHA(int socket_id = 0) : socket_id_(socket_id) {
        detect_cpu_type();

        std::vector<std::vector<int>> base_template;
        switch (type_) {
        case XCC:
            base_template = XCCTemplate;
            break;
        case LCC:
            base_template = LCCTemplate;
            break;
        case MCC:
            base_template = MCCTemplate;
            break;
        case UNKNOWN:
        default:
            std::cerr << "WARNING: Unknown CPU type, falling back to MCC template\n";
            type_ = MCC;
            base_template = MCCTemplate;
            break;
        }

        // Enumerate CAPID6 PCI devices (one per socket)
        auto devices = enumerate_capid6_devices();
        if (socket_id_ < 0 || socket_id_ >= (int)devices.size()) {
            std::cerr << "WARNING: socket_id " << socket_id_ << " out of range (found "
                      << devices.size() << " devices), falling back to socket 0\n";
            socket_id_ = 0;
        }

        // Get disabled core info via CAPID6 for this socket's PCI device
        // Pass base_template by value so each socket gets its own copy
        topology_grid_ = disabled_core(base_template, devices[socket_id_]);

        // Count active CHAs (positions with value 1 after disabled_core processing)
        int num_active_chas = count_active_cores(topology_grid_);
        num_cores_ = num_active_chas;

        int num_sockets = detect_num_sockets();

        // Run the CHA mapping analysis for this socket
        coremap_ = get_coremapping(topology_grid_, num_active_chas, num_sockets, socket_id_);

        // Build core2cha_map: iterate column-major (matching CHA physical layout)
        // coremap_[i][j] < 0 means CHA ID = -(coremap_[i][j] + 1)
        // Only consider positions where the topology template has value 1 (active core)
        core2cha_map.resize(num_active_chas, 0);
        int core_index = 0;
        for (size_t j = 0; j < coremap_[0].size(); j++) {
            for (size_t i = 0; i < coremap_.size(); i++) {
                if (coremap_[i][j] < 0 && topology_grid_[i][j] == 1) {
                    int cha_id = -(coremap_[i][j] + 1);
                    if (core_index < num_active_chas) {
                        core2cha_map[core_index] = cha_id;
                        core_index++;
                    }
                }
            }
        }
    };
    ~SPRCoreCHA() = default;
    enum tile_type type_ = UNKNOWN;
    int socket_id_ = 0;
    std::vector<std::vector<int>> coremap_;
    std::vector<std::vector<int>> topology_grid_;
    std::vector<uint64_t> core2cha_map;
    int num_cores_ = 0;

    uint64_t get_core_cha(const size_t &index) const {
        if (index >= core2cha_map.size()) {
            throw std::out_of_range("Core index out of range");
        }
        return core2cha_map[index];
    }

    std::string get_tile_type() const {
        return tile_type_to_string(type_);
    }

    int get_num_cores() const {
        return num_cores_;
    }

    int get_socket_id() const {
        return socket_id_;
    }

    std::vector<std::vector<int>> get_coremap() const {
        return coremap_;
    }

    std::vector<std::vector<int>> get_topology_grid() const {
        return topology_grid_;
    }

    static int get_num_sockets() {
        return detect_num_sockets();
    }

private:
    void detect_cpu_type() {
        std::string output = exec("lscpu");
        std::string searchTerm = "Model name:";
        std::string modelLine = parseLscpuOutput(output, searchTerm);

        if (modelLine.empty()) {
            std::cerr << "CPU model name not found in lscpu output." << std::endl;
            type_ = UNKNOWN;
            return;
        }

        std::cout << modelLine << std::endl;

        // Use substring matching: cpu_map keys like "Gold 5418Y" should match
        // lscpu lines like "Model name: Intel(R) Xeon(R) Gold 5418Y"
        for (const auto& [key, val] : cpu_map) {
            if (modelLine.find(key) != std::string::npos) {
                type_ = val;
                return;
            }
        }

        std::cerr << "CPU model not found in cpu_map, defaulting to UNKNOWN\n";
        type_ = UNKNOWN;
    }
};
} // namespace sprtop

// ----------------
// Python interface
// ----------------

namespace py = pybind11;

PYBIND11_MODULE(sprtop_core, m) {
    py::class_<sprtop::SPRCoreCHA>(m, "SPRCoreCHA")
        .def(py::init<int>(), py::arg("socket_id") = 0)
        .def("get_core_cha", &sprtop::SPRCoreCHA::get_core_cha, py::arg("index"))
        .def("get_tile_type", &sprtop::SPRCoreCHA::get_tile_type)
        .def("get_num_cores", &sprtop::SPRCoreCHA::get_num_cores)
        .def("get_socket_id", &sprtop::SPRCoreCHA::get_socket_id)
        .def("get_coremap", &sprtop::SPRCoreCHA::get_coremap)
        .def("get_topology_grid", &sprtop::SPRCoreCHA::get_topology_grid)
        .def_static("get_num_sockets", &sprtop::SPRCoreCHA::get_num_sockets);
}
