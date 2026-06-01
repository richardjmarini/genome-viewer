#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <logger.h>
#include <fasta.h>

#define FASTA_LINE_LENGTH 60 // GRCh38 uses 60bp lines

void parse_header(const char *line, SequenceMetadata *metadata) {

    char buf[2048];
    char *nl;
    char *space;
    char *rest;
    const char *hs= "Homo sapiens ";
    char *after_organism;
    char *comma;
    char *sp;
    char *end;
    const char *build_marker= "GRCh";
    char *build_start;
    char *build_end;
    char *last_space;

    // skip the leading '>'
    strncpy(buf, line + 1, sizeof(buf) - 1);
    buf[sizeof(buf) - 1]= '\0';

    // strip trailing newline
    nl= strpbrk(buf, "\r\n");
    if (nl) *nl= '\0';

    // id
    space= strchr(buf, ' ');
    if (space) {
        metadata->id= strndup(buf, space - buf);
    } else {
        metadata->id= strdup(buf);
        return; // nothing else to parse
    }

    rest= space + 1;

    // organism
    after_organism= strstr(rest, hs);
    if (after_organism) {
        metadata->organism= strndup(rest, (after_organism - rest) + strlen(hs) - 1);
        rest= after_organism + strlen(hs);
    }

    // chromosome
    if (strncmp(rest, "chromosome ", 11) == 0) {

        rest+= 11;
        comma= strchr(rest, ',');
        sp= strchr(rest, ' ');

        // chromosome name ends at comma or space, whichever comes first
        //end= comma ? comma : sp;
        end= sp && (!comma || sp < comma) ? sp : comma;
        if (end) {
            metadata->chromosome= strndup(rest, end - rest);
            rest= end;
        }

        if(rest && *rest == ' ') {
            rest++;
        }

        comma= strchr(rest, ',');
        if(comma && comma > rest) {
            metadata->type= strndup(rest, comma - rest);
            rest= comma;
        }

        if(!metadata->type || strstr(metadata->type, "Primary Assembly") || strncmp(metadata->type, "GRCh", 4) == 0) {
            metadata->role= PRIMARY;
        } else if (strstr(metadata->type, "unlocalized")) {
            metadata->role= UNLOCALIZED; 
        } else if (strstr(metadata->type, "fix patch")) {
            metadata->role= FIX_PATCH;
        } else if (strstr(metadata->type, "novel patch")) {
            metadata->role= NOVEL_PATCH;
        } else {
            metadata->role= PRIMARY;
        }
    } else if (strncmp(rest, "unplaced", 8) == 0) {

        metadata->role= UNPLACED;
        metadata->chromosome= strdup("unknown");

        // Capture "unplaced genomic scaffold" as sequence type
        char *comma= strchr(rest, ',');
        if (comma) {
            metadata->type= strndup(rest, comma - rest);
            rest= comma;
        }
    } else if (strncmp(rest, "alternate", 9) == 0 || strstr(rest, "ALT_") != NULL) {
        metadata->role= AT_LOCUS;
    }

    // build
    build_start= strstr(rest, build_marker);
    if (build_start) {
        build_end= strpbrk(build_start, " ,\t");
        metadata->build= build_end
            ? strndup(build_start, build_end - build_start)
            : strdup(build_start);
        rest= build_end ? build_end : rest;
    }

    // locus
    last_space= strrchr(buf, ' ');
    if (last_space) {
        const char *candidate= last_space + 1;
        // Locus names contain underscores and are all-caps/digits
        if (strchr(candidate, '_')) {
            metadata->locus= strdup(candidate);
        }
    }
}

int index_fasta(GenomeFile *gf, const char *path) {
  
    long offset= 0;
    char line[1024];
    long length;
    int current_record= 0;
    SequenceMetadata *metadata;

    gf->record_count= 0;

    gf->file= fopen(path, "rb");
    if(!gf->file) {

        LOG(ERROR, "%s", "failed to open file");
        return -1;
    }

    while(fgets(line, sizeof(line), gf->file)) {

        line[strcspn(line, "\n")]= '\0';

        if(line[0] == '>') {

            current_record= gf->record_count;

            LOG(DEBUG, "Indexing {Offset: %ld, Data: %s}", offset, line);

            metadata= (SequenceMetadata *)realloc((SequenceMetadata *)gf->metadata, sizeof(SequenceMetadata) * (current_record + 1));
            if (metadata == (SequenceMetadata *) NULL) {
                LOG(WARN, "could not reallocate metadata");
                return -1;
            }

            parse_header(line, metadata);

            gf->metadata= metadata;
  
            gf->metadata[current_record].length= 0;
            gf->record_count++;

            LOG(
                DEBUG,
                "Index Created {Offset: %li, Organism: %s, Build: %s, Id: %s, Chromosome: %s, Role: %d, Type: %s, Locus: %s}",
                offset,
                metadata->organism,
                metadata->build,
                metadata->id,
                metadata->chromosome,
                metadata->role,
                metadata->type,
                metadata->locus
            );

        } else if (current_record != -1) {

             length= strlen(line);
             while(length > 0 && (line[length - 1] == '\n' || line[length -1] == '\r')) {
                 length--;
             }
             gf->metadata[current_record].length+= length;
        }
        offset= ftell(gf->file);
        gf->metadata[current_record].offset= offset;
    }

    return 0;
}

char *fetch_sequence(FILE *file, SequenceMetadata *metadata) {

    char buffer[1024];
    long line_length;
    long copy_length;
    char *result;
    long bases_read;

    result= calloc(metadata->length + 1, sizeof(char));
    if(!result) {
        return NULL;
    }

    fseek(file, metadata->offset, SEEK_SET);

    bases_read= 0;   
    while (bases_read < metadata->length && fgets(buffer, sizeof(buffer), file)) {

        line_length= strlen(buffer);
        while (line_length > 0 && (buffer[line_length-1] == '\n' || buffer[line_length-1] == '\r')) {
            line_length--;
        }

        copy_length= metadata->length - bases_read;
        if(copy_length > line_length) {
            copy_length= line_length;
        }

        memcpy(result + bases_read, buffer, copy_length); 
        bases_read+= copy_length;
    
    };

    result[bases_read]= '\0';
    return result;
}
