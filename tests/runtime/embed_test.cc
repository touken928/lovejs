#include <gtest/gtest.h>

#include "runtime/embed.h"

#include <cstring>
#include <fstream>
#include <vector>

namespace {

namespace fs = std::filesystem;

/** Same layout as `Embed::readEmbeddedBytecode` but for an arbitrary file (test-only). */
std::vector<uint8_t> read_embedded_at(const fs::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        return {};

    const auto fileSize = static_cast<size_t>(f.tellg());
    if (fileSize < Embed::FOOTER_SIZE)
        return {};

    f.seekg(-static_cast<std::streamoff>(Embed::FOOTER_SIZE), std::ios::end);
    Embed::Footer footer{};
    f.read(reinterpret_cast<char*>(&footer), sizeof(footer));

    if (std::memcmp(footer.magic, Embed::MAGIC, 8) != 0)
        return {};

    if (footer.bytecodeSize == 0 || footer.bytecodeSize > fileSize - Embed::FOOTER_SIZE)
        return {};

    f.seekg(-static_cast<std::streamoff>(Embed::FOOTER_SIZE + footer.bytecodeSize), std::ios::end);
    std::vector<uint8_t> bytecode(footer.bytecodeSize);
    f.read(reinterpret_cast<char*>(bytecode.data()), footer.bytecodeSize);
    return bytecode;
}

void write_embedded_file(const fs::path& path, const std::vector<uint8_t>& base,
    const std::vector<uint8_t>& bytecode) {
    std::vector<uint8_t> out;
    out.insert(out.end(), base.begin(), base.end());
    out.insert(out.end(), bytecode.begin(), bytecode.end());
    Embed::Footer footer{};
    std::memcpy(footer.magic, Embed::MAGIC, 8);
    footer.bytecodeSize = static_cast<uint32_t>(bytecode.size());
    footer.reserved = 0;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&footer);
    out.insert(out.end(), p, p + sizeof(footer));
    ASSERT_TRUE(Embed::writeBinaryFile(path, out));
}

} // namespace

TEST(EmbedRuntime, FooterLayout) {
    EXPECT_EQ(Embed::FOOTER_SIZE, 16u);
    EXPECT_EQ(std::memcmp(Embed::MAGIC, "QIANJSBC", 8), 0);
}

TEST(EmbedRuntime, ReadWriteBinaryRoundTrip) {
    const fs::path dir = fs::temp_directory_path() / "qianjs_test_embed";
    fs::create_directories(dir);
    const fs::path path = dir / "roundtrip.bin";
    const std::vector<uint8_t> data = {0x01, 0x02, 0xab};

    ASSERT_TRUE(Embed::writeBinaryFile(path, data));
    const std::vector<uint8_t> back = Embed::readBinaryFile(path);
    EXPECT_EQ(back, data);

    fs::remove(path);
    fs::remove(dir);
}

TEST(EmbedRuntime, ReadBinaryFileMissingIsEmpty) {
    const fs::path p = fs::temp_directory_path() / "qianjs_test_no_such_file.bin";
    const std::vector<uint8_t> v = Embed::readBinaryFile(p);
    EXPECT_TRUE(v.empty());
}

TEST(EmbedRuntime, WriteReadEmptyFile) {
    const fs::path dir = fs::temp_directory_path() / "qianjs_test_embed_empty";
    fs::create_directories(dir);
    const fs::path path = dir / "empty.bin";
    ASSERT_TRUE(Embed::writeBinaryFile(path, {}));
    EXPECT_TRUE(Embed::readBinaryFile(path).empty());
    fs::remove(path);
    fs::remove(dir);
}

TEST(EmbedRuntime, EmbeddedTailRoundTrip) {
    const fs::path dir = fs::temp_directory_path() / "qianjs_test_embed_tail";
    fs::create_directories(dir);
    const fs::path path = dir / "fake_exe.bin";
    const std::vector<uint8_t> base = {'E', 'L', 'F'};
    const std::vector<uint8_t> bytecode = {0xca, 0xfe, 0xba, 0xbe};

    write_embedded_file(path, base, bytecode);
    const std::vector<uint8_t> read_back = read_embedded_at(path);
    EXPECT_EQ(read_back, bytecode);

    fs::remove(path);
    fs::remove(dir);
}

TEST(EmbedRuntime, BadMagicReturnsEmpty) {
    const fs::path dir = fs::temp_directory_path() / "qianjs_test_embed_badmagic";
    fs::create_directories(dir);
    const fs::path path = dir / "bad.bin";
    std::vector<uint8_t> raw(8 + 4 + 4, 0);
    raw[raw.size() - 1] = 1;
    ASSERT_TRUE(Embed::writeBinaryFile(path, raw));
    EXPECT_TRUE(read_embedded_at(path).empty());
    fs::remove(path);
    fs::remove(dir);
}

TEST(EmbedRuntime, BytecodeSizeOutOfRangeReturnsEmpty) {
    const fs::path dir = fs::temp_directory_path() / "qianjs_test_embed_range";
    fs::create_directories(dir);
    const fs::path path = dir / "range.bin";
    std::vector<uint8_t> out = {'x'};
    Embed::Footer footer{};
    std::memcpy(footer.magic, Embed::MAGIC, 8);
    footer.bytecodeSize = 999999;
    footer.reserved = 0;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&footer);
    out.insert(out.end(), p, p + sizeof(footer));
    ASSERT_TRUE(Embed::writeBinaryFile(path, out));
    EXPECT_TRUE(read_embedded_at(path).empty());
    fs::remove(path);
    fs::remove(dir);
}
