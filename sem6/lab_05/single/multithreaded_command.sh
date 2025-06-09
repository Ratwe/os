#!/bin/bash

# Проверяем, переданы ли все необходимые аргументы
if [ $# -lt 3 ]; then
    echo "Usage: $0 <num_threads> <delay> <command>"
    exit 1
fi

# Количество потоков, указанное в аргументе
NUM_THREADS=$1
# Задержка между созданием потоков, указанная в аргументе
DELAY=$2
# Команда для выполнения, введенная в аргументах
COMMAND="${@:3}"

# Функция для выполнения команды в нескольких потоках
execute_command() {
    local command=$1
    local thread_id=$2
    echo "Thread $thread_id executing: $command"
    eval $command
}

# Запуск времени перед выполнением команды
start_time=$(date +%s.%N)

# Цикл для запуска команды в нескольких потоках
for ((i=1; i<=$NUM_THREADS; i++)); do
    execute_command "$COMMAND" $i &
    sleep "$DELAY"
done

# Ждем завершения всех потоков
wait

# Завершение времени после выполнения команды
end_time=$(date +%s.%N)

# Рассчитываем время выполнения
execution_time=$(echo "$end_time - $start_time" | bc)

# Выводим время выполнения
echo "Execution time: $execution_time seconds"