cmake_minimum_required(VERSION 3.15)

project(Celestials CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define configurations for multi-config generators like Visual Studio
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available configuration types" FORCE)

# Set runtime library and multi-processor compilation for MSVC
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /MP")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /MP /O2")
endif()

# Set output directories for executables, libraries, and archives
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/out/x64-Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/out/x64-Release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/libs")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/libs")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/libs")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/libs")

# Explicitly create output directories to ensure they exist
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/out/x64-Debug")
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/out/x64-Release")

# Set policy CMP0135 to NEW to ensure extracted timestamps are set to extraction time
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Set policy CMP0111 to NEW to fail if imported target properties are missing
if(POLICY CMP0111)
    cmake_policy(SET CMP0111 NEW)
endif()

# Disable CTest to remove CTestDashboardTargets
set(CMAKE_SKIP_TESTING TRUE)

# Exclude INSTALL target from default build to minimize CMakePredefinedTargets
set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 0)

# Define the executable with all source files in src/
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)
file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
)
add_executable(Celestials ${SOURCES} ${HEADERS})

# Explicitly set the output directory for the Celestials target for all configurations
set_target_properties(Celestials PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/out/x64-Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/out/x64-Release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/out/x64-Release"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/out/x64-Release"
)

# Organize source and header files in Solution Explorer
source_group(TREE "${CMAKE_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${HEADERS})
source_group(TREE "${CMAKE_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${SOURCES})

# Set Celestials as the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Celestials)

# Define GLEW_NO_GLU for the entire project
target_compile_definitions(Celestials PRIVATE GLEW_NO_GLU)

# Enable FetchContent
include(FetchContent)

# Fetch SDL3 as a shared library
message(STATUS "Fetching SDL3...")
FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.10/SDL3-3.2.10.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build SDL3 as a shared library" FORCE)
FetchContent_MakeAvailable(SDL3)

# SDL3_ttf setup
message(STATUS "Configuring SDL3_ttf...")
if(WIN32)
    # Use pre-compiled binaries on Windows
    set(SDL3_TTF_PATH "${CMAKE_SOURCE_DIR}/thirdparty/sdl3_ttf")
    
    # Determine architecture for Windows
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL3_TTF_LIB_DIR "${SDL3_TTF_PATH}/lib/x64")
        set(SDL3_TTF_DLL_DIR_DEBUG "${SDL3_TTF_LIB_DIR}")
        set(SDL3_TTF_DLL_DIR_RELEASE "${SDL3_TTF_PATH}/win32-x64")
    else()
        set(SDL3_TTF_LIB_DIR "${SDL3_TTF_PATH}/lib/x86")
        set(SDL3_TTF_DLL_DIR_DEBUG "${SDL3_TTF_LIB_DIR}")
        set(SDL3_TTF_DLL_DIR_RELEASE "${SDL3_TTF_PATH}/win32-x86")
    endif()

    # Find the library (shared between Debug and Release) and include directories
    find_library(SDL3_TTF_LIBRARY
        NAMES SDL3_ttf sdl3_ttf SDL3_ttf.lib sdl3_ttf.lib
        PATHS "${SDL3_TTF_LIB_DIR}"
        NO_DEFAULT_PATH
    )
    find_path(SDL3_TTF_INCLUDE_DIR
        NAMES SDL_ttf.h
        PATHS "${SDL3_TTF_PATH}/include/SDL3_ttf"
        NO_DEFAULT_PATH
    )

    # Fallback: Hardcode the path to SDL3_ttf.lib if find_library fails
    if(NOT SDL3_TTF_LIBRARY)
        set(SDL3_TTF_LIBRARY "${SDL3_TTF_LIB_DIR}/SDL3_ttf.lib")
        if(NOT EXISTS "${SDL3_TTF_LIBRARY}")
            message(FATAL_ERROR "SDL3_ttf.lib not found in ${SDL3_TTF_LIB_DIR}. Attempted fallback path ${SDL3_TTF_LIBRARY} does not exist. Please ensure the library file is correctly placed.")
        else()
            message(WARNING "find_library failed to locate SDL3_ttf.lib. Using hardcoded path: ${SDL3_TTF_LIBRARY}")
        endif()
    endif()

    # Debugging: Print the paths to verify
    message(STATUS "SDL3_TTF_LIBRARY: ${SDL3_TTF_LIBRARY}")
    message(STATUS "SDL3_TTF_INCLUDE_DIR: ${SDL3_TTF_INCLUDE_DIR}")
    message(STATUS "SDL3_TTF_DLL_DIR_DEBUG: ${SDL3_TTF_DLL_DIR_DEBUG}/SDL3_ttf.dll")
    message(STATUS "SDL3_TTF_DLL_DIR_RELEASE: ${SDL3_TTF_DLL_DIR_RELEASE}/SDL3_ttf.dll")

    # Verify that the required files exist
    if(NOT EXISTS "${SDL3_TTF_LIBRARY}")
        message(FATAL_ERROR "SDL3_ttf.lib does not exist at ${SDL3_TTF_LIBRARY}. Please ensure the library file is correctly placed.")
    endif()
    if(NOT SDL3_TTF_INCLUDE_DIR)
        message(FATAL_ERROR "SDL3_ttf include directory not found in ${SDL3_TTF_PATH}/include/SDL3_ttf. Please ensure the header files are correctly placed.")
    endif()
    if(NOT EXISTS "${SDL3_TTF_DLL_DIR_DEBUG}/SDL3_ttf.dll")
        message(FATAL_ERROR "SDL3_ttf Debug DLL not found at ${SDL3_TTF_DLL_DIR_DEBUG}/SDL3_ttf.dll. Please ensure the Debug DLL is correctly placed.")
    endif()
    if(NOT EXISTS "${SDL3_TTF_DLL_DIR_RELEASE}/SDL3_ttf.dll")
        message(FATAL_ERROR "SDL3_ttf Release DLL not found at ${SDL3_TTF_DLL_DIR_RELEASE}/SDL3_ttf.dll. Please ensure the Release DLL is correctly placed.")
    endif()

    # Create an imported target for SDL3_ttf
    add_library(SDL3_ttf SHARED IMPORTED)
    set_target_properties(SDL3_ttf PROPERTIES
        IMPORTED_LOCATION_DEBUG "${SDL3_TTF_DLL_DIR_DEBUG}/SDL3_ttf.dll"
        IMPORTED_LOCATION_RELEASE "${SDL3_TTF_DLL_DIR_RELEASE}/SDL3_ttf.dll"
        IMPORTED_LOCATION_RELWITHDEBINFO "${SDL3_TTF_DLL_DIR_RELEASE}/SDL3_ttf.dll"
        IMPORTED_LOCATION_MINSIZEREL "${SDL3_TTF_DLL_DIR_RELEASE}/SDL3_ttf.dll"
        IMPORTED_IMPLIB_DEBUG "${SDL3_TTF_LIBRARY}"
        IMPORTED_IMPLIB_RELEASE "${SDL3_TTF_LIBRARY}"
        IMPORTED_IMPLIB_RELWITHDEBINFO "${SDL3_TTF_LIBRARY}"
        IMPORTED_IMPLIB_MINSIZEREL "${SDL3_TTF_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL3_TTF_PATH}/include"
    )

    # Copy DLLs to the output directory based on configuration
    add_custom_command(TARGET Celestials POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Target output directory for configuration $<CONFIG>: $<TARGET_FILE_DIR:Celestials>"
        COMMAND ${CMAKE_COMMAND} -E echo "Copying SDL3_ttf.dll for configuration: $<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Celestials>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${SDL3_TTF_DLL_DIR_DEBUG}/SDL3_ttf.dll,${SDL3_TTF_DLL_DIR_RELEASE}/SDL3_ttf.dll>"
        $<TARGET_FILE_DIR:Celestials>
        COMMAND ${CMAKE_COMMAND} -E echo "Copied SDL3_ttf.dll to $<TARGET_FILE_DIR:Celestials>"
    )
else()
    # On Linux, require SDL3_ttf to be installed via system package
    find_package(SDL3_ttf REQUIRED)
    if(SDL3_ttf_FOUND)
        message(STATUS "SDL3_ttf found on system.")
    else()
        message(FATAL_ERROR "SDL3_ttf not found. Please install it using your package manager (e.g., 'sudo apt install libsdl3-ttf-dev' on Ubuntu).")
    endif()

    # Link SDL3::SDL3-shared to the actual SDL3_ttf target (not the alias)
    if(TARGET SDL3_ttf::SDL3_ttf-shared AND TARGET SDL3::SDL3-shared)
        target_link_libraries(SDL3_ttf::SDL3_ttf-shared INTERFACE SDL3::SDL3-shared)
    endif()

    # Create an alias for the system SDL3_ttf
    if(TARGET SDL3_ttf::SDL3_ttf-shared)
        add_library(SDL3_ttf ALIAS SDL3_ttf::SDL3_ttf-shared)
    else()
        message(FATAL_ERROR "SDL3_ttf::SDL3_ttf-shared target not found. Ensure SDL3_ttf is installed correctly.")
    endif()
endif()

# Fetch SDL3_image as a shared library
message(STATUS "Fetching SDL3_image...")
FetchContent_Declare(
    SDL3_image
    URL https://github.com/libsdl-org/SDL_image/archive/refs/heads/main.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(SDLIMAGE_VENDORED OFF CACHE BOOL "Disable vendored dependencies for SDL3_image" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build SDL3_image as a shared library" FORCE)
FetchContent_MakeAvailable(SDL3_image)

# Fetch libnoise
message(STATUS "Fetching libnoise...")
FetchContent_Declare(
    libnoise
    URL https://github.com/qknight/libnoise/archive/refs/heads/master.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libnoise as a static library" FORCE)
set(BUILD_LIBNOISE_EXAMPLES OFF CACHE BOOL "Disable building libnoise examples" FORCE)
FetchContent_MakeAvailable(libnoise)

# Ensure noiseutils-static can find libnoise headers and depends on noise-static
target_include_directories(noiseutils-static PUBLIC 
    ${libnoise_SOURCE_DIR}/noiseutils
    ${libnoise_SOURCE_DIR}/src
)
target_link_libraries(noiseutils-static noise-static)
set_target_properties(noise-static PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
set_target_properties(noiseutils-static PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# Fetch nlohmann/json
message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(nlohmann_json)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/thirdparty/nlohmann)
file(COPY ${nlohmann_json_SOURCE_DIR}/json.hpp DESTINATION ${CMAKE_BINARY_DIR}/thirdparty/nlohmann/)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_BINARY_DIR}/thirdparty/nlohmann)

# Fetch GLM
message(STATUS "Fetching GLM...")
FetchContent_Declare(
    glm
    URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build GLM as a static library" FORCE)
FetchContent_MakeAvailable(glm)
set_target_properties(glm PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# Fetch GLEW
message(STATUS "Fetching GLEW...")
FetchContent_Declare(
    glew
    URL https://github.com/Perlmint/glew-cmake/archive/refs/tags/glew-cmake-2.2.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build GLEW as a static library" FORCE)
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "Build GLEW as a static library" FORCE)
set(glew-cmake_BUILD_STATIC ON CACHE BOOL "Build GLEW static library" FORCE)
FetchContent_MakeAvailable(glew)
set_target_properties(libglew_static PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# Fetch Box2D
message(STATUS "Fetching Box2D...")
FetchContent_Declare(
    box2d
    URL https://github.com/erincatto/box2d/archive/refs/tags/v3.1.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Box2D as a static library" FORCE)
FetchContent_MakeAvailable(box2d)
set_target_properties(box2d PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# Fetch ImGui
message(STATUS "Fetching ImGui...")
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.9b.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
set_target_properties(imgui PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
target_link_libraries(imgui PUBLIC SDL3::SDL3)
if(WIN32)
    target_link_libraries(imgui PUBLIC opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui PUBLIC OpenGL::GL)
endif()

# Link libraries
target_link_libraries(Celestials PRIVATE
    SDL3::SDL3-shared
    SDL3_ttf
    SDL3_image::SDL3_image-shared
    noise-static
    nlohmann_json
    glm::glm
    libglew_static
    box2d
    imgui
)

# Link OpenGL
if(WIN32)
    target_link_libraries(Celestials PRIVATE opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(Celestials PRIVATE OpenGL::GL)
endif()

# Include directories for the game
target_include_directories(Celestials PRIVATE
    include
    ${libnoise_SOURCE_DIR}/src
    ${sdl3_image_SOURCE_DIR}/include
    ${glew_SOURCE_DIR}/include
)

# Copy shared libraries to the binary directory on Windows
if(WIN32)
    foreach(lib SDL3-shared SDL3_image-shared)
        if(TARGET ${lib})
            add_custom_command(TARGET Celestials POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Copying ${lib} for configuration: $<CONFIG>"
                COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Celestials>
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${lib}>
                $<TARGET_FILE_DIR:Celestials>
                COMMAND ${CMAKE_COMMAND} -E echo "Copied ${lib} to $<TARGET_FILE_DIR:Celestials>"
            )
        endif()
    endforeach()
endif()

# Copy the resources folder to the output directory
add_custom_command(TARGET Celestials POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying resources folder for configuration: $<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Celestials>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources"
    $<TARGET_FILE_DIR:Celestials>/resources
    COMMAND ${CMAKE_COMMAND} -E echo "Copied resources to $<TARGET_FILE_DIR:Celestials>/resources"
)

# Set the working directory for Visual Studio debugging
if(MSVC)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Celestials.vcxproj.user"
        CONTENT [=[
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
</Project>
]=])
endif()
