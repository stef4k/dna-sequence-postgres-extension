#include "postgres.h"
#include "fmgr.h"
#include "access/spgist.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "catalog/pg_type.h"


void spg_kmer_config(spgConfigIn *in, spgConfigOut *out) {
    out->prefixType = TEXTOID;  // Prefix type: text
    out->labelType = TEXTOID;  // Node label type: text
    out->leafType = TEXTOID;   // Leaf type: text (k-mer)
    out->canReturnData = true; // Index can reconstruct the original k-mer
    out->longValuesOK = false; // No long values for now
}


void spg_kmer_choose(spgChooseIn *in, spgChooseOut *out) {
    const char *kmer = TextDatumGetCString(in->datum);

    // Example: Choose node based on the first character of the k-mer
    char first_char = kmer[0];

    out->resultType = spgMatchNode; // Specify that we match a node
    out->result.matchNode.nodeN = first_char - 'A'; // Map 'A' to node 0, etc.
    out->result.matchNode.restDatum = in->datum;    // Pass the datum as-is
}


void spg_kmer_picksplit(spgPickSplitIn *in, spgPickSplitOut *out) {
    int nTuples = in->nTuples;

    // Example: Group k-mers by first character
    out->nNodes = 26; // Assuming A-Z
    out->mapTuplesToNodes = palloc(sizeof(int) * nTuples);

    for (int i = 0; i < nTuples; i++) {
        const char *kmer = TextDatumGetCString(in->datums[i]);
        out->mapTuplesToNodes[i] = kmer[0] - 'A'; // Map to node index
    }

    out->nodeLabels = NULL; // No specific labels
    out->leafTupleDatums = in->datums; // Pass leaf data as-is
}

void spg_kmer_inner_consistent(spgInnerConsistentIn *in, spgInnerConsistentOut *out) {
    out->nNodes = 1; // Example: Always visit one node
    out->nodeNumbers = palloc(sizeof(int) * 1);
    out->nodeNumbers[0] = 0; // Adjust logic to match your tree structure
}

bool spg_kmer_leaf_consistent(spgLeafConsistentIn *in, spgLeafConsistentOut *out) {
    const char *leaf = TextDatumGetCString(in->leafDatum);
    const char *query = TextDatumGetCString(in->scankeys[0].sk_argument);

    return strcmp(leaf, query) == 0; // Example: Exact match
}
