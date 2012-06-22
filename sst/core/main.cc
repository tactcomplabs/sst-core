// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization/core.h"

#include <boost/mpi.hpp>
#include <boost/mpi/timer.hpp>

#include <signal.h>

#include <iomanip>
#include <iostream>
#include <fstream>

#include <sst/core/archive.h>
#include <sst/core/config.h>
#include <sst/core/element.h>
#include <sst/core/factory.h>
#include <sst/core/configGraph.h>
#include <sst/core/zolt.h>
#include <sst/core/simulation.h>
#include <sst/core/action.h>
#include <sst/core/activity.h>

using namespace std;
using namespace SST;

static void
sigHandlerPrintStatus1(int signal)
{
    Simulation::printStatus(false);
}

static void
sigHandlerPrintStatus2(int signal)
{
    Simulation::printStatus(true);
}

int 
main(int argc, char *argv[])
{
    boost::mpi::environment* mpiEnv = new boost::mpi::environment(argc,argv);
    boost::mpi::communicator world;

    Config cfg(world.rank());
    SST::Simulation*  sim= NULL;

    // All ranks parse the command line
    if ( cfg.parse_cmd_line(argc, argv) ) {
	delete mpiEnv;
        return -1;
    }
    // In fast build mode, everyone builds the entire graph structure
    // (saving the slow broadcast).  In non-fast, only rank 0 will
    // parse the sdl and build the graph.  It is then broadcast.  In
    // single rank mode, the option is ignored.
    sdl_parser* parser;
    if ( cfg.sdlfile != "NONE" ) {
	if ( cfg.all_parse || world.rank() == 0 ) {
	    // Create the sdl parser
	    parser = new sdl_parser(cfg.sdlfile);
	    
	    string config_string = parser->getSDLConfigString();
	    cfg.parse_config_file(config_string);
	    // cfg.print();

	}

	// If this is a parallel job, we need to broadcast the configuration
	if ( world.size() > 1 && !cfg.all_parse ) {
	    broadcast(world,cfg,0);
	}
    }
	
    
    Archive archive(cfg.archiveType, cfg.archiveFile);

    boost::mpi::timer* timer = new boost::mpi::timer();
    double start = timer->elapsed();
    double end_build, start_run, end_run;
            
    if ( cfg.verbose) printf("# main() My rank is %d, on %d nodes\n", world.rank(), world.size());
    DebugInit( world.rank(), world.size() );

    if ( cfg.runMode == Config::INIT || cfg.runMode == Config::BOTH ) { 
        sim = Simulation::createSimulation(&cfg, world.rank(), world.size());

        signal(SIGUSR1, sigHandlerPrintStatus1);
        signal(SIGUSR2, sigHandlerPrintStatus2);

	ConfigGraph* graph;
	
	if ( world.size() == 1 ) {
	    if ( cfg.generator != "NONE" ) {
		generateFunction func = sim->getFactory()->GetGenerator(cfg.generator);
		graph = new ConfigGraph();
		func(graph,cfg.generator_options,world.size());
	    }
	    else {
		graph = parser->createConfigGraph();
	    }
	    // Set all components to be instanced on rank 0 (the only
	    // one that exists)
	    graph->setComponentRanks(0);
	}
	// Need to worry about partitioning for parallel jobs
	else if ( world.rank() == 0 || cfg.all_parse ) {
	    if ( cfg.generator != "NONE" ) {
		graph = new ConfigGraph();
		generateFunction func = sim->getFactory()->GetGenerator(cfg.generator);
		func(graph,cfg.generator_options, world.size());
	    }
	    else {
		graph = parser->createConfigGraph();
	    }
	    
	    // Do the partitioning.
	    if ( cfg.partitioner != "self" ) {
		// If partitioning not specified by sdl or generator,
		// set all component ranks to -1 so it's easier to
		// detect some types of partitioning errors
		graph->setComponentRanks(-1);
	    }
	    
	    if ( cfg.partitioner == "self" ) {
		// For now, do nothing.  Eventually we need to
		// have a checker for the partitioning.
	    }
	    else if ( cfg.partitioner == "zoltan" ) {
		printf("Zoltan support is currently not available, aborting...\n");
		abort();
	    }
	    else {
		partitionFunction func = sim->getFactory()->GetPartitioner(cfg.partitioner);
		func(graph,world.size());
	    }
	}
	else {
	    graph = new ConfigGraph();
	}

	// If the user asks us to dump the partionned graph.
	if(cfg.dump_component_graph_file != "" && world.rank() == 0) {
		if(cfg.verbose) 
			std::cout << "# Dumping partitionned component graph to " <<
				cfg.dump_component_graph_file << std::endl;

		ofstream graph_file(cfg.dump_component_graph_file.c_str());
		ConfigComponentMap_t& component_map = graph->getComponentMap();

		for(int i = 0; i < world.size(); i++) {
			graph_file << "Rank: " << i << " Component List:" << std::endl;

			for (ConfigComponentMap_t::const_iterator j = component_map.begin() ; j != component_map.end() ; ++j) {
	   		 	if((*(*j).second).rank == i) {
					graph_file << "   " << (*(*j).second).name << " (ID=" << (*(*j).second).id << ")" << std::endl;
					graph_file << "      -> type      " << (*(*j).second).type << std::endl;
					graph_file << "      -> weight    " << (*(*j).second).weight << std::endl;
					graph_file << "      -> linkcount " << (*(*j).second).links.size() << std::endl;
				}
			}
		}

		graph_file.close();

		if(cfg.verbose) 
			std::cout << "# Dump of partition graph is complete." << std::endl;
	}

	// Broadcast the data structures if only rank 0 built the
	// graph
	if ( !cfg.all_parse ) broadcast(world, *graph, 0);
	
	if ( !graph->checkRanks( world.size() ) ) {
	    if ( world.rank() == 0 ) {
		std::cout << "ERROR: bad partitioning; partition included bad ranks." << endl;
	    }
	    exit(1);
	}
	else {
	    if ( !graph->containsComponentInRank( world.rank() ) ) {
		std::cout << "WARNING: no components assigned to rank: " << world.rank() << "." << endl;
	    }
	}
	
	sim->performWireUp( *graph, world.rank() );
	delete graph;
    }

    end_build = timer->elapsed();
    double build_time = end_build - start;
    // std::cout << "#  Build time: " << build_time << " s" << std::endl;

    double max_build_time;
    all_reduce(world, &build_time, 1, &max_build_time, boost::mpi::maximum<double>() );

    start_run = timer->elapsed();
    
    if ( cfg.runMode == Config::RUN || cfg.runMode == Config::BOTH ) { 
        if ( cfg.archive ) {
            sim = archive.LoadSimulation();
	    printf("# Finished reading serialization file\n");
        }
	
	if ( cfg.verbose ) printf("# Starting main event loop\n");
        sim->Run();


	delete sim;
    }

    end_run = timer->elapsed();

    double run_time = end_run - start_run;
    double total_time = end_run - start;

    double max_run_time, max_total_time;

    all_reduce(world, &run_time, 1, &max_run_time, boost::mpi::maximum<double>() );
    all_reduce(world, &total_time, 1, &max_total_time, boost::mpi::maximum<double>() );

    if ( world.rank() == 0 && cfg.verbose ) {
	std::cout << setiosflags(ios::fixed) << setprecision(2);
	std::cout << "#" << endl << "# Simulation times" << endl;
	std::cout << "#  Build time: " << max_build_time << " s" << std::endl;
	std::cout << "#  Simulation time: " << max_run_time << " s" << std::endl;
	std::cout << "#  Total time: " << max_total_time << " s" << std::endl;
    }

    delete mpiEnv;
    return 0;
}

