#ifndef SST_CORE_SERIALIZATION_OBJECTMAP_DEBUGGER_H
#define SST_CORE_SERIALIZATION_OBJECTMAP_DEBUGGER_H

#include "sst/core/from_string.h"
#include "sst/core/warnmacros.h"
#include "sst/core/componentInfo.h"
#include "sst/core/baseComponent.h"

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

namespace SST::Core::Serialization {

    class ObjTreeCont
    {
        public:
        ObjTreeCont() : parent_(nullptr), children_(){};
        virtual ~ObjTreeCont() = default;

        void addChildObj(ObjTreeCont* obj){
            children_.push_back(obj);
            obj->parent_ = this;
        }

        ObjTreeCont* getParent() const                       { return parent_;}
        const std::vector<ObjTreeCont*>& getChildren() const { return children_; }
        
        template<typename Func>
        void applyRecursive(Func&& func){
            for (auto* child : children_){
                func(child);
            }
        }

        virtual void apply() = 0;
        virtual std::string getTypeName() const = 0;

        protected:
        ObjTreeCont*              parent_;
        std::vector<ObjTreeCont*> children_;

    };

    class ComponentObj;
    template<typename Obj_T>
    class ObjTree : public ObjTreeCont
    {
        public:
        ObjTree(const ComponentInfoMap* compMap);

        Obj_T& getObj(){ return static_cast<Obj_T&>(*this);}
        const Obj_T& getObj() const { return static_cast<const Obj_T&>(*this);}

       /* template<typename T>
        ObjTreeCont<T>*              findVariable(const std::string& name) const;

        template<typename T>
        std::vector<ObjTreeCont<T>*> findGlobal(const std::string& name) const;
        */
        template<typename ObjectType, typename Func>
        void appyRecursiveByType(Func&& func){
            for (auto* child : children_){
                if(auto* child_t = dynamic_cast<ObjectType*>(child)){
                    func(child_t);
                }
            }
        }

        std::string getTypeName() const override {
            return typeid(Obj_T).name();
        }

        protected:
        std::multimap<std::string, ComponentObj> objects_;

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
    };

    class FloatObj : public ObjTree<FloatObj> {
        public:
        using FloatVariant = std::variant<float, double, long double>;
        
        private:
        FloatVariant val_;

        public:
        template<typename T>
        FloatObj(T v) : val_(v) {}

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
    };

    class ComponentObj : public ObjTree<ComponentObj> 
    {
        
        private:
        std::shared_ptr<SST::BaseComponent> val_ = nullptr;

        public:
        ComponentObj() = delete;
        ComponentObj(std::shared_ptr<BaseComponent> v) : val_(std::move(v)) {}

        BaseComponent* getVal() const{ return val_.get(); }

        //void setVal(BaseComponent* v){ }

        void apply() override {
            std::cout << "Processing component: " << val_->getName() << std::endl;
        }
    };

        template<typename Obj_T>
    ObjTree<Obj_T>::ObjTree(const ComponentInfoMap* compMap){
        for ( auto comp = compMap->begin(); comp != compMap->end(); comp++ ) {
        ComponentInfo* compinfo = *comp;
        objects_.emplace(compinfo->getName(), ComponentObj(compinfo->getComponent()));
        }
    }


}

#endif