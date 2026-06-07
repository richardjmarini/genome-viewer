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

int main(int argc, char *argv[]) {

    GLFWwindow *window;
    float angle= 0.0f;
    char *sequence= NULL;
    SequenceMetadata *metadata;
    SequenceMetadata render_metadata;
    int last_record= -1;
    int line_skip= 0;
    float aspect;
    GLuint font_texture;
    float scroll_x;
    GLFWmonitor *monitor;
    const GLFWvidmode *videoMode;
    long bases_remaining;
    long chunk_local_offset;
    char *render_sequence;

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
 
    monitor= glfwGetPrimaryMonitor();
    videoMode= glfwGetVideoMode(monitor);
    if(videoMode) {
        LOG(INFO, "Screen Resolution: %dx%d\n", videoMode->width, videoMode->height);
    }

    aspect= videoMode->width / videoMode->height;
    window= glfwCreateWindow((int)videoMode->width, (int)videoMode->height, "Genome-Viewer", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    ViewerState state= {
        .gf= gf,
        .current_record= 0,
        .base_offset= 0
    };

    glfwSetWindowUserPointer(window, &state);
    glfwSetKeyCallback(window, key_callback);
    init_font(&font_texture);

    while (!glfwWindowShouldClose(window)) {

        if (state.current_record != last_record || state.base_offset < state.chunk_start || state.base_offset + MAX_BASES > state.chunk_start + CHUNK_SIZE) {

            free(sequence);

            state.chunk_start= (state.base_offset > MAX_BASES) ? state.base_offset - MAX_BASES : 0;

            render_metadata= gf->metadata[state.current_record];

            LOG(DEBUG, "State {Record: %i, Offset: %li}", state.current_record, state.base_offset);

            line_skip= state.chunk_start / gf->line_length;
            render_metadata.offset+= state.chunk_start + line_skip;
            render_metadata.length= CHUNK_SIZE;

            bases_remaining= gf->metadata[state.current_record].length - state.chunk_start;
            if(render_metadata.length > bases_remaining) {
                render_metadata.length= bases_remaining;
            }

            sequence= fetch_sequence(gf->file, &render_metadata);
            metadata= &render_metadata; 
            last_record= state.current_record;

            LOG(DEBUG, "Rendering {Record: %i, Offset: %li, %s}", state.current_record, metadata->offset, sequence);
        }

        chunk_local_offset= state.base_offset - state.chunk_start;
        render_sequence= sequence + chunk_local_offset;
        render_metadata.length= MAX_BASES;// only show MAX_BASES worth
    
        bases_remaining= gf->metadata[state.current_record].length - state.base_offset;
        if (render_metadata.length > bases_remaining)
            render_metadata.length= bases_remaining;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(-aspect * 0.1f, aspect * 0.1f, -0.1f, 0.1f, 0.1f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        //scroll_x= -((state.base_offset + state.base_offset / gf->line_length) * RISE_PER_BASE) - (metadata->length * RISE_PER_BASE * 0.5f);
        scroll_x= -(render_metadata.length * RISE_PER_BASE * 0.5f);

        glTranslatef(scroll_x, 0.0f, -8.0f); // center it
        glRotatef(angle, 1.0f, 0.0f, 0.0f); // spin on x

        render(render_sequence, metadata, font_texture);

        angle+= 0.3f;   // auto-rotate

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glfwTerminate();

    free(gf->metadata);
    free(gf);

    return 0;
}
