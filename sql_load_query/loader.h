#ifndef RAYLEE_RESOURCES_LOADER_H
#define RAYLEE_RESOURCES_LOADER_H

#include <thread>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "metadb.h"
#include "table.h"

namespace raylee::resource
{
  using namespace std;

  class TableLoader 
  {
  public:
    TableLoader(std::string file_name);
    ~TableLoader() {}

    int load();

    int load_scheme_part(ifstream& fs, string& tbl_name);
    int load_data_part(ifstream& fs, string& tbl_name, vector<string>& columns);

    void run();

    bool validate();
    void skip(ifstream& fs);

    pair<string, filesystem::file_time_type> find_meta_file(string& fn);

    class TableMeta
    {
    public:
      TableMeta(string& name) : _name(name) {}
      ~TableMeta() {}

      void add_column(string& col) { _columns.push_back(col); }
      void set_data_count(size_t count) { _data_count = count; }
      void set_columns(vector<string>&& vec) { _columns = std::move(vec); }

      const string& get_name() { return _name; } 
      vector<string>& get_columns() { return _columns; }
      size_t get_data_count(size_t count) { return _data_count; }

    private:
      string _name;
      vector<string> _columns;
      size_t _data_count;
    };

    void add_table_meta(string& tblname, vector<string>&& columns, size_t data_count)
    {
      auto tbl = make_shared<TableMeta>(tblname);
      tbl->set_columns(move(columns));
      tbl->set_data_count(data_count);

      _tables.insert(make_pair(tbl->get_name(), tbl));
    }
    
    pair<Data::Type, size_t> get_data_type(vector<string>& vs);

    shared_ptr<TableMeta> find_meta(string& name);

  private:
    string _filename;
    thread _worker_thread;

    unordered_map<string, shared_ptr<TableMeta> > _tables;

  };
  
} // namespace raylee

#endif
