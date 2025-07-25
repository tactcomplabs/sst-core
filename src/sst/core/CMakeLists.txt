# ~~~
# SST-CORE src/sst/core CMake
#
# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
# ~~~
#

add_definitions(-DSST_BUILDING_CORE=1)

# -- Required for build
add_subdirectory(impl)
add_subdirectory(model)
add_subdirectory(shared)
add_subdirectory(sync)
add_subdirectory(testingframework)
add_subdirectory(testElements)

# -- Required for header installation
add_subdirectory(cfgoutput)
add_subdirectory(eli)
add_subdirectory(env)
add_subdirectory(interfaces)
add_subdirectory(interprocess)
add_subdirectory(math)
add_subdirectory(profile)
add_subdirectory(rng)
add_subdirectory(serialization)
add_subdirectory(statapi)

add_library(
  sst-core-lib OBJECT
  action.cc
  clock.cc
  baseComponent.cc
  checkpointAction.cc
  component.cc
  componentExtension.cc
  componentInfo.cc
  config.cc
  configGraph.cc
  cfgoutput/pythonConfigOutput.cc
  cfgoutput/dotConfigOutput.cc
  cfgoutput/xmlConfigOutput.cc
  cfgoutput/jsonConfigOutput.cc
  eli/elibase.cc
  eli/elementinfo.cc
  elemLoader.cc
  event.cc
  exit.cc
  factory.cc
  heartbeat.cc
  initQueue.cc
  link.cc
  memuse.cc
  mempool.cc
  namecheck.cc
  oneshot.cc
  output.cc
  params.cc
  pollingLinkQueue.cc
  simulation.cc
  ssthandler.cc
  subcomponent.cc
  timeLord.cc
  uninitializedQueue.cc
  unitAlgebra.cc
  module.cc
  sstpart.cc
  timeVortex.cc
  timingOutput.cc
  profile/clockHandlerProfileTool.cc
  profile/componentProfileTool.cc
  profile/eventHandlerProfileTool.cc
  profile/syncProfileTool.cc
  profile/profiletool.cc
  serialization/serializable_base.cc
  serialization/serialize_serializable.cc
  serialization/serializer.cc
  serialization/statics.cc
  interfaces/TestEvent.cc
  interfaces/simpleMem.cc
  interfaces/stdMem.cc
  interfaces/simpleNetwork.cc
  interprocess/ipctunnel.cc
  interprocess/mmapparent.cc
  interprocess/shmparent.cc
  rng/marsaglia.cc
  rng/mersenne.cc
  rng/xorshift.cc
  statapi/statengine.cc
  statapi/statgroup.cc
  statapi/statoutput.cc
  statapi/statfieldinfo.cc
  statapi/statoutputtxt.cc
  statapi/statoutputcsv.cc
  statapi/statoutputjson.cc
  statapi/statbase.cc
  stringize.cc
  cputimer.cc
  iouse.cc)

set(SSTHeaders
    action.h
    activity.h
    activityQueue.h
    baseComponent.h
    clock.h
    componentExtension.h
    component.h
    componentInfo.h
    configGraph.h
    configGraphOutput.h
    config.h
    configBase.h
    configShared.h
    cputimer.h
    decimal_fixedpoint.h
    elemLoader.h
    event.h
    exit.h
    factory.h
    from_string.h
    heartbeat.h
    initQueue.h
    iouse.h
    link.h
    linkMap.h
    linkPair.h
    mempool.h
    memuse.h
    module.h
    namecheck.h
    objectComms.h
    oneshot.h
    output.h
    params.h
    pollingLinkQueue.h
    profile.h
    rankInfo.h
    simulation.h
    sparseVectorMap.h
    ssthandler.h
    sstinfo.h
    sstpart.h
    sst_types.h
    stopAction.h
    stringize.h
    subcomponent.h
    threadsafe.h
    timeConverter.h
    timeLord.h
    timeVortex.h
    uninitializedQueue.h
    unitAlgebra.h
    warnmacros.h)

target_include_directories(sst-core-lib PUBLIC ${SST_TOP_SRC_DIR}/src)
target_include_directories(sst-core-lib PUBLIC ${sst-core_BINARY_DIR}/src)
target_compile_definitions(sst-core-lib PUBLIC SST_BUILDING_CORE=1)

if(SST_ENABLE_EVENT_TRACKING)
  target_compile_definitions(sst-core-lib PUBLIC __SST_DEBUG_EVENT_TRACKING__=1)
endif(SST_ENABLE_EVENT_TRACKING)

if(SST_ENABLE_DEBUG_OUTPUT)
  target_compile_definitions(sst-core-lib PUBLIC __SST_DEBUG_OUTPUT__=1)
endif(SST_ENABLE_DEBUG_OUTPUT)

target_link_libraries(sst-core-lib PUBLIC nlohjson Python::Python
                                          sst-config-headers ${CMAKE_DL_LIBS} m)

if(MPI_FOUND)
  target_link_libraries(sst-core-lib PUBLIC MPI::MPI_CXX)
endif()

if(HDF5_FOUND)
  target_link_libraries(sst-core-lib PRIVATE HDF5::HDF5)
endif()

if(ZLIB_FOUND)
  target_link_libraries(sst-core-lib PRIVATE ZLIB::ZLIB)
endif()

add_library(sst-config-lib OBJECT configBase.cc configShared.cc)
target_link_libraries(sst-config-lib PUBLIC sst-config-headers)
target_include_directories(sst-config-lib PUBLIC ${SST_TOP_SRC_DIR}/src)

add_library(sst-boot-lib OBJECT bootshared.cc)
target_link_libraries(sst-boot-lib PUBLIC sst-config-headers)
target_include_directories(sst-boot-lib PUBLIC ${SST_TOP_SRC_DIR}/src)

add_library(sst-env-lib OBJECT env/envquery.cc env/envconfig.cc)
target_link_libraries(sst-env-lib PUBLIC sst-config-headers)
target_include_directories(sst-env-lib PUBLIC ${SST_TOP_SRC_DIR}/src)

add_executable(sstsim.x main.cc)
target_link_libraries(
  sstsim.x
  PRIVATE sst-core-lib
          sst-env-lib
          sst-config-lib
          partitioner
          timeVortex
          modelCore
          modelpython
          modeljson
          sync
          shared)
set_target_properties(sstsim.x PROPERTIES ENABLE_EXPORTS ON)

add_executable(sstinfo.x sstinfo.cc)
target_link_libraries(
  sstinfo.x
  PRIVATE sst-core-lib
          sst-env-lib
          sst-config-lib
          partitioner
          timeVortex
          modelCore
          modelpython
          modeljson
          sync
          shared
          tinyxml)
set_target_properties(sstinfo.x PROPERTIES ENABLE_EXPORTS ON)

if(UNIX
   AND (NOT APPLE)
   AND HAVE_LIBRT)
  target_link_libraries(sstinfo.x PRIVATE rt)
  target_link_libraries(sstsim.x PRIVATE rt)
endif()

if(Threads_FOUND)
  target_link_libraries(sstinfo.x PRIVATE Threads::Threads)
  target_link_libraries(sstsim.x PRIVATE Threads::Threads)
endif()

if(CURSES_FOUND)
  target_link_libraries(sstinfo.x PRIVATE ${CURSES_LIBRARIES})
  # Before means no chance of interfering with a system curses if a
  # user-provided one was specified.
  target_include_directories(sstinfo.x BEFORE PRIVATE ${CURSES_INCLUDE_DIRS})
endif()

add_executable(sst bootsst.cc)
target_link_libraries(sst PRIVATE sst-boot-lib sst-env-lib sst-config-lib)

add_executable(sst-info bootsstinfo.cc)
target_link_libraries(sst-info PRIVATE sst-boot-lib sst-env-lib sst-config-lib)

add_executable(sst-config sstconfigtool.cc)
target_link_libraries(sst-config PRIVATE sst-env-lib)

add_executable(sst-register sstregistertool.cc)
target_link_libraries(sst-register PRIVATE sst-env-lib)

install(TARGETS sst sst-info sst-config sst-register)
install(TARGETS sstsim.x sstinfo.x DESTINATION libexec)

install(FILES ${SSTHeaders} DESTINATION "include/sst/core")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sst_config.h
              ${CMAKE_CURRENT_BINARY_DIR}/build_info.h
        DESTINATION "include/sst/core")

# EOF
