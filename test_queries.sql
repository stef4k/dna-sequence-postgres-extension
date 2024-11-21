DROP EXTENSION IF EXISTS dna_sequence CASCADE;
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

CREATE TABLE kmers (kmer kmer);

INSERT INTO kmers (kmer) VALUES
('ACGT'),
('CGTA'),
('GTAC'),
('TACG'),
('ACGTA'),
('CGTAC'),
('GTACG'),
('TACGT'),
('ACGTAC'),
('CGTACG'),
('GTACGT'),
('TACGTA'),
('ACGTACG'),
('CGTACGT'),
('GTACGTA'),
('TACGTAC'),
('ACGTACGT'),
('CGTACGTA'),
('GTACGTAC'),
('TACGTACG'),
('GATC'),
('TCGA'),
('CAGT'),
('TGAC'),
('GATCA'),
('TCGAT'),
('CAGTC'),
('TGACT'),
('GATCAG'),
('TCGATC'),
('CAGTCA'),
('TGACTG'),
('GATCAGT'),
('TCGATCG'),
('CAGTCGA'),
('TGACTGC'),
('GATCAGTC'),
('TCGATCGA'),
('CAGTCGAC'),
('TGACTGCA'),
('GATCAGTCA'),
('TCGATCGTA'),
('CAGTCGACT'),
('TGACTGCAG'),
('ACGCGT'),
('CGCGTA'),
('GCGTAC'),
('TACGCG'),
('ACGCGTA'),
('CGCGTAC'),
('GCGTACG'),
('TACGCGT'),
('ACGTGCA'),
('TGCGTAC'),
('CGTGCGT'),
('GTACGCG'),
('TCGACGT'),
('AGTCGTA'),
('CGTAGTC'),
('GTCGATG'),
('GACTGCA'),
('CGTAGCT'),
('GCTAGCA'),
('TGACTGA'),
('GCTACGA'),
('GTACTGA'),
('CGTAGTA'),
('GTGACTA'),
('TACGATA'),
('CGTAGTG'),
('TAGCTAG'),
('CTAGCTA'),
('AGTACTA'),
('GACTGTA'),
('CGTAGTG'),
('TGACTGA'),
('TAGCTAC'),
('GTACTGA'),
('CAGTCGT'),
('GTACGTA'),
('CGTGATA'),
('GTCGACT'),
('TACGTCA'),
('CGTACGA'),
('TGACGTA'),
('GATCGAT'),
('CGTAGTG'),
('TCGACGA'),
('AGTCAGT'),
('TGACTGC'),
('CAGTACG'),
('TCGATCG'),
('GTACGCT'),
('ACGTGAC'),
('CGTAGCA'),
('GACTGTA'),
('TACGCGT'),
('TGACGCA');

SELECT *
FROM kmers
WHERE starts_with(kmer, 'ACGT');

SELECT *
FROM kmers
WHERE starts_with(kmer, 'ACGCCCCT'); -- it takes the shortest kmer in the table, this might need to be adjusted

SELECT *
FROM kmers
WHERE kmer ^@ 'ACGT';

SELECT *
FROM kmers
WHERE kmer ^@ 'ACGCCCCT';


-- Check GROUP BY and COUNT for kmer data type
SELECT k.kmer, count(*)
FROM generate_kmers('ATCGATCAC', 3) AS k(kmer)
GROUP BY k.kmer
ORDER BY count(*) DESC;

-- Check total, distinct and unique count of kmer sequence
WITH kmers AS (
SELECT k.kmer, count(*)
FROM generate_kmers('ATCGATCAC', 3) AS k(kmer)
GROUP BY k.kmer
)
SELECT sum(count) AS total_count,
count(*) AS distinct_count,
count(*) FILTER (WHERE count = 1) AS unique_count
FROM kmers;

-- Check GROUP BY and COUNT for canonical kmer data type
SELECT canonical(k.kmer), count(*)
FROM generate_kmers('ATCGATCAC', 3) AS k(kmer)
GROUP BY canonical(k.kmer)
ORDER BY count(*) DESC;

-- Check total, distinct and unique count of canonical kmer sequence
WITH canonical_kmers AS (
SELECT canonical(k.kmer), count(*)
FROM generate_kmers('ATCGATCAC', 3) AS k(kmer)
GROUP BY canonical(k.kmer)
)
SELECT sum(count) AS total_count,
count(*) AS distinct_count,
count(*) FILTER (WHERE count = 1) AS unique_count
FROM canonical_kmers;

-- Should return true
SELECT contains('ANGTA'::qkmer, 'AAGTA'::kmer);
SELECT 'ANGTA'::qkmer @> 'AAGTA'::kmer;

-- Should return false (R is A/G)
SELECT contains('ARGTA'::qkmer, 'ACGTA'::kmer);
SELECT 'ARGTA'::qkmer @> 'ACGTA'::kmer;

SELECT contains('ANGTA'::qkmer, 'ACGT'::kmer);   -- Error

SELECT contains('ANXTA'::qkmer, 'ACGTA'::kmer);  -- Error

DROP TABLE kmers;
CREATE TABLE kmers (kmer kmer);

INSERT INTO kmers (kmer) VALUES
('ACGCGTA'),
('CGCGTAC'),
('GCGTACG'),
('TACGCGT'),
('ACGTGCA'),
('TGCGTAC'),
('CGTGCGT'),
('GTACGCG'),
('TCGACGT'),
('AGTCGTA'),
('CGTAGTC'),
('GTCGATG'),
('GACTGCA'),
('CGTAGCT'),
('GCTAGCA'),
('TGACTGA'),
('GCTACGA'),
('GTACTGA'),
('CGTAGTA'),
('GTGACTA'),
('TACGATA'),
('CGTAGTG'),
('TAGCTAG'),
('CTAGCTA'),
('AGTACTA'),
('GACTGTA'),
('CGTAGTG'),
('TGACTGA'),
('TAGCTAC'),
('GTACTGA'),
('CAGTCGT'),
('GTACGTA'),
('CGTGATA'),
('GTCGACT'),
('TACGTCA'),
('CGTACGA'),
('TGACGTA'),
('GATCGAT'),
('CGTAGTG'),
('TCGACGA'),
('AGTCAGT'),
('TGACTGC'),
('CAGTACG'),
('TCGATCG'),
('GTACGCT'),
('ACGTGAC'),
('CGTAGCA'),
('GACTGTA'),
('TACGCGT'),
('TGACGCA');

SELECT * FROM kmers WHERE contains('GTACGHK', kmer);
SELECT * FROM kmers WHERE 'GTACGHK' @> kmer;