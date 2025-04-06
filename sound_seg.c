#include "sound_seg.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// WAV chunk header structure
struct chunk_header {
    char id[4];
    uint32_t size;
};

// Forward declarations of static functions
static struct audio_node* create_shared_node(struct sound_seg* owner,
                                           size_t start, size_t length);
static bool split_node(struct audio_node* node, size_t pos);

// WAV format chunk
struct fmt_chunk {
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

// WAV file header structure
struct wav_header {
    char riff_id[4];
    uint32_t size;
    char wave_id[4];
    char fmt_id[4];
    uint32_t fmt_size;
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_id[4];
    uint32_t data_size;
};

// Part 1: WAV file interaction and basic sound operations
int16_t* wav_load(const char* filename, size_t* length) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("wav_load: Cannot open file\n");
        return NULL;
    }

    // Read RIFF header
    struct chunk_header riff_header;
    if (fread(&riff_header, sizeof(riff_header), 1, file) != 1) {
        printf("wav_load: Failed to read RIFF header\n");
        fclose(file);
        return NULL;
    }

    // Verify RIFF header
    if (memcmp(riff_header.id, "RIFF", 4) != 0) {
        printf("wav_load: Invalid RIFF header\n");
        fclose(file);
        return NULL;
    }

    // Read WAVE ID
    char wave_id[4];
    if (fread(wave_id, sizeof(wave_id), 1, file) != 1) {
        printf("wav_load: Failed to read WAVE ID\n");
        fclose(file);
        return NULL;
    }

    // Verify WAVE format
    if (memcmp(wave_id, "WAVE", 4) != 0) {
        printf("wav_load: Invalid WAVE format\n");
        fclose(file);
        return NULL;
    }

    struct fmt_chunk fmt;
    bool found_fmt = false;
    bool found_data = false;
    int16_t* samples = NULL;
    uint32_t data_size = 0;

    // Read chunks until we find both fmt and data
    while (!found_data) {
        struct chunk_header chunk;
        if (fread(&chunk, sizeof(chunk), 1, file) != 1) {
            printf("wav_load: Failed to read chunk header\n");
            break;
        }

        printf("Found chunk: %.4s, size: %u\n", chunk.id, chunk.size);

        if (memcmp(chunk.id, "fmt ", 4) == 0) {
            // Read format chunk
            if (fread(&fmt, sizeof(fmt), 1, file) != 1) {
                printf("wav_load: Failed to read fmt chunk\n");
                break;
            }
            found_fmt = true;
            
            // Skip any extra format bytes
            if (chunk.size > sizeof(fmt)) {
                fseek(file, chunk.size - sizeof(fmt), SEEK_CUR);
            }
        }
        else if (memcmp(chunk.id, "data", 4) == 0) {
            if (!found_fmt) {
                printf("wav_load: Found data before fmt chunk\n");
                break;
            }

            data_size = chunk.size;
            samples = malloc(data_size);
            if (!samples) {
                printf("wav_load: Failed to allocate memory\n");
                break;
            }

            size_t read_size = fread(samples, 1, data_size, file);
            if (read_size != data_size) {
                printf("wav_load: Failed to read data. Expected %u bytes, got %zu bytes\n",
                       data_size, read_size);
                free(samples);
                samples = NULL;
                break;
            }
            found_data = true;
        }
        else {
            // Skip unknown chunk
            printf("Skipping chunk: %.4s\n", chunk.id);
            if (fseek(file, chunk.size, SEEK_CUR) != 0) {
                printf("wav_load: Failed to skip chunk\n");
                break;
            }
        }
    }

    fclose(file);

    if (samples && found_fmt && found_data) {
        printf("WAV File Info:\n");
        printf("Format: %u\n", fmt.format);
        printf("Channels: %u\n", fmt.channels);
        printf("Sample Rate: %u\n", fmt.sample_rate);
        printf("Bits per Sample: %u\n", fmt.bits_per_sample);
        printf("Data Size: %u\n", data_size);
        
        *length = data_size / sizeof(int16_t);
        return samples;
    }

    if (samples) {
        free(samples);
    }
    return NULL;
}

bool wav_save(const char* filename, int16_t* samples, size_t length) {
    FILE* file = fopen(filename, "wb");
    if (!file) return false;

    struct wav_header header = {
        .riff_id = "RIFF",
        .wave_id = "WAVE",
        .fmt_id = "fmt ",
        .data_id = "data",
        .fmt_size = 16,
        .format = 1, // PCM
        .channels = 1,
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .block_align = 2,
        .byte_rate = 88200,
        .data_size = length * sizeof(int16_t),
        .size = 36 + length * sizeof(int16_t)
    };

    if (fwrite(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return false;
    }

    if (fwrite(samples, sizeof(int16_t), length, file) != length) {
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

struct sound_seg* tr_init(void) {
    struct sound_seg* track = calloc(1, sizeof(struct sound_seg));
    if (!track) return NULL;
    
    track->head = NULL;
    track->children = NULL;
    track->parents = NULL;
    track->total_length = 0;
    return track;
}

void tr_destroy(struct sound_seg* track) {
    if (!track) return;

    struct audio_node* curr = track->head;
    while (curr) {
        struct audio_node* next = curr->next;
        if (curr->samples && !curr->is_shared) {
            free(curr->samples);
        }
        free(curr);
        curr = next;
    }

    // Free parent-child relationship nodes
    struct parent_child_node* child = track->children;
    while (child) {
        struct parent_child_node* next = child->next;
        free(child);
        child = next;
    }

    struct parent_child_node* parent = track->parents;
    while (parent) {
        struct parent_child_node* next = parent->next;
        free(parent);
        parent = next;
    }

    free(track);
}

size_t tr_length(struct sound_seg* track) {
    return track ? track->total_length : 0;
}

bool tr_read(struct sound_seg* track, size_t pos, size_t len, int16_t* buffer) {
    if (!track || !buffer || pos + len > track->total_length) return false;

    size_t buffer_pos = 0;
    struct audio_node* node = track->head;
    
    // Find the starting node
    while (node && pos >= node->length) {
        pos -= node->length;
        node = node->next;
    }

    // Read data
    while (node && buffer_pos < len) {
        size_t copy_len = len - buffer_pos;
        if (copy_len > node->length - pos) {
            copy_len = node->length - pos;
        }

        memcpy(buffer + buffer_pos, 
               node->samples + node->start + pos,
               copy_len * sizeof(int16_t));

        buffer_pos += copy_len;
        pos = 0;
        node = node->next;
    }

    return true;
}

bool tr_write(struct sound_seg* track, size_t pos, size_t len, const int16_t* buffer) {
    if (!track || !buffer) return false;
    if (len == 0) return true;

    // 找到写入位置
    struct audio_node* curr = track->head;
    struct audio_node* prev = NULL;
    size_t curr_pos = 0;

    while (curr && curr_pos + curr->length <= pos) {
        curr_pos += curr->length;
        prev = curr;
        curr = curr->next;
    }

    // 如果写入位置超出当前长度，需要创建新节点
    if (!curr && pos >= track->total_length) {
        struct audio_node* new_node = malloc(sizeof(struct audio_node));
        if (!new_node) return false;

        new_node->samples = malloc(len * sizeof(int16_t));
        if (!new_node->samples) {
            free(new_node);
            return false;
        }

        memcpy(new_node->samples, buffer, len * sizeof(int16_t));
        new_node->start = 0;
        new_node->length = len;
        new_node->is_shared = false;
        new_node->owner = NULL;
        new_node->next = NULL;

        if (prev) {
            prev->next = new_node;
        } else {
            track->head = new_node;
        }

        track->total_length = pos + len;
        return true;
    }

    // 处理写入位置在现有节点内的情况
    while (len > 0 && curr) {
        size_t write_offset = pos - curr_pos;
        size_t write_len = len;
        
        if (write_len > curr->length - write_offset) {
            write_len = curr->length - write_offset;
        }

        // 如果是共享节点，需要创建新的非共享副本
        if (curr->is_shared) {
            int16_t* new_samples = malloc(curr->length * sizeof(int16_t));
            if (!new_samples) return false;

            // 复制原有数据
            memcpy(new_samples, curr->samples + curr->start, curr->length * sizeof(int16_t));
            
            // 写入新数据
            memcpy(new_samples + write_offset, buffer, write_len * sizeof(int16_t));

            // 更新节点
            curr->samples = new_samples;
            curr->start = 0;
            curr->is_shared = false;
            curr->owner = NULL;
        } else {
            // 直接写入非共享节点
            memcpy(curr->samples + curr->start + write_offset, 
                   buffer, write_len * sizeof(int16_t));
        }

        // 更新位置和长度
        buffer += write_len;
        len -= write_len;
        pos += write_len;
        curr_pos += curr->length;
        curr = curr->next;
    }

    // 如果还有数据需要写入，创建新节点
    if (len > 0) {
        struct audio_node* new_node = malloc(sizeof(struct audio_node));
        if (!new_node) return false;

        new_node->samples = malloc(len * sizeof(int16_t));
        if (!new_node->samples) {
            free(new_node);
            return false;
        }

        memcpy(new_node->samples, buffer, len * sizeof(int16_t));
        new_node->start = 0;
        new_node->length = len;
        new_node->is_shared = false;
        new_node->owner = NULL;
        new_node->next = curr;

        if (prev) {
            prev->next = new_node;
        } else {
            track->head = new_node;
        }

        track->total_length = pos + len;
    }

    // 更新总长度（如果需要）
    if (pos + len > track->total_length) {
        track->total_length = pos + len;
    }

    return true;
}

// 辅助函数：分割节点
static bool split_node(struct audio_node* node, size_t pos) {
    if (!node || pos >= node->length) return false;

    struct audio_node* new_node = malloc(sizeof(struct audio_node));
    if (!new_node) return false;

    // 设置新节点
    new_node->samples = node->samples;
    new_node->start = node->start + pos;
    new_node->length = node->length - pos;
    new_node->is_shared = node->is_shared;
    new_node->owner = node->owner;
    new_node->next = node->next;

    // 更新原节点
    node->length = pos;
    node->next = new_node;

    return true;
}

bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len) {
    // 基本参数检查
    if (!track || pos + len > track->total_length) return false;
    if (len == 0) return true;

    // 检查子段引用
    struct parent_child_node* child = track->children;
    while (child) {
        if (child->parent_start < pos + len && 
            child->parent_start + child->length > pos) {
            return false;  // 存在子段引用，不能删除
        }
        child = child->next;
    }

    // 找到起始节点
    struct audio_node* curr = track->head;
    struct audio_node* prev = NULL;
    size_t curr_pos = 0;

    // 定位到起始位置
    while (curr && curr_pos + curr->length <= pos) {
        curr_pos += curr->length;
        prev = curr;
        curr = curr->next;
    }

    if (!curr) return false;

    // 如果删除起点在节点中间，先分割节点
    if (pos > curr_pos) {
        if (!split_node(curr, pos - curr_pos)) {
            return false;
        }
        prev = curr;
        curr = curr->next;
        curr_pos = pos;
    }

    // 删除完整的节点
    while (curr && curr_pos + curr->length <= pos + len) {
        struct audio_node* to_delete = curr;
        curr = curr->next;
        curr_pos += to_delete->length;

        // 更新链表
        if (prev) {
            prev->next = curr;
        } else {
            track->head = curr;
        }

        // 释放节点
        if (!to_delete->is_shared) {
            free(to_delete->samples);
        }
        free(to_delete);
    }

    // 处理最后一个节点（如果需要）
    if (curr && curr_pos < pos + len) {
        size_t remaining = pos + len - curr_pos;
        
        if (remaining < curr->length) {
            if (curr->is_shared) {
                // 保存原始节点指针
                struct audio_node* node_to_delete = curr;
                
                // 对于共享节点，使用分割
                if (!split_node(node_to_delete, remaining)) {
                    return false;
                }
                
                // 更新链表连接
                if (prev) {
                    prev->next = node_to_delete->next;
                } else {
                    track->head = node_to_delete->next;
                }
                
                // curr 指向保留的部分
                curr = node_to_delete->next;
                
                // 释放代表被删除部分的节点结构体
                free(node_to_delete);
            } else {
                // 对于非共享节点，使用 memmove + realloc
                size_t keep_len = curr->length - remaining;
                if (keep_len > 0) {
                    memmove(curr->samples, curr->samples + remaining,
                           keep_len * sizeof(int16_t));
                    
                    int16_t* resized = realloc(curr->samples, keep_len * sizeof(int16_t));
                    if (!resized && keep_len > 0) {
                        return false;
                    }
                    curr->samples = resized;
                } else {
                    free(curr->samples);
                    curr->samples = NULL;
                }
                curr->length = keep_len;
                curr->start = 0;
            }
        } else {
            // 删除整个节点
            if (!curr->is_shared) {
                free(curr->samples);
            }
            free(curr);
            if (prev) {
                prev->next = curr->next;
            } else {
                track->head = curr->next;
            }
        }
    }

    // 更新总长度
    track->total_length -= len;
    return true;
}

// Part 2: Advertisement identification
static double cross_correlation(const int16_t* x, const int16_t* y, 
                              size_t len_x, size_t len_y, size_t offset) {
    double sum = 0;
    double norm_x = 0;
    double norm_y = 0;

    for (size_t i = 0; i < len_y && (i + offset) < len_x; i++) {
        sum += (double)x[i + offset] * y[i];
        norm_x += (double)x[i + offset] * x[i + offset];
        norm_y += (double)y[i] * y[i];
    }

    // Check for zero division
    if (norm_x == 0 || norm_y == 0) {
        return 0;
    }

    return sum / sqrt(norm_x * norm_y);
}

char* tr_identify(struct sound_seg* target, struct sound_seg* ad) {
    if (!target || !ad || ad->total_length > target->total_length) 
        return strdup("");

    // Calculate advertisement's autocorrelation (zero delay)
    int16_t* ad_buffer = malloc(ad->total_length * sizeof(int16_t));
    if (!ad_buffer) return strdup("");
    tr_read(ad, 0, ad->total_length, ad_buffer);

    double ad_autocorr = cross_correlation(ad_buffer, ad_buffer, 
                                         ad->total_length, ad->total_length, 0);
    double threshold = ad_autocorr * 0.95;

    // Search for advertisement in target
    char* result = NULL;
    size_t result_capacity = 256;
    result = malloc(result_capacity);
    if (!result) {
        free(ad_buffer);
        return strdup("");
    }
    result[0] = '\0';

    int16_t* target_buffer = malloc(target->total_length * sizeof(int16_t));
    if (!target_buffer) {
        free(ad_buffer);
        free(result);
        return strdup("");
    }
    tr_read(target, 0, target->total_length, target_buffer);

    for (size_t i = 0; i <= target->total_length - ad->total_length; i++) {
        double corr = cross_correlation(target_buffer, ad_buffer,
                                      target->total_length, ad->total_length, i);
        
        if (corr >= threshold) {
            char temp[64];
            snprintf(temp, sizeof(temp), "%zu, %zu\n", 
                    i, i + ad->total_length - 1);
            
            size_t needed = strlen(result) + strlen(temp) + 1;
            if (needed > result_capacity) {
                result_capacity *= 2;
                char* new_result = realloc(result, result_capacity);
                if (!new_result) {
                    free(ad_buffer);
                    free(target_buffer);
                    free(result);
                    return strdup("");
                }
                result = new_result;
            }
            
            strcat(result, temp);
            i += ad->total_length - 1; // Skip matched portion
        }
    }

    free(ad_buffer);
    free(target_buffer);

    // Remove trailing newline (if exists)
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == '\n') {
        result[len - 1] = '\0';
    }

    return result;
}

// Part 3: Complex insertion
bool tr_insert(struct sound_seg* dest_track, size_t destpos,
              struct sound_seg* src_track, size_t srcpos, size_t len) {
    // 基本参数检查
    if (!dest_track || !src_track || 
        srcpos + len > src_track->total_length ||
        destpos > dest_track->total_length) return false;
    
    if (len == 0) return true;

    // 找到源节点
    struct audio_node* src_node = src_track->head;
    size_t src_curr_pos = 0;

    while (src_node && src_curr_pos + src_node->length <= srcpos) {
        src_curr_pos += src_node->length;
        src_node = src_node->next;
    }

    if (!src_node) return false;

    // 创建共享节点
    struct audio_node* shared_node = malloc(sizeof(struct audio_node));
    if (!shared_node) return false;

    shared_node->samples = src_node->samples;
    shared_node->start = src_node->start + (srcpos - src_curr_pos);
    shared_node->length = len;
    shared_node->is_shared = true;
    shared_node->owner = src_track;

    // 找到目标位置
    struct audio_node* dest_curr = dest_track->head;
    struct audio_node* dest_prev = NULL;
    size_t dest_curr_pos = 0;

    while (dest_curr && dest_curr_pos + dest_curr->length <= destpos) {
        dest_curr_pos += dest_curr->length;
        dest_prev = dest_curr;
        dest_curr = dest_curr->next;
    }

    // 如果插入位置在现有节点内部，需要分割该节点
    if (dest_curr && dest_curr_pos < destpos) {
        // 创建新节点存储分割后的后半部分
        struct audio_node* split_node = malloc(sizeof(struct audio_node));
        if (!split_node) {
            free(shared_node);
            return false;
        }

        // 设置分割节点
        split_node->samples = dest_curr->samples;
        split_node->start = dest_curr->start + (destpos - dest_curr_pos);
        split_node->length = dest_curr->length - (destpos - dest_curr_pos);
        split_node->is_shared = dest_curr->is_shared;
        split_node->owner = dest_curr->owner;
        split_node->next = dest_curr->next;

        // 更新当前节点
        dest_curr->length = destpos - dest_curr_pos;
        dest_curr->next = shared_node;
        shared_node->next = split_node;
        
        // 更新指针关系
        dest_prev = dest_curr;
        dest_curr = split_node;
    } else {
        // 直接插入节点
        shared_node->next = dest_curr;
        if (dest_prev) {
            dest_prev->next = shared_node;
        } else {
            dest_track->head = shared_node;
        }
    }

    // 创建父子关系节点
    struct parent_child_node* relation = malloc(sizeof(struct parent_child_node));
    if (!relation) {
        // 回滚节点插入
        if (dest_prev) {
            dest_prev->next = dest_curr;
        } else {
            dest_track->head = dest_curr;
        }
        free(shared_node);
        return false;
    }

    relation->parent = src_track;
    relation->parent_start = srcpos;
    relation->child_start = destpos;
    relation->length = len;
    relation->next = dest_track->parents;
    dest_track->parents = relation;

    // 添加到源轨道的子节点列表
    struct parent_child_node* child_relation = malloc(sizeof(struct parent_child_node));
    if (!child_relation) {
        // 回滚所有更改
        if (dest_prev) {
            dest_prev->next = dest_curr;
        } else {
            dest_track->head = dest_curr;
        }
        free(shared_node);
        free(relation);
        return false;
    }

    child_relation->parent = dest_track;
    child_relation->parent_start = srcpos;
    child_relation->child_start = destpos;
    child_relation->length = len;
    child_relation->next = src_track->children;
    src_track->children = child_relation;

    // 简单更新总长度
    dest_track->total_length += len;

    return true;
}

// Helper function to create a new audio node that shares data
static struct audio_node* create_shared_node(struct sound_seg* owner,
                                           size_t start, size_t length) {
    struct audio_node* node = malloc(sizeof(struct audio_node));
    if (!node) return NULL;

    node->samples = NULL;  // Will be set by the caller
    node->start = start;
    node->length = length;
    node->is_shared = true;
    node->owner = owner;
    node->next = NULL;

    return node;
}

// Part 4: Cleanup
void tr_resolve(struct sound_seg** tracks, size_t num_tracks) {
    if (!tracks || num_tracks == 0) return;

    for (size_t i = 0; i < num_tracks; i++) {
        struct sound_seg* track = tracks[i];
        if (!track) continue;

        for (size_t j = 0; j < num_tracks; j++) {
            struct sound_seg* other = tracks[j];
            if (!other || i == j) continue;

            // 处理 other 作为 track 的子轨道的情况
            struct parent_child_node* prev_child = NULL;
            struct parent_child_node* curr_child = track->children;

            while (curr_child) {
                if (curr_child->parent == other) {
                    // 复制共享数据
                    int16_t* buffer = malloc(curr_child->length * sizeof(int16_t));
                    if (!buffer) break;  // 内存分配失败，跳过此关系

                    if (tr_read(track, curr_child->parent_start, 
                              curr_child->length, buffer)) {
                        tr_write(other, curr_child->child_start, 
                                curr_child->length, buffer);
                    }
                    free(buffer);

                    // 从子列表中移除关系
                    if (prev_child) {
                        prev_child->next = curr_child->next;
                    } else {
                        track->children = curr_child->next;
                    }

                    // 从父列表中移除对应关系
                    struct parent_child_node* prev_parent = NULL;
                    struct parent_child_node* curr_parent = other->parents;
                    while (curr_parent) {
                        if (curr_parent->parent == track &&
                            curr_parent->parent_start == curr_child->parent_start &&
                            curr_parent->child_start == curr_child->child_start &&
                            curr_parent->length == curr_child->length) {
                            
                            if (prev_parent) {
                                prev_parent->next = curr_parent->next;
                            } else {
                                other->parents = curr_parent->next;
                            }
                            
                            free(curr_parent);
                            break;
                        }
                        prev_parent = curr_parent;
                        curr_parent = curr_parent->next;
                    }

                    struct parent_child_node* to_free = curr_child;
                    curr_child = curr_child->next;
                    free(to_free);
                } else {
                    prev_child = curr_child;
                    curr_child = curr_child->next;
                }
            }
        }
    }
} 