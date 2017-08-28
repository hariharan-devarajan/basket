//
// Created by hdevarajan on 8/24/17.
//

#ifndef MPIONESIDED_TYPES_H_H
#define MPIONESIDED_TYPES_H_H

#include <vector>
#include <cstdio>
#include <mpi.h>
#include <string>
#include <iostream>

template<typename K>
struct Key {
  virtual bool equals(K)=0;

  virtual size_t size()=0;

  virtual size_t hash_code()=0;

  virtual std::string to_string()=0;

  virtual std::vector<MPI_Datatype> get_types()=0;

  virtual std::vector<MPI_Aint> get_displacements()=0;

  virtual std::vector<int> get_block_lengths()=0;

  friend std::ostream &operator<<(std::ostream &os, K &m) {
    os << m.to_string() << std::endl;
    return os;
  }


};

template<typename V>
struct Value {
  virtual size_t size()=0;

  virtual std::string to_string()=0;

  virtual std::vector<MPI_Datatype> get_types()=0;

  virtual std::vector<MPI_Aint> get_displacements()=0;

  virtual std::vector<int> get_block_lengths()=0;

  friend std::ostream &operator<<(std::ostream &os, V &m) {
    os << m.to_string() << std::endl;
    return os;
  }
};

template<typename Key, typename Value>
struct Node {
  Value value;
  Key key;
  size_t size() {
    return sizeof(Node<Key, Value>);
  }

  Node() : key(), value() {}

  friend std::ostream &operator<<(std::ostream &os, Node &m) {
    std::string key = "";
    std::string value = "";
    os << "{key:" << m.key.to_string()
       << ", value:" << m.value.to_string()
       << "}"
       << std::endl;
    return os;
  }

  Node(const Node &n) : key(n.key), value(n.value) {

  }
};


#endif //MPIONESIDED_TYPES_H_H
