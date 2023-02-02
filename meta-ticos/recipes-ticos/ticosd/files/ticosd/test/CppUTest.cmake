cmake_minimum_required(VERSION 3.16)

pkg_check_modules(CPPUTEST REQUIRED cpputest)
pkg_check_modules(SDBUS REQUIRED libsystemd)

add_library(cpputest_runner AllTests.cpp)
target_link_libraries(cpputest_runner PUBLIC ${CPPUTEST_LIBRARIES})


set(SANITIZE_FLAGS
    -fsanitize=address
    -fsanitize=undefined
    -fno-sanitize-recover=all
    )

function(add_cpputest_target NAME)
    add_executable(${NAME} ${ARGN})
    target_link_libraries(${NAME}
        # cpputest_runner has to come before CPPUTEST_LIBRARIES!!!
        cpputest_runner
        ${CPPUTEST_LIBRARIES}
        )
    target_compile_options(${NAME} PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-unused-private-field
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -DCPPUTEST_USE_EXTENSIONS=1
        -DCPPUTEST_USE_MEM_LEAK_DETECTION=0  # conflicts with ASAN
        -DCPPUTEST_SANITIZE_ADDRESS=1
        -DTICOS_UNITTEST
        -O0
        -g
        ${SANITIZE_FLAGS}
        )
    target_link_options(${NAME} PRIVATE -g -O0 ${SANITIZE_FLAGS})
    add_test(NAME ${NAME}_ctest COMMAND ${NAME})
endfunction()
