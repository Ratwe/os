#!/bin/bash

# Проверка наличия аргумента
if [ $# -eq 0 ]; then
    # Если аргументов нет, выполняем скрипт один раз

    # Запустить команду ps -ajx и найти строку с lamport_server
    line=$(ps -ajx | grep "lamport_server")

    # Получить значение PID из второго столбца
    pid=$(echo "$line" | awk '{print $2}')

    # Выполнить команду ./proc.o с найденным PID и записать вывод в файл proc_out.txt
    ./proc.o "$pid" proc_out.txt

    # Извлечь строку с num_threads из файла proc_out.txt
    grep "num_threads" proc_out.txt
else
    # Если передан аргумент, циклиться с указанным интервалом

    sleep_interval=$1

    while true; do
        # Запустить команду ps -ajx и найти строку с lamport_server
        line=$(ps -ajx | grep "lamport_server")

        # Получить значение PID из второго столбца
        pid=$(echo "$line" | awk '{print $2}')

        # Выполнить команду ./proc.o с найденным PID и записать вывод в файл proc_out.txt
        ./proc.o "$pid" proc_out.txt

        # Извлечь строку с num_threads из файла proc_out.txt
        grep "num_threads" proc_out.txt

        # Подождать указанный интервал
        sleep "$sleep_interval"
    done
fi

