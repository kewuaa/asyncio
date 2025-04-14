set(
    tests
    test_task
    test_sleep
    test_timer
    test_thread
)
foreach (
    test IN LISTS tests
)
    message(STATUS "add test for ${test}")
    add_executable(${test})
    target_sources(${test} PRIVATE ${test}.cpp)
    target_link_libraries(
        ${test}
        PRIVATE
            Boost::ut
            ${PROJECT_NAME}
    )
    add_test(NAME ${test} COMMAND $<TARGET_FILE:${test}>)
endforeach()
