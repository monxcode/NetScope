#include <gtest/gtest.h>
#include "netscope/export/exporter.h"
#include "netscope/discovery/device.h"
#include "netscope/core/filesystem.h"

#include <fstream>
#include <sstream>

using namespace netscope::export_;
using namespace netscope::discovery;

class ExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_path_ = fs::temp_directory_path() / "netscope_test_export.json";
        csv_path_ = fs::temp_directory_path() / "netscope_test_export.csv";
        txt_path_ = fs::temp_directory_path() / "netscope_test_export.txt";

        Device d1;
        d1.SetIP("192.168.1.10");
        d1.SetMAC("00:11:22:33:44:55");
        d1.SetHostname("test-device");
        d1.SetVendor("TestVendor");
        d1.SetOnline(true);
        d1.SetResponseTimeMs(5);
        d1.SetTTL(64);
        d1.SetOS({"Linux", 0.75, 64});
        devices_.push_back(d1);

        Device d2;
        d2.SetIP("192.168.1.20");
        d2.SetMAC("AA:BB:CC:DD:EE:FF");
        d2.SetHostname("windows-pc");
        d2.SetOnline(true);
        d2.SetTTL(128);
        d2.SetOS({"Windows", 0.80, 128});
        devices_.push_back(d2);
    }

    void TearDown() override {
        if (fs::exists(test_path_)) fs::remove(test_path_);
        if (fs::exists(csv_path_)) fs::remove(csv_path_);
        if (fs::exists(txt_path_)) fs::remove(txt_path_);
    }

    fs::path test_path_;
    fs::path csv_path_;
    fs::path txt_path_;
    std::vector<Device> devices_;
};

TEST_F(ExporterTest, FormatToString) {
    EXPECT_EQ(Exporter::FormatToString(ExportFormat::JSON), "json");
    EXPECT_EQ(Exporter::FormatToString(ExportFormat::CSV), "csv");
    EXPECT_EQ(Exporter::FormatToString(ExportFormat::TXT), "txt");
    EXPECT_EQ(Exporter::FormatToString(ExportFormat::DOT), "dot");
}

TEST_F(ExporterTest, StringToFormat) {
    EXPECT_EQ(Exporter::StringToFormat("json"), ExportFormat::JSON);
    EXPECT_EQ(Exporter::StringToFormat("csv"), ExportFormat::CSV);
    EXPECT_EQ(Exporter::StringToFormat("txt"), ExportFormat::TXT);
    EXPECT_EQ(Exporter::StringToFormat("dot"), ExportFormat::DOT);
    EXPECT_EQ(Exporter::StringToFormat("unknown"), ExportFormat::JSON);
}

TEST_F(ExporterTest, ExportJSON) {
    Exporter exp;
    bool ok = exp.ExportJSON(devices_, test_path_);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(fs::exists(test_path_));

    std::ifstream file(test_path_);
    ASSERT_TRUE(file.is_open());
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    EXPECT_NE(content.find("NetScope"), std::string::npos);
    EXPECT_NE(content.find("192.168.1.10"), std::string::npos);
}

TEST_F(ExporterTest, ExportCSV) {
    Exporter exp;
    bool ok = exp.ExportCSV(devices_, csv_path_);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(fs::exists(csv_path_));

    std::ifstream file(csv_path_);
    ASSERT_TRUE(file.is_open());
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    EXPECT_NE(content.find("IP,MAC"), std::string::npos);
    EXPECT_NE(content.find("192.168.1.10"), std::string::npos);
}

TEST_F(ExporterTest, ExportTXT) {
    Exporter exp;
    bool ok = exp.ExportTXT(devices_, txt_path_);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(fs::exists(txt_path_));

    std::ifstream file(txt_path_);
    ASSERT_TRUE(file.is_open());
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    EXPECT_NE(content.find("NetScope Network Scan Report"), std::string::npos);
    EXPECT_NE(content.find("192.168.1.10"), std::string::npos);
}

TEST_F(ExporterTest, ExportAll) {
    Exporter exp;
    bool ok = exp.Export(devices_, test_path_, ExportFormat::JSON);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(fs::exists(test_path_));
}

TEST_F(ExporterTest, ExportEmpty) {
    Exporter exp;
    bool ok = exp.ExportJSON({}, test_path_);
    EXPECT_TRUE(ok);
}
