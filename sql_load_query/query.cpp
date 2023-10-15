#include "../defines/platform.h"

#include <stdexcept>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstdlib>


#include "query.h"


namespace raylee::resource
{
  
  find_result EqualCondition::process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end)
  {
    find_result temp;

    cerr << "[EqualCondition] col : " << _column_name << ", value = " << _value.get_int() << endl;
  
    if(0 == _column_name.compare("Index")) 
    {
      shared_ptr<RowData> p = ptable->find(_value.get_int());
      if(p)
      {
        temp.add_rows(p);
      }
      else
      {
        cout << "Not found index = " << _value.get_int() << "\n";
      }
    }
    else
    {
      int col_index = ptable->get_column_index(_column_name);
      if(col_index < 0) {
        cerr << "Error : Not found column : " << _column_name << endl;
        return temp;
      }

      while(begin != end)
      {
        if(_value == (*begin)->get_data(col_index))
        {
          temp.add_rows(*begin);
        }
        ++begin;
      }
    }

    return temp;
  }

  find_result InCondition::process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end)
  {
    find_result temp;

    cerr << "[InCondition] col : " << _column_name << ", value count = " << _values.size() << endl;
  
    if(0 == _column_name.compare("Index")) 
    {
      auto v_itr = _values.begin();
      auto v_end = _values.end();
      while(v_itr != v_end)
      {
        shared_ptr<RowData> p = ptable->find((*v_itr).get_int());
        if(p)
        {
          temp.add_rows(p);
        }
        else
        {
          cout << "Not found index = " << _value.get_int() << "\n";
        }

        ++v_itr;
      }
    }
    else
    {
      int col_index = ptable->get_column_index(_column_name);
      if(col_index < 0) {
        cerr << "Error : Not found column : " << _column_name << endl;
        return temp;
      }

      while(begin != end)
      {
        auto v_itr = _values.begin();
        auto v_end = _values.end();

        while(v_itr != v_end)
        {
          if((*v_itr) == (*begin)->get_data(col_index))
          {
            temp.add_rows(*begin);
            break;
          }
          ++v_itr;
        }
        
        ++begin;
      }
    }

    return temp;
  }

  find_result NotEqualCondition::process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end)
  {
    find_result temp;

    cerr << "[NotEqualCondition] col : " << _column_name << endl;
   
    int col_index = ptable->get_column_index(_column_name);
    if(col_index < 0) {
      cout << "Error : Not found column : " << _column_name << endl;
      return temp;
    }

    while(begin != end)
    {
      if(_value != (*begin)->get_data(col_index))
      {
        temp.add_rows(*begin);
      }
      ++begin;
    }
    
    return temp;
  }

  find_result GreaterCondition::process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end)
  {
    cerr << "[GreaterCondition] col : " << _column_name << endl;

    find_result temp;
   
    int col_index = ptable->get_column_index(_column_name);
    if(col_index < 0) {
      cout << "Error : Not found column : " << _column_name << endl;
      return temp;
    }

    while(begin != end)
    {
      if((*begin)->get_data(col_index) > _value)
      {
        temp.add_rows(*begin);
      }
      ++begin;
    }
    
    return temp;
  }

  find_result LessCondition::process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end)
  {
    cerr << "[LessCondition] col : " << _column_name << endl;

    find_result temp;
   
    int col_index = ptable->get_column_index(_column_name);
    if(col_index < 0) {
      cout << "Error : Not found column : " << _column_name << endl;
      return temp;
    }

    while(begin != end)
    {
      if((*begin)->get_data(col_index) < _value)
      {
        temp.add_rows(*begin);
      }
      ++begin;
    }
    
    return temp;
  }

  find_result GreaterEqualCondition::process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end)
  {
    cerr << "[GreaterEqualCondition] col : " << _column_name << endl;

    find_result temp;
   
    int col_index = ptable->get_column_index(_column_name);
    if(col_index < 0) {
      cout << "Error : Not found column : " << _column_name << endl;
      return temp;
    }

    while(begin != end)
    {
      if((*begin)->get_data(col_index) >= _value )
      {
        temp.add_rows(*begin);
      }
      ++begin;
    }
    
    return temp;
  }

  find_result LessEqualCondition::process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end)
  {
    cerr << "[LessEqualCondition] col : " << _column_name << endl;

    find_result temp;
   
    int col_index = ptable->get_column_index(_column_name);
    if(col_index < 0) {
      cout << "Error : Not found column : " << _column_name << endl;
      return temp;
    }

    while(begin != end)
    {
      if((*begin)->get_data(col_index) <= _value)
      {
        temp.add_rows(*begin);
      }
      ++begin;
    }
    
    return temp;
  }

  find_result AndComposer::process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end)
  {
    cerr << "[AndComposer]\n";

    auto itr = _conditios.begin();

    bool first = true;
    find_result prev;

    while(itr != _conditios.end())
    {
      find_result cur = (*itr)->process(ptable, begin, end);
      if(first)
      {
        prev = cur;

        first = false;
      }
      else
      {
        prev.and_apply(cur);
      }

      if(prev.count() == 0) break;

      begin = prev.begin();
      end = prev.end();

      ++itr;
    } 

    return prev;
  }

  find_result OrComposer::process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end)
  {
    cerr << "[OrComposer]\n";

    auto itr = _conditios.begin();

    bool first = true;
    find_result prev;

    while(itr != _conditios.end())
    {
      find_result cur = (*itr)->process(ptable, begin, end);
      if(first)
      {
        prev = cur;

        first = false;
      }
      else
      {
        prev.or_apply(cur);
      }

      if(prev.count() == 0) break;

      // begin = prev.begin();
      // end = prev.end();

      ++itr;
    } 

    return prev;
  }

  find_result Query::get_result()
  {
    if(!_table) return find_result();

    if(nullptr == _root) 
    {
      parse();
    }

    print();

    find_result result = calc();
    result.set_table(_table);
    
    result.print();

    return result;
  }

  find_result Query::calc()
  {
    if(_root)
    {
      auto begin = _table->begin();
      auto end = _table->end();

      find_result result = _root->process(_table, begin, end);
      return result;
    }

    return find_result();
  }

  std::string str_tolower(std::string s)
  {
      std::transform(s.begin(), s.end(), s.begin(), 
                  // static_cast<int(*)(int)>(std::tolower)         // wrong
                  // [](int c){ return std::tolower(c); }           // wrong
                  // [](char c){ return std::tolower(c); }          // wrong
                    [](unsigned char c){ return std::tolower(c); } // correct
                    );
      return s;
  }

  enum type
  {
    none = 0,
    word,
    open_par, // (
    close_par, // )
    com_and,  // "and"
    com_or,   // "or"
    op_equal,   // =
    op_in,      // "in"
    op_not_equal, // !=
    op_greater,   // >
    op_greaterequal,  // >= , =>
    op_less,      // <
    op_lessequal, // <=, =<
  };

  type what_is(string& w)
  {
    string t = str_tolower(w);
    type tp = word;
    if(0 == t.compare("("))
    {
      tp = open_par;
    }
    else if(0 == t.compare(")"))
    {
      tp = close_par;
    }
    else if(0 == t.compare("and"))
    {
      tp = com_and;
    } 
    else if(0 == t.compare("or"))
    {
      tp = com_or;
    }
    else if(0 == t.compare("="))
    {
      tp = op_equal;
    }
    else if(0 == t.compare("in"))
    {
      tp = op_in;
    }
    else if(0 == t.compare("!="))
    {
      tp = op_not_equal;
    }
    else if(0 == t.compare(">"))
    {
      tp = op_greater;
    }
    else if(0 == t.compare(">=") || 0 == t.compare("=>"))
    {
      tp = op_greaterequal;
    }
    else if(0 == t.compare("<"))
    {
      tp = op_less;
    }
    else if(0 == t.compare("<=") || 0 == t.compare("=<"))
    {
      tp = op_lessequal;
    }

    return tp;
  }

  void Query::parse() 
  {
    // const char * oper = "<>=";

    vector<string> words;
    split(_query, words);

    cerr << "At " <<  _table->get_name() << " Split '" << _query << "'\n";
    for(size_t i = 0; i < words.size(); ++i)
    {
      cerr << i << " : " <<  words.at(i) << endl;
    }

    auto itr = words.begin();

    _root = parse_unit(itr, words.end());
  }


  Node * Query::parse_unit(vector<string>::iterator itr, vector<string>::iterator end)
  {
    Node * p = nullptr;

    string col{""};
    string value("");
    type cur_oper = none;


    while(itr != end)
    {
      string cur = *itr;
      type tp = what_is(cur);

      // cout << "Parse : " << cur << ", op = " << tp << endl;

      switch(tp)
      {
      case word:
        if(col.length() == 0)
        {
          col = cur;
        }
        else 
        {
          value = cur;

          if(cur_oper >= op_equal )
          {
            // 만든다.
            p = CreateOpNode(cur_oper, col, value);
          }
        }
        break;
      case open_par:
        ++itr;
        return parse_unit(itr, end);
        break;
      case close_par:
        return p;
        break;
      case com_and:
      case com_or: {
        ComposerNode * com = CreateComposerNode(tp);

        if(p)
          com->AddNode(p);

        ++itr;
        com->AddNode( parse_unit(itr, end) );

        p = com;

        return p;
      }
        break;
      case op_equal:
      case op_in:
      case op_not_equal:
      case op_greater:
      case op_greaterequal:
      case op_less:
      case op_lessequal:
        cur_oper = tp;
        break;
      default:
        cout << "Type None Error at : '" << cur << "'\n";
        throw new runtime_error("Type None Error");
      }

      if(itr != end)
        ++itr;
    }

    return p;
  }
  

  
  void Query::split(string_view qry, vector<string>& result)
  {
    size_t length = qry.length();
    size_t i = 0;

    auto is_oper = [](char c) { if(c == '<' || c == '>' || c == '=' || c == '!') return true; else return false; };

    string cur;
    while(i < length)
    {
      char c = qry.at(i);

      if( isalnum(c) || c == '_')
      {
        cur.push_back(c);
      }
      else if(c == '(' || c == ')')
      {
        if(cur.length() > 0)
        {
          result.push_back(cur);
          cur.clear();
        }

        cur.push_back(c);
        result.push_back(cur);
        cur.clear();
      }
      else if(c == '[')
      {
        cur.clear();

        ++i;
        c = qry.at(i);

        while(c != ']' && i < length)
        {
          cur.push_back(c);
          ++i;
          c = qry.at(i);
        }

        if(cur.length() > 0)
          result.push_back(cur);
        
        cur.clear();
        
      }
      else if(c == '\'')
      {
        if(cur.length() > 0)
        {
          result.push_back(cur);
          cur.clear();
        }

        ++i;
        c = qry.at(i);
        
        while( (c = qry.at(i) ) != '\'' && i < length)
        {
          cur.push_back(c);
          ++i;
        }

        result.push_back(cur);
        cur.clear();
      }
      else 
      {
        if(is_oper(c))
        {
          if(cur.length() > 0)
          {
            result.push_back(cur);
            cur.clear();
          }

          char next = qry.at(i+1);
          if(is_oper(next))
          {
            cur.push_back(c);
            cur.push_back(next);

            result.push_back(cur);
            cur.clear();

            ++i;
          }
          else
          {
            cur.push_back(c);

            result.push_back(cur);
            cur.clear();
          }
        }
        else
        {
          if(cur.length() > 0)
          {
            result.push_back(cur);
            cur.clear();
          }
        }
      }

      ++i;
    }

    if(cur.length() > 0)
    {
      result.push_back(cur);
    }
  }



  ConditionNode * Query::CreateOpNode(int tp, string& col, string& value)
  {
    ConditionNode * p = nullptr;
    auto colmeta = _table->get_column_meta(col);
    if(nullptr == colmeta)
    {
      cout << "[EXCEPTION] Table '" << _table->get_name() << "' doesn't have column '" << col << "'\n" ;
      throw runtime_error("table meta not found");
    }

    switch(static_cast<type>(tp))
    {
    case op_equal:
    {
      Data data;
      colmeta->make_data(value, data);
      p = new EqualCondition(col, data);
    }      
      break;
    case op_in:
    {
      vector<Data> data;
      colmeta->make_data(value, data);
      cerr << "[OpIn] value count = " << data.size() << endl;
      
      p = new InCondition(col, data);
    }
      break;
    case op_not_equal:
    {
      Data data;
      colmeta->make_data(value, data);
      p = new NotEqualCondition(col, data);
    }
      break;
    case op_greater:
    {
      Data data;
      colmeta->make_data(value, data);
      p = new GreaterCondition(col, data);
    }
      break;
    case op_greaterequal:
    {
      Data data;
      colmeta->make_data(value, data);
      p = new GreaterEqualCondition(col, data);
    }
      break;
    case op_less:
    {
      Data data;
      colmeta->make_data(value, data);
      p = new LessCondition(col, data);
    }
      break;
    case op_lessequal:
    {
      Data data;
      colmeta->make_data(value, data);
      p = new LessEqualCondition(col, data);
    }
      break;

    default:
      cout << "[Query] Unonwn tp at Op :" << tp << "\n";
      throw new runtime_error("[Query] Unonwn tp at Op");
    }

    return p;
  }


  ComposerNode * Query::CreateComposerNode(int tp)
  {
    ComposerNode * p {nullptr};
    switch(static_cast<type>(tp))
    {
    case com_and:
      p = new AndComposer();
      break;
    case com_or:
      p = new OrComposer();
      break;

    default:
      cout << "[Query] Unonwn tp at Compose :" << tp << "\n";
      throw new runtime_error("[Query] Unonwn tp at Composer");
    }

    return p;
  }

} // namespace raylee::resource
