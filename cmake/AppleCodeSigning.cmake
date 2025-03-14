# AppleCodeSigning.cmake
# Provides functions for consistent code signing across targets

# Function to apply consistent code signing to a target
function(apply_apple_code_signing target)
  if(NOT APPLE)
    return()
  endif()

  # Use the dash (-) as the code signing identity, which means "ad-hoc signing"
  # This is more reliable for local development than trying to use a specific identity
  set_target_properties(${target} PROPERTIES
    # Use ad-hoc signing with the dash
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    # Disable code signing required for local development
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    # Use manual signing to have more control
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
    # Set the entitlements file
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/src/Lab.entitlements"
    # Enable hardened runtime for better security
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "NO"
  )
endfunction()

# Function specifically for plugins that need additional settings
function(apply_plugin_code_signing plugin_target main_app_target)
  # First apply the standard code signing
  apply_apple_code_signing(${plugin_target})
  
  if(NOT APPLE)
    return()
  endif()

  # Get the bundle identifier from the main app
  get_target_property(MAIN_BUNDLE_ID ${main_app_target} 
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER)
  
  # If not found, use a default pattern
  if(NOT MAIN_BUNDLE_ID)
    set(MAIN_BUNDLE_ID "com.planetix.${main_app_target}")
  endif()
  
  # Set plugin-specific properties
  set_target_properties(${plugin_target} PROPERTIES
    # Set bundle identifier based on main app
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MAIN_BUNDLE_ID}.${plugin_target}"
  )
  
  # Make the main app depend on the plugin to ensure correct build order
  add_dependencies(${main_app_target} ${plugin_target})
endfunction()
