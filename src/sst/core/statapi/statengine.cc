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

#include "sst/core/statapi/statengine.h"

#include "sst/core/baseComponent.h"
#include "sst/core/configGraph.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/factory.h"
#include "sst/core/impl/oneshotManager.h"
#include "sst/core/output.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/warnmacros.h"

#include <algorithm>
#include <string>

namespace SST::Statistics {

std::vector<StatisticOutput*> StatisticProcessingEngine::m_statOutputs;

StatisticProcessingEngine::StatisticProcessingEngine() :
    m_output(Output::getDefaultObject())
{}


void
StatisticProcessingEngine::static_setup(StatsConfig* stats_config)
{
    // Outputs are per MPI rank, so have to be static data
    for ( auto& cfg : stats_config->outputs ) {
        m_statOutputs.push_back(createStatisticOutput(cfg));
    }
}

void
StatisticProcessingEngine::stat_outputs_simulation_start()
{
    for ( auto& so : m_statOutputs ) {
        so->startOfSimulation();
    }
}

void
StatisticProcessingEngine::stat_outputs_simulation_end()
{
    for ( auto& so : m_statOutputs ) {
        so->endOfSimulation();
    }
}


void
StatisticProcessingEngine::setup(Simulation_impl* sim, StatsConfig* stats_config)
{
    m_sim = sim;

    m_SimulationStarted = false;
    m_statLoadLevel     = stats_config->load_level;

    m_defaultGroup.output = m_statOutputs[0];
    for ( auto& cfg : stats_config->groups ) {
        m_statGroups.emplace_back(cfg.second, this);
    }
}

void
StatisticProcessingEngine::restart(Simulation_impl* sim)
{
    m_sim                 = sim;
    m_SimulationStarted   = false;
    m_defaultGroup.output = m_statOutputs[0];
    for ( std::vector<StatisticGroup>::iterator it = m_statGroups.begin(); it != m_statGroups.end(); it++ ) {
        it->restartGroup(this);
    }
}

StatisticProcessingEngine::~StatisticProcessingEngine()
{
    StatArray_t*   statArray;
    StatisticBase* stat;

    // Destroy all the Statistics that have been created
    for ( CompStatMap_t::iterator it_m = m_CompStatMap.begin(); it_m != m_CompStatMap.end(); it_m++ ) {
        // Get the Array for this Map Item
        statArray = it_m->second;

        // Walk the stat Array and delete each stat
        for ( StatArray_t::iterator it_v = statArray->begin(); it_v != statArray->end(); it_v++ ) {
            stat = *it_v;
            delete stat;
        }
    }
}

bool
StatisticProcessingEngine::registerStatisticCore(StatisticBase* stat)
{
    if ( stat->isNullStatistic() ) return true;
    auto* comp = stat->getComponent();
    if ( comp == nullptr ) {
        m_output.verbose(CALL_INFO, 1, 0, " Error: Statistic %s hasn't any associated component .\n",
            stat->getFullStatName().c_str());
        return false;
    }

    StatisticGroup& group = getGroupForStatistic(stat);
    if ( group.isDefault ) {
        // If the mode is Periodic Based, the add the statistic to the
        // StatisticProcessingEngine otherwise add it as an Event Based Stat.
        UnitAlgebra collectionRate = stat->getCollectionRate();
        bool        success        = true;
        switch ( stat->getRegisteredCollectionMode() ) {
        case StatisticBase::STAT_MODE_PERIODIC:
            success = addPeriodicBasedStatistic(collectionRate, stat);
            break;
        case StatisticBase::STAT_MODE_COUNT:
            success = addEventBasedStatistic(collectionRate, stat);
            break;
        case StatisticBase::STAT_MODE_DUMP_AT_END:
            success = addEndOfSimStatistic(stat);
            break;
        case StatisticBase::STAT_MODE_UNDEFINED:
            m_output.fatal(
                CALL_INFO, 1, "Stat mode is undefined for %s in registerStatistic", stat->getFullStatName().c_str());
            break;
        }
        if ( !success ) return false;
    }
    else {
        switch ( stat->getRegisteredCollectionMode() ) {
        case StatisticBase::STAT_MODE_PERIODIC:
        case StatisticBase::STAT_MODE_DUMP_AT_END:
            break;
        default:
            m_output.output("ERROR: Statistics in groups must be periodic or dump at end\n");
            return false;
        }
    }

    // Make sure that the wireup has not been completed
    // If it has, stat output must support dynamic registration
    if ( true == Simulation_impl::getSimulation()->isWireUpFinished() ) {
        if ( !group.output->supportsDynamicRegistration() ) {
            m_output.fatal(CALL_INFO, 1,
                "ERROR: Statistic %s - "
                "Cannot be registered for output %s after the Components have been wired up. "
                "Statistics on output %s must be registered on Component creation. exiting...\n",
                stat->getFullStatName().c_str(), group.output->getStatisticOutputName().c_str(),
                group.output->getStatisticOutputName().c_str());
        }
    }

    /* All checks pass.  Add the stat */
    group.addStatistic(stat);

    if ( group.isDefault ) {
        getOutputForStatistic(stat)->registerStatistic(stat);
    }

    setStatisticStartTime(stat);
    setStatisticStopTime(stat);

    return true;
}

void
StatisticProcessingEngine::finalizeInitialization()
{
    for ( auto& g : m_statGroups ) {
        g.output->registerGroup(&g);

        /* Register group clock, if rate is set */
        if ( g.outputFreq.getValue() != 0 ) {
            Simulation_impl::getSimulation()->registerClock(g.outputFreq,
                new Clock::Handler2<StatisticProcessingEngine, &StatisticProcessingEngine::handleGroupClockEvent,
                    StatisticGroup*>(this, &g),
                STATISTICCLOCKPRIORITY);
        }
    }
}

void
StatisticProcessingEngine::startOfSimulation()
{
    m_SimulationStarted = true;
}

void
StatisticProcessingEngine::endOfSimulation()
{
    // This is a redundant call to all this code
    // Looping all the statistic groups and outputting them
    // will cause all of this code to be executed anyway
    // so really we are double dumping the end-of-time stats

    // Output the Event based Statistics
    for ( StatisticBase* stat : m_EventStatisticArray ) {
        // Check to see if the Statistic is to output at end of sim
        if ( true == stat->getFlagOutputAtEndOfSim() ) {
            // Perform the output
            performStatisticOutputImpl(stat, true);
        }
    }

    // Output the Periodic Based Statistics
    for ( auto& it_m : m_PeriodicStatisticMap ) {
        // Get the array from the Map Iterator
        StatArray_t* statArray = it_m.second;

        for ( StatisticBase* stat : *statArray ) {
            // Check to see if the Statistic is to output at end of sim
            if ( true == stat->getFlagOutputAtEndOfSim() ) {
                // Perform the output
                performStatisticOutputImpl(stat, true);
            }
        }
    }

    for ( auto& sg : m_statGroups ) {
        performStatisticGroupOutputImpl(sg, true);
    }
}

StatisticOutput*
StatisticProcessingEngine::createStatisticOutput(const ConfigStatOutput& cfg)
{
    auto& unsafeParams = const_cast<SST::Params&>(cfg.params);
    auto  lcType       = cfg.type;
    std::transform(lcType.begin(), lcType.end(), lcType.begin(), ::tolower);
    StatisticOutput* so = Factory::getFactory()->CreateWithParams<StatisticOutput>(lcType, unsafeParams, unsafeParams);
    if ( nullptr == so ) {
        Output::getDefaultObject().fatal(
            CALL_INFO, 1, " - Unable to instantiate Statistic Output %s\n", cfg.type.c_str());
    }

    if ( false == so->checkOutputParameters() ) {
        // If checkOutputParameters() fail, Tell the user how to use them and abort simulation
        Output::getDefaultObject().output("Statistic Output (%s) :\n", so->getStatisticOutputName().c_str());
        so->printUsage();
        Output::getDefaultObject().output("\n");

        Output::getDefaultObject().output("Statistic Output Parameters Provided:\n");
        cfg.params.print_all_params(Output::getDefaultObject(), "  ");
        Output::getDefaultObject().fatal(CALL_INFO, 1, " - Required Statistic Output Parameters not set\n");
    }
    return so;
}

void
StatisticProcessingEngine::castError(const std::string& type, const std::string& statName, const std::string& fieldName)
{
    Simulation_impl::getSimulationOutput().fatal(CALL_INFO, 1,
        "Unable to cast statistic %s of type %s to correct field type %s", statName.c_str(), type.c_str(),
        fieldName.c_str());
}

StatisticOutput*
StatisticProcessingEngine::getOutputForStatistic(const StatisticBase* stat) const
{
    return getGroupForStatistic(stat).output;
}

/* Return the group that would claim this stat */
StatisticGroup&
StatisticProcessingEngine::getGroupForStatistic(const StatisticBase* stat) const
{
    for ( auto& g : m_statGroups ) {
        if ( g.claimsStatistic(stat) ) {
            return const_cast<StatisticGroup&>(g);
        }
    }
    return const_cast<StatisticGroup&>(m_defaultGroup);
}

bool
StatisticProcessingEngine::addEndOfSimStatistic(StatisticBase* /*stat*/)
{
    return true;
}

bool
StatisticProcessingEngine::addPeriodicBasedStatistic(const UnitAlgebra& freq, StatisticBase* stat)
{
    Simulation_impl*    sim      = Simulation_impl::getSimulation();
    TimeConverter*      tcFreq   = sim->getTimeLord()->getTimeConverter(freq);
    SimTime_t           tcFactor = tcFreq->getFactor();
    Clock::HandlerBase* ClockHandler;
    StatArray_t*        statArray;

    // See if the map contains an entry for this factor
    if ( m_PeriodicStatisticMap.find(tcFactor) == m_PeriodicStatisticMap.end() ) {
        // Check to see if the freq is zero.  Only add a new clock if the freq is non zero
        if ( 0 != freq.getValue() ) {

            // This tcFactor is not found in the map, so create a new clock handler.
            ClockHandler = new Clock::Handler2<StatisticProcessingEngine,
                &StatisticProcessingEngine::handleStatisticEngineClockEvent, SimTime_t>(this, tcFactor);

            // Set the clock priority so that normal clocks events will occur before
            // this clock event.
            sim->registerClock(freq, ClockHandler, STATISTICCLOCKPRIORITY);
        }

        // Also create a new Array of Statistics and relate it to the map
        statArray                        = new std::vector<StatisticBase*>();
        m_PeriodicStatisticMap[tcFactor] = statArray;
    }

    // The Statistic Map has the time factor registered.
    statArray = m_PeriodicStatisticMap[tcFactor];

    // Add the statistic to the lists of statistics to be called when the clock fires.
    statArray->push_back(stat);

    return true;
}

bool
StatisticProcessingEngine::addEventBasedStatistic(const UnitAlgebra& count, StatisticBase* stat)
{
    if ( 0 != count.getValue() ) {
        // Set the Count Limit
        stat->setCollectionCountLimit(count.getRoundedValue());
    }
    else {
        stat->setCollectionCountLimit(0);
    }
    stat->setFlagResetCountOnOutput(true);

    // Add the statistic to the Array of Event Based Statistics
    m_EventStatisticArray.push_back(stat);
    return true;
}

void
StatisticProcessingEngine::setStatisticStartTime(StatisticBase* stat)
{
    UnitAlgebra      startTime   = stat->getStartAtTime();
    Simulation_impl* sim         = Simulation_impl::getSimulation();
    TimeConverter*   tcStartTime = sim->getTimeLord()->getTimeConverter(startTime);
    SimTime_t        tcFactor    = tcStartTime->getFactor();
    StatArray_t*     statArray;

    // Check to see if the time is zero or has already passed, if it is we skip this work
    if ( (0 != startTime.getValue()) && (tcFactor > sim->getCurrentSimCycle()) ) {
        // See if the map contains an entry for this factor
        if ( m_StartTimeMap.find(tcFactor) == m_StartTimeMap.end() ) {
            sim->one_shot_manager_.registerAbsoluteHandler<StatisticProcessingEngine,
                &StatisticProcessingEngine::handleStatisticEngineStartTimeEvent, SimTime_t>(
                tcFactor, STATISTICCLOCKPRIORITY, this, tcFactor);

            // Also create a new Array of Statistics and relate it to the map
            statArray                = new std::vector<StatisticBase*>();
            m_StartTimeMap[tcFactor] = statArray;
        }

        // The Statistic Map has the time factor registered.
        statArray = m_StartTimeMap[tcFactor];

        // Add the statistic to the lists of statistics to be called when the OneShot fires.
        statArray->push_back(stat);

        // Disable the Statistic until the start time event occurs
        stat->disable();
    }
}

void
StatisticProcessingEngine::setStatisticStopTime(StatisticBase* stat)
{
    UnitAlgebra      stopTime   = stat->getStopAtTime();
    Simulation_impl* sim        = Simulation_impl::getSimulation();
    TimeConverter*   tcStopTime = sim->getTimeLord()->getTimeConverter(stopTime);
    SimTime_t        tcFactor   = tcStopTime->getFactor();
    StatArray_t*     statArray;

    // Check to see if the time is zero or has already passed, if it is we skip this work
    if ( (0 != stopTime.getValue()) && (tcFactor > sim->getCurrentSimCycle()) ) {
        // See if the map contains an entry for this factor
        if ( m_StopTimeMap.find(tcFactor) == m_StopTimeMap.end() ) {
            // This tcFactor is not found in the map, so create a new OneShot handler.
            sim->one_shot_manager_.registerAbsoluteHandler<StatisticProcessingEngine,
                &StatisticProcessingEngine::handleStatisticEngineStopTimeEvent, SimTime_t>(
                tcFactor, STATISTICCLOCKPRIORITY, this, tcFactor);

            // Also create a new Array of Statistics and relate it to the map
            statArray               = new std::vector<StatisticBase*>();
            m_StopTimeMap[tcFactor] = statArray;
        }

        // The Statistic Map has the time factor registered.
        statArray = m_StopTimeMap[tcFactor];

        // Add the statistic to the lists of statistics to be called when the OneShot fires.
        statArray->push_back(stat);
    }
}

void
StatisticProcessingEngine::performStatisticOutput(StatisticBase* stat, bool endOfSimFlag /*=false*/)
{
    if ( stat->getGroup()->isDefault )
        performStatisticOutputImpl(stat, endOfSimFlag);
    else
        performStatisticGroupOutputImpl(*const_cast<StatisticGroup*>(stat->getGroup()), endOfSimFlag);
}

void
StatisticProcessingEngine::performStatisticOutputImpl(StatisticBase* stat, bool endOfSimFlag /*=false*/)
{
    StatisticOutput* statOutput = getOutputForStatistic(stat);

    // Has the simulation started?
    if ( true == m_SimulationStarted ) {
        // Is the Statistic Output Enabled?
        if ( false == stat->isOutputEnabled() ) {
            return;
        }

        statOutput->output(stat, endOfSimFlag);

        if ( false == endOfSimFlag ) {
            // Check to see if the Statistic Count needs to be reset
            if ( true == stat->getFlagResetCountOnOutput() ) {
                stat->resetCollectionCount();
            }

            // Check to see if the Statistic Data needs to be cleared
            if ( true == stat->getFlagClearDataOnOutput() ) {
                stat->clearStatisticData();
            }
        }
    }
}

void
StatisticProcessingEngine::performStatisticGroupOutputImpl(StatisticGroup& group, bool endOfSimFlag /*=false*/)
{
    StatisticOutput* statOutput = group.output;

    // Has the simulation started?
    if ( true == m_SimulationStarted ) {

        statOutput->outputGroup(&group, endOfSimFlag);

        if ( false == endOfSimFlag ) {
            for ( auto& stat : group.stats ) {
                // Check to see if the Statistic Count needs to be reset
                if ( true == stat->getFlagResetCountOnOutput() ) {
                    stat->resetCollectionCount();
                }

                // Check to see if the Statistic Data needs to be cleared
                if ( true == stat->getFlagClearDataOnOutput() ) {
                    stat->clearStatisticData();
                }
            }
        }
    }
}

void
StatisticProcessingEngine::performGlobalStatisticOutput(bool endOfSimFlag /*=false*/)
{
    StatArray_t*   statArray;
    StatisticBase* stat;

    // Output Event based statistics
    for ( StatArray_t::iterator it_v = m_EventStatisticArray.begin(); it_v != m_EventStatisticArray.end(); it_v++ ) {
        stat = *it_v;
        performStatisticOutputImpl(stat, endOfSimFlag);
    }

    // Output Periodic based statistics
    for ( StatMap_t::iterator it_m = m_PeriodicStatisticMap.begin(); it_m != m_PeriodicStatisticMap.end(); it_m++ ) {
        statArray = it_m->second;

        for ( StatArray_t::iterator it_v = statArray->begin(); it_v != statArray->end(); it_v++ ) {
            stat = *it_v;
            performStatisticOutputImpl(stat, endOfSimFlag);
        }
    }

    for ( auto& sg : m_statGroups ) {
        performStatisticGroupOutputImpl(sg, endOfSimFlag);
    }
}

bool
StatisticProcessingEngine::handleStatisticEngineClockEvent(Cycle_t UNUSED(CycleNum), SimTime_t timeFactor)
{
    StatArray_t*   statArray;
    StatisticBase* stat;
    unsigned int   x;

    // Get the array for the timeFactor
    statArray = m_PeriodicStatisticMap[timeFactor];

    // Walk the array, and call the output method of each statistic
    for ( x = 0; x < statArray->size(); x++ ) {
        stat = statArray->at(x);

        // Perform the output
        performStatisticOutputImpl(stat, false);
    }
    // Return false to keep the clock going
    return false;
}

bool
StatisticProcessingEngine::handleGroupClockEvent(Cycle_t UNUSED(CycleNum), StatisticGroup* group)
{
    performStatisticGroupOutputImpl(*group, false);
    return false;
}

void
StatisticProcessingEngine::handleStatisticEngineStartTimeEvent(SimTime_t timeFactor)
{
    StatArray_t*   statArray;
    StatisticBase* stat;
    unsigned int   x;

    // Get the array for the timeFactor
    statArray = m_StartTimeMap[timeFactor];

    // Walk the array, and call the output method of each statistic
    for ( x = 0; x < statArray->size(); x++ ) {
        stat = statArray->at(x);

        // Enable the Statistic
        stat->enable();
    }
}

void
StatisticProcessingEngine::handleStatisticEngineStopTimeEvent(SimTime_t timeFactor)
{
    StatArray_t*   statArray;
    StatisticBase* stat;
    unsigned int   x;

    // Get the array for the timeFactor
    statArray = m_StopTimeMap[timeFactor];

    // Walk the array, and call the output method of each statistic
    for ( x = 0; x < statArray->size(); x++ ) {
        stat = statArray->at(x);

        // Disable the Statistic
        stat->disable();
    }
}

void
StatisticProcessingEngine::addStatisticToCompStatMap(
    StatisticBase* Stat, StatisticFieldInfo::fieldType_t UNUSED(fieldType))
{
    StatArray_t*  statArray;
    ComponentId_t compId = Stat->getComponent()->getId();

    // See if the map contains an entry for this Component ID
    if ( m_CompStatMap.find(compId) == m_CompStatMap.end() ) {
        // Nope, Create a new Array of Statistics and relate it to the map
        statArray             = new std::vector<StatisticBase*>();
        m_CompStatMap[compId] = statArray;
    }

    // The CompStatMap has Component ID registered, get the array associated with it
    statArray = m_CompStatMap[compId];

    // Add the statistic to the lists of statistics registered to this component
    statArray->push_back(Stat);
}

void
StatisticProcessingEngine::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(m_SimulationStarted);
    SST_SER(m_statLoadLevel);
    SST_SER(m_statGroups); // Going to have to revisit if changing partitioning - will stat groups need to be global?
                           // Are they global already?
}

} // namespace SST::Statistics
