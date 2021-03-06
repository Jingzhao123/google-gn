// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_UNIQUE_VECTOR_H_
#define TOOLS_GN_UNIQUE_VECTOR_H_

#include <stddef.h>

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace internal {

// This lass allows us to insert things by reference into a hash table which
// avoids copies. The hash function of a UniquifyRef is that of the object it
// points to.
//
// There are two ways it can store data: (1) by (vector*, index) which is used
// to refer to the array in UniqueVector and make it work even when the
// underlying vector is reallocated; (2) by T* which is used to do lookups
// into the hash table of things that aren't in a vector yet.
//
// It also caches the hash value which allows us to query and then insert
// without recomputing the hash.
template <typename T>
class UniquifyRef {
 public:
  UniquifyRef() = default;

  // Initialize with a pointer to a value.
  explicit UniquifyRef(const T* v) : value_(v) { FillHashValue(); }

  // Initialize with an array + index.
  UniquifyRef(const std::vector<T>* v, size_t i) : vect_(v), index_(i) {
    FillHashValue();
  }

  // Initialize with an array + index and a known hash value to prevent
  // re-hashing.
  UniquifyRef(const std::vector<T>* v, size_t i, size_t hash_value)
      : vect_(v), index_(i), hash_val_(hash_value) {}

  const T& value() const { return value_ ? *value_ : (*vect_)[index_]; }
  size_t hash_val() const { return hash_val_; }
  size_t index() const { return index_; }

 private:
  void FillHashValue() {
    std::hash<T> h;
    hash_val_ = h(value());
  }

  // When non-null, points to the object.
  const T* value_ = nullptr;

  // When value is null these are used.
  const std::vector<T>* vect_ = nullptr;
  size_t index_ = static_cast<size_t>(-1);

  size_t hash_val_ = 0;
};

template <typename T>
inline bool operator==(const UniquifyRef<T>& a, const UniquifyRef<T>& b) {
  return a.value() == b.value();
}

template <typename T>
inline bool operator<(const UniquifyRef<T>& a, const UniquifyRef<T>& b) {
  return a.value() < b.value();
}

}  // namespace internal

namespace std {

template <typename T>
struct hash<internal::UniquifyRef<T>> {
  std::size_t operator()(const internal::UniquifyRef<T>& v) const {
    return v.hash_val();
  }
};

}  // namespace std

// An ordered set optimized for GN's usage. Such sets are used to store lists
// of configs and libraries, and are appended to but not randomly inserted
// into.
template <typename T>
class UniqueVector {
 public:
  using Vector = std::vector<T>;
  using iterator = typename Vector::iterator;
  using const_iterator = typename Vector::const_iterator;

  const Vector& vector() const { return vector_; }
  size_t size() const { return vector_.size(); }
  bool empty() const { return vector_.empty(); }
  void clear() {
    vector_.clear();
    set_.clear();
  }
  void reserve(size_t s) { vector_.reserve(s); }

  const T& operator[](size_t index) const { return vector_[index]; }

  const_iterator begin() const { return vector_.begin(); }
  iterator begin() { return vector_.begin(); }
  const_iterator end() const { return vector_.end(); }
  iterator end() { return vector_.end(); }

  // Returns true if the item was appended, false if it already existed (and
  // thus the vector was not modified).
  bool push_back(const T& t) {
    Ref ref(&t);
    if (set_.find(ref) != set_.end())
      return false;  // Already have this one.

    vector_.push_back(t);
    set_.insert(Ref(&vector_, vector_.size() - 1, ref.hash_val()));
    return true;
  }

  bool push_back(T&& t) {
    Ref ref(&t);
    if (set_.find(ref) != set_.end())
      return false;  // Already have this one.

    auto ref_hash_val = ref.hash_val();  // Save across moving t.

    vector_.push_back(std::move(t));  // Invalidates |ref|.
    set_.insert(Ref(&vector_, vector_.size() - 1, ref_hash_val));
    return true;
  }

  // Appends a range of items from an iterator.
  template <typename iter>
  void Append(const iter& begin, const iter& end) {
    for (iter i = begin; i != end; ++i)
      push_back(*i);
  }

  // Returns the index of the item matching the given value in the list, or
  // (size_t)(-1) if it's not found.
  size_t IndexOf(const T& t) const {
    Ref ref(&t);
    typename HashSet::const_iterator found = set_.find(ref);
    if (found == set_.end())
      return static_cast<size_t>(-1);
    return found->index();
  }

 private:
  using Ref = internal::UniquifyRef<T>;
  using HashSet = std::unordered_set<Ref>;

  HashSet set_;
  Vector vector_;
};

#endif  // TOOLS_GN_UNIQUE_VECTOR_H_
