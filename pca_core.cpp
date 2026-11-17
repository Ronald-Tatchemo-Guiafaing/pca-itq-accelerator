#include <hls_stream.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>

// 维度定义
#define D_DIM 784 // 输入特征维度 x (如 MNIST 展平大小)
#define C_DIM 64  // 输出哈希码位数 P 的列数

// AXI4-Stream 数据包定义 (16位量化输入，8位输出承载单 bit)
typedef ap_axis<16, 1, 1, 1> stream_in_type;  
typedef ap_axis<8, 1, 1, 1> stream_out_type; 

void pca_itq_accelerator(
    hls::stream<stream_in_type>&  input_stream,
    hls::stream<stream_out_type>& output_stream,
    const short weight_matrix[D_DIM][C_DIM] 
) {
    // 接口协议指令
    #pragma HLS INTERFACE axis port=input_stream
    #pragma HLS INTERFACE axis port=output_stream
    #pragma HLS INTERFACE s_axilite port=weight_matrix bundle=CTRL_BUS
    #pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS

    int accumulator[C_DIM];
    #pragma HLS ARRAY_PARTITION variable=accumulator complete dim=1

    // 初始化累加器
    init_loop: for (int i = 0; i < C_DIM; i++) {
        #pragma HLS UNROLL
        accumulator[i] = 0;
    }

    // 1. 矩阵向量乘加 (x * P)
    stream_in_type in_packet;
    mac_loop: for (int row = 0; row < D_DIM; row++) {
        #pragma HLS PIPELINE II=1 

        in_packet = input_stream.read();
        short feature_val = in_packet.data;

        for (int col = 0; col < C_DIM; col++) {
            #pragma HLS UNROLL
            accumulator[col] += feature_val * weight_matrix[row][col];
        }
    }

    // 2. 符号阶跃判决 sgn() 与结果流出
    stream_out_type out_packet;
    out_packet.keep = 1;
    out_packet.strb = 1;
    out_packet.user = 0;
    out_packet.id   = 0;
    out_packet.dest = 0;

    output_loop: for (int col = 0; col < C_DIM; col++) {
        #pragma HLS PIPELINE II=1
        
        // sgn() 映射：数字比较器瞬间将累加结果截断为 0 或 1
        if (accumulator[col] > 0) {
            out_packet.data = 1; 
        } else {
            out_packet.data = 0;
        }
        
        // 标记最后一个数据包，触发 DMA 中断[cite: 1]
        if (col == (C_DIM - 1)) {
            out_packet.last = 1;
        } else {
            out_packet.last = 0;
        }

        output_stream.write(out_packet);
    }
}