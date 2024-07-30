set (EXAMPLE_BINARIES
    ec_pp_udp_demo
    ec_pv_udp_demo
)

include_directories (
    ../real-time-edge-servo/include
)

foreach (ex ${EXAMPLE_BINARIES})
    add_executable(${ex} ${ex}.c)
    target_link_libraries(${ex} PUBLIC ${TARGET_LIBS})
endforeach()
