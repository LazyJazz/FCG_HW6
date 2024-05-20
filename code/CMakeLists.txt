file(GLOB SOURCES "*.cpp")

add_executable(main ${SOURCES})

PACK_SHADER_CODE(main)

target_link_libraries(main PRIVATE LongMarch glm::glm Freetype::Freetype tinyobjloader::tinyobjloader)
target_compile_definitions(main PRIVATE ASSETS_PATH="${ASSETS_PATH}")
