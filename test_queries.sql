DROP EXTENSION IF EXISTS dna_sequence;
CREATE EXTENSION dna_sequence;

-- check if dna type works
select dna_sequence('GATTACAaaaa');

-- check type
select pg_typeof(dna_sequence('GATTACAaaaa'));

-- check length of dna
select length(dna_sequence('aaaaaaGGGG'));

-- check type of qkmer
select pg_typeof(qkmer('AcgtWSMKRYBDHVN'));

-- check length of qkmer
select length(qkmer('AcgtWSMKRYBDHVN'));

-- check that error is thrown due to large string
select length(qkmer('AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA'));

-- test kmer creation
SELECT 'ACGTACGTACGTACGTACGTACGTACGTACGT'::kmer;
SELECT 'ACGTN'::kmer; -- Should fail due to invalid character 'N'
SELECT 'ACGTACGTACGTACGTACGTACGTACGTACGTA'::kmer; -- Should fail due to length > 32

-- test length function
SELECT length('ACGTACGT'::kmer);

-- Using the equals function
SELECT equals('ACGTA', 'ACGTA');
SELECT equals('ACGTA', 'ACGTA'::kmer);
SELECT 'ACGTA'::kmer = 'CCGTA';
SELECT 'ACGTA'::kmer = 'ACGTA';
SELECT not_equals('ACGTA', 'ACGTA');
SELECT 'ACGTA'::kmer <> 'ACCTA';

CREATE TABLE kmer_test (id serial PRIMARY KEY, kmer_col kmer);

INSERT INTO kmer_test (kmer_col)
VALUES('ACGTA'),('GATTC'),('ACCTA')

SELECT *
FROM kmer_test
WHERE kmer_col = 'ACGTA';

SELECT *
FROM kmer_test
WHERE  not_equals(kmer_col, 'ACGTA');

-- Test generate_kmers (based on example)
-- from Wikipedia
-- k-mers for GTAGAGCTGT
-- k	k-mers
-- 1	G, T, A, C
-- 2	GT, TA, AG, GA, AG, GC, CT, TG
-- 3	GTA, TAG, AGA, GAG, AGC, GCT, CTG, TGT
-- 4	GTAG, TAGA, AGAG, GAGC, AGCT, GCTG, CTGT
-- 5	GTAGA, TAGAG, AGAGC, GAGCT, AGCTG, GCTGT
-- 6	GTAGAG, TAGAGC, AGAGCT, GAGCTG, AGCTGT
-- 7	GTAGAGC, TAGAGCT, AGAGCTG, GAGCTGT
-- 8	GTAGAGCT, TAGAGCTG, AGAGCTGT
-- 9	GTAGAGCTG, TAGAGCTGT
-- 10	GTAGAGCTGT

-- TODO: REMOVE DUPLICATION
SELECT k.kmer
FROM generate_kmers('GTAGAGCTGT', 6) AS k(kmer);