import pandas as pd

#File paths
file_path = '.venv/files/SRR000001_2.fastq'

#Identifies the type of sequence as either DNA, Kmer or QKmer
def seq_sorter(sequence, sequences):
    if sequence.find("R") + sequence.find("N") + sequence.find("Y") >= -2:
        if len(sequence) <= 32:
            sequences[sequence] = 'qkmer'
        else:
            pass
    else:
        if len(sequence) <= 32:
            sequences[sequence] = 'kmer'
        else:
            sequences[sequence] = 'dna'
    return sequences

# Read the generated .fastq file and return a list of dna sequences
def query_parsing(file):
    with open(file, 'r') as seq:
        val = False
        sequences = {}
        n = 0
        for line in seq.read().splitlines():
            if val:
                sequences = seq_sorter(line, sequences)
                val = False
            else:
                if line.startswith("@") and line.find("length=") >= 0:
                    val = True
    return sequences

def command_generator(sequence):
    with (open(".venv/files/dna_inserts.sql", 'w') as dna_file,
          open('.venv/files/kmer_inserts.sql', 'w') as kmer_file,
          open('.venv/files/qkmer_inserts.sql', 'w') as qkmer_file):
        dna_file.truncate(0)
        kmer_file.truncate(0)
        qkmer_file.truncate(0)

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


seq = query_parsing('.venv/files/SRR000001_2.fastq')
command_generator(seq)

# sequences =  pd.DataFrame(seq.items(), columns=['SEQUENCE', 'TYPE'])
# filename = f"sequences.csv"
# sequences.to_csv(filename, index=False)