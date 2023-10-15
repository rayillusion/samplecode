#ifndef RAYLEE_RESOURCES_QUERRY_H
#define RAYLEE_RESOURCES_QUERRY_H


#include <random>
#include <cstring>
#include <string>
#include <string_view>
#include <algorithm>

#include <map>
#include <vector>

#include <iostream>

#include "metadb.h"

namespace raylee::resource
{
  using namespace std;

  class Node
  {
  public:
    Node() {}
    ~Node() {}

    virtual find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end) = 0;

    virtual void print(int tap)
    {
      if(_child) _child->print(tap + 2);
    }

  private:
    Node * _child;
  };

  class ConditionNode : public Node
  {
  public:
    ConditionNode(string& col, Data& v)
      : Node()
      , _column_name(col)
      , _value(v)
      
    {
    }

    ConditionNode(string& col) : Node(), _column_name(col) {}
    ~ConditionNode() {}

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end) = 0;

    void print(int tap) override
    {
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "Condition Node\n";
    }

    Data& get_value() { return _value; }

  protected:
    string _column_name;
    Data _value;
  };


  // usage
  // col = value
  class EqualCondition : public ConditionNode
  {
  public:
    EqualCondition(string& col, Data& v) : ConditionNode(col, v) {}
    ~EqualCondition() {}

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end);

    void print(int tap) override
    {
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "EqualCondition : " << _column_name << endl;
    }

  };

  // usage: 
  // col in [ adb, efc, dlj ]
  class InCondition : public ConditionNode
  {
  public:
    InCondition(string& col, vector<Data>& v) : ConditionNode(col), _values(std::move(v)) {}
    ~InCondition() {}

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end);

    void print(int tap) override
    {
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "InCondition : " << _column_name << endl;
    }
  private:
    vector<Data> _values;

  };

  // usage: col != value
  class NotEqualCondition : public ConditionNode
  {
  public:
    NotEqualCondition(string& col, Data& v) : ConditionNode(col, v) {}
    ~NotEqualCondition() {}

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end);

    void print(int tap) override
    {
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "NotEqualCondition : " << _column_name << endl;
    }

  };

  // usage: col > value
  class GreaterCondition : public ConditionNode
  {
  public:
    GreaterCondition(string& col, Data& v) : ConditionNode(col, v) {}
    ~GreaterCondition() {}

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end);

    void print(int tap) override
    {
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "GreaterCondition : " << _column_name << endl;
    }

  };

  // usage: col < value
  class LessCondition : public ConditionNode
  {
  public:
    LessCondition(string& col, Data& v) : ConditionNode(col, v) {}
    ~LessCondition() {}

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end);

    void print(int tap) override
    {
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "LessCondition : " << _column_name << endl;
    }
  };

  // usage: col >= value
  class GreaterEqualCondition : public ConditionNode
  {
  public:
    GreaterEqualCondition(string& col, Data& v) : ConditionNode(col, v) {}
    ~GreaterEqualCondition() {}

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end);

    void print(int tap) override
    {
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "GreaterEqualCondition : " << _column_name << endl;
    }
  };

  // usage: col <= value
  class LessEqualCondition : public ConditionNode
  {
  public:
    LessEqualCondition(string& col, Data& v) : ConditionNode(col, v) {}
    ~LessEqualCondition() {}

   find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end);

    void print(int tap) override
    {
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "LessEqualCondition : " << _column_name << endl;
    }
  };

  class ComposerNode : public Node
  {
  public:
    ComposerNode() : Node() {}
    ~ComposerNode() {}

    void AddNode(Node * p)
    {
      _conditios.push_back(p);
    }

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end) = 0;

    void print(int tap) override
    {
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "ComposerNode\n";
    }

  protected:
     vector<Node*> _conditios;
  };

  // usage: state1 and state2
  class AndComposer : public ComposerNode
  {
  public:
    AndComposer() : ComposerNode() {}
    ~AndComposer() {}

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end);

    void print(int tap) override
    {
      int new_tab = tap + 2;
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "AndComposer\n";

      auto itr = _conditios.begin();
      while(itr != _conditios.end())
      {
        (*itr)->print(new_tab);
        ++itr;
      }
    }

  };

  // usage: state1 or state2
  class OrComposer : public ComposerNode
  {
  public:
    OrComposer() : ComposerNode() {}
    ~OrComposer() {}

    find_result process(shared_ptr<Table> ptable, vector<shared_ptr<RowData>>::iterator begin, vector<shared_ptr<RowData>>::iterator end);

    void print(int tap) override
    {
      int new_tab = tap + 2;
      string taps{""};
      while(tap > 0)
      {
        taps.push_back(' ');
        --tap;
      }

      cerr << taps << "OrComposer\n";

      auto itr = _conditios.begin();
      while(itr != _conditios.end())
      {
        (*itr)->print(new_tab);
        ++itr;
      }
    }
  };

    

  class Query
  {
  public:
    Query(shared_ptr<Table> pTable, const string& qry)
      : _root(nullptr)
      , _table(pTable)
      , _query(qry)
    {

    }
    ~Query() {}

    find_result get_result();

    ConditionNode * CreateOpNode(int tp, string& col, string& value);
    ComposerNode * CreateComposerNode(int tp);

  private:
    void parse();

    Node * parse_unit(vector<string>::iterator itr, vector<string>::iterator end);

    void split(string_view qry, vector<string>& result);

    find_result calc();

    void print()
    {
      cerr << "Query '" << _table->get_name() << "' = " << _query << endl;

      if(_root) _root->print(0);
      else cout << "No Root\n";
    }

  private:
    Node * _root;
    shared_ptr<Table> _table;
    string _query;
  };
} // namespace raylee


#endif