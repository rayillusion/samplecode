#include "../defines/platform.h"

#include <iostream>
#include <boost/bind/bind.hpp>

#ifdef _RAYLEE_SERVER_
#include "server.hpp"
#endif
#include "session.h"

#include "../system/helper.h"


namespace raylee
{

  void session::connect(int left_count)
  {
    --left_count;

    boost::asio::ip::tcp::resolver resolver(_io_context);
    boost::asio::ip::tcp::resolver::query query(_opponent_ip, _opponent_port);
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    //비동기(Unblock) 상태로 서버와 접속한다.
    boost::asio::async_connect(_socket, endpoint_iterator,
      boost::bind(&session::handle_connect, this, boost::asio::placeholders::error, left_count));
  }

  void session::try_connect(int retry_count)
  {
    connect(retry_count);    
  }

  void session::handle_connect(const boost::system::error_code& e, int left_retry)
  {
    if(!e)
    {
      std::cout << "Connected!" << std::endl;
      on_connect();
    } 
    else 
    {
      if(left_retry > 0)
      {
        connect(left_retry);
      }
      else 
      {
        std::cout << "Fail Connect : " << _opponent_ip << ", port : " << _opponent_port << std::endl;
      }
    }
  }

  void session::read()
  {
    using namespace raylee::packet;

    static bool first = false;
    if(false == first) 
    {
      header_t hedaer;

      hedaer.checksum(0);
      hedaer.length(32);
      hedaer.type(1);

      std::byte * p = reinterpret_cast<std::byte*>(&hedaer);
      printf( "[header size = %d] : ", (int)sizeof(header_t));
      for(int i = 0; i < (int)sizeof(header_t); ++i) 
      {
        printf("%d ", static_cast<int>(p[i]));
      }
      printf( "\n");

      first = true;
    }

    try
    {
      if(read_header == _read_mode) 
      {
        boost::asio::async_read(_socket, boost::asio::buffer(_header_data, header_size),
            //[this, self](boost::system::error_code ec, std::size_t length)
          [this](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            header_t * ph = reinterpret_cast<header_t*>(this->_header_data);
            if(ph->length() < 4) {
              std::string emsg = string_format("packet length %d when msg_type %d\n", ph->length(), ph->type());
              throw std::runtime_error(emsg);
            } 
            else if(ph->length() > max_packet_length)
            {
              std::string emsg = string_format("packet length over max when msg_type %d, length : %d\n", ph->type(), ph->length());
              throw std::runtime_error(emsg);
            }
            #ifdef _DEBUG
            std::cout << "Recv Header : Packet Type = " << ph->type() << ", Packet length = " << ph->length() << std::endl;
            #endif

            this->_packet_size = ph->length();
            this->_packet_type = static_cast<MSGTYPE>(ph->type());
            this->_read_mode = read_packet;


            // if(ph->checksum != TEMP_CHECKSUM) {
            //   std::string emsg = string_format("packet checksum fail when msg_type %d\n", ph->msg_type);
            //   throw std::runtime_error(emsg);
            // }
            // else {
              read();
            // }
          } 
          else
          {
            this->handle_disconnect(ec, __FILE__, __LINE__);
          }
        } );

      } 
      else if(read_packet == _read_mode)
      {
        boost::asio::async_read(_socket, boost::asio::buffer(_packet_buffer, _packet_size),
            //[this, self](boost::system::error_code ec, std::size_t length)
          [this](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
            {
             
              on_packet(_packet_type, _packet_buffer, _packet_size);

              this->_read_mode = read_header;

              read();
            }
            else
            {
              std::cout << "[Exception] " << ec.what() << "at " << __FILE__ << ":" << __LINE__ << std::endl;
              this->handle_disconnect(ec, __FILE__, __LINE__);
            }
        } );
      }
    }
    catch(const std::exception& e)
    {
      std::cout << e.what() << '\n';
      handle_disconnect(__FILE__, __LINE__);
    }
  }

  void session::write(flatbuffers::FlatBufferBuilder& builder, raylee::packet::MSGTYPE msg_type)
  {
    auto pdb = _detach_q.get();
    *pdb = builder.Release();

    // std::cerr << "[client::write] builder size = " << pdb->size() << std::endl;

    auto ph = _header_q.get();
    ph->length(static_cast<uint16_t>(pdb->size() ) );
    ph->type( msg_type ); 
    // ph->checksum = TEMP_CHECKSUM;

    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(ph, sizeof(raylee::packet::header_t)));
    buffers.push_back(boost::asio::buffer(pdb->data(), pdb->size()));

    std::cerr << "[client::write] send size = " << sizeof(raylee::packet::header_t) + ph->length() << std::endl;

    boost::asio::async_write(_socket, buffers,
      boost::bind(&session::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, ph, pdb) ); 

  }

  void session::handle_write(const boost::system::error_code& e, std::size_t bytes_transferred, raylee::packet::header_t * ph, flatbuffers::DetachedBuffer * pdb)
  {
    if(pdb) _detach_q.release(pdb);
    if(ph) _header_q.release(ph);

    if(e)
    {
      handle_disconnect(e, __FILE__, __LINE__);
    }
  }
  
  void session::handle_disconnect(const boost::system::error_code& ec, const char * file, int line)
  {
    std::cout << "[disconnect] erro = " << ec.what() << "at " << file << ":" << line << std::endl;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, const_cast<boost::system::error_code&>(ec));
    _socket.close();

    #ifdef _RAYLEE_SERVER_
    if(!_freed)
    {
      server<Player>::GetInstance()->free_object(static_cast<Player*>(this));
      _freed = true;
    }
    #endif
  }

  void session::handle_disconnect(const char * file, int line)
  {
    std::cout << "[disconnect] at " << file << ":" << line << std::endl;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    _socket.close(); 

    #ifdef _RAYLEE_SERVER_
    if(!_freed)
    {
      server<Player>::GetInstance()->free_object(static_cast<Player*>(this));
      _freed = true;
    }
    #endif
  }


  void session::on_accept()
  {
    // server 용

  }

  void session::on_connect()
  {
    read();
  }

  void session::on_disconnect()
  {
    // 
  }

  void session::on_packet( raylee::packet::MSGTYPE mt, uint8_t * buffer, size_t len)
  {
    std::cout << "[on_packet] type = " << (int)mt << ", length = " << len << std::endl;
  }  

 
} // namespace raylee

