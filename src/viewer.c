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
                pixels[py * GLYPH_SIZE * NUM_GLYPHS + px] =
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

    char *p= strchr(glyph_chars, toupper((unsigned char)base));
    if (!p) return;
    int g= p - glyph_chars;

    float u0= (float)(g * GLYPH_SIZE) / (GLYPH_SIZE * NUM_GLYPHS);
    float u1= (float)((g + 1) * GLYPH_SIZE) / (GLYPH_SIZE * NUM_GLYPHS);
    float scale= 0.15f;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, font_texture);

    glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(-angle, 0.0f, 1.0f, 0.0f);  // billboard
        base_color(base);
        glBegin(GL_QUADS);
            glTexCoord2f(u0, 1.0f); glVertex2f(-scale, -scale);
            glTexCoord2f(u1, 1.0f); glVertex2f( scale, -scale);
            glTexCoord2f(u1, 0.0f); glVertex2f( scale,  scale);
            glTexCoord2f(u0, 0.0f); glVertex2f(-scale,  scale);
        glEnd();
    glPopMatrix();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void render(char *sequence, SequenceMetadata *metadata) {

    float t;
    int i;
    float y;
    float x1;
    float z1;
    float x2;
    float z2;

    GLuint font_texture;

    init_font(&font_texture);

    // first rail
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    glColor3f(0.9f, 0.9f, 0.9f);
    for(i= 0; i < metadata->length;  i++) {
        t= i * (2.0f * M_PI / BASES_PER_TURN);
        glVertex3f(HELIX_RADIUS * cos(t), i * RISE_PER_BASE, HELIX_RADIUS * sin(t));
    }
    glEnd();

    // second rail
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    glColor3f(0.9f, 0.9f, 0.9f);
    for (i= 0; i < metadata->length; i++) {
        t= i * (2.0f * M_PI / BASES_PER_TURN) + M_PI;
        glVertex3f(HELIX_RADIUS * cos(t), i * RISE_PER_BASE, HELIX_RADIUS * sin(t));
    }
    glEnd();

    for (i= 0; i < metadata->length; i++) {
        t= i * (2.0f * M_PI / BASES_PER_TURN);
        y= i * RISE_PER_BASE;
        x1= HELIX_RADIUS * cos(t);
        x2= HELIX_RADIUS * cos(t + M_PI);
        z1= HELIX_RADIUS * sin(t);
        z2= HELIX_RADIUS * sin(t + M_PI);

        draw_base_label(x1 * 1.5f, y, z1 * 1.5f, sequence[i], 0.0, font_texture);
        draw_base_label(x2 * 1.4f, y, z2 * 1.4f, complement(sequence[i]), 0.0, font_texture);

    }

    // draw base pairs as rungs
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (i= 0; i < metadata->length; i++) {
        t= i * (2.0f * M_PI / BASES_PER_TURN);
        x1= HELIX_RADIUS * cos(t);
        y= i * RISE_PER_BASE;
        z1= HELIX_RADIUS * sin(t);
        x2= HELIX_RADIUS * cos(t + M_PI);
        z2= HELIX_RADIUS * sin(t + M_PI);

        // strand 1
        base_color(sequence[i]);
        glVertex3f(x1, y, z1);
        glVertex3f(0.0f, y, 0.0f);   // midpoint

        // strand 2 
        base_color(complement(sequence[i]));
        glVertex3f(0.0f, y, 0.0f);
        glVertex3f(x2, y, z2);
    }
    glEnd();

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
