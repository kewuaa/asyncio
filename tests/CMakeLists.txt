SET(
    tests
    test_task
    test_sleep
    test_timer
)
foreach (
    test IN LISTS tests
)
    MESSAGE(STATUS "add test for ${test}")
    ADD_EXECUTABLE(${test})
    TARGET_SOURCES(${test} PRIVATE ${test}.cpp)
    TARGET_LINK_LIBRARIES(
        ${test}
        PRIVATE
            Boost::ut
            ${PROJECT_NAME}
    )
    ADD_TEST(NAME ${test} COMMAND $<TARGET_FILE:${test}>)
endforeach()
