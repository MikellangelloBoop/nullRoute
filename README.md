# Null Route

Главный архитектор / Lead Game Designer: **Беличенко Александр Андреевич**.

### Штурмовик и фракции — 20.09.2026

Штурмовик использует настоящее тело и кисти MetaHuman из установленного пресета Epic, с атлетическими пропорциями, тремя LOD и перенесёнными игровыми анимациями. Поверх тела собрана стилизованная модульная броня по концептам: 15 подвижных деталей и тяжёлый BR–74. Chronos Security, Cybernetic Police и Rebel Syndicate различаются геометрией, материалами, подсветкой и шевронами из пользовательских референсов. Комплект выбирается сервером по карте и стороне; смена сторон обновляет внешний вид и HUD. В аркологии доступны тренировка за атаку (Rebel) и оборону (Chronos), затем выбор класса через Esc.

Текущая модель и воспроизведение: [ArtSource/MetaHuman/README.md](ArtSource/MetaHuman/README.md), команда `Scripts/Project.ps1 -Action MetaHumanAssets`. Локальный экспорт тела не является завершённой UE Optimized сборкой: облачная загрузка лицевого рига и текстур Epic пока прерывается на Amazon S3. Лицо закрыто шлемом. Настройка фракций и GLB предыдущей процедурной модели: [ArtSource/Breacher/README.md](ArtSource/Breacher/README.md). Проверки: `-Action BreacherTest`, `Scripts/TestBreacherNetwork.ps1`. Внешний вид BR–74 обновлён без изменения характеристик стрельбы.

Играбельная C++ версия тактического шутера на **Unreal Engine 5.8.1**. Основной каталог проекта: `Z:\nullRoute`.
Это разрабатываемый игровой прототип, а не готовый Release Candidate: названный так исходный документ задаёт целевое состояние.

## Запуск

`Play.cmd` запускает готовую игру без Unreal Editor. Текущая версия выбрана в `Releases\CurrentBuild.txt`; сборка MetaHuman установлена в `Releases\MetaHuman\Windows` рядом с ранее запущенной игрой. Обычный `Scripts/Package.ps1` собирает `Releases\Windows` и переключает запуск на неё после успешной упаковки. Для разработки и пересборки требуются UE 5.8.1 и Visual Studio с C++ toolchain. На этой машине используется `Z:\games\UE_5.8`.
Для другой установки передайте `-EngineRoot` в `Scripts\Project.ps1` или задайте переменную `UE_ROOT`.

1. `OpenEditor.cmd` — открыть проект в Unreal Editor.
2. `Play.cmd` — запустить готовую игру с меню; при отсутствии packaged-билда используется Editor в режиме игры.
3. Из PowerShell: `Scripts\Project.ps1 -Action Build` — собрать C++.
4. `Scripts\Project.ps1 -Action GenerateArena` — заново создать тестовую карту, Geometry Collection и материалы. **Заменяет сгенерированные ассеты**; не запускайте поверх вручную отредактированной арены без копии.

Сеть, двумя отдельными командами:

```powershell
.\Scripts\Project.ps1 -Action Server
.\Scripts\Project.ps1 -Action Join -Address 127.0.0.1
```

Серверный запуск через `UnrealEditor-Cmd -server -nullrhi` использует установленный Editor без рендеринга.
Для самостоятельного shipping Dedicated Server нужен поддерживающий Server target исходный/installed build движка; наличие `NullRouteServer.Target.cs` само по себе не создаёт серверный бинарник.
`-Competitive` отключает тренировочный старт: раунд ожидает 10 игроков. В тренировке можно изучать механику одному.

## Управление

| Клавиша | Действие |
|---|---|
| WASD / мышь / Space | Передвижение, обзор, прыжок |
| Левый Alt | Тихий шаг (удерживать) |
| Q / E | Наклоны влево / вправо (удерживать) |
| Shift / Ctrl / I | Спринт / приседание (удерживать) / осмотр оружия |
| ЛКМ / ПКМ / R | Огонь / прицеливание / перезарядка |
| Esc | Меню, выбор класса, подключение, качество графики |
| T | Визуальная консоль графа: две левые кнопки соединяют узлы, правая меняет операцию |
| Tab во время подготовки | Инженер → Разведчик → Штурмовик |
| Z | Способность текущего класса |
| F | Захват нейтрального терминала, подбор / эвакуация Элары |
| X | Выбрать первый узел, затем второй и соединить |
| G | Переключить операцию своего узла |

Инженер смотрит на свободный пол не дальше 6 м. Печать: полимер 1.5 с, переключение 0.65 с, армирование 0.75 с; стоимость 80 токенов и 10 энергии команды.
Разведчик сканирует вражеский узел до 15 м; завершение перехвата через 3 с требует вновь подтвердить прямую видимость.
Штурмовик устанавливает заряд на разрушаемую панель до 2.5 м; задержка 1.2 с, стоимость 150 токенов.
Чёрный нейтральный узел можно захватить бесплатно. Сканирование захваченного чёрного узла противником запускает каскад по связанному графу.
В бою атакующая сторона забирает диск Элары и доставляет к точке эвакуации. Подготовка 20 с, бой 120 с; победа в матче при 7 выигранных раундах.

## Архитектура

```text
NullRoute.uproject
Config/                  Iris, collision channels, 64 Hz, навигация и ввод
Source/
  NRCore/                типы, IAwakening, ограниченные алгоритмы графа, тесты
  NRNetworking/          спящие акторы, Iris grid + бюджетная проверка видимости
  NRPhysics/             Chaos L1, поля разрушения, навигация, клиентский мусор
  NRGameplay/            GAS, персонаж, баллистика, ноды, раунды, Mass, HUD, TTS
  NREditor/              commandlet генерации арены и материалов
  NullRoute.Target.cs
  NullRouteEditor.Target.cs
  NullRouteServer.Target.cs
Content/
  Maps/NR_Arcology.umap   генерируемая двухуровневая тестовая арена
  Materials/             полимер, SDF-линии, visor stencil и окружение
  Structures/GC_Panel     кластерная разрушаемая панель
backend/                 FastAPI, Telegram Mini App, БД и API-тесты
Scripts/                 сборка, запуск, автоматические проверки
Saved/                   журналы, результаты тестов, снимки
```

Сетевая логика выполняется на сервере. RPC поступают через принадлежащего игроку персонажа; чужой терминал не пытается принимать клиентский Server RPC напрямую.
`DORM_Initial` предназначен для размещённых на карте акторов; созданные во время матча акторы сначала должны реплицироваться и затем переходят в `DORM_DormantAll`.
Перед изменением состояния актор пробуждается. Разрушенная нода сохраняет реплицируемое состояние, чтобы подключившийся позже клиент получил правильный результат.
Фильтр Iris использует расстояние и бюджетные LOS-проверки. Ближняя зона остаётся релевантной для стрельбы через стены и физических взаимодействий.
В установленном UE 5.8.1 нет свойства TargetRules `bUseIris`: используется `SetupIrisSupport(Target)` и конфигурация Iris.

## Визуальная версия

Индустриальная аркология оформлена бетонными и графитовыми материалами, белыми панелями и световыми акцентами команд. Есть модели стоек, терминалов, дронов, укрытий и целей. Оружие имеет прицеливание, отдачу, визуальную перезарядку, вспышки, летящие трассеры, искры и следы попаданий. Локальные эффекты ограничены по количеству; Dedicated Server не создаёт их.

Меню позволяет выбрать класс во время подготовки, начать новый одиночный забег, подключиться по адресу и настроить качество. В одиночной игре меню ставит матч на паузу.

### Обновление вида от первого лица и графики — 20.09.2026

Переработан русский Slate HUD и масштабируемое меню. Добавлены качественные пресеты LOW / MED / HIGH / ULTRA. Модель оружия использует геометрию и текстуры Epic template с новыми материалами; руки имеют закрытые рукава, отдельную анимацию перезарядки, плавное прицеливание, отдачу и покачивание. Для мирового персонажа настроены восьминаправленная ходьба/бег, прыжок, перезарядка, шлем, разгрузка и рюкзак инженера. Источники моделей перечислены в `ArtSource/PROVENANCE.md`.

Включены Lumen, Virtual Shadow Maps и TSR. Перенастроены материалы и свет арены. Дроны визуально интерполируются на клиенте и не блокируют капсулу игрока, сохраняя попадания пуль. У объектов окружения раздельная простая коллизия для движения и точная коллизия для баллистики.

Проверка столкновений: `Scripts/Project.ps1 -Action CollisionTest`. Она проверяет пол, лестницу, укрытие и проём в разрушенной Chaos-панели. Качество картинки и частота кадров зависят от выбранного пресета и оборудования; массовый нагрузочный тест ещё требуется.

### Детали, покрытия, движение и боезапас — 20.09.2026

В меню доступны четыре бесплатных косметических покрытия: «Карбон», «Арктика», «Аварийный» и «Фантом». Выбор сохраняется локально и реплицируется другим игрокам. Покрытие не меняет характеристики оружия. I запускает осмотр оружия.

Shift — спринт вперёд; Ctrl — приседание при удержании. Прицеливание, перезарядка и перенос Элары ограничивают скорость. Приседание уменьшает капсулу; низкий потолок блокирует вставание. Спринт использует сохранённые движения CharacterMovement, включая воспроизведение при сетевой коррекции. Ускорение, торможение, движения оружия и реакция приземления перенастроены.

| Класс | В магазине | В резерве |
|---|---:|---:|
| Инженер | 30 | 90 |
| Разведчик | 12 | 36 |
| Штурмовик | 30 | 60 |

Перезарядка переносит только недостающие патроны из резерва. Остаток в магазине сохраняется. При пустом резерве перезарядка недоступна. Пополнение — при новом раунде или выборе класса во время подготовки; смена класса в бою запрещена. HUD показывает **магазин / резерв**. Разведчик стреляет одним выстрелом на нажатие.

Добавлены мелкие детали винтовки, кабельные лотки, вентиляция, трубы и разметка арены. Звуки оружия и перезарядки переработаны; есть восемь вариантов шагов, приземление и пустой спуск. Удалённые звуки пространственные, с приглушением за препятствиями; косметический аудиопул ограничивает количество голосов.

Проверки: `Scripts/Project.ps1 -Action DetailTest`, `Scripts/TestNetwork.ps1 -Movement`. Для повторной генерации декоративных ассетов: commandlet `NRDetailAssets`, затем `Scripts/DetailArena.py` через Unreal Python. Полная команда GenerateArena заменяет карту — сначала сохраняйте ручные изменения.

## Backend

```powershell
cd backend
.\Run.ps1 -Install
```

Локальный интерфейс: `http://localhost:8000`. Каталог доступен без входа; операции аккаунта требуют настоящего подписанного Telegram `initData`.
Настройки перечислены в `backend/.env.example`. Файл `.env` автоматически не загружается: передавайте переменные окружения процессу или через систему развёртывания.
Для Telegram необходимо HTTPS-размещение и настройка Mini App у собственного бота. Токены в исходники не включаются.
Реализованы HMAC-проверка с ограничением возраста, защита от повтора авторизации, httpOnly cookie, CSRF/Origin, транзакционные покупки, постоянные награды ARG, подписанные результаты матчей и OIDC-привязка с PKCE/nonce.
`/internal/arg/complete` — подписанный интерфейс подтверждения для n8n. Сам workflow n8n и его развёртывание пока не входят в готовую интеграцию.
Каталог сайд-грейдов хранится в БД аккаунта; применение разблокированного снаряжения к игровому loadout ещё не связано с UE.

## Проверки

```powershell
.\Scripts\Project.ps1 -Action Tests
.\Scripts\Project.ps1 -Action Smoke
cd backend
python -m pytest -q
```

Automation: циклы и каскады графа, точный порог уборки мусора, детерминированный seed B.U.P.
Smoke: загрузка игрока и GameState, печать, валидация связей, каскад и вызов Chaos-разрушения. Это функциональная проверка, не измерение производительности.
`NetServerMaxTickRate=64` задаёт целевую частоту, но не гарантирует её под нагрузкой. Для профилирования запускайте сервер с `-netprofile`; необходим отдельный сценарий массовых взрывов и 10 сетевых игроков.

## Границы текущей реализации

- Создана одна индустриальная арена с текстурами, модульной геометрией, серверными стойками, укрытиями и мезонином. Персонаж, анимации и базовое оружие используют ассеты Epic из шаблонов UE; это не финальные уникальные персонажи проекта. Происхождение ассетов описано в `ArtSource/PROVENANCE.md`.
- Баллистика с гравитацией и проникновением ограничена бюджетом; материал и толщина учитываются упрощённо. Есть прицеливание, визуальная отдача и разброс, но нет server rewind/lag compensation.
- Сервер реплицирует Chaos L1. Клиентский мусор ограничен 192 простыми фрагментами; полноценные художественные Geometry Collections уровней 2–3 ещё нужны.
- Mass управляет 24 дронами. Представление реплицируется одним массивом; для тысяч агентов нужны разделение по пространственным секторам, LOD и замеры. Генерация NavMesh ограничена invokers; поведение прыжков по smart links требует развития.
- Граф редактируется в native Slate-консоли на T либо клавишами X/G. Есть выбор операций и соединение узлов с серверной проверкой циклов. Сохранение схем между матчами и расширенный редактор пинов пока отсутствуют.
- SDF-линии, AR stencil и native Slate HUD созданы. Клиентские классы пока находятся в runtime-модуле с проверками Dedicated Server; отдельный клиентский модуль — следующий шаг оптимизации зависимостей.
- `INRElaraTTS` — интерфейс для локального синтезатора и обработчик PCM. Модели F5-TTS/XTTSv2, веса, voice assets и их runtime backend не поставляются.
- Нет matchmaking, античита, рейтинга, боевого аккаунт-сервиса, полноценного OAuth-провайдера, production migrations, системы релизных обновлений и завершённой UE6-миграции.

Статус конкретных запусков смотрите в `Saved` и отчёте проверки. Успешная сборка не означает готовность всей спецификации к релизу.


## Arsenal / Field Operations update

- 1: primary weapon; 2: P-12 semiautomatic pistol (12 + 36); 3: Route tactical knife. LMB fires/strikes, R reloads, I inspects. Server-approved swaps preserve ammunition and cancel reloads.
- Pistol damage: 34 before armor; interval: 0.24 s. Knife: 65 before armor, 155 cm sweep, 0.65 s interval, blocked by the first solid obstacle. Equipping takes 0.35 s.
- Supply stations: north-west and south-east corners; F grants 35 HP, one primary reserve magazine and 12 pistol reserve rounds, capped to loadout capacity. One use per station per round.
- Relay: upper central platform. F starts a four-second channel; stay within 2.6 m and line of sight. Leaving cancels progress. Reward: 180 credits and 25 team energy, once per round.
- Three reactive range targets in the southern lane grant 40 credits each on the first hit per round. All activities reset with the round. Combat now lasts 180 seconds.
- Updated north coolant hall and south maintenance lane, new solid cover, six authored prop/knife assets, dedicated first-person pistol/reload/knife clips, third-person pistol locomotion.
- 18 Russian synthetic voice lines, offline cooked WAV with subtitles. Current operator cast: Piper Denis, Piper Dmitri, AI_Roman trained XTTS (15 replacement lines). Three Elara mission cues retain the earlier SAPI voice. See ArtSource/OpenVoices/Голоса-и-лицензии.md for sources and XTTS non-commercial restrictions.
- New checks: `Scripts/Project.ps1 -Action ExpansionTest`; remote arsenal switching is checked by `Scripts/TestNetwork.ps1 -Movement`.
- Current operator voice source: audited WAV in `ArtSource/Audio`; import with `Scripts/ImportOpenVoices.py` through Unreal's Python commandlet. Offline generators and source notes are in `ArtSource/OpenVoices`; generated raw takes require review/editing before import. The older VoicePack SAPI generator would replace the current cast and is retained only as a legacy tool.


## Physics and HUD follow-up

One server-authoritative grazing metal ricochet per projectile (52% speed, 45% damage). Client-only directional debris with CCD, friction/restitution and a 128-piece cap. Slate HUD adds active weapon slots, reload progress, mission bearing/range, activity counters/capture progress and directional damage feedback. The packaged update passes 60 gameplay/physics checks plus the separate server/client checks. See Releases/Windows/Обновление.md and BUILDINFO.json.


## Modular weapons — 2026-09-20

Press B or open the weapon workshop from Escape. Primary/pistol optics, muzzle devices and magazine modules change visuals and server-authoritative statistics. Preparation only; selections persist; magazine changes conserve ammo. Six original module meshes, 27 primary / 18 pistol combinations. Packaged build: 85 checks passed. Separate dedicated server + owner client + observer client passed. Details: Releases/Windows/WeaponModules/Модули-оружия.md.


## Tactical stance — 2026-09-20

Alt quiet walk, Q/E lean with server collision and exposed head hit detection. F interact, Z class ability, X node link. Updated Windows build and 59 packaged checks passed; separate server + two clients passed. Details: Releases/Windows/TacticalStance/Тихий-шаг-и-наклоны.md.


## Tactical Operations — 2026-09-20

Три типа Mass-дронов со зрением, слухом и предупреждением об атаке; тихий шаг и глушитель влияют на обнаружение. Реле отключает охрану на 12 секунд. СКМ — метка только для союзников. Новый HUD угрозы, звуки, модели и ориентиры на арене; восстановлена генерация NavMesh. Alt/QE/F/Z/X сохранены. Windows-сборка: 158 проверок; отдельный сервер и три клиента: успешно. Подробности: [отчёт](Releases/Windows/Operations/Обновление.md).


## Наклоны и голоса — 2026-09-20

Исправлена инверсия поворота камеры: Q влево, E вправо. Заменены 15 реплик операторов: Denis / Dmitri / Рома из AI_Roman; субтитры синхронизированы с итоговыми WAV. 76 проверок Windows-сборки и отдельный сервер с двумя клиентами прошли. Голос Ромы используется для некоммерческого прототипа по XTTS CPML. Подробности: [отчёт](Releases/Windows/LeanVoice/Обновление.md), [источники и лицензии](Releases/Windows/VOICE-LICENSES.md).

## Операторы MetaHuman и фракции — 2026-09-20

Четыре класса (Инженер, Разведчик, Штурмовик, Медик) и пять фракций: 20 комплектов брони и шевронов, общий MetaHuman-скелет, 335 деталей и рукавов. Медик восстанавливает до 40 HP по Z в бою. Фракции меняются по карте и стороне; в тренировке выбор доступен в меню. Поставка: `Releases/Operators/Windows`. 251 проверка, выделенный сервер и два клиента прошли. [Модели](ArtSource/Operators/Preview.html), [описание](ArtSource/Operators/README.md), [проверки](ArtSource/Operators/VERIFICATION.md). Облачный лицевой риг MetaHuman пока не собран.
# nullRoute
# nullRoute
