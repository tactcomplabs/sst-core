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

#define SER_INI(obj) \
    if (ser.dump_schema()) { \
        ser.update_schema( \
            #obj, ser.size(), \
            typeid(obj).hash_code(), sizeof(obj), typeid(obj).name()); \
    } \
    ser& obj;
// TODO #define SER_INI_PTR(obj) ser | obj;
// #define SER_MARKER( tag, name, size ) \
//     if (ser.dump_schema()) { std::cout << tag << "," << name << "}," << size << std::endl; }
#define SER_MARKER( tag, name, size )


#endif //SST_CORE_SERIALIZATION_SERIALIZE_SCHEMA_H