cmake_minimum_required(VERSION 3.21)
set(MYPROJ VanKVulkanTemplate)
project(${MYPROJ} VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Download CPM.cmake
set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake...")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()
include(${CPM_DOWNLOAD_LOCATION})

# Find Vulkan
find_package(Vulkan REQUIRED)
if(NOT Vulkan_SLANGC_EXECUTABLE)
    get_filename_component(_Vulkan_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
    find_program(Vulkan_SLANGC_EXECUTABLE
        NAMES slangc
        HINTS ${_Vulkan_LIB_DIR}/../Bin
    )
endif()

# Add the executable
add_executable(${MYPROJ} 
    src/minimal_latest.cpp 
    src/debug_util.h
    src/logger.h
    src/Renderer.cpp
    src/Renderer.h
    src/RenderCommand.cpp
    src/RenderCommand.h
    src/RendererAPI.cpp
    src/RendererAPI.h
    src/VulkanRendererAPI.cpp
    src/VulkanRendererAPI.h
    src/Texture.cpp
    src/Texture.h
    src/VulkanTexture.cpp
    src/VulkanTexture.h
    src/Buffer.cpp
    src/Buffer.h
    src/VulkanBuffer.cpp
    src/VulkanBuffer.h
  )
    
    set_target_properties(${MYPROJ} PROPERTIES
  MACOSX_BUNDLE TRUE
)

set_target_properties(${MYPROJ} PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist"
)

set_target_properties(${MYPROJ} PROPERTIES
  MACOSX_BUNDLE_ICON_FILE VanKIcon.icns
)

set_source_files_properties(resources/VanKIcon.icns PROPERTIES
  MACOSX_PACKAGE_LOCATION "Resources"
)

# Make Visual Studio using this project as the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${MYPROJ})

# Set warning level 3 for MSVC
if(MSVC)
  target_compile_options(${MYPROJ} PRIVATE /W4)
endif()

# Include SDL3
CPMAddPackage(
  NAME SDL3
  GITHUB_REPOSITORY libsdl-org/SDL
  GIT_TAG release-3.2.14
  OPTIONS
    "SDL_TEST OFF"
    "SDL_SHARED OFF"
    "SDL_STATIC ON"
)

# Include GLM
CPMAddPackage(
  NAME glm
  GITHUB_REPOSITORY g-truc/glm
  GIT_TAG master
)
set_property(TARGET glm PROPERTY FOLDER "ThirdParty")

# Include Volk
CPMAddPackage(
  NAME volk
  GITHUB_REPOSITORY zeux/volk
  GIT_TAG 1.4.304
)
set_property(TARGET volk PROPERTY FOLDER "ThirdParty")

# Include VMA
CPMAddPackage(
  NAME VulkanMemoryAllocator
  GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
  GIT_TAG master
  DOWNLOAD_ONLY
)
set(VMA_SOURCE ${VulkanMemoryAllocator_SOURCE_DIR}/include/vk_mem_alloc.h)
target_include_directories(${MYPROJ} PRIVATE ${VulkanMemoryAllocator_SOURCE_DIR}/include)
target_sources(${MYPROJ} PRIVATE ${VMA_SOURCE})
source_group("vma" FILES ${VMA_SOURCE})

#include ImGui
CPMAddPackage(
  NAME imgui
  GITHUB_REPOSITORY ocornut/imgui
  GIT_TAG docking
  DOWNLOAD_ONLY
)
set(IMGUI_SOURCES 
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui.h
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.h
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.h
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)
target_link_libraries(imgui PUBLIC volk SDL3::SDL3)
set_property(TARGET imgui PROPERTY FOLDER "ThirdParty")

# Download stb_image.h
set(STB_IMAGE_URL "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h")
set(STB_IMAGE_PATH "${CMAKE_BINARY_DIR}/stb_image.h")
if(NOT EXISTS ${STB_IMAGE_PATH})
    message(STATUS "Downloading stb_image.h...")
    file(DOWNLOAD ${STB_IMAGE_URL} ${STB_IMAGE_PATH})
endif()
target_sources(${MYPROJ} PRIVATE ${STB_IMAGE_PATH})
source_group("stb" FILES ${STB_IMAGE_PATH})
target_include_directories(${MYPROJ} PRIVATE "${CMAKE_BINARY_DIR}")

# Link libraries and include directories
target_link_libraries(${MYPROJ} PRIVATE
  SDL3::SDL3
  volk
  imgui
  glm
)

target_include_directories(${MYPROJ} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Define VK_NO_PROTOTYPES to avoid including Vulkan prototypes
# This is necessary because we are using volk to load Vulkan functions
target_compile_definitions(${MYPROJ} PRIVATE VK_NO_PROTOTYPES)

#------------------------------------------------------------------------------------------------------------------------------
# Compile GLSL shaders
file(GLOB SHADER_FILES "shaders/*.glsl")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/_autogen)
foreach(SHADER ${SHADER_FILES})
  get_filename_component(SHADER_NAME ${SHADER} NAME)
  string(REPLACE "." "_" VN_SHADER_NAME ${SHADER_NAME})
  set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/_autogen/${SHADER_NAME}.h")
  set(_COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -g -D_glsl --target-env vulkan1.3 --vn ${VN_SHADER_NAME} -o ${OUTPUT_FILE} ${SHADER})
  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${_COMMAND}
    # COMMENT  "${_COMMAND}"
    MAIN_DEPENDENCY ${SHADER}
    COMMENT "Compiling GLSL shader ${SHADER_NAME}"
    VERBATIM
  )
  list(APPEND SHADER_HEADERS ${OUTPUT_FILE})
endforeach()

#------------------------------------------------------------------------------------------------------------------------------
# Find Slang compiler
if( Vulkan_SLANGC_EXECUTABLE)
    # Initialize variables
    set(USE_GLSL OFF )
    set(USE_SLANG OFF)
    # Define the available choices for shader languages
    set(CHOICES "GLSL" "SLANG")
    set(USE_SHADER_LANGUAGE "SLANG" CACHE STRING "Choose the shading language to use.") # Shader language to use by default
    set_property(CACHE USE_SHADER_LANGUAGE PROPERTY STRINGS ${CHOICES}) # Provide the choices to the user in the CMake GUI
    if(USE_SHADER_LANGUAGE STREQUAL "SLANG")
        target_compile_definitions(${MYPROJ} PRIVATE USE_SLANG)
    endif()

   

    # Link to Slang libraries
    # Ensure we compute a valid path to slang.lib
    get_filename_component(_Vulkan_SDK_DIR "${Vulkan_LIBRARY}" DIRECTORY)
    get_filename_component(_Vulkan_SDK_DIR "${_Vulkan_SDK_DIR}" DIRECTORY) # up one more level
    set(SLANG_LIB_PATH "${_Vulkan_SDK_DIR}/Lib/slang.lib")

    if(EXISTS "${SLANG_LIB_PATH}")
      message(STATUS "Found slang.lib at: ${SLANG_LIB_PATH}")
      target_link_libraries(${MYPROJ} PRIVATE "${SLANG_LIB_PATH}")
    else()
      message(FATAL_ERROR "Could not find slang.lib at expected location: ${SLANG_LIB_PATH}")
    endif()

    # Setup glslang paths (adjust if needed)
    get_filename_component(_Vulkan_SDK_DIR "${Vulkan_LIBRARY}" DIRECTORY)
    get_filename_component(_Vulkan_SDK_DIR "${_Vulkan_SDK_DIR}" DIRECTORY) # up one more level
    set(SHADERC_INCLUDE_DIR "${_Vulkan_SDK_DIR}/Include/shaderc")
    
    set(SHADERC_LIB_RELEASE "${_Vulkan_SDK_DIR}/Lib/shaderc_combined.lib")
    set(SHADERC_LIB_DEBUG   "${_Vulkan_SDK_DIR}/Lib/shaderc_combinedd.lib")
    # Link to shaderc.lib
    if(EXISTS "${SHADERC_LIB_RELEASE}" AND EXISTS "${SHADERC_LIB_DEBUG}")
  message(STATUS "Found shaderc combined libs (release and debug)")
  target_include_directories(${MYPROJ} PRIVATE "${SHADERC_INCLUDE_DIR}")

  target_link_libraries(${MYPROJ} PRIVATE
    "$<$<CONFIG:Debug>:${SHADERC_LIB_DEBUG}>"
    "$<$<NOT:$<CONFIG:Debug>>:${SHADERC_LIB_RELEASE}>"
  )
else()
  message(FATAL_ERROR "Could not find shaderc combined libraries at expected locations")
endif()

     # Include Slang/Shaderc headers
    target_include_directories(${MYPROJ} PRIVATE
  "${SLANG_INCLUDE_DIR}"
  "${SHADERC_INCLUDE_DIR}"
)

    set(_SLANG_FLAGS
        -profile sm_6_6+spirv_1_6
        -capability spvInt64Atomics+spvShaderInvocationReorderNV+spvShaderClockKHR+spvRayTracingMotionBlurNV
        -target spirv
        -emit-spirv-directly
        -force-glsl-scalar-layout
        -fvk-use-entrypoint-name
        -g3  # Enable debug information
        -O0  # Disable optimization (spirv-opt)
        -source-embed-style text     
    )

    # Compile Slang shaders using slangc
    file(GLOB SHADER_SLANG_FILES "shaders/*.slang")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/_autogen)
    foreach(SHADER ${SHADER_SLANG_FILES})
      get_filename_component(SHADER_NAME ${SHADER} NAME)
      string(REPLACE "." "_" VN_SHADER_NAME ${SHADER_NAME})
      set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/_autogen/${SHADER_NAME}.h")
      set(_COMMAND ${Vulkan_SLANGC_EXECUTABLE} 
        ${_SLANG_FLAGS} 
        -source-embed-name ${VN_SHADER_NAME}
        -o ${OUTPUT_FILE} ${SHADER})
      
      add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${_COMMAND}
        MAIN_DEPENDENCY ${SHADER}
        COMMENT "Compiling Slang shader ${SHADER_NAME} with slangc"
        VERBATIM
      )
      list(APPEND SHADER_HEADERS ${OUTPUT_FILE})
    endforeach()

else()
    message(ERROR "slangc executable not found")
endif()

# Add shader files to the project
file(GLOB SHADER_H_FILES "shaders/*.h")
source_group("Shaders" FILES ${SHADER_FILES} ${SHADER_SLANG_FILES} ${SHADER_H_FILES})
target_sources(${MYPROJ} PRIVATE ${SHADER_FILES} ${SHADER_H_FILES})

# Add the shader headers directly to the executable target
target_sources(${MYPROJ} PRIVATE ${SHADER_HEADERS})

# Include the shader and _autogen directory
target_include_directories(${MYPROJ} PRIVATE "${CMAKE_BINARY_DIR}")
target_include_directories(${MYPROJ} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# Copy .jpg image next to the executable
file(GLOB IMAGE_FILE "${CMAKE_SOURCE_DIR}/resources/*.jpg")
add_custom_command(
  TARGET ${MYPROJ} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${IMAGE_FILE} $<TARGET_FILE_DIR:${MYPROJ}>
)

# Set a default installation prefix
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/_install" CACHE PATH "Default install path" FORCE)

# Define the installation directory for the executable
install(TARGETS ${MYPROJ}
    RUNTIME DESTINATION bin
)

# Installing also the image
install(FILES ${IMAGE_FILE}
    DESTINATION bin
)