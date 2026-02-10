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

#include "sst_config.h"

#include "sst/core/model/simdjson/simdjsonmodel.h"

#include <cstdint>
#include <string>

DISABLE_WARN_STRICT_ALIASING

using namespace SST;
using namespace SST::Core;
using namespace simdjson;

SSTSIMDJSONModelDefinition::SSTSIMDJSONModelDefinition(
    const std::string& script_file, int verbosity, Config* configObj, double start_time) :
    SSTModelDescription(configObj),
    scriptName(script_file),
    output(nullptr),
    config(configObj),
    graph(nullptr),
    start_time(start_time)
{
    output = new Output("SSTSIMDJSONModel: ", verbosity, 0, SST::Output::STDOUT);

    graph = new ConfigGraph();
    if ( !graph ) {
        output->fatal(CALL_INFO, 1, "Could not create graph object in JSON loader.\n");
    }

    output->verbose(CALL_INFO, 2, 0, "SST loading a JSON model from script: %s\n", script_file.c_str());
}

SSTSIMDJSONModelDefinition::~SSTSIMDJSONModelDefinition()
{
    delete output;
}

ComponentId_t
SSTSIMDJSONModelDefinition::findComponentIdByName(const std::string& Name)
{
    ComponentId_t    Id   = -1;
    ConfigComponent* Comp = nullptr;

    if ( Name.length() == 0 ) {
        output->fatal(CALL_INFO, 1, "Error: Name given to findComponentIdByName is null\n");
        return Id;
    }

    // first, find the component pointer from the name
    Comp = graph->findComponentByName(Name);
    if ( Comp == nullptr ) {
        output->fatal(CALL_INFO, 1, "Error finding component ID by name: %s\n", Name.c_str());
        return Id;
    }

    return Comp->id;
}

void
SSTSIMDJSONModelDefinition::parseProgramOptions(dom::element& document)
{
    auto prog_opts = document["program_options"];
    if ( prog_opts.error() == NO_SUCH_FIELD ) {
        output->fatal(CALL_INFO, 1, "Error discovering program options from script: %s\n", scriptName.c_str());
    }

    for ( auto [key, value] : document["program_options"].get_object() ) {
        if ( !value.is_null() ) {
            setOptionFromModel(std::string(key), std::string(value.get_string().value()));
        }
    }
}

void
SSTSIMDJSONModelDefinition::parseStatGroupOptions(dom::element& document)
{
    std::string Name;
    std::string StatName;

    if ( document["statistics_group"].error() == NO_SUCH_FIELD ) {
        return;
    }
    else if ( document["statistics_group"].is_null() ) {
        return;
    }

    for ( auto stat_group : document["statistics_group"] ) {
        // -- name
        auto stat_group_name = stat_group["name"];
        if ( stat_group_name.error() ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering statistics group name from script: %s\n", scriptName.c_str());
        }
        Name = stat_group_name.get_string().value();

        auto* csg = graph->getStatGroup(Name);
        if ( csg == nullptr ) {
            output->fatal(CALL_INFO, 1, "Error creating statistics group from script %s; name=%s\n", scriptName.c_str(),
                Name.c_str());
        }

        // -- frequency
        auto stat_group_freq = stat_group["frequency"];
        if ( stat_group_freq.error() != NO_SUCH_FIELD ) {
            if ( !csg->setFrequency(std::string(stat_group_freq.get_string().value())) ) {
                output->fatal(CALL_INFO, 1, "Error setting frequency for statistics group: %s\n", Name.c_str());
            }
        }

        // -- output
        auto stat_outs = stat_group["output"];
        if ( stat_outs.error() != NO_SUCH_FIELD ) {
            auto& statOuts       = graph->getStatOutputs();
            auto  stat_outs_type = stat_group["output"]["type"];
            if ( stat_outs_type.error() == NO_SUCH_FIELD ) {
                output->fatal(
                    CALL_INFO, 1, "Error discovering statistics group output type for group: %s\n", Name.c_str());
            }
            statOuts.emplace_back(ConfigStatOutput(std::string(stat_outs_type.get_string().value())));
            csg->setOutput(statOuts.size() - 1);

            auto out_params = stat_group["output"]["params"];
            if ( out_params.error() != NO_SUCH_FIELD ) {
                for ( auto [key, value] : stat_group["output"]["params"].get_object() ) {
                    statOuts.back().addParameter(std::string(key), std::string(value.get_string().value()));
                }
            }
        }

        // -- statistics
        for ( auto stats : stat_group["statistics"] ) {
            auto stats_name = stats["name"];
            if ( stats_name.error() == NO_SUCH_FIELD ) {
                output->fatal(
                    CALL_INFO, 1, "Error discovering statistics group stat name from script: %s\n", scriptName.c_str());
            }
            StatName = stats_name.get_string().value();

            Params StatParams;
            auto   stat_params = stats["params"];
            if ( stat_params.error() != NO_SUCH_FIELD ) {
                for ( auto [key, value] : stats["params"].get_object() ) {
                    StatParams.insert(std::string(key), std::string(value.get_string().value()));
                }
            }

            csg->addStatistic(StatName, StatParams);

            bool        verified = false;
            std::string reason;
            std::tie(verified, reason) = csg->verifyStatsAndComponents(graph);
            if ( !verified ) {
                output->fatal(CALL_INFO, 1, "Error verifying statistics and components: %s\n", reason.c_str());
            }
        }

        // -- components
        auto comp_stat_group = stat_group["components"];
        if ( comp_stat_group.error() != NO_SUCH_FIELD && !comp_stat_group.is_null() ) {
            for ( auto comp : stat_group["components"] ) {
                csg->addComponent(findComponentIdByName(std::string(comp.get_string().value())));
            }
        }
    }
}

void
SSTSIMDJSONModelDefinition::parseStatOptions(dom::element& document)
{
    // discover the global statistics options
    auto stat_opt = document["statistics_options"];
    if ( stat_opt.error() != NO_SUCH_FIELD ) {
        for ( auto [key, value] : document["statistics_options"].get_object() ) {
            if ( std::string(key) == "statisticLoadLevel" ) {
                graph->setStatisticLoadLevel((int)(value.get_int64()));
            }
            else if ( std::string(key) == "statisticOutput" ) {
                graph->setStatisticOutput(std::string(value.get_string().value()));
            }
            else if ( std::string(key) == "params" ) {
                for ( auto [pkey, pvalue] : document["statistics_options"]["params"].get_object() ) {
                    graph->addStatisticOutputParameter(std::string(pkey), std::string(pvalue.get_string().value()));
                }
            }
        }
    }

    // discover the statistics groups
    parseStatGroupOptions(document);
}

void
SSTSIMDJSONModelDefinition::parseSharedParams(dom::element& document)
{
    std::string SharedName;

    if ( document["shared_params"].error() == NO_SUCH_FIELD ) {
        return;
    }
    else if ( document["shared_params"].is_null() ) {
        return;
    }

    for ( auto shared : document["shared_params"] ) {
        SharedName = shared.get_string().value();
        for ( auto [key, value] : shared[SharedName].get_object() ) {
            graph->addSharedParam(SharedName, std::string(key), std::string(value.get_string().value()));
        }
    }
}

void
SSTSIMDJSONModelDefinition::recursiveSubcomponent(ConfigComponent* Parent, dom::object comp)
{
    std::string      Name;
    std::string      Type;
    std::string      StatName;
    ConfigComponent* Comp = nullptr;
    int              Slot = 0;

    auto subcomp_objs = comp["subcomponents"];
    if ( subcomp_objs.error() == NO_SUCH_FIELD || subcomp_objs.is_null() ) {
        return;
    }

    for ( auto subcomps : comp["subcomponents"] ) {
        // -- Slot Name
        auto subcomp_slot = subcomps["slot_name"];
        if ( subcomp_slot.error() == NO_SUCH_FIELD ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering subcomponent slot name from script: %s\n", scriptName.c_str());
        }
        Name = subcomp_slot.get_string().value();

        // -- Type
        auto subcomp_type = subcomps["type"];
        if ( subcomp_type.error() == NO_SUCH_FIELD ) {
            output->fatal(CALL_INFO, 1, "Error discovering subcomponent type from script: %s\n", scriptName.c_str());
        }
        Type = subcomp_type.get_string().value();

        // -- Slot Index
        auto subcomp_idx = subcomps["slot_number"];
        if ( subcomp_idx.error() == NO_SUCH_FIELD ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering subcomponent slot number from script: %s\n", scriptName.c_str());
        }
        Slot = (int)(subcomp_idx.get_int64());

        // add the subcomponent
        Comp = Parent->addSubComponent(Name, Type, Slot);


        // read all the parameters
        auto param_result = subcomps["params"];
        if ( param_result.error() != NO_SUCH_FIELD ) {
            for ( auto [key, value] : subcomps["params"].get_object() ) {
                Comp->addParameter(std::string(key), std::string(value.get_string().value()), false);
            }
        }

        // read all the shared parameters
        auto sharedset_result = subcomps["params_shared_sets"];
        if ( sharedset_result.error() != NO_SUCH_FIELD ) {
            for ( auto sharedParamSet : subcomps["params_shared_sets"].get_object() ) {
                Comp->addSharedParamSet(std::string(sharedParamSet.value.get_string().value()));
            }
        }

        // read the statistics
        auto stat_result = subcomps["statistics"];
        if ( stat_result.error() != NO_SUCH_FIELD ) {
            for ( auto stats : subcomps["statistics"] ) {
                auto stats_name = stats["name"];
                if ( stats_name.error() ) {
                    output->fatal(
                        CALL_INFO, 1, "Error discovering component stat name from script: %s\n", scriptName.c_str());
                }
                StatName = stats_name.get_string().value();

                Params StatParams;
                auto   stat_params = stats["params"];
                if ( stat_params.error() != NO_SUCH_FIELD ) {
                    for ( auto [key, value] : stats["params"].get_object() ) {
                        StatParams.insert(std::string(key), std::string(value.get_string().value()));
                    }
                }
                Comp->enableStatistic(StatName, StatParams);
            }
        }

        // recursively build the up the subcomponents
        recursiveSubcomponent(Comp, subcomps);
    }
}

void
SSTSIMDJSONModelDefinition::parseComponents(dom::element& document)
{
    std::string      Name;
    std::string      Type;
    std::string      StatName;
    ComponentId_t    Id;
    ConfigComponent* Comp   = nullptr;
    uint32_t         rank   = 0;
    uint32_t         thread = 0;

    auto comp_result = document["components"];
    if ( comp_result.error() == NO_SUCH_FIELD || comp_result.is_null() ) {
        output->fatal(CALL_INFO, 1, "Error, no \"components\" section in json file: %s\n", scriptName.c_str());
    }

    for ( auto comp_elem : document["components"] ) {

        // -- Name
        auto comp_value = comp_elem["name"];
        if ( comp_value.error() == NO_SUCH_FIELD ) {
            output->fatal(CALL_INFO, 1, "Error discovering link name from script: %s\n", scriptName.c_str());
        }
        Name = comp_value.get_string().value();

        // -- Type
        auto type_value = comp_elem["type"];
        if ( type_value.error() == NO_SUCH_FIELD ) {
            output->fatal(CALL_INFO, 1, "Error discovering component type from script: %s\n", scriptName.c_str());
        }
        Type = type_value.get_string().value();

        // Add the component so we have the ComponentID
        Id   = graph->addComponent(Name, Type);
        Comp = graph->findComponent(Id);

        // read all the parameters
        auto param_result = comp_elem["params"];
        if ( param_result.error() != NO_SUCH_FIELD ) {
            for ( auto [key, value] : comp_elem["params"].get_object() ) {
                Comp->addParameter(std::string(key), std::string(value.get_string().value()), false);
            }
        }

        // read all the shared parameters
        auto sparam_result = comp_elem["params_shared_sets"];
        if ( sparam_result.error() != NO_SUCH_FIELD ) {
            for ( auto sharedParamSet : comp_elem["params_shared_sets"].get_object() ) {
                Comp->addSharedParamSet(std::string(sharedParamSet.value.get_string().value()));
            }
        }

        // read the partition info
        auto part_result = comp_elem["partition"];
        if ( part_result.error() != NO_SUCH_FIELD ) {
            for ( auto [key, value] : comp_elem["partition"].get_object() ) {
                if ( std::string(key) == "rank" ) {
                    rank = (uint32_t)(value.get_uint64());
                }
                else if ( std::string(key) == "thread" ) {
                    thread = (uint32_t)(value.get_uint64());
                }
            }
        }

        // read the statistics
        auto stat_result = comp_elem["statistics"];
        if ( stat_result.error() != NO_SUCH_FIELD ) {
            for ( auto stats : comp_elem["statistics"] ) {
                auto stats_name = stats["name"];
                if ( stats_name.error() ) {
                    output->fatal(
                        CALL_INFO, 1, "Error discovering component stat name from script: %s\n", scriptName.c_str());
                }
                StatName = stats_name.get_string().value();

                Params StatParams;
                auto   stat_params = stats["params"];
                if ( stat_params.error() != NO_SUCH_FIELD ) {
                    for ( auto [key, value] : stats["params"].get_object() ) {
                        StatParams.insert(std::string(key), std::string(value.get_string().value()));
                    }
                }
                Comp->enableStatistic(StatName, StatParams);
            }
        }

        // set the rank information
        RankInfo Rank(rank, thread);
        Comp->setRank(Rank);

        // Recursive subcomponent parser
        recursiveSubcomponent(Comp, comp_elem);

        // reset the variables
        Id     = -1;
        Comp   = nullptr;
        rank   = 0;
        thread = 0;
    }
}

void
SSTSIMDJSONModelDefinition::parseLinks(dom::element& document)
{
    std::string Name;
    bool        NoCut       = false;
    bool        NonLocal    = false;
    int         RightRank   = -1;
    int         RightThread = -1;

    std::string       Comp[2];
    std::string       Port[2];
    std::string       Latency[2];
    const std::string sides[2] = { "left", "right" };

    auto links = document["links"];
    if ( links.error() == NO_SUCH_FIELD || links.is_null() ) {
        return;
    }

    for ( auto link : document["links"] ) {
        // -- name
        auto value = link["name"];
        if ( value.error() == NO_SUCH_FIELD ) {
            output->fatal(CALL_INFO, 1, "Error discovering link name from script: %s\n", scriptName.c_str());
        }
        Name = value.get_string().value();

        // -- noCut
        value = link["noCut"];
        if ( value.error() == NO_SUCH_FIELD ) {
            NoCut = false;
        }
        else {
            NoCut = value.get_bool();
        }

        // -- nolocal
        value = link["nonlocal"];
        if ( value.error() == NO_SUCH_FIELD ) {
            NonLocal = false;
        }
        else {
            NonLocal = value.get_bool();
        }

        // -- Component connections
        for ( int i = 0; i < 2; ++i ) {
            auto side_value = link[sides[i]];
            if ( side_value.error() == NO_SUCH_FIELD ) {
                output->fatal(CALL_INFO, 1, "Error discovering %s link component for Link=%s from script: %s\n",
                    sides[i].c_str(), Name.c_str(), scriptName.c_str());
            }
            auto side_obj = link[sides[i]].get_object();
            auto comp_val = side_obj["component"];
            if ( comp_val.error() == NO_SUCH_FIELD ) {
                output->fatal(CALL_INFO, 1,
                    "Error finding component field of %s link component for Link=%s from script: %s\n",
                    sides[i].c_str(), Name.c_str(), scriptName.c_str());
            }
            Comp[i] = comp_val.get_string().value();

            auto port_val = side_obj["port"];
            if ( port_val.error() == NO_SUCH_FIELD ) {
                output->fatal(CALL_INFO, 1,
                    "Error finding port field of %s link component for Link=%s from script: %s\n", sides[i].c_str(),
                    Name.c_str(), scriptName.c_str());
            }
            Port[i] = port_val.get_string().value();

            auto lat_val = side_obj["latency"];
            if ( lat_val.error() == NO_SUCH_FIELD ) {
                output->fatal(CALL_INFO, 1,
                    "Error finding latency field of %s link component for Link=%s from script: %s\n", sides[i].c_str(),
                    Name.c_str(), scriptName.c_str());
            }
            Latency[i] = lat_val.get_string().value();

            // RHS of the link and NonLocal
            if ( (i == 1) && NonLocal ) {
                auto right_rank = side_obj["rank"];
                if ( right_rank.error() == NO_SUCH_FIELD ) {
                    output->fatal(CALL_INFO, 1,
                        "Error finding rank field of %s right-hand-side non-local link component for Link=%s from "
                        "script: %s\n",
                        sides[i].c_str(), Name.c_str(), scriptName.c_str());
                }

                RightRank = (int)(right_rank.get_int64());

                auto right_thread = side_obj["thread"];
                if ( right_thread.error() == NO_SUCH_FIELD ) {
                    output->fatal(CALL_INFO, 1,
                        "Error finding thread field of %s right-hand-side non-local link component for Link=%s from "
                        "script: %s\n",
                        sides[i].c_str(), Name.c_str(), scriptName.c_str());
                }

                RightThread = (int)(right_thread.get_int64());
            }
        }

        // create the link(s)
        LinkId_t link_id = graph->createLink(Name.c_str(), nullptr);
        if ( NoCut ) graph->setLinkNoCut(link_id);

        // left side
        ComponentId_t CompID = -1;
        CompID               = findComponentIdByName(Comp[0]);
        graph->addLink(CompID, link_id, Port[0].c_str(), Latency[0].c_str());

        // right side
        if ( NonLocal ) {
            graph->addNonLocalLink(link_id, RightRank, RightThread);
        }
        else {
            CompID = findComponentIdByName(Comp[1]);
            graph->addLink(CompID, link_id, Port[1].c_str(), Latency[1].c_str());
        }

        NoCut       = false;
        NonLocal    = false;
        RightRank   = -1;
        RightThread = -1;
    }
}

ConfigGraph*
SSTSIMDJSONModelDefinition::createConfigGraph()
{
    ondemand::parser parser;
    dom::parser      dparser;
    padded_string    json     = padded_string::load(scriptName);
    dom::element     document = dparser.parse(json);

    // parse the program options
    parseProgramOptions(document);

    // parse the statistics options
    parseStatOptions(document);

    // parse the shared parameters
    parseSharedParams(document);

    // parse the components
    parseComponents(document);

    // parse the links
    parseLinks(document);

    return graph;
}
