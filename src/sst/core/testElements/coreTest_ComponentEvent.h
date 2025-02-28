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

#ifndef SST_CORE_CORETEST_COMPONENTEVENT_H
#define SST_CORE_CORETEST_COMPONENTEVENT_H

namespace SST::CoreTestComponent {

class coreTestComponentEvent : public SST::Event
{
public:
    using dataVec = std::vector<char>;
    coreTestComponentEvent() : SST::Event() {}
    dataVec payload;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        ser& payload;
    }

    ImplementSerializable(SST::CoreTestComponent::coreTestComponentEvent);
};

} // namespace SST::CoreTestComponent

#endif // SST_CORE_CORETEST_COMPONENTEVENT_H
