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
//

#include "timingOutput.h"
#include "simulation_impl.h"
#include "sst/core/stringize.h"

namespace SST {
namespace Core {

TimingOutput::TimingOutput(const SST::Output& output, bool printEnable) : output_(output), printEnable_(printEnable) {}

void
TimingOutput::setJSON(const std::string& path)
{
    SST::Util::Filesystem filesystem = Simulation_impl::filesystem;
    outputFile                       = filesystem.fopen(path, "wt");
}

void
TimingOutput::generate()
{
    if (printEnable_) {
        std::string ua_buffer;
        ua_buffer = format_string("%" PRIu64 "KB", u64map_.at(Key::LOCAL_MAX_RSS));
        UnitAlgebra max_rss_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "KB", u64map_.at(Key::GLOBAL_MAX_RSS));
        UnitAlgebra global_rss_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "B", u64map_.at(Key::GLOBAL_MAX_SYNC_DATA_SIZE));
        UnitAlgebra global_max_sync_data_size_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "B", u64map_.at(Key::GLOBAL_SYNC_DATA_SIZE));
        UnitAlgebra global_sync_data_size_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "B", u64map_.at(Key::MAX_MEMPOOL_SIZE));
        UnitAlgebra max_mempool_size_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "B", u64map_.at(GLOBAL_MEMPOOL_SIZE));
        UnitAlgebra global_mempool_size_ua(ua_buffer);

        output_.output("\n");
        output_.output("\n");
        output_.output("------------------------------------------------------------\n");
        output_.output("Simulation Timing Information (Wall Clock Times):\n");
        output_.output("  Build time:                      %f seconds\n", dmap_.at(MAX_BUILD_TIME));
        output_.output("  Run loop time:                   %f seconds\n", dmap_.at(MAX_RUN_TIME));
        output_.output("  Total time:                      %f seconds\n", dmap_.at(MAX_TOTAL_TIME));
        output_.output("\n");
        output_.output(
            "Simulated time:                    %s\n", uamap_.at(SIMULATED_TIME_UA).toStringBestSI().c_str());
        output_.output("\n");
        output_.output("Simulation Resource Information:\n");
        output_.output("  Max Resident Set Size:           %s\n", max_rss_ua.toStringBestSI().c_str());
        output_.output("  Approx. Global Max RSS Size:     %s\n", global_rss_ua.toStringBestSI().c_str());
        output_.output("  Max Local Page Faults:           %" PRIu64 " faults\n", u64map_.at(LOCAL_MAX_PF));
        output_.output("  Global Page Faults:              %" PRIu64 " faults\n", u64map_.at(GLOBAL_PF));
        output_.output("  Max Output Blocks:               %" PRIu64 " blocks\n", u64map_.at(GLOBAL_MAX_IO_OUT));
        output_.output("  Max Input Blocks:                %" PRIu64 " blocks\n", u64map_.at(GLOBAL_MAX_IO_IN));
        output_.output("  Max mempool usage:               %s\n", max_mempool_size_ua.toStringBestSI().c_str());
        output_.output("  Global mempool usage:            %s\n", global_mempool_size_ua.toStringBestSI().c_str());
        output_.output("  Global active activities:        %" PRIu64 " activities\n", u64map_.at(GLOBAL_ACTIVE_ACTIVITIES));
        output_.output("  Current global TimeVortex depth: %" PRIu64 " entries\n", u64map_.at(GLOBAL_CURRENT_TV_DEPTH));
        output_.output("  Max TimeVortex depth:            %" PRIu64 " entries\n", u64map_.at(GLOBAL_MAX_TV_DEPTH));
        output_.output(
            "  Max Sync data size:              %s\n", global_max_sync_data_size_ua.toStringBestSI().c_str());
        output_.output("  Global Sync data size:           %s\n", global_sync_data_size_ua.toStringBestSI().c_str());
        output_.output("------------------------------------------------------------\n");
        output_.output("\n");
        output_.output("\n");
    }

}

void
TimingOutput::set(Key key, const uint64_t v)
{
    u64map_[key] = v;
}

void
TimingOutput::set(Key key, UnitAlgebra v)
{
    uamap_[key] = v;
}

void
TimingOutput::set(Key key, double v)
{
    dmap_[key] = v;
}

TimingOutput::~TimingOutput()
{
    fclose(outputFile);
}

} // namespace Core
} // namespace SST
