# Сторонние компоненты и атрибуции

## TortoiseGit

Проект является кроссплатформенной адаптацией оригинального Revision Graph из
TortoiseGit. Алгоритм, модель графа и поведение интерфейса перенесены из
TortoiseGit. TortoiseGit распространяется под GNU General Public License
version 2.

- Upstream: https://gitlab.com/tortoisegit/tortoisegit
- Лицензия: [LICENSE](LICENSE)

## OGDF

Open Graph Drawing Framework используется для раскладки графа. Подключённая
ревизия хранится как git-submodule в `third_party/ogdf`.

- Upstream: https://github.com/ogdf/ogdf
- Условия: `third_party/ogdf/LICENSE.txt`
- Для данной объединённой работы применяется вариант GPLv2.

## Иконки TortoiseSVN/TortoiseGit

`assets/revgraphbar.bmp` и производный `assets/revgraphbar.png` содержат иконки,
полученные из проекта TortoiseSVN/TortoiseGit. Они используются для действий
клиента системы контроля версий. Требуемое уведомление и полный текст условий
находятся в [LICENSES/TortoiseSVN-Icon-License.txt](LICENSES/TortoiseSVN-Icon-License.txt).

TortoiseGit и TortoiseSVN являются проектами соответствующих правообладателей;
этот проект не заявляет об официальной аффилиации с ними.
