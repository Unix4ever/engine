#include "GsageDefinitions.h"
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include "Serializer.h"

#include <json/json.h>
#include <gtest/gtest.h>

using namespace Gsage;


class Target {
  public:
    void setProperty(int value)
    {
      mInt = value;
    }

    int getProperty() const
    {
      return mInt;
    }

    int getPropertyWithoutConst()
    {
      return mInt;
    }

    void setConstProperty(const std::string& property)
    {
      mString = property;
    }

    const std::string& getConstProperty() const
    {
      return mString;
    }
  private:
    std::string mString;
    int mInt;
    std::string mVar;
};

template<>
class Serializer<Target> {
  public:
    template<class C>
    bool dump(const Target& src, C& dest)
    {
      typename Writer<C>::type().write(std::string("intval"), src.getProperty(), dest);
      return true;
    }
};

class Object {
  public:
    std::string mValue;
};

struct ObjectPreprocessor {
  bool write(const Object& src, const std::string& dst) {
    return true;
  };
};

template<>
struct Preprocessor<Object> {
  typedef ObjectPreprocessor type;
};


TEST(TestDump, TestSerializer)
{
  Target t;
  t.setProperty(10);
  Json::Value root;
  bool success = SerializationFactory::dump(t, root);
  ASSERT_TRUE(success);
//  Serializer().dump(t, root);
  ASSERT_EQ(root["intval"], 10);

  /*SerializationRule<Target>(
      Property("intval", &Target::getProperty(), &Target::setProperty()),
      Property("object", &Target::getObject(), &Target::setObject()),
      Property("converted_to_string", &Target:getObject(), &Target::setObject(), Stringify)
  );*/
}
