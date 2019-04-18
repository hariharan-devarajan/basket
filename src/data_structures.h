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

#ifndef SRC_DATA_STRUCTURES_H_
#define SRC_DATA_STRUCTURES_H_

#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
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

#endif  // SRC_DATA_STRUCTURES_H_
