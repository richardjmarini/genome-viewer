#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <logger.h>
#include <fasta.h>
#include <render.h>

typedef struct {
    char *sequence;
    SequenceMetadata chunk;
} Genome;

typedef struct {
    GLFWwindow *window;
    GLuint font_texture;
    float aspect;
    GLFWmonitor *monitor;
    const GLFWvidmode *videoMode;
} Display;

void display(char *sequence, GenomeFile *gf, RenderState *state, Display *dis) { 

    SequenceMetadata metadata;
    long chunk_local_offset;
    char *render_sequence;
    long bases_remaining;
    float scroll_x;

    chunk_local_offset= state->base_offset - state->chunk_start;
    render_sequence= sequence + chunk_local_offset;
    metadata.length= MAX_BASES;// only show MAX_BASES worth

    bases_remaining= gf->metadata[state->current_record].length - state->base_offset;
    if (metadata.length > bases_remaining)
        metadata.length= bases_remaining;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-dis->aspect * 0.1f, dis->aspect * 0.1f, -0.1f, 0.1f, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    scroll_x= -(metadata.length * RISE_PER_BASE * 0.5f);

    glTranslatef(scroll_x, 0.0f, -8.0f); // center it
    glRotatef(state->angle, 1.0f, 0.0f, 0.0f); // spin on x

    render(render_sequence, &metadata, dis->font_texture);

    state->angle+= 0.3f;   // auto-rotate

    glfwSwapBuffers(dis->window);
    glfwPollEvents();
}

int main(int argc, char *argv[]) {

    Display dis;
    Genome genome= {
        .sequence= NULL
    };
    RenderState state= {
        .current_record= 0,
        .base_offset= 0,  
        .angle= 0.0f
    };
    int last_record= -1;
    int line_skip= 0;

    GenomeFile *gf= (GenomeFile *)malloc(sizeof(GenomeFile));
    gf->metadata= (SequenceMetadata *)malloc(sizeof(SequenceMetadata));
    LOG(INFO, "Reading %s", argv[1]);
     
    if(index_fasta(gf, argv[1]) != 0) {
        LOG(ERROR, "failed to index fasta file");
        return -1;
    }
    LOG(INFO, "Indexes Created {Total: %d}", gf->record_count);

    if(!glfwInit()) {
        LOG(ERROR, "could not initialize gl");
        return -1;
    }
 
    dis.monitor= glfwGetPrimaryMonitor();
    dis.videoMode= glfwGetVideoMode(dis.monitor);
    if(dis.videoMode) {
        LOG(INFO, "Screen Resolution: %dx%d\n", dis.videoMode->width, dis.videoMode->height);
    }

    dis.aspect= dis.videoMode->width / dis.videoMode->height;
    dis.window= glfwCreateWindow((int)dis.videoMode->width, (int)dis.videoMode->height, "Genome-Viewer", NULL, NULL);
    glfwMakeContextCurrent(dis.window);
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    state.gf= gf;

    glfwSetWindowUserPointer(dis.window, &state);
    glfwSetKeyCallback(dis.window, key_callback);
    init_font(&dis.font_texture);

    while (!glfwWindowShouldClose(dis.window)) {

        if (state.current_record != last_record || state.base_offset < state.chunk_start || state.base_offset + MAX_BASES > state.chunk_start + CHUNK_SIZE) {

            free(genome.sequence);

            state.chunk_start= (state.base_offset > MAX_BASES) ? state.base_offset - MAX_BASES : 0;

            genome.chunk= gf->metadata[state.current_record];

            LOG(DEBUG, "State {Record: %i, Offset: %li}", state.current_record, state.base_offset);

            line_skip= state.chunk_start / gf->line_length;
            genome.chunk.offset+= state.chunk_start + line_skip;
            genome.chunk.length= CHUNK_SIZE;

            genome.sequence= fetch_sequence(gf->file, &genome.chunk);

            last_record= state.current_record;

            LOG(DEBUG, "Rendering {Record: %i, Offset: %li, %s}", state.current_record, genome.chunk.offset, genome.sequence);
        }

        display(genome.sequence, gf, &state, &dis);

    }

    glfwTerminate();

    free(gf->metadata);
    free(gf);

    return 0;
}
