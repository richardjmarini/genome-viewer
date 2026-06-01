#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <logger.h>
#include <fasta.h>
#include <viewer.h>

void init_font(GLuint *font_texture) {

    unsigned char pixels[GLYPH_SIZE * GLYPH_SIZE * NUM_GLYPHS];
    memset(pixels, 0, sizeof(pixels));

    for (int g= 0; g < NUM_GLYPHS; g++) {
        for (int row= 0; row < GLYPH_SIZE; row++) {
            unsigned char bits= base_glyphs[g][1][row];
            for (int col= 0; col < GLYPH_SIZE; col++) {
                int px= g * GLYPH_SIZE + col;
                int py= row;
                pixels[py * GLYPH_SIZE * NUM_GLYPHS + px]=
                    (bits & (0x80 >> col)) ? 255 : 0;
            }
        }
    }

    glGenTextures(1, font_texture);
    glBindTexture(GL_TEXTURE_2D, *font_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
                 GLYPH_SIZE * NUM_GLYPHS, GLYPH_SIZE,
                 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

}

void draw_base_label(float x, float y, float z, char base, float angle, GLuint font_texture) {

    char *p;
    int g;
    float u0;
    float u1;
    float scale= 0.08f;

    p= strchr(glyph_chars, toupper((unsigned char)base));
    if (!p) {
        return;
    }

    g= p - glyph_chars;
    u0= (float)(g * GLYPH_SIZE) / (GLYPH_SIZE * NUM_GLYPHS);
    u1= (float)((g + 1) * GLYPH_SIZE) / (GLYPH_SIZE * NUM_GLYPHS);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, font_texture);

    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-angle, 1.0f, 0.0f, 0.0f); 
    base_color(base);

    glBegin(GL_QUADS);
    glTexCoord2f(u0, 1.0f); glVertex3f(0.0f, -scale, -scale);
    glTexCoord2f(u1, 1.0f); glVertex3f(0.0f,  scale, -scale);
    glTexCoord2f(u1, 0.0f); glVertex3f(0.0f,  scale,  scale);
    glTexCoord2f(u0, 0.0f); glVertex3f(0.0f, -scale,  scale);
    glEnd();

    glPopMatrix();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void render(char *sequence, SequenceMetadata *metadata, float angle, GLuint font_texture) {

    float t;
    int i;
    float x;
    float y1, y2;
    float z1, z2;

    // 5' rail
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    glColor3f(0.9f, 0.9f, 0.9f);
    for(i= 0; i < metadata->length; i++) {
        t= i * (2.0f * M_PI / BASES_PER_TURN);
        glVertex3f(i * RISE_PER_BASE, HELIX_RADIUS * cos(t), HELIX_RADIUS * sin(t));
    }
    glEnd();

    // 3' rail
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    glColor3f(0.9f, 0.9f, 0.9f);
    for(i= 0; i < metadata->length; i++) {
        t= i * (2.0f * M_PI / BASES_PER_TURN) + M_PI;
        glVertex3f(i * RISE_PER_BASE, HELIX_RADIUS * cos(t), HELIX_RADIUS * sin(t));
    }
    glEnd();

    // nucleotides as rungs 
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for(i= 0; i < metadata->length; i++) {
        t= i * (2.0f * M_PI / BASES_PER_TURN);
        x= i * RISE_PER_BASE;
        y1= HELIX_RADIUS * cos(t);
        z1= HELIX_RADIUS * sin(t);
        y2= HELIX_RADIUS * cos(t + M_PI);
        z2= HELIX_RADIUS * sin(t + M_PI);

        base_color(sequence[i]);
        glVertex3f(x, y1, z1);
        glVertex3f(x, 0.0f, 0.0f);

        base_color(complement(sequence[i]));
        glVertex3f(x, 0.0f, 0.0f);
        glVertex3f(x, y2, z2);
    }
    glEnd();

    // labels on each rail for each nucleotide
    for(i= 0; i < metadata->length; i++) {
        t= i * (2.0f * M_PI / BASES_PER_TURN);
        x= i * RISE_PER_BASE;
        y1= HELIX_RADIUS * cos(t);
        z1= HELIX_RADIUS * sin(t);
        y2= HELIX_RADIUS * cos(t + M_PI);
        z2= HELIX_RADIUS * sin(t + M_PI);

        // 5' label
        draw_base_label(x, y1 * 1.4f, z1 * 1.4f, sequence[i], angle, font_texture);
        // 3' complement label
        draw_base_label(x, y2 * 1.4f, z2 * 1.4f, complement(sequence[i]), angle, font_texture);
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {

     ViewerState *state= glfwGetWindowUserPointer(window);

     if(action != GLFW_PRESS && action != GLFW_REPEAT) {
         return;
     }

     switch(key) {
         case GLFW_KEY_UP:
             if(state->current_record > 0) {
                 state->current_record--;
                 state->base_offset= 0;
             }
             break;
         case GLFW_KEY_DOWN:
             if(state->current_record < state->gf->record_count - 1) { 
                 state->current_record++;
                 state->base_offset= 0;
             }
             break;
         case GLFW_KEY_LEFT:
             state->base_offset--;
             if(state->base_offset < 0) {
                 state->base_offset= 0;
             }
             break;
         case GLFW_KEY_RIGHT:
             state->base_offset++;
             if(state->base_offset + 1 > state->gf->metadata[state->current_record].length) {
                 state->base_offset= state->gf->metadata[state->current_record].length - 1;
             }
             break;
     }
}
