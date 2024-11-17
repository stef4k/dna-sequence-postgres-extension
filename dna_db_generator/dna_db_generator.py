import pandas as pd

#File paths
file_path = '.venv/files/SRR000001_2.fastq'

#Identifies the type of sequence as either DNA, Kmer or QKmer
def seq_sorter(sequence, sequences):
    if sequence.find("R") + sequence.find("N") + sequence.find("Y") >= 0:
        sequences[sequence] = 'qkmer'
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


seq = query_parsing('.venv/files/SRR000001_2.fastq')
sequences =  pd.DataFrame(seq.items(), columns=['SEQUENCE', 'TYPE'])
filename = f"sequences.csv"
sequences.to_csv(filename, index=False)