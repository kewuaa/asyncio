ADD_EXECUTABLE(test_task)
TARGET_SOURCES(test_task PRIVATE test_task.cpp)
TARGET_LINK_LIBRARIES(
    test_task
    PRIVATE
        Boost::ut
        ${PROJECT_NAME}
)

ADD_TEST(NAME test_task COMMAND $<TARGET_FILE:test_task>)
