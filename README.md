Копия https://github.com/mzakharo/CedarX-12.06.2015

***
Изменения:
использованы makefile от cedarx2

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


