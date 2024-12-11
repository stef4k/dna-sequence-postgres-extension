#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "postgres.h"

#include "access/spgist.h"
#include "access/spgist_private.h"
#include "catalog/pg_type.h"
#include "utils/datum.h"
#include "utils/pg_locale.h"
#include "storage/bufpage.h" //for BLCKSZ
#include "utils/varlena.h"
#include "common/int.h"

#include "dna_sequence.h"

/* Define maximum prefix length for SP-GiST index */
#ifndef SPGIST_MAX_PREFIX_LENGTH
#define SPGIST_MAX_PREFIX_LENGTH    Max((int) (BLCKSZ - 258 * 16 - 100), 32)
#endif

extern Oid TypenameGetTypid(const char *typname);

extern Datum kmer_starts_with(PG_FUNCTION_ARGS);

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

#define SPGIST_MAX_PREFIX_LENGTH    Max((int) (BLCKSZ - 258 * 16 - 100), 32)

/*
 * Strategy numbers for kmer SP-GiST indexing
 */
#define KMER_EQUAL_STRATEGY         1
#define KMER_PREFIX_STRATEGY        2
#define KMER_CONTAINS_STRATEGY      3

/*
 * Define the OID of the kmer type
 */
static Oid get_kmer_oid(void)
{
    Oid kmer_oid = TypenameGetTypid("kmer");
    if (!OidIsValid(kmer_oid))
        elog(ERROR, "Type 'kmer' does not exist");
    return kmer_oid;
}

/* Struct for sorting values in picksplit */
typedef struct spgNodePtr
{
    Datum       d;
    int         i;
    int16       c;
} spgNodePtr;

/* Function declarations */
PG_FUNCTION_INFO_V1(spg_kmer_config);
Datum spg_kmer_config(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(spg_kmer_choose);
Datum spg_kmer_choose(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(spg_kmer_picksplit);
Datum spg_kmer_picksplit(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(spg_kmer_inner_consistent);
Datum spg_kmer_inner_consistent(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(spg_kmer_leaf_consistent);
Datum spg_kmer_leaf_consistent(PG_FUNCTION_ARGS);

/* Helper function declarations */
static Datum formKmerDatum(const char *data, int datalen);
static int commonPrefix(const char *a, const char *b, int lena, int lenb);
static bool searchChar(Datum *nodeLabels, int nNodes, int16 c, int *i);
static int cmpNodePtr(const void *a, const void *b);

/* Implementation */

Datum
spg_kmer_config(PG_FUNCTION_ARGS)
{
    spgConfigOut *cfg = (spgConfigOut *) PG_GETARG_POINTER(1);

    cfg->prefixType = get_kmer_oid();
    cfg->labelType = INT2OID;
    cfg->canReturnData = true; /* allow for reconstructon of original kmers */
    cfg->longValuesOK = false;   /* suffixing will shorten long values */

    /* picksplit can be applied to a single leaf tuple only in the case that the config function 
     * set longValuesOK to true and a larger-than-a-page input value has been supplied. */
    PG_RETURN_VOID();
}

 /*
  * Form a kmer datum from the given not-necessarily-null-terminated string,
  * using short varlena header format if possible
  */
static Datum
formKmerDatum(const char *data, int datalen)
 {
     char       *p;
  
     p = (char *) palloc(datalen + VARHDRSZ);
  
     if (datalen + VARHDRSZ_SHORT <= VARATT_SHORT_MAX)
     {
         SET_VARSIZE_SHORT(p, datalen + VARHDRSZ_SHORT);
         if (datalen)
             memcpy(p + VARHDRSZ_SHORT, data, datalen);
     }
     else
     {
         SET_VARSIZE(p, datalen + VARHDRSZ);
         memcpy(p + VARHDRSZ, data, datalen);
     }
  
     return PointerGetDatum(p);
 }

 /*
  * Find the length of the common prefix of a and b
  */
static int
commonPrefix(const char *a, const char *b, int lena, int lenb)
{
    int         i = 0;

    while (i < lena && i < lenb && *a == *b)
    {
        a++;
        b++;
        i++;
    }

    return i;
}

 /*
  * Binary search an array of int16 datums for a match to c
  *
  * On success, *i gets the match location; on failure, it gets where to insert
  */
static bool
searchChar(Datum *nodeLabels, int nNodes, int16 c, int *i)
{
    int         StopLow = 0,
                StopHigh = nNodes;

    while (StopLow < StopHigh)
    {
        int         StopMiddle = (StopLow + StopHigh) >> 1;
        int16       middle = DatumGetInt16(nodeLabels[StopMiddle]);

        if (c < middle)
            StopHigh = StopMiddle;
        else if (c > middle)
            StopLow = StopMiddle + 1;
        else
        {
            *i = StopMiddle;
            return true;
        }
    }

    *i = StopHigh;
    return false;
}

/* qsort comparator to sort spgNodePtr structs by "c" */
static int
cmpNodePtr(const void *a, const void *b)
{
    const spgNodePtr *aa = (const spgNodePtr *) a;
    const spgNodePtr *bb = (const spgNodePtr *) b;

    if (aa->c < bb->c)
        return -1;
    else if (aa->c > bb->c)
        return 1;
    else
        return 0;
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

Datum
spg_kmer_choose(PG_FUNCTION_ARGS)
{
    spgChooseIn *in = (spgChooseIn *) PG_GETARG_POINTER(0); // The first argument is a pointer to a spgChooseIn C struct, containing input data for the function.
    spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1); // The second argument is a pointer to a spgChooseOut C struct, which the function must fill with result data.

    // See the struct documentation for this and all the following functions here: https://www.postgresql.org/docs/current/spgist.html

    Kmer       *inKmer = (Kmer *) DatumGetPointer(in->datum);
    char       *inStr = VARDATA_ANY(inKmer);
    int         inSize = VARSIZE_ANY_EXHDR(inKmer);
    char       *prefixStr = NULL;
    int         prefixSize = 0;
    int         commonLen = 0;
    int16       nodeChar = 0;
    int         i = 0;

    /* Check for prefix match, set nodeChar to first byte after prefix */
    if (in->hasPrefix)
    {
        Kmer       *prefixKmer = (Kmer *) DatumGetPointer(in->prefixDatum);

        prefixStr = VARDATA_ANY(prefixKmer);
        prefixSize = VARSIZE_ANY_EXHDR(prefixKmer);

        commonLen = commonPrefix(inStr + in->level,
                                  prefixStr,
                                  inSize - in->level,
                                  prefixSize);

        if (commonLen == prefixSize)
        {
            if (inSize - in->level > commonLen)
                nodeChar = *(unsigned char *) (inStr + in->level + commonLen);
            else
                nodeChar = -1;
        }
        else
        {
            /* Must split tuple because incoming value doesn't match prefix */
            out->resultType = spgSplitTuple; // If the new value is inconsistent with the tuple prefix, set resultType to spgSplitTuple.

            if (commonLen == 0)
            {
                out->result.splitTuple.prefixHasPrefix = false;
            }
            else
            {
                out->result.splitTuple.prefixHasPrefix = true;
                out->result.splitTuple.prefixPrefixDatum =
                    formKmerDatum(prefixStr, commonLen);
            }
            out->result.splitTuple.prefixNNodes = 1;
            out->result.splitTuple.prefixNodeLabels =
                (Datum *) palloc(sizeof(Datum));
            out->result.splitTuple.prefixNodeLabels[0] =
                Int16GetDatum(*(unsigned char *) (prefixStr + commonLen));

            out->result.splitTuple.childNodeN = 0;

            /* Set up postfix prefix for the lower-level tuple */
            if (prefixSize - commonLen == 1)
            {
                out->result.splitTuple.postfixHasPrefix = false;
            }
            else
            {
                out->result.splitTuple.postfixHasPrefix = true;
                out->result.splitTuple.postfixPrefixDatum =
                    formKmerDatum(prefixStr + commonLen + 1,
                                  prefixSize - commonLen - 1);
            }

            PG_RETURN_VOID();
        }
    }
    else if (inSize > in->level)
    {
        nodeChar = *(unsigned char *) (inStr + in->level);
    }
    else
    {
        nodeChar = -1;
    }

    /* Look up nodeChar in the node label array */
    if (searchChar(in->nodeLabels, in->nNodes, nodeChar, &i))
    {
        /* Descend to existing node */
        int         levelAdd;

        // If the new value matches one of the existing child nodes, set resultType to spgMatchNode.
        out->resultType = spgMatchNode;
        // Set nodeN to the index (from zero) of that node in the node array.
        out->result.matchNode.nodeN = i;
        // Set levelAdd to the increment in level caused by descending through that node, or leave it as zero if the operator class does not use levels.
        levelAdd = commonLen;
        /* Set restDatum to equal leafDatum if the operator class does not modify datums from one level to the
        next, or otherwise set it to the modified value to be used as leafDatum at the next level. */
        if (nodeChar >= 0)
            levelAdd++;
        out->result.matchNode.levelAdd = levelAdd;
        if (inSize - in->level - levelAdd > 0)
            out->result.matchNode.restDatum =
                formKmerDatum(inStr + in->level + levelAdd,
                              inSize - in->level - levelAdd);
        else
            out->result.matchNode.restDatum =
                formKmerDatum(NULL, 0);
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
        out->result.splitTuple.prefixNNodes = 1;
        // Set prefixNodeLabels to a palloc'd array holding their labels.
        out->result.splitTuple.prefixNodeLabels = (Datum *) palloc(sizeof(Datum));
        out->result.splitTuple.prefixNodeLabels[0] = Int16GetDatum(-2);
        // Set childNodeN to the index (from zero) of the node that will downlink to the new lower-level inner tuple
        out->result.splitTuple.childNodeN = 0;
        /* Set postfixHasPrefix to indicate whether the new lower-level inner tuple should have a prefix,
        and if so setpostfixPrefixDatum to the prefix value (not in this case). */
        out->result.splitTuple.postfixHasPrefix = false;
    }
    else
    {
        /* Add a node for the not-previously-seen nodeChar value */
        out->resultType = spgAddNode; // If a new child node must be added, set resultType to spgAddNode.
        out->result.addNode.nodeLabel = Int16GetDatum(nodeChar); // Set nodeLabel to the label to be used for the new node
        out->result.addNode.nodeN = i; // set nodeN to the index (from zero) at which to insert the node in the node array
    }

    PG_RETURN_VOID();
}

/* Decides how to create a new inner tuple over a set of leaf tuples. */
Datum
spg_kmer_picksplit(PG_FUNCTION_ARGS)
{
    spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0); // The first argument is a pointer to a spgPickSplitIn C struct, containing input data for the function.
    spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1); // The second argument is a pointer to a spgPickSplitOut C struct, which the function must fill with result data.
    Kmer       *kmer0 = (Kmer *) DatumGetPointer(in->datums[0]);
    int         i,
                commonLen;
    spgNodePtr *nodes;

    /* Identify longest common prefix, if any */
    commonLen = VARSIZE_ANY_EXHDR(kmer0);
    for (i = 1; i < in->nTuples && commonLen > 0; i++)
    {
        Kmer       *kmeri = (Kmer *) DatumGetPointer(in->datums[i]);
        int         tmp = commonPrefix(VARDATA_ANY(kmer0),
                                       VARDATA_ANY(kmeri),
                                       VARSIZE_ANY_EXHDR(kmer0),
                                       VARSIZE_ANY_EXHDR(kmeri));

        if (tmp < commonLen)
            commonLen = tmp;
    }

    /*
     * Limit the prefix length, if necessary, to ensure that the resulting
     * inner tuple will fit on a page.
     */
    commonLen = Min(commonLen, SPGIST_MAX_PREFIX_LENGTH);

    /* Set node prefix to be that string, if it's not empty */
    if (commonLen == 0)
    {
        out->hasPrefix = false;
    }
    else
    {
        out->hasPrefix = true;
        out->prefixDatum = formKmerDatum(VARDATA_ANY(kmer0), commonLen);
    }

    /* Extract the node label (first non-common byte) from each value */
    // Prep nodes for sort
    nodes = (spgNodePtr *) palloc(sizeof(spgNodePtr) * in->nTuples);

    for (i = 0; i < in->nTuples; i++)
    {
        Kmer       *kmeri = (Kmer *) DatumGetPointer(in->datums[i]);

        if (commonLen < VARSIZE_ANY_EXHDR(kmeri))
            nodes[i].c = *(unsigned char *) (VARDATA_ANY(kmeri) + commonLen);
        else
            nodes[i].c = -1;    /* use -1 if string is all common */
        nodes[i].i = i;
        nodes[i].d = in->datums[i];
    }

    /*
     * Sort by label values so that we can group the values into nodes.  This
     * also ensures that the nodes are ordered by label value, allowing the
     * use of binary search in searchChar.
     */
    qsort(nodes, in->nTuples, sizeof(*nodes), cmpNodePtr);

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
        Kmer       *kmeri = (Kmer *) DatumGetPointer(nodes[i].d);
        Datum       leafD;

        // If new node label, add to nodeLabels
        if (i == 0 || nodes[i].c != nodes[i - 1].c)
        {
            out->nodeLabels[out->nNodes] = Int16GetDatum(nodes[i].c);
            out->nNodes++;
        }

        if (commonLen < VARSIZE_ANY_EXHDR(kmeri))
            leafD = formKmerDatum(VARDATA_ANY(kmeri) + commonLen + 1,
                                  VARSIZE_ANY_EXHDR(kmeri) - commonLen - 1);
        else
            leafD = formKmerDatum(NULL, 0);

        out->leafTupleDatums[nodes[i].i] = leafD;
        out->mapTuplesToNodes[nodes[i].i] = out->nNodes - 1;
    }

    PG_RETURN_VOID();
}

/* Returns set of nodes (branches) to follow during tree search. */
/* SP-GiST core code handles most of the search logic, but the choose
function must provide the core with the set of nodes to follow during the search. */
Datum
spg_kmer_inner_consistent(PG_FUNCTION_ARGS)
{
    spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0); // The first argument is a pointer to a spgInnerConsistentIn C struct, containing input data for the function.
    spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1); // The second argument is a pointer to a spgInnerConsistentOut C struct, which the function must fill with result data.
    Kmer       *reconstructedValue;
    Kmer       *reconstrKmer;
    int         maxReconstrLen;
    Kmer       *prefixKmer = NULL;
    int         prefixSize = 0;
    int         i;

    /*
     * Reconstruct values represented at this tuple, including parent data,
     * prefix of this tuple if any, and the node label if it's non-dummy.
     * in->level should be the length of the previously reconstructed value,
     * and the number of bytes added here is prefixSize or prefixSize + 1.
     *
     * Note: we assume that in->reconstructedValue isn't toasted and doesn't
     * have a short varlena header. This is okay because it must have been
     * created by a previous invocation of this routine, and we always emit
     * long-format reconstructed values.
     */
    reconstructedValue = (Kmer *) DatumGetPointer(in->reconstructedValue);
    Assert(reconstructedValue == NULL ? in->level == 0 :
           VARSIZE_ANY_EXHDR(reconstructedValue) == in->level);

    maxReconstrLen = in->level + 1;
    if (in->hasPrefix)
    {
        prefixKmer = (Kmer *) DatumGetPointer(in->prefixDatum);
        prefixSize = VARSIZE_ANY_EXHDR(prefixKmer);
        maxReconstrLen += prefixSize;
    }

    reconstrKmer = palloc(VARHDRSZ + maxReconstrLen);
    SET_VARSIZE(reconstrKmer, VARHDRSZ + maxReconstrLen);

    if (in->level)
        memcpy(VARDATA(reconstrKmer),
               VARDATA_ANY(reconstructedValue),
               in->level);
    if (prefixSize)
        memcpy(VARDATA(reconstrKmer) + in->level,
               VARDATA_ANY(prefixKmer),
               prefixSize);

    /* last byte of reconstrKmer will be filled in below */

    /*
     * Scan the child nodes. For each one, complete the reconstructed value
     * and see if it's consistent with the query. If so, emit an entry into
     * the output arrays.
     */
    
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
    // nNodes must be set to the number of child nodes that need to be visited by the search.
    out->nNodes = 0;

    // Debugging
    // elog(NOTICE, "Number of nodes: %d", in->nNodes);

    for (i = 0; i < in->nNodes; i++)
    {
        int16       nodeChar = DatumGetInt16(in->nodeLabels[i]);
        int         thisLen;
        bool        res = true;
        int         j;

        // Debugging
        // elog(NOTICE, "Node: %d", nodeChar);

        /* If nodeChar is a dummy value, don't include it in data */
        if (nodeChar <= 0)
            thisLen = maxReconstrLen - 1;
        else
        {
            VARDATA(reconstrKmer)[maxReconstrLen - 1] = nodeChar;
            thisLen = maxReconstrLen;
        }

        for (j = 0; j < in->nkeys; j++)
        {
            StrategyNumber strategy = in->scankeys[j].sk_strategy;
            Kmer       *inKmer;
            int         inSize;
            int         r;

            switch (strategy)
            {
                case KMER_EQUAL_STRATEGY:
                    inKmer = (Kmer *) DatumGetPointer(in->scankeys[j].sk_argument);
                    inSize = VARSIZE_ANY_EXHDR(inKmer);
                    r = memcmp(VARDATA(reconstrKmer), VARDATA_ANY(inKmer), Min(inSize, thisLen));
                    if (r != 0 || inSize < thisLen)
                        res = false;
                    break;
                case KMER_PREFIX_STRATEGY:
                    inKmer = (Kmer *) DatumGetPointer(in->scankeys[j].sk_argument);
                    inSize = VARSIZE_ANY_EXHDR(inKmer);
                    r = memcmp(VARDATA(reconstrKmer), VARDATA_ANY(inKmer), Min(inSize, thisLen));
                    if (r != 0)
                        res = false;
                    break;
                case KMER_CONTAINS_STRATEGY:
                    {
                        Qkmer *inQkmer = (Qkmer *) DatumGetPointer(in->scankeys[j].sk_argument);
                        int inSize = VARSIZE_ANY_EXHDR(inQkmer);

                        if (inSize < thisLen)
                        {
                            res = false;
                        }
                        else
                        {
                            char *qkmer_str = VARDATA_ANY(inQkmer);
                            char *kmer_str = VARDATA(reconstrKmer);
                            int k;

                            for (k = 0; k < thisLen; k++)
                            {
                                int q_bits = iupac_code_to_bits(qkmer_str[k]);
                                int k_bits = nucleotide_to_bits(kmer_str[k]);

                                if ((q_bits & k_bits) == 0)
                                {
                                    res = false;
                                    break;
                                }
                            }
                        }
                    }
                    break;
                default:
                    elog(ERROR, "unrecognized strategy number: %d",
                         in->scankeys[j].sk_strategy);
                    res = false;
                    break;
            }

            if (!res)
                break;          /* no need to consider remaining conditions */
        }

        if (res)
        {
            out->nodeNumbers[out->nNodes] = i;
            out->levelAdds[out->nNodes] = thisLen - in->level;
            SET_VARSIZE(reconstrKmer, VARHDRSZ + thisLen);
            out->reconstructedValues[out->nNodes] =
                datumCopy(PointerGetDatum(reconstrKmer), false, -1);
            out->nNodes++;
        }
    }

    PG_RETURN_VOID();
}

/* Returns true if a leaf tuple satisfies a query. */
Datum
spg_kmer_leaf_consistent(PG_FUNCTION_ARGS)
{
    spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0); // The first argument is a pointer to a spgLeafConsistentIn C struct, containing input data for the function.
    spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1); // The second argument is a pointer to a spgLeafConsistentOut C struct, which the function must fill with result data.
    int         level = in->level;
    Kmer       *leafValue,
               *reconstrValue = NULL;
    char       *fullValue;
    int         fullLen;
    /* The function must return true if the leaf tuple matches the query, or false if not. */
    bool        res;
    int         j;

    /* All tests are exact */
    out->recheck = false;

    leafValue = (Kmer *) DatumGetPointer(in->leafDatum);

    /* In the true case, if returnData is true then leafValue must be set to the value (of type 
    spgConfigIn.attType) originally supplied to be indexed for this leaf tuple. 
    => reconstruct the full kmer value */
    /* As above, in->reconstructedValue isn't toasted or short. */
    if (DatumGetPointer(in->reconstructedValue))
        reconstrValue = (Kmer *) DatumGetPointer(in->reconstructedValue);

    Assert(reconstrValue == NULL ? level == 0 :
           VARSIZE_ANY_EXHDR(reconstrValue) == level);

    /* Reconstruct the full string represented by this leaf tuple */
    fullLen = level + VARSIZE_ANY_EXHDR(leafValue);
    if (VARSIZE_ANY_EXHDR(leafValue) == 0 && level > 0)
    {
        fullValue = VARDATA_ANY(reconstrValue);
        out->leafValue = PointerGetDatum(reconstrValue);
    }
    else
    {
        Kmer       *fullKmer = palloc(VARHDRSZ + fullLen);

        SET_VARSIZE(fullKmer, VARHDRSZ + fullLen);
        fullValue = VARDATA(fullKmer);
        if (level)
            memcpy(fullValue, VARDATA_ANY(reconstrValue), level);
        if (VARSIZE_ANY_EXHDR(leafValue) > 0)
            memcpy(fullValue + level, VARDATA_ANY(leafValue),
                   VARSIZE_ANY_EXHDR(leafValue));
        out->leafValue = PointerGetDatum(fullKmer);
    }

    /* Perform the required comparison(s) */
    res = true;
    for (j = 0; j < in->nkeys; j++)
    {
        StrategyNumber strategy = in->scankeys[j].sk_strategy;
        Kmer       *query;
        int         queryLen;
        int         r;

        switch (strategy)
        {
            case KMER_EQUAL_STRATEGY:
                query = (Kmer *) DatumGetPointer(in->scankeys[j].sk_argument);
                queryLen = VARSIZE_ANY_EXHDR(query);
                r = memcmp(fullValue, VARDATA_ANY(query), Min(queryLen, fullLen));
                res = (r == 0 && queryLen == fullLen);
                break;
            case KMER_PREFIX_STRATEGY:
            {
                /*
                * if level >= length of query then reconstrValue must begin with
                * query (prefix) string, so we don't need to check it again.
                */
                query = (Kmer *) DatumGetPointer(in->scankeys[j].sk_argument);
                queryLen = VARSIZE_ANY_EXHDR(query);
                res = (level >= queryLen) ||
                    DatumGetBool(DirectFunctionCall2Coll(kmer_starts_with,
                                                        PG_GET_COLLATION(),
                                                        out->leafValue,
                                                        PointerGetDatum(query)));
    
                if (!res)           /* no need to consider remaining conditions */
                    break;
    
                continue;
            }
            case KMER_CONTAINS_STRATEGY:
                {
                    Qkmer *pattern = (Qkmer *) DatumGetPointer(in->scankeys[j].sk_argument);
                    int patternLen = VARSIZE_ANY_EXHDR(pattern);
                    char *qkmer_str;
                    char *kmer_str;
                    int k;
                    if (fullLen != patternLen)
                    {
                        res = false;
                        break;
                    }

                    qkmer_str = VARDATA_ANY(pattern);
                    kmer_str = fullValue;
                    for (k = 0; k < fullLen; k++)
                    {
                        int q_bits = iupac_code_to_bits(qkmer_str[k]);
                        int k_bits = nucleotide_to_bits(kmer_str[k]);

                        if ((q_bits & k_bits) == 0)
                        {
                            res = false;
                            break;
                        }
                    }
                }
                break;
            default:
                elog(ERROR, "unrecognized strategy number: %d",
                     in->scankeys[j].sk_strategy);
                res = false;
                break;
        }

        if (!res)
            break;              /* No need to consider remaining conditions */
    }

    PG_RETURN_BOOL(res);
}