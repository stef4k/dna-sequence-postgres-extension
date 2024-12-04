DROP EXTENSION IF EXISTS dna_sequence CASCADE;
CREATE EXTENSION dna_sequence;

/*
=======================
INPUT/OUTPUT FUNCTIONS
=======================
*/
-- **** DNA SEQUENCE ****
SELECT 	DNA_SEQUENCE('ACAAAGTGGTAAATTAGTGTAAGATAGTGGTATTAGATGATGGATTAGATGA'),
		LENGTH(DNA_SEQUENCE('ACAAAGTGGTAAATTAGTGTAAGATAGTGGTATTAGATGATGGATTAGATGA'))
FROM 	DNAS D
LIMIT 1;

/*	---Output---
	dna_sequence											length
	"ACAAAGTGGTAAATTAGTGTAAGATAGTGGTATTAGATGATGGATTAGATGA"	52
*/

-- INVALID SEQUENCE
SELECT 	DNA_SEQUENCE('ACAAAGTGGTAAATTAGTGTAAGATAGTGGTATTAGAFGATGGATTAGATGA'),
		LENGTH(DNA_SEQUENCE('ACAAAGTGGTAAATTAGTGTAAGATAGTGGFATTAGATGATGGATTAGATGA'))
FROM 	DNAS D
LIMIT 1;

/*  ---Output---
	ERROR:  Invalid input: only 'A', 'C', 'G', 'T' characters are allowed
	LINE 2: SELECT  DNA_SEQUENCE('ACAAAGTGGTAAATTAGTGTAAGATAGTGGTATTAGAF...
	                             ^ 	
	SQL state: XX000
	Character: 42
*/


-- **** KMER SEQUENCE ****
SELECT 	KMER('ACAAAGTGGTAAATTAGTGTAAATGGATAATT'),
		LENGTH(KMER('ACAAAGTGGTAAATTAGTGTAAATGGATAATT'))
FROM 	KMERS K
LIMIT 1;

/*  ---Output---
	kmer								length	
	"ACAAAGTGGTAAATTAGTGTAAATGGATAATT"	32
*/

-- INVALID LENGTH
SELECT 	KMER('ACAAAGTGGTAAATTAGTGTAAATGGATAATTA'),
		LENGTH(KMER('ACAAAGTGGTAAATTAGTGTAAATGGATAATTA'))
FROM 	KMERS K
LIMIT 1;

/*	---Output---
	ERROR:  Input exceeds maximum length of 32
	LINE 2: SELECT  KMER('ACAAAGTGGTAAATTAGTGTAAATGGATAATTA'),
	                     ^ 
	SQL state: 22001
	Character: 32
*/

-- INVALID CHARACTERS
SELECT 	KMER('ACAAAGTGGTAAATTAGTGTAAATGGATTFS'),
		LENGTH(KMER('ACAAAGTGGTAAATTAGTGTAAATGGAATDF'))
FROM 	KMERS K
LIMIT 1;
/*  ---Output---
	ERROR:  Invalid input: only 'A', 'C', 'G', 'T' characters are allowed and length must be <= 32
	LINE 2: SELECT  KMER('ACAAAGTGGTAAATTAGTGTAAATGGATTFS'),
	                     ^ 
	SQL state: 22P02
	Character: 36
*/


-- **** QKMER SEQUENCE ****
SELECT 	QKMER('ACAAABTGRTSACDVTTGWHAKYMNARTATAR'),
		LENGTH(QKMER('ACAAABTGRTSACDVTTGWHAKYMNARTATAR'))
FROM 	QKMERS Q
LIMIT 1;
/*  ---Output---
	qkmer								length
	"ACAAABTGRTSACDVTTGWHAKYMNARTATAR"	32
*/

-- INVALID LENGTH
SELECT 	KMER('ACAAAGTGGTSAAATTAWAATYGGAVTATATTA'),
		LENGTH(KMER('ACAAAGTGGTSAAATTAWAATYGGAVTATATTA'))
FROM 	QKMERS Q
LIMIT 1;
/* 	--Output---
	ERROR:  Input exceeds maximum length of 32
	LINE 2: SELECT  KMER('ACAAAGTGGTSAAATTAWAATYGGAVTATATTA'),
	                     ^ 
	SQL state: 22001
	Character: 32
*/

-- INVALID CHARACTERS
SELECT 	QKMER('ACAAAGTGGTSAAATTAWAATYGGPVTATATT'),
		LENGTH(QKMER('ACAAAGTGGTSAAATTAWAATYGGPVTATATT'))
FROM 	QKMERS Q
LIMIT 1;
/*	---Output---
	ERROR:  Invalid input: only 'A', 'B', 'C', 'D', 'G', 'H', 'K', 'M', 'N', 'R', 'S', 'T', 'V', 'W', 'Y' characters are allowed
	LINE 2: SELECT  QKMER('ACAAAGTGGTSAAATTAWAATYGGPVTATATT'),
	                      ^ 
	SQL state: XX000
	Character: 37
*/


/*
=======================
Length Functions
=======================
*/
SELECT	D.*, 
		LENGTH(D.DNA_SEQUENCE) AS SEQ_LENGTH
FROM	DNAS D 
WHERE 	LENGTH(DNA_SEQUENCE) <= 33
LIMIT 5;
/*	---Output---
	dna_sequence							seq_length
	"TCAGGCAAGAAGTAAACAGCTATTGAATGTCTC"	33
	"TTCTTGTCTGAATAATAAAAGTTCTGCAAACAG"	33
	"CTGTCATTGCAAACTTGACTTATACCATACTAA"	33
	"TGACTTCCTTTACCCAATGGGGTATTAGCAAGT"	33
	"ATTCAAGTCACAGAGTTGAATATTCCCTTTCAC"	33
*/

SELECT	K.*, 
		LENGTH(K.KMER) AS SEQ_LENGTH
FROM	KMERS K 
WHERE 	LENGTH(K.KMER) <= 10
LIMIT 5;
/*	---Output---
	kmer			seq_length
	"CCTCCT"		6
	"AGGAAAATAC"	10
	"GGGGAA"		6
	"AGAGAAAAT"		9
	"GGGTGCTGGA"	10
*/

SELECT	Q.*, 
		LENGTH(Q.QKMER) AS SEQ_LENGTH
FROM	QKMERS Q
LIMIT 5;
/*	---Output---
	qkmer							seq_length
	"ANATGTTCATA"					11
	"AGATCATAGCAGGGGTGGAAATCNC"		25
	"AGTCTCACTCN"					11
	"TGGTTTANAAGGAAGGTTTTAGAATGAA"	28
	"TTGTGTTGCTGAGCGGGCTGGCAAGGCN"	28
*/

/*
=======================
Equality Functions
=======================
*/
SELECT 	K1.KMER KMER_1, 
		K2.KMER KMER_2,
		(K1.KMER = K2.KMER) ARE_EQUAL, 
		(K1.KMER <> K2.KMER) ARE_NOT_EQUAL
FROM KMERS K1, KMERS K2
LIMIT 5;
/*	---Output---
	kmer_1					kmer_2								are_equal	are_not_equal
	"GGGGTGGCTGCAGTTCCC"	"GGGGTGGCTGCAGTTCCC"				true		false
	"GGGGTGGCTGCAGTTCCC"	"ATATGCATTTATGTGTATATATATAT"		false		true
	"GGGGTGGCTGCAGTTCCC"	"GGGGGCATTTGGAATGTT"				false		true
	"GGGGTGGCTGCAGTTCCC"	"CACATTCTGGATAGTTCTCTCCAGTTCT"		false		true
	"GGGGTGGCTGCAGTTCCC"	"GCATCTTTATAGACTAAGAATAAATGATCC"	false		true

*/


/*
=======================
KMER Generation
=======================
*/
SELECT 	*
FROM 	GENERATE_KMERS((SELECT * FROM DNAS LIMIT 1), 32)
LIMIT 5;
/*	---Output---
	generate_kmers
	"ATTCTCCTAGCCTACATCCGTACGAGTTAGCG"
	"TTCTCCTAGCCTACATCCGTACGAGTTAGCGT"
	"TCTCCTAGCCTACATCCGTACGAGTTAGCGTG"
	"CTCCTAGCCTACATCCGTACGAGTTAGCGTGG"
	"TCCTAGCCTACATCCGTACGAGTTAGCGTGGG"
*/

-- Over the length of allowed KMERs
SELECT 	*
FROM 	GENERATE_KMERS((SELECT * FROM DNAS LIMIT 1), 33);
/*	---Output---
	ERROR:  Invalid k: must be between 1 and the length of the DNA sequence 
	SQL state: XX000
*/

-- OVER THE LENGTH OF THE DNA_SEQUENCE
SELECT 	*
FROM 	GENERATE_KMERS((SELECT * FROM DNAS LIMIT 1), 300);
/*	---Output---
	ERROR:  Invalid k: must be between 1 and the length of the DNA sequence 
	SQL state: XX000
*/


/*
=======================
Starts With Function
=======================
*/
-- Find all Kmers that start with "ATTCT"
SELECT *
FROM KMERS
WHERE KMER^@'ATTCT' --Implicit Kmer casting
LIMIT 5;
/*	---Output---
	kmer
	"ATTCTGAGATACTATTCT"
	"ATTCTGCTACATAGGTTCTGAT"
	"ATTCTCTAAAGCC"
	"ATTCTCCATGAC"
	"ATTCTCAGTAACTTCCTTGTGTTGTGTGTA"
*/

-- Find all Kmers that compose ATTCTATAGTGTA
SELECT *
FROM KMERS
WHERE 'ATTCTATAGTGTA'^@KMER
LIMIT 5;
/*	---Output---
	kmer
	"ATTCTATAGTGTA"
	"ATTC"
	"ATTCT"
	"ATTCTA"
	"ATTCTAT"
*/


/*
=======================
Contains Function
=======================
*/
SELECT *
FROM QKMERS
WHERE 'ATTCTATAGTGTA' <@ QKMER
LIMIT 5;
/*	---Output---
	qkmer
	"ATNNNANNNNNNN"
*/

SELECT QKMER, 'AAATGTTCATA' AS KMER, QKMER @> 'AAATGTTCATA' AS CONTAINED, 'AAATGTTCATA' <@ QKMER AS CONTAINS
FROM QKMERS
LIMIT 5;
/*	---Output---
	qkmer							kmer			contained	contains
	"ANATGTTCATA"					"AAATGTTCATA"	true		true
	"AGATCATAGCAGGGGTGGAAATCNC"		"AAATGTTCATA"	false		false
	"AGTCTCACTCN"					"AAATGTTCATA"	false		false
	"TGGTTTANAAGGAAGGTTTTAGAATGAA"	"AAATGTTCATA"	false		false
	"TTGTGTTGCTGAGCGGGCTGGCAAGGCN"	"AAATGTTCATA"	false		false
*/

SELECT QKMER, 'AGTCTCACTCA' AS KMER, CONTAINS(QKMER, 'AGTCTCACTCA') AS WITH_FUNC, QKMER @>'AGTCTCACTCA' AS WITH_OPERATOR
FROM QKMERS
LIMIT 5;
/*	---Output---
	qkmer							kmer			with_func 	with_operator
	"ANATGTTCATA"					"AGTCTCACTCA"	false		false
	"AGATCATAGCAGGGGTGGAAATCNC"		"AGTCTCACTCA"	false		false
	"AGTCTCACTCN"					"AGTCTCACTCA"	true		true
	"TGGTTTANAAGGAAGGTTTTAGAATGAA"	"AGTCTCACTCA"	false		false
	"TTGTGTTGCTGAGCGGGCTGGCAAGGCN"	"AGTCTCACTCA"	false		false
*/

/*
=======================
Canonical Function
=======================
*/
SELECT 	KMER, 
		CANONICAL(KMER) AS CANONICAL_KMER
FROM 	KMERS K
LIMIT 5;
/*	---Output---
	kmer								canonical_kmer
	"CAAAGTG"							"CAAAGTG"
	"CTAAAGTTTATAAG"					"CTAAAGTTTATAAG"
	"TTTGTCTAAATGACCGCCGGAATCCCAGCAA"	"TTGCTGGGATTCCGGCGGTCATTTAGACAAA"
	"TAACAGATGCTCAATAAGGTT"				"AACCTTATTGAGCATCTGTTA"
	"TGCTCTATGAATGGGAA"					"TGCTCTATGAATGGGAA"
*/

-- KMERS where their Canonical Kmer is lexicographically larger than the original
SELECT KMER, CANONICAL(KMER)
FROM KMERS K
WHERE CANONICAL(KMER) = KMER
LIMIT 5;
/*	---Output---
	kmer							canonical
	"CAAAGTG"						"CAAAGTG"
	"CTAAAGTTTATAAG"				"CTAAAGTTTATAAG"
	"TGCTCTATGAATGGGAA"				"TGCTCTATGAATGGGAA"
	"ACTTTGCATTATAATTATCTTTTAC"		"ACTTTGCATTATAATTATCTTTTAC"
	"AACGGGCCTAA"					"AACGGGCCTAA"
*/


/*
====================
INDEX
====================
*/
DROP TABLE IF EXISTS kmers;

CREATE TABLE kmers (
    kmer kmer
);

COPY public.kmers (kmer) FROM '/mnt/d/unique_dna_kmers.csv' DELIMITER ',' CSV;
/*	---Output---
	COPY 29355895
	Query returned successfully in 19 secs 201 msec.
*/

SELECT 	count(*) AS exact_count 
FROM 	kmers;
/*	---Output---
	exact_count
	29355895 (29,355,895)
*/

SELECT 	kmer, 
		count(*) as nr
FROM 	kmers
GROUP BY kmer
ORDER by nr DESC
LIMIT 3;
/*	---Output---
	kmer
	"TGTCTGCTTT"				1
	"TAATGACTCTATGAGGAGATAT"	1
	"GACTTAAGCCTCAGAGTGTTCTTC"	1

	---------------------------------------------------------------------------------------
	All values are unique (see the unique_dna_kmers_generation.py in the repo for details)
*/

CREATE INDEX kmers_spgist_idx ON kmers USING spgist (kmer spgist_kmer_ops);
/*	---Output---
	CREATE INDEX
	Query returned successfully in 1 min 51 secs.
*/

SELECT 	i.relname "Table Name",
		indexrelname "Index Name",
		pg_size_pretty(pg_total_relation_size(relid)) As "Total Size",
		pg_size_pretty(pg_indexes_size(relid)) as "Total Size of all Indexes",
		pg_size_pretty(pg_relation_size(relid)) as "Table Size",
		pg_size_pretty(pg_relation_size(indexrelid)) "Index Size",
		reltuples::bigint "Estimated table row count"
FROM 	pg_stat_all_indexes i 
JOIN 	pg_class c ON i.relid=c.oid 
WHERE 	i.relname='kmers';
/*	---Output---
	"Table Name"	Index Name			Total Size	Total Size of 	Table Size	Index Size	Estimated table row count
													all Indexes					
	"kmers"			"kmers_spgist_idx"	"2840 MB"	"1253 MB"		"1586 MB"	"1253 MB"	29355896
*/ 

SET enable_seqscan = ON;
SET enable_indexscan = OFF;
SET enable_bitmapscan = OFF; -- avoid bitmapscan on the index
SET max_parallel_workers_per_gather = 0; -- avoid parallel seq scan
VACUUM ANALYZE kmers;
/*	---Output---
	VACUUM
	Query returned successfully in 1 secs 85 msec.
*/

ANALYZE VERBOSE kmers;
/*	---Output---
	INFO:  "kmers": scanned 30000 of 203007 pages, 
			containing 4336050 live rows and 0 dead rows; 
			30000 rows in sample, 29341617 estimated total rows
*/


/*
=======================
Equality searches 
=======================
*/
SET enable_indexscan = OFF;
EXPLAIN ANALYZE VERBOSE
SELECT 	* 
FROM 	kmers 
WHERE 	kmer = 'acgt';
/*	---Output---
	"QUERY PLAN"
	"Seq Scan on public.kmers  (cost=0.00..569777.20 rows=1 width=24) (actual time=0.209..931.894 rows=1 loops=1)"
	"  Output: kmer"
	"  Filter: (kmers.kmer = 'ACGT'::kmer)"
	"  Rows Removed by Filter: 29355894"
	"Planning Time: 0.039 ms"
	"Execution Time: 931.905 ms"
*/

SELECT 	* 
FROM 	kmers 
WHERE 	kmer = 'acgt';
/*	---Output---
	kmer
	"ACGT"
*/

SET enable_indexscan = ON;
EXPLAIN ANALYZE VERBOSE
SELECT 	* 
FROM 	kmers 
WHERE 	kmer = 'acgt';
/*	---Output---
	"QUERY PLAN"
	"Index Only Scan using kmers_spgist_idx on public.kmers  (cost=0.42..1.54 rows=1 width=24) (actual time=0.027..0.027 rows=1 loops=1)"
	"  Output: kmer"
	"  Index Cond: (kmers.kmer = 'ACGT'::kmer)"
	"  Heap Fetches: 0"
	"Planning Time: 0.046 ms"
	"Execution Time: 0.039 ms"
*/

SELECT 	* 
FROM 	kmers 
WHERE 	kmer = 'acgt';
/*	---Output---
	"kmer"
	"ACGT"
*/

/* Prefix searches */
SET enable_indexscan = OFF;
EXPLAIN ANALYZE VERBOSE
SELECT 	* 
FROM 	kmers 
WHERE 	kmer ^@ 'acgtaa';
/*	---Output---
	"QUERY PLAN"
	"Seq Scan on public.kmers  (cost=0.00..569777.20 rows=14670808 width=24) (actual time=0.363..993.649 rows=1818 loops=1)"
	"  Output: kmer"
	"  Filter: (kmers.kmer ^@ 'ACGTAA'::kmer)"
	"  Rows Removed by Filter: 29354077"
	"Planning Time: 0.037 ms"
	"Execution Time: 993.920 ms"
*/

SELECT 	* 
FROM 	kmers 
WHERE 	kmer ^@ 'acgtaa';
/*	---Output---
	"kmer"
	"ACGTAAGAAAA"
	"ACGTAAAAAG"
	"ACGTAAGTA"
	...
	Total rows: 1000 of 1818
*/

SET enable_indexscan = ON;
EXPLAIN ANALYZE VERBOSE
SELECT 	* 
FROM 	kmers 
WHERE 	kmer ^@ 'acgtaa';
/*	---Output---
	"QUERY PLAN"
	"Index Only Scan using kmers_spgist_idx on public.kmers  (cost=0.42..344970.56 rows=14670808 width=24) (actual time=0.031..0.306 rows=1818 loops=1)"
	"  Output: kmer"
	"  Index Cond: (kmers.kmer ^@ 'ACGTAA'::kmer)"
	"  Heap Fetches: 0"
	"Planning Time: 0.043 ms"
	"Execution Time: 0.351 ms"
*/

SELECT 	* 
FROM 	kmers 
WHERE 	kmer ^@ 'acgtaa';
/*	---Output---
	"kmer"
	"ACGTAA"
	"ACGTAACCA"
	"ACGTAACCAATTGC"
	...
	Total rows: 1000 of 1818
*/

/* Pattern matching using qkmer */
SET enable_indexscan = OFF;
EXPLAIN ANALYZE VERBOSE
SELECT 	* 
FROM 	kmers 
WHERE 	kmer <@ 'nttnn';
/*	---Output---
	"QUERY PLAN"
	"Seq Scan on public.kmers  (cost=0.00..569777.20 rows=29342 width=24) (actual time=0.601..963.417 rows=64 loops=1)"
	"  Output: kmer"
	"  Filter: (kmers.kmer <@ 'NTTNN'::qkmer)"
	"  Rows Removed by Filter: 29355831"
	"Planning Time: 0.042 ms"
	"Execution Time: 963.435 ms"
*/

SELECT 	* 
FROM 	kmers 
WHERE 	kmer <@ 'nttnn';
/*	---Output---
	"kmer"
	"GTTCT"
	"GTTTC"
	"TTTCC"
	"GTTAG"
	"ATTTG"
	...
	Total rows: 64 of 64
*/

SET enable_indexscan = ON;
EXPLAIN ANALYZE VERBOSE
SELECT 	* 
FROM 	kmers 
WHERE 	kmer <@ 'nttnn';
/*	---Output---
	"QUERY PLAN"
	"Index Only Scan using kmers_spgist_idx on public.kmers  (cost=0.42..691.00 rows=29342 width=24) (actual time=0.042..0.156 rows=64 loops=1)"
	"  Output: kmer"
	"  Index Cond: (kmers.kmer <@ 'NTTNN'::qkmer)"
	"  Heap Fetches: 0"
	"Planning Time: 0.043 ms"
	"Execution Time: 0.170 ms"
*/

SELECT 	* 
FROM 	kmers 
WHERE 	kmer <@ 'nttnn';
/*	---Output---
	"kmer"
	"ATTCT"
	"ATTCG"
	"ATTGC"
	"ATTTT"
	"ATTTG"
	...
	Total rows: 64 of 64
*/

/* More complex example using ^@, <@, = */
SET enable_indexscan = OFF;
EXPLAIN ANALYZE VERBOSE
SELECT kmer FROM kmers WHERE kmer ^@ 'GCTAGCTAAGCT'
UNION
SELECT kmer FROM kmers WHERE kmer <@ 'CGTTAAGCTTGACCGT'
UNION
SELECT kmer FROM kmers WHERE kmer = 'GATTACAGCTAGCTTGA'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'ACGTAGCTTGAACCGT' AND kmer <@ 'TGACCGTTGACTTGA'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'GTACCGTTGACTGAA' AND kmer = 'CAGTGACTTGACGT'
UNION
SELECT kmer FROM kmers WHERE kmer <@ 'TACGTTGACTGAACTGACC' AND kmer = 'GCTACGTACGTTGAAC'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'GCTAGCTTGAACCTGACT' AND kmer <@ 'GATTACAGCTAGCTAAGCTTGACCGT'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'ACGTAGCTAAGCTTGAACCGTAC' AND kmer = 'TGACCGTTGACTTGAACCTGACG';
/*	---Output---
	"QUERY PLAN"
	"HashAggregate  (cost=6214969.44..6765024.06 rows=14729496 width=32) (actual time=8116.785..8116.795 rows=2 loops=1)"
	"  Output: kmers.kmer"
	"  Group Key: kmers.kmer"
	"  Planned Partitions: 256  Batches: 1  Memory Usage: 793kB"
	"  ->  Append  (cost=0.00..5145930.24 rows=14729496 width=32) (actual time=202.829..8116.702 rows=2 loops=1)"
	"        ->  Seq Scan on public.kmers  (cost=0.00..569777.20 rows=14670808 width=24) (actual time=202.827..1001.504 rows=2 loops=1)"
	"              Output: kmers.kmer"
	"              Filter: (kmers.kmer ^@ 'GCTAGCTAAGCT'::kmer)"
	"              Rows Removed by Filter: 29355893"
	"        ->  Seq Scan on public.kmers kmers_1  (cost=0.00..569777.20 rows=29342 width=24) (actual time=965.007..965.007 rows=0 loops=1)"
	"              Output: kmers_1.kmer"
	"              Filter: (kmers_1.kmer <@ 'CGTTAAGCTTGACCGT'::qkmer)"
	"              Rows Removed by Filter: 29355895"
	"        ->  Seq Scan on public.kmers kmers_2  (cost=0.00..569777.20 rows=1 width=24) (actual time=920.391..920.391 rows=0 loops=1)"
	"              Output: kmers_2.kmer"
	"              Filter: (kmers_2.kmer = 'GATTACAGCTAGCTTGA'::kmer)"
	"              Rows Removed by Filter: 29355895"
	"        ->  Seq Scan on public.kmers kmers_3  (cost=0.00..643131.24 rows=14671 width=24) (actual time=982.713..982.713 rows=0 loops=1)"
	"              Output: kmers_3.kmer"
	"              Filter: ((kmers_3.kmer ^@ 'ACGTAGCTTGAACCGT'::kmer) AND (kmers_3.kmer <@ 'TGACCGTTGACTTGA'::qkmer))"
	"              Rows Removed by Filter: 29355895"
	"        ->  Seq Scan on public.kmers kmers_4  (cost=0.00..643131.24 rows=1 width=24) (actual time=1029.220..1029.220 rows=0 loops=1)"
	"              Output: kmers_4.kmer"
	"              Filter: ((kmers_4.kmer ^@ 'GTACCGTTGACTGAA'::kmer) AND (kmers_4.kmer = 'CAGTGACTTGACGT'::kmer))"
	"              Rows Removed by Filter: 29355895"
	"        ->  Seq Scan on public.kmers kmers_5  (cost=0.00..643131.24 rows=1 width=24) (actual time=1107.597..1107.597 rows=0 loops=1)"
	"              Output: kmers_5.kmer"
	"              Filter: ((kmers_5.kmer <@ 'TACGTTGACTGAACTGACC'::qkmer) AND (kmers_5.kmer = 'GCTACGTACGTTGAAC'::kmer))"
	"              Rows Removed by Filter: 29355895"
	"        ->  Seq Scan on public.kmers kmers_6  (cost=0.00..643131.24 rows=14671 width=24) (actual time=1059.045..1059.045 rows=0 loops=1)"
	"              Output: kmers_6.kmer"
	"              Filter: ((kmers_6.kmer ^@ 'GCTAGCTTGAACCTGACT'::kmer) AND (kmers_6.kmer <@ 'GATTACAGCTAGCTAAGCTTGACCGT'::qkmer))"
	"              Rows Removed by Filter: 29355895"
	"        ->  Seq Scan on public.kmers kmers_7  (cost=0.00..643131.24 rows=1 width=24) (actual time=1051.205..1051.205 rows=0 loops=1)"
	"              Output: kmers_7.kmer"
	"              Filter: ((kmers_7.kmer ^@ 'ACGTAGCTAAGCTTGAACCGTAC'::kmer) AND (kmers_7.kmer = 'TGACCGTTGACTTGAACCTGACG'::kmer))"
	"              Rows Removed by Filter: 29355895"
	"Planning Time: 0.105 ms"
	"Execution Time: 8116.874 ms"
*/

SELECT kmer FROM kmers WHERE kmer ^@ 'GCTAGCTAAGCT'
UNION
SELECT kmer FROM kmers WHERE kmer <@ 'CGTTAAGCTTGACCGT'
UNION
SELECT kmer FROM kmers WHERE kmer = 'GATTACAGCTAGCTTGA'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'ACGTAGCTTGAACCGT' AND kmer <@ 'TGACCGTTGACTTGA'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'GTACCGTTGACTGAA' AND kmer = 'CAGTGACTTGACGT'
UNION
SELECT kmer FROM kmers WHERE kmer <@ 'TACGTTGACTGAACTGACC' AND kmer = 'GCTACGTACGTTGAAC'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'GCTAGCTTGAACCTGACT' AND kmer <@ 'GATTACAGCTAGCTAAGCTTGACCGT'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'ACGTAGCTAAGCTTGAACCGTAC' AND kmer = 'TGACCGTTGACTTGAACCTGACG';
/*	---Output---
	kmer
	"GCTAGCTAAGCTAGATAGATGAACAG"
	"GCTAGCTAAGCTGAAAAT"
*/

SET enable_indexscan = ON;
EXPLAIN ANALYZE VERBOSE
SELECT kmer FROM kmers WHERE kmer ^@ 'GCTAGCTAAGCT'
UNION
SELECT kmer FROM kmers WHERE kmer <@ 'CGTTAAGCTTGACCGT'
UNION
SELECT kmer FROM kmers WHERE kmer = 'GATTACAGCTAGCTTGA'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'ACGTAGCTTGAACCGT' AND kmer <@ 'TGACCGTTGACTTGA'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'GTACCGTTGACTGAA' AND kmer = 'CAGTGACTTGACGT'
UNION
SELECT kmer FROM kmers WHERE kmer <@ 'TACGTTGACTGAACTGACC' AND kmer = 'GCTACGTACGTTGAAC'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'GCTAGCTTGAACCTGACT' AND kmer <@ 'GATTACAGCTAGCTAAGCTTGACCGT'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'ACGTAGCTAAGCTTGAACCGTAC' AND kmer = 'TGACCGTTGACTTGAACCTGACG';
/*	---Output---
	"QUERY PLAN"
	"HashAggregate  (cost=1636415.24..2186469.86 rows=14729496 width=32) (actual time=0.119..0.124 rows=2 loops=1)"
	"  Output: kmers.kmer"
	"  Group Key: kmers.kmer"
	"  Planned Partitions: 256  Batches: 1  Memory Usage: 793kB"
	"  ->  Append  (cost=0.42..567376.04 rows=14729496 width=32) (actual time=0.038..0.087 rows=2 loops=1)"
	"        ->  Index Only Scan using kmers_spgist_idx on public.kmers  (cost=0.42..344970.56 rows=14670808 width=24) (actual time=0.038..0.039 rows=2 loops=1)"
	"              Output: kmers.kmer"
	"              Index Cond: (kmers.kmer ^@ 'GCTAGCTAAGCT'::kmer)"
	"              Heap Fetches: 0"
	"        ->  Index Only Scan using kmers_spgist_idx on public.kmers kmers_1  (cost=0.42..691.00 rows=29342 width=24) (actual time=0.019..0.019 rows=0 loops=1)"
	"              Output: kmers_1.kmer"
	"              Index Cond: (kmers_1.kmer <@ 'CGTTAAGCTTGACCGT'::qkmer)"
	"              Heap Fetches: 0"
	"        ->  Index Only Scan using kmers_spgist_idx on public.kmers kmers_2  (cost=0.42..1.54 rows=1 width=24) (actual time=0.021..0.021 rows=0 loops=1)"
	"              Output: kmers_2.kmer"
	"              Index Cond: (kmers_2.kmer = 'GATTACAGCTAGCTTGA'::kmer)"
	"              Heap Fetches: 0"
	"        ->  Index Only Scan using kmers_spgist_idx on public.kmers kmers_3  (cost=0.42..382.94 rows=14671 width=24) (actual time=0.001..0.001 rows=0 loops=1)"
	"              Output: kmers_3.kmer"
	"              Index Cond: ((kmers_3.kmer ^@ 'ACGTAGCTTGAACCGT'::kmer) AND (kmers_3.kmer <@ 'TGACCGTTGACTTGA'::qkmer))"
	"              Heap Fetches: 0"
	"        ->  Index Only Scan using kmers_spgist_idx on public.kmers kmers_4  (cost=0.42..1.54 rows=1 width=24) (actual time=0.001..0.001 rows=0 loops=1)"
	"              Output: kmers_4.kmer"
	"              Index Cond: ((kmers_4.kmer ^@ 'GTACCGTTGACTGAA'::kmer) AND (kmers_4.kmer = 'CAGTGACTTGACGT'::kmer))"
	"              Heap Fetches: 0"
	"        ->  Index Only Scan using kmers_spgist_idx on public.kmers kmers_5  (cost=0.42..1.54 rows=1 width=24) (actual time=0.001..0.001 rows=0 loops=1)"
	"              Output: kmers_5.kmer"
	"              Index Cond: ((kmers_5.kmer <@ 'TACGTTGACTGAACTGACC'::qkmer) AND (kmers_5.kmer = 'GCTACGTACGTTGAAC'::kmer))"
	"              Heap Fetches: 0"
	"        ->  Index Only Scan using kmers_spgist_idx on public.kmers kmers_6  (cost=0.42..382.94 rows=14671 width=24) (actual time=0.002..0.002 rows=0 loops=1)"
	"              Output: kmers_6.kmer"
	"              Index Cond: ((kmers_6.kmer ^@ 'GCTAGCTTGAACCTGACT'::kmer) AND (kmers_6.kmer <@ 'GATTACAGCTAGCTAAGCTTGACCGT'::qkmer))"
	"              Heap Fetches: 0"
	"        ->  Index Only Scan using kmers_spgist_idx on public.kmers kmers_7  (cost=0.42..1.54 rows=1 width=24) (actual time=0.001..0.001 rows=0 loops=1)"
	"              Output: kmers_7.kmer"
	"              Index Cond: ((kmers_7.kmer ^@ 'ACGTAGCTAAGCTTGAACCGTAC'::kmer) AND (kmers_7.kmer = 'TGACCGTTGACTTGAACCTGACG'::kmer))"
	"              Heap Fetches: 0"
	"Planning Time: 0.080 ms"
	"Execution Time: 0.186 ms"
*/

SELECT kmer FROM kmers WHERE kmer ^@ 'GCTAGCTAAGCT'
UNION
SELECT kmer FROM kmers WHERE kmer <@ 'CGTTAAGCTTGACCGT'
UNION
SELECT kmer FROM kmers WHERE kmer = 'GATTACAGCTAGCTTGA'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'ACGTAGCTTGAACCGT' AND kmer <@ 'TGACCGTTGACTTGA'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'GTACCGTTGACTGAA' AND kmer = 'CAGTGACTTGACGT'
UNION
SELECT kmer FROM kmers WHERE kmer <@ 'TACGTTGACTGAACTGACC' AND kmer = 'GCTACGTACGTTGAAC'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'GCTAGCTTGAACCTGACT' AND kmer <@ 'GATTACAGCTAGCTAAGCTTGACCGT'
UNION
SELECT kmer FROM kmers WHERE kmer ^@ 'ACGTAGCTAAGCTTGAACCGTAC' AND kmer = 'TGACCGTTGACTTGAACCTGACG';
/*	---Output---
	"kmer"
	"GCTAGCTAAGCTAGATAGATGAACAG"
	"GCTAGCTAAGCTGAAAAT"
*/

-- Test in production (more operator support might be needed for this to work)
SELECT
  relname                                               AS TableName,
  to_char(seq_scan, '999,999,999,999')                  AS TotalSeqScan, -- Total number of sequential scans performed on the table.
  to_char(idx_scan, '999,999,999,999')                  AS TotalIndexScan, -- Total number of index scans performed on the table.
  to_char(n_live_tup, '999,999,999,999')                AS TableRows, -- Approximate number of live rows in the table.
  pg_size_pretty(pg_relation_size(relname :: regclass)) AS TableSize
FROM pg_stat_all_tables
WHERE schemaname = 'public'
      AND 50 * seq_scan > idx_scan -- Tables where sequential scans are at least 50 times more frequent than index scans
      AND n_live_tup > 10000
      AND pg_relation_size(relname :: regclass) > 5000000 -- >5MB
ORDER BY relname ASC;
/*	---Output---
	tablename	totalseqscan	totalindexscan	tablerows	tablesize
	"kmers"	    89				94				29,341,617	1586 MB"
*/