#include <unistd.h>
#include "server/webserver.h"

int main()
{
    WebServer server(
        // �˿ڡ�ETģʽ��timeoutMs�������˳�?
        1316, 3, 60000, false,
        // MySql����
        3306, "root", "root", "webserver",
        // ���ӳ��������̳߳���������־���ء���־�ȼ�����־�첽��������
        12, 6, true, 1, 1024);
    server.Start();
}