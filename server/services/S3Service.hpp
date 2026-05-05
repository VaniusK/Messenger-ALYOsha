#pragma once

#include <miniocpp/client.h>
#include <optional>
#include <string>
#include <vector>

namespace api {
namespace v1 {

struct UploadPresignedResult {
    std::string attachment_key;
    std::string upload_url;
    std::string content_type;
    std::string file_name;
    int64_t file_size_bytes = 0;
};

struct AttachmentFileInfo {
    std::string file_name;
    std::string ext;
    std::string mime_type;
    int64_t file_size_bytes = 0;
};

class S3ServiceInterface {
public:
    virtual ~S3ServiceInterface() = default;
    virtual std::optional<std::vector<UploadPresignedResult>> generateUploadUrl(
        int64_t chat_id,
        const std::string &message_type,
        const std::vector<AttachmentFileInfo> &files_info
    ) = 0;
    virtual std::optional<std::string> generateDownloadUrl(
        const std::string &attachment_key,
        const std::string &original_filename
    ) = 0;

    virtual std::string getExtension(const std::string &filename) = 0;
    virtual std::string getMimeType(const std::string &ext) = 0;
};

class S3Service : public S3ServiceInterface {
public:
    S3Service(
        std::string access_key,
        std::string secret_key,
        std::string url,
        std::string private_bucket_name,
        bool should_use_https
    );
    std::optional<std::vector<UploadPresignedResult>> generateUploadUrl(
        int64_t chat_id,
        const std::string &message_type,
        const std::vector<AttachmentFileInfo> &files_info
    ) override;
    std::optional<std::string> generateDownloadUrl(
        const std::string &attachment_key,
        const std::string &original_filename
    ) override;

    std::string getExtension(const std::string &filename) override;
    std::string getMimeType(const std::string &ext) override;

private:
    minio::s3::BaseUrl base_url_;
    minio::s3::Client s3_client_;
    minio::creds::StaticProvider provider_;
    std::string private_bucket_name_;

    std::string getFolderName(const std::string &message_type);
    minio::s3::BaseUrl formBaseUrl(
        const std::string &url,
        bool should_use_https,
        const std::string &region
    );
};
}  // namespace v1
}  // namespace api
