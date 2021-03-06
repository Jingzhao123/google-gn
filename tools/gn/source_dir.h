// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_SOURCE_DIR_H_
#define TOOLS_GN_SOURCE_DIR_H_

#include <stddef.h>

#include <algorithm>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/logging.h"

class Err;
class SourceFile;
class Value;

// Represents a directory within the source tree. Source dirs begin and end in
// slashes.
//
// If there is one slash at the beginning, it will mean a system-absolute file
// path. On Windows, absolute system paths will be of the form "/C:/foo/bar".
//
// Two slashes at the beginning indicate a path relative to the source root.
class SourceDir {
 public:
  SourceDir() = default;

  SourceDir(const std::string& s);
  explicit SourceDir(std::string&& s);

  // Resolves a file or dir name (based on as_file parameter) relative
  // to this source directory. Will return an empty string on error
  // and set the give *err pointer (required). Empty input is always an error.
  //
  // Passed non null v_value will be used to resolve path (in cases where
  // a substring has been extracted from the value, as with label resolution).
  // In this use case parameter v is used to generate proper error.
  //
  // If source_root is supplied, these functions will additionally handle the
  // case where the input is a system-absolute but still inside the source
  // tree. This is the case for some external tools.
  std::string ResolveRelativeAs(
      bool as_file,
      const Value& v,
      Err* err,
      const std::string_view& source_root = std::string_view(),
      const std::string* v_value = nullptr) const;

  // Like ResolveRelativeAs above, but allows to produce result
  // without overhead for string conversion (on input value).
  template <typename StringType>
  std::string ResolveRelativeAs(
      bool as_file,
      const Value& blame_input_value,
      const StringType& input_value,
      Err* err,
      const std::string_view& source_root = std::string_view()) const;

  // Wrapper for ResolveRelativeAs.
  SourceFile ResolveRelativeFile(
      const Value& p,
      Err* err,
      const std::string_view& source_root = std::string_view()) const;

  // Wrapper for ResolveRelativeAs.
  template <typename StringType>
  SourceDir ResolveRelativeDir(
      const Value& blame_input_value,
      const StringType& input_value,
      Err* err,
      const std::string_view& source_root = std::string_view()) const {
    SourceDir ret;
    ret.value_ = ResolveRelativeAs<StringType>(false, blame_input_value,
                                               input_value, err, source_root);
    return ret;
  }

  // Wrapper for ResolveRelativeDir where input_value equals to
  // v.string_value().
  SourceDir ResolveRelativeDir(
      const Value& v,
      Err* err,
      const std::string_view& source_root = std::string_view()) const;

  // Resolves this source file relative to some given source root. Returns
  // an empty file path on error.
  base::FilePath Resolve(const base::FilePath& source_root) const;

  bool is_null() const { return value_.empty(); }
  const std::string& value() const { return value_; }

  // Returns true if this path starts with a "//" which indicates a path
  // from the source root.
  bool is_source_absolute() const {
    return value_.size() >= 2 && value_[0] == '/' && value_[1] == '/';
  }

  // Returns true if this path starts with a single slash which indicates a
  // system-absolute path.
  bool is_system_absolute() const { return !is_source_absolute(); }

  // Returns a source-absolute path starting with only one slash at the
  // beginning (normally source-absolute paths start with two slashes to mark
  // them as such). This is normally used when concatenating directories
  // together.
  //
  // This function asserts that the directory is actually source-absolute. The
  // return value points into our buffer.
  std::string_view SourceAbsoluteWithOneSlash() const {
    CHECK(is_source_absolute());
    return std::string_view(&value_[1], value_.size() - 1);
  }

  // Returns a path that does not end with a slash.
  //
  // This function simply returns the reference to the value if the path is a
  // root, e.g. "/" or "//".
  std::string_view SourceWithNoTrailingSlash() const {
    if (value_.size() > 2)
      return std::string_view(&value_[0], value_.size() - 1);
    return std::string_view(value_);
  }

  void SwapValue(std::string* v);

  bool operator==(const SourceDir& other) const {
    return value_ == other.value_;
  }
  bool operator!=(const SourceDir& other) const { return !operator==(other); }
  bool operator<(const SourceDir& other) const { return value_ < other.value_; }

 private:
  friend class SourceFile;
  std::string value_;
};

namespace std {

template <>
struct hash<SourceDir> {
  std::size_t operator()(const SourceDir& v) const {
    hash<std::string> h;
    return h(v.value());
  }
};

}  // namespace std

#endif  // TOOLS_GN_SOURCE_DIR_H_
