import pandas as pd
import os
import psycopg2
import subprocess

subprocess.call(['./sra_toolkit/bin/fasterq-dump', '--split-files', 'SRR900001'])

#File paths
sources_paths = []
output_paths = []
for file in os.listdir('files'):
    if file.endswith(".fastq"):
        sources_paths.append(os.path.join("files", file))

#Databse connection
def db_connect():
    conn = psycopg2.connect(database="complex",
                            host="127.0.0.1",
                            user='owantland1',
                            password='password',
                            port='5432')
    cursor = conn.cursor()
    return cursor

#Identifies the type of sequence as either DNA, Kmer or QKmer
def seq_sorter(sequence, sequences, dnas, kmers, qkmers):
    if sequence.find("R") + sequence.find("N") + sequence.find("Y") >= -2:
        if len(sequence) <= 32:
            sequences[sequence] = 'qkmer'
            qkmers.append(sequence)
        else:
            pass
    else:
        if len(sequence) <= 32:
            sequences[sequence] = 'kmer'
            kmers.append(sequence)
        else:
            sequences[sequence] = 'dna'
            dnas.append(sequence)
    return sequences, dnas, kmers, qkmers

# Read the generated .fastq file and return a list of dna sequences
def query_parsing(files):
    sequences = {}
    dnas = []
    kmers = []
    qkmers = []
    for file in files:
        with open(file, 'r') as seq:
            val = False
            n = 0
            for line in seq.read().splitlines():
                if val:
                    sequences, dnas, kmers, qkmers = seq_sorter(line, sequences, dnas, kmers, qkmers)
                    val = False
                else:
                    if line.startswith("@") and line.find("length=") >= 0:
                        val = True
            seq.close()
    return sequences, dnas, kmers, qkmers

def command_generator(sequence):
    with (open("files/dna_inserts.sql", 'w') as dna_file,
          open('files/kmer_inserts.sql', 'w') as kmer_file,
          open('files/qkmer_inserts.sql', 'w') as qkmer_file):
        dna_file.truncate(0)
        kmer_file.truncate(0)
        qkmer_file.truncate(0)

        # cursor = db_connect()
        #
        # #Drop and create the appropriate tables
        # cursor.execute("DROP TABLE IF EXISTS DNAS;")
        # cursor.execute("CREATE TABLE DNAS (DNA_SEQUENCE DNA_SEQUENCE);")


        dna_file.write("DROP TABLE IF EXISTS DNAS;\n")
        dna_file.write("CREATE TABLE DNAS (DNA_SEQUENCE DNA_SEQUENCE);\n")
        dna_file.write("INSERT INTO DNAS (DNA_SEQUENCE) VALUES\n")

        kmer_file.write("DROP TABLE IF EXISTS KMERS;\n")
        kmer_file.write("CREATE TABLE KMERS (KMER KMER);\n")
        kmer_file.write("INSERT INTO KMERS (KMER) VALUES\n")

        qkmer_file.write("DROP TABLE IF EXISTS QKMERS;\n")
        qkmer_file.write("CREATE TABLE QKMERS (QKMER QKMER);\n")
        qkmer_file.write("INSERT INTO QKMERS (QKMER) VALUES\n")

        first_dna = True
        first_kmer = True
        first_qkmer = True

        for seq, type in sequence.items():
            if type == 'dna':
                if first_dna:
                    dna_file.write(f"('{seq}')\n")
                    first_dna = False
                else:
                    dna_file.write(f",('{seq}')\n")
            elif type == 'kmer':
                if first_kmer:
                    kmer_file.write(f"('{seq}')\n")
                    first_kmer = False
                else:
                    kmer_file.write(f",('{seq}')\n")
            elif type == 'qkmer':
                if first_qkmer:
                    qkmer_file.write(f"('{seq}')\n")
                    first_qkmer = False
                else:
                    qkmer_file.write(f",('{seq}')\n")


seq, dna, kmer, qkmer = query_parsing(sources_paths)
# command_generator(seq)
pd_dna =  pd.DataFrame(dna, columns=['SEQUENCE'])
filename = f"dna.csv"
pd_dna.to_csv(filename, index=False)

pd_kmer =  pd.DataFrame(kmer, columns=['SEQUENCE'])
filename = f"kmer.csv"
pd_kmer.to_csv(filename, index=False)

pd_qkmer =  pd.DataFrame(qkmer, columns=['SEQUENCE'])
filename = f"qkmer.csv"
pd_qkmer.to_csv(filename, index=False)