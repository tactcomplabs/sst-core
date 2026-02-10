add_library(simdjson OBJECT simdjson.cpp)

target_include_directories(
  simdjson
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${SST_TOP_SRC_DIR}/external/simdjson
  PRIVATE ${SST_TOP_SRC_DIR}/src # For warning suppression
)
