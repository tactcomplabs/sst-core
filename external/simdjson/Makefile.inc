# This is going to be included by the makefile in src/sst/core/
# so our relative path is src/sst/core/../../../external/simdjson
# so I'm going to prepend with ../../../
sst_core_sources += \
  ../../../external/simdjson/simdjson.h \
  ../../../external/simdjson/simdjson.cpp
