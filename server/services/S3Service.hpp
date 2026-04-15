#pragma once

#include <miniocpp/client.h>
#include <optional>
#include <string>

namespace api {
namespace v1 {

struct UploadPresignedResult {
    std::string attachment_key;
    std::string upload_url;
    std::string content_type;
};

class S3Service {
public:
    S3Service(
        std::string access_key,
        std::string secret_key,
        std::string url,
        std::string private_bucket_name,
        bool should_use_https
    );
    std::optional<UploadPresignedResult> generateUploadUrl(
        int64_t chat_id,
        const std::string &original_filename,
        bool upload_as_file
    );
    std::optional<std::string> generateDownloadUrl(
        const std::string &attachment_key,
        const std::string &original_filename
    );

private:
    minio::s3::BaseUrl base_url_;
    minio::s3::Client s3_client_;
    minio::creds::StaticProvider provider_;
    std::string private_bucket_name_;

    std::string getExtension(const std::string &filename);
    std::string getMimeType(const std::string &ext);
    minio::s3::BaseUrl formBaseUrl(
        const std::string &url,
        bool should_use_https,
        const std::string &region
    );
};
}  // namespace v1
}  // namespace api
