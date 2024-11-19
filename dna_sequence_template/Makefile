EXTENSION   = dna_sequence
MODULES 	= dna_sequence
DATA        = dna_sequence--1.0.sql dna_sequence.control
OBJS = dna_sequence.o spg_kmer.o

PG_CONFIG ?= pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
