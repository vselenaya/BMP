#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "bmp.h"


// Эта функция возвращает количество нулей (количество байт, состоящих целиком из нулевых бит),
// которое нужно дописать к каждой строке с пикселями в файле, чтобы длина строк была кратна 4 байтам:
static int padding_zeroes(int width) {  // width - ширина картинки
    return (4 - (3 * width) % 4) % 4;
}


// Эта функция для картинки image выделяет память под пиксельный массив (это двумерный
// массив размера image->height на image->width, где каждый элемент типа PIXEL), выделение двумерного массива
// осуществляется двумя malloc:
static int make_array_of_pixels(BMP* image) {
    image->pixels = (PIXEL**) malloc(image->height * sizeof(PIXEL*));  // массив, в котором хранятся адреса рядов с элементами
    if (image->pixels == NULL) {
        printf("Не удалось выделить память под массив(1) пикселей!\n");
        return 1;
    }

    PIXEL* array_rows = (PIXEL*) malloc(image->height * image->width * sizeof(PIXEL));  // массив, где ряды с элементами идут подряд
    if (array_rows == NULL) {
        printf("Не удалось выделить память под массив(2) пикселей!\n");
        return 1;
    }

    for (int i = 0; i < image->height; i++)
        image->pixels[i] = &array_rows[i * image->width];  // записываем адрес начала i-ого ряда элементов

    // теперь image->pixels[i][j] - это сначала берётся i-ый ряд: (image->pixels[i]), а потом в нём ищется j-ый элемент

    return 0;
}


// Функция загружает картинку image из bmp-файла file:
int load_image(FILE* file, BMP* image) {
    fseek(file, 0, SEEK_SET);  // переходим в начало файла

    size_t res_header = fread(&image->header, sizeof(BITMAPFILEHEADER), 1, file);  // считываем заголовки картинки из файла
    size_t res_info = fread(&image->info, sizeof(BITMAPINFO), 1, file);  // (по условию поля заголовков в файле хранятся без выравнивания,
    if (res_header != 1 || res_info != 1) {                              // поэтому мы их считываем так вот одной операцией чтения fread)
        printf("Неверный входной файл: отсутствуют обязательные поля формата bmp!\n");
        return 1;
    }
        
    image->width = image->info.biWidth;  // получаем информацию о картинке из полей заголовков
    image->height = abs(image->info.biHeight);  // высота может быть отрицательной... берём по модулю
    image->shift_zeroes = padding_zeroes(image->width);

    int res_pixel = make_array_of_pixels(image);
    if (res_pixel == 1)
        return 1;

    int res_fseek = fseek(file, image->header.bfOffBits, SEEK_SET);  // переходим в файле на место, где начинается пиксельный массив
    if (res_fseek != 0) {
        printf("Файл неожиданно кончился! Неверная информация в заголовке bmp о положении пикселей!\n");
        return 1;
    }

    // в bmp-файле пиксельный массив хранится в виде строк этого массива записанных подряд, но строки идут начиная с нижней:
    for (int i = image->height - 1; i >= 0; i --) {  
        size_t res_string_pixels = fread(image->pixels[i], sizeof(PIXEL), image->width, file);  // читаем i-ую строку массива 
        int res_fseek = fseek(file, image->shift_zeroes, SEEK_CUR);  // пропускаем нулевые байты, дополняющие строку до кратности 4 байт
        if (res_string_pixels != image->width || res_fseek != 0) {
            printf("Информация о размере картинки не соответствует количеству пикселей!\n");
            return 1;
        }
    }
    return 0;
}


// Функция, которая заполняет заголовки картинки image
static void edit_header_and_info(BMP* image) {
    size_t image_size = (image->width * sizeof(PIXEL) + image->shift_zeroes) * image->height;  // размер картинки как массива пикселей
    size_t prev_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO);  // размер заголовков картинки
    size_t file_size = prev_size + image_size;  // размер всего bmp-файла (у нас в упрощенной версии он состоит только из
                                                // заголовков и сразу за ними идущего массива пикселей)

    image->header.bfSize = file_size;
    image->header.bfOffBits = prev_size;  // массив пикселей начинется на prev_size-ом байте - то есть сразу после заголовков
    image->info.biHeight = image->height;
    image->info.biWidth = image->width;
    image->info.biSizeImage = image_size;
    return;
}


// Функция которая берёт картинку image, вырезает из неё кусок w на h пикселей (начиная c (x,y) пикселя) и сохраняет в cropped_image:
int crop_image(BMP* image, BMP* cropped_image, int x, int y, int w, int h) {
    cropped_image->header = image->header; 
    cropped_image->info = image->info;  // изначально копируем поля исходной картинки image
    cropped_image->width = w;  // размеры
    cropped_image->height = h;
    cropped_image->shift_zeroes = padding_zeroes(w);
    edit_header_and_info(cropped_image);  // редактируем поля cropped_image

    int res_pixel = make_array_of_pixels(cropped_image);
    if (res_pixel == 1)
        return 1;

    for (int i = 0; i < h; i ++)
        for (int j = 0; j < w; j ++)  // массив пикселей cropped_image - просто вырезанный кусок из массива image:
            cropped_image->pixels[i][j] = image->pixels[y + i][x + j];

    return 0;
}


// Функция, которая берёт картинку image, поворачиваем её на 90 градусов по часовой стрелке и сохраняет в rotated_image:
int rotate_image(BMP* image, BMP* rotated_image) {
    rotated_image->header = image->header;  // создаёем rotated_image, заполняя её поля
    rotated_image->info = image->info;
    rotated_image->width = image->height;  // rotated_image - повёрнутая image, поэтому высота и ширина просто меняются местами
    rotated_image->height = image->width;
    rotated_image->shift_zeroes = padding_zeroes(rotated_image->width);
    edit_header_and_info(rotated_image);

    int res_pixel = make_array_of_pixels(rotated_image);
    if (res_pixel == 1)
        return 1;

    for (int i = 0; i < image->height; i ++)
        for (int j = 0; j < image->width; j ++)  // просто поворачиваем массив пикселей:
            rotated_image->pixels[j][image->height - i - 1] = image->pixels[i][j];

    return 0;
}


// Функция, сохраняющая картинку image в файл file:
int save_image(FILE* file, BMP* image) {
    fseek(file, 0, SEEK_SET);  // переходим в начало файла

    size_t res_header = fwrite(&image->header, sizeof(BITMAPFILEHEADER), 1, file);  // записываем заголовки bmp-файла
    size_t res_info = fwrite(&image->info, sizeof(BITMAPINFO), 1, file);
    if (res_header != 1 || res_info != 1) {
        printf("Не удалось записать заголовки bmp в файл!\n");
        return 1;
    }

    const uint32_t zero = 0;  // перемнная, где все 4 байта - это нули (она нам нужна, чтобы была область памяти,
                              // откуда можно записать нули в файл... мы максимум пишем 3 байта нулей (это максимум,
                              // сколько нужно до кратности 4 байта дописать), поэтому переменной размера 4 байта достаточно)
    for (int i = image->height - 1; i >= 0; i --) {  // записываем строки массива пикселей, начиная с нижней
        size_t res_string_pixel = fwrite(image->pixels[i], sizeof(PIXEL), image->width, file);  // записывам строку в файл
        size_t res_zero = fwrite(&zero, 1, image->shift_zeroes, file);  // дописываем нули до кратности 4 байта
        if (res_string_pixel != image->width || res_zero != image->shift_zeroes) {
            printf("Не удалось записать пиксели в файл!\n");
            return 1;
        }
    }

    return 0;
}


// Функция, высвобождающая память, выделенную для изображения image:
void free_image(BMP* image) {
    if (image->pixels != NULL) {  // освобождаем массивы, которые не NULL (то есть у которых действительно выделена память)
        if (image->pixels[0] != NULL)
            free(image->pixels[0]);
        free(image->pixels);
    }
}

