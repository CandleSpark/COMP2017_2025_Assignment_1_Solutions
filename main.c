#include "sound_seg.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main() {
    // Initialize a new track
    struct sound_seg* track = tr_init();
    if (!track) {
        printf("Failed to initialize track\n");
        return 1;
    }

    const char* filename = "input.wav";
    // Load audio from WAV file
    size_t length;
    printf("Attempting to load file: %s\n", filename);
    
    // First check if the file can be opened
    FILE* test = fopen(filename, "rb");
    if (!test) {
        printf("Error opening file: %s\n", strerror(errno));
        tr_destroy(track);
        return 1;
    }
    fclose(test);
    
    int16_t* samples = wav_load(filename, &length);
    if (!samples) {
        printf("Failed to load WAV file: %s\n", filename);
        printf("Make sure the file is a valid WAV file\n");
        tr_destroy(track);
        return 1;
    }

    printf("Successfully loaded WAV file. Length: %zu samples\n", length);

    // Write data to track
    if (!tr_write(track, 0, length, samples)) {
        printf("Failed to write samples to track\n");
        free(samples);
        tr_destroy(track);
        return 1;
    }

    // Display track length
    printf("Track length: %zu samples\n", tr_length(track));

    // Read and display partial data
    int16_t buffer[100];  // Read first 100 samples
    if (tr_read(track, 0, 100, buffer)) {
        printf("First 100 samples:\n");
        for (int i = 0; i < 10; i++) {  // Only display first 10 samples
            printf("%d ", buffer[i]);
        }
        printf("...\n");
    }

    // Clean up resources
    free(samples);
    tr_destroy(track);
    return 0;
} 