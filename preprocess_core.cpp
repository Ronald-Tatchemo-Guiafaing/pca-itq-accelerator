#include <hls_stream.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>

// Hardware parameters
#define IN_WIDTH  112
#define IN_HEIGHT 112
#define POOL_SIZE 4
#define OUT_DIM   784 // 28x28 feature outputs

typedef ap_axis<8, 1, 1, 1> stream_type;

// Hardware absolute difference
#define ABS_DIFF(a, b) ((a > b) ? (a - b) : (b - a))

extern "C" { // C linkage, so the HLS name is not mangled

void img_preprocess_core(
    hls::stream<stream_type>& input_stream,
    hls::stream<stream_type>& output_stream,
    unsigned char threshold // binarisation threshold written by the PS
) {
    // AXI4-Stream channels
    #pragma HLS INTERFACE axis port=input_stream
    #pragma HLS INTERFACE axis port=output_stream
    // AXI4-Lite control bus for the threshold
    #pragma HLS INTERFACE s_axilite port=threshold bundle=CTRL_BUS
    #pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS

    // Line buffer
    unsigned char line_buf[2][IN_WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1

    unsigned char pool_reg = 0;
    int out_cnt = 0;

    // Scan the frame
    frame_loop_row: for (int r = 0; r < IN_HEIGHT; r++) {
        frame_loop_col: for (int c = 0; c < IN_WIDTH; c++) {
            // One pixel per clock
            #pragma HLS PIPELINE II=1

            // Blocking read
            stream_type in_pkt = input_stream.read();
            unsigned char curr_pixel = in_pkt.data;

            // Four neighbours from the line buffer
            unsigned char north = (r > 0) ? line_buf[0][c] : 0;
            unsigned char south = curr_pixel; 
            unsigned char west  = (c > 0) ? line_buf[1][c-1] : 0;
            unsigned char east  = curr_pixel; 

            // Gradient |E - W| + |N - S|
            int diff_ns = ABS_DIFF(north, south);
            int diff_ew = ABS_DIFF(east, west);
            int grad = diff_ns + diff_ew;

            // Threshold
            unsigned char binarized = (grad > threshold) ? 1 : 0;

            // Update the line buffer for the next pixel
            line_buf[0][c] = line_buf[1][c];
            line_buf[1][c] = curr_pixel;

            // 4x4 pool. For 0/1 values, OR is the same as max.
            pool_reg = pool_reg | binarized;

            // Emit one value at the end of each 4x4 block
            if ((r % POOL_SIZE == POOL_SIZE - 1) && (c % POOL_SIZE == POOL_SIZE - 1)) {
                stream_type out_pkt;
                out_pkt.data = pool_reg;
                out_pkt.keep = 1; out_pkt.strb = 1;
                out_pkt.user = 0; out_pkt.id = 0; out_pkt.dest = 0;
                
                // Last beat for the DMA
                out_pkt.last = (out_cnt == OUT_DIM - 1) ? 1 : 0;
                output_stream.write(out_pkt);

                // Clear the pool register
                pool_reg = 0; 
                out_cnt++;
            }
        }
    }
}

} // end of extern "C"
