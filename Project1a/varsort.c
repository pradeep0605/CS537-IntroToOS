/*
 *  +================================================+
 *  ||        University of Wisconsin--Madison      ||
 *  +================================================+
 *
 *  CS537   : Intro to OS, first assignment  (prj1a).
 *  Author  : Pradeep Kashyap Ramaswamy
 *  Email   : pradeep.kashyap@wisc.edu
 *
 *  This file sorts 'R' records based on key 'K' each with size of 'N' ints.
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
  uint R;     /* Number of records */
  uint size;  /* Size of the file */
  uint fd;    /* file descripor */
  char *name; /* Name of the file */
} file_data_t;

typedef struct {
  void *base;                  /* base of one big chunk of memory */
  uint total_size;             /* Total size of memory allocated */
  rec_dataptr_t *rarray;       /* record array base. rarray  = base */
  uint rarray_size;            /* size of rarray in bytes */
  void *data;                  /* pointer to the data of entire file */
  uint data_size;              /* size of file contents in bytes */
} mem_data_t;

/* This function calculates the total memory required for record array and
 * the file contents. Then it alocates one big buffer for everything.
 */
int
allocate_memory(file_data_t *file, mem_data_t *mem_data) {
  /* remove R's space which is not required */
  mem_data->data_size = file->size - sizeof(uint);
  if (read(file->fd, &file->R, sizeof(file->R)) == -1) {
    fprintf(stderr, "Error: Read() returned with -1 at %d!\n", __LINE__);
    goto error;
  }

  /* size required for record array */
  mem_data->rarray_size = (file->R * sizeof(rec_dataptr_t));

  /* Total size to be malloced at once */
  mem_data->total_size = mem_data->rarray_size + mem_data->data_size;

  if ((mem_data->base = malloc(mem_data->total_size)) == NULL) {
    goto error;
  }

  /* Place the pointers accordingly */
  mem_data->rarray = (rec_dataptr_t *) mem_data->base;

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
  mem_data->rarray = NULL;
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
    fprintf(stderr, "Error: Read() returned with -1 at %d!\n", __LINE__);
    goto error;
  }

  cur_rec_ptr = mem_data->data;
  for (i = 0; i < R; ++i) {
    record = (rec_nodata_t *) cur_rec_ptr;
    /* Copy the record related information */
    mem_data->rarray[i] =  *((rec_dataptr_t *) record);
    /* Instead of data_ptr pointing to data insdie a record, it is made to point
     * to the record itself for efficiency while writing to disk.
     */
    mem_data->rarray[i].data_ptr = cur_rec_ptr;
    cur_rec_ptr += (sizeof(*record) + (record->data_ints * sizeof(uint)));
  }

#ifdef VERBOSE
  printf("Number of records = %u\n", R);
  for (i = 0; i < R; ++i) {
    printf("Key = %d, N ints = %d\n", mem_data->rarray[i].key,
      mem_data->rarray[i].data_ints);
  }
#endif /* VERBOSE */

  return 0;
error:
  return -1;
}

inline int
comparator(const void *data1, const void *data2) {
  return (*((uint *) data1) > *((uint *)data2));
}

void
sort_records(mem_data_t *mem_data, uint R) {
  qsort(mem_data->rarray, R, sizeof(rec_dataptr_t), comparator);
}

int
write_sorted_data(mem_data_t *mem_data, file_data_t *file) {
  uint i = 0, R = file->R;
  int size = 0;

  if ((file->fd = open(file->name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU)) == -1) {
    fprintf(stderr, "Error: Cannot open file %s\n", file->name);
    goto error;
  }

  /* write the 'R'--Number of records */
  if (write(file->fd, &file->R, sizeof(file->R)) == -1) {
    fprintf(stderr, "Error in writing to output file at %d\n", __LINE__);
    goto error;
  }

  /* Write each record to the output file */
  for (i = 0; i < R; ++i) {
      size = mem_data->rarray[i].data_ints * sizeof(uint) +
      sizeof(rec_nodata_t);
      if (write(file->fd, (void*) mem_data->rarray[i].data_ptr, size) != size) {
        fprintf(stderr, "%d Error in writing to output file: %d\n", i, __LINE__);
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
  /* If there are no 4 arguments to the program, then print usage and
   * terminate with error.
   */
  if (arg_count != 5) {
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
  /* After parsing the command line arguments, if two of the file names are not
   * present, then print usage and exit.
   */
  if (i_file.name == NULL || o_file.name == NULL) {
    print_usage();
    goto exit;
  }

  if ((i_file.fd = open(i_file.name, O_RDONLY)) == -1) {
    fprintf(stderr, "Error: Cannot open file %s\n", i_file.name);
    goto exit;
  }

  /* Fetch file stats and save it*/
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
    fprintf(stderr, "Error in poluating record array at %d\n", __LINE__);
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
    fprintf(stderr, "Error in closing input file at %d\n", __LINE__);
    goto exit;
  }

  if (close(i_file.fd) == -1) {
    fprintf(stderr, "Error in closing input file at %d\n", __LINE__);
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






