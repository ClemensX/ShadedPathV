#pragma once

#include <gtest/gtest.h>
#include <filesystem>

// Forward declaration
class ShadedPathEngine;

// Base test fixture that changes working directory per test
class WorkingDirectoryTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

private:
    std::filesystem::path original_path;
    std::filesystem::path test_directory;
};

// Shared test helper functions
void minimalEngineInitialization(ShadedPathEngine* engine, int maxMeshes = -1);