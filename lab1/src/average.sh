#!/bin/bash

# Подсчет количества аргументов
count=$#


# Суммирование аргументов
sum=0
for arg in "$@"; do
  sum=$((sum + arg))
done

# Вычисление среднего 
average=$((sum / count))


# Вывод результата
echo "Количество аргументов: $count"
echo "Среднее арифметическое: $average"