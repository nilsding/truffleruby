/*
 * Copyright (c) 2016, 2017 Oracle and/or its affiliates. All rights reserved. This
 * code is released under a tri EPL/GPL/LGPL license. You can use it,
 * redistribute it and/or modify it under the terms of the:
 *
 * Eclipse Public License version 1.0
 * GNU General Public License version 2
 * GNU Lesser General Public License version 2.1
 *
 * This file contains code that is based on the Ruby API headers and implementation,
 * copyright (C) Yukihiro Matsumoto, licensed under the 2-clause BSD licence
 * as described in the file BSDL included with TruffleRuby.
 */

#include <ruby.h>
#include <ruby/debug.h>
#include <ruby/encoding.h>
#include <ruby/io.h>
#include <ruby/thread_native.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

// Private helper macros just for ruby.c

#define rb_boolean(c) ((c) ? Qtrue : Qfalse)

// Helpers

VALUE rb_f_notimplement(int args_count, const VALUE *args, VALUE object) {
  rb_tr_error("rb_f_notimplement");
}

// Memory

void ruby_malloc_size_overflow(size_t count, size_t elsize) {
  rb_raise(rb_eArgError,
     "malloc: possible integer overflow (%"PRIdSIZE"*%"PRIdSIZE")",
     count, elsize);
}

size_t xmalloc2_size(const size_t count, const size_t elsize) {
  size_t ret;
  if (rb_mul_size_overflow(count, elsize, SSIZE_MAX, &ret)) {
    ruby_malloc_size_overflow(count, elsize);
  }
  return ret;
}

void *ruby_xmalloc(size_t size) {
  return malloc(size);
}

void *ruby_xmalloc2(size_t n, size_t size) {
  return malloc(xmalloc2_size(n, size));
}

void *ruby_xcalloc(size_t n, size_t size) {
  return calloc(n, size);
}

void *ruby_xrealloc(void *ptr, size_t new_size) {
  return realloc(ptr, new_size);
}

void *ruby_xrealloc2(void *ptr, size_t n, size_t size) {
  size_t len = size * n;
  if (n != 0 && size != len / n) {
    rb_raise(rb_eArgError, "realloc: possible integer overflow");
  }
  return realloc(ptr, len);
}

void ruby_xfree(void *address) {
  free(address);
}

void *rb_alloc_tmp_buffer(volatile VALUE *buffer_pointer, long length) {
  // TODO CS 13-Apr-17 MRI sometimes uses alloc and sometimes malloc, and wraps it in a Ruby object - is rb_free_tmp_buffer guaranteed to be called or do we need to free in a finalizer?
  void *space = malloc(length);
  *((void**) buffer_pointer) = space;
  return space;
}

void rb_free_tmp_buffer(volatile VALUE *buffer_pointer) {
  free(*((void**) buffer_pointer));
}

// Types

int rb_type(VALUE value) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_type", value));
}

bool RB_TYPE_P(VALUE value, int type) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "RB_TYPE_P", value, type));
}

void rb_check_type(VALUE value, int type) {
  polyglot_invoke(RUBY_CEXT, "rb_check_type", value, type);
}

VALUE rb_obj_is_instance_of(VALUE object, VALUE ruby_class) {
  return polyglot_invoke(RUBY_CEXT, "rb_obj_is_instance_of", object, ruby_class);
}

VALUE rb_obj_is_kind_of(VALUE object, VALUE ruby_class) {
  return polyglot_invoke(RUBY_CEXT, "rb_obj_is_kind_of", object, ruby_class);
}

void rb_check_frozen(VALUE object) {
  polyglot_invoke(RUBY_CEXT, "rb_check_frozen", object);
}

void rb_insecure_operation(void) {
  rb_raise(rb_eSecurityError, "Insecure operation: -r");
}

int rb_safe_level(void) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_safe_level"));
}

void rb_set_safe_level_force(int level) {
  polyglot_invoke(RUBY_CEXT, "rb_set_safe_level_force", level);
}

void rb_set_safe_level(int level) {
  polyglot_invoke(RUBY_CEXT, "rb_set_safe_level", level);
}

void rb_check_safe_obj(VALUE object) {
  if (rb_safe_level() > 0 && OBJ_TAINTED(object)) {
    rb_insecure_operation();
  }
}

bool SYMBOL_P(VALUE value) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "SYMBOL_P", value));
}

VALUE rb_obj_hide(VALUE obj) {
  // In MRI, this deletes the class information which is later set by rb_obj_reveal.
  // It also hides the object from each_object, we do not hide it.
  return obj;
}

VALUE rb_obj_reveal(VALUE obj, VALUE klass) {
  // In MRI, this sets the class of the object, we are not deleting the class in rb_obj_hide, so we
  // ensure that class matches.
  return polyglot_invoke(RUBY_CEXT, "ensure_class", obj, klass,
             rb_str_new_cstr("class %s supplied to rb_obj_reveal does not matches the obj's class %s"));
  return obj;
}

// Constants

// START from tool/generate-cext-constants.rb

VALUE rb_tr_get_undef(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "Qundef");
}

VALUE rb_tr_get_true(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "Qtrue");
}

VALUE rb_tr_get_false(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "Qfalse");
}

VALUE rb_tr_get_nil(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "Qnil");
}

VALUE rb_tr_get_Array(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cArray");
}

VALUE rb_tr_get_Bignum(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cBignum");
}

VALUE rb_tr_get_Class(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cClass");
}

VALUE rb_tr_get_Comparable(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mComparable");
}

VALUE rb_tr_get_Data(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cData");
}

VALUE rb_tr_get_Encoding(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cEncoding");
}

VALUE rb_tr_get_Enumerable(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mEnumerable");
}

VALUE rb_tr_get_FalseClass(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cFalseClass");
}

VALUE rb_tr_get_File(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cFile");
}

VALUE rb_tr_get_Fixnum(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cFixnum");
}

VALUE rb_tr_get_Float(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cFloat");
}

VALUE rb_tr_get_Hash(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cHash");
}

VALUE rb_tr_get_Integer(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cInteger");
}

VALUE rb_tr_get_IO(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cIO");
}

VALUE rb_tr_get_Kernel(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mKernel");
}

VALUE rb_tr_get_Match(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cMatch");
}

VALUE rb_tr_get_Module(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cModule");
}

VALUE rb_tr_get_NilClass(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cNilClass");
}

VALUE rb_tr_get_Numeric(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cNumeric");
}

VALUE rb_tr_get_Object(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cObject");
}

VALUE rb_tr_get_Range(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cRange");
}

VALUE rb_tr_get_Regexp(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cRegexp");
}

VALUE rb_tr_get_String(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cString");
}

VALUE rb_tr_get_Struct(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cStruct");
}

VALUE rb_tr_get_Symbol(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cSymbol");
}

VALUE rb_tr_get_Time(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cTime");
}

VALUE rb_tr_get_Thread(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cThread");
}

VALUE rb_tr_get_TrueClass(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cTrueClass");
}

VALUE rb_tr_get_Proc(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cProc");
}

VALUE rb_tr_get_Method(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cMethod");
}

VALUE rb_tr_get_Dir(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cDir");
}

VALUE rb_tr_get_ArgError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eArgError");
}

VALUE rb_tr_get_EOFError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eEOFError");
}

VALUE rb_tr_get_Errno(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mErrno");
}

VALUE rb_tr_get_Exception(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eException");
}

VALUE rb_tr_get_FloatDomainError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eFloatDomainError");
}

VALUE rb_tr_get_IndexError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eIndexError");
}

VALUE rb_tr_get_Interrupt(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eInterrupt");
}

VALUE rb_tr_get_IOError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eIOError");
}

VALUE rb_tr_get_LoadError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eLoadError");
}

VALUE rb_tr_get_LocalJumpError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eLocalJumpError");
}

VALUE rb_tr_get_MathDomainError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eMathDomainError");
}

VALUE rb_tr_get_EncCompatError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eEncCompatError");
}

VALUE rb_tr_get_NameError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eNameError");
}

VALUE rb_tr_get_NoMemError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eNoMemError");
}

VALUE rb_tr_get_NoMethodError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eNoMethodError");
}

VALUE rb_tr_get_NotImpError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eNotImpError");
}

VALUE rb_tr_get_RangeError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eRangeError");
}

VALUE rb_tr_get_RegexpError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eRegexpError");
}

VALUE rb_tr_get_RuntimeError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eRuntimeError");
}

VALUE rb_tr_get_ScriptError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eScriptError");
}

VALUE rb_tr_get_SecurityError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eSecurityError");
}

VALUE rb_tr_get_Signal(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eSignal");
}

VALUE rb_tr_get_StandardError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eStandardError");
}

VALUE rb_tr_get_SyntaxError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eSyntaxError");
}

VALUE rb_tr_get_SystemCallError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eSystemCallError");
}

VALUE rb_tr_get_SystemExit(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eSystemExit");
}

VALUE rb_tr_get_SysStackError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eSysStackError");
}

VALUE rb_tr_get_TypeError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eTypeError");
}

VALUE rb_tr_get_ThreadError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eThreadError");
}

VALUE rb_tr_get_WaitReadable(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mWaitReadable");
}

VALUE rb_tr_get_WaitWritable(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mWaitWritable");
}

VALUE rb_tr_get_ZeroDivError(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eZeroDivError");
}

VALUE rb_tr_get_stdin(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_stdin");
}

VALUE rb_tr_get_stdout(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_stdout");
}

VALUE rb_tr_get_stderr(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_stderr");
}

VALUE rb_tr_get_output_fs(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_output_fs");
}

VALUE rb_tr_get_rs(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_rs");
}

VALUE rb_tr_get_output_rs(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_output_rs");
}

VALUE rb_tr_get_default_rs(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_default_rs");
}

// END from tool/generate-cext-constants.rb

// Conversions

unsigned long rb_num2ulong(VALUE val) {
  return (unsigned long)polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_num2ulong", val));
}

static char *out_of_range_float(char (*pbuf)[24], VALUE val) {
  char *const buf = *pbuf;
  char *s;

  snprintf(buf, sizeof(*pbuf), "%-.10g", RFLOAT_VALUE(val));
  if ((s = strchr(buf, ' ')) != 0) {
    *s = '\0';
  }
  return buf;
}

#define FLOAT_OUT_OF_RANGE(val, type) do { \
    char buf[24]; \
    rb_raise(rb_eRangeError, "float %s out of range of "type, \
       out_of_range_float(&buf, (val))); \
} while (0)

#define LLONG_MIN_MINUS_ONE ((double)LLONG_MIN-1)
#define LLONG_MAX_PLUS_ONE (2*(double)(LLONG_MAX/2+1))
#define LLONG_MIN_MINUS_ONE_IS_LESS_THAN(n) \
  (LLONG_MIN_MINUS_ONE == (double)LLONG_MIN ? \
   LLONG_MIN <= (n): \
   LLONG_MIN_MINUS_ONE < (n))

LONG_LONG rb_num2ll(VALUE val) {
  if (NIL_P(val)) {
    rb_raise(rb_eTypeError, "no implicit conversion from nil");
  }

  if (FIXNUM_P(val)) {
    return (LONG_LONG)FIX2LONG(val);
  } else if (RB_TYPE_P(val, T_FLOAT)) {
    if (RFLOAT_VALUE(val) < LLONG_MAX_PLUS_ONE
        && (LLONG_MIN_MINUS_ONE_IS_LESS_THAN(RFLOAT_VALUE(val)))) {
      return (LONG_LONG)(RFLOAT_VALUE(val));
    } else {
      FLOAT_OUT_OF_RANGE(val, "long long");
    }
  }
  else if (RB_TYPE_P(val, T_BIGNUM)) {
    return rb_big2ll(val);
  } else if (RB_TYPE_P(val, T_STRING)) {
    rb_raise(rb_eTypeError, "no implicit conversion from string");
  } else if (RB_TYPE_P(val, T_TRUE) || RB_TYPE_P(val, T_FALSE)) {
    rb_raise(rb_eTypeError, "no implicit conversion from boolean");
  }
  
  val = rb_to_int(val);
  return NUM2LL(val);
}

short rb_num2short(VALUE value) {
  rb_tr_error("rb_num2ushort not implemented");
}

unsigned short rb_num2ushort(VALUE value) {
  rb_tr_error("rb_num2ushort not implemented");
}

short rb_fix2short(VALUE value) {
  rb_tr_error("rb_num2ushort not implemented");
}

VALUE RB_INT2FIX(long value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "INT2FIX", value);
}

VALUE RB_LONG2FIX(long value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "LONG2FIX", value);
}

long rb_fix2int(VALUE value) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_fix2int", value));
}

unsigned long rb_fix2uint(VALUE value) {
  return polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_fix2uint", value));
}

int rb_long2int(long value) {
  return polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_long2int", value));
}

ID SYM2ID(VALUE value) {
  return (ID) value;
}

VALUE ID2SYM(ID value) {
  return (VALUE) value;
}

int rb_cmpint(VALUE val, VALUE a, VALUE b) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_cmpint", val, a, b));
}

VALUE rb_int2inum(SIGNED_VALUE n) {
  return (VALUE)LONG2NUM(n);
}

VALUE rb_uint2inum(VALUE n) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_ulong2num",
          // Cast otherwise it's considered as truffle object address
          (unsigned long) n);
}

VALUE rb_ll2inum(LONG_LONG n) {
  /* Long and long long are both 64-bits with clang x86-64. */
  return (VALUE)LONG2NUM(n);
}

VALUE rb_ull2inum(unsigned LONG_LONG val) {
  /* Long and long long are both 64-bits with clang x86-64. */
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_ulong2num", val);
}

double rb_num2dbl(VALUE val) {
  return polyglot_as_double(polyglot_invoke(RUBY_CEXT, "rb_num2dbl", val));
}

long rb_num2int(VALUE val) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_num2int", val));
}

unsigned long rb_num2uint(VALUE val) {
  return (unsigned long)polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_num2uint", val));
}

long rb_num2long(VALUE val) {
  return polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_num2long", val));
}

VALUE rb_num_coerce_bin(VALUE x, VALUE y, ID func) {
  return polyglot_invoke(RUBY_CEXT, "rb_num_coerce_bin", x, y, ID2SYM(func));
}

VALUE rb_num_coerce_cmp(VALUE x, VALUE y, ID func) {
  return polyglot_invoke(RUBY_CEXT, "rb_num_coerce_cmp", x, y, ID2SYM(func));
}

VALUE rb_num_coerce_relop(VALUE x, VALUE y, ID func) {
  return polyglot_invoke(RUBY_CEXT, "rb_num_coerce_relop", x, y, ID2SYM(func));
}

void rb_num_zerodiv(void) {
  rb_raise(rb_eZeroDivError, "divided by 0");
}

// Type checks

int RB_NIL_P(VALUE value) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "RB_NIL_P", value));
}

int RB_FIXNUM_P(VALUE value) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "RB_FIXNUM_P", value));
}

int RB_FLOAT_TYPE_P(VALUE value) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "RB_FLOAT_TYPE_P", value));
}

int RTEST(VALUE value) {
  return value != NULL && polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "RTEST", value));
}

// Kernel

void rb_p(VALUE obj) {
  polyglot_invoke(rb_mKernel, "p", obj);
}

VALUE rb_require(const char *feature) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_require", rb_str_new_cstr(feature));
}

VALUE rb_eval_string(const char *str) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_eval_string", rb_str_new_cstr(str));
}

VALUE rb_exec_recursive(VALUE (*func) (VALUE, VALUE, int), VALUE obj, VALUE arg) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_exec_recursive", func, obj, arg);
}

VALUE rb_f_sprintf(int argc, const VALUE *argv) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_f_sprintf", rb_ary_new4(argc, argv));
}

VALUE rb_yield_block(VALUE val, VALUE arg, int argc, const VALUE *argv, VALUE blockarg) {
  rb_tr_error("rb_yield_block not implemented");
}

int ruby_snprintf(char *str, size_t n, char const *fmt, ...) {
  rb_tr_error("ruby_snprintf not implemented");
}

void rb_need_block(void) {
  if (!rb_block_given_p()) {
    rb_raise(rb_eLocalJumpError, "no block given");
  }
}

void rb_set_end_proc(void (*func)(VALUE), VALUE data) {
  rb_tr_error("rb_set_end_proc not implemented");
}

void rb_iter_break(void) {
  rb_iter_break_value(Qnil);
}

void rb_iter_break_value(VALUE value) {
  polyglot_invoke(RUBY_CEXT, "rb_iter_break_value", value);
  rb_tr_error("rb_iter_break_value should not return");
}

const char *rb_sourcefile(void) {
  return RSTRING_PTR(polyglot_invoke(RUBY_CEXT, "rb_sourcefile"));
}

int rb_sourceline(void) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_sourceline"));
}

int rb_method_boundp(VALUE klass, ID id, int ex) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_method_boundp", klass, id, ex));
}

// Object

VALUE rb_obj_dup(VALUE object) {
  return (VALUE) polyglot_invoke(object, "dup");
}

VALUE rb_any_to_s(VALUE object) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_any_to_s", object);
}

VALUE rb_obj_instance_variables(VALUE object) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_obj_instance_variables", object);
}

VALUE rb_check_convert_type(VALUE val, int type, const char *type_name, const char *method) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_check_convert_type", val, rb_str_new_cstr(type_name), rb_str_new_cstr(method));
}

VALUE rb_check_to_integer(VALUE object, const char *method) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_check_to_integer", object, rb_str_new_cstr(method));
}

VALUE rb_check_string_type(VALUE object) {
  return rb_check_convert_type(object, T_STRING, "String", "to_str");
}

VALUE rb_convert_type(VALUE object, int type, const char *type_name, const char *method) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_convert_type", object, rb_str_new_cstr(type_name), rb_str_new_cstr(method));
}

void rb_extend_object(VALUE object, VALUE module) {
  polyglot_invoke(module, "extend_object", object);
}

VALUE rb_inspect(VALUE object) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_inspect", object);
}

void rb_obj_call_init(VALUE object, int argc, const VALUE *argv) {
  polyglot_invoke(RUBY_CEXT, "rb_obj_call_init", object, rb_ary_new4(argc, argv));
}

const char *rb_obj_classname(VALUE object) {
  return RSTRING_PTR((VALUE) polyglot_invoke(RUBY_CEXT, "rb_obj_classname", object));
}

VALUE rb_obj_id(VALUE object) {
  return (VALUE) polyglot_invoke(object, "object_id");
}

void rb_tr_hidden_variable_set(VALUE object, const char *name, VALUE value) {
  polyglot_invoke(RUBY_CEXT, "hidden_variable_set", object, rb_intern(name), value);
}

VALUE rb_tr_hidden_variable_get(VALUE object, const char *name) {
  return polyglot_invoke(RUBY_CEXT, "hidden_variable_get", object, rb_intern(name));
}

int rb_obj_method_arity(VALUE object, ID id) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_obj_method_arity", object, id));
}

int rb_obj_respond_to(VALUE object, ID id, int priv) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_obj_respond_to", object, id, priv));
}

int rb_special_const_p(VALUE object) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "rb_special_const_p", object));
}

VALUE rb_to_int(VALUE object) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_to_int", object);
}

VALUE rb_obj_instance_eval(int argc, const VALUE *argv, VALUE self) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_obj_instance_eval", self, rb_ary_new4(argc, argv), rb_block_proc());
}

VALUE rb_ivar_defined(VALUE object, ID id) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_ivar_defined", object, id);
}

VALUE rb_equal_opt(VALUE a, VALUE b) {
  rb_tr_error("rb_equal_opt not implemented");
}

VALUE rb_class_inherited_p(VALUE module, VALUE object) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_class_inherited_p", module, object);
}

VALUE rb_equal(VALUE a, VALUE b) {
  return (VALUE) polyglot_invoke(a, "===", b);
}

VALUE rb_obj_taint(VALUE object) {
  return (VALUE) polyglot_invoke(object, "taint");
}

bool rb_tr_obj_taintable_p(VALUE object) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "RB_OBJ_TAINTABLE", object));
}

bool rb_tr_obj_tainted_p(VALUE object) {
  return polyglot_as_boolean(polyglot_invoke((void *)object, "tainted?"));
}

void rb_tr_obj_infect(VALUE a, VALUE b) {
  polyglot_invoke(RUBY_CEXT, "rb_tr_obj_infect", a, b);
}

VALUE rb_obj_freeze(VALUE object) {
  return (VALUE) polyglot_invoke(object, "freeze");
}

VALUE rb_obj_frozen_p(VALUE object) {
  return polyglot_invoke(object, "frozen?");
}

// Integer

VALUE rb_Integer(VALUE value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_Integer", value);
}

#define INTEGER_PACK_WORDORDER_MASK \
  (INTEGER_PACK_MSWORD_FIRST | \
   INTEGER_PACK_LSWORD_FIRST)
#define INTEGER_PACK_BYTEORDER_MASK \
  (INTEGER_PACK_MSBYTE_FIRST | \
   INTEGER_PACK_LSBYTE_FIRST | \
   INTEGER_PACK_NATIVE_BYTE_ORDER)
#define INTEGER_PACK_SUPPORTED_FLAG \
  (INTEGER_PACK_MSWORD_FIRST | \
   INTEGER_PACK_LSWORD_FIRST | \
   INTEGER_PACK_MSBYTE_FIRST | \
   INTEGER_PACK_LSBYTE_FIRST | \
   INTEGER_PACK_NATIVE_BYTE_ORDER | \
   INTEGER_PACK_2COMP | \
   INTEGER_PACK_FORCE_GENERIC_IMPLEMENTATION)

static void validate_integer_pack_format(size_t numwords, size_t wordsize, size_t nails, int flags, int supported_flags) {
  int wordorder_bits = flags & INTEGER_PACK_WORDORDER_MASK;
  int byteorder_bits = flags & INTEGER_PACK_BYTEORDER_MASK;

  if (flags & ~supported_flags) {
    rb_raise(rb_eArgError, "unsupported flags specified");
  }

  if (wordorder_bits == 0) {
    if (1 < numwords) {
      rb_raise(rb_eArgError, "word order not specified");
    }
  } else if (wordorder_bits != INTEGER_PACK_MSWORD_FIRST &&
      wordorder_bits != INTEGER_PACK_LSWORD_FIRST) {
    rb_raise(rb_eArgError, "unexpected word order");
  }
  
  if (byteorder_bits == 0) {
    rb_raise(rb_eArgError, "byte order not specified");
  } else if (byteorder_bits != INTEGER_PACK_MSBYTE_FIRST &&
    byteorder_bits != INTEGER_PACK_LSBYTE_FIRST &&
    byteorder_bits != INTEGER_PACK_NATIVE_BYTE_ORDER) {
      rb_raise(rb_eArgError, "unexpected byte order");
  }
  
  if (wordsize == 0) {
    rb_raise(rb_eArgError, "invalid wordsize: %lu", wordsize);
  }
  
  if (8 < wordsize) {
    rb_raise(rb_eArgError, "too big wordsize: %lu", wordsize);
  }
  
  if (wordsize <= nails / CHAR_BIT) {
    rb_raise(rb_eArgError, "too big nails: %lu", nails);
  }
  
  if (INT_MAX / wordsize < numwords) {
    rb_raise(rb_eArgError, "too big numwords * wordsize: %lu * %lu", numwords, wordsize);
  }
}

static int check_msw_first(int flags) {
  return flags & INTEGER_PACK_MSWORD_FIRST;
}

static int endian_swap(int flags) {
  return flags & INTEGER_PACK_MSBYTE_FIRST;
}

int rb_integer_pack(VALUE value, void *words, size_t numwords, size_t wordsize, size_t nails, int flags) {
  long i;
  long j;
  VALUE msw_first, twosComp, swap, bytes;
  int sign, size, bytes_needed, words_needed, result;
  uint8_t *buf;
  msw_first = rb_boolean(check_msw_first(flags));
  twosComp = rb_boolean(((flags & INTEGER_PACK_2COMP) != 0));
  swap = rb_boolean(endian_swap(flags));
  // Test for fixnum and do the right things here.
  bytes = polyglot_invoke(RUBY_CEXT, "rb_integer_bytes", value,
                         (int)numwords, (int)wordsize, msw_first, twosComp, swap);
  size = (twosComp == Qtrue) ? polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_2scomp_bit_length", value))
    : polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_absint_bit_length", value));
  if (RB_FIXNUM_P(value)) {
    long l = NUM2LONG(value);
    sign = (l > 0) - (l < 0);
  } else {
    sign = polyglot_as_i32(polyglot_invoke(value, "<=>", 0));
  }
  bytes_needed = size / 8 + (size % 8 == 0 ? 0 : 1);
  words_needed = bytes_needed / wordsize + (bytes_needed % wordsize == 0 ? 0 : 1);
  result = (words_needed <= numwords ? 1 : 2) * sign;

  buf = (uint8_t *)words;
  for (i = 0; i < numwords * wordsize; i++) {
    buf[i] = (uint8_t) polyglot_as_i32(polyglot_get_array_element(bytes, i));
  }
  return result;
}

VALUE rb_integer_unpack(const void *words, size_t numwords, size_t wordsize, size_t nails, int flags) {
  rb_tr_error("rb_integer_unpack not implemented");
}

size_t rb_absint_size(VALUE value, int *nlz_bits_ret) {
  int size = polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_absint_bit_length", value));
  if (nlz_bits_ret != NULL) {
    *nlz_bits_ret = size % 8;
  }
  int bytes = size / 8;
  if (size % 8 > 0) {
    bytes++;
  }
  return bytes;
}

VALUE rb_cstr_to_inum(const char* string, int base, int raise) {
  return polyglot_invoke(RUBY_CEXT, "rb_cstr_to_inum", rb_str_new_cstr(string), base, raise);
}

double rb_cstr_to_dbl(const char* string, int badcheck) {
  return polyglot_as_double(polyglot_invoke(RUBY_CEXT, "rb_cstr_to_dbl", rb_str_new_cstr(string), rb_boolean(badcheck)));
}

double rb_big2dbl(VALUE x) {
  return polyglot_as_double(polyglot_invoke(RUBY_CEXT, "rb_num2dbl", x));
}

VALUE rb_dbl2big(double d) {
  return polyglot_invoke(RUBY_CEXT, "DBL2BIG", d);
}

LONG_LONG rb_big2ll(VALUE x) {
  return polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_num2long", x));
}

unsigned LONG_LONG rb_big2ull(VALUE x) {
  return polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_num2ulong", x));
}

long rb_big2long(VALUE x) {
  return polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_num2long", x));
}

VALUE rb_big2str(VALUE x, int base) {
  return (VALUE) polyglot_invoke((void *)x, "to_s", base);
}

unsigned long rb_big2ulong(VALUE x) {
  return polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_num2ulong", x));
}

VALUE rb_big_cmp(VALUE x, VALUE y) {
  return (VALUE) polyglot_invoke(x, "<=>", y);
}

void rb_big_pack(VALUE val, unsigned long *buf, long num_longs) {
  rb_integer_pack(val, buf, num_longs, 8, 0,
                  INTEGER_PACK_2COMP | INTEGER_PACK_NATIVE_BYTE_ORDER | INTEGER_PACK_LSWORD_FIRST);
}

// Float

VALUE rb_float_new(double value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_float_new", value);
}

VALUE rb_Float(VALUE value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_Float", value);
}

double rb_float_value(VALUE value) {
  return polyglot_as_double(polyglot_invoke(RUBY_CEXT, "RFLOAT_VALUE", value));
}

// String

char *RSTRING_PTR_IMPL(VALUE string) {
  char* ret = polyglot_invoke(RUBY_CEXT, "RSTRING_PTR", string);

  // We start off treating RStringPtr as if it weren't a pointer for interop purposes. This is so Sulong doesn't try
  // to convert it to an actual char* when returning from `polyglot_invoke`. Once we have a handle to the real RStringPtr
  // object, we can instruct it to start acting like a pointer, which is necessary for pointer address comparisons.
  polyglot_as_boolean(polyglot_invoke(ret, "act_like_pointer=", Qtrue));

  return ret;
}

char *RSTRING_END(VALUE string) {
  char* ret = polyglot_invoke(RUBY_CEXT, "RSTRING_END", string);

  // We start off treating RStringPtr as if it weren't a pointer for interop purposes. This is so Sulong doesn't try
  // to convert it to an actual char* when returning from `polyglot_invoke`. Once we have a handle to the real RStringPtr
  // object, we can instruct it to start acting like a pointer, which is necessary for pointer address comparisons.
  polyglot_as_boolean(polyglot_invoke(ret, "act_like_pointer=", Qtrue));

  return ret;
}

int MBCLEN_NEEDMORE_P(int r) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "MBCLEN_NEEDMORE_P", r));
}

int MBCLEN_NEEDMORE_LEN(int r) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "MBCLEN_NEEDMORE_LEN", r));
}

int MBCLEN_CHARFOUND_P(int r) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "MBCLEN_CHARFOUND_P", r));
}

int MBCLEN_CHARFOUND_LEN(int r) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "MBCLEN_CHARFOUND_LEN", r));
}

int rb_str_len(VALUE string) {
  return polyglot_as_i32(polyglot_invoke((void *)string, "bytesize"));
}

VALUE rb_str_new(const char *string, long length) {
  if (length < 0) {
    rb_raise(rb_eArgError, "negative string size (or size too big)");
  }

  if (string == NULL) {
    return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_str_new_nul", length);
  } else if (polyglot_is_value((VALUE) string)) {
    return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_str_new", string, length);
  } else {
    // Copy the string to a new unmanaged buffer, because otherwise it's very
    // hard to accomodate all the different things this pointer could really be
    // - unmanaged pointer, foreign object, foreign object plus offset, etc.
    // TODO CS 24-Oct-17 work with Sulong to make this copying not needed

    const char* copy = malloc(length);
    memcpy(copy, string, length);
    VALUE ruby_string = (VALUE) polyglot_invoke(RUBY_CEXT, "rb_str_new_cstr", copy, length);
    free(copy);
    return ruby_string;
  }
}

VALUE rb_tainted_str_new(const char *ptr, long len) {
    VALUE str = rb_str_new(ptr, len);

    OBJ_TAINT(str);
    return str;
}

VALUE rb_str_new_cstr(const char *string) {
  if (polyglot_is_value((VALUE) string)) {
    VALUE ruby_string = (VALUE) polyglot_invoke((VALUE) string, "to_s");
    int len = strlen(string);
    return (VALUE) polyglot_invoke(ruby_string, "[]", 0, len);
  } else {
    // TODO CS 24-Oct-17 would be nice to read in one go rather than strlen followed by read
    return rb_str_new(string, strlen(string));
  }
}

VALUE rb_str_new_shared(VALUE string) {
  return polyglot_invoke((void *)string, "dup");
}

VALUE rb_str_new_with_class(VALUE klass, const char *string, long len) {
  return polyglot_invoke(polyglot_invoke(klass, "class"), "new", rb_str_new(string, len));
}

VALUE rb_tainted_str_new_cstr(const char *ptr) {
    VALUE str = rb_str_new_cstr(ptr);

    OBJ_TAINT(str);
    return str;
}

ID rb_intern_str(VALUE string) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_intern_str", string);
}

VALUE rb_str_cat(VALUE string, const char *to_concat, long length) {
  polyglot_invoke(string, "concat", rb_enc_str_new(to_concat, length, STR_ENC_GET(string)));
  return string;
}

VALUE rb_str_cat2(VALUE string, const char *to_concat) {
  polyglot_invoke(string, "concat", rb_str_new_cstr(to_concat));
  return string;
}

VALUE rb_str_to_str(VALUE string) {
  return rb_convert_type(string, T_STRING, "String", "to_str");
}

VALUE rb_str_buf_new(long capacity) {
  return rb_str_new_cstr("");
}

VALUE rb_vsprintf(const char *format, va_list args) {
  return rb_enc_vsprintf(rb_ascii8bit_encoding(), format, args);
}

VALUE rb_str_append(VALUE string, VALUE to_append) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_str_append", string, to_append);
}

VALUE rb_str_concat(VALUE string, VALUE to_concat) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_str_concat", string, to_concat);
}

void rb_str_set_len(VALUE string, long length) {
  rb_str_resize(string, length);
}

VALUE rb_str_new_frozen(VALUE value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_str_new_frozen", value);
}

VALUE rb_String(VALUE value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_String", value);
}

VALUE rb_str_resize(VALUE string, long length) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_str_resize", string, length);
}

VALUE rb_str_split(VALUE string, const char *split) {
  return (VALUE) polyglot_invoke(string, "split", rb_str_new_cstr(split));
}

void rb_str_modify(VALUE string) {
  // Does nothing because writing to the string pointer will cause necessary invalidations anyway
}

VALUE rb_cstr2inum(const char *string, int base) {
  return rb_cstr_to_inum(string, base, base==0);
}

VALUE rb_str_to_inum(VALUE str, int base, int badcheck) {
  char *s;
  StringValue(str);
  rb_must_asciicompat(str);
  if (badcheck) {
    s = StringValueCStr(str);
  } else {
    s = RSTRING_PTR(str);
  }
  return rb_cstr_to_inum(s, base, badcheck);
}

VALUE rb_str2inum(VALUE string, int base) {
  return rb_str_to_inum(string, base, base==0);
}

VALUE rb_str_buf_new_cstr(const char *string) {
  return rb_str_new_cstr(string);
}

int rb_str_cmp(VALUE a, VALUE b) {
  return polyglot_as_i32(polyglot_invoke((void *)a, "<=>", b));
}

VALUE rb_str_buf_cat(VALUE string, const char *to_concat, long length) {
  return rb_str_cat(string, to_concat, length);
}

POLYGLOT_DECLARE_STRUCT(rb_encoding)

// returns Truffle::CExt::RbEncoding, takes Encoding or String
rb_encoding *rb_to_encoding(VALUE encoding) {
  return polyglot_as_rb_encoding(polyglot_invoke(RUBY_CEXT, "rb_to_encoding", encoding));
}

VALUE rb_str_conv_enc(VALUE string, rb_encoding *from, rb_encoding *to) {
  return rb_str_conv_enc_opts(string, from, to, 0, Qnil);
}

VALUE rb_str_conv_enc_opts(VALUE str, rb_encoding *from, rb_encoding *to, int ecflags, VALUE ecopts) {
  if (!to) return str;
  if (!from) from = rb_enc_get(str);
  if (from == to) return str;
  return polyglot_invoke(RUBY_CEXT, "rb_str_conv_enc_opts", str, rb_enc_from_encoding(from), rb_enc_from_encoding(to), ecflags, ecopts);
}

VALUE
rb_tainted_str_new_with_enc(const char *ptr, long len, rb_encoding *enc) {
  VALUE str = rb_enc_str_new(ptr, len, enc);
  OBJ_TAINT(str);
  return str;
}

VALUE rb_external_str_new_with_enc(const char *ptr, long len, rb_encoding *eenc) {
  VALUE str;
  str = rb_tainted_str_new_with_enc(ptr, len, eenc);
  return rb_external_str_with_enc(str, eenc);
}

VALUE rb_external_str_with_enc(VALUE str, rb_encoding *eenc) {
  if (polyglot_as_boolean(polyglot_invoke(rb_enc_from_encoding(eenc), "==", rb_enc_from_encoding(rb_usascii_encoding()))) &&
    rb_enc_str_coderange(str) != ENC_CODERANGE_7BIT) {
    rb_enc_associate_index(str, rb_ascii8bit_encindex());
    return str;
  }
  rb_enc_associate(str, eenc);
  return rb_str_conv_enc(str, eenc, rb_default_internal_encoding());
}

VALUE rb_external_str_new(const char *string, long len) {
  return rb_external_str_new_with_enc(string, len, rb_default_external_encoding());
}

VALUE rb_external_str_new_cstr(const char *string) {
  return rb_external_str_new_with_enc(string, strlen(string), rb_default_external_encoding());
}

VALUE rb_locale_str_new(const char *string, long len) {
  return rb_external_str_new_with_enc(string, len, rb_locale_encoding());
}

VALUE rb_locale_str_new_cstr(const char *string) {
  return rb_external_str_new_with_enc(string, strlen(string), rb_locale_encoding());
}

VALUE rb_filesystem_str_new(const char *string, long len) {
  return rb_external_str_new_with_enc(string, len, rb_filesystem_encoding());
}

VALUE rb_filesystem_str_new_cstr(const char *string) {
  return rb_external_str_new_with_enc(string, strlen(string), rb_filesystem_encoding());
}

VALUE rb_str_export(VALUE string) {
  return rb_str_conv_enc(string, STR_ENC_GET(string), rb_default_external_encoding());
}

VALUE rb_str_export_locale(VALUE string) {
  return rb_str_conv_enc(string, STR_ENC_GET(string), rb_locale_encoding());
}

VALUE rb_str_export_to_enc(VALUE string, rb_encoding *enc) {
  return rb_str_conv_enc(string, STR_ENC_GET(string), enc);
}

rb_encoding *rb_default_external_encoding(void) {
  VALUE result = polyglot_invoke(RUBY_CEXT, "rb_default_external_encoding");
  if (result == Qnil) {
    return NULL;
  }
  return rb_to_encoding(result);
}

rb_encoding *rb_default_internal_encoding(void) {
  VALUE result = polyglot_invoke(RUBY_CEXT, "rb_default_internal_encoding");
  if (result == Qnil) {
    return NULL;
  }
  return rb_to_encoding(result);
}

rb_encoding *rb_locale_encoding(void) {
  VALUE result = polyglot_invoke(RUBY_CEXT, "rb_locale_encoding");
  if (result == Qnil) {
    return NULL;
  }
  return rb_to_encoding(result);
}

int rb_locale_encindex(void) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_locale_encindex"));
}

rb_encoding *rb_filesystem_encoding(void) {
  VALUE result = polyglot_invoke(RUBY_CEXT, "rb_filesystem_encoding");
  if (result == Qnil) {
    return NULL;
  }
  return rb_to_encoding(result);
}

int rb_filesystem_encindex(void) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_filesystem_encindex"));
}

rb_encoding *get_encoding(VALUE string) {
  return rb_to_encoding(polyglot_invoke(string, "encoding"));
}

VALUE rb_str_intern(VALUE string) {
  return (VALUE) polyglot_invoke(string, "intern");
}

VALUE rb_str_length(VALUE string) {
  return (VALUE) polyglot_invoke(string, "length");
}

VALUE rb_str_plus(VALUE a, VALUE b) {
  return (VALUE) polyglot_invoke(a, "+", b);
}

VALUE rb_str_subseq(VALUE string, long beg, long len) {
  rb_tr_error("rb_str_subseq not implemented");
}

VALUE rb_str_substr(VALUE string, long beg, long len) {
  return (VALUE) polyglot_invoke(string, "[]", beg, len);
}

st_index_t rb_str_hash(VALUE string) {
  return (st_index_t) polyglot_as_i64(polyglot_invoke((void *)string, "hash"));
}

void rb_str_update(VALUE string, long beg, long len, VALUE value) {
  polyglot_invoke(string, "[]=", beg, len, value);
}

VALUE rb_str_replace(VALUE str, VALUE by) {
  return polyglot_invoke(str, "replace", by);
}

VALUE rb_str_equal(VALUE a, VALUE b) {
  return (VALUE) polyglot_invoke(a, "==", b);
}

void rb_str_free(VALUE string) {
//  intentional noop here
}

unsigned int rb_enc_codepoint_len(const char *p, const char *e, int *len_p, rb_encoding *encoding) {
  int len = e - p;
  if (len <= 0) {
    rb_raise(rb_eArgError, "empty string");
  }
  VALUE array = polyglot_invoke(RUBY_CEXT, "rb_enc_codepoint_len", rb_str_new(p, len), rb_enc_from_encoding(encoding));
  if (len_p) *len_p = polyglot_as_i32(polyglot_invoke(array, "[]", 0));
  return (unsigned int)polyglot_as_i32(polyglot_invoke(array, "[]", 1));
}

rb_encoding *rb_enc_get(VALUE object) {
  return rb_to_encoding(polyglot_invoke(RUBY_CEXT, "rb_enc_get", object));
}

void rb_enc_set_index(VALUE obj, int idx) {
  polyglot_invoke(RUBY_CEXT, "rb_enc_set_index", obj, idx);
}

rb_encoding *rb_ascii8bit_encoding(void) {
  return rb_to_encoding(polyglot_invoke(RUBY_CEXT, "ascii8bit_encoding"));
}

int rb_ascii8bit_encindex(void) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_ascii8bit_encindex"));
}

rb_encoding *rb_usascii_encoding(void) {
  return rb_to_encoding(polyglot_invoke(RUBY_CEXT, "usascii_encoding"));
}

int rb_enc_asciicompat(rb_encoding *enc) {
  return polyglot_as_boolean(polyglot_invoke(rb_enc_from_encoding(enc), "ascii_compatible?"));
}

void rb_must_asciicompat(VALUE str) {
  rb_encoding *enc = rb_enc_get(str);
  if (!rb_enc_asciicompat(enc)) {
    rb_raise(rb_eEncCompatError, "ASCII incompatible encoding: %s", rb_enc_name(enc));
  }
}

int rb_usascii_encindex(void) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_usascii_encindex"));
}

rb_encoding *rb_utf8_encoding(void) {
  return rb_to_encoding(polyglot_invoke(RUBY_CEXT, "utf8_encoding"));
}

int rb_utf8_encindex(void) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_utf8_encindex"));
}

enum ruby_coderange_type RB_ENC_CODERANGE(VALUE obj) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "RB_ENC_CODERANGE", obj));
}

int rb_encdb_alias(const char *alias, const char *orig) {
  rb_tr_error("rb_encdb_alias not implemented");
}

VALUE rb_enc_associate(VALUE obj, rb_encoding *enc) {
  return rb_enc_associate_index(obj, rb_enc_to_index(enc));
}

VALUE rb_enc_associate_index(VALUE obj, int idx) {
  return polyglot_invoke(RUBY_CEXT, "rb_enc_associate_index", obj, idx);
}

rb_encoding* rb_enc_compatible(VALUE str1, VALUE str2) {
  VALUE result = polyglot_invoke(rb_cEncoding, "compatible?", str1, str2);
  if (result != Qnil) {
    return rb_to_encoding(result);
  }
  return NULL;
}

void rb_enc_copy(VALUE obj1, VALUE obj2) {
  rb_enc_associate_index(obj1, rb_enc_get_index(obj2));
}

int rb_enc_find_index(const char *name) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_enc_find_index", rb_str_new_cstr(name)));
}

rb_encoding *rb_enc_find(const char *name) {
  int idx = rb_enc_find_index(name);
  if (idx < 0) idx = 0;
  return rb_enc_from_index(idx);
}

// returns Encoding, takes rb_encoding struct or RbEncoding
VALUE rb_enc_from_encoding(rb_encoding *encoding) {
  return polyglot_invoke(RUBY_CEXT, "rb_enc_from_encoding", encoding);
}

rb_encoding *rb_enc_from_index(int index) {
  return rb_to_encoding(polyglot_invoke(RUBY_CEXT, "rb_enc_from_index", index));
}

int rb_enc_str_coderange(VALUE str) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_enc_str_coderange", str));
}

int rb_tr_obj_equal(VALUE first, VALUE second) {
  return RTEST(rb_funcall(first, rb_intern("equal?"), 1, second));
}

int rb_tr_flags(VALUE value) {
  int flags = 0;
  if (OBJ_FROZEN(value)) {
    flags |= RUBY_FL_FREEZE;
  }
  if (OBJ_TAINTED(value)) {
    flags |= RUBY_FL_TAINT;
  }
  // TODO BJF Nov-11-2017 Implement more flags
  return flags;
}

// Undef conflicting macro from encoding.h like MRI
#undef rb_enc_str_new
VALUE rb_enc_str_new(const char *ptr, long len, rb_encoding *enc) {
  return polyglot_invoke(rb_str_new(ptr, len), "force_encoding", rb_enc_from_encoding(enc));
}

void rb_enc_raise(rb_encoding *enc, VALUE exc, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    VALUE mesg = rb_vsprintf(fmt, args);
    va_end(args);
    rb_exc_raise(rb_exc_new_str(exc, (VALUE) polyglot_invoke(mesg, "force_encoding", rb_enc_from_encoding(enc))));
}

VALUE rb_enc_sprintf(rb_encoding *enc, const char *format, ...) {
  rb_tr_error("rb_enc_sprintf not implemented");
}

int rb_enc_to_index(rb_encoding *enc) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_enc_to_index", rb_enc_from_encoding(enc)));
}

VALUE rb_obj_encoding(VALUE obj) {
  return polyglot_invoke(obj, "encoding");
}

VALUE rb_str_encode(VALUE str, VALUE to, int ecflags, VALUE ecopts) {
  return polyglot_invoke(RUBY_CEXT, "rb_str_encode", str, to, ecflags, ecopts);
}

VALUE rb_usascii_str_new(const char *ptr, long len) {
  return polyglot_invoke(rb_str_new(ptr, len), "force_encoding", rb_enc_from_encoding(rb_usascii_encoding()));
}

VALUE rb_usascii_str_new_cstr(const char *ptr) {
  return polyglot_invoke(rb_str_new_cstr(ptr), "force_encoding", rb_enc_from_encoding(rb_usascii_encoding()));
}

int rb_to_encoding_index(VALUE enc) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_to_encoding_index", enc));
}

char* rb_enc_nth(const char *p, const char *e, long nth, rb_encoding *enc) {
  rb_tr_error("rb_enc_nth not implemented");
}

int rb_enc_get_index(VALUE obj) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_enc_get_index", obj));
}

char* rb_enc_left_char_head(char *start, char *p, char *end, rb_encoding *enc) {
  int length = start-end;
  int position = polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_enc_left_char_head",
      rb_enc_from_encoding(enc),
      rb_str_new(start, length),
      0,
      p-start,
      length));
  return start+position;
}

int rb_enc_precise_mbclen(const char *p, const char *e, rb_encoding *enc) {
  int length = p-e;
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_enc_precise_mbclen",
      rb_enc_from_encoding(enc),
      rb_str_new(p, length),
      0,
      length));
}

VALUE rb_str_times(VALUE string, VALUE times) {
  return (VALUE) polyglot_invoke(string, "*", times);
}

int rb_enc_dummy_p(rb_encoding *enc) {
  return polyglot_as_i32(polyglot_invoke(rb_enc_from_encoding(enc), "dummy?"));
}

int rb_enc_mbmaxlen(rb_encoding *enc) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_enc_mbmaxlen", rb_enc_from_encoding(enc)));
}

int rb_enc_mbminlen(rb_encoding *enc) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_enc_mbminlen", rb_enc_from_encoding(enc)));
}

int rb_enc_mbclen(const char *p, const char *e, rb_encoding *enc) {
  int length = e-p;
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_enc_mbclen",
      rb_enc_from_encoding(enc),
      rb_str_new(p, length),
      0,
      length));
}

void rb_econv_close(rb_econv_t *ec) {
  rb_tr_error("rb_econv_close not implemented");
}

rb_econv_t *rb_econv_open_opts(const char *source_encoding, const char *destination_encoding, int ecflags, VALUE opthash) {
  rb_tr_error("rb_econv_open_opts not implemented");
}

VALUE rb_econv_str_convert(rb_econv_t *ec, VALUE src, int flags) {
  rb_tr_error("rb_econv_str_convert not implemented");
}

rb_econv_result_t rb_econv_convert(rb_econv_t *ec, const unsigned char **input_ptr, const unsigned char *input_stop, unsigned char **output_ptr, unsigned char *output_stop, int flags) {
  rb_tr_error("rb_econv_convert not implemented");
}

void rb_econv_check_error(rb_econv_t *ec) {
  rb_tr_error("rb_econv_check_error not implemented");
}

int rb_econv_prepare_opts(VALUE opthash, VALUE *opts) {
  rb_tr_error("rb_econv_prepare_opts not implemented");
}

// Symbol

ID rb_to_id(VALUE name) {
  return SYM2ID((VALUE) polyglot_invoke(name, "to_sym"));
}

ID rb_intern(const char *string) {
  return (ID) polyglot_invoke(RUBY_CEXT, "rb_intern", rb_str_new_cstr(string));
}

ID rb_intern2(const char *string, long length) {
  return (ID) SYM2ID(polyglot_invoke(RUBY_CEXT, "rb_intern", rb_str_new(string, length)));
}

ID rb_intern3(const char *name, long len, rb_encoding *enc) {
  return (ID) SYM2ID(polyglot_invoke(RUBY_CEXT, "rb_intern3", rb_str_new(name, len), rb_enc_from_encoding(enc)));
}

VALUE rb_sym2str(VALUE string) {
  return (VALUE) polyglot_invoke(string, "to_s");
}

const char *rb_id2name(ID id) {
    return RSTRING_PTR(rb_id2str(id));
}

VALUE rb_id2str(ID id) {
  return polyglot_invoke(RUBY_CEXT, "rb_id2str", ID2SYM(id));
}

int rb_is_class_id(ID id) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "rb_is_class_id", ID2SYM(id)));
}

int rb_is_const_id(ID id) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "rb_is_const_id", ID2SYM(id)));
}

int rb_is_instance_id(ID id) {
  return polyglot_as_boolean(polyglot_invoke(RUBY_CEXT, "rb_is_instance_id", ID2SYM(id)));
}

// Array

long rb_array_len(VALUE array) {
  return polyglot_get_array_size(array);
}

int RARRAY_LENINT(VALUE array) {
  return polyglot_get_array_size(array);
}

VALUE RARRAY_AREF(VALUE array, long index) {
  return polyglot_get_array_element(array, (int) index);
}

VALUE rb_Array(VALUE array) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_Array", array);
}

VALUE rb_ary_new() {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_ary_new");
}

VALUE rb_ary_new_capa(long capacity) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_ary_new_capa", capacity);
}

VALUE rb_ary_resize(VALUE ary, long len) {
  rb_tr_error("rb_ary_resize not implemented");
}

VALUE rb_ary_new_from_args(long n, ...) {
  VALUE array = rb_ary_new_capa(n);
  for (int i = 0; i < n; i++) {
    rb_ary_store(array, i, (VALUE) polyglot_get_arg(1+i));
  }
  return array;
}

VALUE rb_ary_new_from_values(long n, const VALUE *values) {
  VALUE array = rb_ary_new_capa(n);
  for (int i = 0; i < n; i++) {
    rb_ary_store(array, i, values[i]);
  }
  return array;
}

VALUE rb_ary_push(VALUE array, VALUE value) {
  polyglot_invoke(array, "push", value);
  return array;
}

VALUE rb_ary_pop(VALUE array) {
  return (VALUE) polyglot_invoke(array, "pop");
}

void rb_ary_store(VALUE array, long index, VALUE value) {
  polyglot_set_array_element(array, (int) index, value);
}

VALUE rb_ary_entry(VALUE array, long index) {
  return polyglot_get_array_element(array, (int) index);
}

VALUE rb_ary_each(VALUE array) {
  rb_tr_error("rb_ary_each not implemented");
}

VALUE rb_ary_unshift(VALUE array, VALUE value) {
  return (VALUE) polyglot_invoke(array, "unshift", value);
}

VALUE rb_ary_aref(int n, const VALUE* values, VALUE array) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "send_splatted", array, rb_str_new_cstr("[]"), rb_ary_new4(n, values));
}

VALUE rb_ary_clear(VALUE array) {
  return (VALUE) polyglot_invoke(array, "clear");
}

VALUE rb_ary_delete(VALUE array, VALUE value) {
  return (VALUE) polyglot_invoke(array, "delete", value);
}

VALUE rb_ary_delete_at(VALUE array, long n) {
  return (VALUE) polyglot_invoke(array, "delete_at", n);
}

VALUE rb_ary_includes(VALUE array, VALUE value) {
  return (VALUE) polyglot_invoke(array, "include?", value);
}

VALUE rb_ary_join(VALUE array, VALUE sep) {
  return (VALUE) polyglot_invoke(array, "join", sep);
}

VALUE rb_ary_to_s(VALUE array) {
  return (VALUE) polyglot_invoke(array, "to_s");
}

VALUE rb_ary_reverse(VALUE array) {
  return (VALUE) polyglot_invoke(array, "reverse!");
}

VALUE rb_ary_shift(VALUE array) {
  return (VALUE) polyglot_invoke(array, "shift");
}

VALUE rb_ary_concat(VALUE a, VALUE b) {
  return (VALUE) polyglot_invoke(a, "concat", b);
}

VALUE rb_ary_plus(VALUE a, VALUE b) {
  return (VALUE) polyglot_invoke(a, "+", b);
}

VALUE rb_iterate(VALUE (*function)(), VALUE arg1, VALUE (*block)(), VALUE arg2) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_iterate", function, arg1, block, arg2);
}

VALUE rb_each(VALUE array) {
  if (rb_block_given_p()) {
    return rb_funcall_with_block(array, rb_intern("each"), 0, NULL, rb_block_proc());
  } else {
    return (VALUE) polyglot_invoke(array, "each");
  }
}

void rb_mem_clear(VALUE *mem, long n) {
  for (int i = 0; i < n; i++) {
    mem[i] = Qnil;
  }
}

VALUE rb_ary_to_ary(VALUE array) {
  VALUE tmp = rb_check_array_type(array);

  if (!NIL_P(tmp)) return tmp;
  return rb_ary_new3(1, array);
}

VALUE rb_ary_subseq(VALUE array, long start, long length) {
  return (VALUE) polyglot_invoke(array, "[]", start, length);
}

VALUE rb_check_array_type(VALUE array) {
  return rb_check_convert_type(array, T_ARRAY, "Array", "to_ary");
}

VALUE rb_ary_cat(VALUE array, const VALUE *cat, long n) {
  return (VALUE) polyglot_invoke(array, "concat", rb_ary_new4(n, cat));
}

VALUE rb_ary_rotate(VALUE array, long n) {
  if (n != 0) {
    return (VALUE) polyglot_invoke(array, "rotate!", n);
  }
  return Qnil;
}

// Hash

VALUE rb_Hash(VALUE obj) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_Hash", obj);
}

VALUE rb_hash(VALUE obj) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_hash", obj);
}

VALUE rb_hash_new() {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_hash_new");
}

VALUE rb_hash_aref(VALUE hash, VALUE key) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_hash_aref", hash, key);
}

VALUE rb_hash_fetch(VALUE hash, VALUE key) {
  return (VALUE) polyglot_invoke(hash, "fetch", key);
}

VALUE rb_hash_aset(VALUE hash, VALUE key, VALUE value) {
  return (VALUE) polyglot_invoke(hash, "[]=", key, value);
}

VALUE rb_hash_dup(VALUE hash) {
  return rb_obj_dup(hash);
}

VALUE rb_hash_lookup(VALUE hash, VALUE key) {
  return rb_hash_lookup2(hash, key, Qnil);
}

VALUE rb_hash_lookup2(VALUE hash, VALUE key, VALUE default_value) {
  return (VALUE) polyglot_invoke(hash, "fetch", key, default_value);
}

VALUE rb_hash_set_ifnone(VALUE hash, VALUE if_none) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_hash_set_ifnone", hash, if_none);
}

st_index_t rb_memhash(const void *data, long length) {
  // Not a proper hash - just something that produces a stable result for now

  long hash = 0;

  for (long n = 0; n < length; n++) {
    hash = (hash << 1) ^ ((uint8_t*) data)[n];
  }

  return (st_index_t) hash;
}

VALUE rb_hash_clear(VALUE hash) {
  return (VALUE) polyglot_invoke(hash, "clear");
}

VALUE rb_hash_delete(VALUE hash, VALUE key) {
  return (VALUE) polyglot_invoke(hash, "delete", key);
}

VALUE rb_hash_delete_if(VALUE hash) {
  if (rb_block_given_p()) {
    return rb_funcall_with_block(hash, rb_intern("delete_if"), 0, NULL, rb_block_proc());
  } else {
    return (VALUE) polyglot_invoke(hash, "delete_if");
  }
}

void rb_hash_foreach(VALUE hash, int (*func)(ANYARGS), VALUE farg) {
  polyglot_invoke(RUBY_CEXT, "rb_hash_foreach", hash, (void (*)(void *)) func, farg);
}

VALUE rb_hash_size(VALUE hash) {
  return (VALUE) polyglot_invoke(hash, "size");
}

// Class

const char* rb_class2name(VALUE ruby_class) {
  return RSTRING_PTR(rb_class_name(ruby_class));
}

VALUE rb_class_real(VALUE ruby_class) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_class_real", ruby_class);
}

VALUE rb_class_superclass(VALUE ruby_class) {
  return (VALUE) polyglot_invoke(ruby_class, "superclass");
}

VALUE rb_obj_class(VALUE object) {
  return rb_class_real(rb_class_of(object));
}

VALUE rb_singleton_class(VALUE object) {
  return polyglot_invoke(object, "singleton_class");
}

VALUE rb_class_of(VALUE object) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_class_of", object);
}

VALUE rb_obj_alloc(VALUE ruby_class) {
  return (VALUE) polyglot_invoke(ruby_class, "__allocate__");
}

VALUE rb_class_path(VALUE ruby_class) {
  return (VALUE) polyglot_invoke(ruby_class, "name");
}

VALUE rb_path2class(const char *string) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_path_to_class", rb_str_new_cstr(string));
}

VALUE rb_path_to_class(VALUE pathname) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_path_to_class", pathname);
}

VALUE rb_class_name(VALUE ruby_class) {
  VALUE name = polyglot_invoke(ruby_class, "name");

  if (NIL_P(name)) {
    return rb_class_name(rb_obj_class(ruby_class));
  } else {
    return name;
  }
}

VALUE rb_class_new(VALUE super) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_class_new", super);
}

VALUE rb_class_new_instance(int argc, const VALUE *argv, VALUE klass) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_class_new_instance", klass, rb_ary_new4(argc, argv));
}

VALUE rb_cvar_defined(VALUE klass, ID id) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cvar_defined", klass, id);
}

VALUE rb_cvar_get(VALUE klass, ID id) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cvar_get", klass, id);
}

void rb_cvar_set(VALUE klass, ID id, VALUE val) {
  polyglot_invoke(RUBY_CEXT, "rb_cvar_set", klass, id, val);
}

VALUE rb_cv_get(VALUE klass, const char *name) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_cv_get", klass, rb_str_new_cstr(name));
}

void rb_cv_set(VALUE klass, const char *name, VALUE val) {
  polyglot_invoke(RUBY_CEXT, "rb_cv_set", klass, rb_str_new_cstr(name), val);
}

void rb_define_attr(VALUE klass, const char *name, int read, int write) {
  polyglot_invoke(RUBY_CEXT, "rb_define_attr", klass, ID2SYM(rb_intern(name)), read, write);
}

void rb_define_class_variable(VALUE klass, const char *name, VALUE val) {
  polyglot_invoke(RUBY_CEXT, "rb_cv_set", klass, rb_str_new_cstr(name), val);
}

VALUE rb_mod_ancestors(VALUE mod) {
  return (VALUE) polyglot_invoke(mod, "ancestors");
}

// Proc

VALUE rb_proc_new(VALUE (*function)(ANYARGS), VALUE value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_proc_new", (void (*)(void *)) function, value);
}

VALUE rb_proc_call(VALUE self, VALUE args) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_proc_call", self, args);
}

int rb_proc_arity(VALUE self) {
  return polyglot_as_i32(polyglot_invoke(self, "arity"));
}

// Utilities

void rb_bug(const char *fmt, ...) {
  rb_tr_error("rb_bug not yet implemented");
}

int rb_tr_to_int_const(VALUE value) {
  if (value == Qfalse) {
    return Qfalse_int_const;
  } else if (value == Qtrue) {
    return Qtrue_int_const;
  } else if (value == Qnil) {
    return Qnil_int_const;
  } else {
    return 8;
  }
}

VALUE rb_enumeratorize(VALUE obj, VALUE meth, int argc, const VALUE *argv) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_enumeratorize", obj, meth, rb_ary_new4(argc, argv));
}

void rb_check_arity(int argc, int min, int max) {
  polyglot_invoke(RUBY_CEXT, "rb_check_arity", argc, min, max);
}

char* ruby_strdup(const char *str) {
  char *tmp;
  size_t len = strlen(str) + 1;

  tmp = xmalloc(len);
  memcpy(tmp, str, len);

  return tmp;
}

// Calls

int rb_respond_to(VALUE object, ID name) {
  return polyglot_as_boolean(polyglot_invoke((void *)object, "respond_to?", name));
}

VALUE rb_funcallv(VALUE object, ID name, int args_count, const VALUE *args) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_funcallv", object, ID2SYM(name), rb_ary_new4(args_count, args));
}

VALUE rb_funcallv_public(VALUE object, ID name, int args_count, const VALUE *args) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_funcallv_public", object, ID2SYM(name), rb_ary_new4(args_count, args));
}

VALUE rb_apply(VALUE object, ID name, VALUE args) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_apply", object, ID2SYM(name), args);
}

VALUE rb_block_call(VALUE object, ID name, int args_count, const VALUE *args, rb_block_call_func_t block_call_func, VALUE data) {
  if (rb_block_given_p()) {
    return rb_funcall_with_block(object, name, args_count, args, rb_block_proc());
  } else if (block_call_func == NULL) {
    return rb_funcallv(object, name, args_count, args);
  } else {
    return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_block_call", object, ID2SYM(name), rb_ary_new4(args_count, args), block_call_func, data);
  }
}

VALUE rb_call_super(int args_count, const VALUE *args) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_call_super", rb_ary_new4(args_count, args));
}

int rb_block_given_p() {
  return !NIL_P(rb_block_proc());
}

VALUE rb_block_proc(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_block_proc");
}

VALUE rb_block_lambda(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_block_lambda");
}

VALUE rb_yield(VALUE value) {
  if (rb_block_given_p()) {
    return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_yield", value);
  } else {
    return polyglot_invoke(RUBY_CEXT, "yield_no_block");
  }
}

VALUE rb_funcall_with_block(VALUE recv, ID mid, int argc, const VALUE *argv, VALUE pass_procval) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_funcall_with_block", recv, ID2SYM(mid), rb_ary_new4(argc, argv), pass_procval);
}

VALUE rb_yield_splat(VALUE values) {
  if (rb_block_given_p()) {
    return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_yield_splat", values);
  } else {
    return polyglot_invoke(RUBY_CEXT, "yield_no_block");
  }
}

VALUE rb_yield_values(int n, ...) {
  VALUE values = rb_ary_new_capa(n);
  for (int i = 0; i < n; i++) {
    rb_ary_store(values, i, (VALUE) polyglot_get_arg(1+i));
  }
  return rb_yield_splat(values);
}

// Instance variables

VALUE rb_iv_get(VALUE object, const char *name) {
  return polyglot_invoke(RUBY_CEXT, "rb_ivar_get", object, rb_str_new_cstr(name));
}

VALUE rb_iv_set(VALUE object, const char *name, VALUE value) {
  polyglot_invoke(RUBY_CEXT, "rb_ivar_set", object, rb_str_new_cstr(name), value);
  return value;
}

VALUE rb_ivar_get(VALUE object, ID name) {
  return polyglot_invoke(RUBY_CEXT, "rb_ivar_get", object, name);
}

VALUE rb_ivar_set(VALUE object, ID name, VALUE value) {
  polyglot_invoke(RUBY_CEXT, "rb_ivar_set", object, name, value);
  return value;
}

VALUE rb_ivar_lookup(VALUE object, const char *name, VALUE default_value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_ivar_lookup", object, name, default_value);
}

VALUE rb_attr_get(VALUE object, ID name) {
  return rb_ivar_lookup(object, name, Qnil);
}

// Accessing constants

int rb_const_defined(VALUE module, ID name) {
  return polyglot_as_boolean(polyglot_invoke((void *)module, "const_defined?", name));
}

int rb_const_defined_at(VALUE module, ID name) {
  return polyglot_as_boolean(polyglot_invoke((void *)module, "const_defined?", name, Qfalse));
}

VALUE rb_const_get(VALUE module, ID name) {
  return (VALUE) polyglot_invoke(module, "const_get", name);
}

VALUE rb_const_get_at(VALUE module, ID name) {
  return (VALUE) polyglot_invoke(module, "const_get", name, Qfalse);
}

VALUE rb_const_get_from(VALUE module, ID name) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_const_get_from", module, name);
}

void rb_const_set(VALUE module, ID name, VALUE value) {
  polyglot_invoke(module, "const_set", name, value);
}

void rb_define_const(VALUE module, const char *name, VALUE value) {
  rb_const_set(module, rb_str_new_cstr(name), value);
}

void rb_define_global_const(const char *name, VALUE value) {
  rb_define_const(rb_cObject, name, value);
}

// Global variables

VALUE rb_gvar_var_getter(ID id, void *data, struct rb_global_variable *gvar) {
  return *(VALUE*)data;
}

void rb_gvar_var_setter(VALUE val, ID id, void *data, struct rb_global_variable *gvar) {
  *((VALUE*)data) = val;
}

void rb_define_hooked_variable(const char *name, VALUE *var, VALUE (*getter)(ANYARGS), void (*setter)(ANYARGS)) {
  if (!getter) {
    getter = rb_gvar_var_getter;
  }

  if (!setter) {
    setter = rb_gvar_var_setter;
  }

  polyglot_invoke(RUBY_CEXT, "rb_define_hooked_variable", rb_str_new_cstr(name), var, getter, setter);
}

void rb_gvar_readonly_setter(VALUE val, ID id, void *data, struct rb_global_variable *gvar) {
  rb_raise(rb_eNameError, "read-only variable");
}

void rb_define_readonly_variable(const char *name, const VALUE *var) {
  rb_define_hooked_variable(name, (VALUE *)var, NULL, rb_gvar_readonly_setter);
}

void rb_define_variable(const char *name, VALUE *var) {
  rb_define_hooked_variable(name, var, 0, 0);
}

VALUE rb_f_global_variables(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_f_global_variables");
}

VALUE rb_gv_set(const char *name, VALUE value) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_gv_set", rb_str_new_cstr(name), value);
}

VALUE rb_gv_get(const char *name) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_gv_get", rb_str_new_cstr(name));
}

VALUE rb_lastline_get(void) {
  rb_tr_error("rb_lastline_get not implemented");
}

void rb_lastline_set(VALUE val) {
  rb_tr_error("rb_lastline_set not implemented");
}

void rb_secure(int safe_level) {
  rb_gv_set("$SAFE", INT2FIX(safe_level));
}

// Exceptions

VALUE rb_exc_new(VALUE etype, const char *ptr, long len) {
  return (VALUE) polyglot_invoke(etype, "new", rb_str_new(ptr, len));
}

VALUE rb_exc_new_cstr(VALUE exception_class, const char *message) {
  return (VALUE) polyglot_invoke(exception_class, "new", rb_str_new_cstr(message));
}

VALUE rb_exc_new_str(VALUE exception_class, VALUE message) {
  return (VALUE) polyglot_invoke(exception_class, "new", message);
}

void rb_exc_raise(VALUE exception) {
  polyglot_invoke(RUBY_CEXT, "rb_exc_raise", exception);
  rb_tr_error("rb_exc_raise should not return");
}

VALUE rb_protect(VALUE (*function)(VALUE), VALUE data, int *status) {
  VALUE ary = polyglot_invoke(RUBY_CEXT, "rb_protect_with_block",
                             (void (*)(void *)) function, data, rb_block_proc());
  *status = NUM2INT(polyglot_get_array_element(ary, 1));
  return polyglot_get_array_element(ary, 0);
}

void rb_jump_tag(int status) {
  if (status) {
    polyglot_invoke(RUBY_CEXT, "rb_jump_tag", status);
  }
  rb_tr_error("rb_jump_tag should not return");
}

void rb_set_errinfo(VALUE error) {
  polyglot_invoke(RUBY_CEXT, "rb_set_errinfo", error);
}

VALUE rb_errinfo(void) {
  return polyglot_invoke(RUBY_CEXT, "rb_errinfo");
}

void rb_syserr_fail(int eno, const char *message) {
  polyglot_invoke(RUBY_CEXT, "rb_syserr_fail", eno, message == NULL ? Qnil : rb_str_new_cstr(message));
  rb_tr_error("rb_syserr_fail should not return");
}

void rb_sys_fail(const char *message) {
  int n = errno;
  errno = 0;

  if (n == 0) {
    rb_bug("rb_sys_fail(%s) - errno == 0", message ? message : "");
  }
  rb_syserr_fail(n, message);
}

VALUE rb_ensure(VALUE (*b_proc)(ANYARGS), VALUE data1, VALUE (*e_proc)(ANYARGS), VALUE data2) {
  return polyglot_invoke(RUBY_CEXT, "rb_ensure", b_proc, data1, e_proc, data2);
}

VALUE rb_rescue(VALUE (*b_proc)(ANYARGS), VALUE data1, VALUE (*r_proc)(ANYARGS), VALUE data2) {
  return polyglot_invoke(RUBY_CEXT, "rb_rescue", b_proc, data1, r_proc, data2);
}

VALUE rb_rescue2(VALUE (*b_proc)(ANYARGS), VALUE data1, VALUE (*r_proc)(ANYARGS), VALUE data2, ...) {
  VALUE rescued = rb_ary_new();
  int n = 4;
  while (true) {
    VALUE arg = polyglot_get_arg(n);
    if (arg == NULL) {
      break;
    }
    rb_ary_push(rescued, arg);
    n++;
  }
  return polyglot_invoke(RUBY_CEXT, "rb_rescue2", b_proc, data1, r_proc, data2, rescued);
}

VALUE rb_make_backtrace(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_make_backtrace");
}

void rb_throw(const char *tag, VALUE val) {
  rb_throw_obj(rb_intern(tag), val);
}

void rb_throw_obj(VALUE tag, VALUE value) {
  polyglot_invoke(rb_mKernel, "throw", tag, value == NULL ? Qnil : value);
  rb_tr_error("rb_throw_obj should not return");
}

VALUE rb_catch(const char *tag, VALUE (*func)(), VALUE data) {
  return rb_catch_obj(rb_intern(tag), func, data);
}

VALUE rb_catch_obj(VALUE t, VALUE (*func)(), VALUE data) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_catch_obj", t, func, data);
}

void rb_memerror(void) {
  polyglot_invoke(RUBY_CEXT, "rb_memerror");
  rb_tr_error("rb_memerror should not return");
}

// Defining classes, modules and methods

VALUE rb_define_class(const char *name, VALUE superclass) {
  return rb_define_class_under(rb_cObject, name, superclass);
}

VALUE rb_define_class_under(VALUE module, const char *name, VALUE superclass) {
  return rb_define_class_id_under(module, rb_str_new_cstr(name), superclass);
}

VALUE rb_define_class_id_under(VALUE module, ID name, VALUE superclass) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_define_class_under", module, name, superclass);
}

VALUE rb_define_module(const char *name) {
  return rb_define_module_under(rb_cObject, name);
}

VALUE rb_define_module_under(VALUE module, const char *name) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_define_module_under", module, rb_str_new_cstr(name));
}

void rb_include_module(VALUE module, VALUE to_include) {
  polyglot_invoke(module, "include", to_include);
}

void rb_define_method(VALUE module, const char *name, VALUE (*function)(ANYARGS), int argc) {
  if (function == rb_f_notimplement) {
    polyglot_invoke(RUBY_CEXT, "rb_define_method_undefined", module, rb_str_new_cstr(name));
  } else {
    polyglot_invoke(RUBY_CEXT, "rb_define_method", module, rb_str_new_cstr(name), (void (*)(void *)) function, argc);
  }
}

void rb_define_private_method(VALUE module, const char *name, VALUE (*function)(ANYARGS), int argc) {
  rb_define_method(module, name, function, argc);
  polyglot_invoke(module, "private", rb_str_new_cstr(name));
}

void rb_define_protected_method(VALUE module, const char *name, VALUE (*function)(ANYARGS), int argc) {
  rb_define_method(module, name, function, argc);
  polyglot_invoke(module, "protected", rb_str_new_cstr(name));
}

void rb_define_module_function(VALUE module, const char *name, VALUE (*function)(ANYARGS), int argc) {
  rb_define_method(module, name, function, argc);
  polyglot_invoke(RUBY_CEXT, "cext_module_function", module, rb_intern(name));
}

void rb_define_global_function(const char *name, VALUE (*function)(ANYARGS), int argc) {
  rb_define_module_function(rb_mKernel, name, function, argc);
}

void rb_define_singleton_method(VALUE object, const char *name, VALUE (*function)(ANYARGS), int argc) {
  rb_define_method(polyglot_invoke(object, "singleton_class"), name, function, argc);
}

void rb_define_alias(VALUE module, const char *new_name, const char *old_name) {
  rb_alias(module, rb_str_new_cstr(new_name), rb_str_new_cstr(old_name));
}

void rb_alias(VALUE module, ID new_name, ID old_name) {
  polyglot_invoke(RUBY_CEXT, "rb_alias", module, new_name, old_name);
}

void rb_undef_method(VALUE module, const char *name) {
  rb_undef(module, rb_str_new_cstr(name));
}

void rb_undef(VALUE module, ID name) {
  polyglot_invoke(RUBY_CEXT, "rb_undef", module, name);
}

void rb_attr(VALUE ruby_class, ID name, int read, int write, int ex) {
  polyglot_invoke(RUBY_CEXT, "rb_attr", ruby_class, name, read, write, ex);
}

void rb_define_alloc_func(VALUE ruby_class, rb_alloc_func_t alloc_function) {
  polyglot_invoke(RUBY_CEXT, "rb_define_alloc_func", ruby_class, (void (*)(void *)) alloc_function);
}

void rb_undef_alloc_func(VALUE ruby_class) {
  polyglot_invoke(RUBY_CEXT, "rb_undef_alloc_func", ruby_class);
}

VALUE rb_obj_method(VALUE obj, VALUE vid) {
  return (VALUE) polyglot_invoke(obj, "method", rb_intern_str(vid));
}

// Rational

VALUE rb_Rational(VALUE num, VALUE den) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_Rational", num, den);
}

VALUE rb_rational_raw(VALUE num, VALUE den) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_rational_raw", num, den);
}

VALUE rb_rational_new(VALUE num, VALUE den) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_rational_new", num, den);
}

VALUE rb_rational_num(VALUE rat) {
  return (VALUE) polyglot_invoke(rat, "numerator");
}

VALUE rb_rational_den(VALUE rat) {
  return (VALUE) polyglot_invoke(rat, "denominator");
}

VALUE rb_flt_rationalize_with_prec(VALUE value, VALUE precision) {
  return (VALUE) polyglot_invoke(value, "rationalize", precision);
}

VALUE rb_flt_rationalize(VALUE value) {
  return (VALUE) polyglot_invoke(value, "rationalize");
}

// Complex

VALUE rb_Complex(VALUE real, VALUE imag) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_Complex", real, imag);
}

VALUE rb_complex_new(VALUE real, VALUE imag) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_complex_new", real, imag);
}

VALUE rb_complex_raw(VALUE real, VALUE imag) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_complex_raw", real, imag);
}

VALUE rb_complex_polar(VALUE r, VALUE theta) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_complex_polar", r, theta);
}

VALUE rb_complex_set_real(VALUE complex, VALUE real) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_complex_set_real", complex, real);
}

VALUE rb_complex_set_imag(VALUE complex, VALUE imag) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_complex_set_imag", complex, imag);
}

// Range

VALUE rb_range_new(VALUE beg, VALUE end, int exclude_end) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_range_new", beg, end, exclude_end);
}

VALUE rb_range_beg_len(VALUE range, long *begp, long *lenp, long len, int err) {
  long beg, end, origbeg, origend;
  VALUE b, e;
  int excl;

  if (!rb_range_values(range, &b, &e, &excl)) {
    return Qfalse;
  }
  
  beg = NUM2LONG(b);
  end = NUM2LONG(e);
  origbeg = beg;
  origend = end;
  if (beg < 0) {
    beg += len;
    if (beg < 0) {
      goto out_of_range;
    }
  }
  if (end < 0) {
    end += len;
  }
  if (!excl) {
    end++;                        /* include end point */
  }
  if (err == 0 || err == 2) {
    if (beg > len) {
      goto out_of_range;
    }
    if (end > len) {
      end = len;
    }
  }
  len = end - beg;
  if (len < 0) {
    len = 0;
  }

  *begp = beg;
  *lenp = len;
  return Qtrue;

out_of_range:
  if (err) {
    rb_raise(rb_eRangeError, "%d..%s%d out of range",
             origbeg, excl ? "." : "", origend);
  }
  return Qnil;
}

// Time

VALUE rb_time_new(time_t sec, long usec) {
  return (VALUE) polyglot_invoke(rb_cTime, "at", sec, usec);
}

VALUE rb_time_nano_new(time_t sec, long nsec) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_time_nano_new", sec, nsec);
}

VALUE rb_time_num_new(VALUE timev, VALUE off) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_time_num_new", timev, off);
}

struct timeval rb_time_interval(VALUE time_val) {
  polyglot_invoke(RUBY_CEXT, "rb_time_interval_acceptable", time_val);

  struct timeval result;

  VALUE time = rb_time_num_new(time_val, Qnil);
  result.tv_sec = polyglot_as_i64(polyglot_invoke((void *)time, "tv_sec"));
  result.tv_usec = polyglot_as_i64(polyglot_invoke((void *)time, "tv_usec"));

  return result;
}

struct timeval rb_time_timeval(VALUE time_val) {
  struct timeval result;

  VALUE time = rb_time_num_new(time_val, Qnil);
  result.tv_sec = polyglot_as_i64(polyglot_invoke((void *)time, "tv_sec"));
  result.tv_usec = polyglot_as_i64(polyglot_invoke((void *)time, "tv_usec"));

  return result;
}

struct timespec rb_time_timespec(VALUE time_val) {
  struct timespec result;

  VALUE time = rb_time_num_new(time_val, Qnil);
  result.tv_sec = polyglot_as_i64(polyglot_invoke((void *)time, "tv_sec"));
  result.tv_nsec = polyglot_as_i64(polyglot_invoke((void *)time, "tv_nsec"));

  return result;
}

VALUE rb_time_timespec_new(const struct timespec *ts, int offset) {
  VALUE is_utc = rb_boolean(offset == INT_MAX-1);
  VALUE is_local = rb_boolean(offset == INT_MAX);
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_time_timespec_new", ts->tv_sec, ts->tv_nsec, offset, is_utc, is_local);
}

void rb_timespec_now(struct timespec *ts) {
  struct timeval tv = rb_time_timeval((VALUE) polyglot_invoke(rb_cTime, "now"));
  ts->tv_sec = tv.tv_sec;
  ts->tv_nsec = tv.tv_usec * 1000;
}

// Regexp

VALUE rb_backref_get(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_backref_get");
}

VALUE rb_reg_match_pre(VALUE match) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_reg_match_pre", match);
}

VALUE rb_reg_new(const char *s, long len, int options) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_reg_new", rb_str_new(s, len), options);
}

VALUE rb_reg_new_str(VALUE s, int options) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_reg_new_str", s, options);
}

VALUE rb_reg_nth_match(int nth, VALUE match) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_reg_nth_match", nth, match);
}

int rb_reg_options(VALUE re) {
  return FIX2INT(polyglot_invoke(RUBY_CEXT, "rb_reg_options", re));
}

VALUE rb_reg_regcomp(VALUE str) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_reg_regcomp", str);
}

VALUE rb_reg_match(VALUE re, VALUE str) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_reg_match", re, str);
}

// Marshal

VALUE rb_marshal_dump(VALUE obj, VALUE port) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_marshal_dump", obj, port);
}

VALUE rb_marshal_load(VALUE port) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_marshal_load", port);
}

// Mutexes

VALUE rb_mutex_new(void) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mutex_new");
}

VALUE rb_mutex_locked_p(VALUE mutex) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mutex_locked_p", mutex);
}

VALUE rb_mutex_trylock(VALUE mutex) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mutex_trylock", mutex);
}

VALUE rb_mutex_lock(VALUE mutex) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mutex_lock", mutex);
}

VALUE rb_mutex_unlock(VALUE mutex) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mutex_unlock", mutex);
}

VALUE rb_mutex_sleep(VALUE mutex, VALUE timeout) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mutex_sleep", mutex, timeout);
}

VALUE rb_mutex_synchronize(VALUE mutex, VALUE (*func)(VALUE arg), VALUE arg) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_mutex_synchronize", mutex, func, arg);
}

// GC

void rb_gc_register_address(VALUE *address) {
}

void rb_gc_unregister_address(VALUE *address) {
  // VALUE is only ever in managed memory. So, it is already garbage collected.
}

void rb_gc_mark(VALUE ptr) {
}

void rb_gc_mark_maybe(VALUE obj) {
  rb_tr_error("rb_gc_mark_maybe not implemented");
}

VALUE rb_gc_enable() {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_gc_enable");
}

VALUE rb_gc_disable() {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_gc_disable");
}

void rb_gc(void) {
  polyglot_invoke(RUBY_CEXT, "rb_gc");
}

// Threads

void *rb_thread_call_with_gvl(gvl_call function, void *data1) {
  return function(data1);
}

void *rb_thread_call_without_gvl(gvl_call function, void *data1, rb_unblock_function_t *unblock_function, void *data2) {
  // TODO CS 9-Mar-17 polyglot_invoke escapes LLVMAddress into Ruby, which goes wrong when Ruby tries to call the callbacks
  return polyglot_invoke(RUBY_CEXT, "rb_thread_call_without_gvl", function, data1, unblock_function, data2);
}

int rb_thread_alone(void) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_thread_alone"));
}

VALUE rb_thread_current(void) {
  return (VALUE) polyglot_invoke(rb_tr_get_Thread(), "current");
}

VALUE rb_thread_local_aref(VALUE thread, ID id) {
  return (VALUE) polyglot_invoke(thread, "[]", ID2SYM(id));
}

VALUE rb_thread_local_aset(VALUE thread, ID id, VALUE val) {
  return (VALUE) polyglot_invoke(thread, "[]=", ID2SYM(id), val);
}

void rb_thread_wait_for(struct timeval time) {
  double seconds = (double)time.tv_sec + (double)time.tv_usec/1000000;
  polyglot_invoke(rb_mKernel, "sleep", seconds);
}

VALUE rb_thread_wakeup(VALUE thread) {
  return (VALUE) polyglot_invoke(thread, "wakeup");
}

VALUE rb_thread_create(VALUE (*fn)(ANYARGS), void *arg) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_thread_create", fn, arg);
}

void rb_thread_schedule(void) {
  polyglot_invoke(rb_cThread, "pass");
}

rb_nativethread_id_t rb_nativethread_self() {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_nativethread_self");
}

// IO

void rb_io_check_writable(rb_io_t *io) {
  if (!rb_tr_writable(io->mode)) {
    rb_raise(rb_eIOError, "not opened for writing");
  }
}

void rb_io_check_readable(rb_io_t *io) {
  if (!rb_tr_readable(io->mode)) {
    rb_raise(rb_eIOError, "not opened for reading");
  }
}

int rb_cloexec_dup(int oldfd) {
  rb_tr_error("rb_cloexec_dup not implemented");
}

void rb_update_max_fd(int fd) {
}

void rb_fd_fix_cloexec(int fd) {
  fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
}

int rb_io_wait_readable(int fd) {
  if (fd < 0) {
    rb_raise(rb_eIOError, "closed stream");
  }

  switch (errno) {
    case EAGAIN:
  #if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
  #endif
      rb_thread_wait_fd(fd);
      return true;

    default:
      return false;
  }
}

int rb_io_wait_writable(int fd) {
  if (fd < 0) {
    rb_raise(rb_eIOError, "closed stream");
  }

  switch (errno) {
    case EAGAIN:
  #if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
  #endif
      rb_tr_error("rb_io_wait_writable wait case not implemented");
      return true;

    default:
      return false;
  }
}

void rb_thread_wait_fd(int fd) {
  polyglot_invoke(RUBY_CEXT, "rb_thread_wait_fd", fd);
}

int rb_wait_for_single_fd(int fd, int events, struct timeval *tv) {
  long tv_sec = -1;
  long tv_usec = -1;
  if (tv != NULL) {
    tv_sec = tv->tv_sec;
    tv_usec = tv->tv_usec;
  }
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_wait_for_single_fd", fd, events, tv_sec, tv_usec));
}

NORETURN(void rb_eof_error(void)) {
  rb_tr_error("rb_eof_error not implemented");
}

VALUE rb_io_addstr(VALUE io, VALUE str) {
  // use write instead of just #<<, it's closer to what MRI does
  // and avoids stack-overflow in zlib where #<< is defined with this method
  rb_io_write(io, str);
  return io;
}

VALUE rb_io_check_io(VALUE io) {
  return rb_check_convert_type(io, T_FILE, "IO", "to_io");
}

void rb_io_check_closed(rb_io_t *fptr) {
  if (fptr->fd < 0) {
    rb_raise(rb_eIOError, "closed stream");
  }
}

VALUE rb_io_taint_check(VALUE io) {
  rb_check_frozen(io);
  return io;
}

VALUE rb_io_close(VALUE io) {
  return (VALUE) polyglot_invoke(io, "close");
}

VALUE rb_io_print(int argc, const VALUE *argv, VALUE out) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_io_print", out, rb_ary_new4(argc, argv));
}

VALUE rb_io_printf(int argc, const VALUE *argv, VALUE out) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_io_printf", out, rb_ary_new4(argc, argv));
}

VALUE rb_io_puts(int argc, const VALUE *argv, VALUE out) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_io_puts", out, rb_ary_new4(argc, argv));
}

VALUE rb_io_write(VALUE io, VALUE str) {
  return (VALUE) polyglot_invoke(io, "write", str);
}

VALUE rb_io_binmode(VALUE io) {
  return (VALUE) polyglot_invoke(io, "binmode");
}

int rb_thread_fd_writable(int fd) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_thread_fd_writable", fd));
}

int rb_cloexec_open(const char *pathname, int flags, mode_t mode) {
  int fd = open(pathname, flags, mode);
  if (fd >= 0) {
    rb_fd_fix_cloexec(fd);
  }
  return fd;
}

VALUE rb_file_open(const char *fname, const char *modestr) {
  return (VALUE) polyglot_invoke(rb_cFile, "open", rb_str_new_cstr(fname), rb_str_new_cstr(modestr));
}

VALUE rb_file_open_str(VALUE fname, const char *modestr) {
  return (VALUE) polyglot_invoke(rb_cFile, "open", fname, rb_str_new_cstr(modestr));
}

VALUE rb_get_path(VALUE object) {
  return (VALUE) polyglot_invoke(rb_cFile, "path", object);
}

int rb_tr_readable(int mode) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_tr_readable", mode));
}

int rb_tr_writable(int mode) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_tr_writable", mode));
}

MUST_INLINE
int rb_io_extract_encoding_option(VALUE opt, rb_encoding **enc_p, rb_encoding **enc2_p, int *fmode_p) {
  // TODO (pitr-ch 12-Jun-2017): review, just approximate implementation
  VALUE encoding = rb_cEncoding;
  VALUE external_encoding = polyglot_invoke(encoding, "default_external");
  VALUE internal_encoding = polyglot_invoke(encoding, "default_internal");
  if (!NIL_P(external_encoding)) {
    *enc_p = rb_tr_handle_for_managed_leaking(rb_to_encoding(external_encoding));
  }
  if (!NIL_P(internal_encoding)) {
    *enc2_p = rb_tr_handle_for_managed_leaking(rb_to_encoding(internal_encoding));
  }
  return 1;
}

// Structs

VALUE rb_struct_aref(VALUE s, VALUE idx) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_struct_aref", s, idx);
}

VALUE rb_struct_aset(VALUE s, VALUE idx, VALUE val) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_struct_aset", s, idx, val);
}

VALUE rb_struct_define(const char *name, ...) {
  VALUE *rb_name = name == NULL ? polyglot_invoke(RUBY_CEXT, "Qnil") : rb_str_new_cstr(name);
  VALUE *ary = rb_ary_new();
  int i = 0;
  char *arg = NULL;
  while ((arg = (char *)polyglot_get_arg(i + 1)) != NULL) {
    rb_ary_store(ary, i++, rb_str_new_cstr(arg));
  }
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_struct_define_no_splat", rb_name, ary);
}

VALUE rb_struct_new(VALUE klass, ...) {
  int members = polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_struct_size", klass));
  VALUE *ary = rb_ary_new();
  int i = 0;
  char *arg = NULL;
  while (i < members) {
    VALUE arg = polyglot_get_arg(i + 1);
    rb_ary_store(ary, i++, arg);
  }
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_struct_new_no_splat", klass, ary);
}

VALUE rb_struct_getmember(VALUE obj, ID id) {
  rb_tr_error("rb_struct_getmember not implemented");
}

VALUE rb_struct_s_members(VALUE klass) {
  rb_tr_error("rb_struct_s_members not implemented");
}

VALUE rb_struct_members(VALUE s) {
  rb_tr_error("rb_struct_members not implemented");
}

VALUE rb_struct_define_under(VALUE outer, const char *name, ...) {
  rb_tr_error("rb_struct_define_under not implemented");
}

// Data

POLYGLOT_DECLARE_STRUCT(RData)

static void *to_free_function(RUBY_DATA_FUNC dfree) {
  // TODO CS 26-Mar-18 we should be able to write this but it doesn't work - you get an LLVMTruffleAddress
  // return (RUBY_DATA_FUNC) dfree;
  return truffle_address_to_function(dfree);
}

struct RData *RDATA(VALUE value) {
  return polyglot_as_RData(polyglot_invoke(RUBY_CEXT, "RDATA", value));
}

VALUE rb_data_object_wrap(VALUE klass, void *datap, RUBY_DATA_FUNC dmark, RUBY_DATA_FUNC dfree) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_data_object_wrap", klass, datap, dmark, to_free_function(dfree));
}

VALUE rb_data_object_zalloc(VALUE klass, size_t size, RUBY_DATA_FUNC dmark, RUBY_DATA_FUNC dfree) {
  VALUE obj = rb_data_object_wrap(klass, 0, dmark, dfree);
  DATA_PTR(obj) = xcalloc(1, size);
  return obj;
}

// Typed data

VALUE rb_data_typed_object_wrap(VALUE ruby_class, void *data, const rb_data_type_t *data_type) {
  return (VALUE) polyglot_invoke(RUBY_CEXT, "rb_data_typed_object_wrap", ruby_class, data, data_type, to_free_function(data_type->function.dfree));
}

VALUE rb_data_typed_object_zalloc(VALUE ruby_class, size_t size, const rb_data_type_t *data_type) {
  VALUE obj = rb_data_typed_object_wrap(ruby_class, 0, data_type);
  DATA_PTR(obj) = calloc(1, size);
  return obj;
}

VALUE rb_data_typed_object_make(VALUE ruby_class, const rb_data_type_t *type, void **data_pointer, size_t size) {
  TypedData_Make_Struct0(result, ruby_class, void, size, type, *data_pointer);
  return result;
}

void *rb_check_typeddata(VALUE value, const rb_data_type_t *data_type) {
  if (rb_tr_hidden_variable_get(value, "data_type") != data_type) {
    rb_raise(rb_eTypeError, "wrong argument type");
  }
  return RTYPEDDATA_DATA(value);
}

// VM

VALUE rb_tr_ruby_verbose_ptr;

VALUE *rb_ruby_verbose_ptr(void) {
  rb_tr_ruby_verbose_ptr = polyglot_invoke(RUBY_CEXT, "rb_ruby_verbose_ptr");
  return &rb_tr_ruby_verbose_ptr;
}

VALUE rb_tr_ruby_debug_ptr;

VALUE *rb_ruby_debug_ptr(void) {
  rb_tr_ruby_debug_ptr = polyglot_invoke(RUBY_CEXT, "rb_ruby_debug_ptr");
  return &rb_tr_ruby_debug_ptr;
}

// Non-standard

void rb_tr_error(const char *message) {
  polyglot_invoke(RUBY_CEXT, "rb_tr_error", rb_str_new_cstr(message));
  abort();
}

void rb_tr_log_warning(const char *message) {
  polyglot_invoke(RUBY_CEXT, "rb_tr_log_warning", rb_str_new_cstr(message));
}

long rb_tr_obj_id(VALUE object) {
  return polyglot_as_i64(polyglot_invoke(RUBY_CEXT, "rb_tr_obj_id", object));
}

VALUE rb_java_class_of(VALUE obj) {
  return polyglot_invoke(RUBY_CEXT, "rb_java_class_of", obj);
}

VALUE rb_java_to_string(VALUE obj) {
  return polyglot_invoke(RUBY_CEXT, "rb_java_to_string", obj);
}

void *rb_tr_handle_for_managed(VALUE managed) {
  return truffle_handle_for_managed(managed);
}

void *rb_tr_handle_for_managed_leaking(VALUE managed) {
  rb_tr_log_warning("rb_tr_handle_for_managed without matching rb_tr_release_handle; handles will be leaking");
  return rb_tr_handle_for_managed(managed);
}

void *rb_tr_handle_if_managed(VALUE pointer) {
  if (polyglot_is_value(pointer)) {
    return rb_tr_handle_for_managed(pointer);
  } else {
    return pointer;
  }
}

void *rb_tr_handle_if_managed_leaking(VALUE pointer) {
  if (polyglot_is_value(pointer)) {
    return rb_tr_handle_if_managed_leaking(pointer);
  } else {
    return pointer;
  }
}

VALUE rb_tr_managed_from_handle_or_null(void *handle) {
  if (handle == NULL) {
    return NULL;
  } else {
    return rb_tr_managed_from_handle(handle);
  }
}

VALUE rb_tr_managed_if_handle(void *pointer) {
  if (truffle_is_handle_to_managed(pointer)) {
    return rb_tr_managed_from_handle(pointer);
  } else {
    return pointer;
  }
}

VALUE rb_tr_managed_from_handle(void *handle) {
  return truffle_managed_from_handle(handle);
}

void rb_tr_release_if_handle(void *pointer) {
  if (truffle_is_handle_to_managed(pointer)) {
    truffle_release_handle(pointer);
  }
}

void rb_tr_release_handle(void *handle) {
  truffle_release_handle(handle);
}

void rb_tr_load_library(VALUE library) {
  truffle_load_library(RSTRING_PTR(library));
}

void rb_big_2comp(VALUE x) {
  rb_tr_error("rb_big_2comp not implemented");
}

int rb_profile_frames(int start, int limit, VALUE *buff, int *lines) {
  rb_tr_error("rb_profile_frames not implemented");
}

VALUE rb_profile_frame_path(VALUE frame) {
  rb_tr_error("rb_profile_frame_path not implemented");
}

VALUE rb_profile_frame_absolute_path(VALUE frame) {
  rb_tr_error("rb_profile_frame_absolute_path not implemented");
}

VALUE rb_profile_frame_label(VALUE frame) {
  rb_tr_error("rb_profile_frame_label not implemented");
}

VALUE rb_profile_frame_base_label(VALUE frame) {
  rb_tr_error("rb_profile_frame_base_label not implemented");
}

VALUE rb_profile_frame_full_label(VALUE frame) {
  rb_tr_error("rb_profile_frame_full_label not implemented");
}

VALUE rb_profile_frame_first_lineno(VALUE frame) {
  rb_tr_error("rb_profile_frame_first_lineno not implemented");
}

VALUE rb_profile_frame_classpath(VALUE frame) {
  rb_tr_error("rb_profile_frame_classpath not implemented");
}

VALUE rb_profile_frame_singleton_method_p(VALUE frame) {
  rb_tr_error("rb_profile_frame_singleton_method_p not implemented");
}

VALUE rb_profile_frame_method_name(VALUE frame) {
  rb_tr_error("rb_profile_frame_method_name not implemented");
}

VALUE rb_profile_frame_qualified_method_name(VALUE frame) {
  rb_tr_error("rb_profile_frame_qualified_method_name not implemented");
}

VALUE rb_debug_inspector_open(rb_debug_inspector_func_t func, void *data) {
  rb_tr_error("rb_debug_inspector_open not implemented");
}

VALUE rb_debug_inspector_frame_self_get(const rb_debug_inspector_t *dc, long index) {
  rb_tr_error("rb_debug_inspector_frame_self_get not implemented");
}

VALUE rb_debug_inspector_frame_class_get(const rb_debug_inspector_t *dc, long index) {
  rb_tr_error("rb_debug_inspector_frame_class_get not implemented");
}

VALUE rb_debug_inspector_frame_binding_get(const rb_debug_inspector_t *dc, long index) {
  rb_tr_error("rb_debug_inspector_frame_binding_get not implemented");
}

VALUE rb_debug_inspector_frame_iseq_get(const rb_debug_inspector_t *dc, long index) {
  rb_tr_error("rb_debug_inspector_frame_iseq_get not implemented");
}

VALUE rb_debug_inspector_backtrace_locations(const rb_debug_inspector_t *dc) {
  rb_tr_error("rb_debug_inspector_backtrace_locations not implemented");
}

void rb_add_event_hook(rb_event_hook_func_t func, rb_event_flag_t events, VALUE data) {
  rb_tr_error("rb_add_event_hook not implemented");
}

int rb_remove_event_hook(rb_event_hook_func_t func) {
  rb_tr_error("rb_remove_event_hook not implemented");
}

int rb_remove_event_hook_with_data(rb_event_hook_func_t func, VALUE data) {
  rb_tr_error("rb_remove_event_hook_with_data not implemented");
}

void rb_thread_add_event_hook(VALUE thval, rb_event_hook_func_t func, rb_event_flag_t events, VALUE data) {
  rb_tr_error("rb_thread_add_event_hook not implemented");
}

int rb_thread_remove_event_hook(VALUE thval, rb_event_hook_func_t func) {
  rb_tr_error("rb_thread_remove_event_hook not implemented");
}

int rb_thread_remove_event_hook_with_data(VALUE thval, rb_event_hook_func_t func, VALUE data) {
  rb_tr_error("rb_thread_remove_event_hook_with_data not implemented");
}

VALUE rb_tracepoint_new(VALUE target_thval, rb_event_flag_t events, void (*func)(VALUE, void *), void *data) {
  rb_tr_error("rb_tracepoint_new not implemented");
}

VALUE rb_tracepoint_enable(VALUE tpval) {
  rb_tr_error("rb_tracepoint_enable not implemented");
}

VALUE rb_tracepoint_disable(VALUE tpval) {
  rb_tr_error("rb_tracepoint_disable not implemented");
}

VALUE rb_tracepoint_enabled_p(VALUE tpval) {
  rb_tr_error("rb_tracepoint_enabled_p not implemented");
}

rb_event_flag_t rb_tracearg_event_flag(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_event_flag not implemented");
}

VALUE rb_tracearg_event(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_event not implemented");
}

VALUE rb_tracearg_lineno(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_lineno not implemented");
}

VALUE rb_tracearg_path(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_path not implemented");
}

VALUE rb_tracearg_method_id(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_method_id not implemented");
}

VALUE rb_tracearg_defined_class(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_defined_class not implemented");
}

VALUE rb_tracearg_binding(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_binding not implemented");
}

VALUE rb_tracearg_self(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_self not implemented");
}

VALUE rb_tracearg_return_value(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_return_value not implemented");
}

VALUE rb_tracearg_raised_exception(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_raised_exception not implemented");
}

VALUE rb_tracearg_object(rb_trace_arg_t *trace_arg) {
  rb_tr_error("rb_tracearg_object not implemented");
}

int rb_postponed_job_register(unsigned int flags, rb_postponed_job_func_t func, void *data) {
  rb_tr_error("rb_postponed_job_register not implemented");
}

int rb_postponed_job_register_one(unsigned int flags, rb_postponed_job_func_t func, void *data) {
  rb_tr_error("rb_postponed_job_register_one not implemented");
}

void rb_add_event_hook2(rb_event_hook_func_t func, rb_event_flag_t events, VALUE data, rb_event_hook_flag_t hook_flags) {
  rb_tr_error("rb_add_event_hook2 not implemented");
}

void rb_thread_add_event_hook2(VALUE thval, rb_event_hook_func_t func, rb_event_flag_t events, VALUE data, rb_event_hook_flag_t hook_flags) {
  rb_tr_error("rb_thread_add_event_hook2 not implemented");
}

void rb_sparc_flush_register_windows(void) {
  rb_tr_error("rb_sparc_flush_register_windows not implemented");
}

int rb_char_to_option_kcode(int c, int *option, int *kcode) {
  rb_tr_error("rb_char_to_option_kcode not implemented");
}

int rb_enc_replicate(const char *name, rb_encoding *encoding) {
  rb_tr_error("rb_enc_replicate not implemented");
}

int rb_define_dummy_encoding(const char *name) {
  rb_tr_error("rb_define_dummy_encoding not implemented");
}

#undef rb_enc_str_new_cstr
VALUE rb_enc_str_new_cstr(const char *ptr, rb_encoding *enc) {
  if (rb_enc_mbminlen(enc) != 1) {
    rb_raise(rb_eArgError, "wchar encoding given");
  }

  VALUE string = rb_str_new_cstr(ptr);
  rb_enc_associate(string, enc);
  return string;
}

VALUE rb_enc_str_new_static(const char *ptr, long len, rb_encoding *enc) {
  if (len < 0) {
    rb_raise(rb_eArgError, "negative string size (or size too big)");
  }

  VALUE string = rb_enc_str_new(ptr, len, enc);
  return string;
}

VALUE rb_enc_reg_new(const char *s, long len, rb_encoding *enc, int options) {
  rb_tr_error("rb_enc_reg_new not implemented");
}

VALUE rb_enc_vsprintf(rb_encoding *enc, const char *format, va_list args) {
  // TODO CS 7-May-17 this needs to use the Ruby sprintf, not C's
  char *buffer;
  if (vasprintf(&buffer, format, args) < 0) {
    rb_tr_error("vasprintf error");
  }
  VALUE string = rb_enc_str_new_cstr(buffer, enc);
  free(buffer);
  return string;
}

long rb_enc_strlen(const char *p, const char *e, rb_encoding *enc) {
  rb_tr_error("rb_enc_strlen not implemented");
}

VALUE rb_enc_str_buf_cat(VALUE str, const char *ptr, long len, rb_encoding *ptr_enc) {
  rb_tr_error("rb_enc_str_buf_cat not implemented");
}

VALUE rb_enc_uint_chr(unsigned int code, rb_encoding *enc) {
  rb_tr_error("rb_enc_uint_chr not implemented");
}

int rb_enc_fast_mbclen(const char *p, const char *e, rb_encoding *enc) {
  rb_tr_error("rb_enc_fast_mbclen not implemented");
}

int rb_enc_ascget(const char *p, const char *e, int *len, rb_encoding *enc) {
  rb_tr_error("rb_enc_ascget not implemented");
}

int rb_enc_codelen(int c, rb_encoding *enc) {
  rb_tr_error("rb_enc_codelen not implemented");
}

#undef rb_enc_code_to_mbclen
int rb_enc_code_to_mbclen(int code, rb_encoding *enc) {
  rb_tr_error("rb_enc_code_to_mbclen not implemented");
}

int rb_enc_toupper(int c, rb_encoding *enc) {
  rb_tr_error("rb_enc_toupper not implemented");
}

int rb_enc_tolower(int c, rb_encoding *enc) {
  rb_tr_error("rb_enc_tolower not implemented");
}

int rb_enc_symname_p(const char *name, rb_encoding *enc) {
  rb_tr_error("rb_enc_symname_p not implemented");
}

int rb_enc_symname2_p(const char *name, long len, rb_encoding *enc) {
  rb_tr_error("rb_enc_symname2_p not implemented");
}

long rb_str_coderange_scan_restartable(const char *s, const char *e, rb_encoding *enc, int *cr) {
  rb_tr_error("rb_str_coderange_scan_restartable not implemented");
}

int rb_enc_str_asciionly_p(VALUE str) {
  rb_tr_error("rb_enc_str_asciionly_p not implemented");
}

int rb_enc_unicode_p(rb_encoding *enc) {
  rb_tr_error("rb_enc_unicode_p not implemented");
}

VALUE rb_enc_default_external(void) {
  rb_tr_error("rb_enc_default_external not implemented");
}

VALUE rb_enc_default_internal(void) {
  rb_tr_error("rb_enc_default_internal not implemented");
}

void rb_enc_set_default_external(VALUE encoding) {
  rb_tr_error("rb_enc_set_default_external not implemented");
}

void rb_enc_set_default_internal(VALUE encoding) {
  rb_tr_error("rb_enc_set_default_internal not implemented");
}

VALUE rb_locale_charmap(VALUE klass) {
  rb_tr_error("rb_locale_charmap not implemented");
}

long rb_memsearch(const void *x0, long m, const void *y0, long n, rb_encoding *enc) {
  rb_tr_error("rb_memsearch not implemented");
}

ID rb_check_id_cstr(const char *ptr, long len, rb_encoding *enc) {
  rb_tr_error("rb_check_id_cstr not implemented");
}

VALUE rb_check_symbol_cstr(const char *ptr, long len, rb_encoding *enc) {
  rb_tr_error("rb_check_symbol_cstr not implemented");
}

int rb_econv_has_convpath_p(const char* from_encoding, const char* to_encoding) {
  rb_tr_error("rb_econv_has_convpath_p not implemented");
}

int rb_econv_prepare_options(VALUE opthash, VALUE *opts, int ecflags) {
  rb_tr_error("rb_econv_prepare_options not implemented");
}

int rb_econv_set_replacement(rb_econv_t *ec, const unsigned char *str, size_t len, const char *encname) {
  rb_tr_error("rb_econv_set_replacement not implemented");
}

int rb_econv_decorate_at_first(rb_econv_t *ec, const char *decorator_name) {
  rb_tr_error("rb_econv_decorate_at_first not implemented");
}

int rb_econv_decorate_at_last(rb_econv_t *ec, const char *decorator_name) {
  rb_tr_error("rb_econv_decorate_at_last not implemented");
}

VALUE rb_econv_open_exc(const char *sname, const char *dname, int ecflags) {
  rb_tr_error("rb_econv_open_exc not implemented");
}

int rb_econv_insert_output(rb_econv_t *ec, const unsigned char *str, size_t len, const char *str_encoding) {
  rb_tr_error("rb_econv_insert_output not implemented");
}

VALUE rb_econv_make_exception(rb_econv_t *ec) {
  rb_tr_error("rb_econv_make_exception not implemented");
}

int rb_econv_putbackable(rb_econv_t *ec) {
  rb_tr_error("rb_econv_putbackable not implemented");
}

void rb_econv_putback(rb_econv_t *ec, unsigned char *p, int n) {
  rb_tr_error("rb_econv_putback not implemented");
}

VALUE rb_econv_substr_convert(rb_econv_t *ec, VALUE src, long byteoff, long bytesize, int flags) {
  rb_tr_error("rb_econv_substr_convert not implemented");
}

VALUE rb_econv_str_append(rb_econv_t *ec, VALUE src, VALUE dst, int flags) {
  rb_tr_error("rb_econv_str_append not implemented");
}

VALUE rb_econv_substr_append(rb_econv_t *ec, VALUE src, long off, long len, VALUE dst, int flags) {
  rb_tr_error("rb_econv_substr_append not implemented");
}

VALUE rb_econv_append(rb_econv_t *ec, const char *ss, long len, VALUE dst, int flags) {
  rb_tr_error("rb_econv_append not implemented");
}

void rb_econv_binmode(rb_econv_t *ec) {
  rb_tr_error("rb_econv_binmode not implemented");
}

VALUE rb_ary_tmp_new(long capa) {
  rb_tr_error("rb_ary_tmp_new not implemented");
}

void rb_ary_free(VALUE ary) {
  rb_tr_error("rb_ary_free not implemented");
}

void rb_ary_modify(VALUE ary) {
  rb_tr_error("rb_ary_modify not implemented");
}

VALUE rb_ary_shared_with_p(VALUE ary1, VALUE ary2) {
  rb_tr_error("rb_ary_shared_with_p not implemented");
}

VALUE rb_ary_resurrect(VALUE ary) {
  rb_tr_error("rb_ary_resurrect not implemented");
}

VALUE rb_ary_sort(VALUE ary) {
  rb_tr_error("rb_ary_sort not implemented");
}

VALUE rb_ary_sort_bang(VALUE ary) {
  rb_tr_error("rb_ary_sort_bang not implemented");
}

VALUE rb_ary_assoc(VALUE ary, VALUE key) {
  rb_tr_error("rb_ary_assoc not implemented");
}

VALUE rb_ary_rassoc(VALUE ary, VALUE value) {
  rb_tr_error("rb_ary_rassoc not implemented");
}

VALUE rb_ary_cmp(VALUE ary1, VALUE ary2) {
  rb_tr_error("rb_ary_cmp not implemented");
}

VALUE rb_ary_replace(VALUE copy, VALUE orig) {
  rb_tr_error("rb_ary_replace not implemented");
}

VALUE rb_get_values_at(VALUE obj, long olen, int argc, const VALUE *argv, VALUE (*func) (VALUE, long)) {
  rb_tr_error("rb_get_values_at not implemented");
}

VALUE rb_big_new(size_t len, int sign) {
  rb_tr_error("rb_big_new not implemented");
}

int rb_bigzero_p(VALUE x) {
  rb_tr_error("rb_bigzero_p not implemented");
}

VALUE rb_big_clone(VALUE x) {
  rb_tr_error("rb_big_clone not implemented");
}

VALUE rb_big_norm(VALUE x) {
  rb_tr_error("rb_big_norm not implemented");
}

void rb_big_resize(VALUE big, size_t len) {
  rb_tr_error("rb_big_resize not implemented");
}

VALUE rb_big_unpack(unsigned long *buf, long num_longs) {
  rb_tr_error("rb_big_unpack not implemented");
}

int rb_uv_to_utf8(char buf[6], unsigned long uv) {
  rb_tr_error("rb_uv_to_utf8 not implemented");
}

VALUE rb_big_eq(VALUE x, VALUE y) {
  rb_tr_error("rb_big_eq not implemented");
}

VALUE rb_big_eql(VALUE x, VALUE y) {
  rb_tr_error("rb_big_eql not implemented");
}

VALUE rb_big_plus(VALUE x, VALUE y) {
  rb_tr_error("rb_big_plus not implemented");
}

VALUE rb_big_minus(VALUE x, VALUE y) {
  rb_tr_error("rb_big_minus not implemented");
}

VALUE rb_big_mul(VALUE x, VALUE y) {
  rb_tr_error("rb_big_mul not implemented");
}

VALUE rb_big_div(VALUE x, VALUE y) {
  rb_tr_error("rb_big_div not implemented");
}

VALUE rb_big_idiv(VALUE x, VALUE y) {
  rb_tr_error("rb_big_idiv not implemented");
}

VALUE rb_big_modulo(VALUE x, VALUE y) {
  rb_tr_error("rb_big_modulo not implemented");
}

VALUE rb_big_divmod(VALUE x, VALUE y) {
  rb_tr_error("rb_big_divmod not implemented");
}

VALUE rb_big_pow(VALUE x, VALUE y) {
  rb_tr_error("rb_big_pow not implemented");
}

VALUE rb_big_and(VALUE x, VALUE y) {
  rb_tr_error("rb_big_and not implemented");
}

VALUE rb_big_or(VALUE x, VALUE y) {
  rb_tr_error("rb_big_or not implemented");
}

VALUE rb_big_xor(VALUE x, VALUE y) {
  rb_tr_error("rb_big_xor not implemented");
}

VALUE rb_big_lshift(VALUE x, VALUE y) {
  rb_tr_error("rb_big_lshift not implemented");
}

VALUE rb_big_rshift(VALUE x, VALUE y) {
  rb_tr_error("rb_big_rshift not implemented");
}

VALUE rb_big_hash(VALUE x) {
  rb_tr_error("rb_big_hash not implemented");
}

size_t rb_absint_numwords(VALUE val, size_t word_numbits, size_t *nlz_bits_ret) {
  rb_tr_error("rb_absint_numwords not implemented");
}

int rb_absint_singlebit_p(VALUE val) {
  return polyglot_as_i32(polyglot_invoke(RUBY_CEXT, "rb_absint_singlebit_p", val));
}

VALUE rb_class_boot(VALUE super) {
  rb_tr_error("rb_class_boot not implemented");
}

VALUE rb_mod_init_copy(VALUE clone, VALUE orig) {
  rb_tr_error("rb_mod_init_copy not implemented");
}

VALUE rb_singleton_class_clone(VALUE obj) {
  rb_tr_error("rb_singleton_class_clone not implemented");
}

void rb_singleton_class_attached(VALUE klass, VALUE obj) {
  rb_tr_error("rb_singleton_class_attached not implemented");
}

VALUE rb_make_metaclass(VALUE obj, VALUE unused) {
  rb_tr_error("rb_make_metaclass not implemented");
}

void rb_check_inheritable(VALUE super) {
  rb_tr_error("rb_check_inheritable not implemented");
}

VALUE rb_class_inherited(VALUE super, VALUE klass) {
  rb_tr_error("rb_class_inherited not implemented");
}

VALUE rb_define_class_id(ID id, VALUE super) {
  rb_tr_error("rb_define_class_id not implemented");
}

VALUE rb_module_new(void) {
  rb_tr_error("rb_module_new not implemented");
}

VALUE rb_define_module_id(ID id) {
  rb_tr_error("rb_define_module_id not implemented");
}

VALUE rb_define_module_id_under(VALUE outer, ID id) {
  rb_tr_error("rb_define_module_id_under not implemented");
}

VALUE rb_include_class_new(VALUE module, VALUE super) {
  rb_tr_error("rb_include_class_new not implemented");
}

VALUE rb_mod_included_modules(VALUE mod) {
  rb_tr_error("rb_mod_included_modules not implemented");
}

VALUE rb_mod_include_p(VALUE mod, VALUE mod2) {
  rb_tr_error("rb_mod_include_p not implemented");
}

VALUE rb_class_instance_methods(int argc, const VALUE *argv, VALUE mod) {
  rb_tr_error("rb_class_instance_methods not implemented");
}

VALUE rb_class_public_instance_methods(int argc, const VALUE *argv, VALUE mod) {
  rb_tr_error("rb_class_public_instance_methods not implemented");
}

VALUE rb_class_protected_instance_methods(int argc, const VALUE *argv, VALUE mod) {
  rb_tr_error("rb_class_protected_instance_methods not implemented");
}

VALUE rb_class_private_instance_methods(int argc, const VALUE *argv, VALUE mod) {
  rb_tr_error("rb_class_private_instance_methods not implemented");
}

VALUE rb_obj_singleton_methods(int argc, const VALUE *argv, VALUE obj) {
  rb_tr_error("rb_obj_singleton_methods not implemented");
}

void rb_define_method_id(VALUE klass, ID mid, VALUE (*func)(ANYARGS), int argc) {
  rb_tr_error("rb_define_method_id not implemented");
}

void rb_frozen_class_p(VALUE klass) {
  rb_tr_error("rb_frozen_class_p not implemented");
}

VALUE rb_fiber_new(VALUE (*func)(ANYARGS), VALUE obj) {
  rb_tr_error("rb_fiber_new not implemented");
}

VALUE rb_fiber_resume(VALUE fibval, int argc, const VALUE *argv) {
  rb_tr_error("rb_fiber_resume not implemented");
}

VALUE rb_fiber_yield(int argc, const VALUE *argv) {
  rb_tr_error("rb_fiber_yield not implemented");
}

VALUE rb_fiber_current(void) {
  rb_tr_error("rb_fiber_current not implemented");
}

VALUE rb_fiber_alive_p(VALUE fibval) {
  rb_tr_error("rb_fiber_alive_p not implemented");
}

VALUE rb_enum_values_pack(int argc, const VALUE *argv) {
  rb_tr_error("rb_enum_values_pack not implemented");
}

void rb_error_untrusted(VALUE obj) {
  rb_tr_error("rb_error_untrusted not implemented");
}

void rb_check_trusted(VALUE obj) {
  rb_tr_error("rb_check_trusted not implemented");
}

void rb_check_copyable(VALUE obj, VALUE orig) {
  rb_tr_error("rb_check_copyable not implemented");
}

VALUE rb_check_funcall(VALUE recv, ID mid, int argc, const VALUE *argv) {
  rb_tr_error("rb_check_funcall not implemented");
}

void rb_fd_init(rb_fdset_t *set) {
  rb_tr_error("rb_fd_init not implemented");
}

void rb_fd_term(rb_fdset_t *set) {
  rb_tr_error("rb_fd_term not implemented");
}

void rb_fd_zero(rb_fdset_t *fds) {
  rb_tr_error("rb_fd_zero not implemented");
}

void rb_fd_set(int fd, rb_fdset_t *set) {
  rb_tr_error("rb_fd_set not implemented");
}

void rb_fd_clr(int n, rb_fdset_t *fds) {
  rb_tr_error("rb_fd_clr not implemented");
}

int rb_fd_isset(int n, const rb_fdset_t *fds) {
  rb_tr_error("rb_fd_isset not implemented");
}

void rb_fd_copy(rb_fdset_t *dst, const fd_set *src, int max) {
  rb_tr_error("rb_fd_copy not implemented");
}

void rb_fd_dup(rb_fdset_t *dst, const rb_fdset_t *src) {
  rb_tr_error("rb_fd_dup not implemented");
}

int rb_fd_select(int n, rb_fdset_t *readfds, rb_fdset_t *writefds, rb_fdset_t *exceptfds, struct timeval *timeout) {
  rb_tr_error("rb_fd_select not implemented");
}

void rb_w32_fd_copy(rb_fdset_t *dst, const fd_set *src, int max) {
  rb_tr_error("rb_w32_fd_copy not implemented");
}

void rb_w32_fd_dup(rb_fdset_t *dst, const rb_fdset_t *src) {
  rb_tr_error("rb_w32_fd_dup not implemented");
}

VALUE rb_f_exit(int argc, const VALUE *argv) {
  rb_tr_error("rb_f_exit not implemented");
}

VALUE rb_f_abort(int argc, const VALUE *argv) {
  rb_tr_error("rb_f_abort not implemented");
}

void rb_remove_method(VALUE klass, const char *name) {
  rb_tr_error("rb_remove_method not implemented");
}

void rb_remove_method_id(VALUE klass, ID mid) {
  rb_tr_error("rb_remove_method_id not implemented");
}

rb_alloc_func_t rb_get_alloc_func(VALUE klass) {
  rb_tr_error("rb_get_alloc_func not implemented");
}

void rb_clear_constant_cache(void) {
  rb_tr_error("rb_clear_constant_cache not implemented");
}

void rb_clear_method_cache_by_class(VALUE klass) {
  rb_tr_error("rb_clear_method_cache_by_class not implemented");
}

int rb_method_basic_definition_p(VALUE klass, ID id) {
  rb_tr_error("rb_method_basic_definition_p not implemented");
}

VALUE rb_eval_cmd(VALUE cmd, VALUE arg, int level) {
  rb_tr_error("rb_eval_cmd not implemented");
}

void rb_interrupt(void) {
  rb_tr_error("rb_interrupt not implemented");
}

void rb_backtrace(void) {
  rb_tr_error("rb_backtrace not implemented");
}

ID rb_frame_this_func(void) {
  rb_tr_error("rb_frame_this_func not implemented");
}

VALUE rb_obj_instance_exec(int argc, const VALUE *argv, VALUE self) {
  rb_tr_error("rb_obj_instance_exec not implemented");
}

VALUE rb_mod_module_eval(int argc, const VALUE *argv, VALUE mod) {
  rb_tr_error("rb_mod_module_eval not implemented");
}

VALUE rb_mod_module_exec(int argc, const VALUE *argv, VALUE mod) {
  rb_tr_error("rb_mod_module_exec not implemented");
}

void rb_load(VALUE fname, int wrap) {
  rb_tr_error("rb_load not implemented");
}

void rb_load_protect(VALUE fname, int wrap, int *state) {
  rb_tr_error("rb_load_protect not implemented");
}

int rb_provided(const char *feature) {
  rb_tr_error("rb_provided not implemented");
}

int rb_feature_provided(const char *feature, const char **loading) {
  rb_tr_error("rb_feature_provided not implemented");
}

void rb_provide(const char *feature) {
  rb_tr_error("rb_provide not implemented");
}

VALUE rb_f_require(VALUE obj, VALUE fname) {
  rb_tr_error("rb_f_require not implemented");
}

VALUE rb_require_safe(VALUE fname, int safe) {
  rb_tr_error("rb_require_safe not implemented");
}

VALUE rb_obj_is_proc(VALUE proc) {
  rb_tr_error("rb_obj_is_proc not implemented");
}

VALUE rb_proc_call_with_block(VALUE self, int argc, const VALUE *argv, VALUE pass_procval) {
  rb_tr_error("rb_proc_call_with_block not implemented");
}

VALUE rb_proc_lambda_p(VALUE procval) {
  rb_tr_error("rb_proc_lambda_p not implemented");
}

VALUE rb_binding_new(void) {
  rb_tr_error("rb_binding_new not implemented");
}

VALUE rb_obj_is_method(VALUE m) {
  rb_tr_error("rb_obj_is_method not implemented");
}

VALUE rb_method_call(int argc, const VALUE *argv, VALUE method) {
  rb_tr_error("rb_method_call not implemented");
}

VALUE rb_method_call_with_block(int argc, const VALUE *argv, VALUE method, VALUE pass_procval) {
  rb_tr_error("rb_method_call_with_block not implemented");
}

int rb_mod_method_arity(VALUE mod, ID id) {
  rb_tr_error("rb_mod_method_arity not implemented");
}

void rb_exec_end_proc(void) {
  rb_tr_error("rb_exec_end_proc not implemented");
}

void rb_thread_fd_close(int fd) {
  rb_tr_error("rb_thread_fd_close not implemented");
}

void rb_thread_sleep(int sec) {
  rb_tr_error("rb_thread_sleep not implemented");
}

void rb_thread_sleep_forever(void) {
  rb_tr_error("rb_thread_sleep_forever not implemented");
}

void rb_thread_sleep_deadly(void) {
  rb_tr_error("rb_thread_sleep_deadly not implemented");
}

VALUE rb_thread_stop(void) {
  rb_tr_error("rb_thread_stop not implemented");
}

VALUE rb_thread_wakeup_alive(VALUE thread) {
  rb_tr_error("rb_thread_wakeup_alive not implemented");
}

VALUE rb_thread_run(VALUE thread) {
  rb_tr_error("rb_thread_run not implemented");
}

VALUE rb_thread_kill(VALUE thread) {
  rb_tr_error("rb_thread_kill not implemented");
}

int rb_thread_fd_select(int max, rb_fdset_t * read, rb_fdset_t * write, rb_fdset_t * except, struct timeval *timeout) {
  rb_tr_error("rb_thread_fd_select not implemented");
}

VALUE rb_thread_main(void) {
  rb_tr_error("rb_thread_main not implemented");
}

void rb_thread_atfork(void) {
  rb_tr_error("rb_thread_atfork not implemented");
}

void rb_thread_atfork_before_exec(void) {
  rb_tr_error("rb_thread_atfork_before_exec not implemented");
}

VALUE rb_exec_recursive_paired(VALUE (*func) (VALUE, VALUE, int), VALUE obj, VALUE paired_obj, VALUE arg) {
  rb_tr_error("rb_exec_recursive_paired not implemented");
}

VALUE rb_exec_recursive_outer(VALUE (*func) (VALUE, VALUE, int), VALUE obj, VALUE arg) {
  rb_tr_error("rb_exec_recursive_outer not implemented");
}

VALUE rb_exec_recursive_paired_outer(VALUE (*func) (VALUE, VALUE, int), VALUE obj, VALUE paired_obj, VALUE arg) {
  rb_tr_error("rb_exec_recursive_paired_outer not implemented");
}

VALUE rb_dir_getwd(void) {
  rb_tr_error("rb_dir_getwd not implemented");
}

VALUE rb_file_s_expand_path(int argc, const VALUE *argv) {
  rb_tr_error("rb_file_s_expand_path not implemented");
}

VALUE rb_file_expand_path(VALUE fname, VALUE dname) {
  rb_tr_error("rb_file_expand_path not implemented");
}

VALUE rb_file_s_absolute_path(int argc, const VALUE *argv) {
  rb_tr_error("rb_file_s_absolute_path not implemented");
}

VALUE rb_file_absolute_path(VALUE fname, VALUE dname) {
  rb_tr_error("rb_file_absolute_path not implemented");
}

VALUE rb_file_dirname(VALUE fname) {
  rb_tr_error("rb_file_dirname not implemented");
}

int rb_find_file_ext_safe(VALUE *filep, const char *const *ext, int safe_level) {
  rb_tr_error("rb_find_file_ext_safe not implemented");
}

VALUE rb_find_file_safe(VALUE path, int safe_level) {
  rb_tr_error("rb_find_file_safe not implemented");
}

int rb_find_file_ext(VALUE *filep, const char *const *ext) {
  rb_tr_error("rb_find_file_ext not implemented");
}

VALUE rb_find_file(VALUE path) {
  rb_tr_error("rb_find_file not implemented");
}

VALUE rb_file_directory_p(VALUE obj, VALUE fname) {
  rb_tr_error("rb_file_directory_p not implemented");
}

VALUE rb_str_encode_ospath(VALUE path) {
  rb_tr_error("rb_str_encode_ospath not implemented");
}

int rb_is_absolute_path(const char *path) {
  rb_tr_error("rb_is_absolute_path not implemented");
}

int rb_during_gc(void) {
  rb_tr_error("rb_during_gc not implemented");
}

void rb_gc_mark_locations(const VALUE *start, const VALUE *end) {
  rb_tr_error("rb_gc_mark_locations not implemented");
}

void rb_mark_tbl(st_table *tbl) {
  rb_tr_error("rb_mark_tbl not implemented");
}

void rb_mark_set(st_table *tbl) {
  rb_tr_error("rb_mark_set not implemented");
}

void rb_mark_hash(st_table *tbl) {
  rb_tr_error("rb_mark_hash not implemented");
}

void rb_gc_force_recycle(VALUE obj) {
  rb_tr_error("rb_gc_force_recycle not implemented");
}

void rb_gc_copy_finalizer(VALUE dest, VALUE obj) {
  rb_tr_error("rb_gc_copy_finalizer not implemented");
}

void rb_gc_finalize_deferred(void) {
  rb_tr_error("rb_gc_finalize_deferred not implemented");
}

void rb_gc_call_finalizer_at_exit(void) {
  rb_tr_error("rb_gc_call_finalizer_at_exit not implemented");
}

VALUE rb_gc_start(void) {
  rb_tr_error("rb_gc_start not implemented");
}

VALUE rb_define_finalizer(VALUE obj, VALUE block) {
  rb_tr_error("rb_define_finalizer not implemented");
}

VALUE rb_undefine_finalizer(VALUE obj) {
  rb_tr_error("rb_undefine_finalizer not implemented");
}

size_t rb_gc_count(void) {
  rb_tr_error("rb_gc_count not implemented");
}

size_t rb_gc_stat(VALUE key) {
  rb_tr_error("rb_gc_stat not implemented");
}

VALUE rb_gc_latest_gc_info(VALUE key) {
  rb_tr_error("rb_gc_latest_gc_info not implemented");
}

VALUE rb_check_hash_type(VALUE hash) {
  rb_tr_error("rb_check_hash_type not implemented");
}

VALUE rb_hash_update_by(VALUE hash1, VALUE hash2, rb_hash_update_func *func) {
  rb_tr_error("rb_hash_update_by not implemented");
}

int rb_path_check(const char *path) {
  rb_tr_error("rb_path_check not implemented");
}

int rb_env_path_tainted(void) {
  rb_tr_error("rb_env_path_tainted not implemented");
}

VALUE rb_env_clear(void) {
  rb_tr_error("rb_env_clear not implemented");
}

VALUE rb_io_gets(VALUE io) {
  rb_tr_error("rb_io_gets not implemented");
}

VALUE rb_io_getbyte(VALUE io) {
  rb_tr_error("rb_io_getbyte not implemented");
}

VALUE rb_io_ungetc(VALUE io, VALUE c) {
  rb_tr_error("rb_io_ungetc not implemented");
}

VALUE rb_io_ungetbyte(VALUE io, VALUE b) {
  rb_tr_error("rb_io_ungetbyte not implemented");
}

VALUE rb_io_flush(VALUE io) {
  rb_tr_error("rb_io_flush not implemented");
}

VALUE rb_io_eof(VALUE io) {
  rb_tr_error("rb_io_eof not implemented");
}

VALUE rb_io_ascii8bit_binmode(VALUE io) {
  rb_tr_error("rb_io_ascii8bit_binmode not implemented");
}

VALUE rb_io_fdopen(int fd, int oflags, const char *path) {
  rb_tr_error("rb_io_fdopen not implemented");
}

VALUE rb_io_get_io(VALUE io) {
  rb_tr_error("rb_io_get_io not implemented");
}

VALUE rb_gets(void) {
  rb_tr_error("rb_gets not implemented");
}

void rb_write_error(const char *mesg) {
  rb_tr_error("rb_write_error not implemented");
}

void rb_write_error2(const char *mesg, long len) {
  rb_tr_error("rb_write_error2 not implemented");
}

void rb_close_before_exec(int lowfd, int maxhint, VALUE noclose_fds) {
  rb_tr_error("rb_close_before_exec not implemented");
}

int rb_pipe(int *pipes) {
  rb_tr_error("rb_pipe not implemented");
}

int rb_reserved_fd_p(int fd) {
  rb_tr_error("rb_reserved_fd_p not implemented");
}

int rb_cloexec_dup2(int oldfd, int newfd) {
  rb_tr_error("rb_cloexec_dup2 not implemented");
}

int rb_cloexec_pipe(int fildes[2]) {
  rb_tr_error("rb_cloexec_pipe not implemented");
}

int rb_cloexec_fcntl_dupfd(int fd, int minfd) {
  rb_tr_error("rb_cloexec_fcntl_dupfd not implemented");
}

void rb_marshal_define_compat(VALUE newclass, VALUE oldclass, VALUE (*dumper)(VALUE), VALUE (*loader)(VALUE, VALUE)) {
  rb_tr_error("rb_marshal_define_compat not implemented");
}

VALUE rb_num_coerce_bit(VALUE x, VALUE y, ID func) {
  rb_tr_error("rb_num_coerce_bit not implemented");
}

VALUE rb_num2fix(VALUE val) {
  rb_tr_error("rb_num2fix not implemented");
}

VALUE rb_fix2str(VALUE x, int base) {
  rb_tr_error("rb_fix2str not implemented");
}

VALUE rb_dbl_cmp(double a, double b) {
  rb_tr_error("rb_dbl_cmp not implemented");
}

int rb_eql(VALUE obj1, VALUE obj2) {
  rb_tr_error("rb_eql not implemented");
}

VALUE rb_obj_clone(VALUE obj) {
  return rb_funcall(obj, rb_intern("clone"), 0);
}

VALUE rb_obj_init_copy(VALUE obj, VALUE orig) {
  rb_tr_error("rb_obj_init_copy not implemented");
}

VALUE rb_obj_tainted(VALUE obj) {
  rb_tr_error("rb_obj_tainted not implemented");
}

VALUE rb_obj_untaint(VALUE obj) {
  rb_tr_error("rb_obj_untaint not implemented");
}

VALUE rb_obj_untrust(VALUE obj) {
  rb_tr_error("rb_obj_untrust not implemented");
}

VALUE rb_obj_untrusted(VALUE obj) {
  rb_tr_error("rb_obj_untrusted not implemented");
}

VALUE rb_obj_trust(VALUE obj) {
  rb_tr_error("rb_obj_trust not implemented");
}

VALUE rb_class_get_superclass(VALUE klass) {
  rb_tr_error("rb_class_get_superclass not implemented");
}

VALUE rb_check_to_float(VALUE val) {
  rb_tr_error("rb_check_to_float not implemented");
}

VALUE rb_check_to_int(VALUE val) {
  rb_tr_error("rb_check_to_int not implemented");
}

VALUE rb_to_float(VALUE val) {
  rb_tr_error("rb_to_float not implemented");
}

double rb_str_to_dbl(VALUE str, int badcheck) {
  rb_tr_error("rb_str_to_dbl not implemented");
}

ID rb_id_attrset(ID id) {
  rb_tr_error("rb_id_attrset not implemented");
}

int rb_is_global_id(ID id) {
  rb_tr_error("rb_is_global_id not implemented");
}

int rb_is_attrset_id(ID id) {
  rb_tr_error("rb_is_attrset_id not implemented");
}

int rb_is_local_id(ID id) {
  rb_tr_error("rb_is_local_id not implemented");
}

int rb_is_junk_id(ID id) {
  rb_tr_error("rb_is_junk_id not implemented");
}

int rb_symname_p(const char *name) {
  rb_tr_error("rb_symname_p not implemented");
}

void rb_backref_set(VALUE val) {
  rb_tr_error("rb_backref_set not implemented");
}

void rb_last_status_set(int status, rb_pid_t pid) {
  rb_tr_error("rb_last_status_set not implemented");
}

VALUE rb_last_status_get(void) {
  rb_tr_error("rb_last_status_get not implemented");
}

int rb_proc_exec(const char *str) {
  rb_tr_error("rb_proc_exec not implemented");
}

VALUE rb_f_exec(int argc, const VALUE *argv) {
  rb_tr_error("rb_f_exec not implemented");
}

rb_pid_t rb_waitpid(rb_pid_t pid, int *st, int flags) {
  rb_tr_error("rb_waitpid not implemented");
}

void rb_syswait(rb_pid_t pid) {
  rb_tr_error("rb_syswait not implemented");
}

rb_pid_t rb_spawn(int argc, const VALUE *argv) {
  rb_tr_error("rb_spawn not implemented");
}

rb_pid_t rb_spawn_err(int argc, const VALUE *argv, char *errmsg, size_t errmsg_buflen) {
  rb_tr_error("rb_spawn_err not implemented");
}

VALUE rb_proc_times(VALUE obj) {
  rb_tr_error("rb_proc_times not implemented");
}

VALUE rb_detach_process(rb_pid_t pid) {
  rb_tr_error("rb_detach_process not implemented");
}

double rb_genrand_real(void) {
  rb_tr_error("rb_genrand_real not implemented");
}

void rb_reset_random_seed(void) {
  rb_tr_error("rb_reset_random_seed not implemented");
}

VALUE rb_random_bytes(VALUE obj, long n) {
  rb_tr_error("rb_random_bytes not implemented");
}

double rb_random_real(VALUE obj) {
  rb_tr_error("rb_random_real not implemented");
}

int rb_memcicmp(const void *x, const void *y, long len) {
  rb_tr_error("rb_memcicmp not implemented");
}

void rb_match_busy(VALUE match) {
  rb_tr_error("rb_match_busy not implemented");
}

VALUE rb_reg_nth_defined(int nth, VALUE match) {
  rb_tr_error("rb_reg_nth_defined not implemented");
}

int rb_reg_backref_number(VALUE match, VALUE backref) {
  rb_tr_error("rb_reg_backref_number not implemented");
}

VALUE rb_reg_last_match(VALUE match) {
  rb_tr_error("rb_reg_last_match not implemented");
}

VALUE rb_reg_match_post(VALUE match) {
  rb_tr_error("rb_reg_match_post not implemented");
}

VALUE rb_reg_match_last(VALUE match) {
  rb_tr_error("rb_reg_match_last not implemented");
}

VALUE rb_reg_alloc(void) {
  rb_tr_error("rb_reg_alloc not implemented");
}

VALUE rb_reg_init_str(VALUE re, VALUE s, int options) {
  rb_tr_error("rb_reg_init_str not implemented");
}

VALUE rb_reg_match2(VALUE re) {
  rb_tr_error("rb_reg_match2 not implemented");
}

VALUE rb_get_argv(void) {
  rb_tr_error("rb_get_argv not implemented");
}

VALUE rb_f_kill(int argc, const VALUE *argv) {
  rb_tr_error("rb_f_kill not implemented");
}

void rb_trap_exit(void) {
  rb_tr_error("rb_trap_exit not implemented");
}

VALUE rb_str_vcatf(VALUE str, const char *fmt, va_list ap) {
  rb_tr_error("rb_str_vcatf not implemented");
}

VALUE rb_str_format(int argc, const VALUE *argv, VALUE fmt) {
  rb_tr_error("rb_str_format not implemented");
}

VALUE rb_str_tmp_new(long len) {
  rb_tr_error("rb_str_tmp_new not implemented");
}

#undef rb_utf8_str_new
VALUE rb_utf8_str_new(const char *ptr, long len) {
  rb_tr_error("rb_utf8_str_new not implemented");
}

#undef rb_utf8_str_new_cstr
VALUE rb_utf8_str_new_cstr(const char *ptr) {
  rb_tr_error("rb_utf8_str_new_cstr not implemented");
}

VALUE rb_str_new_static(const char *ptr, long len) {
  rb_tr_error("rb_str_new_static not implemented");
}

VALUE rb_usascii_str_new_static(const char *ptr, long len) {
  rb_tr_error("rb_usascii_str_new_static not implemented");
}

VALUE rb_utf8_str_new_static(const char *ptr, long len) {
  rb_tr_error("rb_utf8_str_new_static not implemented");
}

void rb_str_shared_replace(VALUE str, VALUE str2) {
  rb_tr_error("rb_str_shared_replace not implemented");
}

VALUE rb_str_buf_append(VALUE str, VALUE str2) {
  rb_tr_error("rb_str_buf_append not implemented");
}

VALUE rb_str_buf_cat_ascii(VALUE str, const char *ptr) {
  rb_tr_error("rb_str_buf_cat_ascii not implemented");
}

VALUE rb_str_locktmp(VALUE str) {
  rb_tr_error("rb_str_locktmp not implemented");
}

VALUE rb_str_unlocktmp(VALUE str) {
  rb_tr_error("rb_str_unlocktmp not implemented");
}

long rb_str_sublen(VALUE str, long pos) {
  rb_tr_error("rb_str_sublen not implemented");
}

void rb_str_modify_expand(VALUE str, long expand) {
  rb_tr_error("rb_str_modify_expand not implemented");
}

#undef rb_str_cat_cstr
VALUE rb_str_cat_cstr(VALUE str, const char *ptr) {
  rb_tr_error("rb_str_cat_cstr not implemented");
}

st_index_t rb_hash_start(st_index_t h) {
  rb_tr_error("rb_hash_start not implemented");
}

int rb_str_hash_cmp(VALUE str1, VALUE str2) {
  rb_tr_error("rb_str_hash_cmp not implemented");
}

int rb_str_comparable(VALUE str1, VALUE str2) {
  rb_tr_error("rb_str_comparable not implemented");
}

VALUE rb_str_drop_bytes(VALUE str, long len) {
  rb_tr_error("rb_str_drop_bytes not implemented");
}

VALUE rb_str_dump(VALUE str) {
  rb_tr_error("rb_str_dump not implemented");
}

void rb_str_setter(VALUE val, ID id, VALUE *var) {
  rb_tr_error("rb_str_setter not implemented");
}

VALUE rb_sym_to_s(VALUE sym) {
  rb_tr_error("rb_sym_to_s not implemented");
}

long rb_str_strlen(VALUE str) {
  rb_tr_error("rb_str_strlen not implemented");
}

long rb_str_offset(VALUE str, long pos) {
  rb_tr_error("rb_str_offset not implemented");
}

size_t rb_str_capacity(VALUE str) {
  rb_tr_error("rb_str_capacity not implemented");
}

VALUE rb_str_ellipsize(VALUE str, long len) {
  rb_tr_error("rb_str_ellipsize not implemented");
}

VALUE rb_str_scrub(VALUE str, VALUE repl) {
  rb_tr_error("rb_str_scrub not implemented");
}

VALUE rb_sym_all_symbols(void) {
  rb_tr_error("rb_sym_all_symbols not implemented");
}

VALUE rb_struct_alloc(VALUE klass, VALUE values) {
  rb_tr_error("rb_struct_alloc not implemented");
}

VALUE rb_struct_initialize(VALUE self, VALUE values) {
  rb_tr_error("rb_struct_initialize not implemented");
}

VALUE rb_struct_alloc_noinit(VALUE klass) {
  rb_tr_error("rb_struct_alloc_noinit not implemented");
}

VALUE rb_struct_define_without_accessor(const char *class_name, VALUE super, rb_alloc_func_t alloc, ...) {
  rb_tr_error("rb_struct_define_without_accessor not implemented");
}

VALUE rb_struct_define_without_accessor_under(VALUE outer, const char *class_name, VALUE super, rb_alloc_func_t alloc, ...) {
  rb_tr_error("rb_struct_define_without_accessor_under not implemented");
}

void rb_thread_check_ints(void) {
  rb_tr_error("rb_thread_check_ints not implemented");
}

int rb_thread_interrupted(VALUE thval) {
  rb_tr_error("rb_thread_interrupted not implemented");
}

VALUE rb_mod_name(VALUE mod) {
  rb_tr_error("rb_mod_name not implemented");
}

VALUE rb_class_path_cached(VALUE klass) {
  rb_tr_error("rb_class_path_cached not implemented");
}

void rb_set_class_path(VALUE klass, VALUE under, const char *name) {
  rb_tr_error("rb_set_class_path not implemented");
}

void rb_set_class_path_string(VALUE klass, VALUE under, VALUE name) {
  rb_tr_error("rb_set_class_path_string not implemented");
}

void rb_name_class(VALUE klass, ID id) {
  rb_tr_error("rb_name_class not implemented");
}

VALUE rb_autoload_load(VALUE mod, ID id) {
  rb_tr_error("rb_autoload_load not implemented");
}

VALUE rb_autoload_p(VALUE mod, ID id) {
  rb_tr_error("rb_autoload_p not implemented");
}

VALUE rb_f_trace_var(int argc, const VALUE *argv) {
  rb_tr_error("rb_f_trace_var not implemented");
}

VALUE rb_f_untrace_var(int argc, const VALUE *argv) {
  rb_tr_error("rb_f_untrace_var not implemented");
}

void rb_alias_variable(ID name1, ID name2) {
  rb_tr_error("rb_alias_variable not implemented");
}

void rb_copy_generic_ivar(VALUE clone, VALUE obj) {
  rb_tr_error("rb_copy_generic_ivar not implemented");
}

void rb_free_generic_ivar(VALUE obj) {
  rb_tr_error("rb_free_generic_ivar not implemented");
}

void rb_ivar_foreach(VALUE obj, int (*func)(ANYARGS), st_data_t arg) {
  rb_tr_error("rb_ivar_foreach not implemented");
}

st_index_t rb_ivar_count(VALUE obj) {
  rb_tr_error("rb_ivar_count not implemented");
}

VALUE rb_obj_remove_instance_variable(VALUE obj, VALUE name) {
  rb_tr_error("rb_obj_remove_instance_variable not implemented");
}

VALUE rb_const_list(void *data) {
  rb_tr_error("rb_const_list not implemented");
}

VALUE rb_mod_constants(int argc, const VALUE *argv, VALUE mod) {
  rb_tr_error("rb_mod_constants not implemented");
}

VALUE rb_mod_remove_const(VALUE mod, VALUE name) {
  rb_tr_error("rb_mod_remove_const not implemented");
}

int rb_const_defined_from(VALUE klass, ID id) {
  rb_tr_error("rb_const_defined_from not implemented");
}

VALUE rb_const_remove(VALUE mod, ID id) {
  rb_tr_error("rb_const_remove not implemented");
}

VALUE rb_mod_const_missing(VALUE klass, VALUE name) {
  rb_tr_error("rb_mod_const_missing not implemented");
}

VALUE rb_mod_class_variables(int argc, const VALUE *argv, VALUE mod) {
  rb_tr_error("rb_mod_class_variables not implemented");
}

VALUE rb_mod_remove_cvar(VALUE mod, VALUE name) {
  rb_tr_error("rb_mod_remove_cvar not implemented");
}

ID rb_frame_callee(void) {
  rb_tr_error("rb_frame_callee not implemented");
}

VALUE rb_str_succ(VALUE orig) {
  rb_tr_error("rb_str_succ not implemented");
}

VALUE rb_time_succ(VALUE time) {
  rb_tr_error("rb_time_succ not implemented");
}

int rb_frame_method_id_and_class(ID *idp, VALUE *klassp) {
  rb_tr_error("rb_frame_method_id_and_class not implemented");
}

VALUE rb_make_exception(int argc, const VALUE *argv) {
  rb_tr_error("rb_make_exception not implemented");
}

int rb_io_modestr_fmode(const char *modestr) {
  rb_tr_error("rb_io_modestr_fmode not implemented");
}

int rb_io_modestr_oflags(const char *modestr) {
  rb_tr_error("rb_io_modestr_oflags not implemented");
}

int rb_io_oflags_fmode(int oflags) {
  rb_tr_error("rb_io_oflags_fmode not implemented");
}

void rb_io_check_char_readable(rb_io_t *fptr) {
  rb_tr_error("rb_io_check_char_readable not implemented");
}

void rb_io_check_byte_readable(rb_io_t *fptr) {
  rb_tr_error("rb_io_check_byte_readable not implemented");
}

int rb_io_fptr_finalize(rb_io_t *fptr) {
  rb_tr_error("rb_io_fptr_finalize not implemented");
}

void rb_io_synchronized(rb_io_t *fptr) {
  rb_tr_error("rb_io_synchronized not implemented");
}

void rb_io_check_initialized(rb_io_t *fptr) {
  rb_tr_error("rb_io_check_initialized not implemented");
}

VALUE rb_io_get_write_io(VALUE io) {
  rb_tr_error("rb_io_get_write_io not implemented");
}

VALUE rb_io_set_write_io(VALUE io, VALUE w) {
  rb_tr_error("rb_io_set_write_io not implemented");
}

void rb_io_set_nonblock(rb_io_t *fptr) {
  rb_tr_error("rb_io_set_nonblock not implemented");
}

ssize_t rb_io_bufwrite(VALUE io, const void *buf, size_t size) {
  rb_tr_error("rb_io_bufwrite not implemented");
}

void rb_io_read_check(rb_io_t *fptr) {
  rb_tr_error("rb_io_read_check not implemented");
}

int rb_io_read_pending(rb_io_t *fptr) {
  rb_tr_error("rb_io_read_pending not implemented");
}

VALUE rb_stat_new(const struct stat *st) {
  rb_tr_error("rb_stat_new not implemented");
}

long rb_reg_search(VALUE re, VALUE str, long pos, int reverse) {
  rb_tr_error("rb_reg_search not implemented");
}

VALUE rb_reg_regsub(VALUE str, VALUE src, struct re_registers *regs, VALUE regexp) {
  rb_tr_error("rb_reg_regsub not implemented");
}

long rb_reg_adjust_startpos(VALUE re, VALUE str, long pos, int reverse) {
  rb_tr_error("rb_reg_adjust_startpos not implemented");
}

VALUE rb_reg_quote(VALUE str) {
  rb_tr_error("rb_reg_quote not implemented");
}

int rb_reg_region_copy(struct re_registers *to, const struct re_registers *from) {
  rb_tr_error("rb_reg_region_copy not implemented");
}

ID rb_sym2id(VALUE sym) {
  rb_tr_error("rb_sym2id not implemented");
}

VALUE rb_id2sym(ID x) {
  rb_tr_error("rb_id2sym not implemented");
}

VALUE rb_get_path_no_checksafe(VALUE obj) {
  rb_tr_error("rb_get_path_no_checksafe not implemented");
}

void rb_secure_update(VALUE obj) {
  rb_tr_error("rb_secure_update not implemented");
}

VALUE rb_uint2big(VALUE n) {
  rb_tr_error("rb_uint2big not implemented");
}

VALUE rb_int2big(SIGNED_VALUE n) {
  // it cannot overflow Fixnum
  return LONG2FIX(n);
}

VALUE rb_newobj(void) {
  rb_tr_error("rb_newobj not implemented");
}

VALUE rb_newobj_of(VALUE klass, VALUE flags) {
  rb_tr_error("rb_newobj_of not implemented");
}

VALUE rb_obj_setup(VALUE obj, VALUE klass, VALUE type) {
  rb_tr_error("rb_obj_setup not implemented");
}

VALUE rb_float_new_in_heap(double d) {
  rb_tr_error("rb_float_new_in_heap not implemented");
}

int rb_typeddata_inherited_p(const rb_data_type_t *child, const rb_data_type_t *parent) {
  rb_tr_error("rb_typeddata_inherited_p not implemented");
}

int rb_typeddata_is_kind_of(VALUE obj, const rb_data_type_t *data_type) {
  rb_tr_error("rb_typeddata_is_kind_of not implemented");
}

void rb_freeze_singleton_class(VALUE x) {
  rb_tr_error("rb_freeze_singleton_class not implemented");
}

void rb_gc_writebarrier(VALUE a, VALUE b) {
  rb_tr_error("rb_gc_writebarrier not implemented");
}

void rb_gc_writebarrier_unprotect(VALUE obj) {
  rb_tr_error("rb_gc_writebarrier_unprotect not implemented");
}

void rb_gc_unprotect_logging(void *objptr, const char *filename, int line) {
  rb_tr_error("rb_gc_unprotect_logging not implemented");
}

void rb_obj_infect(VALUE obj1, VALUE obj2) {
  rb_tr_error("rb_obj_infect not implemented");
}

void rb_glob(const char *path, void (*func)(const char *, VALUE, void *), VALUE arg) {
  rb_tr_error("rb_glob not implemented");
}

void rb_prepend_module(VALUE klass, VALUE module) {
  rb_tr_error("rb_prepend_module not implemented");
}

VALUE rb_gvar_undef_getter(ID id, void *data, struct rb_global_variable *var) {
  rb_tr_error("rb_gvar_undef_getter not implemented");
}

VALUE rb_gvar_val_getter(ID id, void *data, struct rb_global_variable *var) {
  rb_tr_error("rb_gvar_val_getter not implemented");
}

void rb_define_virtual_variable( const char *name, VALUE (*getter)(ANYARGS), void (*setter)(ANYARGS)) {
  rb_tr_error("rb_define_virtual_variable not implemented");
}

void rb_gc_register_mark_object(VALUE obj) {
  rb_tr_error("rb_gc_register_mark_object not implemented");
}

ID rb_check_id(volatile VALUE *namep) {
  rb_tr_error("rb_check_id not implemented");
}

VALUE rb_to_symbol(VALUE name) {
  rb_tr_error("rb_to_symbol not implemented");
}

VALUE rb_check_symbol(volatile VALUE *namep) {
  rb_tr_error("rb_check_symbol not implemented");
}

VALUE rb_eval_string_protect(const char *str, int *state) {
  rb_tr_error("rb_eval_string_protect not implemented");
}

VALUE rb_eval_string_wrap(const char *str, int *state) {
  rb_tr_error("rb_eval_string_wrap not implemented");
}

VALUE rb_funcall_passing_block(VALUE recv, ID mid, int argc, const VALUE *argv) {
  rb_tr_error("rb_funcall_passing_block not implemented");
}

VALUE rb_current_receiver(void) {
  rb_tr_error("rb_current_receiver not implemented");
}

int rb_get_kwargs(VALUE keyword_hash, const ID *table, int required, int optional, VALUE *values) {
  rb_tr_error("rb_get_kwargs not implemented");
}

VALUE rb_extract_keywords(VALUE *orighash) {
  rb_tr_error("rb_extract_keywords not implemented");
}

VALUE rb_syserr_new(int n, const char *mesg) {
  rb_tr_error("rb_syserr_new not implemented");
}

VALUE rb_syserr_new_str(int n, VALUE arg) {
  rb_tr_error("rb_syserr_new_str not implemented");
}

VALUE rb_yield_values2(int argc, const VALUE *argv) {
  rb_tr_error("rb_yield_values2 not implemented");
}

int rb_isalnum(int c) {
  return rb_isalpha(c) || rb_isdigit(c);
}

int rb_isalpha(int c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int rb_isblank(int c) {
  return c == 0x09 || c == 0x20;
}

int rb_iscntrl(int c) {
  return (c >= 0x00 && c <= 0x1f) || c == 0x7f;
}

int rb_isdigit(int c) {
  return c >= '0' && c <= '9';
}

int rb_isgraph(int c) {
  return c >= '!' && c <= '~';
}

int rb_islower(int c) {
  return c >= 'a' && c <= 'z';
}

int rb_isprint(int c) {
  return c == ' ' || rb_isgraph(c);
}

int rb_ispunct(int c) {
  return (c >= '!' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

int rb_isspace(int c) {
  return c == 0x20 || (c >= 0x09 && c <= 0x0d);
}

int rb_isupper(int c) {
  return c >= 'A' && c <= 'Z';
}

int rb_isxdigit(int c) {
  return rb_isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int rb_tolower(int c) {
  return rb_isascii(c) && rb_isupper(c) ? c ^ 0x20 : c;
}

int rb_toupper(int c) {
  return rb_isascii(c) && rb_islower(c) ? c ^ 0x20 : c;
}
