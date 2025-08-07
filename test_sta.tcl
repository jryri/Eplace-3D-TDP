puts "reading lib.."
foreach libFile [glob "./ASAP7/LIB/*nldm*.lib"] {
    puts "lib: $libFile"
    read_liberty $libFile
}
read_liberty ./ASAP7/LIB/asapHBT.lib

puts "reading lef.."
read_lef ./ASAP7/techlef/asap7_tech_1x_201209.lef
foreach lef [glob "./ASAP7/LEF/*.lef"] {
    read_lef $lef
}


puts "reading def.."
#read_def ./solution/aes_cipher_top/aes_cipher_top-eDP.def
#read_def  ./after_buffered/aes_cipher_top_buf.def
read_def ./eplace_solution/aes_cipher_top_buf/aes_cipher_top_buf-eDP.def
read_sdc ./testcase/aes_cipher_top/aes_cipher_top.sdc
source ./ASAP7/setRC.tcl
estimate_parasitics -placement



# 5. 詳細配置
puts "Running detail placement..."

report_tns
report_wns
report_power
