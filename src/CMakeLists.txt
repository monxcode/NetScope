set(NETSCOPE_SOURCES
    main.cpp
    core/application.cpp
    core/config.cpp
    core/logger.cpp
    core/platform.cpp
    core/thread_pool.cpp
    network/socket.cpp
    network/icmp.cpp
    network/arp.cpp
    network/dns.cpp
    scan/scanner.cpp
    scan/ping_sweep.cpp
    scan/port_scanner.cpp
    scan/service_detector.cpp
    discovery/device.cpp
    discovery/topology.cpp
    export/exporter.cpp
    monitor/monitor.cpp
    utils/network_utils.cpp
    utils/statistics.cpp
    fingerprint/os_detector.cpp
)

set(NETSCOPE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/netscope/core/version.h
    ${CMAKE_SOURCE_DIR}/include/netscope/core/platform.h
    ${CMAKE_SOURCE_DIR}/include/netscope/core/filesystem.h
    ${CMAKE_SOURCE_DIR}/include/netscope/core/logger.h
    ${CMAKE_SOURCE_DIR}/include/netscope/core/config.h
    ${CMAKE_SOURCE_DIR}/include/netscope/core/thread_pool.h
    ${CMAKE_SOURCE_DIR}/include/netscope/core/application.h
    ${CMAKE_SOURCE_DIR}/include/netscope/network/socket.h
    ${CMAKE_SOURCE_DIR}/include/netscope/network/icmp.h
    ${CMAKE_SOURCE_DIR}/include/netscope/network/arp.h
    ${CMAKE_SOURCE_DIR}/include/netscope/network/dns.h
    ${CMAKE_SOURCE_DIR}/include/netscope/scan/scanner.h
    ${CMAKE_SOURCE_DIR}/include/netscope/scan/ping_sweep.h
    ${CMAKE_SOURCE_DIR}/include/netscope/scan/port_scanner.h
    ${CMAKE_SOURCE_DIR}/include/netscope/scan/service_detector.h
    ${CMAKE_SOURCE_DIR}/include/netscope/discovery/device.h
    ${CMAKE_SOURCE_DIR}/include/netscope/discovery/topology.h
    ${CMAKE_SOURCE_DIR}/include/netscope/export/exporter.h
    ${CMAKE_SOURCE_DIR}/include/netscope/monitor/monitor.h
    ${CMAKE_SOURCE_DIR}/include/netscope/utils/network_utils.h
    ${CMAKE_SOURCE_DIR}/include/netscope/utils/statistics.h
    ${CMAKE_SOURCE_DIR}/include/netscope/fingerprint/os_detector.h
)

add_executable(netscope ${NETSCOPE_SOURCES} ${NETSCOPE_HEADERS})

target_include_directories(netscope PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(netscope PRIVATE
    nlohmann_json::nlohmann_json
    ${PLATFORM_LIBS}
)

target_include_directories(netscope SYSTEM PRIVATE
    ${nlohmann_json_SOURCE_DIR}/include
)

set_target_properties(netscope PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
