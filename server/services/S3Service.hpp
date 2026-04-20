#pragma once

#include <miniocpp/client.h>
#include <optional>
#include <string>
#include <vector>
#include "repositories/ChatRepository.hpp"

namespace api {
namespace v1 {

struct UploadPresignedResult {
    std::string attachment_key;
    std::string upload_url;
    std::string content_type;
    std::string file_name;
    int64_t file_size_bytes;
};

struct AttachmentFileInfo {
    std::string file_name;
    std::string ext;
    std::string mime_type;
    int64_t file_size_bytes;
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
    std::optional<std::vector<UploadPresignedResult>> generateUploadUrl(
        int64_t chat_id,
        const std::string &message_type,
        const std::vector<AttachmentFileInfo> &files_info
    );
    std::optional<std::string> generateDownloadUrl(
        const std::string &attachment_key,
        const std::string &original_filename
    );

    void setChatRepo(
        std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    ) {
        this->chat_repo_ = chat_repo;
    }

    std::string getExtension(const std::string &filename);
    std::string getMimeType(const std::string &ext);

private:
    minio::s3::BaseUrl base_url_;
    minio::s3::Client s3_client_;
    minio::creds::StaticProvider provider_;
    std::string private_bucket_name_;

    std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
        chat_repo_;

    std::string getFolderName(const std::string &message_type);
    minio::s3::BaseUrl formBaseUrl(
        const std::string &url,
        bool should_use_https,
        const std::string &region
    );
};
}  // namespace v1
}  // namespace api
