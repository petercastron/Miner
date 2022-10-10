#pragma once
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include <vector>

#if defined(_UNICODE)

#define JCD_CHAR wchar_t
#define JCD_STRING std::wstring
#define JCD_T(s) L ## s 
#define JCD_ATOI _wtoi
#define JCD_ATOF _wtof
#define JCD_SPRINTF swprintf_s
#else

#define JCD_CONST_CHAR const char
#define JCD_CHAR char
#define JCD_STRING std::string
#define JCD_T(s) s
#define JCD_ATOI atoi
#define JCD_STRTOLL(s) (strtoll((s), NULL, 10))

#ifdef ANDROID
static __inline__ double JCD_ATOF(const char *nptr)
{
    return (strtod(nptr, NULL));
}
#else
#define JCD_ATOF atof
#endif

#define JCD_SPRINTF sprintf
#endif  // end of #if defined(_UNICODE)

#define JSON_ASSERT(condition) assert(condition)
#define JSON_ASSERT_MESSAGE(condition,message) if (!(condition)) throw (message)//std::runtime_error(message)
//#define JSON_ASSERT_MESSAGE(condition,message) 

namespace FJson {
class Value;
class NullValue;
class StringValue;
class BoolValue;
class NumberValue;
class ArrayValue;

typedef std::map<JCD_STRING, Value> ObjectValues;
typedef std::vector<Value> ArrayValues;

class Value {
public:

	typedef enum {
		JT_NULL = 0,
		JT_OBJECT,
		JT_STRING,
		JT_BOOLEAN,
		JT_INT,
		JT_UINT,
		JT_DOUBLE,
		JT_ARRAY
	} JSON_TYPE;

	friend class Parser;
	Value (JSON_TYPE type = JT_NULL) {
		_value_type = type;
		switch (type) {
		case JT_OBJECT:
		case JT_ARRAY:
		case JT_NULL: break;
		case JT_INT: 
		case JT_UINT: _base_value._uint = 0; break;
		case JT_DOUBLE: _base_value._double = 0.0; break;
		case JT_BOOLEAN: _base_value._bool = false; break;
		case JT_STRING: _value_string = JCD_T("");
		}
	}
	Value (int32_t value) { _value_type = JT_INT; _base_value._uint = value; }
	Value (u_int32_t value) { _value_type = JT_UINT; _base_value._uint = value; }
	Value (int64_t value) { _value_type = JT_INT; _base_value._uint = value; }
	Value (u_int64_t value) { _value_type = JT_UINT; _base_value._uint = value; }
	Value (bool value) { _value_type = JT_BOOLEAN; _base_value._bool = value; }
	Value (const JCD_STRING &value) { _value_type = JT_STRING; _value_string = value; }
	Value (const JCD_CHAR *value) { _value_type = JT_STRING; _value_string = value; }
	Value (double value) { _value_type = JT_DOUBLE; _base_value._double = value; }
	Value (const Value &other) {
		_value_type = other._value_type;
		switch (_value_type) {
		case JT_NULL:
		case JT_INT:
		case JT_UINT:
		case JT_BOOLEAN:
		case JT_DOUBLE:
			_base_value = other._base_value;
			break;
		case JT_STRING:
			_value_string = other._value_string;
			break;
		case JT_OBJECT:
			_value_object.insert(other._value_object.begin(), other._value_object.end());
			break;
		case JT_ARRAY:
			_value_array.assign(other._value_array.begin(), other._value_array.end());
			break;
		}
	}
	~Value () {
		clear ();
	}
	Value& operator=(const Value &other) {
		_value_type = other._value_type;
		switch (_value_type) {
		case JT_NULL:
		case JT_INT:
		case JT_UINT:
		case JT_BOOLEAN:
		case JT_DOUBLE:
			_base_value = other._base_value;
			break;
		case JT_STRING:
			_value_string = other._value_string;
			break;
		case JT_OBJECT:
			_value_object.clear();
			_value_object.insert(other._value_object.begin(), other._value_object.end());
			break;
		case JT_ARRAY:
			_value_array.clear();
			_value_array.assign(other._value_array.begin(), other._value_array.end());
			break;
		}
		return *this;
	}
	JSON_TYPE type () const { return _value_type; }
	bool isMember (JCD_CONST_CHAR *key) {
		return (_value_object.end() != _value_object.find(key));
	}
	bool isMember (JCD_STRING &key) {
		return (_value_object.end() != _value_object.find(key));
	}
	bool isNull() const { return JT_NULL == _value_type; }
	bool isBoolean() const { return JT_BOOLEAN == _value_type; }
  bool isInterger () const { return (JT_INT == _value_type || JT_UINT == _value_type); }
  bool isInt() const { return isInterger(); }
	//bool isUInt() const { return isInterger(); }
	bool isDouble() const { return JT_DOUBLE == _value_type; }
	bool isNumeric() const { return (JT_INT == _value_type || JT_UINT == _value_type || JT_DOUBLE == _value_type); }
	bool isString() const { return JT_STRING == _value_type; }
	bool isArray() const { return JT_ARRAY == _value_type; }
	bool isObject() const { return JT_OBJECT == _value_type; }

	JCD_STRING asString () const {
		switch (_value_type) {
		case JT_NULL:
			return JCD_T("");
		case JT_STRING:
			return _value_string;
		default:
			JSON_ASSERT_MESSAGE (false, "Type can not convert to string.");
		}
		return JCD_T("");
	}
	int64_t asInt () const {
		switch (_value_type) {
		case JT_NULL:
			return 0;
		case JT_INT:
		case JT_UINT:
			return int64_t(_base_value._uint);
		case JT_DOUBLE:
			return int64_t(_base_value._double);
		case JT_BOOLEAN:
			return (_base_value._bool)? 1 : 0;
		default:
			JSON_ASSERT_MESSAGE (false, "Type can not convert to int.");
		}
		return 0;
	}
	u_int64_t asUInt () const {
		switch (_value_type) {
		case JT_NULL:
			return 0;
		case JT_INT:
		case JT_UINT:
			return _base_value._uint;
		case JT_DOUBLE:
			return u_int64_t(_base_value._double);
		case JT_BOOLEAN:
			return (_base_value._bool)? 1 : 0;
		default:
			JSON_ASSERT_MESSAGE (false, "Type can not convert to int.");
		}
		return 0;
	}
	double asDouble () const {
		switch (_value_type) {
		case JT_NULL:
			return 0.0;
		case JT_INT:
		case JT_UINT:
			return (double)((int64_t)(_base_value._uint));
		case JT_DOUBLE:
			return _base_value._double;
		case JT_BOOLEAN:
			return (_base_value._bool) ? 1.0 : 0.0;
		default:
			JSON_ASSERT_MESSAGE (false, "Type can not convert to double.");
		}
		return 0.0;
	}
	bool asBool () const {
		switch (_value_type) {
		case JT_NULL:
			return false;
		case JT_INT:
		case JT_UINT:
			return (0 != _base_value._uint);
		case JT_DOUBLE:
			return (0.0 != _base_value._double);
		case JT_BOOLEAN:
			return _base_value._bool;
		case JT_STRING:
			return !_value_string.empty();
		case JT_OBJECT:
			return (0 != _value_object.size());
		case JT_ARRAY:
			return (0 != _value_array.size());
		}
		return false;
	}
	void clear () {
		if (JT_OBJECT == _value_type) _value_object.clear();
		else if (JT_ARRAY == _value_type) _value_array.clear();
	}
	void push_back (const Value& v) {
		JSON_ASSERT (JT_NULL == _value_type || JT_ARRAY == _value_type);
		if (JT_NULL == _value_type) *this = Value (JT_ARRAY);
		_value_array.push_back(v);
	}
	u_int32_t size () {
		JSON_ASSERT (JT_NULL == _value_type || JT_ARRAY == _value_type || JT_OBJECT == _value_type);
		if (JT_NULL == _value_type) return 0;
		else if (JT_OBJECT == _value_type) return (u_int32_t)_value_object.size();
		else if (JT_ARRAY == _value_type) return (u_int32_t)_value_array.size();
		return 0;
	}
	bool empty () {
		JSON_ASSERT (JT_NULL == _value_type || JT_ARRAY == _value_type || JT_OBJECT == _value_type);
		if (JT_NULL == _value_type) return true;
		else if (JT_OBJECT == _value_type) return _value_object.empty ();
		else if (JT_ARRAY == _value_type) return _value_array.empty();
		return true;
	}
	Value& operator[] (u_int32_t index) {
		JSON_ASSERT (JT_NULL == _value_type || JT_ARRAY == _value_type);
		if (JT_NULL == _value_type) *this = Value (JT_ARRAY);
		if (index < _value_array.size()) return _value_array[index];
		else {
			u_int32_t fill_count = index - (u_int32_t)_value_array.size();
			Value null_value;
			for (u_int32_t i = 0; i < fill_count; ++i)
				_value_array.push_back(null_value);
			_value_array.push_back(null_value);
			return _value_array.back ();
		}
	}
	Value& operator[] (const JCD_CHAR *key) {
		JSON_ASSERT(JT_NULL == _value_type || JT_OBJECT == _value_type);
		if (JT_NULL == _value_type) *this = Value (JT_OBJECT);
		ObjectValues::iterator it = _value_object.find(key);
		if (_value_object.end() == it) {
			Value null_value;
			ObjectValues::value_type defaultValue (key, null_value);
			it = _value_object.insert(it, defaultValue);
			Value &v = (*it).second;
			return v;
		} else return (*it).second;
	}
	Value& operator[] (const JCD_STRING &key) {
		return (*this)[key.c_str()];
	}
	JCD_STRING toCompatString () {
		JCD_STRING str;
		_indents = 0;
		_toCompatString(*this, str);
		return str;
	}
	JCD_STRING toStyledString () {
		JCD_STRING str;
		_indents = 0;
		_toCompatString(*this, str, true);
		return str;
	}
	Value removeMember (const JCD_CHAR *key) {
		ObjectValues::iterator it;
		Value v;
		if (JT_OBJECT != _value_type) return JT_NULL;
		it = _value_object.find(key);
		if (_value_object.end() != it) {
			v = it->second;
			_value_object.erase(it);
            return v;
		} else return JT_NULL;
	}
	Value removeMember (const JCD_STRING &key) {
		return removeMember(key.c_str());
	}
private:
	union ValueHolder {
		u_int64_t _uint;
		double _double;
		bool _bool;
	} _base_value;
	JCD_STRING _value_string;
	ObjectValues _value_object;
	ArrayValues _value_array;
	JSON_TYPE _value_type;
	int _indents;

	void _inc_idents () { _indents += 2; }
	void _dec_idents () { _indents -= 2; if (_indents < 0) _indents = 0; }

	void _fill_indents (JCD_STRING &str) {
		str += JCD_T("\n");
		if (_indents > 0) str.append(_indents, JCD_T(' '));
	}
	void _toCompatString (Value &node, JCD_STRING &str, bool bStyled = false) {
		ObjectValues::iterator it;
		ArrayValues::iterator array_it;
		JCD_CHAR number[40] = {0};

		switch (node.type()) {
		case JT_OBJECT:
			str += JCD_T ("{"); 
			if (true == bStyled) { _inc_idents(); _fill_indents(str); }
			for (it = node._value_object.begin(); it != node._value_object.end(); ++it) {
				if (it != node._value_object.begin()) {
					str += JCD_T(",");
					if (true == bStyled) _fill_indents(str);
				}
				str += (JCD_T("\"") + it->first + JCD_T("\":"));
				_toCompatString(it->second, str, bStyled);
			}
			if (true == bStyled) { _dec_idents(); _fill_indents(str); }
			str += JCD_T ("}");
			break;
		case JT_ARRAY:
			str += JCD_T("[");
			if (true == bStyled) { _inc_idents(); _fill_indents(str); }
			for (array_it = node._value_array.begin(); array_it != node._value_array.end(); ++array_it) {
				if (array_it != node._value_array.begin()) {
					str += JCD_T(",");
					if (true == bStyled) _fill_indents(str);
				}
				_toCompatString((*array_it), str, bStyled);
			}
			if (true == bStyled) { _dec_idents(); _fill_indents(str); }
			str += JCD_T("]");
			break;
		case JT_NULL:
			str += JCD_T("null");
			break;
		case JT_INT:
			JCD_SPRINTF(number, JCD_T("%lld"), node.asInt());
			str += number;
			break;
		case JT_UINT:
			JCD_SPRINTF(number, JCD_T("%llu"), node.asUInt());
			str += number;
			break;
		case JT_DOUBLE:
			JCD_SPRINTF(number, JCD_T("%lf"), node.asDouble());
			str += number;
			break;
		case JT_BOOLEAN:
			if (node.asBool()) str += JCD_T("true");
			else str += JCD_T("false");
			break;
		case JT_STRING:
			str += (JCD_T("\"") + _process_esc (node.asString()) + JCD_T("\""));
			break;
		}
	}
	JCD_STRING _process_esc (const JCD_STRING& str) {
        JCD_STRING big = str;
        _replace_string(big, JCD_T("\\"), JCD_T("\\\\"));
        _replace_string(big, JCD_T("\\\\u"), JCD_T("\\u"));
        _replace_string(big, JCD_T("\""), JCD_T("\\\""));
        _replace_string(big, JCD_T("\b"), JCD_T("\\b"));
        _replace_string(big, JCD_T("\f"), JCD_T("\\f"));
        _replace_string(big, JCD_T("\n"), JCD_T("\\n"));
        _replace_string(big, JCD_T("\r"), JCD_T("\\r"));
        _replace_string(big, JCD_T("\t"), JCD_T("\\t"));
        _replace_string(big, JCD_T("\a"), JCD_T("\\a"));
        _replace_string(big, JCD_T("\v"), JCD_T("\\v"));
        return big;
	}
	void _replace_string (JCD_STRING& strBig, const JCD_STRING& strsrc, const JCD_STRING& strdst) {
		JCD_STRING::size_type pos = 0;
		while((pos = strBig.find(strsrc, pos)) != std::string::npos) {
			strBig.replace(pos, strsrc.length(), strdst);
			pos += strdst.length();
		}
	}
};

class Parser {
public:
	Parser () {
		_start = _end = 0;
	}
	bool load_string (const JCD_STRING &content, Value& root_node) {
		_start = 0;
		_exception_string = JCD_T ("");
		_end = (unsigned int)content.length ();
		_content = content;
		root_node.clear();

		if (false == match(JCD_T('{'))) {
			_exception_string = JCD_T ("Expecting '{' for root object.");
			return false;
		}
		bool r = match_object(root_node);
    return r;
	}
	void get_error (int &pos, JCD_STRING &err) {
		pos = _start;
		err = _exception_string;
	}
private:
	JCD_CHAR process_esc (JCD_CHAR ch) {
		switch (ch) {
		case JCD_T ('\"'):
			return JCD_T ('\"');
		case JCD_T ('\\'):
			return JCD_T ('\\');
		case JCD_T ('/'):
			return JCD_T ('/');
		case JCD_T('b'):
			return JCD_T ('\b');
		case JCD_T ('f'):
			return JCD_T ('\f');
		case JCD_T ('n'):
			return JCD_T ('\n');
		case JCD_T('r'):
			return JCD_T ('\r');
		case JCD_T ('t'):
			return JCD_T ('\t');
		case JCD_T ('\''):
			return JCD_T ('\'');
		case JCD_T('a'):
			return JCD_T ('\a');
		case JCD_T ('v'):
			return JCD_T ('\v');
		case JCD_T('?'):
			return JCD_T ('?');
		default:
			return JCD_T (' ');
		}
	}
	bool read_next (JCD_CHAR &ch, bool forward=true) {
		if (_start >= _end) return false;
		ch = _content.at (_start);
		if (true == forward) ++_start;
		return true;
	}
	void backward () { --_start; }
	void forward () { ++_start; }
	void skip_spaces () {
		JCD_CHAR ch;
		while (true == read_next(ch)) {
			if (JCD_T(' ') == ch || JCD_T('\n') == ch || JCD_T('\t') == ch || JCD_T('\r') == ch)
				continue;
			else { backward (); break; }
		}
	}
	bool match (JCD_CHAR ch) {
		JCD_CHAR comp_ch;
		skip_spaces();
		if (false == read_next(comp_ch)) return false;
		if (ch != comp_ch) { backward (); return false; }

		return true;
	}
	bool match_string (JCD_STRING &str) {
		JCD_CHAR ch;
		while (true) {
			if (false == read_next(ch)) {
				_exception_string = JCD_T ("Expecting '\"' for string.");
				return false;
			}
			if (JCD_T('\\') == ch) {
				if (false == read_next (ch)) {
					_exception_string = JCD_T ("Invalid escape code.");
					return false;
				}
                if ('u' == ch) { // unicode string
                  str += "\\u";
                } else {
                  ch = process_esc(ch);
                  if (JCD_T (' ') == ch) {
                      _exception_string = JCD_T ("Invalid escape code.");
                      return false;
                  }
                  str += ch;
                }
			} else if (JCD_T ('\"') != ch) str += ch;
			else break;
		}
		return true;
	}
	bool check_digit (JCD_STRING &str, Value::JSON_TYPE &type) {
		int pos = 0;
		size_t i;
		bool first_dot = true;
		JCD_CHAR ch;
		ch = str.at (pos);
		type = Value::JT_INT;
		if (JCD_T('-') == ch) ++pos;
		for (i = pos; i < str.length(); ++i) {
			ch = str.at (i);
			if ((JCD_T('0') <= ch && JCD_T('9') >= ch)) continue;
      if ((JCD_T('E') == ch || JCD_T('e') == ch)) continue; //1.000002562698995e7  1.000002562698995E7
			if ((JCD_T('.') == ch)) {
				if (true == first_dot) {
					first_dot = false;
					type = Value::JT_DOUBLE;
				} else {
					_exception_string = JCD_T ("Multiable dot in number.");
					return false;
				}
			} else {
				_exception_string = JCD_T ("Expecting number ('+' '-' '0~9' '.' 'E' 'e').");
				return false;
			}
		}
		return true;
	}
	bool match_digit (JCD_STRING &str) {
		JCD_CHAR ch;
		while (true) {
			if (false == read_next(ch)) {
				_exception_string = JCD_T ("unterminated number object.");
				return false;
			}
			if ((JCD_T('0') <= ch && JCD_T('9') >= ch) || 
				JCD_T('.') == ch ||
				JCD_T('-') == ch || JCD_T('+') == ch ||
        JCD_T('E') == ch || JCD_T('e') == ch) 
				str += ch; 
			else { backward (); break; }
		}
		if (JCD_T('+') == str.at(0)) str.erase(0, 1);
		if (JCD_T('.') == str.at(0)) str.insert(0, JCD_T("0"));

		return true;
	}
	bool match_boolean (bool &value) {
		JCD_CHAR ch;
		JCD_STRING str;
		while (true) {
			if (false == read_next(ch)) {
				_exception_string = JCD_T ("unterminated boolean object.");
				return false;
			}
			if (JCD_T('t') == ch || JCD_T('r') == ch || 
				JCD_T('u') == ch || JCD_T('e') == ch ||
				JCD_T('f') == ch || JCD_T('a') == ch ||
				JCD_T('l') == ch || JCD_T('s') == ch) 
				str += ch; 
			else { backward (); break; }
		}
		if (JCD_T("true") == str) value = true;
		else if (JCD_T("false" == str)) value = false;
		else {
			_exception_string = JCD_T ("unrecognizable boolean object.");
			return false;
		}
		return true;
	}
	bool match_null () {
		JCD_CHAR ch;
		JCD_STRING str;
		while (true) {
			if (false == read_next(ch)) {
				_exception_string = JCD_T ("unterminated object.");
				return false;
			}
			if (JCD_T('n') == ch || JCD_T('u') == ch || JCD_T('l') == ch) 
				str += ch; 
			else { backward (); break; }
		}
		if (JCD_T("null") == str) return true;
		else {
			_exception_string = JCD_T ("unrecognizable null object.");
			return false;
		}
	}
	bool array_value (Value &node) {
		if (false == match_value(node)) return false;
		if (false == _array_value(node)) return false;
		return true;
	}
	bool _array_value (Value &node) {
		if (false == match (JCD_T(','))) return true;
		if (false == match_value(node)) return false;
		return _array_value(node);
	}
	bool match_array (Value& node) {
		node._value_type = Value::JT_ARRAY;
		if (false == array_value(node)) return false;
		if (false == match (JCD_T(']'))) {
			_exception_string = JCD_T ("Expecting ']' for array");
			return false;
		}
		return true;
	}
	bool match_value (Value& node) {
		if (true == match (JCD_T('{')))
			return match_object(node);
		else if (true == match (JCD_T('['))) return match_array (node);
		else if (true == match (JCD_T('\"'))) {
			JCD_STRING value;
			if (false == match_string (value)) return false;
			if (Value::JT_ARRAY == node.type()) node.push_back(value);
			else node = value;
		} else {
			JCD_CHAR ch;
			JCD_STRING value;
			bool bool_value;
			if (false == read_next(ch, false)) return false;
			if (JCD_T('t') == ch || JCD_T('f') == ch) {
				if (false == match_boolean (bool_value)) return false;
				if (Value::JT_ARRAY == node.type()) node.push_back(bool_value);
				else node = bool_value;
			} else if (JCD_T('n') == ch) {
				if (false == match_null()) return false;
				Value null_value;
				if (Value::JT_ARRAY == node.type()) node.push_back(null_value);
				else node = null_value;
			} else if ((JCD_T('0') <= ch && JCD_T('9') >= ch) || 
				JCD_T('.') == ch ||
				JCD_T('-') == ch || JCD_T('+') == ch || 
        JCD_T('E') == ch || JCD_T('e') == ch) {
				Value::JSON_TYPE jt;
				if (false == match_digit(value)) return false;
				if (false == check_digit(value, jt)) return false;
				if (Value::JT_INT == jt) {
					if (Value::JT_ARRAY == node.type()) node.push_back((int64_t)JCD_STRTOLL(value.c_str()));
					else node = (int64_t)JCD_STRTOLL(value.c_str());
				} else {
					if (Value::JT_ARRAY == node.type ()) node.push_back(JCD_ATOF (value.c_str()));
					else node = JCD_ATOF (value.c_str()); 
				}
			}
		}
		return true;
	}
	bool _key_pair (Value& node) {
		if (false == match (JCD_T(','))) return true;
		if (false == key_pair(node)) return false;
		if (false == _key_pair(node)) return false;
		return true;
	}
	bool key_pair (Value& node) {
		JCD_STRING name;
		Value curNode;
		if (false == match (JCD_T('\"'))) {
			_exception_string = JCD_T ("Expecting '\"' for key.");
			return false;
		}
		if (false == match_string (name)) return false;
		if (false == match (JCD_T(':'))) {
			_exception_string = JCD_T ("Expecting ':'");
			return false;
		}
		if (false == match_value(curNode)) return false;
		node[name] = curNode;
		return true;
	}
	bool match_key_pair (Value& node) {
		if (false == key_pair (node)) return false;
		if (false == _key_pair (node)) return false;
		return true;
	}
	bool match_object (Value& node) {
		Value noname_obj (Value::JT_OBJECT);
    if (true == match(JCD_T('}'))) {
      if (Value::JT_ARRAY == node.type()) node.push_back(noname_obj);
      return true;
    }
		if (false == match_key_pair ((Value::JT_ARRAY == node.type ()) ? noname_obj : node)) return false;
		if (false == match(JCD_T('}'))) {
			_exception_string = JCD_T ("unterminated object, Expecting '}'.");
			return false;
		}
		if (Value::JT_ARRAY == node.type()) node.push_back(noname_obj);
		return true;
	}

	unsigned int _start, _end;
	JCD_STRING _content;
	JCD_STRING _exception_string;
};


}
