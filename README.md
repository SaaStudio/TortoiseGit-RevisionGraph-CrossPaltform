# TortoiseGit Revision Graph CrossPaltform

Кроссплатформенная адаптация оригинального **Revision Graph из TortoiseGit**.
Приложение сохраняет алгоритм построения и внешний вид оригинального графа,
показывает топологию веток и тегов Git и не изменяет открываемый репозиторий.
Интерфейс построен на Qt, поэтому код рассчитан на Linux, Windows, macOS и другие
платформы, на которых доступны Qt 5, CMake, Git и OGDF.

## Возможности

- оригинальная OGDF-раскладка `SugiyamaLayout`;
- режим `--simplify-by-decoration` и отображение всех refs;
- точки ветвления и слияния;
- цвета локальных и удалённых веток, тегов и текущей ветки;
- масштабирование, поиск, обзор и экспорт графа;
- только читающие операции Git.

## Поддержка платформ

| Платформа | Статус |
|---|---|
| Linux x86_64 | Сборка и упаковка проверены в GitHub Actions |
| Windows x86_64 | Нативная MSVC-сборка и упаковка с Qt DLL проверены в GitHub Actions |
| macOS x86_64 | Нативная сборка `.app` и упаковка Qt frameworks проверены в GitHub Actions; на Apple Silicon запускается через Rosetta 2 |
| Другие ОС с Qt 5 | Возможна сборка, официально не проверено |

## Готовые сборки

Готовые архивы находятся в каталоге [`dist`](dist):

- Linux x86_64 — `tar.gz`;
- Windows x86_64 — `zip` с исполняемым файлом и Qt DLL;
- macOS x86_64 — `zip` с готовым `.app` и Qt frameworks.

Для чтения репозитория приложению на любой ОС нужен установленный Git. Linux-
сборке дополнительно нужен системный Qt 5 runtime.

## Требования

- операционная система с Qt 5, CMake и Git;
- CMake 3.16+ и компилятор с C++17;
- Qt 5 Widgets;
- Git;
- OGDF — подключается как submodule.

Пример установки зависимостей для Ubuntu/Debian:

```bash
sudo apt install build-essential cmake ninja-build qtbase5-dev git
```

## Сборка

```bash
git clone --recurse-submodules https://github.com/SaaStudio/TortoiseGit-RevisionGraph-CrossPaltform.git
cd TortoiseGit-RevisionGraph-CrossPaltform
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target tgraph
```

Если репозиторий уже клонирован без submodule:

```bash
git submodule update --init --recursive
```

## Запуск

```bash
./build/tgraph /путь/к/git-репозиторию
```

Без аргумента приложение открывает репозиторий из текущего каталога.

Команды CMake одинаковы на всех платформах. Конкретный генератор (`Ninja`,
Visual Studio или Xcode) и способ установки Qt выбираются под целевую ОС.

## Управление

- перетаскивание мышью — перемещение графа;
- `Ctrl` + колесо — изменение масштаба;
- панель и меню «Вид» — масштаб, фильтры, обзор и поиск.

## Происхождение и лицензия

Проект является производной работой TortoiseGit и распространяется под
[GNU GPL v2](LICENSE). Раскладка выполняется библиотекой
[OGDF](third_party/ogdf), доступной для этой сборки по GPLv2. Иконки панели
происходят из TortoiseSVN/TortoiseGit; условия и атрибуции перечислены в
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

См. также [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md) и
[SUPPORT.md](SUPPORT.md).
