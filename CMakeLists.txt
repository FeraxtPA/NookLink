cmake_minimum_required(VERSION 3.25)

project(NookLink LANGUAGES C CXX)

include(CTest)
option(NOOKLINK_BUILD_APP "Build the desktop application" ON)

add_library(nooklink_core STATIC
    src/Core/appPaths.cpp
    src/Core/fileStorage.cpp
    src/Core/logging.cpp
    src/Domain/book.cpp
    src/Domain/bookManager.cpp
    src/Domain/date_utils.cpp
    src/Domain/searchFilter.cpp
    src/Domain/validation.cpp
)
target_compile_features(nooklink_core PUBLIC cxx_std_23)
target_include_directories(nooklink_core PUBLIC include src/Core src/Domain)
set_target_properties(nooklink_core PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(nooklink_core PRIVATE /utf-8)
endif()

if(BUILD_TESTING)
    add_executable(nooklink_tests tests/core_tests.cpp)
    target_link_libraries(nooklink_tests PRIVATE nooklink_core)
    if(MSVC)
        target_compile_options(nooklink_tests PRIVATE /utf-8)
    endif()
    foreach(test_name IN ITEMS roundtrip backup save_failure recovery
            invalid_library next_id lookup search validation app_paths)
        add_test(NAME ${test_name} COMMAND nooklink_tests ${test_name})
    endforeach()
    if(WIN32)
        add_test(NAME locked_target COMMAND nooklink_tests locked_target)
    endif()
endif()

if(NOOKLINK_BUILD_APP)
add_executable(NookLink
    include/tinyfiledialogs/tinyfiledialogs.c
    src/Core/NookLink.cpp
    src/Core/application.cpp
    src/Core/application_persistence.cpp
    src/Core/application_runtime.cpp
    src/Core/debugManager.cpp
    src/Core/inputHandler.cpp
    src/Domain/cameraHandler.cpp
    src/Domain/connectionManager.cpp
    src/Domain/graphLayout.cpp
    src/Domain/graphManager.cpp
    src/Domain/graphManager_physics.cpp
    src/Domain/graphManager_picking.cpp
    src/Domain/graphManager_rendering.cpp
    src/Domain/graphManager_selection.cpp
    src/Domain/nodeRenderer.cpp
    src/Services/goodreadsCsvImporter.cpp
    src/Services/googleBooksClient.cpp
    src/UI/button.cpp
    src/UI/calendarWidget.cpp
    src/UI/checkbox.cpp
    src/UI/dropdown.cpp
    src/UI/flexLayout.cpp
    src/UI/label.cpp
    src/UI/panel.cpp
    src/UI/slider.cpp
    src/UI/textBox.cpp
    src/UI/textInput.cpp
    src/UIManager/uiManager.cpp
    src/UIManager/uiManager_build.cpp
    src/UIManager/uiManager_build_filter_toolbar.cpp
    src/UIManager/uiManager_build_panels.cpp
    src/UIManager/uiManager_interaction.cpp
    src/UIManager/uiManager_overlays.cpp
    src/UIManager/uiManager_render.cpp
    src/UIManager/uiManager_state.cpp
    src/colors.cpp
    src/iconRenderer.cpp
    src/textRenderer.cpp
)

if(WIN32)
    target_sources(NookLink PRIVATE Resource.rc)
endif()

target_compile_features(NookLink PRIVATE cxx_std_23)
set_target_properties(NookLink PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(NookLink PRIVATE
    include
    src
    src/Core
    src/Domain
    src/Services
    src/UI
    src/UIManager
)

find_package(CURL REQUIRED)
find_package(raylib CONFIG QUIET)
if(TARGET raylib)
    set(NOOKLINK_RAYLIB_TARGET raylib)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)
    set(NOOKLINK_RAYLIB_TARGET PkgConfig::RAYLIB)
endif()

target_link_libraries(NookLink PRIVATE nooklink_core CURL::libcurl ${NOOKLINK_RAYLIB_TARGET})

if(MSVC)
    target_compile_options(NookLink PRIVATE /utf-8)
endif()

add_custom_command(TARGET NookLink POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:NookLink>/assets"
    COMMENT "Copying assets"
)

set_target_properties(NookLink PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)
endif()
