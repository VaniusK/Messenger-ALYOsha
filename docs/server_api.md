# Server API

## Base URL
`/api/v1`

---

## Auth
Для доступа к защищенным методам необходимо передавать заголовок:
`Authorization: Bearer <access_token>`

### POST /auth/register
Регистрация нового пользователя.

- **Аргументы (JSON)**:
  - `handle`: string (уникальный никнейм)
  - `display_name`: string (отображаемое имя)
  - `password`: string (пароль)
- **Коды возврата**:
  - `201`: Успешно
  - `409`: Пользователь с таким handle уже существует
  - `400`: Некорректные данные

### POST /auth/login
Авторизация пользователя.

- **Аргументы (JSON)**:
  - `handle`: string
  - `password`: string
- **Возвращает**:
  - `access_token`: string (JWT, содержит user_id и claims)
- **Коды возврата**:
  - `200`: Успешно
  - `401`: Неверный пароль или пользователь не найден

---

## Users

### GET /users/{user_id} 
Получить информацию о пользователе по ID.

- **Аргументы**:
  - `user_id`: int64 (path param)
- **Возвращает**:
  - `user`: User
- **Коды возврата**:
  - `200`: Успешно
  - `404`: Пользователь не найден

### GET /users/handle/{handle}
Поиск пользователя по handle.

- **Аргументы**:
  - `handle`: string (path param)
- **Возвращает**:
  - `user`: User
- **Коды возврата**:
  - `200`: Успешно
  - `404`: Пользователь не найден

### POST /users/batch
Получение информации о нескольких пользователях (batch-загрузка).

- **Аргументы (JSON)**:
  - `user_ids`: vector<int64>
- **Возвращает**:
  - `users`: vector<User>
- **Коды возврата**:
  - `200`: Успешно

### GET /users/search
Поиск пользователей по префиксу имени или handle.

- **Аргументы (Query Params)**:
  - `query`: string (строка запроса)
  - `limit`: int (максимальное кол-во результатов)
- **Возвращает**:
  - `results`: vector<User>
- **Коды возврата**:
  - `200`: Успешно

<!-- ### PUT /users/me (IN FUTURE)
Обновление профиля текущего пользователя.

- **Аргументы (JSON)**:
  - `display_name`: string?
  - `avatar`: string? (path)
  - `description`: string?
- **Возвращает**:
  - `success`: bool
- **Коды возврата**:
  - `200`: Успешно
  - `401`: Не авторизован -->

---

## Chats

<!-- ### GET /chats/{chat_id} (IN FUTURE)
Получить информацию о чате.

- **Аргументы**:
  - `chat_id`: int64 (path param)
- **Возвращает**:
  - `chat`: Chat
- **Коды возврата**:
  - `200`: Успешно
  - `403`: Нет доступа
  - `404`: Чат не найден -->

### GET /chats/user/{user_id}
Получить список всех чатов текущего пользователя (с превью).

- **Аргументы**: Нет
- **Возвращает**:
  - `chats`: vector<ChatPreview>
- **Коды возврата**:
  - `200`: Успешно

### POST /chats/direct 
Получить существующий или создать новый личный чат.

- **Аргументы (JSON)**:
  - `target_user_id`: int64
- **Возвращает**:
  - `chat`: Chat
- **Коды возврата**:
  - `200`: Чат уже был, возвращен
  - `201`: Чат создан

<!-- ### POST /chats/group (IN FUTURE)
Создать групповой чат.

- **Аргументы (JSON)**:
  - `name`: string
  - `members`: vector<int64> (список ID участников)
- **Возвращает**:
  - `chat`: Chat
- **Коды возврата**:
  - `201`: Успешно

### PUT /chats/{chat_id} (IN FUTURE)
Обновить информацию о чате.

- **Аргументы (JSON)**:
  - `name`: string?
  - `avatar`: string?
  - `description`: string?
- **Возвращает**:
  - `success`: bool
- **Коды возврата**:
  - `200`: Успешно
  - `403`: Нет прав (не админ) -->

<!-- ### GET /chats/{chat_id}/members (IN FUTURE)
Получить список участников чата.

- **Аргументы**:
  - `chat_id`: int64 (path param)
- **Возвращает**:
  - `members`: vector<ChatMember>
- **Коды возврата**:
  - `200`: Успешно

### POST /chats/{chat_id}/members (IN FUTURE)
Добавить участника в чат.

- **Аргументы (JSON)**:
  - `user_id`: int64
  - `role`: string (USER, ADMIN)
- **Возвращает**:
  - `success`: bool
- **Коды возврата**:
  - `200`: Успешно
  - `403`: Нет прав

### DELETE /chats/{chat_id}/members/{user_id} (IN FUTURE)
Удалить участника из чата.

- **Аргументы**:
  - `chat_id`: int64 (path param)
  - `user_id`: int64 (path param)
- **Возвращает**:
  - `success`: bool
- **Коды возврата**:
  - `200`: Успешно
  - `403`: Нет прав -->

---

## Messages

### GET /messages/{message_id}
Получить конкретное сообщение.

- **Аргументы**:
  - `message_id`: int64 (path param)
- **Возвращает**:
  - `message`: Message
- **Коды возврата**:
  - `200`: Успешно
  - `404`: Сообщение не найдено

### GET /chats/{chat_id}/messages
Получить историю сообщений чата (пагинация).

- **Аргументы (Query Params)**:
  - `before_id`: int64? (ID сообщения, до которого загружать историю)
  - `limit`: int (по умолчанию 20)
- **Возвращает**:
  - `messages`: vector<Message>
- **Коды возврата**:
  - `200`: Успешно

### POST /chats/{chat_id}/messages
Отправить сообщение в чат.

- **Аргументы (JSON)**:
  - `text`: string
  - `reply_to_id`: int64?
  - `forward_info`: object?
- **Возвращает**:
  - `message`: Message
- **Коды возврата**:
  - `201`: Успешно

### POST /chats/{chat_id}/read
Пометить сообщения как прочитанные.

- **Аргументы (JSON)**:
  - `to_read_messages`: vector<int64>
- **Возвращает**:
  - Пустой ответ или текущий статус
- **Коды возврата**:
  - `200`: Успешно

<!-- ### PUT /messages/{message_id} (IN FUTURE)
Редактировать сообщение.

- **Аргументы (JSON)**:
  - `new_text`: string
- **Возвращает**:
  - `success`: bool
- **Коды возврата**:
  - `200`: Успешно
  - `403`: Нет прав (не автор)

### DELETE /messages/{message_id} (IN FUTURE)
Удалить сообщение.

- **Аргументы**:
  - `message_id`: int64 (path param)
- **Возвращает**:
  - `success`: bool
- **Коды возврата**:
  - `200`: Успешно
  - `403`: Нет прав -->



---

<!-- ## Channels (IN FUTURE)

### GET /channels/{channel_id} (IN FUTURE) 
Получить информацию о канале.

- **Аргументы**:
  - `channel_id`: int64 (path param)
- **Возвращает**:
  - `channel`: Channel
- **Коды возврата**:
  - `200`: Успешно
  - `404`: Канал не найден

### POST /channels (IN FUTURE)
Создать новый канал.

- **Аргументы (JSON)**:
  - `handle`: string
  - `name`: string
  - `is_private`: bool?
- **Возвращает**:
  - `channel`: Channel
- **Коды возврата**:
  - `201`: Успешно
  - `409`: Канал с таким handle уже существует

### POST /channels/{channel_id}/join (IN FUTURE)
Подписаться на канал.

- **Аргументы**:
  - `channel_id`: int64 (path param)
- **Возвращает**:
  - `success`: bool
- **Коды возврата**:
  - `200`: Успешно

### DELETE /channels/{channel_id}/leave (IN FUTURE)
Отписаться от канала.

- **Аргументы**:
  - `channel_id`: int64 (path param)
- **Возвращает**:
  - `success`: bool
- **Коды возврата**:
  - `200`: Успешно

---

## Posts (Channels) (IN FUTURE)

### GET /channels/{channel_id}/posts (IN FUTURE)
Получить ленту постов канала.

- **Аргументы (Query Params)**:
  - `before_id`: int64?
  - `limit`: int
- **Возвращает**:
  - `posts`: vector<Post>
- **Коды возврата**:
  - `200`: Успешно

### POST /channels/{channel_id}/posts (IN FUTURE)
Создать пост (только для админов/владельцев).

- **Аргументы (JSON)**:
  - `text`: string
  - `enable_comments`: bool?
- **Возвращает**:
  - `post`: Post
- **Коды возврата**:
  - `201`: Успешно
  - `403`: Нет прав

---

## Reactions (IN FUTURE)

### POST /messages/{message_id}/reactions (IN FUTURE)
Поставить реакцию на сообщение.

- **Аргументы (JSON)**:
  - `emoji`: string
- **Возвращает**:
  - `reaction`: Reaction
- **Коды возврата**:
  - `200`: Успешно

### DELETE /reactions/{reaction_id} (IN FUTURE)
Удалить реакцию.

- **Аргументы**:
  - `reaction_id`: int64 (path param)
- **Возвращает**:
  - `success`: bool
- **Коды возврата**:
  - `200`: Успешно

---

## Attachments

### POST /messages/{message_id}/attachments (IN FUTURE)
Загрузить вложение к сообщению.

- **Аргументы (Multipart/Form-data)**:
  - `file`: binary
- **Возвращает**:
  - `attachment`: Attachment
- **Коды возврата**:
  - `201`: Успешно -->
