#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "bmp.c"
#include "stego.c"


int main(int argc, char** argv) {
    // -------- данный блок кода - проверка входных параметров, с которыми запускается программа --------
    if (argc < 5) {
        printf("Неверное количество аргументов при запуске!\n");
        return 1;
    }

    if (!(strcmp(argv[1], "crop-rotate") == 0 || strcmp(argv[1], "insert") == 0 || strcmp(argv[1], "extract") == 0)) {
        printf("Неизвестная команда: %s\n", argv[1]);
        return 1;
    }

    if (strcmp(argv[1], "crop-rotate") == 0 && argc != 8) {
        printf("Некорректное количество аргументов команды crop-rotate!\n");
        return 1;
    }

    if (strcmp(argv[1], "insert") == 0 && argc != 6) {
        printf("Некорректное количество аргументов команды insert!\n");
        return 1;
    }

    if (strcmp(argv[1], "extract") == 0 && argc != 5) {
        printf("Некорректное количество аргументов команды extract!\n");
        return 1;
    }
    // -------- считаем, что входные параметры корректны --------



    int return_value = 0;  // код, который вернёт программа (0 - программа успешно завершилась, иначе - были какие-то ошибки)
    const int MAX_SIZE_KEY = 1e6;  // максимальное количество строк в файле-ключе для стеганографии
    const int STEGO_BITS_CODE = 5;  // количество бит, затрачиваемое на кодирование одного символа в стеганографии
                                    // (может быть <= 8, так как символы в C - типа char, который как раз занимает 8 бит)
    set_stego_size(STEGO_BITS_CODE);  // устанавливаем значение



    // -------- код для выполнения команды crop-rotate --------
    if (strcmp(argv[1], "crop-rotate") == 0) {
        FILE* input_file = fopen(argv[2], "rb");
        if (input_file == NULL) {
            printf("Входной файл %s не открывается!\n", argv[2]);
            return 1;
        }

        int x, y, w, h;
        x = atoi(argv[4]);
        y = atoi(argv[5]);
        w = atoi(argv[6]);
        h = atoi(argv[7]);

        BMP image_bmp, cropped_image, rotated_image;
        image_bmp.pixels = NULL;  // изначально все указатели на области памяти делаем NULL, чтобы показать, что 
        cropped_image.pixels = NULL;  // пока память не выделена и они никуда не указывают 
        rotated_image.pixels = NULL;  // (чтобы случайно free от невыделенного массива не вызвать)

        // загружаем из файла image_bmp - исходное изображение:
        if (load_image(input_file, &image_bmp) == 1) {  // при наличии ошибок (функция (в данном случае load_image) возвращает не 0):
            printf("Не удалось прочитать bmp файл!\n");  
            return_value = 1;  // устанавливаем ненулевой код возврата 
            goto out_crop_rotate;  // и переходим к точке выхода из программы
        }

        // проверяем входные параметры на корректность для данного изображения:
        if (x < 0 || y < 0 || x + w > image_bmp.width || y + h > image_bmp.height) {
            printf("Поданные на вход параметры для вырезки изображения выходят за пределы изображения!\n");
            return_value = 1;
            goto out_crop_rotate;
        }

        // получаем cropped_image - обрезанное исходное изображение:
        if (crop_image(&image_bmp, &cropped_image, x, y, w, h) == 1) {  
            printf("Не удалось вырезать картинку!\n");
            return_value = 1;
            goto out_crop_rotate;
        }

        // получаем rotated_image - повёрнутое обрезанное изображение:
        if (rotate_image(&cropped_image, &rotated_image) == 1) {  
            printf("Не удалось повернуть картинку!\n");
            return_value = 1;
            goto out_crop_rotate;
        }
        
        FILE* output_file = fopen(argv[3], "wb");
        if (output_file == NULL) {
            printf("Выходной файл не открылся!\n");
            return_value = 1;
            goto out_crop_rotate;
        }

        // сохраняем результат в файл:
        if (save_image(output_file, &rotated_image) == 1) {  
            printf("Не удалось сохранить bmp файл!\n");
            return_value = 1;
            fclose(output_file);
            goto out_crop_rotate;
        }
        fclose(output_file);  // не забыли закрыть файл!
        

out_crop_rotate:  // точка выхода из программы при выполнении команды crop-rotate (здесь программа оказывается
        free_image(&rotated_image);  //                 или после выполнения всего кода перед этой строкой, или через goto)
        free_image(&cropped_image);
        free_image(&image_bmp);  // освобождаем всю выделенную память, закрываем файлы и возвращаем код возврата
        fclose(input_file);
        return return_value;
    }



    // -------- код для выполнения команды insert --------
    if (strcmp(argv[1], "insert") == 0) {
        FILE* input_file = fopen(argv[2], "rb");
        if (input_file == NULL) {
            printf("Входной файл не открывается!\n");
            return 1;
        }

        FILE* output_file = fopen(argv[3], "wb");  // сначала открываем все файлы 
        if (output_file == NULL) {
            printf("Выходной файл не открылся!\n");
            fclose(input_file);
            return 1;
        }
        
        FILE* key_file = fopen(argv[4], "rb");
        if (key_file == NULL) {  // если где-то не получилось открыть файл, то сразу выходим из программы
            printf("Файл с ключом не открылся!\n");
            fclose(input_file);  // не забывая закрыть уже открытые файлы
            fclose(output_file);  
            return 1;
        }

        FILE* message_file = fopen(argv[5], "rb");
        if (message_file == NULL) {
            printf("Файл с сообщением не открылся!\n");
            fclose(input_file);
            fclose(output_file);
            fclose(key_file);
            return 1;
        }
        
        BMP image_bmp;  // исходная картинка, куда будем стеганограмму записывать
        image_bmp.pixels = NULL;
        SECRET message;  // секретное сообщение
        message.string = NULL;
        message.bits = NULL;
        KEY key;  // ключ для записи секретного сообщения
        key.channel = NULL;
        key.x = NULL;
        key.y = NULL;

        // загружаем картинку:
        if (load_image(input_file, &image_bmp) == 1) {
            printf("Не удалось прочитать bmp файл!\n");
            return 1;
        }

        // считывем ключ, но не более MAX_SIZE_KEY строк:
        if (read_key(key_file, &key, MAX_SIZE_KEY) == 1) {  
            printf("Ключ считать не удалось!\n");
            return_value = 1;
            goto out_insert;
        }

        // считываем сообщение, но не более, чем key.length / STEGO_BITS_CODE символов (элементов типа char), 
        // так как именно столько символов может закодировать ключ из key.length строк:
        // (если символ в сообщении не ASCII - например, буква "ы", то в C она будет кодироваться несколькими
        // элементами типа char, а потому будет считаться за несколько символов... ключ нужен достаточно длинный)
        if (read_message(message_file, &message, key.length / STEGO_BITS_CODE) == 1) {   
            printf("Считать сообщение не удалось!\n");
            return_value = 1;
            goto out_insert;
        }

        // вставляем в картинку секретное сообщение:
        if (insert_message(&image_bmp, &message, &key) == 1) {
            printf("Вставить сообщение в изображение не удалось!\n");
            return_value = 1;
            goto out_insert;
        }

        // сохраняем картинку с сообщением:
        if (save_image(output_file, &image_bmp) == 1) {
            printf("Сохранить изображение с сообщением не удалось!\n");
            return_value = 1;
            goto out_insert;
        }
        printf("Сохранить сообщение в картинке удалось! Задействовано %zu строк ключа!\n", STEGO_BITS_CODE * message.length);


out_insert:  // точка выхода из программы при выполнении команды insert
        free_image(&image_bmp);
        free_message_key(&message, &key);
        fclose(input_file);
        fclose(output_file);
        fclose(message_file);
        fclose(key_file);
        return return_value;
    }



    // -------- код для выполнения команды extract --------
    if (strcmp(argv[1], "extract") == 0) {
        FILE* input_file = fopen(argv[2], "rb");
        if (input_file == NULL) {
            printf("Входной файл не открывается!\n");
            return 1;
        }

        FILE* key_file = fopen(argv[3], "rb");
        if (key_file == NULL) {
            printf("Файл с ключом не открылся!\n");
            fclose(input_file);
            return 1;
        }

        FILE* message_file = fopen(argv[4], "wb");
        if (message_file == NULL) {
            printf("Файл для сообщения не открылся!\n");
            fclose(input_file);
            fclose(key_file);
            return 1;
        }

        BMP image_bmp;
        image_bmp.pixels = NULL;
        SECRET message;
        message.string = NULL;
        message.bits = NULL;
        KEY key;
        key.channel = NULL;
        key.x =NULL;
        key.y = NULL;

        if (load_image(input_file, &image_bmp) == 1) {
            printf("Не удалось прочитать bmp файл!\n");
            return 1;
        }
        
        if (read_key(key_file, &key, MAX_SIZE_KEY) == 1) {
            printf("Считать файл с ключом не удалось!\n");
            return_value = 1;
            goto out_extract;
        }

        if (extract_message(&image_bmp, &message, &key) == 1) {
            printf("Не удалось извлечь сообщение!\n");
            return_value = 1;
            goto out_extract;
        }

        if (save_message(message_file, &message) == 1) {
            printf("Не удалось сохранить извлечённое сообщение!\n");
            return_value = 1;
            goto out_extract;
        }

out_extract:
        free_image(&image_bmp);
        free_message_key(&message, &key);
        fclose(input_file);
        fclose(key_file);
        fclose(message_file);
        return return_value;
    }
}