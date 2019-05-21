//
// Created by hariharan on 5/21/19.
//
#include <basket/common/data_structures.h>
/**
 * Outstream conversions
 */
std::ostream &operator<<(std::ostream &os, char const *m) {
    return os << std::string(m);
}
std::ostream &operator<<(std::ostream &os, uint8_t const &m) {
    return os << std::to_string(m);
}
std::ostream &operator<<(std::ostream &os, CharStruct const &m){
    return os   << "{TYPE:CharStruct," << "value:" << m.c_str()<<"}";
}