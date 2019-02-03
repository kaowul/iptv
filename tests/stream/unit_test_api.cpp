#include "gtest/gtest.h"

#include "config.h"
#include "config_fields.h"
#include "constants.h"
#include "inputs_outputs.h"

#include "stream/configs_factory.h"
#include "stream/stypes.h"

TEST(Api, init) {
  iptv_cloud::utils::ArgsMap emp;
  iptv_cloud::stream::Config* empty_api = nullptr;
  common::Error err = iptv_cloud::stream::make_config(emp, &empty_api);
  ASSERT_TRUE(err);
  iptv_cloud::output_t ouri;
  iptv_cloud::OutputUri uri;
  uri.SetOutput(common::uri::Url("screen"));
  ouri.push_back(uri);

  ASSERT_TRUE(iptv_cloud::IsTestUrl(iptv_cloud::InputUri(0, common::uri::Url(TEST_URL))));

  ASSERT_TRUE(iptv_cloud::stream::IsScreenUrl(common::uri::Url(SCREEN_URL)));
  ASSERT_TRUE(iptv_cloud::stream::IsRecordingUrl(common::uri::Url(RECORDING_URL)));
}

TEST(Api, logo) {
  iptv_cloud::output_t ouri;
  iptv_cloud::OutputUri uri2;
  uri2.SetOutput(common::uri::Url("screen"));
  ouri.push_back(uri2);
}
