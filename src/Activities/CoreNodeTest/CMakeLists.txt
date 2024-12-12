
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};CoreNodeTest")

set(CORENODETEST_SRCS
    CoreDiagram.cpp CoreDiagram.hpp
    CoreLibrary.cpp CoreLibrary.hpp
    CoreNode.cpp CoreNode.hpp
    CoreNodePort.cpp CoreNodePort.hpp
    CoreNodeTestActivity.cpp CoreNodeTestActivity.hpp
    FileDialog.cpp FileDialog.hpp
    GainNode.cpp GainNode.hpp
    Notifier.cpp Notifier.hpp
    TestNode.cpp TestNode.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${CORENODETEST_SRCS})
