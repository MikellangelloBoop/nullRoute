# Null Route — проверка переноса

Дата: 19 сентября 2026 года.

Основной проект находится в **`Z:\nullRoute`**. Старого каталога `outputs\NullRoute` больше нет.
Файлы `outputs\NullRouteArchitecture` относятся к прежней архитектурной документации.

Открытие: `Z:\nullRoute\OpenEditor.cmd` или `Z:\nullRoute\NullRoute.uproject`.
Игровой запуск: `Z:\nullRoute\Play.cmd`.
Управление и структура: `Z:\nullRoute\README.md`.

| Проверка | Результат | Подтверждение в проекте |
|---|---|---|
| NullRouteEditor / Win64 / Development, UE 5.8.1 | Succeeded | Saved/Build.log |
| Генерация карты, материалов, Geometry Collection | 0 ошибок, 0 предупреждений commandlet | Saved/GenerateArena.log |
| C++ Automation Tests | 3 успешных, 0 неуспешных | Saved/Tests/index.json |
| FastAPI pytest после переноса | 16 passed | backend/tests/test_api.py; выполнены 19.09.2026 |
| Игровой smoke-тест | PASSED, без Chaos ensure в итоговом запуске | Saved/Smoke.log |
| Сервер и отдельный клиент через loopback | Join succeeded; клиентский smoke PASSED | Saved/NetworkServer.log, Saved/NetworkClient.log |
| Iris | Сервер создал ReplicationSystem; NetDriver использует Iris | Saved/NetworkServer.log |
| Серверная частота | В журнале задан max tick rate 64 | Saved/NetworkServer.log; не является нагрузочным замером |

Smoke-тест проверил игрока и GameState, завершение печати, запрет цикла графа, каскад B.U.P. и вызов разрушения конструкции. Сетевой тест проверил подключение одного клиента и получение персонажа/GameState. Полная проверка способностей между несколькими сетевыми игроками и нагрузка 5v5 ещё не проведены.

Тестовый сервер остановлен. В журнале Editor-сервера остаются предупреждения Iris о поздней загрузке модулей; упаковка самостоятельного сервера и проверка production-профиля необходимы отдельно. MSVC 14.51 успешно собрал проект, но движок помечает эту версию как более новую, чем предпочтительная 14.50.

Состояние проекта: **игровой C++ прототип**. Это не завершённый релиз всей спецификации. В README перечислены оставшиеся части: финальный контент, полноценный редактор графа, масштабирование Mass, аккаунт-интеграция с игровым loadout, n8n workflow, конкретный TTS backend, matchmaking, античит и нагрузочная проверка.
