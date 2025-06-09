#!/bin/bash

# Запускаем ps, фильтруем результаты через grep, сохраняем в переменной output
output=$(ps -ajx | grep "lamport")

# Используем sed для выбора второй строки из вывода grep
# Затем используем awk для извлечения второго значения из второго столбца
second_value=$(echo "$output" | sed -n '1p' | awk '{print $2}')

# Выводим только второе значение
echo "$second_value"

