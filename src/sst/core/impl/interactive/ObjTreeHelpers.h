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

#ifndef SST_CORE_SERIALIZATION_OBJECTTREEHELPERS_DEBUGGER_H
#define SST_CORE_SERIALIZATION_OBJECTTREEHELPERS_DEBUGGER_H

#include "sst/core/impl/interactive/ObjTree.h"

namespace SST::Core::Serialization {

class ObjTreeComparison
{
public:
    enum class Op : std::uint8_t { LT, LTE, GT, GTE, EQ, NEQ, CHANGED, INVALID };
    enum class valType : std::uint8_t{ OBJ, CONST, UNKNOWN };
    static Op getOperationFromString(const std::string& op)
    {
        if ( op == "<" ) return Op::LT;
        if ( op == "<=" ) return Op::LTE;
        if ( op == ">" ) return Op::GT;
        if ( op == ">=" ) return Op::GTE;
        if ( op == "==" ) return Op::EQ;
        if ( op == "!=" ) return Op::NEQ;
        if ( op == "changed" ) return Op::CHANGED; // Could also use <>
        return Op::INVALID;
    }

    static std::string getStringFromOp(Op op)
    {
        switch ( op ) {
        case Op::LT:
            return "<";
        case Op::LTE:
            return "<+";
        case Op::GT:
            return ">";
        case Op::GTE:
            return ">=";
        case Op::EQ:
            return "==";
        case Op::NEQ:
            return "!=";
        case Op::CHANGED:
            return "CHANGED";
        case Op::INVALID:
            return "INVALID";
        default:
            return "Invalid Op";
        }
    }
    //TODO: add some limits on these
    //The full path and name to the objects for compairson. Vector contains the full path to each object stored as individual
    // elements within the dequeue. Ex: /comp0/subcomp/target --> [comp0][subcomp][target] within the dequeue 
    std::vector<std::tuple<std::deque<std::string>, valType, std::unique_ptr<ObjTreeCont>>> objectsToCompare;
    std::vector<Op> operators;

    void print(std::stringstream& s) const {
        if(objectsToCompare.empty() || operators.empty()){ return; }
        for(size_t i = 0; i < objectsToCompare.size(); i++){
            s << std::get<std::deque<std::string>>(objectsToCompare[i]).back();
            if((i % 2 == 0) &&  (i/2 < operators.size())){
                s << " " << getStringFromOp(operators[i/2]);
            }
            s << " ";
        } 
        s << std::endl;
    }

    bool evaluateComparison(SST::Core::Serialization::ObjTreeCont* treeRoot);

    ObjTreeComparison() = default;
    ~ObjTreeComparison()= default;


};
}
#endif