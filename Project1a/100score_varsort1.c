#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "sort.h"

void
usage() {
  fprintf(stderr, "Usage: varsort -i inputfile -o outputfile\n");
}

int comparator(const void *data1, const void *data2) {
  rec_dataptr_t *rec1 = (rec_dataptr_t *) data1;
  rec_dataptr_t *rec2 = (rec_dataptr_t *) data2;
  if (rec1->key > rec2->key) {
    return 1;
  } else {
    return 0;
  }
}

int
main(int argc, char *argv[]) {
  // arguments
  char *inFile = "/no/such/file";
  char *outFile = "/no/such/file";
  int ifile_valid = 0, ofile_valid = 0;
  int c;
  int i = 0;  // array elmnt no
  int l;

  if (argc != 5) {
    usage();
    exit(1);
  }
  opterr = 0;
  while ((c = getopt(argc, argv, "i:o:")) != -1) {
    switch (c) {
      case 'i':
        inFile = strdup(optarg);
        ifile_valid = 1;
        break;
      case 'o':
        outFile = strdup(optarg);
        ofile_valid = 1;
        break;
      default:
        usage(argv[0]);
    }
  }

  if (ifile_valid != 1 ||  ofile_valid != 1) {
    usage();
    exit(1);
  }

  //************** READ AND STORE INTO MEMORY STARTS **************
  // open input file
  int fd_r = open(inFile, O_RDONLY);
  if (fd_r < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", inFile);
    exit(1);
  }

  // output the number of keys as a header for this file
  unsigned int totalRecords;
  unsigned int recordsLeft;
  int rc;

  // Declare Data Structures for records storage
  rec_dataptr_t *rec_ptr_arr;

  rc = read(fd_r, &recordsLeft, sizeof(recordsLeft));
  if (rc != sizeof(recordsLeft)) {
    perror("read");
    exit(1);
  }
  rec_ptr_arr = malloc(recordsLeft*sizeof(rec_dataptr_t));

  totalRecords = recordsLeft;

  rec_nodata_t r;

  for (i = 0; i < recordsLeft; ++i) {
    // Read the fixed-sized portion of record: key and size of data
    rc = read(fd_r, &r, sizeof(rec_nodata_t));
    if (rc != sizeof(rec_nodata_t)) {
      perror("read");
      exit(1);
    }
    assert(r.data_ints <= MAX_DATA_INTS);

    // Store fixed-sized portion of record: key and size of data
    rec_ptr_arr[i].key = r.key;
    rec_ptr_arr[i].data_ints = r.data_ints;

    // Allocate mem for variable portion of data and store it
    rec_ptr_arr[i].data_ptr = malloc(r.data_ints * sizeof(unsigned int));

    // Read the variable portion of the record
    rc = read(fd_r, rec_ptr_arr[i].data_ptr, r.data_ints *
          sizeof(unsigned int));
    if (rc !=  r.data_ints * sizeof(unsigned int)) {
      perror("read");
      exit(1);
    }
  }
  (void) close(fd_r);

  //*************** READ AND STORE INTO MEMORY ENDS ***************
  //************************* SORT STARTS *************************

  qsort(rec_ptr_arr, totalRecords, sizeof(rec_dataptr_t), comparator);

  //************************** SORT ENDS **************************

  //******************** WRITE TO A FILE STARTS *******************

  // open and create output file
  int fd_w = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (fd_w < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", outFile);
    exit(1);
  }

  recordsLeft = totalRecords;
  rec_data_t rec_w;
  int m;

  // output the number of keys as a header for this file
  int wc = write(fd_w, &recordsLeft, sizeof(recordsLeft));
  if (wc != sizeof(recordsLeft)) {
    perror("write");
    exit(1);
  }

  for (m = 0; m < recordsLeft; m++) {
    // Write data to file
    rec_w.key = rec_ptr_arr[m].key;
    rec_w.data_ints = rec_ptr_arr[m].data_ints;
    int n;
    for (n = 0; n < rec_w.data_ints; n++) {
      rec_w.data[n] = rec_ptr_arr[m].data_ptr[n];
    }

    int data_size = sizeof(rec_nodata_t) + rec_w.data_ints*sizeof(unsigned int);
    wc = write(fd_w, &rec_w, data_size);

    if (wc != data_size) {
      perror("write");
      exit(1);
    }
  }

  for (l = 0; l < totalRecords; ++l) {
    free(rec_ptr_arr[l].data_ptr);
  }
  free(rec_ptr_arr);

  free(inFile);
  free(outFile);
  close(fd_w);

  //********************* WRITE TO A FILE ENDS ********************

  return 0;
}


