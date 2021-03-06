#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rtp/SequenceNumberTranslator.h>
#include <rtp/RtpHeaders.h>
#include <MediaDefinitions.h>
#include <WebRtcConnection.h>

#include <queue>
#include <string>
#include <vector>

#include "../utils/Mocks.h"
#include "../utils/Tools.h"
#include "../utils/Matchers.h"

using ::testing::_;
using ::testing::IsNull;
using ::testing::Eq;
using ::testing::Args;
using ::testing::Return;
using erizo::SequenceNumberTranslator;
using erizo::SequenceNumber;
using erizo::SequenceNumberType;

enum class PacketState {
  Forward = 0,
  Skip = 1,
  Generate = 2,
  Reset = 3
};

struct Packet {
  int sequence_number;
  PacketState state;
  int expected_output;
  SequenceNumberType expected_type;
};

class SequenceNumberTranslatorTest : public ::testing::TestWithParam<std::vector<Packet>> {
 public:
  SequenceNumberTranslatorTest() {
    queue = GetParam();
  }
 protected:
  void SetUp() {
  }

  void TearDown() {
  }

  SequenceNumberTranslator translator;
  std::vector<Packet> queue;
};

TEST_P(SequenceNumberTranslatorTest, shouldReturnRightOutputSequenceNumbers) {
  for (Packet packet : queue) {
    bool skip = packet.state == PacketState::Skip;
    SequenceNumber output;
    if (packet.state == PacketState::Reset) {
      translator.reset();
      continue;
    } else if (packet.state == PacketState::Generate) {
      output = translator.generate();
    } else {
      output = translator.get(packet.sequence_number, skip);
    }
    if (output.type == SequenceNumberType::Valid) {
      EXPECT_THAT(output.output, Eq(packet.expected_output));
    }
    EXPECT_THAT(output.type, Eq(packet.expected_type));

    translator.reverse(packet.expected_output);
    if (output.type == SequenceNumberType::Valid) {
      ASSERT_THAT(output.input, Eq(packet.sequence_number));
    }
    ASSERT_THAT(output.type, Eq(packet.expected_type));
  }
}

std::vector<Packet> getLongQueue(int size) {
  std::vector<Packet> queue;
  for (int i = 0; i <= size; i++) {
    int input_sequence_number = i % 65535;
    queue.push_back(Packet{input_sequence_number, PacketState::Forward,
                           input_sequence_number, SequenceNumberType::Valid});
  }
  return queue;
}

INSTANTIATE_TEST_CASE_P(
  SequenceNumberScenarios, SequenceNumberTranslatorTest, testing::Values(
    //                     input                        expected_output
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     6, PacketState::Forward,               6, SequenceNumberType::Valid},
                         {     7, PacketState::Forward,               7, SequenceNumberType::Valid},
                         {     8, PacketState::Forward,               8, SequenceNumberType::Valid}}),

    // Packet reordering
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     6, PacketState::Forward,               6, SequenceNumberType::Valid},
                         {     8, PacketState::Forward,               8, SequenceNumberType::Valid},
                         {     7, PacketState::Forward,               7, SequenceNumberType::Valid}}),

    // Skip packets
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     6, PacketState::Forward,               6, SequenceNumberType::Valid},
                         {     7, PacketState::Skip,                  7, SequenceNumberType::Skip},
                         {     8, PacketState::Forward,               7, SequenceNumberType::Valid},
                         {     9, PacketState::Forward,               8, SequenceNumberType::Valid}}),

    // Discard packets
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     6, PacketState::Forward,               6, SequenceNumberType::Valid},
                         {     8, PacketState::Forward,               8, SequenceNumberType::Valid},
                         {     7, PacketState::Skip,                  7, SequenceNumberType::Discard},
                         {     9, PacketState::Forward,               9, SequenceNumberType::Valid}}),


    // Retransmissions
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     6, PacketState::Forward,               6, SequenceNumberType::Valid},
                         {     7, PacketState::Forward,               7, SequenceNumberType::Valid},
                         {     8, PacketState::Forward,               8, SequenceNumberType::Valid},
                         {     7, PacketState::Forward,               7, SequenceNumberType::Valid}}),

    // Retransmissions of skipped packets
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     6, PacketState::Forward,               6, SequenceNumberType::Valid},
                         {     7, PacketState::Skip,                  7, SequenceNumberType::Skip},
                         {     8, PacketState::Forward,               7, SequenceNumberType::Valid},
                         {     7, PacketState::Skip,                  7, SequenceNumberType::Skip}}),

    // Rollover
    std::vector<Packet>({{ 65535, PacketState::Forward,           65535, SequenceNumberType::Valid},
                         {     0, PacketState::Forward,               0, SequenceNumberType::Valid}}),

    // Rollover with packets reordering
    std::vector<Packet>({{ 65535, PacketState::Forward,           65535, SequenceNumberType::Valid},
                         {     1, PacketState::Forward,               1, SequenceNumberType::Valid},
                         {     0, PacketState::Forward,               0, SequenceNumberType::Valid}}),

    // Rollover with packets skipped
    std::vector<Packet>({{ 65535, PacketState::Forward,           65535, SequenceNumberType::Valid},
                         {     0, PacketState::Skip,                  0, SequenceNumberType::Skip},
                         {     1, PacketState::Forward,               0, SequenceNumberType::Valid},
                         {     2, PacketState::Forward,               1, SequenceNumberType::Valid}}),

    // Rollover with packets skipped
    std::vector<Packet>({{ 65535, PacketState::Forward,           65535, SequenceNumberType::Valid},
                         {     1, PacketState::Forward,               1, SequenceNumberType::Valid},
                         {     0, PacketState::Skip,                  0, SequenceNumberType::Discard},
                         {     2, PacketState::Forward,               2, SequenceNumberType::Valid}}),

    // Reset after having received skipped packets
    std::vector<Packet>({{  5059, PacketState::Skip,               5059, SequenceNumberType::Skip},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {  1032, PacketState::Skip,               1032, SequenceNumberType::Skip},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         { 23537, PacketState::Forward,           23537, SequenceNumberType::Valid}}),


    // Reset after having received skipped packets
    std::vector<Packet>({{  5059, PacketState::Skip,               5059, SequenceNumberType::Skip},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         { 23537, PacketState::Forward,           23537, SequenceNumberType::Valid}}),

    // Reset after having received skipped packets
    std::vector<Packet>({{  5058, PacketState::Forward,            5058, SequenceNumberType::Valid},
                         {  5059, PacketState::Skip,               5059, SequenceNumberType::Skip},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         { 23537, PacketState::Forward,            5059, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{ 50000, PacketState::Forward,           50000, SequenceNumberType::Valid},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         { 23537, PacketState::Forward,           50001, SequenceNumberType::Valid},
                         { 23538, PacketState::Forward,           50002, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{     0, PacketState::Generate,              1, SequenceNumberType::Generated},
                         {     6, PacketState::Forward,               2, SequenceNumberType::Valid},
                         {     7, PacketState::Forward,               3, SequenceNumberType::Valid},
                         {     8, PacketState::Forward,               4, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{     6, PacketState::Forward,               6, SequenceNumberType::Valid},
                         {    10, PacketState::Skip,                 10, SequenceNumberType::Skip},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {   301, PacketState::Skip,                  7, SequenceNumberType::Skip},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {   901, PacketState::Forward,               7, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{     6, PacketState::Forward,               6, SequenceNumberType::Valid},
                         {    10, PacketState::Skip,                 10, SequenceNumberType::Skip},
                         {     9, PacketState::Forward,               9, SequenceNumberType::Valid},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {   301, PacketState::Skip,                301, SequenceNumberType::Skip},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {   901, PacketState::Forward,              10, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{     6, PacketState::Forward,               6, SequenceNumberType::Valid},
                         {    10, PacketState::Skip,                 10, SequenceNumberType::Skip},
                         {     0, PacketState::Generate,              7, SequenceNumberType::Generated},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {   301, PacketState::Skip,                301, SequenceNumberType::Skip},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {   901, PacketState::Forward,               8, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{  5059, PacketState::Skip,               5059, SequenceNumberType::Skip},
                         {    30, PacketState::Forward,              30, SequenceNumberType::Valid},
                         {     0, PacketState::Generate,             31, SequenceNumberType::Generated},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {     0, PacketState::Generate,             32, SequenceNumberType::Generated},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {     6, PacketState::Forward,              33, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{  5059, PacketState::Skip,               5059, SequenceNumberType::Skip},
                         {    30, PacketState::Forward,              30, SequenceNumberType::Valid},
                         {     0, PacketState::Generate,             31, SequenceNumberType::Generated},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {     0, PacketState::Generate,             32, SequenceNumberType::Generated},
                         {     0, PacketState::Reset,                 0, SequenceNumberType::Skip},
                         {     6, PacketState::Forward,              33, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     0, PacketState::Generate,              6, SequenceNumberType::Generated},
                         {     6, PacketState::Forward,               7, SequenceNumberType::Valid},
                         {     7, PacketState::Forward,               8, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     0, PacketState::Generate,              6, SequenceNumberType::Generated},
                         {     6, PacketState::Forward,               7, SequenceNumberType::Valid},
                         {     7, PacketState::Forward,               8, SequenceNumberType::Valid},
                         {     0, PacketState::Generate,              9, SequenceNumberType::Generated},
                         {     8, PacketState::Forward,              10, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     0, PacketState::Generate,              6, SequenceNumberType::Generated},
                         {     0, PacketState::Generate,              7, SequenceNumberType::Generated},
                         {     6, PacketState::Forward,               8, SequenceNumberType::Valid}}),

    //                     input                        expected_output
    std::vector<Packet>({{     5, PacketState::Forward,               5, SequenceNumberType::Valid},
                         {     0, PacketState::Generate,              6, SequenceNumberType::Generated},
                         {     0, PacketState::Generate,              7, SequenceNumberType::Generated},
                         {     6, PacketState::Forward,               8, SequenceNumberType::Valid},
                         {     0, PacketState::Generate,              9, SequenceNumberType::Generated},
                         {     0, PacketState::Generate,              10, SequenceNumberType::Generated},
                         {     7, PacketState::Forward,               11, SequenceNumberType::Valid}}),


    // Support multiple loops
    getLongQueue(65535 * 2)));
