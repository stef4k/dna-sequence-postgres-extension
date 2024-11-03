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

#define EPSILON         1.0E-06

#define FPzero(A)       (fabs(A) <= EPSILON)
#define FPeq(A,B)       (fabs((A) - (B)) <= EPSILON)
#define FPne(A,B)       (fabs((A) - (B)) > EPSILON)
#define FPlt(A,B)       ((B) - (A) > EPSILON)
#define FPle(A,B)       ((A) - (B) <= EPSILON)
#define FPgt(A,B)       ((A) - (B) > EPSILON)
#define FPge(A,B)       ((B) - (A) <= EPSILON)


/* Structure to represent DNA sequences */
typedef struct {
    int32 length;
    char data[FLEXIBLE_ARRAY_MEMBER];
} Dna_sequence;


/* Function to validate input, checks that string characters are one of A,C,G,T */
bool is_valid_dna_string(const char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] != 'A' && str[i] != 'C' && str[i] != 'G' && str[i] != 'T'
        && str[i] != 'a' && str[i] != 'c' && str[i] != 'g' && str[i] != 't') {
            return false;
        }
    }
    return true;
}

/* Input function */
PG_FUNCTION_INFO_V1(dna_in);
Datum dna_in(PG_FUNCTION_ARGS) {
    char *input = PG_GETARG_CSTRING(0);
    if (!is_valid_dna_string(input)) {
        ereport(ERROR, (errmsg("Invalid input: only 'A', 'C', 'G', 'T' characters are allowed")));
    }

    int32 length = strlen(input);
    Dna_sequence *result = (Dna_sequence *) palloc(VARHDRSZ + length);
    SET_VARSIZE(result, VARHDRSZ + length);
    memcpy(result->data, input, length);

    PG_RETURN_POINTER(result);
}

/* Output function */
PG_FUNCTION_INFO_V1(dna_out);
Datum dna_out(PG_FUNCTION_ARGS) {
    Dna_sequence *input = (Dna_sequence *) PG_GETARG_POINTER(0);
    int32 length = VARSIZE(input) - VARHDRSZ;
    char *result = (char *) palloc(length + 1);

    memcpy(result, input->data, length);
    result[length] = '\0';

    PG_RETURN_CSTRING(result);
}