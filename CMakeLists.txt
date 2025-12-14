cmake_minimum_required(VERSION 3.15)
project(AZTest VERSION 2.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# AZ TEST FRAMEWORK LIBRARY
# ============================================================================

set(AZTEST_HEADERS
    include/AZTest/AZTest.h
    include/AZTest/Core/TestResult.h
    include/AZTest/Core/TestFixture.h
    include/AZTest/Core/TestRegistry.h
    include/AZTest/Reporters/IReporter.h
    include/AZTest/Reporters/ConsoleReporter.h
    include/AZTest/Reporters/XMLReporter.h
    include/AZTest/Reporters/JSONReporter.h
    include/AZTest/Reporters/CallbackReporter.h
    include/AZTest/Utils/Benchmark.h
    include/AZTest/UEIntegration.h
)

set(AZTEST_SOURCES
    src/Core/TestRegistry.cpp
    src/Reporters/ConsoleReporter.cpp
    src/Reporters/XMLReporter.cpp
    src/Reporters/JSONReporter.cpp
)

# Create library
add_library(AZTest STATIC ${AZTEST_SOURCES} ${AZTEST_HEADERS})

# Include directories
target_include_directories(AZTest PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Provide a ready-made main() for quick test executables (like gtest_main)
add_library(AZTestMain STATIC src/AZTestMain.cpp)
target_link_libraries(AZTestMain PUBLIC AZTest)
target_include_directories(AZTestMain PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ============================================================================
# EXAMPLES
# ============================================================================

option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    # Basic Example
    add_executable(BasicExample examples/BasicExample.cpp)
    target_link_libraries(BasicExample PRIVATE AZTest)

    # Advanced Example
    add_executable(AdvancedExample examples/AdvancedExample.cpp)
    target_link_libraries(AdvancedExample PRIVATE AZTest)

    # Fixture Example
    add_executable(FixtureExample examples/FixtureExample.cpp)
    target_link_libraries(FixtureExample PRIVATE AZTest)

    enable_testing()
    add_test(NAME BasicExample COMMAND BasicExample)
    add_test(NAME AdvancedExample COMMAND AdvancedExample)
    add_test(NAME FixtureExample COMMAND FixtureExample)

    # Features Example
    add_executable(FeaturesExample examples/FeaturesExample.cpp)
    target_link_libraries(FeaturesExample PRIVATE AZTest)
    add_test(NAME FeaturesExample COMMAND FeaturesExample)
endif()

# ============================================================================
# INSTALLATION
# ============================================================================

include(GNUInstallDirs)

install(TARGETS AZTest
    EXPORT AZTestTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS AZTestMain
    EXPORT AZTestTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ============================================================================
# PACKAGE CONFIG
# ============================================================================

install(EXPORT AZTestTargets
    FILE AZTestTargets.cmake
    NAMESPACE AZTest::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AZTest
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AZTestConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/AZTestConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AZTest
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/AZTestConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/AZTestConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/AZTestConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AZTest
)

# ============================================================================
# SUMMARY
# ============================================================================

message(STATUS "")
message(STATUS "==========================================")
message(STATUS "  AZTest Framework v${PROJECT_VERSION}")
message(STATUS "==========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "==========================================")
message(STATUS "")
