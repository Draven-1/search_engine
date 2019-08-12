#pragma once
#include<string>
#include<vector>
#include<fstream>
#include<unordered_set>
#include<boost/algorithm/string.hpp>
#include<sys/time.h>


namespace common
{
//这个类包含了所有和字符串相关的操作
class StringUtil
{
public:
  //如何理解压缩关闭:
  //输入字符串 aaa^C^C
  //对于这个字符串,如果我们关闭压缩,切分结果返回三个部分
  //如果打开压缩,切分结果两个部分,把两个相邻的^C合并成了一个
  //
  //vim中输入不可见字符?
  //  插入模式ctrl+v 3+空格       
  static void Split(const std::string& input, std::vector<std::string>* output, 
                                              const std::string& split_char)
  {
    boost::split(*output, input, boost::is_any_of(split_char), 
                                 boost::token_compress_off); 
  }
};

class DictUtil
{
public:
  bool Load(const std::string& path)
  {
    std::ifstream file(path.c_str());
    if(!file.is_open())
      return false;

    std::string line;
    while(std::getline(file, line))
    {
      _set.insert(line);
    }

    file.close();
    return true;
  }

  bool Find(const std::string& key)const
  {
    return _set.find(key) != _set.end();
  }

private:
  std::unordered_set<std::string> _set;
};

class FileUtil
{
public:
  static bool Read(const std::string& input_path, std::string* content)
  {
    std::ifstream file(input_path.c_str());
    if(!file.is_open())
      return false;

    //如何获取到文件的长度
    file.seekg(0, file.end);
    //[注意!!!]使用此方式来读文件,文件最大长度不能超过2G
    int length = file.tellg();
    file.seekg(0, file.beg);
    content->resize(length);
    file.read(const_cast<char*>(content->data()),length);

    file.close();
    return true;
  }

  static bool Write(const std::string& output_path, 
                    const std::string& content)
  {
    std::ofstream file(output_path.c_str());
    if(!file.is_open())
      return false;

    file.write(content.data(), content.size());
    file.close();
    return true;
  }
};

class TimeUtil
{
public:
  //获取到秒级的时间戳
  static int64_t TimeStamp()
  {
    struct ::timeval tv;
    ::gettimeofday(&tv, NULL);    
    return tv.tv_sec;
  }

  //获取到毫秒级的时间戳
  static int64_t TimeStampMS()
  {
    struct ::timeval tv;
    ::gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
  }

  //获取到微妙级的时间戳
  static int64_t TimeStampUS()
  {
    struct ::timeval tv;
    ::gettimeofday(&tv, NULL);
    //return tv.tv_sec * 1000000 + tv.tv_usec;
    //return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    return tv.tv_sec * 1e6 + tv.tv_usec;
  }
};

} //end common
