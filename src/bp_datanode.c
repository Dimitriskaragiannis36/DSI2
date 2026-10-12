#include "bf.h"
#include "bp_file.h"
#include "record.h"
#include <bp_datanode.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void BP_DataNode_Init(BPLUS_DATA_NODE *data_node) {
    if (data_node != NULL) {
        data_node->block_type = 0;
        data_node->num_records = 0;
        data_node->next_data_block = -1;
        memset(data_node->records, 0, sizeof(data_node->records));
    }
}

int BP_DataNode_InsertRecord(BPLUS_DATA_NODE *data_node, Record record) {
    if (data_node == NULL || data_node->num_records >= MAX_RECORDS_PER_BLOCK) {
        return -1;
    }

    int i = data_node->num_records - 1;
    while (i >= 0 && data_node->records[i].id > record.id) {
        data_node->records[i + 1] = data_node->records[i];
        i--;
    }
    
    data_node->records[i + 1] = record;
    data_node->num_records++;
    return 0;
}

Record *BP_DataNode_FindRecordById(BPLUS_DATA_NODE *data_node, int id) {
    if (data_node == NULL) {
        return NULL;
    }

    for (int i = 0; i < data_node->num_records; i++) {
        if (data_node->records[i].id == id) {
            return &data_node->records[i];
        }
    }

    return NULL;
}

Record *BP_DataNode_GetRecord(BPLUS_DATA_NODE *data_node, int index) {
    if (data_node == NULL || index < 0 || index >= data_node->num_records) {
        return NULL;
    }

    return &data_node->records[index];
}

int BP_DataNode_GetNumRecords(BPLUS_DATA_NODE *data_node) {
    if (data_node == NULL) {
        return -1;
    }

    return data_node->num_records;
}

void BP_DataNode_SetNextBlock(BPLUS_DATA_NODE *data_node, int next_block) {
    if (data_node != NULL) {
        data_node->next_data_block = next_block;
    }
}

int BP_DataNode_GetNextBlock(BPLUS_DATA_NODE *data_node) {
    if (data_node == NULL) {
        return -1;
    }

    return data_node->next_data_block;
}