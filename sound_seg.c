#include "sound_seg.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// WAV文件头结构
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

// Part 1: WAV文件交互和基本声音操作
int16_t* wav_load(const char* filename, size_t* length) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    struct wav_header header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return NULL;
    }

    // 验证WAV文件格式
    if (memcmp(header.riff_id, "RIFF", 4) != 0 ||
        memcmp(header.wave_id, "WAVE", 4) != 0 ||
        memcmp(header.fmt_id, "fmt ", 4) != 0 ||
        memcmp(header.data_id, "data", 4) != 0) {
        fclose(file);
        return NULL;
    }

    *length = header.data_size / sizeof(int16_t);
    int16_t* samples = malloc(header.data_size);
    if (!samples) {
        fclose(file);
        return NULL;
    }

    if (fread(samples, 1, header.data_size, file) != header.data_size) {
        free(samples);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return samples;
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
    track->parent = NULL;
    track->total_length = 0;
    return track;
}

static void free_audio_nodes(struct audio_node* head) {
    while (head) {
        struct audio_node* next = head->next;
        free(head->samples);
        free(head);
        head = next;
    }
}

static void free_parent_child_nodes(struct parent_child_node* head) {
    while (head) {
        struct parent_child_node* next = head->next;
        free(head);
        head = next;
    }
}

void tr_destroy(struct sound_seg* track) {
    if (!track) return;

    // 释放音频节点
    free_audio_nodes(track->head);

    // 释放父子关系节点
    free_parent_child_nodes(track->children);
    free(track->parent);

    free(track);
}

size_t tr_length(struct sound_seg* track) {
    return track ? track->total_length : 0;
}

bool tr_read(struct sound_seg* track, size_t pos, size_t len, int16_t* buffer) {
    if (!track || !buffer || pos + len > track->total_length) return false;

    size_t buffer_pos = 0;
    struct audio_node* node = track->head;
    
    // 找到起始节点
    while (node && pos >= node->length) {
        pos -= node->length;
        node = node->next;
    }

    // 读取数据
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

    // 如果写入位置超出当前长度，需要扩展音轨
    if (pos + len > track->total_length) {
        size_t new_samples = pos + len - track->total_length;
        int16_t* new_data = calloc(new_samples, sizeof(int16_t));
        if (!new_data) return false;

        struct audio_node* new_node = malloc(sizeof(struct audio_node));
        if (!new_node) {
            free(new_data);
            return false;
        }

        new_node->samples = new_data;
        new_node->start = 0;
        new_node->length = new_samples;
        new_node->next = NULL;

        if (!track->head) {
            track->head = new_node;
        } else {
            struct audio_node* last = track->head;
            while (last->next) last = last->next;
            last->next = new_node;
        }

        track->total_length = pos + len;
    }

    // 写入数据
    size_t written = 0;
    struct audio_node* node = track->head;
    
    while (node && pos >= node->length) {
        pos -= node->length;
        node = node->next;
    }

    while (node && written < len) {
        size_t copy_len = len - written;
        if (copy_len > node->length - pos) {
            copy_len = node->length - pos;
        }

        memcpy(node->samples + node->start + pos,
               buffer + written,
               copy_len * sizeof(int16_t));

        written += copy_len;
        pos = 0;
        node = node->next;
    }

    return true;
}

bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len) {
    if (!track || pos + len > track->total_length) return false;

    // 检查是否有子片段引用这个范围
    struct parent_child_node* child = track->children;
    while (child) {
        if (child->parent_start < pos + len && 
            child->parent_start + child->length > pos) {
            return false;
        }
        child = child->next;
    }

    // 创建新节点存储删除后的数据
    struct audio_node* new_head = NULL;
    struct audio_node* new_tail = NULL;
    size_t new_length = track->total_length - len;

    // 复制前半部分
    if (pos > 0) {
        new_head = malloc(sizeof(struct audio_node));
        if (!new_head) return false;

        new_head->samples = malloc(pos * sizeof(int16_t));
        if (!new_head->samples) {
            free(new_head);
            return false;
        }

        tr_read(track, 0, pos, new_head->samples);
        new_head->start = 0;
        new_head->length = pos;
        new_head->next = NULL;
        new_tail = new_head;
    }

    // 复制后半部分
    if (pos + len < track->total_length) {
        struct audio_node* after = malloc(sizeof(struct audio_node));
        if (!after) {
            if (new_head) {
                free(new_head->samples);
                free(new_head);
            }
            return false;
        }

        size_t after_len = track->total_length - (pos + len);
        after->samples = malloc(after_len * sizeof(int16_t));
        if (!after->samples) {
            free(after);
            if (new_head) {
                free(new_head->samples);
                free(new_head);
            }
            return false;
        }

        tr_read(track, pos + len, after_len, after->samples);
        after->start = 0;
        after->length = after_len;
        after->next = NULL;

        if (new_tail)
            new_tail->next = after;
        else
            new_head = after;
    }

    // 释放原有节点
    free_audio_nodes(track->head);
    track->head = new_head;
    track->total_length = new_length;

    return true;
}

// Part 2: 广告识别
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

    return sum / sqrt(norm_x * norm_y);
}

char* tr_identify(struct sound_seg* target, struct sound_seg* ad) {
    if (!target || !ad || ad->total_length > target->total_length) 
        return strdup("");

    // 计算广告的自相关值（零延迟）
    int16_t* ad_buffer = malloc(ad->total_length * sizeof(int16_t));
    if (!ad_buffer) return strdup("");
    tr_read(ad, 0, ad->total_length, ad_buffer);

    double ad_autocorr = cross_correlation(ad_buffer, ad_buffer, 
                                         ad->total_length, ad->total_length, 0);
    double threshold = ad_autocorr * 0.95;

    // 在目标中搜索广告
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
            i += ad->total_length - 1; // 跳过已匹配的部分
        }
    }

    free(ad_buffer);
    free(target_buffer);

    // 移除最后的换行符（如果存在）
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == '\n') {
        result[len - 1] = '\0';
    }

    return result;
}

// Part 3: 复杂插入
bool tr_insert(struct sound_seg* dest_track, size_t destpos,
              struct sound_seg* src_track, size_t srcpos, size_t len) {
    if (!dest_track || !src_track || 
        srcpos + len > src_track->total_length ||
        destpos > dest_track->total_length) return false;

    // 创建新的父子关系节点
    struct parent_child_node* relation = malloc(sizeof(struct parent_child_node));
    if (!relation) return false;

    relation->parent = src_track;
    relation->parent_start = srcpos;
    relation->length = len;
    relation->next = dest_track->children;
    dest_track->children = relation;

    // 调整目标音轨的总长度
    if (destpos + len > dest_track->total_length) {
        dest_track->total_length = destpos + len;
    }

    return true;
}

// Part 4: 清理
void tr_resolve(struct sound_seg** tracks, size_t num_tracks) {
    if (!tracks || num_tracks == 0) return;

    for (size_t i = 0; i < num_tracks; i++) {
        struct sound_seg* track = tracks[i];
        if (!track) continue;

        for (size_t j = 0; j < num_tracks; j++) {
            struct sound_seg* other = tracks[j];
            if (!other || i == j) continue;

            // 检查是否存在直接父子关系
            struct parent_child_node* prev = NULL;
            struct parent_child_node* curr = other->children;

            while (curr) {
                if (curr->parent == track) {
                    // 复制共享数据
                    int16_t* buffer = malloc(curr->length * sizeof(int16_t));
                    if (!buffer) break;

                    tr_read(track, curr->parent_start, curr->length, buffer);
                    tr_write(other, curr->parent_start, curr->length, buffer);
                    free(buffer);

                    // 断开关系
                    if (prev)
                        prev->next = curr->next;
                    else
                        other->children = curr->next;

                    struct parent_child_node* to_free = curr;
                    curr = curr->next;
                    free(to_free);
                } else {
                    prev = curr;
                    curr = curr->next;
                }
            }
        }
    }
} 