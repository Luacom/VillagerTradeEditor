#ifndef VILLAGEREDITOR_H
#define VILLAGEREDITOR_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QJsonObject>
#include <QJsonArray>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>

// ==================== 数据模型 ====================
struct ItemData {
    QString name = "minecraft:air";
    int count = 1;
    int damage = 0;

    // Tag 字段
    bool enableName = false;
    QString displayName = "自定义名称";
    bool enableLore = false;
    QString lore = "自定义注释";
    bool enableEnch = false;
    int enchId = 9;
    int enchLevel = 5;

    // 新增：自定义 NBT 节点
    bool enableCustom = false;
    QJsonArray customNodes;   // 存储自定义节点数组，每个元素是完整的 {name,value,type}
};

struct TradeOption {
    ItemData buyA;
    ItemData buyB;
    ItemData sell;
    int uses = 0;
    int maxUses = 12;
    int tier = 0;
};

struct ItemMapping {
    QString englishId;
    QString chineseName;
    int defaultDamage;
    QString category;  // <== 新增分类字段
    QString presetJson;  // 新增
};

// ==================== UI 控件组映射 ====================
struct ItemWidgets {
    QLineEdit *leName;
    QPushButton *btnSelect;
    QSpinBox *sbCount;
    QSpinBox *sbDamage;

    QCheckBox *cbEnableName;
    QTextEdit *leDisp;      // 原 QLineEdit*

    QCheckBox *cbEnableLore;
    QTextEdit *leLore;      // 原 QLineEdit*

    QCheckBox *cbEnableEnch;
    QSpinBox *sbEnchId;
    QSpinBox *sbEnchLvl;

    // 新增：自定义 NBT 节点
    QCheckBox *cbEnableCustom;
    QTextEdit *teCustom;   // 用于输入 JSON 数组
};

class VillagerEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit VillagerEditor(QWidget *parent = nullptr);
    ~VillagerEditor();

private slots:
    void openItemConfigEditor();
    void loadFile();
    void saveFile();
    void onTableItemSelected(int row, int column);
    void addTradeOption();
    void deleteTradeOption();

    // 统一的数据同步与 UI 联动槽函数
    void onDataChanged();
    void onTagCheckboxToggled();
    void openItemSelector(ItemWidgets *widgets);

private:
    bool validateCustomNodes() const;  // 新增：验证所有自定义节点是否有效
    void initUI();
    QGroupBox* createItemSection(const QString &title, ItemWidgets &widgets);
    void updateTradeTable();

    // 数据同步核心
    void populateUIFromData(const TradeOption &trade);
    void syncDataFromUI();

    // NBT 解析
    QList<TradeOption> parseNbtData(const QString &nbtText);
    ItemData parseItemData(const QJsonArray &itemArr);

    // NBT 构建核心
    QJsonObject createNode(const QString &name, const QJsonValue &value, int type);
    QJsonObject buildItemNbt(const QString &key, const ItemData &data);
    QJsonObject buildTagNbt(const ItemData &data);
    QJsonObject buildSingleTradeJson(const TradeOption &trade);
    QString serializeNbtData(const QList<TradeOption> &trades);

    // 物品选择器辅助
    QList<ItemMapping> buildItemMappingList();
    QString selectItemFromDialog(int &outDamage, QString &outPresetJson);

    // 控件与状态
    QTableWidget *m_tradeTable;
    QTextEdit *m_tePreview;
    QSpinBox *m_sbUses;
    QSpinBox *m_sbMaxUses;
    QSpinBox *m_sbTier;

    ItemWidgets wBuyA;
    ItemWidgets wBuyB;
    ItemWidgets wSell;

    QList<TradeOption> m_tradeOptions;
    int m_selectedTradeRow = -1;
    bool m_isUpdatingUI = false; // 用于阻止 UI 填充时触发 onDataChanged
    void updateCompleters();     // 新增：更新输入框的自动补全列表
    QList<ItemMapping> loadItemMappings(); // 修改：从文件加载映射
    void createDefaultItemConfig(const QString &path); // 新增：生成默认配置
};

#endif // VILLAGEREDITOR_H
