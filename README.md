# Audio Track Editor

A robust C implementation of an audio track editor that supports WAV file manipulation, advertisement identification, and complex audio segment operations.

## Features

### 1. WAV File Operations
- Load and save WAV audio files
- Support for 16-bit PCM format
- Robust error handling for file operations

### 2. Track Management
- Dynamic memory allocation for audio segments
- Efficient linked-list based storage structure
- Support for shared audio data between tracks

### 3. Core Operations
- `tr_write`: Write audio data to specific positions
- `tr_read`: Read audio data from tracks
- `tr_delete_range`: Delete audio segments
- `tr_insert`: Insert audio segments with data sharing
- `tr_identify`: Identify advertisement segments using cross-correlation
- `tr_resolve`: Resolve shared data dependencies between tracks

### 4. Memory Management
- Efficient memory usage through data sharing
- Proper cleanup of resources
- Memory leak prevention
- Clear ownership tracking of audio data

## Technical Details

### Data Structures
```c
struct audio_node {
    int16_t* samples;        // Audio data
    size_t start;           // Start position
    size_t length;          // Number of samples
    bool is_shared;         // Sharing status
    struct sound_seg* owner; // Original owner
    struct audio_node* next; // Next node
};
```

### Memory Safety
- AddressSanitizer integration
- Comprehensive memory ownership tracking
- Proper cleanup in all operations

## Building and Running

### Prerequisites
- GCC compiler
- Make build system

### Build Instructions
```bash
make clean
make
```

### Running the Editor
```bash
make editor
```

### Testing
The project includes comprehensive tests covering:
- Basic operations
- Edge cases
- Memory management
- Shared data handling

To run tests:
```bash
make test
```

## Implementation Details

### Audio Data Management
- Efficient linked list structure for audio segments
- Copy-on-write optimization for shared data
- Dynamic memory allocation for scalability

### Performance Considerations
- O(n) complexity for most operations
- Optimized memory usage through data sharing
- Efficient advertisement identification algorithm

## Error Handling
- Comprehensive input validation
- Robust memory allocation checks
- Clear error reporting

## License
This project is an academic assignment. All rights reserved.

## Acknowledgments
- COMP2017/COMP9017 Course Staff
- The University of Sydney 