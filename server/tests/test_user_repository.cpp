#include <drogon/orm/Result.h>
#include <algorithm>
#include <latch>
#include "fixtures/UserTestFixture.hpp"

using UserRepository = messenger::repositories::UserRepository;
using User = drogon_model::messenger_db::Users;

using namespace drogon;
using namespace drogon::orm;

TEST_F(UserTestFixture, TestCreate) {
    /* When valid data is provided,
    create() should return true
    and the user should be retrievable via getAll() */
    bool res =
        sync_wait(repo_.create("konobeitsev3", "Ivan konobeitsev", "hash_idk"));
    EXPECT_TRUE(res);
    auto users = sync_wait(repo_.getAll());
    EXPECT_EQ(users.size(), 1);
    User user = users[0];
    EXPECT_EQ(user.getValueOfHandle(), "konobeitsev3");
}

TEST_F(UserTestFixture, TestCreateSameHandle) {
    /* When user with given handle already exists,
    create() with the same handle should return false
    and no new user should be created */
    bool res =
        sync_wait(repo_.create("konobeitsev3", "Ivan konobeitsev", "hash_idk"));
    EXPECT_TRUE(res);
    bool res2 =
        sync_wait(repo_.create("konobeitsev3", "Ivan konobeitsev", "hash_idk"));
    EXPECT_FALSE(res2);
    auto users = sync_wait(repo_.getAll());
    EXPECT_EQ(users.size(), 1);
}

TEST_F(UserTestFixture, TestGetByHandle) {
    /* When user with given handle exists,
    getByHandle() should return it */
    bool res =
        sync_wait(repo_.create("konobeitsev3", "Ivan konobeitsev", "hash_idk"));
    EXPECT_TRUE(res);
    auto result = sync_wait(repo_.getByHandle("konobeitsev3"));
    EXPECT_TRUE(result.has_value());
    auto user = result.value();
    EXPECT_EQ(user.getValueOfHandle(), "konobeitsev3");
}

TEST_F(UserTestFixture, TestGetByHandleFail) {
    /* When user with given handle doesn't exist,
    getByHandle() should return nullopt */
    bool res =
        sync_wait(repo_.create("konobeitsev3", "Ivan konobeitsev", "hash_idk"));
    EXPECT_TRUE(res);
    auto result = sync_wait(repo_.getByHandle("some_user"));
    EXPECT_FALSE(result.has_value());
}

TEST_F(UserTestFixture, TestGetById) {
    /* When user with given id exists,
    getById() should return it */
    bool res =
        sync_wait(repo_.create("konobeitsev3", "Ivan konobeitsev", "hash_idk"));
    EXPECT_TRUE(res);
    auto result = sync_wait(repo_.getByHandle("konobeitsev3"));
    EXPECT_TRUE(result.has_value());
    auto user = result.value();
    auto result2 = sync_wait(repo_.getById(user.getValueOfId()));
    EXPECT_TRUE(result2.has_value());
    auto user2 = result2.value();
    EXPECT_EQ(user.getValueOfId(), user2.getValueOfId());
}

TEST_F(UserTestFixture, TestGetByIdFail) {
    /* When user with given id doesn't exist,
    getById() should return nullopt */
    bool res =
        sync_wait(repo_.create("konobeitsev3", "Ivan konobeitsev", "hash_idk"));
    EXPECT_TRUE(res);
    auto result = sync_wait(repo_.getById(100));
    EXPECT_FALSE(result.has_value());
}

TEST_F(UserTestFixture, TestGetAll) {
    /* When getAll called,
    it should return all users*/
    bool res1 = sync_wait(repo_.create("user1", "user1", "hash_idk"));
    bool res2 = sync_wait(repo_.create("user2", "user1", "hash_idk"));
    bool res3 = sync_wait(repo_.create("user3", "user1", "hash_idk"));
    EXPECT_TRUE(res1);
    EXPECT_TRUE(res2);
    EXPECT_TRUE(res3);
    auto users = sync_wait(repo_.getAll());
    EXPECT_EQ(users.size(), 3);
}

TEST_F(UserTestFixture, TestConcurrentCreateDifferentHandles) {
    /* When multiple threads attemp to create users
    with the different handle concurrently,
    they all should succeed
    and all users should be created*/

    // otherwise the test does nothing
    EXPECT_TRUE(std::thread::hardware_concurrency() > 1);

    std::latch start_gate(1);
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    for (int i = 0; i < 100; i++) {
        threads.push_back(std::thread([&, i]() {
            start_gate.wait();
            bool res = sync_wait(repo_.create(
                "konobeitsev" + std::to_string(i), "Ivan konobeitsev",
                "hash_idk"
            ));
            if (res) {
                success_count++;
            }
        }));
    }
    start_gate.count_down();
    for (auto &t : threads) {
        t.join();
    }
    EXPECT_EQ(success_count, 100);
    auto users = sync_wait(repo_.getAll());
    EXPECT_EQ(users.size(), 100);
}

TEST_F(UserTestFixture, TestByIds) {
    /* When getByIds called,
    it should return all users with given ids
    and ignore invalid ids*/
    bool res1 = sync_wait(repo_.create("user1", "user1", "hash_idk"));
    bool res2 = sync_wait(repo_.create("user2", "user1", "hash_idk"));
    bool res3 = sync_wait(repo_.create("user3", "user1", "hash_idk"));
    EXPECT_TRUE(res1);
    EXPECT_TRUE(res2);
    EXPECT_TRUE(res3);
    auto user1 = sync_wait(repo_.getByHandle("user1")).value();
    auto user2 = sync_wait(repo_.getByHandle("user2")).value();
    auto user3 = sync_wait(repo_.getByHandle("user3")).value();
    auto users = sync_wait(repo_.getByIds(std::vector<int64_t>{
        user1.getValueOfId(), user2.getValueOfId(), user3.getValueOfId(), 9999
    }));
    EXPECT_EQ(users.size(), 3);
    EXPECT_EQ(
        std::count_if(
            users.begin(), users.end(),
            [&user1](const User &u) {
                return user1.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            users.begin(), users.end(),
            [&user2](const User &u) {
                return user2.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            users.begin(), users.end(),
            [&user3](const User &u) {
                return user3.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
}

TEST_F(UserTestFixture, TestSearchByHandle) {
    /* When search called,
    it should return all users with matching handle*/
    bool res1 = sync_wait(repo_.create("user1", "name", "hash_idk"));
    bool res2 = sync_wait(repo_.create("user2", "name", "hash_idk"));
    bool res3 = sync_wait(repo_.create("ohoho", "name", "hash_idk"));
    EXPECT_TRUE(res1);
    EXPECT_TRUE(res2);
    EXPECT_TRUE(res3);
    auto user1 = sync_wait(repo_.getByHandle("user1")).value();
    auto user2 = sync_wait(repo_.getByHandle("user2")).value();
    auto user3 = sync_wait(repo_.getByHandle("ohoho")).value();
    auto users = sync_wait(repo_.search("user", 100));
    EXPECT_EQ(users.size(), 2);
    EXPECT_EQ(
        std::count_if(
            users.begin(), users.end(),
            [&user1](const User &u) {
                return user1.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            users.begin(), users.end(),
            [&user2](const User &u) {
                return user2.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
}

TEST_F(UserTestFixture, TestSearchByMixed) {
    /* When search called,
    it should return all users with matching handle or name*/
    bool res1 = sync_wait(repo_.create("user1", "name", "hash_idk"));
    bool res2 = sync_wait(repo_.create("user2", "name", "hash_idk"));
    bool res3 = sync_wait(repo_.create("ohoho", "user3", "hash_idk"));
    EXPECT_TRUE(res1);
    EXPECT_TRUE(res2);
    EXPECT_TRUE(res3);
    auto user1 = sync_wait(repo_.getByHandle("user1")).value();
    auto user2 = sync_wait(repo_.getByHandle("user2")).value();
    auto user3 = sync_wait(repo_.getByHandle("ohoho")).value();
    auto users = sync_wait(repo_.search("user", 100));
    EXPECT_EQ(users.size(), 3);
    EXPECT_EQ(
        std::count_if(
            users.begin(), users.end(),
            [&user1](const User &u) {
                return user1.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            users.begin(), users.end(),
            [&user2](const User &u) {
                return user2.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            users.begin(), users.end(),
            [&user3](const User &u) {
                return user3.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
}

TEST_F(UserTestFixture, TestSearchLimit) {
    /* When search called,
    it should return all users with matching handle or name
    and result size should not exceed the limit*/
    bool res1 = sync_wait(repo_.create("user1", "name", "hash_idk"));
    bool res2 = sync_wait(repo_.create("user2", "name", "hash_idk"));
    bool res3 = sync_wait(repo_.create("user3", "name", "hash_idk"));
    EXPECT_TRUE(res1);
    EXPECT_TRUE(res2);
    EXPECT_TRUE(res3);
    auto user1 = sync_wait(repo_.getByHandle("user1")).value();
    auto user2 = sync_wait(repo_.getByHandle("user2")).value();
    auto user3 = sync_wait(repo_.getByHandle("user3")).value();
    auto users = sync_wait(repo_.search("user", 2));
    EXPECT_EQ(users.size(), 2);
}

TEST_F(UserTestFixture, TestUpdateProfile) {
    /* When updateProfile called
    and user exists,
    it should update every non-nullptr field
    and return true*/
    bool res = sync_wait(repo_.create("user1", "name", "hash_idk"));
    auto user = sync_wait(repo_.getByHandle("user1")).value();
    auto update_res = sync_wait(repo_.updateProfile(
        user.getValueOfId(), "new_name", "new_avatar", "new_description"
    ));
    EXPECT_TRUE(update_res);
    auto updated_user = sync_wait(repo_.getByHandle("user1")).value();
    EXPECT_EQ(user.getValueOfHandle(), updated_user.getValueOfHandle());
    EXPECT_EQ(user.getValueOfId(), updated_user.getValueOfId());
    EXPECT_EQ(updated_user.getValueOfDisplayName(), "new_name");
    EXPECT_EQ(updated_user.getValueOfAvatarPath(), "new_avatar");
    EXPECT_EQ(updated_user.getValueOfDescription(), "new_description");
}

TEST_F(UserTestFixture, TestUpdateProfileNullopt) {
    /* When updateProfile called with all nullptrs
    and user exists,
    it should not change anything
    and return true*/
    bool res = sync_wait(repo_.create("user1", "name", "hash_idk"));
    auto user = sync_wait(repo_.getByHandle("user1")).value();
    auto update_res = sync_wait(repo_.updateProfile(
        user.getValueOfId(), std::nullopt, std::nullopt, std::nullopt
    ));
    EXPECT_TRUE(update_res);
    auto updated_user = sync_wait(repo_.getByHandle("user1")).value();
    EXPECT_EQ(user.getValueOfHandle(), updated_user.getValueOfHandle());
    EXPECT_EQ(user.getValueOfId(), updated_user.getValueOfId());
    EXPECT_EQ(
        updated_user.getValueOfDisplayName(), user.getValueOfDisplayName()
    );
    EXPECT_EQ(updated_user.getValueOfAvatarPath(), user.getValueOfAvatarPath());
    EXPECT_EQ(
        updated_user.getValueOfDescription(), user.getValueOfDescription()
    );
}

TEST_F(UserTestFixture, TestUpdateProfileFail) {
    /* When updateProfile called
    and user does not exists,
    it should not change anything
    and return false*/
    bool res = sync_wait(repo_.create("user1", "name", "hash_idk"));
    auto user = sync_wait(repo_.getByHandle("user1")).value();
    auto update_res = sync_wait(repo_.updateProfile(
        user.getValueOfId() + 10, "new_name", "new_avatar", "new_description"
    ));
    EXPECT_FALSE(update_res);
    auto updated_user_res = sync_wait(repo_.getById(user.getValueOfId() + 10));
    EXPECT_FALSE(updated_user_res);
}
