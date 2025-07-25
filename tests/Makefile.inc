# -*- Makefile -*-
#
#

EXTRA_DIST += \
    tests/testsuite_default_Checkpoint.py \
    tests/testsuite_default_Component.py \
    tests/testsuite_default_ComponentExtension.py \
    tests/testsuite_default_Links.py \
    tests/testsuite_default_MemPoolTest.py \
    tests/testsuite_default_Module.py \
    tests/testsuite_default_Output.py \
    tests/testsuite_default_ParamComponent.py \
    tests/testsuite_default_PerfComponent.py \
    tests/testsuite_default_profiling.py \
    tests/testsuite_default_RNGComponent.py \
    tests/testsuite_default_RealTime.py \
    tests/testsuite_default_SharedObject.py \
    tests/testsuite_default_Serialization.py \
    tests/testsuite_default_SubComponent.py \
    tests/testsuite_default_UnitAlgebra.py \
    tests/testsuite_default_config_input_output.py \
    tests/testsuite_default_partitioner.py \
    tests/testsuite_default_PortModule.py \
    tests/testsuite_testengine_testing.py \
    tests/test_Checkpoint.py \
    tests/test_Component.py \
    tests/test_Component_time_overflow.py \
    tests/test_ComponentExtension.py \
    tests/test_ClockerComponent.py \
    tests/test_DistribComponent_discrete.py \
    tests/test_DistribComponent_expon.py \
    tests/test_DistribComponent_gaussian.py \
    tests/test_DistribComponent_poisson.py \
    tests/test_LookupTable.py \
    tests/test_LookupTable2.py \
    tests/test_MessageMesh.py \
    tests/test_Output.py \
    tests/test_ParamComponent.py \
    tests/test_ParallelLoad.py \
    tests/test_RNGComponent_mersenne.py \
    tests/test_RNGComponent_marsaglia.py \
    tests/test_RNGComponent_xorshift.py \
    tests/test_Serialization.py \
    tests/test_SharedObject.py \
    tests/test_StatisticsComponent.py \
    tests/test_StatisticsComponent_basic.py \
    tests/test_Links.py \
    tests/test_MessageGeneratorComponent.py \
    tests/test_MemPool_overflow.py \
    tests/test_MemPool_undeleted_items.py \
    tests/test_Module.py \
    tests/test_PortModule.py \
    tests/test_RealTime.py \
    tests/test_SubComponent.py \
    tests/test_SubComponent_2.py \
    tests/test_UnitAlgebra.py \
    tests/test_PythonUnitAlgebra.py \
    tests/test_PerfComponent.py \
    tests/refFiles/test_Component.out \
    tests/refFiles/test_Component_time_overflow.out \
    tests/refFiles/test_ComponentExtension.out \
    tests/refFiles/test_PerfComponent.out \
    tests/refFiles/test_DistribComponent_discrete.out \
    tests/refFiles/test_DistribComponent_expon.out \
    tests/refFiles/test_DistribComponent_gaussian.out \
    tests/refFiles/test_LookupTableComponent.out \
    tests/refFiles/test_Output_TraceFunction.out \
    tests/refFiles/test_Output_TraceFunction_IndentMarker.out \
    tests/refFiles/test_ParamComponent.out \
    tests/refFiles/test_PortModule_drop.out \
    tests/refFiles/test_PortModule_modify.out \
    tests/refFiles/test_PortModule_pass.out \
    tests/refFiles/test_PortModule_replace.out \
    tests/refFiles/test_PortModule_randomdrop_recv.out \
    tests/refFiles/test_PortModule_randomdrop_send.out \
    tests/refFiles/test_Profiling_event_global.out \
    tests/refFiles/test_Profiling_event_type.out \
    tests/refFiles/test_Profiling_event_component.out \
    tests/refFiles/test_Profiling_event_subcomponent.out \
    tests/refFiles/test_MessageGeneratorComponent.out \
    tests/refFiles/test_MemPool_overflow.out \
    tests/refFiles/test_MemPool_undeleted_items.out \
    tests/refFiles/test_RNGComponent_marsaglia.out \
    tests/refFiles/test_RNGComponent_mersenne.out \
    tests/refFiles/test_RNGComponent_xorshift.out \
    tests/refFiles/test_StatisticsComponent_basic.out \
    tests/refFiles/test_StatisticsComponent_basic_h5.out \
    tests/refFiles/test_StatisticsComponent_basic_group_stats.csv \
    tests/refFiles/test_StatisticsComponent_basic_group_stats.h5 \
    tests/refFiles/test_StatisticsComponent_basic_group_stats.txt \
    tests/refFiles/test_Links_basic.out \
    tests/refFiles/test_Links_dangling.out \
    tests/refFiles/test_Links_wrong_port.out \
    tests/refFiles/test_Module.out \
    tests/refFiles/test_Serialization.out \
    tests/refFiles/test_Serialization_componentinfo.out \
    tests/refFiles/test_Serialization_default.out \
    tests/refFiles/test_Serialization_handler.out \
    tests/refFiles/test_SubComponent_2.out \
    tests/refFiles/test_SubComponent.out \
    tests/refFiles/test_UnitAlgebra.out \
    tests/refFiles/test_PythonUnitAlgebra.out \
    tests/refFiles/test_Checkpoint.out \
    tests/refFiles/test_SharedObject_array.out	\
    tests/refFiles/test_SharedObject_map.out \
    tests/refFiles/test_SharedObject_bool_array.out \
    tests/refFiles/test_SharedObject_set.out \
    tests/subcomponent_tests/test_sc_2a.py \
    tests/subcomponent_tests/test_sc_2u2u.py \
    tests/subcomponent_tests/test_sc_2u.py \
    tests/subcomponent_tests/test_sc_a.py \
    tests/subcomponent_tests/test_sc_u2u.py \
    tests/subcomponent_tests/test_sc_u.py \
    tests/subcomponent_tests/test_sc_2u2a.py \
    tests/subcomponent_tests/test_sc_2ua.py \
    tests/subcomponent_tests/test_sc_2uu.py \
    tests/subcomponent_tests/test_sc_u2a.py \
    tests/subcomponent_tests/test_sc_ua.py \
    tests/subcomponent_tests/test_sc_uu.py \
    tests/subcomponent_tests/refFiles/test_sc_2a.out \
    tests/subcomponent_tests/refFiles/test_sc_2u.out \
    tests/subcomponent_tests/refFiles/test_sc_2u2a.out \
    tests/subcomponent_tests/refFiles/test_sc_2u2u.out \
    tests/subcomponent_tests/refFiles/test_sc_2ua.out \
    tests/subcomponent_tests/refFiles/test_sc_2uu.out \
    tests/subcomponent_tests/refFiles/test_sc_a.out \
    tests/subcomponent_tests/refFiles/test_sc_u.out \
    tests/subcomponent_tests/refFiles/test_sc_u2a.out \
    tests/subcomponent_tests/refFiles/test_sc_u2u.out \
    tests/subcomponent_tests/refFiles/test_sc_ua.out \
    tests/subcomponent_tests/refFiles/test_sc_uu.out

deprecated_EXTRA_DIST =

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif

##########################################################################
##########################################################################
##########################################################################


