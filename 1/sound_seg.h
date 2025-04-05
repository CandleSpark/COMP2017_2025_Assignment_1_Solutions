#ifndef SOUND_SEG_H
#define SOUND_SEG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Audio segment node structure for linked storage
struct audio_node {
    int16_t* samples;        // Pointer to actual audio data
    size_t start;           // Starting position in original data
    size_t length;          // Number of samples in this node
    struct audio_node* next; // Pointer to next node
};

// Parent-child relationship node
struct parent_child_node {
    struct sound_seg* parent;    // Parent track
    size_t parent_start;         // Starting position in parent track
    size_t length;               // Length of shared data
    struct parent_child_node* next;
};

// Main track structure
struct sound_seg {
    struct audio_node* head;          // Head of audio data linked list
    struct parent_child_node* children; // List of child tracks
    struct parent_child_node* parent;   // Parent track info (if this is a child track)
    size_t total_length;               // Total number of samples
};

// Part 1: WAV file interaction and basic sound operations
int16_t* wav_load(const char* filename, size_t* length);
bool wav_save(const char* filename, int16_t* samples, size_t length);
struct sound_seg* tr_init(void);
void tr_destroy(struct sound_seg* track);
size_t tr_length(struct sound_seg* track);
bool tr_read(struct sound_seg* track, size_t pos, size_t len, int16_t* buffer);
bool tr_write(struct sound_seg* track, size_t pos, size_t len, const int16_t* buffer);
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len);

// Part 2: Advertisement identification
char* tr_identify(struct sound_seg* target, struct sound_seg* ad);

// Part 3: Complex insertion
bool tr_insert(struct sound_seg* dest_track, size_t destpos,
              struct sound_seg* src_track, size_t srcpos, size_t len);

// Part 4: Cleanup [COMP9017]
void tr_resolve(struct sound_seg** tracks, size_t num_tracks);

#endif // SOUND_SEG_H 