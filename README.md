# Модуль ядра Linux: `dmp` (Device Mapper Proxy)

## Описание

Модуль `dmp` (Device Mapper Proxy) — это модуль ядра Linux для подсистемы Device Mapper. Он перенаправляет запросы ввода-вывода (I/O) на указанное базовое устройство и собирает статистику по операциям чтения и записи. Полученные данные доступны через интерфейс `sysfs`

**Основные возможности:**
* Перенаправление I/O-запросов на базовое устройство
* Сбор и экспорт статистики через `sysfs` (`/sys/module/dmp/stat/volumes`)
  - Количество запросов чтения и их средний размер блока
  - Количество запросов записи и их средний размер блока
  - Общее количество запросов и их средний размер блока

---

## Зависимости

Для сборки, установки и использования модуля требуются

### **Ubuntu / Debian**
* `linux-headers-$(uname -r)` — заголовочные файлы текущего ядра
* `build-essential` — базовые инструменты сборки
* `dmsetup` — утилита для управления устройствами Device Mapper

```bash
sudo apt-get install linux-headers-$(uname -r) build-essential dmsetup
```

### **Fedora**
* `kernel-devel-$(uname -r)` — заголовочные файлы текущего ядра
* `gcc`, `make` — компилятор и инструменты сборки
* `device-mapper` — содержит `dmsetup` для работы с Device Mapper

```bash
sudo dnf install kernel-devel-$(uname -r) gcc make device-mapper
```

---

## Установка
Для того, чтобы загрузить модуль в ядро Linux следует выполнить шаги

1. **Сборка модуля**

    ```bash
    sudo make
    ```

2. **Загрузка модуля**

    Загрузите модуль в ядро

    ```bash
    sudo insmod dmp.ko
    ```

    Проверьте, что модуль был успешно загружен

    ```bash
    lsmod | grep dmp
    ```

---

## Использование

1. **Создание устройства Device Mapper**

    Используйте `dmsetup` для создания блочного устройства

    ```bash
    sudo dmsetup create zero1 --table "0 $size zero" 
    ```

    > `$size` - произвольный размер устройства

2. **Создание нашего proxy устройства**

    Создайте proxy устройство и передайте ему в качестве аргумента устройство, с которого вы хотите собирать статистику

    ```bash
    sudo dmsetup create dmp1 --table "0 $size dmp /dev/mapper/zero1"
    ```

    > `$size` - размер устройства `/dev/mapper/zero1`

3. **Просмотр созданных устройств**

    ```bash
    sudo ls -al /dev/mapper/*
    ```

3. **Сбор статистики с устройства**

    Статистика доступна через `sysfs`

    ```bash
    sudo cat /sys/module/dmp/stat/volumes
    ```

    Пример вывода статистики

    ```txt
    read:
        reqs: 1234
        avg size: 4096
    write:
        reqs: 567
        avg size: 8192
    total:
        reqs: 1801
        avg size: 6144
    ```

    > `reqs` — количество запросов  
    > `avg size` — средний размер блока данных (округлен вверх)

---

## Тестирование

Для того, чтобы протестировать модуль `dmp`, используйте скрипт `test_dmp.sh`, который создает тестовое устройство с целью `zero` (`/dev/mapper/zero_test`),затем создаёт устройство `/dev/mapper/dmp_test` с целью `dmp`, привязанное к `/dev/mapper/zero_test`, и выполняет операции чтения/записи

* Убедитесь, что модуль загружен (`sudo insmod dmp.ko`)
* Убедитесь в отсуствии устройств с именами `zero_test` и `dmp_test`

```bash
chmod +x test_dmp.sh
sudo ./test_dmp.sh /dev/mapper/zero1
```

**Вывод**
```txt
Creating temporary zero Device Mapper device...
Creating test Device Mapper device with dmp target...
Test statistics before:
read:
        reqs: 47444
        avg size: 7498
write:
        reqs: 8704
        avg size: 4096
total:
        reqs: 56148
        avg size: 6971

Performing test read (1 MB)...
Test statistics:
read:
        reqs: 47702
        avg size: 7513
write:
        reqs: 8704
        avg size: 4096
total:
        reqs: 56406
        avg size: 6985

Performing test write (1 MB)...
Test statistics:
read:
        reqs: 48320
        avg size: 7505
write:
        reqs: 8960
        avg size: 4096
total:
        reqs: 57280
        avg size: 6972
Removing test device...
Removing temporary zero device...
Test completed successfully
```

---

## Удаление

Для того, чтобы выгрузить модуль `dmp` из ядра Linux, сначала нужно удалить все созданные устройства Device Mapper с целью `dmp`. Это необходимо для освобождения ресурсов модуля

1. **Удаление устройств Device Mapper**

    ```bash
    sudo dmsetup remove dmp1
    ```

2. **Выгрузка модуля**

    Выгрузите модуль

    ```bash
    sudo rmmod dmp
    ```

    Проверьте, что модуль успешно выгружен

    ```bash
    lsmod | grep dmp
    ```

3. Очистка после сборки:

    ```bash
    sudo make clean
    ```

---

## Отладка

Сообщения модуля в системном логе имеют префикс `dmp`. Для просмотра логов используйте следующую команду

```bash
dmesg | grep "dmp:"
```

Примеры сообщений

```txt
dmp: Failed to allocate memory for dmp_data
dmp: Invalid number of arguments
```

## Лицензия
Распространяется под лицензией GNU GPLv3. См. файл [LICENSE](LICENSE) для подробностей