cmake_minimum_required(VERSION 3.24)

project(UnitTests)


juce_add_console_app(UnitTestRunner
    VERSION 1.0.0
    PRODUCT_NAME "JUCE Unit Tests"
)

target_sources(UnitTestRunner PRIVATE
    UnitTestMain.cpp
    BasicTests.cpp
)

target_include_directories(UnitTestRunner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(UnitTestRunner PRIVATE
    juce::juce_core
    juce::juce_events
)