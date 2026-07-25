#ifndef NETSCOPE_DISCOVERY_TOPOLOGY_H
#define NETSCOPE_DISCOVERY_TOPOLOGY_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "netscope/discovery/device.h"

namespace netscope {
namespace discovery {

struct TopologyNode {
    std::string id;
    std::string label;
    std::string type;
    std::vector<std::string> children;
    std::string parent;
};

class Topology {
public:
    Topology() = default;

    void AddDevice(const Device& device);
    void SetGateway(const std::string& ip);
    void SetInternet(bool connected);

    std::string GenerateASCII() const;
    std::string GenerateDOT() const;
    bool ExportDOT(const std::string& path) const;

    const std::vector<TopologyNode>& Nodes() const { return nodes_; }
    void Clear();

    Topology(const Topology&) = delete;
    Topology& operator=(const Topology&) = delete;

private:
    TopologyNode BuildNode(const Device& device) const;
    std::string DetermineDeviceType(const Device& device) const;
    void BuildTree();

    std::vector<TopologyNode> nodes_;
    std::string gateway_ip_;
    bool internet_connected_{false};
};

} // namespace discovery
} // namespace netscope

#endif
