# 1. 打开现有的 Vivado 工程
open_project ./pca_itq_7010_proj.xpr

# 2. 重置之前的运行状态（防止缓存冲突）
reset_run synth_1
reset_run impl_1

# 3. 启动综合 (Synthesis)
# -jobs 4 表示使用 4 个 CPU 核心，你可以根据你的电脑配置修改（如 -jobs 8）
puts "Starting Synthesis..."
launch_runs synth_1 -jobs 4
wait_on_run synth_1

# 4. 启动实现 (Implementation) 并生成 Bitstream
puts "Starting Implementation and Bitstream generation..."
launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

# 5. 导出包含 bitstream 的硬件平台 (.xsa) 供 Vitis 使用
puts "Exporting Hardware Platform..."
write_hw_platform -fixed -include_bit -force -file ./pca_itq_7010_proj.xsa

puts "================================================="
puts "Success! Bitstream and XSA generation completed."
puts "================================================="
close_project
