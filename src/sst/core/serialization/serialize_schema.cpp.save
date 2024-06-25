#include "serialize_schema.h"

namespace SST::Core::Serialization {

serialize_schema::serialize_schema(std::string& pfx) { 
    std::string schema_filename  = "schema.json";
    if (pfx != "")
        schema_filename = pfx + "_" + schema_filename;
    sfs = std::ofstream(schema_filename, std::ios::out);
 }

serialize_schema::~serialize_schema() {   
    if (sfs) sfs.close();
}

void
serialize_schema::update(std::string name, size_t pos, size_t hash_code, size_t sz, std::string type_name)
{
    namepos_vector.push_back(std::make_tuple(name, pos, hash_code));
    if (type_map.find(hash_code) != type_map.end()) return;
    type_map[hash_code] = std::make_pair(type_name,sz);
}

void serialize_schema::flush_segment(std::string name, size_t size)
{
        char q = '\"';
        std::string sp = "   ";
        std::string sp2 = sp + sp;
        sfs << "{\n" ;

        // "type_info" [ { "hash_code" : "0x1234", "name" : "fubar", "size" : "8" }, ... ]
        sfs << q << "type_info" << q << ": [\n";
        std::string term = "";
        for (auto it=type_map.begin(); it != type_map.end(); ++it) {
            auto r = it->second;
            sfs << term;
            sfs << sp << "{" 
                << q << "hash_code" << q << " : " << q << "0x" << it->first  << q << " , " 
                << q << "name" << q << " : " << q << r.first  << q << " , " 
                << q << "size"  << q << " : " << q << r.second << q 
                << " }";
            term = ",\n";
        }
        sfs << "\n],\n";

        // "name_pos" [ { "name" : "fubar", "pos" : "8", "hash_code" : "0x1234"}, ... ]
        sfs << q << "name_pos" << q << ": [\n";
        term = "";
        for (auto r : namepos_vector) {
            sfs << term;
            sfs << sp << "{" 
                << q << "name"      << q << " : " << q << std::get<0>(r) << q << " , " 
                << q << "pos"       << q << " : " << q << std::get<1>(r) << q << " , "
                << q << "hash_code" << q << " : " << q << std::get<2>(r) << q
                << " }";
            term = ",\n";
        }
        sfs << "\n]\n";

        sfs << "}\n";

        // reset collections

}

} //namespace SST::Core::Serialization