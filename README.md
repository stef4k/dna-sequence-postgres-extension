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
- dna_sequence--1.0.sql: SQL script used to create the extension.
- dna_sequence.c: C source code for data types and functions used to create the extension.
- spg_kmer.c: C source code for index implementation.
- Makefile: Makefile used to compile the extension.
- dna_sequence.control: Control file for the extension.

### dna_db_generator
Collection of Python scripts used for data generation.
- dna_db_generator.py: Python script used to parse through a collection of .fastq files downloaded using the [NCBI SRA Toolkit](https://github.com/ncbi/sra-tools/wiki/01.-Downloading-SRA-Toolkit) and create usable collections of real world data for testing the extension. Returns both .csv and .sql files for ease of data import.
- unique_dna_kmers_generation.py: Python script for generating additional K-mer sequences based on the DNA sequences obtained from the .fastq files. This was used because we needed a large amount of K-mer objects to properly test index functionality and the real world data did not naturally provide enough K-mer sequences by itself. **Requires the existence of a DNA.csv file.**
- files:
	- fastqs: *.fastq* files obtained from the NCBI database containing real world sequences.
	- csv_files: Directory for *.csv* files created by the generator script.
	- sql_inserts: Directory for *.sql* files that can be run to create, and populate a database of each file type. 
	- dna_databases.sql: *.sql* file for creating the three empty tables that can be populated using the *.csv* or *.sql* files created by the script.

### test_queries.sql
Collection of queries used for validating the proper functionality of the extension as per the project requirement.

## Extension Installation
This project relies on the user having already installed [PostgreSQL](https://www.postgresql.org/) and being able to use PLQL commands.
Follow these steps to properly install the extension:
1. Clone the github directory to the desired location.

2. Open a terminal and go to the **dna-sequence-postgres-extension** directory.

3. Run
	```
	make
	``` 
	command to compile the extension.

4. Run 
	```
	sudo make install
	``` 
command to install the extension.

5. Connect to the desired PostgreSQL server and run:
	```
	CREATE EXTENSION dna_sequence;
	```
to create the extension.

If the steps above are followed correctly the PLSQL terminal should show a **CREATE EXTENSION** message. 

## Data Generation
