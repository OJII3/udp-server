// ros1_udp_server.cpp
#include <ros/ros.h>
#include <std_msgs/String.h>
#include <boost/asio.hpp>
#include <jsoncpp/json/json.h>

using boost::asio::ip::udp;

// UDPサーバーのクラス
class UdpServer
{
public:
  // コンストラクタ
  UdpServer(ros::NodeHandle& nh, const std::string& ip, int port)
    : nh_(nh), socket_(io_service_, udp::endpoint(udp::v4(), port))
  {
    // /chatterトピックに文字列メッセージを送信するパブリッシャーを作成
    pub_ = nh_.advertise<std_msgs::String>("/chatter", 10);
    // /listenerトピックから文字列メッセージを受信するサブスクライバーを作成
    sub_ = nh_.subscribe("/listener", 10, &UdpServer::listenerCallback, this);
    // UDPパケットを受信する
    receivePacket();
  }

  // デストラクタ
  ~UdpServer()
  {
    // ソケットを閉じる
    socket_.close();
  }

  // /listenerトピックからメッセージを受信したときのコールバック関数
  void listenerCallback(const std_msgs::String::ConstPtr& msg)
  {
    // 受信したメッセージをROS_INFOで表示
    ROS_INFO("I heard: [%s]", msg->data.c_str());
    // 受信したメッセージをJSON形式に変換
    Json::Value json;
    json["op"] = "publish";
    json["topic"] = "/listener";
    json["msg"]["data"] = msg->data;
    json["type"] = "std_msgs/String";
    // JSON形式のメッセージを文字列に変換
    Json::FastWriter writer;
    std::string data = writer.write(json);
    // クライアントにUDPパケットを送信
    sendPacket(data);
  }

  // UDPパケットを受信する関数
  void receivePacket()
  {
    // 非同期でUDPパケットを受信する
    socket_.async_receive_from(
      boost::asio::buffer(data_, max_length), sender_endpoint_,
      boost::bind(&UdpServer::handleReceive, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  }

  // UDPパケットを受信したときのハンドラ関数
  void handleReceive(const boost::system::error_code& error,
      std::size_t bytes_recvd)
  {
    if (!error && bytes_recvd > 0)
    {
      // 受信したUDPパケットを文字列に変換
      std::string data(data_, bytes_recvd);
      // 受信したUDPパケットをJSON形式に変換
      Json::Value json;
      Json::Reader reader;
      if (reader.parse(data, json))
      {
        // JSON形式のメッセージに必要な情報が含まれているかチェック
        if (json.isMember("op") && json.isMember("topic") && json.isMember("msg") && json.isMember("type"))
        {
          // JSON形式のメッセージの内容を取得
          std::string op = json["op"].asString();
          std::string topic = json["topic"].asString();
          std::string type = json["type"].asString();
          // opがpublishでtopicが/chatterでtypeがstd_msgs/Stringであれば
          if (op == "publish" && topic == "/chatter" && type == "std_msgs/String")
          {
            // JSON形式のメッセージから文字列メッセージを取得
            std::string msg = json["msg"]["data"].asString();
            // 文字列メッセージをROS_INFOで表示
            // 文字列メッセージをROS_INFOで表示
            ROS_INFO("I received: [%s]", msg.c_str());
            // 文字列メッセージを/chatterトピックに送信
            std_msgs::String ros_msg;
            ros_msg.data = msg;
            pub_.publish(ros_msg);
          }
        }
      }
      // UDPパケットを受信する
      receivePacket();
    }
    else
    {
      // エラーが発生した場合はROS_ERRORで表示
      ROS_ERROR("Error in receiving UDP packet: %s", error.message().c_str());
    }
  }

  // UDPパケットを送信する関数
  void sendPacket(const std::string& data)
  {
    // 非同期でUDPパケットを送信する
    socket_.async_send_to(
      boost::asio::buffer(data, data.size()), sender_endpoint_,
      boost::bind(&UdpServer::handleSend, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  }

  // UDPパケットを送信したときのハンドラ関数
  void handleSend(const boost::system::error_code& error,
      std::size_t bytes_sent)
  {
    if (!error && bytes_sent > 0)
    {
      // 送信したUDPパケットのサイズをROS_INFOで表示
      ROS_INFO("Sent UDP packet: %lu bytes", bytes_sent);
    }
    else
    {
      // エラーが発生した場合はROS_ERRORで表示
      ROS_ERROR("Error in sending UDP packet: %s", error.message().c_str());
    }
  }

private:
  ros::NodeHandle nh_; // ノードハンドル
  ros::Publisher pub_; // パブリッシャー
  ros::Subscriber sub_; // サブスクライバー
  boost::asio::io_service io_service_; // IOサービス
  udp::socket socket_; // UDPソケット
  udp::endpoint sender_endpoint_; // クライアントのエンドポイント
  enum { max_length = 1024 }; // UDPパケットの最大サイズ
  char data_[max_length]; // UDPパケットのデータ
};

// メイン関数
int main(int argc, char **argv)
{
  // ノードの初期化
  ros::init(argc, argv, "ros1_udp_server");
  ros::NodeHandle nh;

  // パラメータの取得
  std::string ip;
  int port;
  nh.param<std::string>("ip", ip, "127.0.0.1");
  nh.param<int>("port", port, 9090);

  // UDPサーバーのインスタンスを作成
  UdpServer server(nh, ip, port);

  // スピン
  ros::spin();

  return 0;
}
