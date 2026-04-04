![alt text](https://github.com/VaniusK/Messenger-ALYOsha/raw/dev/docs/alesha_sticker_1.png "Стикер 1")

![Version](https://img.shields.io/github/v/tag/VaniusK/Messenger-ALYOsha?label=version)
![Build](https://img.shields.io/github/actions/workflow/status/VaniusK/Messenger-ALYOsha/ci.yml)
![Tests](https://img.shields.io/badge/tests-passing-brightgreen)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue)
![Drogon](https://img.shields.io/badge/framework-Drogon-orange)
![License](https://img.shields.io/badge/license-GPLv3-green)
[![Join the telegram channel at https://t.me/+V9sf31BJYg5kNzAy](https://img.shields.io/badge/Telegram-2CA5E0?style=flat&logo=telegram&logoColor=white)](https://t.me/+V9sf31BJYg5kNzAy)


# Алёша (ALYOsha) — Современный мессенджер на C++

Проект разрабатывается студентами ВШЭ (СПб) как учебная работа по курсу C++. Наша цель — создать производительный<!--ха -->, масштабируемый<!--ха-ха --> и безопасный<!--ахахахахахах --> мессенджер с использованием современных архитектурных паттернов.

![alt text](https://github.com/VaniusK/Messenger-ALYOsha/raw/dev/docs/alesha_sticker_6.png "Стикер 6")

### Текущие возможности (MVP):
*   **Личные чаты:** Поддержка диалогов в реальном времени.
*   **Сохранённые сообщения:** Отдельный чат для записи
*   **Кроссплатформенность клиента:** Нативные приложения под Windows и Linux

---

## Технологический стек

*   **Язык:** C++20 (корутины).
*   **Сервер:** [Drogon Framework](https://drogon.org/) — один из самых быстрых асинхронных веб-фреймворков.
*   **База данных:** PostgreSQL
*   **ORM:** Drogon ORM для интеграции с БД.
*   **Клиент:** Qt / QML для создания кроссплатформенного и плавного интерфейса.
*   **DevOps:** Docker & Docker Compose для изоляции зависимостей и CI/CD через GitHub Actions.
*   **Тестирование:** [Google Test](https://github.com/google/googletest)

---

## Архитектура системы

Стандартная многослойная архитектура для обеспечения тестируемости и гибкости разработки:

1.  **Repository Layer:** Прямая работа с базой данных (CRUD-операции через ORM).
2.  **Service Layer:** Бизнес-логика приложения. Здесь принимаются решения, обрабатываются данные и управляются состояния.
3.  **Controller Layer:** Обработка входящих HTTP/WebSocket запросов от клиента, валидация входных данных.
4.  **Client (QML):** Представление данных и взаимодействие с пользователем.

---

![alt text](https://github.com/VaniusK/Messenger-ALYOsha/raw/dev/docs/alesha_sticker_9.png "Стикер 9")

## Быстрый старт

### Предварительные требования
*   Установленный **Docker** и **Docker Compose**.
*   Компилятор с поддержкой **C++20** (GCC 11+ или Clang 13+).
*   **Make**-утилита.

### Запуск сервера и БД
Для автоматической сборки Docker-образов и поднятия инфраструктуры выполните:
```bash
make run_server
```

### Сборка клиента
Для компиляции клиентского приложения:
```bash
make build_client
```

---

## Структура проекта

*   `server/` — Исходный код бэкенда (Drogon, Repository, Services).
*   `client/` — Код клиентского приложения на QML/Qt.
*   `common/` — Общие структуры данных и утилиты, используемые и сервером, и клиентом.
*   `docs/` — Проектная документация и диаграммы.

---
## Участники

<a href="https://github.com/VaniusK/Messenger-ALYOsha/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=VaniusK/Messenger-ALYOsha" />
</a>
