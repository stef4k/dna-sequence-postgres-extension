import psycopg2
import csv

# Database connection parameters
DB_NAME = "test_db"
DB_USER = "postgres"
DB_PASSWORD = "passkey"
DB_HOST = "localhost"
DB_PORT = "5432"

# File paths
CSV_FILE_PATH = "../dna_db_generator/sequences.csv"
EXPLAIN_BEFORE_FILE = "explain_before_index.txt"
EXPLAIN_AFTER_FILE = "explain_after_index.txt"

# Connect to PostgreSQL
def connect_db():
    try:
        conn = psycopg2.connect(
            dbname=DB_NAME, user=DB_USER, password=DB_PASSWORD, host=DB_HOST, port=DB_PORT
        )
        return conn
    except Exception as e:
        print("Error: Unable to connect to the database")
        print(e)
        return None

# Drop extension, table, create extension, and table
def setup_database(conn):
    with conn.cursor() as cur:
        cur.execute("DROP EXTENSION IF EXISTS dna_sequence CASCADE;")
        cur.execute("DROP TABLE IF EXISTS dna_sequences;")
        cur.execute("CREATE EXTENSION dna_sequence;")
        cur.execute(
            """
            CREATE TABLE dna_sequences (
                id SERIAL PRIMARY KEY,
                sequence dna_sequence
            );
            """
        )
        conn.commit()

# Insert data from CSV
def insert_data_from_csv(conn):
    with conn.cursor() as cur:
        with open(CSV_FILE_PATH, 'r') as csvfile:
            reader = csv.reader(csvfile)
            next(reader)  # Skip header row if present
            for row in reader:
                cur.execute("INSERT INTO dna_sequences (sequence) VALUES (%s);", (row[0],))
        conn.commit()

# Run EXPLAIN ANALYZE and write output to a file
def explain_analyze(conn, file_path, query):
    with conn.cursor() as cur:
        cur.execute(f"EXPLAIN ANALYZE {query}")
        result = cur.fetchall()
        with open(file_path, 'w') as f:
            for row in result:
                f.write(row[0] + "\n")

# Create indexes
def create_indexes(conn):
    with conn.cursor() as cur:
        cur.execute("CREATE INDEX dna_sequence_btree_idx ON dna_sequences USING btree (sequence);")
        cur.execute("CREATE INDEX dna_sequence_hash_idx ON dna_sequences USING hash (sequence);")
        conn.commit()

# Main function
def main():
    conn = connect_db()
    if conn is None:
        return

    try:
        setup_database(conn)
        insert_data_from_csv(conn)

        # Explain analyze before creating indexes
        explain_analyze(conn, EXPLAIN_BEFORE_FILE, "SELECT * FROM dna_sequences WHERE sequence = 'ACGTACGTACGTACGTACGTACGTACGTACGT';")

        # Create indexes
        create_indexes(conn)

        # Explain analyze after creating indexes
        explain_analyze(conn, EXPLAIN_AFTER_FILE, "SELECT * FROM dna_sequences WHERE sequence = 'ACGTACGTACGTACGTACGTACGTACGTACGT';")
    finally:
        conn.close()

if __name__ == "__main__":
    main()

