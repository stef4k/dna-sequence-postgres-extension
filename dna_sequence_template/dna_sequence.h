#ifndef DNA_SEQUENCE_H
#define DNA_SEQUENCE_H

#include "postgres.h"
#include "fmgr.h"
#include "access/hash.h"

/* Maximum lengths */
#define KMER_SIZE 32
#define QKMER_SIZE 32

/******************************************************************************
    Structure Definitions
 ******************************************************************************/

/* Structure Definitions */
typedef struct {
    int32 length;
    char data[];
} Dna_sequence;

typedef struct {
    int32 length;
    char data[];
} Kmer;

typedef struct {
    int32 length;
    char data[];
} Qkmer;

/* Validation Functions */
bool is_valid_dna_string(const char *str);
bool is_valid_kmer_string(const char *str);
bool is_valid_qkmer_string(const char *str);

/* Shared Helper Functions */
/* Helper functions */

static int nucleotide_to_bits(char c){
    switch (c) {
        case 'A': return 1; // 0001
        case 'C': return 2; // 0010
        case 'G': return 4; // 0100
        case 'T': return 8; // 1000
        default: 
            ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                errmsg("Invalid nucleotide character '%c' in kmer", c)));
            return 0; // Should never reach here                
    }
}

static int iupac_code_to_bits(char c) {
    switch (c) {
        case 'A': return 1; // A
        case 'C': return 2; // C
        case 'G': return 4; // G
        case 'T': return 8; // T
        case 'R': return 1|4; // A or G
        case 'Y': return 2|8; // C or T
        case 'S': return 2|4; // G or C
        case 'W': return 1|8; // A or T
        case 'K': return 4|8; // G or T
        case 'M': return 1|2; // A or C
        case 'B': return 2|4|8; // C or G or T
        case 'D': return 1|4|8; // A or G or T
        case 'H': return 1|2|8; // A or C or T
        case 'V': return 1|2|4; // A or C or G
        case 'N': return 1|2|4|8; // A or C or G or T (any)
        default:
            ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                errmsg("Invalid IUPAC code character '%c' in qkmer", c)));
            return 0; // Should also never reach here
    }
}

#endif /* DNA_SEQUENCE_H */
