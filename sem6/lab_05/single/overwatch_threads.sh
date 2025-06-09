#!/bin/bash

# Проверяем, переданы ли все необходимые аргументы
if [ $# -lt 1 ]; then
    echo "Usage: $0 <update_delay> [-n]"
    exit 1
fi

# Задержка между обновлениями графика
UPDATE_DELAY=$1

# Флаг для определения, нужно ли ждать начала и окончания multithreaded_command.sh
WAIT_FLAG=1

# Проверяем наличие флага -n
if [ "$2" == "-n" ]; then
    WAIT_FLAG=0
fi

# Функция для получения значения num_threads
get_num_threads() {
    ./get_num_threads.sh | grep -o "'[0-9]\+'" | tr -d "'"
}

# Функция для рисования графика
draw_graph() {
    local count=0
    local x_values=()
    local y_values=()

    if [ $WAIT_FLAG -eq 1 ]; then
        echo "Waiting for multithreaded_command.sh to start..."
        # Ожидаем запуска multithreaded_command.sh
        while ! ps -ef | grep multithreaded_command.sh | grep -v grep > /dev/null; do
            sleep 1
        done
    fi

    # Начало времени выполнения команды
    start_time=$(date +%s)

    # Постоянно получаем значение num_threads и рисуем график
    while true; do
        # Получаем значение num_threads
        num_threads=$(get_num_threads)
        
        # Если получено значение num_threads, добавляем его в график
        if [ -n "$num_threads" ]; then
            ((count++))
            x_values+=("$(date +%T)")
            y_values+=("$num_threads")

            # Очистка экрана
            clear

            # Рисуем график
            echo "Time Graph of num_threads"
            echo "-------------------------"
            for ((i = 0; i < ${#x_values[@]}; i++)); do
                printf "%s: " "${x_values[$i]}"
                for ((j = 0; j < ${y_values[$i]}; j++)); do
                    printf "#"
                done
                echo ""
            done
            echo "-------------------------"
        fi

        if [ $WAIT_FLAG -eq 1 ]; then
            # Проверяем, завершилась ли multithreaded_command.sh
            if ! ps -ef | grep multithreaded_command.sh | grep -v grep > /dev/null; then
                # Завершаем график
                end_time=$(date +%s)
                execution_time=$(($end_time - $start_time))
                echo "Multithreaded command has finished. Execution time: $execution_time seconds."
                break
            fi
        fi

        # Задержка перед следующим обновлением графика
        sleep "$UPDATE_DELAY"

    done
}

# Запускаем функцию рисования графика в фоновом режиме
draw_graph &

# Ждем завершения выполнения обоих потоков
wait
