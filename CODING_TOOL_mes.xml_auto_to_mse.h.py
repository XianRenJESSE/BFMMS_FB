#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
_mes.xml 转换器
将 xxx_mes.xml 文件转换为 xxx_mes.h 文件
"""

import os
import sys
import re
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, List, Optional

def parse_key_value_line(line: str) -> tuple:
    """解析键值对格式的行"""
    line = line.strip()
    if '=' in line:
        key, value = line.split('=', 1)
        return key.strip(), value.strip()
    return None, None

def parse_mes_xml(xml_file: Path) -> Optional[Dict]:
    """
    解析 _mes.xml 文件，提取模式信息和错误代码
    
    Args:
        xml_file: XML文件路径
        
    Returns:
        包含解析数据的字典结构，或None如果解析失败
    """
    try:
        # 由于XML结构特殊，我们先手动解析info部分
        with open(xml_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 初始化数据结构
        parsed_data = {
            'mode_info': {},
            'suggestions': [],
            'warnings': [],
            'errors': []
        }
        
        # 使用正则表达式提取info部分
        info_match = re.search(r'<info>(.*?)</info>', content, re.DOTALL)
        if info_match:
            info_content = info_match.group(1)
            lines = info_content.strip().split('\n')
            for line in lines:
                key, value = parse_key_value_line(line)
                if key and value:
                    parsed_data['mode_info'][key] = value
        
        # 解析sug部分
        sug_match = re.search(r'<sug>(.*?)</sug>', content, re.DOTALL)
        if sug_match:
            sug_content = sug_match.group(1).strip()
            if sug_content != 'null' and sug_content != '':
                suggestions = [s.strip() for s in sug_content.split('\n') if s.strip()]
                parsed_data['suggestions'] = suggestions
        
        # 解析war部分
        war_match = re.search(r'<war>(.*?)</war>', content, re.DOTALL)
        if war_match:
            war_content = war_match.group(1).strip()
            if war_content != 'null' and war_content != '':
                warnings = [w.strip() for w in war_content.split('\n') if w.strip()]
                parsed_data['warnings'] = warnings
        
        # 解析err部分
        err_match = re.search(r'<err>(.*?)</err>', content, re.DOTALL)
        if err_match:
            err_content = err_match.group(1).strip()
            if err_content != 'null' and err_content != '':
                # 按行分割，过滤掉空行和注释
                errors = []
                lines = err_content.split('\n')
                for line in lines:
                    line = line.strip()
                    if line and not line.startswith('<!--'):
                        errors.append(line)
                parsed_data['errors'] = errors
        
        return parsed_data
        
    except Exception as e:
        print(f"解析文件 {xml_file} 时出错: {e}")
        import traceback
        traceback.print_exc()
        return None

def generate_header_content(xml_file: Path, parsed_data: Dict) -> str:
    """
    根据解析的数据生成头文件内容
    
    Args:
        xml_file: 源XML文件路径
        parsed_data: 解析后的数据
        
    Returns:
        生成的C++头文件内容
    """
    # 提取文件名前缀
    file_stem = xml_file.stem
    
    # 获取模式信息 - 必须从XML中读取
    mode_info = parsed_data.get('mode_info', {})
    
    # 必须从XML中读取这三个关键信息
    mode_name = mode_info.get('mode_name', '').strip()
    mode_macro = mode_info.get('mode_macro', '').strip()
    namespace = mode_info.get('namespace', '').strip()
    
    # 验证必要信息是否存在
    missing_fields = []
    if not mode_name:
        missing_fields.append('mode_name')
    if not mode_macro:
        missing_fields.append('mode_macro')
    if not namespace:
        missing_fields.append('namespace')
    
    if missing_fields:
        print(f"  ✗ 错误: XML中缺少以下必要字段: {', '.join(missing_fields)}")
        print(f"     找到的字段: {list(mode_info.keys())}")
        return None
    
    # 获取列表数据
    suggestions = parsed_data.get('suggestions', [])
    warnings = parsed_data.get('warnings', [])
    errors = parsed_data.get('errors', [])
    
    # 构建头文件内容
    content = []
    
    # 添加pragma once和include
    content.append("#pragma once")
    content.append('#include "SIMPLE_TYPE_NAME.h"')
    content.append('#include "INIT.h"')
    content.append("")
    content.append("import MES;")
    content.append("")
    
    # 添加命名空间开始
    content.append(f"namespace {namespace} {{")
    content.append("")
    
    # 生成sug命名空间
    content.append(f"\tnamespace sug{{")
    if suggestions:
        for sug in suggestions:
            content.append(f'\t\tinline mes::a_mes {sug}() {{ return mes::a_mes({mode_macro}, __LINE__, "{sug}"); }};')
    else:
        content.append(f'\t\tinline mes::a_mes null() {{ return mes::a_mes({mode_macro}, __LINE__, "null"); }};')
    content.append("\t};")
    content.append("")
    
    # 生成war命名空间
    content.append(f"\tnamespace war {{")
    if warnings:
        for war in warnings:
            content.append(f'\t\tinline mes::a_mes {war}() {{ return mes::a_mes({mode_macro}, __LINE__, "{war}"); }};')
    else:
        content.append(f'\t\tinline mes::a_mes null() {{ return mes::a_mes({mode_macro}, __LINE__, "null"); }};')
    content.append("\t};")
    content.append("")
    
    # 生成err命名空间
    content.append(f"\tnamespace err {{")
    if errors:
        for err in errors:
            # 移除可能存在的XML注释标记
            err = err.strip()
            if err.startswith('<!--'):
                continue
            content.append(f'\t\tinline mes::a_mes {err}() {{ return mes::a_mes({mode_macro}, __LINE__, "{err}"); }};')
    else:
        content.append(f'\t\tinline mes::a_mes null() {{ return mes::a_mes({mode_macro}, __LINE__, "null"); }};')
    content.append("\t};")
    content.append("")
    
    # 添加命名空间结束
    content.append("};")
    
    return "\n".join(content)

def display_file_content(xml_file: Path):
    """
    显示XML文件的关键信息
    """
    print(f"\n文件内容 - {xml_file.name}:")
    print("=" * 60)
    
    try:
        # 读取文件内容
        with open(xml_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 显示前500个字符
        preview = content[:500] + "..." if len(content) > 500 else content
        print(preview)
        
        # 尝试提取info部分
        print("\n尝试提取info部分:")
        info_match = re.search(r'<info>(.*?)</info>', content, re.DOTALL)
        if info_match:
            info_content = info_match.group(1)
            print("找到info部分:")
            print(info_content)
            
            # 解析键值对
            lines = info_content.strip().split('\n')
            for line in lines:
                line = line.strip()
                if '=' in line:
                    key, value = line.split('=', 1)
                    print(f"  {key.strip()}: {value.strip()}")
        else:
            print("未找到info部分")
            
    except Exception as e:
        print(f"读取文件时出错: {e}")

def convert_mes_files(directory: str = "."):
    """
    转换指定目录下的所有 *_mes.xml 文件
    
    Args:
        directory: 要扫描的目录，默认为当前目录
    """
    dir_path = Path(directory)
    
    if not dir_path.exists():
        print(f"错误: 目录 {directory} 不存在")
        return
    
    # 查找所有 *_mes.xml 文件
    xml_files = list(dir_path.glob("*_mes.xml"))
    
    if not xml_files:
        print(f"在目录 {directory} 中未找到 *_mes.xml 文件")
        return
    
    print(f"找到 {len(xml_files)} 个 *_mes.xml 文件:")
    for xml_file in xml_files:
        print(f"  - {xml_file.name}")
    
    print("\n开始转换...")
    
    successful = 0
    failed = 0
    
    for xml_file in xml_files:
        print(f"\n{'='*60}")
        print(f"处理文件: {xml_file.name}")
        
        # 如果需要调试，显示文件内容
        if os.environ.get('SHOW_BFS_CONTENT'):
            display_file_content(xml_file)
        
        # 解析XML文件
        parsed_data = parse_mes_xml(xml_file)
        
        if parsed_data is None:
            print(f"  ✗ 解析失败")
            failed += 1
            continue
        
        # 显示从XML读取的信息
        mode_info = parsed_data.get('mode_info', {})
        print(f"  从XML读取的信息:")
        for key, value in mode_info.items():
            print(f"    {key}: {value}")
        
        print(f"  建议数量: {len(parsed_data.get('suggestions', []))}")
        print(f"  警告数量: {len(parsed_data.get('warnings', []))}")
        print(f"  错误数量: {len(parsed_data.get('errors', []))}")
        
        # 检查必要的信息
        required_keys = ['mode_name', 'mode_macro', 'namespace']
        missing_keys = [key for key in required_keys if key not in mode_info]
        
        if missing_keys:
            print(f"  ✗ 错误: XML中缺少以下必要字段: {', '.join(missing_keys)}")
            failed += 1
            continue
        
        # 生成头文件内容
        header_content = generate_header_content(xml_file, parsed_data)
        
        if header_content is None:
            print(f"  ✗ 生成头文件内容失败")
            failed += 1
            continue
        
        # 生成头文件名
        header_filename = xml_file.stem + ".h"
        header_path = xml_file.parent / header_filename
        
        try:
            # 写入头文件
            with open(header_path, 'w', encoding='utf-8') as f:
                f.write(header_content)
            
            print(f"  ✓ 生成成功: {header_filename}")
            print(f"  模式宏: {mode_info['mode_macro']}")
            print(f"  命名空间: {mode_info['namespace']}")
            successful += 1
            
        except Exception as e:
            print(f"  ✗ 写入文件失败: {e}")
            failed += 1
    
    print(f"\n{'='*60}")
    print("转换完成!")
    print(f"成功: {successful} 个文件")
    print(f"失败: {failed} 个文件")

def main():
    """
    主函数
    """
    # 检查命令行参数
    if len(sys.argv) > 1:
        directory = sys.argv[1]
    else:
        directory = "."
    
    print("=== _mes.xml 转换器 ===")
    print(f"扫描目录: {directory}")
    print("\n环境变量选项:")
    print("  SHOW_BFS_CONTENT=1  显示XML文件内容（调试用）")
    print("")
    
    # 执行转换
    convert_mes_files(directory)

if __name__ == "__main__":
    main()