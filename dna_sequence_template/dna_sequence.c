/*
 * dna_sequence.C 
 *
 * PostgreSQL DNA Type:
 *
 * dna_sequence '(a,b)'
 *
 * Author: Otto Wantland
 */

#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"
#include "utils/fmgrprotos.h"
#include "utils/builtins.h"

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

/* Function to validate inputted kmer sequence,
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

/* Function to validate inputted qkmer sequence,
 checks that string characters are one of the standard nucleotides: A,C,G,T 
 or the ambiguity characters W,S,M,K,R,Y,B,D,H,V,N */
bool is_valid_qkmer_string(const char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] != 'A' && str[i] != 'C' && str[i] != 'G' && str[i] != 'T'
        && str[i] != 'W' && str[i] != 'S' && str[i] != 'M' && str[i] != 'K'
        && str[i] != 'R' && str[i] != 'Y' && str[i] != 'B' && str[i] != 'D'
        && str[i] != 'H' && str[i] != 'V' && str[i] != 'N'
        && str[i] != 'a' && str[i] != 'c' && str[i] != 'g' && str[i] != 't'
        && str[i] != 'w' && str[i] != 's' && str[i] != 'm' && str[i] != 'k'
        && str[i] != 'r' && str[i] != 'y' && str[i] != 'b' && str[i] != 'd'
        && str[i] != 'h' && str[i] != 'v' && str[i] != 'n' ) {
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
