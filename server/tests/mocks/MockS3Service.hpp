#pragma once

#include <gmock/gmock.h>
#include "services/S3Service.hpp"

using namespace api::v1;

class MockS3Service : public api::v1::S3ServiceInterface {
public:
    MOCK_METHOD(
        std::optional<std::vector<UploadPresignedResult>>,
        generateUploadUrl,
        (int64_t, const std::string &, const std::vector<AttachmentFileInfo> &),
        (override)
    );
    MOCK_METHOD(
        std::optional<std::string>,
        generateDownloadUrl,
        (const std::string &, const std::string &),
        (override)
    );
    MOCK_METHOD(std::string, getExtension, (const std::string &), (override));
    MOCK_METHOD(std::string, getMimeType, (const std::string &), (override));
};