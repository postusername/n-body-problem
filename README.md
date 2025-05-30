# Симулятор задачи N тел
Данная программа позволяет симулировать задачу N тел при помощи различных численных методов.


## Содержание
- [Сборка](#сборка)
- [Запуск](#запуск)
- [Системы](#системы)
- [Методы](#методы)
- [Справка по GUI](#справка-по-интерфейсу)


## Сборка
На данный момент протестирован билд только под Ubuntu-alike ОС. Для остальных систем следует установить зависимости, указанные в CMake.
```bash
sudo apt update
sudo apt install make cmake clang pkg-config libgtkmm-4.0-dev libsigc++-3.0-dev libgtk-4-dev libglibmm-2.68-dev libcairomm-1.16-dev libpangomm-2.48-dev libglib2.0-dev libpango-1.0-0 libcairo2-dev libgdk-pixbuf-2.0-0 gir1.2-graphene-1.0 ffmpeg libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libfftw3-bin libfftw3-dev
git clone https://github.com/postusername/n-body-problem
cd n-body-problem
mkdir -p build
cd build
cmake ..
make
```


## Запуск
Поддерживаются следующие флаги командной строки:
- `--help-all` показывает все команды справки
- `--dt` устанавливает временной шаг для интерактивной симуляции

Код полностью модульный. Чтобы сменить моделируемую систему или численный метод, необходимо поменять названия классов в строках 252-254 файла `main.cpp`.


## Системы
- `TwoBodySystem` модель спутника
- `ThreeBodySystem` устойчивое планарное решение для $N=3$, образующее лемнискату
- `CircleSystem` модель типа "кольцо"
- `SolarSystem` модель солнечной системы (рекомендуется не ставить $dt < 5e-2$ для обыкновенного симулятора)


## Методы
- `NewtonSimulator` обыкновенный симулятор, работающий по методу leapfrog, временная сложность $O(N^2)$
- `ParcticleMashSimulator` симулятор, основанный на решении уравнения Пуассона в Фурье-пространстве, сложность $O(N\log N)$


## Справка по интерфейсу

У программы есть несколько вкладок. Первая из них -- "Симуляция". На ней отображается интерактивный процесс обсчёта траекторий движения N тел. В самом начале, при генерации первого кадра, внизу окна отображается соответствующая надпись. При неправильном выборе настроек (большом количестве тел и малом временном шаге) или слабом железе она может отображаться довольно долго. В течение этого времени необходимо подождать. После этого становятся доступны элементы управления. По окну можно перемещаться при помощи мыши и колёсика. Также доступны повороты по осям при нажатии колёсика или правой кнопки мыши. Справа снизу отражаются текущие координаты курсора и параметры поворота. Чтобы вернуться к базовому виду на систему, нужно нажать кнопку "Сбросить вид". Ползунок "Длина отслеживания траектории" позволяет регулировать, как долго траектория тела должна отображаться на экране. Нажатие на клавишу F1 де-/активирует режим просмотра в глубину, который более ближние точки рисует более тёмными, а более дальние -- более светлыми. Кнопка "Рендер" переведёт программу в режим обсчёта траектории для сохранения их в видео. Положение камеры фиксируется после нажатия кнопки "Начать запись".

Вторая вкладка -- "График". На ней отражён график полной механической энергии моделируемой системы. По графику можно перемещаться и масштабировать его при помощи мышки и колёсика. Если навести курсор на одну из осей, то масштабирование будет проводиться только по ней. Кнопка "Сбросить вид графика" отмасштабирует график так, чтобы полностью поместить его на экране.
