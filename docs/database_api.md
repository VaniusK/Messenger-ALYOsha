## Users

### Model: User
| Поле | Тип | Описание |
|---|---|---|
| `id` | `int64` | Уникальный ID пользователя |
| `handle` | `string` | Уникальное имя пользователя (@handle) |
| `display_name` | `string` | Отображаемое имя |
| `password_hash` | `string` | Хеш пароля |
| `avatar_path` | `string?` | Путь к аватарке |
| `description` | `string` | Описание профиля |
| `last_synced_message_id` | `int64` | ID последнего синхронизированного сообщения |
| `last_synced_post_id` | `int64` | ID последнего синхронизированного поста |
| `created_at` | `timestamp` | Дата регистрации |


| Метод                                                              | Описание                            | Возвращает     |
| ------------------------------------------------------------------ | ----------------------------------- | -------------- |
| `create(handle, display_name, password_hash)` | Регистрация нового пользователя | `bool`             |
| `getById(user_id)`                                             | Получить пользователя по ID         | `User?`        |
| `getByHandle(handle)`                                          | Поиск по @handle                    | `User?`        |
| `getByIds(vector<id>)`                                        | Batch-загрузка пользователей        | `vector<User>` |
| `search(query, limit)`                                        | Поиск пользователей по имени/handle | `vector<User>` |
| `updateProfile(user_id, display_name?, avatar?, description?)` | Обновить профиль                    | `bool`         |

---

## Chats

### Model: Chat
| Поле | Тип | Описание |
|---|---|---|
| `id` | `int64` | Уникальный ID чата |
| `name` | `string?` | Название (для групп/каналов) |
| `type` | `ChatType` | Direct, Group, Discussion, Saved |
| `avatar_path` | `string?` | Аватарка чата |
| `description` | `string` | Описание |
| `direct_user1_id` | `int64?` | Участник 1 (для Direct) |
| `direct_user2_id` | `int64?` | Участник 2 (для Direct) |
| `created_at` | `timestamp` | Дата создания |

### Model: ChatMember
| Поле | Тип | Описание |
|---|---|---|
| `chat_id` | `int64` | ID чата |
| `user_id` | `int64` | ID пользователя |
| `role` | `ChatRole` | Member, Moderator, Admin, Owner |
| `last_read_message_id` | `int64?` | Последнее прочитанное сообщение |
| `joined_at` | `timestamp` | Дата вступления |


| Метод                                                   | Описание                                   | Возвращает            |
| ------------------------------------------------------- | ------------------------------------------ | --------------------- |
| `getMessageById(message_id)`                                         | Получить сообщение            | `Message?`        |
| `getMessagesByChat(chat_id, before_id?, limit)`                        | Сообщения чата с пагинацией   | `vector<Message>` |
| `sendMessage(chat_id, sender_id, text, reply_to_id?, forwarded_from_id?)` | Отправить сообщение           | `Message`         |
| `editMessage(message_id, new_text)`                                  | Редактировать сообщение       | `bool`            |
| `removeMessage(message_id)`                                         | Удалить сообщение             | `bool`            |
| `getById(chat_id)`                                  | Получить чат по ID                         | `Chat?`               |
| `getByUser(user_id)`                                 | Список чатов пользователя                  | `vector<ChatPreview>` |
| `getOrCreateDirect(user1_id, user2_id)`             | Получить или создать личку (идемпотентно!) | `Chat`                |
| `getDirect(user1_id, user2_id)`             | Получить личку | `Chat?`                |
| `createGroup(name, creator_id, member_ids(vector<id>))`         | Создать групповой чат                      | `Chat`                |
| `getMembers(chat_id)`                               | Список участников чата                     | `vector<ChatMember>`  |
| `addMember(chat_id, user_id, role)`                 | Добавить участника                         | `ChatMember`                |
| `removeMember(chat_id, user_id)`                    | Удалить участника                          | `bool`                |
| `updateMemberRole(chat_id, user_id, new_role)`          | Изменить роль участника                    | `bool`                |
| `updateInfo(chat_id, name?, avatar?, description?)` | Обновить инфо чата                         | `bool`                |
| `markAsRead(chat_id, user_id, last_read_message_id)`                 | Обновить last_read_message_id | `bool`            |
| `createSaved(user_id)` | Создать сохранёнки для пользователя                         | `Chat`                |
| `getSaved(user_id)` | Вернуть сохранёнки пользователя                         | `Chat`                |
### ChatPreview

### Model: ChatPreview
| Поле | Тип | Описание |
|---|---|---|
| `chat_id` | `int64` | ID чата |
| `title` | `string` | Название (имя собеседника или название группы) |
| `avatar_path` | `string?` | Аватарка |
| `last_message` | `Message?` | Последнее сообщение (для превью) |
| `unread_count` | `int` | Кол-во непрочитанных сообщений |
| `type` | `str`(значения из `ChatType`!) | Тип чаты(личный, группа, сохранённые...) |

---

## Messages(Internal, use ChatRepository for API instead)

### Model: Message
| Поле | Тип | Описание |
|---|---|---|
| `id` | `int64` | ID сообщения |
| `chat_id` | `int64` | ID чата |
| `sender_id` | `int64?` | ID отправителя (null если удален) |
| `reply_to_message_id` | `int64?` | Ответ на сообщение |
| `forwarded_from_user_id` | `int64?` | ID автора оригинала (при пересылке) |
| `forwarded_from_user_name`| `string?` | Имя автора оригинала |
| `text` | `string` | Текст сообщения |
| `sent_at` | `timestamp` | Время отправки |
| `edited_at` | `timestamp?`| Время редактирования |


| Метод                                                                | Описание                      | Возвращает        |
| -------------------------------------------------------------------- | ----------------------------- | ----------------- |
| `getById(message_id)`                                         | Получить сообщение            | `Message?`        |
| `getByChat(chat_id, before_id?, limit)`                        | Сообщения чата с пагинацией   | `vector<Message>` |
| `send(chat_id, sender_id, text, reply_to_id?, forwarded_from_id?)` | Отправить сообщение           | `Message`         |
| `edit(message_id, new_text)`                                  | Редактировать сообщение       | `bool`            |
| `remove(message_id)`                                          | Удалить сообщение             | `bool`            |

Пагинация через before_id` + `limit`

---

## Channels(TODO)

### Model: Channel
| Поле | Тип | Описание |
|---|---|---|
| `id` | `int64` | ID канала |
| `name` | `string` | Название канала |
| `handle` | `string` | @handle канала |
| `is_private` | `bool` | Приватный канал |
| `discussion_chat_id` | `int64?` | ID чата для комментариев |
| `avatar_path` | `string?` | Аватарка |
| `description` | `string` | Описание |
| `created_at` | `timestamp` | Дата создания |

### Model: ChannelMember
| Поле | Тип | Описание |
|---|---|---|
| `channel_id` | `int64` | ID канала |
| `user_id` | `int64` | ID подписчика |
| `role` | `ChannelRole` | Member, Admin, Owner |
| `joined_at` | `timestamp` | Дата подписки |


| Метод                                                         | Описание                                 | Возвращает              |
| ------------------------------------------------------------- | ---------------------------------------- | ----------------------- |
| `getById(channel_id)`                                  | Получить канал по ID                     | `Channel?`              |
| `getByHandle(handle)`                                  | Получить канал по @handle                | `Channel?`              |
| `getByUser(user_id)`                                    | Каналы, на которые подписан пользователь | `vector<Channel>`       |
| `create(handle, name, owner_id, is_private?)`          | Создать канал                            | `Channel`               |
| `getMembers(channel_id, offset?, limit?)`              | Подписчики канала                        | `vector<ChannelMember>` |
| `getMemberCount(channel_id)`                           | Количество подписчиков                   | `int64`                 |
| `join(channel_id, user_id)`                            | Подписаться на канал                     | `bool`                  |
| `leave(channel_id, user_id)`                           | Отписаться от канала                     | `bool`                  |
| `updateInfo(channel_id, name?, avatar?, description?)` | Обновить инфо канала                     | `bool`                  |

---

## Posts(TODO)

### Model: Post
| Поле | Тип | Описание |
|---|---|---|
| `id` | `int64` | ID поста |
| `channel_id` | `int64` | ID канала |
| `discussion_message_id` | `int64?` | ID сообщения в discussion-чате |
| `text` | `string` | Текст поста |
| `posted_at` | `timestamp` | Время публикации |
| `edited_at` | `timestamp?`| Время редактирования |


| Метод                                                   | Описание                    | Возвращает     |
| ------------------------------------------------------- | --------------------------- | -------------- |
| `getById(post_id)`                                  | Получить пост               | `Post?`        |
| `getChannelPosts(channel_id, before_id?, limit)`        | Посты канала                | `vector<Post>` |
| `create(channel_id, text, enable_comments?)` | Создать пост в канале       | `Post`         |
| `edit(post_id, new_text)`                           | Редактировать пост          | `bool`         |
| `delete(post_id)`                                   | Удалить пост                | `bool`         |

### Комментарии

Комментарии реализованы как сообщения в discussion-чате(как в телеграме).
При создании канала автоматически создаётся его discussion-чат.
При создании поста с комментариями:
1. Создаётся сообщение(копия поста) в этом чате.
2. `discussion_message_id` в посте указывает на это сообщение

---

## Reactions(TODO)

### Model: Reaction
| Поле | Тип | Описание |
|---|---|---|
| `id` | `int64` | ID реакции |
| `message_id` | `int64?` | ID сообщения (XOR post_id) |
| `post_id` | `int64?` | ID поста (XOR message_id) |
| `user_id` | `int64` | Кто поставил |
| `emoji` | `string` | Смайлик |
| `reacted_at` | `timestamp` | Время реакции |


| Метод                                            | Описание                       | Возвращает         |
| ------------------------------------------------ | ------------------------------ | ------------------ |
| `addMessageReaction(message_id, user_id, emoji)` | Поставить реакцию на сообщение | `Reaction`         |
| `addPostReaction(post_id, user_id, emoji)`       | Поставить реакцию на пост      | `Reaction`         |
| `remove(reaction_id)`                    | Убрать реакцию                 | `bool`             |
| `getByMessage(message_id)`                | Реакции на сообщение           | `vector<Reaction>` |
| `getByPost(post_id)`                      | Реакции на пост                | `vector<Reaction>` |

---

## Attachments

### Model: Attachment
| Поле | Тип | Описание |
|---|---|---|
| `id` | `int64` | ID вложения |
| `message_id` | `int64` | ID сообщения |
| `file_name` | `string` | Название файла |
| `file_type` | `string` | MIME-тип |
| `file_size_bytes` | `int64` | Размер в байтах |
| `uploaded_at` | `timestamp` | Время загрузки |


| Метод                                                               | Описание                      | Возвращает           |
| ------------------------------------------------------------------- | ----------------------------- | -------------------- |
| `add(message_id, file_name, file_type, file_size_bytes)` | Добавить вложение к сообщению | `Attachment`         |
| `getByMessage(message_id)`                                 | Вложения сообщения            | `vector<Attachment>` |
| `getByMessages(vector<message_id>)`                                 | Вложения сообщений            | `vector<vector<Attachment>>` |
| `remove(id)`                                   | Удалить вложение              | `bool`               |
