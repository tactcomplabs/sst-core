// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_TIMING_OUTPUT_H
#define SST_CORE_TIMING_OUTPUT_H

#include "sst/core/util/filesystem.h"
#include "sst/core/output.h"
#include "sst/core/unitAlgebra.h"

#include <map>

namespace SST::Core {

/**
 * Outputs configuration data to a specified file path.
 */
class TimingOutput
{
public:
    /** Timing Parameters
     */
    enum Key {
        LOCAL_MAX_RSS,
        GLOBAL_MAX_RSS,
        LOCAL_MAX_PF,
        GLOBAL_PF,
        GLOBAL_MAX_IO_IN,
        GLOBAL_MAX_IO_OUT,
        GLOBAL_MAX_SYNC_DATA_SIZE,
        GLOBAL_SYNC_DATA_SIZE,
        MAX_MEMPOOL_SIZE,
        GLOBAL_MEMPOOL_SIZE,
        MAX_BUILD_TIME,
        MAX_RUN_TIME,
        MAX_TOTAL_TIME,
        SIMULATED_TIME_UA,
        GLOBAL_ACTIVE_ACTIVITIES,
        GLOBAL_CURRENT_TV_DEPTH,
        GLOBAL_MAX_TV_DEPTH,
    };

    TimingOutput(const SST::Output& output, bool printEnable);
    virtual ~TimingOutput();
    void setJSON(const std::string& path);
    void generate();

    void set(Key key, uint64_t v);
    void set(Key key, UnitAlgebra v);

 private:
    SST::Output output_;
    bool printEnable_;

    std::map<Key, uint64_t> u64map_ = {};
    std::map<Key, UnitAlgebra> uamap_ = {};
    FILE* outputFile = nullptr;

};

} // namespace SST::Core

#endif // SST_CORE_TIMING_OUTPUT_H
