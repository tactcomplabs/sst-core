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

#ifndef SST_CORE_ELI_ELEMENTINFO_H
#define SST_CORE_ELI_ELEMENTINFO_H

#include "sst/core/eli/attributeInfo.h"
#include "sst/core/eli/categoryInfo.h"
#include "sst/core/eli/defaultInfo.h"
#include "sst/core/eli/elementbuilder.h"
#include "sst/core/eli/elibase.h"
#include "sst/core/eli/interfaceInfo.h"
#include "sst/core/eli/paramsInfo.h"
#include "sst/core/eli/portsInfo.h"
#include "sst/core/eli/profilePointInfo.h"
#include "sst/core/eli/simpleInfo.h"
#include "sst/core/eli/statsInfo.h"
#include "sst/core/eli/subcompSlotInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <map>
#include <string>
#include <vector>

namespace SST {
class Component;
class Module;
class SubComponent;
class BaseComponent;
class RealTimeAction;
namespace Partition {
class SSTPartitioner;
}
namespace Statistics {
template <class T>
class Statistic;
class StatisticBase;
} // namespace Statistics
class RankInfo;
class SSTElementPythonModule;

/****************************************************
   Base classes for templated documentation classes
*****************************************************/

namespace ELI {

template <class T>
class DataBase
{
public:
    static T* get(const std::string& elemlib, const std::string& elem)
    {
        auto libiter = infos().find(elemlib);
        if ( libiter != infos().end() ) {
            auto& submap   = libiter->second;
            auto  elemiter = submap.find(elem);
            if ( elemiter != submap.end() ) {
                return elemiter->second;
            }
        }
        return nullptr;
    }

    static void add(const std::string& elemlib, const std::string& elem, T* info) { infos()[elemlib][elem] = info; }

private:
    static std::map<std::string, std::map<std::string, T*>>& infos()
    {
        static std::map<std::string, std::map<std::string, T*>> infos_;
        return infos_;
    }
};

template <class Policy, class... Policies>
class BuilderInfoImpl : public Policy, public BuilderInfoImpl<Policies...>
{
    using Parent = BuilderInfoImpl<Policies...>;

public:
    template <class... Args>
    BuilderInfoImpl(const std::string& elemlib, const std::string& elem, Args&&... args) :
        Policy(args...),
        Parent(elemlib, elem, args...) // forward as l-values
    {
        DataBase<Policy>::add(elemlib, elem, this);
    }

    template <class XMLNode>
    void outputXML(XMLNode* node)
    {
        Policy::outputXML(node);
        Parent::outputXML(node);
    }

    void toString(std::ostream& os) const
    {
        Policy::toString(os);
        Parent::toString(os);
    }

    BuilderInfoImpl(const BuilderInfoImpl&)            = delete;
    BuilderInfoImpl& operator=(const BuilderInfoImpl&) = delete;
};

template <>
class BuilderInfoImpl<void>
{
protected:
    template <class... Args>
    explicit BuilderInfoImpl(Args&&... UNUSED(args))
    {}

    template <class XMLNode>
    void outputXML(XMLNode* UNUSED(node))
    {}

    void toString(std::ostream& UNUSED(os)) const {}

    BuilderInfoImpl(const BuilderInfoImpl&)            = delete;
    BuilderInfoImpl& operator=(const BuilderInfoImpl&) = delete;
};

template <class Base>
class InfoLibrary
{
public:
    using BaseInfo = typename Base::BuilderInfo;

    explicit InfoLibrary(const std::string& name) :
        name_(name)
    {}

    BaseInfo* getInfo(const std::string& name)
    {
        auto iter = infos_.find(name);
        if ( iter == infos_.end() ) {
            return nullptr;
        }
        else {
            return iter->second;
        }
    }

    bool hasInfo(const std::string& name) const { return infos_.find(name) != infos_.end(); }

    int numEntries(bool exclude_aliases = false) const
    {
        if ( !exclude_aliases ) return infos_.size();
        int count = 0;
        for ( auto x : infos_ ) {
            if ( x.first != x.second->getAlias() ) ++count;
        }
        return count;
    }

    const std::map<std::string, BaseInfo*>& getMap() const { return infos_; }

    void readdInfo(const std::string& name, BaseInfo* info)
    {
        infos_[name] = info;

        // Add the alias
        const std::string& alias = info->getAlias();
        if ( !alias.empty() ) infos_[alias] = info;
    }

    bool addInfo(const std::string& elem, BaseInfo* info)
    {
        readdInfo(elem, info);
        // dlopen might thrash this later - add a loader to put it back in case
        addLoader(name_, elem, info);
        return true;
    }

private:
    void addLoader(const std::string& lib, const std::string& name, BaseInfo* info);

    std::map<std::string, BaseInfo*> infos_;

    std::string name_;
};

template <class Base>
class InfoLibraryDatabase
{
public:
    using Library  = InfoLibrary<Base>;
    using BaseInfo = typename Library::BaseInfo;
    using Map      = std::map<std::string, Library*>;

    static std::vector<std::string> getRegisteredElementNames()
    {
        // Need to pull out all the elements from the libraries that
        // have an element of this type

        std::vector<std::string> ret;
        // First iterate over libraries
        for ( auto& [name, lib] : libraries() ) {
            for ( auto& [elemlib, info] : lib->getMap() ) {
                ret.push_back(name + "." + elemlib);
            }
        }
        return ret;
    }

    static Library* getLibrary(const std::string& name)
    {
        auto& lib = libraries()[name];
        if ( !lib ) lib = new Library(name);
        return lib;
    }

private:
    // Database
    static Map& libraries()
    {
        static Map libs;
        return libs;
    }
};

template <class Base, class Info>
struct InfoLoader : public LibraryLoader
{
    InfoLoader(const std::string& elemlib, const std::string& elem, Info* info) :
        elemlib_(elemlib),
        elem_(elem),
        info_(info)
    {}

    void load() override
    {
        auto* lib = InfoLibraryDatabase<Base>::getLibrary(elemlib_);
        if ( !lib->hasInfo(elem_) ) {
            lib->readdInfo(elem_, info_);
        }
    }

private:
    std::string elemlib_;
    std::string elem_;
    Info*       info_;

    InfoLoader(const InfoLoader&)            = delete;
    InfoLoader& operator=(const InfoLoader&) = delete;
};

template <class Base>
void
InfoLibrary<Base>::addLoader(const std::string& elemlib, const std::string& elem, BaseInfo* info)
{
    auto loader = new InfoLoader<Base, BaseInfo>(elemlib, elem, info);
    LoadedLibraries::addLoader(elemlib, elem, info->getAlias(), loader);
}

template <class Base>
struct ElementsInfo
{
    static InfoLibrary<Base>* getLibrary(const std::string& name)
    {
        return InfoLibraryDatabase<Base>::getLibrary(name);
    }

    template <class T>
    static bool add()
    {
        return Base::template addDerivedInfo<T>(T::ELI_getLibrary(), T::ELI_getName());
    }
};

template <class Base, class T>
struct InstantiateBuilderInfo
{
    static bool isLoaded() { return loaded; }

    static inline const bool loaded = ElementsInfo<Base>::template add<T>();
};

struct InfoDatabase
{
    template <class T>
    static InfoLibrary<T>* getLibrary(const std::string& name)
    {
        return InfoLibraryDatabase<T>::getLibrary(name);
    }

    template <class T>
    static std::vector<std::string> getRegisteredElementNames()
    {
        return InfoLibraryDatabase<T>::getRegisteredElementNames();
    }
};

void force_instantiate_bool(bool b, const char* name);

template <class T>
struct ForceExport
{
    static bool ELI_isLoaded() { return T::ELI_isLoaded(); }
};

} // namespace ELI

/**************************************************************************
  Class and constexpr functions to extract integers from version number.
**************************************************************************/

struct SST_ELI_element_version_extraction
{
    const unsigned major;
    const unsigned minor;
    const unsigned tertiary;

    constexpr unsigned getMajor() const { return major; }
    constexpr unsigned getMinor() const { return minor; }
    constexpr unsigned getTertiary() const { return tertiary; }
};

constexpr unsigned
SST_ELI_getMajorNumberFromVersion(SST_ELI_element_version_extraction ver)
{
    return ver.getMajor();
}

constexpr unsigned
SST_ELI_getMinorNumberFromVersion(SST_ELI_element_version_extraction ver)
{
    return ver.getMinor();
}

constexpr unsigned
SST_ELI_getTertiaryNumberFromVersion(SST_ELI_element_version_extraction ver)
{
    return ver.getTertiary();
}

/**************************************************************************
  Macros used by elements to add element documentation
**************************************************************************/

#define SST_ELI_DECLARE_INFO(...)                                                                      \
    using BuilderInfo = ::SST::ELI::BuilderInfoImpl<__VA_ARGS__, SST::ELI::ProvidesDefaultInfo, void>; \
    static bool addInfo(const std::string& elemlib, const std::string& elem, BuilderInfo* info)        \
    {                                                                                                  \
        return ::SST::ELI::InfoDatabase::getLibrary<__LocalEliBase>(elemlib)->addInfo(elem, info);     \
    }                                                                                                  \
    SST_ELI_DECLARE_INFO_COMMON()

#define SST_ELI_DECLARE_DEFAULT_INFO()                                                             \
    using BuilderInfo = ::SST::ELI::BuilderInfoImpl<SST::ELI::ProvidesDefaultInfo, void>;          \
    template <class BuilderImpl>                                                                   \
    static bool addInfo(const std::string& elemlib, const std::string& elem, BuilderImpl* info)    \
    {                                                                                              \
        return ::SST::ELI::InfoDatabase::getLibrary<__LocalEliBase>(elemlib)->addInfo(elem, info); \
    }                                                                                              \
    SST_ELI_DECLARE_INFO_COMMON()

#define SST_ELI_DECLARE_INFO_EXTERN(...)                                                               \
    using BuilderInfo = ::SST::ELI::BuilderInfoImpl<SST::ELI::ProvidesDefaultInfo, __VA_ARGS__, void>; \
    static bool addInfo(const std::string& elemlib, const std::string& elem, BuilderInfo* info);       \
    SST_ELI_DECLARE_INFO_COMMON()

#define SST_ELI_DECLARE_DEFAULT_INFO_EXTERN()                                                    \
    using BuilderInfo = ::SST::ELI::BuilderInfoImpl<SST::ELI::ProvidesDefaultInfo, void>;        \
    static bool addInfo(const std::string& elemlib, const std::string& elem, BuilderInfo* info); \
    SST_ELI_DECLARE_INFO_COMMON()

#define SST_ELI_DEFINE_INFO_EXTERN(base)                                                           \
    bool base::addInfo(const std::string& elemlib, const std::string& elem, BuilderInfo* info)     \
    {                                                                                              \
        return ::SST::ELI::InfoDatabase::getLibrary<__LocalEliBase>(elemlib)->addInfo(elem, info); \
    }

#define SST_ELI_EXTERN_DERIVED(base, cls, lib, name, version, desc) \
    bool ELI_isLoaded();                                            \
    SST_ELI_DEFAULT_INFO(lib, name, ELI_FORWARD_AS_ONE(version), desc)

// The Intel compilers do not correctly instantiate symbols
// even though they are required. We have to force the instantiation
// in source files, header files are not good enough
// we do this by creating a static bool that produces an undefined ref
// if the instantiate macro is missing in a source file
#ifdef __INTEL_COMPILER
#define SST_ELI_FORCE_INSTANTIATION(base, cls)                                \
    template <class T>                                                        \
    struct ELI_ForceRegister                                                  \
    {                                                                         \
        ELI_ForceRegister()                                                   \
        {                                                                     \
            bool b = SST::ELI::InstantiateBuilder<base, cls>::isLoaded() &&   \
                     SST::ELI::InstantiateBuilderInfo<base, cls>::isLoaded(); \
            SST::ELI::force_instantiate_bool(b, #cls);                        \
        }                                                                     \
    };                                                                        \
    ELI_ForceRegister<cls> force_instantiate;
// if the implementation is entirely in a C++ file
// the Intel compiler will not generate any code because
// it none of the symbols can be observed by other files
// this forces the Intel compiler to export symbols self-contained in a C++ file
#define SST_ELI_EXPORT(cls) template class SST::ELI::ForceExport<cls>;
#else
#define SST_ELI_FORCE_INSTANTIATION(base, cls)
#define SST_ELI_EXPORT(cls)
#endif

// This call needs to be made for classes that will acually be
// instanced, as opposed to only be declared as an API (using
// SST_ELI_DECLARE_BASE or SST_ELI_DECLARE_NEW_BASE).  This class will
// inherit the ELI information from it's parent ELI API classes (the
// informatin in the parent APIs will be added to the ELI declared in
// this class.  Any local information will overwrite any inherited
// information.  See comment for SST_ELI_DECLARE_BASE in elibase.h for
// info on how __EliDerivedLevel is used.
#define SST_ELI_REGISTER_DERIVED(base, cls, lib, name, version, desc)                                         \
    [[maybe_unused]]                                                                                          \
    static constexpr int __EliDerivedLevel = std::is_same_v<base, cls> ? __EliBaseLevel : __EliBaseLevel + 1; \
    static bool          ELI_isLoaded()                                                                       \
    {                                                                                                         \
        return SST::ELI::InstantiateBuilder<base, cls>::isLoaded() &&                                         \
               SST::ELI::InstantiateBuilderInfo<base, cls>::isLoaded();                                       \
    }                                                                                                         \
    SST_ELI_FORCE_INSTANTIATION(base, cls)                                                                    \
    SST_ELI_DEFAULT_INFO(lib, name, ELI_FORWARD_AS_ONE(version), desc)

#define SST_ELI_REGISTER_EXTERN(base, cls)                              \
    bool cls::ELI_isLoaded()                                            \
    {                                                                   \
        return SST::ELI::InstantiateBuilder<base, cls>::isLoaded() &&   \
               SST::ELI::InstantiateBuilderInfo<base, cls>::isLoaded(); \
    }

} // namespace SST

#endif // SST_CORE_ELI_ELEMENTINFO_H
