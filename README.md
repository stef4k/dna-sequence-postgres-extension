# DNA Sequence Database Extension for Postgres
Repository for the development of the DNA Sequence Extension for Postgres as part of the 2024 INFO-H417 Database System Architecture project.
This repository contains the extension template and scripts used to create the extension as well as the datasets and sql functions to test it out.

Group Members:
- [Kristóf Balázs](https://github.com/Kiklar)
- [Stefanos Kypritidis](https://github.com/stef4k)
- [Otto Wantland](https://github.com/Owantland)
- [Nima Kamali Lassem]()

Professor: Mahmoud Sakr

## Repo Structure
### dna_sequence_template
The extension's template. Contains both a _c_ and an _sql file_ with the extension's data types, functions and operators as well as the Makefile necesarry for its installation.
### dna_db_generator
Python script used to obtain real world datasets utilizing the [NCBI SRA Toolkit](https://github.com/ncbi/sra-tools/wiki/01.-Downloading-SRA-Toolkit), parse the dataset and create the proper .csv files for loading into the test database.
