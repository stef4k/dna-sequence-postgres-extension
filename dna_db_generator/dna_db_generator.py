import pandas as pd

#File paths
file_path = '.venv/files/SRR000001_2.fastq'

# Read the generated .fastq file and return a list of dna sequences
def query_parsing(file):
    sequences = []
    with open(file, 'r') as seq:
        n = 0
        val = False
        for line in seq.read().splitlines():
            if val:
                sequences.append(line)
                val = False
                n+=1
            # Validate the length of the dna sequence
            else:
                if line.startswith("@"):
                    len = line.find("length=")
                    len = line[len+7:]
                    try:
                        if int(len) <= 35:
                            val = True
                    except:
                        val = False
    return sequences

seq = query_parsing('.venv/files/SRR000001_2.fastq')
sequences =  pd.DataFrame(seq, columns=['SEQUENCE'])
filename = f"sequences.csv"
sequences.to_csv(filename, index=False)