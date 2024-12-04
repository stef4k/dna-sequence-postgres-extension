import os
import psycopg2
from random import randint


#File paths
sources_paths = []
output_paths = []
for file in os.listdir('files/fastqs'):
    if file.endswith(".fastq"):
        sources_paths.append(os.path.join("files/fastqs", file))

#Databse connection
def db_connect():
    conn = psycopg2.connect(database="postgres",
                            host="127.0.0.1",
                            user='owantland1',
                            password='password',
                            port='5432')
    cursor = conn.cursor()
    return cursor, conn

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
        cursor, conn = db_connect()

        #Drop and create the appropriate tables
        cursor.execute("DROP TABLE IF EXISTS DNAS;")
        cursor.execute("CREATE TABLE DNAS (DNA_SEQUENCE DNA_SEQUENCE);")

        cursor.execute("DROP TABLE IF EXISTS KMERS;")
        cursor.execute("CREATE TABLE KMERS (KMER KMER);")

        cursor.execute("DROP TABLE IF EXISTS QKMERS;")
        cursor.execute("CREATE TABLE QKMERS (QKMER QKMER);")

        conn.commit()

        for seq, type in sequence.items():
            if type == 'dna':
                cmd = f"INSERT INTO DNAS (DNA_SEQUENCE) VALUES ('{seq}');"
                try:
                    cursor.execute(cmd)
                    conn.commit()
                except:
                    pass
            elif type == 'kmer':
                cmd = f"INSERT INTO KMERS (KMER) VALUES ('{seq}');"
                try:
                    cursor.execute(cmd)
                    conn.commit()
                except:
                    pass
            elif type == 'qkmer':
                cmd = f"INSERT INTO QKMERS (QKMER) VALUES ('{seq}');"
                try:
                    cursor.execute(cmd)
                    conn.commit()
                except:
                    pass

        cursor.close()
        conn.close()
        print("connection closed")

def kmer_generator():
    cursor, conn = db_connect()
    cursor.execute("SELECT * FROM DNAS;")
    cmds = []
    inserts = []

    for x in range(1,10000001):
        m = cursor.fetchone()
        n = randint(10,32)
        cmds.append(f"SELECT generate_kmers('{m[0]}', {n}) LIMIT 1;")
    conn.commit()

    for cmd in cmds:
        cursor.execute(cmd)
        m = cursor.fetchone()
        inserts.append(f"INSERT INTO KMERS (KMER) VALUES ('{m[0]}');")
    conn.commit()

    for insert in inserts:
        cursor.execute(insert)
        conn.commit()
    conn.close()

seq, dna, kmer, qkmer = query_parsing(sources_paths)
command_generator(seq)
kmer_generator()