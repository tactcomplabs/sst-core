// -*- c++ -*-

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

#ifndef SST_CORE_MODEL_SIMDJSON_SIMDJSONMODEL_H
#define SST_CORE_MODEL_SIMDJSON_SIMDJSONMODEL_H

#include "sst/core/component.h"
#include "sst/core/config.h"
#include "sst/core/cputimer.h"
#include "sst/core/factory.h"
#include "sst/core/memuse.h"
#include "sst/core/model/configGraph.h"
#include "sst/core/model/sstmodel.h"
#include "sst/core/output.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "simdjson/simdjson.h"

// Enable optimization
#define NDEBUG 1

namespace SST::Core {

using namespace simdjson;

class SSTSIMDJSONModelDefinition : public SSTModelDescription
{
public:
    SST_ELI_REGISTER_MODEL_DESCRIPTION(
          SST::Core::SSTSIMDJSONModelDefinition,
          "sst",
          "model.simdjson",
          SST_ELI_ELEMENT_VERSION(1,0,0),
          "Fast (simd) JSON model for building SST simulation graphs",
          true)

    SST_ELI_DOCUMENT_MODEL_SUPPORTED_EXTENSIONS(".json")

    SSTSIMDJSONModelDefinition(const std::string& script_file, int verbosity, Config* config, double start_time);
    virtual ~SSTSIMDJSONModelDefinition();

    ConfigGraph* createConfigGraph() override;

protected:
    std::string  scriptName;
    Output*      output;
    Config*      config;
    ConfigGraph* graph;
    double       start_time;

private:

    void parseProgramOptions(dom::element& document);
    void parseStatOptions(dom::element& document);
    void parseSharedParams(dom::element& document);
    void parseComponents(dom::element& document);
    void parseLinks(dom::element& document);
    void parseStatGroupOptions(dom::element& document);

    void recursiveSubcomponent(ConfigComponent* Parent, dom::object comp);

    ComponentId_t findComponentIdByName(const std::string& Name);
};

} // namespace SST::Core

#endif // SST_CORE_MODEL_SIMDJSON_SIMDJSONMODEL_H
