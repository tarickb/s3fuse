#include <string.h>

#include <mutex>
#include <stdexcept>

#include <gtest/gtest.h>

#include "base/xml.h"

namespace s3 {
namespace base {
namespace tests {

namespace {
constexpr char XML_SAMPLE_TWO_LEVEL[] =
    "<a><b>element_b_0</b><c>element_c_0</c><b>element_b_1</b></a>";

constexpr char XML_SAMPLE_FOUR_LEVEL[] =
    "<a><b><c>ec0</c><c>ec1</c></b><b><c>ec2</c><c>ec3</c></b><c>ec4</"
    "c><d><e><f><c>ec5</c></f></e></d></a>";

class Xml : public ::testing::Test {
 protected:
  void SetUp() override {
    static std::once_flag flag;
    std::call_once(flag, []() { XmlDocument::Init(); });
  }
};
}  // namespace

TEST_F(Xml, MatchOnNoXmlDeclaration) {
  constexpr char XML[] = "<a><b></b></a>";
  auto doc = XmlDocument::Parse(XML);
  ASSERT_TRUE(doc);
  EXPECT_EQ(true, doc->Match("/a/b"));
}

TEST_F(Xml, FailOnMalformedXml) {
  constexpr char XML[] = "<?xml version=\"1.0\"?><a><b></a>";
  auto doc = XmlDocument::Parse(XML);
  ASSERT_FALSE(doc);
}

TEST_F(Xml, Match) {
  constexpr char XML[] = "<?xml version=\"1.0\"?><a><b><c><d/></c></b></a>";
  auto doc = XmlDocument::Parse(XML);
  ASSERT_TRUE(doc);
  EXPECT_EQ(true, doc->Match("//d"));
}

TEST_F(Xml, MatchWithNamespace) {
  constexpr char XML[] =
      "<s3:a xmlns:s3=\"uri:something\"><s3:b><s3:c/></s3:b></s3:a>";
  auto doc = XmlDocument::Parse(XML);
  ASSERT_TRUE(doc);
  EXPECT_EQ(true, doc->Match("/a/b"));
}

TEST_F(Xml, FindSingleElementB) {
  auto doc = XmlDocument::Parse(XML_SAMPLE_TWO_LEVEL);
  ASSERT_TRUE(doc);
  std::string s;
  ASSERT_EQ(doc->Find("//b", &s), 0);
  EXPECT_EQ(s, "element_b_0");
}

TEST_F(Xml, FindSingleElementSecondB) {
  auto doc = XmlDocument::Parse(XML_SAMPLE_TWO_LEVEL);
  ASSERT_TRUE(doc);
  std::string s;
  ASSERT_EQ(doc->Find("//b[2]", &s), 0);
  EXPECT_EQ(s, "element_b_1");
}

TEST_F(Xml, FindSingleElementC) {
  auto doc = XmlDocument::Parse(XML_SAMPLE_TWO_LEVEL);
  ASSERT_TRUE(doc);
  std::string s;
  ASSERT_EQ(doc->Find("//c", &s), 0);
  EXPECT_EQ(s, "element_c_0");
}

TEST_F(Xml, FindListB) {
  auto doc = XmlDocument::Parse(XML_SAMPLE_TWO_LEVEL);
  ASSERT_TRUE(doc);
  std::list<std::string> l;
  ASSERT_EQ(doc->Find("//b", &l), 0);
  ASSERT_EQ(l.size(), 2ul);
  EXPECT_EQ(l.front(), "element_b_0");
  EXPECT_EQ(l.back(), "element_b_1");
}

TEST_F(Xml, FindListC) {
  auto doc = XmlDocument::Parse(XML_SAMPLE_TWO_LEVEL);
  ASSERT_TRUE(doc);
  std::list<std::string> l;
  ASSERT_EQ(doc->Find("//c", &l), 0);
  ASSERT_EQ(l.size(), 1ul);
  EXPECT_EQ(l.front(), "element_c_0");
}

TEST_F(Xml, FindListCMultiple) {
  auto doc = XmlDocument::Parse(XML_SAMPLE_FOUR_LEVEL);
  ASSERT_TRUE(doc);
  std::list<std::string> l;
  ASSERT_EQ(doc->Find("//c", &l), 0);
  ASSERT_EQ(l.size(), 6ul);
  int i = 0;
  for (auto iter = l.begin(); iter != l.end(); ++iter) {
    std::string exp = "ec" + std::to_string(i++);
    EXPECT_EQ(exp, *iter);
  }
}

TEST_F(Xml, FindMissing) {
  auto doc = XmlDocument::Parse(XML_SAMPLE_FOUR_LEVEL);
  ASSERT_TRUE(doc);
  std::list<std::string> l;
  ASSERT_EQ(doc->Find("//thiselementdoesntexist", &l), 0);
  EXPECT_EQ(l.size(), 0ul);
}

TEST_F(Xml, InvalidXPath) {
  auto doc = XmlDocument::Parse(XML_SAMPLE_FOUR_LEVEL);
  ASSERT_TRUE(doc);
  std::list<std::string> l;
  ASSERT_NE(doc->Find("//().", &l), 0);
  EXPECT_EQ(l.size(), 0ul);
}

TEST_F(Xml, ElementMap) {
  constexpr char XML[] =
      "<a>"
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
  auto doc = XmlDocument::Parse(XML);
  ASSERT_TRUE(doc);
  std::list<std::map<std::string, std::string>> l;
  ASSERT_EQ(doc->Find("//b|//c", &l), 0);
  ASSERT_EQ(l.size(), 3ul);

  auto iter = l.begin();
  auto first = *iter++;
  auto second = *iter++;
  auto third = *iter++;
  EXPECT_EQ(iter, l.end());

  EXPECT_EQ(first.size(), 4ul);
  EXPECT_EQ(first[XmlDocument::MAP_NAME_KEY], "b");
  EXPECT_EQ(first["k00"], "v00");
  EXPECT_EQ(first["k01"], "v01");
  EXPECT_EQ(first["k02"], "v02");

  EXPECT_EQ(second.size(), 4ul);
  EXPECT_EQ(second[XmlDocument::MAP_NAME_KEY], "c");
  EXPECT_EQ(second["k10"], "v10");
  EXPECT_EQ(second["k11"], "v11");
  EXPECT_EQ(second["k12"], "v12");

  EXPECT_EQ(third.size(), 4ul);
  EXPECT_EQ(third[XmlDocument::MAP_NAME_KEY], "b");
  EXPECT_EQ(third["k20"], "v20");
  EXPECT_EQ(third["k21"], "v21");
  EXPECT_EQ(third["k22"], "v22");
}

}  // namespace tests
}  // namespace base
}  // namespace s3
