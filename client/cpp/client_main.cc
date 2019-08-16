//当前先写一个简单的用于辅助测试的客户端
//后面再改进成一个能够搭配HTTP服务器使用的CGI程序
//
//为了让搜索客户端能和HTTP服务器关联到一起,就需要让客户端能够支持CGI协议
#include <base/base.h>
#include <sofa/pbrpc/pbrpc.h>
#include "../../common/util.hpp"
#include "server.pb.h"

DEFINE_string(server_addr, "127.0.0.1:10000", "服务器端的地址");

namespace doc_client
{
typedef doc_server_proto::Request Request;
typedef doc_server_proto::Response Response;


int GetQueryString(char output[])
{
  //1.先从环境变量中获取到方法
  //char* method = getenv("REQUEST_METHOD");
  char* method = getenv("METHOD");

#if 0
  printf("method:%s\n", method);
#endif

  if(method == NULL)
  {
    fprintf(stderr, "REQUEST_METHOD failed\n");
    return -1;
  }
  //2.如果是GET方法,就直接从环境变量中获取到QUERY_STRING
  if(strcasecmp(method, "GET") == 0)
  {
    //char* query_string = getenv("QUERY_STRING");
    char* query_string = getenv("QUERY_STRING");

#if 0
    printf("query_string:%s\n", query_string);
#endif

    if(query_string == NULL)
    {
      fprintf(stderr, "QUERY_STRING failed\n");
        return -1;
    }
    strcpy(output, query_string);
  }
  //3.如果是post方法,先通过环境变量获取到CONTENT_LENGTH,再从标准输入读取body
  else 
  {
    char* content_length_str = getenv("CONTENT_LENGTH");
    if(content_length_str == NULL)
    {
      fprintf(stderr, "CONTENT_LENGTH failed\n");
      return -1;
    }
    int content_length = atoi(content_length_str);
    int i = 0; //表示当前已经往 output中写了多少字符了
    for(; i < content_length; ++i)
    {
      read(0, &output[i], 1);
    }
    output[content_length] = '\0';
  }
  return 0;
}

void PackageRequest(Request* req)
{
  //当前实现的是一个简单的加法服务.
  //后面自改进成一个搜索客户端
  /*
  req->set_a(10);
  req->set_b(20);
  */
  //此处先不考虑sid 的生成方式
  req->set_sid(0);
  req->set_timestamp(common::TimeUtil::TimeStamp());
  char query_string[1024 * 4] = {0};
  GetQueryString(query_string);

#if 0
  printf("query_string :%s\n", query_string);
#endif
  
  //获取到的query_string的内容格式形如:
  //query=filesystem
  char query[1024 * 4] = {0};
  //这是一个不太好的方案, 更严谨的方案是字符串切分
  
  //sscanf(query_string, "query=%s", query);
  sscanf(query_string, "query_string=%s", query);

  req->set_query(query);
  std::cerr << "query_string=" << query_string << std::endl;
  std::cerr << "query=" << query << std::endl;
}

void Search(const Request& req, Response* resp)
{
  //基于RPC方式调用服务器对应的Add函数
  using namespace sofa::pbrpc;
  //1.RPC 概念1 RpcClient
  RpcClient client;
  //2.RPC 概念2 RpcChannel,描述一次链接
  RpcChannel channel(&client, fLS::FLAGS_server_addr);
  //3.RPC 概念3 DocServerAPI_Stub:
  //  描述你这次请求具体调用哪个RPC函数
  doc_server_proto::DocServerAPI_Stub stub(&channel);
  //4.RPC 概念4 RpcController 网络通信细节的管理对象
  //  管理了网络通信中的五元组,以及超时时间等概念
  RpcController ctrl;

  //把第四个参数设为NULL,表示按照同步的方式来进行请求
  /* stub.Add(&ctrl, &req, resp, NULL); */

  stub.Search(&ctrl, &req, resp, NULL);
  if(ctrl.Failed())
    std::cerr << "RPC Search failed\n";
  else 
    std::cerr << "RPC Search ok\n";

  return;
}

void ParseResponse(const Response& resp)
{
  /* std::cout << "sum = " << resp.sum() << std::endl; */
  //std::cout << resp.Utf8DebugString();
  //期望的输出结果是HTML格式的数据
  std::cout << "<html>";
  std::cout << "<body>";
  for (int i = 0; i < resp.item_size(); ++i)
  {
    const auto& item = resp.item(i);
    std::cout << "<div>";
    // 拼装标题和 jump_url 对应的 html
    std::cout << "<div><a href=\"" << item.show_url()
              << "\">" << item.title() << "</a></div>";
    // 拼装描述
    std::cout << "<div>" << item.desc() << "</div>";
    // 拼装 show_url
    std::cout << "<div>" << item.show_url() << "</div>";
    std::cout << "</div>";
  }
  std::cout << "</body>";
  std::cout << "</html>";
}

//此函数负责一次请求的完整过程
void CallServer()
{
  //1.构造请求
  Request req;
  Response resp;
  PackageRequest(&req);
  //2.发送请求给服务器,并获取到响应
  /* Add(req, &resp); */
  Search(req, &resp);
  //3.解析响应并打印结果
  ParseResponse(resp);

  return;
}


} //end doc_client


int main(int argc, char* argv[])
{
  base::InitApp(argc, argv);
  doc_client::CallServer();
  return 0;
}

