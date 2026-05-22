#pragma once
#include <string>

namespace messenger::models {

namespace ChatRole {
inline const std::string Member = "member";
inline const std::string Moderator = "moderator";
inline const std::string Admin = "admin";
inline const std::string Owner = "owner";
}  // namespace ChatRole

namespace ChannelRole {
inline const std::string Member = "member";
inline const std::string Admin = "admin";
inline const std::string Owner = "owner";
}  // namespace ChannelRole

namespace ChatType {
inline const std::string Direct = "direct";
inline const std::string Group = "group";
inline const std::string Channel = "discussion";
inline const std::string Saved = "saved";
}  // namespace ChatType

namespace MessageType {
inline const std::string Text = "text";
inline const std::string Media = "media";
inline const std::string Voice = "voice";
inline const std::string Round = "round";
inline const std::string Sticker = "sticker";
}  // namespace MessageType

}  // namespace messenger::models
