# AppleMakeBundle.cmake
# Provides a function to create a macOS bundle with a custom Info.plist and icon
#
# Usage:
#   apple_make_bundle(target org_id plist [icon])
# plist should be the path to an Info.plist file
# icon should be the path to an .icns file
#
function(apple_make_bundle target org_id plist)
    # Parse optional arguments (icon)
    set(icon "")  # Default to empty
    if(ARGN)
        list(GET ARGN 0 icon)
    endif()

    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${plist}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.${org_id}.${target}"
        MACOSX_BUNDLE_BUNDLE_NAME "${target}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.${org_id}.${target}"
    )

    # If an icon is specified, set MACOSX_BUNDLE_ICON_FILE (just the filename)
    if (icon)
        get_filename_component(icon_filename "${icon}" NAME)
        set_target_properties(${target} PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "${icon_filename}"
        )

        # Ensure the .icns file gets copied into the Resources folder
        target_sources(${target} PRIVATE ${icon})
        set_source_files_properties(${icon} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
    endif()
endfunction()
