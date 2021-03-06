// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#ifndef CEPH_FORMATTER_H
#define CEPH_FORMATTER_H

#include "include/int_types.h"

#include <deque>
#include <iosfwd>
#include <list>
#include <vector>
#include <sstream>
#include <stdarg.h>
#include <string>
#include <map>

#include "include/buffer.h"

namespace ceph {

  struct FormatterAttrs {
    std::list< std::pair<std::string, std::string> > attrs;

    FormatterAttrs(const char *attr, ...);
  };

  class Formatter {
  public:
    static Formatter *create(const std::string& type,
			     const std::string& default_type,
			     const std::string& fallback);
    static Formatter *create(const std::string& type,
			     const std::string& default_type) {
      return create(type, default_type, "");
    }
    static Formatter *create(const std::string& type) {
      return create(type, "json-pretty", "");
    }

    Formatter();
    virtual ~Formatter();

    virtual void flush(std::ostream& os) = 0;
    void flush(bufferlist &bl)
    {
      std::stringstream os;
      flush(os);
      bl.append(os.str());
    }
    virtual void reset() = 0;

    virtual void open_array_section(const char *name) = 0;
    virtual void open_array_section_in_ns(const char *name, const char *ns) = 0;
    virtual void open_object_section(const char *name) = 0;
    virtual void open_object_section_in_ns(const char *name, const char *ns) = 0;
    virtual void close_section() = 0;
    virtual void dump_unsigned(const char *name, uint64_t u) = 0;
    virtual void dump_int(const char *name, int64_t s) = 0;
    virtual void dump_float(const char *name, double d) = 0;
    virtual void dump_string(const char *name, const std::string& s) = 0;
    virtual void dump_bool(const char *name, bool b)
    {
      dump_format_unquoted(name, "%s", (b ? "true" : "false"));
    }
    virtual std::ostream& dump_stream(const char *name) = 0;
    virtual void dump_format_va(const char *name, const char *ns, bool quoted, const char *fmt, va_list ap) = 0;
    virtual void dump_format(const char *name, const char *fmt, ...);
    virtual void dump_format_ns(const char *name, const char *ns, const char *fmt, ...);
    virtual void dump_format_unquoted(const char *name, const char *fmt, ...);
    virtual int get_len() const = 0;
    virtual void write_raw_data(const char *data) = 0;
    /* with attrs */
    virtual void open_array_section_with_attrs(const char *name, const FormatterAttrs& attrs)
    {
      open_array_section(name);
    }
    virtual void open_object_section_with_attrs(const char *name, const FormatterAttrs& attrs)
    {
      open_object_section(name);
    }
    virtual void dump_string_with_attrs(const char *name, const std::string& s, const FormatterAttrs& attrs)
    {
      dump_string(name, s);
    }
  };

  class JSONFormatter : public Formatter {
  public:
    JSONFormatter(bool p = false);

    void flush(std::ostream& os);
    void reset();
    virtual void open_array_section(const char *name);
    void open_array_section_in_ns(const char *name, const char *ns);
    void open_object_section(const char *name);
    void open_object_section_in_ns(const char *name, const char *ns);
    void close_section();
    void dump_unsigned(const char *name, uint64_t u);
    void dump_int(const char *name, int64_t u);
    void dump_float(const char *name, double d);
    void dump_string(const char *name, const std::string& s);
    std::ostream& dump_stream(const char *name);
    void dump_format_va(const char *name, const char *ns, bool quoted, const char *fmt, va_list ap);
    int get_len() const;
    void write_raw_data(const char *data);

  private:

    struct json_formatter_stack_entry_d {
      int size;
      bool is_array;
      json_formatter_stack_entry_d() : size(0), is_array(false) { }
    };

    bool m_pretty;
    void open_section(const char *name, bool is_array);
    void print_quoted_string(const std::string& s);
    void print_name(const char *name);
    void print_comma(json_formatter_stack_entry_d& entry);
    void finish_pending_string();

    std::stringstream m_ss, m_pending_string;
    std::list<json_formatter_stack_entry_d> m_stack;
    bool m_is_pending_string;
  };

  class XMLFormatter : public Formatter {
  public:
    static const char *XML_1_DTD;
    XMLFormatter(bool pretty = false);

    void flush(std::ostream& os);
    void reset();
    void open_array_section(const char *name);
    void open_array_section_in_ns(const char *name, const char *ns);
    void open_object_section(const char *name);
    void open_object_section_in_ns(const char *name, const char *ns);
    void close_section();
    void dump_unsigned(const char *name, uint64_t u);
    void dump_int(const char *name, int64_t u);
    void dump_float(const char *name, double d);
    void dump_string(const char *name, const std::string& s);
    std::ostream& dump_stream(const char *name);
    void dump_format_va(const char *name, const char *ns, bool quoted, const char *fmt, va_list ap);
    int get_len() const;
    void write_raw_data(const char *data);

    /* with attrs */
    void open_array_section_with_attrs(const char *name, const FormatterAttrs& attrs);
    void open_object_section_with_attrs(const char *name, const FormatterAttrs& attrs);
    void dump_string_with_attrs(const char *name, const std::string& s, const FormatterAttrs& attrs);
  private:
    void open_section_in_ns(const char *name, const char *ns, const FormatterAttrs *attrs);
    void finish_pending_string();
    void print_spaces();
    static std::string escape_xml_str(const char *str);
    void get_attrs_str(const FormatterAttrs *attrs, std::string& attrs_str);

    std::stringstream m_ss, m_pending_string;
    std::deque<std::string> m_sections;
    bool m_pretty;
    std::string m_pending_string_name;
  };

  class TableFormatter : public Formatter {
  public:
    TableFormatter(bool keyval = false);

    void flush(std::ostream& os);
    void reset();
    virtual void open_array_section(const char *name);
    void open_array_section_in_ns(const char *name, const char *ns);
    void open_object_section(const char *name);
    void open_object_section_in_ns(const char *name, const char *ns);

    void open_array_section_with_attrs(const char *name, const FormatterAttrs& attrs);
    void open_object_section_with_attrs(const char *name, const FormatterAttrs& attrs);

    void close_section();
    void dump_unsigned(const char *name, uint64_t u);
    void dump_int(const char *name, int64_t u);
    void dump_float(const char *name, double d);
    void dump_string(const char *name, const std::string& s);
    void dump_format_va(const char *name, const char *ns, bool quoted, const char *fmt, va_list ap);
    void dump_string_with_attrs(const char *name, const std::string& s, const FormatterAttrs& attrs);
    std::ostream& dump_stream(const char *name);

    int get_len() const;
    void write_raw_data(const char *data);
    void get_attrs_str(const FormatterAttrs *attrs, std::string& attrs_str);

  private:
    void open_section_in_ns(const char *name, const char *ns, const FormatterAttrs *attrs);
    std::vector< std::vector<std::pair<std::string, std::string> > > m_vec;
    std::stringstream m_ss;
    size_t m_vec_index(const char* name);
    std::string get_section_name(const char* name);
    void finish_pending_string();
    std::string m_pending_name;
    bool m_keyval;

    int m_section_open;
    std::vector< std::string > m_section;
    std::map<std::string, int> m_section_cnt;
    std::vector<size_t> m_column_size;
    std::vector< std::string > m_column_name;
  };


}
#endif
