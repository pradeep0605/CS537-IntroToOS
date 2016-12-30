/*        +================================================+
 *        ||        University of Wisconsin--Madison      ||
 *        +================================================+
 *
 *        CS537         : Intro to OS, first assignment  (prj1a).
 *        Author        : Pradeep Kashyap Ramaswamy
 *        Email         : pradeep.kashyap@wisc.edu
 *
 *        This file sorts 'R' records based on key 'K' each with data size 'N'.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "sort.h"

/* Typedefing to avoid typing full unsigned int */
typedef unsigned int uint;

typedef struct {
        uint R;           /* Number of records */
        uint size;        /* Size of the file */
        uint fd;          /* file descripor */
        char *name;       /* Name of the file */
} file_data_t;

typedef struct {
        void *base;                    /* base of one big chunk of memory */
        uint total_size;               /* Total size of memory allocated */
        rec_dataptr_t *rarray_base;    /* record array base, mostly = base */
        uint rarray_size;              /* size of rarray in bytes */
        void *data;                    /* pointer to the data of entire file */
        uint data_size;                /* size of file contents in bytes */
} mem_data_t;

/* This function calculates the total memory required for record array and
 * the file contents. Then it alocates one big buffer for everything.
 */
int
allocate_memory(file_data_t *file, mem_data_t *mem_data) {
        /* remove R's space which is not required */
        mem_data->data_size = file->size - sizeof(uint);
        if (read(file->fd, &file->R, sizeof(file->R)) == -1) {
                fprintf(stderr, "Error: Read() returned with -1 at %d!\n",
                        __LINE__);
                goto error;
        }

        /* size required for record array */
        mem_data->rarray_size = (file->R * sizeof(rec_dataptr_t));

        mem_data->total_size = mem_data->rarray_size + mem_data->data_size;

        if ((mem_data->base = malloc(mem_data->total_size)) == NULL) {
                goto error;
        }

        mem_data->rarray_base = (rec_dataptr_t *) mem_data->base;

        mem_data->data = mem_data->base + mem_data->rarray_size;

        return 0;
error:
        return -1;
}

void
deallocate_memory(mem_data_t *mem_data) {
        /* Just have to free the mem_data->base as every other pointer is just
         * pointing somewhere within base + total_size.
         */
         free(mem_data->base);
         mem_data->base = NULL;
         mem_data->rarray_base = NULL;
         mem_data->data = NULL;
}

int
populate_record_array(file_data_t *file, mem_data_t *mem_data) {
        int i = 0;
        int R = file->R;
        void *cur_rec_ptr;
        rec_nodata_t *record = NULL;
        /* read the entire file */
        if (read(file->fd, mem_data->data, mem_data->data_size) == -1) {
                fprintf(stderr, "Error: Read() returned with -1 at %d!\n",
                        __LINE__);
                goto error;
        }

        cur_rec_ptr = mem_data->data;
        for (i = 0; i < R; ++i) {
                record = (rec_nodata_t *) cur_rec_ptr;
                mem_data->rarray_base[i] =  *((rec_dataptr_t *) record);
                mem_data->rarray_base[i].data_ptr = cur_rec_ptr +
                        sizeof(*record);
                cur_rec_ptr += (sizeof(*record) + (record->data_ints *
                        sizeof(uint)));
        }

#ifdef VERBOSE
        printf("Number of records = %u\n", R);
        for (i = 0; i < R; ++i) {
                printf("Key = %d, N ints = %d\n", mem_data->rarray_base[i].key,
                        mem_data->rarray_base[i].data_ints);
        }
#endif /* VERBOSE */

        return 0;
error:
        return -1;
}


#define SWAP(a, b, tmp_swap) do { \
                        tmp_swap = a; \
                        a = b; \
                        b = tmp_swap; \
                } while(0)

void quick_sort(rec_dataptr_t *arr, int N)
{
        rec_dataptr_t tmp_swap;
        int pivot = arr[0].key;
        int i = 0, j = N;
        /* if there are only 1 or less elements, then nothing to sort */
        if (N < 2)
                return;
        /* Optimized code to handle arrays with only two elements */
        if (N == 2) {
                if (arr[0].key > arr[1].key) {
                        /* swapping two numbers would sort them */
                        SWAP(arr[0], arr[1], tmp_swap);
                }
                return;
        }

        while (i < j) {
                while (arr[++i].key < pivot && i < (N -  1));
                while (arr[--j].key > pivot && j > 0);

                if (i < j) {
                        SWAP(arr[i], arr[j], tmp_swap);
                }
        }

        SWAP(arr[0], arr[j], tmp_swap);
        /* Divide the array into two parts and quick sort them */
        quick_sort(&arr[0], j);
        quick_sort(&arr[j + 1], N - j - 1);
}

void
sort_records(mem_data_t *mem_data, uint R) {
        quick_sort(mem_data->rarray_base, R);
}

int
write_sorted_data(mem_data_t *mem_data, file_data_t *file) {
        uint i = 0, R = file->R;

        if ((file->fd = open(file->name, O_WRONLY | O_CREAT, S_IRWXU)) == -1) {
                fprintf(stderr, "Error: Cannot open file %s\n", file->name);
                goto error;
        }

        /* write the 'R'--Number of records */
        if (write(file->fd, &file->R, sizeof(file->R)) == -1) {
                fprintf(stderr, "Error in writing to output file at %d\n",
                        __LINE__);
                goto error;
        }

        /* Write each record to the output file */
        for (i = 0; i < R; ++i) {
                if (write(file->fd, &mem_data->rarray_base[i].key,
                        sizeof(mem_data->rarray_base[i].key)) == -1) {
                        fprintf(stderr, "Error in writing to output file: %d\n",
                                __LINE__);
                        goto error;
                }
                if (write(file->fd, &mem_data->rarray_base[i].data_ints,
                        sizeof(mem_data->rarray_base[i].data_ints)) == -1) {
                        fprintf(stderr, "Error in writing to output file: %d\n",
                                __LINE__);
                        goto error;
                }
                if (write(file->fd, mem_data->rarray_base[i].data_ptr,
                        mem_data->rarray_base[i].data_ints *
                        sizeof(uint)) == -1) {
                        fprintf(stderr, "Error in writing to output file: %d\n",
                                __LINE__);
                        goto error;
                }
        }
        return 0;
error:
        return -1;
}

void
print_usage() {
        fprintf(stderr, "Usage: varsort -i inputfile -o outputfile\n");
}

int
main(int arg_count, char *arg_vector[]) {
        file_data_t i_file, o_file;
        int option;
        struct stat stats;
        mem_data_t mem_data;
        i_file.name = NULL;
        o_file.name = NULL;
        /* If there are no arguments to the program, then print usage and
         * terminate with error.
         */
        if (arg_count < 5) {
                print_usage();
                /* Exit with error */
                goto exit;
        }

        while ((option = getopt(arg_count, arg_vector, "i:o:")) != -1) {
                switch (option) {
                        case 'i':
                                i_file.name = strdup(optarg);
                                break;
                        case 'o':
                                o_file.name = strdup(optarg);
                                break;
                        default:
                                print_usage();
                                /* Exit with error */
                        goto exit;
                }
        }
        if (i_file.name == NULL || o_file.name == NULL) {
                print_usage();
                goto exit;
        }

        if ((i_file.fd = open(i_file.name, O_RDONLY)) == -1) {
                fprintf(stderr, "Error: Cannot open file %s\n",
                        i_file.name);
                goto exit;
        }

        /* Fetch file stats */
        fstat(i_file.fd, &stats);
        i_file.size = (uint) stats.st_size;

#ifdef VERBOSE
        printf("Size of the file = %d\n", (int)i_file.size);
#endif /* VERBOSE */

        /* Allocate one chunk of memory of record array and the file contents */
        if (allocate_memory(&i_file, &mem_data) != 0) {
                fprintf(stderr, "Error: Malloc failure at %d\n", __LINE__);
                goto exit;
        }

        /* Read the file and populate the record array */
        if (populate_record_array(&i_file, &mem_data) != 0) {
                fprintf(stderr, "Error in poluating record array at %d\n",
                        __LINE__);
                goto exit;
        }

        /* Sort all the records based on the key values */
        sort_records(&mem_data, i_file.R);

        /* Write all the sorted data to the file */
        o_file.R = i_file.R;
        o_file.size = i_file.size;

        if (write_sorted_data(&mem_data, &o_file) != 0) {
                goto exit;
        }

        if (close(o_file.fd) == -1) {
                fprintf(stderr, "Error in closing input file at %d\n",
                        __LINE__);
                goto exit;
        }

        if (close(i_file.fd) == -1) {
                fprintf(stderr, "Error in closing input file at %d\n",
                        __LINE__);
                goto exit;
        }

        deallocate_memory(&mem_data);
        free(i_file.name); i_file.name = NULL;
        free(o_file.name); o_file.name = NULL;

        /* Everying's done gracefully ! */
        exit(0);
exit:
        /* Something went wrong ! exit with the error code 1 */
        exit(1);
}






