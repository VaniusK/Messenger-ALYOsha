#include "services/S3Service.hpp"
#include <drogon/utils/Utilities.h>
#include <miniocpp/args.h>
#include <miniocpp/response.h>
#include <trantor/utils/Date.h>
#include <trantor/utils/Logger.h>
#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include "utils/Enum.hpp"

using namespace api::v1;

namespace {
template <typename TResponse>
std::string
formatS3Failure(const std::string &operation, const TResponse &resp) {
    return "S3: Failed to " + operation + ": " + resp.Error().String();
}
}  // namespace

minio::s3::BaseUrl S3Service::formBaseUrl(
    const std::string &url,
    bool should_use_https,
    const std::string &region
) {
    minio::s3::BaseUrl base_url(url, should_use_https);
    base_url.region = region;
    return base_url;
}

#include <iostream>

S3Service::S3Service(
    std::string access_key,
    std::string secret_key,
    std::string url,
    std::string private_bucket_name,
    bool should_use_https
)
    : base_url_(formBaseUrl(url, should_use_https, "ru-central1")),
      provider_(access_key, secret_key),
      s3_client_(base_url_, &provider_),
      private_bucket_name_(private_bucket_name) {
    std::cout << "S3Service: connecting to " << url
              << " (https=" << should_use_https << ")"
              << ", bucket=" << private_bucket_name_ << std::endl;

    constexpr int kMaxRetries = 10;
    constexpr int kRetryDelaySec = 2;

    for (int attempt = 1; attempt <= kMaxRetries; ++attempt) {
        minio::s3::BucketExistsArgs exists_args;
        exists_args.bucket = private_bucket_name_;
        minio::s3::BucketExistsResponse resp =
            s3_client_.BucketExists(exists_args);

        if (resp) {
            if (!resp.exist) {
                minio::s3::MakeBucketArgs make_args;
                make_args.bucket = private_bucket_name_;
                minio::s3::MakeBucketResponse make_resp =
                    s3_client_.MakeBucket(make_args);
                if (!make_resp) {
                    throw std::runtime_error(
                        formatS3Failure("create bucket", make_resp)
                    );
                }
            }
            std::cout << "S3Service: bucket '" << private_bucket_name_
                      << "' is ready (attempt " << attempt << ")" << std::endl;
            return;
        }

        std::string err_str = resp.Error().String();
        std::cerr << "S3Service: BucketExists failed (attempt " << attempt
                  << "/" << kMaxRetries << "): " << err_str << std::endl;

        if (err_str.find("ResourceNotFound") != std::string::npos ||
            err_str.find("NoSuchBucket") != std::string::npos) {
            std::cout << "S3Service: Assuming bucket doesn't exist. Attempting "
                         "MakeBucket..."
                      << std::endl;
            minio::s3::MakeBucketArgs make_args;
            make_args.bucket = private_bucket_name_;
            minio::s3::MakeBucketResponse make_resp =
                s3_client_.MakeBucket(make_args);

            if (make_resp) {
                std::cout << "S3Service: bucket created successfully (attempt "
                          << attempt << ")" << std::endl;
                return;
            } else if (make_resp.Error().String().find("BucketAlreadyOwnedByYou") != std::string::npos ||
                       make_resp.Error().String().find("BucketAlreadyExists") != std::string::npos) {
                std::cout << "S3Service: bucket already exists." << std::endl;
                return;
            } else {
                std::cerr << "S3Service: MakeBucket also failed: "
                          << make_resp.Error().String() << std::endl;
            }
        }

        if (attempt < kMaxRetries) {
            std::this_thread::sleep_for(std::chrono::seconds(kRetryDelaySec));
        }
    }

    throw std::runtime_error(
        "S3Service: failed to connect after " + std::to_string(kMaxRetries) +
        " attempts"
    );
}

std::string S3Service::getExtension(const std::string &filename) {
    std::size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }

    std::string ext = filename.substr(dot_pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

std::string S3Service::getMimeType(const std::string &ext) {
    if (ext == ".jpg" || ext == ".jpeg") {
        return "image/jpeg";
    }
    if (ext == ".png") {
        return "image/png";
    }
    if (ext == ".webp") {
        return "image/webp";
    }
    if (ext == ".gif") {
        return "image/gif";
    }
    if (ext == ".bmp") {
        return "image/bmp";
    }

    if (ext == ".mp4") {
        return "video/mp4";
    }
    if (ext == ".webm") {
        return "video/webm";
    }
    if (ext == ".mov") {
        return "video/quicktime";
    }
    if (ext == ".avi") {
        return "video/x-msvideo";
    }

    return "application/octet-stream";
}

std::string S3Service::getFolderName(const std::string &message_type) {
    if (message_type == messenger::models::MessageType::Text) {
        return "attachments";
    }
    if (message_type == messenger::models::MessageType::Voice) {
        return "voice-messages";
    }
    if (message_type == messenger::models::MessageType::Round) {
        return "round-messages";
    }
    return "attachments";
}

drogon::Task<std::optional<UploadPresignedResult>> S3Service::generateUploadUrl(
    int64_t chat_id,
    const std::string &original_filename,
    bool upload_as_file,
    int64_t message_id
) {
    std::string ext = getExtension(original_filename);
    std::string mime_type =
        upload_as_file ? "application/octet-stream" : getMimeType(ext);

    std::string date_prefix =
        trantor::Date::now().toCustomFormattedString("%Y-%m-%d", false);
    std::string uuid = drogon::utils::getUuid(false);

    std::optional<Message> message =
        co_await chat_repo_->getMessageById(message_id);
    if (!message.has_value()) {
        LOG_ERROR << "Failed to generate PUT presigned URL: couldn't get "
                     "message for attachment";
        co_return std::nullopt;
    }
    std::string folder_name;
    folder_name = getFolderName(message->getValueOfType());

    std::string object_key = "chat_" + std::to_string(chat_id) + "/" +
                             folder_name + "/" + date_prefix + "/" + uuid + ext;

    minio::s3::GetPresignedObjectUrlArgs args;
    args.bucket = private_bucket_name_;
    args.object = object_key;
    args.expiry_seconds = 600;
    args.method = minio::http::Method::kPut;

    args.extra_headers.Add("Content-Type", mime_type);
    auto result = s3_client_.GetPresignedObjectUrl(args);
    if (result.url.empty()) {
        LOG_ERROR << "Failed to generate PUT presigned URL: "
                  << result.status_code << " " << result.message;
        co_return std::nullopt;
    }
    co_return UploadPresignedResult{object_key, result.url, mime_type};
}

std::optional<std::string> S3Service::generateDownloadUrl(
    const std::string &object_key,
    const std::string &original_filename = ""
) {
    minio::s3::GetPresignedObjectUrlArgs args;
    args.bucket = private_bucket_name_;
    args.object = object_key;
    args.method = minio::http::Method::kGet;
    args.expiry_seconds = 3600;

    if (!original_filename.empty()) {
        std::string encoded_name = drogon::utils::urlEncode(original_filename);
        std::string disposition =
            "attachment; filename=\"" + encoded_name + "\"";
        args.extra_query_params.Add(
            "response-content-disposition", disposition
        );
    }

    auto result = s3_client_.GetPresignedObjectUrl(args);
    if (result.url.empty()) {
        LOG_ERROR << "Failed to generate GET presigned URL: "
                  << result.status_code << " " << result.message;
        return std::nullopt;
    }
    return result.url;
}