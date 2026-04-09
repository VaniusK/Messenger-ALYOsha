# Server API

## Base URL
`http://api.*hostname*/v1` \
`https://...` Если используется HTTPS

---

## WebSocket
`ws://api.*hostname*/ws/chat` \
`wss://...` Если используется HTTPS
### handleNewConnection - Открытие подключения
При открытии подключения нужно передавать токен в таком же формате, как для защищенных http-запросов. После этого сможем получать новые уведомления о событиях от сервера.
### handleConnectionClosed - Закрытие подключения
Закрывает подключение
### notifyUser - Пересылает сообщение о событии получателю
Сообщения приходят как обычные строки, но оформленные как Json, поэтому их нужно конвертировать на стороне клиента.
  - **Типы событий(поле 'event_type')**:
    - 'NEW_MESSAGE': новое сообщение(Поля сообщения от вебсокета: ['data']['message']: message)
    - 'MESSAGE_READ': сообщения прочитаны(Поля сообщения от вебсокета: soon)
   
## Защищенные методы
Помечены в документации как `(protected)`
- Для доступа к защищенным методам необходимо передавать заголовок:
`Authorization: Bearer <access_token>`

## Общие коды возврата
- `500`: Неизвестная ошибка на сервере(ошибка бд)
- `401`: Невалидный токен(у защищенных методов)
- `400`: Невалидный JSON(отсутствует или не хватает полей)

## HTTPS guide
Если вам интересно захостить сервер самим, то либо напишите dast3rr, либо обратитесь к нейросети.(возможно по запросам зрителей будет полный гайд) На текущий момент в файле nginx.conf находятся настройки для HTTPS соединения, которое при деплое требует создания сертификатов. А чтобы использовать обычный HTTP, нужно немного поменять nginx.conf. Однако для локального запуска сервера достаточно будет создать в директории server директорию certbot/conf/live/localhost и туда создать локальный сертификат через openssl командой "openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout ./certbot/conf/live/localhost/privkey.pem -out ./certbot/conf/live/localhost/fullchain.pem -subj "/C=RU/L=Local/O=Dev/CN=localhost" 

## Auth

### POST /auth/register
Регистрация нового пользователя.

- **Аргументы (JSON)**:
  - `handle`: string (уникальный никнейм)
  - `display_name`: string (отображаемое имя)
  - `password`: string (пароль)
- **Коды возврата**:
  - `201`: Успешно
  - `409`: Пользователь с таким handle уже существует

### POST /auth/login
Авторизация пользователя.

- **Аргументы (JSON)**:
  - `handle`: string
  - `password`: string
- **Возвращает**:
  - `token`: string (JWT)
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

### GET /chats/user/{user_id} (protected)
Получить список всех чатов текущего пользователя (с превью).

- **Аргументы**: Нет
- **Возвращает**:
  - `chats`: vector<ChatPreview>
- **Коды возврата**:
  - `200`: Успешно
  - `403`: '403': Ошибка доступа(пользователь пытается получить сообщение не свои чаты)

### POST /chats/direct (protected)
Получить существующий или создать новый личный чат.

- **Аргументы (JSON)**:
  - `target_user_id`: int64
- **Возвращает**:
  - `chat`: Chat
- **Коды возврата**:
  - `200`: Чат уже был, возвращен
  - `201`: Чат создан
 
### GET chats/messages/{message_id} (protected)
Получить конкретное сообщение.

- **Аргументы**: Нет
- **Возвращает**:
  - `message`: Message
- **Коды возврата**:
  - `200`: Успешно
  - `404`: Сообщение не найдено
  - `403`: Ошибка доступа(пользователь пытается получить сообщение не из своих чатов)

### GET /chats/{chat_id}/messages (protected)
Получить историю сообщений чата (пагинация).

- **Аргументы (Query Params)**:
  - `before_id`: int64? (ID сообщения, до которого загружать историю)
  - `limit`: int (по умолчанию 20)
- **Возвращает**:
  - `messages`: vector<Message>
- **Коды возврата**:
  - `200`: Успешно
  - `403`: Ошибка доступа(пользователь пытается получить сообщения не из своих чатов)

### POST /chats/{chat_id}/messages (protected)
Отправить сообщение в чат.

- **Аргументы (JSON)**:
  - `text`: string
  - `reply_to_id`: int64?
  - `forward_info`: object?
- **Возвращает**:
  - `message`: Message
- **Коды возврата**:
  - `201`: Успешно
  - `403`: Ошибка доступа(пользователь пытается отправить сообщение не в свой чат)

### POST /chats/{chat_id}/read (protected)
Пометить сообщения как прочитанные. Все сообщения раньше того, айди которого указано в аргументе, становятся прочитанными

- **Аргументы (JSON)**:
  - `last_read_message_id`: int64 - последнее прочитанное сообщение
- **Возвращает**:
  - Пустой ответ или текущий статус
- **Коды возврата**:
  - `200`: Успешно
  - `403`: Ошибка доступа(пользователь пытается прочитать сообщения не из своих чатов)

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
