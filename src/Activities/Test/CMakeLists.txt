
# Automatically use the top-level project name if available
if (NOT DEFINED PARENT_PROJECT_NAME)
    set(PARENT_PROJECT_NAME ${CMAKE_PROJECT_NAME})
endif()

project(TestPlugin VERSION 1.0)

set(TEST_PLUGIN_SOURCES
    ${CMAKE_SOURCE_DIR}/src/Activities/Test/TestPlugin.cpp
)
add_library(${PROJECT_NAME} MODULE ${TEST_PLUGIN_SOURCES})

# add ${CMAKE_SOURCE_DIR}/src as an include directory
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)

# echo the sources
message(INFO, "TestPlugin sources: ${TEST_PLUGIN_SOURCES}")

# Update the global PLUGINS_LIST with this new plugin
get_property(CURRENT_PLUGINS GLOBAL PROPERTY PLUGINS_LIST)
set_property(GLOBAL PROPERTY PLUGINS_LIST "${CURRENT_PLUGINS};${PROJECT_NAME}")

# if PLUGINS_DIR is not defined, fatal
if (NOT DEFINED PLUGINS_DIR)
    message(FATAL_ERROR "PLUGINS_DIR is not defined")
endif()

if (APPLE)
    # enable ARC
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC TRUE
    )
endif()

# set the output directory for the plugin manually to suppress cmake's default
# behavior of putting it in a build type subdirectory
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGINS_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PLUGINS_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PLUGINS_DIR}
    
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${PLUGINS_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${PLUGINS_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${PLUGINS_DIR}
    
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGINS_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PLUGINS_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PLUGINS_DIR}

    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PLUGINS_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PLUGINS_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PLUGINS_DIR}
)

# Conditionally add dependency on the parent project if it’s defined
if (TARGET ${PARENT_PROJECT_NAME})
    add_dependencies(${PARENT_PROJECT_NAME} ${PROJECT_NAME})
endif()

# Configure signing with entitlements for the plugin
if (APPLE)
    set(ENTITLEMENTS_PATH "${CMAKE_SOURCE_DIR}/src/Lab.entitlements")
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        //XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "" # Ensure it's empty to let Xcode prompt you
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${ENTITLEMENTS_PATH}"
        # Inherit bundle identifier prefix from main app
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.planetix.raven.testplugin"
        # Enable hardened runtime like the main app
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    )
endif()

GroupSourcesByFolder(TestPlugin)
