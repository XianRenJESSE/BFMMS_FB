import os
import shutil

def convert_ixx_to_ixxcpp():
    """
    将当前目录下所有.ixx文件另存为.ixx.cpp文件，强制覆盖已存在的文件
    """
    current_dir = os.getcwd()
    
    for filename in os.listdir(current_dir):
        if filename.endswith('.ixx'):
            source = os.path.join(current_dir, filename)
            target = os.path.join(current_dir, filename + '.cpp')
            
            # 复制并覆盖已存在的文件
            shutil.copy2(source, target)
            print(f"已复制: {filename} -> {filename}.cpp")

if __name__ == "__main__":
    convert_ixx_to_ixxcpp()