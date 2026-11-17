# 1. Open the existing Vivado project
open_project ./pca_itq_7010_proj.xpr

# 2. Reset previous runs so a stale cache is not reused
reset_run synth_1
reset_run impl_1

# 3. Synthesis. -jobs 4 uses 4 CPU cores.
puts "Starting Synthesis..."
launch_runs synth_1 -jobs 4
wait_on_run synth_1

# 4. Implementation and bitstream
puts "Starting Implementation and Bitstream generation..."
launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

# 5. Export the XSA, including the bitstream, for Vitis
puts "Exporting Hardware Platform..."
write_hw_platform -fixed -include_bit -force -file ./pca_itq_7010_proj.xsa

puts "================================================="
puts "Success! Bitstream and XSA generation completed."
puts "================================================="
close_project
