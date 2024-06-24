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

#if 1
#define SER_INI(obj) \
   if (ser.dump_schema()) { std::cout << "#V," << #obj << " {"; } \
   ser& obj; \
   if (ser.dump_schema()) { std::cout << ser.size() << "}," << std::endl; };
// TODO #define SST_SER_AS_PTR_DBG(obj) ser | obj;
#define SER_MARKER( tag, name, size ) \
    if (ser.dump_schema()) { std::cout << tag << "," << name << "}," << size << std::endl; }
#else
#define SER_INI( obj ) ser& var
#define SER_MARKER
#endif

#endif //SST_CORE_SERIALIZATION_SERIALIZE_SCHEMA_H