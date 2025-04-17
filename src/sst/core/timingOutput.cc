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

namespace SST {
namespace Core {

TimingOutput::TimingOutput(const char* path)
{
    SST::Util::Filesystem filesystem = Simulation_impl::filesystem;
    outputFile                       = filesystem.fopen(path, "wt");
}

TimingOutput::~TimingOutput() {
    fclose(outputFile);
}

} // namespace Core
} // namespace SST
