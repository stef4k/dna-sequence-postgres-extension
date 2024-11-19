-- Complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION dna_sequence" to load this file. \quit

/******************************************************************************
 * Input/Output
 ******************************************************************************/

/*dna_sequence*****************************************************************/

-- Function to convert text to dna_sequence
CREATE OR REPLACE FUNCTION dna_in(cstring)
    RETURNS dna_sequence
    AS 'MODULE_PATHNAME', 'dna_in'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Function to convert dna_sequence to text
CREATE OR REPLACE FUNCTION dna_out(dna_sequence)
    RETURNS cstring
    AS 'MODULE_PATHNAME', 'dna_out'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Define the new dna_sequence type
CREATE TYPE dna_sequence (
    internallength = VARIABLE,
    input = dna_in,
    output = dna_out,
    category = 'S', -- to classify it as a string-like type
    preferred = true
);

/* Cast Function ****************************************************************/

-- Conversion function from text to dna_sequence
CREATE OR REPLACE FUNCTION dna_sequence(text)
    RETURNS dna_sequence
    AS 'MODULE_PATHNAME', 'dna_in'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Create an implicit cast to allow automatic conversion from text to dna_sequence
CREATE CAST (text AS dna_sequence) WITH FUNCTION dna_sequence(text) AS IMPLICIT;

/******************************************************************************
 * Length functions
 ******************************************************************************/

-- Define the length function for dna_sequence
CREATE FUNCTION length(dna_sequence)
    RETURNS integer AS 'MODULE_PATHNAME', 'dna_sequence_length' 
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/******************************************************************************
 * Equality functions for dna_sequence
 ******************************************************************************/

-- Define a function to compare two dna_sequence values for equality
CREATE FUNCTION dna_sequence_equals(dna_sequence, dna_sequence)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'dna_sequence_equals'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Create the '=' operator for dna_sequence type
CREATE OPERATOR = (
    LEFTARG = dna_sequence,
    RIGHTARG = dna_sequence,
    PROCEDURE = dna_sequence_equals,
    COMMUTATOR = '=',
    NEGATOR = '<>',
    HASHES,
    MERGES
);

/*kmer************************************************************************/

-- Function to convert text to kmer
CREATE OR REPLACE FUNCTION kmer_in(cstring)
    RETURNS kmer
    AS 'MODULE_PATHNAME', 'kmer_in'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Function to convert kmer to text
CREATE OR REPLACE FUNCTION kmer_out(kmer)
    RETURNS cstring
    AS 'MODULE_PATHNAME', 'kmer_out'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Define the new kmer type
CREATE TYPE kmer (
    internallength = VARIABLE,
    input = kmer_in,
    output = kmer_out,
    category = 'S' -- classify as string-like type
);

/******************************************************************************
 * Equality functions for kmer
 ******************************************************************************/

-- Function to compare two kmers for equality
CREATE FUNCTION kmer_equals(kmer, kmer)
    RETURNS boolean AS 'MODULE_PATHNAME', 'kmer_equals'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Function to compare two kmers for inequality
CREATE FUNCTION kmer_not_equals(kmer, kmer)
    RETURNS boolean AS 'MODULE_PATHNAME', 'kmer_not_equals'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Create the '=' operator for kmer type
CREATE OPERATOR = (
    LEFTARG = kmer,
    RIGHTARG = kmer,
    PROCEDURE = kmer_equals,
    COMMUTATOR = '=',
    NEGATOR = '<>',
    HASHES, --this operator supports hash joins and hashing operations - needed for HASH index
    MERGES --this operator can be used in merge joins - needed for HASH index
);

-- Create the '<>' operator for kmer type
CREATE OPERATOR <> (
    LEFTARG = kmer,
    RIGHTARG = kmer,
    PROCEDURE = kmer_not_equals,
    COMMUTATOR = '<>',
    NEGATOR = '='
);

/* qkmer **********************************************************************/

-- Function to convert text to qkmer
CREATE OR REPLACE FUNCTION qkmer_in(cstring)
    RETURNS qkmer
    AS 'MODULE_PATHNAME', 'qkmer_in'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Function to convert qkmer to text
CREATE OR REPLACE FUNCTION qkmer_out(qkmer)
    RETURNS cstring
    AS 'MODULE_PATHNAME', 'qkmer_out'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Define the new qkmer type
CREATE TYPE qkmer (
    internallength = VARIABLE,
    input = qkmer_in,
    output = qkmer_out,
    category = 'S' -- to classify it as a string-like type
);

/******************************************************************************
 * Set-Returning Function: generate_kmers
 ******************************************************************************/

-- First option from the docs is used
CREATE OR REPLACE FUNCTION generate_kmers(dna_sequence, integer)
    RETURNS SETOF kmer
    AS 'MODULE_PATHNAME', 'generate_kmers'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/******************************************************************************
 * Additional Operator Definitions
 ******************************************************************************/

-- Function to cast text to kmer
CREATE FUNCTION kmer(text)
    RETURNS kmer
    AS 'MODULE_PATHNAME', 'kmer_cast_text'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Create an implicit cast from text to kmer
CREATE CAST (text AS kmer) WITH FUNCTION kmer(text) AS IMPLICIT;

-- Add operators for qkmer contains kmer
CREATE FUNCTION contains(qkmer, kmer)
    RETURNS boolean AS 'MODULE_PATHNAME', 'contains_qkmer_kmer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Create the '@>' operator for qkmer type
CREATE OPERATOR @> (
    LEFTARG = qkmer,
    RIGHTARG = kmer,
    PROCEDURE = contains,
    COMMUTATOR = '<@'
);

/******************************************************************************
 * Hash function for dna_sequence
 ******************************************************************************/

-- Define a hash function for dna_sequence type
CREATE FUNCTION dna_sequence_hash(dna_sequence) RETURNS integer
    AS 'MODULE_PATHNAME', 'dna_sequence_hash'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Register the dna_sequence type as hashable with a hash operator class
CREATE OPERATOR CLASS dna_sequence_hash_ops DEFAULT FOR TYPE dna_sequence USING hash AS
    OPERATOR 1 = (dna_sequence, dna_sequence),  -- Equality operator
    FUNCTION 1 dna_sequence_hash(dna_sequence); -- Hash function

/******************************************************************************
 * SP-GiST Functions for dna_sequence Indexing
 ******************************************************************************/

-- Create SP-GiST functions and operator class for dna_sequence

CREATE FUNCTION spg_config(internal, internal)
    RETURNS void
    AS 'MODULE_PATHNAME', 'spg_config'
    LANGUAGE c STRICT;

CREATE FUNCTION spg_choose(internal, internal)
    RETURNS void
    AS 'MODULE_PATHNAME', 'spg_choose'
    LANGUAGE c STRICT;

CREATE FUNCTION spg_picksplit(internal, internal)
    RETURNS void
    AS 'MODULE_PATHNAME', 'spg_picksplit'
    LANGUAGE c STRICT;

CREATE FUNCTION spg_inner_consistent(internal, internal)
    RETURNS void
    AS 'MODULE_PATHNAME', 'spg_inner_consistent'
    LANGUAGE c STRICT;

CREATE FUNCTION spg_leaf_consistent(internal, internal)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'spg_leaf_consistent'
    LANGUAGE c STRICT;

-- Create SP-GiST operator class for dna_sequence
CREATE OPERATOR CLASS dna_sequence_spgist_ops
    DEFAULT FOR TYPE dna_sequence USING spgist AS
    FUNCTION 1 spg_config(internal, internal),
    FUNCTION 2 spg_choose(internal, internal),
    FUNCTION 3 spg_picksplit(internal, internal),
    FUNCTION 4 spg_inner_consistent(internal, internal),
    FUNCTION 5 spg_leaf_consistent(internal, internal);

    /******************************************************************************
 * Hash Operator Class for kmer
 ******************************************************************************/

 CREATE FUNCTION kmer_hash(kmer)
    RETURNS integer
    AS 'MODULE_PATHNAME', 'kmer_hash'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Define a hash operator class for kmer
CREATE OPERATOR CLASS kmer_hash_ops DEFAULT FOR TYPE kmer USING hash AS
    OPERATOR 1 =(kmer, kmer),  -- Equality operator
    FUNCTION 1 kmer_hash(kmer); -- Hash function

