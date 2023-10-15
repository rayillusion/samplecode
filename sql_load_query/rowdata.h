#ifndef RAYLEE_RESOURCES_ROWDATA_H
#define RAYLEE_RESOURCES_ROWDATA_H

#include <cstring>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

namespace raylee::resource
{
  using IndexT = int;
  using namespace std;

  struct Data
  {
    Data() = default;
    Data(const Data& dt) : _type(dt._type) {
      switch(dt._type)
      {
      case Data::STRING:
        if(dt._pstr) 
        {
          size_t len = strlen(dt._pstr);
          _pstr = new char[len+1];
          memcpy(_pstr, dt._pstr, len);
          _pstr[len] = 0;
        } 
        else
        {
          _pstr = new char[1];
          _pstr[0] = 0;
        }
        break;
      
      case Data::CHAR:
      case Data::SHORT:
      case Data::INT:
        _i = dt._i;
        break;
      case Data::UCHAR:
      case Data::USHORT:
      case Data::UINT:
        _ui = dt._ui;
        break;

      case Data::UINT64:
        _ui64 = dt._ui64;
        break;
      case Data::INT64:
        _i64 = dt._i64;
        break;
      case Data::FLOAT:
        _f = dt._f;
        break;
      case Data::DOUBLE:
        _d = dt._d; 
        break;
      case Data::TIME:
        _time = dt._time; 
        break;
      default:
        ;
      }
    }
    ~Data() 
    {
      if(_type == Type::STRING){
        delete _pstr;
        _pstr = nullptr;
      }
    }

    enum Type { 
      UCHAR, CHAR,        // (unsigned) tinyint,
      USHORT, SHORT,      // (unsigned) smallint
      UINT, INT,          // (unsigned) int
      UINT64, INT64,      // (unsigned) bigint
      FLOAT,              // float
      DOUBLE,             // double
      STRING,             // varchar(), char(), binary(), timestamp, date, set, ...
      TIME,               // timestamp / date, 
      NONE,
    };

    bool is_set() { return _type != Type::NONE; }

    Type _type { Type::NONE };

    union {
      int32_t _i;
      uint32_t _ui;

      int64_t _i64;
      uint64_t _ui64;

      float _f;
      double _d;
      char * _pstr;
      time_t _time;
    };

    void set_char(char v) { _type = CHAR; _i = v; }
    void set_uchar(unsigned char v) { _type = UCHAR; _ui = v; }
    void set_short(short v) { _type = SHORT; _i = v; }
    void set_ushort(ushort v) { _type = USHORT; _ui = v; }
    void set_int(int32_t v) { _type = INT; _i = v; }
    void set_uint(uint32_t v) { _type = UINT; _ui = v; }
    void set_int64(int64_t v) { _type = INT64; _i64 = v; }
    void set_uint64(uint64_t v) { _type = UINT64; _ui64 = v; }
    void set_time(time_t t) { _type = TIME; _time = t;}

    void set_float(float v) { _type = FLOAT; _f = v; }
    void set_double(double v) { _type = DOUBLE; _d = v; }
    void set_string(const char * v, size_t len) { _type = STRING; 
      _pstr = new char[len+1]; 
      memcpy(_pstr, v, len);
      _pstr[len] = 0;
    }

    int get_int() { return _i; }

    bool is_true()
    {
      switch(_type)
      {
      case STRING:
        return (0 == strcmp(_pstr, "TRUE"));
      case TIME:
        return _time != 0;
        break;
      case FLOAT:
        return abs(_f) < 0.001f;
      case DOUBLE:
        return abs(_d) < 0.000001;
      case CHAR:
      case SHORT:
      case INT:
        return _i != 0;
      case UCHAR:
      case USHORT:
      case UINT:
        return _ui != 0;
      case INT64:
        return _i64 != 0;
      case UINT64: 
        return _ui64 != 0;
      default:
        return false;
      }

      return false;
    }

    bool operator==(const Data& other)
    {
      if(_type != other._type)
        return false;

      switch(_type)
      {
      case Data::STRING:
        // cout << "My : " << ( (_pstr) ? string(_pstr) : string("Null") ) << ", Ot : " <<  ((other._pstr) ? string(other._pstr) : string("Null")) << endl;
        return (0 == strcmp(_pstr, other._pstr));

      case Data::CHAR:
      case Data::SHORT:
      case Data::INT:
        return (_i == other._i);

      case Data::USHORT:
      case Data::UCHAR:
      case Data::UINT:
        return (_ui == other._ui);

      case Data::UINT64:
        return (_ui64 == other._ui64);
        
      case Data::INT64:
        return (_i64 == other._i64);

      case Data::FLOAT:
        return (_f == other._f);
        
      case Data::DOUBLE:
        return (_d == other._d);
      
      case Data::TIME:
        return (_time == other._time);

      default:
        cout << "Type Diff My = " << _type << ", other = " << other._type << endl;
        return false;
      }
    }

    bool operator!=(const Data& other)
    {
      if(_type != other._type)
        return false;

      return !(*this == other);
    }

    bool operator>(const Data& other)
    {
      if(_type != other._type)
        return false;

      switch(_type)
      {
      case Data::STRING:
        return false;

      case Data::CHAR:
      case Data::SHORT:
      case Data::INT:
        return (_i > other._i);

      case Data::UCHAR:
      case Data::USHORT:
      case Data::UINT:
        return (_ui > other._ui);
        
      case Data::UINT64:
        return (_ui64 > other._ui64);
        
      case Data::INT64:
        return (_i64 > other._i64);

      case Data::FLOAT:
        return (_f > other._f);
        
      case Data::DOUBLE:
        return (_d > other._d);

      case Data::TIME:
        return (_time > other._time);

      default:
        return false;
      }
    }

    bool operator<(const Data& other)
    {
      if(_type != other._type)
        return false;

      switch(_type)
      {
      case Data::STRING:
        return false;

      case Data::CHAR:
      case Data::SHORT:
      case Data::INT:
        return (_i < other._i);

      case Data::UCHAR:
      case Data::USHORT:
      case Data::UINT:
        return (_ui < other._ui);
      
      case Data::UINT64:
        return (_ui64 < other._ui64);
        
      case Data::INT64:
        return (_i64 < other._i64);

      case Data::FLOAT:
        return (_f < other._f);
        
      case Data::DOUBLE:
        return (_d < other._d);

      case Data::TIME:
        return (_time < other._time);

      default:
        return false;
      }
    }

    bool operator>=(const Data& other)
    {
      if(_type != other._type)
        return false;

      return !operator<(other);
    }

    bool operator<=(const Data& other)
    {
      if(_type != other._type)
        return false;

      return !operator>(other);
    }

  };



  class RowData
  {
  public:

    // template<typename T>
    // T get(int index) 
    // {
    //   if(index >= static_cast<int>(_data.size())) return T{};

    //   auto& d = _data[index];
    //   switch(d._type)
    //   {
    //   case Data::STRING:
    //     return d._pstr;
    //   case Data::UCHAR:
    //     return d._uc;
    //   case Data::CHAR:
    //     return d._c;
    //   case Data::USHORT:
    //     return d._us;
    //   case Data::SHORT:
    //     return d._s;
    //   case Data::UINT:
    //     return d._ui;
    //   case Data::INT:
    //     return d._i;
    //   case Data::UINT64:
    //     return d._ui64;
    //   case Data::INT64:
    //     return d._i64;
    //   case Data::FLOAT:
    //     return d._f;
    //   case Data::DOUBLE:
    //     return d._d;
    //   }
    //   return T{};
    // }

    char get_char(int i) { return static_cast<char>(_data[i]._i); }
    unsigned char get_uchar(int i) { return static_cast<unsigned char>(_data[i]._ui); }
    short get_short(int i) { return static_cast<short>(_data[i]._i); }
    ushort get_ushort(int i) { return static_cast<unsigned short>(_data[i]._ui); }
    int get_int(int i) { return _data[i]._i; }
    uint get_uint(int i) { return _data[i]._ui; }
    int64_t get_int64(int i) { return _data[i]._i64; }
    uint64_t get_uint64(int i) { return _data[i]._ui64; }
    time_t get_time(int i) { return _data[i]._time ; }
    
    float get_float(int i) { return _data[i]._f; }
    double get_double(int i) { return _data[i]._d; }
    const char * get_string(int i) { return _data[i]._pstr; }

    Data& get_data(int i) { return _data[i]; }

    int get_index() { return _data[0]._i; }

    template<class T>
    T get(int i);
    
    RowData(int cp) { _data.resize(cp); ++_count; };
    ~RowData() { _data.clear(); --_count; }

    void print();

    static int get_count() { return _count; }


    // UCHAR, CHAR,        // (unsigned) tinyint,
    // USHORT, SHORT,      // (unsigned) smallint
    // UINT, INT,          // (unsigned) int
    // UINT64, INT64,      // (unsigned) bigint
    // FLOAT,              // float
    // DOUBLE,             // double
    // STRING,             // varchar(), char(), binary(), timestamp, date, set, ...
    // NONE,


  private:
    vector<Data> _data;

    static int _count;
  };
}

#endif