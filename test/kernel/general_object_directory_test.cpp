// Copyright 2019 Lawrence Livermore National Security, LLC and other Metall Project Developers.
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include "gtest/gtest.h"
#include <memory>
#include <metall/kernel/attributed_object_directory.hpp>
#include "../test_utility.hpp"

namespace {

using directory_type = metall::kernel::attributed_object_directory<ssize_t, std::size_t>;

TEST(GeneralObjectDirectoryTest, Insert) {
  directory_type obj;

  ASSERT_TRUE(obj.insert("item1", 1, 2));
  ASSERT_TRUE(obj.insert("item2", 3, 4, "description2"));
}

// Should insert uniquely
TEST(GeneralObjectDirectoryTest, UniqueInsert) {
  directory_type obj;

  obj.insert("item1", 1, 2);
  ASSERT_FALSE(obj.insert("item1", 1, 2));

  obj.insert("item2", 3, 4, "description2");
  ASSERT_FALSE(obj.insert("item2", 3, 4));
}

TEST(GeneralObjectDirectoryTest, CountByName) {
  directory_type obj;

  ASSERT_EQ(obj.count("item1"), 0);
  obj.insert("item1", 1, 2);
  ASSERT_EQ(obj.count("item1"), 1);

  ASSERT_EQ(obj.count("item2"), 0);
  obj.insert("item2", 3, 4);
  ASSERT_EQ(obj.count("item2"), 1);
}

TEST(GeneralObjectDirectoryTest, CountByOffset) {
  directory_type obj;

  ASSERT_EQ(obj.count(1), 0); // count by offset
  obj.insert("item1", 1, 2);
  ASSERT_EQ(obj.count(1), 1);

  ASSERT_EQ(obj.count(3), 0); // count by offset
  obj.insert("item2", 3, 4);
  ASSERT_EQ(obj.count(3), 1);
}

TEST(GeneralObjectDirectoryTest, FindByName) {
  directory_type obj;

  ASSERT_EQ(obj.find("item1"), obj.end());
  obj.insert("item1", 1, 2);
  ASSERT_EQ(obj.find("item1")->name, "item1");

  ASSERT_EQ(obj.find("item2"), obj.end());
  obj.insert("item2", 3, 4);
  ASSERT_EQ(obj.find("item2")->name, "item2");
}

TEST(GeneralObjectDirectoryTest, FindByOffset) {
  directory_type obj;

  ASSERT_EQ(obj.find(1), obj.end()); // find by offset
  obj.insert("item1", 1, 2);
  ASSERT_EQ(obj.find(1)->name, "item1");

  ASSERT_EQ(obj.find(3), obj.end()); // find by offset
  obj.insert("item2", 3, 4);
  ASSERT_EQ(obj.find(3)->name, "item2");
}

TEST(GeneralObjectDirectoryTest, EraseByIterator) {
  directory_type obj;

  obj.insert("item1", 1, 2);
  obj.insert("item2", 3, 4);

  ASSERT_EQ(obj.erase(obj.find("item1")), 1);
  ASSERT_EQ(obj.count("item1"), 0);

  ASSERT_EQ(obj.erase(obj.find("item2")), 1);
  ASSERT_EQ(obj.count("item2"), 0);

  ASSERT_EQ(obj.erase(obj.end()), 0);
}

TEST(GeneralObjectDirectoryTest, EraseByName) {
  directory_type obj;

  ASSERT_EQ(obj.erase("item1"), 0);
  obj.insert("item1", 1, 2);

  ASSERT_EQ(obj.erase("item2"), 0);
  obj.insert("item2", 3, 4);

  ASSERT_EQ(obj.erase("item1"), 1);
  ASSERT_EQ(obj.count("item1"), 0);
  ASSERT_EQ(obj.erase("item1"), 0);

  ASSERT_EQ(obj.erase("item2"), 1);
  ASSERT_EQ(obj.count("item2"), 0);
  ASSERT_EQ(obj.erase("item2"), 0);
}

TEST(GeneralObjectDirectoryTest, EraseByOffset) {
  directory_type obj;

  ASSERT_EQ(obj.erase(1), 0);
  obj.insert("item1", 1, 2);

  ASSERT_EQ(obj.erase(3), 0);
  obj.insert("item2", 3, 4);

  ASSERT_EQ(obj.erase(1), 1);
  ASSERT_EQ(obj.count(1), 0);
  ASSERT_EQ(obj.erase(1), 0);

  ASSERT_EQ(obj.erase(3), 1);
  ASSERT_EQ(obj.count(3), 0);
  ASSERT_EQ(obj.erase(3), 0);
}

TEST(GeneralObjectDirectoryTest, Iterator) {
  directory_type obj;

  ASSERT_EQ(obj.begin(), obj.end());
  obj.insert("item1", 1, 2);
  obj.insert("item2", 3, 4);

  int count = 0;
  bool found1 = false;
  bool found2 = false;
  for (auto itr = obj.begin(); itr != obj.end(); ++itr) {
    ASSERT_TRUE(itr->name == "item1" || itr->name == "item2");
    found1 |= (itr->name == "item1");
    found2 |= (itr->name == "item2");
    ++count;
  }
  ASSERT_TRUE(found1);
  ASSERT_TRUE(found2);
  ASSERT_EQ(count, 2);

  obj.erase("item1");
  ASSERT_EQ(obj.begin()->name, "item2");

  obj.erase("item2");
  ASSERT_EQ(obj.begin(), obj.end());
}

TEST(GeneralObjectDirectoryTest, Serialize) {
  directory_type obj;

  obj.insert("item1", 1, 2);
  obj.insert("item2", 3, 4, "description2");

  test_utility::create_test_dir();
  const auto file(test_utility::make_test_path());

  ASSERT_TRUE(obj.serialize(file.c_str()));
}

TEST(GeneralObjectDirectoryTest, Deserialize) {
  test_utility::create_test_dir();
  const auto file(test_utility::make_test_path());

  {
    directory_type obj;
    obj.insert("item1", 1, 2);
    obj.insert("item2", 3, 4, "description2");
    obj.serialize(file.c_str());
  }

  {
    directory_type obj;
    ASSERT_TRUE(obj.deserialize(file.c_str()));

    // Get values correctly
    const auto itr1 = obj.find("item1");
    ASSERT_EQ(itr1->name, "item1");
    ASSERT_EQ(itr1->offset, 1);
    ASSERT_EQ(itr1->length, 2);
    ASSERT_TRUE(itr1->description.empty());


    // Get values correctly
    const auto itr2 = obj.find("item2");
    ASSERT_EQ(itr2->name, "item2");
    ASSERT_EQ(itr2->offset, 3);
    ASSERT_EQ(itr2->length, 4);
    ASSERT_EQ(itr2->description, "description2");
  }
}

}