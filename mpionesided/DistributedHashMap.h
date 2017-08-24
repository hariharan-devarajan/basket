//
// Created by hdevarajan on 8/22/17.
//

#ifndef MPIONESIDED_DISTRIBUTEDHASHMAP_H
#define MPIONESIDED_DISTRIBUTEDHASHMAP_H
#include "mpi.h"
#include "debug.h"
#include <vector>
#include <iostream>
#include <cstring>

template<typename K>
struct Key{
  virtual bool equals(K)=0;
  virtual size_t size()=0;
  virtual size_t hash_code()=0;
  virtual std::string to_string()=0;
  virtual std::vector<MPI_Datatype> get_types()=0;
  virtual std::vector<MPI_Aint> get_displacements()=0;
  virtual std::vector<int> get_block_lengths()=0;
  friend std::ostream& operator << (std::ostream& os, K& m)
  {
    os << m.to_string() << std::endl;
    return os ;
  }


};

template<typename V>
struct Value{
  virtual size_t size()=0;
  virtual std::string to_string()=0;
  virtual std::vector<MPI_Datatype> get_types()=0;
  virtual std::vector<MPI_Aint> get_displacements()=0;
  virtual std::vector<int> get_block_lengths()=0;
  friend std::ostream& operator << (std::ostream& os, V& m)
  {
    os << m.to_string() << std::endl;
    return os ;
  }
};

template<typename Key,typename Value>
struct Node{
  Value value;
  Key key;
  size_t size(){
    return sizeof(Node<Key,Value>);
  }
  Node():key(),value(){}
  friend std::ostream& operator << (std::ostream& os, Node& m)
  {
    std::string key="";
    std::string value="";
    os << "{key:" << m.key.to_string()
       <<", value:" << m.value.to_string()
       << "}"
       << std::endl;
    return os ;
  }
  Node(const Node& n):key(n.key),value(n.value){

  }
};

template<typename Key,typename Value>
class DistributedHashMap {
public:
  int bucket_size=256;
  const int allowed_conflicts=10;
private:
  int comm_size;
  MPI_Win key_bucket_win;
  MPI_Win value_bucket_win;
  MPI_Win bucket_size_win;
  MPI_Win presence_win;
  int rank;
  MPI_Datatype key_type;
  MPI_Datatype value_type;
  Key **key_buckets;
  Value **value_buckets;
  int **presence;
  void buildTypes() {
    Key k;
    Value v;
    int size=k.get_types().size();
    MPI_Type_struct(size, &k.get_block_lengths()[0], &k.get_displacements()[0],&k.get_types()[0], &key_type);
    MPI_Type_commit(&key_type);
    size=v.get_types().size();
    MPI_Type_struct(size, &v.get_block_lengths()[0], &v.get_displacements()[0], &v.get_types()[0], &value_type);
    MPI_Type_commit(&value_type);
  }

  void putNode(Node<Key, Value> &n, int server, int displacement){
#ifdef DEBUG
    DBGVAR3(std::cout, n,server,displacement);
    int objectsize=n.size();
    DBGVAR2(std::cout, key_bucket_win,value_bucket_win);
#endif
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, server, 0, key_bucket_win);
    int error=MPI_Put(&n.key,1,key_type,server,displacement,1,key_type,key_bucket_win);
    int error2=MPI_Win_unlock(server, key_bucket_win);
#ifdef DEBUG
    DBGVAR3(std::cout, n,error,error2);
#endif
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, server, 0, value_bucket_win);
    error=MPI_Put(&n.value,1,value_type,server,displacement,1,value_type,value_bucket_win);
    error2=MPI_Win_unlock(server, value_bucket_win);
#ifdef DEBUG
    DBGVAR3(std::cout, n,error,error2);
#endif
  }
  void putPresence(int &presence, int server, int displacement){
#ifdef DEBUG
    DBGVAR3(std::cout, presence,server,displacement);
    DBGVAR(std::cout, presence_win);
#endif
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, server, 0, presence_win);
    int error=MPI_Put(&presence,1,MPI_INT,server,displacement,1,MPI_INT,presence_win);
    int error2=MPI_Win_unlock(server, presence_win);
#ifdef DEBUG
    DBGVAR3(std::cout, presence,error,error2);
#endif
  }

  void getNode(Node<Key, Value> &n, int server, int displacement){
#ifdef DEBUG
    DBGVAR3(std::cout, n,server,displacement);
    int objectsize=n.size();
    DBGVAR2(std::cout, key_bucket_win,value_bucket_win);
#endif
    MPI_Win_lock(MPI_LOCK_SHARED, server, 0, key_bucket_win);
    int error=MPI_Get(&n.key,1,key_type,server,displacement,1,key_type,key_bucket_win);
    int error2=MPI_Win_unlock(server, key_bucket_win);
#ifdef DEBUG
    DBGVAR3(std::cout, n,error,error2);
#endif

    MPI_Win_lock(MPI_LOCK_SHARED, server, 0, value_bucket_win);
    error=MPI_Get(&n.value,1,value_type,server,displacement,1,value_type,value_bucket_win);
    error2=MPI_Win_unlock(server, value_bucket_win);
#ifdef DEBUG
    DBGVAR3(std::cout, n.value,error,error2);
#endif
  }

  void getPresence(int &presence, int server, int displacement){
#ifdef DEBUG
    DBGVAR3(std::cout, presence,server,displacement);
#endif
    MPI_Win_lock(MPI_LOCK_SHARED, server, 0, presence_win);
    int error=MPI_Get(&presence,1,MPI_INT,server,displacement,1,MPI_INT,presence_win);
    int error2=MPI_Win_unlock(server, presence_win);
#ifdef DEBUG
    DBGVAR3(std::cout, presence,error,error2);
#endif
  }

  void get_bucket_size(int &bucket_size,int server){
#ifdef DEBUG
    DBGVAR3(std::cout, bucket_size,server,rank);
#endif
    MPI_Win_lock(MPI_LOCK_SHARED, server, 0, bucket_size_win);
    int error=MPI_Get(&bucket_size,1,MPI_INT,server,0,1,MPI_INT,bucket_size_win);
    int error2=MPI_Win_unlock(server, bucket_size_win);
#ifdef DEBUG
    DBGVAR3(std::cout, bucket_size,error,error2);
#endif
  }
  inline void init(){
    key_buckets = new Key*[bucket_size];
    for(int i = 0; i < bucket_size; ++i)
      key_buckets[i] = new Key[allowed_conflicts];
    value_buckets = new Value*[bucket_size];
    for(int i = 0; i < bucket_size; ++i)
      value_buckets[i] = new Value[allowed_conflicts];
    presence=new int*[bucket_size];
    for(int i = 0; i < bucket_size; ++i)
      presence[i] = new int[allowed_conflicts];

    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    Node<Key,Value> n;
    buildTypes();

    MPI_Win_create( key_buckets,bucket_size * allowed_conflicts*sizeof(Key),sizeof(Key) , MPI_INFO_NULL, MPI_COMM_WORLD, &key_bucket_win);
    MPI_Win_create( value_buckets,bucket_size * allowed_conflicts*sizeof(Value),sizeof(Value) , MPI_INFO_NULL, MPI_COMM_WORLD, &value_bucket_win);
    MPI_Win_create( presence,bucket_size * allowed_conflicts*sizeof(int),sizeof(int) , MPI_INFO_NULL, MPI_COMM_WORLD, &presence_win);
    MPI_Win_create(&bucket_size, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &bucket_size_win);
    MPI_Barrier(MPI_COMM_WORLD);

  }
public:
  DistributedHashMap(){
    init();
  }



  Value get(Key key) {
    Value val;
    size_t hash= key.hash_code();
    int server=hash%comm_size;
    int bucket;
#ifdef DEBUG
    DBGVAR2(std::cout, server,rank);
    DBGMSG(std::cout,"get");
#endif

      Node<Key,Value> existing;
      int bucket_size;
      get_bucket_size(bucket_size,server);
      bucket=hash%bucket_size;
#ifdef DEBUG
      DBGVAR(std::cout, bucket);
#endif
      size_t count=bucket*allowed_conflicts;
      int presence=0;
      getPresence(presence,server,count);
      getNode(existing, server, count);
    if(presence==0){
      val= Value();
    }else if(!existing.key.equals(key)){
        int c;
        while(presence== 1 &&
              !existing.key.equals(key)){
          count++;
          getPresence(presence,server,count);
          getNode(existing, server, count);
          val=existing.value;
          if(count==(bucket+1)*allowed_conflicts){
            val= Value();
            break;
          }
        }
      }else{
#ifdef DEBUG
      DBGVAR(std::cout, existing);
#endif
        val= existing.value;

    }


    return val;
  }

  void put(Key key, Value value) {
#ifdef DEBUG
      DBGVAR2(std::cout, key , value);
#endif
    size_t hash= key.hash_code();
#ifdef DEBUG
      DBGVAR(std::cout, hash);
#endif
    Node<Key,Value> current;
    current.key=key;
    current.value=value;
    int current_presence=1;
    size_t server=hash%comm_size;
    size_t bucket;
#ifdef DEBUG
      DBGVAR(std::cout, server);
#endif

      int bucket_size=this->bucket_size;
      get_bucket_size(bucket_size,server);

      bucket=hash%bucket_size;
#ifdef DEBUG
      DBGVAR(std::cout, bucket);
#endif
      Node<Key,Value> existing;
      size_t count=bucket*allowed_conflicts;
      int existing_present=0;
      getPresence(existing_present,server,count);
      if(existing_present==1){
        while(existing_present== 1){
          count++;
          getPresence(existing_present,server,count);
        }

        if(count==(bucket+1)*allowed_conflicts){
          /*
           * TODO: trigger re-allocation
           */
        }
      }
      putPresence(current_presence,server,count);
      putNode(current, server, count);

  }
  void kill(){

      MPI_Win_free(&key_bucket_win);
      MPI_Win_free(&value_bucket_win);
      MPI_Win_free(&presence_win);
      MPI_Win_free(&bucket_size_win);
      MPI_Type_free(&key_type);
      MPI_Type_free(&value_type);
    }


};


#endif //MPIONESIDED_DISTRIBUTEDHASHMAP_H
