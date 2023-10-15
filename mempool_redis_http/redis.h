#ifndef RAYLEE_SYSTEM_REDIS_H
#define RAYLEE_SYSTEM_REDIS_H


#include <string_view>
#include <queue>

#include <boost/asio.hpp>
#include <mutex>
#include <future>
#include <atomic>
#include <thread>
#include <boost/thread.hpp>
// #include <boost/fiber/all.hpp>
#include <boost/redis.hpp>
#include <chrono>

using namespace std::chrono_literals;

#include "singleton.h"

/* not use : use asyncredis.h/cpp


namespace raylee
{
  using boost::redis::connection;
  using boost::redis::request;
  using boost::redis::response;
  using boost::redis::config;
  namespace net = boost::asio;

  class Redis : public Singleton<Redis>
  {
  public: 
    Redis();
    ~Redis() {}

    void set();
    void start(int pool_count);
    void stop();

    // template<class ResultT, class... Ts>
    // bool read(request& req, ResultT &out)
    // {
    //   response<TS...> resp;

    // }

    template <class Response>
    bool read(request& req, Response& resp)
    {
      try
      {
        std::chrono::time_point<std::chrono::steady_clock> cur = std::chrono::steady_clock::now();

        auto conn = std::make_shared<connection>(_io_context);
      
        conn->async_run(_config, {}, net::consign(net::detached, conn));

        if(conn && conn->will_reconnect())
        {
          //std::cout << "[Redis] async_exec [" << time(nullptr) << "]\n"; 

          std::promise<bool> promise;
          auto fut = promise.get_future();

          conn->async_exec(req, resp, [&](auto ec, auto) {
            if(!ec) {
              std::cout << "[Redis] Read Success exec\n";
              promise.set_value(true);
            } 
            else{
              std::cout << "[Redis] Read Fail exec : " << ec << " [" << time(nullptr) << "]\n";
              promise.set_value(false);
            }
          });

          if(std::future_status::ready == fut.wait_for(2s))
          {
            std::chrono::time_point<std::chrono::steady_clock> end_tp = std::chrono::steady_clock::now();
            auto el_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_tp - cur);
            std::cout << "  - 1 Elapse " << el_ms.count() << " ms\n";

            std::cout << "[Redis] Read return [" << time(nullptr) << "]\n";

            conn->cancel();
            return fut.get();
          }
          else
          {
            std::chrono::time_point<std::chrono::steady_clock> end_tp = std::chrono::steady_clock::now();
            auto el_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_tp - cur);
            std::cout << "  - 2 Elapse " << el_ms.count() << " ms\n";

            std::cout << "[Redis] time out [" << time(nullptr) << "]\n";
            conn->cancel();
            return false;
          }
          
        }
        else
        {
          std::cout << "[REDIS] Fail Connect\n";
        }
        conn->cancel();

        std::chrono::time_point<std::chrono::steady_clock> end_tp = std::chrono::steady_clock::now();
        auto el_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_tp - cur);
        std::cout << "  - 3 Elapse " << el_ms.count() << " ms\n";
        return false;
      }
      catch(const std::exception& e)
      {
        std::cout << e.what() << '\n';
      }

      return false;
    }

    // bool save(request& req);

    void test();

  private:
    std::shared_ptr<connection> get_free_con();
    void free_connection(std::shared_ptr<connection> conn);

  private:
    boost::asio::io_context _io_context{4};
    config _config;
    // std::queue<std::shared_ptr<connection>> _connection_q;
    std::shared_ptr<connection> _conn{nullptr};
    std::mutex _mutex;
    bool _continue{true};
    boost::thread_group _thread_group;

  };

}
*/
#endif