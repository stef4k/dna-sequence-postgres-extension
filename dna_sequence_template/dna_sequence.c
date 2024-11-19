/*
 * dna_sequence.C 
 *
 * PostgreSQL DNA Type:
 *
 * dna_sequence '(a,b)'
 *
 * Authors: Kristóf Balázs, Stefanos Kypritidis, Otto Wantland, Nima Kamali Lassem
 */

#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"
#include "utils/fmgrprotos.h"
#include "utils/builtins.h"
#include "funcapi.h"  // SRF macros
#include "access/hash.h" // for HASH index implementation - REMOVE LATER
#include "access/spgist.h"  // Include this header for SP-GiST-related types and functions


bool is_valid_dna_string(const char *str);
bool is_valid_kmer_string(const char *str);
bool is_valid_qkmer_string(const char *str);

PG_MODULE_MAGIC;

#define KMER_SIZE 32  // Maximum length for kmer
#define QKMER_SIZE 32  // Define the fixed size for the Qkmer

/******************************************************************************
    Structure Definitions
 ******************************************************************************/

/* Structure to represent DNA sequences */
typedef struct {
    int32 length;
    char data[FLEXIBLE_ARRAY_MEMBER];
} Dna_sequence;

/* Structure to represent kmer sequences */
typedef struct {
    int32 length;
    char data[FLEXIBLE_ARRAY_MEMBER];
} Kmer;

/* Structure to represent qkmer sequences */
typedef struct {
    int32 length;
    char data[FLEXIBLE_ARRAY_MEMBER];
} Qkmer;

/* Structure for generate_kmers function context - refer to docs*/
typedef struct {
    char *dna_sequence;
    int dna_length;
    int k;
} generate_kmers_fctx;

/******************************************************************************
    Validation Functions
 ******************************************************************************/

/* Function to validate inputted DNA sequence,
 checks that string characters are one of A,C,G,T */
bool is_valid_dna_string(const char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] != 'A' && str[i] != 'C' && str[i] != 'G' && str[i] != 'T'
        && str[i] != 'a' && str[i] != 'c' && str[i] != 'g' && str[i] != 't') {
            return false;
        }
    }
    return true;
}

/*  Function to validate inputted kmer sequence,
    checks for maximum length of kmer string and
    checks that string characters are one of A,C,G,T */
bool is_valid_kmer_string(const char *str){
    int len = strlen(str);
    if (len > KMER_SIZE) {
        ereport(ERROR,
            (errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
            errmsg("Input exceeds maximum length of %d", KMER_SIZE)));
    }
    for (int i = 0; i < len; i++) {
        if (toupper(str[i]) != 'A' && toupper(str[i]) != 'C'
            && toupper(str[i]) != 'G' && toupper(str[i]) != 'T') {
            return false;
        }
    }
    return true;
}

/*  Function to validate inputted qkmer sequence,
    checks for maximum allowed length of qkmer string and
    checks that string characters are one of the standard nucleotides: A,C,G,T 
    or the ambiguity characters W,S,M,K,R,Y,B,D,H,V,N */
bool is_valid_qkmer_string(const char *str) {
    int len = strlen(str);
    if (len > QKMER_SIZE) {
        ereport(ERROR,
            (errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
            errmsg("Input exceeds maximum length of %d", QKMER_SIZE)));
    }
    for (int i = 0; i < len; i++) {
        if (toupper(str[i]) != 'A' && toupper(str[i]) != 'C' && toupper(str[i]) != 'G' && toupper(str[i]) != 'T'
        && toupper(str[i]) != 'W' && toupper(str[i]) != 'S' && toupper(str[i]) != 'M' && toupper(str[i]) != 'K'
        && toupper(str[i]) != 'R' && toupper(str[i]) != 'Y' && toupper(str[i]) != 'B' && toupper(str[i]) != 'D'
        && toupper(str[i]) != 'H' && toupper(str[i]) != 'V' && toupper(str[i]) != 'N') {
            return false;
        }
    }
    return true;
}

/******************************************************************************
    Input/Output Functions
 ******************************************************************************/

/* Input function dna sequence */
PG_FUNCTION_INFO_V1(dna_in);
Datum dna_in(PG_FUNCTION_ARGS) {
    char *input = PG_GETARG_CSTRING(0);
    int32 length;
    Dna_sequence *result;

    // Validate characters
    if (!is_valid_dna_string(input)) {
        ereport(ERROR, (errmsg("Invalid input: only 'A', 'C', 'G', 'T' characters are allowed")));
    }

    length = strlen(input);
    result = (Dna_sequence *) palloc(VARHDRSZ + length);  // Correct assignment
    SET_VARSIZE(result, VARHDRSZ + length);
    memcpy(result->data, input, length);

    PG_RETURN_POINTER(result);
}


/* Output function dna sequence*/
PG_FUNCTION_INFO_V1(dna_out);
Datum dna_out(PG_FUNCTION_ARGS) {
    Dna_sequence *input = (Dna_sequence *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(input) - VARHDRSZ;
    char *result = (char *) palloc(length + 1);

    memcpy(result, input->data, length);
    result[length] = '\0';

    PG_FREE_IF_COPY(input, 0);
    PG_RETURN_CSTRING(result);
}

/* Input function kmer sequence */
PG_FUNCTION_INFO_V1(kmer_in);
Datum kmer_in(PG_FUNCTION_ARGS) {
    char *input = PG_GETARG_CSTRING(0);
    int32 length;
    Kmer *result;

    // Validate the kmer string
    if (!is_valid_kmer_string(input)) {
        ereport(ERROR,
        (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
         errmsg("Invalid input: only 'A', 'C', 'G', 'T' characters are allowed and length must be <= %d", KMER_SIZE)));
    }

    length = strlen(input);
    result = (Kmer *) palloc(VARHDRSZ + length);  // Correct assignment
    SET_VARSIZE(result, VARHDRSZ + length);
    memcpy(result->data, input, length);

    PG_RETURN_POINTER(result);
}


/* Output function kmer sequence*/
PG_FUNCTION_INFO_V1(kmer_out);
Datum kmer_out(PG_FUNCTION_ARGS){
    Kmer *input = (Kmer *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(input) - VARHDRSZ;
    char *result = (char *) palloc(length + 1);

    memcpy(result, input->data, length);
    result[length] = '\0';

    PG_FREE_IF_COPY(input, 0);
    PG_RETURN_CSTRING(result);
}

/* Input function qkmer sequence */
PG_FUNCTION_INFO_V1(qkmer_in);
Datum qkmer_in(PG_FUNCTION_ARGS) {
    char *input = PG_GETARG_CSTRING(0);
    int32 length;
    Qkmer *result;  // Declare the pointer

    // Validate length
    if (strlen(input) > QKMER_SIZE) {
        ereport(ERROR, (errmsg("Input exceeds maximum length of %d", QKMER_SIZE)));
    }

    // Validate characters
    if (!is_valid_qkmer_string(input)) {
        ereport(ERROR, (errmsg("Invalid input: only 'A', 'B', 'C', 'D', 'G', "
        "'H', 'K', 'M', 'N', 'R', 'S', 'T', 'V', 'W', 'Y' characters are allowed")));
    }

    length = strlen(input);
    result = (Qkmer *) palloc(VARHDRSZ + length);  // Correct assignment
    SET_VARSIZE(result, VARHDRSZ + length);
    memcpy(result->data, input, length);

    PG_RETURN_POINTER(result);
}

/* Output function qkmer sequence*/
PG_FUNCTION_INFO_V1(qkmer_out);
Datum qkmer_out(PG_FUNCTION_ARGS) {
    Qkmer *input = (Qkmer *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(input) - VARHDRSZ;
    char *result = (char *) palloc(length + 1);

    memcpy(result, input->data, length);
    result[length] = '\0';

    PG_FREE_IF_COPY(input, 0);
    PG_RETURN_CSTRING(result);
}

/******************************************************************************
    Length Functions
 ******************************************************************************/

/* Length Function for dna sequence*/
PG_FUNCTION_INFO_V1(dna_sequence_length);
Datum dna_sequence_length(PG_FUNCTION_ARGS) {
    Dna_sequence *input = (Dna_sequence *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(input) - VARHDRSZ; // Get the length of the string
    PG_RETURN_INT32(length);
}

/* Length Function for kmer sequence*/
PG_FUNCTION_INFO_V1(kmer_length);
Datum kmer_length(PG_FUNCTION_ARGS) {
    Kmer *input = (Kmer *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(input) - VARHDRSZ; // Get the length of the string
    PG_RETURN_INT32(length);
}

/* Length Function for qkmer sequence*/
PG_FUNCTION_INFO_V1(qkmer_length);
Datum qkmer_length(PG_FUNCTION_ARGS) {
    Qkmer *input = (Qkmer *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(input) - VARHDRSZ; // Get the length of the string
    PG_RETURN_INT32(length);
}

/******************************************************************************
    Equaltiy Functions for kmer
 ******************************************************************************/

/* Equals Function for kmer */
PG_FUNCTION_INFO_V1(kmer_equals);
Datum kmer_equals(PG_FUNCTION_ARGS){
    Kmer *kmer1 = (Kmer *) PG_GETARG_POINTER(0);
    Kmer *kmer2 = (Kmer *) PG_GETARG_POINTER(1);

    int32 len1 = VARSIZE(kmer1) - VARHDRSZ;
    int32 len2 = VARSIZE(kmer2) - VARHDRSZ;

    if (len1 != len2) {
        PG_RETURN_BOOL(false);
    }

    // Compares the first num bytes of the block of memory pointed by ptr1 to the first  
    // num bytes pointed by ptr2, returning zero if they all match
    if (memcmp(kmer1->data, kmer2->data, len1) == 0) {
        PG_RETURN_BOOL(true);
    } else {
        PG_RETURN_BOOL(false);
    }
}

/* OPTIONAL: Not Equals Function for kmer */
PG_FUNCTION_INFO_V1(kmer_not_equals);
Datum kmer_not_equals(PG_FUNCTION_ARGS) {
    Kmer *kmer1 = (Kmer *) PG_GETARG_POINTER(0);
    Kmer *kmer2 = (Kmer *) PG_GETARG_POINTER(1);

    int32 len1 = VARSIZE(kmer1) - VARHDRSZ;
    int32 len2 = VARSIZE(kmer2) - VARHDRSZ;

    if (len1 != len2) {
        PG_RETURN_BOOL(true);
    }

    if (memcmp(kmer1->data, kmer2->data, len1) != 0) {
        PG_RETURN_BOOL(true);
    } else {
        PG_RETURN_BOOL(false);
    }
}

/* Cast Function from text to kmer (for comparisons like kmer =/<> 'ACGTA') */
PG_FUNCTION_INFO_V1(kmer_cast_text);
Datum kmer_cast_text(PG_FUNCTION_ARGS) {
    text *txt = PG_GETARG_TEXT_P(0);
    char *str = text_to_cstring(txt);
    int32 length;
    Kmer *result;

    // Validate the kmer string
    if (!is_valid_kmer_string(str)) {
        ereport(ERROR, (errmsg("Invalid input: only 'A', 'C', 'G', 'T' characters are allowed and length must be <= %d", KMER_SIZE)));
    }

    length = strlen(str);
    result = (Kmer *) palloc(VARHDRSZ + length);  // Correct assignment
    SET_VARSIZE(result, VARHDRSZ + length);
    memcpy(result->data, str, length);
    pfree(str);

    PG_RETURN_POINTER(result);
}

/******************************************************************************
 * Set-Returning Function: generate_kmers
 ******************************************************************************/

// Function option for returning sets: ValuePerCall mode
// -> Memory allocated in multi_call_memory_ctx (like fctx and fctx->dna_sequence) does not need to be manually freed
// -> it will be cleaned up automatically when the SRF is done.

// Comments in most places will correcpond to the documentation in the topic
// Refer to it here: https://www.postgresql.org/docs/current/xfunc-c.html#XFUNC-C-RETURN-SET

PG_FUNCTION_INFO_V1(generate_kmers);
Datum generate_kmers(PG_FUNCTION_ARGS) {
    FuncCallContext *funcctx;
    generate_kmers_fctx *fctx;

    /* stuff done only on the first call of the function */
    if (SRF_IS_FIRSTCALL()) {
        MemoryContext oldcontext;
        Dna_sequence *dna_input;
        int k;
        int dna_length;

        /* Create a function context for cross-call persistence */
        funcctx = SRF_FIRSTCALL_INIT();

        /* Switch to memory context (appropriate for multiple function calls) */
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        /* Extract function arguments in the form of: ('ACGTACGT', 6)*/
        dna_input = (Dna_sequence *) PG_GETARG_POINTER(0);
        k = PG_GETARG_INT32(1);

        /* Validate k */
        dna_length = VARSIZE(dna_input) - VARHDRSZ;
        //TODO: Ask if we need to account for the max kmer lenght, might be an edge case 
        if (k <= 0 || k > dna_length) {
            ereport(ERROR,
                (errmsg("Invalid k: must be between 1 and the length of the DNA sequence")));
        }

        /* Allocate memory for user context */
        fctx = (generate_kmers_fctx *) palloc(sizeof(generate_kmers_fctx));

        /* Copy DNA sequence into the user context */
        fctx->dna_sequence = (char *) palloc(dna_length + 1);
        memcpy(fctx->dna_sequence, dna_input->data, dna_length);
        fctx->dna_sequence[dna_length] = '\0';  // null-terminate
        fctx->dna_length = dna_length;
        fctx->k = k;

        /* Store the user context */
        funcctx->user_fctx = fctx;

        /* Total nr of k-mers */
        funcctx->max_calls = dna_length - k + 1;

        MemoryContextSwitchTo(oldcontext);
    }

     /* Stuff done on every call of the function */
     funcctx = SRF_PERCALL_SETUP();
     fctx = (generate_kmers_fctx *) funcctx->user_fctx;

     /* Check if there are more k-mers to return (also taken from docs) */
     if (funcctx->call_cntr < funcctx->max_calls) {
        int current_pos = funcctx->call_cntr;
        Datum result;
        Kmer *kmer_result;

        /* Get the next k-mer */
        char *kmer_str = (char *) palloc(fctx->k + 1);
        memcpy(kmer_str, fctx->dna_sequence + current_pos, fctx->k);
        kmer_str[fctx->k] = '\0';  // null-terminate

        /* Create a Kmer object */
        /* Explanation to Otto (only the first line below):
            1. palloc allocates memory from PSQL's memory context (PSQL will automatically clean up this memory when no longer needed) 
            2. (Kmer *) casts the pointer returned by palloc to a pointer of type Kmer (defined above in a struct) 
            3. VARHDRSZ: PSQL macro -> size of the variable-length data header (4 bytes)
            4. fctx->k: the length of the k-mer
            5. By adding the two, there will be enough space for both the header and the k-mer data (k bytes)
            */
        kmer_result = (Kmer *) palloc(VARHDRSZ + fctx->k);
        SET_VARSIZE(kmer_result, VARHDRSZ + fctx->k);
        memcpy(kmer_result->data, kmer_str, fctx->k);

        /* Convert to Datum */
        result = PointerGetDatum(kmer_result);

        /* Clean up temporary memory */
        pfree(kmer_str);

        /* Return the next k-mer */
        SRF_RETURN_NEXT(funcctx, result);
    }
    else
    {
        /* do when there is no more left */
        SRF_RETURN_DONE(funcctx);
    }
}

/******************************************************************************
 * Starts_with Function for kmer
 ******************************************************************************/

PG_FUNCTION_INFO_V1(kmer_starts_with);
Datum kmer_starts_with(PG_FUNCTION_ARGS) {
    Kmer *kmer = (Kmer *) PG_GETARG_POINTER(0);
    Kmer *prefix = (Kmer *) PG_GETARG_POINTER(1);

    int32 prefix_len = VARSIZE(prefix) - VARHDRSZ;
    int32 kmer_len = VARSIZE(kmer) - VARHDRSZ;

    /* Check if prefix length is greater than kmer length */
    if (prefix_len > kmer_len) {
        ereport(ERROR,
            (errcode (ERRCODE_INVALID_PARAMETER_VALUE),
            errmsg("Prefix length (%d) is greater than kmer length (%d)", prefix_len, kmer_len)));
    }

    /* Compare (prefix_len bytes of) kmer->data and prefix->data */
    // for reference for the others: https://www.tutorialspoint.com/c_standard_library/c_function_memcmp.htm
    if (memcmp(kmer->data, prefix->data, prefix_len) == 0) {
        PG_RETURN_BOOL(true);
    } else {
        PG_RETURN_BOOL(false);
    }
}

/******************************************************************************
 * Contains Function for qkmer and kmer
 ******************************************************************************/

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

PG_FUNCTION_INFO_V1(contains_qkmer_kmer);
Datum contains_qkmer_kmer(PG_FUNCTION_ARGS) {
    Qkmer *pattern = (Qkmer *) PG_GETARG_POINTER(0);
    Kmer *kmer = (Kmer *) PG_GETARG_POINTER(1);

    int32 pattern_len = VARSIZE(pattern) - VARHDRSZ;
    int32 kmer_len = VARSIZE(kmer) - VARHDRSZ;

    /* Check if lengths are equal */
    if (pattern_len != kmer_len) {
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
            errmsg("Pattern length (%d) does not match kmer length (%d)", pattern_len, kmer_len)));
    }

    /* For each position, compare according to IUPAC codes */
    for (int i =0; i < pattern_len; i++) {
        char qc = toupper(pattern->data[i]);
        char kc = toupper(kmer->data[i]);

        int q_bits = iupac_code_to_bits(qc);
        int k_bits = nucleotide_to_bits(kc);

        if ((q_bits & k_bits) == 0) {
            PG_RETURN_BOOL(false);
        }
    }

    PG_RETURN_BOOL(true);
}

/******************************************************************************
 * HASH index implementation
 ******************************************************************************/

/* Compute a hash value based on the contents of the Kmer data type */
PG_FUNCTION_INFO_V1(kmer_hash);
Datum kmer_hash(PG_FUNCTION_ARGS) {
    Kmer *kmer = (Kmer *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(kmer) - VARHDRSZ;

    /* Compute hash using PostgreSQL's hash_any function */
    uint32 hash = hash_any((unsigned char *) kmer->data, length);

    PG_RETURN_UINT32(hash);
}




/****************************************************************************
    SP-GiST Functions for dna_sequence Indexing
 ******************************************************************************/

/* Config Function for SP-GiST */
PG_FUNCTION_INFO_V1(spg_config);
Datum spg_config(PG_FUNCTION_ARGS) {
    spgConfigIn *cfgin = (spgConfigIn *) PG_GETARG_POINTER(0);
    spgConfigOut *cfgout = (spgConfigOut *) PG_GETARG_POINTER(1);

    cfgout->prefixType = VOIDOID;  // No prefix used
    cfgout->labelType = VOIDOID;   // No label used
    cfgout->leafType = cfgin->attType; // Leaf type matches the attribute type
    cfgout->canReturnData = true;  // Can reconstruct data from leaf values
    cfgout->longValuesOK = false;  // Long values are not supported

    PG_RETURN_VOID();
}

/* Choose Function for SP-GiST */
PG_FUNCTION_INFO_V1(spg_choose);
Datum spg_choose(PG_FUNCTION_ARGS) {
    spgChooseIn *in = (spgChooseIn *) PG_GETARG_POINTER(0);
    spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1);

    // For simplicity, let's assume all values go to the same node for now.
    out->resultType = spgMatchNode;  // This means we want to match an existing node
    out->result.matchNode.nodeN = 0; // Always choose node 0 for now
    out->result.matchNode.restDatum = in->leafDatum;

    PG_RETURN_VOID();
}

/* Picksplit Function for SP-GiST */
PG_FUNCTION_INFO_V1(spg_picksplit);
Datum spg_picksplit(PG_FUNCTION_ARGS) {
    spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
    spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);

    // Simple implementation: split data into two nodes
    out->nNodes = 2;
    out->hasPrefix = false;
    out->nodeLabels = NULL;

    // Divide tuples evenly between the two nodes
    out->mapTuplesToNodes = palloc(sizeof(int) * in->nTuples);
    for (int i = 0; i < in->nTuples; i++) {
        out->mapTuplesToNodes[i] = i % 2;  // Even index tuples to node 0, odd to node 1
    }

    PG_RETURN_VOID();
}

/* Inner Consistent Function for SP-GiST */
PG_FUNCTION_INFO_V1(spg_inner_consistent);
Datum spg_inner_consistent(PG_FUNCTION_ARGS) {
    spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0);
    spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1);

    out->nNodes = 1;  // Always visit one node for now
    out->nodeNumbers = palloc(sizeof(int));
    out->nodeNumbers[0] = 0;

    PG_RETURN_VOID();
}

/* Leaf Consistent Function for SP-GiST */
PG_FUNCTION_INFO_V1(spg_leaf_consistent);
Datum spg_leaf_consistent(PG_FUNCTION_ARGS) {
    spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0);
    spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1);

    Dna_sequence *leaf = (Dna_sequence *) DatumGetPointer(in->leafDatum);

    // For now, just return true for all matches
    out->recheck = false;
    PG_RETURN_BOOL(true);
}


/* Equals Function for dna_sequence */
PG_FUNCTION_INFO_V1(dna_sequence_equals);
Datum dna_sequence_equals(PG_FUNCTION_ARGS) {
    Dna_sequence *seq1 = (Dna_sequence *) PG_GETARG_POINTER(0);
    Dna_sequence *seq2 = (Dna_sequence *) PG_GETARG_POINTER(1);

    int32 len1 = VARSIZE(seq1) - VARHDRSZ;
    int32 len2 = VARSIZE(seq2) - VARHDRSZ;

    if (len1 != len2) {
        PG_RETURN_BOOL(false);
    }

    if (memcmp(seq1->data, seq2->data, len1) == 0) {
        PG_RETURN_BOOL(true);
    } else {
        PG_RETURN_BOOL(false);
    }
}

/******************************************************************************
 * HASH function for dna_sequence type
 ******************************************************************************/

/* Compute a hash value based on the contents of the dna_sequence data type */
PG_FUNCTION_INFO_V1(dna_sequence_hash);
Datum dna_sequence_hash(PG_FUNCTION_ARGS) {
    Dna_sequence *seq = (Dna_sequence *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(seq) - VARHDRSZ;

    /* Compute hash using PostgreSQL's hash_any function */
    uint32 hash = hash_any((unsigned char *) seq->data, length);

    PG_RETURN_UINT32(hash);
}


























