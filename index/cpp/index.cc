#include<fstream>
#include"index.h"

DEFINE_string(dict_path, 
    "../../third_part/data/jieba_dict/jieba.dict.utf8",
    "字典路径");
DEFINE_string(hmm_path, 
    "../../third_part/data/jieba_dict/hmm_model.utf8",
    "hmm字典路径");
DEFINE_string(user_dict_path, 
    "../../third_part/data/jieba_dict/user.dict.utf8",
    "用户自定制字典路径");
DEFINE_string(idf_path, 
    "../../third_part/data/jieba_dict/idf.utf8",
    "idf字典路径");
DEFINE_string(stop_word_path, 
    "../../third_part/data/jieba_dict/stop_words.utf8",
    "暂停词字典路径");

namespace doc_index
{
Index* Index::_inst = NULL;

Index::Index()  
  : _jieba(FLAGS_dict_path,
           fLS::FLAGS_hmm_path,
           fLS::FLAGS_user_dict_path,
           fLS::FLAGS_idf_path,
           fLS::FLAGS_stop_word_path)
  {
    CHECK(_stop_word_dict.Load(fLS::FLAGS_stop_word_path));
  }


//1.制作：读取raw_input文件，分析生产内存中的索引结构
//从raw_input文件中读取数据，在内存中构建成索引结构
bool Index::Build(const std::string& input_path)
{
  LOG(INFO) << "Index Build";
  //1.打开文件并且按行读取文件,针对每行进行解析.打开的文件其实是
  //raw_input文件,这个文件是预处理之后得到的行文本文件,每行对应一个html.
  //每行又是一个三元组(jump_url, title, content)
  std::ifstream file(input_path.c_str());
  CHECK(file.is_open());
  std::string line;
  while(std::getline(file, line))
  {
    //2.把这一行数据制作成一个DocInfo
    // 此处获取到的doc_info是为了接下来制作倒排方便
    const DocInfo* doc_info = BuildForward(line);
    //如果构建正排失败，及立刻让进程终止
    CHECK(file.is_open()) << "input_path: " << input_path;
    //3.更新倒排信息
    //此函数的输出结构,直接放到Index::_inverted_index
    BuildInverted(*doc_info);
  }
  //4.处理完所有的文档之后,针对所有的倒排拉链进行排序
  //key-value 中的value进行排序,排序的依据按照权重降序排序
  SortInverted();
  file.close();
  LOG(INFO) << "Index Buidld Done!!!";
  return true;
}

const DocInfo* Index::BuildForward(const std::string& line)
{
  //1.先对line进行字符串切分
  std::vector<std::string> tokens;
  //当前的Split不会破坏原字符串
  common::StringUtil::Split(line,&tokens, "\3");
  if(tokens.size() != 3)
  {
    LOG(FATAL) << "line split not 3 tokens! tokens.size()=" << tokens.size();
    return NULL;
  }
  //2.构造一个DocInfo结构,把切分的结果赋值到DocInfo
  //  除了分词结果之外都能进行赋值
  DocInfo doc_info;
  doc_info.set_doc_id(_forward_index.size());
  doc_info.set_title(tokens[1]);
  doc_info.set_content(tokens[2]);
  doc_info.set_jump_url(tokens[0]);
  //此处把show_url直接填成和jump_url一致,实际上真实的搜索引擎通常是show_url
  //只包含jump_url的域名,但是此处我们先不这样处理
  doc_info.set_show_url(tokens[0]);
  //3.对标题和正文进行分词,把分词结果保存到DocInfo中
  //此处doc_info 是输出参数,用指针的方式传进去
  SplitTitle(tokens[1], &doc_info);
  SplitContent(tokens[2], &doc_info);
  //4.把这个DocInfo插入到正排索引中
  _forward_index.push_back(doc_info);
  return &_forward_index.back();

} 


void Index::SplitTitle(const std::string& title, DocInfo* doc_info)
{
  std::vector<cppjieba::Word> words;
  //1.要调用cppjieba 进行分词,需要先创建一个jieba 对象
  //  然后用接口完成对标题的分词(offset 风格的接口)
  _jieba.CutForSearch(title, words);
  //2.把jieba分词结果转成我们需要的前闭后开区间的风格
  //  words里面包含的分词结果,每个结果包含一个offset
  //  offset表示的是当前词在文档中的起始位置的下标
  //  而实际上我们需要知道的是一个前闭后开区间
  for(size_t i = 0; i < words.size(); ++i)
  {
    auto* token = doc_info->add_title_token();
    token->set_beg(words[i].offset);
    if(i + 1 < words.size())
    {
      //i不是最后一个元素
      token->set_end(words[i+1].offset);
    }
    else
    {
      //i是最后一个元素
      token->set_end(title.size());
    }
  } //end for
  return;
}
void Index::SplitContent(const std::string& content, DocInfo* doc_info)
{
  //要调用cppjieba 进行分词,需要先创建一个jieba 对象
  std::vector<cppjieba::Word> words;
  _jieba.CutForSearch(content, words); 
  //words里面包含的分词结果,每个结果包含一个offset
  //offset表示的是当前词在文档中的起始位置的下标
  //而实际上我们需要知道的是一个前闭后开区间
  if(words.size() <= 1)
  {
    //错误处理
    LOG(FATAL) << "SplitContent failed!";
    return;
  }
  for(size_t i = 0; i < words.size(); ++i)
  {
    auto* token = doc_info->add_content_token();
    token->set_beg(words[i].offset);
    if(i + 1 < words.size())
    {
      //i不是最后一个元素
      token->set_end(words[i+1].offset);
    }
    else
    {
      //i是最后一个元素
      token->set_end(content.size());
    }
  } //end for
  return;
}

void Index::BuildInverted(const DocInfo& doc_info)
{
  //0.构造倒排索引的核心流程:
  //  先统计好doc_info中的每个词在标题和正文中出现的次数
  //  此时得到一个hash表,key是当前的关键词,value就是一个结构体,
  //  该结构体中包含着该词在标题和正文中出现的次数
  WordCountMap word_count_map;
  //1.统计title中每个词出现的个数
  for(int i = 0; i < doc_info.title_token_size(); ++i)
  {
    const auto& token = doc_info.title_token(i);
    std::string word = doc_info.title().substr(token.beg(), 
                                 token.end() - token.beg());
    //假设文档中,Hello, hello 这两种情况算作一个词,大小写不敏感
    boost::to_lower(word);
    //暂停词:有些词出现频率极高,但是有没有实际意义,
    //因此不把这些词做到倒排索引中
    if(_stop_word_dict.Find(word))
    {
      continue;
    }
    ++word_count_map[word].title_count;
    //记录下这个词在正文中第一次出现的位置
    if(word_count_map[word].content_count == 1)
    {
      word_count_map[word].first_pos = token.beg();
    }
  }
  //2.统计正文中每个词出现的个数
  //  此时得到了一个hash表,hash表中的key就是关键词(切分结果)
  //  hash表中的value就是一个结构体,结构题里面包含了该词在标题中出现的次数
  //  和该词在正文中出现的次数
  for(int i = 0; i < doc_info.content_token_size(); ++i)
  {
    const auto& token = doc_info.content_token(i);
    std::string word = doc_info.content().substr(token.beg(), 
                                token.end() - token.beg());
    //忽略大小写
    boost::to_lower(word);
    //过滤掉暂停词
    if(_stop_word_dict.Find(word))
    {
      continue;
    }
    ++word_count_map[word].content_count;
    //更新当前词在正文中第一次出现的位置
    if(word_count_map[word].content_count == 1)
    {
      word_count_map[word].first_pos = token.beg();
    }
  }
  //3.根据个数的统计结果,更新到倒排索引中
  //  遍历刚才的这个hash表,拿着key去倒排索引中去查
  //  如果倒排索引中不存在这个词,就新增一项,
  //  如果倒排索引中已经存在这个词,就根据当前构造好的Weight结果添加到倒排索引
  //  中对应的倒排拉链中(vector<Weight>)
  for(const auto& word_pair : word_count_map)  //c++11中的for循环
  {
    Weight weight;
    weight.set_doc_id(doc_info.doc_id());
    weight.set_weight(CalcWeight(word_pair.second));
    weight.set_title_count(word_pair.second.title_count);
    weight.set_content_count(word_pair.second.content_count);
    weight.set_first_pos(word_pair.second.first_pos);
    //先获取当前词对应的倒排拉链
    InvertedList& inverted_list = _inverted_index[word_pair.first];
    //把Weight对象插入到获取的倒排拉链中
    inverted_list.push_back(weight);
  }
  return;
}

int Index::CalcWeight(const WordCount& word_count)
{
  //权重我们使用一种简单粗暴的方式来进行计算
  return 10 * word_count.title_count + word_count.content_count;
}

void Index::SortInverted()
{
  for(auto& inverted_pair: _inverted_index)
  {
    InvertedList& inverted_list = inverted_pair.second;
    std::sort(inverted_list.begin(), inverted_list.end(), CmpWeight);
  }
}

bool Index::CmpWeight(const Weight& w1, const Weight& w2)
{
  //为了完成降序排序
  return w1.weight() > w2.weight();
}

//2.保存：基于 protobuf 把内存中的索引结构写到文件中
//把内存中的索引数据保存到磁盘上
bool Index::Save(const std::string& output_path)
{
  //1.把内存中的结构构造成对应的protobuf结构
  std::string proto_data;
  CHECK(ConvertToProto(&proto_data));
  //2.基于protobuf结构进行排序,序列化的结构写到磁盘上
  CHECK(common::FileUtil::Write(output_path, proto_data));
  return true;
}

bool Index::ConvertToProto(std::string* proto_data)
{
  doc_index_proto::Index index;
  //1.现将正排构造到protobuf
  for(const auto& doc_info : _forward_index)
  {
    auto* proto_doc_info = index.add_forward_index();
    *proto_doc_info = doc_info;
  }
  //2.再将倒排构造到protobuf
  for(const auto& inverted_pair : _inverted_index)
  {
     //inverted_pair.first 关键词
     //inverted_pair.second 倒排拉链(weight数组) 
     auto* kwd_info = index.add_inverted_index();
     kwd_info->set_key(inverted_pair.first);
     for(const auto&weight : inverted_pair.second)
     {
       auto* proto_weight = kwd_info->add_weight();
       *proto_weight = weight;
     }
  }
  //3.使用protobuf对象进行序列化
  index.SerializeToString(proto_data);

  return true;
}


//3.加载：把文件中的索引结构加载到内存中
bool Index::Load(const std::string& index_path)
{
  //1.读取文件内容,基于protobuf进行反序列化
  std::string proto_data;
  CHECK(common::FileUtil::Read(index_path, &proto_data));
  //2.把protobuf结构转换回内存结构
  CHECK(ConvertFromProto(proto_data));

  return true;
}

bool Index::ConvertFromProto(const std::string& proto_data)
{
  doc_index_proto::Index index;
  index.ParseFromString(proto_data);
  //1.把正排转换到内存中
  for(int i = 0; i < index.forward_index_size(); ++i)
  {
    const auto& doc_info = index.forward_index(i);
    _forward_index.push_back(doc_info);
  }
  //2.把倒排转换到内存中
  for(int i = 0; i < index.inverted_index_size(); ++i)
  {
    const auto& kwd_info = index.inverted_index(i);
    InvertedList& inverted_list = _inverted_index[kwd_info.key()];
    for(int j = 0; j < kwd_info.weight_size(); ++j)
    {
      const auto& weight = kwd_info.weight(j);
      inverted_list.push_back(weight);
    }
  }
  return true;
}

//4.反解：内存中的索引结构按照可读性比较好的格式打印到文件中
// 这个是用于辅助测试的接口
// 把内存中的索引结构遍历出来,按照比较方便肉眼查看的格式
// 打印到文件中
bool Index::Dump(const std::string& forward_dump_path, 
          const std::string& inverted_dump_path)
{
  //1.处理正排
  std::ofstream forward_dump_file(forward_dump_path.c_str());
  CHECK(forward_dump_file.is_open());
  for(const auto& doc_info : _forward_index)
  {
    //Utf8DebugString protobuf 提供的接口, 
    //构造出一个格式化的字符串,方便肉眼观察
    forward_dump_file << doc_info.Utf8DebugString() << "\n================\n";
  }
  //2.处理倒排
  std::ofstream inverted_dump_file(inverted_dump_path.c_str());
  CHECK(inverted_dump_file.is_open());
  for(const auto& inverted_pair : _inverted_index)
  {
    inverted_dump_file << inverted_pair.first << "\n";
    for(const auto& weight : inverted_pair.second)
    {
      inverted_dump_file << weight.Utf8DebugString();
    }
    inverted_dump_file << "\n================\n";
  }

  inverted_dump_file.close(); 
  return true;
}

//5.查正排：给定文档id，获取到文档详细内容(取vector的下标)
const DocInfo* Index::GetDocInfo(uint64_t doc_id) const
{
  if(doc_id >= _forward_index.size())
  {
    return NULL;
  }
  return &_forward_index[doc_id];
}

//6.查倒排：给定关键词，获取到倒排拉链(查hash)
const InvertedList* Index::GetInvertedList(const std::string& key) const
{
  auto it = _inverted_index.find(key);
  if(it == _inverted_index.end())
  {
    return NULL;
  }
  return &(it->second);
}

void Index::CutWordWithoutStopWord(const std::string& query, 
                            std::vector<std::string>* words)
{
  //先清空掉输出参数中原有的内容
  words->clear();
  //调用jieba分词对应的函数来分词就行了
  std::vector<std::string> tmp;
  _jieba.CutForSearch(query, tmp);
  //接下来需要把暂停词表中的词干掉
  //遍历tmp,如果其中分词结果不在暂停词表中,才加入到最终结果
  for(std::string word : tmp) 
  {
    //由于搜索结果不区分大小写
    boost::to_lower(word);
    if(_stop_word_dict.Find(word))
    {
      //该分词结果是暂停词,就不能放到最终结果中
      continue;
    }
    words->push_back(word);
  }
  return;
}

} //doc_index
