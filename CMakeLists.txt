project(RealTimeEdgeServo C)
cmake_minimum_required(VERSION 3.2)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -Wall -g -ggdb")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -Wall")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(NSERVO_INC_PATH "./real-time-edge-servo/include")
set(NSERVO_TOOL_PATH "./real-time-edge-servo/tool")
set(NSERVO_EXAMPLE_PATH "./real-time-edge-servo/example")


find_package(EtherCAT REQUIRED)
find_package(LibXml2 REQUIRED)

set(TARGET_LIBS EtherLab::ethercat nservo pthread)

list(APPEND BUILT_IN_EXAMPLE_SOURCES
    Cyclic_Position_Mode_Test_2HSS458-EC
    Profile_Velocty_Mode_Test_2HSS458-EC
    Profile_Position_Mode_Test_2HSS458-EC)

add_subdirectory (example)
# built-in examples
add_executable(nser_console_tool ${NSERVO_TOOL_PATH}/console_tool.c)
add_executable(nservo_run ${NSERVO_TOOL_PATH}/nservo_run.c)
add_executable(nservo_client ${NSERVO_TOOL_PATH}/nservo_client.c)

foreach (ex ${BUILT_IN_EXAMPLE_SOURCES})
    add_executable(${ex} ${NSERVO_EXAMPLE_PATH}/${ex}.c)
    target_link_libraries(${ex} PUBLIC ${TARGET_LIBS})
endforeach()


# build nservo static library
add_library(nservo STATIC 
    ./real-time-edge-servo/ds402.c
    ./real-time-edge-servo/nser_xml_parse.c
    ./real-time-edge-servo/nservo_config.c
    ./real-time-edge-servo/nservo.c)

include_directories (
    ${NSERVO_INC_PATH}
)

# link libraries
target_link_libraries(nservo PUBLIC LibXml2::LibXml2 EtherLab::ethercat)
target_link_libraries(nser_console_tool PUBLIC ${TARGET_LIBS})
target_link_libraries(nservo_run PUBLIC ${TARGET_LIBS} m)
target_link_libraries(nservo_client PUBLIC ${TARGET_LIBS} m)
