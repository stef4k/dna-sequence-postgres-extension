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
#include "access/hash.h" // for HASH
#include "common/int.h"

#include "dna_sequence.h" // Take a look at dna_sequence.h for the data structures and other helper functions

PG_MODULE_MAGIC;

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
        if (toupper(str[i]) != 'A' && toupper(str[i]) != 'C'
            && toupper(str[i]) != 'G' && toupper(str[i]) != 'T') {
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
        char ch = toupper(str[i]);
        if (ch != 'A' && ch != 'C' && ch != 'G' && ch != 'T'
        && ch != 'W' && ch != 'S' && ch != 'M' && ch != 'K'
        && ch != 'R' && ch != 'Y' && ch != 'B' && ch != 'D'
        && ch != 'H' && ch != 'V' && ch != 'N') {
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
    for (int i = 0; i < length; i++) {
        result->data[i] = toupper(input[i]);
    }

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
    for (int i = 0; i < length; i++) {
        result->data[i] = toupper(input[i]);
    }

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
    for (int i = 0; i < length; i++) {
        result->data[i] = toupper(input[i]);
    }

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
        if (k <= 0 || k > dna_length || k > KMER_SIZE) {
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

    /* If prefix length is greater than kmer length, return false */
    if (prefix_len > kmer_len) {
        PG_RETURN_BOOL(false);
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
 * Canonical Function for kmer. Based on DNA theory canonical DNA is the
 lexicographically smaller representation of a DNA k-mer and its reverse
 complement (in other words alphabetically sorted, the one that comes first).
 The reverse complement is obtained by reversing the sequence and 
 replacing each kmer character with its complement: A ↔ T , C ↔ G
 For example, if GAT is the kmer, the reverse complement is ATC and the 
 canonical is also ATC since it is lexicographically smaller than GAT
 In detail this function:
 - iterates in reverse over the kmer with a for loop
 - finds the reverse complement of the k-mer
 - compares the k-mer and the reverse coomplement and returns the smaller one
 as a kmer type
 ******************************************************************************/

PG_FUNCTION_INFO_V1(canonical_kmer);
Datum canonical_kmer(PG_FUNCTION_ARGS) {
    Kmer *kmer = (Kmer *) PG_GETARG_POINTER(0);
    int32 len = VARSIZE(kmer) - VARHDRSZ;
    char *input = VARDATA(kmer);       // Gets the actual string data
    char *reverse_complement = palloc(len + VARHDRSZ);   // Allocate space for the reverse complement

    // iterates over the input string in reverse order 
    for (int i = 0; i < len; i++) {
        char ch = input[len - 1 - i];
        switch (ch) {
            // Maps each kmer character to its complement A<->T, C<->G
            case 'A': reverse_complement[VARHDRSZ + i] = 'T'; break;
            case 'T': reverse_complement[VARHDRSZ + i] = 'A'; break;
            case 'C': reverse_complement[VARHDRSZ + i] = 'G'; break;
            case 'G': reverse_complement[VARHDRSZ + i] = 'C'; break;
            default:
            // if there is a character other than 'A', 'T', 'C', or 'G', the function throws an error
                ereport(ERROR, 
                        (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                         errmsg("Invalid character '%c' in kmer string", ch)));
        }
    }

    // strcmp compares the kmer and the reverse_complement string lexicographically
    if (strcmp(input, reverse_complement + VARHDRSZ) <= 0) {
        // Case that original kmer comes alphabetically before reverse complement
        PG_RETURN_POINTER(kmer); // Returns the pointer to the original kmer
    } else {
        // Case that original kmer comes alphabetically after reverse complement
        Kmer *reverse_complement_result;
        reverse_complement_result = (Kmer *) palloc(VARHDRSZ + len);  // Memory Allocation to reverse_complement_result
        SET_VARSIZE(reverse_complement_result, VARHDRSZ + len); //Sets the total size of the variable-length object, including its header and data
        memcpy(reverse_complement_result->data, reverse_complement + VARHDRSZ, len); // Transfers the string into the kmer type reverse_complement_result
        pfree(reverse_complement); // Frees the memory allocated to the string
        PG_RETURN_POINTER(reverse_complement_result); // Returns the pointer to reverse_complement
    }

}
/******************************************************************************
 * Contains Function for qkmer and kmer
 ******************************************************************************/

// Added to fix the commuter issue
PG_FUNCTION_INFO_V1(contained_qkmer_kmer);
Datum contained_qkmer_kmer(PG_FUNCTION_ARGS) {
    Qkmer *pattern = (Qkmer *) PG_GETARG_POINTER(1);
    Kmer *kmer = (Kmer *) PG_GETARG_POINTER(0);

    int32 pattern_len = VARSIZE(pattern) - VARHDRSZ;
    int32 kmer_len = VARSIZE(kmer) - VARHDRSZ;

    if (pattern_len != kmer_len) {
        PG_RETURN_BOOL(false);
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

PG_FUNCTION_INFO_V1(contains_qkmer_kmer);
Datum contains_qkmer_kmer(PG_FUNCTION_ARGS) {
    Qkmer *pattern = (Qkmer *) PG_GETARG_POINTER(0);
    Kmer *kmer = (Kmer *) PG_GETARG_POINTER(1);

    int32 pattern_len = VARSIZE(pattern) - VARHDRSZ;
    int32 kmer_len = VARSIZE(kmer) - VARHDRSZ;

    if (pattern_len != kmer_len) {
        PG_RETURN_BOOL(false);
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

