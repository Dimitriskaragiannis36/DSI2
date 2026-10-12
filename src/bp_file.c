#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "bf.h"
#include "bp_file.h"
#include "record.h"
#include "bp_datanode.h"
#include "bp_indexnode.h"

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return bplus_ERROR;     \
    }                         \
  }

#define BPLUS_METADATA_BLOCK 0

/*
 * Δημιουργία αρχείου B+ δέντρου και αρχικοποίηση των μεταδεδομένων του.
 */
int BP_CreateFile(char *fileName) {
    CALL_BF(BF_CreateFile(fileName));

    int file_desc;
    CALL_BF(BF_OpenFile(fileName, &file_desc));

    BF_Block *block;
    BF_Block_Init(&block);
    CALL_BF(BF_AllocateBlock(file_desc, block));

    char *data = BF_Block_GetData(block);
    memset(data, 0, BF_BLOCK_SIZE);

    BPLUS_INFO metadata = {
        .root_block = -1,
        .height = 0,
        .num_blocks = 1,
        .data_block_start = -1,
        .index_block_start = -1,
        .block_type = 0,
        .record_size = sizeof(Record),
        .num_records_per_block = MAX_RECORDS_PER_BLOCK,
        .num_keys_per_index_block = 3,
        .is_bplus_file = true
    };

    memcpy(data, &metadata, sizeof(BPLUS_INFO));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    CALL_BF(BF_CloseFile(file_desc));
    return bplus_OK;
}

/*
 * Άνοιγμα αρχείου B+ δέντρου.
 */
BPLUS_INFO *BP_OpenFile(char *fileName, int *file_desc) {
    if (BF_OpenFile(fileName, file_desc) != BF_OK) {
        return NULL;
    }

    BPLUS_INFO *info = malloc(sizeof(BPLUS_INFO));
    if (!info) {
        BF_CloseFile(*file_desc);
        return NULL;
    }

    BF_Block *block;
    BF_Block_Init(&block);

    if (BF_GetBlock(*file_desc, BPLUS_METADATA_BLOCK, block) != BF_OK) {
        free(info);
        BF_CloseFile(*file_desc);
        return NULL;
    }

    char *data = BF_Block_GetData(block);
    memcpy(info, data, sizeof(BPLUS_INFO));
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);

    return info;
}

/*
 * Κλείσιμο αρχείου B+ δέντρου με αποθήκευση των ενημερωμένων μεταδεδομένων.
 */
int BP_CloseFile(int file_desc, BPLUS_INFO *info) {
    BF_Block *block;
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(file_desc, BPLUS_METADATA_BLOCK, block));

    char *data = BF_Block_GetData(block);
    memcpy(data, info, sizeof(BPLUS_INFO));

    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    free(info);
    CALL_BF(BF_CloseFile(file_desc));
    return bplus_OK;
}

/*
 * Εισαγωγή εγγραφής με διάσχιση και διαχείριση διασπάσεων (Splits).
 */
int BP_InsertEntry(int file_desc, BPLUS_INFO *bplus_info, Record record) {
    BF_Block *block;
    BF_Block_Init(&block);

    // Διαχείριση εντελώς άδειου δέντρου
    if (bplus_info->root_block == -1) {
        CALL_BF(BF_AllocateBlock(file_desc, block));
        bplus_info->root_block = bplus_info->num_blocks++;
        
        BPLUS_DATA_NODE data_node;
        BP_DataNode_Init(&data_node);
        BP_DataNode_InsertRecord(&data_node, record);

        memcpy(BF_Block_GetData(block), &data_node, sizeof(BPLUS_DATA_NODE));
        BF_Block_SetDirty(block);
        CALL_BF(BF_UnpinBlock(block));
        BF_Block_Destroy(&block);

        return bplus_OK;
    }

    int current_block = bplus_info->root_block;
    int path[256]; // Επαυξημένος πίνακας για πλήρη προστασία από overflow
    int path_len = 0;

    // Φάση 1: Διάσχιση (Traversal) του Δέντρου μέχρι το επίπεδο φύλλων
    while (true) {
        CALL_BF(BF_GetBlock(file_desc, current_block, block));
        char *data = BF_Block_GetData(block);
        int block_type = ((BPLUS_DATA_NODE *)data)->block_type;

        if (block_type == 1) { // Κόμβος Ευρετηρίου
            BPLUS_INDEX_NODE *index_node = (BPLUS_INDEX_NODE *)data;
            int i;
            for (i = 0; i < index_node->num_pointers - 1; i++) {
                if (record.id < index_node->keys[i]) break;
            }
            int next_block = index_node->pointers[i];
            
            path[path_len++] = current_block;
            CALL_BF(BF_UnpinBlock(block));
            current_block = next_block;
        } else {
            break; // Κατάληξη στον σωστό κόμβο δεδομένων
        }
    }

    BPLUS_DATA_NODE *data_node = (BPLUS_DATA_NODE *)BF_Block_GetData(block);

    // Προστασία από διπλότυπα
    if (BP_DataNode_FindRecordById(data_node, record.id) != NULL) {
        fprintf(stderr, "Σφάλμα: Η εγγραφή με id %d υπάρχει ήδη.\n", record.id);
        CALL_BF(BF_UnpinBlock(block));
        BF_Block_Destroy(&block);
        return bplus_ERROR;
    }

    // Περίπτωση: Επαρκής χώρος στο τρέχον Data Block
    if (BP_DataNode_InsertRecord(data_node, record) == 0) {
        BF_Block_SetDirty(block);
        CALL_BF(BF_UnpinBlock(block));
        BF_Block_Destroy(&block);
        return bplus_OK;
    }

    // Φάση 2: Διάσπαση Κόμβου Δεδομένων (Data Node Split)
    BPLUS_DATA_NODE new_data_node;
    BP_DataNode_Init(&new_data_node);

    int mid = data_node->num_records / 2;
    memcpy(new_data_node.records, &data_node->records[mid], sizeof(Record) * (data_node->num_records - mid));
    new_data_node.num_records = data_node->num_records - mid;
    data_node->num_records = mid;

    // Δρομολόγηση της νέας εγγραφής
    if (record.id < new_data_node.records[0].id) {
        BP_DataNode_InsertRecord(data_node, record);
    } else {
        BP_DataNode_InsertRecord(&new_data_node, record);
    }

    int new_block_num = bplus_info->num_blocks++;
    new_data_node.next_data_block = data_node->next_data_block;
    data_node->next_data_block = new_block_num;

    BF_Block *new_block;
    BF_Block_Init(&new_block);
    CALL_BF(BF_AllocateBlock(file_desc, new_block));
    memcpy(BF_Block_GetData(new_block), &new_data_node, sizeof(BPLUS_DATA_NODE));
    BF_Block_SetDirty(new_block);
    CALL_BF(BF_UnpinBlock(new_block));
    BF_Block_Destroy(&new_block);

    int key_to_insert = new_data_node.records[0].id;
    int right_block = new_block_num;

    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    // Φάση 3: Προώθηση της διάσπασης προς τα πάνω (Index Nodes Propagation)
    while (path_len > 0) {
        int parent_block = path[--path_len];
        BF_Block *p_block;
        BF_Block_Init(&p_block);
        CALL_BF(BF_GetBlock(file_desc, parent_block, p_block));
        
        BPLUS_INDEX_NODE *p_node = (BPLUS_INDEX_NODE *)BF_Block_GetData(p_block);
        BPLUS_INDEX_NODE new_index_node;
        int median_key;

        int res = BP_IndexNode_AddKeyPointer(p_node, key_to_insert, right_block, &median_key, &new_index_node);
        
        if (res == 0) {
            BF_Block_SetDirty(p_block);
            CALL_BF(BF_UnpinBlock(p_block));
            BF_Block_Destroy(&p_block);
            return bplus_OK; // Η προώθηση απορροφήθηκε
        } else {
            int new_idx_block_num = bplus_info->num_blocks++;
            BF_Block *new_idx_block;
            BF_Block_Init(&new_idx_block);
            CALL_BF(BF_AllocateBlock(file_desc, new_idx_block));
            memcpy(BF_Block_GetData(new_idx_block), &new_index_node, sizeof(BPLUS_INDEX_NODE));
            BF_Block_SetDirty(new_idx_block);
            CALL_BF(BF_UnpinBlock(new_idx_block));
            BF_Block_Destroy(&new_idx_block);

            key_to_insert = median_key;
            right_block = new_idx_block_num;

            BF_Block_SetDirty(p_block);
            CALL_BF(BF_UnpinBlock(p_block));
            BF_Block_Destroy(&p_block);
        }
    }

    // Φάση 4: Δημιουργία Νέας Ρίζας Δέντρου (Αν η διάσπαση έφτασε στην κορυφή)
    BF_Block *root_block;
    BF_Block_Init(&root_block);
    CALL_BF(BF_AllocateBlock(file_desc, root_block));
    
    BPLUS_INDEX_NODE new_root;
    BP_IndexNode_Init(&new_root);
    new_root.keys[0] = key_to_insert;
    new_root.pointers[0] = bplus_info->root_block;
    new_root.pointers[1] = right_block;
    new_root.num_pointers = 2;

    memcpy(BF_Block_GetData(root_block), &new_root, sizeof(BPLUS_INDEX_NODE));
    BF_Block_SetDirty(root_block);
    CALL_BF(BF_UnpinBlock(root_block));
    BF_Block_Destroy(&root_block);

    // ΚΡΙΤΙΚΗ ΔΙΟΡΘΩΣΗ: Πλήρης συγχρονισμός της νέας ρίζας με τα διαθέσιμα μπλοκ.
    bplus_info->root_block = bplus_info->num_blocks++; 
    
    return bplus_OK;
}

/*
 * Εύρεση εγγραφής με χρήση του Β+-Δέντρου.
 */
int BP_GetEntry(int file_desc, BPLUS_INFO *bplus_info, int id, Record **record) {
    BF_Block *block;
    BF_Block_Init(&block);

    int current_block = bplus_info->root_block;

    while (current_block != -1) {
        CALL_BF(BF_GetBlock(file_desc, current_block, block));
        char *data = BF_Block_GetData(block);
        int block_type = ((BPLUS_DATA_NODE *)data)->block_type;

        if (block_type == 0) { // Φτάσαμε στο Data Node
            BPLUS_DATA_NODE *data_node = (BPLUS_DATA_NODE *)data;
            Record *found = BP_DataNode_FindRecordById(data_node, id);
            
            if (found) {
                *record = malloc(sizeof(Record));
                memcpy(*record, found, sizeof(Record));
                CALL_BF(BF_UnpinBlock(block));
                BF_Block_Destroy(&block);
                return bplus_OK;
            }
            
            CALL_BF(BF_UnpinBlock(block));
            BF_Block_Destroy(&block);
            return bplus_ERROR;
        } else { // Αναζήτηση μέσω των Index Nodes
            BPLUS_INDEX_NODE *index_node = (BPLUS_INDEX_NODE *)data;
            int i;
            for (i = 0; i < index_node->num_pointers - 1; i++) {
                if (id < index_node->keys[i]) break;
            }
            current_block = index_node->pointers[i];
            CALL_BF(BF_UnpinBlock(block));
        }
    }

    BF_Block_Destroy(&block);
    return bplus_ERROR;
}