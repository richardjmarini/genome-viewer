#ifndef __FASTA_H__
#define __FASTA_H__

#define FASTA_LINE_LENGTH 80

typedef enum {
    PRIMARY,     // primary chromosome
    AT_LOCUS,    // alternate haplotype
    UNPLACED,    // chromosome unknown
    UNLOCALIZED, // exact position on chromosome unknown,
    FIX_PATCH,   // corrected existing primary sequence
    NOVEL_PATCH  // adds a new sequence not in primary
} SequenceRole;
    
typedef struct {
    char *id;
    char *organism;
    char *chromosome;     // primary chromosome
    char *type;           // scaffolding
    char *build;
    char *locus;

    SequenceRole role;

    long start;           // start coordinate of chromosome
    long end;             // end coordinate o chromosome

    long offset;          // byte position of sequence
    long length;          // number of base pairs
 } SequenceMetadata;


typedef struct {
    FILE *file;
    int record_count;
    SequenceMetadata  *metadata;
} GenomeFile;

void parse_header(const char *line, SequenceMetadata *metadata);
int index_fasta(GenomeFile *gf, const char *path);
char *fetch_sequence(FILE *file, SequenceMetadata *metadata);

#endif
