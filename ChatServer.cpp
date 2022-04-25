#include <iostream>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <map>
 
using namespace std;
using json = nlohmann::json;
 
const string PUBLIC_ALL = "public_all";
 
const string COMMAND = "command";
 
const string PRIVATE_MSG = "private_msg";
const string MESSAGE = "message";
const string USER_ID = "user_id";
const string USER_ID_FROM = "user_id_from";
 
const string PUBLIC_MSG = "public_msg";
 
const string SET_NAME = "set_name";
const string NAME = "name";
 
const string STATUS = "status";
const string ONLINE = "online";
 
struct UserData 
{
    int user_id;
    string name;
};
 
map<int, UserData*> all_users;
 
typedef uWS::WebSocket<false, true, UserData> UWEBSOCK;
 
string status(UserData* data, bool online) 
{
    json response;
    response[COMMAND] = STATUS;
    response[NAME] = data->name;
    response[USER_ID] = data->user_id;
    response[ONLINE] = online;
    return response.dump();
}
 
void processPrivateMsg(UWEBSOCK* ws, json parsed, int user_id) 
{
    int user_id_to = parsed[USER_ID];
    string user_msg = parsed[MESSAGE];
 
    json response;
    response[COMMAND] = PRIVATE_MSG;
    response[USER_ID_FROM] = user_id;
    response[MESSAGE] = user_msg;
 
    ws->publish("userN" + to_string(user_id_to), response.dump());
}
void processPublicMsg(UWEBSOCK* ws, json parsed, int user_id) 
{
    string user_msg = parsed[MESSAGE];
 
    json response;
    response[COMMAND] = PUBLIC_MSG;
    response[USER_ID_FROM] = user_id;
    response[MESSAGE] = user_msg;
 
    ws->publish(PUBLIC_ALL, response.dump());
}
void processSetName(UWEBSOCK* ws, json parsed, UserData* data) 
{
    data->name = parsed[NAME];
}
void processMessage(UWEBSOCK *ws, string_view message) 
{
    UserData* data = ws->getUserData();
 
    cout << "Message from user ID: " << data->user_id << " message: " << message << endl;
    auto parsed = json::parse(message);
 
    if (parsed[COMMAND] == PRIVATE_MSG) {
        processPrivateMsg(ws, parsed, data->user_id);
    }
   
    if (parsed[COMMAND] == PUBLIC_MSG) {
        processPublicMsg(ws, parsed, data->user_id);
    }
 
    if (parsed[COMMAND] == SET_NAME) {
        processSetName(ws, parsed, data);
        ws->publish(PUBLIC_ALL, status(data, true));
    }
}
 
int main()
{
    int latest_id = 10;
 
    uWS::App().ws<UserData>("/*", 
        {
        .idleTimeout = 1000,
        .open = [&latest_id](auto *ws) 
        {
            UserData *data = ws->getUserData();
            data->user_id = latest_id++;
            data->name = "UNNAMED";
 
            cout << "New user connected ID: " << data->user_id << endl;
            ws->subscribe("userN" + to_string(data->user_id));
 
            ws->publish(PUBLIC_ALL, status(data, true));
 
            ws->subscribe(PUBLIC_ALL);
 
            for (auto entry : all_users) 
            {
                ws->send(status(entry.second, true), uWS::OpCode::TEXT);
            }
 
            all_users[data->user_id] = data;
            
        },
        .message = [](auto* ws, string_view message, uWS::OpCode) {
            processMessage(ws, message);
        },

        .close = [](auto *ws, int code, string_view message) {
            UserData* data = ws->getUserData();
            ws->publish(PUBLIC_ALL, status(data, false));
            cout << "User disconnected ID: " << data->user_id << endl;
            all_users.erase(data->user_id); 
        },
        
        }).listen(9001, [](auto*) {
            cout << "Server Started successfully" << endl;
        }).run();
}