/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#pragma once

#include <event2/buffer.h>
#include <glog/logging.h>
#include <rocksdb/status.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#define CRLF "\r\n"

namespace Redis {

inline void Reply(evbuffer *output, std::string_view data) { evbuffer_add(output, data.data(), data.size()); }

struct ReplyData {
  enum Type : std::size_t {
    SimpleString = 0,
    Error,
    Integer,
    BulkString,
    NullBulkString,
    Array,
  };

  std::variant<std::string_view, std::string_view, std::int64_t, std::string_view, std::monostate,
               std::vector<ReplyData>>
      data;

  ReplyData() = default;

  template <size_t I, typename... Args>
  explicit ReplyData(std::in_place_index_t<I>, Args &&...args)
      : data(std::in_place_index<I>, std::forward<Args>(args)...) {}

  Type GetType() const { return static_cast<Type>(data.index()); }

  void Dump(std::string &s) const {
    if (GetType() == SimpleString) {
      s.append("+");
      s.append(std::get<SimpleString>(data));
      s.append(CRLF);
    } else if (GetType() == Error) {
      s.append("-");
      s.append(std::get<Error>(data));
      s.append(CRLF);
    } else if (GetType() == Integer) {
      s.append(":");
      s.append(std::to_string(std::get<Integer>(data)));
      s.append(CRLF);
    } else if (GetType() == BulkString) {
      s.append("$");
      s.append(std::to_string(std::get<BulkString>(data).size()));
      s.append(CRLF);
      s.append(std::get<BulkString>(data));
      s.append(CRLF);
    } else if (GetType() == NullBulkString) {
      s.append("$-1" CRLF);
    } else if (GetType() == Array) {
      s.append("*");
      s.append(std::to_string(std::get<Array>(data).size()));
      s.append(CRLF);
      for (const auto &i : std::get<Array>(data)) {
        i.Dump(s);
      }
    }
  }

  template <Type type>
  decltype(auto) Get() {
    CHECK_EQ(GetType(), type);

    return std::get<type>(data);
  }

  template <typename... Args>
  decltype(auto) Push(Args &&...args) {
    return Get<Array>().emplace_back(std::forward<Args>(args)...);
  }

  std::string String() const {
    std::string buf;
    Dump(buf);
    return buf;
  }

  void Reply(evbuffer *output) const { Redis::Reply(output, String()); }
};

inline auto SimpleString(std::string_view data) {
  return ReplyData(std::in_place_index<ReplyData::SimpleString>, data);
}

inline auto Error(std::string_view data) { return ReplyData(std::in_place_index<ReplyData::Error>, data); }

inline auto Integer(std::int64_t data) { return ReplyData(std::in_place_index<ReplyData::Integer>, data); }

inline auto BulkString(std::string_view data) { return ReplyData(std::in_place_index<ReplyData::BulkString>, data); }

inline auto NullBulkString() { return ReplyData(std::in_place_index<ReplyData::NullBulkString>); }

template <typename... Args>
auto Array(Args &&...args) {
  return ReplyData(std::in_place_index<ReplyData::Array>, std::forward<Args>(args)...);
}

auto Array(std::initializer_list<ReplyData> list) {
  return ReplyData(std::in_place_index<ReplyData::Array>, std::move(list));
}

template <typename Container = std::initializer_list<std::string_view>>
auto MultiBulkString(const Container &con, bool output_nil_for_empty_string = true) {
  auto array = Array();

  array.template Get<ReplyData::Array>().reserve(con.size());

  for (const auto &v : con) {
    if (v.empty() && output_nil_for_empty_string)
      array.Push(NullBulkString());
    else
      array.Push(BulkString(v));
  }

  return array;
}

template <typename ValueCon, typename StatusCon>
auto MultiBulkString(const ValueCon &values, const StatusCon &statuses) {
  auto array = Array();

  for (size_t i = 0; i < values.size(); i++) {
    if (i < statuses.size() && !statuses[i].ok()) {
      array.Push(NullBulkString());
    } else {
      array.Push(BulkString(values[i]));
    }
  }

  return array;
}

inline auto Command2RESP(const std::vector<std::string> &cmd_args) { return MultiBulkString(cmd_args); }
}  // namespace Redis
