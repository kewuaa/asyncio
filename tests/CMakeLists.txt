CPMAddPackage(
    GIT_REPOSITORY "https://github.com/boost-ext/ut.git"
    GIT_TAG v2.3.1
    OPTIONS "BOOST_UT_DISABLE_MODULE ON"
)

CPMAddPackage(
    tiny_thread_pool
    GIT_REPOSITORY "https://github.com/kewuaa/tiny_thread_pool.git"
    GIT_TAG master
)

set(
    tests
    test_task
    test_sleep
    test_timer
    test_thread
    test_locks
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
    if (test STREQUAL test_thread)
        target_link_libraries(${test} PRIVATE tiny_thread_pool)
    endif()
    add_test(NAME ${test} COMMAND $<TARGET_FILE:${test}>)
endforeach()
