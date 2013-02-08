#include <string.h>

#include <stdexcept>
#include <gtest/gtest.h>

#include "base/xml.h"

using s3::base::xml;

namespace
{
  bool s_is_init = false;

  const char *XML_0 = "<a><b></b></a>";
  const char *XML_1 = "<?xml version=\"1.0\"?><a><b><c><d/></c></b></a>";
  const char *XML_2 = "<?xml version=\"1.0\"?><a><b></a>";
  const char *XML_3 = "<s3:a xmlns:s3=\"uri:something\"><s3:b><s3:c/></s3:b></s3:a>";

  void init()
  {
    if (s_is_init)
      return;

    xml::init();
    s_is_init = true;
  }
}

TEST(xml, match_on_no_xml_declaration)
{
  init();

  EXPECT_EQ(true, xml::match(XML_0, strlen(XML_0), "/a/b"));
}

TEST(xml, fail_on_malformed_xml)
{
  init();

  ASSERT_EQ(NULL, xml::parse(XML_2).get());
}

TEST(xml, match)
{
  init();

  EXPECT_EQ(true, xml::match(XML_1, strlen(XML_1), "//d"));
}

TEST(xml, match_with_namespace)
{
  init();

  EXPECT_EQ(true, xml::match(XML_3, strlen(XML_3), "/a/b"));
}
