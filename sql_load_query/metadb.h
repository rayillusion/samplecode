#ifndef RAYLEE_RESOURCES_METADB_H
#define RAYLEE_RESOURCES_METADB_H

#include <string>
#include <string_view>
#include <ctime>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <filesystem>
#include <iostream>

#include "table.h"

#include "../system/singleton.h"

namespace raylee::resource
{
  class Table;

  using namespace std;

  class find_result
  {
  public:
    void set_table(shared_ptr<Table> tbl) { _table = tbl; }
    void add_rows(shared_ptr<RowData> row) { _set.push_back(row); }

    size_t count() { return _set.size(); }

    vector<shared_ptr<RowData>>::iterator begin() { return _set.begin(); }
    vector<shared_ptr<RowData>>::iterator end() { return _set.end(); }

    shared_ptr<RowData> get_one() {
      if(_set.size() > 0)
        return *(_set.begin());
      return shared_ptr<RowData>{nullptr};
    }

    shared_ptr<Table> get_table() { return _table; }

    void and_apply(find_result &other)
    {
      vector<shared_ptr<RowData>> output(_set.size());
      auto iter = set_intersection(_set.begin(), _set.end(), other.begin(), other.end(), output.begin());
      output.erase(iter, output.end());

      _set = output;    // 교체
    }

    void or_apply(find_result &other)
    {
      vector<shared_ptr<RowData>> output(_set.size() + other.count());
      auto iter = set_union(_set.begin(), _set.end(), other.begin(), other.end(), output.begin());
      output.erase(iter, output.end());

      _set = output;    // 교체
    }

    int get_column_index(const string& column)
    {
      if(_table)
      {
        return _table->get_column_index(column);
      }
      return -1;    // 이상.
    }

    resource::Data get_value(int where, const string& column)
    {
      if(_set.size() <= static_cast<size_t>(where))
      {
        return resource::Data();
      }

      int index = _table->get_column_index(column);
      if(index < 0) return Data();

      return _set[where]->get_data(index);
    }

    void print()
    { 
      cerr << "[Find Result] Count = " << count() << endl;
      for(size_t i = 0; i < count(); ++i)
      {
        cerr << "  Id(" << _set[i]->get_int(0) << ")\n";
      }
    }

    void get_map( map<int, shared_ptr<RowData>>& out)
    {
      for( auto& row : _set)
      {
        out.insert( { row->get_index(), row });
      }
    }

  private: 
    vector<shared_ptr<RowData> > _set;
    shared_ptr<Table> _table;
  };

  // const char * datafile = "scheme/metadb.sql";
  class MetaData : public Singleton<MetaData>
  {
    const double META_RELOAD_SECOND = 10.0;
    // const double META_RELOAD_SECOND = 180.0;

  public:
    MetaData();
    ~MetaData();

    /// @brief compare file time of local metafile and set or return false
    /// @param ft filetime of loacl metafile
    /// @return if update needed.
    bool check_and_update(time_t& ft)
    {
      if(_last_file_time == ft)
        return false;

      _last_file_time = ft;
      return true;
    }
    void set_time(time_t& ft)
    {
      _last_file_time = ft;
    }

    // start thread : run
    int start();
    void stop();

    // thread function.
    void run();   

    shared_ptr<Table> find_table(string& tbl);
    shared_ptr<Table> find_table(string_view tbl);

    void add_table(string& name, shared_ptr<Table> sptr);

    bool is_continue() { return _continue; }

    void check_and_reload();

    int reload();

    void set_file_time(filesystem::file_time_type& ftt)
    {
      _meta_file_time = ftt;
    }

    bool is_changed(filesystem::file_time_type& newer)
    {
      return (_meta_file_time != newer);
    }

    filesystem::file_time_type& get_file_time()
    { 
      return _meta_file_time;
    }

    void print_table();

    string array_to_query_string(vector<int>& v);
    string array_to_query_string(vector<string>& v);

    find_result select(const string& tbl, const string& querystr);
    find_result find_with_index(const string& tbl, int idx);
    find_result find_with_index(const string& tbl, vector<int>& indexes);

    template<class T>
    T find_max(const string& tbl, const string& field)
    {
      auto pTable = find_table(tbl);
      if(nullptr == pTable)
      {
        return T{0};
      }

      int column_idx = pTable->get_column_index(field);
      if(column_idx < 0)
      {
        return T{0};
      }

      auto begin = pTable->begin();
      auto end = pTable->end();

      auto max_compare = [column_idx](shared_ptr<RowData> a, shared_ptr<RowData> b)
      {
        return a->template get<T>(column_idx) < b->template get<T>(column_idx);
      };

      auto max_itr = std::max_element(begin, end, max_compare);

      // set : https://en.cppreference.com/w/cpp/language/dependent_name
      // 'The template disambiguator for dependent names'
      return (*max_itr)->template get<T>(column_idx);
      
    }

  private:
    time_t _last_file_time{0};
    mutex _table_lock;
    // key = Table Name, Data = Table ptr
    unordered_map<string, shared_ptr<Table> > _tables;

    thread * _run_thread{nullptr};
    chrono::steady_clock::time_point _prev_loading_time;
    filesystem::file_time_type _meta_file_time;
   
    bool _continue{true};
  };



};

#endif