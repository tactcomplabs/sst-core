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


#include "sst/core/impl/interactive/ObjTreeHelpers.h"
#include "sst/core/impl/interactive/NumericHandle.h"

namespace SST::Core::Serialization {

bool 
ObjTreeComparison::evaluateComparison(SST::Core::Serialization::ObjTreeCont* treeRoot){

    //Get the error conditions out of the way
    if(objectsToCompare.empty() || operators.empty() || (operators[0] == Op::INVALID)){ return false; }

    //How many objects we got?
    if(1 == objectsToCompare.size() ){ //Just 1? means we got a CHANGED
        if( operators[0] != Op::CHANGED ){
            std::cout << "Invalid Watchpoint compairson" << std::endl;
            return false;
        }else{
            return std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[0])->hasChanged();
        }
    }else if( 2 == objectsToCompare.size() ){ // 2 means we have a compairson
        //which sides hold objects, and which side (if any) holds constants?
        //auto& lhsTuple = objectsToCompare[0];
        //auto& rhsTuple = objectsToCompare[1];

        std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[0])->syncFromSim();
        std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[1])->syncFromSim();

//        Core::Serialization::ObjTreeCont* lhsObj = (std::get<valType>(lhsTuple) == valType::OBJ)
//                        ? std::get<std::unique_ptr<ObjTreeCont>>(lhsTuple).get()
//                        : nullptr;

//        Core::Serialization::ObjTreeCont* rhsObj = (std::get<valType>(rhsTuple) == valType::OBJ)
//                        ? std::get<std::unique_ptr<ObjTreeCont>>(rhsTuple).get()
 //                       : nullptr;  
         
        auto lhs = SST::Core::Serialization::NumericHandle::from(std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[0]).get());
        auto rhs = SST::Core::Serialization::NumericHandle::from(std::get<std::unique_ptr<Core::Serialization::ObjTreeCont>>(objectsToCompare[1]).get());
        switch(operators[0]){
            case Op::LT:  return lhs <  rhs;
            case Op::LTE: return lhs <= rhs;
            case Op::GT:  return lhs >  rhs;
            case Op::GTE: return lhs >= rhs;
            case Op::EQ:  return lhs == rhs;
            case Op::NEQ: return lhs != rhs;
            default:      return false;
        }
    }


}


};
