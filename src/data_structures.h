/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html. COPYING can be found at the root    *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page. It can also be found at      *
 * http://hdfgroup.org/HDF5/doc/Copyright.html. If you do not have           *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
*
* Created: data_structures.h
* May 28 2018
* Hariharan Devarajan <hdevarajan@hdfgroup.org>
*
* Purpose: Defines all data structures required in the Hermes Platform.
*
*-------------------------------------------------------------------------
*/

#ifndef HERMES_DATA_STRUCTURES_H
#define HERMES_DATA_STRUCTURES_H

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>

using namespace std;
namespace bip=boost::interprocess;

typedef long t_mili;
typedef struct CharStruct{
private:
    char value[256];
public:
    CharStruct(){}
    CharStruct(std::string data_){
        sprintf(this->value,"%s",data_.c_str());
    }
    CharStruct(bip::string data_){
        sprintf(this->value,"%s",data_.c_str());
    }
    CharStruct(char* data_, size_t size){
        sprintf(this->value,"%s",data_);
    }
    const char* c_str() const{
        return value;
    }

    char* data(){
        return value;
    }
    const size_t size() const{
        return strlen(value);
    }
    /* equal operator for comparing two Matrix. */
    bool operator==(const CharStruct &o) const {
        return strcmp(value,o.value)==0;
    }
} CharStruct;

#endif //HERMES_DATA_STRUCTURES_H
