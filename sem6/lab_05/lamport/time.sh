#!/bin/bash

LOG_FILE="time.log"  # Файл для записи логов
DATA_FILE="time_data.txt"  # Файл для хранения данных для графика

# Проверка наличия аргумента
if [ $# -ne 1 ]; then
    echo "Usage: $0 <max_iterations>"
    exit 1
fi

max_iterations=$1

# Функция для запуска процессов и записи логов
run_processes() {
    local iteration=$1

    echo "Running $iteration processes..."
    echo "Running $iteration processes..." >> "$LOG_FILE"

    # Запуск процессов и запись логов
    for ((i = 1; i <= iteration; i++)); do
        ./lamport_client localhost | tr -d '\0' >> "$LOG_FILE" &  # Запуск процесса в фоновом режиме и запись вывода в лог
    done

    # Ожидание завершения всех процессов
    wait
}

# Функция для вычисления среднего времени выполнения
calculate_average_time() {
    local iteration=$1

    # Вычисление среднего времени выполнения и запись в файл
    average_time=$(awk '/Total execution time/{total += $4} END {print total / '"$iteration"'}' "$LOG_FILE")
    echo "$iteration $average_time" >> "$DATA_FILE"
}

# Очистка файла логов и файла данных перед началом записи
> "$DATA_FILE"

# Вывод заголовка таблицы
echo "Processes | Average Total execution time (ms)"
echo "--------------------------------------------"

# Запуск цикла для разного числа процессов
for ((n = 1; n <= max_iterations; n++)); do
	> "$LOG_FILE"
    run_processes $n
    calculate_average_time $n
done

# Построение графика
gnuplot -persist <<-EOFMarker
    set title "Average Total execution time" font ",10"
    set xlabel "Number of processes" font ",8"
    set ylabel "Time (ms)" font ",8"
    set grid
    set xtics 1
    set format x "%.0f"
    set style line 1 linewidth 2
    plot "$DATA_FILE" with linespoints title "Average Time"
EOFMarker
