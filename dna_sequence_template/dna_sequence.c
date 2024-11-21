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

#include "access/spgist.h" // for SP-GiST
#include "catalog/pg_type.h" // for SP-GiST
#include "utils/datum.h" // for SP-GiST
#include "utils/pg_locale.h" // for SP-GiST

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


/******************************************************************************
 * SP-GiST index implementation
 ******************************************************************************/


/* spgkmerproc.c
 *
 * Implementation of radix tree (trie) over kmer
 * 
 * In a kmer_ops SPGiST index, inner tuples can have a prefix which is the
 * common prefix of all kmers indexed under that tuple. The node labels
 * represent the next character of the kmer(s) after the prefix.
 *
 * To reconstruct the indexed kmer for any index entry, concatenate the
 * inner-tuple prefixes and node labels starting at the root and working
 * down to the leaf entry, then append the datum in the leaf entry.
 *
 */

PG_FUNCTION_INFO_V1(spg_kmer_config);
Datum
spg_kmer_config(PG_FUNCTION_ARGS) {
    spgConfigIn *cfgin = (spgConfigIn *) PG_GETARG_POINTER(0);
    spgConfigOut *cfg = (spgConfigOut *) PG_GETARG_POINTER(1);

    cfg->prefixType = TEXTOID;
    cfg->labelType = CHAROID;
    cfg->leafType = cfgin->attType;
    cfg->canReturnData = true; /* allow for reconstructon of original kmers */
    cfg->longValuesOK = false; /* kmers have limited length of 32 */
    /* picksplit can be applied to a single leaf tuple only in the case that the config function 
     * set longValuesOK to true and a larger-than-a-page input value has been supplied. */

    PG_RETURN_VOID();
}

/* Find common prefix length (of two strings) */
static int
commonPrefix(const char *a, const char *b, int lena, int lenb) {
    int i = 0;

    while (i < lena && i < lenb && a[i] == b[i])
        i++;

    return i;
}

/* Search for a character c' in an array of node labels. (using binary search) */
static bool
searchChar(Datum *nodeLabels, int nNodes, char c, int *i)
{
    int low = 0, high = nNodes;

    while (low < high)
    {
        int mid = (low + high) / 2;
        char middle = DatumGetChar(nodeLabels[mid]);

        if (c < middle)
            high = mid;
        else if (c > middle)
            low = mid + 1;
        else    /* c == middle */ 
        {
            *i = mid;
            return true;
        }
    }

    *i = high;
    return false;
}

/* The choose function can determine either that the new value matches one of the
existing child nodes, or that a new child node must be added, or that the new value is inconsistent with the
tuple prefix and so the inner tuple must be split to create a less restrictive prefix.
Source: https://www.postgresql.org/docs/current/spgist.html */

/* Determines the action to take when inserting a kmer.
If the prefix matches:
Continues down the tree by matching the next character.
If the prefix diverges:
Splits the current node to accommodate the new kmer.
Creates new nodes or adjusts prefixes.
Handles all-the-same nodes:
If all entries are the same, it may need to split to maintain the tree structure.*/

PG_FUNCTION_INFO_V1(spg_kmer_choose);
Datum
spg_kmer_choose(PG_FUNCTION_ARGS)
{
    spgChooseIn *in = (spgChooseIn *) PG_GETARG_POINTER(0); // The first argument is a pointer to a spgChooseIn C struct, containing input data for the function.
    spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1); // The second argument is a pointer to a spgChooseOut C struct, which the function must fill with result data.
    
    // See the struct documentation for this and all the following functions here: https://www.postgresql.org/docs/current/spgist.html
    
    text *inText = DatumGetTextPP(in->leafDatum);
    char *inStr = VARDATA_ANY(inText);
    int inSize = VARSIZE_ANY_EXHDR(inText);
    text *prefixText = NULL;
    char *prefixStr = NULL;
    int prefixSize = 0;
    int commonLen = 0;
    char nodeChar = '\0';
    int i;
    int offset;

    /* Check for prefix match, set nodeChar to first byte after prefix */
    if (in->hasPrefix)
    {
        prefixText = DatumGetTextPP(in->prefixDatum);
        prefixStr = VARDATA_ANY(prefixText);
        prefixSize = VARSIZE_ANY_EXHDR(prefixText);

        commonLen = commonPrefix(inStr, prefixStr, inSize, prefixSize);

        if (commonLen == prefixSize)
        {
            /* Entire prefix matches, determine next character */
            if (inSize > commonLen)
                nodeChar = inStr[commonLen];
            else
                nodeChar = '\0';
        }
        else
        {
            /* Must split tuple because incoming value doesn't match prefix */
            out->resultType = spgSplitTuple; // If the new value is inconsistent with the tuple prefix, set resultType to spgSplitTuple.

            /* Set up the new prefix */
            if (commonLen > 0)
            {
                out->result.splitTuple.prefixHasPrefix = true;
                out->result.splitTuple.prefixPrefixDatum =
                    PointerGetDatum(cstring_to_text_with_len(prefixStr, commonLen));
            }
            else
            {
                out->result.splitTuple.prefixHasPrefix = false;
            }

            /* Set up node labels for the split */
            out->result.splitTuple.prefixNNodes = 2;
            out->result.splitTuple.prefixNodeLabels =
                (Datum *) palloc(sizeof(Datum) * 2);
            out->result.splitTuple.prefixNodeLabels[0] = CharGetDatum(prefixStr[commonLen]);
            out->result.splitTuple.prefixNodeLabels[1] = CharGetDatum(inStr[commonLen]);

             /* Indicate which node to descend into. */
            out->result.splitTuple.childNodeN = 1; // The new value goes to the second node

            /* Set up postfix prefix for the lower-level tuple */
            if (prefixSize > commonLen + 1)
            {
                out->result.splitTuple.postfixHasPrefix = true;
                out->result.splitTuple.postfixPrefixDatum =
                    PointerGetDatum(cstring_to_text_with_len(prefixStr + commonLen + 1, prefixSize - (commonLen + 1)));
            }
            else
            {
                out->result.splitTuple.postfixHasPrefix = false;
            }

            PG_RETURN_VOID();
        }
    }
    else if (inSize > 0)
    {
         /* No prefix, take first character as node label. */
        nodeChar = inStr[0];
    }
    else
    {
        /* Empty kmer (should not happen), use '\0' */
        nodeChar = '\0';
    }

    /* Look up nodeChar in the node label array */
    if (searchChar(in->nodeLabels, in->nNodes, nodeChar, &i))
    {
        // If the new value matches one of the existing child nodes, set resultType to spgMatchNode.
        out->resultType = spgMatchNode;
        // Set nodeN to the index (from zero) of that node in the node array.
        out->result.matchNode.nodeN = i;
        // Set levelAdd to the increment in level caused by descending through that node, or leave it as zero if the operator class does not use levels.
        out->result.matchNode.levelAdd = (nodeChar == '\0') ? 0 : 1; 
        /* Set restDatum to equal leafDatum if the operator class does not modify datums from one level to the
        next, or otherwise set it to the modified value to be used as leafDatum at the next level. */
        offset = (in->hasPrefix ? prefixSize : 0) + out->result.matchNode.levelAdd;
        if (inSize > offset)
        {
            out->result.matchNode.restDatum =
                PointerGetDatum(cstring_to_text_with_len(inStr + offset, inSize - offset));
        }
        else
        {
            out->result.matchNode.restDatum = PointerGetDatum(cstring_to_text(""));
        }
    }
    else if (in->allTheSame)
    {
        /* If the new value is inconsistent with the tuple prefix, set resultType to spgSplitTuple. 
        This action moves all the existing nodes into a new lower-level inner tuple, and replaces 
        the existing inner tuple with a tuple having a single downlink pointing to the new lower-level inner tuple. */
        out->resultType = spgSplitTuple;
        // Set prefixHasPrefix to indicate whether the new upper tuple should have a prefix.
        out->result.splitTuple.prefixHasPrefix = in->hasPrefix;
        /* If so, set prefixPrefixDatum to the prefix value. This new prefix value must be sufficiently
         less restrictive than the original to accept the new value to be indexed. */
        out->result.splitTuple.prefixPrefixDatum = in->prefixDatum;
        // Set prefixNNodes to the number of nodes needed in the new tuple.
        out->result.splitTuple.prefixNNodes = 2;
        // Set prefixNodeLabels to a palloc'd array holding their labels.
        out->result.splitTuple.prefixNodeLabels =
            (Datum *) palloc(sizeof(Datum) * 2);
        out->result.splitTuple.prefixNodeLabels[0] = in->nodeLabels[0];
        out->result.splitTuple.prefixNodeLabels[1] = CharGetDatum(nodeChar);
        // Set childNodeN to the index (from zero) of the node that will downlink to the new lower-level inner tuple.
        out->result.splitTuple.childNodeN = 1;
        /* Set postfixHasPrefix to indicate whether the new lower-level inner tuple should have a prefix,
        and if so setpostfixPrefixDatum to the prefix value (not in this case). */
        out->result.splitTuple.postfixHasPrefix = false;

        PG_RETURN_VOID();
    }
    else
    {
        /* Add a node for the not-previously-seen nodeChar value */
        out->resultType = spgAddNode; // If a new child node must be added, set resultType to spgAddNode.
        out->result.addNode.nodeLabel = CharGetDatum(nodeChar); // Set nodeLabel to the label to be used for the new node
        out->result.addNode.nodeN = i; // set nodeN to the index (from zero) at which to insert the node in the node array
    }

    PG_RETURN_VOID();
}

// Used in the PickSplit function to sort kmers based on their next character.
typedef struct spgKmerNode
{
    Datum d; /* Datum (kmer string) */
    int i; /* Original index in the input array */
    char c; /* Character at the split position */
} spgKmerNode;

// Comparison function for qsort in spg_kmer_picksplit.
static int
cmpKmerNode(const void *a, const void *b)
{
    spgKmerNode *aa = (spgKmerNode *) a;
    spgKmerNode *bb = (spgKmerNode *) b;

    return (aa->c - bb->c);
}

/* Decides how to create a new inner tuple over a set of leaf tuples. */
PG_FUNCTION_INFO_V1(spg_kmer_picksplit);
Datum
spg_kmer_picksplit(PG_FUNCTION_ARGS)
{
    spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0); // The first argument is a pointer to a spgPickSplitIn C struct, containing input data for the function.
    spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1); // The second argument is a pointer to a spgPickSplitOut C struct, which the function must fill with result data.
    int i, commonLen;
    spgKmerNode *nodes;
    char *str;
    int len;

    /// Find common prefix from all input kmers
    text *firstText = DatumGetTextPP(in->datums[0]);
    char *firstStr = VARDATA_ANY(firstText);
    int firstLen = VARSIZE_ANY_EXHDR(firstText);
    commonLen = firstLen;

    /* If more than one leaf tuple is supplied, it is expected that the picksplit function will
    classify them into more than one node; otherwise it is not possible to split the leaf tuples
    across multiple pages, which is the ultimate purpose of this operation.
    (otherwise the core SP-GiST code will override that decision and generate an inner tuple in which
    the leaf tuples are assigned at random to several identically-labeled nodes) */
    for (i = 1; i < in->nTuples && commonLen > 0; i++)
    {
        text *currText = DatumGetTextPP(in->datums[i]);
        char *currStr = VARDATA_ANY(currText);
        int currLen = VARSIZE_ANY_EXHDR(currText);
        int tmpLen = commonPrefix(firstStr, currStr, firstLen, currLen);
        if (tmpLen < commonLen)
            commonLen = tmpLen;
    }

    // Set prefix if commonLen > 0
    if (commonLen > 0)
    {
        out->hasPrefix = true;
        out->prefixDatum = PointerGetDatum(cstring_to_text_with_len(firstStr, commonLen));
    }
    else
    {
        out->hasPrefix = false;
    }

    // Prep nodes for sort
    nodes = (spgKmerNode *) palloc(sizeof(spgKmerNode) * in->nTuples);

    for (i = 0; i < in->nTuples; i++)
    {
        text *strText = DatumGetTextPP(in->datums[i]);
        str = VARDATA_ANY(strText);
        len = VARSIZE_ANY_EXHDR(strText);

        if (commonLen < len)
            nodes[i].c = str[commonLen];
        else
            nodes[i].c = '\0'; /* No more characters */
        nodes[i].d = in->datums[i];
        nodes[i].i = i;
    }

    /* Sort nodes by char to group by node labels */
    qsort(nodes, in->nTuples, sizeof(spgKmerNode), cmpKmerNode);

    // Set nNodes to indicate the number of nodes that the new inner tuple will contain
    out->nNodes = 0;
    // Set nodeLabels to an array of their label values.
    out->nodeLabels = (Datum *) palloc(sizeof(Datum) * in->nTuples);
    // Set mapTuplesToNodes to an array that gives the index (from zero) of the node that each leaf tuple should be assigned to.
    out->mapTuplesToNodes = (int *) palloc(sizeof(int) * in->nTuples);
    /* Set leafTupleDatums to an array of the values to be stored in the new leaf tuples (these will be the 
    same as the input datums if the operator class does not modify datums from one level to the next).*/
    out->leafTupleDatums = (Datum *) palloc(sizeof(Datum) * in->nTuples);

    // (the picksplit function is responsible for palloc'ing the nodeLabels, mapTuplesToNodes and leafTupleDatums arrays)

    // Build nodes -> map tuples to nodes
    for (i = 0; i < in->nTuples; i++)
    {
        text *texti = NULL;
        // If new node label, add to nodeLabels
        if (i == 0 || nodes[i].c != nodes[i - 1].c)
        {
            out->nodeLabels[out->nNodes] = CharGetDatum(nodes[i].c);
            out->nNodes++;
        }

        // Prep leaf datum for each tuple (remaining string after prefix and node char)
        texti = DatumGetTextPP(nodes[i].d);
        str = VARDATA_ANY(texti);
        len = VARSIZE_ANY_EXHDR(texti);

        if (commonLen + 1 < len)
            out->leafTupleDatums[nodes[i].i] =
                PointerGetDatum(cstring_to_text_with_len(str + commonLen + 1, len - (commonLen + 1)));
        else
            out->leafTupleDatums[nodes[i].i] = PointerGetDatum(cstring_to_text(""));

        out->mapTuplesToNodes[nodes[i].i] = out->nNodes - 1;
    }

    PG_RETURN_VOID();
}


/* Returns set of nodes (branches) to follow during tree search. */
/* SP-GiST core code handles most of the search logic, but the choose
function must provide the core with the set of nodes to follow during the search. */

// !!! IN THIS IMPLEMENTATION, IT PREPARES TO VISIT ALL POSSIBLE CHILD NODES, THIS WILL HAVE TO BE WRITTEN PROPERLY LATER !!!
PG_FUNCTION_INFO_V1(spg_kmer_inner_consistent);
Datum
spg_kmer_inner_consistent(PG_FUNCTION_ARGS)
{
    spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0); // The first argument is a pointer to a spgInnerConsistentIn C struct, containing input data for the function.
    spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1); // The second argument is a pointer to a spgInnerConsistentOut C struct, which the function must fill with result data.

    int i;

    // nNodes must be set to the number of child nodes that need to be visited by the search.
    out->nNodes = 0;
    // nodeNumbers must be set to an array of their indexes.
    out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);
    /* If the operator class keeps track of levels, set levelAdds to an array of the level 
    increments required when descending to each node to be visited.
    (Often these increments will be the same for all the nodes, but that's not necessarily so, so an array is used.) */
    out->levelAdds = (int *) palloc(sizeof(int) * in->nNodes);
    /* If value reconstruction is needed, set reconstructedValues to an array of the values 
    reconstructed for each child node to be visited; otherwise, leave reconstructedValues as NULL. 
    The reconstructed values are assumed to be of type spgConfigOut.leafType.*/
    out->reconstructedValues = (Datum *) palloc(sizeof(Datum) * in->nNodes);

    for (i = 0; i < in->nNodes; i++)
    {
        out->nodeNumbers[out->nNodes] = i;
        out->levelAdds[out->nNodes] = (DatumGetChar(in->nodeLabels[i]) == '\0') ? 0 : 1;

        /* Reconstruct the value up to this point */
        if (DatumGetPointer(in->reconstructedValue))
        {
            text *reconText = DatumGetTextPP(in->reconstructedValue);
            int reconLen = VARSIZE_ANY_EXHDR(reconText);
            char *reconData = VARDATA_ANY(reconText);
            char nodeChar = DatumGetChar(in->nodeLabels[i]);

            /* Allocate new reconstr. value */
            int newReconLen = reconLen + ((nodeChar == '\0') ? 0 : 1);
            text *newReconText = (text *) palloc(VARHDRSZ + newReconLen);
            SET_VARSIZE(newReconText, VARHDRSZ + newReconLen);
            memcpy(VARDATA(newReconText), reconData, reconLen);
            if (nodeChar != '\0')
                VARDATA(newReconText)[reconLen] = nodeChar;

            out->reconstructedValues[out->nNodes] = PointerGetDatum(newReconText);
        }
        else
        {
            char nodeChar = DatumGetChar(in->nodeLabels[i]);

            /* Start a new reconstr. value */
            if (nodeChar != '\0')
            {
                text *newReconText = (text *) palloc(VARHDRSZ + 1);
                SET_VARSIZE(newReconText, VARHDRSZ + 1);
                VARDATA(newReconText)[0] = nodeChar;

                out->reconstructedValues[out->nNodes] = PointerGetDatum(newReconText);
            }
            else
            {
                /* Empty text */
                out->reconstructedValues[out->nNodes] = PointerGetDatum(cstring_to_text(""));
            }
        }

        out->nNodes++;
    }

    PG_RETURN_VOID();
}

/* Returns true if a leaf tuple satisfies a query. */
PG_FUNCTION_INFO_V1(spg_kmer_leaf_consistent);
Datum
spg_kmer_leaf_consistent(PG_FUNCTION_ARGS)
{
    spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0); // The first argument is a pointer to a spgLeafConsistentIn C struct, containing input data for the function.
    spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1); // The second argument is a pointer to a spgLeafConsistentOut C struct, which the function must fill with result data.

    /* The function must return true if the leaf tuple matches the query, or false if not. */
    bool res = true;
    text *leafText = DatumGetTextPP(in->leafDatum);
    char *leafValue = VARDATA_ANY(leafText);
    int leafLen = VARSIZE_ANY_EXHDR(leafText);
    char *fullValue;
    int fullLen;
    int j;

    /* In the true case, if returnData is true then leafValue must be set to the value (of type 
    spgConfigIn.attType) originally supplied to be indexed for this leaf tuple. 
    => reconstruct the full kmer value */
    if (DatumGetPointer(in->reconstructedValue))
    {
        text *reconText = DatumGetTextPP(in->reconstructedValue);
        char *reconstructed = VARDATA_ANY(reconText);
        int reconLen = VARSIZE_ANY_EXHDR(reconText);
        fullLen = reconLen + leafLen;
        fullValue = (char *) palloc(fullLen + 1);
        memcpy(fullValue, reconstructed, reconLen);
        memcpy(fullValue + reconLen, leafValue, leafLen);
        fullValue[fullLen] = '\0';
    }
    else
    {
        fullValue = (char *) palloc(leafLen + 1);
        memcpy(fullValue, leafValue, leafLen);
        fullValue[leafLen] = '\0';
        fullLen = leafLen;
    }

    // Check if the kmer matches the search conditions.
    for (j = 0; j < in->nkeys; j++)
    {
        StrategyNumber strategy = in->scankeys[j].sk_strategy;
        text *queryText = DatumGetTextPP(in->scankeys[j].sk_argument);
        char *queryStr = VARDATA_ANY(queryText);
        int queryLen = VARSIZE_ANY_EXHDR(queryText);
        int k;

        if (strategy == BTEqualStrategyNumber)
        {
            if (fullLen != queryLen || memcmp(fullValue, queryStr, fullLen) != 0)
            {
                res = false;
                break;
            }
        }
        else if (strategy == 6) /* starts_with operator */
        {
            if (queryLen > fullLen || memcmp(fullValue, queryStr, queryLen) != 0)
            {
                res = false;
                break;
            }
        }
        else if (strategy == 7) /* contains operator */
        {
            // Pattern matching with IUPAC codes
            int kmer_len = fullLen;
            int pattern_len = queryLen;
            if (kmer_len != pattern_len)
            {
                res = false;
                break;
            }
            for (k = 0; k < kmer_len; k++)
            {
                char kmer_char = toupper(fullValue[k]);
                char pattern_char = toupper(queryStr[k]);
                int q_bits = iupac_code_to_bits(pattern_char);
                int k_bits = nucleotide_to_bits(kmer_char);

                if ((q_bits & k_bits) == 0)
                {
                    res = false;
                    break;
                }
            }
            if (!res)
                break;
        }
        else
        {
            elog(ERROR, "unrecognized strategy number: %d", strategy);
        }
    }

    // Return the reconstructed full kmer value (if requested)
    if (in->returnData)
    {
        text *resultText = cstring_to_text_with_len(fullValue, fullLen);
        out->leafValue = PointerGetDatum(resultText);
    }
    else
    {
        out->leafValue = (Datum) 0;
    }
    out->recheck = false; /* No need to recheck at heap level */

    PG_RETURN_BOOL(res);
}


/*
 * !!! NEEDS TO BE LOOK into later, THIS IS A SIMPLE EXAMPLE OF HOW TO IMPLEMENT THE COMPRESS FUNCTION !!!
 * Compress function to convert from input type (kmer/text) to leaf type (cstring).
 */
/* Compress function to convert from input type to leaf type */
PG_FUNCTION_INFO_V1(spg_kmer_compress);
Datum
spg_kmer_compress(PG_FUNCTION_ARGS)
{
    text *inText = PG_GETARG_TEXT_PP(0);  // Get the input as text
    int32 inLen = VARSIZE_ANY_EXHDR(inText);  // Extract length of the input text
    char *inData = VARDATA_ANY(inText);  // Extract pointer to the data portion

    // Create a new text object for compression output
    text *compressedText = (text *) palloc(VARHDRSZ + inLen);
    SET_VARSIZE(compressedText, VARHDRSZ + inLen);
    memcpy(VARDATA(compressedText), inData, inLen);

    // Return the compressed text as a Datum
    PG_RETURN_POINTER(compressedText);
}