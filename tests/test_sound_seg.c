#include "../sound_seg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 测试辅助函数
static void print_track_info(struct sound_seg* track) {
    printf("Track length: %zu\n", tr_length(track));
    struct audio_node* node = track->head;
    size_t node_count = 0;
    while (node) {
        printf("Node %zu: start=%zu, length=%zu, is_shared=%d\n",
               node_count++, node->start, node->length, node->is_shared);
        node = node->next;
    }
}

// 基本功能测试
static void test_basic_operations(void) {
    printf("\n=== Testing Basic Operations ===\n");
    
    // 初始化轨道
    struct sound_seg* track = tr_init();
    assert(track != NULL);
    assert(tr_length(track) == 0);
    assert(track->head == NULL);

    // 写入数据
    int16_t data[] = {1, 2, 3, 4, 5};
    assert(tr_write(track, 0, 5, data));
    assert(tr_length(track) == 5);

    // 读取数据
    int16_t buffer[5] = {0};
    assert(tr_read(track, 0, 5, buffer));
    for (int i = 0; i < 5; i++) {
        assert(buffer[i] == data[i]);
    }

    // 删除数据
    assert(tr_delete_range(track, 1, 2));
    assert(tr_length(track) == 3);

    tr_destroy(track);
    printf("Basic operations test passed!\n");
}

// 边界条件测试
static void test_edge_cases(void) {
    printf("\n=== Testing Edge Cases ===\n");
    
    struct sound_seg* track = tr_init();
    int16_t data[] = {1, 2, 3, 4, 5};

    // 空操作测试
    assert(tr_write(track, 0, 0, data));
    assert(tr_delete_range(track, 0, 0));
    assert(tr_length(track) == 0);

    // 写入后再写入
    assert(tr_write(track, 0, 5, data));
    assert(tr_write(track, 2, 2, data));
    assert(tr_length(track) == 5);

    // 删除全部数据
    assert(tr_delete_range(track, 0, 5));
    assert(tr_length(track) == 0);

    tr_destroy(track);
    printf("Edge cases test passed!\n");
}

// 共享节点测试
static void test_shared_nodes(void) {
    printf("\n=== Testing Shared Nodes ===\n");
    
    struct sound_seg* src = tr_init();
    struct sound_seg* dest = tr_init();
    int16_t data[] = {1, 2, 3, 4, 5};

    // 写入源数据
    assert(tr_write(src, 0, 5, data));

    // 插入共享数据
    assert(tr_insert(dest, 0, src, 1, 3));
    assert(tr_length(dest) == 3);

    // 验证共享数据
    int16_t buffer[3] = {0};
    assert(tr_read(dest, 0, 3, buffer));
    assert(buffer[0] == 2);
    assert(buffer[1] == 3);
    assert(buffer[2] == 4);

    // 写入共享节点
    int16_t new_data[] = {10};
    assert(tr_write(dest, 1, 1, new_data));

    // 验证写入后数据分离
    assert(tr_read(dest, 1, 1, buffer));
    assert(buffer[0] == 10);

    // 验证源数据未变
    assert(tr_read(src, 2, 1, buffer));
    assert(buffer[0] == 3);

    tr_destroy(src);
    tr_destroy(dest);
    printf("Shared nodes test passed!\n");
}

// 复杂操作测试
static void test_complex_operations(void) {
    printf("\n=== Testing Complex Operations ===\n");
    
    struct sound_seg* tracks[3];
    for (int i = 0; i < 3; i++) {
        tracks[i] = tr_init();
    }

    int16_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 创建复杂的共享关系
    assert(tr_write(tracks[0], 0, 10, data));
    assert(tr_insert(tracks[1], 0, tracks[0], 2, 4));
    assert(tr_insert(tracks[2], 0, tracks[1], 1, 2));

    // 解析共享关系
    tr_resolve(tracks, 3);

    // 验证数据独立性
    int16_t buffer[4];
    assert(tr_read(tracks[1], 0, 4, buffer));
    assert(buffer[0] == 3);
    assert(buffer[1] == 4);
    assert(buffer[2] == 5);
    assert(buffer[3] == 6);

    for (int i = 0; i < 3; i++) {
        tr_destroy(tracks[i]);
    }
    printf("Complex operations test passed!\n");
}

int main(void) {
    test_basic_operations();
    test_edge_cases();
    test_shared_nodes();
    test_complex_operations();
    
    printf("\nAll tests passed successfully!\n");
    return 0;
} 