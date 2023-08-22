#ifndef __BMP_H__
#define __BMP_H__

#include <stdio.h>
#include <stdint.h>


#pragma pack(push, 1)  // убираем выравнивание, чтобы все поля всех структур находились ровно друг за другом
                       // (именно так (друг за другом) распологаются поля и файле bmp)

typedef struct tagBITMAPFILEHEADER {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFO {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFO;

typedef struct tagPIXEL {  // структура для записи каждого пикселя картинки bmp
    uint8_t blue;  // цветове каналы пикселя идут именно в таком порядке в bmp
    uint8_t green;  
    uint8_t red;
} PIXEL;

#pragma(pop)  // далее выравнивание не мешает


typedef struct tagBMP {  // структура для записи изображения в программе
    int width;  // ширина (количество пикслелей по горизонтали) изображения
    int height;  // высота
    int shift_zeroes;  // количество нулевых байт, которые дописываются в строке пикселей до кратности 4 байт
    PIXEL** pixels;  // указатель на двумерный массив пикселей
    BITMAPFILEHEADER header;  // заголовки bmp файла
    BITMAPINFO info; 
} BMP;

static int padding_zeroes(int);  // перечисляем сигнатуры функций, которые есть в файле bmp.c
static int make_array_of_pixels(BMP*);
int load_image(FILE*, BMP*);
static void edit_header_and_info(BMP*);
int crop_image(BMP*, BMP*, int, int, int, int);
int rotate_image(BMP*, BMP*);
int save_image(FILE*, BMP*);
void free_image(BMP*);

#endif
