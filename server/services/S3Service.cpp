#include "services/S3Service.hpp"
#include <drogon/utils/Utilities.h>
#include <trantor/utils/Date.h>
#include <trantor/utils/Logger.h>
#include <algorithm>
#include <string>

using namespace api::v1;

S3Service::S3Service(
    std::string access_key,
    std::string secret_key,
    std::string url,
    std::string private_bucket_name
)
    : base_url_(url, true),
      provider_(access_key, secret_key),
      s3_client_(base_url_, &provider_),
      private_bucket_name_(private_bucket_name) {
    base_url_.https = true;
    base_url_.region = "ru-central1";
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
    if (ext == ".pdf") {
        return "application/pdf";
    }
    if (ext == ".mp4") {
        return "video/mp4";
    }

    return "application/octet-stream";
}

std::optional<UploadPresignedResult> S3Service::generateUploadUrl(
    int64_t chat_id,
    const std::string &original_filename,
    bool upload_as_file
) {
    std::string ext = getExtension(original_filename);
    std::string mime_type =
        upload_as_file ? "application/octet-stream" : getMimeType(ext);

    std::string date_prefix =
        trantor::Date::now().toCustomFormattedString("%Y-%m-%d", false);
    std::string uuid = drogon::utils::getUuid(false);

    std::string object_key = "chat_" + std::to_string(chat_id) + "/" +
                             date_prefix + "/" + uuid + ext;

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
        return std::nullopt;
    }
    return UploadPresignedResult{object_key, result.url, mime_type};
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