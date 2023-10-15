#ifndef EIDOS_SYSTEM_ASYNCREDIS_H
#define EIDOS_SYSTEM_ASYNCREDIS_H

#include <atomic>
#include <future>
#include <optional>
#include <string>
#include <string_view>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <sw/redis++/redis++.h>
#include "config.h"
#include "../utils/singleton.h"
#include "../logger/log.h"

namespace eidos::redis
{
  class Redis
  {
  public:
    static void start();
    static void end();

    static sw::redis::Redis * Get() { return Redis::_redis; }

  private:
    static sw::redis::Redis * _redis;
  };

  class Executor
  {
  public:
    virtual void run() = 0;


    void set_success(bool s = true) {
        _success = s;
    }
    bool is_success() { return _success; }

  private:
    std::atomic<bool> _success {false};

  };


  class Getter : public Executor
  {
  public:
    Getter(const string& key)
      : _key(key)
    {
    }

    void run() override
    {
      try
      {
        auto str = Redis::Get()->get(_key);
        if(str.has_value())
        {
          _result = str.value();
          set_success();
        }
   
      }
      catch(const std::exception& e)
      {
        PLOGE << e.what();
      }
    }

    string& get_result() { return _result; }
  
  private:
    string _key;
    string _result;
  };

  class Setter : public Executor
  {
  public:
    Setter(const string& key, const string& value)
      : _key(key)
      , _value(value)
    {
    }

    void run() override
    {
      try
      {
        if(Redis::Get()->set(_key, _value))
          set_success();
      }
      catch(const std::exception& e)
      {
        PLOGE << e.what();
      }
    }
  
  private:
    string _key;
    string _value;
  };


  class HMGetter : public Executor
  {
  public:
    HMGetter(const string& key, std::vector<std::string>& fields)
      : _key(key)
      , _fields(fields)
    {}


    void run() override
    {
      try
      {
        Redis::Get()->hmget(_key, _fields.begin(), _fields.end(), std::back_inserter(_res));
        set_success();

      }
      catch(const std::exception& e)
      {
        PLOGE << e.what();
      }
    }

    std::vector<std::optional<std::string>>& get_result() { return _res; }

  private:
    std::string _key;
    std::vector<std::string> _fields;
    std::vector<std::optional<std::string>> _res;
  };

  class HMSetter : public Executor
  {
  public:
    HMSetter(const string& key, std::unordered_map<std::string, std::string>& kvs)
      : _key_values(kvs), _key(key)
    {
    }

    void run() override
    {
      try
      {
        Redis::Get()->hmset(_key, _key_values.begin(), _key_values.end());
        set_success();
      }
      catch(const std::exception& e)
      {
        PLOGE << e.what();
      }
    }

  private:
    std::unordered_map<std::string, std::string> _key_values;
    std::string _key;
  };

}

#endif