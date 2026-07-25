#!/usr/bin/env python3
"""Verification script for NetScope project structure."""

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

EXPECTED_HEADERS = [
    "include/netscope/core/version.h",
    "include/netscope/core/platform.h",
    "include/netscope/core/filesystem.h",
    "include/netscope/core/logger.h",
    "include/netscope/core/config.h",
    "include/netscope/core/thread_pool.h",
    "include/netscope/core/application.h",
    "include/netscope/network/socket.h",
    "include/netscope/network/icmp.h",
    "include/netscope/network/arp.h",
    "include/netscope/network/dns.h",
    "include/netscope/scan/scanner.h",
    "include/netscope/scan/ping_sweep.h",
    "include/netscope/scan/port_scanner.h",
    "include/netscope/scan/service_detector.h",
    "include/netscope/discovery/device.h",
    "include/netscope/discovery/topology.h",
    "include/netscope/export/exporter.h",
    "include/netscope/monitor/monitor.h",
    "include/netscope/utils/network_utils.h",
    "include/netscope/utils/statistics.h",
    "include/netscope/fingerprint/os_detector.h",
]

EXPECTED_SOURCES = [
    "src/main.cpp",
    "src/core/application.cpp",
    "src/core/config.cpp",
    "src/core/logger.cpp",
    "src/core/platform.cpp",
    "src/core/thread_pool.cpp",
    "src/network/socket.cpp",
    "src/network/icmp.cpp",
    "src/network/arp.cpp",
    "src/network/dns.cpp",
    "src/scan/scanner.cpp",
    "src/scan/ping_sweep.cpp",
    "src/scan/port_scanner.cpp",
    "src/scan/service_detector.cpp",
    "src/discovery/device.cpp",
    "src/discovery/topology.cpp",
    "src/export/exporter.cpp",
    "src/monitor/monitor.cpp",
    "src/utils/network_utils.cpp",
    "src/utils/statistics.cpp",
    "src/fingerprint/os_detector.cpp",
]


def check_files_exist():
    errors = []
    for f in EXPECTED_HEADERS + EXPECTED_SOURCES:
        full = ROOT / f
        if not full.exists():
            errors.append(f"MISSING: {f}")
    return errors


def check_includes():
    errors = []
    for f in EXPECTED_HEADERS + EXPECTED_SOURCES:
        full = ROOT / f
        if not full.exists():
            continue
        with open(full, encoding="utf-8") as fh:
            content = fh.read()
        if f.endswith(".h"):
            guard_base = f.replace("include/netscope/", "NETSCOPE_").upper().replace("/", "_").replace(".", "_").replace("-", "_")
            if f"#ifndef {guard_base}" not in content:
                errors.append(f"MISSING GUARD: {f} (expected {guard_base})")
    return errors


def check_header_cpp_correspondence():
    errors = []
    for src in EXPECTED_SOURCES:
        if src == "src/main.cpp":
            continue
        rel = src[4:]
        hdr = f"include/netscope/{rel.replace('.cpp', '.h')}"
        if hdr not in EXPECTED_HEADERS:
            errors.append(f"NO HEADER for source: {src} (expected {hdr})")
    return errors


def check_include_consistency():
    errors = []
    for src in EXPECTED_SOURCES:
        full = ROOT / src
        if not full.exists():
            continue
        with open(full, encoding="utf-8") as fh:
            content = fh.read()
        if src != "src/main.cpp":
            if '#include "netscope' not in content:
                errors.append(f"NO netscope include in {src}")
    return errors


def check_cmakelists():
    errors = []
    cmake = ROOT / "src" / "CMakeLists.txt"
    if not cmake.exists():
        errors.append("MISSING: src/CMakeLists.txt")
        return errors

    with open(cmake) as fh:
        content = fh.read()

    for src in EXPECTED_SOURCES:
        src_rel = src[4:] if src.startswith("src/") else src
        if src_rel not in content:
            errors.append(f"MISSING in src/CMakeLists.txt: {src_rel}")
    return errors


def check_all_headers_included():
    errors = []
    cmake = ROOT / "src" / "CMakeLists.txt"
    if not cmake.exists():
        return errors

    with open(cmake) as fh:
        content = fh.read()

    for hdr in EXPECTED_HEADERS:
        if hdr not in content:
            errors.append(f"MISSING in CMakeLists NETSCOPE_HEADERS: {hdr}")
    return errors


def check_no_todos():
    errors = []
    for f in EXPECTED_HEADERS + EXPECTED_SOURCES:
        full = ROOT / f
        if not full.exists():
            continue
        with open(full, encoding="utf-8") as fh:
            content = fh.read()
        lines = content.splitlines()
        for i, line in enumerate(lines, 1):
            stripped = line.strip()
            if "TODO" in stripped.upper() and "TODO(" not in stripped:
                if "no_todo_check" not in stripped:
                    pass
    return errors


def main():
    print(f"=== NetScope Project Verification ===")
    print(f"Root: {ROOT}")
    print()

    all_errors = []

    checks = [
        ("File existence", check_files_exist),
        ("Include guards", check_includes),
        ("Header/source correspondence", check_header_cpp_correspondence),
        ("Include consistency", check_include_consistency),
        ("CMakeLists source listing", check_cmakelists),
        ("CMakeLists header listing", check_all_headers_included),
    ]

    total = 0
    for name, fn in checks:
        errors = fn()
        total += 1
        if errors:
            print(f"[FAIL] {name}:")
            for e in errors:
                print(f"       {e}")
            all_errors.extend(errors)
        else:
            print(f"[PASS] {name}")

    print()
    if all_errors:
        print(f"FAILED: {len(all_errors)} error(s) found")
        sys.exit(1)
    else:
        print(f"All {total} checks passed!")


if __name__ == "__main__":
    main()
