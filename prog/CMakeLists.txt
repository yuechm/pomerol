include_directories (../pomerol)

# tclap
find_package (TCLAP)
message(STATUS "TCLAP includes: " ${TCLAP_INCLUDE_DIR} )
include_directories(${TCLAP_INCLUDE_DIR})

link_libraries(
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    pomerol
)

set (progs 
anderson
)

if (CXX11)
list(APPEND progs hubbard2d)
endif(CXX11)

foreach (prog ${progs})
    set(prog_src ${prog}.cpp)
    add_executable(${prog}.pomerol ${prog_src})
    install ( TARGETS ${prog}.pomerol DESTINATION bin )
endforeach(prog)

