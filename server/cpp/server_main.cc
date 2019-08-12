//定义一个类继承自DocServerAPI,去重写Add函数
//就相当于对一次请求的处理过程.

#include <sofa/pbrpc/pbrpc.h>
#include <base/base.h>
#include "../../common/util.hpp"
#include "server.pb.h"
#include "doc_searcher.h"

DEFINE_string(port, "10000", "服务器端口号");
DEFINE_string(index_path, "../index/index_file", "索引所在的目录");

namespace doc_server
{
typedef doc_server_proto::Request Request;
typedef doc_server_proto::Response Response;

//典型的服务器程序:
//1.读取请求解析(反序列化)
//2.根据Request,计算生成Response
//3.把Response 写回到客户端(序列化)
class DocServerAPIImpl : public doc_server_proto::DocServerAPI
{
public:
  /*
  原型:
  virtual void Add(::google::protobuf::RpcController* controller,
                       const ::doc_server_proto::Request* request,
                       ::doc_server_proto::Response* response,
                       ::google::protobuf::Closure* done);
  */

  //描述了这次请求如何处理
  void Search(::google::protobuf::RpcController* controller,
                       const Request* request,
                       Response* response,
                       ::google::protobuf::Closure* done)
  {
    (void)controller;
    //controller:网络通信中的细节控制
    //request, response 
    //done:如果这个请求处理完毕了.就调用done,来通知RPC框架这次请求处理结束
    
    //此过程就死根据请求计算响应的过程(和业务相关)
    //当前的业务很简单,就是直接计算相加
    //因此下面的实现逻辑也很简单
    /*
    response->set_sum(request->a() + request->b());
    */

    response->set_sid(request->sid());
    response->set_timestamp(common::TimeUtil::TimeStamp());
    //std::cout << "hello\n";
    //和业务相关
    DocSearcher searcher;
    searcher.Search(*request, response);

    //调用闭包
    done->Run();
  }
  
};

} //end doc_server

//服务器初始化
int main(int argc, char* argv[])
{
  base::InitApp(argc, argv);
  using namespace sofa::pbrpc;
  //0.加上索引初始化和加载的过程
  doc_index::Index* index = doc_index::Index::Instance();
  CHECK(index->Load(fLS::FLAGS_index_path));
  LOG(INFO) << "Index Load Done";
  std::cout << "Index Load Done" << std::endl;

  //1.创建 RpcServer 对象
  RpcServerOptions options;
  options.work_thread_num = 4;
  RpcServer server(options);

  //2.启动 RpcServer
  CHECK(server.Start("0.0.0.0:" + fLS::FLAGS_port));

  //3.注册请求处理方式
  doc_server::DocServerAPIImpl* server_impl
          = new doc_server::DocServerAPIImpl();
  server.RegisterService(server_impl);

  //4.进入循环等待服务器结束信号
  server.Run();

  return 0;
}
