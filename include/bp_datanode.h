#ifndef BP_DATANODE_H
#define BP_DATANODE_H

#include <record.h>
#include <bf.h>
#include <bp_file.h>
#include <bp_indexnode.h>

//ο μέγιστος αριθμός εγγραφών ανά μπλοκ που βρήκαμε
#define MAX_RECORDS_PER_BLOCK 8

// ορίζουμε την δομήν για ένα μπλοκ δεδομένων
typedef struct {
    int block_type;                 // Τύπος του μπλοκ (0: Data Block)
    int num_records;                // Αριθμός εγγραφών στο μπλοκ
    int next_data_block;            // Δείκτης στο επόμενο μπλοκ δεδομένων (ή -1 αν δεν υπάρχει επόμενο)
    Record records[MAX_RECORDS_PER_BLOCK];  // Στατικός πίνακας εγγραφών
} BPLUS_DATA_NODE;

/*
 * Αρχικοποιεί ένα μπλοκ δεδομένων, ορίζοντας αρχικές τιμές.
 * @param data_node: Δομή μπλοκ δεδομένων προς αρχικοποίηση.
 */
void BP_DataNode_Init(BPLUS_DATA_NODE *data_node);

/*
 * Εισάγει μια εγγραφή στο μπλοκ δεδομένων.
 * @param data_node: Το μπλοκ δεδομένων στο οποίο θα γίνει εισαγωγή.
 * @param record: Η εγγραφή που θα εισαχθεί.
 * @return: 0 αν η εισαγωγή επιτύχει, -1 αν το μπλοκ είναι πλήρες.
 */
int BP_DataNode_InsertRecord(BPLUS_DATA_NODE *data_node, Record record);

/*
 * Αναζητά μια εγγραφή με βάση το id της.
 * @param data_node: Το μπλοκ δεδομένων.
 * @param id: Το id της εγγραφής που αναζητείται.
 * @return: Δείκτης στην εγγραφή αν βρεθεί, NULL αν δεν υπάρχει.
 */
Record *BP_DataNode_FindRecordById(BPLUS_DATA_NODE *data_node, int id);

/*
 * Επιστρέφει την εγγραφή με βάση τον δείκτη.
 * @param data_node: Το μπλοκ δεδομένων.
 * @param index: Η θέση της εγγραφής στον πίνακα.
 * @return: Δείκτης στην εγγραφή στη θέση index ή NULL αν η θέση είναι εκτός ορίων.
 */
Record *BP_DataNode_GetRecord(BPLUS_DATA_NODE *data_node, int index);

/*
 * Επιστρέφει τον αριθμό εγγραφών στο μπλοκ δεδομένων.
 * @param data_node: Το μπλοκ δεδομένων.
 * @return: Ο αριθμός των εγγραφών.
 */
int BP_DataNode_GetNumRecords(BPLUS_DATA_NODE *data_node);

/*
 * Θέτει το δείκτη στο επόμενο μπλοκ δεδομένων.
 * @param data_node: Το μπλοκ δεδομένων.
 * @param next_block: Ο αριθμός του επόμενου μπλοκ (ή -1 αν δεν υπάρχει επόμενο).
 */
void BP_DataNode_SetNextBlock(BPLUS_DATA_NODE *data_node, int next_block);

/*
 * Επιστρέφει το δείκτη στο επόμενο μπλοκ δεδομένων.
 * @param data_node: Το μπλοκ δεδομένων.
 * @return: Ο αριθμός του επόμενου μπλοκ (ή -1 αν δεν υπάρχει επόμενο).
 */
int BP_DataNode_GetNextBlock(BPLUS_DATA_NODE *data_node);

#endif
