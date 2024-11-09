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
 * Equality functions
 ******************************************************************************/

 -- Function to compare two kmers for equality
CREATE FUNCTION equals(kmer, kmer)
    RETURNS boolean AS 'MODULE_PATHNAME', 'kmer_equals'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Function to compare two kmers for inequality
CREATE FUNCTION not_equals(kmer, kmer)
    RETURNS boolean AS 'MODULE_PATHNAME', 'kmer_not_equals'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Create the '=' operator for kmer type
CREATE OPERATOR = (
    LEFTARG = kmer,
    RIGHTARG = kmer,
    PROCEDURE = equals,
    COMMUTATOR = =,
    NEGATOR = <>,
    HASHES, --this operator supports hash joins and hashing operations
    MERGES --this operator can be used in merge joins
);

-- Create the '<>' operator for kmer type
CREATE OPERATOR <> (
    LEFTARG = kmer,
    RIGHTARG = kmer,
    PROCEDURE = not_equals,
    COMMUTATOR = <>,
    NEGATOR = =
);

-- Function to cast text to kmer
CREATE FUNCTION kmer(text)
    RETURNS kmer
    AS 'MODULE_PATHNAME', 'kmer_cast_text'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Create an implicit cast (a cast that the database server can invoke automatically when 
-- it encounters data types that cannot be compared with built-in casts) from text to kmer
-- Needed for the '=' and '<>' operators to work with text
CREATE CAST (text AS kmer) WITH FUNCTION kmer(text) AS IMPLICIT;

/******************************************************************************
 * Set-Returning Function: generate_kmers
 ******************************************************************************/

-- First option from the docs is used
CREATE OR REPLACE FUNCTION generate_kmers(dna_sequence, integer)
    RETURNS SETOF kmer
    AS 'MODULE_PATHNAME', 'generate_kmers'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


/******************************************************************************
 * Starts_with Function
 ******************************************************************************/

-- !! Order in starts_with() is reversed compared to the requirement, otherwise it can't be used as an operator
CREATE FUNCTION starts_with(kmer, kmer)
    RETURNS boolean AS 'MODULE_PATHNAME', 'kmer_starts_with'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- '^@' operator for kmer type
CREATE OPERATOR ^@ (
    LEFTARG = kmer,
    RIGHTARG = kmer,
    PROCEDURE = starts_with,
    COMMUTATOR = @^
);
-- We already have an implicit cast from text to kmer, so that doen't need to be handled here
-- (more info on operators: https://www.postgresql.org/docs/current/sql-createoperator.html)
-- Something to look into in the future: CREATE OPERATOR CLASS (https://www.postgresql.org/docs/current/sql-createopclass.html)

/******************************************************************************
 * A custom hash function allowing hash-based indexes and hash joins
 in order to implement group by, DISTINCT and COUNT
 SHOULD BE UPDATED WHEN SP-GIST IS IMPLEMENTED
 ******************************************************************************/

-- Create hash function for kmer
CREATE FUNCTION kmer_hash(kmer) RETURNS integer
    AS 'MODULE_PATHNAME', 'kmer_hash'
    LANGUAGE C IMMUTABLE STRICT;

-- Register the kmer type as hashable
CREATE OPERATOR CLASS kmer_hash_ops DEFAULT FOR TYPE kmer USING hash AS
    OPERATOR 1 = (kmer, kmer),
    FUNCTION 1 kmer_hash(kmer);



/******************************************************************************
 * Operators
 ******************************************************************************/



/******************************************************************************/


/******************************************************************************/