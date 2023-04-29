//
// Created by 69572 on 2023/3/29.
//

#include <unistd.h>
#include "WebServer.h"

int main() {
    /* 守护进程 后台运行 */
    //daemon(1, 0);

    WebServer server(1316, 3, 60000, false, 4);
    server.Start();
}