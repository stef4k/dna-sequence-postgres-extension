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

PG_MODULE_MAGIC;

#define QKMER_SIZE 32  // Define the fixed size for the Qkmer


/* Structure to represent DNA sequences */
typedef struct {
    int32 length;
    char data[FLEXIBLE_ARRAY_MEMBER];
} Dna_sequence;


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


/* Input function dna sequence */
PG_FUNCTION_INFO_V1(dna_in);
Datum dna_in(PG_FUNCTION_ARGS) {
    char *input = PG_GETARG_CSTRING(0);
    // Validate characters
    if (!is_valid_dna_string(input)) {
        ereport(ERROR, (errmsg("Invalid input: only 'A', 'C', 'G', 'T' characters are allowed")));
    }

    int32 length = strlen(input);
    Dna_sequence *result = (Dna_sequence *) palloc(VARHDRSZ + length);
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

    PG_RETURN_CSTRING(result);
}

/* Length Function for dna sequence*/
PG_FUNCTION_INFO_V1(dna_sequence_length);
Datum dna_sequence_length(PG_FUNCTION_ARGS) {
    Dna_sequence *input = (Dna_sequence *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(input) - VARHDRSZ; // Get the length of the string
    PG_RETURN_INT32(length);
}

/* Structure to represent qkmer sequences */
typedef struct {
    int32 length;
    char data[QKMER_SIZE];
} Qkmer;


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

/* Input function qkmer sequence */
PG_FUNCTION_INFO_V1(qkmer_in);
Datum qkmer_in(PG_FUNCTION_ARGS) {
    char *input = PG_GETARG_CSTRING(0);
    // Validate length
    if (strlen(input) > QKMER_SIZE) {
        ereport(ERROR, (errmsg("Input exceeds maximum length of %d", QKMER_SIZE)));
    }
    // Validate characters
    if (!is_valid_qkmer_string(input)) {
        ereport(ERROR, (errmsg("Invalid input: only 'A', 'B', 'C', 'D', 'G', "
        "'H', 'K', 'M', 'N', 'R', 'S', 'T', 'V', 'W', 'Y' characters are allowed")));
    }

    int32 length = strlen(input);
    Qkmer *result = (Qkmer *) palloc(VARHDRSZ + length);
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

    PG_RETURN_CSTRING(result);
}

/* Length Function for qkmer sequence*/
PG_FUNCTION_INFO_V1(qkmer_length);
Datum qkmer_length(PG_FUNCTION_ARGS) {
    Qkmer *input = (Qkmer *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(input) - VARHDRSZ; // Get the length of the string
    PG_RETURN_INT32(length);
}