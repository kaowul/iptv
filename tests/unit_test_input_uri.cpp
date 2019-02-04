#include "gtest/gtest.h"

#include <json-c/json_object.h>

#include "input_uri.h"

#define RTMP_INPUT "rtmp://4.31.30.153:1935/devapp/tokengenffmpeg1"
#define FILE_INPUT "file:///home/sasha/2.txt"
#define DEVICE_VIDEO "/dev/video3"
#define DEVICE_AUDIO "audio=hw:3,0"
#define DEVICE_INPUT "dev://" DEVICE_VIDEO "?" DEVICE_AUDIO

TEST(InputUri, ConvertFromString) {
  const std::string invalid_uri_json =
      "{ \"id\": 0, \"uri\": \"\", \"mute\": false, \"relay_video\": false, \"relay_audio\": false }";
  iptv_cloud::InputUri invalid_uri;
  ASSERT_EQ(invalid_uri.GetID(), 0);
  ASSERT_EQ(invalid_uri.GetInput(), common::uri::Url());
  std::string conv;
  common::Error err = invalid_uri.SerializeToString(&conv);
  ASSERT_FALSE(err);
  ASSERT_EQ(conv, invalid_uri_json);

  const std::string uri_json = "{ \"id\": 1, \"uri\": \"" RTMP_INPUT
                               "\", \"volume\": 1.00, \"mute\": 0, \"relay_video\": false, \"relay_audio\": false}";
  iptv_cloud::InputUri uri;
  err = uri.DeSerializeFromString(uri_json);
  ASSERT_FALSE(err);
  ASSERT_EQ(uri.GetID(), 1);
  common::uri::Url ro(RTMP_INPUT);
  ASSERT_EQ(uri.GetInput(), ro);
  // conv = common::ConvertToString(uri);
  // ASSERT_EQ(conv, uri_json);

  const std::string file_uri_json = "{ \"id\": 2, \"uri\": \"" FILE_INPUT
                                    "\", \"volume\": 1.0, \"mute\": false, \"relay_video\": 0, \"relay_audio\": 0}";
  iptv_cloud::InputUri file_uri;
  err = file_uri.DeSerializeFromString(file_uri_json);
  ASSERT_FALSE(err);
  ASSERT_EQ(file_uri.GetID(), 2);
  common::uri::Url file_ro(FILE_INPUT);
  ASSERT_EQ(file_uri.GetInput(), file_ro);
  // conv = common::ConvertToString(file_uri);
  // ASSERT_EQ(conv, file_uri_json);

  const std::string dev_uri_json = "{ \"id\": 2, \"uri\": \"" DEVICE_INPUT
                                   "\", \"volume\": 1.00, \"mute\": 0, \"relay_video\": 0, \"relay_audio\": 0}";
  iptv_cloud::InputUri dev_uri;
  err = dev_uri.DeSerializeFromString(dev_uri_json);
  ASSERT_FALSE(err);
  ASSERT_EQ(dev_uri.GetID(), 2);
  common::uri::Url dev_ro(DEVICE_INPUT);
  ASSERT_EQ(dev_uri.GetInput(), dev_ro);
  ASSERT_TRUE(dev_ro.GetScheme() == common::uri::Url::dev);
  common::uri::Upath dpath = dev_ro.GetPath();
  ASSERT_EQ(dpath.GetPath(), DEVICE_VIDEO);
  ASSERT_EQ(dpath.GetQuery(), DEVICE_AUDIO);
  // conv = common::ConvertToString(dev_uri);
  // ASSERT_EQ(conv, dev_uri_json);
}
