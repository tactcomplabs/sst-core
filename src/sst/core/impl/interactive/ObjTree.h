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

#ifndef SST_CORE_SERIALIZATION_OBJECTMAP_DEBUGGER_H
#define SST_CORE_SERIALIZATION_OBJECTMAP_DEBUGGER_H

#include "sst/core/from_string.h"
#include "sst/core/warnmacros.h"
#include "sst/core/componentInfo.h"
#include "sst/core/baseComponent.h"
#include "sst/core/serialization/objectMapDeferred.h"

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>
#include <any>
#include <unordered_set>

namespace SST::Core::Serialization {

    class ObjTreeCont
    {
        public:
        ObjTreeCont() : parent_(nullptr), children_(), name_("uninit"), type_("uninit")  {};
        ObjTreeCont(const std::string& name, const std::string& type)
        : parent_(nullptr), children_(), name_(name), type_(type) {}
        virtual ~ObjTreeCont() = default;

        ObjTreeCont(const ObjTreeCont& rhs)
        : parent_(nullptr), children_(), name_(rhs.name_), type_(rhs.type_)
        {
            children_.reserve(rhs.children_.size());
            for (const auto& child : rhs.children_) {
                auto* cloned = child->clone();         
                cloned->parent_ = this;                
                children_.emplace_back(cloned);
            }
        }

        ObjTreeCont& operator=(const ObjTreeCont& rhs) {
            if (this == &rhs) return *this;
            parent_ = nullptr;
            name_   = rhs.name_;
            type_   = rhs.type_;
            children_.clear();
            children_.reserve(rhs.children_.size());
            for (const auto& child : rhs.children_) {
                auto* cloned = child->clone();
                cloned->parent_ = this;
                children_.emplace_back(cloned);
            }
            return *this;
        }

        virtual ObjTreeCont* clone() const {};

    //    virtual bool operator<(const ObjTreeCont& rhs) const {
    //        return false;    
     //   }

        void addChildObj(ObjTreeCont* obj){
            children_.push_back(std::unique_ptr<ObjTreeCont>(obj));
            obj->parent_ = this;
        }

        void setParent(ObjTreeCont* p) { parent_ = p; }
        ObjTreeCont* getParent() const                       { return parent_;}
        const std::vector<std::unique_ptr<ObjTreeCont>>& getChildren() const { return children_; }

        const std::string& getObjName() const { return name_; }
        const std::string& getType() const { return type_; }
        void setName(const std::string& name) { name_ = name; }
        void setType(const std::string& type) { type_ = type; }
        bool isRoot(){ return parent_ == nullptr; }

        template<typename Func>
        void applyRecursive(Func&& func){
            for (auto& child : children_){
                func(child.get());
            }
        }

         template<typename ObjectType, typename Func>
        void applyRecursiveByType(Func&& func) {
            for (auto& child : children_) {
                if (auto* child_t = dynamic_cast<ObjectType*>(child.get())) {
                    func(child_t);
             }
            }
        }

        template<typename ObjectType, typename Func>
        ObjectType* findByType(Func&& predicate) {
            for (auto& child : children_) {
                if (auto* child_t = dynamic_cast<ObjectType*>(child.get())) {
                    if (predicate(child_t)) return child_t;
                }
            }
            return nullptr;
        }

        ObjTreeCont* findByName(const std::string name) {
            for(size_t i=0; i < children_.size(); i++){
                if(children_[i]->getObjName() == name){
                    return children_[i].get();
                }
            }
            return nullptr;
        }

        virtual void apply() {};
        virtual std::string getTypeName() const {return type_;};
        virtual void Dump(const int verbosity){
            std::cout << name_ << "/ " << std::endl; //(" << type_ << ")" << std::endl;
            if (verbosity > 0) {
                applyRecursive([verbosity](ObjTreeCont* child) {
                    child->Dump(verbosity - 1);
                });
            }
        }

        protected:
        ObjTreeCont*              parent_;
        std::vector<std::unique_ptr<ObjTreeCont>> children_;
        std::string                                 name_;
        std::string                                 type_;

    };

    template<typename Obj_T>
    class ObjTree : public ObjTreeCont
    {
        public:
        ObjTree() = default;

        ObjTree(const ObjTree<Obj_T>& rhs): ObjTreeCont(rhs), objects_()
        {
            objects_.reserve(rhs.objects_.size());
            for (const auto& obj : rhs.objects_) {
                auto* cloned = obj->clone();
                cloned->setParent(this);
                objects_.emplace_back(cloned);
            }
        }

        ObjTree<Obj_T>& operator=(const ObjTree<Obj_T>& other) {
            if (this == &other) return *this;
            ObjTreeCont::operator=(other);
            objects_.clear();
            objects_.reserve(other.objects_.size());
            for (const auto& obj : other.objects_) {
                auto* cloned = obj->clone();
                cloned->setParent(this);
                objects_.emplace_back(cloned);
            }
            return *this;
        }

  //      virtual bool operator<(const ObjTreeCont& rhs) const override {
  //          return false;
  //       }

        void BuildTree(const ComponentInfoMap& compMap);

        Obj_T& getObj(){ return static_cast<Obj_T&>(*this);}
        const Obj_T& getObj() const { return static_cast<const Obj_T&>(*this);}


        template<typename ObjectType, typename Func>
        void applyRecursiveByType(Func&& func){
            for (auto& child : children_){
                if(auto* child_t = dynamic_cast<ObjectType*>(child.get())){
                    func(child_t);
                }
            }
        }

        std::string getTypeName() const override {
            return typeid(Obj_T).name();
        }

        bool isEmpty(){return objects_.empty();}

        void Dump(const int verbosity) override { std::cout << "Root/" << std::endl;}

        protected:
        std::vector<std::unique_ptr<ObjTreeCont>> objects_;

    };

    class IntegerObj : public ObjTree<IntegerObj> {
        public:
        using IntVariant = std::variant<int8_t, int16_t, int32_t, int64_t, 
                                     uint8_t, uint16_t, uint32_t, uint64_t>;
        
        private:
        IntVariant val_;

        public:
        template<typename T>
        IntegerObj(T v) : val_(v) {}

        IntegerObj(const IntegerObj& rhs): 
            ObjTree<IntegerObj>(rhs),
            val_(rhs.val_)
        {}
        IntegerObj& operator=(const IntegerObj& rhs) {
            if (this == &rhs) return *this;
            ObjTree<IntegerObj>::operator=(rhs);
            val_  = rhs.val_;
            return *this;
        }
        ObjTreeCont* clone() const override {
           return new IntegerObj(*this);
        }

        template<typename T>
        T getVal() const{ return std::get<T>(val_); }

        template<typename T>
        void setVal(T v){ val_ = v;}

        template<typename Visitor>
        auto visit(Visitor&& visitor) { return std::visit(std::forward<Visitor>(visitor), val_); }
    
        template<typename Visitor>
        auto visit(Visitor&& visitor) const { return std::visit(std::forward<Visitor>(visitor), val_);}

        void apply() override {
            visit([](auto val) {
                std::cout << "Processing integer: " << static_cast<int64_t>(val) << std::endl;
            });
        }

        // using cmpare = std::variant<SST::Core::Serialization::IntegerObj, SST::Core::Serialization::FloatObj>;
     //   virtual bool operator<(ObjTreeCont& rhs) final; 
        
        void Dump(const int verbosity) override{
            if(verbosity == 0) return;
            if(verbosity == 1){ 
                visit([&](auto val) {
                    std::cout << getObjName() << " = " << static_cast<int64_t>(val) << std::endl;
                });
            }else{
                visit([&](auto val) {
                    std::cout << getObjName() << " = " << static_cast<int64_t>(val) << " (" << getType() << ")" << std::endl;
                });
            }
        }
    };

    class FloatObj : public ObjTree<FloatObj> {
        public:
        using FloatVariant = std::variant<float, double, long double>;
        
        private:
        FloatVariant val_;

        public:
        template<typename T>
        FloatObj(T v) : val_(v) {}

        FloatObj(const FloatObj& rhs): ObjTree<FloatObj>(rhs), val_(rhs.val_)
        {}

        FloatObj& operator=(const FloatObj& rhs) {
            if (this == &rhs) return *this;
            ObjTree<FloatObj>::operator=(rhs);
            val_ = rhs.val_;
            return *this;
        }

     //   virtual bool operator<(ObjTreeCont& rhs) final;

        ObjTreeCont* clone() const override {
            return new FloatObj(*this);
        }

        template<typename T>
        T getVal() const{ return std::get<T>(val_); }

        template<typename T>
        void setVal(T v){ val_ = v;}

        template<typename Visitor>
        auto visit(Visitor&& visitor) { return std::visit(std::forward<Visitor>(visitor), val_); }
    
        template<typename Visitor>
        auto visit(Visitor&& visitor) const { return std::visit(std::forward<Visitor>(visitor), val_);}

        void apply() override {
            visit([](auto val) {
                std::cout << "Processing float: " << val << std::endl;
            });
        }
        void Dump(const int verbosity) override{
            if(verbosity == 0) return;
            if(verbosity == 1){
                visit([&](auto val) {
                    std::cout << getObjName() << " = " << std::setprecision(6) << getObjName() << " = " << val << std::endl;
                });
            }else{
                visit([&](auto val) {
                    std::cout << getObjName() << " = " << std::setprecision(6) << getObjName() << " = " << val << " (" << getType() << ")" << std::endl;
                });
            }
        }
    };

 /*   inline bool IntegerObj::operator<(ObjTreeCont& rhs) {
    if (IntegerObj* tmp = dynamic_cast<IntegerObj*>(&rhs)) {
        return std::visit([&](auto lhs) {
            return tmp->visit([&](auto r) { return lhs < r; });
        }, val_);
    } else if (FloatObj* tmp = dynamic_cast<FloatObj*>(&rhs)) {
        return std::visit([&](auto lhs) {
            return tmp->visit([&](auto r) { return lhs < r; });
        }, val_);
    }
    return false;
}

    inline bool FloatObj::operator<(ObjTreeCont& rhs) {
    if (IntegerObj* tmp = dynamic_cast<IntegerObj*>(&rhs)) {
        return std::visit([&](auto lhs) {
            return tmp->visit([&](auto r) { return lhs < r; });
        }, val_);
    } else if (FloatObj* tmp = dynamic_cast<FloatObj*>(&rhs)) {
        return std::visit([&](auto lhs) {
            return tmp->visit([&](auto r) { return lhs < r; });
        }, val_);
    }
    return false;
}*/

    class ComponentObj : public ObjTree<ComponentObj> 
    {
        
        private:
        BaseComponent* val_ = nullptr;
        ComponentInfo* compInfo_ = nullptr;

        public:
        ComponentObj() = default;
        ComponentObj(BaseComponent* v, ComponentInfo* ci) : val_(v), compInfo_(ci) {setName(v->getName());}
        
        ComponentObj(const ComponentObj& rhs): ObjTree<ComponentObj>(rhs), val_(rhs.val_), compInfo_(rhs.compInfo_) {}

        ComponentObj& operator=(const ComponentObj& rhs) {
            if (this == &rhs) return *this;
            ObjTree<ComponentObj>::operator=(rhs);
            val_ = rhs.val_;
            compInfo_ = rhs.compInfo_;
            return *this;
        }

        ObjTreeCont* clone() const override {
            return new ComponentObj(*this);
        }

        BaseComponent* getVal() const{ return val_; }
        ComponentInfo* getInfo() const{ return compInfo_;}

        void setVal(BaseComponent* v){ }

        void apply() override {
            std::cout << "Processing component: " << val_->getName() << std::endl;
        }
        void Dump(const int verbosity) override{
            std::cout << val_->getName() << std::endl;
        }

        ComponentObj* find(const std::string name){
            ComponentObj* result = nullptr;
            applyRecursiveByType<ComponentObj>([&result, &name](ComponentObj* obj) {
                if (obj->getVal()->getName() == name) {
                result = obj;
            }
            });
            return result;
        }
    };

    template<typename Obj_T>
    void ObjTree<Obj_T>::BuildTree(const ComponentInfoMap& compMap){
        for ( auto comp = compMap.begin(); comp != compMap.end(); comp++ ) {
        ComponentInfo* compinfo = *comp;
        BaseComponent* bc = compinfo->getComponent();
        ComponentObj* c = new ComponentObj(bc, compinfo);
        addChildObj(c);
        }
    }

class ContainerObj : public ObjTree<ContainerObj> {
    size_t size_ = 0;

public:
    ContainerObj(const std::string& name, const std::string& type, size_t size)
        :size_(size) {
            setName(name);
            setType(type);
        }

    ContainerObj(const ContainerObj& rhs): ObjTree<ContainerObj>(rhs), size_(rhs.size_) {
            setName(rhs.name_);
            setType(rhs.type_);
    }

    ContainerObj& operator=(const ContainerObj& rhs) {
        if (this == &rhs) return *this;
        ObjTree<ContainerObj>::operator=(rhs);
        name_ = rhs.name_;
        type_ = rhs.type_;
        size_ = rhs.size_;
        return *this;
    }

    ObjTreeCont* clone() const override {
        return new ContainerObj(*this);
    }


    const std::string& getContainerType() const { return type_; }
    size_t getSize() const { return size_; }

    void apply() override {
        std::cout << "Container: " << name_ << " (" << type_ 
                  << ") size=" << size_ << std::endl;
    }

    void Dump(const int verbosity) override {
        std::cout << name_ << "[" << size_ << " elements] (" << type_ << ")"<< std::endl;
        if (verbosity > 0) {
            applyRecursive([verbosity](ObjTreeCont* child) {
                child->Dump(verbosity - 1);
            });
        }
    }

    ObjTreeCont* getElementAt(std::vector<size_t> indices) const {
    ObjTreeCont* current = const_cast<ContainerObj*>(this);

    for (size_t idx : indices) {
        if (!current) return nullptr;

        // If current node is a ContainerObj, its actual elements may be
        // nested under a wrapper child (from convertNode). Detect that.
        auto& children = current->getChildren();

        // Check if there's a single wrapper ContainerObj child
        ObjTreeCont* elementParent = current;
        if (children.size() == 1) {
            if (auto* wrapper = dynamic_cast<ContainerObj*>(children[0].get())) {
                elementParent = wrapper;
            }
        }

        auto& elems = elementParent->getChildren();
        if (idx >= elems.size()) return nullptr;

        current = elems[idx].get();
    }
    return current;
}

void printElementAt(std::vector<size_t> indices, int verbosity = 1) const {
    ObjTreeCont* elem = getElementAt(indices);
    if (elem) {
        elem->Dump(verbosity);
    } else {
        std::cout << "Element not found at path {";
        bool first = true;
        for (auto i : indices) {
            if (!first) std::cout << ", ";
            std::cout << i;
            first = false;
        }
        std::cout << "}" << std::endl;
    }
}

};

// Node for string types (treated specially since they're fundamental-like)
class StringObj : public ObjTree<StringObj> {
    std::string val_;

public:
    StringObj(const std::string& v) : val_(v) {}
    StringObj(const StringObj& rhs) : ObjTree<StringObj>(rhs) , val_(rhs.val_) {}

    StringObj& operator=(const StringObj& rhs) {
        if (this == &rhs) return *this;
        ObjTree<StringObj>::operator=(rhs);
        val_ = rhs.val_;
        return *this;
    }

    ObjTreeCont* clone() const override {
        return new StringObj(*this);
    }

    const std::string& getVal() const { return val_; }
    void setVal(const std::string& v) { val_ = v; }

    void apply() override {
        std::cout << "Processing string: " << val_ << std::endl;
    }

    void Dump(const int verbosity) override {
        if(verbosity == 0) return;
        if(verbosity == 1){
            std::cout << getObjName() << " = \"" << val_ << "\"" << std::endl;
        }else{
            std::cout << getObjName() << " = \"" << val_ << "\"" << " (" << getType() << ")" <<  std::endl;
        }
    }
};

// Node for bool (separate from integer for clarity)
class BoolObj : public ObjTree<BoolObj> {
    bool val_;

public:
    BoolObj(bool v) : val_(v) {}
    BoolObj(const BoolObj& rhs): ObjTree<BoolObj>(rhs), val_(rhs.val_) {}

    BoolObj& operator=(const BoolObj& rhs) {
        if (this == &rhs) return *this;
        ObjTree<BoolObj>::operator=(rhs);
        val_ = rhs.val_;
        return *this;
    }

    ObjTreeCont* clone() const override {
        return new BoolObj(*this);
    }

    bool getVal() const { return val_; }
    void setVal(bool v) { val_ = v; }

    void apply() override {
        std::cout << "Processing bool: " << (val_ ? "true" : "false") << std::endl;
    }

    void Dump(const int verbosity) override { 
        if(verbosity == 0) return;
        if(verbosity == 1){ 
            std::cout << getObjName() << " = " << (val_ ? "true" : "false") << std::endl;
        }else {
            std::cout << getObjName() << " = " << (val_ ? "true" : "false") << " (" << getType() << ")" <<  std::endl;
        }
    }
};

class ObjectMapToTree {
    using IntVariant = IntegerObj::IntVariant;
    using FloatVariant = FloatObj::FloatVariant;

    // Known integer type strings
    static bool isIntegerType(const std::string& type) {
        static const std::unordered_set<std::string> intTypes = {
            "signed char", "int8_t",
            "short", "int16_t",
            "int", "int32_t",
            "long", "long long", "int64_t",
            "unsigned char", "uint8_t",
            "unsigned short", "uint16_t",
            "unsigned int", "unsigned", "uint32_t",
            "unsigned long", "unsigned long long", "uint64_t"
        };
        return intTypes.count(type) > 0;
    }

    static bool isFloatType(const std::string& type) {
        return type == "float" || type == "double" || type == "long double";
    }

    static bool isStringType(const std::string& type) {
        // Demangled std::string can appear in various forms
        return type.find("std::string") != std::string::npos
            || type.find("std::__cxx11::basic_string") != std::string::npos
            || type.find("basic_string") != std::string::npos;
    }

    static bool isContainerType(const std::string& type) {
        return type.find("std::vector") != std::string::npos
            || type.find("std::map") != std::string::npos
            || type.find("std::unordered_map") != std::string::npos
            || type.find("std::set") != std::string::npos
            || type.find("std::unordered_set") != std::string::npos
            || type.find("std::list") != std::string::npos
            || type.find("std::deque") != std::string::npos
            || type.find("std::multimap") != std::string::npos
            || type.find("std::array") != std::string::npos;
    }

    static std::unique_ptr<IntegerObj> makeIntegerObj(const std::string& type, void* addr, std::string name)  {
        if (!addr) return nullptr;
        if (type == "signed char"    || type == "int8_t" || type == "char")   return std::make_unique<IntegerObj>(*static_cast<int8_t*>(addr));
        if (type == "short"          || type == "int16_t")  return std::make_unique<IntegerObj>(*static_cast<int16_t*>(addr));
        if (type == "int"            || type == "int32_t")  return std::make_unique<IntegerObj>(*static_cast<int32_t*>(addr));
        if (type == "long" || type == "long long" || type == "int64_t")
            return std::make_unique<IntegerObj>(*static_cast<int64_t*>(addr));
        if (type == "unsigned char"  || type == "uint8_t")  return std::make_unique<IntegerObj>(*static_cast<uint8_t*>(addr));
        if (type == "unsigned short" || type == "uint16_t") return std::make_unique<IntegerObj>(*static_cast<uint16_t*>(addr));
        if (type == "unsigned int"   || type == "unsigned" || type == "uint32_t")
            return std::make_unique<IntegerObj>(*static_cast<uint32_t*>(addr));
        if (type == "unsigned long"  || type == "unsigned long long" || type == "uint64_t")
            return std::make_unique<IntegerObj>(*static_cast<uint64_t*>(addr));
        return nullptr;
    }

    static std::unique_ptr<FloatObj> makeFloatObj(const std::string& type, void* addr) {
        if (!addr) return nullptr;
        if (type == "float")       return std::make_unique<FloatObj>(*static_cast<float*>(addr));
        if (type == "double")      return std::make_unique<FloatObj>(*static_cast<double*>(addr));
        if (type == "long double") return std::make_unique<FloatObj>(*static_cast<long double*>(addr));
        return nullptr;
    }

public:
    // Convert a single ObjectMap* into an ObjTreeCont*
    // Caller takes ownership of the returned pointer
    static ObjTreeCont* convert(const std::string& name, ObjectMap* objMap) {
        if (!objMap) return nullptr;

        std::string type = objMap->getType();
        void* addr = objMap->getAddr();

        // --- Fundamental types ---
        if (objMap->isFundamental()) {
            // Bool
            if (type == "bool" && addr) {
                auto boolObj = new BoolObj(*static_cast<bool*>(addr));
                boolObj->setName(name);
                boolObj->setType(type);
                return boolObj;
            }

            // Integer types
            if (isIntegerType(type)) {
                auto intObj = makeIntegerObj(type, addr, name);
                intObj->setName(name);
                intObj->setType(type);
                if (intObj) return intObj.release();
            }

            // Float types
            if (isFloatType(type)) {
                auto floatObj = makeFloatObj(type, addr);
                floatObj->setName(name);
                floatObj->setType(type);
                if (floatObj) return floatObj.release();
            }

            // String 
            if (isStringType(type) && addr) {
                auto stringObj = new StringObj(*static_cast<std::string*>(addr));
                stringObj->setName(name);
                stringObj->setType(type);
                return stringObj;
            }

            // Unknown fundamental — wrap in GenericObj with the value as name
            auto* node = new ObjTreeCont(name + " = " + objMap->get(), type);
            return node;
        }

        // --- Containers (vector, map, set, etc.) ---
        if (objMap->isContainer() || isContainerType(type)) {
            const auto& variables = objMap->getVariables();
            auto* container = new ContainerObj(name, type, variables.size());

            ObjTreeCont* childNode = convertNode(name, objMap);
            container->addChildObj(childNode);

            /*for (const auto& [childName, childMap] : variables) {
                ObjTreeCont* childNode = convert(childName, childMap);
                if (childNode) {
                    container->addChildObj(childNode);
                }
            }*/
            return container;
        }

        // --- BaseComponent types ---
        if (objMap->getCategory() == ObjectMap::ObjectCategory::Component) {
            auto* comp = static_cast<BaseComponent*>(objMap->getAddr());
            if (comp) {
                auto* compObj = new ComponentObj(comp, nullptr);
                compObj->setName(name);
                compObj->setType(type);
              //  const auto& variables = objMap->getVariables();
              //  for (const auto& [childName, childMap] : variables) {
              //      ObjTreeCont* childNode = convertNode(childName, childMap);
              //      if (childNode) compObj->addChildObj(childNode);
             //   }
                return compObj;
            }
        }
        
        // --- Generic non-fundamental, non-container (user-defined classes) ---
         auto* node = new ObjTreeCont(name, type);
        const auto& variables = objMap->getVariables();
        for (const auto& [childName, childMap] : variables) {
            ObjTreeCont* childNode = convertNode(childName, childMap);
            if (childNode) {
                node->addChildObj(childNode);
            }
        }
        ObjTreeCont* childNode = convertNode(name, objMap);
        node->addChildObj(childNode);

        return node;
    }

     static ObjTreeCont* convertNode(const std::string& name, ObjectMap* objMap) {
        if (!objMap) return nullptr;

        std::string type = objMap->getType();
        void* addr = objMap->getAddr();

        // Fundamental types
        if (objMap->isFundamental()) {
            if (type == "bool" && addr) {
                auto boolObj = new BoolObj(*static_cast<bool*>(addr));
                boolObj->setName(name);
                boolObj->setType(type);
                return boolObj;
            }
            if (isIntegerType(type)) {
                auto intObj = makeIntegerObj(type, addr, name);
                if (intObj) {
                    intObj->setName(name);
                    intObj->setType(type);
                    return intObj.release();
                }
            }
            if (isFloatType(type)) {
                auto floatObj = makeFloatObj(type, addr);
                if (floatObj){
                    floatObj->setName(name);
                    floatObj->setType(type);
                    return floatObj.release();
                }
            }
            if (isStringType(type) && addr) {
                auto stringObj = new StringObj(*static_cast<std::string*>(addr));
                stringObj->setName(name);
                stringObj->setType(type);
                return stringObj;
            }
            return new ObjTreeCont(name + " = " + objMap->get(), type);
        }

        // Container
        if (objMap->isContainer() || isContainerType(type)) {
            const auto& variables = objMap->getVariables();
            auto* container = new ContainerObj(name, type, variables.size());
            for (const auto& [childName, childMap] : variables) {
                ObjTreeCont* child = convertNode(childName, childMap);
                if (child) container->addChildObj(child);
            }
            return container;
        }

        // BaseComponent (using category flag)
        if (objMap->getCategory() == ObjectMap::ObjectCategory::Component) {
            auto* comp = static_cast<BaseComponent*>(objMap->getAddr());
            if (comp){
                auto* compObj = new ComponentObj(comp, nullptr);
                compObj->setName(name);
                compObj->setType(objMap->getType());
                return compObj;
            } 
        }

        // Generic
        return new ObjTreeCont(name, type);
    }

    static void addChildrenFromMap(ObjTreeCont* parent, const ObjectMultimap& variables) {
        if (!parent) return;
        for (const auto& [name, objMap] : variables) {
            ObjTreeCont* child = convertNode(name, objMap);
            if (child) parent->addChildObj(child);
        }
    }

     static void addChildrenFromMapRecursive(ObjTreeCont* parent, const ObjectMultimap& variables) {
        if (!parent) return;
        for (const auto& [name, objMap] : variables) {
            ObjTreeCont* child = convert(name, objMap);
            if (child) parent->addChildObj(child);
        }
    }

    // Convert an entire ObjectMap's variables into an ObjTreeCont tree
    static std::unique_ptr<ObjTreeCont> convertTree(const std::string& rootName, ObjectMap* objMap) {
        auto root = std::make_unique<ObjTreeCont>(rootName, objMap->getType());
        const auto& variables = objMap->getVariables();
        for (const auto& [name, childMap] : variables) {
            ObjTreeCont* child = convert(name, childMap);
            if (child) {
                root->addChildObj(child);
            }
        }
        return root;
    }

     /**
     * Takes a ComponentObj, serializes its BaseComponent via
     * ObjectMapDeferred, and populates the ComponentObj's children
     * with the serialized variables.
     *
     * @param compNode  The ComponentObj node to expand
     * @param recursive If true, recursively convert all children
     * @return true if serialization succeeded
     */
    static bool serializeComponent(ComponentObj* compNode, bool recursive = false) {
        if (!compNode) return false;

        BaseComponent* comp = compNode->getVal();
        if (!comp) return false;

       ComponentInfo* compInfo = compNode->getInfo();


        // Create a temporary deferred map and trigger serialization
        ComponentSerializer serializer(comp);
        serializer.serialize();

         if (serializer.hasSerialized()) {
        const auto& variables = serializer.getVariables();

        // Collect sub-component addresses so we skip them during conversion
        std::vector<void*> subCompAddrs;
        if (compInfo) {
            collectSubComponentAddrs(compInfo, subCompAddrs);
        }

        for (const auto& [name, objMap] : variables) {
            if (!objMap) continue;

            // Skip variables that are sub-components
            if (std::find(subCompAddrs.begin(), subCompAddrs.end(), objMap->getAddr()) != subCompAddrs.end()) continue;

            ObjTreeCont* child = recursive 
                ? convert(name, objMap) 
                : convertNode(name, objMap);
            if (child) compNode->addChildObj(child);
        }
    }

    // Serialize sub-components from ComponentInfo
    if (compInfo) {
        serializeSubComponents(compNode, compInfo, recursive);
    }

        //if (!serializer.hasSerialized()) return false;

        // Get the serialized variables and convert them to tree nodes
//        const auto& variables = serializer.getVariables();
     /*   if (recursive) {
            addChildrenFromMapRecursive(compNode, variables);
        }
        else {
            addChildrenFromMap(compNode, variables);
        }*/

        return true;
    }

    /**
     * Serialize all ComponentObj children of a given node.
     *
     * @param parent    The parent node whose ComponentObj children to expand
     * @param recursive If true, recursively convert all grandchildren
     */
    static void serializeAllComponents(ObjTreeCont* parent, bool recursive = false) {
        if (!parent) return;

        parent->applyRecursiveByType<ComponentObj>([recursive](ComponentObj* comp) {
            serializeComponent(comp, recursive);
        });
    }

    /**
     * Full pipeline: convert an ObjectMap tree, then serialize
     * any ComponentObj nodes found in the result.
     *
     * @param rootName  Name for the root node
     * @param objMap    Source ObjectMap to convert
     * @param recursive If true, fully expand all levels
     * @return Root of the converted and serialized tree
     */
    static std::unique_ptr<ObjTreeCont> convertAndSerialize(
        const std::string& rootName, ObjectMap* objMap, bool recursive = true)
    {
        auto root = convertTree(rootName, objMap);
        if (!root) return nullptr;

        // Serialize all component nodes in the tree
        serializeAllComponents(root.get(), recursive);

        return root;
    }

    private:
    static void collectSubComponentAddrs(ComponentInfo* compInfo,
                                          std::vector<void*>& addrs) {
        auto& subComps = compInfo->getSubComponents();

        for (auto it = subComps.begin(); it != subComps.end(); ++it) {

            BaseComponent* sub = it->second.getComponent();
            if (sub) {
                addrs.push_back(static_cast<void*>(sub));
            }
        }

    }

    static void serializeSubComponents(ObjTreeCont* parent,
                                        ComponentInfo* compInfo,
                                        bool recursive) {
        auto& subComps = compInfo->getSubComponents();
        for (auto& [compId, subInfo] : subComps) {
            BaseComponent* sub = subInfo.getComponent();
            if (!sub) continue;

            auto* subObj = new ComponentObj(sub, &subInfo);
            subObj->setName(subInfo.getName());
            subObj->setType(subInfo.getType());

            // Serialize this sub-component's own variables
            ComponentSerializer subSerializer(sub);
            subSerializer.serialize();

            if (subSerializer.hasSerialized()) {
                const auto& subVars = subSerializer.getVariables();

                // Collect nested sub-component addresses
                std::vector<void*> nestedAddrs;
                collectSubComponentAddrs(&subInfo, nestedAddrs);

                for (const auto& [name, objMap] : subVars) {
                    if (!objMap) continue;
                    if (std::find(nestedAddrs.begin(), nestedAddrs.end(), objMap->getAddr()) != nestedAddrs.end()) continue;

                    ObjTreeCont* child = recursive
                        ? convert(name, objMap)
                        : convertNode(name, objMap);
                    if (child) subObj->addChildObj(child);
                }
            }

            // Recurse into this sub-component's own sub-components
            if (recursive) {
                serializeSubComponents(subObj, &subInfo, recursive);
            }

            parent->addChildObj(subObj);
        }
    }
};

}

#endif