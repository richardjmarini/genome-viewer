#ifndef __VIEWER_H__
#define __VIEWER_H__

#define BASES_PER_TURN 25.5f
#define HELIX_RADIUS 1.0f
#define RISE_PER_BASE 0.34f
#define MAX_BASES 200
#define GLYPH_SIZE 8
#define NUM_GLYPHS 5  // A T G C N

typedef struct {
    GenomeFile *gf;
    int current_record;
    long base_offset;
} ViewerState;

static inline void base_color(char base) {
    switch(toupper((unsigned char)base)) {
        case 'A': glColor3f(0.2f, 0.8f, 0.2f); break;
        case 'T': glColor3f(0.8f, 0.2f, 0.2f); break;
        case 'G': glColor3f(0.8f, 0.8f, 0.2f); break;
        case 'C': glColor3f(0.2f, 0.2f, 0.8f); break;
        default:  glColor3f(0.8f, 0.8f, 0.8f); break;
    }
}

static inline char complement(char base) {
    switch(toupper((unsigned char)base)) {
        case 'A': return 'T';
        case 'T': return 'A';
        case 'G': return 'C';
        case 'C': return 'G';
        default:  return 'N';
    }
}

static const unsigned char font8x8[96][8]= {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};

static const unsigned char base_glyphs[5][2][8] = {
    //  char   bitmask rows
    { "A", { 0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00 } },
    { "T", { 0x7E, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 } },
    { "G", { 0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00 } },
    { "C", { 0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00 } },
    { "N", { 0x66, 0x76, 0x7E, 0x6E, 0x66, 0x66, 0x66, 0x00 } },
};

static const char glyph_chars[] = "ATGCN";

void init_font(GLuint *font_texture);
void render(char *sequence, SequenceMetadata *metadata);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);


#endif
