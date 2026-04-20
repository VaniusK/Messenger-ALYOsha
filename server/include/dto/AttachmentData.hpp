#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace messenger::dto {

struct AttachmentData {
    std::string file_name;
    std::string file_type;
    int64_t file_size_bytes;
    std::string s3_object_key;
};

}  // namespace messenger::dto
