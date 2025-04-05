#ifndef SOUND_SEG_H
#define SOUND_SEG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// 音频片段节点结构，用于链式存储
struct audio_node {
    int16_t* samples;        // 指向实际音频数据的指针
    size_t start;           // 在原始数据中的起始位置
    size_t length;          // 节点包含的样本数
    struct audio_node* next; // 指向下一个节点
};

// 父子关系节点
struct parent_child_node {
    struct sound_seg* parent;    // 父音轨
    size_t parent_start;         // 在父音轨中的起始位置
    size_t length;               // 共享的长度
    struct parent_child_node* next;
};

// 主音轨结构
struct sound_seg {
    struct audio_node* head;          // 音频数据链表头
    struct parent_child_node* children; // 子音轨列表
    struct parent_child_node* parent;   // 父音轨信息（如果是子音轨）
    size_t total_length;               // 总样本数
};

// Part 1: WAV文件交互和基本声音操作
int16_t* wav_load(const char* filename, size_t* length);
bool wav_save(const char* filename, int16_t* samples, size_t length);
struct sound_seg* tr_init(void);
void tr_destroy(struct sound_seg* track);
size_t tr_length(struct sound_seg* track);
bool tr_read(struct sound_seg* track, size_t pos, size_t len, int16_t* buffer);
bool tr_write(struct sound_seg* track, size_t pos, size_t len, const int16_t* buffer);
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len);

// Part 2: 广告识别
char* tr_identify(struct sound_seg* target, struct sound_seg* ad);

// Part 3: 复杂插入
bool tr_insert(struct sound_seg* dest_track, size_t destpos,
              struct sound_seg* src_track, size_t srcpos, size_t len);

// Part 4: 清理 [COMP9017]
void tr_resolve(struct sound_seg** tracks, size_t num_tracks);

#endif // SOUND_SEG_H 