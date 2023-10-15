#ifndef RAYLEE_SERVER_SESSION_H
#define RAYLEE_SERVER_SESSION_H

#include <mutex>
#include <boost/asio.hpp>
#include <boost/lockfree/queue.hpp>

#include "flatbuffers/flatbuffers.h"


#include "../defines/constants.h"

#include "../packet/allpacket.h"

namespace raylee
{
  template<typename T>
  class SimpleQ
  {
  public:
    SimpleQ(size_t initial_size)
      : _queue(initial_size)
    {
    }

    ~SimpleQ()
    {
      T * p;

      while( _queue.pop(p) )
      {
        delete p;
      }
    }

    T * get()
    {
      T * p = nullptr;

      if(false == _queue.pop(p)) 
      {
        p = new T();
      }
      return p;
    }

    void release(T* p)
    {
      _queue.push(p);
    }

  private:
    boost::lockfree::queue<T *> _queue;
  };

  class session
  {
  public:

  #ifdef _RAYLEE_SERVER_
    session(boost::asio::ip::tcp::socket& socket, boost::asio::io_context& io_context)
      : _io_context(io_context), _freed(false), _socket(std::move(socket))
    {

    }
  #else   // for test client

    session(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
      : _opponent_ip(host), _opponent_port(port), _io_context(io_context), _socket(io_context)
    {
    }
  #endif
    virtual ~session()
    {
    }


  private:
    void connect(int left_count);

  public: // network
    void try_connect(int retry_count);
    void handle_connect(const boost::system::error_code& e, int left_retry );

    void read();

    void write(flatbuffers::FlatBufferBuilder& builder, raylee::packet::MSGTYPE msg_type);
    void handle_write(const boost::system::error_code& e, std::size_t bytes_transferred, raylee::packet::header_t * ph, flatbuffers::DetachedBuffer * db);
    
    void handle_disconnect(const boost::system::error_code& ec, const char * file, int line);
    void handle_disconnect(const char * file, int line);

  public: // override
    
    virtual void on_accept();
    virtual void on_connect();
    virtual void on_disconnect();
    virtual void on_packet( raylee::packet::MSGTYPE mt, uint8_t *, size_t );

  protected:
    SimpleQ<raylee::packet::header_t> _header_q{8};
    SimpleQ<flatbuffers::DetachedBuffer> _detach_q{8};

    raylee::packet::MSGTYPE get_packet_type() { return _packet_type; }

  private:

    std::string _opponent_ip;
    std::string _opponent_port;
    boost::asio::io_context& _io_context;
  #ifdef _RAYLEE_SERVER_
    bool _freed;
  #endif
    
    // socket 
    int _retry_connect_count {1};
    boost::asio::ip::tcp::socket _socket;
    enum { header_size = raylee::packet::Constants::Constants_header_size };
    char _header_data[header_size];

    // read manipulator
    enum { read_header = 0, read_packet = 1, max_packet_length = RAYLEE_SOCKET_BUFFER_SIZE };
    int _read_mode {read_header};
    raylee::packet::MSGTYPE _packet_type;

    size_t _packet_size{ 0 };
    uint8_t _packet_buffer[RAYLEE_SOCKET_BUFFER_SIZE];
  };


} // namespace raylee 

#endif