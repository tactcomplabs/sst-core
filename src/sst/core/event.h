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

#ifndef SST_CORE_EVENT_H
#define SST_CORE_EVENT_H

#include "sst/core/activity.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"

#include <atomic>
#include <cinttypes>
#include <string>

namespace SST {

class Link;
class NullEvent;
class RankSync;
class ThreadSync;

namespace pvt {
class DeliveryInfoCompEvent;
} // namespace pvt

/**
 * Base class for Events - Items sent across links to communicate between
 * components.
 */
class Event : public Activity
{
    friend class pvt::DeliveryInfoCompEvent;

public:
    /**
       Base handler for event delivery.
     */
    using HandlerBase = SSTHandlerBase<void, Event*>;

    /**
       Used to create handlers for event delivery.  The callback
       function is expected to be in the form of:

         void func(Event* event)

       In which case, the class is created with:

         new Event::Handler<classname>(this, &classname::function_name)

       Or, to add static data, the callback function is:

         void func(Event* event, dataT data)

       and the class is created with:

         new Event::Handler<classname, dataT>(this, &classname::function_name, data)
     */
    template <typename classT, typename dataT = void>
    using Handler
        [[deprecated("Handler has been deprecated. Please use Handler2 instead as it supports checkpointing.")]] =
            SSTHandler<void, Event*, classT, dataT>;


    /**
       New style (checkpointable) SSTHandler
    */
    template <typename classT, auto funcT, typename dataT = void>
    using Handler2 = SSTHandler2<void, Event*, classT, dataT, funcT>;

    /**
       Class used to sort events during checkpointing.  This is used
       to sort events by delivery_info so we can use std::lower_bound
       to easily find events targeting a specific handler.  For Events
       targetting the same handler, it will sort according to
       Activity::less.  This will ensure that things get inserted back
       in correct order, though the only important thing is that
       insertion order is maintained, so two events to be delivered at
       the same time maintain their send order.

     */
    class less
    {
    public:
        bool operator()(const Event* lhs, const Event* rhs) const
        {
            if ( lhs->delivery_info != rhs->delivery_info ) return lhs->delivery_info < rhs->delivery_info;
            return Activity::less<true, true, true>()(lhs, rhs);
        }
    };


    /** Type definition of unique identifiers */
    using id_type = std::pair<uint64_t, int>;
    /** Constant, default value for id_types */
    static const id_type NO_ID;

    Event() :
        Activity(),
        delivery_info(0)
    {
        setPriority(EVENTPRIORITY);
#if __SST_DEBUG_EVENT_TRACKING__
        first_comp = "";
        last_comp  = "";
#endif
    }
    ~Event() override = default;

    /** Clones the event in for the case of a broadcast */
    virtual Event* clone();


#ifdef __SST_DEBUG_EVENT_TRACKING__

    virtual void printTrackingInfo(const std::string& header, Output& out) const override
    {
        out.output("%s Event first sent from: %s:%s (type: %s) and last received by %s:%s (type: %s)\n", header.c_str(),
            first_comp.c_str(), first_port.c_str(), first_type.c_str(), last_comp.c_str(), last_port.c_str(),
            last_type.c_str());
    }

    const std::string& getFirstComponentName() { return first_comp; }
    const std::string& getFirstComponentType() { return first_type; }
    const std::string& getFirstPort() { return first_port; }
    const std::string& getLastComponentName() { return last_comp; }
    const std::string& getLastComponentType() { return last_type; }
    const std::string& getLastPort() { return last_port; }

    void addSendComponent(const std::string& comp, const std::string& type, const std::string& port)
    {
        if ( first_comp == "" ) {
            first_comp = comp;
            first_type = type;
            first_port = port;
        }
    }
    void addRecvComponent(const std::string& comp, const std::string& type, const std::string& port)
    {
        last_comp = comp;
        last_type = type;
        last_port = port;
    }

#endif

    bool isEvent() const override final { return true; }
    bool isAction() const override final { return false; }

    void copyAllDeliveryInfo(const Activity* act) override final
    {
        Activity::copyAllDeliveryInfo(act);
        const Event* ev = static_cast<const Event*>(act);
        delivery_info   = ev->delivery_info;
    }


    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Activity::serialize_order(ser);
        SST_SER(delivery_info);
#ifdef __SST_DEBUG_EVENT_TRACKING__
        SST_SER(first_comp);
        SST_SER(first_type);
        SST_SER(first_port);
        SST_SER(last_comp);
        SST_SER(last_type);
        SST_SER(last_port);
#endif
    }

protected:
    /**
     * Generates an ID that is unique across ranks, components and events.
     */
    id_type generateUniqueId();


private:
    friend class Link;
    friend class NullEvent;
    friend class RankSync;
    friend class ThreadSync;
    friend class TimeVortex;
    friend class Simulation_impl;


    /** Cause this event to fire */
    void execute() override;

    /**
       This sets the information needed to get the event properly
       delivered for the next step of transfer.

       The tag is used to deterministically sort the events and is
       based off of the sorted link names.  This field is unused for
       events sent across link connected to sync objects.

       For links that are going to a sync, the delivery_info is used
       on the remote side to send the event on the proper link.  For
       local links, delivery_info contains the delivery functor.
       @return void
     */
    inline void setDeliveryInfo(LinkId_t tag, uintptr_t delivery_info)
    {
        setOrderTag(tag);
        this->delivery_info = delivery_info;
    }

    /**
       Update the delivery_info during a restart.  This will fixup the
       handler pointer.

       @param dinfo New handler pointer cast as a uintptr_t
     */
    void updateDeliveryInfo(uintptr_t dinfo) { delivery_info = dinfo; }


    /** Gets the link id used for delivery.  For use by SST Core only */
    inline Link* getDeliveryLink() { return reinterpret_cast<Link*>(delivery_info); }

    /** Gets the link id associated with this event.  For use by SST Core only */
    inline LinkId_t getTag() const { return getOrderTag(); }


    /** Holds the delivery information.  This is stored as a
      uintptr_t, but is actually a pointer converted using
      reinterpret_cast.  For events send on links connected to a
      Component/SubComponent, this holds a pointer to the delivery
      functor.  For events sent on links connected to a Sync object,
      this holds a pointer to the remote link to send the event on
      after synchronization.
    */
    uintptr_t delivery_info;

private:
    static std::atomic<uint64_t> id_counter;

#ifdef __SST_DEBUG_EVENT_TRACKING__
    std::string first_comp;
    std::string first_type;
    std::string first_port;
    std::string last_comp;
    std::string last_type;
    std::string last_port;
#endif

    ImplementVirtualSerializable(SST::Event)
};

/**
 * Empty Event.  Does nothing.
 */
class EmptyEvent : public Event
{
public:
    EmptyEvent() :
        Event()
    {}
    ~EmptyEvent() {}

private:
    ImplementSerializable(SST::EmptyEvent)
};

class EventHandlerMetaData : public AttachPointMetaData
{
public:
    const ComponentId_t comp_id;
    const std::string   comp_name;
    const std::string   comp_type;
    const std::string   port_name;

    EventHandlerMetaData(
        ComponentId_t id, const std::string& cname, const std::string& ctype, const std::string& pname) :
        comp_id(id),
        comp_name(cname),
        comp_type(ctype),
        port_name(pname)
    {}

    ~EventHandlerMetaData() {}
};

namespace pvt {

/**
   Class used with std::lower_bound to find the start of events in a
   sorted list with the specified delivery_info.
 */
class DeliveryInfoCompEvent : public Event
{
public:
    static uintptr_t getDeliveryInfo(Event* ev) { return ev->delivery_info; }

    DeliveryInfoCompEvent(uintptr_t delivery_info) { setDeliveryInfo(0, delivery_info); }
};

} // namespace pvt

} // namespace SST

#endif // SST_CORE_EVENT_H
