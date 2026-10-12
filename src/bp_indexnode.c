#include <bp_indexnode.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static void BP_IndexNode_EnsureSizeFits() {
    assert(sizeof(BPLUS_INDEX_NODE) <= BF_BLOCK_SIZE &&
           "BPLUS_INDEX_NODE does not fit in a block");
}

void BP_IndexNode_Init(BPLUS_INDEX_NODE *node) {
    BP_IndexNode_EnsureSizeFits();
    if (node != NULL) {
        node->block_type = 1;
        node->num_pointers = 0;
        memset(node->pointers, -1, sizeof(node->pointers));
        memset(node->keys, 0, sizeof(node->keys));
    }
}

int BP_IndexNode_AddKeyPointer(BPLUS_INDEX_NODE *node, int key, int pointer, 
                               int *median_key, BPLUS_INDEX_NODE *new_node) {
    BP_IndexNode_EnsureSizeFits();

    if (node == NULL) {
        return -1;
    }

    if (node->num_pointers < 4) {
        int i;
        for (i = node->num_pointers - 2; i >= 0 && node->keys[i] > key; i--) {
            node->keys[i + 1] = node->keys[i];
            node->pointers[i + 2] = node->pointers[i + 1];
        }
        node->keys[i + 1] = key;
        node->pointers[i + 2] = pointer;
        node->num_pointers++;
        return 0;
    }

    if (new_node == NULL || median_key == NULL) {
        return -1;
    }

    BP_IndexNode_Split(node, median_key, new_node);

    if (key < *median_key) {
        BP_IndexNode_AddKeyPointer(node, key, pointer, NULL, NULL);
    } else {
        BP_IndexNode_AddKeyPointer(new_node, key, pointer, NULL, NULL);
    }

    return 1;
}

int BP_IndexNode_Split(BPLUS_INDEX_NODE *node, int *median_key, BPLUS_INDEX_NODE *new_node) {
    BP_IndexNode_EnsureSizeFits();

    if (node == NULL || new_node == NULL || node->num_pointers < 4) {
        return -1;
    }

    BP_IndexNode_Init(new_node);

    *median_key = node->keys[1];

    new_node->keys[0] = node->keys[2];
    new_node->pointers[0] = node->pointers[2];
    new_node->pointers[1] = node->pointers[3];
    new_node->num_pointers = 2;

    node->num_pointers = 2;

    return 0;
}

int BP_IndexNode_LoadFromBlock(BPLUS_INDEX_NODE *node, BF_Block *block) {
    BP_IndexNode_EnsureSizeFits();
    if (node == NULL || block == NULL) {
        return -1;
    }

    char *block_data = BF_Block_GetData(block);
    if (block_data == NULL) {
        return -1;
    }

    memcpy(node, block_data, sizeof(BPLUS_INDEX_NODE));
    return 0;
}

int BP_IndexNode_SaveToBlock(BPLUS_INDEX_NODE *node, BF_Block *block) {
    BP_IndexNode_EnsureSizeFits();
    if (node == NULL || block == NULL) {
        return -1;
    }

    char *block_data = BF_Block_GetData(block);
    if (block_data == NULL) {
        return -1;
    }

    memcpy(block_data, node, sizeof(BPLUS_INDEX_NODE));
    BF_Block_SetDirty(block);
    return 0;
}

void BP_IndexNode_Print(BPLUS_INDEX_NODE *node) {
    BP_IndexNode_EnsureSizeFits();
    if (node == NULL) {
        return;
    }

    printf("Index Node:\n");
    printf("Block Type: %d\n", node->block_type);
    printf("Num Pointers: %d\n", node->num_pointers);

    printf("Keys: ");
    for (int i = 0; i < node->num_pointers - 1; i++) {
        printf("%d ", node->keys[i]);
    }
    printf("\n");

    printf("Pointers: ");
    for (int i = 0; i < node->num_pointers; i++) {
        printf("%d ", node->pointers[i]);
    }
    printf("\n");
}