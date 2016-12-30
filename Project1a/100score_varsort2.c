#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<assert.h>
#include<ctype.h>
#include<string.h>
#include<libgen.h>
#include "sort.h"

void
usage(char *prog) {
  fprintf(stderr, "Usage: %s -i inputfile -o outputfile\n", prog);
  exit(1);
}
int
cmpfunc(const void *a, const void *b) {
  rec_dataptr_t *recA = (rec_dataptr_t *)a;
  rec_dataptr_t *recB = (rec_dataptr_t *)b;
  return (recA->key - recB->key);
}

int
main(int argc, char* argv[]) {
  // Arguments to be input
  char* inputfile = "";
  char* outputfile = "";
  int allinvalid = 1;
  int c;

  // Check if the number of arguement input is correct
  if (argc != 5) {
    usage(basename(argv[0]));
  }
  opterr = 0;
  // Read the command line arguements
  while ((c = getopt(argc, argv, "i:o:")) != -1) {
    allinvalid = 0;
    switch (c) {
      case 'i':
        inputfile = strdup(optarg);
        break;
      case 'o':
        outputfile = strdup(optarg);
        break;
      default :
        usage(basename(argv[0]));
    }
  }
  if (allinvalid) {
    usage(basename(argv[0]));
  }
  // Open the input file in the read mode
  int inFile = open(inputfile, O_RDONLY);
  if (inFile < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", inputfile);
    exit(1);
  }
  free(inputfile);

  // Open the output file for writing
  int outFile = open(outputfile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (outFile < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", outputfile);
    exit(1);
  }
  free(outputfile);

  int NumOfRecs = 0;
  int rc, wc;

  // Read the number of records in the inputfile
  rc = read(inFile, &NumOfRecs, sizeof(NumOfRecs));
  if (rc != sizeof(NumOfRecs)) {
    // REMOVE the file
    perror("read");
    exit(1);
  }

  // Write the number of records to the outputfile
  wc = write(outFile, &NumOfRecs, sizeof(NumOfRecs));
  if (wc != sizeof(NumOfRecs)) {
    // REMOVE the file
    perror("write");
    exit(1);
  }

  // Store the data read from inputfile to a structure
  rec_nodata_t in;
  unsigned int **data;
  data = (unsigned int **) malloc(NumOfRecs * sizeof(unsigned int *));
  int i;

  rec_dataptr_t *out;
  out = (rec_dataptr_t *) malloc(NumOfRecs * sizeof(rec_dataptr_t));
  rec_data_t outF;
  unsigned int *data_ptr_free;

  for (i=0; i < NumOfRecs; i++) {
    // Read the fixed portion of the record
    rc = read(inFile, &in, sizeof(rec_nodata_t));
    if (rc != sizeof(rec_nodata_t)) {
      perror("read");
      exit(1);
    }
    assert(in.data_ints <= MAX_DATA_INTS);

    // Read the variable portion of the record
    data[i] = (unsigned int *) malloc(in.data_ints * sizeof(unsigned int));
    if (data[i] == NULL) {
      perror("malloc");
      exit(1);
    }

    rc = read(inFile, data[i], in.data_ints * sizeof(unsigned int));
    if (rc !=  in.data_ints * sizeof(unsigned int)) {
      perror("read");
      exit(1);
    }
    // Write the records read to a structure of type rec_dataptr_t
    out[i].key = in.key;
    out[i].data_ints = in.data_ints;
    out[i].data_ptr = data[i];
  }
  // Sort the records using inbuilt quick sort
  qsort(out, NumOfRecs, sizeof(rec_dataptr_t), cmpfunc);

  // Write the sorted records into the output file
  for (i = 0; i < NumOfRecs; i++) {
    outF.key = out[i].key;
    outF.data_ints = out[i].data_ints;
    int j;
    data_ptr_free = out[i].data_ptr;
    for (j=0; j < outF.data_ints ; j++) {
         outF.data[j] = *(out[i].data_ptr)++;
    }
    int data_size = 2*sizeof(unsigned int) +
      outF.data_ints*sizeof(unsigned int);
    wc = write(outFile, &outF, data_size);
    if (wc != data_size) {
      perror("write");
      exit(1);
    }
    free(data_ptr_free);
  }
  free(data);
  free(out);
  (void) close(inFile);
  (void) close(outFile);
  return 0;
}

