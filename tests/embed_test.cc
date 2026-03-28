#include <gtest/gtest.h>

#include "runtime/embed.h"

#include <cstring>

TEST(Embed, FooterLayout) {
    EXPECT_EQ(Embed::FOOTER_SIZE, 16u);
    EXPECT_EQ(std::memcmp(Embed::MAGIC, "QIANJSBC", 8), 0);
}

TEST(Embed, ReadWriteRoundTrip) {
    const fs::path dir = fs::temp_directory_path() / "qianjs_gtest";
    fs::create_directories(dir);
    const fs::path path = dir / "roundtrip.bin";
    const std::vector<uint8_t> data = {0x01, 0x02, 0xab};

    ASSERT_TRUE(Embed::writeBinaryFile(path, data));
    const std::vector<uint8_t> back = Embed::readBinaryFile(path);
    EXPECT_EQ(back, data);

    fs::remove(path);
    fs::remove(dir);
}
