#ifndef BP_INDEXNODE_H
#define BP_INDEXNODE_H

#include <record.h>
#include <bf.h>
#include <bp_file.h>

/*
 * Δομή για έναν κόμβο ευρετηρίου στο B+ δέντρο.
 */
typedef struct {
    int block_type;                 // Τύπος μπλοκ (1 για κόμβο ευρετηρίου)
    int num_pointers;               // Αριθμός δεικτών (μέγιστο: 3)
    int pointers[4];                // Δείκτες προς παιδιά (μέγιστο: 4)
    int keys[3];                    // Τιμές κλειδιών (μέγιστο: 3)
} BPLUS_INDEX_NODE;

/* 
 * Αρχικοποιεί έναν νέο κόμβο ευρετηρίου.
 * Θέτει τις αρχικές τιμές στα πεδία της δομής.
 * @param node: Δομή κόμβου ευρετηρίου προς αρχικοποίηση.
 */
void BP_IndexNode_Init(BPLUS_INDEX_NODE *node);

/* 
 * Προσθέτει ένα κλειδί και έναν δείκτη σε έναν κόμβο ευρετηρίου.
 * Αν ο κόμβος είναι πλήρης, γίνεται split.
 * @param node: Ο κόμβος ευρετηρίου στον οποίο θα προστεθούν.
 * @param key: Το κλειδί που θα προστεθεί.
 * @param pointer: Ο δείκτης που θα προστεθεί.
 * @param median_key: Επιστρέφει το μεσαίο κλειδί σε περίπτωση split.
 * @param new_node: Επιστρέφει τον νέο κόμβο σε περίπτωση split.
 * @return: 0 αν η εισαγωγή ήταν επιτυχής, 1 αν έγινε split, -1 σε περίπτωση σφάλματος.
 */
int BP_IndexNode_AddKeyPointer(BPLUS_INDEX_NODE *node, int key, int pointer, 
                               int *median_key, BPLUS_INDEX_NODE *new_node);

/* 
 * Διαχωρίζει έναν κόμβο ευρετηρίου σε δύο νέους κόμβους.
 * @param node: Ο αρχικός κόμβος που θα διαχωριστεί.
 * @param median_key: Το μεσαίο κλειδί που θα ανέβει προς τα πάνω.
 * @param new_node: Ο νέος κόμβος που θα περιέχει τα δεδομένα του διαχωρισμού.
 * @return: 0 αν ο διαχωρισμός ήταν επιτυχής, -1 σε περίπτωση σφάλματος.
 */
int BP_IndexNode_Split(BPLUS_INDEX_NODE *node, int *median_key, BPLUS_INDEX_NODE *new_node);

/* 
 * Φορτώνει τα δεδομένα ενός κόμβου ευρετηρίου από ένα μπλοκ.
 * @param node: Ο κόμβος ευρετηρίου που θα φορτωθεί.
 * @param block: Το μπλοκ από το οποίο θα φορτωθούν τα δεδομένα.
 * @return: 0 αν ήταν επιτυχές, -1 σε περίπτωση σφάλματος.
 */
int BP_IndexNode_LoadFromBlock(BPLUS_INDEX_NODE *node, BF_Block *block);

/* 
 * Αποθηκεύει τα δεδομένα ενός κόμβου ευρετηρίου σε ένα μπλοκ.
 * @param node: Ο κόμβος ευρετηρίου που θα αποθηκευτεί.
 * @param block: Το μπλοκ στο οποίο θα αποθηκευτούν τα δεδομένα.
 * @return: 0 αν ήταν επιτυχές, -1 σε περίπτωση σφάλματος.
 */
int BP_IndexNode_SaveToBlock(BPLUS_INDEX_NODE *node, BF_Block *block);

/* 
 * Εκτυπώνει τα περιεχόμενα ενός κόμβου ευρετηρίου για debugging.
 * @param node: Ο κόμβος ευρετηρίου που θα εκτυπωθεί.
 */
void BP_IndexNode_Print(BPLUS_INDEX_NODE *node);

#endif // BP_INDEXNODE_H
