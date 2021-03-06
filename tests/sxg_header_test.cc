// Copyright 2019 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////////////

#include "libsxg/sxg_header.h"

#include "gtest/gtest.h"
#include "libsxg/internal/sxg_buffer.h"
#include "libsxg/internal/sxg_header.h"

namespace {

sxg_buffer_t StringToBuffer(const char* src) {
  sxg_buffer_t buf = sxg_empty_buffer();
  sxg_write_string(src, &buf);
  return buf;
}

std::string BufferToString(const sxg_buffer_t& buf) {
  return std::string(reinterpret_cast<char* const>(buf.data), buf.size);
}

TEST(SxgHeaderTest, Release) {
  sxg_header_t header = sxg_empty_header();
  sxg_header_release(&header);
}

TEST(SxgHeaderTest, WriteBuffer) {
  sxg_header_t header = sxg_empty_header();
  sxg_buffer_t buf = StringToBuffer("world");

  EXPECT_TRUE(sxg_header_append_buffer("hello", &buf, &header));
  EXPECT_EQ(1u, header.size);
  EXPECT_EQ(std::string("hello"), header.entries[0].key);
  EXPECT_EQ(std::string("world"), BufferToString(header.entries[0].value));

  sxg_buffer_release(&buf);
  sxg_header_release(&header);
}

TEST(SxgHeaderTest, WriteString) {
  sxg_header_t header = sxg_empty_header();

  EXPECT_TRUE(sxg_header_append_string("foo", "bar", &header));
  EXPECT_TRUE(sxg_header_append_string("hey", "baz", &header));
  EXPECT_EQ(2u, header.size);
  EXPECT_EQ(std::string("foo"), header.entries[0].key);
  EXPECT_EQ(std::string("bar"), BufferToString(header.entries[0].value));
  EXPECT_EQ(std::string("hey"), header.entries[1].key);
  EXPECT_EQ(std::string("baz"), BufferToString(header.entries[1].value));

  sxg_header_release(&header);
}

TEST(SxgHeaderTest, ConcatValues) {
  sxg_header_t header = sxg_empty_header();

  EXPECT_TRUE(sxg_header_append_string("hello", "happy", &header));
  EXPECT_TRUE(sxg_header_append_string("hello", " world", &header));
  EXPECT_EQ(1u, header.size);
  EXPECT_EQ(std::string("hello"), header.entries[0].key);
  EXPECT_EQ(std::string("happy, world"),
            BufferToString(header.entries[0].value));

  sxg_header_release(&header);
}

TEST(SxgHeaderTest, WriteInteger) {
  sxg_header_t header = sxg_empty_header();

  EXPECT_TRUE(sxg_header_append_integer("num", 12345, &header));
  EXPECT_EQ(1u, header.size);
  EXPECT_EQ(std::string("num"), header.entries[0].key);

  sxg_header_release(&header);
}

TEST(SxgHeaderTest, WriteBigInteger) {
  sxg_header_t header = sxg_empty_header();

  EXPECT_TRUE(sxg_header_append_integer("num", 0xffffffffffffffff, &header));
  EXPECT_EQ(1u, header.size);
  EXPECT_EQ(std::string("num"), header.entries[0].key);
  EXPECT_EQ(std::string("18446744073709551615"),
            BufferToString(header.entries[0].value));

  sxg_header_release(&header);
}

TEST(SxgHeaderTest, CapacityExpansion) {
  const size_t kSize = 200;  // 200 is enough size to cause expansion.
  sxg_header_t header = sxg_empty_header();

  for (size_t i = 0; i < kSize; ++i) {
    std::string key = std::to_string(i);
    EXPECT_TRUE(sxg_header_append_integer(key.c_str(), i * i, &header));
  }
  EXPECT_EQ(200u, header.size);
  for (size_t i = 0; i < kSize; ++i) {
    EXPECT_EQ(std::to_string(i), header.entries[i].key);
    EXPECT_EQ(std::to_string(i * i), BufferToString(header.entries[i].value));
  }

  sxg_header_release(&header);
}

TEST(SxgHeaderTest, Copy) {
  sxg_header_t header1 = sxg_empty_header();
  sxg_header_t header2 = sxg_empty_header();
  sxg_header_append_string("foo", "bar", &header1);

  EXPECT_TRUE(sxg_header_copy(&header1, &header2));
  sxg_header_append_string("hoge", "piyo", &header1);
  EXPECT_EQ(1u, header2.size);
  EXPECT_EQ(std::string("foo"), header2.entries[0].key);
  EXPECT_EQ(std::string("bar"), BufferToString(header2.entries[0].value));

  sxg_header_release(&header1);
  sxg_header_release(&header2);
}

TEST(SxgHeaderTest, Merge) {
  sxg_header_t header1 = sxg_empty_header();
  sxg_header_t header2 = sxg_empty_header();
  sxg_header_append_string("foo", "bar", &header1);
  sxg_header_append_string("hoge", "piyo", &header2);

  EXPECT_TRUE(sxg_header_merge(&header2, &header1));
  EXPECT_EQ(2u, header1.size);
  EXPECT_EQ(std::string("foo"), header1.entries[0].key);
  EXPECT_EQ(std::string("bar"), BufferToString(header1.entries[0].value));
  EXPECT_EQ(std::string("hoge"), header1.entries[1].key);
  EXPECT_EQ(std::string("piyo"), BufferToString(header1.entries[1].value));
  EXPECT_EQ(1u, header2.size);
  EXPECT_EQ(std::string("hoge"), header2.entries[0].key);
  EXPECT_EQ(std::string("piyo"), BufferToString(header2.entries[0].value));

  sxg_header_release(&header1);
  sxg_header_release(&header2);
}

std::string GetMapHeader(size_t length) {
  sxg_buffer_t buf = sxg_empty_buffer();
  EXPECT_TRUE(sxg_write_cbor_map_header(length, &buf));
  const std::string header = BufferToString(buf);
  sxg_buffer_release(&buf);
  return header;
}

TEST(SxgHeaderTest, CborHeader) {
  EXPECT_EQ("\xa0", GetMapHeader(0));
  EXPECT_EQ("\xa3", GetMapHeader(3));

  // 0xb7 is the biggest number represented in 1 byte.
  EXPECT_EQ("\xb7", GetMapHeader(0x17));

  // 0xb8 is the smallest number represented in 2 bytes.
  EXPECT_EQ("\xb8\x18", GetMapHeader(0x18));
  EXPECT_EQ("\xb8\xff", GetMapHeader(0xff));

  // 0x0100 is represented in 3 bytes.
  EXPECT_EQ(std::string("\xb9\x01\x00", 3), GetMapHeader(0x100));
  EXPECT_EQ("\xb9\xd3\xd7", GetMapHeader(0xd3d7));
  EXPECT_EQ("\xb9\xff\xff", GetMapHeader(0xffff));

  // 0x010000 is represented in 5 bytes.
  EXPECT_EQ(std::string("\xba\x00\x01\x00\x00", 5), GetMapHeader(0x10000));
  EXPECT_EQ("\xba\x12\x34\x56\x78", GetMapHeader(0x12345678));
  EXPECT_EQ("\xba\xff\xff\xff\xff", GetMapHeader(0xffffffff));

  // 0x0100000000 is represented in 9 bytes.
  EXPECT_EQ(std::string("\xbb\x00\x00\x00\x01\x00\x00\x00\x00", 9),
            GetMapHeader(0x100000000));
  EXPECT_EQ("\xbb\xff\xff\xff\xff\xff\xff\xff\xff",
            GetMapHeader(0xffffffffffffffff));
}

TEST(SxgHeaderTest, SerializeInCbor) {
  sxg_header_t header = sxg_empty_header();
  sxg_header_append_string("foo", "bar", &header);
  std::string expected =
      "\xa1"
      "CfooCbar";
  sxg_buffer_t output = sxg_empty_buffer();

  EXPECT_TRUE(sxg_header_serialize_cbor(&header, &output));
  EXPECT_EQ(expected, BufferToString(output));

  sxg_buffer_release(&output);
  sxg_header_release(&header);
}

TEST(SxgHeaderTest, SerializeCborIsCanonical) {
  sxg_header_t header = sxg_empty_header();
  sxg_header_append_string("looong", "value", &header);
  sxg_header_append_string("short", "value", &header);
  std::string expected(
      "\xA2"
      "EshortEvalueFlooongEvalue");  // "short" must come to the beginning.
  sxg_buffer_t output = sxg_empty_buffer();

  EXPECT_TRUE(sxg_header_serialize_cbor(&header, &output));
  EXPECT_EQ(expected, BufferToString(output));

  sxg_buffer_release(&output);
  sxg_header_release(&header);
}

TEST(SxgHeaderTest, CborCanonicalLexicographic) {
  sxg_header_t header = sxg_empty_header();
  sxg_header_append_string("BbB", "v1", &header);
  sxg_header_append_string("aAa", "v2", &header);
  std::string expected(
      "\xa2"
      "CaaaBv2CbbbBv1");  // "aaa" must come to the beginning.
  sxg_buffer_t output = sxg_empty_buffer();

  EXPECT_TRUE(sxg_header_serialize_cbor(&header, &output));
  EXPECT_EQ(expected, BufferToString(output));

  sxg_buffer_release(&output);
  sxg_header_release(&header);
}

TEST(SxgHeaderTest, DuplicatedKeyMustBeConcatenated) {
  sxg_header_t header = sxg_empty_header();
  sxg_header_append_string("bbB", "v1", &header);
  sxg_header_append_string("aaA", "v2", &header);
  sxg_header_append_string("Aaa", "v3", &header);
  sxg_header_append_string("BbB", "v4", &header);
  std::string expected(
      "\xA2"
      "CaaaEv2,v3CbbbEv1,v4");
  sxg_buffer_t output = sxg_empty_buffer();

  EXPECT_TRUE(sxg_header_serialize_cbor(&header, &output));
  EXPECT_EQ(expected, BufferToString(output));

  sxg_buffer_release(&output);
  sxg_header_release(&header);
}

}  // namespace
