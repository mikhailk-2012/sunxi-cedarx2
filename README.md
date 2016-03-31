Копия allwinner-zh/media-codec и  allwinner-zh/media-codec-lib

***
Изменения относительно allwinner-zh
makefile созданы заново, библиотеки теперь компилируются

***
Кросс-компиляция
В Makefile задать путь к компилятору в переменной CROSS_COMPILE
далее выполнить
make all
make pack
после make pack будет создан архив для целевой фс.

***
Установка
(по инструкции github.com/rosimildo/videoenc)
Распаковать созданный make pack архив на целевой системе.


***
Тестирование
Проект Demo allwinner-zh/media-codec не работает и исключен из данного репозитария.
Тестировался только h264 encoder на плате marsboard A20 с ос linux-sunxi 3.4.103
Тестирование производилось с помощью rosimildo/videoenc
Исходный файл для тестирования должен быть в формате NV12
Пример:
touch ./videosrc/out1.h264
./videoenc -i videosrc/paris_cif_nv12.yuv -k 3 -r 25 -b 1024 -s 352x288 -o ./videosrc/out1.h264

Кроме h264 encoder ничего не тестировалось. На системах отличных от A20 не тестировалось.



