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

#ifndef SST_CORE_CORETEST_COMPONENT_H
#define SST_CORE_CORETEST_COMPONENT_H

#include "sst/core/component.h"
#include "sst/core/link.h"
#include "sst/core/rng/marsaglia.h"

#include <cstddef>
#include <cstdio>

namespace SST::CoreTestComponent {

// These first two classes are just base classes to test ELI
// inheritance.  The definition of the ELI items are spread through 2
// component base classes to make sure they get inherited in the
// actual component that can be instanced.
class coreTestComponentBase : public SST::Component
{
public:
    SST_ELI_REGISTER_COMPONENT_BASE(SST::CoreTestComponent::coreTestComponentBase)

    SST_ELI_DOCUMENT_PARAMS(
        { "workPerCycle", "Count of busy work to do during a clock tick.", NULL},
        { "clockFrequency", "Frequency of the clock", "1GHz"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "N", "events sent on N link", "counts", 1 }
    )

    SST_ELI_DOCUMENT_PORTS(
        {"Nlink", "Link to the coreTestComponent to the North", { "coreTestComponent.coreTestComponentEvent", "" } }
    )

    SST_ELI_DOCUMENT_ATTRIBUTES(
        { "test_element", "true" }
    )

    explicit coreTestComponentBase(ComponentId_t id) :
        SST::Component(id)
    {}
    ~coreTestComponentBase() {}
    coreTestComponentBase() :
        SST::Component()
    {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override { SST::Component::serialize_order(ser); }
    ImplementSerializable(SST::CoreTestComponent::coreTestComponentBase)
};

class coreTestComponentBase2 : public coreTestComponentBase
{
public:
    SST_ELI_REGISTER_COMPONENT_DERIVED_BASE(
        SST::CoreTestComponent::coreTestComponentBase2, SST::CoreTestComponent::coreTestComponentBase)

    SST_ELI_DOCUMENT_PARAMS(
        { "commFreq", "There is a 1/commFreq chance each clock cycle of sending an event to a neighbor", NULL}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "S", "events sent on S link", "counts", 1 }
    )

    SST_ELI_DOCUMENT_PORTS(
        {"Slink", "Link to the coreTestComponent to the South", { "coreTestComponent.coreTestComponentEvent", "" } }
    )

    explicit coreTestComponentBase2(ComponentId_t id) :
        coreTestComponentBase(id)
    {}
    ~coreTestComponentBase2() {}

    coreTestComponentBase2() :
        coreTestComponentBase()
    {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::CoreTestComponent::coreTestComponentBase::serialize_order(ser);
    }
    ImplementSerializable(SST::CoreTestComponent::coreTestComponentBase2)
};

class coreTestComponent : public coreTestComponentBase2
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestComponent,
        "coreTestElement",
        "coreTestComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "CoreTest Test Component",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "commSize",     "Size of communication to send.", "16"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "E", "events sent on E link", "counts", 1 },
        { "W", "events sent on W link", "counts", 1 }
    )

    SST_ELI_DOCUMENT_PORTS(
        {"Elink", "Link to the coreTestComponent to the East",  { "coreTestComponent.coreTestComponentEvent", "" } },
        {"Wlink", "Link to the coreTestComponent to the West",  { "coreTestComponent.coreTestComponentEvent", "" } }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    coreTestComponent(SST::ComponentId_t id, SST::Params& params);
    ~coreTestComponent();

    void setup() override {}
    void finish() override { printf("Component Finished.\n"); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTestComponent::coreTestComponent)
    coreTestComponent() = default; // for serialization only

private:
    coreTestComponent(const coreTestComponent&)            = delete; // do not implement
    coreTestComponent& operator=(const coreTestComponent&) = delete; // do not implement

    void         handleEvent(SST::Event* ev);
    virtual bool clockTic(SST::Cycle_t);

    int                 workPerCycle;
    int                 commFreq;
    int                 commSize;
    int                 neighbor;
    SST::Event::id_type last_event_id;

    SST::RNG::MarsagliaRNG*          rng;
    SST::Link*                       N;
    SST::Link*                       S;
    SST::Link*                       E;
    SST::Link*                       W;
    SST::Statistics::Statistic<int>* countN;
    SST::Statistics::Statistic<int>* countS;
    SST::Statistics::Statistic<int>* countE;
    SST::Statistics::Statistic<int>* countW;
};

} // namespace SST::CoreTestComponent

#endif // SST_CORE_CORETEST_COMPONENT_H
