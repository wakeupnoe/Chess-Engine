add_executable(Chess-EngineTests test_core.cpp)

target_link_libraries(Chess-EngineTests PRIVATE EngineLib Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(Chess-EngineTests)