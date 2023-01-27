find_package(protobuf CONFIG REQUIRED)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)
add_library(ipc-proto OBJECT ${sources})
target_link_libraries(ipc-proto PUBLIC protobuf::libprotobuf)
#set(PROTOBUF_GENERATED_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/common/ipc/generated_proto" CACHE INTERNAL "Path to generated protobuf files.")
#target_include_directories(ipc-proto PUBLIC "$<BUILD_INTERFACE:${PROTOBUF_GENERATED_BINARY_DIR}>")
protobuf_generate(TARGET ipc-proto)
