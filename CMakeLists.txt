cmake_minimum_required (VERSION 3.16)

project (viewer)

find_package(OpenGL REQUIRED)

#Tells compiler to use c++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Enable the required dependencies
set(DEP_GLFW ON CACHE BOOL "Build GLFW")
set(DEP_GLM ON CACHE BOOL "Build GLM")
set(DEP_GLEW ON CACHE BOOL "Build GLEW")
set(DEP_FREETYPE ON CACHE BOOL "Build FreeType")
set(DEP_STB ON CACHE BOOL "Include STB headers")
set(DEP_IMGUI ON CACHE BOOL "Build Dear ImGui")
set(DEP_ASSIMP ON CACHE BOOL "Build ASSIMP") 

# ImGui options
set(DEP_IMGUI_BACKEND_GLFW ON CACHE BOOL "ImGui GLFW backend")
set(DEP_IMGUI_BACKEND_OPENGL3 ON CACHE BOOL "ImGui OpenGL3 backend")
set(DEP_IMGUI_FREETYPE ON CACHE BOOL "ImGui FreeType support")

# Add dependencies
add_subdirectory(dependencies)

set (SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/")
file (GLOB SOURCE_FILES "${SOURCE_DIR}/*.cpp")

add_executable (${PROJECT_NAME} ${SOURCE_FILES}
        src/window.cpp
        src/window.h
        src/camera.cpp
        src/camera.h
        src/animations/skinnedMesh.cpp
        src/animations/skinnedMesh.h
        src/texture.cpp
        src/texture.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE glew_static)
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE glm)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE stb)


#target_link_libraries(${PROJECT_NAME} PRIVATE
#        ${OPENGL_LIBRARIES},
#        glm,
#        glew_static,
#        stb,
#        assimp,
#        imgui
#)

if (WIN32)
    add_compile_definitions(GLEW_STATIC)
    target_link_libraries(${PROJECT_NAME} opengl32)
    target_link_libraries(${PROJECT_NAME} glu32)
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} GL)
endif()
