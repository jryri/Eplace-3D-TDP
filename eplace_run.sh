# ./ePlace -aux ./testcase/aes_cipher_top/aes_cipher_top.aux  -fullPlot -targetDensity 0.6  -targetOverflow 0.1  -noQP -internalLegal  -internalDP  -TDP -bb -outputPath ./eplace_solution
#
# ./ePlace -aux ./testcase/aes_cipher_top/aes_cipher_top.aux  -fullPlot -targetDensity 0.6  -targetOverflow 0.1  -noQP -internalLegal  -internalDP  -TDP -bktrk -outputPath ./eplace_solution
./ePlace -aux ./after_buffered/aes_cipher_top_buf.aux  -fullPlot -targetDensity 0.6  -targetOverflow 0.2  -noQP -internalLegal  -internalDP  -TDP -bb -outputPath ./eplace_solution

rm sta_iter*
rm *.rpt
rm eplace_sta.tcl




