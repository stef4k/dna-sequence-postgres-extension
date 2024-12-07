import csv
import random

def sliding_window(sequence, k):
    """generate k-mers of size k from the given DNA sequence."""
    return [sequence[i:i+k] for i in range(len(sequence) - k + 1)]

def process_dna_sequences(input_file, output_file, max_rows=100_000_000):
    """take DNA sequences from the CSV file and save unique k-mers"""
    with open(input_file, mode='r') as infile, open(output_file, mode='w', newline='') as outfile:
        reader = csv.reader(infile)
        writer = csv.writer(outfile)

        unique_kmers = set()
        row_count = 0

        next(reader)  # skip header
        
        for row in reader:
            if row_count >= max_rows:
                break
            
            dna_sequence = row[0].strip()
            dna_length = len(dna_sequence)

            # we only take sequences <= 100 characters
            if not (1 <= dna_length <= 100):
                print(f"Skipping invalid DNA sequence: {dna_sequence}")
                continue

            # generate random k (1-32) + validate against DNA length
            k = random.randint(1, min(32, dna_length))

            # generate k-mers
            kmers = sliding_window(dna_sequence, k)

            # add unique k-mers to the set
            for kmer in kmers:
                if kmer not in unique_kmers:
                    unique_kmers.add(kmer)
                    writer.writerow([kmer])
                    row_count += 1
                    if row_count >= max_rows:
                        break

input_csv = "files/csv_files/dnas.csv"
output_csv = "files/csv_files/unique_dna_kmers.csv"

# Careful, there will be a lot of output
# In our cases, we have over 1 million DNA sequences and it yields more than 29 million unique k-mers
process_dna_sequences(input_csv, output_csv)

print(f"Unique k-mers saved to {output_csv}, up to a maximum of 100 million rows.")
