#ifndef RAYLEE_SYSTEM_MEMPOO_H
#define RAYLEE_SYSTEM_MEMPOO_H

#include <queue>
#include <list>
#include <vector>
#include <mutex>
#ifdef USE_FIBER_MUTEX
#include <boost/fiber/all.hpp>
#endif
#include <condition_variable>
#include <unordered_map>

#include "flatbuffers/default_allocator.h"



// #define RAYLEE_DEBUG_POOL

#ifdef USE_FIBER_MUTEX
  typedef boost::fibers::mutex mutex_t;
#else
  typedef std::mutex mutex_t;
#endif

#include <boost/serialization/singleton.hpp>


#ifndef mlock
#define mlock(x) std::unique_lock<mutex_t> aGuard(x)
#endif

namespace raylee

{
  // chunk size = 16, 
  
  using namespace std;
  
  class Block
  {

    struct _Bucket
    {
      Block * block{nullptr};
      uint32_t size{0};  // 0 : in pool, not 0 : out ( size ) 
      uint32_t padding{0};  // 
    };
    
    enum { min_bucket_count = 32, max_bucket_count = 1024, max_block_size = 128 * 1024, header_size = sizeof(_Bucket) };
    enum { padding_check = 0x7fffffff };

  public:
    Block(uint32_t bucket_size);
    ~Block();

  private:

    friend class FixedSizeBlocks;
    friend class MemoryPool;

    void * alloc();
    void dealloc(_Bucket * bucket);

    bool empty()
    {
      return (_cur_free == _end && 0 == _free_buckets.size() );
    }

    bool is_this(void * p)
    {
      return (p >= (void*)_buffer && p <= _end);
    }
  
  public:
    static _Bucket * get_bucket_info(void * p)
    {
      void * header_pos = (char*)p - header_size;
      _Bucket * bucket_info = static_cast<_Bucket*>(header_pos);
      return bucket_info; // ->block;
    }
    static uint32_t get_size(void * p)
    {
      void * header_pos = (char*)p - header_size;
      _Bucket * bucket_info = static_cast<_Bucket*>(header_pos);
      return bucket_info->size;
    }

    void print();

  private:
    queue<void *> _free_buckets;
    uint32_t _bucket_size;

    void * _end { nullptr };
    void * _cur_free { nullptr };

    char * _buffer { nullptr };
  };

  class FixedSizeBlocks
  {
    friend class MemoryPool;

  public:
    void print();
    void release();

  private:
    FixedSizeBlocks(uint32_t size); 
    ~FixedSizeBlocks();

    void * alloc();
    void dealloc(Block::_Bucket * bucket);

    void insert_block(Block * p);

    Block * get_allocate(void * p);
    Block * create_block();

    


  private:
    uint32_t _chunk_size;
#ifdef USE_FIBER_MUTEX
    boost::fibers::condition_variable_any _block_making_cv;
#else
    condition_variable _block_making_cv;
#endif
    mutex_t _cv_m;
    int _block_making_flag = 0;

    mutex_t _block_mutex;
    list<Block*> _blocks;
  };


  class MemoryPool : public flatbuffers::Allocator, 
                    public boost::serialization::singleton<MemoryPool>
  {
    friend class boost::serialization::singleton<MemoryPool>;

    // 16, 32, 64, 128, 256   ~ 8192 까지 128 byte 씩 
    enum { min_size = 16, max_size = 8192, basic_size = 128 };
  public:
    MemoryPool() {
      create();
    }
    ~MemoryPool() {
      // release();
      printf("[MemoryPool] ~MemoryPool\n");
    }

    void release();

    static MemoryPool& get() {

      return boost::serialization::singleton<MemoryPool>::get_mutable_instance();
    }


  private:
    void create();
    

    uint32_t get_bucket_size(uint32_t need_size);

  public:
    uint8_t * allocate(size_t size) override;
    void deallocate(uint8_t * p, size_t) override;

    void print(uint32_t size);

  private:
    unordered_map<uint32_t, FixedSizeBlocks*> _blocks_map;
  };

  struct release_helper
  {
    release_helper(uint8_t * p) : _p(p) {}
    ~release_helper() {
      if(_p)
        MemoryPool::get().deallocate(_p, 0);
    }
    uint8_t * _p;
  };
} // end of raylee

#endif