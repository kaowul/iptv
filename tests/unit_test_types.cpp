#include <gtest/gtest.h>

#include "stream_commands_info/statistic_info.h"

TEST(StreamStructInfo, SerializeDeSerialize) {
  iptv_cloud::StreamInfo sha;
  sha.id = "test";
  sha.input = {0, 1, 2, 3};
  sha.output = {4, 1, 2, 3};

  time_t start_time = 15;
  time_t lst = 33;
  size_t rest = 1;
  iptv_cloud::StreamStruct str(sha, start_time, lst, rest);
  iptv_cloud::StatisticInfo::cpu_load_t cpu_load = 0.33;
  iptv_cloud::StatisticInfo::rss_t rss = 12;
  time_t time = 10;

  iptv_cloud::StatisticInfo sinf(str, cpu_load, rss, time);
  json_object* serialized = NULL;
  common::Error err = sinf.Serialize(&serialized);
  ASSERT_FALSE(err);

  iptv_cloud::StatisticInfo sinf2;
  err = sinf2.DeSerialize(serialized);
  ASSERT_FALSE(err);
  // ASSERT_EQ(sinf.GetStreamStruct(), sinf2.GetStreamStruct());
  ASSERT_EQ(sinf.GetCpuLoad(), sinf2.GetCpuLoad());
  ASSERT_EQ(sinf.GetRss(), sinf2.GetRss());
  ASSERT_EQ(sinf.GetTimestamp(), sinf2.GetTimestamp());

  json_object_put(serialized);
}
