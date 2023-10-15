#ifndef RAYLEE_SYSTEM_HTTP_REQUEST_H
#define RAYLEE_SYSTEM_HTTP_REQUEST_H

#include "../defines/platform.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/fiber/all.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include <boost/json.hpp>

#include "config.h"
#include "singleton.h"


namespace raylee 
{
  namespace beast = boost::beast;         // from <boost/beast.hpp>
  namespace http = beast::http;           // from <boost/beast/http.hpp>
  namespace net = boost::asio;            // from <boost/asio.hpp>
  namespace lockfree = boost::lockfree;  // from <boost/lockfree/queue.hpp>
  using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
  using namespace std;


  template<server_type ST>
  class HttpPool;

  // T = raylee::server_type (in config.h)
  template<server_type ST>
  class HttpRequest
  {
  public:
    explicit
    HttpRequest(net::io_context& ioc, string& host, string& port)
        : _host(host) 
        , _port(port)
        , _resolver(net::make_strand(ioc))
        , _stream(net::make_strand(ioc))
    {
      
    }

    boost::fibers::future<bool> get(char const* target)
    {
      _promise = boost::fibers::promise<bool>();
      _req = {};

      // Set up an HTTP GET request message
      _req.method(http::verb::get);
      _req.target(target);
      _req.version(RAYLEE_HTTPREQUEST_VERSION);
      
      _req.set(http::field::host, _host.c_str());
      _req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      

      // cout << "[HttpRequest] Get : Host = " << _host << ", Port = " << _port << endl;

      // Look up the domain name
      _resolver.async_resolve(
        _host.c_str(),
        _port.c_str(),
        beast::bind_front_handler(
          &HttpRequest<ST>::on_resolve,
          this ));

      return _promise.get_future();
    }

    boost::fibers::future<bool> post(
      char const* target,
      char const* body = nullptr)
    {
      _promise = boost::fibers::promise<bool>();
      _req = {};

      // Set up an HTTP GET request message
      _req.method(http::verb::post);
      _req.target(target);

      _req.version(RAYLEE_HTTPREQUEST_VERSION);
      _req.set(http::field::host, _host.c_str());
      _req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      _req.set(http::field::content_type, "application/json" );
      if(body)
      {
        _req.body() = body;
      }
      else
      {
        _req.body() = "";
      }
      _req.prepare_payload();   // body() 셋팅 후에 호출해야 한다. 반드시.

      // cout << "[HttpRequest] Post  : Host = " << _host << ", Port = " << _port << endl;

      // Look up the domain name
      _resolver.async_resolve(
        _host.c_str(),
        _port.c_str(),
        beast::bind_front_handler(
          &HttpRequest<ST>::on_resolve,
          this ));

      return _promise.get_future();
    }

    void on_resolve(
      beast::error_code ec,
      tcp::resolver::results_type results)
    {
      if(ec)
      {
        _promise.set_value(false);
        return fail(ec, "resolve"); 
      }

      // Set a timeout on the operation
      _stream.expires_after(std::chrono::seconds(5));

      // cout << "[HttpRequest] on_resolve" << endl;

      // Make the connection on the IP address we get from a lookup
      _stream.async_connect(
        results,
        beast::bind_front_handler(
          &HttpRequest<ST>::on_connect,
          this));
          
      boost::this_fiber::yield();
    }

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
      if(ec)
      {
        _promise.set_value(false);
        return fail(ec, "connect");
      }

      // Set a timeout on the operation
      _stream.expires_after(std::chrono::seconds(5));

      // cout << "[HttpRequest] on_connect" << endl;

      // Send the HTTP request to the remote host
      http::async_write(_stream, _req,
        beast::bind_front_handler(
          &HttpRequest<ST>::on_write,
          this));

      boost::this_fiber::yield();
    }

    void on_write(
      beast::error_code ec,
      std::size_t bytes_transferred)
    {
      boost::ignore_unused(bytes_transferred);

      if(ec)
      {
        _promise.set_value(false);
        return fail(ec, "write");
      }

      // cout << "[HttpRequest] on_write ( buffer size = " << _buffer.size() << ")" << endl;
      
      // Receive the HTTP response
      _buffer.consume(_buffer.size());
      _res = {};

      http::async_read(_stream, _buffer, _res,
        beast::bind_front_handler(
          &HttpRequest<ST>::on_read,
          this));

      boost::this_fiber::yield();
    }

    void on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
        {
          _promise.set_value(false);
          return fail(ec, "read");
        }

        // cout << "[HttpRequest] on_read ( buffer size = " << _buffer.size() << ")" << endl;

        // Write the message to standard out
        // std::cout << _res << std::endl;
        _promise.set_value(true);

        // Gracefully close the socket
        _stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if(ec && ec != beast::errc::not_connected)
          return fail(ec, "shutdown");

        // If we get here then the connection is closed gracefully

        // release();
    }

    void
    fail(beast::error_code ec, char const* what)
    {
      std::cerr << what << ": " << ec.message() << "\n";

      if(_stream.socket().is_open())
        _stream.socket().shutdown(tcp::socket::shutdown_both, ec);
      // release();
    }

    // bool is_ready_result() { return _ready_result; }


    void release()
    {
      HttpPool<ST>::GetInstance()->release_http(this);
    }
    http::response<http::string_body>& get_res() { return _res; }

    bool is_release() { return _is_release; }
    void set_release() { _is_release = true; }
    void reset_release() { _is_release = false; }

  private:
    string _host;
    string _port;
    tcp::resolver _resolver;
    beast::tcp_stream _stream;
    beast::flat_buffer _buffer; // (Must persist between reads)
    http::request<http::string_body> _req;
    http::response<http::string_body> _res;
    
    // bool _ready_result { false };
    boost::fibers::promise<bool> _promise{};
    bool _is_release { false };
  };


  // c++14 
  template <typename E>
  constexpr auto to_underlying(E e) noexcept
  {
      return static_cast<std::underlying_type_t<E>>(e);
  }
  // c++23 : use std::to_underlying(E)

  template<server_type ST>
  class HttpPool : public Singleton<HttpPool<ST>>
  {
    using HttpRequestPtr = HttpRequest<ST>*;

  public:

    HttpPool() {}
    ~HttpPool() { clear(); }


    void prepare(size_t count = 1)
    {
      for(size_t i = 0; i < count; ++i)
      {
        auto ptr = new HttpRequest<ST>(*_io, _ip, _port);
        _pool.push(ptr);
      }
    }

    void prepare(net::io_context * io, const service_info * sinfo )
    {
      _io = io;

      _ip = sinfo->ip;
      _port = sinfo->port_str;

      prepare();
    }
    void clear()
    {
      HttpRequestPtr ptr;
      while(_pool.pop(ptr))
      {
        delete ptr;
      }
    }


    string& get_ip() { return _ip; }
    string& get_port() { return _port;  }


    HttpRequestPtr get_http()
    {
      HttpRequest<ST> * ptr;
      while(false == _pool.pop(ptr))
      {
        prepare();
      }

      // cout << "[HttpPool] get : " << ptr << endl;

      ptr->reset_release();

      return ptr;
    }

    void release_http( HttpRequestPtr ptr)
    {
      if(false == ptr->is_release()) 
      {
        // cout << "[HttpPool] release : " << ptr << endl;
        ptr->set_release();
        _pool.push(ptr);
      }
    }


  private:
    
    // lockfree::queue 는 shared_ptr 을 못 담는다. 제한.
    lockfree::queue< HttpRequestPtr > _pool{128};
    net::io_context* _io{nullptr};

    string _ip;
    string _port;
  };


  #define GetHttpRequest(type) raylee::HttpPool<type>::GetInstance()->get_http()

  template<server_type ST>
  class HttpObject
  {
  public:
    HttpObject(HttpRequest<ST> * ptr) : _ptr(ptr) {}
    ~HttpObject() { if(_ptr) _ptr->release(); }

    HttpRequest<ST> * operator->() { return _ptr; }

  private:
    HttpRequest<ST> * _ptr;
  };
  

}
#endif