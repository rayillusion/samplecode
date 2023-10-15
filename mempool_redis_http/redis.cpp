#include "../defines/platform.h"
#include "../defines/constants.h"

#include <iostream>
#include "redis.h"


namespace raylee::redis
{
  sw::redis::Redis * Redis::_redis = nullptr;

  void Redis::start()
  {
    sw::redis::ConnectionOptions connection_options;
    connection_options.host = "192.168.0.85";
    connection_options.port = 6379;
    // connection_options.user = "test-rpp";
    
    // NOTE: if any command is timed out, we throw a TimeoutError exception.
    connection_options.connect_timeout = std::chrono::seconds(20);
    connection_options.socket_timeout = std::chrono::seconds(20);

    sw::redis::ConnectionPoolOptions pool_options;
    pool_options.size = RAYLEE_REDIS_CONNECTION_COUNT;  // Pool size, i.e. max number of connections.

    // Optional. Max time to wait for a connection. 0ms by default, which means wait forever.
    // Say, the pool size is 3, while 4 threds try to fetch the connection, one of them will be blocked.
    pool_options.wait_timeout = std::chrono::seconds(10);

    // Optional. Max lifetime of a connection. 0ms by default, which means never expire the connection.
    // If the connection has been created for a long time, i.e. more than `connection_lifetime`,
    // it will be expired and reconnected.
    pool_options.connection_lifetime = std::chrono::minutes(10);

    _redis = new sw::redis::Redis(connection_options, pool_options);
  }

  void Redis::end()
  {
    
  }

}