/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019-2020 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright holder for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include "../env.h"

#include <packeteer.h>
#include <packeteer/registry.h>
#include <packeteer/connector.h>

/***************************************************************************
 * Query parameter interface
 */

TEST(Registry, param_empty_name)
{
  using namespace packeteer;
  auto api = api::create();

  ASSERT_EQ(ERR_INVALID_VALUE, api->reg().add_parameter("", nullptr));
}



TEST(Registry, param_empty_mapper)
{
  using namespace packeteer;
  auto api = api::create();

  ASSERT_EQ(ERR_EMPTY_CALLBACK, api->reg().add_parameter("foo", nullptr));
}



TEST(Registry, param_duplicate)
{
  using namespace packeteer;
  auto api = api::create();

  auto dummy = [] (std::string const &, bool) -> connector_options {
    // Return CO_DEFAULT to not mess with other tests by always setting
    // an option.
    return CO_DEFAULT;
  };

  ASSERT_EQ(ERR_SUCCESS, api->reg().add_parameter("foo", dummy));
  ASSERT_EQ(ERR_INVALID_VALUE, api->reg().add_parameter("foo", dummy));

  // Same mapper for a different name works
  ASSERT_EQ(ERR_SUCCESS, api->reg().add_parameter("bar", dummy));
}


TEST(Registry, parse_user)
{
  using namespace packeteer;
  auto api = api::create();

  std::map<std::string, std::string> query;
  query["foo"] = "bar";

  // First test: ensure that "foo" is not recognized. The global default is
  // to use non-blocking operations, so that should be the only option set.
  auto options = api->reg().options_from_query(query);
  ASSERT_EQ(CO_NON_BLOCKING, options);

  // Register foo, and try again.
  auto dummy = [] (std::string const &, bool found) -> connector_options {
    if (!found) {
      return CO_DEFAULT;
    }
    return CO_USER + 42;
  };
  ASSERT_EQ(ERR_SUCCESS, api->reg().add_parameter("foo", dummy));

  // First, if "foo" isn't set, we should still get the same result.
  options = api->reg().options_from_query({});
  ASSERT_EQ(CO_NON_BLOCKING, options);

  // However, with "foo" provided, we should see the user flag
  options = api->reg().options_from_query(query);
  ASSERT_EQ(CO_NON_BLOCKING | (CO_USER + 42), options);
}


/***************************************************************************
 * Scheme interface
 */
namespace {

struct test_connector : packeteer::connector_interface
{
  test_connector()
  {
  }

  virtual ~test_connector() {};

  virtual packeteer::error_t listen() { return packeteer::ERR_SUCCESS; }
  virtual bool listening() const { return false; }

  virtual packeteer::error_t connect() { return packeteer::ERR_SUCCESS; }
  virtual bool connected() const { return false; }

  virtual packeteer::connector_interface *
  accept(liberate::net::socket_address &)
  {
    return nullptr;
  }

  virtual packeteer::handle get_read_handle() const
  {
    return packeteer::handle();
  }
  virtual packeteer::handle get_write_handle() const
  {
    return packeteer::handle();
  }

  virtual packeteer::error_t close() { return packeteer::ERR_SUCCESS; }

  virtual bool is_blocking() const { return true; }
  virtual packeteer::connector_options get_options() const { return packeteer::CO_DEFAULT; }

  virtual packeteer::error_t receive(void *, size_t, size_t &,
      ::liberate::net::socket_address &) { return packeteer::ERR_SUCCESS; }
  virtual packeteer::error_t send(void const *, size_t, size_t &,
      ::liberate::net::socket_address const &) { return packeteer::ERR_SUCCESS; }
  virtual size_t peek() const { return 0; }

  virtual packeteer::error_t read(void *, size_t, size_t &) { return packeteer::ERR_SUCCESS; }
  virtual packeteer::error_t write(void const *, size_t, size_t &) { return packeteer::ERR_SUCCESS; }
};

} // anonymous namespace


TEST(Registry, scheme_missing)
{
  using namespace packeteer;
  auto api = api::create();

  ASSERT_THROW(api->reg().info_for_scheme("test"), exception);
  ASSERT_THROW(api->reg().info_for_type(1234), exception);
}



TEST(Registry, scheme_empty_name)
{
  using namespace packeteer;
  auto api = api::create();

  auto info = registry::connector_info{CT_USER + 42, CO_STREAM|CO_NON_BLOCKING,
    CO_STREAM|CO_NON_BLOCKING|CO_BLOCKING|(CT_USER + 42),
    [] (liberate::net::url const &, connector_type const &, connector_options const &, registry::connector_info const *) -> connector_interface *
    {
      return nullptr;
    }
  };

  ASSERT_EQ(ERR_INVALID_VALUE, api->reg().add_scheme("", info));
}



TEST(Registry, scheme_bad_type)
{
  using namespace packeteer;
  auto api = api::create();

  auto info = registry::connector_info{CT_UNSPEC, CO_STREAM|CO_NON_BLOCKING,
    CO_STREAM|CO_NON_BLOCKING|CO_BLOCKING|(CT_USER + 42),
    [] (liberate::net::url const &, connector_type const &, connector_options const &, registry::connector_info const *) -> connector_interface *
    {
      return nullptr;
    }
  };

  ASSERT_EQ(ERR_INVALID_VALUE, api->reg().add_scheme("test", info));
}



TEST(Registry, scheme_empty_creator)
{
  using namespace packeteer;
  auto api = api::create();

  auto info = registry::connector_info{CT_USER + 42, CO_STREAM|CO_NON_BLOCKING,
    CO_STREAM|CO_NON_BLOCKING|CO_BLOCKING|(CT_USER + 42),
    nullptr
  };

  ASSERT_EQ(ERR_EMPTY_CALLBACK, api->reg().add_scheme("test", info));
}



TEST(Registry, scheme_register_success)
{
  using namespace packeteer;
  auto api = api::create();

  auto info = registry::connector_info{CT_USER + 42, CO_STREAM|CO_NON_BLOCKING,
    CO_STREAM|CO_NON_BLOCKING|CO_BLOCKING|(CT_USER + 42),
    [] (liberate::net::url const &, connector_type const &, connector_options const &, registry::connector_info const *) -> connector_interface *
    {
      return nullptr;
    }
  };

  ASSERT_EQ(ERR_SUCCESS, api->reg().add_scheme("test", info));
}



TEST(Registry, scheme_fail_instantiation)
{
  using namespace packeteer;
  auto api = api::create();

  auto info = registry::connector_info{CT_USER + 42, CO_STREAM|CO_NON_BLOCKING,
    CO_STREAM|CO_NON_BLOCKING|CO_BLOCKING|(CT_USER + 42),
    [] (liberate::net::url const &, connector_type const &, connector_options const &, registry::connector_info const *) -> connector_interface *
    {
      return nullptr;
    }
  };

  EXPECT_EQ(ERR_SUCCESS, api->reg().add_scheme("test", info));

  // Now if we create a connector with the "test" scheme, we should get an error
  // because the scheme instantiates nothing.

  EXPECT_THROW(new connector(api, "test://foo"), exception);
}



TEST(Registry, scheme_instantiation)
{
  using namespace packeteer;
  auto api = api::create();

  auto info = registry::connector_info{CT_USER + 42, CO_STREAM|CO_NON_BLOCKING,
    CO_STREAM|CO_NON_BLOCKING|CO_BLOCKING|(CT_USER + 42),
    [] (liberate::net::url const &, connector_type const &, connector_options const &, registry::connector_info const *) -> connector_interface *
    {
      return new test_connector{};
    }
  };

  EXPECT_EQ(ERR_SUCCESS, api->reg().add_scheme("test", info));

  // Now if we create a connector with the "test" scheme, it should work.
  connector test{api, "test://foo"};
}
