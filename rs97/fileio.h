#ifndef _FILEIO_H_
#define _FILEIO_H_

#include <stdint.h>

/* Global variables */
extern int32_t cart_size;
extern int8_t cart_name[0x100];

/* Function prototypes */
uint8_t *load_archive(int8_t *filename, int32_t *file_size);
int32_t load_cart(int8_t *filename);
int32_t check_zip(int8_t *filename);
int32_t gzsize(gzFile *gd);

#endif /* _FILEIO_H_ */
