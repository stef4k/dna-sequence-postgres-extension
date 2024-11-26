drop table kmers_big cascade;
drop table kmers_big_noindex cascade;

DROP EXTENSION dna_sequence CASCADE; 
CREATE EXTENSION dna_sequence;


DO $$
DECLARE
    idx RECORD;
BEGIN
    FOR idx IN
        SELECT indexname
        FROM pg_indexes
        WHERE tablename = 'kmers_big'
    LOOP
        EXECUTE format('DROP INDEX IF EXISTS %I;', idx.indexname);
        RAISE NOTICE 'Dropped index: %', idx.indexname;
    END LOOP;
END $$;


CREATE TABLE kmers_big (
    id BIGSERIAL PRIMARY KEY,
    kmer kmer
);

CREATE TABLE kmers_big_noindex (
    id BIGSERIAL PRIMARY KEY,
    kmer kmer
);
--
--SELECT COUNT(*)
--FROM generate_kmers(
--    'ACGTACGTACGTACGTACGTACGTACGTACGT',
--    trunc(random() * 32 + 1)::int
--);

SET work_mem = '2GB'; -- Increase for better handling of large operations
SET maintenance_work_mem = '10GB'; -- Allocate more memory for index creation

DO $$
DECLARE
    total_rows BIGINT := 0;
    batch_size INT := 100000; -- Number of rows per batch
    max_rows BIGINT := 500000; -- Target: 10 millionrows
BEGIN
    WHILE total_rows < max_rows LOOP
        INSERT INTO kmers_big (kmer)
        SELECT generate_kmers(
            'ACGTACGTACGTACGTACGTACGTACGTACGT', -- Example DNA sequence
            trunc(random() * 32 + 1)::int -- Random k length between 1 and 32
        )
        FROM generate_series(1, batch_size);

        total_rows := total_rows + batch_size;
        RAISE NOTICE 'Inserted % rows so far', total_rows;
    END LOOP;
END;
$$;


DO $$
DECLARE
    total_rows BIGINT := 0;
    batch_size INT := 1000000; -- Number of rows per batch
    max_rows BIGINT := 10000000; -- Target: 10 millionrows
BEGIN
    WHILE total_rows < max_rows LOOP
        INSERT INTO kmers_big_noindex (kmer)
        SELECT generate_kmers(
            'ACGTACGTACGTACGTACGTACGTACGTACGT', -- Example DNA sequence
            trunc(random() * 32 + 1)::int -- Random k length between 1 and 32
        )
        FROM generate_series(1, batch_size);

        total_rows := total_rows + batch_size;
        RAISE NOTICE 'Inserted % rows so far', total_rows;
    END LOOP;
END;
$$;

EXPLAIN ANALYZE SELECT * FROM kmers_big WHERE kmer = 'ACGTA';


SHOW enable_indexscan;
show enable_bitmapscan;
SET enable_indexscan = on;
SET enable_bitmapscan = on;
-- Complex kmer query for testing SP-GiST index speed
EXPLAIN ANALYZE
SELECT *
FROM kmers_big kb 
WHERE (kmer = 'ACGTA'         -- Exact match
       OR kmer ^@ 'ACG'       -- Prefix match
       OR 'ANGTA' @> kmer)    -- Pattern match
  AND (kmer ^@ 'TGC'          -- Another prefix match condition
       OR 'CTGTA' @> kmer);   -- Another pattern match

-- Complex kmer query for testing SP-GiST index speed
EXPLAIN ANALYZE
SELECT *
FROM kmers_big_noindex kb 
WHERE (kmer = 'ACGTA'         -- Exact match
       OR kmer ^@ 'ACG'       -- Prefix match
       OR 'ANGTA' @> kmer)    -- Pattern match
  AND (kmer ^@ 'TGC'          -- Another prefix match condition
       OR 'CTGTA' @> kmer);   -- Another pattern match


       
       
-- Expanded complex kmer query for performance testing
EXPLAIN ANALYZE 
SELECT *
FROM kmers_big kb 
WHERE (
        kmer = 'ACGTA'
        OR kmer = 'TTGCA'
        OR kmer = 'GGCAA'
        OR kmer = 'AACGT'
        OR kmer = 'CCGTA'
        OR kmer ^@ 'AC'
        OR kmer ^@ 'TG'
        OR kmer ^@ 'CT'
        OR kmer ^@ 'GG'
        OR kmer ^@ 'CC'
        OR 'ANGTA' @> kmer
        OR 'TGCAT' @> kmer
        OR 'GACTG' @> kmer
        OR 'CGTAA' @> kmer
        OR 'AATCC' @> kmer
      )
  AND (
        kmer ^@ 'AA'
        OR kmer ^@ 'TT'
        OR kmer ^@ 'GG'
        OR kmer ^@ 'CC'
        OR 'AAGTA' @> kmer
        OR 'TTTGC' @> kmer
        OR 'GGGCA' @> kmer
        OR 'CCCCG' @> kmer
      )
  AND (
        kmer != 'AAAAA'
        AND kmer != 'TTTTT'
        AND kmer != 'GGGGG'
        AND kmer != 'CCCCC'
      )
  AND (
        kmer ^@ 'AT'
        OR kmer ^@ 'CG'
        OR kmer ^@ 'GC'
        OR kmer ^@ 'TA'
        OR 'ATTGC' @> kmer
        OR 'CCGAT' @> kmer
        OR 'GGAAT' @> kmer
        OR 'TCCGA' @> kmer
      )
  AND (
        kmer ^@ 'TT'
        OR kmer ^@ 'AA'
        OR kmer ^@ 'GG'
        OR kmer ^@ 'CC'
        OR 'AAGCA' @> kmer
        OR 'TTGCT' @> kmer
        OR 'GGCTT' @> kmer
        OR 'CCGTT' @> kmer
      );

     
     
-- Insert rows that match and do not match the query
INSERT INTO kmers_big (kmer)
VALUES
    ('ACGTA'),        -- Matches equality
    ('ACGAA'),        -- Matches prefix
    ('ACGTA'),        -- Matches pattern
    ('TGCAT'),        -- Matches another pattern
    ('CCCCG'),        -- Doesn't match but adds complexity
    ('GGGCA'),        -- Matches pattern
    ('AACGT'),        -- Matches equality
    ('ACCCC');        -- Matches prefix

SELECT COUNT(*)
FROM kmers_big kb 
WHERE kmer = 'ACGTA' OR kmer ^@ 'AC' OR 'ANGTA' @> kmer;


EXPLAIN ANALYZE SELECT * FROM kmers_big WHERE kmer = 'ACGTAAAAA';

CREATE INDEX kmer_spgist_idx ON kmers_big USING spgist (kmer);


SELECT
    relname AS index_name,
    idx_scan AS index_scans,
    idx_tup_read AS tuples_read,
    idx_tup_fetch AS tuples_fetched
FROM
    pg_stat_all_indexes
WHERE
    schemaname = 'public' AND relname = 'kmer_spgist_idx';
   
   
   
   
SELECT
    relname AS index_name,
    pg_size_pretty(pg_relation_size(indexrelid)) AS index_size
FROM
    pg_stat_all_indexes
WHERE
    relname = 'kmer_spgist_idx';
   
   
   
max_parallel_maintenance_workers = 4  # Adjust based on available CPUs

   
SELECT
    schemaname,
    tablename,
    indexname,
    indexdef
FROM
    pg_indexes
WHERE
    indexname = 'kmer_spgist_idx';

   
   
   
   
   
   
SELECT COUNT(*) FROM kmers_big;



SELECT
    relname AS index_name,
    reltuples AS number_of_items,
    pg_size_pretty(pg_relation_size(oid)) AS index_size
FROM
    pg_class
WHERE
    relname = 'kmer_spgist_idx';



SET enable_indexscan = on ;
SET enable_bitmapscan = off;
SET enable_seqscan =on;
show enable_seqscan;

EXPLAIN ANALYZE SELECT * FROM kmers_big WHERE kmer ^@ 'A';

EXPLAIN ANALYZE SELECT * FROM kmers_big WHERE 'A' @> kmer;

SELECT * FROM kmers_big WHERE kmer = 'A';
   
explain analyze SELECT *
FROM (
    SELECT kmer
    FROM kmers_big kb 
    WHERE kmer ^@ 'AC'
) subquery
WHERE subquery.kmer = 'ACGTA'
  OR subquery.kmer ^@ 'TGC';

 
 
SELECT *
FROM kmers_big kb 
WHERE kmer = 'acgta'
  OR SUBSTRING(kmer, 1, 2) = 'AC';

 
 
 
 
 
 
 
 
-- Heavy kmer query for stress testing
EXPLAIN ANALYZE
SELECT a.kmer, b.kmer
FROM kmers_big a
CROSS JOIN kmers_big b
WHERE 
    (a.kmer = b.kmer OR a.kmer ^@ b.kmer OR 'ANGTA' @> a.kmer)
  AND (b.kmer ^@ 'ACG' OR 'CCGT' @> b.kmer)
  AND (a.kmer = 'ACGTA' OR a.kmer ^@ 'TGCA' OR 'TGCAT' @> a.kmer)
  AND (b.kmer != 'GGGGG' AND b.kmer != 'TTTTT');




