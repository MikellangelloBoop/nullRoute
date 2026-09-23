# Проверка штурмовика — 20.09.2026

- Unreal Engine 5.8.1, Editor Win64 Development: сборка успешна.
- NullRoute Win64 Development: сборка успешна, cook/stage/archive завершены.
- **84 проверки готовой Windows-игры: PASS** — Breacher 32, Module 25, Stance 27.
- Отдельный Editor dedicated server + два клиента: оба персонажа, все три фракции и смена сторон синхронизируются; сервер не создаёт косметические компоненты.
- Контрольный рендер всех трёх комплектов и вида от первого лица выполнен в packaged game. Стрельба, перезарядка, ADS и меню дополнительно просмотрены в Editor game mode при 1600×900.
- 54 Static Mesh assets, 18 материалов, 3 текстуры шевронов. GLB-экспорт: три файла, по 15 деталей с иерархией костей; проверены структура и размер бинарного контейнера.

| Вариант | Треугольников в мировой броне |
|---|---:|
| Chronos | 20 527 |
| Police | 21 214 |
| Rebel | 22 388 |

Оружие и детали от первого лица учитываются отдельно. Это проверка функций и ассетов, не измерение FPS или производительности матча на десять игроков.

Логи: `Saved/BreacherBuild.log`, `Saved/Package.log`, `Saved/PackagedBreacher-Breacher.log`, `Saved/PackagedBreacher-Module.log`, `Saved/PackagedBreacher-Stance.log`, `Saved/BreacherServer.log`, `Saved/BreacherClient.log`, `Saved/BreacherObserver.log`, `Saved/BreacherPackagedVisual.log`.

Проверка числа треугольников packaged-ассетов запускается с настоящим RHI: режим NullRHI удаляет render data и возвращает ноль полигонов. Скрипт `Scripts/TestBreacherPackage.ps1` учитывает это; игровые проверки Module и Stance остаются в NullRHI.

Упаковка обходит ошибку вложенного запуска UBT с кириллицей в служебном пути: `Scripts/Package.ps1` сначала вызывает сборку Editor и Game напрямую, затем cook/stage/archive без повторной сборки в UAT. Исходники движка и настройки профиля Windows не изменялись.
