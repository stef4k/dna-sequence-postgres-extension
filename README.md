# DNA Sequence Database Extension for Postgres
Repository for the development of the DNA Sequence Extension for Postgres as part of the 2024 INFO-H417 Database System Architecture project.
This repository contains the extension template and scripts used to create the extension as well as the datasets and sql functions to test it out.

Group Members:
- [Kristóf Balázs](https://github.com/Kiklar)
- [Stefanos Kypritidis](https://github.com/stef4k)
- [Otto Wantland](https://github.com/Owantland)
- [Nima Kamali Lassem](https://github.com/NimakamaliLassem)

Professor: Mahmoud Sakr

## Repo Structure
### dna_sequence_template
The extension's template. Contains both a _c_ and an _sql file_ with the extension's data types, functions and operators as well as the Makefile necesarry for its installation.
- dna_sequence--1.0.sql: SQL script used to create the extension.
- dna_sequence.h: Header file for the extension (used by both C files).
- dna_sequence.c: C source code for data types and functions used to create the extension.
- spg_kmer.c: C source code for index implementation.
- Makefile: Makefile used to compile the extension.
- dna_sequence.control: Control file for the extension.

### dna_db_generator
Collection of Python scripts used for data generation.
- dna_db_generator.py: Python script used to parse through a collection of .fastq files downloaded using the [NCBI SRA Toolkit](https://github.com/ncbi/sra-tools/wiki/01.-Downloading-SRA-Toolkit) and create usable collections of real world data for testing the extension. Returns both .csv and .sql files for ease of data import.
- unique_dna_kmers_generation.py: Python script for generating additional K-mer sequences based on the DNA sequences obtained from the .fastq files. This was used because we needed a large amount of K-mer objects to properly test index functionality and the real world data did not naturally provide enough K-mer sequences by itself. **Requires the existence of a DNA.csv file.**
- files:
	- fastqs: Directory to store *.fastq* files obtained from the NCBI database containing real world sequences. The link to the *.zip* file containing a small collection of these files for testing is found in the **data generation** instructions.
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

	> :warning: **Attention:** In the latest versions of Postgres, when running make, the following error will be shown:
	> ```
	> spg_kmer.c:123:20: error: ‘VARHDRSZ_SHORT’ undeclared (first use in this function)
	> if (datalen + VARHDRSZ_SHORT <= VARATT_SHORT_MAX)
	> ...
	> ```
	> In this case, modify the `spg_kmer.c` by adding `#include "varatt.h"` at the beginning of the file.

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
The data generation tool uses *.fastq* files stored in the **fastqs** directory to generate the proper *.csv* and *.sql* files. A small collection of these files can be found in the [here](https://universitelibrebruxelles-my.sharepoint.com/:u:/g/personal/otto_wantland_conde_ulb_be/EeUpgcmiqbhDiduJJh8bTZQBHF4zPycz_4wgU9oLqQdZhQ?e=9KpBoV).
The script also requires a working [Python](https://www.python.org/) installation with the [Pandas](https://pandas.pydata.org/) library installed.
1. Unzip the downloaded file and place its contents in the **dna_db_generator/files/fastqs** directory.
2. Run *dna_db_generator.py* to create three distinct *.csv* files in the **csv_files** directory and three distinct *.sql* files in the **sql_files** director.
3. Depending on the user the resulting data can be imported into the desired schema in the following manners:
	- **CSV Files**: Execute the *dna_databases.sql* script to create the required tables and then use a DBMS like [pgAdmin](https://www.pgadmin.org/) to import the three files created in the **csv_files** directory.
	- **SQL Files**: Run each file found in the **sql_files** directory. Each *.sql* file creates the proper table and then populates the table through a series of value inserts.
4. In case that more K-mers are needed for index testing, run *unique_dna_kmers_generation.py* which will utilize the *dna.csv* file to generate new unique K-mers in a *.csv* file.

## Testing
**test_queries.sql** contains a series of different queries that can be executed in order to test the proper functionality of the extension. These queries range from simple data type tests to validate length and character constraints to function tests to validate proper implementation of the required functions. Each one shows the expected output based on what was generated when running the queries with our test database.
Additionally the file contains a test of the index functionality with a large collection of K-mer values.
