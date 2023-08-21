#ifndef __STEGO_H__
#define __STEGO_H__

#include <stdint.h>
#include <stdio.h>

#include "bmp.h"


typedef struct tagSECRET {  // структура для хранения сообщения, которое мы храним в изображении
    size_t length;  // длина - количество символов (элементов char) в сообщении
    char* string;  // указатель на массив символов сообщения
    uint8_t* bits;  // указатель на массив битов, которыми кодируется сообщение
                    // (точнее: bits[i] - это элемент типа uint8_t, биты которого отвечают за кодировку символа string[i] сообщения)
} SECRET;           

typedef struct tagKEY {  // структура для хранения ключа
    size_t length;  // длина - количество строк в ключе = количество бит, которые можно этим ключом записать в изображение
    int* x;  // указатель на массив x-координат пикселей
    int* y;  // массив y-координат пикселей
    char* channel;  // массив каналов пикселей
} KEY;

void set_stego_size(int);  // функции, используемые в stego.c
static uint8_t get_num(char);
static char get_char(uint8_t);
static int make_message_arrays(SECRET*, size_t);
int read_message(FILE*, SECRET*, size_t);
static int make_key_arrays(KEY*, size_t);
int read_key(FILE*, KEY*, size_t);
static uint8_t* get_channel(BMP*, KEY*, size_t);
int extract_message(BMP*, SECRET*, KEY*);
int insert_message(BMP*, SECRET*, KEY*);
void free_message_key(SECRET*, KEY*);
int save_message(FILE*, SECRET*);


#endif