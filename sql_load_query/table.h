#ifndef RAYLEE_RESOURCES_TABLE_H
#define RAYLEE_RESOURCES_TABLE_H


#include <initializer_list>
#include <cstring>
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <map>
#include <algorithm>

#include "rowdata.h"

namespace raylee::resource
{
  using IndexT = int;
  using namespace std;

  class ColumnMeta
  {
  public:
    ColumnMeta() { ++_count; }
    ~ColumnMeta() { --_count; }

    ColumnMeta(int idx, Data::Type type, string& name, size_t max_length = 0)
      : _index(idx), _type(type), _name(name), _max_length(max_length)
    {
      ++_count;
    }

    const int get_index() { return _index; }
    const Data::Type get_type() { return _type; }
    const string& get_name() { return _name; }
    const size_t get_max_length() { return _max_length; }

    static int get_count() { return _count; }

    void make_data(string& value, Data& data);
    void make_data(string& value, vector<Data>& data);

  private:
    int _index;   // 0 ~
    Data::Type _type;
    string _name;

    size_t _max_length{0};  // only for string

    static int _count;
  };

  class Table
  {
  public:
    Table(string& name);
    ~Table();

    const string& get_name() { return _name; }

    int get_column_index(const string& column) {
      auto itr = _columnm_index.find(column); 
      if(_columnm_index.end() != itr)
        return itr->second;
      return -1;
    }

    shared_ptr<ColumnMeta> get_column_meta(string& column) {
      auto itr = _columns.find(column); 
      if(_columns.end() != itr)
        return itr->second;
      return shared_ptr<ColumnMeta>(nullptr);
    }

    size_t get_column_size() { return _columns.size(); }
    size_t get_row_count() { return _vec_data.size(); }

    bool has_column(string& column)
    {
      return (_columnm_index.end() != _columnm_index.find(column) );
    }
    const string& name() { return _name; }

    void add_column(shared_ptr< ColumnMeta > cm);
    shared_ptr<ColumnMeta> find_column(const string& name);
    void add_row(int index, shared_ptr<RowData> rd);

    void print_table();

    shared_ptr<RowData> find(int index)
    {
      auto itr = _data.find(index);
      if(itr != _data.end())
      {
        return itr->second;
      }

      return shared_ptr<RowData>(nullptr);
    }

    vector<shared_ptr<RowData>>::iterator begin() { return _vec_data.begin(); }
    vector<shared_ptr<RowData>>::iterator end() { return _vec_data.end(); }

  private:
    map<string, int>  _columnm_index;
    map<string, shared_ptr<ColumnMeta> > _columns;
    unordered_map<IndexT, shared_ptr<RowData>> _data;

    // <name, ..>
    vector<shared_ptr<RowData>> _vec_data;

    string _name;
  };
};

#endif