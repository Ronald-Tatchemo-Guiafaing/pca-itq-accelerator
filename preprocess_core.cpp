#include <hls_stream.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>

// 硬件参数化定义
#define IN_WIDTH  112
#define IN_HEIGHT 112
#define POOL_SIZE 4
#define OUT_DIM   784 // 28x28 维特征输出

typedef ap_axis<8, 1, 1, 1> stream_type;

// abs() 的硬件宏
#define ABS_DIFF(a, b) ((a > b) ? (a - b) : (b - a))

extern "C" { // <--- 添加在这里，强制禁止 C++ 名字修饰

void img_preprocess_core(
    hls::stream<stream_type>& input_stream,
    hls::stream<stream_type>& output_stream,
    unsigned char threshold // 由 PS 端动态配置的二值化阈值
) {
    // AXI4-Stream 数据流通道
    #pragma HLS INTERFACE axis port=input_stream
    #pragma HLS INTERFACE axis port=output_stream
    // AXI4-Lite 控制总线，用于 PS 端写入阈值
    #pragma HLS INTERFACE s_axilite port=threshold bundle=CTRL_BUS
    #pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS

    // 实例化硬件行缓存 (Line Buffer)
    unsigned char line_buf[2][IN_WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1

    unsigned char pool_reg = 0;
    int out_cnt = 0;

    // 图像级流水线扫描
    frame_loop_row: for (int r = 0; r < IN_HEIGHT; r++) {
        frame_loop_col: for (int c = 0; c < IN_WIDTH; c++) {
            // 指示综合器将循环打造为每个时钟周期吞吐 1 个像素的高速流水线
            #pragma HLS PIPELINE II=1

            // 阻塞读取输入流
            stream_type in_pkt = input_stream.read();
            unsigned char curr_pixel = in_pkt.data;

            // 1. 获取四邻域像素 (利用行缓存)
            unsigned char north = (r > 0) ? line_buf[0][c] : 0;
            unsigned char south = curr_pixel; 
            unsigned char west  = (c > 0) ? line_buf[1][c-1] : 0;
            unsigned char east  = curr_pixel; 

            // 2. 硬件梯度计算: |E - W| + |N - S|
            int diff_ns = ABS_DIFF(north, south);
            int diff_ew = ABS_DIFF(east, west);
            int grad = diff_ns + diff_ew;

            // 3. 阈值比较与二值化判定
            unsigned char binarized = (grad > threshold) ? 1 : 0;

            // 更新硬件行缓存，为下一个时钟周期做准备
            line_buf[0][c] = line_buf[1][c];
            line_buf[1][c] = curr_pixel;

            // 4. 4x4 硬件最大池化 (降维)
            pool_reg = pool_reg | binarized;

            // 当 4x4 像素块扫描完毕时，向外输出 1 个降维后的特征点
            if ((r % POOL_SIZE == POOL_SIZE - 1) && (c % POOL_SIZE == POOL_SIZE - 1)) {
                stream_type out_pkt;
                out_pkt.data = pool_reg;
                out_pkt.keep = 1; out_pkt.strb = 1;
                out_pkt.user = 0; out_pkt.id = 0; out_pkt.dest = 0;
                
                // 为 DMA 标记最后一个数据包，触发中断
                out_pkt.last = (out_cnt == OUT_DIM - 1) ? 1 : 0;
                output_stream.write(out_pkt);

                // 寄存器复位
                pool_reg = 0; 
                out_cnt++;
            }
        }
    }
}

} // <--- extern "C" 结束大括号