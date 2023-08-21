#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "bmp.h"
#include "stego.h"


static int STEGO_BITS_CODE = 8;  // количество бит, используемое для кодирования одного символа (char) сообщения

// Функция для установки значения STEGO_BITS_CODE
void set_stego_size(int new_param) {
    if (!(new_param > 0 && new_param <= 8)) {
        printf("Попытка установить некорректное значение STEGO_BITS_CODE! Использовано значение 8 по умолчанию!\n");
        STEGO_BITS_CODE = 8;
        return;
    }
    STEGO_BITS_CODE = new_param;
    return;
}


// Функция, выдающая по символу (char) 'с' его код - число, в котором не более STEGO_BITS_CODE первых бит значащие (не 0):
// (эту и следующую функцию можно писать так, как требуется для кодирования сообщения... я написал, чтобы
// символы A...Z, пробел, запятая и точка кодировались числами от 0 до 28 (всего 29 чисел, для их кодирования
// достаточно STEGO_BITS_CODE = 5) - как и требуется в лаборатрной работе...
// для STEGO_BITS_CODE = 8 можно просто само значение char возвращать, так как char занимает 8 битов)
static uint8_t get_num(char c) {
    if (STEGO_BITS_CODE == 8)
        return (uint8_t) c;  // просто сам код char вовзращаю

    if ('A' <= c && c <= 'Z')
        return c - 'A';
    if (c == ' ')
        return 26;
    if (c == '.')
        return 27;
    if (c == ',')
        return 28;

    printf("В сообщении неподходящий символ с кодом %d\n", c);
    return -1;
}


// Функция, которая по коду num возвращает символ char:
static char get_char(uint8_t num) {
    if (STEGO_BITS_CODE == 8)
        return (char) num;
        
    if (0 <= num && num <= 25)
        return num + 'A';
    if (num == 26)
        return ' ';
    if (num == 27)
        return '.';
    if (num == 28)
        return ',';

    printf("Попытка получить неизвестный символ с кодом %d\n", num);
    return -1;
}


// Функция, очищающая память у message и key:
void free_message_key(SECRET* message, KEY* key) {
    if (message->string != NULL)
        free(message->string);
    if (message->bits != NULL)
        free(message->bits);
    if (key->channel != NULL)
        free(key->channel);
    if (key->x != NULL)
        free(key->x);
    if (key->y != NULL)
        free(key->y);
}


// Функция создаёт массивы длины size для сообщения message:
static int make_message_arrays(SECRET* message, size_t size) {
    message->string = (char*) malloc(size * sizeof(char));
    message->bits = (uint8_t*) calloc(size, sizeof(uint8_t));  // важно, что массив, где находятся биты изнчально заполняем нулями!
                                                               // (ведь мы будем напрямую менять нужный бит, а остальные - нулями остаются)
    if (message->string == NULL || message->bits == NULL) {
        printf("Ошибка при попытке выделить память для кодового сообщения!\n");
        return 1;
    }
    return 0;
}


// Функция читает из файла file сообщение message, читает не более max_size символов:
int read_message(FILE* file, SECRET* message, size_t max_size) {
    int res_array = make_message_arrays(message, max_size);
    if (res_array != 0)
        return 1;

    size_t res_string = fread(message->string, sizeof(char), max_size, file);  // читает <= max_size символов
    if (res_string == 0) {
        printf("Ошибка при считывании файла с сообщением!\n");
        return 1;
    }
    message->length = res_string;  // длина сообщения - столько, сколько символов было прочитано

    for (size_t i = 0; i < message->length; i ++)
        message->bits[i] = get_num(message->string[i]);  // заполняем массив кодов символов

    return 0;
}


// Функция для создания массивов длины size для ключа key:
static int make_key_arrays(KEY* key, size_t size) {
    key->x = (int*) malloc(size * sizeof(int));
    key->y = (int*) malloc(size * sizeof(int));
    key->channel = (char*) malloc(size * sizeof(char));

    if (key->x == NULL || key->y == NULL || key->channel == NULL) {
        printf("Не удалось выделить память под массивы структуры KEY!\n");
        return 1;
    }

    return 0;
}


// Функция, читающая из файла file ключ key, не более max_size строк:
int read_key(FILE* file, KEY* key, size_t max_size) {   
    int res_array = make_key_arrays(key, max_size);
    if (res_array != 0)
        return 1;

    key->length = 0;  // изначально длина ключа 0
    for (size_t i = 0; i < max_size; i ++) {  // читаем не более max_size
        int res = fscanf(file, "%d %d %c", &key->x[i], &key->y[i], &key->channel[i]);  // читаем строку в файле ключа
        if (i == 0 && res == EOF) {
            printf("Не удалось считать ключ!\n");
            return 1;
        }

        if (res == EOF)
            break;  // если файл кончился - выходим
        
        if (res != 3) {
            printf("Некорректный файл с ключом! Каждая строка должна содержать два числа и символ - координаты и цвет пикселя!\n");
            return 1;
        }

        key->length ++;  // увеличиваем длину на 1
    }
    return 0;
}


// Функция, получающая указатель не память, где лежит нужный цветовой канал пикселя в изображении image -
// - берётся канал, записанный в i-ом индексе ключа key:
static uint8_t* get_channel(BMP* image, KEY* key, size_t i) {
    if (key->channel[i] == 'R')
        return &image->pixels[key->y[i]][key->x[i]].red;
    if (key->channel[i] == 'G')
        return &image->pixels[key->y[i]][key->x[i]].green;
    if (key->channel[i] == 'B')
        return &image->pixels[key->y[i]][key->x[i]].blue;  
}


// Функция, извлекающая из изображния image сообщение message по ключу key:
int extract_message(BMP* image, SECRET* message, KEY* key) {
    message->length = key->length / STEGO_BITS_CODE;  // каждая строка ключа кодирует один бит символа сообщения, тогда
                                                      // длина сообщения - целая часть от деления длины ключа на количество
                                                      // бит для кодировки одного символа сообщения
    int res_array = make_message_arrays(message, message->length);
    if (res_array == 1)
        return 1;

    for (size_t i = 0; i < STEGO_BITS_CODE * message->length; i ++) {  // проходимся по каждому биту всех символов сообщения
        // (заметим: i-ый бит сообщения - это (i % STEGO_BITS_CODE)-ый бит (i / STEGO_BITS_CODE)-ого символа сообщения)

        if (*get_channel(image, key, i) & 1)  // если младший бит канала (в котором записан i-ый бит сообщения) равен 1
            message->bits[i / STEGO_BITS_CODE] |= ((uint8_t) 1 << (i % STEGO_BITS_CODE));  // значит соответствующий бит символа 1
        else  // если же младший бит 0
            message->bits[i / STEGO_BITS_CODE] &= ~((uint8_t) 1 << (i % STEGO_BITS_CODE));  // значит и бит символа тоже 0
    }

    for (size_t i = 0; i < message->length; i ++)
        message->string[i] = get_char(message->bits[i]);  // получаем сами символы сообщения

    return 0;
}


// Функция, которая сохраняет сообщение message в файл file:
int save_message(FILE* file, SECRET* message) {
    size_t res_message = fwrite(message->string, sizeof(char), message->length, file);  // записываем сообщение
    int res_next_string = putc('\n', file);  // в конец добавляем перевод строки
    if (res_message != message->length || res_next_string == EOF) {
        printf("Файл с расшированным сообщением записан не полностью!\n");
        return 1;
    }
    return 0;
}


// Функция, которая вставляет сообщение message по ключу key в изображение image:
int insert_message(BMP* image, SECRET* message, KEY* key) {
    /*
    Кодировка сообщения происходит так:
    У нас сообщение message состоит из message->length символов, каждый символ мы кодируем STEGO_BITS_CODE битами...
    то есть всего на кодировку сообщения уходит STEGO_BITS_CODE * message->length битов.
    Каждый из этих битов записывается в младший разряд того цветового канала, который казан в ключе: а именно i-ая строка
    ключа содержит x, y - координаты пикселя и channel - канал (R (красный), G (зелёный), B (синий)) этого пикселя... каждый канал
    каждого пикселя занимает несколько бит - и в младший бит как раз и записывается бит сообщения.
    То есть мы берём i-ый бит сообщения, берём i-ый канал из ключа (это делает get_channek, которая просто возвращает
    указатель на память, где лежит нудный канал) и в младий бит этого канала помещаем наш бит. Так и кодируем все
    STEGO_BITS_CODE * message->length битов - на что уходит STEGO_BITS_CODE * message->length битов каналов, то
    есть STEGO_BITS_CODE * message->length битов строк ключа.
    */
    for (size_t i = 0; i < STEGO_BITS_CODE * message->length; i ++) {  // проходимся по всем битам сообщения
        // (бит номер i сообщения - это бит номер (i % STEGO_BITS_CODE) в символе (i / STEGO_BITS_CODE))
        if ((message->bits[i / STEGO_BITS_CODE] >> (i % STEGO_BITS_CODE)) & 1)  // если бит сообщения равен 1
            *get_channel(image, key, i) |= (uint8_t) 1;  // записываем в младший бит канала 1
        else  // есди же 0
            *get_channel(image, key, i) &= ~((uint8_t) 1);  // записываем в младший бит 0
    }
    return 0;
}
