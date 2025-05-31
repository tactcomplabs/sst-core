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

#include "sst_config.h"

#include "sst/core/serialization/serializer.h"

#include "sst/core/output.h"
#include "sst/core/serialization/serializable.h"

namespace SST::Core::Serialization {
namespace pvt {

void
ser_unpacker::unpack_buffer(void* buf, size_t size)
{
    if ( size == 0 ) {
        Output& output = Output::getDefaultObject();
        output.fatal(__LINE__, __FILE__, "ser_unpacker::unpack_buffer", 1, "trying to unpack buffer of size 0");
    }
    // void* only for convenience... actually a void**
    void** bufptr = (void**)buf;
    *bufptr       = new char[size];
    char* charstr = next_str(size);
    ::memcpy(*bufptr, charstr, size);
}

void
ser_packer::pack_buffer(void* buf, size_t size)
{
    if ( buf == nullptr ) {
        Output& output = Output::getDefaultObject();
        output.fatal(__LINE__, __FILE__, "ser_packer::pack_bufffer", 1, "trying to pack nullptr buffer");
    }
    char* charstr = next_str(size);
    ::memcpy(charstr, buf, size);
}

void
ser_unpacker::unpack_string(std::string& str)
{
    int size;
    unpack(size);
    char* charstr = next_str(size);
    str.resize(size);
    str.assign(charstr, size);
}

void
ser_packer::pack_string(std::string& str)
{
    int size = str.size();
    pack(size);
    char* charstr = next_str(size);
    ::memcpy(charstr, str.data(), size);
}

void
ser_sizer::size_string(std::string& str)
{
    size_ += sizeof(int);
    size_ += str.size();
}

} // namespace pvt


void
serializer::string(std::string& str)
{
    switch ( mode_ ) {
    case SIZER:
    {
        sizer_.size_string(str);
        break;
    }
    case PACK:
    {
        packer_.pack_string(str);
        break;
    }
    case UNPACK:
    {
        unpacker_.unpack_string(str);
        break;
    }
    case MAP:
        break;
    }
}

serialize_schema::serialize_schema(const std::string& schema_filename) { 
    sfs = std::ofstream(schema_filename, std::ios::out);
    sfs << "{ ";
    sfs << q << "checkpoint_def" << q << " : [\n";
 }


void serialize_schema::close() {
    if (sfs.is_open()) {
        sfs << "]}\n";
        sfs.flush();
        sfs.close();
    }
}
serialize_schema::~serialize_schema() {   
    this->close();
}

void
serialize_schema::update(std::string name, size_t pos, size_t hash_code, size_t sz, std::string type_name)
{
    namepos_vector.push_back(std::make_tuple(name, pos, hash_code));
    if (type_map.find(hash_code) != type_map.end()) return;
    type_map[hash_code] = std::make_pair(type_name,sz);
}

void serialize_schema::write_segment(std::string name, size_t size, bool inc_size)
{
    //  "seg_name" : "stuff",
    //  "seg_num"  : "2",
    //  "seg_size" : "8",
    //  "names" :
    //  [
    //     {"name" : "fubar" , "pos" : "0" , "hash_code" : "0x2343f2321" },
    //     {"name" : "snafu" , "pos" : "4" , "hash_code" : "0x1cde3123" }
    //  ]

    std::string term = "";

    // The first entry is always size_t size. We can use this as a consistency check.
    // Just need to offset all the positions by size_of(size_t);
    size_t offset = inc_size ? sizeof(size_t) : 0;
    size += offset;
    sfs << "{\n";
    sfs << q << "rec_type" << q << " : " << q             << "seg_info" << q << ",\n"
        << q << "seg_name" << q << " : " << q             << name       << q << ",\n"
        << q << "seg_num"  << q << " : " << q << std::dec << seg_num++  << q << ",\n"
        << q << "seg_size" << q << " : " << q << std::dec << size       << q << ",\n"
        << q << "names"    << q << " :\n[\n";
    term = "";

    for (auto r : namepos_vector) {
        sfs << term;
        sfs << sp << "{" 
            << q << "name"      << q << " : " << q <<                      std::get<0>(r) << q << " , " 
            << q << "pos"       << q << " : " << q << std::dec << offset + std::get<1>(r) << q << " , "
            << q << "hash_code" << q << " : " << q << "0x" << std::hex <<  std::get<2>(r) << q
            << " }";
        term = ",\n";
    }
    sfs << "\n]\n},\n";

    // reset collection
    namepos_vector.clear();
} 

void serialize_schema::write_types()
{
        std::string term = "";
        std::string sp = "   ";

        // "type_info" [ { "hash_code" : "0x1234", "name" : "fubar", "size" : "8" }, ... ]
        sfs << "{\n";
        sfs << q << "rec_type"  << q << " : " << q             << "type_info" << q << ",\n";
        sfs << q << "type_info" << q << ": [\n";
        for (auto it=type_map.begin(); it != type_map.end(); ++it) {
            auto r = it->second;
            sfs << term;
            sfs << sp << "{" 
                << q << "hash_code" << q << " : " << q << "0x" << std::hex << it->first  << q << " , " 
                << q << "name" << q << " : " << q << r.first  << q << " , " 
                << q << "size"  << q << " : " << q << std::dec << r.second << q 
                << " }";
            term = ",\n";
        }
        sfs << "\n]\n}\n";

        // reset collection
        type_map.clear();
}

void
serializer::report_object_map(ObjectMap* ptr)
{
    ser_pointer_map[reinterpret_cast<uintptr_t>(ptr->getAddr())] = reinterpret_cast<uintptr_t>(ptr);
}

const char*
serializer::getMapName() const
{
    if ( !mapContext ) throw std::invalid_argument("Internal error: Empty map name when map serialization requires it");
    return mapContext->getName();
}

} // namespace SST::Core::Serialization
