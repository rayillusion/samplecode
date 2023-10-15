#include "../defines/platform.h"
#include "../defines/constants.h"

#include <iostream>
#include <coroutine>
#include <future>

#include "config.h"
#include "redis.h"
#include <boost/redis/src.hpp>

/*
not use : use asyncredis.h/cpp

namespace raylee
{
  using boost::redis::connection;
  using boost::redis::request;
  using boost::redis::response;
  using boost::redis::config;
  namespace net = boost::asio;

  Redis::Redis()
  {
  }

  void Redis::set()
  {
    auto info = Config::GetInstance()->get_info(server_type::redis);
    if(info)
    {
      _config.addr.host = info->ip;
      _config.addr.port = info->port_str;
      // 레디스 이상일 경우, 얼마동안 기다릴 수 있느가? 넘어가면, 
      // exception 이 나며, 다운된다.
      _config.connect_timeout = std::chrono::seconds(2);
      _config.health_check_interval = std::chrono::seconds::zero(); // std::chrono::seconds(5);
      _config.clientname = "EidosBattleServer";
      _config.resolve_timeout = std::chrono::seconds{3};
    }
    else
    {
      throw new runtime_error("Config : redis server info not");
    }
  }

  void Redis::start(int pool_count)
  {
    std::cout << "[Redis] Create " << pool_count << " Connections\n";

    // for(int i = 0; i < pool_count; ++i)
    // {
    //   auto conn = std::make_shared<connection>(_io_context);
    //   conn->async_run(_config, {}, net::consign(net::detached, conn));

    //   _mutex.lock();
    //   _connection_q.push(conn);
    //   _mutex.unlock();
    // }
    _conn = std::make_shared<connection>(_io_context);
    _conn->async_run(_config, {}, net::consign(net::detached, _conn));

    for(int i = 0; i < pool_count; ++i) 
    {
      _thread_group.create_thread( [&]{ 
        while(_continue)
        {
          try 
          {
            _io_context.run();
          }
          catch(std::exception& ex)
          {
            std::cerr << "[Exception] io_context run : " << ex.what() << std::endl;
          }
        }
        std::cout << "io_worker stopped\n"; 
      } );
    }
  }

  void Redis::stop()
  {
    _continue = false;
    // _mutex.lock();
    // while(!_connection_q.empty())
    // {
    //   try
    //   {
    //     auto con = _connection_q.front();
    //     _connection_q.pop();
    //   }
    //   catch(const std::exception& e)
    //   {
    //     std::cerr << e.what() << '\n';
    //   }
    // }
    // _mutex.unlock();

    _io_context.stop();

    _thread_group.join_all();
  }

  std::shared_ptr<connection> Redis::get_free_con()
  {
    std::shared_ptr<connection> ptr{};

    // _mutex.lock();
    // if(!_connection_q.empty())
    // {
    //   ptr = _connection_q.front();
    //   _connection_q.pop();
    // }
    // _mutex.unlock();

    return ptr;
  }

  void Redis::free_connection(std::shared_ptr<connection> conn)
  {
    _mutex.lock();

    // _connection_q.push(conn);

    _mutex.unlock();
  }

  // bool Redis::save(request& req)
  // {
  //   auto conn = get_free_con();
  //   if(conn) 
  //   {
  //     std::cout << "[Redis] Request exec\n"; 
  //     conn->async_run(_config, {}, net::consign(net::detached, conn));

  //     auto promise = std::promise<bool>();
  //     auto fut = promise.get_future();

  //     conn->async_exec(req, boost::redis::ignore, [&](const boost::system::error_code& ec, auto) {
  //       if(!ec) {
  //         std::cout << "[Redis] Save Success exec\n";
  //         promise.set_value(true);
  //       } 
  //       else{
  //         std::cout << "[Redis] Save Fail exec\n";

  //         promise.set_value(false);

  //         // if(_continue)
  //         //   start(1);
  //       }
  //     });

  //     free_connection(conn);

  //     return fut.get();
  //   }  
  //   else 
  //   {
  //     std::cerr << "[Redis] No  free Connections!\n";
  //   }

  //   return false;
    
  // }

  void Redis::test()
  {
    std::cout << "[Redis] Try Lock";
    if(_mutex.try_lock())
    {
      _mutex.unlock();
      std::cout << " => Success (& unlock)\n";
    }
    else
    {
      std::cout << " => Lock Fail\n";
    }

    // GET 테스트
    // std::string key {"user:3NWanmexnES0Ly1x34eQoaMwtHi1-"};
    // request req;
    // req.push("GET", key);

    // response<std::string> resp;

    // std::cout << "[Redis] Read Test\n";
    // if(read(req, resp))
    // {
    //   //std::cout << " - Success\n";
    //   try {
    //     std::cout << "[Redis] Get '" << key << std::endl << "String = " << std::get<0>(resp).value().c_str() << std::endl;     
    //   }
    //   catch(std::exception& ex)
    //   {
    //     std::cout << " Test Exception : " << ex.what() << std::endl;
    //   }
    // }
    // else
    // {
    //   std::cerr << "[Redis] Fail : Read\n";
    // }

    // std::cout <<"[Redis] Test End\n";

    // HGET
    std::string key {"1bs6-test"};
    std::string field {"adventure:3d885cce-9028-43f8-a7e3-8768814c76de"};
    string skey { "skey:3NWanmexnES0Ly1x34eQoaMwtHi1-" };

    request req;
    req.push("GET", skey);
    req.push("HGET", key, field);

    response<std::string, std::string> resp;

    std::cout << "[Redis] Read Test\n";
    if(read(req, resp))
    {
      //std::cout << " - Success\n";
      try {
        if(std::get<0>(resp).has_value())
          std::cout << " => Get '" << key << std::endl << "String = " << std::get<0>(resp).value().c_str() << std::endl;  
        else
          std::cout << " => Don't have Get(skey)\n";

        if(std::get<1>(resp).has_value())
          std::cout << " Get '" << key << "field : " << field << " : value = " << std::get<1>(resp).value().c_str() << std::endl; 
        else
          std::cout << " => Don't have HGET(key)\n";
      }
      catch(std::exception& ex)
      {
        std::cout << " Test Exception : " << ex.what() << std::endl;
      }
    }
    else
    {
      std::cerr << "[Redis] Fail : Read\n";
    }

    std::cout <<"[Redis] Test End\n";
  }
}

*/