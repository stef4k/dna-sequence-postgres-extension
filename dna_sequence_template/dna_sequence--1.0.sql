-- complain if script is sourced in psql, rather than via CREATE EXTENSION
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
    storage = extended, -- to compress and store large DNA sequences efficiently
    category = 'S', -- to classify it as a string-like type
    preferred = true
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
    storage = extended, -- to compress and store efficiently
    category = 'S' -- classify as string-like type
);

/*qkmer***********************************************************************/

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
    storage = extended, -- to compress and store large QKMER sequences efficiently
    category = 'S' -- to classify it as a string-like type
);

/******************************************************************************
 * Length functions
 ******************************************************************************/

-- Define the length function for dna_sequence
CREATE FUNCTION length(dna_sequence)
    RETURNS integer AS 'MODULE_PATHNAME', 'dna_sequence_length' 
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Define the length function for kmer
CREATE FUNCTION length(kmer)
    RETURNS integer AS 'MODULE_PATHNAME', 'kmer_length' 
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Define the length function for qkmer
CREATE FUNCTION length(qkmer)
    RETURNS integer AS 'MODULE_PATHNAME', 'qkmer_length' 
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/******************************************************************************
 * Constructor
 ******************************************************************************/



/******************************************************************************
 * Operators
 ******************************************************************************/



/******************************************************************************/


/******************************************************************************/