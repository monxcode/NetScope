#include "netscope/discovery/topology.h"
#include "netscope/core/logger.h"

#include <sstream>
#include <algorithm>
#include <fstream>

namespace netscope {
namespace discovery {

void Topology::AddDevice(const Device& device) {
    auto node = BuildNode(device);
    nodes_.push_back(node);
}

void Topology::SetGateway(const std::string& ip) {
    gateway_ip_ = ip;
}

void Topology::SetInternet(bool connected) {
    internet_connected_ = connected;
}

std::string Topology::GenerateASCII() const {
    std::ostringstream oss;

    oss << "Internet\n";
    if (internet_connected_) {
        oss << "    |\n";
    }

    auto gateway_it = std::find_if(nodes_.begin(), nodes_.end(),
                                    [this](const TopologyNode& n) {
                                        return n.id == gateway_ip_;
                                    });

    if (gateway_it != nodes_.end()) {
        oss << " " << gateway_it->label << " (" << gateway_it->id << ")\n";
    } else if (!gateway_ip_.empty()) {
        oss << " Gateway (" << gateway_ip_ << ")\n";
    } else if (!nodes_.empty()) {
        oss << " Router\n";
    }

    int child_count = 0;
    for (const auto& node : nodes_) {
        if (node.id == gateway_ip_) continue;
        child_count++;
        if (child_count == 1) {
            oss << "  ├── ";
        } else if (child_count < static_cast<int>(nodes_.size())) {
            oss << "  ├── ";
        } else {
            oss << "  └── ";
        }
        oss << node.label;
        if (!node.id.empty()) {
            oss << " (" << node.id << ")";
        }
        oss << "\n";
    }

    if (nodes_.empty()) {
        oss << "  (no devices discovered)\n";
    }

    return oss.str();
}

std::string Topology::GenerateDOT() const {
    std::ostringstream oss;
    oss << "digraph NetScopeTopology {\n";
    oss << "    rankdir=TB;\n";
    oss << "    node [shape=box, style=rounded];\n\n";

    oss << "    internet [label=\"Internet\", shape=cloud];\n";

    for (const auto& node : nodes_) {
        std::string safe_id = node.id;
        std::replace(safe_id.begin(), safe_id.end(), '.', '_');
        std::string escaped_label = node.label;
        auto pos = escaped_label.find('"');
        while (pos != std::string::npos) {
            escaped_label.replace(pos, 1, "\\\"");
            pos = escaped_label.find('"', pos + 2);
        }
        oss << "    " << safe_id << " [label=\"" << escaped_label << "\"";
        if (node.type == "router") {
            oss << ", color=blue, style=filled, fillcolor=lightblue";
        } else if (node.type == "server") {
            oss << ", color=green, style=filled, fillcolor=lightgreen";
        } else if (node.type == "printer") {
            oss << ", color=orange, style=filled, fillcolor=lightyellow";
        }
        oss << "];\n";
    }

    oss << "\n";
    std::string gw_id = gateway_ip_.empty() ? "router" : gateway_ip_;
    std::replace(gw_id.begin(), gw_id.end(), '.', '_');
    oss << "    internet -> " << gw_id << ";\n";

    for (const auto& node : nodes_) {
        if (node.parent.empty()) {
            std::string safe_id = node.id;
            std::replace(safe_id.begin(), safe_id.end(), '.', '_');
            oss << "    " << gw_id << " -> " << safe_id << ";\n";
        }
    }

    oss << "}\n";
    return oss.str();
}

bool Topology::ExportDOT(const std::string& path) const {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            core::Logger::Instance().Error("Failed to write DOT file: " + path);
            return false;
        }
        file << GenerateDOT();
        core::Logger::Instance().Info("Topology exported to DOT: " + path);
        return true;
    } catch (const std::exception& e) {
        core::Logger::Instance().Error("DOT export error: " + std::string(e.what()));
        return false;
    }
}

void Topology::Clear() {
    nodes_.clear();
    gateway_ip_.clear();
    internet_connected_ = false;
}

TopologyNode Topology::BuildNode(const Device& device) const {
    TopologyNode node;
    node.id = device.IP();
    node.label = device.Hostname().empty() ? device.IP() : device.Hostname();
    node.type = DetermineDeviceType(device);
    return node;
}

std::string Topology::DetermineDeviceType(const Device& device) const {
    if (device.IP() == gateway_ip_) return "router";

    for (const auto& port : device.OpenPorts()) {
        if (port.service == "HTTP" || port.service == "HTTPS") {
            return "server";
        }
        if (port.service == "FTP" || port.service == "SSH") {
            return "server";
        }
    }

    if (device.Vendor() == "Apple" || device.Vendor() == "Samsung" ||
        device.Vendor() == "Google") {
        return "mobile";
    }

    for (const auto& p : device.OpenPorts()) {
        if (p.service == "SMB" || p.service == "NetBIOS") {
            return "desktop";
        }
    }

    return "device";
}

void Topology::BuildTree() {
}

} // namespace discovery
} // namespace netscope
