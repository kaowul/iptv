#include "gtest/gtest.h"

#include "constants.h"

#include "server/options/options.h"
#include "utils/arg_converter.h"

#define LOGO_FIELD "logo"

namespace {
const std::string kTimeshiftRecorderConfig = R"({
    "id" : "test_1",
    "input" : {"urls" : [ {"id" : 1, "uri" : "http://example.com/manager/fo/forward2.php?cid=14"} ]},
    "output" : {"urls" : [ {"id" : 80, "timeshift_dir" : "/var/www/html/live/14"} ]},
    "type" : 3
  })";
}

TEST(Options, logo_path) {
  std::string cfg = "{\"" LOGO_FIELD "\" : {\"path\": \"file:///home/user/logo.png\"}}";
  auto args = iptv_cloud::server::options::ValidateConfig(cfg);
  ASSERT_EQ(args.size(), 1);

  cfg = "{\"" LOGO_FIELD "\" : {\"path\": \"http://home/user/logo.png\"}}";
  args = iptv_cloud::server::options::ValidateConfig(cfg);
  ASSERT_EQ(args.size(), 1);
}

TEST(Options, cfgs) {
  auto args = iptv_cloud::server::options::ValidateConfig(kTimeshiftRecorderConfig);
  ASSERT_EQ(args.size(), 4);
}
