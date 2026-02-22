#include "villagereditor.h"
#include <QApplication>
#include <QIcon>   // 可能需要包含

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序图标（将影响所有未单独设置图标的窗口）
    a.setWindowIcon(QIcon(":/icons/app.ico"));   // 如果使用资源文件（见步骤3）
    // 或者使用相对路径（不推荐，但可以临时测试）：a.setWindowIcon(QIcon("resources/app.ico"));

    VillagerEditor w;
    w.show();
    return a.exec();
}
