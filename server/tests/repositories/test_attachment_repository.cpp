#include <drogon/orm/Result.h>
#include "../fixtures/AttachmentTestFixture.hpp"

using AttachmentRepository = messenger::repositories::AttachmentRepository;
using Attachment = drogon_model::messenger_db::Attachments;

using namespace drogon;
using namespace drogon::orm;

TEST_F(AttachmentTestFixture, TestCreate) {
    /* When valid data is provided,
    create() should return an attachment
    and its fields should be correct */
    auto attachment = sync_wait(repo_.create(
        dummy_message_1.getValueOfId(), "cat_image", "image/png", 1337, "oooaaa"
    ));
    EXPECT_EQ(attachment.getValueOfMessageId(), dummy_message_1.getValueOfId());
    EXPECT_EQ(attachment.getValueOfFileName(), "cat_image");
    EXPECT_EQ(attachment.getValueOfFileType(), "image/png");
    EXPECT_EQ(attachment.getValueOfFileSizeBytes(), 1337);
    EXPECT_EQ(attachment.getValueOfS3ObjectKey(), "oooaaa");
}

TEST_F(AttachmentTestFixture, TestCreateNoMessage) {
    /* When message does not exists,
    create() should throw an error*/
    EXPECT_THROW(
        sync_wait(repo_.create(
            dummy_message_1.getValueOfId() - 1, "cat_image", "image/png", 1337,
            "oooaaa"
        )),
        std::runtime_error
    );
}

TEST_F(AttachmentTestFixture, TestGetByMessage) {
    /* When getByMessage is called,
    it should return all of message's attachments*/
    auto attachment1 = sync_wait(repo_.create(
        dummy_message_1.getValueOfId(), "cat_image", "image/png", 1337, "oooaaa"
    ));
    auto attachment2 = sync_wait(repo_.create(
        dummy_message_1.getValueOfId(), "cat_image", "image/png", 1337, "ooobbb"
    ));
    auto attachment3 = sync_wait(repo_.create(
        dummy_message_2.getValueOfId(), "cat_image", "image/png", 1337, "oooccc"
    ));
    auto attachments =
        sync_wait(repo_.getByMessage(dummy_message_1.getValueOfId()));
    EXPECT_EQ(attachments.size(), 2);
    EXPECT_EQ(
        std::count_if(
            attachments.begin(), attachments.end(),
            [attachment1](const Attachment &a) {
                return a.getValueOfId() == attachment1.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            attachments.begin(), attachments.end(),
            [attachment2](const Attachment &a) {
                return a.getValueOfId() == attachment2.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            attachments.begin(), attachments.end(),
            [attachment3](const Attachment &a) {
                return a.getValueOfId() == attachment3.getValueOfId();
            }
        ),
        0
    );
}

TEST_F(AttachmentTestFixture, TestGetByMessages) {
    /* When getByMessages is called,
    it should return all of messages' attachments*/
    auto attachment1 = sync_wait(repo_.create(
        dummy_message_1.getValueOfId(), "cat_image", "image/png", 1337, "oooaaa"
    ));
    auto attachment2 = sync_wait(repo_.create(
        dummy_message_1.getValueOfId(), "cat_image", "image/png", 1337, "ooobbb"
    ));
    auto attachment3 = sync_wait(repo_.create(
        dummy_message_2.getValueOfId(), "cat_image", "image/png", 1337, "oooccc"
    ));
    auto attachments = sync_wait(repo_.getByMessages(
        {dummy_message_1.getValueOfId(), dummy_message_2.getValueOfId()}
    ));
    EXPECT_EQ(attachments.size(), 3);
    EXPECT_EQ(
        std::count_if(
            attachments.begin(), attachments.end(),
            [attachment1](const Attachment &a) {
                return a.getValueOfId() == attachment1.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            attachments.begin(), attachments.end(),
            [attachment2](const Attachment &a) {
                return a.getValueOfId() == attachment2.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            attachments.begin(), attachments.end(),
            [attachment3](const Attachment &a) {
                return a.getValueOfId() == attachment3.getValueOfId();
            }
        ),
        1
    );
}