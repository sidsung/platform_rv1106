#!/bin/bash

# 检查是否有输入参数
if [ $# -ne 1 ]; then
    echo "Usage: $0 <input-file>"
    exit 1
fi

# 输入文件
input_file=$1

# 检查文件是否存在
if [ ! -f "$input_file" ]; then
    echo "Error: File '$input_file' does not exist."
    exit 1
fi

# 输出文件
output_file="function_config.h"

# 写入头文件开始部分
echo "#ifndef FUNCTION_CONFIG_H" > $output_file
echo "#define FUNCTION_CONFIG_H" >> $output_file
echo "" >> $output_file

# 读取输入文件的每一行并转换为宏定义
while IFS='=' read -r key value; do
    # 跳过注释行
    if [[ $key == \#* ]]; then
        continue
    fi

    # 检查宏值是否为'y'，如果是，则定义宏为1，否则为0
    if [ "$value" == "y" ]; then
        echo "#define $key 1" >>$output_file
    else
        echo "#define $key 0" >>$output_file
    fi
done < "$input_file"

# 写入头文件结束部分
echo "" >> $output_file
echo "#endif /* CONFIG_H */" >> $output_file

echo "Config written to $output_file"
