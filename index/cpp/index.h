#pragma once
#include <unordered_map>
#include <vector>
#include<base/base.h>
#include <cppjieba/Jieba.hpp>
#include "index.pb.h"
#include "../../common/util.hpp"

//ifndef  不科学，ifndef 后面要跟宏名，如果两个文件名字一样，则宏名有可能一样

namespace doc_index
{
//定义一些类型
typedef doc_index_proto::DocInfo DocInfo;
typedef doc_index_proto::KwdInfo KwdInfo;
typedef doc_index_proto::Weight Weight;
typedef std::vector<DocInfo> ForwardIndex;
typedef std::vector<Weight> InvertedList; //倒排拉链
typedef std::unordered_map<std::string, InvertedList> InvertedIndex;

struct WordCount
{
  int title_count;
  int content_count;
  int first_pos;  //记录了这个词在正文中第一次出现的位置
                  //为了方便后面构造描述信息

  WordCount()
    :title_count(0)
    ,content_count(0)
    ,first_pos(-1)
  {}
};

typedef std::unordered_map<std::string, WordCount> WordCountMap;

//包含了实现索引的数据结构以及索引需要提供的API接口
//
//索引模块核心类，和索引相关的全部操作都包含在这个类中
//  ａ）构建（制作）raw_input　中的内容进行解析在内存中构造出索引结构(hash)
//  ｂ）保存，把内存中的索引结构进行序列化，存到文件中（序列化就依赖了index.proto）制作
//      索引的可执行程序来调用保存
//  ｃ）加载，把磁盘上的索引文件读取出来，反序列化，生产内存中的索引结构　搜索服务器
//  ｄ）反解，内存中的索引结果按照一定的格式打印出来，方便测试
//  ｅ）查正排，给定文档id，获取到文档的详细信息
//  ｆ）查倒排，给定关键词，获取到和关键词相关的文档列表
//

//这个类需要设计成单例模式
class Index
{
public:
  Index();

  static Index* Instance()
  {
    if(_inst == NULL)
      _inst = new Index(); 

    return _inst;
  }
  //1.制作：读取raw_input文件，分析生产内存中的索引结构
  bool Build(const std::string& input_path);
  //2.保存：基于 protobuf 把内存中的索引结构写到文件中
  bool Save(const std::string& out_put);
  //3.加载：把文件中的索引结构加载到内存中
  bool Load(const std::string& index_path);
  //4.反解：内存中的索引结构按照可读性比较好的格式打印到文件中
  bool Dump(const std::string& forward_dump_path, 
            const std::string& inverted_dump_path);
  //5.查正排：给定文档id，获取到文档详细内容(取vector的下标)
  const DocInfo* GetDocInfo(uint64_t doc_id) const;
  //6.查倒排：给定关键词，获取到倒排拉链(查hash)
  const InvertedList* GetInvertedList(const std::string& key) const;

  //此处新增一个API,作为分词时使用(过滤掉暂停词)
  void CutWordWithoutStopWord(const std::string& query, 
                            std::vector<std::string>* words);
  //把比较函数作为public,方便后面 server 模块也去使用
  static bool CmpWeight(const Weight& w1, const Weight& w2);

private:
  ForwardIndex _forward_index;
  InvertedIndex _inverted_index;
  cppjieba::Jieba _jieba;
  common::DictUtil _stop_word_dict;
  
  static Index* _inst;

  //以下函数为内部使用的函数
  const DocInfo* BuildForward(const std::string& line);
  void BuildInverted(const DocInfo& doc_info);
  void SortInverted();
  void SplitTitle(const std::string& title, DocInfo* doc_info);
  void SplitContent(const std::string& content, DocInfo* doc_info);
  int CalcWeight(const WordCount& word_count);
  bool ConvertToProto(std::string* proto_data);
  bool ConvertFromProto(const std::string& proto_data);

};

} //end doc_dex
