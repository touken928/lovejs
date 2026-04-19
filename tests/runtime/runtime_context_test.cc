#include <gtest/gtest.h>

#include "runtime/runtime_context.h"

#include <string>

TEST(RuntimeContext, CaptureEnvironmentSmoke) {
    std::vector<std::pair<std::string, std::string>> env = qianjs::captureEnvironment();

    for (const auto& kv : env) {
        EXPECT_FALSE(kv.first.empty()) << "environment key must not be empty";
    }
}

TEST(RuntimeContext, RuntimeContextDefaults) {
    qianjs::RuntimeContext ctx;
    EXPECT_TRUE(ctx.argv.empty());
    EXPECT_TRUE(ctx.env.empty());
    EXPECT_EQ(ctx.exit_code, 0);
}
