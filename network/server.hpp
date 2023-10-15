#ifndef RAYLEE_SERVER_SERVER_H
#define RAYLEE_SERVER_SERVER_H

#include "../defines/platform.h"
#include "../defines/constants.h"

#include <thread>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind/bind.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/fiber/detail/thread_barrier.hpp>
#include <string>


#include "../system/config.h"
#include "../system/singleton.h"
#include "../contents/object/player.h"

#ifndef __in
#define __in 
#endif


// extern raylee::Pool<raylee::Player> gPlayerPool;

namespace raylee
{
  class RoomManager;

  template <typename session_t>
  class server : public Singleton< server <session_t > >
  {
  public:
   
    server()
      // : _acceptor(_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), RAYLEE_BS_ACCEPT_PORT))
      : _acceptor(_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string("0.0.0.0"), RAYLEE_BS_ACCEPT_PORT))
    { 
        // do_accept();
    }

    void start(int n_worker)
    {
      // std::thread acc_thread(do_accept);
      // do_accept();
      // do_accept 가 끝난 후에, _io_context.run() 이 호출되어야 한다.

      boost::fibers::detail::thread_barrier b_accept{2};

      _thread_group.create_thread( 
        [&] {
          this->do_accept();  
          b_accept.wait();
        }
      );

      b_accept.wait();

      std::cout << "start accept threads\n";

      for(int i = 0; i < n_worker; ++i) 
      {
        _thread_group.create_thread( [&]{ 
          try 
          {
            _io_context.run();
          }
          catch(std::exception& ex)
          {
            std::cerr << "[Exception] io_context run : " << ex.what() << std::endl;
          }
          std::cout << "io_worker stopped\n"; 
        } );
      }

      get_my_ip();

    }

    void join()
    {
      _thread_group.join_all();

    }

    void get_my_ip()
    {
      bool find_local = false;
      boost::asio::ip::tcp::resolver resolver(_io_context);
      boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(),"");
      boost::asio::ip::tcp::resolver::iterator it=resolver.resolve(query);

      while(it!=boost::asio::ip::tcp::resolver::iterator())
      {
          boost::asio::ip::address addr=(it++)->endpoint().address();

          string ip = addr.to_string();
          if(false == ip.starts_with("127") && false == ip.starts_with("0"))
          {
            find_local = true;;
            _ip = ip;
          }

          std::cout<< "Local IP: " << addr.to_string()<<std::endl;
      }

      if(false == find_local)
      {
        _ip = "127.0.0.1";
      }
    }

    void stop()
    {
      std::cout << "Server Request Stop\n";
      _acceptor.cancel();

      _io_context.stop();

      join();

      std::cout << "Server Stop!\n";
    }

    boost::asio::io_context& get_io() { return _io_context; }
    boost::asio::io_context* get_io_ptr() { return &_io_context; }

    void free_object( Player * player ) 
    {
      _player_pool.destroy(player);
    }

  private:
    void do_accept()
    {
      auto tid = std::this_thread::get_id();

      std::cout << "TID : " << tid << ", do_accept() " << std::endl;

      _acceptor.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
        {
          try
          {
            if (!ec)
            {
              auto tid2 = std::this_thread::get_id();
              std::cout << "TID : " << tid2 << ", Client Accept " << std::endl;

              auto player = _player_pool.construct(_io_context, socket);
              // start() is a function of raylee::EidosSession 
              player->on_accept();

              // std::make_shared<session_t>(std::move(socket))->start();
            } else {
              auto tid2 = std::this_thread::get_id();
              std::cout << "TID : " << tid2 << ", Client Accept with ec = " << ec << std::endl;
            }
          }
          catch(std::exception& e)
          {
            std::cout << "[Exception] " << e.what() << "at " << __FILE__ << ":" << __LINE__ << endl;
          }
          catch(...)
          {
            std::cout << "[Exception] " << "at " << __FILE__ << ":" << __LINE__ << endl;
          }

          do_accept();
        }
      );
    }


    boost::asio::io_context _io_context;
    boost::asio::ip::tcp::acceptor _acceptor;
    boost::thread_group _thread_group;

    boost::object_pool<session_t> _player_pool;  

    enum State { Ready = 0, Connected = 1 };
    State _state { State::Ready };
    std::string _ip;

  };

} // namespace raylee::server

#undef __in

#endif