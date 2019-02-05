#include <string.h>

#include <gtest/gtest.h>
#include <stdexcept>

#include "base/xml.h"

namespace s3 {
namespace base {
namespace tests {

namespace {
bool s_is_init = false;

const char *XML_0 = "<a><b></b></a>";
const char *XML_1 = "<?xml version=\"1.0\"?><a><b><c><d/></c></b></a>";
const char *XML_2 = "<?xml version=\"1.0\"?><a><b></a>";
const char *XML_3 =
    "<s3:a xmlns:s3=\"uri:something\"><s3:b><s3:c/></s3:b></s3:a>";
const char *XML_4 =
    "<a><b>element_b_0</b><c>element_c_0</c><b>element_b_1</b></a>";
const char *XML_5 = "<a><b><c>ec0</c><c>ec1</c></b><b><c>ec2</c><c>ec3</c></"
                    "b><c>ec4</c><d><e><f><c>ec5</c></f></e></d></a>";
const char *XML_6 = "<a>"
                    "  <b>"
                    "    <k00>v00</k00><k01>v01</k01><k02>v02</k02>"
                    "  </b>"
                    "  <c>"
                    "    <k10>v10</k10><k11>v11</k11><k12>v12</k12>"
                    "  </c>"
                    "  <b>"
                    "    <k20>v20</k20><k21>v21</k21><k22>v22</k22>"
                    "  </b>"
                    "</a>";

void init() {
  if (s_is_init)
    return;

  xml::init();
  s_is_init = true;
}
} // namespace

TEST(xml, match_on_no_xml_declaration) {
  init();

  EXPECT_EQ(true, xml::match(XML_0, strlen(XML_0), "/a/b"));
}

TEST(xml, fail_on_malformed_xml) {
  init();

  ASSERT_EQ(NULL, xml::parse(XML_2).get());
}

TEST(xml, match) {
  init();

  EXPECT_EQ(true, xml::match(XML_1, strlen(XML_1), "//d"));
}

TEST(xml, match_with_namespace) {
  init();

  EXPECT_EQ(true, xml::match(XML_3, strlen(XML_3), "/a/b"));
}

TEST(xml, find_single_element_b) {
  xml::document_ptr doc;
  std::string s;

  init();

  doc = xml::parse(XML_4);
  ASSERT_FALSE(doc.get() == NULL);

  xml::find(doc, "//b", &s);

  EXPECT_EQ(std::string("element_b_0"), s);
}

TEST(xml, find_single_element_second_b) {
  xml::document_ptr doc;
  std::string s;

  init();

  doc = xml::parse(XML_4);
  ASSERT_FALSE(doc.get() == NULL);

  xml::find(doc, "//b[2]", &s);

  EXPECT_EQ(std::string("element_b_1"), s);
}

TEST(xml, find_single_element_c) {
  xml::document_ptr doc;
  std::string s;

  init();

  doc = xml::parse(XML_4);
  ASSERT_FALSE(doc.get() == NULL);

  xml::find(doc, "//c", &s);

  EXPECT_EQ(std::string("element_c_0"), s);
}

TEST(xml, find_list_b) {
  xml::document_ptr doc;
  xml::element_list list;

  init();

  doc = xml::parse(XML_4);
  ASSERT_FALSE(doc.get() == NULL);

  xml::find(doc, "//b", &list);

  EXPECT_EQ(static_cast<int>(list.size()), 2);

  EXPECT_EQ(std::string("element_b_0"), list.front());
  EXPECT_EQ(std::string("element_b_1"), list.back());
}

TEST(xml, find_list_c) {
  xml::document_ptr doc;
  xml::element_list list;

  init();

  doc = xml::parse(XML_4);
  ASSERT_FALSE(doc.get() == NULL);

  xml::find(doc, "//c", &list);

  EXPECT_EQ(static_cast<int>(list.size()), 1);

  EXPECT_EQ(std::string("element_c_0"), list.front());
}

TEST(xml, find_list_c_multiple) {
  xml::document_ptr doc;
  xml::element_list list;
  int i = 0;

  init();

  doc = xml::parse(XML_5);
  ASSERT_FALSE(doc.get() == NULL);

  xml::find(doc, "//c", &list);

  EXPECT_EQ(list.size(), static_cast<size_t>(6));

  for (xml::element_list::const_iterator itor = list.begin();
       itor != list.end(); ++itor) {
    std::string exp = "ec";

    exp += std::to_string(i++);

    EXPECT_EQ(exp, *itor);
  }
}

TEST(xml, find_missing) {
  xml::document_ptr doc;
  xml::element_list list;

  init();

  doc = xml::parse(XML_5);
  ASSERT_FALSE(doc.get() == NULL);

  xml::find(doc, "/thiselementdoesntexist", &list);

  EXPECT_EQ(list.size(), static_cast<size_t>(0));
}

TEST(xml, invalid_xpath) {
  xml::document_ptr doc;
  xml::element_list list;

  init();

  doc = xml::parse(XML_5);
  ASSERT_FALSE(doc.get() == NULL);

  xml::find(doc, "//().", &list);

  EXPECT_EQ(list.size(), static_cast<size_t>(0));
}

TEST(xml, element_map) {
  xml::document_ptr doc;
  xml::element_map_list list;

  init();

  doc = xml::parse(XML_6);
  ASSERT_FALSE(doc.get() == NULL);

  xml::find(doc, "//b|//c", &list);

  EXPECT_EQ(list.size(), 3);

  xml::element_map_list::const_iterator itor = list.begin();
  xml::element_map first = *itor++;
  xml::element_map second = *itor++;
  xml::element_map third = *itor++;
  EXPECT_EQ(itor, list.end());

  EXPECT_EQ(first.size(), 4);
  EXPECT_EQ(first[xml::MAP_NAME_KEY], "b");
  EXPECT_EQ(first["k00"], "v00");
  EXPECT_EQ(first["k01"], "v01");
  EXPECT_EQ(first["k02"], "v02");

  EXPECT_EQ(second.size(), 4);
  EXPECT_EQ(second[xml::MAP_NAME_KEY], "c");
  EXPECT_EQ(second["k10"], "v10");
  EXPECT_EQ(second["k11"], "v11");
  EXPECT_EQ(second["k12"], "v12");

  EXPECT_EQ(third.size(), 4);
  EXPECT_EQ(third[xml::MAP_NAME_KEY], "b");
  EXPECT_EQ(third["k20"], "v20");
  EXPECT_EQ(third["k21"], "v21");
  EXPECT_EQ(third["k22"], "v22");
}

} // namespace tests
} // namespace base
} // namespace s3
