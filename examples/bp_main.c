#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "bp_file.h"
#include "bp_datanode.h"
#include "bp_indexnode.h"
#include "record.h"

#define RECORDS_NUM 500
#define FILE_NAME "data.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

void insertEntries();
void findEntries();
Record randomRecord();
void printRecord(Record record);

int main() {
    srand(time(NULL));  // αρχικοποιούμε τυχαίους αριθμούς

    printf("=== Insert Entries ===\n");
    insertEntries();

    printf("\n=== Find Entries ===\n");
    findEntries();

    return 0;
}

void insertEntries() {
    BF_Init(LRU);
    if (BP_CreateFile(FILE_NAME) != bplus_OK) {
        fprintf(stderr, "Error creating B+ tree file.\n");
        return;
    }

    int file_desc;
    BPLUS_INFO *info = BP_OpenFile(FILE_NAME, &file_desc);
    if (!info) {
        fprintf(stderr, "Error opening B+ tree file.\n");
        return;
    }

    Record record;
    int success_count = 0;

    for (int i = 0; i < RECORDS_NUM; i++) {
        record = randomRecord();
        if (BP_InsertEntry(file_desc, info, record) == bplus_OK) {
            success_count++;
            printf("Inserted record with ID: %d\n", record.id);
        } else {
            fprintf(stderr, "Error inserting record with ID: %d\n", record.id);
        }
    }

    printf("Total records inserted: %d\n", success_count);

    if (BP_CloseFile(file_desc, info) != bplus_OK) {
        fprintf(stderr, "Error closing B+ tree file.\n");
    }

    if (BF_Close() != BF_OK) {
        fprintf(stderr, "Error closing BF manager.\n");
    }
}

void findEntries() {
    int file_desc;
    BPLUS_INFO *info;

    BF_Init(LRU);
    info = BP_OpenFile(FILE_NAME, &file_desc);

    if (!info) {
        fprintf(stderr, "Error opening B+ tree file.\n");
        BF_Close();
        return;
    }

    int id = 159;  // η εγγραφή που αναζητούμε
    Record *result = NULL;

    printf("Searching for: %d\n", id);
    if (BP_GetEntry(file_desc, info, id, &result) == bplus_OK && result != NULL) {
        printRecord(*result);
    } else {
        printf("Record with ID=%d not found.\n", id);
    }

    if (BP_CloseFile(file_desc, info) != bplus_OK) {
        fprintf(stderr, "Error closing B+ tree file.\n");
    }
    if (BF_Close() != BF_OK) {
        fprintf(stderr, "Error closing BF manager.\n");
    }
}

