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
