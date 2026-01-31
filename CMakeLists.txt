cmake_minimum_required(VERSION 3.16)
project(goodbyedpi-linux VERSION 0.2.3 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_SYSTEMD "Enable systemd integration" ON)
option(ENABLE_TESTING "Enable unit tests" OFF)
option(ENABLE_DEBUG "Enable debug features" OFF)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -D_FORTIFY_SOURCE=2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")

# Debug flags
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
endif()

# Security flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE -pie")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Wformat-security")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-z,relro,-z,now")

# Find required packages
find_package(PkgConfig REQUIRED)

# Netfilter Queue
pkg_check_modules(NETFILTER_QUEUE REQUIRED libnetfilter_queue>=1.0)
if(NETFILTER_QUEUE_FOUND)
    message(STATUS "Found libnetfilter_queue ${NETFILTER_QUEUE_VERSION}")
endif()

pkg_check_modules(LIBMNL REQUIRED libmnl>=1.0)
if(LIBMNL_FOUND)
    message(STATUS "Found libmnl ${LIBMNL_VERSION}")
endif()

# Systemd (optional)
if(ENABLE_SYSTEMD)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SYSTEMD libsystemd>=209)
    if(SYSTEMD_FOUND)
        message(STATUS "Found libsystemd ${SYSTEMD_VERSION}")
        add_definitions(-DENABLE_SYSTEMD)
    else()
        message(STATUS "libsystemd not found - systemd integration disabled")
    endif()
endif()

# Threading
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/include)

# Source files
set(CORE_SOURCES
    src/main.c
    src/core/config.c
    src/core/packet_processor.c
    src/core/logging.c
)

set(CAPTURE_SOURCES
    src/capture/raw_socket.c
    src/capture/packet_utils.c
    src/capture/netfilter_capture.c
)

set(EVASION_SOURCES
    src/evasion/fragmentation.c
    src/evasion/header_mangle.c
    src/evasion/fake_packets.c
    src/evasion/sni_extractor.c
    src/evasion/turkey_specific.c
)

set(TRACKING_SOURCES
    src/tracking/conntrack.c
    src/tracking/dns_tracker.c
    src/tracking/ttl_tracker.c
)

set(SERVICE_SOURCES
    src/service/privileges.c
    src/service/daemon_funcs.c
)

if(SYSTEMD_FOUND)
    list(APPEND SERVICE_SOURCES src/service/systemd.c)
endif()

set(UTILS_SOURCES
    src/utils/hash.c
    src/utils/string_utils.c
    src/utils/net_utils.c
)

# All sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${CAPTURE_SOURCES}
    ${EVASION_SOURCES}
    ${TRACKING_SOURCES}
    ${SERVICE_SOURCES}
    ${UTILS_SOURCES}
)

# Verify all source files exist (with warnings for missing files)
set(MISSING_FILES "")
foreach(src_file ${ALL_SOURCES})
    if(NOT EXISTS ${CMAKE_SOURCE_DIR}/${src_file})
        list(APPEND MISSING_FILES ${src_file})
    endif()
endforeach()

# Report missing files
if(MISSING_FILES)
    message(WARNING "The following source files are missing:")
    foreach(missing_file ${MISSING_FILES})
        message(WARNING "  - ${missing_file}")
    endforeach()
    message(WARNING "Build may fail if these files are required.")
    message(WARNING "Consider creating stub implementations or removing them from the source list.")
endif()

# Create executable
add_executable(goodbyedpi ${ALL_SOURCES})

# Link libraries
target_link_libraries(goodbyedpi
    ${CMAKE_THREAD_LIBS_INIT}
    ${NETFILTER_QUEUE_LIBRARIES}
    ${LIBMNL_LIBRARIES}
)

if(SYSTEMD_FOUND)
    target_link_libraries(goodbyedpi ${SYSTEMD_LIBRARIES})
endif()

# Include directories for targets
target_include_directories(goodbyedpi PRIVATE
    ${NETFILTER_QUEUE_INCLUDE_DIRS}
    ${LIBMNL_INCLUDE_DIRS}
)

if(SYSTEMD_FOUND)
    target_include_directories(goodbyedpi PRIVATE ${SYSTEMD_INCLUDE_DIRS})
endif()

# Installation
install(TARGETS goodbyedpi
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# Install configuration files
install(FILES config/goodbyedpi.conf.example
    DESTINATION etc/goodbyedpi
    RENAME goodbyedpi.conf
    COMPONENT Config
    OPTIONAL
)

install(FILES config/blacklist.txt.example
    DESTINATION etc/goodbyedpi
    RENAME blacklist.txt
    COMPONENT Config
    OPTIONAL
)

# Install systemd service file
if(SYSTEMD_FOUND)
    # Check if service file template exists
    if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/goodbyedpi.service.in)
        configure_file(
            ${CMAKE_SOURCE_DIR}/scripts/goodbyedpi.service.in
            ${CMAKE_BINARY_DIR}/goodbyedpi.service
            @ONLY
        )
        install(FILES ${CMAKE_BINARY_DIR}/goodbyedpi.service
            DESTINATION lib/systemd/system
            COMPONENT Systemd
        )
    else()
        message(WARNING "systemd service template not found: scripts/goodbyedpi.service.in")
    endif()
endif()

# Install scripts
if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/install.sh)
    install(PROGRAMS scripts/install.sh
        DESTINATION libexec/goodbyedpi
        COMPONENT Scripts
    )
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/uninstall.sh)
    install(PROGRAMS scripts/uninstall.sh
        DESTINATION libexec/goodbyedpi
        COMPONENT Scripts
    )
endif()

# Testing
if(ENABLE_TESTING)
    enable_testing()
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    else()
        message(WARNING "Testing enabled but tests directory not found")
    endif()
endif()

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "goodbyedpi-linux")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Linux DPI bypass and circumvention utility")
set(CPACK_PACKAGE_VENDOR "GoodbyeDPI Community")
set(CPACK_PACKAGE_CONTACT "https://github.com/goodbyedpi-linux")

if(EXISTS ${CMAKE_SOURCE_DIR}/LICENSE)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/README.md)
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
endif()

# DEB package specific settings
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libnetfilter-queue1 (>= 1.0), libmnl0 (>= 1.0), iptables")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

# RPM package specific settings
set(CPACK_RPM_PACKAGE_REQUIRES "libnetfilter_queue >= 1.0, libmnl >= 1.0, iptables")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Internet")
set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "GoodbyeDPI Linux ${PROJECT_VERSION} Configuration Summary:")
message(STATUS "  Build type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "  C compiler:          ${CMAKE_C_COMPILER}")
message(STATUS "  Install prefix:      ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Systemd integration: ${ENABLE_SYSTEMD}")
message(STATUS "  Debug mode:          ${ENABLE_DEBUG}")
message(STATUS "  Testing enabled:     ${ENABLE_TESTING}")
message(STATUS "")
