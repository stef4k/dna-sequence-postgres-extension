EXTENSION = dna_sequence
MODULE_big = dna_sequence
OBJS = dna_sequence.o spg_kmer.o
DATA = dna_sequence--1.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
