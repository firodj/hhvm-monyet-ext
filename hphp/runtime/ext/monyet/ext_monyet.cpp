#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/ext/array/ext_array.h"
#include "hphp/runtime/base/builtin-functions.h"
#include "hphp/runtime/base/collections.h"
#include "hphp/runtime/base/comparisons.h"
#include "hphp/runtime/base/container-functions.h"
#include "hphp/runtime/base/mixed-array.h"
#include "hphp/runtime/base/intercept.h"
#include "hphp/runtime/base/php-globals.h"
#include "hphp/runtime/base/hphp-system.h"
#include "hphp/runtime/ext/std/ext_std_errorfunc.h"
#include "hphp/runtime/ext/string/ext_string.h"
#include "hphp/runtime/ext/std/ext_std_variable.h"
#include "hphp/util/logger.h"

#include <vector>

namespace HPHP {
/////////////////////////////////////////////////////////////////////////////

const StaticString
    s_compact("compact"),
    s_obs_compact("obs_compact"),
    s_my_compact("my_compact"),

    s_compact_sl("__SystemLib\\compact_sl"), // Native
    s_my_compact_sl("my_compact_sl"),
    s_obs_compact_sl("obs_compact_sl"),

    s_extract("extract"),
    s_my_extract("my_extract"),
    s_obs_extract("obs_extract"),

    s_extract_sl("__SystemLib\\extract"), // Native
    s_my_extract_sl("my_extract_sl"),
    s_obs_extract_sl("obs_extract_sl"),

    s_comma(", "),
    s_squote("'"),
    s_file("file"),
    s_line("line"),
    s_qmark("?");


// declaration of:
// hphp/runtime/ext/std/ext_std_variable.cpp
int64_t HHVM_FUNCTION(extract, VRefParam vref_array,
                      int64_t extract_type /* = EXTR_OVERWRITE */,
                      const String& prefix /* = "" */);

int64_t HHVM_FUNCTION(SystemLib_extract,
                      VRefParam vref_array,
                      int64_t extract_type = EXTR_OVERWRITE,
                      const String& prefix = "");

static void dump_compact_arg(const Variant& var, Array& ary) {
  if (var.isArray()) {
    for (ArrayIter iter(var.getArrayData()); iter; ++iter) {
      dump_compact_arg(iter.second(), ary);
    }
  } else {
    String varname = s_squote + var.toString() + s_squote;
    ary.append( varname );
  }
}

String dump_compact(const Variant &varname, const Array& args)
{
  Array ary;

  dump_compact_arg(varname, ary);
  dump_compact_arg(args, ary);

  return HHVM_FN(implode)(s_comma, ary);
}

String dump_extract(VRefParam vref_array) {
  auto arr_tv = vref_array.wrapped().asTypedValue();
  if (arr_tv->m_type == KindOfRef) {
    arr_tv = arr_tv->m_data.pref->tv();
  }
  if (!isArrayType(arr_tv->m_type)) return s_qmark;

  auto& carr = tvAsCVarRef(arr_tv).asCArrRef();
  Array ary;

  for (ArrayIter iter(carr); iter; ++iter) {
    String varname = s_squote + iter.first().toString() + s_squote;
    ary.append( varname );
  }

  return HHVM_FN(implode)(s_comma, ary);
}

String dump_back_trace() {
  StringBuffer buf;
  Array bt = HHVM_FN(debug_backtrace)(k_DEBUG_BACKTRACE_PROVIDE_OBJECT, 1);

  for (ArrayIter it = bt.begin(); !it.end(); it.next()) {
    Array frame = it.second().toArray();

    if (frame.exists(s_file)) {
      buf.append(frame->get(s_file).toString());
      buf.append(':');
      buf.append(frame->get(s_line).toString());
    }
  }

  return buf.detach();
}

Array HHVM_FUNCTION(my_compact,
                    const Variant& varname,
                    const Array& args /* = null array */) {
  Array ret = HHVM_FN(compact)(varname, args);

  Logger::Info("compact(%s) in [%s]", dump_compact(varname, args).c_str(), dump_back_trace().c_str());

  return ret;
}

Array HHVM_FUNCTION(my_compact_sl,
                    const Variant& varname,
                    const Array& args /* = null array */) {
  Array ret = HHVM_FN(__SystemLib_compact_sl)(varname, args);

  Logger::Info("compact(%s) in [%s]", dump_compact(varname, args).c_str(), dump_back_trace().c_str());

  return ret;
}

void HHVM_FUNCTION(compact_intercept) {
    Logger::Info("compact intercepted!");
    rename_function(s_compact, s_obs_compact);
    rename_function(s_my_compact, s_compact);
    rename_function(s_compact_sl, s_obs_compact_sl);
    rename_function(s_my_compact_sl, s_compact_sl);
}

int64_t HHVM_FUNCTION(my_extract,
                      VRefParam vref_array,
                      int64_t extract_type /* = EXTR_OVERWRITE */,
                      const String& prefix /* = "" */) {
  int64_t ret = HHVM_FN(extract)(vref_array, extract_type, prefix);

  Logger::Info("extract(%s) in [%s]", dump_extract(vref_array).c_str(), dump_back_trace().c_str());

  return ret;
}

int64_t HHVM_FUNCTION(my_extract_sl,
                      VRefParam vref_array,
                      int64_t extract_type = EXTR_OVERWRITE,
                      const String& prefix = "") {

  int64_t ret = HHVM_FN(SystemLib_extract)(vref_array, extract_type, prefix);

  Logger::Info("extract(%s) in [%s]", dump_extract(vref_array).c_str(), dump_back_trace().c_str());

  return ret;
}

void HHVM_FUNCTION(extract_intercept) {
    Logger::Info("extract intercept!");
    rename_function(s_extract, s_obs_extract);
    rename_function(s_my_extract, s_extract);
    rename_function(s_extract_sl, s_obs_extract_sl);
    rename_function(s_my_extract_sl, s_extract_sl);
}

class MonyetExtension : public Extension {
 public:
  MonyetExtension(): Extension("monyet", "1.0") {}

  void moduleInit() override {

    HHVM_FE(compact_intercept);
    HHVM_FE(my_compact);
    HHVM_FE(my_compact_sl);

    HHVM_FE(extract_intercept);
    HHVM_FE(my_extract);
    HHVM_FE(my_extract_sl);

    loadSystemlib();
  }
} s_monyet_extension;

HHVM_GET_MODULE(monyet);

/////////////////////////////////////////////////////////////////////////////
} // namespace HPHP
