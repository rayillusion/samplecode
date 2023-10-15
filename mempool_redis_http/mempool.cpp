#include "../defines/platform.h"

#include <iostream>
#include <string_view>
#include <string>

#include "helper.h"

#include "mempool.h"

namespace raylee
{

  Block::Block(uint32_t bucket_size)
  {
    _bucket_size = bucket_size + header_size;

    uint32_t chunk_count = max_block_size / _bucket_size; // 정수배로.

    if(chunk_count < min_bucket_count) chunk_count = min_bucket_count;
    else if(chunk_count > max_bucket_count) chunk_count = max_bucket_count;

    uint32_t need_size = chunk_count * _bucket_size;

    _buffer = new char[need_size];

    char * p = _buffer;
    _Bucket * b = nullptr;
    for(uint32_t i = 0; i < chunk_count; ++i)
    {
      b = static_cast<_Bucket*>((void*)p);
      b->block = this;
      b->size = 0;
      b->padding = padding_check;

      p += _bucket_size;
    }

    _cur_free = _buffer;
    _end = _buffer + need_size - _bucket_size;
  }


  Block::~Block()
  {
    if(_buffer) 
      delete [] _buffer;
    _buffer = nullptr;
  }

  void * Block::alloc()
  {
    void * p = nullptr;

    if(empty())
    {
      throw std::runtime_error( "Fail: Block alloc");
    }

    if(_free_buckets.empty())
    {
      #ifdef RAYLEE_DEBUG_POOL
      // cout << "[Block::allo] _free_buckets Empty\n";
      #endif

      p = _cur_free;
      _cur_free = (char*)p + _bucket_size;  
    }
    else 
    {
      p = _free_buckets.front();
      _free_buckets.pop();
    }

    _Bucket * binfo = static_cast<_Bucket*>(p);
    binfo->size = _bucket_size;

    #ifdef RAYLEE_DEBUG_POOL
    std::cerr << "[Block::alloc] bucket = " << binfo << ", ret = " << (char*)p + header_size << std::endl;
    #endif

    return (char*)p + header_size;
  }


  void Block::dealloc(Block::_Bucket * bucket)
  {
    //void * header_pos = (char*)p - header_size;
    // _Bucket * bucket = static_cast<_Bucket*>(header_pos);
    #ifdef RAYLEE_DEBUG_POOL
    std::cerr << "[Block::dealloc] bucket(size = " << _bucket_size << ") ptr = " << bucket << std::endl;
    // cout << "[Block::dealloc] bucket " << ((nullptr == bucket) ? "null" : "found") << endl;
    #endif

    if(0 != bucket->size) 
    {
      bucket->size = 0;
      _free_buckets.push(bucket);

      #ifdef RAYLEE_DEBUG_POOL
      if(bucket->padding != padding_check)
      {
        throw runtime_error("[Pool] padding modified!!");
      }
      #endif
    }
//     #ifdef RAYLEE_DEBUG_POOL
    else
    {
      cout << "[Block::dealloc] Duplicated size = " << _bucket_size << endl;
      // throw runtime_error("[Pool] duplicated deallocation");
    }
//    #endif
  }

  void Block::print()
  {
    cout << "  - block : " << _free_buckets.size() << ", ended = " << ((_end == _cur_free) ? "true" : "false") << endl;
  }


  FixedSizeBlocks::FixedSizeBlocks(uint32_t size)
  {
    Block * block = new Block(size);

    insert_block(block);

    _chunk_size = size;
  }

  FixedSizeBlocks::~FixedSizeBlocks()
  {
    // mlock(_block_mutex);

    // auto itr = _blocks.begin();
    // while(itr != _blocks.end())
    // {
    //   delete *itr;
    //   ++itr;
    // }
  }

  void FixedSizeBlocks::release()
  {
    mlock(_block_mutex);

    auto itr = _blocks.begin();
    while(itr != _blocks.end())
    {
      delete *itr;
      ++itr;
    }
  }

  void * FixedSizeBlocks::alloc()
  {
    void * p = nullptr;
    Block * free_block = nullptr;

    {
      mlock(_block_mutex);
      size_t block_count = _blocks.size();
      size_t try_count = min((size_t)2, block_count);

      while( try_count > 0 )
      {
        auto first = _blocks.begin();

        if( (*first)->empty() )
        {
          if(1 < block_count)
          {
            _blocks.push_back(*first);
            _blocks.pop_front();
          }

          --try_count;
        } 
        else
        {
          free_block = *first;
          break;
        }
      }
    } // lock(_block_mutex)

    if(nullptr == free_block)
    {
      free_block = create_block();
    }
    
    p = free_block->alloc();
    
    return p;
  }


  Block * FixedSizeBlocks::create_block()
  {
    Block * block = nullptr;
    mlock(_cv_m);

    if(0 == _block_making_flag) 
    {
      _block_making_flag = 1;

      block = new Block(_chunk_size);

      insert_block(block);

      _block_making_flag = 0;

      _block_making_cv.notify_all();
    } 
    else 
    {
      // 블록 생성 기다림.
      _block_making_cv.wait(aGuard, [&] { return 0 == _block_making_flag; });

      // 생성 후라, _blocks 가 

      mlock(_block_mutex);
      block = _blocks.front();
    }

    return block;
  }

  void FixedSizeBlocks::insert_block(Block * p)
  {
    mlock(_block_mutex);

    _blocks.push_front(p);

  }
  
  void FixedSizeBlocks::dealloc(Block::_Bucket * bucket)
  {
    // Block::_Bucket * bucket = Block::get_bucket_info(p);
    bucket->block->dealloc(bucket);
  }


  void FixedSizeBlocks::print()
  {
    cout << "[FixedBlocks " << _chunk_size << "] Block count = " << _blocks.size() << endl;

    auto itr = _blocks.begin();
    while(itr != _blocks.end())
    {
      (*itr)->print();
      ++itr;
    }
  }


  void MemoryPool::create()
  {
    array<uint32_t, 3> small{ 16, 32, 64 };

    for(int i = 0; i < 3; ++i)
    {
      FixedSizeBlocks * sb = new FixedSizeBlocks( small[i] );
      _blocks_map[small[i]] = sb;
    }

    for(int i = 1; i <= max_size/basic_size; ++i)
    {
      uint32_t s = i * basic_size;
      FixedSizeBlocks * sb = new FixedSizeBlocks( s );
      _blocks_map[s] = sb;
    } 
  }

  void MemoryPool::release()
  {
    cout << "[MemoryPool] release()\n";

    auto itr = _blocks_map.begin();
    while( itr != _blocks_map.end())
    {
      itr->second->release();
      
      delete (itr->second);
      ++itr;
    }
    _blocks_map.clear();
  }

  uint32_t MemoryPool::get_bucket_size(uint32_t size)
  {
    uint32_t chunk_size = 0;

    uint32_t div = size / basic_size;

    switch(div)
    {
    case 0:
      if(size > 64) chunk_size = 128;
      else if(size > 32) chunk_size = 64;
      else if(size > 16) chunk_size = 32;
      else chunk_size = 16;
      break;

    default:
      chunk_size = ( (size - 1) / basic_size + 1) * basic_size;
    }
    
    return chunk_size;

  }

  uint8_t * MemoryPool::allocate(size_t size)
  {
    uint32_t chunk_size = get_bucket_size(static_cast<uint32_t>(size));
    #ifdef RAYLEE_DEBUG_POOL
    cerr << "[MemoryPool::alloc] size = " << size << ", chunk size = " << chunk_size << endl;
    #endif

    auto itr = _blocks_map.find(chunk_size);
    if(itr != _blocks_map.end())  
    {
      return static_cast<uint8_t*>(itr->second->alloc());
    }
    else
    {
      string msg = string_format("Not found Blocks for size = %u(req size = %u)", chunk_size, size);
      throw runtime_error(msg);
    }

    return nullptr;
  }

  void MemoryPool::deallocate(uint8_t * p, size_t)
  {
    Block::_Bucket * bucket = Block::get_bucket_info(p);

    uint32_t chunk_size = bucket->size - Block::header_size;
    #ifdef RAYLEE_DEBUG_POOL
    cerr << "[MemoryPool::dealloc] - chunk size = " << chunk_size << " ptr = " << p << endl;
    #endif

    auto itr = _blocks_map.find(chunk_size);
    if(itr != _blocks_map.end())
    {
      itr->second->dealloc(bucket);
    }
    else 
    {
      cout << "[Error] Not found chunk size " << chunk_size << endl;
    }
  }

  void MemoryPool::print(uint32_t size)
  {
    uint32_t real = get_bucket_size(size);
    cout << "Memory Pool for size " << size <<", real = " << real << endl;

    auto itr = _blocks_map.find(real);
    if(itr != _blocks_map.end()) 
    {
      itr->second->print();
    }
    else
    {
      cout << "No found for real size " << real << endl;
    }

    // auto itr = _blocks_map.begin();
    // while(itr != _blocks_map.end())
    // {
    //   itr->second->print();
    //   ++itr;
    // }
  }
}


// QueueMemory::QueueMemory(uint32_t need)
//   : _size(need)
// {
// }

// QueueMemory::~QueueMemory()
// {
//   mlock(_lock);

//   auto itr = _free.begin();
//   while(itr != _free.end()) 
//   {
//     delete *itr;
//     ++itr;
//   }
// }

// void * QueueMemory::alloc()
// {
//   char * p = nullptr;
//   mlock(_lock);

//   if(_free.empty()) 
//   {
//     p = new char[_size + sizeof(uint32_t)];
    
//   }
//   else 
//   {
//     p = (char*) _free.front();
//     _free.pop_front();
//   }

//   *((uint32_t*)p) = _size;
//   p += sizeof(uint32_t);
//   return p;
// }

// void QueueMemory::dealloc(void * p)
// {
//   mlock(_lock);

//   _free.push_back((char*)p);
// }




// MemoryQueue::MemoryQueue()
// {

// }

// MemoryQueue::~MemoryQueue()
// {

// }

// QueueMemory * MemoryQueue::find_queue(uint32_t real)
// {
//   mlock(_lock);

//   QueueMemory * q = nullptr;

//   auto itr = _queue_map.find(real);
//   if(_queue_map.end() == itr)
//   {
//     q = new QueueMemory(real);
//     _queue_map.insert( make_pair(real, q));
//   }
//   else 
//   {
//     q = itr->second;
//   }

//   return q;
// }

// void * MemoryQueue::alloc(uint32_t size)
// {
//   uint32_t real = bucket_size(size);
//   QueueMemory * q = find_queue(real);
//   if(q)
//   {
//     return q->alloc();
//   }
//   else 
//   {
//     throw runtime_error("[Fail] Queue Allocate");
//   }
// }

// void MemoryQueue::dealloc(void * p)
// {
//   uint32_t * size_ptr = reinterpret_cast<uint32_t*>((char*)p - sizeof(uint32_t));

//   QueueMemory * q = find_queue(*size_ptr);
//   if(q)
//   {
//     q->dealloc((char*)p - sizeof(uint32_t));
//   }
//   else 
//   {
//     cout << "[Fail] MemoryQueue::dealloc : size = " << *size_ptr << endl;
//     throw runtime_error("[Fail] MemoryQueue::dealloc");
//   }
// }

// uint32_t MemoryQueue::bucket_size(uint32_t need)
// {
//   uint32_t real = 16; 

//   if(need < 128)
//   {
//     real = ((need / 16) + 1) * 16;
//   }
//   else if(need < 1024)
//   {
//     real = ((need / 32) + 1) * 32;
//   }
//   else if(need < 4096)
//   {
//     real = ((need / 128) + 1) * 128;
//   }
//   else 
//   {
//     real = ((need / 256) + 1) * 256;
//   }
//   return real;
// }