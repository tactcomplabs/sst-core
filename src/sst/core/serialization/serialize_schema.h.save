// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_SCHEMA_H
#define SST_CORE_SERIALIZATION_SERIALIZE_SCHEMA_H

#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace SST::Core::Serialization {

class serialize_schema {
public:
    serialize_schema(std::string& schema_filename);
    virtual ~serialize_schema();
    void update(std::string name, size_t pos, size_t hash_code, size_t sz, std::string type_name);
    void flush_segment( std::string name, size_t size);

private:
    std::ofstream sfs;
    std::map<size_t, std::pair<std::string, size_t>> type_map;           // type hash_code, <name, size>
    std::vector<std::tuple<std::string, size_t, size_t>> namepos_vector; // variable name, position, hash_code
};

} //namespace SST::CORE::Serialization

#define SER_INI(obj) \
    if (ser.schema_enabled()) { \
        ser.schema()->update(  \
            #obj, ser.size(),   \
            typeid(obj).hash_code(), sizeof(obj), typeid(obj).name()); \
    } \
    ser& obj;

// TODO #define SER_INI_PTR(obj) ser | obj;

#define SER_SEG_DONE( name, size )

#define SER_COMPONENTS_START(compInfoMap, size)

#define SER_COMPONENTS_END()


#endif //SST_CORE_SERIALIZATION_SERIALIZE_SCHEMA_H