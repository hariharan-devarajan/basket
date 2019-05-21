// Copyright 2019 Hariharan Devarajan
/*-------------------------------------------------------------------------
 *
 * Created: data_structures.h
 * May 28 2018
 * Hariharan Devarajan <hdevarajan@hdfgroup.org>
 *
 * Purpose: Defines all data structures required in Basket.
 *
 *-------------------------------------------------------------------------
 */

#ifndef SRC_BASKET_COMMON_DATA_STRUCTURES_H_
#define SRC_BASKET_COMMON_DATA_STRUCTURES_H_

#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <rpc/msgpack.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace bip = boost::interprocess;

typedef struct CharStruct {
 private:
  char value[256];
 public:
  CharStruct() {}
  CharStruct(std::string data_) {
    snprintf(this->value, sizeof(this->value), "%s", data_.c_str());
  }
  CharStruct(bip::string data_) {
    snprintf(this->value, sizeof(this->value), "%s", data_.c_str());
  }
  CharStruct(char* data_, size_t size) {
    snprintf(this->value, sizeof(this->value), "%s", data_);
  }
  const char* c_str() const {
    return value;
  }

  char* data() {
    return value;
  }
  const size_t size() const {
    return strlen(value);
  }
  /* equal operator for comparing two Matrix. */
  bool operator==(const CharStruct &o) const {
    return strcmp(value, o.value) == 0;
  }
} CharStruct;
namespace std {
template<>
struct hash<CharStruct> {
  size_t operator()(const CharStruct &k) const {
    std::string val(k.c_str());
    return std::hash<std::string>()(val);
  }
};
}

namespace clmdep_msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {
namespace mv1 = clmdep_msgpack::v1;
template<>
struct convert<CharStruct> {
  mv1::object const &operator()(mv1::object const &o,
                                CharStruct &input) const {
    std::string v = std::string();
    v.assign(o.via.str.ptr, o.via.str.size);
    input = CharStruct(v);
    return o;
  }
};

template<>
struct pack<CharStruct> {
  template<typename Stream>
  packer <Stream> &operator()(mv1::packer <Stream> const &o,
                              CharStruct const &input) const {
    uint32_t size = checked_get_container_size(input.size());
    o.pack_str(size);
    o.pack_str_body(input.c_str(), size);
    return o;
  }
};

template<>
struct object_with_zone<CharStruct> {
  void operator()(mv1::object::with_zone &o,
                  CharStruct const &input) const {
    uint32_t size = checked_get_container_size(input.size());
    o.type = clmdep_msgpack::type::STR;
    char *ptr = static_cast<char *>(
        o.zone.allocate_align(size, MSGPACK_ZONE_ALIGNOF(char)));
    o.via.str.ptr = ptr;
    o.via.str.size = size;
    std::memcpy(ptr, input.c_str(), input.size());
  }
};
}  // namespace adaptor
}
}  // namespace clmdep_msgpack


/**
 * Outstream conversions
 */

std::ostream &operator<<(std::ostream &os, char const *m);
template <typename T,typename O>
std::ostream &operator<<(std::ostream &os, std::pair<T,O> const m){
    return os   << "{TYPE:pair," << "first:" << m.first << ","
                << "second:" << m.second << "}";
}
std::ostream &operator<<(std::ostream &os, uint8_t const &m);
std::ostream &operator<<(std::ostream &os, CharStruct const &m);
template <typename T>
std::ostream &operator<<(std::ostream &os, std::vector<T> const &ms){
    os << "[";
    for(auto m:ms){
        os <<m<<",";
    }
    os << "]";
    return os;
}
#endif  // SRC_BASKET_COMMON_DATA_STRUCTURES_H_
