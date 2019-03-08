#!/bin/bash

# Output Files
OUT_BIN=drplib-config.bin
OUT_HEADER=drplib-config-api.h

# Address in QSPI Flash (last 4MB of 64MB QSPI Flash)
DRP_FLASH_ADDR=0x23C00000

####################
# Choose the DRP 'configuration data' files you want
# by uncommented the lines below
####################

#CONFIG1=drp_lib/r_drp_argb2grayscale/r_drp_argb2grayscale.dat
CONFIG2=drp_lib/r_drp_bayer2grayscale/r_drp_bayer2grayscale.dat
#CONFIG3=drp_lib/r_drp_binarization_adaptive_bit/r_drp_binarization_adaptive_bit.dat
#CONFIG4=drp_lib/r_drp_binarization_adaptive/r_drp_binarization_adaptive.dat
CONFIG5=drp_lib/r_drp_binarization_fixed/r_drp_binarization_fixed.dat
CONFIG6=drp_lib/r_drp_canny_calculate/r_drp_canny_calculate.dat
CONFIG7=drp_lib/r_drp_canny_hysterisis/r_drp_canny_hysterisis.dat
#CONFIG8=drp_lib/r_drp_circle_fitting/r_drp_circle_fitting.dat
CONFIG9=drp_lib/r_drp_corner_harris/r_drp_corner_harris.dat
#CONFIG10=drp_lib/r_drp_cropping/r_drp_cropping.dat
CONFIG11=drp_lib/r_drp_dilate/r_drp_dilate.dat
CONFIG12=drp_lib/r_drp_erode/r_drp_erode.dat
CONFIG13=drp_lib/r_drp_gamma_correction/r_drp_gamma_correction.dat
#CONFIG14=drp_lib/r_drp_gaussian_blur/r_drp_gaussian_blur.dat
#CONFIG15=drp_lib/r_drp_histogram/r_drp_histogram.dat
CONFIG16=drp_lib/r_drp_median_blur/r_drp_median_blur.dat
#CONFIG17=drp_lib/r_drp_prewitt/r_drp_prewitt.dat
#CONFIG18=drp_lib/r_drp_reed_solomon/r_drp_reed_solomon.dat
#CONFIG19=drp_lib/r_drp_resize_bilinear_fixed/r_drp_resize_bilinear_fixed.dat
#CONFIG20=drp_lib/r_drp_resize_bilinear/r_drp_resize_bilinear.dat
#CONFIG21=drp_lib/r_drp_resize_nearest/r_drp_resize_nearest.dat
#CONFIG22=drp_lib/r_drp_simple_isp/r_drp_simple_isp_bayer2grayscale_3.dat
#CONFIG23=drp_lib/r_drp_simple_isp/r_drp_simple_isp_bayer2grayscale_6.dat
#CONFIG24=drp_lib/r_drp_simple_isp/r_drp_simple_isp_bayer2yuv_3.dat
#CONFIG25=drp_lib/r_drp_simple_isp/r_drp_simple_isp_bayer2yuv_6.dat
CONFIG26=drp_lib/r_drp_sobel/r_drp_sobel.dat
#CONFIG27=drp_lib/r_drp_unsharp_masking/r_drp_unsharp_masking.dat




######################################################
# Do not edit below this point
######################################################

CONFIG1_HEADER=drp_lib/r_drp_argb2grayscale/r_drp_argb2grayscale.h
CONFIG1_SIZE=14368

CONFIG2_HEADER=drp_lib/r_drp_bayer2grayscale/r_drp_bayer2grayscale.h
CONFIG2_SIZE=62912

CONFIG3_HEADER=drp_lib/r_drp_binarization_adaptive_bit/r_drp_binarization_adaptive_bit.h
CONFIG3_SIZE=155968

CONFIG4_HEADER=drp_lib/r_drp_binarization_adaptive/r_drp_binarization_adaptive.h
CONFIG4_SIZE=153568

CONFIG5_HEADER=drp_lib/r_drp_binarization_fixed/r_drp_binarization_fixed.h
CONFIG5_SIZE=16960

CONFIG6_HEADER=drp_lib/r_drp_canny_calculate/r_drp_canny_calculate.h
CONFIG6_SIZE=126080

CONFIG7_HEADER=drp_lib/r_drp_canny_hysterisis/r_drp_canny_hysterisis.h
CONFIG7_SIZE=358752

CONFIG8_HEADER=drp_lib/r_drp_circle_fitting/r_drp_circle_fitting.h
CONFIG8_SIZE=160160

CONFIG9_HEADER=drp_lib/r_drp_corner_harris/r_drp_corner_harris.h
CONFIG9_SIZE=353088

CONFIG10_HEADER=drp_lib/r_drp_cropping/r_drp_cropping.h
CONFIG10_SIZE=14688

CONFIG11_HEADER=drp_lib/r_drp_dilate/r_drp_dilate.h
CONFIG11_SIZE=56800

CONFIG12_HEADER=drp_lib/r_drp_erode/r_drp_erode.h
CONFIG12_SIZE=60480

CONFIG13_HEADER=drp_lib/r_drp_gamma_correction/r_drp_gamma_correction.h
CONFIG13_SIZE=13120

CONFIG14_HEADER=drp_lib/r_drp_gaussian_blur/r_drp_gaussian_blur.h
CONFIG14_SIZE=60992

CONFIG15_HEADER=drp_lib/r_drp_histogram/r_drp_histogram.h
CONFIG15_SIZE=79072

CONFIG16_HEADER=drp_lib/r_drp_median_blur/r_drp_median_blur.h
CONFIG16_SIZE=57536

CONFIG17_HEADER=drp_lib/r_drp_prewitt/r_drp_prewitt.h
CONFIG17_SIZE=40256

CONFIG18_HEADER=drp_lib/r_drp_reed_solomon/r_drp_reed_solomon.h
CONFIG18_SIZE=118848

CONFIG19_HEADER=drp_lib/r_drp_resize_bilinear_fixed/r_drp_resize_bilinear_fixed.h
CONFIG19_SIZE=138240

CONFIG20_HEADER=drp_lib/r_drp_resize_bilinear/r_drp_resize_bilinear.h
CONFIG20_SIZE=379744

CONFIG21_HEADER=drp_lib/r_drp_resize_nearest/r_drp_resize_nearest.h
CONFIG21_SIZE=303456

CONFIG22_HEADER=drp_lib/r_drp_simple_isp/r_drp_simple_isp.h
CONFIG22_SIZE=200896

CONFIG23_HEADER=drp_lib/r_drp_simple_isp/r_drp_simple_isp.h
CONFIG23_SIZE=361152

CONFIG24_HEADER=drp_lib/r_drp_simple_isp/r_drp_simple_isp.h
CONFIG24_SIZE=230624

CONFIG25_HEADER=drp_lib/r_drp_simple_isp/r_drp_simple_isp.h
CONFIG25_SIZE=403488

CONFIG26_HEADER=drp_lib/r_drp_sobel/r_drp_sobel.h
CONFIG26_SIZE=40352

CONFIG27_HEADER=drp_lib/r_drp_unsharp_masking/r_drp_unsharp_masking.h
CONFIG27_SIZE=156512


if [ -e $OUT_BIN ] ; then rm $OUT_BIN ; fi
if [ -e $OUT_HEADER ] ; then rm $OUT_HEADER ; fi

flash_index=0

TMP1=/tmp/drp1.txt
echo "" > $TMP1


# define flash index
echo -e "\n#define DRP_FLASH_ADDR ${DRP_FLASH_ADDR}\n\n" >> $OUT_HEADER

for i in $(seq 1 27)
do
	config=$(eval echo CONFIG${i})
	config_header=$(eval echo CONFIG${i}_HEADER)
	config_size=$(eval echo CONFIG${i}_SIZE)
	if [ "${!config}" != "" ] ; then
		echo "Processing $config ( ${!config} )"

		cat ${!config} >> $OUT_BIN
		cat ${!config_header} >> $TMP1

		filename=$(basename ${!config})
		echo "/* $filename */" >> $OUT_HEADER

		# remove .dat and make uppercase
		name=$(echo ${filename::-4} | awk '{print toupper($0)}')

		# define flash index
		echo "#define ${name}_INDEX ${flash_index}" >> $OUT_HEADER

		# define binary size
		echo -e "#define ${name}_SIZE ${!config_size}\n" >> $OUT_HEADER

		# increment our flash index
		let "flash_index += ${!config_size}"
	fi
done

# Remove license text (we don' need it that many times)
sed -i -e '/DISCLAIMER/,/reserved/d' $TMP1
cat $TMP1 >> $OUT_HEADER

#find drp_lib -name "*.dat" | sort
#find drp_lib -name "*.h" | sort
#ls -l $(find drp_lib -name "*.dat" | sort)

#
#rm $OUT_BIN
#for line in `cat /tmp/drp_files.txt`
#do
#	cat $line >> $OUT_BIN
#done


