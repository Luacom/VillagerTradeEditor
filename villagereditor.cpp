#include "villagereditor.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QCompleter>
#include <QFile>
#include <QHeaderView>

// 递归查找指定 name 的 NBT 数组节点，无视嵌套深度
static QJsonArray findNbtArray(const QJsonArray &arr, const QString &targetName) {
    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        QJsonObject obj = v.toObject();

        // 找到了目标节点，并且它的 value 是个数组
        if (obj.value("name").toString() == targetName && obj.value("value").isArray()) {
            return obj.value("value").toArray();
        }

        // 没找到，但当前节点的 value 是数组，则递归往深处继续找
        if (obj.value("value").isArray()) {
            QJsonArray res = findNbtArray(obj.value("value").toArray(), targetName);
            if (!res.isEmpty()) return res;
        }
    }
    return QJsonArray(); // 没找到则返回空
}

VillagerEditor::VillagerEditor(QWidget *parent)
    : QMainWindow(parent)
{
    initUI();
}

VillagerEditor::~VillagerEditor() {}

void VillagerEditor::initUI()
{
    setWindowTitle("村民交易编辑器 - 专业生产力重构版");
    resize(1300, 900);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    // 工具栏
    QHBoxLayout *toolLayout = new QHBoxLayout();
    QPushButton *btnLoad = new QPushButton("加载源文件", this);
    QPushButton *btnSave = new QPushButton("保存文件", this);
    QPushButton *btnAdd = new QPushButton("添加交易项", this);
    QPushButton *btnDelete = new QPushButton("删除选中项", this);
    QPushButton *btnEditItems = new QPushButton("⚙️ 编辑物品库", this); // <== 新增按钮
    toolLayout->addWidget(btnLoad);
    toolLayout->addWidget(btnSave);
    toolLayout->addWidget(btnAdd);
    toolLayout->addWidget(btnDelete);
    toolLayout->addWidget(btnEditItems); // <== 添加到布局
    mainLayout->addLayout(toolLayout);

    // 交易表格
    m_tradeTable = new QTableWidget(this);
    m_tradeTable->setColumnCount(9);
    QStringList headers = {"BuyA物品名", "BuyA数量", "BuyB物品名", "BuyB数量", "Sell物品名", "Sell数量", "已用次数", "最大次数", "Tier"};
    m_tradeTable->setHorizontalHeaderLabels(headers);
    m_tradeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tradeTable->setEditTriggers(QTableWidget::NoEditTriggers);
    m_tradeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(m_tradeTable, 1);

    // 交易项参数编辑区
    QGroupBox *editGroup = new QGroupBox("当前交易项编辑", this);
    QHBoxLayout *editLayout = new QHBoxLayout(editGroup);

    // 抽象出三个相同的编辑面板
    editLayout->addWidget(createItemSection("Buy A (主输入)", wBuyA));
    editLayout->addWidget(createItemSection("Buy B (副输入)", wBuyB));
    editLayout->addWidget(createItemSection("Sell (输出)", wSell));

    mainLayout->addWidget(editGroup);

    // 基础属性区 (Uses, Tier)
    QGroupBox *baseAttrGroup = new QGroupBox("基础属性", this);
    QHBoxLayout *baseAttrLayout = new QHBoxLayout(baseAttrGroup);
    baseAttrLayout->addWidget(new QLabel("已用次数(uses):"));
    m_sbUses = new QSpinBox(this); m_sbUses->setRange(0, 999);
    baseAttrLayout->addWidget(m_sbUses);
    baseAttrLayout->addWidget(new QLabel("最大次数(maxUses):"));
    m_sbMaxUses = new QSpinBox(this); m_sbMaxUses->setRange(1, 999);
    baseAttrLayout->addWidget(m_sbMaxUses);
    baseAttrLayout->addWidget(new QLabel("购买等级(Tier):"));
    m_sbTier = new QSpinBox(this); m_sbTier->setRange(0, 5); m_sbTier->setValue(0);
    baseAttrLayout->addWidget(m_sbTier);
    baseAttrLayout->addStretch();
    mainLayout->addWidget(baseAttrGroup);

    // 预览区
    QGroupBox *previewGroup = new QGroupBox("文件预览", this);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    m_tePreview = new QTextEdit(this);
    m_tePreview->setReadOnly(true);
    m_tePreview->setLineWrapMode(QTextEdit::NoWrap);
    previewLayout->addWidget(m_tePreview);
    mainLayout->addWidget(previewGroup, 1);

    // 信号连接
    connect(btnLoad, &QPushButton::clicked, this, &VillagerEditor::loadFile);
    connect(btnSave, &QPushButton::clicked, this, &VillagerEditor::saveFile);
    connect(btnAdd, &QPushButton::clicked, this, &VillagerEditor::addTradeOption);
    connect(btnDelete, &QPushButton::clicked, this, &VillagerEditor::deleteTradeOption);
    connect(btnEditItems, &QPushButton::clicked, this, &VillagerEditor::openItemConfigEditor); // <== 绑定点击事件
    connect(m_tradeTable, &QTableWidget::cellClicked, this, &VillagerEditor::onTableItemSelected);

    // 绑定基础属性的同步槽
    connect(m_sbUses, &QSpinBox::valueChanged, this, &VillagerEditor::onDataChanged);
    connect(m_sbMaxUses, &QSpinBox::valueChanged, this, &VillagerEditor::onDataChanged);
    connect(m_sbTier, &QSpinBox::valueChanged, this, &VillagerEditor::onDataChanged);

    // 自动补全
    updateCompleters();
}

// 核心重构：高度抽象的 UI 生成器
QGroupBox* VillagerEditor::createItemSection(const QString &title, ItemWidgets &w)
{
    QGroupBox *group = new QGroupBox(title, this);
    QGridLayout *layout = new QGridLayout(group);

    // 基础物品信息
    layout->addWidget(new QLabel("物品名:"), 0, 0);
    w.leName = new QLineEdit(this);
    layout->addWidget(w.leName, 0, 1, 1, 2);
    w.btnSelect = new QPushButton("选择", this);
    layout->addWidget(w.btnSelect, 0, 3);

    layout->addWidget(new QLabel("数量:"), 1, 0);
    w.sbCount = new QSpinBox(this); w.sbCount->setRange(0, 999);
    layout->addWidget(w.sbCount, 1, 1);

    layout->addWidget(new QLabel("Damage:"), 1, 2);
    w.sbDamage = new QSpinBox(this); w.sbDamage->setRange(0, 32767);
    layout->addWidget(w.sbDamage, 1, 3);

    // Tag 信息
    w.cbEnableName = new QCheckBox("启用自定义名称", this);
    layout->addWidget(w.cbEnableName, 2, 0, 1, 2);
    w.leDisp = new QLineEdit(this);
    layout->addWidget(w.leDisp, 2, 2, 1, 2);

    w.cbEnableLore = new QCheckBox("启用注释(Lore)", this);
    layout->addWidget(w.cbEnableLore, 3, 0, 1, 2);
    w.leLore = new QLineEdit(this);
    layout->addWidget(w.leLore, 3, 2, 1, 2);

    w.cbEnableEnch = new QCheckBox("启用附魔", this);
    layout->addWidget(w.cbEnableEnch, 4, 0, 1, 2);
    QHBoxLayout *enchLayout = new QHBoxLayout();
    enchLayout->addWidget(new QLabel("ID:"));
    w.sbEnchId = new QSpinBox(this); w.sbEnchId->setRange(0, 255);
    enchLayout->addWidget(w.sbEnchId);
    enchLayout->addWidget(new QLabel("Lvl:"));
    w.sbEnchLvl = new QSpinBox(this); w.sbEnchLvl->setRange(1, 255);
    enchLayout->addWidget(w.sbEnchLvl);
    layout->addLayout(enchLayout, 4, 2, 1, 2);
    // 新增：自定义 NBT 节点
    w.cbEnableCustom = new QCheckBox("启用自定义NBT节点", this);
    layout->addWidget(w.cbEnableCustom, 5, 0, 1, 4); // 占用整行
    w.teCustom = new QTextEdit(this);
    w.teCustom->setPlaceholderText("输入JSON数组，例如：\n[{\"name\":\"CanPlaceOn\",\"value\":[\"minecraft:grass\"],\"type\":9}]");
    w.teCustom->setMaximumHeight(100);
    layout->addWidget(w.teCustom, 6, 0, 1, 4);
    w.teCustom->setVisible(false); // 默认隐藏

    // 绑定所有的统一更新事件
    auto syncSlot = &VillagerEditor::onDataChanged;
    connect(w.leName, &QLineEdit::textChanged, this, syncSlot);
    connect(w.sbCount, &QSpinBox::valueChanged, this, syncSlot);
    connect(w.sbDamage, &QSpinBox::valueChanged, this, syncSlot);
    connect(w.leDisp, &QLineEdit::textChanged, this, syncSlot);
    connect(w.leLore, &QLineEdit::textChanged, this, syncSlot);
    connect(w.sbEnchId, &QSpinBox::valueChanged, this, syncSlot);
    connect(w.sbEnchLvl, &QSpinBox::valueChanged, this, syncSlot);

    // 绑定复选框状态显示/隐藏事件
    connect(w.cbEnableName, &QCheckBox::stateChanged, this, &VillagerEditor::onTagCheckboxToggled);
    connect(w.cbEnableLore, &QCheckBox::stateChanged, this, &VillagerEditor::onTagCheckboxToggled);
    connect(w.cbEnableEnch, &QCheckBox::stateChanged, this, &VillagerEditor::onTagCheckboxToggled);

    // 绑定物品选择按钮
    connect(w.btnSelect, &QPushButton::clicked, this, [this, &w]() { openItemSelector(&w); });
    // 连接自定义节点复选框的切换事件
    connect(w.cbEnableCustom, &QCheckBox::toggled, this, [this, &w](bool checked) {
        // 当自定义节点启用时，禁用并取消勾选其他三个复选框
        w.cbEnableName->setEnabled(!checked);
        w.cbEnableLore->setEnabled(!checked);
        w.cbEnableEnch->setEnabled(!checked);

        if (checked) {
            w.cbEnableName->setChecked(false);
            w.cbEnableLore->setChecked(false);
            w.cbEnableEnch->setChecked(false);
        }

        // 显示/隐藏自定义编辑框
        w.teCustom->setVisible(checked);
        onDataChanged();   // 触发数据更新
    });
    // 连接文本变化
    connect(w.teCustom, &QTextEdit::textChanged, this, &VillagerEditor::onDataChanged);

    return group;
}

// 核心重构：统一的 UI 状态切换
void VillagerEditor::onTagCheckboxToggled()
{
    if (m_isUpdatingUI) return;

    auto toggle = [](ItemWidgets &w) {
        w.leDisp->setVisible(w.cbEnableName->isChecked());
        w.leLore->setVisible(w.cbEnableLore->isChecked());
        w.sbEnchId->setVisible(w.cbEnableEnch->isChecked());
        w.sbEnchLvl->setVisible(w.cbEnableEnch->isChecked());
    };
    toggle(wBuyA); toggle(wBuyB); toggle(wSell);

    onDataChanged(); // 触发保存
}

// 核心重构：单向数据流 - 数据推送到 UI
void VillagerEditor::populateUIFromData(const TradeOption &trade)
{
    m_isUpdatingUI = true; // 锁定，防止触发 onChange

    auto fillItem = [this](ItemWidgets &w, const ItemData &d) {
        w.leName->setText(d.name);
        w.sbCount->setValue(d.count);
        w.sbDamage->setValue(d.damage);

        // 如果启用了自定义节点，则强制禁用并取消勾选内置 Tag 复选框
        if (d.enableCustom) {
            w.cbEnableName->setChecked(false);
            w.cbEnableLore->setChecked(false);
            w.cbEnableEnch->setChecked(false);
            w.cbEnableName->setEnabled(false);
            w.cbEnableLore->setEnabled(false);
            w.cbEnableEnch->setEnabled(false);
        } else {
            w.cbEnableName->setChecked(d.enableName);
            w.cbEnableLore->setChecked(d.enableLore);
            w.cbEnableEnch->setChecked(d.enableEnch);
            w.cbEnableName->setEnabled(true);
            w.cbEnableLore->setEnabled(true);
            w.cbEnableEnch->setEnabled(true);
        }

        // 设置显示文本（即使被禁用也保留内容）
        w.leDisp->setText(d.displayName);
        w.leLore->setText(d.lore);
        w.sbEnchId->setValue(d.enchId);
        w.sbEnchLvl->setValue(d.enchLevel);

        // 根据复选框状态控制输入框可见性
        w.leDisp->setVisible(w.cbEnableName->isChecked());
        w.leLore->setVisible(w.cbEnableLore->isChecked());
        w.sbEnchId->setVisible(w.cbEnableEnch->isChecked());
        w.sbEnchLvl->setVisible(w.cbEnableEnch->isChecked());

        // 自定义节点本身
        w.cbEnableCustom->setChecked(d.enableCustom);
        if (d.enableCustom) {
            QJsonDocument doc(d.customNodes);
            w.teCustom->setPlainText(doc.toJson(QJsonDocument::Indented));
        } else {
            w.teCustom->clear();
        }
        w.teCustom->setVisible(d.enableCustom);
    };

    fillItem(wBuyA, trade.buyA);
    fillItem(wBuyB, trade.buyB);
    fillItem(wSell, trade.sell);

    m_sbUses->setValue(trade.uses);
    m_sbMaxUses->setValue(trade.maxUses);
    m_sbTier->setValue(trade.tier);

    m_isUpdatingUI = false; // 解锁
}

// 核心重构：单向数据流 - UI 更新到数据模型
void VillagerEditor::syncDataFromUI()
{
    if (m_selectedTradeRow < 0 || m_selectedTradeRow >= m_tradeOptions.size()) return;

    TradeOption &trade = m_tradeOptions[m_selectedTradeRow];

    auto readItem = [](const ItemWidgets &w, ItemData &d) {
        d.name = w.leName->text().trimmed();
        d.count = w.sbCount->value();
        d.damage = w.sbDamage->value();
        d.enableName = w.cbEnableName->isChecked();
        d.displayName = w.leDisp->text().trimmed();
        d.enableLore = w.cbEnableLore->isChecked();
        d.lore = w.leLore->text().trimmed();
        d.enableEnch = w.cbEnableEnch->isChecked();
        d.enchId = w.sbEnchId->value();
        d.enchLevel = w.sbEnchLvl->value();
        // 新增：自定义节点
        d.enableCustom = w.cbEnableCustom->isChecked();
        if (d.enableCustom) {
            QString customText = w.teCustom->toPlainText().trimmed();
            if (!customText.isEmpty()) {
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(customText.toUtf8(), &err);
                if (err.error == QJsonParseError::NoError && doc.isArray()) {
                    d.customNodes = doc.array();
                } else {
                    d.customNodes = QJsonArray(); // 解析失败则清空
                }
            } else {
                d.customNodes = QJsonArray();
            }
        } else {
            d.customNodes = QJsonArray();
        }
    };

    readItem(wBuyA, trade.buyA);
    readItem(wBuyB, trade.buyB);
    readItem(wSell, trade.sell);

    trade.uses = m_sbUses->value();
    trade.maxUses = m_sbMaxUses->value();
    trade.tier = m_sbTier->value();
}

void VillagerEditor::onDataChanged()
{
    if (m_isUpdatingUI) return; // 如果正在填充界面，则不响应更改

    syncDataFromUI();
    updateTradeTable();
    m_tePreview->setText(serializeNbtData(m_tradeOptions));
}

// ==================== 原有的其他逻辑封装 ====================

void VillagerEditor::onTableItemSelected(int row, int /*column*/)
{
    if (row < 0 || row >= m_tradeOptions.size()) {
        m_selectedTradeRow = -1;
        TradeOption emptyTrade; // 加载默认空项
        populateUIFromData(emptyTrade);
        return;
    }
    m_selectedTradeRow = row;
    populateUIFromData(m_tradeOptions[row]);
}

void VillagerEditor::addTradeOption()
{
    TradeOption newTrade;
    newTrade.buyB.count = 0; // 默认 BuyB 不启用
    m_tradeOptions.append(newTrade);

    updateTradeTable();
    m_tradeTable->selectRow(m_tradeOptions.size() - 1);
    onTableItemSelected(m_tradeOptions.size() - 1, 0);

    m_tePreview->setText(serializeNbtData(m_tradeOptions));
}

void VillagerEditor::deleteTradeOption()
{
    if (m_selectedTradeRow < 0 || m_selectedTradeRow >= m_tradeOptions.size()) return;

    m_tradeOptions.removeAt(m_selectedTradeRow);
    m_selectedTradeRow = -1;
    TradeOption emptyTrade;
    populateUIFromData(emptyTrade);
    updateTradeTable();
    m_tePreview->setText(serializeNbtData(m_tradeOptions));
}

void VillagerEditor::updateTradeTable()
{
    m_isUpdatingUI = true; // 防止触发表格变动带来的副作用
    m_tradeTable->setRowCount(0);

    for (int i = 0; i < m_tradeOptions.size(); ++i) {
        const TradeOption &t = m_tradeOptions[i];
        m_tradeTable->insertRow(i);
        m_tradeTable->setItem(i, 0, new QTableWidgetItem(t.buyA.name));
        m_tradeTable->setItem(i, 1, new QTableWidgetItem(QString::number(t.buyA.count)));
        m_tradeTable->setItem(i, 2, new QTableWidgetItem(t.buyB.name));
        m_tradeTable->setItem(i, 3, new QTableWidgetItem(QString::number(t.buyB.count)));
        m_tradeTable->setItem(i, 4, new QTableWidgetItem(t.sell.name));
        m_tradeTable->setItem(i, 5, new QTableWidgetItem(QString::number(t.sell.count)));
        m_tradeTable->setItem(i, 6, new QTableWidgetItem(QString::number(t.uses)));
        m_tradeTable->setItem(i, 7, new QTableWidgetItem(QString::number(t.maxUses)));
        m_tradeTable->setItem(i, 8, new QTableWidgetItem(QString::number(t.tier)));
    }

    if (m_selectedTradeRow >= 0 && m_selectedTradeRow < m_tradeOptions.size()) {
        m_tradeTable->selectRow(m_selectedTradeRow);
    }
    m_isUpdatingUI = false;
}

// 物品选择器
void VillagerEditor::openItemSelector(ItemWidgets *widgets)
{
    int damage = 0;
    QString itemName = selectItemFromDialog(damage);
    if (!itemName.isEmpty()) {
        widgets->leName->setText(itemName);
        widgets->sbDamage->setValue(damage);
        if (widgets->sbCount->value() == 0) widgets->sbCount->setValue(1);
    }
}

QString VillagerEditor::selectItemFromDialog(int &outDamage)
{
    QDialog dialog(this);
    dialog.setWindowTitle("选择物品");
    dialog.setModal(true);
    dialog.resize(500, 550); // 稍微加宽一点以显示分类

    QVBoxLayout vLayout(&dialog);

    // ========== 1. 顶部搜索与分类区 ==========
    QHBoxLayout hLayout;

    QComboBox *categoryCombo = new QComboBox(&dialog);
    categoryCombo->addItem("全部"); // 默认选项

    QLineEdit searchEdit(&dialog);
    searchEdit.setPlaceholderText("搜索物品(中/英文)...");

    // 设置下拉框和搜索框比例为 1:2
    hLayout.addWidget(categoryCombo, 1);
    hLayout.addWidget(&searchEdit, 2);
    vLayout.addLayout(&hLayout);

    // ========== 2. 物品列表加载 ==========
    QListWidget itemList(&dialog);
    QList<ItemMapping> items = loadItemMappings();

    QStringList categories;
    for (const auto &mapping : items) {
        // 收集不重复的分类用于填充下拉框
        if (!categories.contains(mapping.category)) {
            categories.append(mapping.category);
        }

        // UI 显示格式：[矿物] 绿宝石（minecraft:emerald）
        QListWidgetItem *item = new QListWidgetItem(QString("[%1] %2（%3）")
                                                        .arg(mapping.category, mapping.chineseName, mapping.englishId));

        // 绑定隐藏数据
        item->setData(Qt::UserRole, mapping.englishId);
        item->setData(Qt::UserRole + 1, mapping.defaultDamage);
        item->setData(Qt::UserRole + 2, mapping.category); // 存入分类名，用于过滤

        itemList.addItem(item);
    }

    categoryCombo->addItems(categories);
    vLayout.addWidget(&itemList);

    // ========== 3. 双重过滤逻辑 (分类 + 搜索) ==========
    auto filterItems = [&]() {
        QString searchText = searchEdit.text();
        QString selectedCategory = categoryCombo->currentText();

        for (int i = 0; i < itemList.count(); ++i) {
            QListWidgetItem *item = itemList.item(i);
            QString itemCategory = item->data(Qt::UserRole + 2).toString();
            QString itemText = item->text();

            // 条件1：分类匹配（或是选择了"全部"）
            bool categoryMatch = (selectedCategory == "全部" || selectedCategory == itemCategory);
            // 条件2：搜索文本匹配
            bool textMatch = itemText.contains(searchText, Qt::CaseInsensitive);

            // 只有同时满足分类和搜索词才显示
            item->setHidden(!(categoryMatch && textMatch));
        }
    };

    // 搜索框输入和下拉框选择改变时，都触发过滤
    connect(&searchEdit, &QLineEdit::textChanged, filterItems);
    connect(categoryCombo, &QComboBox::currentTextChanged, filterItems);

    // ========== 4. 确认与取消逻辑 ==========
    QString selectedId;
    auto onSelect = [&]() {
        if (QListWidgetItem *item = itemList.currentItem()) {
            selectedId = item->data(Qt::UserRole).toString();
            outDamage = item->data(Qt::UserRole + 1).toInt();
            dialog.accept();
        }
    };

    QPushButton btnConfirm("确认");
    QPushButton btnCancel("取消");
    QHBoxLayout btnLayout;
    btnLayout.addStretch();
    btnLayout.addWidget(&btnConfirm);
    btnLayout.addWidget(&btnCancel);
    vLayout.addLayout(&btnLayout);

    connect(&btnConfirm, &QPushButton::clicked, onSelect);
    connect(&btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(&itemList, &QListWidget::itemDoubleClicked, onSelect); // 双击直接选择

    dialog.exec();
    return selectedId;
}

// ==================== NBT 序列化与解析重构 ====================

QJsonObject VillagerEditor::createNode(const QString &name, const QJsonValue &value, int type) {
    return QJsonObject{{"name", name}, {"value", value}, {"type", type}};
}

QJsonObject VillagerEditor::buildTagNbt(const ItemData &data)
{
    QJsonArray tagArr;

    if (data.enableName || data.enableLore) {
        QJsonArray displayArr;
        if (data.enableName) displayArr.append(createNode("Name", data.displayName, 8));
        if (data.enableLore) {
            QJsonArray loreArr; loreArr.append(createNode("", data.lore, 8));
            displayArr.append(createNode("Lore", loreArr, 9));
        }
        tagArr.append(createNode("display", displayArr, 10));
    }

    if (data.enableEnch) {
        QJsonArray enchInner;
        enchInner.append(createNode("id", data.enchId, 2));
        enchInner.append(createNode("lvl", data.enchLevel, 2));
        QJsonArray enchList; enchList.append(createNode("", enchInner, 10));
        tagArr.append(createNode("ench", enchList, 9));
    }

    return tagArr.isEmpty() ? QJsonObject() : createNode("tag", tagArr, 10);
}

QJsonObject VillagerEditor::buildItemNbt(const QString &key, const ItemData &data)
{
    QJsonArray arr;
    arr.append(createNode("Count", data.count, 1));
    arr.append(createNode("Damage", data.damage, 2));
    arr.append(createNode("Name", data.name, 8));
    arr.append(createNode("WasPickedUp", 0, 1));

    QJsonObject tag = buildTagNbt(data);
    if (!tag.isEmpty()) arr.append(tag);

    // 追加自定义节点
    if (data.enableCustom) {
        for (const QJsonValue &cv : data.customNodes) {
            if (cv.isObject()) {
                arr.append(cv.toObject());
            }
        }
    }

    return createNode(key, arr, 10);
}

QJsonObject VillagerEditor::buildSingleTradeJson(const TradeOption &trade)
{
    QJsonArray tradeValueArr;
    tradeValueArr.append(buildItemNbt("buyA", trade.buyA));
    tradeValueArr.append(buildItemNbt("buyB", trade.buyB));
    tradeValueArr.append(createNode("buyCountA", trade.buyA.count, 3));
    tradeValueArr.append(createNode("buyCountB", trade.buyB.count, 3));
    tradeValueArr.append(createNode("demand", 0, 3));
    tradeValueArr.append(createNode("maxUses", trade.maxUses, 3));
    tradeValueArr.append(createNode("priceMultiplierA", 0.05, 5));
    tradeValueArr.append(createNode("priceMultiplierB", 0.0, 5));
    tradeValueArr.append(createNode("rewardExp", 1, 1));
    tradeValueArr.append(buildItemNbt("sell", trade.sell));
    tradeValueArr.append(createNode("tier", trade.tier, 3));
    tradeValueArr.append(createNode("traderExp", 5, 3));
    tradeValueArr.append(createNode("uses", trade.uses, 3));

    return createNode("", tradeValueArr, 10);
}

QString VillagerEditor::serializeNbtData(const QList<TradeOption> &trades)
{
    QJsonArray recipesArr;
    for (const TradeOption &trade : trades) {
        recipesArr.append(buildSingleTradeJson(trade));
    }
    QJsonObject recipesObj = createNode("Recipes", recipesArr, 9);

    // 构建 TierExpRequirements (硬编码以适配格式)
    QJsonArray tierExpArr;
    tierExpArr.append(createNode("", QJsonArray{createNode("0", 0, 3)}, 10));
    tierExpArr.append(createNode("", QJsonArray{createNode("1", 10, 3)}, 10));
    tierExpArr.append(createNode("", QJsonArray{createNode("2", 70, 3)}, 10));
    tierExpArr.append(createNode("", QJsonArray{createNode("3", 150, 3)}, 10));
    tierExpArr.append(createNode("", QJsonArray{createNode("4", 250, 3)}, 10));
    QJsonObject tierExpObj = createNode("TierExpRequirements", tierExpArr, 9);

    QJsonArray offersArr;
    offersArr.append(recipesObj);
    offersArr.append(tierExpObj);
    QJsonObject offersObj = createNode("Offers", offersArr, 10);

    // 头尾硬编码保护格式绝对正确 (保留你原本的长字符串)
    QString fixedHead = "{\"name\":\"\",\"value\":[{\"name\":\"format_version\",\"value\":1,\"type\":3},{\"name\":\"size\",\"value\":[{\"name\":\"\",\"value\":1,\"type\":3},{\"name\":\"\",\"value\":1,\"type\":3},{\"name\":\"\",\"value\":1,\"type\":3}],\"type\":9},{\"name\":\"structure\",\"value\":[{\"name\":\"block_indices\",\"value\":[{\"name\":\"\",\"value\":[{\"name\":\"\",\"value\":-1,\"type\":3}],\"type\":9},{\"name\":\"\",\"value\":[{\"name\":\"\",\"value\":-1,\"type\":3}],\"type\":9}],\"type\":9},{\"name\":\"entities\",\"value\":[{\"name\":\"\",\"value\":[{\"name\":\"Air\",\"value\":300,\"type\":2},{\"name\":\"Armor\",\"value\":[{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10}],\"type\":9},{\"name\":\"Attributes\",\"value\":[{\"name\":\"\",\"value\":[{\"name\":\"Base\",\"value\":20.0,\"type\":5},{\"name\":\"Current\",\"value\":20.0,\"type\":5},{\"name\":\"DefaultMax\",\"value\":20.0,\"type\":5},{\"name\":\"DefaultMin\",\"value\":0.0,\"type\":5},{\"name\":\"Max\",\"value\":20.0,\"type\":5},{\"name\":\"Min\",\"value\":0.0,\"type\":5},{\"name\":\"Name\",\"value\":\"minecraft:health\",\"type\":8}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Base\",\"value\":128.0,\"type\":5},{\"name\":\"Current\",\"value\":128.0,\"type\":5},{\"name\":\"DefaultMax\",\"value\":2048.0,\"type\":5},{\"name\":\"DefaultMin\",\"value\":0.0,\"type\":5},{\"name\":\"Max\",\"value\":2048.0,\"type\":5},{\"name\":\"Min\",\"value\":0.0,\"type\":5},{\"name\":\"Name\",\"value\":\"minecraft:follow_range\",\"type\":8}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Base\",\"value\":0.0,\"type\":5},{\"name\":\"Current\",\"value\":0.0,\"type\":5},{\"name\":\"DefaultMax\",\"value\":1.0,\"type\":5},{\"name\":\"DefaultMin\",\"value\":0.0,\"type\":5},{\"name\":\"Max\",\"value\":1.0,\"type\":5},{\"name\":\"Min\",\"value\":0.0,\"type\":5},{\"name\":\"Name\",\"value\":\"minecraft:knockback_resistance\",\"type\":8}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Base\",\"value\":0.5,\"type\":5},{\"name\":\"Current\",\"value\":0.5,\"type\":5},{\"name\":\"DefaultMax\",\"value\":3.4028235E38,\"type\":5},{\"name\":\"DefaultMin\",\"value\":0.0,\"type\":5},{\"name\":\"Max\",\"value\":3.4028235E38,\"type\":5},{\"name\":\"Min\",\"value\":0.0,\"type\":5},{\"name\":\"Name\",\"value\":\"minecraft:movement\",\"type\":8}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Base\",\"value\":0.02,\"type\":5},{\"name\":\"Current\",\"value\":0.02,\"type\":5},{\"name\":\"DefaultMax\",\"value\":3.4028235E38,\"type\":5},{\"name\":\"DefaultMin\",\"value\":0.0,\"type\":5},{\"name\":\"Max\",\"value\":3.4028235E38,\"type\":5},{\"name\":\"Min\",\"value\":0.0,\"type\":5},{\"name\":\"Name\",\"value\":\"minecraft:underwater_movement\",\"type\":8}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Base\",\"value\":0.02,\"type\":5},{\"name\":\"Current\",\"value\":0.02,\"type\":5},{\"name\":\"DefaultMax\",\"value\":3.4028235E38,\"type\":5},{\"name\":\"DefaultMin\",\"value\":0.0,\"type\":5},{\"name\":\"Max\",\"value\":3.4028235E38,\"type\":5},{\"name\":\"Min\",\"value\":0.0,\"type\":5},{\"name\":\"Name\",\"value\":\"minecraft:lava_movement\",\"type\":8}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Base\",\"value\":0.0,\"type\":5},{\"name\":\"Current\",\"value\":0.0,\"type\":5},{\"name\":\"DefaultMax\",\"value\":16.0,\"type\":5},{\"name\":\"DefaultMin\",\"value\":0.0,\"type\":5},{\"name\":\"Max\",\"value\":16.0,\"type\":5},{\"name\":\"Min\",\"value\":0.0,\"type\":5},{\"name\":\"Name\",\"value\":\"minecraft:absorption\",\"type\":8}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Base\",\"value\":0.0,\"type\":5},{\"name\":\"Current\",\"value\":0.0,\"type\":5},{\"name\":\"DefaultMax\",\"value\":1024.0,\"type\":5},{\"name\":\"DefaultMin\",\"value\":-1024.0,\"type\":5},{\"name\":\"Max\",\"value\":1024.0,\"type\":5},{\"name\":\"Min\",\"value\":-1024.0,\"type\":5},{\"name\":\"Name\",\"value\":\"minecraft:luck\",\"type\":8}],\"type\":10}],\"type\":9},{\"name\":\"ChestItems\",\"value\":[{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"Slot\",\"value\":0,\"type\":1},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"Slot\",\"value\":1,\"type\":1},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"Slot\",\"value\":2,\"type\":1},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"Slot\",\"value\":3,\"type\":1},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"Slot\",\"value\":4,\"type\":1},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"Slot\",\"value\":5,\"type\":1},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"Slot\",\"value\":6,\"type\":1},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10},{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"Slot\",\"value\":7,\"type\":1},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10}],\"type\":9},{\"name\":\"Chested\",\"value\":0,\"type\":1},{\"name\":\"Color\",\"value\":0,\"type\":1},{\"name\":\"Color2\",\"value\":0,\"type\":1},{\"name\":\"Dead\",\"value\":0,\"type\":1},{\"name\":\"DeathTime\",\"value\":0,\"type\":2},{\"name\":\"DwellingUniqueID\",\"value\":\"00000000-0000-0000-0000-000000000000\",\"type\":8},{\"name\":\"FallDistance\",\"value\":0.0,\"type\":5},{\"name\":\"HighTierCuredDiscount\",\"value\":0,\"type\":3},{\"name\":\"HurtTime\",\"value\":0,\"type\":2},{\"name\":\"InventoryVersion\",\"value\":\"1.21.132\",\"type\":8},{\"name\":\"Invulnerable\",\"value\":0,\"type\":1},{\"name\":\"IsAngry\",\"value\":0,\"type\":1},{\"name\":\"IsAutonomous\",\"value\":0,\"type\":1},{\"name\":\"IsBaby\",\"value\":0,\"type\":1},{\"name\":\"IsEating\",\"value\":0,\"type\":1},{\"name\":\"IsGliding\",\"value\":0,\"type\":1},{\"name\":\"IsGlobal\",\"value\":0,\"type\":1},{\"name\":\"IsIllagerCaptain\",\"value\":0,\"type\":1},{\"name\":\"IsInRaid\",\"value\":0,\"type\":1},{\"name\":\"IsOrphaned\",\"value\":0,\"type\":1},{\"name\":\"IsOutOfControl\",\"value\":0,\"type\":1},{\"name\":\"IsPregnant\",\"value\":0,\"type\":1},{\"name\":\"IsRoaring\",\"value\":0,\"type\":1},{\"name\":\"IsScared\",\"value\":0,\"type\":1},{\"name\":\"IsStunned\",\"value\":0,\"type\":1},{\"name\":\"IsSwimming\",\"value\":0,\"type\":1},{\"name\":\"IsTamed\",\"value\":0,\"type\":1},{\"name\":\"IsTrusting\",\"value\":0,\"type\":1},{\"name\":\"LeasherID\",\"value\":\"-1\",\"type\":4},{\"name\":\"LootDropped\",\"value\":0,\"type\":1},{\"name\":\"LowTierCuredDiscount\",\"value\":0,\"type\":3},{\"name\":\"Mainhand\",\"value\":[{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10}],\"type\":9},{\"name\":\"MarkVariant\",\"value\":0,\"type\":3},{\"name\":\"NaturalSpawn\",\"value\":0,\"type\":1},{\"name\":\"NearbyCuredDiscount\",\"value\":0,\"type\":3},{\"name\":\"NearbyCuredDiscountTimeStamp\",\"value\":0,\"type\":3},";
    QString fixedFoot = ",{\"name\":\"Offhand\",\"value\":[{\"name\":\"\",\"value\":[{\"name\":\"Count\",\"value\":0,\"type\":1},{\"name\":\"Damage\",\"value\":0,\"type\":2},{\"name\":\"Name\",\"value\":\"\",\"type\":8},{\"name\":\"WasPickedUp\",\"value\":0,\"type\":1}],\"type\":10}],\"type\":9},{\"name\":\"OnGround\",\"value\":1,\"type\":1},{\"name\":\"OwnerNew\",\"value\":\"-1\",\"type\":4},{\"name\":\"Persistent\",\"value\":1,\"type\":1},{\"name\":\"PortalCooldown\",\"value\":0,\"type\":3},{\"name\":\"Pos\",\"value\":[{\"name\":\"\",\"value\":-58.5,\"type\":5},{\"name\":\"\",\"value\":-59.0,\"type\":5},{\"name\":\"\",\"value\":-223.5,\"type\":5}],\"type\":9},{\"name\":\"PreferredProfession\",\"value\":\"cartographer\",\"type\":8},{\"name\":\"ReactToBell\",\"value\":0,\"type\":1},{\"name\":\"RewardPlayersOnFirstFounding\",\"value\":1,\"type\":1},{\"name\":\"Riches\",\"value\":0,\"type\":3},{\"name\":\"Rotation\",\"value\":[{\"name\":\"\",\"value\":97.6936,\"type\":5},{\"name\":\"\",\"value\":39.88098,\"type\":5}],\"type\":9},{\"name\":\"Saddled\",\"value\":0,\"type\":1},{\"name\":\"Sheared\",\"value\":0,\"type\":1},{\"name\":\"ShowBottom\",\"value\":0,\"type\":1},{\"name\":\"Sitting\",\"value\":0,\"type\":1},{\"name\":\"SkinID\",\"value\":2,\"type\":3},{\"name\":\"SlotDropChances\",\"value\":[{\"name\":\"\",\"value\":[{\"name\":\"DropChance\",\"value\":0.0,\"type\":5},{\"name\":\"Slot\",\"value\":\"mainhand\",\"type\":8}],\"type\":10}],\"type\":9},{\"name\":\"Strength\",\"value\":0,\"type\":3},{\"name\":\"StrengthMax\",\"value\":0,\"type\":3},{\"name\":\"Surface\",\"value\":0,\"type\":1},{\"name\":\"Tags\",\"value\":[],\"type\":9},{\"name\":\"TargetID\",\"value\":\"-1\",\"type\":4},{\"name\":\"TradeExperience\",\"value\":0,\"type\":3},{\"name\":\"TradeTier\",\"value\":0,\"type\":3},{\"name\":\"UniqueID\",\"value\":\"-317827579897\",\"type\":4},{\"name\":\"Variant\",\"value\":6,\"type\":3},{\"name\":\"Willing\",\"value\":0,\"type\":1},{\"name\":\"boundX\",\"value\":0,\"type\":3},{\"name\":\"boundY\",\"value\":0,\"type\":3},{\"name\":\"boundZ\",\"value\":0,\"type\":3},{\"name\":\"canPickupItems\",\"value\":0,\"type\":1},{\"name\":\"definitions\",\"value\":[{\"name\":\"\",\"value\":\"+minecraft:villager_v2\",\"type\":8},{\"name\":\"\",\"value\":\"+villager_skin_2\",\"type\":8},{\"name\":\"\",\"value\":\"+adult\",\"type\":8},{\"name\":\"\",\"value\":\"+cartographer\",\"type\":8},{\"name\":\"\",\"value\":\"+basic_schedule\",\"type\":8},{\"name\":\"\",\"value\":\"-job_specific_goals\",\"type\":8}],\"type\":9},{\"name\":\"hasBoundOrigin\",\"value\":0,\"type\":1},{\"name\":\"hasSetCanPickupItems\",\"value\":1,\"type\":1},{\"name\":\"identifier\",\"value\":\"minecraft:villager_v2\",\"type\":8},{\"name\":\"internalComponents\",\"value\":[],\"type\":10}],\"type\":10}],\"type\":9},{\"name\":\"palette\",\"value\":[{\"name\":\"default\",\"value\":[{\"name\":\"block_palette\",\"value\":[],\"type\":9},{\"name\":\"block_position_data\",\"value\":[],\"type\":10}],\"type\":10}],\"type\":10}],\"type\":10},{\"name\":\"structure_world_origin\",\"value\":[{\"name\":\"\",\"value\":-59,\"type\":3},{\"name\":\"\",\"value\":-59,\"type\":3},{\"name\":\"\",\"value\":-224,\"type\":3}],\"type\":9}],\"type\":10}";

    QJsonDocument doc;
    doc.setObject(offersObj);
    return fixedHead + doc.toJson(QJsonDocument::Compact) + fixedFoot;
}

// ==================== 文件读写 ====================

ItemData VillagerEditor::parseItemData(const QJsonArray &arr)
{
    ItemData item;
    QJsonArray customNodes; // 临时收集自定义节点
    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        QJsonObject obj = v.toObject();
        QString n = obj.value("name").toString();
        QJsonValue val = obj.value("value");

        if (n == "Count") item.count = val.toInt();
        else if (n == "Damage") item.damage = val.toInt();
        else if (n == "Name") item.name = val.toString();
        else if (n == "WasPickedUp") {
            // 忽略，保持默认值
        }
        else if (n == "tag" && val.isArray()) {
            // 原有 tag 解析保持不变
            QJsonArray tagArr = val.toArray();
            for (const QJsonValue &tv : tagArr) {
                QJsonObject tobj = tv.toObject();
                QString tn = tobj.value("name").toString();

                if (tn == "display" && tobj.value("value").isArray()) {
                    for (const QJsonValue &dv : tobj.value("value").toArray()) {
                        QJsonObject dobj = dv.toObject();
                        if (dobj.value("name").toString() == "Name") {
                            item.enableName = true;
                            item.displayName = dobj.value("value").toString();
                        } else if (dobj.value("name").toString() == "Lore") {
                            item.enableLore = true;
                            QJsonArray loreArr = dobj.value("value").toArray();
                            if (!loreArr.isEmpty()) item.lore = loreArr[0].toObject().value("value").toString();
                        }
                    }
                } else if (tn == "ench" && tobj.value("value").isArray()) {
                    item.enableEnch = true;
                    QJsonArray enchArr = tobj.value("value").toArray();
                    if (!enchArr.isEmpty()) {
                        QJsonArray innerArr = enchArr[0].toObject().value("value").toArray();
                        for (const QJsonValue &iv : innerArr) {
                            QJsonObject iobj = iv.toObject();
                            if (iobj.value("name").toString() == "id") item.enchId = iobj.value("value").toInt();
                            if (iobj.value("name").toString() == "lvl") item.enchLevel = iobj.value("value").toInt();
                        }
                    }
                }
                // 注意：tag 内部的其他节点不会单独处理，它们将保留在原有的 tag 节点中，不会丢失
            }
        }
        else {
            // 不是标准字段，则视为自定义节点，保留原样
            customNodes.append(obj);
        }
    }

    if (!customNodes.isEmpty()) {
        item.enableCustom = true;
        item.customNodes = customNodes;
    }
    return item;
}

QList<TradeOption> VillagerEditor::parseNbtData(const QString &nbtText)
{
    QList<TradeOption> trades;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(nbtText.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        return trades;
    }

    // 1. 获取最外层根数组
    QJsonArray rootArr;
    if (doc.isObject() && doc.object().contains("value")) {
        rootArr = doc.object().value("value").toArray();
    } else if (doc.isArray()) {
        rootArr = doc.array();
    }

    // 2. 核心修复：使用递归函数，无视固定头尾的层层嵌套，直接提取 Offers 和 Recipes
    QJsonArray offersArr = findNbtArray(rootArr, "Offers");
    QJsonArray recipesArr = findNbtArray(offersArr, "Recipes");

    // 3. 开始解析交易列表
    for (const QJsonValue &r : recipesArr) {
        TradeOption trade;
        for (const QJsonValue &f : r.toObject().value("value").toArray()) {
            QJsonObject fObj = f.toObject();
            QString name = fObj.value("name").toString();

            if (name == "buyA") trade.buyA = parseItemData(fObj.value("value").toArray());
            else if (name == "buyB") trade.buyB = parseItemData(fObj.value("value").toArray());
            else if (name == "sell") trade.sell = parseItemData(fObj.value("value").toArray());
            else if (name == "uses") trade.uses = fObj.value("value").toInt();
            else if (name == "maxUses") trade.maxUses = fObj.value("value").toInt();
            else if (name == "tier") trade.tier = fObj.value("value").toInt();
        }
        trades.append(trade);
    }

    return trades;
}

void VillagerEditor::loadFile()
{
    QString path = QFileDialog::getOpenFileName(this, "加载文件", "", "JSON (*.json);;所有 (*.*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString text = file.readAll();
    file.close();

    m_tradeOptions = parseNbtData(text);
    updateTradeTable();

    if (!m_tradeOptions.isEmpty()) {
        m_tradeTable->selectRow(0);
        onTableItemSelected(0, 0);
    } else {
        TradeOption empty;
        populateUIFromData(empty);
        m_selectedTradeRow = -1;
    }
    m_tePreview->setText(text);
    QMessageBox::information(this, "加载成功", QString("解析到 %1 条交易").arg(m_tradeOptions.size()));
}

void VillagerEditor::saveFile()
{
    // 新增：先验证所有自定义节点的有效性
    if (!validateCustomNodes()) {
        QMessageBox::warning(this, "保存失败",
                             "存在启用了自定义NBT节点但内容为空或无效的交易项。\n"
                             "请确保每个启用了自定义节点的输入框中的JSON格式正确（必须是数组，每个元素为对象，包含name, value, type）。");
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, "保存文件", "", "JSON (*.json)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    file.write(serializeNbtData(m_tradeOptions).toUtf8());
    file.close();
    QMessageBox::information(this, "成功", "保存完毕");
}

void VillagerEditor::updateCompleters()
{
    QList<ItemMapping> itemMappings = loadItemMappings();
    QStringList completerItems;
    for (const auto &mapping : itemMappings) {
        completerItems << mapping.chineseName
                       << mapping.englishId
                       << QString("%1（%2）").arg(mapping.chineseName, mapping.englishId);
    }

    auto setupCompleter = [&](QLineEdit *le) {
        QCompleter *completer = new QCompleter(completerItems, this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        le->setCompleter(completer);
    };

    setupCompleter(wBuyA.leName);
    setupCompleter(wBuyB.leName);
    setupCompleter(wSell.leName);
}

// ==================== 物品库配置系统 ====================

// 1. 生成默认的配置文件
void VillagerEditor::createDefaultItemConfig(const QString &path)
{
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);  // 写入时 (out)
        out << "# Minecraft 村民交易物品配置文件\n";
        out << "# 格式：分类, 英文ID, 中文名, 默认Damage值\n";
        out << "# 以 # 开头的行是注释，不会被读取\n\n";

        out << "基础, minecraft:air, 空气, 0\n";
        out << "矿物, minecraft:emerald, 绿宝石, 0\n";
        out << "矿物, minecraft:diamond, 钻石, 0\n";
        out << "矿物, minecraft:iron_ingot, 铁锭, 0\n";
        out << "矿物, minecraft:gold_ingot, 金锭, 0\n";
        out << "武器, minecraft:iron_sword, 铁剑, 32767\n";
        out << "武器, minecraft:diamond_sword, 钻石剑, 32767\n";
        out << "食物, minecraft:bread, 面包, 0\n";
        out << "食物, minecraft:apple, 苹果, 0\n";
        out << "方块, minecraft:chest, 箱子, 0\n";
        file.close();
    }
}

// 2. 从文件读取物品映射列表
QList<ItemMapping> VillagerEditor::loadItemMappings()
{
    QList<ItemMapping> items;
    // 配置文件存放在可执行文件同级目录
    QString path = QCoreApplication::applicationDirPath() + "/items_config.csv";
    QFile file(path);

    // 如果文件不存在，则自动创建一个默认的
    if (!file.exists()) {
        createDefaultItemConfig(path);
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);   // 读取时 (in)
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            // 跳过空行和注释
            if (line.isEmpty() || line.startsWith("#")) continue;

            QStringList parts = line.split(",");
            if (parts.size() >= 4) {
                ItemMapping mapping;
                mapping.category = parts[0].trimmed();
                mapping.englishId = parts[1].trimmed();
                mapping.chineseName = parts[2].trimmed();
                mapping.defaultDamage = parts[3].trimmed().toInt();
                items.append(mapping);
            }
        }
        file.close();
    }
    return items;
}

// 3. 核心功能：内置的配置文件文本编辑器
void VillagerEditor::openItemConfigEditor()
{
    QDialog dialog(this);
    dialog.setWindowTitle("⚙️ 编辑物品库配置 (items_config.csv)");
    dialog.resize(600, 500);
    QVBoxLayout layout(&dialog);

    QLabel *helpLabel = new QLabel("<b>配置格式：</b> 分类, 英文ID, 中文名, Damage<br>"
                                   "<b>示例：</b> <code>矿物, minecraft:emerald, 绿宝石, 0</code><br>"
                                   "<font color='gray'>修改后点击保存即可全局生效。你可以随时添加 Mod 物品。</font>", &dialog);
    layout.addWidget(helpLabel);

    // 文本编辑区
    QTextEdit *editor = new QTextEdit(&dialog);
    editor->setStyleSheet("font-family: Consolas, monospace; font-size: 14px;");

    QString path = QCoreApplication::applicationDirPath() + "/items_config.csv";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        editor->setPlainText(QString::fromUtf8(file.readAll()));
        file.close();
    }
    layout.addWidget(editor);

    // 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnSave = new QPushButton("保存并刷新", &dialog);
    QPushButton *btnCancel = new QPushButton("取消", &dialog);
    btnLayout->addStretch();
    btnLayout->addWidget(btnSave);
    btnLayout->addWidget(btnCancel);
    layout.addLayout(btnLayout);

    // 保存逻辑
    connect(btnSave, &QPushButton::clicked, [&]() {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);  // 写入时 (out)
            out << editor->toPlainText();
            file.close();

            // 保存后立刻刷新自动补全列表
            updateCompleters();

            dialog.accept();
            QMessageBox::information(this, "成功", "物品库配置已更新并生效！");
        } else {
            QMessageBox::warning(this, "错误", "无法保存文件，请检查权限！");
        }
    });
    connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    dialog.exec();
}

bool VillagerEditor::validateCustomNodes() const
{
    for (const TradeOption &trade : m_tradeOptions) {
        // 检查三个物品的自定义节点
        const ItemData* items[3] = { &trade.buyA, &trade.buyB, &trade.sell };
        for (const ItemData* item : items) {
            if (item->enableCustom) {
                // 如果启用了自定义节点，但 customNodes 为空（解析失败或内容为空），则视为无效
                if (item->customNodes.isEmpty()) {
                    return false;
                }
                // 可选：进一步检查每个节点是否包含必要的字段，但通常 JSON 解析时已保证结构
            }
        }
    }
    return true;
}
