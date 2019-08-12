#pragma once 
#include "../../index/cpp/index.h"
#include "server.pb.h"

namespace doc_server
{

typedef doc_server_proto::Request Request;
typedef doc_server_proto::Response Response;
typedef doc_index_proto::Weight Weight;

//上下文,一次请求过程中所依赖的数据以及生成的中间数据
//1.使用Context 统一函数参数,代码看起来更整齐
//2.使用Context 方便后续参数进行扩展
//3.Context 放置数据一定是请求相关的数据
struct Context
{
  const Request* req;
  Response* resp;
  //分词结果
  std::vector<std::string> words;
  //用来保存本次请求中所有触发结果的倒排拉链结构
  std::vector<Weight> all_query_chain;
};

//;该类描述了搜索的详细业务流程
class DocSearcher
{
public:
  //DocSearcher 对外提供的API函数, 完成搜索过程
  bool Search(const Request& req, Response* resp);

private:
  //1.根据查询词进行分词
  //  输入内容:查询词
  //  输出结果:分词结果(vector<string>)
  bool CutQuery(Context* context);

  //2.针对分词结果进行触发,触发出和这些分词结果相关的倒排拉链
  //  输入内容:分词结果
  //  输出内容:若干个倒排拉链(合并成一个大的拉链 vector<Weight>)
  bool Retrieve(Context* context);

  //3.针对触发的文档进行排序
  //  输入内容:触发出的倒排拉链结果
  //  输出内容:倒排拉链结果按照权重进行降序排序
  bool Rank(Context* context);

  //4.把所得到的结果进行包装,构造成Response结构
  //  输入内容:排序好的倒排拉链(doc_id)
  //  输出内容:得到了完整的Rseponse 对象
  bool PackageResponse(Context* context);
  
  //5.记录日志
  bool Log(Context* context);

  
  //一下代码为一些辅助性的函数
  
  //生成描述信息
  //依赖了正文, 还依赖了first_pos(该词在该文档中第一次出现的位置)
  std::string GetDesc(const std::string& content, int first_pos);

  int FindSentenceBeg(const std::string& content, int first_pos);
  void ReplaceEscape(std::string* desc);
};

} //end doc_server
