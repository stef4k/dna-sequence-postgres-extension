EXTENSION   = dna_sequence
MODULES 	= dna_sequence spg_kmer
DATA        = dna_sequence--1.0.sql dna_sequence.control

PG_CONFIG ?= pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
